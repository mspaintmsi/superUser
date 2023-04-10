@echo off
::
:: Checks the SHA256 hash for the files listed in the SHA256SUMS file.
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
:: Equivalent to GNU "sha256sum -c --ignore-missing SHA256SUMS"
::

set "hash_file_name=SHA256SUMS"

setlocal EnableDelayedExpansion

echo.
if not exist "%hash_file_name%" (
	echo Hash file cannot be found ^("%hash_file_name%"^).
	set "exit_code=3"
	goto end
)

set "files_found=0"
set "read_errors=0"
set "failures=0"
set "line_number=0"

for /f "usebackq tokens=1,2" %%i in ("%hash_file_name%") do (
	set /a line_number += 1
	set "param2=%%~j"

	:: Check first char of second parameter (* = binary mode)
	if not "!param2:~0,1!"=="*" (
		echo Missing binary indicator "*" at line !line_number!, hash file "%hash_file_name%".
		echo Text mode is not supported by this script.
		set "exit_code=5"
		goto end
	)
	:: Remove first char "*"
	set "filename=!param2:~1!"

	if "!filename!"=="" (
		echo Missing filename at line !line_number!, hash file "%hash_file_name%".
		set "exit_code=6"
		goto end
	)
	if exist "!filename!" (
		set /a files_found += 1

		:: Check the file hash
		call :check_file "!filename!" "%%~i"
		if errorlevel 3 (
			set "exit_code=7"
			goto end
		) else if errorlevel 2 (set /a read_errors += 1
		) else if errorlevel 1 set /a failures += 1
	)
)

if %line_number% equ 0 (
	echo Hash file is empty ^("%hash_file_name%"^).
	set "exit_code=4"
	goto end
)

set "exit_code=0"
if %files_found% equ 0 (
	echo No files found.
) else (
	if not "%read_errors%%failures%"=="00" echo.
	if %read_errors% neq 0 (
		echo WARNING: %read_errors% file^(s^) could not be open/read.
		set "exit_code=1"
	)
	if %failures% neq 0 (
		echo WARNING: %failures% computed checksum^(s^) did NOT match.
		set "exit_code=2"
	)
)

:end
echo.
pause
exit /b %exit_code%


::
:: Read the file and compute the hash.
::
:check_file
setlocal
set "file=%~1"
set "file_hash=%~2"
echo Checking "%file%" ...
set "hash="
for /f "skip=1 delims=" %%h in (
	'certutil -hashfile "%file%" SHA256'
	) do if not defined hash set "hash=%%h"

:: CertUtil start error
if not defined hash (
	echo An error has occurred.
	exit /b 3
)
echo.%hash% | find /i "CertUtil" >nul
if %errorlevel% equ 0 (
	echo FAILED: Unable to open/read the file.
	exit /b 2
)
set "hash=%hash: =%"
if not "%hash%"=="%file_hash%" (
	echo FAILED
	exit /b 1
)
echo OK
exit /b 0
