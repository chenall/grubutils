/* disk_io.c - implement abstract BIOS disk input and output */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004  Free Software Foundation, Inc.
 *  Copyright (C) 2010  Tinybit(tinybit@tom.com)
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


#include "shared.h"

/* instrumentation variables */
void (*disk_read_hook) (unsigned long long, unsigned long, unsigned long) = NULL;
void (*disk_read_func) (unsigned long long, unsigned long, unsigned long) = NULL;

char saved_dir[256];

/* Forward declarations.  */
static char open_filename[512];
static unsigned long relative_path;

int print_possibilities;

static int unique;
static char *unique_string;
int do_completion;

int dir (char *dirname);

/* XX used for device completion in 'set_device' and 'print_completions' */
static int incomplete, disk_choice;
static int part_choice;

/* used by open_partition() */
unsigned long dest_partition; /* 0xFEFFFF to enumerate all partitions */

/* used by attempt_mount() */
unsigned long request_fstype;

/* The first sector of stage2 can be reused as a tmp buffer.
 * Do NOT write more than 512 bytes to this buffer!
 * The stage2-body, i.e., the pre_stage2, starts at 0x8200!
 * Do NOT overwrite the pre_stage2 code at 0x8200!
 */
char *mbr = (char *)0x8000; /* 512-byte buffer for any use. */

extern unsigned long next_partition_drive;
extern unsigned long *next_partition_type;
extern unsigned long long *next_partition_start;
extern unsigned long long *next_partition_len;
extern unsigned long long *next_partition_offset;
extern unsigned long *next_partition_entry;
extern unsigned long long *next_partition_ext_offset;
extern char *next_partition_buf;

unsigned long long part_offset;
unsigned long entry;
unsigned long long ext_offset;

unsigned long long fsmax;
struct fsys_entry fsys_table[NUM_FSYS + 1] =
{
  {"ext2fs", ext2fs_mount, ext2fs_read, ext2fs_dir/*, 0, 0*/},
  {"fat", fat_mount, fat_read, fat_dir/*, 0, 0*/},
  {"ntfs", ntfs_mount, ntfs_read, ntfs_dir/*, 0, 0*/},
  {"unknown", 0, 0, 0/*, 0, 0*/}
};

/*
 *  Global variables describing details of the filesystem
 */

/* filesystem type */
int fsys_type = NUM_FSYS;

/* disk buffer parameters */

//unsigned long buf_drive = -1;
//unsigned long long buf_track = -1;/* low 32 bit to invalidate the buffer */

//int rawread_ignore_memmove_overflow = 0;/* blocklist_func() set this to 1 */

/* Convert unicode filename to UTF-8 filename. N is the max UTF-16 characters
 * to be converted. The caller should asure there is enough room in the UTF8
 * buffer. Return the length of the converted UTF8 string.
 */
unsigned long
unicode_to_utf8 (unsigned short *filename, unsigned char *utf8, unsigned long n)
{
	unsigned short uni;
	unsigned long j, k;

	for (j = 0, k = 0; j < n && (uni = filename[j]); j++)
	{
		if (uni <= 0x007F)
		{
			if (uni != ' ')
				utf8[k++] = uni;
			else
			{
				/* quote the SPACE with a backslash */
				utf8[k++] = '\\';
				utf8[k++] = uni;
			}
		}
		else if (uni <= 0x07FF)
		{
			utf8[k++] = 0xC0 | (uni >> 6);
			utf8[k++] = 0x80 | (uni & 0x003F);
		}
		else
		{
			utf8[k++] = 0xE0 | (uni >> 12);
			utf8[k++] = 0x80 | ((uni >> 6) & 0x003F);
			utf8[k++] = 0x80 | (uni & 0x003F);
		}
	}
	utf8[k] = 0;
	return k;
}

#if 0 //def STAGE1_5
/* Read bytes from DRIVE to BUF. The bytes start at BYTE_OFFSET in absolute
 * sector number SECTOR and with BYTE_LEN bytes long.
 */
int
rawread (unsigned long drive, unsigned long long sector, unsigned long byte_offset, unsigned long byte_len, unsigned long long buf, unsigned long write)
{
  unsigned long slen;

  if (write != 0x900ddeed && write != 0xedde0d90)
	return !(errnum = ERR_FUNC_CALL);

  //errnum = 0;

  if (write == 0x900ddeed && ! buf)
    return 1;

  while (byte_len > 0)
  {
      unsigned long soff, num_sect, size = byte_len;
      unsigned long long track;
      char *bufaddr;
      int bufseg;

      if (! buf)
	goto list_blocks;

      /* Reset geometry and invalidate track buffer if the disk is wrong. */
      if (buf_drive != drive)
      {
	  buf_drive = drive;
	  buf_track = -1;
      }

      /* Sectors that need to read. */
      slen = ((byte_offset + byte_len + SECTOR_SIZE - 1) >> SECTOR_BITS);

      /* Get the first sector number in the track.  */
      soff = sector % 64;

      /* Get the starting sector number of the track. */
      track = sector - soff;

      /* max number of sectors to read in the track. */
      num_sect = 64 - soff;

      /* Read data into the track buffer; Not all sectors in the track would be filled in. */
      bufaddr = ((char *) BUFFERADDR + (soff << SECTOR_BITS) + byte_offset);
      bufseg = BUFFERSEG;

      if (track != buf_track)
      {
	  unsigned long long read_start = track;	/* = sector - soff <= sector */
	  unsigned long read_len = 64;		/* >= num_sect */

	  buf_track = track;

	  /* If more than one track need to read, only read the portion needed
	   * rather than the whole track with data that won't be used.  */
	  if (slen > num_sect)
	  {
	      buf_track = -1;		/* invalidate the buffer */
	      read_start = sector;	/* read the portion from this sector */
	      read_len = num_sect;	/* to the end of the track */
	      //bufaddr = (char *) BUFFERADDR + byte_offset;
	      bufseg = BUFFERSEG + (soff << (SECTOR_BITS - 4));
	  }

	  if (biosdisk (BIOSDISK_READ, drive, read_start, read_len, bufseg))
	  {
	      buf_track = -1;		/* invalidate the buffer */
	      /* On error try again to load only the required sectors. */
	      if (slen > num_sect || slen == read_len)
		    return !(errnum = ERR_READ);
	      bufseg = BUFFERSEG + (soff << (SECTOR_BITS - 4));
	      if (biosdisk (BIOSDISK_READ, drive, sector, slen, bufseg))
		    return !(errnum = ERR_READ);
	      //bufaddr = (char *) BUFFERADDR + byte_offset;
	      /* slen <= num_sect && slen < sectors_per_vtrack */
	      num_sect = slen;
	  }
      } /* if (track != buf_track) */

      /* num_sect is sectors that has been read at BUFADDR and will be used. */
      if (size > (num_sect << SECTOR_BITS) - byte_offset)
	  size = (num_sect << SECTOR_BITS) - byte_offset;

      if (write == 0x900ddeed)
      {
	  /* buf != 0 */
	  if (grub_memcmp ((char *)(unsigned int)buf, bufaddr, size) == 0)
		goto next;		/* no need to write */
	  buf_track = -1;		/* invalidate the buffer */
	  grub_memmove (bufaddr, (char *)(unsigned int)buf, size);	/* update data at bufaddr */
	  /* write it! */
	  bufseg = BUFFERSEG + (soff << (SECTOR_BITS - 4));
	  if (biosdisk (BIOSDISK_WRITE, drive, sector, num_sect, bufseg))
		return !(errnum = ERR_WRITE);
	  goto next;
      }

list_blocks:
      /* Use this interface to tell which sectors were read and used. */
      if (disk_read_func)
      {
	  unsigned long long sector_num = sector;
	  unsigned long length = SECTOR_SIZE - byte_offset;
	  if (length > size)
	      length = size;
	  (*disk_read_func) (sector_num++, byte_offset, length);
	  length = size - length;
	  if (length > 0)
	  {
	      while (length > SECTOR_SIZE)
	      {
		  (*disk_read_func) (sector_num++, 0, SECTOR_SIZE);
		  length -= SECTOR_SIZE;
	      }
	      (*disk_read_func) (sector_num, 0, length);
	  }
      }

      if (buf)
      {
	  grub_memmove ((char *)(unsigned int)buf, bufaddr, size);
//      if (errnum == ERR_WONT_FIT)
//      {
//	  if (! rawread_ignore_memmove_overflow && buf)
//		return 0;
//	  errnum = 0;
//	  buf = 0/*NULL*/; /* so that further memcheck() always fail */
//      }
//      else
next:
	  buf += size;
      }
      byte_len -= size;		/* byte_len always >= size */
      sector += num_sect;
      byte_offset = 0;
  } /* while (byte_len > 0) */

  return 1;//(!errnum);
}
#endif /* STAGE1_5 */


#if 0 //def STAGE1_5
int
devread (unsigned long long sector, unsigned long byte_offset, unsigned long byte_len, unsigned long long buf, unsigned long write)
{
  /* Check partition boundaries */
  if (part_start && ((sector + ((byte_offset + byte_len - 1) >> SECTOR_BITS)) >= part_length))
      return !(errnum = ERR_OUTSIDE_PART);

  /* Get the read to the beginning of a partition. */
  sector += byte_offset >> SECTOR_BITS;
  byte_offset &= SECTOR_SIZE - 1;

  sector += part_start;

  /*  Call RAWREAD, which is very similar, but:
   *  --  It takes an extra parameter, the drive number.
   *  --  It requires that "sector" is relative to the beginning of the disk.
   *  --  It doesn't handle offsets across the sector boundary.
   */
  return rawread (current_drive, sector, byte_offset, byte_len, buf, write);
}
#endif /* STAGE1_5 */


#if 0 //def STAGE1_5
  /* Get next PC slice. Be careful of that this function may return
     an empty PC slice (i.e. a partition whose type is zero) as well.  */
int
next_partition (void)
{
redo:
      /* If this is the first time...  */
      if ((char)(*next_partition_partnum) == -2)
	{
		*next_partition_offset = 0;
		*next_partition_ext_offset = 0;
		*next_partition_entry = -1;
		*next_partition_type = 0x0C;
		*next_partition_start = 0;
		*next_partition_len = 0;
		(*next_partition_partnum)++;
		return 1;
	}

      /* Read the MBR or the boot sector of the extended partition.  */
      if (! rawread (next_partition_drive, *next_partition_offset, 0, SECTOR_SIZE, (unsigned long long)(unsigned int)next_partition_buf, 0xedde0d90))
	return 0;

      /* Check if it is valid.  */
      if (! PC_MBR_CHECK_SIG (next_partition_buf))
	{
	  errnum = ERR_BAD_PART_TABLE;
	  return 0;
	}

next_entry:
      /* Increase the entry number.  */
      (*next_partition_entry)++;

      /* If this is out of current partition table...  */
      if (*next_partition_entry == 4)
	{
	  int i;

	  /* Search the first extended partition in current table.  */
	  for (i = 0; i < 4; i++)
	    {
	      if (IS_PC_SLICE_TYPE_EXTENDED (PC_SLICE_TYPE (next_partition_buf, i)))
		{
		  /* Found. Set the new offset and the entry number, and restart this function.  */
		  *next_partition_offset = *next_partition_ext_offset + PC_SLICE_START (next_partition_buf, i);
		  if (! *next_partition_ext_offset)
		    *next_partition_ext_offset = *next_partition_offset;
		  *next_partition_entry = -1;

		  goto redo;
		}
	    }

	  errnum = ERR_NO_PART;
	  return 0;
	}

      *next_partition_type = PC_SLICE_TYPE (next_partition_buf, *next_partition_entry);
      *next_partition_start = *next_partition_offset + PC_SLICE_START (next_partition_buf, *next_partition_entry);
      *next_partition_len = PC_SLICE_LENGTH (next_partition_buf, *next_partition_entry);

      /* The calculation of a PC slice number is complicated, because of
	 the rather odd definition of extended partitions. Even worse,
	 there is no guarantee that this is consistent with every
	 operating systems. Uggh.  */
      if ((char)(*next_partition_partnum) >= 3	/* if it is a logical partition */
	  && (PC_SLICE_ENTRY_IS_EMPTY (next_partition_buf, *next_partition_entry))) /* ignore the garbage entry(typically all bytes are 0xF6). */
	goto next_entry;
#if 1
      /* disable partition id 00. */
      if ((char)(*next_partition_partnum) >= 3	/* if it is a logical partition */
	  && *next_partition_type == PC_SLICE_TYPE_NONE)	/* ignore the partition with id=00. */
	goto next_entry;
#else
      /* enable partition id 00. */
#endif

      if ((char)(*next_partition_partnum) < 3 || ! IS_PC_SLICE_TYPE_EXTENDED (*next_partition_type))
	(*next_partition_partnum)++;

      if ((char)(*next_partition_partnum) < 0)
      {
	errnum = ERR_NO_PART;
	return 0;
      }
      //errnum = 0;
      return 1;
}
#endif /* STAGE1_5 */


#if 0
int
attempt_mount (void)
{
  for (fsys_type = 0; fsys_type < NUM_FSYS; fsys_type++)
  {
    if ((fsys_table[fsys_type].mount_func) ())
      break;
  }

  errnum = ((fsys_type == NUM_FSYS) ? ERR_FSYS_MOUNT : 0);
  return (! errnum);
}
#endif


/*
 *  This performs a "mount" on the current device, both drive and partition
 *  number.
 */

int
open_device (void)
{
  request_fstype = 1;
  dest_partition = current_partition;
  return open_partition ();
}


#if 0 //def STAGE1_5
/* Open a partition.  */
int
real_open_partition (unsigned long dest_partition)
{
  current_slice = 0;
  part_start = 0;
  part_length = 0;

  /* If this is the whole disk, return here.  */
  if (dest_partition == current_partition && current_partition == 0xFFFFFF)
    return 1;

  /* now if dest_partition == current_partition, current_partition != 0xFFFFFF */

//  if (dest_partition != current_partition)
//    dest_partition = 0xFFFFFF;

  /* if flags == 0, dest_partition == current_partition */
  /* if flags == 0, dest_partition != 0xFFFFFF */
  /* if flags != 0, dest_partition == 0xFFFFFF */

  /* Initialize CURRENT_PARTITION for next_partition.  */
  current_partition = 0xFEFFFF;

  while ((	next_partition_drive		= current_drive,
		next_partition_partnum		= ((unsigned char *)(&current_partition)) + 2,
		next_partition_type		= &current_slice,
		next_partition_start		= &part_start,
		next_partition_len		= &part_length,
		next_partition_offset		= &part_offset,
		next_partition_entry		= &entry,
		next_partition_ext_offset	= &ext_offset,
		next_partition_buf		= mbr,
		next_partition ()))
    {
      /* If this is a valid partition...  */
      if (current_slice)
	{
	  /* Display partition information.  */
	  if (dest_partition == 0xFEFFFF)
	    {
		//if (! IS_PC_SLICE_TYPE_EXTENDED (current_slice))
		{
			if (! do_completion)
			  {
				grub_printf ("%2ld:%016lx,%016lx:%02X,%02X:"
						, (long long)(char)*next_partition_partnum
						, (unsigned long long)part_start
						, (unsigned long long)part_length
						, *( (unsigned char *) (((int) mbr) + 446 + (entry << 4)) )
						, *( (unsigned char *) (((int) mbr) + 450 + (entry << 4)) )
					    );
				attempt_mount ();
				printf ("%s\n", fsys_table[fsys_type].name);
			  }
			else
			  {
				char str[8];
				grub_sprintf (str, "%ld)", (long long)(char)*next_partition_partnum);
				print_a_completion (str);
			  }
			errnum = 0;
		}
	    }
	  else
	    {
		if (current_partition == dest_partition)
			return (request_fstype ? attempt_mount () : 1);
	    }
	}
    }

//  if (dest_partition == 0xFEFFFF)
//    {
//	errnum = 0;
//	return 1;
//    }

  return 0;
}
#endif /* STAGE1_5 */


//int
//open_partition (void)
//{
//	dest_partition = current_partition;
//	return real_open_partition ();
//}


/* Parse a device string and initialize the global parameters. */
char *
set_device (char *device)
{
  int result = 0;

  errnum = 0;
  incomplete = 0;
  disk_choice = 1;
  part_choice = 0;
  current_drive = saved_drive;
  current_partition = 0xFFFFFF;

  if (*device == '(' && !*(device + 1))
    /* user has given '(' only, let disk_choice handle what disks we have */
    return device + 1;

  if (*device == '(' && *(++device))
    {
      if (*device == ')')	/* the device "()" is for the current root */
	{
	  current_partition = saved_partition;
	  part_choice = 2;
	  return device + 1;
	}
      if (*device != ',' /* && *device != ')' */ )
	{
	  char ch = *device;
	  if (*device == 'f' || *device == 'h' || *device == 'm' || *device == 'r' || *device == 'b' 
	     )
	    {
	      /* user has given '([fhn]', check for resp. add 'd' and
		 let disk_choice handle what disks we have */
	      if (!*(device + 1))
		{
		  device++;
		  *device++ = 'd';
		  *device = '\0';
		  return device;
		}
	      else if (*(device + 1) == 'd' && !*(device + 2))
		return device + 2;
	    }

	  if ((*device == 'f'
	      || *device == 'h'
	      || *device == 'm'
	      || *device == 'r'
	      || *device == 'b'
	      )
	      && *(++device, device++) != 'd')
	    errnum = ERR_NUMBER_PARSING;

	    {
	      if (ch == 'm')
		current_drive = 0xffff;
	      else if (ch == 'r')
		current_drive = ram_drive;
	      else if (ch == 'b')
		{
			current_partition = install_partition;
			current_drive = boot_drive;
			return device + 1;
		}
	      else if (ch == 'h' && (*device == ',' || *device == ')'))
		{
			/* it is (hd) for the next new drive that can be added. */
			current_drive = (unsigned char)(0x80 + (*(unsigned char *)0x475));
		}
	      else
		{
		  unsigned long long ull;

		  safe_parse_maxint (&device, &ull);
		  current_drive = ull;
		  disk_choice = 0;
		  if (ch == 'h')
		  {
			if ((long long)ull < 0)
			{
				if ((-ull) <= (unsigned long long)(*(unsigned char *)0x475))
					current_drive = (unsigned char)(0x80 + (*(unsigned char *)0x475) + current_drive);
				else
					return (char *)!(errnum = ERR_DEV_FORMAT);
			} else
				current_drive |= 0x80;
		  }
		}
	    }
	}

      if (errnum)
	return 0;

      if (*device == ')')
	{
	  part_choice = 2;
	  result = 1;
	}
      else if (*device == ',')
	{
	  disk_choice = 0;
	  part_choice ++;
	  device++;

	  if ((*device == '-') || (*device >= '0' && *device <= '9'))
	    {
	      unsigned long long ull;
	      //current_partition = 0;

	      if (!safe_parse_maxint (&device, &ull)
		  || ((long long)ull < (long long)-1LL)
		  || ((long long)ull > (long long)32767)
		  || ((unsigned char)ull == 254)
		  || (*device == ',')
		 )
		{
		  errnum = ERR_DEV_FORMAT;
		  return 0;
		}

	      part_choice ++;
	      current_partition = (((unsigned char)ull != (unsigned char)255) ? (((unsigned long)(unsigned short)ull) << 16) + 0xFFFF : 0xFFFFFF);

	    }

	  if (*device == ')')
	    {
	      if (part_choice == 1)
		{
		  current_partition = saved_partition;
		  part_choice ++;
		}

	      result = 1;
	    }
	}
    }

  if (result)
    return device + 1;
  else
    {
      if (!*device)
	incomplete = 1;
      errnum = ERR_DEV_FORMAT;
    }

  return 0;
}

/*
 * setup CURRENT_DRIVE and CURRENT_PARTITION for FILENAME, and check if it
 * is available.
 */

static char *
setup_part (char *filename)
{
  relative_path = 1;
  errnum = 0;

  if (*filename == '(')
    {
	relative_path = 0;
	if ((filename = set_device (filename)) == 0)
	{
	  //current_drive = GRUB_INVALID_DRIVE;
	  return 0;
	}
	request_fstype = (*filename == '/');
	dest_partition = current_partition;
	open_partition ();
    }
  else
    {
	/* Always do open_partition to setup the global part_start variable
	 * properly. Comment out these conditions. -- tinybit 2012-11-02 */
	//if (current_drive != saved_drive
	//   || current_partition != saved_partition
	//   || (*filename == '/' && fsys_type == NUM_FSYS)
	//   || buf_drive == -1)
	{
	    current_drive = saved_drive;
	    current_partition = saved_partition;
	    /* allow for the error case of "no filesystem" after the partition
	       is found.  This makes block files work fine on no filesystem */
	    request_fstype = (*filename == '/');
	    dest_partition = current_partition;
	    open_partition ();
	}
    }


  if (errnum && (*filename == '/' || errnum != ERR_FSYS_MOUNT))
    return 0;

  errnum = 0;
  return filename;
}


int
dir (char *dirname)
{
  if (!(dirname = setup_part (dirname)))
    return 0;

  if (*dirname != '/')
    errnum = ERR_BAD_FILENAME;

  if (fsys_type == NUM_FSYS)
    errnum = ERR_FSYS_MOUNT;

  if (relative_path)
  {
    if (grub_strlen(saved_dir) + grub_strlen(dirname) >= sizeof(open_filename))
      errnum = ERR_WONT_FIT;
  }
  else
  {
    if (grub_strlen(dirname) >= sizeof(open_filename))
      errnum = ERR_WONT_FIT;
  }

  if (errnum)
    return 0;

  /* set "dir" function to list completions */
  print_possibilities = 1;

  if (relative_path)
    grub_sprintf (open_filename, "%s%s", saved_dir, dirname);
  else
    grub_sprintf (open_filename, "%s", dirname);
  nul_terminate (open_filename);
  return (*(fsys_table[fsys_type].dir_func)) (open_filename);
}


/* If DO_COMPLETION is true, just print NAME. Otherwise save the unique
   part into UNIQUE_STRING.  */
void
print_a_completion (char *name, int case_insensitive)
{
  /* If NAME is "." or "..", do not count it.  */
  if (grub_strcmp (name, ".") == 0 || grub_strcmp (name, "..") == 0)
    return;

  if (do_completion)
    {
      char *buf = unique_string;

      if (! unique)
	while ((*buf++ = (case_insensitive ? tolower(*name++): (*name++))))
	  ;
      else
	{
	  while (*buf && (*buf == (case_insensitive ? tolower(*name) : *name)))
	    {
	      buf++;
	      name++;
	    }
	  /* mismatch, strip it.  */
	  *buf = '\0';
	}
    }
  else
    grub_printf ("%s\t", name);

  unique++;
}

/*
 *  This lists the possible completions of a device string, filename, or
 *  any sane combination of the two.
 */

extern int is_filename;
//extern int is_completion;

int
print_completions (void)
{
  char *buf = (char *) COMPLETION_BUF;
  char *ptr = buf;

  unique_string = (char *) UNIQUE_BUF;
  *unique_string = 0;
  unique = 0;
  //do_completion = is_completion;

  if (! is_filename)
    {
	/* Print the completions of builtin commands.  */
	struct builtin **builtin;

	for (builtin = builtin_table; (*builtin); builtin++)
	{
	  if (substring (buf, (*builtin)->name, 0) <= 0)
	    print_a_completion ((*builtin)->name, 0);
	}

	if (do_completion && *unique_string)
	{
	  if (unique == 1)
	    {
	      char *u = unique_string + grub_strlen (unique_string);

	      *u++ = ' ';
	      *u = 0;
	    }

	  grub_strcpy (buf, unique_string);
	}

	//do_completion = 0;
	return unique - 1;
    }

  if (*buf == '/' || (ptr = set_device (buf)) || incomplete)
    {
      errnum = 0;

      if (*buf == '(' && (incomplete || ! *ptr))
	{
	  if (! part_choice)
	    {
	      /* disk completions */
	      int j;

	      if (!ptr || *(ptr-1) != 'd' || (*(ptr-2) != 'm' && *(ptr-2) != 'r'))
		{
		  int k;
		  for (k = (ptr && (*(ptr-1) == 'd' && *(ptr-2) == 'h') ? 1:0);
		       k < (ptr && (*(ptr-1) == 'd' && *(ptr-2) == 'f') ? 1:2);
		       k++)
		    {
#define HARD_DRIVES (*((char *)0x475))
#define FLOPPY_DRIVES ((*(char*)0x410 & 1)?(*(char*)0x410 >> 6) + 1 : 0)
		      for (j = 0; j < (k ? HARD_DRIVES : FLOPPY_DRIVES); j++)
#undef HARD_DRIVES
#undef FLOPPY_DRIVES
			{
			  unsigned long i;
			  i = (k * 0x80) + j;
			  if ((disk_choice || i == current_drive)
			      /*&& ! get_diskinfo (i, &tmp_geom)*/)
			    {
			      char dev_name[8];

			      grub_sprintf (dev_name, "%cd%d", (k ? 'h':'f'), (unsigned long)j);
			      print_a_completion (dev_name, 0);
			    }
			}
		    }
		}

	      if (do_completion && *unique_string)
		{
		  ptr = buf;
		  while (*ptr != '(')
		    ptr--;
		  ptr++;
		  grub_strcpy (ptr, unique_string);
		  if (unique == 1)
		    {
		      ptr += grub_strlen (ptr);
//		      if (*unique_string == 'h')
//			{
			  *ptr++ = ',';
			  *ptr = 0;
//			}
//		      else
//			{
//			  *ptr++ = ')';
//			  *ptr = 0;
//			}
		    }
		}
	    }
	  else
	    {
	      /* partition completions */
	      if (part_choice == 2 && ((request_fstype = 0), (dest_partition = current_partition), open_partition ()))
		{
		    {
		      unique = 1;
		      ptr = buf + grub_strlen (buf);
		      if (*(ptr - 1) != ')')
			{
			  *ptr++ = ')';
			  *ptr = 0;
			}
		      else
			{
			  *ptr++ = '/';
			  *ptr = 0;
			}
		    }
		}
	      else
		{
		    {
			//request_fstype = 1;
			dest_partition = 0xFEFFFF;
			open_partition ();
			errnum = 0;

			if (do_completion && *unique_string)
			  {
			    ptr = buf;
			    while (*ptr++ != ',')
				;
			    grub_strcpy (ptr, unique_string);
			  }
		    }
		}
	    }
	}
      else if (ptr && *ptr == '/')
	{
	  /* filename completions */

	  dir (buf);

	  if (do_completion && *unique_string)
	    {
	      ptr += grub_strlen (ptr);
	      while (*ptr != '/')
		ptr--;
	      ptr++;

	      grub_strcpy (ptr, unique_string);

	      if (unique == 1)
		{
		  ptr += grub_strlen (unique_string);

		  /* Check if the file UNIQUE_STRING is a directory.  */
		  *ptr = '/';
		  *(ptr + 1) = 0;

		  dir (buf);

		  /* Restore the original unique value.  */
		  unique = 1;

		  if (errnum)
		    {
		      /* Regular file */
		      errnum = 0;
		      *ptr = ' ';
		      *(ptr + 1) = 0;
		    }
		}
	    }
	}
      else
	errnum = ERR_BAD_FILENAME;
    }

  //do_completion = 0;
  if (errnum)
    return -1;
  else
    return unique - 1;
}


static int block_file = 0;

/*
 *  This is the generic file open function.
 */

int
grub_open (char *filename)
{
  filepos = 0;

  if (!(filename = setup_part (filename)))
    return 0;

  block_file = 0;

  /* This accounts for partial filesystem implementations. */
  fsmax = -1ULL;

  if (*filename != '/')
    {
      char *ptr = filename;
      unsigned long list_addr = FSYS_BUF + 12;  /* BLK_BLKLIST_START */
      filemax = 0;

      while (list_addr < FSYS_BUF + 0x77F9)	/* BLK_MAX_ADDR */
	{
	  unsigned long long tmp;
	  tmp = 0;
	  safe_parse_maxint (&ptr, &tmp);
	  errnum = 0;

	  if (*ptr != '+')
	    {
	      /* Set FILEMAX in bytes, Undocumented!! */

	      /* The FILEMAX must not exceed the one calculated from
	       * all the blocks in the list.
	       */

	      if ((*ptr && *ptr != '/' && *ptr != ' ' && *ptr != '\t')
		  || tmp == 0 || tmp > filemax)
		break;		/* failure */

	      filemax = tmp;
	      goto block_file;		/* success */
	    }

	  /* since we use the same filesystem buffer, mark it to
	     be remounted */
	  fsys_type = NUM_FSYS;

	  *((unsigned long*)list_addr) = tmp;	/* BLK_BLKSTART */
	  ptr++;		/* skip the plus sign */

	  safe_parse_maxint (&ptr, &tmp);

	  if (errnum)
		return 0;

	  if (!tmp || (*ptr && *ptr != ',' && *ptr != '/' && *ptr != ' ' && *ptr != '\t'))
		break;		/* failure */

	  *((unsigned long*)(list_addr+4)) = tmp;	/* BLK_BLKLENGTH */

	  filemax += tmp * SECTOR_SIZE;
	  list_addr += 8;			/* BLK_BLKLIST_INC_VAL */

	  if (*ptr != ',')
		goto block_file;		/* success */

	  ptr++;		/* skip the comma sign */
	} /* while (list_addr < FSYS_BUF + 0x77F9) */

      return !(errnum = ERR_BAD_FILENAME);

block_file:
	  block_file = 1;
	  (*((unsigned long*)FSYS_BUF))= 0;	/* BLK_CUR_FILEPOS */
	  (*((unsigned long*)(FSYS_BUF+4))) = FSYS_BUF + 12;	/* let BLK_CUR_BLKLIST = BLK_BLKLIST_START */
	  (*((unsigned long*)(FSYS_BUF+8))) = 0;	/* BLK_CUR_BLKNUM */

	  if (current_drive == ram_drive && filemax == 512/* &&  filemax < rd_size*/ && (*(long *)(FSYS_BUF + 12)) == 0)
	  {
		filemax = rd_size;
		*(long *)(FSYS_BUF + 16) = (filemax + 0x1FF) >> 9;
	  }
	  return 1;
    } /* if (*filename != '/') */

  if (!errnum && fsys_type == NUM_FSYS)
    errnum = ERR_FSYS_MOUNT;

  /* set "dir" function to open a file */
  print_possibilities = 0;

  if (relative_path)
  {
    if (grub_strlen(saved_dir) + grub_strlen(filename) >= sizeof(open_filename))
      errnum = ERR_WONT_FIT;
  }
  else
  {
    if (grub_strlen(filename) >= sizeof(open_filename))
      errnum = ERR_WONT_FIT;
  }

  if (errnum)
    return 0;

  if (relative_path)
    grub_sprintf (open_filename, "%s%s", saved_dir, filename);
  else
    grub_sprintf (open_filename, "%s", filename);
  nul_terminate (open_filename);
  if (!errnum && (*(fsys_table[fsys_type].dir_func)) (open_filename))
    {
      return 1;
    }

  return 0;
}


unsigned long long
grub_read (unsigned long long buf, unsigned long long len, unsigned long write)
{
  if (filepos >= filemax)
      return 0;//!(errnum = ERR_FILELENGTH);

  if (len > filemax - filepos)
      len = filemax - filepos;

  if (filepos + len > fsmax)
      return !(errnum = ERR_FILELENGTH);

  if (block_file)
    {
      unsigned long size, off, ret = 0;

      while (len && !errnum)
	{
	  /* we may need to look for the right block in the list(s) */
	  if (filepos < (*((unsigned long*)FSYS_BUF)) /* BLK_CUR_FILEPOS */)
	    {
	      (*((unsigned long*)FSYS_BUF)) = 0;
	      (*((unsigned long*)(FSYS_BUF+4))) = FSYS_BUF + 12;	/* let BLK_CUR_BLKLIST = BLK_BLKLIST_START */
	      (*((unsigned long*)(FSYS_BUF+8))) = 0;	/* BLK_CUR_BLKNUM */
	    }

	  /* run BLK_CUR_FILEPOS up to filepos */
	  while (filepos > (*((unsigned long*)FSYS_BUF)))
	    {
	      if ((filepos - ((*((unsigned long*)FSYS_BUF)) & ~(SECTOR_SIZE - 1))) >= SECTOR_SIZE)
		{
		  (*((unsigned long*)FSYS_BUF)) += SECTOR_SIZE;
		  (*((unsigned long*)(FSYS_BUF+8)))++;

		  if ((*((unsigned long*)(FSYS_BUF+8))) >= *((unsigned long*) ((*((unsigned long*)(FSYS_BUF+4))) + 4)) )
		    {
		      (*((unsigned long*)(FSYS_BUF+4))) += 8;	/* BLK_CUR_BLKLIST */
		      (*((unsigned long*)(FSYS_BUF+8))) = 0;	/* BLK_CUR_BLKNUM */
		    }
		}
	      else
		(*((unsigned long*)FSYS_BUF)) = filepos;
	    }

	  off = filepos & (SECTOR_SIZE - 1);

	  {
	    unsigned long long tmp;

	    tmp = ((unsigned long long)(*((unsigned long*)((*((unsigned long*)(FSYS_BUF+4))) + 4)) - (*((unsigned long*)(FSYS_BUF+8))))
		  * (unsigned long long)(SECTOR_SIZE)) - (unsigned long long)off;
	    size = (tmp > (unsigned long long)len) ? len : (unsigned long)tmp;
	  }

	  disk_read_func = disk_read_hook;

	  /* read current block and put it in the right place in memory */
	  devread ((*((unsigned long*)(*((unsigned long*)(FSYS_BUF+4))))) + (*((unsigned long*)(FSYS_BUF+8))),
		   off, size, buf, write);

	  disk_read_func = NULL;

	  len -= size;
	  filepos += size;
	  ret += size;
	  if (buf)
		buf += size;
	}

      if (errnum)
	ret = 0;

      return ret;
    }

  if (fsys_type == NUM_FSYS)
      return !(errnum = ERR_FSYS_MOUNT);

  return (*(fsys_table[fsys_type].read_func)) (buf, len, write);
}

