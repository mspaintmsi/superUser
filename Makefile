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
CPPFLAGS = -D_WIN32_WINNT=_WIN32_WINNT_VISTA
CFLAGS = $(OPT) $(INCLUDE)
LDFLAGS = -Wl,--exclude-all-symbols,--dynamicbase,--nxcompat,--subsystem,console
LDLIBS= -lwtsapi32
WRFLAGS = --codepage=65001 -O coff

SRC = superUser.c common.c tokens.c 

.PHONY: all clean x86 x64

all: $(TARGETS)

clean:
	rm -f *.exe *.res

x86: superUser32.exe
x64: superUser64.exe

superUser32.exe superUser64.exe: tokens.h winnt2.h

superUser32.exe: $(SRC) superUser32.res
	$(CC32) $(OPT32) $(CPPFLAGS) $(CFLAGS) $(SRC) $(LDFLAGS) superUser32.res $(LDLIBS) -o $@ 

superUser64.exe: $(SRC) superUser64.res
	$(CC64) $(OPT64) $(CPPFLAGS) $(CFLAGS) $(SRC) $(LDFLAGS) superUser64.res $(LDLIBS) -o $@

superUser32.res: superUser.rc
	$(WINDRES32) $(WRFLAGS) -F pe-i386 -DTARGET32 $< $@

superUser64.res: superUser.rc
	$(WINDRES64) $(WRFLAGS) -F pe-x86-64 -DTARGET64 $< $@
