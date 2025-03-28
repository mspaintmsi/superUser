#
# superUser Makefile
#
# Build x86 / x64 executables for Windows (Intel/AMD).
#
# Supported development OS and toolchains:
#
#	* Windows
#		- Cygwin / MinGW-w64
#		- MSYS2 / MINGW32, MINGW64, CLANG64, UCRT64
#		- LLVM-MinGW
#
#	* Linux
#		- GCC-MinGW (gcc-mingw-w64 package)
#		- LLVM-MinGW
#
# Read BUILD_INSTRUCTIONS.md file for details.
#

NATIVEWIN =
DEVNUL = /dev/null
ifeq ($(OS),Windows_NT)
ifeq (,$(findstring OSTYPE=cygwin,$(shell set)))
ifeq ($(findstring ;,$(PATH)),;)
NATIVEWIN = 1
DEVNUL = nul
endif
endif
endif

HOST32 =
HOST64 =
CC32 = $(HOST32)gcc
CC64 = $(HOST64)gcc
WINDRES32 = $(HOST32)windres
WINDRES64 = $(HOST64)windres

TARGETS =
ifeq (CLANGARM64,$(findstring CLANGARM64,$(MSYSTEM)))	# MSYS2 CLANGARM64
CC32 =
CC64 =
else ifeq (32,$(findstring 32,$(MSYSTEM)))	# MSYS2 32-bit
TARGETS = x86
CC64 =
else ifeq (64,$(findstring 64,$(MSYSTEM)))	# MSYS2 64-bit
TARGETS = x64
CC32 =
else	# Cygwin, LLVM-MinGW or Linux
HOST32 = i686-w64-mingw32-
HOST64 = x86_64-w64-mingw32-
ifneq (,$(shell $(CC32) --version 2>$(DEVNUL)))	# 32-bit compiler exists
TARGETS += x86
else
CC32 =
endif
ifneq (,$(shell $(CC64) --version 2>$(DEVNUL)))	# 64-bit compiler exists
TARGETS += x64
else
CC64 =
endif
endif

# _WIN32_WINNT: the minimal Windows version the app can run on.
# Windows Vista: the earliest to utilize the Trusted Installer.

CPPFLAGS = -D_WIN32_WINNT=_WIN32_WINNT_VISTA -D_UNICODE
CFLAGS = -municode -Os -s -flto -fno-ident -Wall
LDFLAGS = -Wl,--exclude-all-symbols,--dynamicbase,--nxcompat,--subsystem,console
LDLIBS = -lwtsapi32
WRFLAGS = --codepage 65001 -O coff

SRCS = tokens.c utils.c
DEPS = tokens.h utils.h winnt2.h

.PHONY: all clean check check32 check64 x86 x64

all: $(TARGETS) | check

clean:
ifdef NATIVEWIN
	if exist *.exe del *.exe
	if exist *.res del *.res
else
	rm -f *.exe *.res
endif

check:
ifeq (,$(CC32)$(CC64))
	@echo ERROR: No suitable toolchain.
	@exit 1
endif

check32:
ifndef CC32
	@echo ERROR: No toolchain to build x86.
	@exit 1
endif

check64:
ifndef CC64
	@echo ERROR: No toolchain to build x64.
	@exit 1
endif

x86: superUser32.exe sudo32.exe
x64: superUser64.exe sudo64.exe


# superUser

superUser32.exe superUser64.exe: $(SRCS) $(DEPS)

superUser32.exe: superUser32.res | check32
	$(CC32) $(CPPFLAGS) $(CFLAGS) superUser.c $(SRCS) $(LDFLAGS) superUser32.res $(LDLIBS) -o $@

superUser64.exe: superUser64.res | check64
	$(CC64) $(CPPFLAGS) $(CFLAGS) superUser.c $(SRCS) $(LDFLAGS) superUser64.res $(LDLIBS) -o $@

superUser32.res: superUser.rc | check32
	$(WINDRES32) $(WRFLAGS) -F pe-i386 -DTARGET32 $< $@

superUser64.res: superUser.rc | check64
	$(WINDRES64) $(WRFLAGS) -F pe-x86-64 -DTARGET64 $< $@


# sudo

sudo32.exe sudo64.exe: $(SRCS) $(DEPS)

sudo32.exe: sudo32.res | check32
	$(CC32) $(CPPFLAGS) $(CFLAGS) sudo.c $(SRCS) $(LDFLAGS) sudo32.res $(LDLIBS) -o $@

sudo64.exe: sudo64.res | check64
	$(CC64) $(CPPFLAGS) $(CFLAGS) sudo.c $(SRCS) $(LDFLAGS) sudo64.res $(LDLIBS) -o $@

sudo32.res: sudo.rc | check32
	$(WINDRES32) $(WRFLAGS) -F pe-i386 -DTARGET32 $< $@

sudo64.res: sudo.rc | check64
	$(WINDRES64) $(WRFLAGS) -F pe-x86-64 -DTARGET64 $< $@
