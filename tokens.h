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
int getTrustedInstallerProcess( DWORD* pdwTIProcessId, HANDLE* phProcess );
int getTrustedInstallerToken( DWORD dwTIProcessId, HANDLE* phToken );
void setAllPrivileges( HANDLE hProcessToken, BOOL bVerbose );
