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
| /h      | Display the help message.                                       |
| /r      | Return process exit code to the standard output. Requires /w.   |
| /v      | Verbose. Display progress info.                                 |
| /w      | Wait for the created process to exit. Used for scripts.         |
| /c      | Used to specify the command to run. Without it, cmd is started. |

Notes: You can also use a dash (-) in place of the slash (/) in command.
Multiple options can be grouped together, option c last (eg: /wrc).

## Exit Codes
| Exit Code |                      Meaning                      |
|:---------:|:-------------------------------------------------:|
|     1     | Invalid argument                                  |
|     2     | Failed acquiring SeDebugPrivilege                 |
|     3     | Could not open/start the TrustedInstaller service |
|     4     | Process creation failed (prints error code)       |