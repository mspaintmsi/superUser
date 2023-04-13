@echo off
::
:: Extract msvcrt*.lib files from WDK 7.1 iso file.
::
:: - Requires 7-Zip to be installed (from https://7-zip.org ), or just the
::   two files 7z.exe and 7z.dll.
::   Please set the 7-Zip install directory below if it is not the default one.
::
:: - The WDK 7.1 iso file must be present in the current directory:
::   GRMWDK_EN_7600_1.ISO
::   	or (same file with another name)
::   en_windows_driver_kit_version_7.1.0_x86_x64_ia64_dvd_496758.iso
::
:: Error codes:
:: 	1:	A required file is missing.
:: 	2:	The WDK file could not be open/read.
:: 	3:	The WDK file hash does not match.
:: 	4:	The Windows utility "CertUtil.exe" could not be started.
:: 	10:	Another error has occured.
::

:: *** Custom 7-Zip install directory ***
set "seven_zip_dir=%ProgramFiles%\7-Zip"

set "wdk_filename=GRMWDK_EN_7600_1.ISO"
set "wdk_sha256=5edc723b50ea28a070cad361dd0927df402b7a861a036bbcf11d27ebba77657d"
set "wdk_filename2=en_windows_driver_kit_version_7.1.0_x86_x64_ia64_dvd_496758.iso"

set "cab_x86=libs_x86fre_cab001.cab"
set "cab_x64=libs_x64fre_cab001.cab"
set "lib_tmp_x86=_msvcrt.lib_00025"
set "lib_tmp_x64=_msvcrt.lib_00024"
set "lib_x86=msvcrt32.lib"
set "lib_x64=msvcrt64.lib"

set "err_prefix=%~n0:"
set "interactive="
echo %cmdcmdline%| find /i "%~0" >nul
if not errorlevel 1 set "interactive=1"

set "exit_code=10"

if not exist "%wdk_filename%" if exist "%wdk_filename2%" (
	set "wdk_filename=%wdk_filename2%"
)
:: Search 7-Zip in alternate locations
if not exist "%seven_zip_dir%\7z.exe" for %%d in (
	"%ProgramFiles%\7-Zip" "%ProgramFiles(x86)%\7-Zip" "."
	) do if exist "%%~d\7z.exe" set "seven_zip_dir=%%~d"
set "seven_zip=%seven_zip_dir%\7z.exe"

echo(

:: Check required files
for %%f in ("%wdk_filename%" "%seven_zip%" "%seven_zip_dir%\7z.dll"
	) do if not exist "%%~f" (
	echo %err_prefix% Required file "%%~f" is missing.>&2
	set "exit_code=1"
	goto end
)

:: Check WDK file
call :check_file "%wdk_filename%" "%wdk_sha256%"
if errorlevel 1 (
	set /a "exit_code = %errorlevel% + 1"
	goto end
)
echo(

:: Clean old files to recreate
for %%f in ("%cab_x86%" "%cab_x64%" "%lib_tmp_x86%" "%lib_tmp_x64%"
	"%lib_x86%" "%lib_x64%"
	) do call :remove_file "%%~f" || goto error

:: Extract CAB files from WDK iso file
echo Extracting %cab_x86% and %cab_x64% ...
"%seven_zip%" e -aoa "%wdk_filename%" "WDK\%cab_x86%" "WDK\%cab_x64%" >nul
if errorlevel 1 goto error
echo(

:: Extract LIB file from CAB file (32-bit)
echo Extracting %lib_x86% ...
"%seven_zip%" e -aoa "%cab_x86%" "%lib_tmp_x86%" >nul
if errorlevel 1 goto error
ren "%lib_tmp_x86%" "%lib_x86%"
if errorlevel 1 goto error
del "%cab_x86%" >nul 2>&1
echo(

:: Extract LIB file from CAB file (64-bit)
echo Extracting %lib_x64% ...
"%seven_zip%" e -aoa "%cab_x64%" "%lib_tmp_x64%" >nul
if errorlevel 1 goto error
ren "%lib_tmp_x64%" "%lib_x64%"
if errorlevel 1 goto error
del "%cab_x64%" >nul 2>&1
echo(

echo Done.
set "exit_code=0"

:end
if defined interactive (
	echo(
	pause
)
exit /b %exit_code%

:error
echo %err_prefix% An error has occurred.>&2
goto end


::
:: remove_file <filename>
::
:remove_file
if "%~1"=="" exit /b 2
if exist "%~1\" (
	echo %err_prefix% Conflict with existing directory "%~1".>&2
	exit /b 1
)
if exist "%~1" del "%~1"
if exist "%~1" exit /b 1
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
	echo FAILED: SHA256 hash does not match.
	exit /b 2
)
exit /b 0
