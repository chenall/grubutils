CC      := gcc
OBJCOPY := objcopy
RM      := rm
MV      := mv
MKDIR   := mkdir

DATE = $(shell date -u +%Y-%m-%d)

GCCVERSIONGTEQ5 := $(shell expr `$(CC) -dumpversion | cut -f1 -d.` \>= 5)

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
	zip -9 -j ../g4eext-$(DATE).zip bin/*

%.o: %.c
	$(CC) $(CFLAGS) -Iinclude/ $< -o $@

%.mod: %.o
	$(OBJCOPY) -O binary $< $@
	$(MV) $@ bin/$$(basename -s .mod $@)

.PHONY: prepare
prepare:
	$(MKDIR) -p bin

.PHONY: clean
clean:
	$(RM) -f *.o *.mod bin/*
