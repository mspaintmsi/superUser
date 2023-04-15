Visual Studio / MSVCRT
======================

By default, Visual Studio (since 2015) uses the Visual C++ runtime (vcruntimeXXX.dll) included in the latest "Visual C++ Redistributable", and the UCRT dlls. The compiled program needs these DLLs to run.

To eliminate this dependency, the program can be statically linked with the library, but this increases enormously the size of the executable (more than a hundred KBytes).

The solution is to link against the old unversioned msvcrt.dll that comes with all versions of Windows.
This will completely eliminate any dependency on a specific toolchain/dll on end-user systems.
Moreover, the size of the executable is very small (around 13 KB !).

To do so, the program must be statically linked against the minimal MSVCRT.LIB import library. This one is not included in Visual Studio. It must be extracted from the Windows Driver Kit (WDK).
It should not be confused with the normal MSVCRT.LIB file contained in Visual Studio, which requires the classic VC++ runtime.

We need both versions of the library, a 32-bit and a 64-bit, renamed msvcrt32.lib and msvcrt64.lib. These files must be copied to this "msvc" directory before compiling/linking the project with Visual Studio.

The WDK 7.1 can be downloaded directly from Microsoft:

https://www.microsoft.com/en-us/download/details.aspx?id=11800
File: GRMWDK_EN_7600_1.ISO
Size: 619 MBytes
SHA1: DE6ABDB8EB4E08942ADD4AA270C763ED4E3D8242
SHA256: 5EDC723B50EA28A070CAD361DD0927DF402B7A861A036BBCF11D27EBBA77657D

The "extract_msvcrt_libs.cmd" script can automatically extract the msvcrt*.lib files from the WDK iso file.
It requires 7-Zip application (https://7-zip.org) to be installed. Put the iso in the "msvc" directory, and run the script.

If you don't want to use the script or 7-Zip, you can do it manually:

Open the ISO file and extract the \WDK\libs_x86fre_cab001.cab and \WDK\libs_x64fre_cab001.cab files.
Extract the _msvcrt.lib_* files and rename them to msvcrt*.lib:

libs_x86fre_cab001.cab / _msvcrt.lib_00025  -> msvcrt32.lib
libs_x64fre_cab001.cab / _msvcrt.lib_00024  -> msvcrt64.lib

and copy them to the "msvc" directory.

Then you can build the project with Visual Studio.

- Open the "msvc\superUser" project.
- In the toolbar, choose the "Release" configuration.
- Select the platform (x64 or x86).
- Build the project (menu Build > Build Solution, or press Ctrl+Shift+B).

This creates superUser32.exe or superUser64.exe in the project directory ("msvc").
