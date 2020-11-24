CC      := gcc
OBJCOPY := objcopy
RM      := rm

CFLAGS  := -m32 -mno-sse

all: bin2c umbr umbr.h

bin2c: bin2c.c
	$(CC) $< -o $@

umbr.o: umbr.S
	$(CC) $(CFLAGS) -c $< -o $@

umbr: umbr.o
	$(OBJCOPY) -O binary $< $@

umbr.h: umbr
	./bin2c $< $@ umbr

clean:
	$(RM) -f bin2c *.o umbr umbr.h
