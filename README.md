# About
The superUser Project is a simple and lightweight way to start any process with TrustedInstaller privileges.

# How it works
This program does the following things: (In order)
* Acquires the `SeDebugPrivilege` for its own token.
The purpose of the `SeDebugPrivilege` is to allow the process to be able to acquire a `PROCESS_ALL_ACCESS` handle to any process regardless of its security descriptors.
* Opens a Service Manager handle to query the status of the `TrustedInstaller` service.
* Starts the `TrustedInstaller` service and acquires its handle with `PROCESS_ALL_ACCESS`.
* Creates the required `STARTUPINFOEX` structure containing the data on how the created process behaves.
* An attribute list for the process is created.
* The attribute list is filled to set the new process' parent to the `TrustedInstaller` service using the `PROC_THREAD_ATTRIBUTE_PARENT_PROCESS` attribute.
* Finally the process is created, its PID is printed and the main thread is resumed. (It was created suspended for possible future changes/fixes)

This method is (almost certainly) in no way inferior to the method which captures the TI token and creates a process with it. The acquired privileges are identical and, from what I've tested, there are no differences between them.

Please contact me if I'm wrong.

# Usage
There are two ways to run the program:

## From the File Explorer
Simply double click the executable, grant it administrator rights and a command prompt with TI privileges will start soon after.
In case of any problems use the second method of running the program to see the "debug" output.

## From the Command Prompt
Simply run the command prompt (preferably elevated to see the program output) in the folder containing the executable and type:

#### ```superUser "<process name>"```

It is important that you enclose the process name in brackets if it contains spaces, otherwise a command prompt will start.
## Exit Codes
| Exit Code  | Decimal  |  Meaning                                          |
|------------|----------|---------------------------------------------------|
| `0xDEAD`   | 57005    | Failed acquiring SeDebugPrivilege.<sup>(1)</sup>  |
| `0xDEDDED` | 14605805 | Process creation failed with printed error code.  |

<sup>(1)</sup> - Make sure you have administrative privileges and that your group has SeDebugPrivilege enabled.
