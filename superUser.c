// Windows Vista, the earliest to utilize the Trusted Installer
#define _WIN32_WINNT _WIN32_WINNT_VISTA

#include <windows.h>
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

	If /r option is specified, exit code of the child process is returned.
	If superUser fails, it returns the code -(EXIT_CODE_BASE + errCode),
	where errCode is one of the codes listed above.
*/

#define wputs _putws
#define wprintfv(...) \
if (params.bVerbose) wprintf(__VA_ARGS__); // Only use when bVerbose in scope
#define EXIT_CODE_BASE 1000000

struct parameters {
	unsigned int bCommandPresent : 1; // Whether there is a user-specified command ("/c" argument)
	unsigned int bVerbose : 1;        // Whether to print debug messages or not
	unsigned int bWait : 1;           // Whether to wait to finish created process
	unsigned int bReturnCode : 1;     // Whether to return process exit code
	unsigned int bSeamless : 1;       // Whether child process shares parent's console
};

struct parameters params = {0};
int nChildExitCode = 0;

static inline int enableTokenPrivilege(
	HANDLE hToken,
	const wchar_t* lpwcszPrivilege
) {
	TOKEN_PRIVILEGES tp;
	LUID luid;
	TOKEN_PRIVILEGES prevTp;
	DWORD cbPrevious = sizeof( TOKEN_PRIVILEGES );

	if (!LookupPrivilegeValue( NULL, lpwcszPrivilege, &luid ))
		return 0; // Cannot lookup privilege value

	tp.PrivilegeCount = 1;
	tp.Privileges[ 0 ].Luid = luid;
	tp.Privileges[ 0 ].Attributes = 0;

	AdjustTokenPrivileges( hToken, FALSE, &tp, sizeof( TOKEN_PRIVILEGES ), &prevTp, &cbPrevious );

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


static inline int acquireSeDebugPrivilege( void )
{
	HANDLE hThreadToken;
	int retry = 1;

reacquire_token:
	OpenThreadToken( GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hThreadToken );
	if (GetLastError() == ERROR_NO_TOKEN && retry) {
		ImpersonateSelf( SecurityImpersonation );
		retry--;

		goto reacquire_token;
	}

	if (!enableTokenPrivilege( hThreadToken, SE_DEBUG_NAME )) {
		fwprintf( stderr, L"Acquiring SeDebugPrivilege failed!" );
		return 2;
	}

	return 0;
}


static inline void setAllPrivileges( HANDLE hProcessToken )
{
	// Iterate over lplpwcszTokenPrivileges to add all privileges to a token
	for (int i = 0; i < (sizeof( lplpcwszTokenPrivileges ) / sizeof( *lplpcwszTokenPrivileges )); ++i)
		if (!enableTokenPrivilege( hProcessToken, lplpcwszTokenPrivileges[ i ] ))
			wprintfv( L"[D] Could not set privilege [%ls], you most likely don't have it.\n", lplpcwszTokenPrivileges[ i ] );
}


static inline HANDLE getTrustedInstallerPHandle( void )
{
	HANDLE hSCManager, hTIService;
	SERVICE_STATUS_PROCESS lpServiceStatusBuffer = {0};

	hSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_CREATE_SERVICE | SC_MANAGER_CONNECT );
	hTIService = OpenService( hSCManager, L"TrustedInstaller", SERVICE_START | SERVICE_QUERY_STATUS );

	if (hTIService == NULL)
		goto cleanup_and_fail;

	do {
		unsigned long ulBytesNeeded;
		QueryServiceStatusEx( hTIService, SC_STATUS_PROCESS_INFO, (unsigned char*) &lpServiceStatusBuffer, sizeof( SERVICE_STATUS_PROCESS ), &ulBytesNeeded );

		if (lpServiceStatusBuffer.dwCurrentState == SERVICE_STOPPED)
			if (!StartService( hTIService, 0, NULL ))
				goto cleanup_and_fail;

	}
	while (lpServiceStatusBuffer.dwCurrentState == SERVICE_STOPPED);

	CloseServiceHandle( hSCManager );
	CloseServiceHandle( hTIService );

	return OpenProcess( PROCESS_CREATE_PROCESS, FALSE, lpServiceStatusBuffer.dwProcessId );

cleanup_and_fail:
	CloseServiceHandle( hSCManager );
	CloseServiceHandle( hTIService );

	return NULL;
}


static inline int createTrustedInstallerProcess( wchar_t* lpwszImageName )
{
	// Start the TrustedInstaller service
	HANDLE hTIPHandle = getTrustedInstallerPHandle();
	if (hTIPHandle == NULL) {
		fwprintf( stderr, L"[E] Could not open/start the TrustedInstaller service\n" );
		return 3;
	}

	STARTUPINFOEX startupInfo = {0};

	// Initialize STARTUPINFO

	startupInfo.StartupInfo.cb = sizeof( STARTUPINFOEX );

	startupInfo.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	startupInfo.StartupInfo.wShowWindow = SW_SHOWNORMAL;

	// Initialize attribute lists for "parent assignment"

	SIZE_T attributeListLength;

	InitializeProcThreadAttributeList( NULL, 1, 0, (PSIZE_T) &attributeListLength );

	startupInfo.lpAttributeList = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, attributeListLength );
	InitializeProcThreadAttributeList( startupInfo.lpAttributeList, 1, 0, (PSIZE_T) &attributeListLength );

	UpdateProcThreadAttribute( startupInfo.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, &hTIPHandle, sizeof( HANDLE ), NULL, NULL );

	// Create process
	PROCESS_INFORMATION processInfo = {0};
	wprintfv( L"[D] Creating specified process\n" );

	DWORD dwCreationFlags = CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT;
	if (! params.bSeamless) dwCreationFlags |= CREATE_NEW_CONSOLE;

	BOOL bCreateResult = CreateProcess(
		NULL,
		lpwszImageName,
		NULL,
		NULL,
		FALSE,
		dwCreationFlags,
		NULL,
		NULL,
		&startupInfo.StartupInfo,
		&processInfo
	);
	DeleteProcThreadAttributeList( startupInfo.lpAttributeList );
	HeapFree( GetProcessHeap(), 0, startupInfo.lpAttributeList );

	if (bCreateResult) {
		HANDLE hProcessToken;
		OpenProcessToken( processInfo.hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hProcessToken );
		setAllPrivileges( hProcessToken );

		wprintfv( L"[D] Created process ID: %ld and assigned additional token privileges.\n", processInfo.dwProcessId );

		ResumeThread( processInfo.hThread );

		if (params.bWait) {
			wprintfv( L"[D] Waiting for process to exit\n" );
			WaitForSingleObject( processInfo.hProcess, INFINITE );
			wprintfv( L"[D] Process exited\n" );

			// Return the child's exit code to the standard output if requested
			DWORD dwExitCode;
			if (GetExitCodeProcess( processInfo.hProcess, &dwExitCode )) {
				nChildExitCode = dwExitCode;
				wprintfv( L"[D] Process exit code: %ld\n", dwExitCode );
			}
		}

		CloseHandle( processInfo.hThread );
		CloseHandle( processInfo.hProcess );
	}
	else {
		// Most commonly - 0x2 - The system cannot find the file specified.
		fwprintf( stderr, L"[E] Process creation failed. Error code: 0x%08X\n", GetLastError() );
		return 4;
	}

	return 0;
}


static inline void printHelp( void )
{
	wputs(
		L"superUser.exe [options] /c [Process Name]\n\
Options: (You can use either '-' or '/')\n\
  /c - Specify command to execute. If not specified, a cmd instance is spawned.\n\
  /h - Display this help message.\n\
  /r - Return exit code of child process. Requires /w.\n\
  /s - Child process shares parent's console.\n\
  /v - Display verbose messages.\n\
  /w - Wait for the created process to finish before exiting." );
}


int wmain( int argc, wchar_t* argv[] )
{
	// Name of the process to create - basically what's after "/c" or "cmd.exe"
	wchar_t* lpwszCommandLine = NULL;

	// Parse commandline options

	int iCommandArgIndex, iCommandArgOffset;

	for (int i = 1; i < argc; i++) {
		// Check for an at-least-two-character string beginning with '/' or '-'
		if ((*argv[ i ] == L'/' || *argv[ i ] == L'-') && argv[ i ][ 1 ] != L'\0') {
			int j = 1;
			wchar_t opt;
			while ((opt = argv[ i ][ j ]) != L'\0') {
				// Multiple options can be grouped together, option c last (eg: /wrc)
				switch (opt) {
				case 'h':
					printHelp();
					return 0;
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
					params.bCommandPresent = 1;
					iCommandArgIndex = i;
					iCommandArgOffset = j;
					goto done_params;
				default:
					fwprintf( stderr, L"[E] Invalid option\n" );
					return 1;
				}
				j++;
			}
		}
		else {
			fwprintf( stderr, L"[E] Invalid argument\n" );
			return 1;
		}
	}
done_params:

	if (params.bCommandPresent) {
		// Find "c" parameter offset.
		wchar_t* p = wcsstr( GetCommandLine(), argv[ iCommandArgIndex ] );

		// Skip ahead iCommandArgOffset+1 characters ("-...c")
		p += iCommandArgOffset + 1;
		// Skip optional spaces before command
		while (*p == L' ' || *p == L'\t') p++;

		if (*p != L'\0') lpwszCommandLine = p;
	}
	if (! lpwszCommandLine) lpwszCommandLine = L"cmd.exe";

	wprintfv( L"[D] Your commandline is \"%ls\"\n", lpwszCommandLine );

	// lpwszCommandLine may be read-only. It must be copied to a writable area.
	SIZE_T nCommandLineBufSize = (wcslen( lpwszCommandLine ) + 1) * sizeof( wchar_t );
	wchar_t* lpwszImageName = HeapAlloc( GetProcessHeap(), 0, nCommandLineBufSize );
	memcpy( lpwszImageName, lpwszCommandLine, nCommandLineBufSize );

	int errCode = acquireSeDebugPrivilege();
	if (! errCode) errCode = createTrustedInstallerProcess( lpwszImageName );

	HeapFree( GetProcessHeap(), 0, lpwszImageName );

	if (params.bReturnCode) {
		if (errCode) errCode = -(EXIT_CODE_BASE + errCode);
		else errCode = nChildExitCode;
	}
	return errCode;
}
