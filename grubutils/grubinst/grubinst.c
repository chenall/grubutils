/*
 *  GRUB Utilities --  Utilities for GRUB Legacy, GRUB2 and GRUB for DOS
 *  Copyright (C) 2007 Bean (bean123ch@gmail.com)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * +2 patch by Steve6375
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#if defined(WIN32)
#include <windows.h>
#include <winioctl.h>
#endif
#ifdef LINUX
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <linux/hdreg.h>
#include <sys/io.h>
#endif

#include "grub_mbr45cp.h"
#include "grub_mbr46ap.h"
#include "grub_pbr46al.h"
#include "utils.h"
#include "version.h"
#include "keytab.h"
#include "xdio.h"

#ifdef WIN32
//#define STORE_RMB_STATUS    1
#endif

// Application flags, used by this program

#define AFG_VERBOSE            1
#define AFG_PAUSE              2
#define AFG_READ_ONLY          4
#define AFG_NO_BACKUP_MBR      8
//#define AFG_FORCE_BACKUP_MBR	16
#define AFG_RESTORE_PREVMBR    32
#define AFG_LIST_PART          64
#define AFG_IS_FLOPPY          128
#define AFG_LBA_MODE           256
#define AFG_CHS_MODE           512
#define AFG_OUTPUT             1024
#define AFG_EDIT               2048
#define AFG_SKIP_MBR_TEST      4096
#define AFG_COPY_BPB           8192
#define AFG_SILENT_BOOT        16384
#define AFG_USE_NEW_VERSION    32768
#define AFG_LOCK               65536
#define AFG_UNMOUNT            131072

// Grldr flags, this flag is used by grldr.mbr

#define GFG_DISABLE_FLOPPY    1
#define GFG_DISABLE_OSBR      2
#define GFG_DUCE              4
#define CFG_CHS_NO_TUNE       8         /*--chs-no-tune add by chenall* 2008-12-15*/
#define GFG_PREVMBR_LAST      128

#ifdef STORE_RMB_STATUS
#define GFG_IS_REMOVABLE      16
#define GFG_IS_FIXED          32
#endif

#define APP_NAME              "grubinst: "

#define print_pause           if (afg & AFG_PAUSE) { fputs("Press <ENTER> to continue ...\n", stderr); fflush(stderr); fgetc(stdin); }

#define print_apperr(a)    { fprintf(stderr, APP_NAME "%s\n", a); print_pause; }
#define print_syserr(a)    { perror(APP_NAME a); print_pause; }

#ifdef WIN32
typedef struct _STORAGE_DEVICE_NUMBER
{
  DEVICE_TYPE DeviceType;
  ULONG       DeviceNumber;
  ULONG       PartitionNumber;
} STORAGE_DEVICE_NUMBER, *PSTORAGE_DEVICE_NUMBER;
typedef enum _STORAGE_BUS_TYPE_OVERRIDE
{
  BusTypeUnknownOverride           = 0x00,
  BusTypeScsiOverride              = 0x1,
  BusTypeAtapiOverride             = 0x2,
  BusTypeAtaOverride               = 0x3,
  BusType1394Override              = 0x4,
  BusTypeSsaOverride               = 0x5,
  BusTypeFibreOverride             = 0x6,
  BusTypeUsbOverride               = 0x7,
  BusTypeRAIDOverride              = 0x8,
  BusTypeiScsiOverride             = 0x9,
  BusTypeSasOverride               = 0xA,
  BusTypeSataOverride              = 0xB,
  BusTypeSdOverride                = 0xC,
  BusTypeMmcOverride               = 0xD,
  BusTypeVirtualOverride           = 0xE,
  BusTypeFileBackedVirtualOverride = 0xF,
  BusTypeSpacesOverride            = 0x10,
  BusTypeMaxOverride               = 0x11,
  BusTypeMaxReservedOverride       = 0x7F
} STORAGE_BUS_TYPE_OVERRIDE, *PSTORAGE_BUS_TYPE_OVERRIDE;
typedef struct _STORAGE_DEVICE_DESCRIPTOR
{
  ULONG                     Version;
  ULONG                     Size;
  UCHAR                     DeviceType;
  UCHAR                     DeviceTypeModifier;
  BOOLEAN                   RemovableMedia;
  BOOLEAN                   CommandQueueing;
  ULONG                     VendorIdOffset;
  ULONG                     ProductIdOffset;
  ULONG                     ProductRevisionOffset;
  ULONG                     SerialNumberOffset;
  STORAGE_BUS_TYPE_OVERRIDE BusType;
  ULONG                     RawPropertiesLength;
  UCHAR                     RawDeviceProperties[1];
} STORAGE_DEVICE_DESCRIPTOR, *PSTORAGE_DEVICE_DESCRIPTOR;
typedef enum _STORAGE_PROPERTY_ID
{
  StorageDeviceProperty = 0,
  StorageAdapterProperty,
  StorageDeviceIdProperty
} STORAGE_PROPERTY_ID;
typedef enum _STORAGE_QUERY_TYPE
{
  PropertyStandardQuery = 0,
  PropertyExistsQuery,
  PropertyMaskQuery,
  PropertyQueryMaxDefined
} STORAGE_QUERY_TYPE;
typedef struct _STORAGE_PROPERTY_QUERY
{
  STORAGE_PROPERTY_ID PropertyId;
  STORAGE_QUERY_TYPE  QueryType;
  UCHAR               AdditionalParameters[1];
} STORAGE_PROPERTY_QUERY;
#define IOCTL_STORAGE_QUERY_PROPERTY    CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

void help(void)
{
  fputs("Usage:\n"
        "\tgrubinst  [OPTIONS]  DEVICE_OR_FILE\n\n"
        "OPTIONS:\n\n"
        "\t--help,-h\t\tShow usage information\n\n"
        "\t--pause\t\t\tPause before exiting\n\n"
        "\t--version\t\tShow version information\n\n"
        "\t--verbose,-v\t\tVerbose output\n\n"
        "\t--list-part,-l\t\tList all logical partitions in DEVICE_OR_FILE\n\n"
        "\t--save=FN,-s=FN\t\tSave the original/Copy the backup\n\t\t\t\tMBR/BS/PBR to FN\n\n"
        "\t--restore=FN,-r=FN\tRestore MBR/PBR/BS from previously saved FN\n\n"
        "\t--restore-prevmbr,-r\tRestore previous MBR saved in the second sector\n"
        "\t\t\t\tof DEVICE_OR_FILE\n\n"
        "\t--read-only,-t\t\tdo everything except the actual write to the\n"
        "\t\t\t\tspecified DEVICE_OR_FILE. (test mode)\n\n"
        "\t--no-backup-mbr\t\tdo not copy the old MBR to the second sector of\n"
        "\t\t\t\tDEVICE_OR_FILE.\n\n"
        "\t--force-backup-mbr\tforce the copy of old MBR to the second sector\n"
        "\t\t\t\tof DEVICE_OR_FILE.(default)\n\n"
        "\t--mbr-enable-floppy\tenable the search for GRLDR on floppy.(default)\n\n"
        "\t--mbr-disable-floppy\tdisable the search for GRLDR on floppy.\n\n"
        "\t--mbr-enable-osbr\tenable the boot of PREVIOUS MBR with invalid\n"
        "\t\t\t\tpartition table (usually an OS boot sector).\n"
        "\t\t\t\t(default)\n\n"
        "\t--mbr-disable-osbr\tdisable the boot of PREVIOUS MBR with invalid\n"
        "\t\t\t\tpartition table (usually an OS boot sector).\n\n"
        "\t--duce\t\t\tdisable the feature of unconditional entrance\n"
        "\t\t\t\tto the command-line.\n\n"
        "\t--boot-prevmbr-first\ttry to boot PREVIOUS MBR before the search for\n"
        "\t\t\t\tGRLDR.\n\n"
        "\t--boot-prevmbr-last\ttry to boot PREVIOUS MBR after the search for\n"
        "\t\t\t\tGRLDR.(default)\n\n"
        "\t--preferred-drive=D\tpreferred boot drive number, 0 <= D < 255.\n\n"
        "\t--preferred-partition=P\tpreferred partition number, 0 <= P < 255.\n\n"
        "\t--time-out=T,-t=T\twait T seconds before booting PREVIOUS MBR. if\n"
        "\t\t\t\tT is 0xff, wait forever. The default is 5.\n\n"
        "\t--hot-key=K,-k=K\tif the desired key K is pressed, start GRUB\n"
        "\t\t\t\tbefore booting PREVIOUS MBR. K is a word\n"
        "\t\t\t\tvalue, just as the value in AX register\n"
        "\t\t\t\treturned from int16/AH=1. The high byte is the\n"
        "\t\t\t\tscan code and the low byte is ASCII code. The\n"
        "\t\t\t\tdefault is 0x3920 for space bar.\n\n"
        "\t--key-name=S\t\tSpecify the name of the hot key.\n\n"
        "\t--floppy,-f\t\tif DEVICE_OR_FILE is floppy, use this option.\n\n"
        "\t--floppy=N\t\tif DEVICE_OR_FILE is a partition on a hard\n"
        "\t\t\t\tdrive, use this option. N is used to specify\n"
        "\t\t\t\tthe partition number: 0,1,2 and 3 for the\n"
        "\t\t\t\tprimary partitions, and 4,5,6,... for the\n"
        "\t\t\t\tlogical partitions.\n\n"
        "\t--sectors-per-track=S\tspecifies sectors per track for --floppy.\n"
        "\t\t\t\t1 <= S <= 63, default is 63.\n\n"
        "\t--heads=H\t\tspecifies number of heads for --floppy.\n"
        "\t\t\t\t1 <= H <= 256, default is 255.\n\n"
        "\t--start-sector=B\tspecifies hidden sectors for --floppy=N.\n\n"
        "\t--total-sectors=C\tspecifies total sectors for --floppy.\n"
        "\t\t\t\tdefault is 0.\n\n"
        "\t--lba\t\t\tuse lba mode for --floppy. If the floppy BIOS\n"
        "\t\t\t\thas LBA support, you can specify --lba here.\n"
        "\t\t\t\tIt is assumed that all floppy BIOSes have CHS\n"
        "\t\t\t\tsupport. So you would rather specify --chs.\n"
        "\t\t\t\tIf neither --chs nor --lba is specified, then\n"
        "\t\t\t\tthe LBA indicator(i.e., the third byte of the\n"
        "\t\t\t\tboot sector) will not be touched.\n\n"
        "\t--chs\t\t\tuse chs mode for --floppy. You should specify\n"
        "\t\t\t\t--chs if the floppy BIOS does not support LBA.\n"
        "\t\t\t\tWe assume all floppy BIOSes have CHS support.\n"
        "\t\t\t\tSo it is likely you want to specify --chs.\n"
        "\t\t\t\tIf neither --chs nor --lba is specified, then\n"
        "\t\t\t\tthe LBA indicator(i.e., the third byte of the\n"
        "\t\t\t\tboot sector) will not be touched.\n\n"
        "\t--install-partition=I\tInstall the boot record onto the boot area of\n"
        "\t-p=I\t\t\tpartition number I of the specified hard drive\n"
        "\t\t\t\tor harddrive image DEVICE_OR_FILE.\n\n"
        "\t--boot-file=F,-b=F\tChange the name of boot file.\n\n"
        "\t--load-seg=S\t\tChange load segment for boot file.\n\n"
        "\t--grub2,-2\t\tLoad grub2 kernel g2ldr instead of grldr.\n\n"
        "\t--output,-o\t\tSave embedded grldr.mbr to DEVICE_OR_FILE.\n\n"
        "\t--edit,-e\t\tEdit external grldr/grldr.mbr.\n\n"
        "\t--skip-mbr-test\t\tSkip chs validity test in mbr.\n\n"
        "\t--copy-bpb\t\tCopy bpb of the first partition to mbr.\n\n"
        "\t--chs-no-tune\t\tdisable the feature of geometry tune.\n\n"/*--chs-no-tune add by chenall* 2008-12-15*/
        "\t--silent-boot\t\tIt will display messages only on\n\t\t\t\tsevere change(s) and/or severe error(s).\n\n"
        "\t--file-system-type=fst\tIt will autodetect or set the file system type.\n\t\t\t\tValid values: AutoDetect (default), MBR, GPT,\n\t\t\t\tNTFS, exFAT, FAT32, FAT16, EXT2, EXT3 and EXT4\n\n"
        #ifdef WIN32
        "\t--lock\t\t\tLock DEVICE_OR_FILE before making\n\t\t\t\tany modifications\n\n"
        "\t--unmount\t\tUnmount DEVICE_OR_FILE before making\n\t\t\t\tany modifications\n\n"
        #endif
        "\t--g4d-version=ver\tSets the Grub4Dos MBR/BS/PBR version.\n\t\t\t\tValid values: 0.4.5c (default) or 0.4.6a\n"
        #ifdef STORE_RMB_STATUS
        "\n\t--rmb-status=st\t\tStore the removable bit status in the Grub4dos\n\t\t\t\tMBR control byte. Valid values: auto, on or off\n"
        #endif
        ,
        stderr);
}

unsigned char  grub_mbr[0x3000];
int            gfg, def_drive, def_part, time_out, hot_key, part_num;
unsigned long  part_stadd;
long           afg;
int            def_spt, def_hds, def_ssc, def_tsc;
char           *save_fn, *restore_fn, *boot_file, boot_file_83[12], *key_name, *fstype;
#ifdef STORE_RMB_STATUS
char           *rmbs;
#endif
unsigned short load_seg;
int            drheads = 255, drspt = 63, drbps = 512;

char* str_lowcase(char* str)
{
  int i;

  for (i = 0; str[i]; i++)
    if ((str[i] >= 'A') && (str[i] <= 'Z'))
      str[i] += 'a' - 'A';

  return str;
}

int SetBootFile(char* fn)
{
  char* pc;

  if (*fn == 0)
    return 1;
  pc = strchr(fn, '.');
  if (pc)
  {
    if ((pc == fn) || (pc - fn > 8) || (strlen(pc + 1) > 3))
      return 1;
  }
  else
  {
    if (strlen(fn) > 8)
      return 1;
  }
  str_upcase(fn);
  memset(boot_file_83, ' ', sizeof(boot_file_83) - 1);
  if (pc)
  {
    memcpy(boot_file_83, fn, pc - fn);
    memcpy(&boot_file_83[8], pc + 1, strlen(pc + 1));
  }
  else
    memcpy(boot_file_83, fn, strlen(fn));
  str_lowcase(fn);
  boot_file = fn;
  return 0;
}

int chk_mbr(unsigned char* buf);

void list(xd_t *xd)
{
  xde_t xe;

  xe.cur = xe.nxt = 0xFF;
  fprintf(stderr,
          "Partition list\n"
          " #  id      base      leng\n");
  while (!xd_enum(xd, &xe))
    fprintf(stderr, "%2d  %02X  %8lX  %8lX (%uM)\n", xe.cur, xe.dfs, xe.bse, xe.len, (unsigned int) (xe.len + 1024) >> 11);
}

void print_mbr(char* buf)
{
  int i, j;

  fprintf(stderr,
          "Partition table\n"
          "  bt  h0  s0  c0  fs  h1  s1  c1      base      leng\n");
  for (i = 0x1BE; i < 0x1FE; i += 16)
  {
    for (j = 0; j < 8; j++)
      fprintf(stderr, "  %02X", (unsigned char) buf[i + j]);
    fprintf(stderr, "  %8lX  %8lX\n", valueat(buf[i], 8, unsigned long), valueat(buf[i], 12, unsigned long));
  }
}

int is_grldr_mbr(char* buf)
{
  int i;

  for (i = 0x170; i < 0x1F0; i++)
    if (!memcmp(&buf[i], "\r\nMissing ", 10))
    {
      if (!memcmp(&buf[i + 10], "helper.", 7))
        return 2;
      if (!memcmp(&buf[i + 10], "MBR-helper.", 11))
        return 1;
    }
  return 0;
}

int install(char* fn)
{
  xd_t          * xd;
  int           hd, nn, fs, slen, mbr_size;
  char          prev_mbr[sizeof(grub_mbr)];
  unsigned long ssec, tsec;
  int           sv_res_len;
  int           str_sv = 0;

  if (fn == NULL)
    return 1;

  if (afg & AFG_USE_NEW_VERSION)
    mbr_size = sizeof(grub_mbr46ap);
  else
    mbr_size = sizeof(grub_mbr45cp);

  if (afg & AFG_OUTPUT)
  {
    if (!(afg & AFG_READ_ONLY))
    {
      int mode;

      mode = (!(afg & AFG_READ_ONLY)) ? (O_TRUNC | O_CREAT) : 0;
      if (!(afg & AFG_EDIT))
      {
        if (afg & AFG_VERBOSE)
          fprintf(stderr, "Extract mode\n");
        hd = open(fn, O_RDWR | O_BINARY | mode, 0644);
        if (hd == -1)
        {
          print_syserr("open");
          return errno;
        }
      }
      if (!(afg & AFG_READ_ONLY))
        if (write(hd, grub_mbr, mbr_size) != mbr_size)
        {
          print_apperr("Write to output file fails");
          close(hd);
          return 1;
        }
      close(hd);
    }
    goto quit;
  }

  if (afg & AFG_SILENT_BOOT)
  {
    if (afg & AFG_USE_NEW_VERSION)
    {
      memset(&grub_mbr[0x5E0], 0, 2);
      memset(&grub_mbr[0x1040], 0, 2);
      memset(&grub_pbr46al[0x1E0], 0, 2);
      memset(&grub_pbr46al[0x3E0], 0, 2);
      memset(&grub_pbr46al[0x5E0], 0, 2);
      memset(&grub_pbr46al[0x9E0], 0, 2);
      memset(&grub_pbr46al[0xDE0], 0, 2);
      memset(&grub_mbr[0x1BC1], 0, 66);
      memset(&grub_mbr[0x1CAD], 0, 12);
      memset(&grub_mbr[0x1C42], 0, 106);
      memset(&grub_mbr[0x1F74], 0, 32);
    }
    else
    {
      memset(&grub_mbr[0xA00 + (valueat(grub_mbr, 0xA00 + 0x1EC, unsigned short) & 0x7FF) - 3], 0, 2);
      memset(&grub_mbr[0x400 + (valueat(grub_mbr, 0x400 + 0x1EC, unsigned short) & 0x7FF) - 3], 0, 2);
      memset(&grub_mbr[0x600 + (valueat(grub_mbr, 0x600 + 0x1EC, unsigned short) & 0x7FF) - 3], 0, 2);
      memset(&grub_mbr[0x800 + (valueat(grub_mbr, 0x800 + 0x1EE, unsigned short) & 0x7FF) - 3], 0, 2);
      memset(&grub_mbr[0x1EA7], 0, 74);
      memset(&grub_mbr[0x1F30], 0, 98);
      memset(&grub_mbr[0x22D2], 0, 32);
    }
  }


  if (afg & AFG_EDIT)
  {
    unsigned short r1, r2;

    if (afg & AFG_VERBOSE)
      fprintf(stderr, "Edit mode\n");
    hd = open(fn, O_RDWR | O_BINARY, 0644);
    if (hd == -1)
    {
      print_syserr("open");
      return errno;
    }
//      r1=valueat(grub_mbr[0x1FFA],0,unsigned short);
    r1 = valueat(grub_mbr[0x23FA], 0, unsigned short);
//注:这里不改变也是可以的,
//新版本GRLDR.MBR版本标志位置在0x23fa,0X1FFA也是,后面修改GRLDR部份的位置不变 by chenall
    nn = read(hd, grub_mbr, mbr_size);
    if (nn == -1)
    {
      print_syserr("read");
      close(hd);
      return errno;
    }
    if (nn < mbr_size)
    {
      print_apperr("The input file is too short");
      close(hd);
      return 1;
    }
    if (valueat(grub_mbr[0x1FFC], 0, unsigned long) != 0xAA555247)
    {
      print_apperr("Invalid input file");
      close(hd);
      return 1;
    }
    r2 = valueat(grub_mbr[0x1FFA], 0, unsigned short);
    if (r1 != r2)
    {
      char buf[80];

      sprintf(buf, "Version number mismatched (old=%d new=%d)", r2, r1);
      print_apperr(buf);
      close(hd);
      return 1;
    }
    lseek(hd, 0, SEEK_SET);
    afg |= AFG_OUTPUT;
  }

  memset(&grub_mbr[512], 0, 512);
  if ((key_name == NULL) && (hot_key == 0x3920))
    key_name = "SPACE";

  if ((afg & AFG_LOCK) && (part_num == -1))
  {
    xd = xd_open(fn, (!(afg & AFG_READ_ONLY)), 1);
    if (xd == NULL)
    {
      print_syserr("open");
      return 1;
    }
  }
  else
  {
    xd = xd_open(fn, (!(afg & AFG_READ_ONLY)), 0);
    if (xd == NULL)
    {
      print_syserr("open");
      return 1;
    }
  }

  #if defined(WIN32)
  if (xd->flg & XDF_DISK)
  {
    DISK_GEOMETRY pdg;
    DWORD         dwBytesReturned = 0;

    if (DeviceIoControl((HANDLE) xd->num, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &pdg, sizeof(DISK_GEOMETRY), &dwBytesReturned, (LPOVERLAPPED) NULL))
    {
      drheads = pdg.TracksPerCylinder;
      drspt   = pdg.SectorsPerTrack;
      drbps   = pdg.BytesPerSector;
    }
  }
  #endif

  #ifdef LINUX
  struct hd_geometry geometry;
  memset(&geometry, 0, sizeof geometry);
  if (ioctl(xd->num, HDIO_GETGEO, &geometry) >= 0)
  {
    drheads = geometry.heads;
    drspt   = geometry.sectors;
  }
  ioctl(xd->num, BLKSSZGET, &drbps);
  #endif

  #ifdef STORE_RMB_STATUS
  if ((xd->flg & XDF_DISK) && (rmbs))
  {
    if (!strcmp(rmbs, "auto"))
    {
      DWORD                     outsize;
      STORAGE_PROPERTY_QUERY    spq;
      STORAGE_DEVICE_DESCRIPTOR sdd = { 0 };
      spq.PropertyId = StorageDeviceProperty;
      spq.QueryType  = PropertyStandardQuery;
      memset(&sdd, 0, sizeof(sdd));
      if (DeviceIoControl((HANDLE) xd->num, IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(STORAGE_PROPERTY_QUERY), &sdd, sizeof(STORAGE_DEVICE_DESCRIPTOR), &outsize, NULL))
      {
		if (sdd.RemovableMedia)
		{
          if (afg & AFG_VERBOSE)
		    fputs("The disk is removable\n", stderr);
		  gfg |= GFG_IS_REMOVABLE;
		}
        else
		{
          if (afg & AFG_VERBOSE)
		    fputs("The disk is fixed\n", stderr);			
          gfg |= GFG_IS_FIXED;			
		}
      }
    }
    else
    if (!strcmp(rmbs, "on"))
      gfg |= GFG_IS_REMOVABLE;
    else
    if (!strcmp(rmbs, "off"))
      gfg |= GFG_IS_FIXED;
  }
  #endif
  
  if (afg & AFG_USE_NEW_VERSION)
  {
    valueat(grub_mbr, 0x5A, unsigned char)  = gfg;
    valueat(grub_mbr, 0x5B, unsigned char)  = time_out;
    valueat(grub_mbr, 0x5C, unsigned short) = hot_key;
    valueat(grub_mbr, 0x5E, unsigned char)  = def_drive;
    valueat(grub_mbr, 0x5F, unsigned char)  = def_part;
    if (key_name)
      strcpy((char *) &grub_mbr[0x1CBA], key_name);
  }
  else
  {
    valueat(grub_mbr, 2 + 2, unsigned char)  = gfg;
    valueat(grub_mbr, 3 + 2, unsigned char)  = time_out;
    valueat(grub_mbr, 4 + 2, unsigned short) = hot_key;
    valueat(grub_mbr, 6 + 2, unsigned char)  = def_drive;
    valueat(grub_mbr, 7 + 2, unsigned char)  = def_part;
    if (key_name)
      strcpy((char *) &grub_mbr[0x1FE8], key_name);
  }  

  if (afg & AFG_LIST_PART)
  {
    list(xd);
    xd_close(xd);
    return 0;
  }
  if (part_num != -1)
  {
    if ((def_ssc != -1) && (def_tsc != -1))
    {
      ssec = def_ssc;
      tsec = def_tsc;
    }
    else
    {
      xde_t xe;

      xe.cur = 0xFF;
      xe.nxt = part_num;
      if (xd_enum(xd, &xe))
      {
        print_apperr("Partition not found");
        xd_close(xd);
        return 1;
      }
      if (def_ssc == -1)
        ssec = xe.bse;
      if (def_tsc == -1)
        tsec = (unsigned long) xe.len / drbps;
      if (afg & AFG_VERBOSE)
        fprintf(stderr, "Part Fs: %02X (%s)\nPart Leng: %lu\n", xe.dfs, dfs2str(xe.dfs), xe.len);
    }
  }
  else
  {
    ssec = xd->ofs;
    // tsec=(unsigned long)xd->len / drbps;
    if (ssec)
      part_num = 0;
  }
  if (afg & AFG_VERBOSE)
    fprintf(stderr, "Start sector: 0x%lX\n", ssec);
  if ((ssec != xd->ofs) && (xd_seek(xd, ssec)))
  {
    print_apperr("Can\'t seek to the start sector");
    xd_close(xd);
    return 1;
  }
  if (xd_read(xd, prev_mbr, sizeof(prev_mbr) >> 9))
  {
    print_apperr("Read error");
    xd_close(xd);
    return 1;
  }

  if (fstype == NULL)
    fs = get_fstype((unsigned char *) prev_mbr);
  else
  if (!strcmp(fstype, "MBR"))
    fs = FST_MBR;
  else
  if (!strcmp(fstype, "GPT"))
    fs = FST_GPT;
  else
  if (!strcmp(fstype, "NTFS"))
    fs = FST_NTFS;
  else
  if (!strcmp(fstype, "exFAT"))
    fs = FST_EXFAT;
  else
  if (!strcmp(fstype, "FAT32"))
    fs = FST_FAT32;
  else
  if (!strcmp(fstype, "FAT16"))
    fs = FST_FAT16;
  else
  if (!strcmp(fstype, "EXT2"))
    fs = FST_EXT2;
  else
  if (!strcmp(fstype, "EXT3"))
    fs = FST_EXT3;
  else
  if (!strcmp(fstype, "EXT4"))
    fs = FST_EXT4;
  else
    fs = get_fstype((unsigned char *) prev_mbr);

  if (afg & AFG_VERBOSE)
  {
    fprintf(stderr, "Image type: %s\n", fst2str(fs));
    if (fs == FST_MBR)
    {
      fprintf(stderr, "Num of heads: %d\nSectors per track: %d\n", mbr_nhd, mbr_spt);
    }
    if ((fs == FST_MBR) || (fs == FST_MBR_FORCE_LBA) || (fs == FST_MBR_BAD))
      print_mbr(prev_mbr);
  }
  if (fs == FST_MBR_FORCE_LBA)
    fs = FST_MBR;
  if (fs == FST_MBR_BAD)
  {
    if (afg & AFG_SKIP_MBR_TEST)
      fs = FST_MBR;
    else
    {
      if ((afg & AFG_VERBOSE) == 0)
        print_mbr(prev_mbr);
      list(xd);
      print_apperr("Bad partition table, if you're sure that the partition list is ok, please run this program again with --skip-mbr-test option.");
      xd_close(xd);
      return 1;
    }
  }
  if (fs == FST_OTHER)
  {
    print_apperr("Unknown image type");
    xd_close(xd);
    return 1;
  }
  if (((part_num != -1) || (afg & AFG_IS_FLOPPY)) && ((fs == FST_MBR) || (fs == FST_GPT)))
  {
    print_apperr("Should be a file system image");
    xd_close(xd);
    return 1;
  }
  if ((part_num == -1) && ((afg & AFG_IS_FLOPPY) == 0) && ((fs != FST_MBR) && (fs != FST_GPT)))
  {
    print_apperr("Should be a disk image");
    xd_close(xd);
    return 1;
  }

  if (boot_file)
  {
    unsigned short ofs, len, len1;

    len = strlen(boot_file);

    if (afg & AFG_USE_NEW_VERSION)
    {
      char buf[64];

      strcpy(buf, boot_file);
      str_upcase(buf);
      boot_file = buf;
      switch (fs)
      {
      case FST_MBR:
      {
        if (len > 12)
        {
          char buf[80];

          sprintf(buf, "Boot file name too long (%d>12)", len);
          print_apperr(buf);
          close(hd);
          return 1;
        }
        memset(&grub_mbr[0x5E3], 0, 12);
        strcpy((char *) &grub_mbr[0x5E3], boot_file);
        break;
      }
      case FST_FAT32:
      {
        strcpy((char *) &grub_pbr46al[0x1E3], boot_file_83);
        break;
      }
      case FST_FAT16:
      {
        strcpy((char *) &grub_pbr46al[0x3E3], boot_file_83);
        break;
      }
      case FST_EXT2:
      {
        if (len > 11)
        {
          char buf[80];

          sprintf(buf, "Boot file name too long for ext2 partition (%d>11)", len);
          print_apperr(buf);
          close(hd);
          return 1;
        }
        memset(&grub_pbr46al[0x5E3], 0, 12);
        strcpy((char *) &grub_pbr46al[0x5E3], boot_file);
        break;
      }
      case FST_EXT3:
      {
        if (len > 11)
        {
          char buf[80];

          sprintf(buf, "Boot file name too long for ext3 partition (%d>11)", len);
          print_apperr(buf);
          close(hd);
          return 1;
        }
        memset(&grub_pbr46al[0x5E3], 0, 12);
        strcpy((char *) &grub_pbr46al[0x5E3], boot_file);
        break;
      }
      case FST_EXT4:
      {
        if (len > 11)
        {
          char buf[80];

          sprintf(buf, "Boot file name too long for ext4 partition (%d>11)", len);
          print_apperr(buf);
          close(hd);
          return 1;
        }
        memset(&grub_pbr46al[0x5E3], 0, 12);
        strcpy((char *) &grub_pbr46al[0x5E3], boot_file);
        break;
      }
      case FST_EXFAT:
      {
        if (len > 12)
        {
          char buf[80];

          sprintf(buf, "Boot file name too long (%d>12)", len);
          print_apperr(buf);
          close(hd);
          return 1;
        }
        memset(&grub_pbr46al[0x9E3], 0, 12);
        strcpy((char *) &grub_pbr46al[0x9E3], boot_file);
        break;
      }
      case FST_NTFS:
      {
        if (len > 25)
        {
          char buf[80];

          sprintf(buf, "Boot file name too long for ntfs partition (%d>25)", len);
          print_apperr(buf);
          close(hd);
          return 1;
        }
        memset(&grub_pbr46al[0xDE3], 0, 12);
        strcpy((char *) &grub_pbr46al[0xDE3], boot_file);
        break;
      }
      }
    }
    else
    {
      // Patching the FAT32 boot sector
      if ((fs == FST_MBR) || (fs == FST_FAT32))
      {
        ofs = valueat(grub_mbr, 0x400 + 0x1EC, unsigned short) & 0x7FF;
        strcpy((char *) &grub_mbr[0x400 + ofs], boot_file_83);
        if (load_seg)
          valueat(grub_mbr, 0x400 + 0x1EA, unsigned short) = load_seg;
      }

      // Patching the FAT12/FAT16 boot sector
      if ((fs == FST_MBR) || (fs == FST_FAT16))
      {
        ofs = valueat(grub_mbr, 0x600 + 0x1EC, unsigned short) & 0x7FF;
        strcpy((char *) &grub_mbr[0x600 + ofs], boot_file_83);
        if (load_seg)
          valueat(grub_mbr, 0x600 + 0x1EA, unsigned short) = load_seg;
      }

      // Patching the EXT2/3/4 boot sector
      if ((fs == FST_MBR) || (fs == FST_EXT2) || (fs == FST_EXT3) || (fs == FST_EXT4))
      {
        ofs  = valueat(grub_mbr, 0x800 + 0x1EE, unsigned short) & 0x7FF;
        len1 = valueat(grub_mbr, 0x800 + 0x1EE, unsigned short) >> 11;
        if (len > len1)
        {
          char buf[80];

          sprintf(buf, "Boot file name too long for ext2/3/4 partition (%d>%d)", len, len1);
          print_apperr(buf);
          close(hd);
          return 1;
        }
        else
          strcpy((char *) &grub_mbr[0x800 + ofs], boot_file);
      }

      // Patching the NTFS sector
      if ((fs == FST_MBR) || (fs == FST_NTFS))
      {
        ofs  = valueat(grub_mbr, 0xA00 + 0x1EC, unsigned short) & 0x7FF;
        len1 = valueat(grub_mbr, 0xA00 + 0x1EC, unsigned short) >> 11;
        if (len > len1)
        {
          char buf[80];

          sprintf(buf, "Boot file name too long for ntfs partition (%d>%d)", len, len1);
          print_apperr(buf);
          close(hd);
          return 1;
        }
        else
          strcpy((char *) &grub_mbr[0xA00 + ofs], boot_file);
        if (load_seg)
          valueat(grub_mbr, 0xA00 + 0x1EA, unsigned short) = load_seg;
      }
    }

    if (afg & AFG_VERBOSE)
    {
      if (afg & AFG_READ_ONLY)
      {
        fprintf(stderr, "Boot file can be changed to %s\n", boot_file);
        if (load_seg)
          fprintf(stderr, "Load segment can be changed to %04X\n", load_seg);
      }
      else
      {
        fprintf(stderr, "Boot file has been changed to %s\n", boot_file);
        if (load_seg)
          fprintf(stderr, "Load segment has been changed to %04X\n", load_seg);
      }
    }
  }

  if (fs == FST_MBR)
  {
    int           n;
    unsigned long ofs;

    ofs = 0xFFFFFFFF;
    for (n = 0x1BE; n < 0x1FE; n += 16)
      if (prev_mbr[n + 4])
      {
        if (ofs > valueat(prev_mbr[n], 8, unsigned long))
          ofs = valueat(prev_mbr[n], 8, unsigned long);
      }
    if (ofs < (mbr_size >> 9))
    {
      print_apperr("Not enough room to install mbr");
      xd_close(xd);
      return 1;
    }
    slen = mbr_size;
    switch (is_grldr_mbr(prev_mbr))
    {
    case 1:
      sv_res_len = sizeof(grub_mbr45cp);
      break;
    case 2:
      sv_res_len = sizeof(grub_mbr46ap);
      break;
    default:
      sv_res_len = 2048;
    }
    if (afg & AFG_COPY_BPB)
    {
      int  nfs, sln;
      char bs[1024];

      if (xd_seek(xd, ofs))
      {
        print_apperr("Can\'t seek to the first partition");
        xd_close(xd);
        return 1;
      }
      if (xd_read(xd, bs, sizeof(bs) >> 9))
      {
        print_apperr("Fail to read boot sector");
        xd_close(xd);
        return 1;
      }
      nfs = get_fstype((unsigned char *) bs);
      if (nfs == FST_FAT32)
        sln = 0x5A - 0xB;
      else if (nfs == FST_FAT16)
        sln = 0x3E - 0xB;
      else
        sln = 0;
      if (sln)
      {
        memcpy(&grub_mbr[0xB], &bs[0xB], sln);
        valueat(grub_mbr[0], 0x1C, unsigned long)  = 0;
        valueat(grub_mbr[0], 0xE, unsigned short) += ofs;
        valueat(grub_mbr[0], 0x20, unsigned long) += ofs;
      }
    }
  }
  else
  {
    if ((fs == FST_EXFAT) || (((fs == FST_EXT2) || (fs == FST_EXT3) || (fs == FST_EXT4)) && (afg & AFG_USE_NEW_VERSION)))
    {
      slen       = 1024;
      sv_res_len = 1024;
    }
    else
    {
      if (fs == FST_NTFS)
      {
        slen       = 2048;
        sv_res_len = 2048;
      }
      else
      {
        slen       = 512;
        sv_res_len = 512;
      }
    }
  }

  if (xd_seek(xd, ssec))
  {
    print_apperr("Can\'t seek to the start sector");
    xd_close(xd);
    return 1;
  }

  if (save_fn && (!(afg & AFG_READ_ONLY)))
  {
    int h2;

    if (!is_grldr_mbr(prev_mbr))
    {
      h2 = open(save_fn, O_CREAT | O_TRUNC | O_RDWR | O_BINARY, S_IREAD | S_IWRITE);
      if (h2 == -1)
      {
        print_syserr("open save file");
        xd_close(xd);
        return 1;
      }
      nn = write(h2, prev_mbr, sv_res_len);
      if (nn == -1)
      {
        print_syserr("write save file");
        xd_close(xd);
        close(h2);
        return 1;
      }
      if (nn < sv_res_len)
      {
        print_apperr("Can\'t write the whole MBR to the save file");
        xd_close(xd);
        close(h2);
        return 1;
      }
    }
    else
    {
      if (valueat(prev_mbr, 512 + 510, unsigned short) != 0xAA55)
      {
        print_apperr("No previous saved MBR");
        xd_close(xd);
        return 1;
      }
      else
      {
        h2 = open(save_fn, O_CREAT | O_TRUNC | O_RDWR | O_BINARY, S_IREAD | S_IWRITE);
        if (h2 == -1)
        {
          print_syserr("open save file");
          xd_close(xd);
          return 1;
        }
        nn = write(h2, &prev_mbr[512], 512);
        if (nn == -1)
        {
          print_syserr("write save file");
          xd_close(xd);
          close(h2);
          return 1;
        }
        if (nn < 512)
        {
          print_apperr("Can\'t write the whole MBR to the save file");
          xd_close(xd);
          close(h2);
          return 1;
        }
        str_sv = 1;
      }
    }
    close(h2);
  }
  if (afg & AFG_RESTORE_PREVMBR)
  {
    if (fs != FST_MBR)
    {
      print_apperr("Not a disk image");
      xd_close(xd);
      return 1;
    }
    if (!is_grldr_mbr(prev_mbr))
    {
      print_apperr("GRLDR is not installed");
      xd_close(xd);
      return 1;
    }
    if (valueat(prev_mbr, 512 + 510, unsigned short) != 0xAA55)
    {
      print_apperr("No previous saved MBR");
      xd_close(xd);
      return 1;
    }
    memset(&grub_mbr, 0, mbr_size);
    memcpy(&grub_mbr, &prev_mbr[512], 512);
    memcpy(&grub_mbr[0x1b8], &prev_mbr[0x1b8], 72);

    if (afg & AFG_VERBOSE)
      fprintf(stderr, "Restore previous MBR mode\n");
  }
  else
  {
    // Load MBR/BS from restore file or configure grub_mbr
    if (restore_fn)
    {
      int h2;

      h2 = open(restore_fn, O_RDONLY | O_BINARY, S_IREAD);
      if (h2 == -1)
      {
        print_syserr("open restore file");
        xd_close(xd);
        return 1;
      }
      nn = read(h2, grub_mbr, sv_res_len);
      if (nn == -1)
      {
        print_syserr("read restore file");
        xd_close(xd);
        close(h2);
        return 1;
      }
      if (((nn < 512) || ((nn & 0x1FF) != 0) || ((fs != FST_EXT2) && (fs != FST_EXT3) && (fs != FST_EXT4))) && (valueat(grub_mbr, 510, unsigned short) != 0xAA55))
      {
        print_apperr("Invalid restore file");
        xd_close(xd);
        close(h2);
        return 1;
      }
      close(h2);
      if (nn < sv_res_len)
        memset(&grub_mbr[nn], 0, slen - nn);

      //if ((fs==FST_FAT16) || (fs==FST_FAT32) || (fs==FST_NTFS))
      if ((fs != FST_EXT2) && (fs != FST_EXT3) && (fs != FST_EXT4))
      {
        int new_fs;

        new_fs = get_fstype(grub_mbr);
        if (new_fs != fs)
        {
          print_apperr("Invalid restore file");
          xd_close(xd);
          return 1;
        }
      }

      if (afg & AFG_VERBOSE)
        fprintf(stderr, "Restore mode\n");
    }
    else
    {
      if (fs == FST_MBR)
      {
        if (!(afg & AFG_NO_BACKUP_MBR))
        {
          /*
             int i;

             if (afg & AFG_FORCE_BACKUP_MBR)
             i=512;
             else
             for (i=1;i<512;i++)
              if (prev_mbr[512+i]!=prev_mbr[512])
                 break;

             if ((i==512) && (! is_grldr_mbr(prev_mbr)))
             memcpy(&grub_mbr[512],prev_mbr,512);
             else
             memcpy(&grub_mbr[512],&prev_mbr[512],512);
           */
          if (!is_grldr_mbr(prev_mbr))
            memcpy(&grub_mbr[512], prev_mbr, 512);
          else
            memcpy(&grub_mbr[512], &prev_mbr[512], 512);
        }
        memcpy(&grub_mbr[0x1b8], &prev_mbr[0x1b8], 72);
      }
      else if (fs == FST_FAT16)
      {
        if (afg & AFG_USE_NEW_VERSION)
          memcpy(grub_mbr, &grub_pbr46al[0x200], slen);
        else
        {
          memcpy(grub_mbr, &grub_mbr[0x600], slen);
          if (afg & AFG_IS_FLOPPY)
            grub_mbr[0x41] = 0xFF;
          else
            grub_mbr[0x41] = part_num;
        }
      }
      else if (fs == FST_FAT32)
      {
        if (afg & AFG_USE_NEW_VERSION)
          memcpy(grub_mbr, grub_pbr46al, slen);
        else
        {
          memcpy(grub_mbr, &grub_mbr[0x400], slen);
          if (afg & AFG_IS_FLOPPY)
            grub_mbr[0x5D] = 0xFF;
          else
            grub_mbr[0x5D] = part_num;
        }
      }
      else if (fs == FST_NTFS)
      {
        if (afg & AFG_USE_NEW_VERSION)
          memcpy(grub_mbr, &grub_pbr46al[0xC00], slen);
        else
        {
          memcpy(grub_mbr, &grub_mbr[0xA00], slen);
          if (afg & AFG_IS_FLOPPY)
            grub_mbr[0x57] = 0xFF;
          else
            grub_mbr[0x57] = part_num;
        }
      }
      else if (fs == FST_EXFAT)
      {
        memcpy(grub_mbr, &grub_pbr46al[0x800], slen);
        memcpy(&grub_mbr[slen], &prev_mbr[slen], 0x1800 - slen);
        slen = 0x3000;
      }
      else if ((fs == FST_EXT2) || (fs == FST_EXT3) || (fs == FST_EXT4))
      {
        if (afg & AFG_USE_NEW_VERSION)
          memcpy(grub_mbr, &grub_pbr46al[0x400], slen);
        else
        {
          memcpy(grub_mbr, &grub_mbr[0x800], slen);
          if (afg & AFG_IS_FLOPPY)
            grub_mbr[0x25] = 0xFF;
          else
            grub_mbr[0x25] = part_num;
          if (afg & AFG_LBA_MODE)
            grub_mbr[2] = 0x42;
          else if (afg & AFG_CHS_MODE)
            grub_mbr[2] = 0x2;
          else
            grub_mbr[2] = 0x0;

          if (def_spt != -1)
            valueat(grub_mbr, 0x18, unsigned short) = def_spt;
          else
            valueat(grub_mbr, 0x18, unsigned short) = drspt;
          if (def_hds != -1)
            valueat(grub_mbr, 0x1A, unsigned short) = def_hds;
          else
            valueat(grub_mbr, 0x1A, unsigned short) = drheads;
          valueat(grub_mbr, 0x20, unsigned long) = tsec;
          valueat(grub_mbr, 0x1C, unsigned long) = ssec;

          // bytes per block
          unsigned long bpb;

          bpb                                  = 1024 << valueat(prev_mbr[1024], 0x18, unsigned long);
          valueat(grub_mbr, 0xE, unsigned int) = bpb;
          //sectors per block
          valueat(grub_mbr, 0xD, unsigned short) = (unsigned short) (bpb / drbps);
          //number of blocks covered by a indirect block
          valueat(grub_mbr, 0x14, unsigned long) = (unsigned long) bpb / 4;
          //number of blocks covered by a double-indirect block
          valueat(grub_mbr, 0x10, unsigned long) = (unsigned long) (bpb / 4) * (bpb / 4);
          //inode size in bytes
          valueat(grub_mbr, 0x26, unsigned int) = valueat(prev_mbr[1024], 0x58, unsigned int);
          // s_inodes_per_group
          valueat(grub_mbr, 0x28, unsigned long) = valueat(prev_mbr[1024], 0x28, unsigned long);
          // s_first_data_block+1
          valueat(grub_mbr, 0x2C, unsigned long) = valueat(prev_mbr[1024], 0x14, unsigned long) + 1;
        }
      }
      else
      {
        // Shouldn't be here
        if (fs == FST_GPT)
        {
          print_apperr("Unsupported file system");
        }
        else
        {
          print_apperr("Invalid file system");
        }
        xd_close(xd);
        return 1;
      }
      if ((fs == FST_FAT16) || (fs == FST_FAT32) || (fs == FST_NTFS) || (fs == FST_EXFAT))
      {
        if (afg & AFG_LBA_MODE)
          grub_mbr[2] = 0xe;
        else if (afg & AFG_CHS_MODE)
          grub_mbr[2] = 0x90;
        else
          grub_mbr[2] = prev_mbr[2];
      }

      if (afg & AFG_VERBOSE)
        fprintf(stderr, "Install mode\n");
    }
    // Patch the new MBR/BS with information from prev_mbr
    if (fs == FST_MBR)
      memcpy(&grub_mbr[0x1b8], &prev_mbr[0x1b8], 72);
    else if (fs == FST_FAT16)
    {
      if (!(afg & AFG_USE_NEW_VERSION) && !(afg & AFG_IS_FLOPPY))
        valueat(grub_mbr, 0x1C, unsigned long) = ssec;
      if (strncmp(&prev_mbr[0x3], "GRLDR", 5) == 0)
      {
        if (strncmp(&prev_mbr[0x8], "5.0", 3) == 0)
        {
          memcpy(&grub_mbr[0x3], "MSDOS", 5);
          memcpy(&grub_mbr[0x8], &prev_mbr[0x8], 0x3E - 0x8);
        }
        else
        if (strncmp(&prev_mbr[0x8], "4.1", 3) == 0)
        {
          memcpy(&grub_mbr[0x3], "MSWIN", 5);
          memcpy(&grub_mbr[0x8], &prev_mbr[0x8], 0x3E - 0x8);
        }
        else
          memcpy(&grub_mbr[0x3], &prev_mbr[0x3], 0x3E - 0x3);
      }
      else
        memcpy(&grub_mbr[0x3], &prev_mbr[0x3], 0x3E - 0x3);
    }
    else if (fs == FST_FAT32)
    {
      if (!(afg & AFG_USE_NEW_VERSION) && !(afg & AFG_IS_FLOPPY))
        valueat(grub_mbr, 0x1C, unsigned long) = ssec;
      if (strncmp(&prev_mbr[0x3], "GRLDR", 5) == 0)
      {
        if (strncmp(&prev_mbr[0x8], "5.0", 3) == 0)
        {
          memcpy(&grub_mbr[0x3], "MSDOS", 5);
          memcpy(&grub_mbr[0x8], &prev_mbr[0x8], 0x5A - 0x8);
        }
        else
        if (strncmp(&prev_mbr[0x8], "4.1", 3) == 0)
        {
          memcpy(&grub_mbr[0x3], "MSWIN", 5);
          memcpy(&grub_mbr[0x8], &prev_mbr[0x8], 0x5A - 0x8);
        }
        else
          memcpy(&grub_mbr[0x3], &prev_mbr[0x3], 0x5A - 0x3);
      }
      else
        memcpy(&grub_mbr[0x3], &prev_mbr[0x3], 0x5A - 0x3);
    }
    else if (fs == FST_NTFS)
    {
      memcpy(&grub_mbr[0x8], &prev_mbr[0x8], 0x54 - 0x8);
      if (!(afg & AFG_USE_NEW_VERSION) && !(afg & AFG_IS_FLOPPY))
        valueat(grub_mbr, 0x1C, unsigned long) = ssec;
    }
    else if (fs == FST_EXFAT)
    {
      memcpy(&grub_mbr[0x8], &prev_mbr[0x8], 0x78 - 0x8);
      memcpy(&grub_mbr[0x1800 + 0x8], &prev_mbr[0x1800 + 0x8], 0x78 - 0x8);
      int      i;
          #ifdef WIN32
      UINT32   checksum = 0;
          #else
      uint32_t checksum = 0;
          #endif

      for (i = 0; i < (0x200 * 11); i++)
      {
        if (i == 106 || i == 107 || i == 112)
          continue;
        checksum = ((checksum << 31) | (checksum >> 1)) + grub_mbr[i];
      }
      for (i = 0x200 * 11; i < (0x200 * 12); i += 4)
            #ifdef WIN32
        valueat(grub_mbr, i, UINT32) = checksum;
            #else
        valueat(grub_mbr, i, uint32_t) = checksum;
            #endif

      memcpy(&grub_mbr[0x1800], grub_mbr, 12 * 0x200);
    }
  }
   #ifdef WIN32
  HANDLE volume_handle = INVALID_HANDLE_VALUE;
  char   volume_letter;
   #endif

  if (!(afg & AFG_READ_ONLY))
  {
      #ifdef WIN32
    if (part_num > -1)
      if (xd->flg & XDF_DISK)
      {
        STORAGE_DEVICE_NUMBER d1;
        char                  dn[8];
        DWORD                 nr;

        if (DeviceIoControl((HANDLE) xd->num, IOCTL_STORAGE_GET_DEVICE_NUMBER,
                            NULL, 0, &d1, sizeof(d1), &nr, 0))
        {
          strcpy(dn, "\\\\.\\A:");
          for (volume_letter = 'C'; volume_letter < 'Z'; volume_letter++)
          {
            STORAGE_DEVICE_NUMBER d2;

            dn[4]         = volume_letter;
            volume_handle = CreateFile(dn, GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL, OPEN_EXISTING, 0, 0);
            if (volume_handle == INVALID_HANDLE_VALUE)
              continue;
            if (!DeviceIoControl(volume_handle, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &d2, sizeof(d2), &nr, 0))
            {
              CloseHandle(volume_handle);
              volume_handle = INVALID_HANDLE_VALUE;
              continue;
            }
            else
            if ((d1.DeviceType != d2.DeviceType) || (d1.DeviceNumber != d2.DeviceNumber))
            {
              CloseHandle(volume_handle);
              volume_handle = INVALID_HANDLE_VALUE;
              continue;
            }
            PARTITION_INFORMATION pi;

            if (!DeviceIoControl(volume_handle, IOCTL_DISK_GET_PARTITION_INFO, NULL, 0, &pi, sizeof(pi), &nr, 0))
              continue;
            else
            if ((unsigned long) (pi.StartingOffset.QuadPart / drbps) != ssec)
            {
              CloseHandle(volume_handle);
              volume_handle = INVALID_HANDLE_VALUE;
              continue;
            }
            if ((afg & AFG_LOCK) || (afg & AFG_UNMOUNT))
              FlushFileBuffers(volume_handle);
            if (afg & AFG_LOCK)
            {
              char buf[64];

              if (!DeviceIoControl(volume_handle, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &nr, 0))
              {
                CloseHandle(volume_handle);
                sprintf(buf, "Could not lock volume %c!\n", volume_letter);
                fputs(buf, stderr);
                fflush(stderr);
                return 1;
              }
            }

            if (afg & AFG_UNMOUNT)
            {
              char buf[64];

              if (!DeviceIoControl(volume_handle, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &nr, 0))
              {
                CloseHandle(volume_handle);
                sprintf(buf, "Could not unmount volume %c!\n", volume_letter);
                fputs(buf, stderr);
                fflush(stderr);
                return 1;
              }
            }
            if (((afg & AFG_LOCK) || (afg & AFG_UNMOUNT)) && (volume_handle != INVALID_HANDLE_VALUE) && (afg & AFG_VERBOSE))
            {
              char buf[64];

              if ((afg & AFG_UNMOUNT) && (afg & AFG_LOCK))
                sprintf(buf, "Volume %c was locked and unmounted\n", volume_letter);
              else
              if (afg & AFG_UNMOUNT)
                sprintf(buf, "Volume %c was unmounted\n", volume_letter);
              else
              if (afg & AFG_LOCK)
                sprintf(buf, "Volume %c was locked\n", volume_letter);
              fputs(buf, stderr);
            }
            break;
          }
        }
      }
      #endif

    if (xd_write(xd, (char *) grub_mbr, slen >> 9))
    {
      print_apperr("Write error");
      xd_close(xd);
          #ifdef WIN32
      if ((!(afg & AFG_LOCK)) && (!(afg & AFG_UNMOUNT)))
      {
        fputs("\nThis could be the OS or another app locking the boot record, please run this program again with --lock and/or --unmount option(s)\n", stderr);
        fflush(stderr);
      }
      else
      if (!(afg & AFG_LOCK))
      {
        fputs("\nThis could be the OS or another app locking the boot record, please run this program again with --lock option\n", stderr);
        fflush(stderr);
      }
      else
      if (!(afg & AFG_UNMOUNT))
      {
        fputs("\nThis could be the OS or another app locking the boot record, please run this program again with --unmount option\n", stderr);
        fflush(stderr);
      }
      return 1;
      #endif
    }

    #ifdef WIN32
    if (xd->flg & XDF_DISK)
    {
      DWORD nr;

      if (volume_handle != INVALID_HANDLE_VALUE)
      {
        if (!(afg & AFG_UNMOUNT))
          DeviceIoControl(volume_handle, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &nr, 0);
        CloseHandle(volume_handle);
      }
      DeviceIoControl((HANDLE) xd->num, IOCTL_DISK_UPDATE_PROPERTIES, NULL, 0, NULL, 0, &nr, 0);
    }
    #endif
  }
  else if (afg & AFG_VERBOSE)
    fprintf(stderr, "Read only mode\n");

  xd_close(xd);

 quit:
  {
    char msg[1024] = "";
    if (save_fn)
    {
      if (str_sv == 1)
        strcpy(msg, "The stored ");
      else
        strcpy(msg, "The current ");
      if ((part_num > -1) || (afg & AFG_IS_FLOPPY))
        strcat(msg, "PBR ");
      else
        strcat(msg, "MBR/BS ");
      if (afg & AFG_READ_ONLY)
        strcat(msg, "can be");
      else
        strcat(msg, "has been");
      strcat(msg, " successfully saved\n");
    }
    if ((afg & AFG_RESTORE_PREVMBR) || (restore_fn))
      strcat(msg, "The ");
    else
      strcat(msg, "The Grub4Dos ");
    if ((part_num > -1) || (afg & AFG_IS_FLOPPY))
      strcat(msg, "PBR ");
    else
      strcat(msg, "MBR/BS ");
    if (afg & AFG_READ_ONLY)
      strcat(msg, "can be");
    else
      strcat(msg, "has been");
    strcat(msg, " successfully ");
    if (afg & AFG_OUTPUT)
      strcat(msg, "extracted\n");
    else
    if ((afg & AFG_RESTORE_PREVMBR) || (restore_fn))
      strcat(msg, "restored\n");
    else
      strcat(msg, "installed\n");
    fputs(msg, stderr);

    #ifdef WIN32
    if (((afg & AFG_LOCK) || (afg & AFG_UNMOUNT)) && (part_num > -1) && (volume_handle != INVALID_HANDLE_VALUE) && (afg & AFG_VERBOSE))
    {
      char buf[64];

      if ((afg & AFG_UNMOUNT) && (afg & AFG_LOCK))
        sprintf(buf, "Volume %c was remounted and unlocked\n", volume_letter);
      else
      if (afg & AFG_UNMOUNT)
        sprintf(buf, "Volume %c was remounted\n", volume_letter);
      else
      if (afg & AFG_LOCK)
        sprintf(buf, "Volume %c was unlocked\n", volume_letter);
      fputs(buf, stderr);
    }
    #endif

    if (afg & AFG_PAUSE)
    {
      print_pause;
    }
    else
      fflush(stderr);
    return 0;
  }
}

int main(int argc, char** argv)
{
  int idx;

  afg        = gfg = 0;
  part_num   = def_drive = def_part = def_spt = def_hds = def_ssc = def_tsc = -1;
  afg        = 0;
  gfg        = GFG_PREVMBR_LAST;
  time_out   = 5;
  hot_key    = 0x3920;
  save_fn    = NULL;
  restore_fn = NULL;
  fstype     = NULL;

  if ((argc == 4) && (!strcmp(argv[1], "getpart")))
  {
    char          buf_inf[512 * 4];
    unsigned char buf_pbr[512];
    char          strfstype[64];
    int           fs, len;
    char          prev_mbr[sizeof(grub_mbr)];
    xd_t          *xd;
    double        sz;
    char          mu[3];

    xd = xd_open(argv[3], 0, 0);
    if (xd == NULL)
    {
      fputs("error", stderr);
      fflush(stderr);
      return 1;
    }

    #if defined(WIN32)
    if (xd->flg & XDF_DISK)
    {
      DISK_GEOMETRY pdg;
      DWORD         dwBytesReturned = 0;

      if (DeviceIoControl((HANDLE) xd->num, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &pdg, sizeof(DISK_GEOMETRY), &dwBytesReturned, (LPOVERLAPPED) NULL))
        drbps = pdg.BytesPerSector;
    }
    #endif

    #ifdef LINUX
    ioctl(xd->num, BLKSSZGET, &drbps);
    #endif

    if (xd_read(xd, prev_mbr, sizeof(prev_mbr) >> 9))
    {
      fputs("error", stderr);
      fflush(stderr);
      xd_close(xd);
      return 2;
    }
    if (!strcmp(argv[2], "MBR"))
      fs = FST_MBR;
    else
    if (!strcmp(argv[2], "GPT"))
      fs = FST_GPT;
    else
    if (!strcmp(argv[2], "NTFS"))
      fs = FST_NTFS;
    else
    if (!strcmp(argv[2], "exFAT"))
      fs = FST_EXFAT;
    else
    if (!strcmp(argv[2], "FAT32"))
      fs = FST_FAT32;
    else
    if (!strcmp(argv[2], "FAT16"))
      fs = FST_FAT16;
    else
    if (!strcmp(argv[2], "EXT2"))
      fs = FST_EXT2;
    else
    if (!strcmp(argv[2], "EXT3"))
      fs = FST_EXT3;
    else
    if (!strcmp(argv[2], "EXT4"))
      fs = FST_EXT4;
    else
    {
      fs = get_fstype((unsigned char *) prev_mbr);
      if (fs == FST_MBR_FORCE_LBA)
      {
        fs = FST_MBR;
        fputs("MBR (force LBA)\n", stderr);
      }
      else
      if (fs == FST_MBR_BAD)
      {
        fs = FST_MBR;
        fputs("MBR (bad)\n", stderr);
      }
    }

    strcpy(buf_inf, "Whole disk");
    len = strlen(buf_inf);
    sprintf(&buf_inf[len], " (%s)", fst2str(fs));
    fputs(buf_inf, stderr);
    if (fs == FST_MBR)
    {
      xde_t xe;

      xe.cur = xe.nxt = 0xFF;
      while (!xd_enum(xd, &xe))
      {
        if (xe.btf == 0x80)
          strcpy(strfstype, "(B)");
        else
          strcpy(strfstype, "");
        switch (xe.dfs)
        {
        case 0x7:
        {
          if (xd_seek(xd, xe.bse) || xd_read(xd, (char *) buf_pbr, sizeof(buf_pbr) >> 9))
          {
            strcat(strfstype, "NTFS/exFAT");
            break;
          }
          if (buf_pbr[0] == 0xEB)
          {
            if ((buf_pbr[1] == 0x52) && (!strncmp((char *) &buf_pbr[0x3], "NTFS", 4)))
              strcat(strfstype, "NTFS");
            else
            if ((buf_pbr[1] == 0x76) && (!strncmp((char *) &buf_pbr[0x3], "EXFAT", 5)))
              strcat(strfstype, "exFAT");
          }
          break;
        }
        case 0x17:
        {
          if (xd_seek(xd, xe.bse) || xd_read(xd, (char *) buf_pbr, sizeof(buf_pbr) >> 9))
          {
            strcat(strfstype, "(H)NTFS/exFAT");
            break;
          }
          if (buf_pbr[0] == 0xEB)
          {
            if ((buf_pbr[1] == 0x52) && (!strncmp((char *) &buf_pbr[0x3], "NTFS", 4)))
              strcat(strfstype, "(H)NTFS");
            else
            if ((buf_pbr[1] == 0x76) && (!strncmp((char *) &buf_pbr[0x3], "EXFAT", 5)))
              strcat(strfstype, "(H)exFAT");
          }
          break;
        }
        case 0xB:
          strcat(strfstype, "FAT32");
          break;
        case 0xC:
          strcat(strfstype, "FAT32X");
          break;
        case 0x1B:
          strcat(strfstype, "(H)FAT32");
          break;
        case 0x1C:
          strcat(strfstype, "(H)FAT32X");
          break;
        case 0x1:
          strcat(strfstype, "FAT12");
          break;
        case 0x4:
          strcat(strfstype, "FAT16");
          break;
        case 0x6:
          strcat(strfstype, "FAT16B");
          break;
        case 0xE:
          strcat(strfstype, "FAT16X");
          break;
        case 0x11:
          strcat(strfstype, "(H)FAT12");
          break;
        case 0x14:
          strcat(strfstype, "(H)FAT16");
          break;
        case 0x16:
          strcat(strfstype, "(H)FAT16B");
          break;
        case 0x1E:
          strcat(strfstype, "(H)FAT16X");
          break;
        case 0x83:
        {
          if (xd_seek(xd, xe.bse + 2) || xd_read(xd, (char *) buf_pbr, sizeof(buf_pbr) >> 9))
          {
            strcat(strfstype, "EXT2/3/4");
            break;
          }
          if (valueat(buf_pbr, 0xE0, unsigned long) == 0)
            strcat(strfstype, "EXT2");
          else
          {
            unsigned long sfrc;

            sfrc = valueat(buf_pbr, 0x64, unsigned long);
            if ((sfrc & 0x8) || (sfrc & 0x40))
              strcat(strfstype, "EXT4");
            else
              strcat(strfstype, "EXT3");
          }
          break;
        }
        case 0x5:
          strcat(strfstype, "Extended");
          break;
        case 0xF:
          strcat(strfstype, "ExtendedX");
          break;
        case 0x82:
          strcat(strfstype, "Swap");
          break;
        case 0xA5:
          strcat(strfstype, "FBSD");
          break;
        default:
          strcat(strfstype, "Other");
        }
        sz = 1.0 * drbps / 1073741824 * xe.len;
        if (sz < 1)
        {
          sz *= 1024;
          strcpy(mu, "MB");
        }
        else
        {
          if (sz >= 1024)
          {
            sz /= 1024;
            strcpy(mu, "TB");
          }
          else
            strcpy(mu, "GB");
        }
        sprintf(buf_inf, "\np%d:   %s", xe.cur, strfstype);
        fputs(buf_inf, stderr);
        if (sz >= 100)
          sprintf(buf_inf, "\n%.0f %s", sz, mu);
        else
        if (sz >= 10)
          sprintf(buf_inf, "\n%.1f %s", sz, mu);
        else
        if (sz >= 1)
          sprintf(buf_inf, "\n%.2f %s", sz, mu);
        else
        if (sz >= 0.1)
          sprintf(buf_inf, "\n%.3f %s", sz, mu);
        else
        if (sz >= 0.01)
          sprintf(buf_inf, "\n%.4f %s", sz, mu);
        else
          sprintf(buf_inf, "\n");
        fputs(buf_inf, stderr);
      }
    }
    xd_close(xd);
    fflush(stderr);
    return 0;
  }

  for (idx = 1; idx < argc; idx++)
  {
    if (argv[idx][0] != '-')
      break;
    if ((!strcmp(argv[idx], "--help"))
        || (!strcmp(argv[idx], "-h")))
    {
      help();
      print_pause;
      return 1;
    }
    else if (!strcmp(argv[idx], "--version"))
    {
      fprintf(stderr, "grubinst version : " VERSION "\n");
      print_pause;
      return 1;
    }
    else if ((!strcmp(argv[idx], "--verbose")) ||
             (!strcmp(argv[idx], "-v")))
      afg |= AFG_VERBOSE;
    else if (!strcmp(argv[idx], "--pause"))
      afg |= AFG_PAUSE;
    else if ((!strcmp(argv[idx], "--read-only"))
             || (!strcmp(argv[idx], "-t")))
      afg |= AFG_READ_ONLY;
    else if (!strcmp(argv[idx], "--no-backup-mbr"))
      afg |= AFG_NO_BACKUP_MBR;
    else if (!strcmp(argv[idx], "--force-backup-mbr"))
      //afg|=AFG_FORCE_BACKUP_MBR;
      afg &= ~AFG_NO_BACKUP_MBR;
    else if (!strcmp(argv[idx], "--silent-boot"))
      afg |= AFG_SILENT_BOOT;
    else if (!strcmp(argv[idx], "--mbr-enable-floppy"))
      gfg &= ~GFG_DISABLE_FLOPPY;
    else if (!strcmp(argv[idx], "--mbr-disable-floppy"))
      gfg |= GFG_DISABLE_FLOPPY;
    else if (!strcmp(argv[idx], "--mbr-enable-osbr"))
      gfg &= ~GFG_DISABLE_OSBR;
    else if (!strcmp(argv[idx], "--mbr-disable-osbr"))
      gfg |= GFG_DISABLE_OSBR;
    else if (!strcmp(argv[idx], "--duce"))
      gfg |= GFG_DUCE;
    else if (!strcmp(argv[idx], "--chs-no-tune"))       /*--chs-no-tune add by chenall* 2008-12-15*/
      gfg |= CFG_CHS_NO_TUNE;
    else if (!strcmp(argv[idx], "--boot-prevmbr-first"))
      gfg &= ~GFG_PREVMBR_LAST;
    else if (!strcmp(argv[idx], "--boot-prevmbr-last"))
      gfg |= GFG_PREVMBR_LAST;
    else if (!strncmp(argv[idx], "--preferred-drive=", 18))
    {
      def_drive = strtol(&argv[idx][18], NULL, 0);
      if ((def_drive < 0) || (def_drive >= 255))
      {
        print_apperr("Invalid preferred drive number");
        return 1;
      }
    }
    else if (!strncmp(argv[idx], "--preferred-partition=", 22))
    {
      def_part = strtol(&argv[idx][22], NULL, 0);
      if ((def_part < 0) || (def_part >= 255))
      {
        print_apperr("Invalid preferred partition number");
        return 1;
      }
    }
    else if ((!strncmp(argv[idx], "--time-out=", 11)) ||
             (!strncmp(argv[idx], "-t=", 3)))
    {
      time_out = strtol((argv[idx][2] == '=') ? &argv[idx][3] : &argv[idx][11], NULL, 0);
      if ((time_out < 0) || (time_out > 255))
      {
        print_apperr("Invalid timeout value");
        return 1;
      }
    }
    else if ((!strncmp(argv[idx], "--hot-key=", 10)) ||
             (!strncmp(argv[idx], "-k=", 3)))
    {
      char *pk;

      pk      = (argv[idx][2] == '=') ? &argv[idx][3] : &argv[idx][10];
      hot_key = get_keycode(pk);
      if (hot_key == 0)
      {
        print_apperr("Invalid hot key");
        return 1;
      }
      if ((pk[0] != '0') && (pk[1] != 'x') &&
          (key_name == NULL) && (strlen(pk) <= 11))
        key_name = pk;
    }
    else if ((!strncmp(argv[idx], "--key-name=", 11)))
    {
      key_name = &argv[idx][11];
      if (strlen(key_name) > 13)
      {
        print_apperr("Key name too long");
        return 1;
      }
    }
    else if ((!strcmp(argv[idx], "--restore-prevmbr")) ||
             (!strcmp(argv[idx], "-r")))
      afg |= AFG_RESTORE_PREVMBR;
    else if ((!strncmp(argv[idx], "--save=", 7)) ||
             (!strncmp(argv[idx], "-s=", 3)))
    {
      save_fn = (argv[idx][2] == '=') ? &argv[idx][3] : &argv[idx][7];
      if (*save_fn == 0)
      {
        print_apperr("Empty filename");
        return 1;
      }
    }
    else if ((!strncmp(argv[idx], "--restore=", 10)) ||
             (!strncmp(argv[idx], "-r=", 3)))
    {
      restore_fn = (argv[idx][2] == '=') ? &argv[idx][3] : &argv[idx][10];
      if (*restore_fn == 0)
      {
        print_apperr("Empty filename");
        return 1;
      }
    }
    else if ((!strcmp(argv[idx], "--list-part")) ||
             (!strcmp(argv[idx], "-l")))
      afg |= AFG_LIST_PART;
    else if ((!strcmp(argv[idx], "--floppy")) ||
             (!strcmp(argv[idx], "-f")))
      afg |= AFG_IS_FLOPPY;
    else if ((!strncmp(argv[idx], "--floppy=", 9)) ||
             (!strncmp(argv[idx], "--install-partition=", 20)) ||
             (!strncmp(argv[idx], "-p=", 3)))
    {
      char *p;

      if (argv[idx][2] == 'f')
        p = &argv[idx][9];
      else if (argv[idx][2] == 'i')
        p = &argv[idx][20];
      else
        p = &argv[idx][3];
      part_num = strtoul(p, NULL, 0);
      if ((part_num < 0) || (part_num >= MAX_PARTS))
      {
        print_apperr("Invalid partition number");
        return 1;
      }
    }
    else if (!strcmp(argv[idx], "--lba"))
      afg |= AFG_LBA_MODE;
    else if (!strcmp(argv[idx], "--chs"))
      afg |= AFG_CHS_MODE;
    else if (!strncmp(argv[idx], "--sectors-per-track=", 20))
    {
      def_spt = strtol(&argv[idx][20], NULL, 0);
      if ((def_spt < 1) || (def_spt > 63))
      {
        print_apperr("Invalid sector per track");
        return 1;
      }
    }
    else if (!strncmp(argv[idx], "--heads=", 8))
    {
      def_hds = strtol(&argv[idx][8], NULL, 0);
      if ((def_hds < 1) || (def_hds > 255))
      {
        print_apperr("Invalid number of heads");
        return 1;
      }
    }
    else if (!strncmp(argv[idx], "--start-sector=", 15))
    {
      def_ssc = strtol(&argv[idx][15], NULL, 0);
      if (def_ssc < 0)
      {
        print_apperr("Invalid start sector");
        return 1;
      }
    }
    else if (!strncmp(argv[idx], "--total-sectors=", 16))
    {
      def_tsc = strtol(&argv[idx][16], NULL, 0);
      if (def_tsc < 0)
      {
        print_apperr("Invalid total sectors");
        return 1;
      }
    }
    else if ((!strncmp(argv[idx], "--boot-file=", 12)) ||
             (!strncmp(argv[idx], "-b=", 3)))
    {
      if (SetBootFile((argv[idx][2] == '=') ? &argv[idx][3] : &argv[idx][12]))
      {
        print_apperr("Invalid boot file name");
        return 1;
      }
    }
    else if (!strncmp(argv[idx], "--load-seg=", 11))
    {
      load_seg = strtoul(&argv[idx][11], NULL, 16);
      if (load_seg < 0x1000)
      {
        print_apperr("Load address too small");
        return 1;
      }
    }
    else if ((!strcmp(argv[idx], "--grub2")) ||
             (!strcmp(argv[idx], "-2")))
    {
      if (!boot_file)
      {
        boot_file = "g2ldr";
        strcpy(boot_file_83, "G2LDR      ");
      }
    }
    else if ((!strcmp(argv[idx], "--output")) ||
             (!strcmp(argv[idx], "-o")))
      afg |= AFG_OUTPUT;
    else if ((!strcmp(argv[idx], "--edit")) ||
             (!strcmp(argv[idx], "-e")))
      afg |= AFG_EDIT;
    else if (!strcmp(argv[idx], "--skip-mbr-test"))
      afg |= AFG_SKIP_MBR_TEST;
    else if (!strcmp(argv[idx], "--copy-bpb"))
      afg |= AFG_COPY_BPB;
    else if (!strcmp(argv[idx], "--silent-boot"))
      afg |= AFG_SILENT_BOOT;
    else if (!strncmp(argv[idx], "--g4d-version=", 14))
    {
      if (!strcmp(&argv[idx][14], "0.4.6a"))
        afg |= AFG_USE_NEW_VERSION;
      else
      if (strcmp(&argv[idx][14], "0.4.5c"))
      {
        print_apperr("Invalid version, please use 0.4.5c or 0.4.6a");
        return 1;
      }
    }
    else if ((!strncmp(argv[idx], "--file-system-type=", 19)))
    {
      if ((!strcmp(&argv[idx][19], "AutoDetect")) || (!strcmp(&argv[idx][19], "MBR")) || (!strcmp(&argv[idx][19], "GPT")) || (!strcmp(&argv[idx][19], "NTFS")) || (!strcmp(&argv[idx][19], "exFAT")) || (!strcmp(&argv[idx][19], "FAT32")) || (!strcmp(&argv[idx][19], "FAT16")) || (!strcmp(&argv[idx][19], "EXT2")) || (!strcmp(&argv[idx][19], "EXT3")) || (!strcmp(&argv[idx][19], "EXT4")))
        fstype = &argv[idx][19];
      else
      {
        print_apperr("Invalid file system type, please use AutoDetect, MBR, GPT, NTFS, exFAT, FAT32, FAT16, EXT2, EXT3 or EXT4");
        return 1;
      }
    }
    #if defined(WIN32)
    else if (!strcmp(argv[idx], "--lock"))
      afg |= AFG_LOCK;
    else if (!strcmp(argv[idx], "--unmount"))
      afg |= AFG_UNMOUNT;
    #endif
        #ifdef STORE_RMB_STATUS
    else if ((!strncmp(argv[idx], "--rmb-status=", 13)))
    {
      if ((!strcmp(&argv[idx][13], "auto")) || (!strcmp(&argv[idx][13], "on")) || (!strcmp(&argv[idx][13], "off")))
        rmbs = &argv[idx][13];
      else
      {
        print_apperr("Invalid removable bit status value, please use auto, on or off");
        return 1;
      }
    }
        #endif
    else
    {
      print_apperr("Invalid option, please use --help to see all valid options");
      return 1;
    }
  }

  if (idx >= argc)
  {
    print_apperr("No filename specified");
    return 1;
  }
  if (idx < argc - 1)
  {
    print_apperr("Unknown parameter(s)");
    return 1;
  }

  if (afg & AFG_USE_NEW_VERSION)
    memcpy(grub_mbr, grub_mbr46ap, sizeof(grub_mbr46ap));
  else
    memcpy(grub_mbr, grub_mbr45cp, sizeof(grub_mbr45cp));
  return install(argv[idx]);
}