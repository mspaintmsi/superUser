/*
	superUser 6.0

	Copyright 2019-2025 https://github.com/mspaintmsi/superUser

	utils.c

	Utility functions

	- Memory allocation
	- Console output

*/

#include <windows.h>
#include <stdio.h>


//
// Allocate a block of memory from the process heap.
//
// dwFlags: 0 or HEAP_ZERO_MEMORY
//
__declspec(noinline) LPVOID allocHeap( DWORD dwFlags, SIZE_T dwBytes )
{
	LPVOID p = HeapAlloc( GetProcessHeap(), dwFlags, dwBytes );
	if (! p) abort();
	return p;
}


//
// Free a block of memory allocated from the process heap.
//
__declspec(noinline) void freeHeap( LPVOID lpMem )
{
	HeapFree( GetProcessHeap(), 0, lpMem );
}


//
// Print a string to a stream using the current console output code page.
//
static BOOL printConsoleStream( FILE* stream, const wchar_t* pwszString )
{
	BOOL bSuccess = FALSE;

	// Convert the string (wide chars) to console output code page (bytes)
	UINT nCodePage = GetConsoleOutputCP();
	int nSize = WideCharToMultiByte( nCodePage, 0, pwszString, -1, NULL, 0, NULL, NULL );
	if (nSize > 0) {
		char* pBuffer = allocHeap( 0, nSize );
		if (WideCharToMultiByte( nCodePage, 0, pwszString, -1, pBuffer, nSize,
			NULL, NULL ) > 0) {
			// Write to the console stream
			bSuccess = fputs( pBuffer, stream ) >= 0;
		}
		freeHeap( pBuffer );
	}

	return bSuccess;
}


//
// Print a string to standard output using the current console output code page.
//
BOOL printConsole( const wchar_t* pwszString )
{
	return printConsoleStream( stdout, pwszString );
}


//
// Print a formatted string with a list of variable arguments to a new string. 
// 
// The caller must use freeHeap to free the returned string.
// Returns NULL if an error occurs.
//
static wchar_t* v_printFmtString( const wchar_t* pwszFormat, va_list arg_list )
{
	// Calculate the length of the formatted string (wide chars) and allocate a buffer
	int nLen = _vscwprintf( pwszFormat, arg_list );
	if (nLen < 0) return NULL;
	SIZE_T nSize = (SIZE_T) nLen + 1;
	wchar_t* pBuffer = allocHeap( 0, nSize * sizeof( wchar_t ) );

	// Write the formatted string to the buffer
	if (_vsnwprintf_s( pBuffer, nSize, _TRUNCATE, pwszFormat, arg_list ) < 0) {
		freeHeap( pBuffer );
		return NULL;
	}

	return pBuffer;
}


//
// Print a formatted string with variable arguments to a stream
// using the current console output code page.
//
static BOOL printFmtConsoleStream( FILE* stream, const wchar_t* pwszFormat, ... )
{
	va_list args;
	va_start( args, pwszFormat );
	BOOL bSuccess = FALSE;

	// Allocate a buffer and write the formatted string to it
	wchar_t* pBuffer = v_printFmtString( pwszFormat, args );
	if (pBuffer) {
		// Print the buffer to the stream using the current console output code page
		bSuccess = printConsoleStream( stream, pBuffer );

		freeHeap( pBuffer );
	}

	va_end( args );
	return bSuccess;
}


//
// Print a formatted debug message with variable arguments to standard output.
//
void printFmtDebug( const wchar_t* pwszFormat, ... )
{
	va_list args;
	va_start( args, pwszFormat );

	// Allocate a buffer and write the formatted message to it
	wchar_t* pBuffer = v_printFmtString( pwszFormat, args );
	if (pBuffer) {
		// Print the debug message to standard output
		printFmtConsoleStream( stdout, L"[D] %ls\n", pBuffer );

		freeHeap( pBuffer );
	}

	va_end( args );
}


//
// Print an error message to standard error output.
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
	printFmtConsoleStream( stderr, pwszFormat, pwszMessage, dwCode, iPosition );
}


//
// Print a formatted error message with variable arguments to standard error output.
//
void printFmtError( DWORD dwCode, int iPosition, const wchar_t* pwszFormat, ... )
{
	va_list args;
	va_start( args, pwszFormat );

	// Allocate a buffer and write the formatted error message to it
	wchar_t* pBuffer = v_printFmtString( pwszFormat, args );
	if (pBuffer) {
		// Print the error message to standard error output
		printError( pBuffer, dwCode, iPosition );

		freeHeap( pBuffer );
	}

	va_end( args );
}
