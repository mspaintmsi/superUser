#pragma once
/*
	https://github.com/mspaintmsi/superUser

	tokens.h

	Tokens and privileges management functions

*/

#include <winnt.h>
#ifdef __GNUC__
#include "winnt2.h"
#endif

int acquireSeDebugPrivilege( void );
int createChildProcessToken( HANDLE hBaseProcess, HANDLE* phNewToken );
int createSystemContext( void );
int getTrustedInstallerProcess( HANDLE* phTIProcess );
void setAllPrivileges( HANDLE hToken, BOOL bVerbose );

void printConsole( const wchar_t* pwszFormat, ... );
void printError( const wchar_t* pwszMessage, DWORD dwCode, int iPosition );

__declspec(noinline) LPVOID allocHeap( DWORD dwFlags, SIZE_T dwBytes );
__declspec(noinline) VOID freeHeap( LPVOID lpMem );
