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


//
// Allocate a block of memory from the process heap.
//
__declspec(noinline) LPVOID allocHeap( DWORD dwFlags, SIZE_T dwBytes )
{
	return HeapAlloc( GetProcessHeap(), dwFlags, dwBytes );
}


//
// Free a block of memory allocated from the process heap.
//
__declspec(noinline) BOOL freeHeap( LPVOID lpMem )
{
	return HeapFree( GetProcessHeap(), 0, lpMem );
}


//
// Print a formatted string with a list of variable arguments to a stream
// using the current console output code page.
//
static void v_printConsoleStream( FILE* stream, const wchar_t* pwszFormat,
	va_list arg_list )
{
	// Write the formatted string to a buffer (wide chars)
	SIZE_T nSize1 = _vscwprintf( pwszFormat, arg_list ) + 1;
	if (nSize1 == 0) return;
	wchar_t* pBuffer1 = allocHeap( 0, nSize1 * sizeof( wchar_t ) );
	_vsnwprintf_s( pBuffer1, nSize1, _TRUNCATE, pwszFormat, arg_list );

	// Convert buffer to console output code page (bytes)
	UINT nCodePage = GetConsoleOutputCP();
	int nSize2 = WideCharToMultiByte( nCodePage, 0, pBuffer1, -1, NULL, 0, NULL, NULL );
	if (nSize2 > 0) {
		char* pBuffer2 = allocHeap( 0, nSize2 );
		if (WideCharToMultiByte( nCodePage, 0, pBuffer1, -1, pBuffer2, nSize2, NULL, NULL ))
		{
			// Write to the console stream
			fputs( pBuffer2, stream );
		}
		freeHeap( pBuffer2 );
	}

	freeHeap( pBuffer1 );
}


//
// Print a formatted string with variable arguments to a stream
// using the current console output code page.
//
static void printConsoleStream( FILE* stream, const wchar_t* pwszFormat, ... )
{
	va_list args;
	va_start( args, pwszFormat );
	v_printConsoleStream( stream, pwszFormat, args );
	va_end( args );
}


//
// Print a formatted string with variable arguments to standard output.
//
void printConsole( const wchar_t* pwszFormat, ... )
{
	va_list args;
	va_start( args, pwszFormat );
	v_printConsoleStream( stdout, pwszFormat, args );
	va_end( args );
}


//
// Print an error message to standard error output
//
void printError( const wchar_t* pwszMessage, DWORD dwCode, int iPosition )
{
	wchar_t pwszFormat[] = L"[E] %ls (code: 0x%08lX, pos: %d)\n";
	wchar_t* pEnd = NULL;
	if (dwCode == 0) {
		// Remove the error code/position part from the format
		pEnd = pwszFormat + 7;
	}
	else if (iPosition == 0) {
		// Remove the error position part from the format
		pEnd = pwszFormat + 22;
		*pEnd++ = L')';
	}
	if (pEnd) {
		*pEnd++ = L'\n';
		*pEnd = L'\0';
	}
	printConsoleStream( stderr, pwszFormat, pwszMessage, dwCode, iPosition );
}


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


void setAllPrivileges( HANDLE hToken, BOOL bVerbose )
{
	// Iterate over apcwszTokenPrivileges to add all privileges to a token
	for (int i = 0; i < (sizeof( apcwszTokenPrivileges ) /
		sizeof( *apcwszTokenPrivileges )); i++)
		if (! enableTokenPrivilege( hToken, apcwszTokenPrivileges[ i ] ) && bVerbose)
			printConsole(
				L"[D] Could not set privilege [%ls], you most likely don't have it.\n",
				apcwszTokenPrivileges[ i ] );
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
		printError( L"Failed to acquire SeDebugPrivilege", dwLastError, iStep );
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
	else dwLastError = 0xA0001000; // Process not found, custom error code

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
		printError( L"Failed to create system context", dwLastError, iStep );
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

	if (bStopped) dwLastError = GetLastError();

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
		printError( L"Failed to open TrustedInstaller process", dwLastError, iStep );
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
		printError( L"Failed to create child process token", dwLastError, iStep );
		return 5;
	}

	return 0;
}
