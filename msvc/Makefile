#
# superUser 6.0
#
# Copyright 2019-2025 https://github.com/mspaintmsi/superUser
#
# Makefile LLVM-MSVC (version for GNU make)
#
# Author: Matrix3600 https://github.com/Matrix3600
#
# - Build x86 / x64 executables for Windows (Intel/AMD).
#
# Toolchain: LLVM (clang-cl/lld-link/llvm-rc) + MSVC/SDK headers/libraries
# Visual Studio does not need to be installed.
# MinGW or GCC are NOT used, even on Linux!
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

# -----------------------------------------------------------------------------
# Locate the MSVC and SDK headers/libraries
# -----------------------------------------------------------------------------

ifdef MSVC_LIBS_PATH
 #
 # Use the repackaged headers and libraries (the only Linux compatible ones!)
 #
 $(info Using MSVC_LIBS_PATH)
 MSVC_CRT_PATH = $(MSVC_LIBS_PATH)/crt
 MSVC_SDK_INCLUDE_PATH = $(MSVC_LIBS_PATH)/sdk/include
 MSVC_SDK_LIB_PATH = $(MSVC_LIBS_PATH)/sdk/lib
else
 msvc_path_ok =
 #
 # If Visual Studio is installed (includes SDK), use its environment variables
 #
 ifdef VCToolsInstallDir
  ifdef WindowsSdkDir
   ifdef WindowsSDKVersion
    $(info Using Visual Studio environment)
    # $(info VCToolsInstallDir=$(VCToolsInstallDir))
    # $(info WindowsSdkDir=$(WindowsSdkDir))
    # $(info WindowsSDKVersion=$(WindowsSDKVersion))
    # $(info )
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

# Convert $(1) pathname (backslashes) to Unix pathname
empty :=
space := $(empty) $(empty)
CONVERT_PATH = $(patsubst %/,%,$(subst //,/,$(subst \,/,$(1))))
QUOTE_SPACES = $(subst $(space),\ ,$(1))

MSVC_CRT_PATH := $(call CONVERT_PATH,$(MSVC_CRT_PATH))
MSVC_SDK_INCLUDE_PATH := $(call CONVERT_PATH,$(MSVC_SDK_INCLUDE_PATH))
MSVC_SDK_LIB_PATH := $(call CONVERT_PATH,$(MSVC_SDK_LIB_PATH))
# $(info MSVC_CRT_PATH=$(MSVC_CRT_PATH))
# $(info MSVC_SDK_INCLUDE_PATH=$(MSVC_SDK_INCLUDE_PATH))
# $(info MSVC_SDK_LIB_PATH=$(MSVC_SDK_LIB_PATH))
# $(info )

ifeq (,$(wildcard $(call QUOTE_SPACES,$(MSVC_CRT_PATH)/include/stdarg.h)))
 $(error ERROR: MSVC library not found)
endif

ifeq (,$(wildcard $(call QUOTE_SPACES,$(MSVC_SDK_INCLUDE_PATH)/um/windows.h)))
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

.DEFAULT_GOAL = intel

ARCHS = 32 64

HOST_32 =
HOST_64 =

# "clang-cl" is identical to "clang --driver-mode=cl".
# Use one or the other depending on their availability in your toolchain.
# CC = clang-cl
CC = clang --driver-mode=cl
CC_32 = $(HOST_32)$(CC)
CC_64 = $(HOST_64)$(CC)
CC_ = $(CC)

LD = lld-link
LD_32 = $(HOST_32)$(LD)
LD_64 = $(HOST_64)$(LD)
LD_ = $(LD)

RC = llvm-rc
RC_32 = $(HOST_32)$(RC)
RC_64 = $(HOST_64)$(RC)
RC_ = $(RC)

TARGETS_INTEL =

ifeq (CLANG64,$(MSYSTEM))	# MSYS2 CLANG64
 TARGETS_INTEL = x86 x64
else ifeq (32,$(findstring 32,$(MSYSTEM)))	# MSYS2 32-bit Intel/AMD (GNU)
 CC_32 =
 CC_64 =
else ifeq (64,$(findstring 64,$(MSYSTEM)))	# MSYS2 64-bit Intel/AMD (GNU)
 CC_64 =
 CC_32 =
else	# Cygwin, LLVM-MinGW or Linux
 HOST_32 = i686-pc-windows-msvc-
 HOST_64 = x86_64-pc-windows-msvc-

 # Check if a native toolchain (C compiler/linker/resource compiler) exists,
 # and detect its target architecture.
 NATIVE_CC_ARCH =
 ifneq (,$(and $(shell $(CC_) --version 2>$(DEVNUL)),$\
    $(shell $(LD_) --version 2>$(DEVNUL)),$\
    $(shell $(RC_) -h 2>$(DEVNUL))))	# If they both exist
  target := $(shell $(CC_) -print-target-triple 2>$(DEVNUL))
  ifneq (,$(filter %-windows-msvc,$(target)))
   ifneq (,$(filter i686-%,$(target)))
    NATIVE_CC_ARCH = 32
   else ifneq (,$(filter x86_64-%,$(target)))
    NATIVE_CC_ARCH = 64
   endif
  endif
 endif
 ifndef NATIVE_CC_ARCH
  CC_ =
 endif

 define CHECK_COMPILER
 # Check if a toolchain (C compiler/linker/resource compiler) exists for the
 # specified target architecture.
 # $(1): 32 or 64
 #
 ifeq (,$$(and $$(shell $$(CC_$(1)) --version 2>$$(DEVNUL)),$\
    $$(shell $$(LD_$(1)) --version 2>$$(DEVNUL)),$\
    $$(shell $$(RC_$(1)) -h 2>$$(DEVNUL))))	# If at least one does not exist
  ifdef CC_	# Use native ones if available
   CC_$(1) = $$(CC_)
   RC_$(1) = $$(RC_)
   LD_$(1) = $$(LD_)
  else  # Otherwise, disable this architecture
   CC_$(1) =
  endif
 endif
 endef

 $(foreach arch,$(ARCHS),$(eval $(call CHECK_COMPILER,$(arch))))

 ifdef CC_32
  TARGETS_INTEL += x86
 endif
 ifdef CC_64
  TARGETS_INTEL += x64
 endif
endif

# -----------------------------------------------------------------------------

# Intermediate directories for temporary files
INTDIR_32 = llvm-x86
INTDIR_64 = llvm-x64

.PHONY: all intel x86 x64 default clean \
	check_all check_intel check_32 check_64

default: $(.DEFAULT_GOAL)

all: $(TARGETS_INTEL) | check_all
intel: $(TARGETS_INTEL) | check_intel

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

define ERROR_NO_TOOLCHAIN
	@echo ERROR: No toolchain to build $(1).
	@exit 1
endef

check_all:
ifeq (,$(CC_32)$(CC_64))
	@echo ERROR: No suitable toolchain.
	@exit 1
endif

check_intel:
ifeq (,$(CC_32)$(CC_64))
	$(call ERROR_NO_TOOLCHAIN,intel)
endif

check_32:
ifndef CC_32
	$(call ERROR_NO_TOOLCHAIN,x86)
endif

check_64:
ifndef CC_64
	$(call ERROR_NO_TOOLCHAIN,x64)
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
CL_32 = $(CL) --target=i686-pc-windows-msvc
CL_64 = $(CL) --target=x86_64-pc-windows-msvc
export CL

# Linker
LINK = -incremental:no -nodefaultlib:libcmt.lib -manifest:no -subsystem:console
LINK += -opt:ref -dynamicbase -nxcompat -safeseh:no -release
export LINK

LIB_32 = $(MSVC_CRT_PATH)/lib/x86;$(MSVC_SDK_LIB_PATH)/um/x86
LIB_64 = $(MSVC_CRT_PATH)/lib/x64;$(MSVC_SDK_LIB_PATH)/um/x64
export LIB

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

# ENV_VAR: Define the $(1) environment variable for $(2) architecture
# e.g., $(call ENV_VAR,LIB,32) evaluates to LIB=$(LIB_32)
ENV_VAR = $(eval $(1)=$($(1)_$(2)))

define BUILD_OBJECTS
# $(1): 32 or 64

# Create the intermediate directory for the $(1) architecture
$(INTDIR_$(1)):
	$$(info --- Create directory $(INTDIR_$(1)) ---)
	@mkdir $(INTDIR_$(1))

OBJS_$(1) := $(OBJS:%=$(INTDIR_$(1))/%)

# Compile the secondary C source files
$$(OBJS_$(1)): $$(SRCS) $$(DEPS) | check_$(1) \
		$$(INTDIR_$(1)) ; $$(call ENV_VAR,CL,$(1))
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
$$(INTDIR_$(2))/$(1).obj: ../$(1).c $$(DEPS) | check_$(2) \
		$$(INTDIR_$(2)) ; $$(call ENV_VAR,CL,$(2))
	$$(info --- Compile $(1).c ($(2)) ---)
	$$(CC_$(2)) $$(CFLAGS) $$< -Fo$$(INTDIR_$(2))/

# Link the project
$(1)$(2).exe: $$(INTDIR_$(2))/$(1).obj $$(OBJS_$(2)) \
		$$(INTDIR_$(2))/$(1).res | check_$(2) ; $$(call ENV_VAR,LIB,$(2))
	$$(info --- Link $(1)$(2).exe ---)
	$$(LD_$(2)) $$(LDFLAGS_$(1)_$(2)) -out:$$@ $$(INTDIR_$(2))/$(1).obj \
		$$(OBJS_$(2)) $$(INTDIR_$(2))/$(1).res $$(LDLIBS_$(2))

# Compile the resource file
$$(INTDIR_$(2))/$(1).res: ../$(1).rc | check_$(2) $$(INTDIR_$(2))
	$$(info --- Compile $(1).rc ($(2)) ---)
	$$(RC_$(2)) $$(RCFLAGS) -D TARGET=$(2) -FO $$@ $$<

endef

$(foreach project,$(PROJECTS),\
	$(foreach arch,$(ARCHS),\
		$(eval $(call BUILD_PROJECT,$(project),$(arch)))))
