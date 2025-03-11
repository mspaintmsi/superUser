/*
	superUser 6.0

	A simple and lightweight utility to start any process
	as the System user with Trusted Installer privileges.

	Copyright 2019-2025 https://github.com/mspaintmsi/superUser

	superUser.c

*/

#include <windows.h>

#include "utils.h"  // Utility functions
#include "tokens.h" // Tokens and privileges management functions

// Program options
static struct {
	unsigned int bMinimize : 1;    // Whether to minimize created window
	unsigned int bSeamless : 1;    // Whether child process shares parent's console
	unsigned int bVerbose : 1;     // Whether to print debug messages or not
	unsigned int bWait : 1;        // Whether to wait for child process to finish
} options = {0};

#define printFmtVerbose(...) \
	if (options.bVerbose) printFmtDebug(__VA_ARGS__);

/*
	Return codes (without /w option):
		1 - Invalid argument
		2 - Failed to acquire SeDebugPrivilege
		3 - Failed to open/start TrustedInstaller process/service
		4 - Process creation failed
		5 - Another fatal error occurred

	If the /w option is specified, the exit code of the child process is returned.
	If superUser fails, it returns the code -(EXIT_CODE_BASE + errCode),
	where errCode is one of the codes listed above.
	If the exit code could not be got (very unlikely), it returns -(EXIT_CODE_BASE + 6).
*/

#define EXIT_CODE_BASE 1000000
static int nChildExitCode = 0;


static int getExitCode( int code )
{
	if (code == -1) code = 0;  // Print help, exit with code 0
	if (options.bWait) {
		if (code) code = -(EXIT_CODE_BASE + code);
		else code = nChildExitCode;
	}
	return code;
}


static int createChildProcess( wchar_t* pwszImageName )
{
	int errCode = 0;
	HANDLE hBaseProcess = NULL, hChildProcessToken = NULL;

	// Start the TrustedInstaller service and get its process handle
	errCode = getTrustedInstallerProcess( &hBaseProcess );
	if (errCode) return errCode;

	if (options.bSeamless) {
		// Create the child process token
		errCode = createChildProcessToken( hBaseProcess, &hChildProcessToken );
		if (errCode) {
			CloseHandle( hBaseProcess );
			return errCode;
		}

		// Get the console session id and set it in the token
		DWORD dwSessionId = WTSGetActiveConsoleSessionId();
		if (dwSessionId != (DWORD) -1) {
			SetTokenInformation( hChildProcessToken, TokenSessionId, (PVOID) &dwSessionId,
				sizeof( DWORD ) );
		}

		// Set all privileges in the child process token
		setAllPrivileges( hChildProcessToken, options.bVerbose );
	}

	// Initialize startupInfo

	STARTUPINFOEX startupInfo = {0};

	startupInfo.StartupInfo.cb = sizeof( STARTUPINFOEX );
	startupInfo.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	if (options.bMinimize)
		startupInfo.StartupInfo.wShowWindow = SW_SHOWMINNOACTIVE;
	else
		startupInfo.StartupInfo.wShowWindow = SW_SHOWNORMAL;

	if (! options.bSeamless) {
		// Initialize attribute lists for "parent assignment"

		SIZE_T attributeListLength = 0;
		InitializeProcThreadAttributeList( NULL, 1, 0, (PSIZE_T) &attributeListLength );
		startupInfo.lpAttributeList = allocHeap( HEAP_ZERO_MEMORY, attributeListLength );
		InitializeProcThreadAttributeList( startupInfo.lpAttributeList, 1, 0,
			(PSIZE_T) &attributeListLength );

		UpdateProcThreadAttribute( startupInfo.lpAttributeList, 0,
			PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, &hBaseProcess, sizeof( HANDLE ), NULL, NULL );
	}

	// Create process

	PROCESS_INFORMATION processInfo = {0};
	DWORD dwCreationFlags = 0;
	if (! options.bSeamless)
		dwCreationFlags = CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT |
		CREATE_NEW_CONSOLE;

	printFmtVerbose( L"Creating specified process" );

	BOOL bCreateResult = CreateProcessAsUser(
		hChildProcessToken,
		NULL,
		pwszImageName,
		NULL,
		NULL,
		FALSE,
		dwCreationFlags,
		NULL,
		NULL,
		(LPSTARTUPINFO) &startupInfo,
		&processInfo
	);

	DWORD dwCreateError = bCreateResult ? 0 : GetLastError();

	if (options.bSeamless) CloseHandle( hChildProcessToken );
	else {
		DeleteProcThreadAttributeList( startupInfo.lpAttributeList );
		freeHeap( startupInfo.lpAttributeList );
	}
	CloseHandle( hBaseProcess );

	if (bCreateResult) {
		if (! options.bSeamless) {
			HANDLE hProcessToken = NULL;
			OpenProcessToken( processInfo.hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
				&hProcessToken );
			// Set all privileges in the child process token
			setAllPrivileges( hProcessToken, options.bVerbose );
			CloseHandle( hProcessToken );

			ResumeThread( processInfo.hThread );
		}

		printFmtVerbose( L"Created process ID: %lu", processInfo.dwProcessId );

		if (options.bWait) {
			printFmtVerbose( L"Waiting for process to exit" );
			WaitForSingleObject( processInfo.hProcess, INFINITE );

			// Get exit code of child process
			DWORD dwExitCode;
			if (! GetExitCodeProcess( processInfo.hProcess, &dwExitCode ))
				dwExitCode = getExitCode( 6 );

			printFmtVerbose( L"Process exited with code %ld", dwExitCode );
			nChildExitCode = dwExitCode;
		}

		CloseHandle( processInfo.hProcess );
		CloseHandle( processInfo.hThread );
	}
	else {
		// Most commonly - 0x2 - The system cannot find the file specified.
		printError( L"Process creation failed", dwCreateError, 0 );
		return 4;
	}

	return 0;
}


static BOOL getArgument( wchar_t** ppArgument, wchar_t** ppArgumentIndex )
{
	// Current pointer to the remainder of the line to be parsed.
	// Initialized with the full command line on the first call.
	static wchar_t* p = NULL;
	if (! p) {
		p = GetCommandLine();

		// Skip program name
		BOOL bQuote = FALSE;
		while (*p) {
			if (*p == L'"') bQuote = ! bQuote;
			else if (! bQuote && (*p == L' ' || *p == L'\t')) break;
			p++;
		}
	}

	// Free the previous argument (if it exists)
	if (*ppArgument) freeHeap( *ppArgument );
	*ppArgument = NULL;

	// Search argument

	// Skip spaces
	while (*p == L' ' || *p == L'\t') p++;

	if (*p) {
		// Argument found
		wchar_t* pBegin = p;

		// Search the end of the argument
		while (*p && *p != L' ' && *p != L'\t') p++;

		size_t nArgSize = (p - pBegin) * sizeof( wchar_t );
		*ppArgument = allocHeap( HEAP_ZERO_MEMORY, nArgSize + sizeof( wchar_t ) );
		memcpy( *ppArgument, pBegin, nArgSize );
		*ppArgumentIndex = pBegin;
		return TRUE;
	}

	// Argument not found
	return FALSE;
}


static void printHelp( void )
{
	printConsole( L"\n\
superUser [options] [command_to_run]\n\n\
Options (you can use either \"-\" or \"/\"):\n\
  /h  Display this help message.\n\
  /m  Minimize the created window.\n\
  /s  The child process shares the parent's console. Requires /w.\n\
  /v  Display verbose messages.\n\
  /w  Wait for the child process to finish before exiting.\n\
" );
}


int wmain( void )
{
	int errCode = 0;  // superUser error code

	// Command to run (executable filename of process to create, followed by
	// arguments) - basically the first non-option argument or "cmd.exe".
	wchar_t* pwszCommandLine = NULL;

	wchar_t* pwszArgument = NULL;  // Command line argument
	wchar_t* pwszArgumentIndex = NULL;  // Pointer to argument in command line

	// Parse command line options

	while (getArgument( &pwszArgument, &pwszArgumentIndex )) {
		// Check for an at-least-two-character string beginning with '/' or '-'
		if ((*pwszArgument == L'/' || *pwszArgument == L'-') && pwszArgument[ 1 ]) {
			int j = 1;
			wchar_t opt;
			while ((opt = pwszArgument[ j ])) {
				// Multiple options can be grouped together (eg: /ws)
				switch (opt) {
				case 'h':
					printHelp();
					errCode = -1;
					goto done_params;
				case 'm':
					options.bMinimize = 1;
					break;
				case 's':
					options.bSeamless = 1;
					break;
				case 'v':
					options.bVerbose = 1;
					break;
				case 'w':
					options.bWait = 1;
					break;
				default:
					printFmtError( 0, 0, L"Invalid option '%lc'", opt );
					errCode = 1;
					goto done_params;
				}
				j++;
			}
		}
		else {
			// First non-option argument found
			pwszCommandLine = pwszArgumentIndex;
			break;
		}
	}
done_params:
	// Free the last argument (if it exists)
	if (pwszArgument) freeHeap( pwszArgument );

	if (errCode) return getExitCode( errCode );

	// Check the consistency of the options
	if (options.bSeamless && ! options.bWait) {
		printError( L"/s option requires /w", 0, 0 );
		return getExitCode( 1 );
	}

	if (! pwszCommandLine) pwszCommandLine = L"cmd.exe";

	printFmtVerbose( L"Your command line is '%ls'", pwszCommandLine );

	// pwszCommandLine may be read-only. It must be copied to a writable area.
	size_t nCommandLineBufSize = (wcslen( pwszCommandLine ) + 1) * sizeof( wchar_t );
	wchar_t* pwszImageName = allocHeap( 0, nCommandLineBufSize );
	memcpy( pwszImageName, pwszCommandLine, nCommandLineBufSize );

	errCode = acquireSeDebugPrivilege();
	if (! errCode && options.bSeamless) errCode = createSystemContext();
	if (! errCode) errCode = createChildProcess( pwszImageName );

	freeHeap( pwszImageName );

	return getExitCode( errCode );
}
