SHELL=/bin/bash
SRC_C := builtins.c disk_io.c fsys_ext2fs.c fsys_fat.c fsys_ntfs.c
SRC_S := asm.S wee63start.S
BUILDROOT := .
LDFLAGS := 

ifneq ($(WEE127),1)
OBJDIR := .
DEFINES := 
else
OBJDIR := wee127
DEFINES := -DMBRSECTORS127
endif

ifneq ($(BUILDIDUNSUPPORTTED),1)
LDFLAGS += -Wl,--build-id=none
BUILDIDUNSUPPORTTED := 0
endif

OBJS := $(addprefix $(OBJDIR)/,$(subst .c,.o,$(SRC_C))) $(addprefix $(OBJDIR)/,$(subst .S,.o,$(SRC_S)))


#all: PRE $(OBJDIR)/wee63.mbr $(OBJDIR)/wee127.mbr POST
all: wee63 wee127

wee63: $(OBJDIR)/wee63.mbr

wee127:	wee127.mbr

.SUFFIXES: .exec .S .c .o

#PRE:
#	@mkdir -p wee127
#	-rm -f $(BUILDROOT)/wee127.mbr

#POST:
#	-cp wee127/wee63.mbr $(BUILDROOT)/wee127.mbr

clean:
	rm -f $(OBJDIR)/*.o $(OBJDIR)/wee63start $(OBJDIR)/pre_stage2
ifneq ($(WEE127),1)
	$(MAKE) WEE127=1 clean
else
	rm -f $(OBJDIR)/wee63.mbr
endif

distclean:
	rm -rf *~ *.o wee63start pre_stage2 wee127

ifneq ($(WEE127),1)
wee127.mbr: wee127/wee63.mbr
	cp wee127/wee63.mbr wee127.mbr

wee127/wee63.mbr: $(SRC_C) $(SRC_S) shared.h preset_menu_used
	$(MAKE) WEE127=1 BUILDIDUNSUPPORTTED=$(BUILDIDUNSUPPORTTED) wee127/wee63.mbr
endif

.PHONY: all clean distclean wee63 wee127

$(OBJDIR)/%.o: %.c $(BUILDROOT)/shared.h
	@mkdir -p $(OBJDIR)
	gcc -m32 -mno-sse -g -Os -fno-stack-protector -fno-builtin -mpreferred-stack-boundary=2 -fno-strict-aliasing -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables -nostdinc -Wall -Wmissing-prototypes -Wunused -Wshadow -Wpointer-arith -Wundef $(DEFINES) -c $*.c -o $(OBJDIR)/$*.o

$(OBJDIR)/%.o: %.S $(BUILDROOT)/shared.h
	@mkdir -p $(OBJDIR)
	gcc -m32 -mno-sse -g $(DEFINES) -c $*.S -o $(OBJDIR)/$*.o

$(OBJDIR)/wee63start: $(OBJDIR)/wee63start.o
	objcopy -O binary $< $@

$(OBJDIR)/pre_stage2: $(OBJDIR)/asm.o $(OBJDIR)/builtins.o $(OBJDIR)/disk_io.o $(OBJDIR)/fsys_ext2fs.o $(OBJDIR)/fsys_fat.o $(OBJDIR)/fsys_ntfs.o
	gcc -m32 -mno-sse -g -o $@ -nostdlib -Wl,-N -Wl,-Ttext -Wl,308200 -Wl,-N $(LDFLAGS) $(OBJDIR)/asm.o $(OBJDIR)/builtins.o $(OBJDIR)/disk_io.o $(OBJDIR)/fsys_ext2fs.o $(OBJDIR)/fsys_fat.o $(OBJDIR)/fsys_ntfs.o
	objcopy -O binary $@
	echo -n -e "\0260\002\032\0316" >> $@

$(OBJDIR)/wee63start.o: $(BUILDROOT)/wee63start.S $(OBJDIR)/stage2_size.h

$(OBJDIR)/asm.o: $(BUILDROOT)/asm.S $(BUILDROOT)/a20.inc

$(OBJDIR)/stage2_size.h: $(OBJDIR)/pre_stage2
	-rm -f $(OBJDIR)/stage2_size.h
	set dummy `ls -l $(OBJDIR)/pre_stage2`; \
	echo "#define STAGE2_SIZE $$6" > $(OBJDIR)/stage2_size.h

$(OBJDIR)/wee63.mbr: $(OBJDIR)/pre_stage2 $(OBJDIR)/wee63start $(BUILDROOT)/preset_menu_used
	-rm -f $(OBJDIR)/wee63.mbr
	echo -n -e "\000\000\000\000\000\000\000\000\000\000\000\000" | cat $(OBJDIR)/wee63start $(OBJDIR)/pre_stage2 - > $(OBJDIR)/wee63.mbr
	if [ -f $(BUILDROOT)/preset_menu_used ]; then echo -n -e "\000" | cat $(BUILDROOT)/preset_menu_used - >> $(OBJDIR)/wee63.mbr ;fi

$(BUILDROOT)/preset_menu_used:
	echo File not found: preset_menu_used
	exit 1
