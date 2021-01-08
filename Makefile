OPT32=-Wall -m32
OPT64=-Wall -m64
LDFLAGS=-municode -lm -lPsapi -Wl,--gc-sections
CFLAGS=-std=c99 -Os -ffunction-sections -fdata-sections
STRIPFLAGS=-s -R .comment -R .gnu.version
WRFLAGS=--codepage=65001 -O coff

GCC=gcc -municode
STRIP=strip
WINDRES=windres

all: superUser32.res superUser32.exe superUser64.res superUser64.exe

superUser32.exe: superUser.c superUser32.res
	$(GCC) $(OPT32) $< superUser32.res -o $@ $(CFLAGS) $(LDFLAGS)
	$(STRIP) $(STRIPFLAGS) $@

superUser64.exe: superUser.c superUser64.res
	$(GCC) $(OPT64) $< superUser64.res -o $@ $(CFLAGS) $(LDFLAGS)
	$(STRIP) $(STRIPFLAGS) $@

superUser32.res: superUser.rc
	$(WINDRES) $(WRFLAGS) -F pe-i386 -DBUILD32 $< $@

superUser64.res: superUser.rc
	$(WINDRES) $(WRFLAGS) -F pe-x86-64 -DBUILD64 $< $@