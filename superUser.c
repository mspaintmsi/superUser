#include "superUser.h"

BOOLEAN bVerbose = FALSE, bWait = FALSE, bCommandPresent = FALSE;

int wmain(int argc, wchar_t *argv[]) {

	wchar_t lpwszNewProcessName[MAX_COMMANDLINE];

	STARTUPINFOEX startupInfo;

	int iReturnStatus = 0;

	int cOptions = 1;
	for(int a = 0; a < argc; ++a) {
		if(wcscmp(argv[a], L"/h") == 0) {
			// Print help and exit
			wprintf(L"%s [options] /c [Process Name]\n", argv[0]);
			wprintf(L"Options:\n");
			wprintf(L"\t/v - Display progress info.\n");
			wprintf(L"\t/w - Wait for the created process to finish before exiting.\n");
			wprintf(L"\t/h - Display this help message.\n\n");
			wprintf(L"\t/c - Specify command/process to execute. If not specified, cmd starts by default.\n\n");

			goto cleanupandexit;
		} else if (wcscmp(argv[a], L"/v") == 0) {
			bVerbose = TRUE;
		} else if (wcscmp(argv[a], L"/w") == 0) {
			bWait = TRUE;
		} else if (wcscmp(argv[a], L"/c") == 0) {
			bCommandPresent = TRUE;
			break;
		}

		cOptions++;
	}

	if(bCommandPresent)
		wcscpy(lpwszNewProcessName, GetCommandLineArgs(argv, cOptions));
	else
		wcscpy(lpwszNewProcessName, DEFAULT_PROCESS);

	//Acquire SeDebugPrivilege
	{
		HANDLE hToken = OpenCurrentThreadToken();

		if(!SetPrivilege(hToken, SE_DEBUG_NAME, TRUE)) {
			CloseHandle(hToken);
			wprintf(L"Could not acquire SeDebugPrivilege.\n");

			iReturnStatus = 0xDEAD;
			goto cleanupandexit;
		}

		CloseHandle(hToken);
		VERB_PRINT(L"SeDebugPrivilege acquired.\r\n");
	}

	// Start the TrustedInstaller service.
	// Create child process for TI

	HANDLE hTIProcess = NULL;
	if(!CreateTrustedInstallerProcessHandle(&hTIProcess)){
		wprintf(L"Critical failure. Could not open the TI process.");

		iReturnStatus = 0xDAD;
		goto cleanupandexit;
	}

	// Initialize startupInfo.
	{
		ZeroMemory(&startupInfo, sizeof(STARTUPINFOEX));
		startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);

		startupInfo.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
		startupInfo.StartupInfo.wShowWindow = SW_SHOWNORMAL;
	}

	// Initialize Thread Attribute List for parent assignment.
	{
		size_t attributeListLength;

		InitializeProcThreadAttributeList(NULL, 1, 0, (PSIZE_T) &attributeListLength);

		startupInfo.lpAttributeList = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, attributeListLength);
		InitializeProcThreadAttributeList(startupInfo.lpAttributeList, 1, 0, (PSIZE_T) &attributeListLength);

		UpdateProcThreadAttribute(startupInfo.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, &hTIProcess, sizeof(HANDLE), NULL, NULL);
	}

	// Accomodate space for the process' information
	PROCESS_INFORMATION newProcInfo;
	ZeroMemory(&newProcInfo, sizeof(newProcInfo));

	wprintf(L"Creating process..\n");

	if(CreateProcessW(
			NULL,
			lpwszNewProcessName,
			NULL,
			NULL,
			FALSE,
			CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT | CREATE_NEW_CONSOLE,
			NULL,
			NULL,
			&startupInfo.StartupInfo,
			&newProcInfo)
	){
		DeleteProcThreadAttributeList(startupInfo.lpAttributeList);

		// Add all possible Token privileges for the created process.
		// Not acquiring some those privileges is not fatal, but will be reported to the user when verbose.

		HANDLE hProcessToken;
		if(OpenProcessToken(newProcInfo.hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hProcessToken)) {
			for(int i = 0; i < 35; i++)
				if(!SetPrivilege(hProcessToken, lplpcwszTokenPrivileges[i], TRUE))
					VERB_PRINT(L"Could not set %s privilege. Does your group have it?\n", lplpcwszTokenPrivileges[i]);
		} else {
			VERB_PRINT(L"Could not adjust token for additional privileges.\nError: 0x%08X\n", GetLastError());
		}

		wprintf(L"Created Process ID = %ld\r\n", newProcInfo.dwProcessId);

		ResumeThread(newProcInfo.hThread);

		// If waiting to finish.
		if(bWait) {
			DWORD dwExitCode = 0;
			WaitForSingleObject(newProcInfo.hProcess, INFINITE);

			GetExitCodeThread(newProcInfo.hThread, &dwExitCode);
			VERB_PRINT(L"Main thread exit code: 0x%X\n", dwExitCode);
		}
		// Free unneeded handles from newProcInfo.
		CloseHandle(newProcInfo.hProcess);
		CloseHandle(newProcInfo.hThread);

	} else {
		//It will happen when an incorrect process name is specified. (Will exit with error 0x00000002 - File not found)
		wprintf(L"Process Creation Failed.\r\n\tError code: 0x%X\r\n", GetLastError());
		iReturnStatus = 0xDEDDED;
	}

	cleanupandexit:

	if(hTIProcess)
		CloseHandle(hTIProcess);

	return iReturnStatus;
}

/*
 * Used for enabling/disabling a privilege for a given token.
 * Will return false in case of failure.
 * Most failures occur when the user's group does not have a given privilege.
 * 
 * See the comments in the header's array of privileges for examples of tokens most users won't have.
 */
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

/*
 * A messy way to acquire a valid commandline string from the argv array excluding the beginning arguments.
 */
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

/* 
 * Returns the current thread's token for adding the SeDebugPrivilege. 
 */
HANDLE OpenCurrentThreadToken() {
	HANDLE hToken;

	if(!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken))
		if(GetLastError() == ERROR_NO_TOKEN) {
			ImpersonateSelf(SecurityImpersonation);
			OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken);
		}

	return hToken;
}

/* 
 * Handles acquiring the TI Process handle. 
 */
BOOL CreateTrustedInstallerProcessHandle(HANDLE *lpProcessHandle) {
	HANDLE hSCManager, hTIService, hTIProcess;
	SERVICE_STATUS_PROCESS lpServiceStatusBuffer = { 0 };

	unsigned long ulBytesNeeded;

	hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE | SC_MANAGER_CONNECT);
	hTIService = OpenServiceW(hSCManager, L"TrustedInstaller", SERVICE_START | SERVICE_QUERY_STATUS);

	if(hTIService == NULL || hSCManager == NULL)
		return FALSE;

	DWORD dwServiceStatus = 0;
	do {
		QueryServiceStatusEx(hTIService, SC_STATUS_PROCESS_INFO, (BYTE *) &lpServiceStatusBuffer, sizeof(SERVICE_STATUS_PROCESS), &ulBytesNeeded);

		dwServiceStatus = lpServiceStatusBuffer.dwCurrentState;
		if(dwServiceStatus == SERVICE_STOPPED)
			if(!StartService(hTIService, 0, NULL))
				return FALSE;

	} while (dwServiceStatus == SERVICE_STOPPED);

	// Get process handle.
	hTIProcess = OpenProcess(PROCESS_CREATE_PROCESS, FALSE, lpServiceStatusBuffer.dwProcessId);

	if(lpProcessHandle != NULL && hTIProcess != NULL)
		*lpProcessHandle = hTIProcess;
	else
		return FALSE;

	CloseServiceHandle(hSCManager);
	CloseServiceHandle(hTIService);

	return TRUE;
}