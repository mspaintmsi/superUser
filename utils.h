#pragma once
/*
	superUser 6.0

	Copyright 2019-2025 https://github.com/mspaintmsi/superUser

	utils.h

	Utility functions

	- Memory allocation
	- Console output

*/

// Allocate a block of memory from the process heap.
LPVOID allocHeap( DWORD dwFlags, SIZE_T dwBytes );

// Free a block of memory allocated from the process heap.
void freeHeap( LPVOID lpMem );

// Print a string to standard output using the current console output code page.
BOOL printConsole( const wchar_t* pwszString );

// Print a formatted string with variable arguments to standard output
// using the current console output code page.
BOOL printFmtConsole( const wchar_t* pwszFormat, ... );

// Print an error message to standard error output.
void printError( const wchar_t* pwszMessage, DWORD dwCode, int iPosition );
