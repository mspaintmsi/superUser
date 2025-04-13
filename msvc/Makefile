#
# superUser 6.0
#
# Copyright 2019-2025 https://github.com/mspaintmsi/superUser
#
# Makefile LLVM-MSVC (version for GNU make)
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
# Requirements:
#
# - [Not required for MSYS2 / CLANG64]
#   The LLVM toolchain for your development platform, e.g.,
#   "LLVM-*-win64.exe" for Windows or "LLVM-*-Linux-X64.tar.xz" for Linux.
#   https://github.com/llvm/llvm-project/releases
#
# - The MSVC headers and libraries (pulled from Visual Studio).
#   On Linux, we need a special repackaged version (with files renamed to
#   lowercase and include directives adjusted accordingly in header files).
#
# - The library files msvcrt32.lib and msvcrt64.lib in this msvc directory.
#   Read the msvc/README.md file for details.
#
# Environment variables must be set to the locations of the above tools:
#
# - [Not required for MSYS2 / CLANG64]
#   The LLVM toolchain "bin" directory must be added to the PATH.
#
# - The following environment variables must be set to locate the
#   MSVC/SDK library:
#
#     * MSVC_LIBS_PATH for the repackaged version of the MSVC library
#       (Linux or Windows).
#     OR
#     * VCToolsInstallDir, WindowsSdkDir and WindowsSDKVersion defined by the
#       Visual Studio environment (if installed!).
#     OR
#     * Individual variables pointing to library items:
#       MSVC_CRT_PATH, MSVC_SDK_INCLUDE_PATH and MSVC_SDK_LIB_PATH.
#
# There is currently a script to create the repackaged libraries, but it is
# not yet fully finalized to be released. If anyone is interested, maybe...
#
# Good luck...
#
# ----------------------------------------------------------------------------

#
# Try to locate the MSVC and SDK headers/libraries...
#
ifdef MSVC_LIBS_PATH
 #
 # Use the repackaged headers and libraries (the only Linux compatible ones!).
 #
 MSVC_CRT_PATH = $(MSVC_LIBS_PATH)/crt
 MSVC_SDK_INCLUDE_PATH = $(MSVC_LIBS_PATH)/sdk/include
 MSVC_SDK_LIB_PATH = $(MSVC_LIBS_PATH)/sdk/lib
else
 msvc_path_ok =
 ifdef MSVC_CRT_PATH
  ifdef MSVC_SDK_INCLUDE_PATH
   ifdef MSVC_SDK_LIB_PATH
    msvc_path_ok = 1
   endif
  endif
 endif
 ifndef msvc_path_ok
  #
  # If Visual Studio is installed (includes SDK), use the MSVC environment variables.
  #
  ifdef VCToolsInstallDir
   ifdef WindowsSdkDir
    ifdef WindowsSDKVersion
     $(info VCToolsInstallDir=$(VCToolsInstallDir))
     $(info WindowsSdkDir=$(WindowsSdkDir))
     $(info WindowsSDKVersion=$(WindowsSDKVersion))
     $(info )
     MSVC_CRT_PATH = $(VCToolsInstallDir)
     MSVC_SDK_INCLUDE_PATH = $(WindowsSdkDir)/Include/$(WindowsSDKVersion)
     MSVC_SDK_LIB_PATH = $(WindowsSdkDir)/Lib/$(WindowsSDKVersion)
     msvc_path_ok = 1
    endif
   endif
  endif
 endif
 ifndef msvc_path_ok
  $(error ERROR: MSVC/SDK libraries not found)
 endif
endif

# Convert $(1) pathname (if it contains backslashes) to Unix pathname
empty :=
space := $(empty) $(empty)
CONVERT_PATH = $(patsubst %/,%,$(subst //,/,$(subst \,/,$(1))))
QUOTE_SPACES = $(subst $(space),\ ,$(1))

MSVC_CRT_PATH := $(call CONVERT_PATH,$(MSVC_CRT_PATH))
MSVC_SDK_INCLUDE_PATH := $(call CONVERT_PATH,$(MSVC_SDK_INCLUDE_PATH))
MSVC_SDK_LIB_PATH := $(call CONVERT_PATH,$(MSVC_SDK_LIB_PATH))

$(info MSVC_CRT_PATH=$(MSVC_CRT_PATH))
$(info MSVC_SDK_INCLUDE_PATH=$(MSVC_SDK_INCLUDE_PATH))
$(info MSVC_SDK_LIB_PATH=$(MSVC_SDK_LIB_PATH))
$(info )

ifeq (,$(wildcard $(call QUOTE_SPACES,$(MSVC_CRT_PATH)/include/stdarg.h)))
 $(error ERROR: MSVC CRT library not found)
endif

ifeq (,$(wildcard $(call QUOTE_SPACES,$(MSVC_SDK_INCLUDE_PATH)/um/windows.h)))
 $(error ERROR: Windows SDK library not found)
endif

INTDIR_32 = llvm-x86
INTDIR_64 = llvm-x64

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


# _WIN32_WINNT: the minimal Windows version the app can run on.
# Windows Vista: the earliest to utilize the Trusted Installer.

CPPFLAGS = -D _WIN32_WINNT=_WIN32_WINNT_VISTA -D _NO_CRT_STDIO_INLINE -D _UNICODE -D UNICODE

CFLAGS = -c -W3 -O1 -Os -MT -GS- -Gy -TC
CFLAGS += -Wno-nonportable-include-path
CFLAGS += -Wno-microsoft-anon-tag
CFLAGS += -Wno-pragma-pack -Wno-unknown-pragmas -Wno-ignored-pragma-intrinsic
CFLAGS += -I"$(MSVC_CRT_PATH)/include" -I"$(MSVC_SDK_INCLUDE_PATH)/ucrt"
CFLAGS += -I"$(MSVC_SDK_INCLUDE_PATH)/um" -I"$(MSVC_SDK_INCLUDE_PATH)/shared"
CFLAGS_32 = $(CFLAGS) --target=i686-pc-windows-msvc
CFLAGS_64 = $(CFLAGS) --target=x86_64-pc-windows-msvc

LDFLAGS = -incremental:no -nodefaultlib:libcmt.lib -manifest:no -subsystem:console
LDFLAGS += -opt:ref -dynamicbase -nxcompat -safeseh:no -release
LDFLAGS_32 = $(LDFLAGS) -libpath:"$(MSVC_CRT_PATH)/lib/x86" -libpath:"$(MSVC_SDK_LIB_PATH)/um/x86"
LDFLAGS_64 = $(LDFLAGS) -libpath:"$(MSVC_CRT_PATH)/lib/x64" -libpath:"$(MSVC_SDK_LIB_PATH)/um/x64"

LDLIBS = advapi32.lib wtsapi32.lib
LDLIBS_32 = $(LDLIBS) msvcrt32.lib
LDLIBS_64 = $(LDLIBS) msvcrt64.lib

RCFLAGS = -C 65001 -L 0x0409

SRCS = ../tokens.c ../utils.c msvcrt.c
DEPS = ../tokens.h ../utils.h

OBJS := $(patsubst %.c,%.obj,$(notdir $(SRCS)))

PROJECTS = superUser sudo

x86: $(PROJECTS:%=%32.exe)
x64: $(PROJECTS:%=%64.exe)

.DELETE_ON_ERROR:


define BUILD_OBJECTS
# $(1): 32 or 64
#
# Create the intermediate directory for the $(1) architecture
$$(INTDIR_$(1)):
	@echo [$(INTDIR_$(1))]
	@mkdir $$(INTDIR_$(1))

OBJS_$(1) := $$(addprefix $$(INTDIR_$(1))/,$$(OBJS))

# Build objects
$$(OBJS_$(1)): $$(SRCS) $$(DEPS) | check_$(1) $$(INTDIR_$(1))
	@echo $$(notdir $$(SRCS))
	@$$(CC_$(1)) $$(CPPFLAGS) $$(CFLAGS_$(1)) $$(SRCS) -Fo$$(INTDIR_$(1))/
endef

$(foreach arch,$(ARCHS),$(eval $(call BUILD_OBJECTS,$(arch))))


define BUILD_PROJECT
# $(1): Project name
# $(2): 32 or 64
#
$$(INTDIR_$(2))/$(1).obj: ../$(1).c $$(DEPS) | check_$(2) $$(INTDIR_$(2))
	@echo $(1).c
	@$$(CC_$(2)) $$(CPPFLAGS) $$(CFLAGS_$(2)) $$< -Fo$$(INTDIR_$(2))/

$(1)$(2).exe: $$(INTDIR_$(2))/$(1).obj $$(OBJS_$(2)) $$(INTDIR_$(2))/$(1).res | check_$(2)
	@echo $(1)$(2).exe
	@$$(LD_$(2)) $$(LDFLAGS_$(2)) $$(LDLIBS_$(2)) -out:$$@ \
		$$(INTDIR_$(2))/$(1).res $$(INTDIR_$(2))/$(1).obj $$(OBJS_$(2))

$$(INTDIR_$(2))/$(1).res: ../$(1).rc | check_$(2) $$(INTDIR_$(2))
	@echo $(1).rc
	@$$(RC_$(2)) $$(RCFLAGS) -D TARGET=$(2) -FO $$@ $$<
endef

$(foreach project,$(PROJECTS),\
	$(foreach arch,$(ARCHS),\
		$(eval $(call BUILD_PROJECT,$(project),$(arch)))))
