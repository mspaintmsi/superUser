
Building from source
====================

All the solutions presented below produce executables that run on __Windows__.

Some can also generate executables for [Windows on Arm](#building-arm-executables).

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

On the other hand, the development tools for building these executables may run on
different systems (Windows, Linux, macOS) and architectures (Intel/AMD or Arm,
32 or 64 bit).

First, choose the system on which you want to run these tools.



Windows
=======


Visual Studio
-------------

The `msvc` directory contains what is needed to build the project. Read the file
[msvc/README](msvc/README.md), which explains how.

You will need to download the library files from Microsoft as indicated. If you
don't want to do this, see the alternative in [msvc/ucrt](msvc/ucrt).



LLVM-MinGW
----------

LLVM-MinGW is a simple toolchain based on the modern Clang compiler.
It produces smaller executables than GCC and does not need to be installed.

Go to <https://github.com/mstorsjo/llvm-mingw/releases> and download the latest file
that matches your __development__ system's architecture.  

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
	cd /d %USERPROFILE%\Desktop\superUser 	&rem (or wherever you put the source to)

To build the Intel/AMD executables, run:

	mingw32-make
	
To build the ARM executables, run:

	mingw32-make -f Makefile-arm

If successful, both 32-bit and 64-bit executables are created.



MSYS2
-----

MSYS2 is a complete environment for building, installing and running native 
Windows software. It uses a Linux-like shell and tools.

Run the installer following these instructions: <https://www.msys2.org> .  
Do NOT install the packages `*ucrt*` indicated in the "_Installation_" procedure.

In the __MSYS2 UCRT64__ terminal, run:

	pacman -S make
	pacman -S mingw-w64-i686-gcc
	pacman -S mingw-w64-x86_64-gcc

Run the following command several times until all the packages are up-to-date:
(read the beginning of this page for details: <https://www.msys2.org/docs/updating/> )

	pacman -Suy

When done, close the __MSYS2 UCRT64__ terminal.


To build the 64-bit executables, open the __MSYS2 MINGW64__ terminal and run:

	cd /c/Users/$USER/Desktop/superUser 	# (or wherever you put the source to)
	make

If successful, the 64-bit executables are created.


To build the 32-bit executables, open the __MSYS2 MINGW32__ terminal and run:

	cd /c/Users/$USER/Desktop/superUser 	# (or wherever you put the source to)
	make

If successful, the 32-bit executables are created.


It is also possible to use the __CLANG64__ environment, which have a newer
compiler that builds smaller executables (24 KB instead of 27 KB).
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

- make
- mingw64-i686-binutils
- mingw64-i686-gcc-core
- mingw64-x86_64-binutils
- mingw64-x86_64-gcc-core

Open a Cygwin terminal and run the following commands:

	cd /cygdrive/c/Users/$USER/Desktop/superUser 	# (or wherever you put the source to)
	make

If successful, both 32-bit and 64-bit executables are created.



Linux
=====

These solutions have been tested on Linux Mint based on Ubuntu.



GCC-MinGW
---------

Install the `gcc-mingw-w64` package:

	sudo apt install gcc-mingw-w64


Open a terminal and run the following commands:

	cd $HOME/Desktop/superUser 	# (or wherever you put the source to)
	make

If successful, both 32-bit and 64-bit executables are created.



LLVM-MinGW
----------

LLVM-MinGW is a simple toolchain based on the modern Clang compiler.
It produces smaller executables than GCC and does not need to be installed.

Go to <https://github.com/mstorsjo/llvm-mingw/releases> and download the latest file
that matches your __development__ system's architecture.  

- Linux Intel/AMD 64-bit  
`llvm-mingw-<version>-msvcrt-ubuntu-<ubuntu_version>-x86_64.tar.xz`

- Linux Arm64  
`llvm-mingw-<version>-ucrt-ubuntu-<ubuntu_version>-aarch64.tar.xz`

Extract its contents to a folder, for example `$HOME/llvm-mingw`.

Open a terminal and run the following commands:

	PATH=$HOME/llvm-mingw/bin:$PATH
	cd $HOME/Desktop/superUser 	# (or wherever you put the source to)

To build the Intel/AMD executables, run:

	make
	
To build the ARM executables, run:

	make -f Makefile-arm

If successful, both 32-bit and 64-bit executables are created.



Building ARM executables
========================


Visual Studio
-------------

The following components must be installed:

- For ARM (32-bit):  
"MSVC v143 - VS 2022 C++ ARM build tools (Latest)"
- For ARM64 (64-bit):  
"MSVC v143 - VS 2022 C++ ARM64/ARM64EC build tools (Latest)"

Follow the instructions in [msvc/ucrt/README](msvc/ucrt/README.md).

Select the ___ARM___ or ___ARM64___ platform instead of _x64_/_x86_ and build 
the solution.

This creates the 32-bit or 64-bit executables in the solution directory (`msvc\ucrt`).



LLVM-MinGW
----------

LLVM-MinGW produces smaller executables than Visual Studio and does not need
to be installed.

There is a version for every system (Windows, Linux, macOS) and architecture
(Intel/AMD, ARM, 32 or 64 bit).

Follow the instructions for LLVM-MinGW in the Windows or Linux section above.



MSYS2 (Windows 11 on Arm64 ___only___)
--------------------------------------

Follow the instructions for MSYS2 in the Windows section above.

Install the `mingw-w64-clang-aarch64-clang` package instead of the
`mingw-w64-i686-gcc` and `mingw-w64-x86_64-gcc` packages.

Execute `clangarm64.exe` in the MSYS2 install directory to open the
__MSYS2 CLANGARM64__ terminal.

Use the `make -f Makefile-arm` command to build the ARM executables. 
