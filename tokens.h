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
int getTrustedInstallerProcess( HANDLE* phTIProcess );
void setAllPrivileges( HANDLE hProcessToken, BOOL bVerbose );
