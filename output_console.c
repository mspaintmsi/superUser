/*
	superUser 6.1

	Copyright 2019-2026 https://github.com/mspaintmsi/superUser

	output_console.c

	Display functions (console version)

*/

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

#include "utils.h"  // Utility functions

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
// Show an informational message.
//
BOOL showInfo( const wchar_t* pwszString )
{
	return printConsoleStream( stdout, pwszString );
}


//
// Show an error message.
//
void showError( const wchar_t* pwszMessage, DWORD dwCode, int iPosition )
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
// Show a formatted error message with variable arguments.
//
void showFmtError( DWORD dwCode, int iPosition, const wchar_t* pwszFormat, ... )
{
	va_list args;
	va_start( args, pwszFormat );

	// Allocate a buffer and write the formatted error message to it
	wchar_t* pBuffer = v_printFmtString( pwszFormat, args );
	if (pBuffer) {
		// Show the error message
		showError( pBuffer, dwCode, iPosition );

		freeHeap( pBuffer );
	}

	va_end( args );
}


//
// Show a formatted debug message with variable arguments.
//
void showFmtDebug( const wchar_t* pwszFormat, ... )
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
