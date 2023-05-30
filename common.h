#pragma once
/*
	https://github.com/mspaintmsi/superUser

	common.h

	Global variables and macros

*/

#include <stdio.h>

struct parameters {
	unsigned int bReturnCode : 1;     // Whether to return process exit code
	unsigned int bSeamless : 1;       // Whether child process shares parent's console
	unsigned int bVerbose : 1;        // Whether to print debug messages or not
	unsigned int bWait : 1;           // Whether to wait to finish created process
};

extern struct parameters params;

#define wputs _putws
#define wprintfv(...) \
if (params.bVerbose) wprintf(__VA_ARGS__); // Only use when bVerbose in scope
