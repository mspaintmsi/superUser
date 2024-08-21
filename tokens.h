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
int createSystemContext( void );
int getTrustedInstallerProcess( HANDLE* phProcess );
int getTrustedInstallerToken( HANDLE hTIProcess, HANDLE* phToken );
void printError( wchar_t* pwszMessage, DWORD dwCode );
void setAllPrivileges( HANDLE hProcessToken, BOOL bVerbose );
