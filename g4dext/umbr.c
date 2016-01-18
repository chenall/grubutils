/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004  Free Software Foundation, Inc.
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
 * to get this source code & binary: http://grub4dos-chenall.google.com
 * For more information.Please visit web-site at http://chenall.net
 * 2016-01-18
 */
#include "../include/grub4dos.h"
#define PACKED			__attribute__ ((packed))
#include "../umbr/umbr.h"
#undef disk_read_hook
#define disk_read_hook ((*(int **)0x8300)[31])
typedef struct {
	short CRC;
	short BlockCount;	// 启动代码块数(以扇区为单位)
	long BufferAddr;	// 传输缓冲地址(segment:offset),也是该代码的启动地址
	long long BlockNum;	
} PACKED DAP;
static unsigned long long readsec = 0;
static int umbr_read_hook(unsigned long long sector,unsigned long offset,unsigned long len);
static short check_sum(char *buf,unsigned length);
/* this is needed, see the comment in grubprog.h */
#include "grubprog.h"
/* Do not insert any other asm lines here. */


int main(char *arg,int flags)
{
	DAP *dap;
	int	testmode=0;
	unsigned long disk = -1;
	unsigned long part = -1;
	char *file[3]={0,0,0};
	unsigned int count=0;
	int ret;
	char *buf=malloc(0x80100);
	int no_decompression_bak = no_decompression;
	
	dap=(DAP*)&umbr[0x10];
	no_decompression=1;

	if (!*arg){
			printf("\n    UMBR for grub4dos version 1.0,Compiled time: %s %s\n",__DATE__,__TIME__);
		return printf("\n\tUMBR is a simple mbr bootloader.\n"
			"\n\tumbr [-d=D] [-p=P] [--test] [file1] [file2] [file3]\n"
			"\t-d=D\t\t	umbr Install Drive(default current drive).\n"
			"\t-p=D\t\t	fail boot partition.\n"
			"\t--test\t\t	test only.\n"
			"\n\tFor more information.Please visit web-site at http://chenall.net/\n"
		);
	}
	while(*arg){
		if (memcmp(arg, "--test",6) == 0 )
			testmode = 1;
		else if (memcmp(arg, "-d=",3) == 0){
			arg+=3;
			disk = *arg-'0';
		} else if (memcmp(arg,"-p=",3) == 0){
			arg+=3;
			part = *arg -'0';
		} else  if (count < 3){
			file[count++]=arg;
		} else
			break;
		arg = skip_to(0,arg);
	}
	if (disk == -1){//默认当前磁盘
		disk=saved_drive;
	} else disk |= 0x80;

	builtin_cmd("setlocal",NULL,flags);//保存当前环境
	for(count=0;count<3;++count)
	{
		if (!file[count])
			break;
		nul_terminate(file[count]);
		printf("[%d]:%s\n",count,file[count]);
		if (!open(file[count])){
			printf("\tError: File not found!\n");
			continue;
		}
		if (current_drive !=disk || filemax > 0x80000){
			printf("\tError: %d=>%ld\n",current_drive,filemax );
			goto err;
		}
		memset(buf,0,0x80100);
		disk_read_hook = (int)&umbr_read_hook;
		readsec = 0;
		ret = read((unsigned long long)(int)buf,0x200,GRUB_READ);
		if (!ret)
			goto err;
		if (!readsec || readsec > 0xFFFFFBFF)//~0x400
		{
			printf("\tError:  starting sector number: 0x%lx",readsec);
			goto err;
		}
		dap->BlockNum=readsec;
		disk_read_hook=0;
		if (filepos < filemax && !read((unsigned long long)(int)buf+filepos,filemax-filepos,GRUB_READ))
			goto err;
		disk_read_hook = (int)&umbr_read_hook;
		filepos=filemax?filemax-1:0;
		ret = read((unsigned long long)(int)buf+filemax,1,GRUB_READ);

		if (readsec && readsec <= 0x400){
			dap->CRC=check_sum(buf,readsec);
			dap->BlockCount=readsec;
			dap->BufferAddr=(readsec > 1?0x7e00000:0x7c00000);
			printf("\tsuccess:[%x]\n",dap->CRC);
			++dap;
		} else {
			printf("\tError: Filename must be either an absolute pathname or blocklist!\n");
		}
		disk_read_hook = 0;
		err:
			close();
	}

	if (part == -1){
		for(count=0;count<3;++count){
			sprintf(buf,"(%d,%d)+1",disk,count);
			if (open(buf)){
				part=count;
				close();
				break;
			}
		}
	}
	if (part >= 0){
		sprintf(buf,"(%d,%d)+1",disk,part);
		if (open(buf)){
			readsec = 0;
			disk_read_hook = (int)&umbr_read_hook;
			if (read((unsigned long long)(int)buf,0x200,GRUB_READ) == 0x200){
				dap=(DAP*)umbr+3;
				dap->CRC=check_sum(buf,1);
				dap->BlockNum=readsec;
				dap->BlockCount=1;
				dap->BufferAddr=0x7C00000;
			}
			disk_read_hook = 0;
			close();
			printf("[3]:(%d,%d)+1\n",disk,part);
		}
	}
	builtin_cmd("endlocal",NULL,flags);//恢复环境
	no_decompression=no_decompression_bak;
	free(buf);
	if (sizeof(umbr) > 0x200) testmode = 1;
	if (testmode){
		rd_base = (unsigned long long)(int)umbr;
		rd_size = sizeof(umbr);
		builtin_cmd(NULL,"cat --hex (rd)+1,0x50 && pause Press any key to Boot .... && chainloader (rd)+1 && boot",flags);
	}
	else
		rawread(disk,0,0,0x1B7,(unsigned long long)(int)umbr,GRUB_WRITE);
}

static short check_sum(char *buf,unsigned length){
	long chk=0;
	long *p=(long*)buf;
	if (length>0x7F) length=0x7F;
	length<<=7;
	while(length--){
		chk ^= *p++;
	}
	return ((chk & 0xFFFF) ^ (chk>>16));
}

static int umbr_read_hook(unsigned long long sector,unsigned long offset,unsigned long len)
{
	if (readsec){
		readsec=sector - readsec;
		if ((readsec<<9) > filemax){
			readsec=0;
		} else ++readsec;
	} else readsec=sector;
}
