CC      := gcc
OBJCOPY := objcopy
RM      := rm
MV      := mv
MKDIR   := mkdir

DATE = $(shell date -u +%Y-%m-%d)

CFLAGS := -Wl,--build-id=none -m64 -mno-sse -nostdlib \
          -fno-zero-initialized-in-bss -fno-function-cse \
          -fno-jump-tables -Wl,-N -fPIE

OBJECTS := $(patsubst %.c,%.o,$(wildcard *.c))
MODULES := $(patsubst %.c,%.mod,$(wildcard *.c))

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