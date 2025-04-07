#
# superUser 6.0
#
# Copyright 2019-2025 https://github.com/mspaintmsi/superUser
#
# Makefile (version for GNU make)
#
# - Build x86 / x64 executables for Windows (Intel/AMD).
# - Build native ARMv7 / ARM64 executables for "Windows on Arm".
#
# Supported development OS and toolchains:
#
#    Toolchain     |       Runs on       | Generates executables for
# -------------------------------------------------------------------
# LLVM-MinGW         Windows (all),        Intel/AMD, Arm
#                    Linux,
#                    macOS
# WinLibs GCC-MinGW  Windows (x86/x64)     Intel/AMD (*)
# GCC-MinGW          Linux,                Intel/AMD
#                    macOS
# MSYS2              Windows (x64)         Intel/AMD
# MSYS2              Windows 11 on Arm64   Intel/AMD, Arm
# Cygwin             Windows (x64)         Intel/AMD
#
# (*) See instructions before use.
#
# Read the BUILD_INSTRUCTIONS.md file for details.
#

NATIVEWIN =
DEVNUL = /dev/null
ifeq ($(OS),Windows_NT)
 ifeq ($(shell echo $$PPID),$$PPID)
  NATIVEWIN = 1
  DEVNUL = nul
 endif
endif

.DEFAULT_GOAL = intel
ifeq ($(OS),Windows_NT)
 ifneq (,$(filter ARM%,$(PROCESSOR_ARCHITECTURE)))
  .DEFAULT_GOAL = arm
 endif
else
 ifneq (,$(filter aarch64% arm%,$(shell uname -m 2>$(DEVNUL))))
  .DEFAULT_GOAL = arm
 endif
endif

HOST_32 =
HOST_64 =
HOST_A32 =
HOST_A64 =
CC_32 = $(HOST_32)gcc
CC_64 = $(HOST_64)gcc
CC_A32 = $(HOST_A32)gcc
CC_A64 = $(HOST_A64)gcc
WINDRES_32 = $(HOST_32)windres
WINDRES_64 = $(HOST_64)windres
WINDRES_A32 = $(HOST_A32)windres
WINDRES_A64 = $(HOST_A64)windres

TARGETS_INTEL =
TARGETS_ARM =

ifeq (CLANGARM64,$(MSYSTEM))	# MSYS2 CLANGARM64
 .DEFAULT_GOAL = arm
 TARGETS_ARM = arm64
 CC_A64 = clang
 CC_32 =
 CC_64 =
 CC_A32 =
else ifeq (32,$(findstring 32,$(MSYSTEM)))	# MSYS2 32-bit Intel/AMD
 .DEFAULT_GOAL = intel
 TARGETS_INTEL = x86
 CC_64 =
 CC_A32 =
 CC_A64 =
else ifeq (64,$(findstring 64,$(MSYSTEM)))	# MSYS2 64-bit Intel/AMD
 .DEFAULT_GOAL = intel
 TARGETS_INTEL = x64
 CC_32 =
 CC_A32 =
 CC_A64 =
else	# Cygwin, LLVM-MinGW or Linux
 HOST_32 = i686-w64-mingw32-
 HOST_64 = x86_64-w64-mingw32-
 HOST_A32 = armv7-w64-mingw32-
 HOST_A64 = aarch64-w64-mingw32-

 ifneq (,$(shell $(CC_32) --version 2>$(DEVNUL)))	# 32-bit Intel/AMD compiler exists
  TARGETS_INTEL += x86
 else
  CC_32 =
 endif
 ifneq (,$(shell $(CC_64) --version 2>$(DEVNUL)))	# 64-bit Intel/AMD compiler exists
  TARGETS_INTEL += x64
 else
  CC_64 =
 endif
 ifneq (,$(shell $(CC_A32) --version 2>$(DEVNUL)))	# 32-bit ARM compiler exists
  TARGETS_ARM += arm32
 else
  CC_A32 =
 endif
 ifneq (,$(shell $(CC_A64) --version 2>$(DEVNUL)))	# 64-bit ARM compiler exists
  TARGETS_ARM += arm64
 else
  CC_A64 =
 endif
endif


.PHONY: all intel arm x86 x64 arm32 arm64 default clean \
	check_all check_intel check_arm check_32 check_64 check_A32 check_A64

default: $(.DEFAULT_GOAL)

all: $(TARGETS_INTEL) $(TARGETS_ARM) | check_all
intel: $(TARGETS_INTEL) | check_intel
arm: $(TARGETS_ARM) | check_arm

clean:
ifdef NATIVEWIN
	if exist *.exe del *.exe
	if exist *.res del *.res
else
	rm -f *.exe *.res
endif

define ERROR_NO_TOOLCHAIN
	@echo ERROR: No toolchain to build $(1).
	@exit 1
endef

check_all:
ifeq (,$(CC_32)$(CC_64)$(CC_A32)$(CC_A64))
	@echo ERROR: No suitable toolchain.
	@exit 1
endif

check_intel:
ifeq (,$(CC_32)$(CC_64))
	$(call ERROR_NO_TOOLCHAIN,intel)
endif

check_arm:
ifeq (,$(CC_A32)$(CC_A64))
	$(call ERROR_NO_TOOLCHAIN,arm)
endif

check_32:
ifndef CC_32
	$(call ERROR_NO_TOOLCHAIN,x86)
endif

check_64:
ifndef CC_64
	$(call ERROR_NO_TOOLCHAIN,x64)
endif

check_A32:
ifndef CC_A32
	$(call ERROR_NO_TOOLCHAIN,arm32)
endif

check_A64:
ifndef CC_A64
	$(call ERROR_NO_TOOLCHAIN,arm64)
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

PROJECTS = superUser sudo
ARCHS = 32 64 A32 A64

x86: $(PROJECTS:%=%32.exe)
x64: $(PROJECTS:%=%64.exe)
arm32: $(PROJECTS:%=%A32.exe)
arm64: $(PROJECTS:%=%A64.exe)

.DELETE_ON_ERROR:

define BUILD_PROJECT
# $(1): Project name
# $(2): 32, 64, A32 or A64
#
$(1)$(2).exe: $(1).c $$(SRCS) $$(DEPS) $(1)$(2).res | check_$(2)
	$$(CC_$(2)) $$(CPPFLAGS) $$(CFLAGS) $$< $$(SRCS) $$(LDFLAGS) $(1)$(2).res $$(LDLIBS) -o $$@

$(1)$(2).res: $(1).rc | check_$(2)
	$$(WINDRES_$(2)) $$(WRFLAGS) -DTARGET=$(2) $$< $$@
endef

$(foreach project,$(PROJECTS),\
	$(foreach arch,$(ARCHS),\
		$(eval $(call BUILD_PROJECT,$(project),$(arch)))))
