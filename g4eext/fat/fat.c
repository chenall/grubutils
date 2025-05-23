/*
 *  fat  --  FAT file system utility for grub4dos.
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

/* ****  FatFs Module Source Files R0.08 *** from http://elm-chan.org/fsw/ff/00index_e.html
   FatFs module is an open source software to implement FAT file system to
   small embedded systems. This is a free software and is opened for education,
   research and commercial developments under license policy of following trems.

   Copyright (C) 2010, ChaN, all right reserved.

 * The FatFs module is a free software and there is NO WARRANTY.
 * No restriction on use. You can use, modify and redistribute it for
   personal, non-profit or commercial product UNDER YOUR RESPONSIBILITY.
 * Redistributions of source code must retain the above copyright notice.
 *
 */

/*
 * compile:

 * gcc -Wl,--build-id=none -m64 -mno-sse -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE -Wl,-Ttext -Wl,0 fat.c -o fat.o 2>&1|tee build.log

 * disassemble:			objdump -d fat.o
 * confirm no relocation:	readelf -r fat.o
 * generate executable:		objcopy -O binary fat.o fat.tmp
 * strip ending 00's:		sed -e '$s/\x00*$//' fat.tmp > fat
 *
 */
/*
 * to get this source code & binary: http://grub4dos-chenall.google.com
 * For more information.Please visit web-site at http://chenall.net
 * 2010-06-24
 * 2010-06-24 添加获取当前时间代码.修正dir显示时间日期错误.copy时按原文件复制禁用GRUB4DOS的自动解压功能
 * 2010-07-27 减小读取文件次数，解决复制PXE服务器上的文件卡死的问题。
  */
#include "grub4dos.h"
#include "fat.h"
#define SKIP_LINE		0x100
#define SKIP_NONE		0
#define SKIP_WITH_TERMINATE	0x200
#define f_buf_sz 0x300000
static char *f_buf = NULL;
static FATFS *FatFs[_DRIVES];
static int fat_func(char *arg,int flags);
static unsigned int cur_drive = 0L;		//grub4dos drive num for FatFs Module
static unsigned int cur_partition = 0L; 	//grub4dos partition num for FatFs Module
static FATFS *fs;
static grub_size_t g4e_data = 0;
static void get_G4E_image(void);

#ifdef DEBUG
static char fat_err[][100]={
	[FR_OK] = 0,
	[FR_DISK_ERR] = " (1) A hard error occured in the low level disk I/O layer ",
	[FR_INT_ERR] = "(2) Assertion failed ",
	[FR_NOT_READY] = " (3) The physical drive cannot work ",
	[FR_NO_FILE] = "(4) Could not find the file ",
	[FR_NO_PATH] = "(5) Could not find the path ",
	[FR_INVALID_NAME] = " (6) The path name format is invalid ",
	[FR_DENIED] = "(7) Acces denied due to prohibited access or directory full ",
	[FR_EXIST] = "(8) Acces denied due to prohibited access ",
	[FR_INVALID_OBJECT] = " (9) The file/directory object is invalid ",
	[FR_WRITE_PROTECTED] = " (10) The physical drive is write protected ",
	[FR_INVALID_DRIVE] = " (11) The logical drive number is invalid ",
	[FR_NOT_ENABLED] = " (12) The volume has no work area ",
	[FR_NO_FILESYSTEM] = " (13) There is no valid FAT volume on the physical drive ",
	[FR_MKFS_ABORTED] = " (14) The f_mkfs() aborted due to any parameter error ",
	[FR_TIMEOUT] = "(15) Could not get a grant to access the volume within defined period ",
	[FR_LOCKED] = "(16) The operation is rejected according to the file shareing policy ",
	[FR_NOT_ENOUGH_CORE] = " (17) LFN working buffer could not be allocated ",
	[FR_TOO_MANY_OPEN_FILES] = " (18) Number of open files > _FS_SHARE ",
};
#endif

/* this is needed, see the comment in grubprog.h */
#include "grubprog.h"
/* Do not insert any other asm lines here. */

static int main (char *arg,int flags);
static int
main (char *arg,int flags)
{
  get_G4E_image();
  if (! g4e_data)
    return 0;
  
//	if (*(int *)0x8278 < 20101228)
  if (*(int *)IMG(0x8278) < 20101228)
	{
		return !printf_errinfo("Err grub4dos version\n");
	}

	int ret=fat_func (arg , flags);
	free(f_buf);
  f_buf = 0;
  free(fs);
  fs = 0;
  free(FatFs[0]);
  FatFs[0] = 0;
	return ret;
}

/*
  Inidialize a Drive for FatFs Module.
  因为GRUB4DOS同一时刻只能访问一个磁盘，操作了其它磁盘以后必须重新初始化FAT磁盘才能使FAT模块正常使用
  程序的FatFs Module都是直接针对某个GRUB4DOS磁盘分区的访问，而非整个磁盘。
  注：2010-06-03已经整合到diskio函数中了。
*/
/*
static int fat_init (void)
{
	current_drive = cur_drive;
	current_partition = cur_partition;
	if (real_open_partition(0))
	{
		#ifdef DEBUG
		if (debug) printf("Current drive:%x,%x\n",current_drive,current_partition);
		#endif
		return 0;
	}
	return 1;
}
*/
/*
	FatFs Module模块不能直接使用带有盘符信息的GRUB4DOS路径，所以必须分离命令行路径参数的磁盘号和目录提供给FAT模块使用。
*/
static char* fat_set_path(char *arg);
static char* fat_set_path(char *arg)
{
//	if (! arg) return 0;
	if (*arg == '/')
	{
		return arg;
	}
	if (! set_device (arg)) return 0;
//	if (! *(++P)) return 0;
	/*设置要操作的FAT分区磁盘号和分区号供FatFs模块使用*/
	cur_drive = current_drive;
	cur_partition = current_partition;
	return strstr(arg,"/");
}

#if F_DIR
static FRESULT fat_dir (char *arg);
static FRESULT fat_dir (char *arg)
{

	FRESULT res;
	FILINFO fno;
	DIR dir;
	BYTE Attrib = 0;
	BYTE no_attr = 0;

	if (memcmp(arg , "/a" , 2) == 0)
	{
		arg += 2;
		BYTE *AM;
		char *P = arg;
		while(*P)
		{
			if (*P == '-') 	
				AM = &no_attr,P++;
			else
				AM = &Attrib;
			
			switch(*P++)
			{
				case 'h':
				case 'H':
					*AM |= AM_HID;
					break;
				case 'a':
				case 'A':
					*AM |= AM_ARC;
					break;
				case 's':
				case 'S':
					*AM |= AM_SYS;
					break;
				case 'r':
				case 'R':
					*AM |= AM_RDO;
					break;
				case 'd':
				case 'D':
					*AM |= AM_DIR;
					break;
				default:
					AM = 0;
					break;
			}
			if (! AM) break;
		}
		arg = skip_to (0,arg);
	}
	
	if (arg = fat_set_path(arg))
	{	/*去掉目录后面的"/"，如果有加"/"，f_opendir会找不到文件*/
		char *P;
		for (P = arg;P[1];P++);
		if (*P == '/') *P = 0;
		if (! *arg) arg = NULL;
	}

	res = f_opendir(&dir,arg?arg:"");
	if (res) return res;
	
	printf_debug("FAT dir:[%s]\n",arg?arg:"/");
	
	for (;;)
	{
		res = f_readdir (&dir, &fno);
		if (res || fno.fname[0] == 0) break;
		if (fno.fname[0] == '.' || (fno.fattrib & no_attr) || ((fno.fattrib & Attrib) != Attrib)) continue;
		printf_debug("%04d-%02d-%02d  %02d:%02d:%02d ",(fno.fdate >> 9) + 1980,fno.fdate >> 5 & 15,fno.fdate & 31,fno.ftime >> 11 & 31,fno.ftime >> 5 & 63,fno.ftime & 31);
		if (fno.fattrib & AM_DIR)
			printf_debug(" <%s>\n",fno.fname);
		else
		{
			printf_debug(" %12s  %u %s\n",fno.fname,fno.fsize>>20?fno.fsize>>10:fno.fsize,fno.fsize>>20?"KB":"");
		}
	}
	return 0;
}
#endif

#if F_GETFREE
static FRESULT fat_free_space(char *arg,unsigned long long *free_space);
static FRESULT fat_free_space(char *arg,unsigned long long *free_space)
{

    FATFS *FS=fs;
    DWORD fre_clust, fre_sect;
    FRESULT res;
    arg = fat_set_path(arg);
    res = f_getfree(arg,&fre_clust,&FS);
    if (res) return res;
    *free_space = (unsigned long long)fre_clust * fs->csize << 9;
    return res;
}
#endif

#if FAT_MKFILE
static FRESULT fat_mkfile(char *arg);
static FRESULT fat_mkfile(char *arg)
{
	unsigned long long fil_size,free_space;
	UINT bw;
	FRESULT res;
	FIL file;
	fil_size = 0;
	
	if (memcmp(arg,"size=",5) == 0)
	{
		arg += 5;
	
		if (*arg=='*')
			fil_size = filesize;
		else if (! safe_parse_maxint (&arg, &fil_size))
			return 0;
		arg = skip_to(0,arg);
	}

	arg = fat_set_path(arg);

	#if F_GETFREE
	if (res = fat_free_space(arg,&free_space))
		return res;
	
	if (free_space < fil_size) 
	{
		#ifdef DEBUG
			printf_debug("Need more space: %lu KB,Current available drive space: %lu KB\n",fil_size - free_space >> 10, free_space >> 10);
		#endif
		return FR_DENIED;
	}
	#endif
	if ((res = f_open(&file, arg, FA_CREATE_NEW | FA_WRITE)) || (res = f_write(&file, 0, fil_size, &bw)))
	{
		f_close(&file);
		return res;
	}
	f_close(&file);
	return res;
}
#endif

#if FAT_INFO
static FRESULT fat_info (char *arg);
static FRESULT fat_info (char *arg)
{
	FRESULT res;
	unsigned long long free_space;
    res = fat_free_space(arg,&free_space);
	if (res) return res;
    printf_debug("FAT sub-type:\t%s\n",fs->fs_type == FS_FAT12?"FAT12":fs->fs_type == FS_FAT16?"FAT16":"FAT32");
    printf_debug("Sectors per cluster: %d\n"
			"Sectors per FAT: %d\n"
			"Number of free clusters: %d\n"
			"Total clusters: %d\n"
			"FAT start sector: %d\n"
			"%u KB total drive space.\n"
			"%u KB available.\n",
			fs->csize,fs->fsize,fs->free_clust,fs->n_fatent - 2,fs->fatbase,(fs->n_fatent - 2) * fs->csize>>1,free_space >> 10);
	return 0;
}
#endif

static FRESULT fat_copy (char *arg);
static FRESULT fat_copy (char *arg)
{
	BYTE Mode = FA_WRITE;
	TCHAR *from, *to;
	char new_to[256];
	unsigned long long f_pos = 0;
	unsigned long long fil_size;
	UINT br, bw;/*br 读取字节数；bw 写入字节数。*/
	FRESULT res;
	FIL file;

	if (memcmp(arg, "/o", 2) == 0)
	{
		Mode |= FA_CREATE_ALWAYS;/*有加参数“/o”使用覆盖的模式创建文件*/
		arg = skip_to(0,arg);
	}
	else Mode |= FA_CREATE_NEW;

	from = arg;
	to = skip_to (SKIP_WITH_TERMINATE, arg);
	to = fat_set_path(to);

	if (!to) 
	{
		/*如果只提供了源文件名，则目标文件名使用来源文件名复制到当前根目录下*/
		char *P = to = from;
		while(P = strstr(P,"/"))
		   to = P++;
	}
		else if (to[strlen(to)-1] == '/') /*如果未提供目标名称，则使用原名复制*/
	{
		strcpy(new_to,to);
		char *P = to = from;
		while (P = strstr(P,"/")) to = ++P;
		strncat(new_to,to,255);
		to=new_to;
	}

	printf_debug("Copy file: %s ==> %s\n",from,to);

	#if F_GETFREE
	res = fat_free_space("",&f_pos);
	#endif

	if (! open (from))
		return FR_NO_FILE;
	fil_size = filemax;

	#if F_GETFREE
	if (f_pos < fil_size) 
	{
		#ifdef DEBUG
			printf_debug("Need more space: %lu KB,Current available drive space: %lu KB\n",fil_size - f_pos >> 10, f_pos >> 10);
		#endif
		return FR_DENIED;
	}
	#endif
	if ((f_buf = malloc(filemax>f_buf_sz?f_buf_sz:filemax)) == NULL)
	{
		close();
		return 0;
	}
	br = f_pos = read((unsigned long long)(grub_size_t)f_buf,f_buf_sz, GRUB_READ);
	close();

	if (! f_pos)
	{
		return 0;
	}

	if (res = f_open(&file, to, Mode))
	{
		f_close(&file);
		return res;
	}


	while (br)
	{
		#ifdef DEBUG1
		printf_debug("Read Bytes:%d\n",br);
		#endif
		res = f_write(&file, f_buf, br, &bw);
		#ifdef DEBUG1
		printf_debug ("Write Bytes:%d\n", bw);
		#endif
		if (res || bw < br || f_pos >= fil_size) break;

		if (!open(from))
		{
			f_close(&file);
			return 0;
		}

		filepos = f_pos;
		br = read((unsigned long long)(grub_size_t)f_buf,f_buf_sz, GRUB_READ);
		close();
		f_pos += br;
	}
	f_close(&file);
	return res;
}

static int fat_help (void);
static int fat_help (void)
{
	printf("\n    FAT for grub4dos_for_UEFI"
			#ifdef FAT_MINI
			"(mini)"
			#endif
			",Compiled time: %s %s\n",__DATE__,__TIME__);
	printf(	
			#if FAT_MKFILE
			"\n    FAT mkfile size=SIZE|* FILE\n"
			"\tCreate file FILE of a certain size SIZE\n"
			"\tif size=* is specified,get SIZE from memory address 0x8290.\n"
			#endif
			"    FAT copy [/o] FILE1 [FILE2]\n"
			"\tCopy file FILE1 to file FILE2\n"
			#if F_REN
			"    FAT ren OldName NewName\n"
			"\tRename/Move a File or Directory\n"
			#endif
			#if F_DIR
			"    FAT dir [PATH]\n"
			"\tList all files and directories in the directory PATH\n"
			#endif
			#if F_DEL
			"    FAT del FILE|DIRECTORY\n"
			"\tRemove a File or Directory (Directory must be empty)\n"
			#endif
			#if F_MKDIR
			"    FAT mkdir DIRECTORY\n"
			"\tCreate a new directory\n"
			#endif
			#if _USE_MKFS && !_FS_READONLY
			"    FAT mkfs [/A:UNIT-SIZE] DRIVE\n"
			"\tCreates an FAT volume on the DRIVE\n"
			"\t/A:UNIT-SIZE: Force the allocation unit (cluter) size in unit of byte\n"
			#endif
			"\n\tFor more information.Please visit web-site at http://chenall.net/\n"
		);
}

static int fat_func (char *arg,int flags);
static int fat_func (char *arg,int flags)
{
	DIR dir;
	FRESULT res = FR_INVALID_NAME;
	cur_drive = saved_drive;
	cur_partition = saved_partition;
//	memset(fs,0,sizeof(FATFS));
  fs = zalloc (sizeof(FATFS));
	f_mount(0,fs);

	if (memcmp (arg, "copy ", 5) == 0)
	{
		unsigned int no_dec = no_decompression;
		arg = skip_to (0, arg);
		no_decompression = 1;
		res = fat_copy (arg);
		no_decompression = no_dec;
	}
	#if F_DIR
		else if (memcmp (arg, "dir", 3) == 0)
		{
			arg = skip_to(0, arg);
			res = fat_dir(arg);
		}
	#endif
	
	#if F_CHMOD
		else if (memcmp (arg, "chmod", 5) == 0)
		{
			return 0;
		}
	#endif
	
	#ifdef DEBUG
		else if (memcmp (arg, "info", 4) == 0)
		{
			arg = skip_to(0, arg);
			res = fat_info(arg);
		}
	#endif
	
	#if FAT_MKFILE
	else if (memcmp (arg, "mkfile", 6) == 0)
	{
		arg = skip_to (0, arg);
		if (arg)
		{
			res = fat_mkfile (arg);
		}
	}
	#endif
	
	#if F_MKDIR
	else if (memcmp (arg, "mkdir ", 6) == 0)
	{
		arg = skip_to (0, arg);
		arg = fat_set_path(arg);
		if (arg)
		{
			res = f_mkdir (arg);
		}
	}
	#endif
	
	#if F_DEL
	else if (memcmp (arg, "del ", 4) == 0)
	{
		arg = skip_to (0, arg);
		arg = fat_set_path(arg);
		if (arg)
		{
			res = f_unlink (arg);
		}
	}
	#endif
	
	#if _USE_MKFS && !_FS_READONLY
	else if (memcmp (arg,"mkfs ",5) == 0)
	{
		unsigned long long unit = 0;
		int mbr = 0;
		arg = skip_to (0, arg);
		if (memcmp (arg,"/A:", 3) == 0)
		{
			arg += 3;
			if (! safe_parse_maxint (&arg, &unit))
			{
				return 0;
			}
			arg = skip_to (0,arg);
		}
    if (memcmp (arg,"mbr", 3) == 0) //创建mbr，硬盘格式    2024-11-25
    {
      arg += 3;
      mbr = 1;
      arg = skip_to (0,arg);
    }
		arg = fat_set_path(arg);
		res = f_mkfs(0,(mbr?0:1),(UINT)unit);
		if (res == 0 ) res = fat_info(arg);
	}
	#endif
	#if F_REN
	else if (memcmp (arg, "ren ", 4) == 0)
	{
		arg = skip_to (0, arg);
		TCHAR *OldName,*NewName;
		OldName = arg;
		NewName = skip_to (0, arg);
		nul_terminate(arg);
		OldName = fat_set_path(OldName);
		if (OldName && NewName)
		{
			res = f_rename (OldName, NewName);
		}
	}
	#endif
	else 
	{
		fat_help();
		errnum = 0;
		res = 0;
	}
	
	f_mount(0, NULL);
	
	if (res) {
	#ifdef DEBUG
		printf_debug("FAT Error: %s\n",fat_err[res]);
	#else
		printf_debug("FAT error: %d\n",res);
	#endif
		errnum = 0xff;
	}
	return !res;
};

/*以下内容本来是要放在DISKIO.C的，因为只有几个函数，为了方便就直接整合到这里来了*/

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
/*BYTE drv Physical drive nmuber (0..) */
static DSTATUS disk_initialize (BYTE drv);
static DSTATUS disk_initialize (BYTE drv)
{
	current_drive = cur_drive;
	current_partition = cur_partition;
	#ifdef DEBUG1
		printf_debug("Current drive:%x,%x,%d,%d\n",current_drive,current_partition,fs->id,fs->fs_type);
	#endif
	if (real_open_partition(0))
	{
		return 0;
	}
	return STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */
/*-----------------------------------------------------------------------*/
/*
　注：为了防止在进行其它操作时切换了GRUB4DOS的当前磁盘和分区号导致FAT模块的操作失败。
　在这个函数里面重新指定FAT分区（也就是上面的初始化操作），以保证FAT模块正常使用。
*/
static DSTATUS disk_status (BYTE drv);
static DSTATUS disk_status (BYTE drv)
{
	if (disk_initialize(0))/*重新指定要操作的FAT分区，写在这里使得每次调用FAT模块时都执行一次*/
		return STA_NOINIT;

	if (fs->id && fs->fs_type) return 0;

	return STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/
static DRESULT disk_read (BYTE drv,	BYTE *buff, DWORD sector,	BYTE count);
static DRESULT disk_read (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..255) */
)
{
	if (! devread ((unsigned long long)sector, 0LL, (unsigned long long)count << _SS_BIT ,(unsigned long long)(grub_size_t)buff, GRUB_READ))
	{
		return RES_PARERR;
	}
	return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/
/* The FatFs module will issue multiple sector transfer request
/  (count > 1) to the disk I/O layer. The disk function should process
/  the multiple sector transfer properly Do. not translate it into
/  multiple single sector transfers to the media, or the data read/write
/  performance may be drasticaly decreased. 
*/
#if _READONLY == 0
static DRESULT disk_write (BYTE drv, const BYTE *buff, DWORD sector, BYTE count);
static DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	BYTE count			/* Number of sectors to write (1..255) */
)
{
	if (! devread ((unsigned long long)sector, 0LL,(unsigned long long)count << _SS_BIT ,(unsigned long long)(grub_size_t)buff, GRUB_WRITE))
	{
		return RES_PARERR;
	}
	return RES_OK;
}
#endif /* _READONLY */

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
/*注：因为程序目前不需要使用这些功能，所以这个函数是一个空函数，直接返回。*/
static DRESULT disk_ioctl (BYTE drv, BYTE ctrl,	void *buff);
static DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
#if _USE_MKFS
	if (disk_initialize(0))
		return RES_NOTRDY;
	switch(ctrl)
	{
		case GET_SECTOR_COUNT:
			//*(DWORD *)buff =(DWORD)buf_geom->total_sectors;
			*(DWORD *)buff = (DWORD)part_length;
			break;
		#if _MAX_SS != 512
		case GET_SECTOR_SIZE:
			*(DWORD *)buff = buf_geom->sector_size;
			break;
		#endif
		case GET_BLOCK_SIZE:
			*(DWORD *)buff= 1L;
			break;
	}
#endif
	return 0;
	//return RES_PARERR;
}

/*
	函数用于设置新建文件、文件夹日期时间，因为不知如何获取时间，所以使用了一个固定的时间，目前是使用程序编译的时间。
	如果你知道并且想使用系统时间。可以自己添加代码进去。
	2010-06-24已添加获取当前时间代码.
*/
static DWORD get_fattime (void);
static DWORD get_fattime (void)
{
  struct grub_datetime datetime;
	get_datetime(&datetime);
  return ((datetime.year - 60) << 25) + (datetime.month << 21) + (datetime.day << 16) + (datetime.hour << 11) + (datetime.minute << 5) + datetime.second;
};

static void get_G4E_image(void)
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

#if _USE_LFN
WCHAR ff_convert (WCHAR wch, UINT dir)
{ 
  if (wch < 0x80) {
    /* ASCII Char */
    return wch;
  }

  /* I don't support unicode it is too big! */
  return 0;
}

WCHAR ff_wtoupper (WCHAR wch)
{ 
  if (wch < 0x80) {
    /* ASCII Char */
    if (wch >= 'a' && wch <= 'z') {
      wch &= ~0x20;
    }
    return wch;
  }

  /* I don't support unicode it is too big! */
  return 0;
}
#endif

#include "ff.c"

