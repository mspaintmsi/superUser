@echo off
::
:: Check the SHA256 hash for the files listed in the SHA256SUMS file.
::
:: Missing files are ignored.
::
:: The format of the hash file is the same as the output of the GNU "sha256sum"
:: utility. It consists of lines of the form:
:: <sha256> *<filename>
::
:: The "*" means "binary mode", the only supported by this script (as opposed
:: to "text mode").
::
:: Equivalent to GNU "sha256sum -c --ignore-missing --warn --strict SHA256SUMS"
::
:: Error codes:
:: 	1:	Some lines of the hash file are incorrectly formatted.
:: 	2:	Some files could not be open/read.
:: 	3:	Some file hashes do not match.
:: 	4:	The hash file could not be found.
:: 	5:	The hash file is empty.
:: 	6:	The Windows utility "CertUtil.exe" could not be started.
::

set "hash_file_name=SHA256SUMS"

:: To ignore missing files, set "ignore_missing=1" otherwise "ignore_missing="
set "ignore_missing=1"

setlocal EnableDelayedExpansion
set "err_prefix=%~n0:"
set "interactive="
echo %cmdcmdline%| find /i "%~0" >nul
if not errorlevel 1 set "interactive=1"

echo(
if not exist "%hash_file_name%" (
	echo %err_prefix% Hash file could not be found ^("%hash_file_name%"^).>&2
	set "exit_code=4"
	goto end
)

set "files_found=0"
set "line_format_errors=0"
set "read_errors=0"
set "failures=0"
set "line_number=0"

:: Read hash file line by line
for /f "usebackq tokens=1*" %%i in ("%hash_file_name%") do (
	set /a line_number += 1

	set "file_hash=%%~i"
	set "token2=%%~j"

	:: Check line format and extract filename
	call :check_line_format
	if errorlevel 1 (
		set /a line_format_errors += 1
		echo %err_prefix% !error_msg! at line !line_number!, file "%hash_file_name%".>&2
	) else (
		set "do_check=1"
		if defined ignore_missing if not exist "!filename!" set "do_check="
		if defined do_check (
			set /a files_found += 1

			:: Read the file, compute and compare the hash
			call :check_file "!filename!" "!file_hash!"
			if errorlevel 3 (
				set "exit_code=6"
				goto end
			) else if errorlevel 2 (set /a failures += 1
			) else if errorlevel 1 set /a read_errors += 1
		)
	)
)

if %line_number% equ 0 (
	echo %err_prefix% Hash file is empty ^("%hash_file_name%"^).>&2
	set "exit_code=5"
	goto end
)

set "exit_code=0"

:: Display information or warning messages

: No files to check were found
if %files_found% equ 0 (
	echo %err_prefix% No files found.>&2
) else (
	if not "%line_format_errors%%read_errors%%failures%"=="000" echo(>&2

	:: Some lines are incorrectly formatted
	if %line_format_errors% neq 0 (
		if %line_format_errors% equ 1 (set "w=line is") else set "w=lines are"
		echo %err_prefix% WARNING: %line_format_errors% !w! incorrectly formatted.>&2
		set "exit_code=1"
	)
	:: Some files could not be open/read
	if %read_errors% neq 0 (
		if %read_errors% equ 1 (set "w=file") else set "w=files"
		echo %err_prefix% WARNING: %read_errors% !w! could not be open/read.>&2
		set "exit_code=2"
	)
	:: Some file hashes did not match
	if %failures% neq 0 (
		if %failures% equ 1 (set "w=checksum") else set "w=checksums"
		echo %err_prefix% WARNING: %failures% computed !w! did NOT match.>&2
		set "exit_code=3"
	)
)

:end
if defined interactive (
	echo(
	pause
)
exit /b %exit_code%


::
:: check_line_format
::
:: Check line format of hash file and extract filename.
::
:: Input: file_hash, token2
:: Output: filename, error_msg
::
:check_line_format

:: Is this a valid SHA256 hash?
set "err_pos="
call :check_hash "%file_hash%" err_pos
if errorlevel 1 (
	set "error_msg=Invalid SHA256 hash"
	if defined err_pos set "error_msg=!error_msg!, position %err_pos%"
	exit /b 1
)

:: Check first char of second token (* = binary mode)
if not "%token2:~0,1%"=="*" (
	:: The text mode is not supported by this script
	set "error_msg=No binary indicator "*""
	exit /b 2
)
:: Remove first char "*"
set "filename=%token2:~1%"
:: Remove leading and trailing spaces
call :trim filename

if "%filename%"=="" (
	set "error_msg=Missing filename"
	exit /b 3
)
exit /b 0


::
:: check_file <filename> <hash>
::
:: Read the file, compute and compare the hash.
::
:check_file
setlocal
set "file=%~1"
set "file_hash=%~2"
echo Checking "%file%" ...
if not exist "%file%" (
	echo FAILED: No such file or directory.
	exit /b 1
)
if exist "%file%\" (
	echo FAILED: This is a directory.
	exit /b 1
)
set "hash="
:: Empty file
for /f "delims=" %%f in ("%file%") do if %%~zf equ 0 (
	set "hash=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
)
if not defined hash (
	for /f "skip=1 delims=" %%h in (
		'certutil -hashfile "%file%" SHA256'
		) do if not defined hash set "hash=%%h"
)
:: CertUtil launch error
if not defined hash (
	echo %err_prefix% An error has occurred.>&2
	exit /b 3
)
echo(%hash%| find /i "CertUtil" >nul
if %errorlevel% equ 0 (
	echo FAILED: Unable to open/read the file.
	exit /b 1
)
set "hash=%hash: =%"
if /i not "%hash%"=="%file_hash%" (
	echo FAILED
	exit /b 2
)
echo OK
exit /b 0


::
:: check_hash <hash> <pos_var_name>
::
:: Check if <hash> is a valid SHA256 hash.
:: In case of error, its position is returned in the <pos_var_name> variable.
::
:check_hash
setlocal EnableDelayedExpansion
set "hash_length=64"
set "str=%~1"
for /l %%i in (0,1,%hash_length%) do (
	set "c=!str:~%%i,1!"
	if "!c!"=="" set "p=%%i" & goto _ch_endloop
	set /a "d=0x!c!" 2>nul
	if errorlevel 1 set "p=%%i" & goto _ch_error
)
set "p=%hash_length%"
:_ch_error
set /a p += 1
endLocal & set "%~2=%p%"
exit /b 1

:_ch_endloop
if %p% lss %hash_length% goto _ch_error
exit /b 0


::
:: trim <var_name>
:: Remove leading and trailing spaces in variable <var_name>
::
:trim
setLocal
call :trimSub %%%1%%
endLocal & set "%~1=%tempvar%"
exit /b
:trimSub
set "tempvar=%*"
exit /b
