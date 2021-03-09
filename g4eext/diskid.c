#include <grub4dos.h>
#include <pc_slice.h>
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
 *  G4E external commands
 *  Copyright (C) 2021  a1ive
 *
 *  G4EEXT is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  G4EEXT is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with G4EEXT.  If not, see <http://www.gnu.org/licenses/>.
 */

#define RET_VAR 0x4FF00
#define MAX_PART 26

static int diskid_func (char *arg,int flags);
typedef struct part_info {unsigned long id, part, start;} partinfo;

static grub_size_t g4e_data = 0;
static void get_G4E_image (void);

/* this is needed, see the comment in grubprog.h */
#include <grubprog.h>
/* Do not insert any other asm lines here. */

static int main (char *arg, int flags);
static int main (char *arg, int flags)
{
  get_G4E_image ();
  if (! g4e_data)
    return 0;
  return diskid_func (arg , flags);
}

static int get_partinfo(partinfo *PART_INFO)
{
	char mbr[512];
	unsigned long part = 0xFFFFFF;
	unsigned long start, len, offset, ext_offset1;
	unsigned long type, entry1;
	partinfo *PI;
	unsigned long id;
	/* Look for the partition.  */
  PI = PART_INFO;
  id = 0UL;
  while ((	next_partition_drive		= current_drive,
		next_partition_dest		= 0xFFFFFF,
		next_partition_partition	= (void *)&part,
		next_partition_type		= (void *)&type,
		next_partition_start		= (void *)&start,
		next_partition_len		= (void *)&len,
		next_partition_offset		= (void *)&offset,
		next_partition_entry		= (void *)&entry1,
		next_partition_ext_offset	= (void *)&ext_offset1,
		next_partition_buf		= (void *)mbr,
		next_partition ()))
	{
	  if (type != PC_SLICE_TYPE_NONE
		&& ! IS_PC_SLICE_TYPE_BSD (type)
		&& ! IS_PC_SLICE_TYPE_EXTENDED (type))
	  {
		PI->id = ++id;
		PI->part = part;
		PI->start = start;
		++PI;
		if (id >= MAX_PART) break;
	  }
	  errnum = ERR_NONE;
	}
	PI->id = 0;
	errnum = ERR_NONE;

	long i,j;
	partinfo t_pi;
/*接分区位置排序(Ghost Style)*/
	for (i = 0; i < id; i++)
	  for (j = i+1; j < id; j++)
		if (PART_INFO[i].start > PART_INFO[j].start)
		{
			t_pi = PART_INFO[j];
			PART_INFO[j] = PART_INFO[i];
			PART_INFO[i] = t_pi;
		}

	return id;
}

static int
diskid_func (char *arg,int flags)
{
  unsigned long long ret = 0;
  partinfo PART_INFO[MAX_PART];
  unsigned long id;

  errnum = ERR_BAD_ARGUMENT;
  if (*arg && (memcmp(arg,"ret=",4) == 0))
  {
	arg =skip_to(1,arg);
	safe_parse_maxint(&arg,&ret);
	arg = skip_to (0,arg);
  }
  if (! *arg || *arg == ' ' || *arg == '\t')
  {
	current_drive = saved_drive;
	current_partition = saved_partition;
  }
	else if (memcmp(arg,"info",4) == 0)
	{
		unsigned long tmp_drive = saved_drive;
		unsigned long tmp_partition = saved_partition;
		char tmp[20];
		for (id=0;id<(*((char *)0x475));)
		{
			saved_drive = current_drive = 0x80 + id;
			current_partition = 0xffffff;
			open_device();
			ret = sprintf(tmp,"%ld",part_length<<9);
			printf(" Disk: %d\t",++id);
			if (ret > 9)
				printf("%.*s GB\n",(int)(ret-9),tmp);
			else
				printf("%.*s MB\n",(int)(ret-6),tmp);
			partinfo *pi = PART_INFO;
			get_partinfo(PART_INFO);
			while (pi->id)
			{
				current_partition = saved_partition = pi->part;
				open_device();
				if (current_drive == tmp_drive && current_partition == tmp_partition)
				{
					if (current_term->SETCOLORSTATE)
						current_term->SETCOLORSTATE (COLOR_STATE_HIGHLIGHT);
					printf("   * %d:%d\t",id,pi->id);
				}
				else
					printf("     %d:%d\t",id,pi->id);
				if (part_length>>22)
					printf("%d GB\t",part_length>>21);
				else
					printf("%d MB\t",part_length>>11);
				builtin_cmd("root","", 1);
				if (current_term->SETCOLORSTATE)
					current_term->SETCOLORSTATE (COLOR_STATE_STANDARD);
				++pi;
			}
		}
		saved_drive = tmp_drive;
		saved_partition = tmp_partition;
		return 1;
	}
	else if (memcmp(arg, "gid=", 4) == 0) /*gid x:y */
	{
		int dd,dp;
		arg =skip_to(1,arg);
		if (*arg=='*')/*如果是以*开头的,则读取对应内存地址的值*/
		{
			arg++;
			if (! safe_parse_maxint(&arg,&ret))
				return 0;
			arg =(char *)(grub_addr_t)ret;
		}

		if (! safe_parse_maxint(&arg,&ret))
			return 0;
		dd = (int)ret;
		arg++;
		if (! safe_parse_maxint(&arg,&ret))
			return 0;
		++arg;
		dp = (int)ret;
		if (dd == 0)
			dd = saved_drive - 0x7f;
		else if (dd < 0)
			dd = (*((char *)0x475)) + dd + 1;

		if (dd < 1)
			return 0;
		for(current_drive = 0x7f + dd;dd && dd<=(*((char *)0x475));++arg)
		{
			id = get_partinfo(PART_INFO);

			if (dp < 0)
				dp += id + 1;
			if (dp > id || dp <= 0)
				break;
			if (*arg == '+')
			{
				if (dp >= id)
				{
					dp = 1;
					if (dd < *((char *)0x475))
						++dd;
					else
						dd = 1;
					continue;
				}
				++dp;
			}
			else if (*arg == '-')
			{
				if (dp <= 1)
				{
					dp = -1;
					if (dd > 1)
						--dd;
					else
						dd = *((char *)0x475);
					continue;
				}
				--dp;
			}
			saved_drive = current_drive;
			saved_partition = PART_INFO[dp-1].part;
			sprintf((char *)RET_VAR,"%d:%d\r\r",dd,dp);
			sprintf(((char*)0x4CA00),"%d.%d",dd,dp);
			if (debug > 0)
				printf(" %d:%d",dd,dp);
			return builtin_cmd("root","()", 1);
		}

		return !(errnum = ERR_NO_PART);
	}
	else if (memcmp(arg, "uid=", 4) == 0)
  {
	ret = 0x20000;
  }
  else if (! set_device (arg))
    return 0;

  /* The drive must be a hard disk.  */
  if (! (current_drive & 0x80))
    {
       errnum = ERR_BAD_ARGUMENT;
      return 0;
    }
  
  /* The partition must be a PC slice.  */
  if ((current_partition >> 16) == 0xFF
      || (current_partition & 0xFFFF) != 0xFFFF)
    {
      errnum = ERR_BAD_ARGUMENT;
      return 0;
    }
	/* Look for the partition.  */
	id = get_partinfo(PART_INFO);
/*获取指定分区对应的ID*/	
	long i;
	for (i=0UL;i<id;++i)
		if (PART_INFO[i].part == current_partition)
		{
			if (ret) /*指定了ret参数,把ID信息写入到指定内存中格式:0xXXYYAABB */
			{
				int *P = (int *)(grub_addr_t)ret;
				*P = (current_drive - 0x80) & 0xff; /*XX 从0开始的磁盘编号*/
				*P <<= 8;
				*P |= PART_INFO[i].id;				/*YY 从1开始的分区编号,以该分在分区表中的位置排序*/
				*P <<= 8;
				*P |= (current_drive - 0x7F);		/*AA 从1开始的磁盘编号*/
				*P <<= 8;
				*P |= i + 1;						/*BB 从1开始的分区编号,以该分区在磁盘的位置排序*/
			}
			sprintf((char *)RET_VAR,"%d:%d\r\r",(unsigned char)(current_drive - 0x7F),i+1);
			sprintf(((char*)0x4CA00),"%d.%d",(unsigned char)(current_drive - 0x7F),i+1);
			if (debug > 0)
				printf(" (hd%d,%d) in Ghost Style is: %d:%d\n",(unsigned int)(current_drive-0x80),(unsigned int)(current_partition >> 16),(unsigned int)(current_drive - 0x7F),i+1);
			return 1;
		}

	return 0;
}

static void get_G4E_image (void)
{
  grub_size_t i;

  //在内存0-0x9ffff, 搜索特定字符串"GRUB4EFI"，获得GRUB_IMGE
  for (i = 0x9F100; i >= 0; i -= 0x1000)
  {
    if (*(unsigned long long *)i == 0x4946453442555247)	//比较数据
    {
      g4e_data = *(grub_size_t *)(i+16); //GRUB4DOS_for_UEFI入口
      return;
    }
  }
  return;
}
