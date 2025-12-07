/*
	superUser 6.0

	Copyright 2019-2026 https://github.com/mspaintmsi/superUser

	utils.c

	Utility functions

	- Memory allocation
	- String formatting

*/

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

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
// Print a formatted string with a list of variable arguments to a new string.
//
// The caller must use freeHeap to free the returned string.
// Returns NULL if an error occurs.
//
wchar_t* v_printFmtString( const wchar_t* pwszFormat, va_list arg_list )
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
