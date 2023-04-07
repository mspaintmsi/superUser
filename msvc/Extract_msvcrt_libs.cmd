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

set "seven_zip_dir=C:\Program Files\7-Zip"		:: 7-Zip install directory

set "wdk_filename=GRMWDK_EN_7600_1.ISO"
set "wdk_sha256=5edc723b50ea28a070cad361dd0927df402b7a861a036bbcf11d27ebba77657d"
set "wdk_filename2=en_windows_driver_kit_version_7.1.0_x86_x64_ia64_dvd_496758.iso"

set "cab_x86_filename=libs_x86fre_cab001.cab"
set "cab_x64_filename=libs_x64fre_cab001.cab"
set "x86_lib=msvcrt32.lib"
set "x64_lib=msvcrt64.lib"

if not exist "%wdk_filename%" if exist "%wdk_filename2%" set "wdk_filename=%wdk_filename2%"
if not exist "%seven_zip_dir%\7z.exe" if exist "C:\Program Files (x86)\7-Zip\7z.exe" (
set "seven_zip_dir=C:\Program Files (x86)\7-Zip"
)
set "seven_zip=%seven_zip_dir%\7z.exe"

echo.
for %%# in ("%wdk_filename%" "%seven_zip%" "%seven_zip_dir%\7z.dll") do if not exist "%%~#" (
echo Required file "%%~#" is missing.
goto :end
)

echo Checking "%wdk_filename%" ...
for /f "delims=" %%h in (
'certutil -hashfile "%wdk_filename%" SHA256 ^| find /v "SHA256" ^| find /v "CertUtil"'
) do set "hash=%%h"
set "hash=%hash: =%"
if not "%hash%"=="%wdk_sha256%" (
echo The SHA256 does not match for "%wdk_filename%":
echo "%hash%"
echo instead of:
echo "%wdk_sha256%"
goto :end
)
echo.

echo Extracting %cab_x86_filename% and %cab_x64_filename% ...
"%seven_zip%" e -aoa -y "%wdk_filename%" "WDK\%cab_x86_filename%" "WDK\%cab_x64_filename%" >nul
if errorlevel 1 goto error
echo.

echo Extracting %x86_lib% ...
"%seven_zip%" e -aoa -y "%cab_x86_filename%" _msvcrt.lib_00025 >nul
if errorlevel 1 goto error
if exist "%x86_lib%" del "%x86_lib%"
ren _msvcrt.lib_00025 "%x86_lib%"
if errorlevel 1 goto error
del "%cab_x86_filename%" >nul 2>&1
echo.

echo Extracting %x64_lib% ...
"%seven_zip%" e -aoa -y "%cab_x64_filename%" _msvcrt.lib_00024 >nul
if errorlevel 1 goto error
if exist "%x64_lib%" del "%x64_lib%"
ren _msvcrt.lib_00024 "%x64_lib%"
if errorlevel 1 goto error
del "%cab_x64_filename%" >nul 2>&1
echo.

echo Done.

:end
echo.
pause
exit

:error
echo An error occured.
goto :end
