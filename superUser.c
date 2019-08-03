#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include <processthreadsapi.h>
#include <winsvc.h>
#include <winbase.h>

#define PROC_THREAD_ATTRIBUTE_PARENT_PROCESS 0x20000

BOOL SetPrivilege(
	HANDLE hToken,
	LPCTSTR Privilege,
	BOOL bEnablePrivilege
	){
	
	TOKEN_PRIVILEGES tp;
	LUID luid;
	TOKEN_PRIVILEGES tpPrevious;
	DWORD cbPrevious=sizeof(TOKEN_PRIVILEGES);

	if(!LookupPrivilegeValue( NULL, Privilege, &luid )) return FALSE;

	tp.PrivilegeCount		   = 1;
	tp.Privileges[0].Luid	   = luid;
	tp.Privileges[0].Attributes = 0;

	AdjustTokenPrivileges(
			hToken,
			FALSE,
			&tp,
			sizeof(TOKEN_PRIVILEGES),
			&tpPrevious,
			&cbPrevious
			);

	if (GetLastError() != ERROR_SUCCESS) return FALSE;

	tpPrevious.PrivilegeCount	   = 1;
	tpPrevious.Privileges[0].Luid   = luid;

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
			NULL
			);

	if (GetLastError() != ERROR_SUCCESS) return FALSE;

	return TRUE;
}

FARPROC GetProcedureFromLibrary(char *module, char *procedure) {
	HMODULE mod = GetModuleHandle(module);
	FARPROC pProc = GetProcAddress(mod, procedure);
	
	return pProc;
}

//For some reason I don't have it in the 
typedef struct _STARTUPINFOEX {
	STARTUPINFO StartupInfo;
	LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList;
} STARTUPINFOEX, *LPSTARTUPINFOEX;

int main(int argc, char **argv) {
	
	SC_HANDLE hSCManager;
	SC_HANDLE hTIService;
	HANDLE hTIProcess;
	SERVICE_STATUS_PROCESS lpServiceStatusBuffer = { 0 };
	ULONG ulBytesNeeded = 0;
	HANDLE hToken;
	
	STARTUPINFOEX startupInfo;
	size_t attributeListLength;
	
	FARPROC InitializeProcThreadAttributeList_D = GetProcedureFromLibrary("kernelbase.dll", "InitializeProcThreadAttributeList");
	FARPROC UpdateProcThreadAttribute_D = GetProcedureFromLibrary("kernelbase.dll", "UpdateProcThreadAttribute");

	PROCESS_INFORMATION newProcInfo;
	
	//Input check.
	if(argc != 2)
		argv[1] = "C:\\Windows\\System32\\cmd.exe";
	if(argv[1][0] == '\0')
		argv[1] = "C:\\Windows\\System32\\cmd.exe";
	
	//Acquire SeDebugPrivilege
	if(!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken)) {
		if (GetLastError() == ERROR_NO_TOKEN) {
			ImpersonateSelf(SecurityImpersonation);
			OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken);		
		}
	}

	if(!SetPrivilege(hToken, SE_DEBUG_NAME, TRUE)) {
		CloseHandle(hToken);
		printf("Failed acquiring the SeDebugPrivilege.\r\n");
		return -5;
	} else {
		printf("SeDebugPrivilege acquired.\r\n");
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
			printf("Started the TrustedInstaller service.\r\n");
		} else {
			printf("Failed starting the TrustedInstaller service.\r\n");
			goto cleanupandexit;
		}
	}
	
	//With SeDebugPrivilege we can successfully get any handle with PROCESS_ALL_ACCESS, but in this case we need to make sure the service is actually running.
	hTIProcess = OpenProcess(PROCESS_ALL_ACCESS, 1, lpServiceStatusBuffer.dwProcessId);
	if(hTIProcess == NULL) {
		printf("Failed opening TrustedInstaller process. Retrying..\r\n\tError code: 0x%08lX\r\n", GetLastError());
		goto queryService;
	} else {
		printf("TrustedInstaller process handle acquired.\r\n");
	}
	
	//Create the child process. 

	ZeroMemory(&startupInfo, sizeof(STARTUPINFOEX));
	startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);
	startupInfo.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	startupInfo.StartupInfo.wShowWindow = SW_SHOWNORMAL;
		
	
	InitializeProcThreadAttributeList_D(NULL, 1, 0, &attributeListLength);	
	
	startupInfo.lpAttributeList = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, attributeListLength);

	InitializeProcThreadAttributeList_D(startupInfo.lpAttributeList, 1, 0, &attributeListLength);
	UpdateProcThreadAttribute_D(startupInfo.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, &hTIProcess, sizeof(HANDLE), NULL, NULL);
	
	printf("Creating specified process.\r\n");
	
	ZeroMemory(&newProcInfo, sizeof(newProcInfo));
	
	if(CreateProcess(
			NULL,
			argv[1], //Not sanitized, shouldn't matter.
			NULL,
			NULL,
			FALSE,
			CREATE_SUSPENDED | CREATE_NEW_CONSOLE | EXTENDED_STARTUPINFO_PRESENT,
			NULL,
			NULL,
			&startupInfo.StartupInfo,
			&newProcInfo)
	){
		
		printf("Created Process ID = %ld\r\nResuming main thread.\r\n", newProcInfo.dwProcessId);
		ResumeThread(newProcInfo.hThread);
		//There is no need to start it suspended, but it can be used to set properties before the window is shown.
		//Maybe in the future it'll be used.
		
		// Free unneeded handles from newProcInfo.
		CloseHandle(newProcInfo.hProcess);
		CloseHandle(newProcInfo.hThread);
		
	} else {
		//It will happen when an incorrect process name is specified. (Will exit with error 0x00000002 - File not found)
		printf("Process Creation Failed.\r\n\tError code: 0x%08lX\r\n", GetLastError());
	}

	cleanupandexit:
	
	if(hSCManager)
		CloseServiceHandle(hSCManager);
	
	if(hTIService)
		CloseServiceHandle(hTIService);
	
	if(hTIProcess)
		CloseHandle(hTIProcess);
	
	return 0;
}