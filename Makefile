OPT32=-Wall -m32
OPT64=-Wall -m64
LDFLAGS=-lm -lPsapi -Wl,--gc-sections
CFLAGS=-std=c99 -Os -ffunction-sections -fdata-sections
STRIPFLAGS=-s -R .comment -R .gnu.version

GCC=gcc
STRIP=strip

all: superUser32.res superUser32.exe superUser64.res superUser64.exe

superUser32.exe: superUser.c
	$(GCC) $(OPT32) $< superUser32.res -o $@ $(CFLAGS) $(LDFLAGS)
	$(STRIP) $(STRIPFLAGS) $@
	
superUser64.exe: superUser.c
	$(GCC) $(OPT64) $< superUser64.res -o $@ $(CFLAGS) $(LDFLAGS)
	$(STRIP) $(STRIPFLAGS) $@