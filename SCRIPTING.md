
Scripting
=========

To run _superUser_ from a script and wait for the child process to exit, use the
`/w` option.

To retrieve the exit code of the completed process, additionally use the `/r`
option. It returns this code to standard output.

To copy it into a variable, there are several solutions.


Solution 1
----------

	set "exit_code="
	for /f %%c in ('superUser64 /wrc "my_process_name" params...  ') do set "exit_code=%%c"
	if defined exit_code (
		echo The child process exit code is "%exit_code%"
	) else echo superUser returned an error.


Replace `"my_process_name" params...` with the command line of your application.

Here, there is no possibility to know the exit code of _superUser_ itself, if an
error occurs.


Solution 2
----------

This method uses a temporary file.
Both the exit code of _superUser_ and the exit code of the child process can be obtained.


	setlocal EnableDelayedExpansion
	set "exit_code="
	superUser64 /wrc "my_process_name" params...  >test_main.tmp
	if errorlevel 1 (
		echo The superUser exit code is "%errorlevel%"
	) else (
		set /p exit_code=<test_main.tmp
		if defined exit_code (
			echo The child process exit code is "!exit_code!"
		) else echo An unknown error has occurred.
	)
	del test_main.tmp


Solution 3
----------

Like the previous method, but without a temporary file.


	set "exit_code="
	setlocal DisableDelayedExpansion
	for /f %%c in ('cmd /v:on /c
		"superUser64 /wrc "my_process_name" params...  ||echo #!errorlevel!"
		') do set "exit_code=%%c"
	if defined exit_code (
		if "%exit_code:~0,1%"=="#" (
			echo The superUser exit code is "%exit_code:~1%"
		) else (
			echo The child process exit code is "%exit_code%"
		)
	) else echo An unknown error has occurred.


If _superUser_ failed to start the process, it returns a non-zero code. The script
gets this code (from `errorlevel`), marks it with a `#` to know that it is a
_superUser_ code, and prints it on the standard output.

Otherwise, _superUser_ starts the process and waits for it to complete. It retrieves
its exit code and, if the `/r` option is used, prints it on standard output.

In all cases, the code is copied into the `exit_code` variable. The first character
is tested to know where it comes from. If this is a `#`, it is a _superUser_ exit code,
otherwise an exit code of the child process.


Testing
-------

To test that this works, it is convenient to use a batch file as a child process.
It is easy to modify its return code.

For example, create this script `test_process.cmd`:


	@echo off
	title %~nx0 - Called script

	set "exit_code=123"

	echo This process returns the code %exit_code%.
	echo(
	pause
	exit /b %exit_code%


Then create a calling batch `test_main.cmd`:


	@echo off
	title %~nx0 - Main script
	cd "%~dp0"
	echo Calling "test_process.cmd" script...
	echo(

	:: Solution 3
	set "exit_code="
	setlocal DisableDelayedExpansion
	for /f %%c in ('cmd /v:on /c
		"superUser64 /wrc "test_process.cmd"	  ||echo #!errorlevel!"
		') do set "exit_code=%%c"
	if defined exit_code (
		if "%exit_code:~0,1%"=="#" (
			echo The superUser exit code is "%exit_code:~1%"
		) else (
			echo The child process exit code is "%exit_code%"
		)
	) else echo An unknown error has occurred.

	echo(
	pause


Both scripts must be in the same directory as the `superUser64` executable.

Always run the `test_main.cmd` script "_As administrator_".

To test the case of a process start error, change the process name in
`test_main.cmd` to a non-existent file, for example `unknown_process.cmd`, and
see the result.

The output is as follows:


	Calling "test_process.cmd" script...

	The child process exit code is "123"


	Calling "unknown_process.cmd" script...

	[E] Process creation failed. Error code: 0x00000002
	The superUser exit code is "4"
