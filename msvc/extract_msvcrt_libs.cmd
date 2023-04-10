@echo off
::
:: Extracts msvcrt*.lib files from WDK 7.1 iso file.
::
:: - Requires the installation of 7-Zip (from https://7-zip.org ).
::   Please set the 7-Zip install directory below.
::
:: - The WDK 7.1 iso file must be present in the current directory:
::   GRMWDK_EN_7600_1.ISO
::   	or (same file with another name)
::   en_windows_driver_kit_version_7.1.0_x86_x64_ia64_dvd_496758.iso
::

:: *** 7-Zip install directory ***
set "seven_zip_dir=C:\Program Files\7-Zip"

set "wdk_filename=GRMWDK_EN_7600_1.ISO"
set "wdk_sha256=5edc723b50ea28a070cad361dd0927df402b7a861a036bbcf11d27ebba77657d"
set "wdk_filename2=en_windows_driver_kit_version_7.1.0_x86_x64_ia64_dvd_496758.iso"

set "cab_x86=libs_x86fre_cab001.cab"
set "cab_x64=libs_x64fre_cab001.cab"
set "lib_x86=msvcrt32.lib"
set "lib_x64=msvcrt64.lib"

set "exit_code=10"

if not exist "%wdk_filename%" if exist "%wdk_filename2%" set "wdk_filename=%wdk_filename2%"
set "seven_zip_dir2=C:\Program Files (x86)\7-Zip"
if not exist "%seven_zip_dir%\7z.exe" if exist "%seven_zip_dir2%\7z.exe" (
	set "seven_zip_dir=%seven_zip_dir2%"
)
set "seven_zip=%seven_zip_dir%\7z.exe"

echo(
for %%# in ("%wdk_filename%" "%seven_zip%" "%seven_zip_dir%\7z.dll") do if not exist "%%~#" (
	echo Required file "%%~#" is missing.
	set "exit_code=1"
	goto end
)

:: Check WDK file
call :check_file "%wdk_filename%" "%wdk_sha256%"
if errorlevel 1 (
	set "exit_code=2"
	goto end
)
echo(

echo Extracting %cab_x86% and %cab_x64% ...
"%seven_zip%" e -aoa "%wdk_filename%" "WDK\%cab_x86%" "WDK\%cab_x64%" >nul
if errorlevel 1 goto error
echo(

echo Extracting %lib_x86% ...
"%seven_zip%" e -aoa "%cab_x86%" _msvcrt.lib_00025 >nul
if errorlevel 1 goto error
if exist "%lib_x86%" del "%lib_x86%"
if exist "%lib_x86%" goto error
ren _msvcrt.lib_00025 "%lib_x86%"
if errorlevel 1 goto error
del "%cab_x86%" >nul 2>&1
echo(

echo Extracting %lib_x64% ...
"%seven_zip%" e -aoa "%cab_x64%" _msvcrt.lib_00024 >nul
if errorlevel 1 goto error
if exist "%lib_x64%" del "%lib_x64%"
if exist "%lib_x64%" goto error
ren _msvcrt.lib_00024 "%lib_x64%"
if errorlevel 1 goto error
del "%cab_x64%" >nul 2>&1
echo(

echo Done.
set "exit_code=0"

:end
echo(
pause
exit /b %exit_code%

:error
echo An error has occurred.
goto end


::
:: Read the file and compute the hash.
::
:check_file
setlocal
set "file=%~1"
set "file_hash=%~2"
echo Checking "%file%" ...
set "hash="
:: Empty file
for /f %%f in ("%file%") do if %%~zf equ 0 (
	set "hash=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
)
if not defined hash (
	for /f "skip=1 delims=" %%h in (
		'certutil -hashfile "%file%" SHA256'
		) do if not defined hash set "hash=%%h"
)
:: CertUtil start error
if not defined hash (
	echo An error has occurred.
	exit /b 3
)
echo(%hash%| find /i "CertUtil" >nul
if %errorlevel% equ 0 (
	echo FAILED: Unable to open/read the file.
	exit /b 2
)
set "hash=%hash: =%"
if /i not "%hash%"=="%file_hash%" (
	echo SHA256 hash does not match:
	echo "%hash%"
	echo instead of:
	echo "%file_hash%"
	exit /b 1
)
exit /b 0
