/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000,2001,2005   Free Software Foundation, Inc.
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

typedef __signed__ char __s8;
typedef unsigned char __u8;
typedef __signed__ short __s16;
typedef unsigned short __u16;
typedef __signed__ int __s32;
typedef unsigned int __u32;

/* Note that some shorts are not aligned, and must therefore
 * be declared as array of two bytes.
 */
struct fat_bpb {
	__s8	ignored[3];	/* Boot strap short or near jump */
	__s8	system_id[8];	/* Name - can be used to special case
				   partition manager volumes */
	__u8	bytes_per_sect[2];	/* bytes per logical sector */
	__u8	sects_per_clust;/* sectors/cluster */
	__u8	reserved_sects[2];	/* reserved sectors */
	__u8	num_fats;	/* number of FATs */
	__u8	dir_entries[2];	/* root directory entries */
	__u8	short_sectors[2];	/* number of sectors */
	__u8	media;		/* media code (unused) */
	__u16	fat_length;	/* sectors/FAT */
	__u16	secs_track;	/* sectors per track */
	__u16	heads;		/* number of heads */
	__u32	hidden;		/* hidden sectors (unused) */
	__u32	long_sectors;	/* number of sectors (if short_sectors == 0) */

	/* The following fields are only used by FAT32 */
	__u32	fat32_length;	/* sectors/FAT */
	__u16	flags;		/* bit 8: fat mirroring, low 4: active fat */
	__u8	version[2];	/* major, minor filesystem version */
	__u32	root_cluster;	/* first cluster in root directory */
	__u16	info_sector;	/* filesystem info sector */
	__u16	backup_boot;	/* backup boot sector */
	__u16	reserved2[6];	/* Unused */
};

#define FAT_CVT_U16(bytarr) (* (__u16*)(bytarr))

/*
 *  Defines how to differentiate a 12-bit and 16-bit FAT.
 */

#define FAT_MAX_12BIT_CLUST       4087	/* 4085 + 2 */

/*
 *  Defines for the file "attribute" byte
 */

#define FAT_ATTRIB_OK_MASK        0x37
#define FAT_ATTRIB_NOT_OK_MASK    0xC8
#define FAT_ATTRIB_DIR            0x10
#define FAT_ATTRIB_LONGNAME       0x0F

/*
 *  Defines for FAT directory entries
 */

#define FAT_DIRENTRY_LENGTH       32

#define FAT_DIRENTRY_ATTRIB(entry)	(*(unsigned char *)(entry+11))
#define FAT_DIRENTRY_VALID(entry) \
  ( ((*((unsigned char *) entry)) != 0) \
    && ((*((unsigned char *) entry)) != 0xE5) \
    && !(FAT_DIRENTRY_ATTRIB(entry) & FAT_ATTRIB_NOT_OK_MASK) )
#define FAT_DIRENTRY_FIRST_CLUSTER(entry) \
  ((*(unsigned short *)(entry+26))+((*(unsigned short *)(entry+20)) << 16))
#define FAT_DIRENTRY_FILELENGTH(entry)	(*(unsigned long *)(entry+28))

#define FAT_LONGDIR_ID(entry)	(*(unsigned char *)(entry))
#define FAT_LONGDIR_ALIASCHECKSUM(entry)	(*(unsigned char *)(entry+13))

struct fat_superblock 
{
  int fat_offset;
  int fat_length;
  int fat_size;
  int root_offset;
  int root_max;
  int data_offset;
  
  int num_sectors;
  int num_clust;
  int clust_eof_marker;
  int sects_per_clust;
//  int sectsize_bits;
  unsigned int clustsize;
  int root_cluster;
  
  int cached_fat;
  int file_cluster;
  unsigned long current_cluster_num;
  int current_cluster;
};

/* pointer(s) into filesystem info buffer for DOS stuff */
#define FAT_SUPER ((struct fat_superblock *)(FSYS_BUF + 32256))/* 512 bytes long */
#define FAT_BUF   (FSYS_BUF + 30208)	/* 4 sector FAT buffer(2048 bytes) */
#define NAME_BUF  (FSYS_BUF + 28160)	/* unicode name buffer(833*2 bytes)*/
#define UTF8_BUF  (FSYS_BUF + 25600)	/* UTF8 name buffer (833*3 bytes)*/

#define FAT_CACHE_SIZE 2048

#if 0
int
fat_mount (void)
{
  struct fat_bpb bpb;
  __u32 magic, first_fat;
  
  /* Read bpb */
  if (! devread (0, 0, sizeof (bpb), (unsigned long long)(unsigned int)(char *) &bpb, 0xedde0d90))
    return 0;

  /* Check if the number of sectors per cluster is zero here, to avoid
     zero division.  */
  if (bpb.sects_per_clust == 0)
    return 0;
  
  /*  Sectors per cluster. Valid values are 1, 2, 4, 8, 16, 32, 64 and 128.
   *  But a cluster size larger than 32K should not occur.
   */
  if (128 % bpb.sects_per_clust)
    return 0;

  /* sector size must be 512 */
  if (FAT_CVT_U16(bpb.bytes_per_sect) != SECTOR_SIZE)
    return 0;

  FAT_SUPER->clustsize = (bpb.sects_per_clust << 9);
  
  /* reserved sectors should not be 0 for fat_fs */
  if (FAT_CVT_U16 (bpb.reserved_sects) == 0)
    return 0;

  /* Number of FATs(nearly always 2).  */
  if ((unsigned char)(bpb.num_fats - 1) > 1)
    return 0;
  
  /* sectors per track(between 1 and 63).  */
  if ((unsigned short)(bpb.secs_track - 1) > 62)
    return 0;
  
  /* number of heads(between 1 and 256).  */
  if ((unsigned short)(bpb.heads - 1) > 255)
    return 0;
  
  /* Fill in info about super block */
  FAT_SUPER->num_sectors = FAT_CVT_U16 (bpb.short_sectors) 
    ? FAT_CVT_U16 (bpb.short_sectors) : bpb.long_sectors;
  
  /* FAT offset and length */
  FAT_SUPER->fat_offset = FAT_CVT_U16 (bpb.reserved_sects);
  FAT_SUPER->fat_length = 
    bpb.fat_length ? bpb.fat_length : bpb.fat32_length;
  
  /* Rootdir offset and length for FAT12/16 */
  FAT_SUPER->root_offset = 
    FAT_SUPER->fat_offset + bpb.num_fats * FAT_SUPER->fat_length;
  FAT_SUPER->root_max = FAT_DIRENTRY_LENGTH * FAT_CVT_U16(bpb.dir_entries);
  
  /* Data offset and number of clusters */
  FAT_SUPER->data_offset = 
    FAT_SUPER->root_offset
    + ((FAT_SUPER->root_max + SECTOR_SIZE - 1) >> 9);
  FAT_SUPER->num_clust = 
    2 + ((FAT_SUPER->num_sectors - FAT_SUPER->data_offset) 
	 / bpb.sects_per_clust);
  FAT_SUPER->sects_per_clust = bpb.sects_per_clust;
  
  if (!bpb.fat_length)
    {
      /* This is a FAT32 */
      if (FAT_CVT_U16(bpb.dir_entries))
 	return 0;
      
      if (bpb.flags & 0x0080)
	{
	  /* FAT mirroring is disabled, get active FAT */
	  int active_fat = bpb.flags & 0x000f;
	  if (active_fat >= bpb.num_fats)
	    return 0;
	  FAT_SUPER->fat_offset += active_fat * FAT_SUPER->fat_length;
	}
      
      FAT_SUPER->fat_size = 8;
      FAT_SUPER->root_cluster = bpb.root_cluster;

      /* Yes the following is correct.  FAT32 should be called FAT28 :) */
      FAT_SUPER->clust_eof_marker = 0xffffff8;
    } 
  else 
    {
      if (!FAT_SUPER->root_max)
 	return 0;
      
      FAT_SUPER->root_cluster = -1;
      if (FAT_SUPER->num_clust > FAT_MAX_12BIT_CLUST) 
	{
	  FAT_SUPER->fat_size = 4;
	  FAT_SUPER->clust_eof_marker = 0xfff8;
	} 
      else
	{
	  FAT_SUPER->fat_size = 3;
	  FAT_SUPER->clust_eof_marker = 0xff8;
	}
    }

  /* Now do some sanity checks */
  
  if (FAT_SUPER->num_clust <= 2
      || (FAT_SUPER->fat_size * FAT_SUPER->num_clust / (2 * SECTOR_SIZE)
 	  > FAT_SUPER->fat_length))
    return 0;
  
  /* kbs: Media check on first FAT entry [ported from PUPA] */

  if (!devread(FAT_SUPER->fat_offset, 0,
               sizeof(first_fat), (unsigned long long)(unsigned int)(char *)&first_fat, 0xedde0d90))
    return 0;

  if (FAT_SUPER->fat_size == 8)
    {
      first_fat &= 0x0fffffff;
      magic = 0x0fffff00;
    }
  else if (FAT_SUPER->fat_size == 4)
    {
      first_fat &= 0x0000ffff;
      magic = 0xff00;
    }
  else
    {
      first_fat &= 0x00000fff;
      magic = 0x0f00;
    }

  FAT_SUPER->cached_fat = - 2 * FAT_CACHE_SIZE;
  return 1;
}
#endif

#if 0
unsigned long
fat_read (unsigned long long buf, unsigned long long len, unsigned long write)
{
  unsigned long logical_clust;
  unsigned long offset;
  unsigned long ret = 0;
  unsigned long size;
  
  if (FAT_SUPER->file_cluster < 0)
    {
      /* root directory for fat16 */
      size = FAT_SUPER->root_max - filepos;
      if (size > len)
 	size = len;
      if (!devread(FAT_SUPER->root_offset, filepos, size, buf, 0xedde0d90))
 	return 0;
      filepos += size;
      return size;
    }
  
  logical_clust = (unsigned long)filepos / FAT_SUPER->clustsize;
  offset = ((unsigned long)filepos & (FAT_SUPER->clustsize - 1));
  if (logical_clust < FAT_SUPER->current_cluster_num)
    {
      FAT_SUPER->current_cluster_num = 0;
      FAT_SUPER->current_cluster = FAT_SUPER->file_cluster;
    }
  
  while (len > 0)
    {
      unsigned long sector;
      while (logical_clust > FAT_SUPER->current_cluster_num)
	{
	  /* calculate next cluster */
	  unsigned long fat_entry = 
	    FAT_SUPER->current_cluster * FAT_SUPER->fat_size;
	  unsigned long next_cluster;
	  unsigned long cached_pos = (fat_entry - FAT_SUPER->cached_fat);
	  
	  if (cached_pos < 0 || 
	      (cached_pos + FAT_SUPER->fat_size) > 2*FAT_CACHE_SIZE)
	    {
	      FAT_SUPER->cached_fat = (fat_entry & ~(2*SECTOR_SIZE - 1));
	      cached_pos = (fat_entry - FAT_SUPER->cached_fat);
	      sector = FAT_SUPER->fat_offset
		+ FAT_SUPER->cached_fat / (2*SECTOR_SIZE);
	      if (!devread (sector, 0, FAT_CACHE_SIZE, (unsigned long long)(unsigned int)(char*) FAT_BUF, 0xedde0d90))
		return 0;
	    }
	  next_cluster = * (unsigned long *) (FAT_BUF + (cached_pos >> 1));
	  if (FAT_SUPER->fat_size == 3)
	    {
	      if (cached_pos & 1)
		next_cluster >>= 4;
	      next_cluster &= 0xFFF;
	    }
	  else if (FAT_SUPER->fat_size == 4)
	    next_cluster &= 0xFFFF;
	  
	  if (next_cluster >= FAT_SUPER->clust_eof_marker)
	    return ret;
	  if (next_cluster < 2 || next_cluster >= FAT_SUPER->num_clust)
	    {
	      errnum = ERR_FSYS_CORRUPT;
	      return 0;
	    }
	  
	  FAT_SUPER->current_cluster = next_cluster;
	  FAT_SUPER->current_cluster_num++;
	}
      
      sector = FAT_SUPER->data_offset + ((FAT_SUPER->current_cluster - 2) * (FAT_SUPER->sects_per_clust));
      
      size = FAT_SUPER->clustsize - offset;
      
      if (size > len)
	  size = len;
      
      disk_read_func = disk_read_hook;
      
      devread(sector, offset, size, buf, write);
      
      disk_read_func = NULL;
      
      len -= size;	/* len always >= 0 */
      if (buf)
	buf += size;
      ret += size;
      filepos += size;
      logical_clust++;
      offset = 0;
    }
  return errnum ? 0 : ret;
}
#endif

#if 0
int
fat_dir (char *dirname)
{
  char *rest, ch, dir_buf[FAT_DIRENTRY_LENGTH];
  unsigned short *filename = (unsigned short *) NAME_BUF; /* unicode */
  unsigned char *utf8 = (unsigned char *) UTF8_BUF; /* utf8 filename */
  int attrib = FAT_ATTRIB_DIR;
  
  /* XXX I18N:
   * the positions 2,4,6 etc are high bytes of a 16 bit unicode char 
   */
  static unsigned char longdir_pos[] = 
  { 1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30 };
  int slot = -2;
  int alias_checksum = -1;
  
  FAT_SUPER->file_cluster = FAT_SUPER->root_cluster;
  
  /* main loop to find desired directory entry */
 loop:
  filepos = 0;
  FAT_SUPER->current_cluster_num = 0xFFFFFFFF;
  
  /* if we have a real file (and we're not just printing possibilities),
     then this is where we want to exit */
  
  if (!*dirname || *dirname == ' ' || *dirname == '\t')
    {
      if (attrib & FAT_ATTRIB_DIR)
	{
	  errnum = ERR_BAD_FILETYPE;
	  return 0;
	}
      
      return 1;
    }
  
  /* continue with the file/directory name interpretation */
  
  /* skip over slashes */
  while (*dirname == '/')
    dirname++;
  
  if (!(attrib & FAT_ATTRIB_DIR))
    {
      errnum = ERR_BAD_FILETYPE;
      return 0;
    }
  /* Directories don't have a file size */
  filemax = 0xFFFFFFFF;
  
  /* check if the dirname ends in a slash(saved in CH) and end it in a NULL */
  //for (rest = dirname; (ch = *rest) && !isspace (ch) && ch != '/'; rest++);
  for (rest = dirname; (ch = *rest) && ch != ' ' && ch != '\t' && ch != '/'; rest++)
  {
	if (ch == '\\')
	{
		rest++;
		if (! (ch = *rest))
			break;
	}
  }
  
  *rest = 0;
  
  while (1)
    {
      /* read the dir entry */
      if (fat_read ((unsigned long long)(unsigned int)dir_buf, FAT_DIRENTRY_LENGTH, 0xedde0d90) != FAT_DIRENTRY_LENGTH
		/* read failure */
	  || dir_buf[0] == 0 /* end of dir entry */)
	{
	  if (errnum == 0)
	    {
# ifndef STAGE1_5
	      if (print_possibilities < 0)
		{
		  /* previously succeeded, so return success */
		  *rest = ch;	/* XXX: Should restore the byte? */
		  return 1;
		}
# endif /* STAGE1_5 */
	      
	      errnum = ERR_FILE_NOT_FOUND;
	    }
	  
	  *rest = ch;
	  return 0;
	}
      
      if (FAT_DIRENTRY_ATTRIB (dir_buf) == FAT_ATTRIB_LONGNAME)
	{
	  /* This is a long filename.  The filename is build from back
	   * to front and may span multiple entries.  To bind these
	   * entries together they all contain the same checksum over
	   * the short alias.
	   *
	   * The id field tells if this is the first entry (the last
	   * part) of the long filename, and also at which offset this
	   * belongs.
	   *
	   * We just write the part of the long filename this entry
	   * describes and continue with the next dir entry.
	   */
	  int i, offset;
	  unsigned char id = FAT_LONGDIR_ID(dir_buf);
	  
	  if ((id & 0x40)) 
	    {
	      id &= 0x3f;
	      slot = id;
	      filename[slot * 13] = 0;
	      alias_checksum = FAT_LONGDIR_ALIASCHECKSUM(dir_buf);
	    } 
	  
	  if (id != slot || slot == 0
	      || alias_checksum != FAT_LONGDIR_ALIASCHECKSUM(dir_buf))
	    {
	      alias_checksum = -1;
	      continue;
	    }
	  
	  slot--;
	  offset = slot * 13;
	  
	  for (i=0; i < 13; i++)
	    filename[offset+i] = *(unsigned short *)(dir_buf+longdir_pos[i]);
	  continue;
	}
      
      if (!FAT_DIRENTRY_VALID (dir_buf))
	continue;
      
      if (alias_checksum != -1 && slot == 0)
	{
	  int i;
	  unsigned char sum;
	  
	  slot = -2;
	  for (sum = 0, i = 0; i< 11; i++)
	    sum = ((sum >> 1) | (sum << 7)) + dir_buf[i];
	  
	  if (sum == alias_checksum)
	    {
	      goto valid_filename;
	    }
	}
      
      /* XXX convert to 8.3 filename format here */
      {
	int i, j, c;
	
	for (i = 0; i < 8 && (c = filename[i] = tolower (dir_buf[i]))
	       && /*!isspace (c)*/ c != ' '; i++);
	
	filename[i++] = '.';
	
	for (j = 0; j < 3 && (c = filename[i + j] = tolower (dir_buf[8 + j]))
	       && /*!isspace (c)*/ c != ' '; j++);
	
	if (j == 0)
	  i--;
	
	filename[i + j] = 0;
      }
      
valid_filename:
      unicode_to_utf8 (filename, utf8, 832);
# ifndef STAGE1_5
      if (print_possibilities && ch != '/')
	{
//	print_filename:
	  if (substring (dirname, (char *)utf8, 1) <= 0)
	    {
	      if (print_possibilities > 0)
		print_possibilities = -print_possibilities;
	      print_a_completion ((char *)utf8, 1);
	    }
	  continue;
	}
# endif /* STAGE1_5 */
      
      if (substring (dirname, (char *)utf8, 1) == 0)
	break;
    }
  
  *(dirname = rest) = ch;
  
  attrib = FAT_DIRENTRY_ATTRIB (dir_buf);
  filemax = FAT_DIRENTRY_FILELENGTH (dir_buf);
  FAT_SUPER->file_cluster = FAT_DIRENTRY_FIRST_CLUSTER (dir_buf);
  
  /* go back to main loop at top of function */
  goto loop;
}
#endif

