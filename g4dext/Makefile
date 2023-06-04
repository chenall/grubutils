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
GCCVERSIONGTEQ10 := $(shell expr `$(CC) -dumpversion | cut -f1 -d.` \< 10)

CFLAGS := -Wl,--build-id=none -m32 -mno-sse -nostdlib \
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
all: prepare $(MODULES) bin/fatmini
	zip -9 -j ../g4dext-$(DATE).zip bin/*

%.o: %.c
	$(CC) $(CFLAGS) -I../include/ $< -o $@

%.mod: %.o
	$(OBJCOPY) -O binary $< $@

#ifneq ($(A)$(B), 00),) # 'A || B' = (NOT)(NOT)'A || B' =(NOT)'(NOT)A && (NOT)B 
ifeq ($(GCCVERSIONGTEQ8)$(GCCVERSIONGTEQ10), 11)  # $(GCCVERSIONGTEQ8) && $(GCCVERSIONGTEQ10)
#	echo eq
	dd bs=1 skip=28 if=$@ of=$@new
	$(MV) $@new bin/$$(basename $@new .modnew)
	$(RM) -f $@
else
#	echo neq
	$(MV) $@ bin/$$(basename $@ .mod)
endif

bin/fatmini:
	$(CC) $(CFLAGS) -I../include/ -DFAT_MINI fat/fat.c -o fat/fatmini.o
	$(OBJCOPY) -O binary fat/fatmini.o fat/fatmini.mod
ifeq ($(GCCVERSIONGTEQ8)$(GCCVERSIONGTEQ10), 11)  # $(GCCVERSIONGTEQ8) && $(GCCVERSIONGTEQ10)
#	echo eq
	dd bs=1 skip=28 if=fat/fatmini.mod of=fat/fatmini.modnew
	$(MV) fat/fatmini.modnew bin/fatmini
	$(RM) -f fat/fatmini.mod
else
#	echo neq
	$(MV) fat/fatmini.mod bin/fatmini
endif	

.PHONY: prepare
prepare:
	$(MKDIR) -p bin

.PHONY: clean
clean:
ifeq ($(GCCVERSIONGTEQ8)$(GCCVERSIONGTEQ10), 1)
	$(RM) -f *.o *.mod *.modnew fat/*.o fat/*.mod fat/*.modnew bin/* ../*.zip	
else
	$(RM) -f *.o *.mod fat/*.o fat/*.mod bin/* ../*.zip
endif

