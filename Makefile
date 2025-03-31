#
# superUser 6.0
#
# Copyright 2019-2025 https://github.com/mspaintmsi/superUser
#
# superUser Makefile
#
# Build x86 / x64 executables for Windows (Intel/AMD).
# Build native ARMv7 / ARM64 executables for "Windows on Arm".
#
# Supported development OS and toolchains:
#
#	* Windows (Intel/AMD)
#		- Cygwin / MinGW-w64	-> [Intel/AMD]
#		- MSYS2 / MINGW32, MINGW64, CLANG64, UCRT64	-> [Intel/AMD]
#		- LLVM-MinGW	-> [Intel/AMD, ARM]
#
#	* Windows 11 on Arm64
#	  - MSYS2 / CLANGARM64	-> [ARM]
#
#	* Linux
#		- GCC-MinGW (gcc-mingw-w64 package)	-> [Intel/AMD]
#		- LLVM-MinGW	-> [Intel/AMD, ARM]
#
# Read BUILD_INSTRUCTIONS.md file for details.
#

NATIVEWIN =
DEVNUL = /dev/null
ifeq ($(OS),Windows_NT)
 ifeq ($(patsubst PPID=%,%,$(filter PPID=%,$(shell set))),$(PPID))
  NATIVEWIN = 1
  DEVNUL = nul
 endif
endif

DEFAULT_TARGET = intel
ifeq ($(OS),Windows_NT)
 ifneq (,$(filter ARM%,$(PROCESSOR_ARCHITECTURE)))
  DEFAULT_TARGET = arm
 endif
else
 proctype = $(shell uname -p 2>$(DEVNUL))
# ifneq (,$(strip $(foreach arch,aarch64 arm,$(findstring $(arch),$(proctype)))))
 ifneq (,$(filter aarch64% arm%,$(proctype)))
  DEFAULT_TARGET = arm
 endif
endif

HOST32 =
HOST64 =
HOSTA32 =
HOSTA64 =
CC32 = $(HOST32)gcc
CC64 = $(HOST64)gcc
CCA32 = $(HOSTA32)gcc
CCA64 = $(HOSTA64)gcc
WINDRES32 = $(HOST32)windres
WINDRES64 = $(HOST64)windres
WINDRESA32 = $(HOSTA32)windres
WINDRESA64 = $(HOSTA64)windres

TARGETS_INTEL =
TARGETS_ARM =

ifeq (CLANGARM64,$(MSYSTEM))	# MSYS2 CLANGARM64
 DEFAULT_TARGET = arm
 TARGETS_ARM = arm64
 CCA64 = clang
 CC32 =
 CC64 =
 CCA32 =
else ifeq (32,$(findstring 32,$(MSYSTEM)))	# MSYS2 32-bit Intel/AMD
 DEFAULT_TARGET = intel
 TARGETS_INTEL = x86
 CC64 =
 CCA32 =
 CCA64 =
else ifeq (64,$(findstring 64,$(MSYSTEM)))	# MSYS2 64-bit Intel/AMD
 DEFAULT_TARGET = intel
 TARGETS_INTEL = x64
 CC32 =
 CCA32 =
 CCA64 =
else	# Cygwin, LLVM-MinGW or Linux
 HOST32 = i686-w64-mingw32-
 HOST64 = x86_64-w64-mingw32-
 HOSTA32 = armv7-w64-mingw32-
 HOSTA64 = aarch64-w64-mingw32-

 ifneq (,$(shell $(CC32) --version 2>$(DEVNUL)))	# 32-bit Intel/AMD compiler exists
  TARGETS_INTEL += x86
 else
  CC32 =
 endif
 ifneq (,$(shell $(CC64) --version 2>$(DEVNUL)))	# 64-bit Intel/AMD compiler exists
  TARGETS_INTEL += x64
 else
  CC64 =
 endif
 ifneq (,$(shell $(CCA32) --version 2>$(DEVNUL)))	# 32-bit ARM compiler exists
  TARGETS_ARM += arm32
 else
  CCA32 =
 endif
 ifneq (,$(shell $(CCA64) --version 2>$(DEVNUL)))	# 64-bit ARM compiler exists
  TARGETS_ARM += arm64
 else
  CCA64 =
 endif
endif


.PHONY: all intel arm x86 x64 arm32 arm64 clean \
	default check checkintel checkarm check32 check64 checkA32 checkA64

default: $(DEFAULT_TARGET)

all: $(TARGETS_INTEL) $(TARGETS_ARM) | check
intel: $(TARGETS_INTEL) | checkintel
arm: $(TARGETS_ARM) | checkarm

clean:
ifdef NATIVEWIN
	if exist *.exe del *.exe
	if exist *.res del *.res
else
	rm -f *.exe *.res
endif

check:
ifeq (,$(CC32)$(CC64)$(CCA32)$(CCA64))
	@echo ERROR: No suitable toolchain.
	@exit 1
endif

checkintel:
ifeq (,$(CC32)$(CC64))
	@echo ERROR: No suitable toolchain to build intel.
	@exit 1
endif

checkarm:
ifeq (,$(CCA32)$(CCA64))
	@echo ERROR: No suitable toolchain to build arm.
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

checkA32:
ifndef CCA32
	@echo ERROR: No toolchain to build arm32.
	@exit 1
endif

checkA64:
ifndef CCA64
	@echo ERROR: No toolchain to build arm64.
	@exit 1
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

define COMPILE_PROJECT
# $(1) is the project name
# $(2) is 32, 64, A32 or A64
#
$(1)$(2).exe: $(1)$(2).res | check$(2)
	$$(CC$(2)) $$(CPPFLAGS) $$(CFLAGS) $(1).c $$(SRCS) $$(LDFLAGS) $(1)$(2).res $$(LDLIBS) -o $$@
endef

define COMPILE_PROJECT_RESOURCE
# $(1) is the project name
# $(2) is 32, 64, A32 or A64
#
$(1)$(2).res: $(1).rc | check$(2)
	$$(WINDRES$(2)) $$(WRFLAGS) -DTARGET=$(2) $$< $$@
endef

define BUILD_PROJECT
# $(1) is the project name
# $(2) is 32, 64, A32 or A64
#
$(1)$(2).exe: $$(SRCS) $$(DEPS)

$(call COMPILE_PROJECT,$(1),$(2))
$(call COMPILE_PROJECT_RESOURCE,$(1),$(2))
endef


$(foreach project,$(PROJECTS),\
	$(foreach arch,$(ARCHS),\
		$(eval $(call BUILD_PROJECT,$(project),$(arch)))))
