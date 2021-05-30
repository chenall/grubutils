ifeq ($(SRCDIR),)
SRCDIR := $(dir $(firstword $(MAKEFILE_LIST)))
endif

PACKAGE = makemod

makemod_CSRC = makemod.c
makemod_SRCS = $(makemod_CSRC)
makemod_EXES = $(subst .c,$(EXEEXT),$(makemod_CSRC))

extra_SRCS = Makefile

all_SRCS = $(makemod_SRCS) $(extra_SRCS)
all_EXES = $(makemod_EXES)

-include common.mk $(SRCDIR)../common/common.mk

makemod$(EXEEXT): makemod.o
	$(CC) $(CFLAGS) -o $@ $^
