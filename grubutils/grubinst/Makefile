#
#  GRUB Utilities --  Utilities for GRUB Legacy, GRUB2 and GRUB for DOS
#  Copyright (C) 2007 Bean (bean123ch@gmail.com)
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Affero General Public License as
#  published by the Free Software Foundation, either version 3 of the
#  License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Affero General Public License for more details.
#
#  You should have received a copy of the GNU Affero General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

ifeq ($(SRCDIR),)
SRCDIR := $(dir $(firstword $(MAKEFILE_LIST)))
endif

PACKAGE = grubinst

# To build the standalone NTFS boot sector, uncomment the following line
# BUILD_NTFSBS ?= y

# To build the grub2 header, uncomment the following line
# BUILD_G2HDR ?= y

# USE_RC ?=y

extra_SRCS = Makefile Makefile.vc6 Makefile.dos version resource.h ntfsbs.S g2hdr.S grldrstart45cp.S grldrstart45cnp.S grldrstart46ap.S grldrstart46anp.S grubinst.ico

all_DIST = readme.txt changelog.txt grubinst_gui.chs

cmn_SRCS = common.mk ldscript keytab.c keytab.h bin2h.c bin2h.pl ver2h.c ver2h.pl utils.c utils.h xdio.c xdio.h xd16.asm

all_SRCS := grubinst.c grubinst_gui.c grubinst_gui.rc grubmenu.c $(extra_SRCS)
all_OBJS = grubinst.o grubmenu.o grubinst_gui.o
all_EXES = grubinst$(EXEEXT) grubmenu$(EXEEXT) grubinst_gui$(EXEEXT)


ifeq ($(BUILD_NTFSBS),y)
all_BINS += ntfsbs.bin
endif

ifeq ($(BUILD_G2HDR),y)
all_BINS += g2hdr.bin
endif

extra_CLEAN = grldr45cp.mbr grldr46ap.mbr grldr45cnp.mbr grldr46anp.mbr version.h grub_mbr45cp.h grub_mbr46ap.h grub_mbr45cnp.h grub_mbr46anp.h grub_pbr46al.h *.ro

-include common.mk $(SRCDIR)../common/common.mk

grubinst$(EXEEXT): grubinst.o utils.o xdio.o keytab.o
	$(CC) $(CFLAGS) -o$@ $^

grubinst.o: version.h grub_mbr45cp.h grub_mbr46ap.h grub_pbr46al.h

grubinst_gui.o: version.h grub_mbr45cp.h grub_mbr46ap.h grub_mbr45cnp.h grub_mbr46anp.h grub_pbr46al.h

grldr45cp.mbr: grldrstart45cp.S ntfsbs.S
	$(CC) -nostdlib -DGRLDR_MBR -Wl,-N -Wl,-T -Wl,$(CMNDIR)ldscript -o grldr$(EXEEXT) $<
	$(OC) -O binary grldr$(EXEEXT) $@
	rm grldr$(EXEEXT)

grldr45cnp.mbr: grldrstart45cnp.S ntfsbs.S
	$(CC) -nostdlib -DGRLDR_MBR -Wl,-N -Wl,-T -Wl,$(CMNDIR)ldscript -o grldr$(EXEEXT) $<
	$(OC) -O binary grldr$(EXEEXT) $@
	rm grldr$(EXEEXT)
grldr46ap.mbr: grldrstart46ap.S ntfsbs.S
	$(CC) -nostdlib -DGRLDR_MBR -Wl,-N -Wl,-T -Wl,$(CMNDIR)ldscript -o grldr$(EXEEXT) $<
	$(OC) -O binary grldr$(EXEEXT) $@
	rm grldr$(EXEEXT)
grldr46anp.mbr: grldrstart46anp.S ntfsbs.S
	$(CC) -nostdlib -DGRLDR_MBR -Wl,-N -Wl,-T -Wl,$(CMNDIR)ldscript -o grldr$(EXEEXT) $<
	$(OC) -O binary grldr$(EXEEXT) $@
	rm grldr$(EXEEXT)

ifneq ($(IS_WIN32),)

ifeq ($(USE_RC),y)
HAVE_RC  = $(shell which rc.exe)
endif

ifneq ($(HAVE_RC),)

grubinst_gui.ro: grubinst_gui.rc resource.h grubinst.ico
	rc -fo grubinst_gui.res $<
	cvtres -nologo -machine:ix86 -out:$@ grubinst_gui.res
	rm -f grubinst_gui.res

else

grubinst_gui.ro: grubinst_gui.rc resource.h grubinst.ico
	windres -I$(SRCDIR) $< $@

endif

grubinst_gui$(EXEEXT): grubinst_gui.o utils.o keytab.o xdio.o grubinst_gui.ro
	$(CC) $(CFLAGS) -o$@ $^ -mwindows

else
grubinst$(EXEEXT): grubinst.o utils.o xdio.o keytab.o
	$(CC) $(CFLAGS) -o$@ $^
grubinst_gui$(EXEEXT): grubinst_gui.o utils.o keytab.o xdio.o
	$(CC) $(CFLAGS) -o$@ $^
endif

grub_mbr45cp.h: grldr45cp.mbr $(BIN2H_DEPS)
	./$(BIN2H_EXEC) $< $@ grub_mbr45cp
grub_mbr45cnp.h: grldr45cnp.mbr $(BIN2H_DEPS)
	./$(BIN2H_EXEC) $< $@ grub_mbr45cnp
grub_mbr46ap.h: grldr46ap.mbr $(BIN2H_DEPS)
	./$(BIN2H_EXEC) $< $@ grub_mbr46ap
grub_mbr46anp.h: grldr46anp.mbr $(BIN2H_DEPS)
	./$(BIN2H_EXEC) $< $@ grub_mbr46anp
grub_pbr46al.h: grldr46al.pbr $(BIN2H_DEPS)
	./$(BIN2H_EXEC) $< $@ grub_pbr46al
