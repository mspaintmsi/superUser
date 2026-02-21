/*
	superUser 6.2

	Copyright 2019-2026 https://github.com/mspaintmsi/superUser

	tokens.c

	Tokens and privileges management functions

*/

#include "tokens.h"

#define CUSTOM_ERROR_PROCESS_NOT_FOUND 0xA0001000
#define CUSTOM_ERROR_SERVICE_START_FAILED 0xA0001001

#include <wchar.h>
#include <windows.h>
#include <wtsapi32.h>
#ifdef __GNUC__
#include "winnt2.h"
#endif

#include "output.h" // Display functions

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


static BOOL enableTokenPrivilege( HANDLE hToken, const wchar_t* pcwszPrivilege )
{
	LUID luid;
	if (! LookupPrivilegeValue( NULL, pcwszPrivilege, &luid ))
		return FALSE; // Cannot lookup privilege value

	TOKEN_PRIVILEGES tp = {
		.PrivilegeCount = 1,
		.Privileges[ 0 ].Luid = luid,
		.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED
	};

	AdjustTokenPrivileges( hToken, FALSE, &tp, 0, NULL, NULL );
	return (GetLastError() == ERROR_SUCCESS);
}


void setAllPrivileges( HANDLE hToken, MissingPrivilegeFunc fnMPCb )
{
	// Iterate over apcwszTokenPrivileges to add all privileges to a token
	for (int i = 0; i < (sizeof( apcwszTokenPrivileges ) /
		sizeof( *apcwszTokenPrivileges )); i++)
		if (! enableTokenPrivilege( hToken, apcwszTokenPrivileges[ i ] ) && fnMPCb)
			fnMPCb( apcwszTokenPrivileges[ i ] );
}


int acquireSeDebugPrivilege( void )
{
	DWORD dwLastError = 0;
	int iStep = 1;

	BOOL bSuccess = FALSE;
	HANDLE hToken = NULL;
	if (OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken )) {
		iStep++;
		bSuccess = enableTokenPrivilege( hToken, SE_DEBUG_NAME );
		if (! bSuccess) dwLastError = GetLastError();
		CloseHandle( hToken );
	}
	else dwLastError = GetLastError();

	if (! bSuccess) {
		showError( L"Failed to acquire SeDebugPrivilege", dwLastError, iStep );
		return 2;
	}

	return 0;
}


int createSystemContext( void )
{
	DWORD dwLastError = 0;
	int iStep = 1;

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
	else dwLastError = GetLastError();

	HANDLE hToken = NULL;

	if (dwSysPid != (DWORD) -1) {
		iStep++;
		HANDLE hSysProcess = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
			dwSysPid );
		if (hSysProcess) {
			iStep++;
			// Get the process token
			HANDLE hSysToken = NULL;
			if (OpenProcessToken( hSysProcess, TOKEN_DUPLICATE, &hSysToken )) {
				iStep++;
				if (! DuplicateTokenEx( hSysToken,
					TOKEN_ADJUST_PRIVILEGES | TOKEN_IMPERSONATE, NULL,
					SecurityImpersonation, TokenImpersonation, &hToken )) {
					dwLastError = GetLastError();
					hToken = NULL;
				}
				CloseHandle( hSysToken );
			}
			else dwLastError = GetLastError();
			CloseHandle( hSysProcess );
		}
		else dwLastError = GetLastError();
	}
	else dwLastError = CUSTOM_ERROR_PROCESS_NOT_FOUND; // Process not found

	BOOL bSuccess = FALSE;
	if (hToken) {
		iStep++;
		if (enableTokenPrivilege( hToken, SE_ASSIGNPRIMARYTOKEN_NAME )) {
			iStep++;
			bSuccess = SetThreadToken( NULL, hToken );
		}
		if (! bSuccess) dwLastError = GetLastError();
		CloseHandle( hToken );
	}

	if (! bSuccess) {
		showError( L"Failed to create system context", dwLastError, iStep );
		return 5;
	}

	return 0;
}


int getTrustedInstallerProcess( HANDLE* phTIProcess )
{
	DWORD dwLastError = 0;
	int iStep = 1;
	HANDLE hSCManager, hTIService;
	SERVICE_STATUS_PROCESS serviceStatusBuffer = {0};

	SetLastError( 0 );

	hSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_CONNECT );
	hTIService = OpenService( hSCManager, L"TrustedInstaller",
		SERVICE_QUERY_STATUS | SERVICE_START );

	// Start the TrustedInstaller service
	BOOL bStopped = TRUE;
	if (hTIService) {
		iStep++;
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

	if (bStopped) {
		dwLastError = GetLastError();
		if (dwLastError == 0) dwLastError = CUSTOM_ERROR_SERVICE_START_FAILED;
	}

	CloseServiceHandle( hSCManager );
	CloseServiceHandle( hTIService );

	*phTIProcess = NULL;

	if (! bStopped) {
		iStep++;
		// Get the TrustedInstaller process handle
		*phTIProcess = OpenProcess( PROCESS_CREATE_PROCESS | PROCESS_QUERY_INFORMATION,
			FALSE, serviceStatusBuffer.dwProcessId );
		if (! *phTIProcess) dwLastError = GetLastError();
	}

	if (! *phTIProcess) {
		showError( L"Failed to open TrustedInstaller process", dwLastError, iStep );
		return 3;
	}

	return 0;
}


int createChildProcessToken( HANDLE hBaseProcess, HANDLE* phNewToken )
{
	DWORD dwLastError = 0;
	int iStep = 1;
	*phNewToken = NULL;

	// Get the base process token
	HANDLE hBaseToken = NULL;
	if (OpenProcessToken( hBaseProcess, TOKEN_DUPLICATE, &hBaseToken )) {
		iStep++;
		if (! DuplicateTokenEx( hBaseToken,
			TOKEN_ADJUST_DEFAULT | TOKEN_ADJUST_PRIVILEGES | TOKEN_ADJUST_SESSIONID |
			TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY,
			NULL,
			SecurityIdentification, TokenPrimary, phNewToken )) {
			dwLastError = GetLastError();
			*phNewToken = NULL;
		}
		CloseHandle( hBaseToken );
	}
	else dwLastError = GetLastError();

	if (! *phNewToken) {
		showError( L"Failed to create child process token", dwLastError, iStep );
		return 5;
	}

	return 0;
}
