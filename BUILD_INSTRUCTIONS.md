
Building from source
====================

All the solutions presented below produce executables that run on __Windows__.

Some can also generate executables for Windows on Arm.

On the other hand, the development tools for building these executables may run on
different systems (Windows, Linux, macOS) and architectures (Intel/AMD or Arm,
32 or 64 bit).

|   Toolchain   |       Runs on       |   Generates executables for    |
|:-------------:|---------------------|--------------------------------|
| Visual Studio | Windows (x64/Arm64) | Intel/AMD, Arm                 |
| LLVM-MinGW    | Windows (all)<br />Linux<br />macOS | Intel/AMD, Arm |
| LLVM-MSVC     | Windows (all)<br />Linux<br />macOS | Intel/AMD      |
| WinLibs<br />GCC-MinGW | Windows (x86/x64) | Intel/AMD               |
| GCC-MinGW     | Linux<br />macOS    | Intel/AMD                      |
| MSYS2         | Windows (x64)       | Intel/AMD                      |
| MSYS2         | Windows 11 on Arm64 | Intel/AMD, Arm                 |
| Cygwin        | Windows (x64)       | Intel/AMD                      |

<br />
The executable names are as follows:

- Windows Intel/AMD 32-bit  
`superUser32.exe`
`sudo32.exe`

- Windows Intel/AMD 64-bit  
`superUser64.exe`
`sudo64.exe`

- Windows on ARMv7 32-bit  
`superUserA32.exe`
`sudoA32.exe`

- Windows on Arm64  
`superUserA64.exe`
`sudoA64.exe`

<br />
First, choose the system on which you want to run the development tools.


<br /><br />

Windows
=======


Visual Studio
-------------

The `msvc` directory contains what is needed to build the Intel/AMD executables.
Read the [msvc/README](/msvc/README.md) file, which explains how.

You will need to download the library files from Microsoft as indicated. If you
don't want to do this, see the alternative in [msvc/ucrt](/msvc/ucrt).



LLVM-MinGW
----------

LLVM-MinGW is a simple toolchain based on the modern Clang compiler.
It produces smaller executables than GCC and does not need to be installed.

Go to <https://github.com/mstorsjo/llvm-mingw/releases> and download the latest file
that matches your __development__ system's architecture:  

- Windows Intel/AMD 64-bit  
`llvm-mingw-<version>-msvcrt-x86_64.zip`

- Windows on Arm64  
`llvm-mingw-<version>-ucrt-aarch64.zip`

- Windows Intel/AMD 32-bit  
`llvm-mingw-<version>-msvcrt-i686.zip`

- Windows on Arm 32-bit  
`llvm-mingw-<version>-ucrt-armv7.zip`

Extract its contents to a folder, for example `C:\llvm-mingw`.

Open a command prompt and run the following commands:

	path=C:\llvm-mingw\bin;%path%
	cd /d "%USERPROFILE%\Desktop\superUser" 	&:: (or wherever you put the source to)

To build the executables, run one of these commands:

	mingw32-make 		&:: Executables for your machine architecture
	mingw32-make intel 	&:: Intel/AMD executables
	mingw32-make arm 	&:: ARM executables
	mingw32-make all 	&:: All the executables

If successful, both 32-bit and 64-bit executables are created.



LLVM-MSVC
---------

Refer to the instructions in [msvc/BUILD_LLVM-MSVC](/msvc/BUILD_LLVM-MSVC.md).



WinLibs
-------

WinLibs is a simple toolchain based on the GCC compiler and MinGW-w64 library.
It does not need to be installed.

Go to <https://winlibs.com> and download the latest release files for the MSVCRT 
runtime:

- The _Win32_ version runs on 32/64-bit Windows and generates executables for 32-bit Windows.
- The _Win64_ version runs on 64-bit Windows and generates executables for 64-bit Windows.

Extract their contents into a folder, for example `C:\WinLibs`. It will
contain the `mingw32` and `mingw64` subfolders.

Open a command prompt and run the following commands:

	path=C:\WinLibs\mingw32\bin;C:\WinLibs\mingw64\bin;%path%
	cd /d "C:\WinLibs"
	copy /y mingw32\bin\windres.exe mingw32\bin\i686-w64-mingw32-windres.exe
	copy /y mingw64\bin\windres.exe mingw64\bin\x86_64-w64-mingw32-windres.exe

To build the executables, run:

	cd /d "%USERPROFILE%\Desktop\superUser" 	&:: (or wherever you put the source to)
	mingw32-make



MSYS2
-----

MSYS2 is a complete environment for building, installing and running native 
Windows software. It uses a Linux-like shell and tools.

Run the installer following these instructions: <https://www.msys2.org> .  
Do NOT install the packages `*ucrt*` indicated in the "_Installation_" procedure.

In the __MSYS2 UCRT64__ terminal, run:

	pacman -S make
	pacman -S mingw-w64-i686-gcc	# for 32-bit Intel/AMD executables
	pacman -S mingw-w64-x86_64-gcc	# for 64-bit Intel/AMD executables

Run the following command several times until all the packages are up-to-date:
(read the beginning of this page for details: <https://www.msys2.org/docs/updating/> )

	pacman -Suy

When done, close the __MSYS2 UCRT64__ terminal.

Open the terminal associated with the type of Intel/AMD executables you want to build:

- For 64-bit executables: the __MSYS2 MINGW64__ terminal.
- For 32-bit executables: the __MSYS2 MINGW32__ terminal.

Then run:

	cd "/c/Users/$USER/Desktop/superUser" 	# (or wherever you put the source to)
	make

It is also possible to use the __CLANG64__ environment, which have a newer
compiler that builds smaller 64-bit executables (24 KB instead of 27 KB).
To do this, you will need to install the appropriate package as above
(`mingw-w64-clang-x86_64-gcc-compat`).
The generated executables require the UCRT dll to run (included in Windows 10 or
installed by the cumulative updates in older versions).



Cygwin
------

Run the Cygwin installer, available from <https://www.cygwin.com/setup-x86_64.exe> .
When you get to the package selection page, select "_Category_" in the list "_View_"
at the top left, and expand the tree by clicking on "_All_" and "_Devel_".

Then choose the following packages (you can use the search box to reduce the
list), by selecting the version to install in the column "_New_":

	make
	mingw64-i686-binutils	# for 32-bit Intel/AMD executables
	mingw64-i686-gcc-core	# -
	mingw64-x86_64-binutils	# for 64-bit Intel/AMD executables
	mingw64-x86_64-gcc-core	# -

Open a Cygwin terminal and run the following commands:

	cd "/cygdrive/c/Users/$USER/Desktop/superUser" 	# (or wherever you put the source to)
	make

If successful, both 32-bit and 64-bit Intel/AMD executables are created.


<br />

Linux
=====

These solutions have been tested on Ubuntu-based Linux Mint.



GCC-MinGW
---------

Install the `gcc-mingw-w64` package:

	sudo apt install gcc-mingw-w64


Open a terminal and run the following commands:

	cd "$HOME/Desktop/superUser" 	# (or wherever you put the source to)
	make

If successful, both 32-bit and 64-bit Intel/AMD executables are created.



LLVM-MinGW
----------

LLVM-MinGW is a simple toolchain based on the modern Clang compiler.
It produces smaller executables than GCC and does not need to be installed.

Go to <https://github.com/mstorsjo/llvm-mingw/releases> and download the latest file
that matches your __development__ system's architecture:  

- Linux Intel/AMD 64-bit  
`llvm-mingw-<version>-msvcrt-ubuntu-<ubuntu_version>-x86_64.tar.xz`

- Linux Arm64  
`llvm-mingw-<version>-ucrt-ubuntu-<ubuntu_version>-aarch64.tar.xz`

Extract its contents to a folder, for example `$HOME/llvm-mingw`.

Open a terminal and run the following commands:

	PATH=$HOME/llvm-mingw/bin:$PATH
	cd "$HOME/Desktop/superUser" 	# (or wherever you put the source to)

To build the executables, run one of these commands:

	make		# Executables for your machine architecture
	make intel	# Intel/AMD executables
	make arm	# ARM executables
	make all	# All the executables

If successful, both 32-bit and 64-bit executables are created.



LLVM-MSVC
---------

Refer to the instructions in [msvc/BUILD_LLVM-MSVC](/msvc/BUILD_LLVM-MSVC.md).


<br />

Building ARM executables
========================


Visual Studio
-------------

The following components must be installed:

- For ARM64 (64-bit):  
"MSVC v143 - VS 2022 C++ ARM64 build tools (Latest)"
- For ARM (32-bit):  
"MSVC v143 - VS 2022 C++ ARM build tools (Latest)"  
"Windows 11 Software Development Kit (SDK) 10.0.22621" or below

Then follow the instructions in [msvc/ucrt/README](/msvc/ucrt/README.md).

Select the ___ARM___ or ___ARM64___ platform and build the solution.



LLVM-MinGW
----------

LLVM-MinGW produces smaller executables than Visual Studio and does not need
to be installed.

Depending on your development system, follow the instructions for LLVM-MinGW in the
__Windows__ or __Linux__ section above.



MSYS2 (Windows 11 on Arm64 ___only___)
--------------------------------------

Follow the instructions for MSYS2 in the Windows section above, except for the
following:

Install the `mingw-w64-clang-aarch64-clang` package instead of the
`mingw-w64-i686-gcc` and `mingw-w64-x86_64-gcc` packages.

Execute `clangarm64.exe` in the MSYS2 install directory to open the
__MSYS2 CLANGARM64__ terminal, and run:

	cd "/c/Users/$USER/Desktop/superUser" 	# (or wherever you put the source to)
	make
