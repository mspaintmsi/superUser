// Windows Vista, the earliest to utilize the Trusted Installer
#define _WIN32_WINNT _WIN32_WINNT_VISTA

#include <windows.h>
#include <wtsapi32.h>
#include <stdio.h>
#ifdef _MSC_VER
#include "msvc/msvcrt.h"
#endif

#include "tokens.h" // Defines lplpwcszTokenPrivileges

/*
	Return codes (without /r option):
		1 - Invalid argument
		2 - Failed acquiring SeDebugPrivilege
		3 - Could not open/start the TrustedInstaller service
		4 - Process creation failed
		5 - Another fatal error occurred

	If /r option is specified, exit code of the child process is returned.
	If superUser fails, it returns the code -(EXIT_CODE_BASE + errCode),
	where errCode is one of the codes listed above.
*/

#define EXIT_CODE_BASE 1000000
#define wputs _putws
#define wprintfv(...) \
if (params.bVerbose) wprintf(__VA_ARGS__); // Only use when bVerbose in scope

struct parameters {
	unsigned int bReturnCode : 1;     // Whether to return process exit code
	unsigned int bSeamless : 1;       // Whether child process shares parent's console
	unsigned int bVerbose : 1;        // Whether to print debug messages or not
	unsigned int bWait : 1;           // Whether to wait to finish created process
};

static struct parameters params = {0};
static int nChildExitCode = 0;


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


static void setAllPrivileges( HANDLE hProcessToken, BOOL bSilent )
{
	// Iterate over lplpwcszTokenPrivileges to add all privileges to a token
	for (int i = 0; i < (sizeof( lplpcwszTokenPrivileges ) /
		sizeof( *lplpcwszTokenPrivileges )); ++i)
		if (! enableTokenPrivilege( hProcessToken, lplpcwszTokenPrivileges[ i ] ) &&
			! bSilent)
			wprintfv( L"[D] Could not set privilege [%ls], you most likely don't have it.\n", lplpcwszTokenPrivileges[ i ] );
}


static int acquireSeDebugPrivilege( void )
{
	HANDLE hThreadToken;
	int retry = 1;

reacquire_token:
	OpenThreadToken( GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE,
		&hThreadToken );
	if (GetLastError() == ERROR_NO_TOKEN && retry) {
		ImpersonateSelf( SecurityImpersonation );
		retry--;

		goto reacquire_token;
	}

	if (! enableTokenPrivilege( hThreadToken, SE_DEBUG_NAME )) {
		fwprintf( stderr, L"[E] Acquiring SeDebugPrivilege failed" );
		return 2;
	}

	return 0;
}


static int createSystemContext( void )
{
	DWORD dwSysPid = (DWORD) -1;
	PWTS_PROCESS_INFOW pProcList = NULL;
	DWORD dwProcCount = 0;

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


static int getTrustedInstallerToken( PHANDLE phToken )
{
	HANDLE hSCManager, hTIService;
	SERVICE_STATUS_PROCESS lpServiceStatusBuffer = {0};

	hSCManager = OpenSCManager( NULL, NULL,
		SC_MANAGER_CREATE_SERVICE | SC_MANAGER_CONNECT );
	hTIService = OpenService( hSCManager, L"TrustedInstaller",
		SERVICE_START | SERVICE_QUERY_STATUS );

	if (hTIService == NULL)
		goto cleanup_and_fail;

	do {
		unsigned long ulBytesNeeded;
		QueryServiceStatusEx( hTIService, SC_STATUS_PROCESS_INFO,
			(unsigned char*) &lpServiceStatusBuffer, sizeof( SERVICE_STATUS_PROCESS ),
			&ulBytesNeeded );

		if (lpServiceStatusBuffer.dwCurrentState == SERVICE_STOPPED)
			if (! StartService( hTIService, 0, NULL ))
				goto cleanup_and_fail;

	}
	while (lpServiceStatusBuffer.dwCurrentState == SERVICE_STOPPED);

	CloseServiceHandle( hSCManager );
	CloseServiceHandle( hTIService );

	*phToken = NULL;
	HANDLE hTIPHandle = OpenProcess(
		MAXIMUM_ALLOWED, FALSE, lpServiceStatusBuffer.dwProcessId );
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

cleanup_and_fail:
	CloseServiceHandle( hSCManager );
	CloseServiceHandle( hTIService );

	fwprintf( stderr, L"[E] Could not open/start the TrustedInstaller service\n" );
	return 3;
}


static int createTrustedInstallerProcess( wchar_t* lpwszImageName )
{
	// Start the TrustedInstaller service and get the token
	HANDLE hToken = NULL;
	int errCode = getTrustedInstallerToken( &hToken );
	if (errCode) return errCode;

	// Get the console session id and set it in the token
	DWORD dwSessionID = WTSGetActiveConsoleSessionId();
	if (dwSessionID != (DWORD) -1) {
		SetTokenInformation( hToken, TokenSessionId, (PVOID) &dwSessionID,
			sizeof( DWORD ) );
	}

	// Set all privileges in the child process token
	setAllPrivileges( hToken, FALSE );


	// Initialize STARTUPINFO

	STARTUPINFO startupInfo = {0};
	startupInfo.cb = sizeof( STARTUPINFO );
	startupInfo.dwFlags = STARTF_USESHOWWINDOW;
	startupInfo.wShowWindow = SW_SHOWNORMAL;

	// Create process

	PROCESS_INFORMATION processInfo = {0};
	DWORD dwCreationFlags = 0;
	if (! params.bSeamless) dwCreationFlags |= CREATE_NEW_CONSOLE;

	wprintfv( L"[D] Creating specified process\n" );

	BOOL bCreateResult = CreateProcessAsUser(
		hToken,
		NULL,
		lpwszImageName,
		NULL,
		NULL,
		FALSE,
		dwCreationFlags,
		NULL,
		NULL,
		&startupInfo,
		&processInfo
	);
	CloseHandle( hToken );

	if (bCreateResult) {
		wprintfv( L"[D] Created process ID: %ld\n", processInfo.dwProcessId );

		if (params.bWait) {
			wprintfv( L"[D] Waiting for process to exit\n" );
			WaitForSingleObject( processInfo.hProcess, INFINITE );
			wprintfv( L"[D] Process exited\n" );

			// Get the child's exit code
			DWORD dwExitCode;
			if (GetExitCodeProcess( processInfo.hProcess, &dwExitCode )) {
				nChildExitCode = dwExitCode;
				wprintfv( L"[D] Process exit code: %ld\n", dwExitCode );
			}
		}

		CloseHandle( processInfo.hProcess );
		CloseHandle( processInfo.hThread );
	}
	else {
		// Most commonly - 0x2 - The system cannot find the file specified.
		fwprintf( stderr, L"[E] Process creation failed. Error code: 0x%08X\n",
			GetLastError() );
		return 4;
	}

	return 0;
}


static int getExitCode( int code )
{
	if (code == -1) code = 0;  // Print help, exit with code 0
	if (params.bReturnCode) {
		if (code) code = -(EXIT_CODE_BASE + code);
		else code = nChildExitCode;
	}
	return code;
}


static void printHelp( void )
{
	wputs(
		L"superUser.exe [options] [command_to_run]\n\
Options: (You can use either '-' or '/')\n\
  /h - Display this help message.\n\
  /r - Return exit code of child process. Requires /w.\n\
  /s - Child process shares parent's console. Requires /w.\n\
  /v - Display verbose messages.\n\
  /w - Wait for the created process to finish before exiting." );
}


static BOOL getArgument( BOOL bQuotedString, wchar_t** ppArgument,
	wchar_t** ppArgumentIndex )
{
	// Current pointer to the remainder of the line to be parsed.
	// Initialized with the full command line on the first call.
	static wchar_t* p = NULL;
	if (! p) p = GetCommandLine();

	// Free the previous argument (if it exists)
	if (*ppArgument) HeapFree( GetProcessHeap(), 0, *ppArgument );
	*ppArgument = NULL;

	// Search argument

	// Skip spaces
	while (*p == L' ' || *p == L'\t') p++;

	if (*p != L'\0') {
		// Argument found
		wchar_t* pBegin = p;

		// Search the end of the argument
		if (bQuotedString) {
			// Quotes are interpreted
			/*
			* This is a simplified algorithm.
			* In this application, we only need to know the size of the parameter, not its
			* content. Also, the parsing is different for an executable name (like here)
			* and for a normal parameter.
			*/
			BOOL bQuote = FALSE;
			while (*p != L'\0') {
				if (*p == L'"') bQuote = ! bQuote;
				else if (! bQuote && (*p == L' ' || *p == L'\t')) break;
				p++;
			}
		}
		else {
			// Quotes are not interpreted
			while (*p != L' ' && *p != L'\t' && *p != L'\0') p++;
		}

		size_t nArgSize = p - pBegin;
		*ppArgument = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, (nArgSize + 1) * 2 );
		memcpy( *ppArgument, pBegin, nArgSize * 2 );
		*ppArgumentIndex = pBegin;
		return TRUE;
	}

	// Argument not found
	return FALSE;
}


int wmain( int argc, wchar_t* argv[] )
{
	int errCode = 0;  // superUser error code

	// Command to run (name of process to create followed by parameters) -
	// basically the first non-option argument or "cmd.exe".
	wchar_t* lpwszCommandLine = NULL;

	// Parse command line

	wchar_t* lpwszArgument = NULL;  // Command line argument 
	wchar_t* lpwszArgumentIndex = NULL;  // Pointer to argument in command line

	// Skip program name (argv[0])
	getArgument( TRUE, &lpwszArgument, &lpwszArgumentIndex );

	// Parse command line options

	while (getArgument( FALSE, &lpwszArgument, &lpwszArgumentIndex )) {
		// Check for an at-least-two-character string beginning with '/' or '-'
		if (*lpwszArgument == L'/' || *lpwszArgument == L'-') {
			if (lpwszArgument[ 1 ] != L'\0') {
				int j = 1;
				wchar_t opt;
				while ((opt = lpwszArgument[ j ]) != L'\0') {
					// Multiple options can be grouped together (eg: /wrs)
					switch (opt) {
					case 'h':
						printHelp();
						errCode = -1;
						goto done_params;
					case 'r':
						params.bReturnCode = 1;
						break;
					case 's':
						params.bSeamless = 1;
						break;
					case 'v':
						params.bVerbose = 1;
						break;
					case 'w':
						params.bWait = 1;
						break;
					case 'c':
						/*
						This option is no longer useful. Do not use it.
						It is kept only for compatibility with previous versions.
						*/
						lpwszCommandLine = lpwszArgumentIndex + (j + 1);
						while (*lpwszCommandLine == L' ' || *lpwszCommandLine == L'\t')
							lpwszCommandLine++;
						goto done_params;
					default:
						fwprintf( stderr, L"[E] Invalid option\n" );
						errCode = 1;
						goto done_params;
					}
					j++;
				}
			}
			else {
				fwprintf( stderr, L"[E] Invalid argument\n" );
				errCode = 1;
				goto done_params;
			}
		}
		else {
			// Command parameter found
			lpwszCommandLine = lpwszArgumentIndex;
			break;
		}
	}
done_params:
	// Free the last argument (if it exists)
	if (lpwszArgument) HeapFree( GetProcessHeap(), 0, lpwszArgument );

	if (errCode) return getExitCode( errCode );

	// Check the consistency of the options
	if ((params.bReturnCode || params.bSeamless) && ! params.bWait) {
		fwprintf( stderr, L"[E] /r or /s option requires /w\n" );
		return getExitCode( 1 );
	}

	if (! lpwszCommandLine) lpwszCommandLine = L"cmd.exe";

	wprintfv( L"[D] Your commandline is \"%ls\"\n", lpwszCommandLine );

	// lpwszCommandLine may be read-only. It must be copied to a writable area.
	size_t nCommandLineBufSize = (wcslen( lpwszCommandLine ) + 1) * sizeof( wchar_t );
	wchar_t* lpwszImageName = HeapAlloc( GetProcessHeap(), 0, nCommandLineBufSize );
	memcpy( lpwszImageName, lpwszCommandLine, nCommandLineBufSize );

	errCode = acquireSeDebugPrivilege();
	if (! errCode) errCode = createSystemContext();
	if (! errCode) errCode = createTrustedInstallerProcess( lpwszImageName );

	HeapFree( GetProcessHeap(), 0, lpwszImageName );

	return getExitCode( errCode );
}
