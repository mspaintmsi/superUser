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
int getTrustedInstallerToken( PHANDLE phToken );
void setAllPrivileges( HANDLE hProcessToken, BOOL bSilent );
