
Building from source
====================


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

LLVM-MinGW is a simple toolchain based on the modern CLANG compiler.
It produces smaller executables than GCC and does not need to be installed.

Go to <https://github.com/mstorsjo/llvm-mingw/releases> and download the latest
`llvm-mingw-<version>-msvcrt-x86_64.zip` file (if your development system
architecture is Intel/AMD 64-bit).  
Extract its contents to a folder, for example `C:\llvm-mingw`.

Open a command prompt and run the following commands:

	path=C:\llvm-mingw\bin;%path%
	cd /d %USERPROFILE%\Desktop\superUser 	&rem (or wherever you put the source to)
	mingw32-make

If successful, the files `superUser32.exe` and `superUser64.exe` are created.



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


To build the 64-bit executable, open the __MSYS2 MINGW64__ terminal and run:

	cd /c/Users/$USER/Desktop/superUser 	# (or wherever you put the source to)
	make

If successful, the file `superUser64.exe` is created.


To build the 32-bit executable, open the __MSYS2 MINGW32__ terminal and run:

	cd /c/Users/$USER/Desktop/superUser 	# (or wherever you put the source to)
	make

If successful, the file `superUser32.exe` is created.


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

If successful, the files `superUser32.exe` and `superUser64.exe` are created.



Linux
=====

To build on Linux, native executables running on Windows.

Tested on Linux Mint based on Ubuntu.



GCC-MinGW
---------

Install the `gcc-mingw-w64` package:

	sudo apt install gcc-mingw-w64


Open a terminal and run the following commands:

	cd $HOME/Desktop/superUser 	# (or wherever you put the source to)
	make

If successful, the files `superUser32.exe` and `superUser64.exe` are created.



LLVM-MinGW
----------

LLVM-MinGW is a simple toolchain based on the modern CLANG compiler.
It produces smaller executables than GCC and does not need to be installed.

Go to <https://github.com/mstorsjo/llvm-mingw/releases> and download the latest
`llvm-mingw-<version>-msvcrt-ubuntu-<version>-x86_64.tar.xz` file (if your
development system architecture is Intel/AMD 64-bit).  
Extract its contents to a folder, for example `$HOME/llvm-mingw`.

Open a terminal and run the following commands:

	PATH=$HOME/llvm-mingw/bin:$PATH
	cd $HOME/Desktop/superUser 	# (or wherever you put the source to)
	make

If successful, the files `superUser32.exe` and `superUser64.exe` are created.



Building ARM executables
========================


Visual Studio
-------------

The following components must be installed:

- For ARM (32-bits):  
"MSVC v143 - VS 2022 C++ ARM build tools (Latest)"
- For ARM64 (64-bits):  
"MSVC v143 - VS 2022 C++ ARM64/ARM64EC build tools (Latest)"

Follow the instructions in [msvc/ucrt/README](msvc/ucrt/README.md).

Select the ___ARM___ or ___ARM64___ platform instead of _x64_/_x86_ and build the solution.

This creates `superUserA32.exe` or `superUserA64.exe` in the solution directory (`msvc\ucrt`).



LLVM-MinGW
----------

LLVM-MinGW produces smaller executables than Visual Studio and does not need to be installed.

Go to <https://github.com/mstorsjo/llvm-mingw/releases> and download the latest
`llvm-mingw-<version>-msvcrt-*` file that matches your __development__ OS and architecture
(not necessarily ARM). If no file matches your architecture, choose the `ucrt` version 
`llvm-mingw-<version>-ucrt-*`.

Follow the instructions for LLVM-MinGW in the Windows or Linux section above.

Add `-f Makefile-arm` after the `make` (or `mingw32-make`) command to build the ARM executables:

	make -f Makefile-arm
or

	mingw32-make -f Makefile-arm
