/*
	https://github.com/mspaintmsi/superUser

	tokens.c

	Tokens and privileges management functions

*/

#include <windows.h>
#include <stdio.h>
#include <wtsapi32.h>

#include "common.h" // Defines global variables (param) and macros
#include "tokens.h"

const wchar_t* apcwszTokenPrivileges[ 36 ] = {
	SE_ASSIGNPRIMARYTOKEN_NAME,
	SE_AUDIT_NAME,
	SE_BACKUP_NAME,
	SE_CHANGE_NOTIFY_NAME,
	SE_CREATE_GLOBAL_NAME,
	SE_CREATE_PAGEFILE_NAME,
	SE_CREATE_PERMANENT_NAME,
	SE_CREATE_SYMBOLIC_LINK_NAME,
	SE_CREATE_TOKEN_NAME, // Most users won't have that.
	SE_DEBUG_NAME,
	SE_DELEGATE_SESSION_USER_IMPERSONATE_NAME,
	SE_ENABLE_DELEGATION_NAME, // Most users won't have that.
	SE_IMPERSONATE_NAME,
	SE_INC_BASE_PRIORITY_NAME,
	SE_INC_WORKING_SET_NAME,
	SE_INCREASE_QUOTA_NAME,
	SE_LOAD_DRIVER_NAME,
	SE_LOCK_MEMORY_NAME,
	SE_MACHINE_ACCOUNT_NAME, // Most users won't have that.
	SE_MANAGE_VOLUME_NAME,
	SE_PROF_SINGLE_PROCESS_NAME,
	SE_RELABEL_NAME, // Most users won't have that.
	SE_REMOTE_SHUTDOWN_NAME, // Most users won't have that.
	SE_RESTORE_NAME,
	SE_SECURITY_NAME,
	SE_SHUTDOWN_NAME,
	SE_SYNC_AGENT_NAME, // Most users won't have that.
	SE_SYSTEM_ENVIRONMENT_NAME,
	SE_SYSTEM_PROFILE_NAME,
	SE_SYSTEMTIME_NAME,
	SE_TAKE_OWNERSHIP_NAME,
	SE_TCB_NAME,
	SE_TIME_ZONE_NAME,
	SE_TRUSTED_CREDMAN_ACCESS_NAME, // Most users won't have that.
	SE_UNDOCK_NAME,
	SE_UNSOLICITED_INPUT_NAME // Most users won't have that.
};


static int enableTokenPrivilege(
	HANDLE hToken,
	const wchar_t* lpwcszPrivilege
) {
	TOKEN_PRIVILEGES tp;
	LUID luid;
	TOKEN_PRIVILEGES prevTp;
	DWORD cbPrevious = sizeof( TOKEN_PRIVILEGES );

	if (! LookupPrivilegeValue( NULL, lpwcszPrivilege, &luid ))
		return 0; // Cannot lookup privilege value

	tp.PrivilegeCount = 1;
	tp.Privileges[ 0 ].Luid = luid;
	tp.Privileges[ 0 ].Attributes = 0;

	AdjustTokenPrivileges( hToken, FALSE, &tp, sizeof( TOKEN_PRIVILEGES ), &prevTp,
		&cbPrevious );

	if (GetLastError() != ERROR_SUCCESS)
		return 0;

	prevTp.PrivilegeCount = 1;
	prevTp.Privileges[ 0 ].Luid = luid;

	prevTp.Privileges[ 0 ].Attributes |= SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges( hToken, FALSE, &prevTp, cbPrevious, NULL, NULL );

	if (GetLastError() != ERROR_SUCCESS)
		return 0;

	return 1;
}


void setAllPrivileges( HANDLE hProcessToken, BOOL bSilent )
{
	// Iterate over lplpwcszTokenPrivileges to add all privileges to a token
	for (int i = 0; i < (sizeof( apcwszTokenPrivileges ) /
		sizeof( *apcwszTokenPrivileges )); i++)
		if (! enableTokenPrivilege( hProcessToken, apcwszTokenPrivileges[ i ] ) &&
			! bSilent)
			wprintfv( L"[D] Could not set privilege [%ls], you most likely don't have it.\n",
				apcwszTokenPrivileges[ i ] );
}


int acquireSeDebugPrivilege( void )
{
	HANDLE hThreadToken = NULL;
	int retry = 1;

	do {
		if (! OpenThreadToken( GetCurrentThread(),
			TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hThreadToken ) &&
			GetLastError() == ERROR_NO_TOKEN && retry)
		{
			ImpersonateSelf( SecurityImpersonation );
			retry--;
		}
		else break;
	}
	while (TRUE);

	if (! enableTokenPrivilege( hThreadToken, SE_DEBUG_NAME )) {
		fwprintf( stderr, L"[E] Acquiring SeDebugPrivilege failed" );
		return 2;
	}

	return 0;
}


int createSystemContext( void )
{
	DWORD dwSysPid = (DWORD) -1;
	PWTS_PROCESS_INFOW pProcList = NULL;
	DWORD dwProcCount = 0;

	// Get the process id
	if (WTSEnumerateProcessesW( WTS_CURRENT_SERVER_HANDLE, 0, 1,
		&pProcList, &dwProcCount )) {
		for (DWORD i = 0; i < dwProcCount; i++) {
			PWTS_PROCESS_INFOW pProc = &pProcList[ i ];
			if (! pProc->SessionId && pProc->pProcessName &&
				(! _wcsicmp( L"lsass.exe", pProc->pProcessName )) &&
				pProc->pUserSid &&
				IsWellKnownSid( pProc->pUserSid, WinLocalSystemSid )) {
				dwSysPid = pProc->ProcessId;
				break;
			}
		}
		WTSFreeMemory( pProcList );
	}

	HANDLE hToken = NULL;

	if (dwSysPid != (DWORD) -1) {
		HANDLE hSysProcess = OpenProcess( MAXIMUM_ALLOWED, FALSE, dwSysPid );
		if (hSysProcess) {
			// Get the process token
			HANDLE hSysToken = NULL;
			if (OpenProcessToken( hSysProcess, MAXIMUM_ALLOWED, &hSysToken )) {
				if (! DuplicateTokenEx( hSysToken, MAXIMUM_ALLOWED, NULL,
					SecurityImpersonation, TokenImpersonation, &hToken )) hToken = NULL;
				CloseHandle( hSysToken );
			}
			CloseHandle( hSysProcess );
		}
	}
	if (! hToken) {
		fwprintf( stderr, L"[E] Failed to create system context" );
		return 5;
	}
	setAllPrivileges( hToken, TRUE );
	SetThreadToken( NULL, hToken );
	CloseHandle( hToken );

	return 0;
}


int getTrustedInstallerToken( PHANDLE phToken )
{
	BOOL bFailed = FALSE;
	HANDLE hSCManager, hTIService;
	SERVICE_STATUS_PROCESS serviceStatusBuffer = {0};

	hSCManager = OpenSCManager( NULL, NULL,
		SC_MANAGER_CREATE_SERVICE | SC_MANAGER_CONNECT );
	hTIService = OpenService( hSCManager, L"TrustedInstaller",
		SERVICE_START | SERVICE_QUERY_STATUS );
	if (hTIService) {
		do {
			DWORD dwBytesNeeded;
			QueryServiceStatusEx( hTIService, SC_STATUS_PROCESS_INFO,
				(LPBYTE) &serviceStatusBuffer, sizeof( SERVICE_STATUS_PROCESS ),
				&dwBytesNeeded );

			if (serviceStatusBuffer.dwCurrentState == SERVICE_STOPPED)
				if (! StartService( hTIService, 0, NULL )) { bFailed = TRUE; break; }
		}
		while (serviceStatusBuffer.dwCurrentState == SERVICE_STOPPED);
	}
	else bFailed = TRUE;

	CloseServiceHandle( hSCManager );
	CloseServiceHandle( hTIService );

	if (bFailed) {
		fwprintf( stderr, L"[E] Could not open/start the TrustedInstaller service\n" );
		return 3;
	}

	*phToken = NULL;
	HANDLE hTIPHandle = OpenProcess( MAXIMUM_ALLOWED, FALSE,
		serviceStatusBuffer.dwProcessId );
	if (hTIPHandle) {
		// Get the TrustedInstaller process token
		HANDLE hTIToken = NULL;
		if (OpenProcessToken( hTIPHandle, MAXIMUM_ALLOWED, &hTIToken )) {
			if (! DuplicateTokenEx( hTIToken, MAXIMUM_ALLOWED, NULL,
				SecurityIdentification, TokenPrimary, phToken )) *phToken = NULL;
			CloseHandle( hTIToken );
		}
		CloseHandle( hTIPHandle );
	}

	if (! *phToken) {
		fwprintf( stderr, L"[E] Failed to create TrustedInstaller token\n" );
		return 5;
	}
	return 0;
}
