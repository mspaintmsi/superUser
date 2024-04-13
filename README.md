# About
_superUser_ is a simple and lightweight utility to start any process with TrustedInstaller privileges.

# How it Works
The program acquires the Trusted Installer's Process' access token and creates a new (user-specified) process as Trusted Installer using this token.

# Usage
There are two ways to run the program:

## From the File Explorer
Double click the executable, grant admin privileges and wait for a command prompt to appear.

## From the Command Prompt
Simply run _superUser_ from the command prompt (preferably one with admin privileges) using the following arguments:

#### ```superUser [options] [command_to_run]```

|  Option |                         Meaning                                 |
|:-------:|-----------------------------------------------------------------|
|   /h    | Display the help message.                                       |
|   /r    | Return the exit code of the child process. Requires /w.         |
|   /s    | Child process shares parent's console. Requires /w.             |
|   /v    | Verbose. Display progress info.                                 |
|   /w    | Wait for the created process to exit. Used for scripts.         |

Notes:
- You can also use a dash (-) in place of a slash (/) in front of an option.
- Multiple options can be grouped together (e.g., `/wrs`).
- `command_to_run` is the filename of an executable (.exe) or script (.cmd),
followed by parameters. If not specified, `cmd.exe` is started.
- The /s option only works in superUser 5.5, on Windows Vista, 7, 8, 10 and 11 up to 23H2.


## Exit Codes

| Exit Code |                      Meaning                       |
|:---------:|----------------------------------------------------|
|     1     | Invalid argument.                                  |
|     2     | Failed acquiring SeDebugPrivilege.                 |
|     3     | Could not open/start the TrustedInstaller service. |
|     4     | Process creation failed (prints error code).       |
|     5     | Another fatal error occurred.                      |

If the `/r` option is specified, the exit code of the child process is returned.
If _superUser_ fails, it returns a code from -1000001 to -1000005 (e.g., -1000002 instead of 2).
