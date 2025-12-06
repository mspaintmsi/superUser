#pragma once
/*
	superUser 6.0

	Copyright 2019-2026 https://github.com/mspaintmsi/superUser

	utils.h

	Utility functions

	- Memory allocation
	- String formatting

*/

#include <windows.h>
#include <stdarg.h>

// Allocate a block of memory from the process heap.
LPVOID allocHeap( DWORD dwFlags, SIZE_T dwBytes );

// Free a block of memory allocated from the process heap.
void freeHeap( LPVOID lpMem );

//
// Print a formatted string with a list of variable arguments to a new string. 
// 
// The caller must use freeHeap to free the returned string.
// Returns NULL if an error occurs.
wchar_t* v_printFmtString( const wchar_t* pwszFormat, va_list arg_list );
