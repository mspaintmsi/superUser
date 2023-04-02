#
# superUser Makefile
#
# Build native executables running on Windows, on different platforms.
#
#	* Windows 
#		- Cygwin / MinGW-w64
#		- MSYS2 / MINGW32, MINGW64, CLANG32, CLANG64, UCRT64
#
#	* Linux
#		- with mingw-w64 package
#
# Read BUILD_INSTRUCTIONS.txt file for details.
#

HOST32 =
HOST64 =
INCLUDE =
TARGETS = x86 x64

ifeq (32,$(findstring 32,$(MSYSTEM)))	# MSYS2 32-bit
TARGETS = x86
else ifeq (64,$(findstring 64,$(MSYSTEM)))	# MSYS2 64-bit
TARGETS = x64
else	# Cygwin or Linux
HOST32 = i686-w64-mingw32-
HOST64 = x86_64-w64-mingw32-
endif
ifneq ($(OS),Windows_NT)
INCLUDE += -I/usr/share/mingw-w64/include	# Linux mingw-w64 package
endif

CC32 = $(HOST32)gcc
CC64 = $(HOST64)gcc
WINDRES32 = $(HOST32)windres
WINDRES64 = $(HOST64)windres

OPT32 = -m32
OPT64 = -m64
OPT = -municode -Os -s -flto -fno-ident -Wall
CCFLAGS = $(OPT) $(INCLUDE)
LDFLAGS = -Wl,--exclude-all-symbols,--dynamicbase,--nxcompat,--subsystem,console
WRFLAGS = --codepage=65001 -O coff

all: $(TARGETS)

clean:
	rm -f *.exe *.res

x86: superUser32.exe
x64: superUser64.exe

superUser32.exe superUser64.exe: tokens.h

superUser32.exe: superUser.c superUser32.res
	$(CC32) $(OPT32) $(CCFLAGS) $< superUser32.res -o $@ $(LDFLAGS)

superUser64.exe: superUser.c superUser64.res
	$(CC64) $(OPT64) $(CCFLAGS) $< superUser64.res -o $@ $(LDFLAGS)

superUser32.res: superUser.rc
	$(WINDRES32) $(WRFLAGS) -F pe-i386 -DTARGET32 $< $@

superUser64.res: superUser.rc
	$(WINDRES64) $(WRFLAGS) -F pe-x86-64 -DTARGET64 $< $@
