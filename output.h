#pragma once
/*
	superUser 6.1

	Copyright 2019-2026 https://github.com/mspaintmsi/superUser

	output.h

	Display functions

*/

#include <windows.h>

// Show an informational message.
BOOL showInfo( const wchar_t* pwszString );

// Show an error message.
void showError( const wchar_t* pwszMessage, DWORD dwCode, int iPosition );

// Show a formatted error message with variable arguments.
void showFmtError( DWORD dwCode, int iPosition, const wchar_t* pwszFormat, ... );

// Show a formatted debug message with variable arguments.
void showFmtDebug( const wchar_t* pwszFormat, ... );

// Set the output title.
void setOutputTitle( const wchar_t* pwszString );
