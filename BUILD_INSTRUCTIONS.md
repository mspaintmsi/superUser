
Building from source
====================


Visual Studio
-------------

The `msvc` directory contains what is needed to build the project. Read the file
[msvc/README](msvc/README.md), which explains how.

You will need to download the library files from Microsoft as indicated. If you
don't want to do this, see the alternative in [msvc/ucrt](msvc/ucrt).



Cygwin
------

Run the Cygwin installer, available from <https://www.cygwin.com/setup-x86_64.exe>.
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



MSYS2
-----

If you prefer to use the MSYS2 platform, run the installer following these
instructions: <https://www.msys2.org>  
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


It is also possible to use the __CLANG32/CLANG64__ environments, which have a newer
compiler (llvm) that builds smaller executables (19-22 KB). To do this, you will
need to install the appropriate packages as above (`mingw-w64-clang-i686-gcc-compat`
and `mingw-w64-clang-x86_64-gcc-compat`). Generated executables require the UCRT
dll to run (included in Windows 10 or installed by the cumulative updates in
older versions).



Linux
-----

To build on Linux, native executables running on Windows.

Tested on Linux Mint based on Ubuntu.


Install the `gcc-mingw-w64` package:

	sudo apt install gcc-mingw-w64


Open a terminal and run the following commands:

	cd $HOME/Desktop/superUser 	# (or wherever you put the source to)
	make

If successful, the files `superUser32.exe` and `superUser64.exe` are created.
