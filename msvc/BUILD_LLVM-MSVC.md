
Building from source with the LLVM-MSVC toolchain
=================================================

The solution presented below produces executables that run on __Windows__.

On the other hand, the development tools for building these executables may run
on different systems (Windows, Linux, macOS) and architectures (Intel/AMD or
Arm, 32 or 64 bit).



Requirements
------------

You need:
- The LLVM toolchain (clang-cl/lld-link/llvm-rc).
- The Visual Studio library (MSVC and SDK).
- The MSVCRT library files.
- GNU make.


The Visual Studio library can be repackaged to be usable on Linux.

The repackaged version can also be used on Windows, without the need to have
Visual Studio installed.

Note that the Visual Studio library is not redistributable, so neither is the
repackaged version.



LLVM toolchain
--------------

LLVM is a toolchain based on the modern Clang compiler.
It produces smaller executables than GCC or Visual Studio and does not need to
be installed.

It comes preinstalled on some platforms, such as MSYS2. If you have MSYS2 
installed, you can use the CLANG64 environment. In this case, you don't need
to download LLVM and can skip the following steps.

Otherwise, go to <https://github.com/llvm/llvm-project/releases> and download
the latest file that matches your __development__ system's architecture, for
example:  

- Windows Intel/AMD 64-bit  
`LLVM-<version>-win64.exe`

- Windows on Arm64  
`LLVM-<version>-woa64.exe` (maybe not the latest version)

- Linux Intel/AMD 64-bit   
`LLVM-<version>-Linux-X64.tar.xz`

- Linux on Arm64  
`LLVM-<version>-Linux-ARM64.tar.xz`

Extract its contents to a folder, and add its `bin` subdirectory to your PATH.



Visual Studio library
---------------------

If Visual Studio is installed, the library is ready to use.

Otherwise, the original files are downloadable from the Microsoft servers.

Follow these [instructions](
https://github.com/Matrix3600/msvc-libs/blob/main/download_vslib.md)
to download the Visual Studio MSVC/SDK library to a directory
`<download_directory>` where you want to put the downloaded files.

To get the script to build the repackaged library, go to
<https://github.com/Matrix3600/msvc-libs/releases> and download the source
code.

Choose the `make_msvc-libs` script (`.cmd` or `.sh`) that matches your system
(Windows or Linux). Place it in `<download_directory>`, along with the
`make_msvc-libs_conf.txt` configuration file.

Then run (you can also double-click on the script):

	# On Linux:
	chmod u+x make_msvc-libs.sh
	./make_msvc-libs.sh
	
	:: On Windows:
	make_msvc-libs.cmd

This creates the repackaged version in a new `msvc-libs` subdirectory.
Move it to the final location and set the `MSVC_LIBS_PATH` environment 
variable to it to use it.

You can then delete the download directory and its contents.



MSVCRT library files
--------------------

The two files `msvcrt32.lib` and `msvcrt64.lib` must be present in this `msvc`
directory.  
Read the [msvc/README](/msvc/README.md) file for details.



GNU make
--------

On Windows, if you don't have a working version of GNU Make on your computer,
you can either use the version included in LLVM-MinGW or download the one
provided by [ezwinports](https://sourceforge.net/projects/ezwinports).

Go to <https://sourceforge.net/projects/ezwinports/files> and download the latest
file `make-<version>-without-guile-w32-bin.zip`.

Extract the executable `\bin\make.exe` and put it anywhere on your machine. Add
its directory in your PATH.



Building from source
--------------------

The makefile must know where the MSVC/SDK library is located.

There are two possibilities:

- Visual Studio is installed.

Open the "_Developer Command Prompt for VS_" using the Start menu shortcut.
It sets the necessary environment variables automatically.

- Visual Studio is not installed.

You need the MSVC/SDK repackaged version.  

Set the `MSVC_LIBS_PATH` environment variable to the directory where it is
located.


To build the executables, in the Windows command prompt or Linux terminal,
change the working directory to the `msvc` subdirectory of the _superUser_
distribution, then run:

	make
