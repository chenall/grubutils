ifeq ($(CC),cc)
     CC := gcc
endif
OBJCOPY := objcopy
RM      := rm
MV      := mv
MKDIR   := mkdir

DATE = $(shell date -u +%Y-%m-%d)

GCCVERSIONGTEQ5 := $(shell expr `$(CC) -dumpversion | cut -f1 -d.` \>= 5)
GCCVERSIONGTEQ8 := $(shell expr `$(CC) -dumpversion | cut -f1 -d.` \>= 8)

CFLAGS := -Wl,--build-id=none -m64 -mno-sse -nostdlib \
          -fno-zero-initialized-in-bss -fno-function-cse \
          -fshort-wchar -fno-jump-tables -Wl,-N

ifeq "$(GCCVERSIONGTEQ5)" "1"
	CFLAGS += -no-pie
else
	CFLAGS += -fPIE
endif

OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c)) fat/fat.o
MODULES := $(patsubst %.c,%.mod,$(wildcard *.c)) fat/fat.mod

.PHONY: all
all: prepare $(MODULES)

	cat ntboot/ntboot >> bin/ntboot
	cp -f ChangeLog.txt bin/
	zip -9 -j ../g4eext-$(DATE).zip bin/*

%.o: %.c
	$(CC) $(CFLAGS) -Iinclude/ $< -o $@

%.mod: %.o
	$(OBJCOPY) -O binary $< $@

ifeq ($(GCCVERSIONGTEQ8),1)
#	echo eq
	dd bs=1 skip=32 if=$@ of=$@new
	$(MV) $@new bin/$$(basename $@new .modnew)
	$(RM) -f $@
else
#	echo neq
	$(MV) $@ bin/$$(basename $@ .mod)
endif

.PHONY: prepare
prepare:
	$(MKDIR) -p bin

.PHONY: clean
clean:
ifeq ($(GCCVERSIONGTEQ8),1)
	$(RM) -f *.o *.mod *.modnew bin/*
else
	$(RM) -f *.o *.mod fat/*.o fat/*.mod bin/* ../*.zip
endif
	
