# About
superUser is a simple and lightweight utility to start any process with TrustedInstaller privileges.

# How it Works
The program acquires the Trusted Installer's Process' handle and creates a new (user-specified) process attributing Trusted Installer as its parent.

# Usage
There are two ways to run the program:

## From the File Explorer
Double click the executable, grant admin privileges and wait for a command prompt to appear.

## From the Command Prompt
Simply run superUser from the command prompt (preferably one with admin privileges) using the following arguments:

#### ```superUser [options] /c <process name>```
|  Option |  Meaning                                                        |
|---------|-----------------------------------------------------------------|
| /c      | Used to specify the command to run. Without it, cmd is started. |
| /h      | Display the help message.                                       |
| /r      | Return the exit code of the child process. Requires /w.         |
| /s      | Child process shares the parent's console.                      |
| /v      | Verbose. Display progress info.                                 |
| /w      | Wait for the created process to exit. Used for scripts.         |

Notes: You can also use a dash (-) in place of the slash (/) in command.  
Multiple options can be grouped together, the c option last (e.g., `/wrc`).

## Exit Codes
| Exit Code |                      Meaning                      |
|:---------:|:-------------------------------------------------:|
|     1     | Invalid argument                                  |
|     2     | Failed acquiring SeDebugPrivilege                 |
|     3     | Could not open/start the TrustedInstaller service |
|     4     | Process creation failed (prints error code)       |

If the `/r` option is specified, the exit code of the child process is returned.
If superUser fails, it returns a code from -1000001 to -1000004 (e.g., -1000002 instead of 2).
