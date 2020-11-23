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

PACKAGE = weesetup

extra_SRCS = Makefile weesetup.c mbr.h

all_DIST =

cmn_SRCS = common.mk ldscript bin2h.c bin2h.pl utils.c utils.h xdio.c xdio.h xd16.asm

all_SRCS := weesetup.c $(extra_SRCS)
all_OBJS =
all_EXES = weesetup$(EXEEXT)

extra_CLEAN = mbr.h

-include common.mk $(SRCDIR)../common/common.mk

weesetup.o: mbr.h
weesetup$(EXEEXT): weesetup.o xdio.o utils.o
	$(CC) $(CFLAGS) -s -o$@ $^
	rm *.o *.d mbr.h
mbr.h: wee63.mbr $(BIN2H_DEPS)
	$(BIN2H_EXEC) $< $@ wee63_mbr
wee63.mbr: nt6mbr.bin ../wee/wee63.mbr
	./mkmbr