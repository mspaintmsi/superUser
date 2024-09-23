# About
_superUser_ is a simple and lightweight utility to start any process as the System user with Trusted Installer privileges.

# How It Works
The program acquires the TrustedInstaller process' access token and creates a new (user-specified) process as the System user with Trusted Installer privileges using this token.

# Usage
There are two ways to run the program:

## From the File Explorer
Double-click the executable, grant administrator privileges and wait for a command prompt to appear.

## From the Command Prompt
Simply run _superUser_ from the command prompt (preferably one with administrator privileges) using the following arguments:

__`superUser [options] [command_to_run]`__


`command_to_run` is the command line used to create the new process. It is a filename followed by arguments. If not specified, `cmd.exe` is started.

This filename can be:
- An executable name (the _.exe_ extension can be omitted).
- A batch name (_.cmd_ or _.bat_).


### Options

| Option |                           Meaning                           |
|:------:|-------------------------------------------------------------|
|   /h   | Display the help message.                                   |
|   /r   | Return the exit code of the child process. Requires /w.     |
|   /s   | The child process shares the parent's console. Requires /w. |
|   /v   | Display verbose messages with progress information.         |
|   /w   | Wait for the child process to finish. Used for scripts.     |

- You can also use a dash (-) in place of a slash (/) in front of an option.
- Multiple options can be grouped together (e.g., `/wrs`).


### Notes

The `/wrs` options allow you to run a process in a completely transparent way:

- The new process runs in the same window and performs its inputs and outputs there.
- The exit code of the new process is returned and you can retrieve it with the errorlevel variable.


### Examples

	superUser64 /ws whoami /user
	superUser64 /ws whoami /groups | find "TrustedInstaller"
	superUser64 /wr my_script.cmd arg1 arg2


## Exit Codes

| Exit Code |                        Meaning                         |
|:---------:|--------------------------------------------------------|
|     1     | Invalid argument.                                      |
|     2     | Failed to acquire SeDebugPrivilege.                    |
|     3     | Failed to open/start TrustedInstaller process/service. |
|     4     | Process creation failed (prints error code).           |
|     5     | Another fatal error occurred.                          |

If the `/r` option is specified, the exit code of the child process is returned.
If _superUser_ fails, it returns a code from -1000001 to -1000005 (e.g., -1000002 instead of 2).
