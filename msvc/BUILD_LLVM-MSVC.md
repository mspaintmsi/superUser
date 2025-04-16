
Building from source with the LLVM-MSVC toolchain
=================================================

The solution presented below produces executables that run on __Windows__.

On the other hand, the development tools for building these executables may run on
different systems (Windows, Linux, macOS) and architectures (Intel/AMD or Arm,
32 or 64 bit).



Requirements
------------

You need:
- The LLVM toolchain (clang-cl/lld-link/llvm-rc).
- The Visual Studio library (MSVC and SDK).
- GNU make.


The Visual Studio library can be repackaged to be usable on Linux.

Note that it isn't redistributable, so the repackaged version isn't either.

This version can be used also on Windows, without the need to have Visual
Studio installed.



LLVM toolchain
--------------

LLVM is a toolchain based on the modern Clang compiler.
It produces smaller executables than GCC or Visual Studio and does not need to 
be installed.

It comes preinstalled on some platforms, such as MSYS2. If you have MSYS2 
installed, you can use the CLANG64 environment. In this case, you don't need
to download LLVM and can skip the following steps.

Otherwise, go to <https://github.com/llvm/llvm-project/releases> and download the latest file
that matches your __development__ system's architecture, for example:  

- Windows Intel/AMD 64-bit  
`LLVM-<version>-win64.exe`

- Linux Intel/AMD 64-bit   
`LLVM-<version>-Linux-X64.tar.xz`

- Linux on Arm64  
`LLVM-<version>-Linux-ARM64.tar.xz`

Extract its contents to a folder, and add its `bin` subdirectory to your PATH.



Visual Studio library
---------------------

If Visual Studio is installed, the library is ready to use.

On Linux, we need the repackaged version (with files renamed to lowercase
and include directives adjusted accordingly in header files).

If you don't have Visual Studio installed, the original files are downloadable 
from the Microsoft servers.
You need to accept the [license](https://visualstudio.microsoft.com/en/license-terms/vs2022-ga-community/).

You can use the Python script in the [msvc-wine](https://github.com/mstorsjo/msvc-wine)
github repository to download the files.

Open a terminal and run:

	# On Linux:
	./vsdownload.py --dest "<download_directory>"
	
	:: On Windows:
	vsdownload.py --dest "<download_directory>"

where `<download_directory>` is the directory where you want to put the downloaded files.

Then put the `make_msvc-libs` script (available soon) to the download directory and run it.

	# On Linux:
	chmod u+x make_msvc-libs.sh
	./make_msvc-libs.sh
	
	:: On Windows (you can also double-click on it):
	make_msvc-libs.cmd

This creates the repackaged version in the new `msvc-libs` subdirectory.
Move it to the final location and set the `MSVC_LIBS_PATH` environment 
variable to it to use it.

You can then delete the download directory and its contents.



GNU make
--------

On Windows, if you don't have a version of GNU Make running on your computer,
you can use the version included in LLVM-MinGW.

Go to <https://github.com/mstorsjo/llvm-mingw/releases> and download the latest file
that matches your __development__ system's architecture:  

- Windows Intel/AMD 64-bit  
`llvm-mingw-<version>-msvcrt-x86_64.zip`

- Windows on Arm64  
`llvm-mingw-<version>-ucrt-aarch64.zip`

- Windows Intel/AMD 32-bit  
`llvm-mingw-<version>-msvcrt-i686.zip`

Extract the executable `\bin\mingw32-make.exe` and put it anywhere on your machine.
Rename it to `make.exe` and add its directory in your PATH.



Building from source
--------------------

The makefile must know where the MSVC/SDK library is located.

There are two possibilities:

- Visual Studio is installed

Open the "Developer Command Prompt for VS" using the Start menu shortcut.
It sets the necessary environment variables for you.

- Visual Studio is not installed

You need the MSVC/SDK repackaged version.  

Set the `MSVC_LIBS_PATH` environment variable to the directory where it is located.


To build the executables, in the Windows command prompt or Linux terminal,
change the working directory to the `msvc` subdirectory of the _superUser_
distribution, then run:

	make
