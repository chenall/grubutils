CC      := gcc
OBJCOPY := objcopy
SED     := sed
RM      := rm
MV      := mv
MKDIR   := mkdir

_TIME_Y = `date +%-Y`
_TIME_m = `date +%-m`
_TIME_d = `date +%-d`
_TIME_H = `date +%-H`
_TIME_M = `date +%-M`
_TIME_S = `date +%-S`
DATE = $(shell date -u +%Y-%m-%d)
ASMDATE = $(shell date -u +".long "%Y%m%d)

CFLAGS = -nostdlib -DASM_BUILD_DATE=\""$(ASMDATE)"\" -fno-reorder-functions -fno-strict-aliasing -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables --entry main -Wl,-N -Wl,--build-id=none -m32 -mno-sse -Wl,-Ttext -Wl,0 -fPIE

OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c)) fat/fat.o
TMPFILES := $(patsubst %.c,%.tmp,$(wildcard *.c)) fat/fat.tmp
MODULES := $(patsubst %.c,%.mod,$(wildcard *.c)) fat/fat.mod

.PHONY: all
all: prepare $(MODULES) bin/fatmini
	zip -9 -j ../g4dext-$(DATE).zip bin/*

%.o: %.c
	$(CC) $(CFLAGS) -I../include/ $< -o $@

%.tmp: %.o
	$(OBJCOPY) -O binary $< $@

%.mod: %.tmp
	$(SED) -e '$$s/\x00*$$//' $< > $@
	$(MV) $@ bin/$$(basename -s .mod $@)

bin/fatmini:
	$(CC) $(CFLAGS) -I../include/ -DFAT_MINI fat/fat.c -o fat/fatmini.o
	$(OBJCOPY) -O binary fat/fatmini.o fat/fatmini.tmp
	$(SED) -e '$$s/\x00*$$//' fat/fatmini.tmp > bin/fatmini

.PHONY: prepare
prepare:
	$(MKDIR) -p bin

.PHONY: clean
clean:
	$(RM) -f *.o *.tmp *.mod fat/*.o fat/*.tmp bin/*