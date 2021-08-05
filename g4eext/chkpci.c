/*
 *  chkpci  --  report information about PCI devices.
 *  Copyright (C) 2015  tinybit(tinybit@tom.com)
 *  Copyright (C) 2010  chenall(chenall.cn@gmail.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * compile:

 * gcc -Wl,--build-id=none -m32 -mno-sse -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE -Wl,-Ttext -Wl,0 chkpci.c -o chkpci.o

 * disassemble:			objdump -d chkpci.o
 * confirm no relocation:	readelf -r chkpci.o
 * generate executable:		objcopy -O binary chkpci.o chkpci.tmp
 * strip ending 00's:		sed -e '$s/\x00*$//' chkpci.tmp > chkpci
 *
 */

/*
 * to get this source code & binary: http://grub4dos-chenall.google.com
 * For more information.Please visit web-site at http://chenall.net
 更新信息(changelog):
        2014-04-01
                1.使用drivepack.ini格式时支持指定-w参数,比如-w:xp,会匹配适用于winxp的驱动
	2010-11-03
		1.-cc,-sc参数支持列表
		例子：　chkpci -cc:1 -sc:0,4
		2.添加-srs -net -vga参数（不能同时使用，也不可以和-cc -sc参数使用，若是同时使用以最后一个为准）
	2010-11-02
		１.添加参数-sc:
	2010-11-01
		1.添加一个参数-o.
	2010-09-04
		1.尝试添加新的PCI信息显示格式（类似CHKPCI).
		文件内容格式:
			第一行固定PCI$
			可选固定输出内容。
			...
			$PCI设备信息
			匹配后要显示的内容
			...
			$pci设备信息2
			匹配后要显示的内容
			...
		一个例子:
			===========CHKPCI.PCI=============
			PCI$
			$PCI\VEN_8086&DEV_7113
			Intel
			test
			$PCI\VEN_8086&DEV_7000&CC_020000&REV_00
			fat copy /IASTOR.SYS (fd0)/
			fat copy /iastor.inf (fd0)/
			fat copy /txtsetup.oem (fd0)/
			===========CHKPCI.PCI=============
	2010-08-28
		1.添加帮助信息 -h 参数.
		2.添加参数 -cc:CC,用于显示指定设备.
	2010-08-27
		修正,在实机使用时会造成卡机的问题.
	2010-08-26
		添加读取PCIDEVS.TXT按格式显示设备信息的功能.
	2010-08-25
		第一版,只能显示PCI信息.
 */

#include <grub4dos.h>
#include <pc_slice.h>
#include <uefi.h>

union bios32 {
	struct {
		unsigned int signature;	/* _32_ */
		unsigned int entry;		/* 32 bit physical address */
		unsigned char revision;		/* Revision level, 0 */
		unsigned char length;		/* Length in paragraphs should be 01 */
		unsigned char checksum;		/* All bytes must add up to zero */
		unsigned char reserved[5];	/* Must be zero */
	} fields;
	char chars[16];
};

struct pci_dev {
	unsigned short	venID;
	unsigned short	devID;
	unsigned char revID;
	unsigned char prog;
	unsigned short class;

	unsigned int subsys;
} __attribute__ ((packed));

struct vdata
{
	unsigned int addr;
	unsigned int dev;
} __attribute__ ((packed));
#define DEBUG 0
#if 0
#define VDATA	 ((struct vdata *)(0x600000+0x4000))
#define FILE_BUF ((char *)0x684000)
#define PCI ((struct pci_dev *)0x600000)
#else
static struct vdata *VDATA = NULL;
static struct pci_dev *PCI = NULL;
static char *FILE_BUF = NULL;
#endif
static int out_fmt = 0;
static grub_size_t g4e_data = 0;
static void get_G4E_image (void);

/* this is needed, see the comment in grubprog.h */
#include "grubprog.h"
/* Do not insert any other asm lines here. */

int chkpci_func (char *,int);

int main (char *arg,int flags)
{
  get_G4E_image ();
  if (! g4e_data)
    return 0;

	char *p = malloc(0x84000);
	if (p == NULL)
		return 0;
	PCI = (struct pci_dev *)p;
	VDATA = (struct vdata *)(p+0x4000);
	int ret = chkpci_func (arg , flags);
	free(p);
	free(FILE_BUF);
	return ret;
}

int inl(int port)
{
		int ret_val;
		__asm__ volatile ("inl %%dx,%%eax" : "=a" (ret_val) : "d"(port));
		return ret_val;
}

void outb(int port, char val) {
		__asm__ volatile ("out %%al,%%dx"::"a" (val), "d"(port));
}

void outl(int port, int val)
{
		__asm__ volatile ("outl %%eax,%%dx" : : "a"(val), "d"(port));
}

/*检测是否存在PCI BIOS*/
unsigned int pcibios_init(int flags)
{
	unsigned int tmp;
	outb(0xCFB,1);
	tmp = inl(0xCF8);
	outl(0xCF8,0x80000000);
	if (inl(0xcf8) == 0x80000000)
	{
		outl(0xCF8,tmp);
		if (debug>1)
		printf("pcibios_init: Using configuration type 1\n");
		return 1;
	}
#if 0
	outl(0xcf8,tmp);
	outl(0xcfb,0);
	outl(0xcf8,0);
	outl(0xcfa,0);
	if (inl(0xcf8) == 0 && inl(0xcfb) == 0)
	{
		if (debug>1)
		printf("pcibios_init: Using configuration type 2\n");
		return 2;
	}
	printf("pcibios_init: Not supported chipset for direct PCI access !\n");
#endif
	return 0;
}

int help(void)
{
return printf("\n CHKPCI For GRUB4DOS,Compiled time: %s %s\n\
\n CHKPCI is a utility that can be used to scan PCI buses and report information about the PCI device.\n\
\n CHKPCI [-h] [-o] [-u] [-cc:CC] [-sc:SC] [-srs] [FILE]\n\
\n Options:\n\
\n -h      show this info.\n\
\n -u      Unique.\n\
\n -cc:CC  scan Class Codes with CC only.\n\
\n -sc:SC  scan SubClass Codes.\n\
\n -srs    for SATA/SCSI/RAID.\n\
\n FILE    PCI device database file.\n\
\n -w:[xp|2k|2k3] only find driver for specified OS,use for driverpack.\n\
\n For more information.Please visit web-site at http://chenall.net/tag/grub4dos\n\
 to get this source and bin: http://grubutils.googlecode.com\n",__DATE__,__TIME__);
}

/*字符串转换成HEX数据如AB12转成0xAB12*/
unsigned int asctohex(char *asc)
{
	unsigned int t = 0L;
	unsigned char a;
	while(a=*asc++)
	{
		if ((a -= '0') > 9) /*字符减去'0',如果是'0'-'9'得到0-9数字*/
		{
			a = (a | 32) - 49;/*其它的字母转换成大写再减去49应该在0-5之间否则都是非法.*/

			if (a > 5)
				break;
			a += 10;
		}
		t <<= 4;
		t |= a;
	}
	return t;
}

/*查找以ch开头的行,碰到以ch1字符开头就结束.
	如果ch是0,则查询任意非以ch1字符开头的行.
	返回值该行在内存中的地址.否则返回0;
*/
unsigned char *find_line(char *P,char ch,char ch1)
{
	while(*P)
	{
		if (*P == 0xd || *P == 0xa)
		{
			while (isspace(*P))
				++P;
			if (*P == ch1)
				return NULL;
			if ( ch == 0 || *P == ch)
				return P;
		}
		++P;
	}
	return NULL;
}

/*读取PCIDEVS.TXT文件到内容中,并作简单索引*/
int read_cfg(int len)
{
	unsigned char *P=FILE_BUF;
	char *P1;
	unsigned int t;
	memset((char *)VDATA,0,0x70000);
	while( P = find_line(P, 'V', 0))
	{
		P += 2;
		t = asctohex(P);
		P = skip_to (0, P);
		VDATA[t].addr = (unsigned int)(grub_size_t)P;
		while (*P != 0x0D && *++P);
		
		if (*P == 0x0D)
		{
			*P++ = 0;
			VDATA[t].dev = (unsigned int)(grub_size_t)find_line(P,'D',0 )-1;
		}
	}
	return 0;
}

/*按PCIDEVS.TXT格式显示对应的硬件信息,具体格式请查看相关文档*/
int show_dev(struct pci_dev *pci)
{
	char *P,*P1;
	unsigned int t;
	P = (char *)(grub_size_t)VDATA[pci->venID].addr;
	printf("VEN_%04X:\t%s\nDEV_%04X:\t",pci->venID,P,pci->devID);
	P = (char *)(grub_size_t)VDATA[pci->venID].dev;
	while (P = find_line (P, 'D' , 'V'))
	{
		P += 2;
		t = asctohex(P);
		if (t == pci->devID)
		{
			P1 = P;
			while (P1 = find_line (P1,'R','D'))
			{
				P1 += 2;
				t = asctohex(P1);
				if (t == pci->revID)
				{
					P = P1;
					printf("[R=%02X]",pci->revID);
					break;
				}
			}
			P = skip_to (0, P);
			while (*P && *P != 0xD)
				putchar(*P++);
			putchar('\n');
			if (pci->subsys)
			{
			}
			return 1;
		}
	}
	printf("Unknown\n");
	return 0;
}
#define VEN 0x5F4E4556
#define DEV 0X5F564544
#define SUB 0x53425553
char *find_pci(char *hwIDs, int *flags);

int chkpci(void)
{
	struct pci_dev *pci;
	char *p1,*p2,*P;
	unsigned int t;
	struct pci_dev dev;
	p1 = P = skip_to(0x100 | ';',FILE_BUF);//跳过第一行。!
	/*输出文件头部份*/
	while (P && *P != '$')
	{
		p1 = skip_to(0x100 | ';',P);
		printf("%s\n",P);
		P = p1;
	}

	while ((P = p1))
	{
		p1 = skip_to (0x100 | ';', P);
		if (*P != '$')
			continue;
#if 0
		if (*(unsigned int *)(P+1) & 0XFFDFDFDF != 0x5C494350) //PCI
			continue;
		if (*(unsigned int *)(P+5) & 0xFFDFDFDF != VEN)//check VEN_
			continue;
		if (*(unsigned int *)(P+14) & 0xFFDFDFDF != DEV)// check DEV_
			continue;
		p2 = P + 1;
		memset (&dev,0,sizeof(struct pci_dev));
		P = P + 9;
		t = asctohex(P);
		dev.venID = t;
		P += 9;
		t = asctohex(P);
		dev.devID = t;
		P += 5;

		if (*(unsigned int *)P == SUB)//check SUBSYS
		{
			P += 7;
			t = asctohex(P);
			P += 9;
			dev.subsys = t;
		}
		
		if (*(unsigned short *)P == 0x4343) //CC_
		{
			P += 3;
			t = asctohex(P);
			t = t < 0xffff?t:t>>8;
			dev.class = (unsigned short)t;
			P += 4;
			while (*P != 0xD && *P++ != '&');
		}
		
		if (*(unsigned short *)P == 0x5F564552) //REV_
		{
			P += 4;
			t = asctohex(P);
			dev.revID = (char)t;
		}
#endif

		if (find_pci(P+1,NULL))
		{
			if (out_fmt)
			{
				printf("%s\n",P+1);
			}
			else
			{
				while (*p1 == '$')
				{
					p1 = skip_to (0x100 | ';',p1);
				}
				while (p1 && *p1 != '$')
				{
					p2 = p1;
					p1 = skip_to (0x100 | ';',p1);
					printf("%s\n",p2);
				}
				//输出内容直到文件尾或下一个以$开始的行。
			}
		}
	}
	return 1;
}

int chkpci_func(char *arg,int flags)
{
	unsigned int regVal,ret;
#ifdef DEBUG
	unsigned int bus,dev,func;
#endif
	char sc[6] = "\xff\xff\xff\xff\xff";
	char cc[6] = "\xff\xff\xff\xff\xff";
	char skip[6] = "\0";
	struct pci_dev *devs = PCI;
	int i,db_type=0;
	int sort = 0;

	if (! pcibios_init(0)) return 0;
	while (*arg)
	{
		if (memcmp(arg, "-o",2) == 0)
		{
			out_fmt = 1;
		}
		else if (memcmp(arg,"-cc:",4) == 0 )
		{
			unsigned long long t;
			arg += 4;
			for (i = 0;i < 4; i++)
			{
				if (! safe_parse_maxint(&arg,&t))
					return 0;
				cc[i] = (char)t;
				if (*arg != ',')
					break;
				arg++;
			}
		}
		else if (memcmp(arg,"-sc:",4) == 0)
		{
			unsigned long long t;
			arg += 4;
			for (i = 0;i < 4; i++)
			{
				if (! safe_parse_maxint(&arg,&t))
					return 0;
				sc[i] = (char)t;
				if (*arg != ',')
					break;
				arg++;
			}
		}
		else if (memcmp(arg, "-h",2) == 0)
		{
			return help();
		}
		else if (memcmp(arg,"-srs",4) == 0)
		{
			*(int *)cc = 0xFFFFFF01;
			*(int *)sc = 0x07060400;
		}
		else if (memcmp(arg,"-net",4) == 0)
		{
			*(int *)cc = 0xFFFFFF02;
			*(int *)sc = 0XFFFFFFFF;
		}
		else if (memcmp(arg,"-vga",4) == 0)
		{
			*(int *)cc = 0xFFFFFF03;
			*(int *)sc = 0XFFFFFFFF;
		}
		else if (memcmp(arg,"-u",2) == 0)
		{
			sort = 1;
		}
		else if (memcmp(arg,"-w:",3) == 0 )
		{
                    arg += 3;
                    for (i=0;i<5;++i)
                    {
                      if (*arg && *arg != ' ')
                        skip[i] = tolower(*arg++);
                    }
                    skip[i] = 0;
		}
		else
			break;
		arg = skip_to(0,arg);
	}

	if (cc[0] == '\xff')//必须指定cc参数，否则sc参数无效．
		sc[0] = '\xff';

	if (*arg)
	{
		if (! open(arg))
			return 0;
		if ((FILE_BUF = malloc(filemax)) == NULL)
			return 0;
		if (! read ((unsigned long long)(grub_size_t)FILE_BUF,-1,GRUB_READ))
			return 0;
		FILE_BUF[filemax] = 0;
		if (*(unsigned int *)FILE_BUF == 0X24494350)  //检测文件头是否PCI$
			db_type = 2;
		else if (substring("[DriverPack]",FILE_BUF,1) == (unsigned int)-1)
			db_type = 3;
		else
		{
			db_type = 1;
			read_cfg(filemax);
		}
	}

	int check = (cc[0] == '\xff')?0x100:1;
	for (regVal = 0x80000000;regVal < 0x8010FF00;regVal += 0x100)
	{
		outl(0xCF8,regVal);
		ret = inl(0xCFC);

#ifdef DEBUG
		if (debug > 1)
		{
			bus = regVal >> 16 & 0xFF;
			dev = regVal >> 11 & 0x1f;
			func = regVal >> 8 & 0x7;
			printf("Check:%08X B:%02x E:%02X F:%02X\n",regVal,bus,dev,func);
		}
#endif
		if (ret == (unsigned int)-1L) //0xFFFFFF无效设备
		{
			if ((regVal & 0x700) == 0) regVal += 0x700;//功能号为0,跳过该设备.
			continue;
		}

		*(unsigned int *)&devs->venID = ret;/*ret返回值低16位是PCI_VENDOR_ID,高16位是PCI_DEVICE_ID*/
		
		/*获取PCI_CLASS_REVISION,高24位是CLASS信息(CC_XXXXXX),低8位是版本信息(REG_XX)*/
		outl(0xCF8,regVal | 8);
		*(unsigned int *)&devs->revID = inl(0xCFC); /* High 24 bits are class, low 8 revision */
//		if ((cc == '\xff' || (char)(devs->class >> 8) == cc) && (sc == '\xff' || (char)(devs->class) == sc)) //如果指定了CD参数,CD值不符合时不显示.
//		/{
		/*获取PCI_HEADER_TYPE类形,目前只用于后面检测是否单功能设备*/
		outl(0xCF8,regVal | 0xC);
		ret = inl(0xCFC);
		ret >>= 16;

		if (check & 1) //检测Class Code 和　Sub class
		{
			check = 1;
			char chk_tmp = (char)(devs->class >> 8);
			for (i=0;i<4 && cc[i] != '\xff';i++)
			{
				if (cc[i] == chk_tmp)
				{
					if (sc[0] != '\xff')
					{
						chk_tmp = (char)devs->class;
						for (i=0;i<4 && sc[i] != '\xff';i++)
						{
							if (sc[i] == chk_tmp)
							{
								check = 0x101;
								break;
							}
						}
					}
					else
					{
						check = 0x101;
					}
					break;
				}
			}
		}

		if (check & 0x100)
		{//读取SUBSYS信息,低16位是PCI_SUBSYSTEM_ID,高16位是PCI_SUBSYSTEM_VENDOR_ID(也叫OEM信息)
			outl(0xcf8,regVal | 0x2c);
			devs->subsys = inl(0xcfc);
			devs++;
		}

		devs->venID = 0;
		/*如果是单功能设备,检测下一设备*/
		if ( (regVal & 0x700) == 0 && (ret & 0x80) == 0) regVal += 0x700;
	}
	if (sort)
	{
		struct pci_dev *devs1,*devs2;
		for (devs1 = PCI; devs1 < devs; ++devs1)
		{
			for (devs2=devs1+1;devs2<devs;++devs2)
			{
				if (memcmp((void*)devs1,(void*)devs2,sizeof(struct pci_dev)) == 0)
				{
					memmove((void*)devs2,(void*)(devs-1),sizeof(struct pci_dev));
					--devs;
				}
			}
		}
		devs->venID = 0;
	}

	int DriverPack(char *skip);
	switch (db_type)
	{
		case 2:
			return chkpci();
		case 3:
			return DriverPack(skip);
		default:
			for(devs = PCI;devs->venID;++devs)
			{
				printf("PCI\\VEN_%04X&DEV_%04X&SUBSYS_%08X&CC_%04X%02X&REV_%02X\n",
						devs->venID,devs->devID,devs->subsys,devs->class,devs->prog,devs->revID);
				if (db_type == 1) show_dev(devs);
			}
			break;
	}
	return 1;
}
/*
	查找匹配的PCI记录,查找失败返回NULL.
*/
char *find_pci(char *hwIDs,int *flags)
{
	struct pci_dev dev,*pci;
	char *p;
	loop:
	while (*hwIDs == ',' || *hwIDs == ' ' || *hwIDs == '\t')
		++hwIDs;
	p = hwIDs;
	if ((*(unsigned int *)(p) & 0XFFDFDFDF != 0x5C494350) //PCI
		  || (*(unsigned int *)(p+4) & 0xFFDFDFDF != VEN)//check VEN_
		  || (*(unsigned int *)(p+13) & 0xFFDFDFDF != DEV)// check DEV_
		)
			return 0;
	memset (&dev,0,sizeof(struct pci_dev));
	dev.venID = asctohex(p+8);
	dev.devID = asctohex(p+17);
	p += 21;
	while (*p == '&')
	{
		++p;
		if (*(unsigned int *)p == SUB)//check SUBSYS
		{
			dev.subsys = asctohex(p+7);
			p += 15;
		}
		else if (*(unsigned short *)p == 0x4343) //CC_
		{
			int t = asctohex(p+3);
			if (t > 0xFFFF)
			{
				p += 9;
				dev.class = t>>8;
			}
			else 
			{
				p += 7;
				dev.class = t;
			}
		}
		else if (*(unsigned int *)p == 0x5F564552) //REV_
		{
			dev.revID = (char)asctohex(p+4);
			p += 6;
		}
	}

	for (pci=PCI;pci->venID; pci++)
	{
		if (pci->venID == dev.venID && pci->devID == dev.devID)
		{
			if (dev.subsys && dev.subsys != pci->subsys) continue;
			if (dev.class && dev.class != pci->class) continue;
			if (dev.revID && dev.revID != pci->revID) continue;
			if (flags)
			{
				*flags = p - hwIDs;
				return hwIDs;
			}
			pci->devID = 0;
			return p;
		}
	}
	if (*p == ',')
	{
		hwIDs = p+1;
		goto loop;
	}
	return NULL;
}

static char *f_pos;
static char *ms_name;
char *find_section(char *section)
{
	char *f;
	while (f=f_pos)
	{
		f_pos = skip_to(0x100,f_pos);
		if (*f == '[' && (section == NULL || substring(section,f,1) == 0))
		{
			section = ++f;
			while (*++section != ']')
				;
			*section = 0;
			return f;
		}
	}
	return NULL;
}

char *get_cfg(char *string)
{
	char *f;
	while (f = f_pos)
	{
		f_pos = skip_to(0x100,f_pos);
		if (*(short*)string == 0x736d && strncmpi(string,f,4) < 0)
		{
                  return NULL;
                }
		if (substring(string,f,1) == (unsigned int)-1)
		{
			f += strlen(string);
			while (*f && *f != '=')
				++f;
			while (isspace(*++f));
			skip_to(0x200,f);
			if (*f == '\"')
			{
				++f;
				f[strlen(f)-1] = 0;
			}
			break;
		}
	}
	return f;
}

char *get_ms_cfg(char *string,unsigned int count)
{
	sprintf(ms_name,"ms_%d_%s",count,string);
	return get_cfg(ms_name);
}

/*
	分析DRIVEPACK文件,根据匹配的PCI记录设置参数输出.
*/
int DriverPack(char *skip)
{
	char *rootdir,*devdir,*deviceName,*tag,*hwids,*sysFile,*p,*skip_s;
	int skip_len = strlen(skip);
	ms_name = f_pos = FILE_BUF;
	if (!find_section("[DriverPack]"))
		return 0;
	if ((rootdir = get_cfg("rootdir")) == NULL)
		return 0;
	for (p=rootdir;*p;++p)
	{
		if (*p =='\\')
			*p = '/';
	}
	while (	devdir = find_section(NULL))
	{
		char name[24];
		unsigned int count = 1,ms_count;
		deviceName = get_cfg("ms_count");
		ms_count = *deviceName++ - '0';
		if (*deviceName)
			ms_count=ms_count*10+(*deviceName - '0');
		while (count <= ms_count)
		{
			deviceName = get_ms_cfg("deviceName",count);
			tag = get_ms_cfg("tag",count);
			sysFile = get_ms_cfg("sysFile",count);
			if (*devdir=='N' && count == '2' && hwids)
			{
				hwids = get_ms_cfg("hwids",count);
				printf("\"%s\"=\"%s\" ; %s/%s \"%s\" \"%s\"\n",hwids,tag,rootdir,devdir,sysFile,deviceName);
			}
			else
			{
				hwids = get_ms_cfg("hwids",count);
				int len = 1;
				if (hwids = find_pci(hwids, &len))
				{
          if (skip[0])
          {
            skip_s = get_ms_cfg("exc_skipIfOS",count);
            if (skip_s)
            {
              int j;
              for(j=0;skip_s[j];++j) skip_s[j] = tolower(skip_s[j]);
              char *p = strstr(skip_s,skip);
              if (p && p[skip_len] <= ',')
              {
                ++count;
                continue;
              }
            }
          }
					putchar('\"');
					while(hwids)
					{
						printf("%.*s",len,hwids);
						if (hwids = find_pci(hwids+len , &len))
							putchar(',');
					}
					printf("\"=\"%s\" ; %s/%s \"%s\" \"%s\"\n",tag,rootdir,devdir,sysFile,deviceName);
				}
			}
			++count;
		}
	}
	return 1;
}

static void get_G4E_image (void)
{
  grub_size_t i;

  //在内存0-0x9ffff, 搜索特定字符串"GRUB4EFI"，获得GRUB_IMGE
  for (i = 0x40100; i <= 0x9f100 ; i += 0x1000)
  {
    if (*(unsigned long long *)i == 0x4946453442555247)	//比较数据
    {
      g4e_data = *(grub_size_t *)(i+16); //GRUB4DOS_for_UEFI入口
      return;
    }
  }
  return;
}