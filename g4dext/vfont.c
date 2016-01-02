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

/* this is needed, see the comment in grubprog.h */
#include "grubprog.h"
/* Do not insert any other asm lines here. */

int
main (char *arg,int flags)
{
	/*
	通过调用BIOS   10h替换系统字模来显示汉字   
	入口:	ax=1100h
		bh=字模的高度(有效值：0～20h，默认值：10h)
		bl=被替换的字模集代号(有效值：0～7)
		cx=要替换的字模数
		dx=被替换的第一个字模所对应的字符的ASCII
		es:bp=新字模起始地址
		int   10h
	恢复系统原字符集：
		ax=1104h
		bl=字模集代号(有效值：0～7)   
		int 10h
	*/
	struct realmode_regs int_regs = {0,0,0,-1,0,0,0,0x1104,-1,-1,-1,-1,-1,0xFFFF10CD,-1,-1};
	/*
		详请请查看以下贴子。
		http://bbs.znpc.net/viewthread.php?tid=5838&page=12&fromuid=29#pid46781
		只是希望运行一条简单的 int 软中断指令，则用户应该设置 CS=0xFFFFFFFF, EIP=0xFFFFXXCD。其中 XX 表示中断号。
	*/
	if (*arg == 0)//如果没有任何参数，则恢复字模
	{
		return realmode_run((long)&int_regs);
	}

	if (!open(arg))
	{
		unsigned long long addr = 0LL;
		if (safe_parse_maxint(&arg,&addr))//这一部份只供测试代码使用。
		{
			errnum = 0;
			int_regs.eip = addr & 0xFFFFFFFF;
			int_regs.cs = int_regs.es = int_regs.ds =  addr >> 32;
			int_regs.esp = -1;
			return realmode_run((long)&int_regs);
		}
		errnum = 0;
		return !printf("Error: open_file\n");
	}
	unsigned int size;
	filepos = 0x59;
	if (! read((unsigned long long)(unsigned int)&size,4,GRUB_READ) || size != 0x52515350)
	{
		close();
		return !printf("Err: fil");
	}
	filepos = 0x1ce;
	if (! read((unsigned long long)(unsigned int)&size,2,GRUB_READ))
	{
		close();
		return !printf("Error:read1\n");
	}
	unsigned char prog[32] = {
			0xB9, 0x00, 0x00, 0xBA, 0xC0, 0x00, 0xBD, 0x20,
			0x7C, 0xB8, 0x00, 0x11, 0xBB, 0x00, 0x10, 0xCD,
			0x10, 0xBA, 0x80, 0x00, 0xBD, 0x00, 0x7E, 0xCD,
			0x10,
			/*ljmp 0000:8200*/
			0xEA, 0x00, 0x82, 0x00,0x00};
			/*
			以下程序用于恢复字模，汇编源码:
			B8 04 11	mov ax,1104
			B3 00		mov bl,0
			CD 10		int 10
			*/
			//0xB8,0x04,0x11,0xb3,0x00,0xcd,0x10,

	memmove((char*)0x7c00,prog,32);
	size &= 0xFFFF;
	if (size > 30)
		size = 30;
	int i;
	for (i=0;i<size;i++)
	{
		read((unsigned long long)(unsigned int)(0x7C20 + (i<<4)),16,GRUB_READ);
		read((unsigned long long)(unsigned int)(0x7E00 + (i<<4)),16,GRUB_READ);
	}
	close();
	*(short*)0x7C01 = size;//设置ECX值;
	int_regs.eip = 0x7C00;
	int_regs.cs = int_regs.es = int_regs.ds = 0;
	int_regs.esp = int_regs.ss = -1;
	return realmode_run((long)&int_regs);
}