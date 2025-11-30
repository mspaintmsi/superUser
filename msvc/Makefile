#
# superUser 6.0
#
# Copyright 2019-2026 https://github.com/mspaintmsi/superUser
#
# Makefile LLVM-MSVC (version for GNU make)
#
# Author: Matrix3600 https://github.com/Matrix3600
#
# - Build x86 / x64 executables for Windows (Intel/AMD).
#
# Toolchain: LLVM (clang-cl/lld-link/llvm-rc) + MSVC/SDK headers/libraries
# Visual Studio does not need to be installed.
# MinGW or GCC are NOT used, even on Linux.
#
# Supported development OS and toolchains:
#
#    Toolchain     |       Runs on       | Generates executables for
# -------------------------------------------------------------------
# LLVM-MSVC          Windows (all),        Intel/AMD
#                    Linux,
#                    macOS
# MSYS2 / CLANG64,   Windows (x64)         Intel/AMD
# + MSVC
#
# Targets: *-pc-windows-msvc
# CRT version: MSVCRT
#
# Read the msvc/BUILD_LLVM-MSVC.md file for details.
#
# -----------------------------------------------------------------------------

override undefine build_targets
build_targets := $(if $(MAKECMDGOALS),$(filter-out clean,$(MAKECMDGOALS)),$\
  default)

# -----------------------------------------------------------------------------
# Locate the MSVC and SDK headers/libraries
# -----------------------------------------------------------------------------

ifdef MSVC_LIBS_PATH
 # Use the repackaged headers and libraries (the only Linux compatible ones)
 $(if $(build_targets),$(info Using MSVC_LIBS_PATH))
 MSVC_CRT_PATH = $(MSVC_LIBS_PATH)/crt
 MSVC_SDK_INCLUDE_PATH = $(MSVC_LIBS_PATH)/sdk/include
 MSVC_SDK_LIB_PATH = $(MSVC_LIBS_PATH)/sdk/lib
else
 override undefine msvc_path_ok
 msvc_path_ok =
 # If Visual Studio is installed (includes SDK), use its environment variables
 ifdef VCToolsInstallDir
  ifdef WindowsSdkDir
   ifdef WindowsSDKVersion
    $(if $(build_targets),$(info Using Visual Studio environment))
    MSVC_CRT_PATH = $(VCToolsInstallDir)
    MSVC_SDK_INCLUDE_PATH = $(WindowsSdkDir)/Include/$(WindowsSDKVersion)
    MSVC_SDK_LIB_PATH = $(WindowsSdkDir)/Lib/$(WindowsSDKVersion)
    msvc_path_ok = 1
   endif
  endif
 endif
 ifndef msvc_path_ok
  $(error ERROR: MSVC/SDK libraries not found)
 endif
endif

# Clean pathname and escape spaces
override undefine empty
override undefine space
empty :=
space := $(empty) $(empty)
CLEAN_PATH = $(patsubst %/,%,$(subst //,/,$(subst \,/,$(1))))
ESC_SPACE = $(subst $(space),\ ,$(1))

MSVC_CRT_PATH := $(call CLEAN_PATH,$(MSVC_CRT_PATH))
MSVC_SDK_INCLUDE_PATH := $(call CLEAN_PATH,$(MSVC_SDK_INCLUDE_PATH))
MSVC_SDK_LIB_PATH := $(call CLEAN_PATH,$(MSVC_SDK_LIB_PATH))

ifeq (,$(wildcard $(call ESC_SPACE,$(MSVC_CRT_PATH)/include/stdarg.h)))
 $(error ERROR: MSVC library not found)
endif

ifeq (,$(wildcard $(call ESC_SPACE,$(MSVC_SDK_INCLUDE_PATH)/um/windows.h)))
 $(error ERROR: Windows SDK library not found)
endif

# -----------------------------------------------------------------------------
# Detect system and available toolchains
# -----------------------------------------------------------------------------

NATIVEWIN =
DEVNUL = /dev/null
ifeq ($(OS),Windows_NT)
 ifeq ($(shell echo $$PPID),$$PPID)
  NATIVEWIN = 1
  DEVNUL = nul
 endif
endif

# "clang-cl" is equivalent to "clang --driver-mode=cl".
# "lld-link" is equivalent to "lld -flavor link".

CC = clang-cl
CC_ALT = clang --driver-mode=cl
CC_32 = $(CC) --target=i686-pc-windows-msvc
CC_64 = $(CC) --target=x86_64-pc-windows-msvc

LD = lld-link
LD_ALT = lld -flavor link

RC = llvm-rc

# Check if a native toolchain (C compiler/linker/resource compiler) exists,
# and detect its target architecture.
ifdef build_targets
 ifeq (,$(shell $(CC) --version 2>$(DEVNUL)))
  CC = $(CC_ALT)
 endif
 ifeq (,$(shell $(LD) --version 2>$(DEVNUL)))
  LD = $(LD_ALT)
 endif
 override undefine native_toolchain
 native_toolchain =
 ifneq (,$(and $(shell $(CC) --version 2>$(DEVNUL)),$\
    $(shell $(LD) --version 2>$(DEVNUL)),$\
    $(shell $(RC) -h 2>$(DEVNUL))))  # If they both exist
  target := $(shell $(CC) -print-target-triple 2>$(DEVNUL))
  ifneq (,$(filter %-windows-msvc,$(target)))
   native_toolchain = 1
  endif
 endif
 ifndef native_toolchain
  CC =
 endif
endif

# -----------------------------------------------------------------------------

.DEFAULT_GOAL = intel

ARCHS = 32 64
TARGETS_INTEL = x86 x64

# Intermediate directories for temporary files
override undefine INTDIR_32
override undefine INTDIR_64
INTDIR_32 = llvm-x86
INTDIR_64 = llvm-x64

.PHONY: all intel x86 x64 default clean check_all

default: $(.DEFAULT_GOAL)

all: $(TARGETS_INTEL) | check_all
intel: $(TARGETS_INTEL) | check_all

clean:
ifdef NATIVEWIN
	if exist *.exe del *.exe
	if exist $(INTDIR_32)\ rd /s/q $(INTDIR_32)
	if exist $(INTDIR_64)\ rd /s/q $(INTDIR_64)
else
	rm -f *.exe
	rm -rf $(INTDIR_32)
	rm -rf $(INTDIR_64)
endif

check_all:
ifndef CC
	@echo ERROR: No suitable toolchain.
	@exit 1
endif

# -----------------------------------------------------------------------------
# Set environment variables for the compiler and linker
# -----------------------------------------------------------------------------

# C Compiler
INCLUDE = $(MSVC_CRT_PATH)/include;$(MSVC_SDK_INCLUDE_PATH)/ucrt;$\
  $(MSVC_SDK_INCLUDE_PATH)/um;$(MSVC_SDK_INCLUDE_PATH)/shared
export INCLUDE

# _WIN32_WINNT: the minimal Windows version the app can run on.
# Windows Vista: the earliest to utilize the Trusted Installer.

CL = -W3 -O1 -Os -MT -GS- -Gy -TC
CL += -Wno-nonportable-include-path -Wno-microsoft-anon-tag
CL += -Wno-pragma-pack -Wno-unknown-pragmas -Wno-ignored-pragma-intrinsic
CL += -D _WIN32_WINNT=_WIN32_WINNT_VISTA -D _NO_CRT_STDIO_INLINE
CL += -D _UNICODE -D UNICODE
export CL

# Linker
LINK = -incremental:no -nodefaultlib:libcmt.lib -manifest:no -subsystem:console
LINK += -opt:ref -dynamicbase -nxcompat -safeseh:no -release
export LINK

LIB_32 = $(MSVC_CRT_PATH)/lib/x86;$(MSVC_SDK_LIB_PATH)/um/x86
LIB_64 = $(MSVC_CRT_PATH)/lib/x64;$(MSVC_SDK_LIB_PATH)/um/x64

# -----------------------------------------------------------------------------

PROJECTS = superUser sudo

# Initialize the variables
INITIALIZE_LDFLAGS = LDFLAGS_$(1)_$(2) =
$(foreach project,$(PROJECTS),$(foreach arch,$(ARCHS),\
  $(eval $(call INITIALIZE_LDFLAGS,$(project),$(arch)))))

CFLAGS = -c

LDFLAGS_64 = -merge:".pdata=.text"

LDLIBS = advapi32.lib wtsapi32.lib
LDLIBS_32 = $(LDLIBS) msvcrt32.lib
LDLIBS_64 = $(LDLIBS) msvcrt64.lib

RCFLAGS = -C 65001 -L 0x0409

SRCS = ../tokens.c ../utils.c msvcrt.c
DEPS = ../tokens.h ../utils.h

OBJS := $(patsubst %.c,%.obj,$(notdir $(SRCS)))

x86: $(PROJECTS:%=%32.exe)
x64: $(PROJECTS:%=%64.exe)

.DELETE_ON_ERROR:

define BUILD_OBJECTS
# $(1): 32 or 64

# Create the intermediate directory for the $(1) architecture
$(INTDIR_$(1)):
	$$(info --- Create directory $(INTDIR_$(1)) ---)
	@mkdir $(INTDIR_$(1))

OBJS_$(1) := $(OBJS:%=$(INTDIR_$(1))/%)

# Compile the secondary C source files
$$(OBJS_$(1)): $$(SRCS) $$(DEPS) | check_all $$(INTDIR_$(1))
	$$(info --- Compile $$(notdir $$(SRCS)) ($(1)) ---)
	$$(CC_$(1)) $$(CFLAGS) $$(SRCS) -Fo$$(INTDIR_$(1))/

endef

$(foreach arch,$(ARCHS),$(eval $(call BUILD_OBJECTS,$(arch))))


define BUILD_PROJECT
# $(1): Project name
# $(2): 32 or 64

ifndef LDFLAGS_$(1)_$(2)
 LDFLAGS_$(1)_$(2) =
 ifdef LDFLAGS_$(2)
  LDFLAGS_$(1)_$(2) += $$(LDFLAGS_$(2))
 endif
endif

# Compile the main C source file
$$(INTDIR_$(2))/$(1).obj: ../$(1).c $$(DEPS) | check_all $$(INTDIR_$(2))
	$$(info --- Compile $(1).c ($(2)) ---)
	$$(CC_$(2)) $$(CFLAGS) $$< -Fo$$(INTDIR_$(2))/

# Link the project
$(1)$(2).exe: export LIB=$$(LIB_$(2))
$(1)$(2).exe: $$(INTDIR_$(2))/$(1).obj $$(OBJS_$(2)) \
		$$(INTDIR_$(2))/$(1).res | check_all
	$$(info --- Link $(1)$(2).exe ---)
	$$(LD) $$(LDFLAGS_$(1)_$(2)) -out:$$@ $$(INTDIR_$(2))/$(1).obj \
		$$(OBJS_$(2)) $$(INTDIR_$(2))/$(1).res $$(LDLIBS_$(2))

# Compile the resource file
$$(INTDIR_$(2))/$(1).res: ../$(1).rc | check_all $$(INTDIR_$(2))
	$$(info --- Compile $(1).rc ($(2)) ---)
	$$(RC) $$(RCFLAGS) -D TARGET=$(2) -FO $$@ $$<

endef

$(foreach project,$(PROJECTS),\
  $(foreach arch,$(ARCHS),\
    $(eval $(call BUILD_PROJECT,$(project),$(arch)))))
