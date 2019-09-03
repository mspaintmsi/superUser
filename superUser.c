#define _WIN32_WINNT _WIN32_WINNT_VISTA
#define _UNICODE

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include <processthreadsapi.h>
#include <winsvc.h>
#include <winbase.h>

#define DEFAULT_PROCESS L"cmd.exe"

// I couldn't seem to find a constant in the headers for this.
#define MAX_COMMANDLINE 8192
// Can be set to any number
#define MAX_ARGUMENTS 1000

#define VERB_PRINT(...) if(bVerbose) { wprintf(__VA_ARGS__); }

BOOL SetPrivilege(
	HANDLE hToken,
	LPCTSTR Privilege,
	BOOL bEnablePrivilege){

	TOKEN_PRIVILEGES tp;
	LUID luid;
	TOKEN_PRIVILEGES tpPrevious;
	DWORD cbPrevious=sizeof(TOKEN_PRIVILEGES);

	if(!LookupPrivilegeValue( NULL, Privilege, &luid )) return FALSE;

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = 0;

	AdjustTokenPrivileges(
			hToken,
			FALSE,
			&tp,
			sizeof(TOKEN_PRIVILEGES),
			&tpPrevious,
			&cbPrevious);

	if (GetLastError() != ERROR_SUCCESS) return FALSE;

	tpPrevious.PrivilegeCount = 1;
	tpPrevious.Privileges[0].Luid = luid;

	if(bEnablePrivilege) {
		tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
	} else {
		tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED & tpPrevious.Privileges[0].Attributes);
	}

	AdjustTokenPrivileges(
			hToken,
			FALSE,
			&tpPrevious,
			cbPrevious,
			NULL,
			NULL);

	if (GetLastError() != ERROR_SUCCESS)
		return FALSE;

	return TRUE;
}

// Get all the arguments after the executable path
/* wchar_t *GetCommandLineArgs(int argc, wchar_t *argv[]) {

	wchar_t *parsedCommandLine = calloc(MAX_COMMANDLINE, sizeof(wchar_t));

	for(int a = 1; a < argc; ++a) {
		wchar_t lpwszTempCmd[MAX_COMMANDLINE];
		wprintf(L"Argument %d = \"%s\"\n", a, argv[a]);
		if((wcscspn(argv[a], L"&<>()@^| ") != wcslen(argv[a])) || (wcscmp(argv[a], L"\0") == 0))
			// Surround with "
			swprintf(lpwszTempCmd, sizeof(lpwszTempCmd), L" \"%s\"", argv[a]);
		else
			// Leave as is
			swprintf(lpwszTempCmd, sizeof(lpwszTempCmd), L" %s", argv[a]);

		wcscat(parsedCommandLine, lpwszTempCmd);
	}

	// Remove leading space
	swscanf(parsedCommandLine, L" %[^\n]s", parsedCommandLine);

	return parsedCommandLine;
} */

wchar_t *GetCommandLineArgs(wchar_t *argv[], int skip) {
	wchar_t *lpwszCommandLine = GetCommandLine();
	int executablePathLength = wcslen(argv[0]);

	int totlen = -1;
	for(int a = 1; a < skip; ++a) {
		totlen++; // Count space before argument
		totlen += wcslen(argv[a]);
	}

	if(*lpwszCommandLine == L'\"')
		executablePathLength += 2;

	lpwszCommandLine += executablePathLength + 2;
	lpwszCommandLine += totlen;

	while(*lpwszCommandLine == L' ')
		lpwszCommandLine++;

	return lpwszCommandLine;
}

int wmain(int argc, wchar_t *argv[]) {

	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hTIService = NULL;
	HANDLE hTIProcess = NULL;
	SERVICE_STATUS_PROCESS lpServiceStatusBuffer = { 0 };
	ULONG ulBytesNeeded = 0;
	HANDLE hToken;

	BOOLEAN bVerbose = FALSE, bWait = FALSE;
	wchar_t lpwszNewProcessName[MAX_COMMANDLINE];

	STARTUPINFOEX startupInfo;
	size_t attributeListLength;

	PROCESS_INFORMATION newProcInfo;

	int returnStatus = 0;

	int optcount = 1;
	BOOLEAN bCommandPresent = FALSE;
	for(int a = 0; a < argc; ++a) {
		if(wcscmp(argv[a], L"/h") == 0) {
			// Print help and exit
			wprintf(L"%s [options] /c [Process Name]\n", argv[0]);
			wprintf(L"Options:\n");
			wprintf(L"\t/v - Display progress info.\n");
			wprintf(L"\t/w - Wait for the created process to finish before exiting.\n");
			wprintf(L"\t/h - Display help info.\n\n");
			wprintf(L"\t/c - Specify command/process to execute. MUST BE THE LAST ARGUMENT. If it's not specified cmd starts by default.\n\n");

			exit(0);
		} else if (wcscmp(argv[a], L"/v") == 0) {
			bVerbose = TRUE;
			optcount++;
		} else if (wcscmp(argv[a], L"/w") == 0) {
			bWait = TRUE;
			optcount++;
		} else if (wcscmp(argv[a], L"/c") == 0) {
			bCommandPresent = TRUE;
			optcount++;
			break;
		}
	}

	if(!bCommandPresent) {
		wcscpy(lpwszNewProcessName, L"cmd.exe");
	} else {
		wcscpy(lpwszNewProcessName, GetCommandLineArgs(argv, optcount));
	}

	//Acquire SeDebugPrivilege
	if(!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken)) {
		if (GetLastError() == ERROR_NO_TOKEN) {
			ImpersonateSelf(SecurityImpersonation);
			OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken);
		}
	}

	if(!SetPrivilege(hToken, SE_DEBUG_NAME, TRUE)) {
		CloseHandle(hToken);
		VERB_PRINT(L"Failed acquiring the SeDebugPrivilege.\r\n");

		returnStatus = 0xDEAD;
		goto cleanupandexit;
	} else {
		VERB_PRINT(L"SeDebugPrivilege acquired.\r\n");
	}

	//We could acquire a handle with SC_MANAGER_ALL_ACCESS, but it's not really needed.
	hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE | SC_MANAGER_CONNECT );

	//We need to query its status, start it and get its PID.
	hTIService = OpenServiceW(hSCManager, L"TrustedInstaller", SERVICE_START | SERVICE_QUERY_STATUS);

	//In theory, this will never fail if we've successfully acquired SeDebugPrivilege, so there's probably no point in error checks.
	queryService:

	QueryServiceStatusEx(hTIService, SC_STATUS_PROCESS_INFO, (PBYTE) &lpServiceStatusBuffer, sizeof(SERVICE_STATUS_PROCESS), &ulBytesNeeded);

	if(lpServiceStatusBuffer.dwCurrentState == SERVICE_STOPPED) {
		if(StartServiceW(hTIService, 0, NULL)) {
			VERB_PRINT(L"Started the TrustedInstaller service.\r\n");
		} else {
			VERB_PRINT(L"Failed starting the TrustedInstaller service.\r\n");
			goto cleanupandexit;
		}
	}

	//With SeDebugPrivilege we can successfully get any handle with PROCESS_ALL_ACCESS, but in this case we need to make sure the service is actually running.
	hTIProcess = OpenProcess(PROCESS_ALL_ACCESS, 1, lpServiceStatusBuffer.dwProcessId);
	if(hTIProcess == NULL) {
		VERB_PRINT(L"Failed opening TrustedInstaller process. Retrying..\r\n\tError code: 0x%08lX\r\n", GetLastError());
		goto queryService;
	} else {
		VERB_PRINT(L"TrustedInstaller process handle acquired.\r\n");
	}

	//Create the child process.

	ZeroMemory(&startupInfo, sizeof(STARTUPINFOEX));
	startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);

	startupInfo.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	startupInfo.StartupInfo.wShowWindow = SW_SHOWNORMAL;

	InitializeProcThreadAttributeList(NULL, 1, 0, (PSIZE_T) &attributeListLength);

	startupInfo.lpAttributeList = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, attributeListLength);

	InitializeProcThreadAttributeList(startupInfo.lpAttributeList, 1, 0, (PSIZE_T) &attributeListLength);
	UpdateProcThreadAttribute(startupInfo.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, &hTIProcess, sizeof(HANDLE), NULL, NULL);

	VERB_PRINT(L"Creating specified process.\r\n");
	ZeroMemory(&newProcInfo, sizeof(newProcInfo));

	DWORD dwCreationFlags = CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT | CREATE_NEW_CONSOLE;

	if(CreateProcessW(
			NULL,
			lpwszNewProcessName,
			NULL,
			NULL,
			FALSE,
			dwCreationFlags,
			NULL,
			NULL,
			&startupInfo.StartupInfo,
			&newProcInfo)
	){
		DeleteProcThreadAttributeList(startupInfo.lpAttributeList);

		wprintf(L"Created Process ID = %ld\r\n", newProcInfo.dwProcessId);
		ResumeThread(newProcInfo.hThread);

		if(bWait)
			WaitForSingleObject(newProcInfo.hProcess, INFINITE);

		// Free unneeded handles from newProcInfo.
		CloseHandle(newProcInfo.hProcess);
		CloseHandle(newProcInfo.hThread);

	} else {
		//It will happen when an incorrect process name is specified. (Will exit with error 0x00000002 - File not found)
		wprintf(L"Process Creation Failed.\r\n\tError code: 0x%08lX\r\n", GetLastError());
		returnStatus = 0xDEDDED;
	}

	cleanupandexit:

	if(hToken)
		CloseHandle(hToken);

	if(hSCManager)
		CloseServiceHandle(hSCManager);

	if(hTIService)
		CloseServiceHandle(hTIService);

	if(hTIProcess)
		CloseHandle(hTIProcess);

	return returnStatus;
}
