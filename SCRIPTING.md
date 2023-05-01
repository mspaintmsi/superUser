
Scripting
=========

To run _superUser_ from a script and wait for the child process to exit, use the
`/w` option.

To retrieve the exit code of the completed process, additionally use the `/r`
option. _superUser_ returns this code, unless it fails itself. In this case, it 
returns a code from -1000001 to -1000004 instead of the normal code from 1 to 4.


Examples
--------

This example returns the exit code of the child process (or the _superUser_ error 
code if it fails):

	superUser64 /wrc child_process.exe
	echo Exit code is: %errorlevel%


Returns only the _superUser_ exit code, not the child process exit code:

	superUser64 /wc child_process.exe
	echo superUser exit code is: %errorlevel%
