/*
	https://github.com/mspaintmsi/superUser

	tokens.c

	Tokens and privileges management functions

*/

#include <windows.h>
#include <wtsapi32.h>
#include <stdio.h>

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


static int enableTokenPrivilege( HANDLE hToken, const wchar_t* pcwszPrivilege )
{
	LUID luid;
	if (! LookupPrivilegeValue( NULL, pcwszPrivilege, &luid ))
		return 0; // Cannot lookup privilege value

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[ 0 ].Luid = luid;
	tp.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges( hToken, FALSE, &tp, 0, NULL, NULL );
	if (GetLastError() != ERROR_SUCCESS) return 0;

	return 1;
}


void setAllPrivileges( HANDLE hProcessToken, BOOL bVerbose )
{
	// Iterate over apcwszTokenPrivileges to add all privileges to a token
	for (int i = 0; i < (sizeof( apcwszTokenPrivileges ) /
		sizeof( *apcwszTokenPrivileges )); i++)
		if (! enableTokenPrivilege( hProcessToken, apcwszTokenPrivileges[ i ] ) &&
			bVerbose)
			wprintf( L"[D] Could not set privilege [%ls], you most likely don't have it.\n",
				apcwszTokenPrivileges[ i ] );
}


int acquireSeDebugPrivilege( void )
{
	HANDLE hThreadToken = NULL;

	int retry = 1;
	while (! OpenThreadToken( GetCurrentThread(),
		TOKEN_ADJUST_PRIVILEGES, FALSE, &hThreadToken ) &&
		GetLastError() == ERROR_NO_TOKEN && retry)
	{
		ImpersonateSelf( SecurityImpersonation );
		retry = 0;
	}

	BOOL bSuccess = FALSE;
	if (hThreadToken) {
		bSuccess = enableTokenPrivilege( hThreadToken, SE_DEBUG_NAME );
		CloseHandle( hThreadToken );
	}
	if (! bSuccess) {
		fwprintf( stderr, L"[E] Acquiring SeDebugPrivilege failed\n" );
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
		PWTS_PROCESS_INFOW pProc = pProcList;
		while (dwProcCount > 0) {
			if (! pProc->SessionId && pProc->pProcessName &&
				! _wcsicmp( L"services.exe", pProc->pProcessName ) &&
				pProc->pUserSid &&
				IsWellKnownSid( pProc->pUserSid, WinLocalSystemSid )) {
				dwSysPid = pProc->ProcessId;
				break;
			}
			pProc++;
			dwProcCount--;
		}
		WTSFreeMemory( pProcList );
	}

	HANDLE hToken = NULL;

	if (dwSysPid != (DWORD) -1) {
		HANDLE hSysProcess = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
			dwSysPid );
		if (hSysProcess) {
			// Get the process token
			HANDLE hSysToken = NULL;
			if (OpenProcessToken( hSysProcess, TOKEN_DUPLICATE, &hSysToken )) {
				if (! DuplicateTokenEx( hSysToken,
					TOKEN_ADJUST_PRIVILEGES | TOKEN_IMPERSONATE, NULL,
					SecurityImpersonation, TokenImpersonation, &hToken )) hToken = NULL;
				CloseHandle( hSysToken );
			}
			CloseHandle( hSysProcess );
		}
	}

	BOOL bSuccess = FALSE;
	if (hToken) {
		bSuccess =
			enableTokenPrivilege( hToken, SE_ASSIGNPRIMARYTOKEN_NAME ) &&
			SetThreadToken( NULL, hToken );
		CloseHandle( hToken );
	}
	if (! bSuccess) {
		fwprintf( stderr, L"[E] Failed to create system context\n" );
		return 5;
	}

	return 0;
}


int getTrustedInstallerProcess( DWORD* pdwTIProcessId, HANDLE* phProcess )
{
	HANDLE hSCManager, hTIService;
	SERVICE_STATUS_PROCESS serviceStatusBuffer = {0};

	hSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT );
	hTIService = OpenService( hSCManager, L"TrustedInstaller",
		SERVICE_QUERY_STATUS | SERVICE_START );

	// Start the TrustedInstaller service
	BOOL bStopped = TRUE;
	if (hTIService) {
		int retry = 1;
		DWORD dwBytesNeeded;
		while (
			QueryServiceStatusEx( hTIService, SC_STATUS_PROCESS_INFO,
				(LPBYTE) &serviceStatusBuffer, sizeof( SERVICE_STATUS_PROCESS ),
				&dwBytesNeeded ) &&
			(bStopped = (serviceStatusBuffer.dwCurrentState == SERVICE_STOPPED)) &&
			retry &&
			StartService( hTIService, 0, NULL )
			) {
			retry = 0;
		}
	}

	CloseServiceHandle( hSCManager );
	CloseServiceHandle( hTIService );

	*phProcess = NULL;

	if (! bStopped) {
		*pdwTIProcessId = serviceStatusBuffer.dwProcessId;

		// Get the TrustedInstaller process handle
		*phProcess = OpenProcess( PROCESS_CREATE_PROCESS, FALSE, *pdwTIProcessId );
	}

	if (! *phProcess) {
		fwprintf( stderr, L"[E] Could not open/start the TrustedInstaller service\n" );
		return 3;
	}

	return 0;
}


int getTrustedInstallerToken( DWORD dwTIProcessId, HANDLE* phToken )
{
	*phToken = NULL;

	HANDLE hTIPHandle = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, dwTIProcessId );
	if (hTIPHandle) {
		// Get the TrustedInstaller process token
		HANDLE hTIToken = NULL;
		if (OpenProcessToken( hTIPHandle, TOKEN_DUPLICATE, &hTIToken )) {
			if (! DuplicateTokenEx( hTIToken,
				TOKEN_ADJUST_DEFAULT | TOKEN_ADJUST_PRIVILEGES | TOKEN_ADJUST_SESSIONID |
				TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY,
				NULL,
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
