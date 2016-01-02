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

	typedef unsigned long DWORD;
	typedef unsigned long LONG;
	typedef unsigned short WORD;
	struct { /* bmfh */ 
			WORD bfType;
			DWORD bfSize; 
			DWORD bfReserved1; 
			DWORD bfOffBits;
		} __attribute__ ((packed)) bmfh;
	struct { /* bmih */ 
		DWORD biSize; 
		LONG biWidth; 
		LONG biHeight; 
		WORD biPlanes; 
		WORD biBitCount; 
		DWORD biCompression; 
		DWORD biSizeImage; 
		LONG biXPelsPerMeter; 
		LONG biYPelsPerMeter; 
		DWORD biClrUsed; 
		DWORD biClrImportant;
	} __attribute__ ((packed)) bmih;

	if (! read((unsigned long long)(unsigned int)&bmfh,sizeof(bmfh),GRUB_READ) || bmfh.bfType != 0x4d42)
	{
		close();
		return !printf("Err: fil");
	}
	if (! read((unsigned long long)(unsigned int)&bmih,sizeof(bmih),GRUB_READ))
	{
		close();
		return !printf("Error:read1\n");
	}

	/*
	入口:	ax=1100h
		bh=字模的高度(有效值：0～20h，默认值：10h)
		bl=被替换的字模集代号(有效值：0～7)
		cx=要替换的字模数
		dx=被替换的第一个字模所对应的字符的ASCII
		es:bp=新字模起始地
	*/
	int perline_char = (bmih.biWidth + 31)>>5<<2;
	int i,j;
	char s1[perline_char+4];
	unsigned int size = (bmih.biHeight + 15) >> 4;
	char ch = 0xb0;
	if (size * perline_char > 79)
	{
		close();
		return 0;
	}
	filepos = bmfh.bfOffBits;
	while (size--)
	{
		char *chars = (char *)0x40000 + size * perline_char * 16;
		for (i=15;i>=0;--i)
		{
			read((unsigned long long)(unsigned int)s1,perline_char,GRUB_READ);
			char *s = s1;
			for (j=0;j<perline_char;++j)
			{
				chars[j*16+i] = *s++;
			}
		}
		putchar('\n');
		for (i = 0;i < perline_char;++i)
			putchar(ch++);
	}
	close();
	size = ((bmih.biHeight + 15) >> 4) * perline_char;
	int_regs.eax = 0x1100;
	int_regs.ebx = 0x1000;
	int_regs.ecx = size;
	int_regs.edx = 0xb0;
	int_regs.es = 0x4000;
	int_regs.ebp = 0;
	realmode_run((long)&int_regs);
	return 1;
}
