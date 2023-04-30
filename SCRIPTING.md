
Scripting
=========

To run _superUser_ from a script and wait for the child process to exit, use the
`/w` option.

To retrieve the exit code of the completed process, additionally use the `/r`
option. _superUser_ returns this code, unless it fails itself. In this case, it 
returns a code from -1000001 to -1000004 instead of the normal code from 1 to 4.

By default, if the child process is a console application, a new console is 
created for it (in a new window). If you specify the `/s` option, the child process
shares the parent's console (in the same window).

If the child process is a GUI application, the `/s` option has no effect.


Examples
--------

This example returns the exit code of the child process (or the _superUser_ error 
code if it fails):

	superUser64 /wrc child_process.exe
	echo Exit code is: %errorlevel%


As above, but the child process, if it is a console application, shares the 
parent's console window:

	superUser64 /wrsc child_process.exe


Returns only the superUser exit code, not the child process exit code:

	superUser64 /wc child_process.exe
	echo superUser exit code is: %errorlevel%
