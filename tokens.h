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
void printError( const wchar_t* pwszMessage, DWORD dwCode, int iPosition );
void setAllPrivileges( HANDLE hToken, BOOL bVerbose );
