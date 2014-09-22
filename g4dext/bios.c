#include "grub4dos.h"

unsigned long long GRUB = 0x534f443442555247LL;/* this is needed, see the following comment. */
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
static unsigned int inl(unsigned short port);
static unsigned char inb(unsigned short port);
static void outb(unsigned short port, char val);
static unsigned short inw(unsigned short port);
static void outw(unsigned short port, unsigned short val);
static void outl(unsigned short port, int val);
/* gcc treat the following as data only if a global initialization like the
 * above line occurs.
 */

/* a valid executable file for grub4dos must end with these 8 bytes */
asm(ASM_BUILD_DATE);
asm(".long 0x03051805");
asm(".long 0xBCBAA7BA");

/* thank goodness gcc will place the above 8 bytes at the end of the b.out
 * file. Do not insert any other asm lines here.
 */

static int main(char *arg,int flags)
{
	struct realmode_regs int_regs = {0,0,0,-1,0,0,0,0,-1,-1,-1,-1,-1,0,-1,-1};
	unsigned long long ll;
	char *p;
	if (memcmp(arg,"int=",4) == 0)
	{
		arg += 4;
		if (!safe_parse_maxint(&arg,&ll))
			return 0;
		int_regs.eip= 0xFFFF00CD | ((char)ll << 8);
		arg = skip_to(0,arg);
		while (*arg)
		{
			p = arg;
			arg = skip_to(1,arg);
			if (!safe_parse_maxint(&arg,&ll))
				return 0;
			if (memcmp(p,"edi",3) == 0)
				int_regs.edi = (unsigned long)ll;
			else if (memcmp(p,"esi",3) == 0)
				int_regs.esi = (unsigned long)ll;
			else if (memcmp(p,"ebp",3) == 0)
				int_regs.ebp = (unsigned long)ll;
			else if (memcmp(p,"esp",3) == 0)
				int_regs.esp = (unsigned long)ll;
			else if (memcmp(p,"ebx",3) == 0)
				int_regs.ebx = (unsigned long)ll;
			else if (memcmp(p,"edx",3) == 0)
				int_regs.edx = (unsigned long)ll;
			else if (memcmp(p,"ecx",3) == 0)
				int_regs.ecx = (unsigned long)ll;
			else if (memcmp(p,"eax",3) == 0)
				int_regs.eax = (unsigned long)ll;
			else if (memcmp(p,"gs",2) == 0)
				int_regs.gs = (unsigned long)ll;
			else if (memcmp(p,"fs",2) == 0)
				int_regs.fs = (unsigned long)ll;
			else if (memcmp(p,"es",2) == 0)
				int_regs.es = (unsigned long)ll;
			else if (memcmp(p,"ds",2) == 0)
				int_regs.ds = (unsigned long)ll;
			else if (memcmp(p,"ss",2) == 0)
				int_regs.ss = (unsigned long)ll;
			else if (memcmp(p,"eip",3) == 0)
				int_regs.eip = (unsigned long)ll;
			else if (memcmp(p,"cs",2) == 0)
				int_regs.cs = (unsigned long)ll;
			else if (memcmp(p,"eflags",2) == 0)
				int_regs.eflags = (unsigned long)ll;
			else
				return 0;
			arg = skip_to(0,arg);
		}
		ll = realmode_run((long)&int_regs);
		flags = int_regs.eflags;
		printf("  EAX=%08X  EBX=%08X  ECX=%08X  EDX=%08X  ESI=%08X"
				 "\n  EDI=%08X  EBP=%08X  ESP=%08X  EIP=%08X  eFLAG=%08X"
				 "\n  DS=%04X  ES=%04X  FS=%04X  GS=%04X  SS=%04X  CS=%04X  %s %s %s %s %s %s %s %s",
				int_regs.eax,int_regs.ebx,int_regs.ecx,int_regs.edx,int_regs.esi,
				int_regs.edi,int_regs.ebp,int_regs.esp,int_regs.eip,flags,
				(short)int_regs.ds,(short)int_regs.es,(short)int_regs.fs,(short)int_regs.gs,(short)int_regs.ss,(short)int_regs.cs,
				(flags&(1<<11))?"OV":"NV",flags&(1<<10)?"DN":"UP",flags&(1<<9)?"EI":"DI",flags&(1<<7)?"NG":"PL",
				flags&(1<<6)?"ZR":"NZ",flags&(1<<4)?"AC":"NA",flags&4?"PE":"PO",flags&1?"CY":"NC"
				);
		return int_regs.eax;
	}
	else
	{
		unsigned short port;
		int val;
		p = skip_to(1,arg);
		if (!safe_parse_maxint(&p,&ll))
		{
			return 0;
		}
		port = (unsigned short)ll;
		if (*(unsigned short *)arg == 0x756F)
		{
			p = skip_to(0,p);
			if (!safe_parse_maxint(&p,&ll))
				return 0;
			switch(arg[3])
			{
				case 'b':
					outb(port,(char)ll);
					break;
				case 'w':
					outw(port,(unsigned short)ll);
					break;
				case 'l':
					outl(port,(unsigned int)ll);
					break;
				default:
					return 0;
			}
			return 1;
		}
		else if (*(unsigned short *)arg == 0x6E69)
		{
			switch(arg[2])
			{
				case 'b':
					ll=inb(port);
					break;
				case 'w':
					ll=inw(port);
					break;
				case 'l':
					ll=inl(port);
					break;
				default:
					return 0;
			}
			printf("0x%X",(unsigned int)ll);
			return ll;
		}
	}
	return 0;
}

static unsigned int inl(unsigned short port)
{
	int ret_val;
	__asm__ volatile ("inl %%dx,%%eax" : "=a" (ret_val) : "d"(port));
	return (int)ret_val;
}

static unsigned char inb(unsigned short port)
{
	char ret_val;
	__asm__ volatile ("inb %%dx,%%al" : "=a" (ret_val) : "d"(port));
	return ret_val;
}

static unsigned short inw(unsigned short port)
{
	unsigned short ret_val;
	__asm__ volatile ("inw %%dx,%%ax" : "=a" (ret_val) : "d"(port));
	return ret_val;
}

static void outw(unsigned short port, unsigned short val)
{
	__asm__ volatile ("outw %%ax,%%dx"::"a" (val), "d"(port));
}

static void outb(unsigned short port, char val)
{
	__asm__ volatile ("outb %%al,%%dx"::"a" (val), "d"(port));
}

static void outl(unsigned short port, int val)
{
	__asm__ volatile ("outl %%eax,%%dx" : : "a"(val), "d"(port));
}