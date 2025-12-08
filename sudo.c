/*
	superUser 6.1

	A simple and lightweight utility to start any process
	as the System user with Trusted Installer privileges.

	Copyright 2019-2026 https://github.com/mspaintmsi/superUser

	sudo.c

*/

#include <windows.h>
#include <wchar.h>

#include "output.h" // Display functions
#include "utils.h"  // Utility functions
#include "tokens.h" // Tokens and privileges management functions

// Program options
static struct {
	unsigned int bMinimize : 1;    // Whether to minimize created window
} options = {0};

/*
	sudo.exe - Return codes

	The exit code of the child process is returned.

	If sudo fails, it returns the code -(EXIT_CODE_BASE + errCode),
	where errCode is one of the codes listed below.

		1 - Invalid argument
		2 - Failed to acquire SeDebugPrivilege
		3 - Failed to open/start TrustedInstaller process/service
		4 - Process creation failed
		5 - Another fatal error occurred
		6 - The child process' exit code could not be got (very unlikely)
*/

#define EXIT_CODE_BASE 1000000
static int nChildExitCode = 0;


static int getExitCode( int code )
{
	if (code == -1) code = 0;  // Print help, exit with code 0
	if (code) code = -(EXIT_CODE_BASE + code);
	else code = nChildExitCode;
	return code;
}


static int createChildProcess( wchar_t* pwszImageName )
{
	int errCode = 0;
	HANDLE hBaseProcess = NULL, hChildProcessToken = NULL;

	// Start the TrustedInstaller service and get its process handle
	errCode = getTrustedInstallerProcess( &hBaseProcess );
	if (errCode) return errCode;

	// Create the child process token
	errCode = createChildProcessToken( hBaseProcess, &hChildProcessToken );
	CloseHandle( hBaseProcess );
	if (errCode) return errCode;

	// Get the console session id and set it in the token
	DWORD dwSessionId = WTSGetActiveConsoleSessionId();
	if (dwSessionId != (DWORD) -1) {
		SetTokenInformation( hChildProcessToken, TokenSessionId, (PVOID) &dwSessionId,
			sizeof( DWORD ) );
	}

	// Set all privileges in the child process token
	setAllPrivileges( hChildProcessToken, NULL );

	// Initialize startupInfo

	STARTUPINFO startupInfo = {0};

	startupInfo.cb = sizeof( STARTUPINFO );
	startupInfo.dwFlags = STARTF_USESHOWWINDOW;
	if (options.bMinimize)
		startupInfo.wShowWindow = SW_SHOWMINNOACTIVE;
	else
		startupInfo.wShowWindow = SW_SHOWNORMAL;

	// Create process

	PROCESS_INFORMATION processInfo = {0};

	BOOL bCreateResult = CreateProcessAsUser(
		hChildProcessToken,
		NULL,
		pwszImageName,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&startupInfo,
		&processInfo
	);

	DWORD dwCreateError = bCreateResult ? 0 : GetLastError();
	CloseHandle( hChildProcessToken );

	if (bCreateResult) {
		WaitForSingleObject( processInfo.hProcess, INFINITE );

		// Get exit code of child process
		DWORD dwExitCode;
		if (! GetExitCodeProcess( processInfo.hProcess, &dwExitCode ))
			dwExitCode = getExitCode( 6 );

		nChildExitCode = dwExitCode;

		CloseHandle( processInfo.hProcess );
		CloseHandle( processInfo.hThread );
	}
	else {
		// Most commonly - 0x2 - The system cannot find the file specified.
		showError( L"Process creation failed", dwCreateError, 0 );
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


static void showHelp( void )
{
	showInfo( L"\n\
sudo [options] [command_to_run]\n\n\
Options (you can use either \"-\" or \"/\"):\n\
  /h  Display this help message.\n\
  /m  Minimize the created window.\n\
" );
}


int wmain( void )
{
	int errCode = 0;  // sudo error code

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
				switch (opt) {
				case 'h':
					showHelp();
					errCode = -1;
					goto done_params;
				case 'm':
					options.bMinimize = 1;
					break;
				default:
					showFmtError( 0, 0, L"Invalid option '%lc'", opt );
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

	if (! pwszCommandLine) pwszCommandLine = L"cmd.exe";

	// pwszCommandLine may be read-only. It must be copied to a writable area.
	size_t nCommandLineBufSize = (wcslen( pwszCommandLine ) + 1) * sizeof( wchar_t );
	wchar_t* pwszImageName = allocHeap( 0, nCommandLineBufSize );
	memcpy( pwszImageName, pwszCommandLine, nCommandLineBufSize );

	errCode = acquireSeDebugPrivilege();
	if (! errCode) errCode = createSystemContext();
	if (! errCode) errCode = createChildProcess( pwszImageName );

	freeHeap( pwszImageName );

	return getExitCode( errCode );
}
