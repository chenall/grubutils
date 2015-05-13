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

#ifdef LINUX

#define _FILE_OFFSET_BITS 64	// This is required to enable 64-bit off_t
#include <unistd.h>
#include <sys/ioctl.h>

#ifndef BLKGETSIZE64
#define BLKGETSIZE64  _IOR(0x12,114,size_t)    /* return device size */
#endif /* ! BLKGETSIZE64 */

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>

#include "xdio.h"

#if defined(WIN32)

#include <windows.h>
#include <winioctl.h>

#ifdef __GNUC__			// Mingw or Cygwin

#define u_off_t		off64_t
#define u_lseek		lseek64

#else

#define u_off_t		__int64
#define u_lseek		_lseeki64

#endif

#else

#define u_off_t		off_t	// In FreeBSD, off_t is 64-bit !
#define u_lseek		lseek

#endif

#ifdef WIN32

typedef struct _STORAGE_DEVICE_NUMBER {
  DEVICE_TYPE DeviceType;
  ULONG       DeviceNumber;
  ULONG       PartitionNumber;
} STORAGE_DEVICE_NUMBER, *PSTORAGE_DEVICE_NUMBER;

int
get_disk (char drive, int rdwr, int lock)
{
  STORAGE_DEVICE_NUMBER d1;
  char dn[24];
  HANDLE hd;
  DWORD nr;
  int i;

  sprintf (dn, "\\\\.\\%c:", drive);
  if (lock==0)
    hd = CreateFile (dn, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		   NULL, OPEN_EXISTING, 0, 0);
  else
    hd = CreateFile (dn, GENERIC_READ, FILE_SHARE_READ,
		   NULL, OPEN_EXISTING, 0, 0);
  if (hd == INVALID_HANDLE_VALUE)
    return -1;

  if (! DeviceIoControl (hd, IOCTL_STORAGE_GET_DEVICE_NUMBER,
			 NULL, 0, &d1, sizeof (d1), &nr, 0))
    return -1;

  CloseHandle (hd);

  for (i = 0; i < MAX_DISKS; i++)
    {
      STORAGE_DEVICE_NUMBER d2;

      sprintf (dn, "\\\\.\\PhysicalDrive%d", i);
      hd = CreateFile (dn, ((rdwr) ? GENERIC_WRITE : 0) | GENERIC_READ,
		       FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		       FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING, 0);
      if (hd == INVALID_HANDLE_VALUE)
	continue;

      if ((DeviceIoControl (hd, IOCTL_STORAGE_GET_DEVICE_NUMBER,
			    NULL, 0, &d2, sizeof (d2), &nr, 0)) &&
	  (d1.DeviceType == d2.DeviceType) &&
	  (d1.DeviceNumber == d2.DeviceNumber))
	return (int) hd;

      CloseHandle (hd);
    }

  return -1;
}
#endif

xd_t *
xd_open (char *fn, int rdwr, int lock)
{
  char dn[24];
  xd_t *xd;
  int hd, pp, tt;

  pp = -1;
  tt = XDF_FILE;
  if (fn[0] == '(')
    {
      int dd;
      char *p;

      if (((fn[1] != 'h') && (fn[1] != 'f')) || (fn[2] != 'd'))
	return NULL;

      dd = strtol (fn + 3, &p, 0);
      if ((dd < 0) || (dd >= MAX_DISKS))
	return NULL;

      if (*p == ',')
	{
	  pp = strtol (p + 1, &p, 0);
	  if ((pp < 0) || (pp >= MAX_PARTS))
	    return NULL;
	}

      if ((*p != ')') || (*(p + 1) != 0))
	return NULL;

#if defined(DOS)
      tt = XDF_DISK;
      hd = dd;
      if (fn[1] == 'h')
	hd += 0x80;
      goto next;
#else
      tt |= XDF_DISK;
      if (fn[1] == 'h')
	{
#if defined(WIN32)
	  sprintf (dn, "\\\\.\\PhysicalDrive%d", dd);
#elif defined(LINUX)
	  sprintf (dn, "/dev/sd%c", 'a' + dd);
#elif defined(FREEBSD)
	  sprintf (dn, "/dev/ad%d", dd);
#else
	  return NULL;
#endif
	}
      else
	{
#if defined(WIN32)
	  if (dd > 1)
	    return NULL;
	  sprintf (dn, "\\\\.\\%c:", 'A' + dd);
#elif defined(LINUX) || defined(FREEBSD)
	  sprintf (dn, "/dev/fd%d", dd);
#else
	  return NULL;
#endif
	}
      fn = dn;
#endif
    }

#ifdef WIN32
  hd = -1;
  if ((strlen (fn) == 2) && (fn[1] = ':'))
    {
      int c;

      c = toupper (fn[0]);

      if ((c >= 'A') && (c <= 'Z'))
	{
	  hd = get_disk (c, rdwr, lock);
	  tt |= XDF_DISK;
	}
    }

  if (hd == -1)
    {
    if (lock==0)
      hd =
        (int) CreateFile (fn, ((rdwr) ? GENERIC_WRITE : 0) | GENERIC_READ,
	  		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
	  		((rdwr) && ((tt & XDF_DISK) == 0))? OPEN_ALWAYS :
	  		OPEN_EXISTING,
	  		FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING, 0);
    else
        hd =
          (int) CreateFile (fn, ((rdwr) ? GENERIC_WRITE : 0) | GENERIC_READ,
	    		FILE_SHARE_READ, NULL,
	    		((rdwr) && ((tt & XDF_DISK) == 0))? OPEN_ALWAYS :
	     		OPEN_EXISTING,
	  	  	FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING, 0);
   }
#else
  hd = open (fn, ((rdwr) ? O_RDWR : O_RDONLY) | O_BINARY);
#endif

  if (hd < 0)
    return NULL;

  xd = malloc (sizeof (xd_t));
  if (xd == NULL)
    return NULL;

  xd->flg = tt;
  xd->num = hd;
  xd->ofs = 0;

#ifdef DOS
  if (tt == XDF_DISK)
    xd16_init (xd);
#endif

  if (pp != -1)
    {
      xde_t xe;

      xe.cur = 0xFF;
      xe.nxt = pp;
      if ((xd_enum (xd, &xe)) || (xd_seek (xd, xe.bse)))
	{
	  xd_close (xd);
	  return NULL;
	}
    }
  return xd;
}

int
xd_seek (xd_t * xd, unsigned long sec)
{
  xd->ofs = sec;
  if (xd->flg & XDF_FILE)
    {
#ifdef WIN32
      unsigned long bs[2];

      bs[0] = sec << 9;
      bs[1] = sec >> 23;

      return (SetFilePointer ((HANDLE) xd->num, bs[0], (PLONG)&bs[1], FILE_BEGIN)
	      == 0xFFFFFFFF);
#else
      // Test if 64-bit seek is supported
#ifndef DOS
      if (sizeof (u_off_t) >= 8)
	{
	  u_off_t bs, rs;

	  bs = sec;
	  bs <<= 9;
	  rs = u_lseek (xd->num, bs, SEEK_SET);
	  return (bs != rs);
	}
      else
#endif
	{
	  unsigned long bs[2];

	  bs[0] = sec << 9;
	  bs[1] = sec >> 23;
	  if (bs[1])
	    return 1;
	  return ((unsigned long) lseek (xd->num, bs[0], SEEK_SET) != bs[0]);
	}
#endif
    }
  else
    return 0;
}

static unsigned char ebuf[512];

// Partition enumerator
// xe->cur is the current partition number, before the first call to xd_enum,
// it should be set to 0xFF
// xe->nxt is the target partition number, if it equals 0xFF, it means enumerate
// all partitions, otherwise, it means jump to the specific partition.
int
xd_enum (xd_t * xd, xde_t * xe)
{
  int kk = 1, cc;

  for (cc = xe->cur;;)
    {
      if (cc == 0xFF)
	{
	  unsigned long pt[4][2];
	  int i, j, np;

	  if (xd_seek (xd, 0))
	    return 1;
	  if (xd_read (xd, (char *) ebuf, 1))
	    return 1;
	  if (valueat (ebuf, 0x1FE, unsigned short) != 0xAA55)
	      return 1;
	  np = 0;
	  for (i = 0x1BE; i < 0x1FE; i += 16)
	    if (ebuf[i + 4])
	      {
		if ((pt[np][1] = valueat (ebuf, i + 12, unsigned long)) == 0)
		  return 1;
		pt[np++][0] = valueat (ebuf, i + 8, unsigned long);
	      }
	  if (np == 0)
	    return 1;
	  // Sort partition table base on start address
	  for (i = 0; i < np - 1; i++)
	    {
	      int k = i;
	      for (j = i + 1; j < np; j++)
		if (pt[k][0] > pt[j][0])
		  k = j;
	      if (k != i)
		{
		  unsigned long tt;

		  tt = pt[i][0];
		  pt[i][0] = pt[k][0];
		  pt[k][0] = tt;
		  tt = pt[i][1];
		  pt[i][1] = pt[k][1];
		  pt[k][1] = tt;
		}
	    }
	  // Should have space for MBR
	  if (pt[0][0] == 0)
	    return 1;
	  // Check for partition overlap
	  for (i = 0; i < np - 1; i++)
	    if (pt[i][0] + pt[i][1] > pt[i + 1][0])
	      return 1;
	  cc = 0;
	}
      else if (kk)
	cc++;
      if ((unsigned char) cc > xe->nxt)
	return 1;
      if (cc < 4)
	{
	  if (xe->nxt < 4)
	    {
	      // Empty partition
	      if (!ebuf[xe->nxt * 16 + 4 + 0x1BE])
		return 1;
	      xe->cur = xe->nxt;
	      xe->dfs = ebuf[xe->nxt * 16 + 4 + 0x1BE];
	      xe->bse =
		valueat (ebuf, xe->nxt * 16 + 8 + 0x1BE, unsigned long);
	      xe->len =
		valueat (ebuf, xe->nxt * 16 + 12 + 0x1BE, unsigned long);
	      return 0;
	    }
	  else if (xe->nxt != 0xFF)
	    cc = 4;
	  else
	    while (cc < 4)
	      {
		if (ebuf[cc * 16 + 4 + 0x1BE])
		  {
		    xe->cur = cc;
		    xe->dfs = ebuf[cc * 16 + 4 + 0x1BE];
                xe->btf = ebuf[cc * 16 + 0x1BE];
		    xe->bse =
		      valueat (ebuf, cc * 16 + 8 + 0x1BE, unsigned long);
		    xe->len =
		      valueat (ebuf, cc * 16 + 12 + 0x1BE, unsigned long);
		    return 0;
		  }
		cc++;
	      }
	}
      if ((cc == 4) && (kk))
	{
	  int i;

	  // Scan for extended partition
	  for (i = 0; i < 4; i++)
	    if ((ebuf[i * 16 + 4 + 0x1BE] == 5)
		|| (ebuf[i * 16 + 4 + 0x1BE] == 0xF))
	      break;
	  if (i == 4)
	    return 1;
	  xe->ebs = xe->bse =
	    valueat (ebuf, i * 16 + 8 + 0x1BE, unsigned long);
	}
      else
	{
	  // Is end of extended partition chain ?
	  if (((ebuf[4 + 0x1CE] != 0x5) && (ebuf[4 + 0x1CE] != 0xF)) ||
	      (valueat (ebuf, 8 + 0x1CE, unsigned long) == 0))
	    return 1;
	  xe->bse = xe->ebs + valueat (ebuf, 8 + 0x1CE, unsigned long);
	}
      {
	while (1)
	  {
	    if (xd_seek (xd, xe->bse))
	      return 1;

	    if (xd_read (xd, (char *) ebuf, 1))
	      return 1;

	    if (valueat (ebuf, 0x1FE, unsigned short) != 0xAA55)
		return 1;

	    if ((ebuf[4 + 0x1BE] == 5) || (ebuf[4 + 0x1BE] == 0xF))
	      {
		if (valueat (ebuf, 8 + 0x1BE, unsigned long) == 0)
		  return 1;
		else
		  {
		    xe->bse =
		      xe->ebs + valueat (ebuf, 8 + 0x1BE, unsigned long);
		    continue;
		  }
	      }
	    break;
	  }
	kk = (ebuf[4 + 0x1BE] != 0);
	if ((kk) && ((xe->nxt == 0xFF) || (cc == xe->nxt)))
	  {
	    xe->cur = cc;
	    xe->dfs = ebuf[4 + 0x1BE];
	    xe->bse += valueat (ebuf, 8 + 0x1BE, unsigned long);
	    xe->len = valueat (ebuf, 12 + 0x1BE, unsigned long);
	    return 0;
	  }
      }
    }
}

int
xd_read (xd_t * xd, char *buf, int len)
{
  int nr;

#ifdef DOS
  if (xd->flg & XDF_DISK)
    return (xd16_read (xd, buf, len));
#endif
  nr = len << 9;
#ifdef WIN32
  {
    unsigned long nn;

    if ((!ReadFile ((HANDLE) xd->num, buf, nr, &nn, NULL))
	|| (nr != (int) nn))
      return 1;
  }
#else
  if (read (xd->num, buf, nr) != nr)
    return 1;
#endif
  xd->ofs += (unsigned long) len;
  return 0;
}

int
xd_write (xd_t * xd, char *buf, int len)
{
  int nr;

#ifdef DOS
  if (xd->flg & XDF_DISK)
    return (xd16_write (xd, buf, len));
#endif
  nr = len << 9;

#ifdef WIN32
  {
    unsigned long nn;

    if ((!WriteFile ((HANDLE) xd->num, buf, nr, &nn, NULL))
	|| (nr != (int) nn))
      return 1;
  }
#else
  if (write (xd->num, buf, nr) != nr)
    return 1;
#endif

  xd->ofs += (unsigned long) len;
  return 0;
}

void
xd_close (xd_t * xd)
{
  if (xd->flg & XDF_FILE)
#ifdef WIN32
    CloseHandle ((HANDLE) xd->num);
#else
    close (xd->num);
#endif

  free (xd);
}

unsigned long
xd_size (xd_t * xd)
{
#ifdef WIN32
  if (xd->flg & XDF_DISK)
    {
      DISK_GEOMETRY_EX g1;
      DISK_GEOMETRY g2;
      DWORD nr;

      if (DeviceIoControl ((HANDLE) xd->num, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
			   0, 0, &g1, sizeof (g1), &nr, 0))
	return g1.DiskSize.QuadPart >> 9;

      if (DeviceIoControl ((HANDLE) xd->num, IOCTL_DISK_GET_DRIVE_GEOMETRY,
			   0, 0, &g2, sizeof (g2), &nr, 0))
	return (g2.Cylinders.QuadPart * g2.TracksPerCylinder *
		g2.SectorsPerTrack);

      return XD_INVALID_SIZE;
    }
  else
    {
      DWORD sz_lo, sz_hi;

      sz_lo = GetFileSize ((HANDLE) xd->num, &sz_hi);
      if (sz_lo == XD_INVALID_SIZE)
	return sz_lo;

      return ((sz_lo >> 9) | (sz_hi << 23));
    }
#elif defined(LINUX)
  struct stat st;
  int hd = xd->num;
  unsigned long long nr;

  if (! ioctl (hd, BLKGETSIZE64, &nr))
    {
      nr >>= 9;
      return nr;
    }

  if (fstat (hd, &st) < 0)
    return XD_INVALID_SIZE;

  return st.st_size >> 9;
#endif

  return XD_INVALID_SIZE;
}

#ifdef WIN32
static HANDLE locked_volumes[24];
#endif

int
xd_lock (xd_t * xd)
{
#ifdef WIN32
  STORAGE_DEVICE_NUMBER d1;
  DWORD nr;
  char dn[8], c;

  if (! (xd->flg & XDF_DISK))
    return 0;

  if (! DeviceIoControl ((HANDLE) xd->num, IOCTL_STORAGE_GET_DEVICE_NUMBER,
			 NULL, 0, &d1, sizeof (d1), &nr, 0))
    return 0;

  strcpy (dn, "\\\\.\\A:");
  for (c = 'C'; c < 'Z'; c++)
    {
      HANDLE hd;
      STORAGE_DEVICE_NUMBER d2;

      dn[4] = c;
      hd = CreateFile (dn, GENERIC_READ | GENERIC_WRITE,
		       FILE_SHARE_READ | FILE_SHARE_WRITE,
		       NULL, OPEN_EXISTING, 0, 0);
      if (hd == INVALID_HANDLE_VALUE)
	continue;

      if ((! DeviceIoControl (hd, IOCTL_STORAGE_GET_DEVICE_NUMBER,
			      NULL, 0, &d2, sizeof (d2), &nr, 0)) ||
	  (d1.DeviceType != d2.DeviceType) ||
	  (d1.DeviceNumber != d2.DeviceNumber))
	{
	  CloseHandle (hd);
	  continue;
	}

      if (! DeviceIoControl (hd, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0,
			     &nr, 0))
	continue;

      locked_volumes[c - 'C'] = hd;
    }
#else
  (void) xd;
#endif

  return 0;
}

void
xd_unlock (void)
{
#ifdef WIN32
  int i;

  for (i = 0; i < 24; i++)
    if (locked_volumes[i])
      {
	DWORD nr;

	DeviceIoControl (locked_volumes[i], IOCTL_DISK_UPDATE_PROPERTIES,
			 NULL, 0, NULL, 0, &nr, 0);
	DeviceIoControl (locked_volumes[i], FSCTL_DISMOUNT_VOLUME,
			 NULL, 0, NULL, 0, &nr, 0);
	CloseHandle (locked_volumes[i]);
	locked_volumes[i] = 0;
      }
#endif
}
