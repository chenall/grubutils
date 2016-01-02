#include "grub4dos.h"

struct realmode_regs {
	unsigned long edi; // as input and output
	unsigned long esi; // as input and output
	unsigned long ebp; // as input and output
	unsigned long esp; // stack pointer, as input
	unsigned long ebx; // as input and output
	unsigned long edx; // as input and output
	unsigned long ecx; // as input and output
	unsigned long eax;// as input and output
	unsigned long gs; // as input and output
	unsigned long fs; // as input and output
	unsigned long es; // as input and output
	unsigned long ds; // as input and output
	unsigned long ss; // stack segment, as input
	unsigned long eip; // instruction pointer, as input, 
	unsigned long cs; // code segment, as input
	unsigned long eflags; // as input and output
};
static unsigned long BCD2DEC(const char *bcd,int len);

/* this is needed, see the comment in grubprog.h */
#include "grubprog.h"
/* Do not insert any other asm lines here. */

/*
 INT 1aH 03H: Set Time on Real-Time Clock
 Expects: AH    03H

          CH    hours, in BCD^

          CL    minutes, in BCD

          DH    seconds, in BCD

          DL    00h = no Daylight Savings Time option

                01h = Daylight Savings Time option

 Returns: AH    0

          AL    value written to CMOS 0bH register

          CF    NC (0) clock operating

                CY (1) clock not operating or busy being updated
*/

int main (char *arg,int flags)
{
	struct realmode_regs int_regs = {0,0,0,-1,0,0,0,0x200,-1,-1,-1,-1,-1,0xFFFF1ACD,-1,-1};
	if (!*arg)
	{
		realmode_run((long)&int_regs);
		return printf("%02X:%02X:%02X\n",(unsigned char)(int_regs.ecx>>8),(unsigned char)(int_regs.ecx),(unsigned char)(int_regs.edx>>8));
	}
	char tmp[8];
	int_regs.ecx = (BCD2DEC(arg,2)<<8)|BCD2DEC(arg+3,2);
	int_regs.edx = BCD2DEC(arg+6,2)<<8;
	int_regs.eax = 0x300;
	return realmode_run((long)&int_regs);;
}

static unsigned long BCD2DEC(const char *bcd,int len)
{
	unsigned long dec = 0;
	while (len--)
	{
		dec <<= 4;
		dec += (*bcd++&0xF);
	}
	return dec;
}