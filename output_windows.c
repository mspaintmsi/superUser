/*
	superUser 6.0

	Copyright 2019-2026 https://github.com/mspaintmsi/superUser

	output_windows.c

	Display functions (windows version, no console)

*/

#include <windows.h>
#include <stdarg.h>

#include "utils.h"    // Utility functions

//
// Show a message.
//
static BOOL showMessage( BOOL bError, const wchar_t* pwszString )
{
	wchar_t* pwszTitle = NULL;
	UINT uFlags = MB_OK;
	if (bError) {
		pwszTitle = L"Error";
		uFlags |= MB_ICONERROR;
	}
	else pwszTitle = L"Help";

	return MessageBox( NULL, pwszString, pwszTitle, uFlags ) > 0;
}


//
// Show a formatted message with variable arguments.
//
static BOOL showFmtMessage( BOOL bError, const wchar_t* pwszFormat, ... )
{
	va_list args;
	va_start( args, pwszFormat );
	BOOL bSuccess = FALSE;

	// Allocate a buffer and write the formatted string to it
	wchar_t* pBuffer = v_printFmtString( pwszFormat, args );
	if (pBuffer) {
		// Show the message
		bSuccess = showMessage( bError, pBuffer );

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
	return showMessage( FALSE, pwszString );
}


//
// Show an error message.
//
void showError( const wchar_t* pwszMessage, DWORD dwCode, int iPosition )
{
	wchar_t pwszFormat[] = L"%ls (code: 0x%08lX, pos: %d).\n";
	wchar_t* pEnd = NULL;
	if (dwCode == 0) {
		// Remove the error code/position part from the format
		pEnd = pwszFormat + 3;
	}
	else if (iPosition == 0) {
		// Remove the error position part from the format
		pEnd = pwszFormat + 18;
		*pEnd++ = L')';
	}
	if (pEnd) {
		*pEnd++ = L'.';
		*pEnd++ = L'\n';
		*pEnd = L'\0';
	}
	showFmtMessage( TRUE, pwszFormat, pwszMessage, dwCode, iPosition );
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
void showFmtDebug( const wchar_t* pwszFormat, ... ) {}
