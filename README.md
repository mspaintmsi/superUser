# About
_superUser_ is a simple and lightweight utility to start any process as the System user with Trusted Installer privileges.

Supported operating systems: Windows 11, 10, 8.1, 8, 7, Vista.

# How It Works
The program acquires the TrustedInstaller process' access token and creates a new (user-specified) process as the System user with Trusted Installer privileges using this token.

# Usage
There are two ways to run the program:

## From the File Explorer
Double-click the executable, grant administrator privileges and wait for a command prompt to appear.

## From the Command Prompt
Run _superUser_ from the command prompt opened __as administrator__, using the following arguments:

__`superUser [options] [command_to_run]`__


`command_to_run` is the command line used to create the new process. It is a filename followed by arguments. If not specified, `cmd.exe` is started.

This filename can be:
- An executable name (the _.exe_ extension can be omitted).
- A batch name (_.cmd_ or _.bat_).


### Options

| Option |                           Meaning                           |
|:------:|-------------------------------------------------------------|
|   /h   | Display the help message.                                   |
|   /m   | Minimize the created window.                                |
|   /s   | The child process shares the parent's console. Requires /w. |
|   /v   | Display verbose messages with progress information.         |
|   /w   | Wait for the child process to finish. Used for scripts.<br />Returns the exit code of the child process. |

- You can also use a dash (-) in place of a slash (/) in front of an option.
- Multiple options can be grouped together (e.g., `/ws` is equivalent to `/w /s`).


### Notes

The `/ws` options allow you to run a process in a completely transparent way:

- The new process runs in the same window and performs its inputs and outputs there.
- The exit code of the new process is returned and you can retrieve it with the errorlevel variable.


### Examples

Open a command prompt __as administrator__ to run these commands.

	superUser64 /ws whoami /user
	superUser64 /ws whoami /groups | find "TrustedInstaller"
	superUser64 /w my_script.cmd arg1 arg2


## Exit Codes

| Exit Code |                        Meaning                         |
|:---------:|--------------------------------------------------------|
|     1     | Invalid argument.                                      |
|     2     | Failed to acquire SeDebugPrivilege.                    |
|     3     | Failed to open/start TrustedInstaller process/service. |
|     4     | Process creation failed (prints error code).           |
|     5     | Another fatal error occurred.                          |

If the `/w` option is specified, the exit code of the child process is returned.
If _superUser_ fails, it returns a code from -1000001 to -1000005 (e.g., -1000002 instead of 2).


# sudo

`sudo32.exe` and `sudo64.exe` are simpler versions of superUser.

They are equivalent to `superUser /ws`.

- The child process runs in the same window and performs its inputs and outputs there.
- `sudo` waits for this process to finish and returns its exit code.

Usage is the same as superUser, except that the `s`, `v`, and `w` options do not exist.


### Examples

Open a command prompt __as administrator__ to run these commands.

	sudo64 whoami /user
	sudo64 whoami /groups | find "TrustedInstaller"
	sudo64 my_script.cmd arg1 arg2
