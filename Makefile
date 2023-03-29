SYS32 =
SYS64 =
SYSOPT =
OPT =
TARGETS = x86 x64

ifeq ($(MSYSTEM),MINGW32)	# MSYS2 MINGW32
TARGETS = x86
else ifeq ($(MSYSTEM),MINGW64)	# MSYS2 MINGW64
TARGETS = x64
else	# CYGWIN
SYS32 = i686-w64-mingw32-
SYS64 = x86_64-w64-mingw32-
OPT = -Wp,-DCYGWIN
endif

CC32 = $(SYS32)gcc
CC64 = $(SYS64)gcc
WINDRES32 = $(SYS32)windres
WINDRES64 = $(SYS64)windres

OPT32 = -m32
OPT64 = -m64
OPT += -municode -Os -s -flto -Wall -fno-ident
LDFLAGS = -Wl,--exclude-all-symbols,--dynamicbase,--nxcompat,--subsystem,console
WRFLAGS = --codepage=65001 -O coff

all: $(TARGETS)

clean:
	-rm *.exe *.res

x86: superUser32.exe
x64: superUser64.exe

superUser32.exe superUser64.exe: tokens.h

superUser32.exe: superUser.c superUser32.res
	$(CC32) $(OPT32) $(OPT) $< superUser32.res -o $@ $(LDFLAGS)

superUser64.exe: superUser.c superUser64.res
	$(CC64) $(OPT64) $(OPT) $< superUser64.res -o $@ $(LDFLAGS)

superUser32.res: superUser.rc
	$(WINDRES32) $(WRFLAGS) -F pe-i386 -DBUILD32 $< $@

superUser64.res: superUser.rc
	$(WINDRES64) $(WRFLAGS) -F pe-x86-64 -DBUILD64 $< $@
