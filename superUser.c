#define _WIN32_WINNT _WIN32_WINNT_VISTA
#define _UNICODE

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include <processthreadsapi.h>
#include <winsvc.h>
#include <winbase.h>

// In some cases this is not defined. Not sure why.
#define SE_DELEGATE_SESSION_USER_IMPERSONATE_NAME TEXT("SeDelegateSessionUserImpersonatePrivilege")

const wchar_t *lplpcwszTokenPrivileges[35] = {
	SE_ASSIGNPRIMARYTOKEN_NAME,
	SE_AUDIT_NAME,
	SE_BACKUP_NAME,
	SE_CHANGE_NOTIFY_NAME,
	SE_CREATE_GLOBAL_NAME,
	SE_CREATE_PAGEFILE_NAME,
	SE_CREATE_PERMANENT_NAME,
	SE_CREATE_SYMBOLIC_LINK_NAME,
	SE_CREATE_TOKEN_NAME,
	SE_DEBUG_NAME,
	SE_DELEGATE_SESSION_USER_IMPERSONATE_NAME,
	SE_ENABLE_DELEGATION_NAME,
	SE_IMPERSONATE_NAME,
	SE_INC_BASE_PRIORITY_NAME,
	SE_INCREASE_QUOTA_NAME,
	SE_LOAD_DRIVER_NAME,
	SE_LOCK_MEMORY_NAME,
	SE_MACHINE_ACCOUNT_NAME,
	SE_MANAGE_VOLUME_NAME,
	SE_PROF_SINGLE_PROCESS_NAME,
	SE_RELABEL_NAME,
	SE_REMOTE_SHUTDOWN_NAME,
	SE_RESTORE_NAME,
	SE_SECURITY_NAME,
	SE_SHUTDOWN_NAME,
	SE_SYNC_AGENT_NAME,
	SE_SYSTEM_ENVIRONMENT_NAME,
	SE_SYSTEM_PROFILE_NAME,
	SE_SYSTEMTIME_NAME,
	SE_TAKE_OWNERSHIP_NAME,
	SE_TCB_NAME,
	SE_TIME_ZONE_NAME,
	SE_TRUSTED_CREDMAN_ACCESS_NAME,
	SE_UNDOCK_NAME,
	SE_UNSOLICITED_INPUT_NAME
};

#define DEFAULT_PROCESS L"cmd.exe"

// I couldn't seem to find a constant in the headers for this.
#define MAX_COMMANDLINE 8192

// wide char printf to be used when the /v option is used.
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

	BOOLEAN bVerbose = FALSE, bWait = FALSE, bCommandPresent = FALSE;
	wchar_t lpwszNewProcessName[MAX_COMMANDLINE];

	STARTUPINFOEX startupInfo;
	size_t attributeListLength;

	PROCESS_INFORMATION newProcInfo;

	int returnStatus = 0;

	int cOptions = 1;
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
	if(!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken))
		if (GetLastError() == ERROR_NO_TOKEN) {
			ImpersonateSelf(SecurityImpersonation);
			OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken);
		}

	if(!SetPrivilege(hToken, SE_DEBUG_NAME, TRUE)) {
		CloseHandle(hToken);
		VERB_PRINT(L"Failed acquiring the SeDebugPrivilege.\r\n");

		returnStatus = 0xDEAD;
		goto cleanupandexit;
	}
	VERB_PRINT(L"SeDebugPrivilege acquired.\r\n");

	hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE | SC_MANAGER_CONNECT );
	hTIService = OpenServiceW(hSCManager, L"TrustedInstaller", SERVICE_START | SERVICE_QUERY_STATUS);

	DWORD dwServiceState;
	do {
		QueryServiceStatusEx(hTIService, SC_STATUS_PROCESS_INFO, (BYTE *) &lpServiceStatusBuffer, sizeof(SERVICE_STATUS_PROCESS), &ulBytesNeeded);
		dwServiceState = lpServiceStatusBuffer.dwCurrentState;

		if(dwServiceState == SERVICE_STOPPED)
			if(StartService(hTIService, 0, NULL))
				VERB_PRINT(L"Started the TI service.\n");

	} while (dwServiceState == SERVICE_STOPPED);

	// Create child process for TI

	// Initialize startupInfo.
	ZeroMemory(&startupInfo, sizeof(STARTUPINFOEX));
	startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);

	// Makes the window appear normally, as in some cases it can start minimized.
	startupInfo.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	startupInfo.StartupInfo.wShowWindow = SW_SHOWNORMAL;

	// Initialize Thred Attribute List for parent assignment.
	InitializeProcThreadAttributeList(NULL, 1, 0, (PSIZE_T) &attributeListLength);
	startupInfo.lpAttributeList = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, attributeListLength);
	InitializeProcThreadAttributeList(startupInfo.lpAttributeList, 1, 0, (PSIZE_T) &attributeListLength);

	// Add the parent process.
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

		// Add all possible Token privileges for the created process.

		// Not acquiring those privileges is not fatal, but will be reported to the user when verbose.
		HANDLE hProcessToken;
		if(OpenProcessToken(newProcInfo.hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hProcessToken)) {
			for(int i = 0; i < 35; i++)
				if(!SetPrivilege(hProcessToken, lplpcwszTokenPrivileges[i], TRUE))
					VERB_PRINT(L"Could not set %s privilege. Does your group have it?\n", lplpcwszTokenPrivileges[i]);
		} else {
			VERB_PRINT(L"Could not adjust token for additional privileges.\nError: 0x%08X\nToken: 0x%08X\n", GetLastError());
		}

		ResumeThread(newProcInfo.hThread);

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
