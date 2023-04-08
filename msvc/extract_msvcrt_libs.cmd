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

echo.
for %%# in ("%wdk_filename%" "%seven_zip%" "%seven_zip_dir%\7z.dll") do if not exist "%%~#" (
echo Required file "%%~#" is missing.
set "exit_code=1"
goto end
)

echo Checking "%wdk_filename%" ...
for /f "delims=" %%h in (
'certutil -hashfile "%wdk_filename%" SHA256 ^| find /v "SHA256" ^| find /v "CertUtil"'
) do set "hash=%%h"
set "hash=%hash: =%"
if not "%hash%"=="%wdk_sha256%" (
echo SHA256 hash does not match for "%wdk_filename%":
echo "%hash%"
echo instead of:
echo "%wdk_sha256%"
set "exit_code=2"
goto end
)
echo.

echo Extracting %cab_x86% and %cab_x64% ...
"%seven_zip%" e -aoa "%wdk_filename%" "WDK\%cab_x86%" "WDK\%cab_x64%" >nul
if errorlevel 1 goto error
echo.

echo Extracting %lib_x86% ...
"%seven_zip%" e -aoa "%cab_x86%" _msvcrt.lib_00025 >nul
if errorlevel 1 goto error
if exist "%lib_x86%" del "%lib_x86%"
ren _msvcrt.lib_00025 "%lib_x86%"
if errorlevel 1 goto error
del "%cab_x86%" >nul 2>&1
echo.

echo Extracting %lib_x64% ...
"%seven_zip%" e -aoa "%cab_x64%" _msvcrt.lib_00024 >nul
if errorlevel 1 goto error
if exist "%lib_x64%" del "%lib_x64%"
ren _msvcrt.lib_00024 "%lib_x64%"
if errorlevel 1 goto error
del "%cab_x64%" >nul 2>&1
echo.

echo Done.
set "exit_code=0"

:end
echo.
pause
exit /b %exit_code%

:error
echo An error has occurred.
goto end
