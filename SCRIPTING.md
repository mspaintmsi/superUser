
Scripting
=========

To run _superUser_ from a script and wait for the child process to exit, use the
`/w` option.

To retrieve the exit code of the completed process, additionally use the `/r`
option. _superUser_ returns this code, unless it fails itself. In this case, it 
returns a code from -1000001 to -1000005 instead of the normal code from 1 to 5.

By default, the child process creates a new console (window). If you specify the
`/s` option, it shares the console of the parent. In other words, it doesn't open
another window. This is useful for running commands or small scripts.

The `/wrs` options allow you to run a process in a completely transparent way:

- The new process runs in the same window and performs its inputs and outputs there.
- The exit code of the new process is returned and you can retrieve it with the errorlevel variable.

Examples
--------

This example returns the exit code of the child process (or the _superUser_ error 
code if it fails):

	superUser64 /wr child_process.exe
	echo Exit code is: %errorlevel%


As above, but the process uses the current console:

	superUser64 /wrs child_process.exe
	echo Exit code is: %errorlevel%


Returns only the _superUser_ exit code, not the child process exit code:

	superUser64 /w child_process.exe
	echo superUser exit code is: %errorlevel%
