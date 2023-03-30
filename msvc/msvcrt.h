#pragma once
#ifndef _INC_MSVCRT
#define _INC_MSVCRT

/*
	Fix for the MSVCRT library

	Prerequisites:

	- The project must be linked against MSVCRT32.LIB or MSVCRT64.LIB,
		These files are pulled from the Windows Driver Kit (WDK).
	- The following VS project settings must be set :
		o C/C++ / Code generation / Runtime library : Multithread (/MT)
		o C/C++ / Preprocessor / Definitions : _NO_CRT_STDIO_INLINE
		o Linker / Input / Additional dependencies : Add "msvcrt32.lib" or "msvcrt64.lib"
		o Linker / Input / Ignore all default libraries : Yes (/NODEFAULTLIB)
			OR
		  Linker / Input / Ignore specific default libraries : libcmt.lib;libcmtd.lib
*/

#include <stdio.h>

// Support for old CRT with VC2015 or newer
// ----------------------------------------
//
// VC2015 switched to another CRT model called "Universal CRT".
// It has some incompatibilities in header files.
//
// Access stdin/stdout/stderr
// --------------------------
//
// Now, these streams are defined in corecrt-wstdio.h
//
// #define stdin  (__acrt_iob_func(0))
// #define stdout (__acrt_iob_func(1))
// #define stderr (__acrt_iob_func(2))
//
// UCRT uses __acrt_iob_func(). Older CRT used __iob_func. Also, older CRT used
// "_iobuf" structure with alias "FILE", however "FILE" in UCRT is just a
// pointer to some internal structure.

// Define __iob_func locally because it is missing in UCRT
__declspec(dllimport) FILE* __cdecl __iob_func();

// FILE structure for VS2013 and older
struct _old_iobuf {
	char* _ptr;
	int _cnt;
	char* _base;
	int _flag;
	int _file;
	int _charbuf;
	int _bufsiz;
	char* _tmpfname;
};

// Warning !
// It is not enough to sum the size of each field to obtain the total size
// of the structure. The alignment of each field must be taken into account.
// Otherwise, the size for the 64-bit architecture is wrong !
// The pointers are aligned to a multiple of a pointer size (4 or 8 bytes).
//
// This DOESN'T work:
// CRT_FILE_SIZE = (sizeof( char* ) * 3 + sizeof( int ) * 5)
//  gives 32 for 32-bit (good) and 44 for 64-bit (wrong!)
//  The right size for 64-bit is 48 bytes.
//
// This works:
//  sizeof( struct _old_iobuf )
// This works too:
//	enum { CRT_FILE_SIZE =
//		((sizeof( char* ) + sizeof( int ) - 1) / sizeof( char* ) + 1) * sizeof( char* ) +
//		((sizeof( char* ) + sizeof( int ) * 4 - 1) / sizeof( char* ) + 1) * sizeof( char* ) +
//		sizeof( char* )	};

FILE* __cdecl __acrt_iob_func( unsigned index )
{
	return (FILE*) ((char*) __iob_func() + index * sizeof( struct _old_iobuf ));
}

#endif // _INC_MSVCRT
