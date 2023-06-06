/*
	superUser

	A simple and lightweight utility to start any process
	with TrustedInstaller privileges.

	https://github.com/mspaintmsi/superUser

	superUser.c

*/

#include <windows.h>
#include <stdio.h>

#include "tokens.h" // Defines tokens and privileges management functions

// Program options
struct {
	unsigned int bReturnCode : 1;  // Whether to return process exit code
	unsigned int bSeamless : 1;    // Whether child process shares parent's console
	unsigned int bVerbose : 1;     // Whether to print debug messages or not
	unsigned int bWait : 1;        // Whether to wait to finish created process
} options = {0};

#define wputs _putws
#define wprintfv(...) \
if (options.bVerbose) wprintf(__VA_ARGS__); // Only use when bVerbose in scope

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
static int nChildExitCode = 0;


static int createTrustedInstallerProcess( wchar_t* pwszImageName )
{
	// Start the TrustedInstaller service and get the token
	HANDLE hToken = NULL;
	int errCode = getTrustedInstallerToken( &hToken );
	if (errCode) return errCode;

	// Get the console session id and set it in the token
	DWORD dwSessionId = WTSGetActiveConsoleSessionId();
	if (dwSessionId != (DWORD) -1) {
		SetTokenInformation( hToken, TokenSessionId, (PVOID) &dwSessionId,
			sizeof( DWORD ) );
	}

	// Set all privileges in the child process token
	setAllPrivileges( hToken, options.bVerbose );


	// Initialize STARTUPINFO

	STARTUPINFO startupInfo = {0};
	startupInfo.cb = sizeof( STARTUPINFO );
	startupInfo.dwFlags = STARTF_USESHOWWINDOW;
	startupInfo.wShowWindow = SW_SHOWNORMAL;

	// Create process

	PROCESS_INFORMATION processInfo = {0};
	DWORD dwCreationFlags = 0;
	if (! options.bSeamless) dwCreationFlags |= CREATE_NEW_CONSOLE;

	wprintfv( L"[D] Creating specified process\n" );

	BOOL bCreateResult = CreateProcessAsUser(
		hToken,
		NULL,
		pwszImageName,
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

		if (options.bWait) {
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
	if (options.bReturnCode) {
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


static BOOL getArgument( wchar_t** ppArgument, wchar_t** ppArgumentIndex )
{
	// Current pointer to the remainder of the line to be parsed.
	// Initialized with the full command line on the first call.
	static wchar_t* p = NULL;
	if (! p) {
		p = GetCommandLine();

		// Skip program name
		BOOL bQuote = FALSE;
		while (*p != L'\0') {
			if (*p == L'"') bQuote = ! bQuote;
			else if (! bQuote && (*p == L' ' || *p == L'\t')) break;
			p++;
		}
	}

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
		while (*p != L' ' && *p != L'\t' && *p != L'\0') p++;

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
	wchar_t* pwszCommandLine = NULL;

	wchar_t* pwszArgument = NULL;  // Command line argument
	wchar_t* pwszArgumentIndex = NULL;  // Pointer to argument in command line

	// Parse command line options

	while (getArgument( &pwszArgument, &pwszArgumentIndex )) {
		// Check for an at-least-two-character string beginning with '/' or '-'
		if (*pwszArgument == L'/' || *pwszArgument == L'-') {
			if (pwszArgument[ 1 ] != L'\0') {
				int j = 1;
				wchar_t opt;
				while ((opt = pwszArgument[ j ]) != L'\0') {
					// Multiple options can be grouped together (eg: /wrs)
					switch (opt) {
					case 'h':
						printHelp();
						errCode = -1;
						goto done_params;
					case 'r':
						options.bReturnCode = 1;
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
					case 'c':
					{	/*
						This option is no longer useful. Do not use it.
						It is kept only for compatibility with previous versions.
						*/
						wchar_t* p = pwszArgumentIndex + (j + 1);
						while (*p == L' ' || *p == L'\t') p++;
						pwszCommandLine = p;
						goto done_params;
					}
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
			// First non-option argument found
			pwszCommandLine = pwszArgumentIndex;
			break;
		}
	}
done_params:
	// Free the last argument (if it exists)
	if (pwszArgument) HeapFree( GetProcessHeap(), 0, pwszArgument );

	if (errCode) return getExitCode( errCode );

	// Check the consistency of the options
	if ((options.bReturnCode || options.bSeamless) && ! options.bWait) {
		fwprintf( stderr, L"[E] /r or /s option requires /w\n" );
		return getExitCode( 1 );
	}

	if (! pwszCommandLine) pwszCommandLine = L"cmd.exe";

	wprintfv( L"[D] Your commandline is \"%ls\"\n", pwszCommandLine );

	// pwszCommandLine may be read-only. It must be copied to a writable area.
	size_t nCommandLineBufSize = (wcslen( pwszCommandLine ) + 1) * sizeof( wchar_t );
	wchar_t* pwszImageName = HeapAlloc( GetProcessHeap(), 0, nCommandLineBufSize );
	memcpy( pwszImageName, pwszCommandLine, nCommandLineBufSize );

	errCode = acquireSeDebugPrivilege();
	if (! errCode) errCode = createSystemContext();
	if (! errCode) errCode = createTrustedInstallerProcess( pwszImageName );

	HeapFree( GetProcessHeap(), 0, pwszImageName );

	return getExitCode( errCode );
}
