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
int GRUB = 0x42555247;/* this is needed, see the following comment. */
/* gcc treat the following as data only if a global initialization like the
 * above line occurs.
 */
asm(".long 0x534F4434");
asm(ASM_BUILD_DATE);
/* a valid executable file for grub4dos must end with these 8 bytes */
asm(".long 0x03051805");
asm(".long 0xBCBAA7BA");

/* thank goodness gcc will place the above 8 bytes at the end of the b.out
 * file. Do not insert any other asm lines here.
 */

static int main (char *arg,int flags)
{
	struct realmode_regs int_regs = {0,0,0,-1,0,0,0,0x400,-1,-1,-1,-1,-1,0xFFFF1ACD,-1,-1};
	if (!*arg)
	{
		realmode_run((long)&int_regs);
		return printf("%04X-%02X-%02X\n",int_regs.ecx,(char)(int_regs.edx>>8),(char)int_regs.edx);
	}
	char tmp[8];
	int_regs.ecx = BCD2DEC(arg,4);
	int_regs.edx = (BCD2DEC(arg+5,2)<<8)+BCD2DEC(arg+8,2);
	int_regs.eax = 0x500;
	return realmode_run((long)&int_regs);;
}

static unsigned long BCD2DEC(const char *bcd,int len)
{
	unsigned long dec = 0;
	while (len--)
	{
		dec <<= 4;
		dec += (*bcd++ - '0');
	}
	return dec;
}