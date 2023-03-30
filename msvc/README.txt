Visual Studio / MSVCRT
======================

By default, Visual Studio uses the Visual C++ runtime (msvcrtXX.dll). The compiled program needs this DLL to run. 

To eliminate this dependance, the program can be linked statically with the library, but this increases enormously the size of the executable (hundreds of Kbytes).

The solution is to link against the unversioned msvcrt.dll that has shipped with all versions of Windows.
This will eliminate completely any dependence on a specific toolchain/dll on end-user systems.
Also, the size of the executable is very small (approximately 13 KB !).

To do so, the program must be linked statically against the MSVCRT.LIB library. This is not provided with Visual Studio. It must be extracted from the Windows Driver Kit (WDK).

We need the two versions of the library, one 32-bit and one 64-bit, renamed msvcrt32.lib and msvcrt64.lib. These files must be copied to this /msvc directory before compiling/linking the project with Visual Studio.

The WDK 7.1 can be downloaded directly from Microsoft:

https://www.microsoft.com/en-us/download/details.aspx?id=11800
File: GRMWDK_EN_7600_1.ISO
SHA1: DE6ABDB8EB4E08942ADD4AA270C763ED4E3D8242
SHA256: 5EDC723B50EA28A070CAD361DD0927DF402B7A861A036BBCF11D27EBBA77657D

Open the ISO file and extract the \WDK\libs_x86fre_cab001.cab and \WDK\libs_x64fre_cab001.cab files.
Extract the files _msvcrt.lib_* rename them to msvcrt*.lib:

libs_x86fre_cab001.cab / _msvcrt.lib_00025  -> msvcrt32.lib
libs_x64fre_cab001.cab / _msvcrt.lib_00024  -> msvcrt64.lib

and copy them to the /msvc directory.

