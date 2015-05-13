/*
 *  GRUB Utilities --  Utilities for GRUB Legacy, GRUB2 and GRUB for DOS
 *  Copyright(C) 2007 Bean(bean123ch@gmail.com)
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

#if defined(WIN32)

#include <windows.h>
#include <string.h>
#include <winioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <richedit.h>
#include <windowsx.h>
#include <sys/stat.h>
#include <dbt.h>

#include "resource.h"
#include "version.h"
#include "utils.h"
#include "xdio.h"
#include "grub_mbr45cp.h"
#include "grub_mbr45cnp.h"
#include "grub_mbr46ap.h"
#include "grub_mbr46anp.h"
#include "grub_pbr46al.h"
#include "keytab.h"

HINSTANCE     hInst;
PCHAR         lng_ext;
char          * comp_tab_main[IDC_COUNT_MAIN];
char          * comp_tab_output[IDC_COUNT_OUTPUT];
char          * str_tab[IDS_COUNT];
unsigned char p_fstype[MAX_PARTS + 1];
unsigned long p_start[MAX_PARTS + 1], p_size[MAX_PARTS + 1];
int           p_index[MAX_PARTS + 1];
char          btnInstName[32] = "Install";
BOOLEAN       AlreadyScanned  = TRUE;
HDEVNOTIFY    usbnotification;
HBRUSH        status_brush;
COLORREF      status_color;
char          det_ver[64]    = "";
BOOLEAN       isWin6orabove  = FALSE;
BOOLEAN       isFloppy       = FALSE;
char          MsgToShow[256] = "";
int           DiffWidth      = 15;

#define WM_POSTSHOWJOB    (WM_USER + 3)

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
#define IOCTL_STORAGE_QUERY_PROPERTY    CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _STORAGE_DEVICE_NUMBER
{
  DEVICE_TYPE DeviceType;
  ULONG       DeviceNumber;
  ULONG       PartitionNumber;
} STORAGE_DEVICE_NUMBER, *PSTORAGE_DEVICE_NUMBER;

void SwitchToInstallOrEdit(HWND hWnd, BOOLEAN isInstall, BOOLEAN isMBRorGPT);
void SwitchToMBRorPBR(HWND hWnd, BOOLEAN noRedraw);
void GetOptCurBR(HWND hWnd);
void SetStatusText(HWND hWnd, char* text, signed char installed);

char* LoadText(int id)
{
  static char buf[256];

  if ((id >= IDS_BEGIN) && (id <= IDS_END) && (str_tab[id - IDS_BEGIN]))
    return str_tab[id - IDS_BEGIN];

  LoadString(hInst, id, buf, sizeof(buf));
  return buf;
}

void ChangeText(HWND hWnd)
{
  char LocaleName[4] = "", buf[2560], *pc;
  FILE *in;

  if (!lng_ext)
  {
    GetLocaleInfo(GetUserDefaultLCID(), LOCALE_SABBREVLANGNAME, LocaleName, sizeof(LocaleName));
    lng_ext = LocaleName;
    GetModuleFileName(hInst, buf, sizeof(buf));
    pc = strrchr(buf, '.');
    if (pc)
      strcpy(pc + 1, lng_ext);
  }
  in = fopen(buf, "rt");
  if (in == NULL)
  {
    if (strlen(LocaleName) == 3)
    {
      buf[strlen(buf) - 1] = 0;
      in                   = fopen(buf, "rt");
      if (in == NULL)
        return;
    }
    else
      return;
  }
  int wnl, wol, t;

  wol = strlen(LoadText(IDS_SAVE));
  t   = strlen(LoadText(IDS_COPY));
  if (t > wol)
    wol = t;
  wol = wol + strlen(LoadText(IDS_INSTALL));
  while (fgets(buf, sizeof(buf), in))
  {
    char *pb;
    int  nn;

    if (buf[0] == '#')
      continue;
    pb = &buf[strlen(buf)];
    while ((pb != buf) && ((*(pb - 1) == '\r') || (*(pb - 1) == '\n')))
      pb--;
    if (pb == buf)
      continue;
    *pb = 0;
    pb  = strchr(buf, '=');
    if (pb == NULL)
      continue;
    *(pb++) = 0;
    nn      = atoi(buf);
    if ((nn >= IDS_BEGIN) && (nn <= IDS_END))
    {
      nn -= IDS_BEGIN;
      if (str_tab[nn])
        free(str_tab[nn]);
      str_tab[nn] = malloc(strlen(pb) + 1);
      if (str_tab[nn])
        strcpy(str_tab[nn], pb);
    }
    else
    if (nn < IDC_COUNT_MAIN)
    {
      if (comp_tab_main[nn])
        free(comp_tab_main[nn]);
      comp_tab_main[nn] = malloc(strlen(pb) + 1);
      if (comp_tab_main[nn])
        strcpy(comp_tab_main[nn], pb);
    }
    else
    if ((nn >= IDC_COUNT_MAIN) && (nn < IDS_BEGIN))
    {
      if (comp_tab_output[nn - IDC_COUNT_MAIN])
        free(comp_tab_output[nn - IDC_COUNT_MAIN]);
      comp_tab_output[nn - IDC_COUNT_MAIN] = malloc(strlen(pb) + 1);
      if (comp_tab_output[nn - IDC_COUNT_MAIN])
        strcpy(comp_tab_output[nn - IDC_COUNT_MAIN], pb);
    }
  }
  fclose(in);
  wnl = strlen(LoadText(IDS_SAVE));
  t   = strlen(LoadText(IDS_COPY));
  if (t > wnl)
    wnl = t;
  wnl       = wnl + strlen(LoadText(IDS_INSTALL));
  DiffWidth = 5 * (wnl - wol) + DiffWidth;
}

void PrintError(HWND hWnd, DWORD msg)
{
  MessageBox(hWnd, LoadText(msg), NULL, MB_OK | MB_ICONERROR);
}

char* flipAndCodeBytes(const char* str, int pos, int flip, char* buf)
{
  int i;
  int j = 0;
  int k = 0;

  buf [0] = '\0';
  if (pos <= 0)
    return buf;

  if (!j)
  {
    char p = 0;

    j      = 1;
    k      = 0;
    buf[k] = 0;
    for (i = pos; j && str[i] != '\0'; ++i)
    {
      char c = tolower(str[i]);

      if (isspace(c))
        c = '0';

      ++p;
      buf[k] <<= 4;

      if (c >= '0' && c <= '9')
        buf[k] |= (unsigned char) (c - '0');
      else if (c >= 'a' && c <= 'f')
        buf[k] |= (unsigned char) (c - 'a' + 10);
      else
      {
        j = 0;
        break;
      }

      if (p == 2)
      {
        if (buf[k] != '\0' && !isprint(buf[k]))
        {
          j = 0;
          break;
        }
        ++k;
        p      = 0;
        buf[k] = 0;
      }
    }
  }

  if (!j)
  {
    j = 1;
    k = 0;
    for (i = pos; j && str[i] != '\0'; ++i)
    {
      char c = str[i];

      if (!isprint(c))
      {
        j = 0;
        break;
      }

      buf[k++] = c;
    }
  }

  if (!j)
    k = 0;

  buf[k] = '\0';

  if (flip)
    for (j = 0; j < k; j += 2)
    {
      char t = buf[j];
      buf[j]     = buf[j + 1];
      buf[j + 1] = t;
    }

  i = j = -1;
  for (k = 0; buf[k] != '\0'; ++k)
  {
    if (!isspace(buf[k]))
    {
      if (i < 0)
        i = k;
      j = k;
    }
  }

  if ((i >= 0) && (j >= 0))
  {
    for (k = i; (k <= j) && (buf[k] != '\0'); ++k)
      buf[k - i] = buf[k];
    buf[k - i] = '\0';
  }
  return buf;
}

void RefreshPart(HWND hWnd, BOOLEAN silent);

HANDLE       hd[MAX_DISKS];
int          bt[MAX_DISKS], ic, icp, prev_idx;
byte         rem[MAX_DISKS];
char         prinfo[MAX_DISKS][256];
char         hdsz[MAX_DISKS][64];
unsigned int hdspt[MAX_DISKS], hdbps[MAX_DISKS], hdh[MAX_DISKS];
char         hdl[MAX_DISKS][MAX_PARTS];

void RefreshDisk(HWND hWnd, BOOLEAN FirstPart)
{
  char dn[24];
  int  i;
  HWND hdDISKS;
  char nn[256];

  if (!FirstPart)
    goto jp1;
  for (i = 0; i < MAX_DISKS; i++)
    hd[i] = 0;
  hdDISKS = GetDlgItem(hWnd, IDC_DISKS);
  SetWindowRedraw(hdDISKS, FALSE);
  prev_idx = ComboBox_GetCurSel(hdDISKS);
  ic       = -1;
  if (prev_idx > -1)
  {
    char buf[256];

    if (GetDlgItemText(hWnd, IDC_DISKS, buf, sizeof(buf)) != 0)
    {
      ic = strtoul(&buf[2], NULL, 0);
      sprintf(dn, "\\\\.\\PhysicalDrive%d", ic);
      hd[ic] = CreateFile(dn, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
      if (hd[ic] == INVALID_HANDLE_VALUE)
        ic = -1;
      else
      {
        DWORD                     outsize;
        STORAGE_PROPERTY_QUERY    spq;
        STORAGE_DEVICE_DESCRIPTOR sdd = { 0 };
        spq.PropertyId = StorageDeviceProperty;
        spq.QueryType  = PropertyStandardQuery;
        memset(&sdd, 0, sizeof(sdd));

        if (DeviceIoControl(hd[ic], IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(STORAGE_PROPERTY_QUERY), &sdd, sizeof(STORAGE_DEVICE_DESCRIPTOR), &outsize, NULL))
        {
          bt[ic] = sdd.BusType + 1;
          if (sdd.RemovableMedia)
            rem[ic] = 2;
          else
            rem[ic] = 1;
        }
      }
    }
  }
  SendMessage(hdDISKS, CB_RESETCONTENT, 0, 0);
  if (ic == -1)
  {
    strcpy(dn, "\\\\.\\PhysicalDrive0");
    for (i = 1; i < MAX_DISKS; i++)
    {
      if (i < 10)
        ++dn[17];
      else if (i > 10)
        ++dn[18];
      else
        *(unsigned short *) &dn[17] = 0x3031;        //or use dn[17]='1',dn[18]='0',dn[19]=0;
      bt[i]  = 0;
      rem[i] = 0;
      hd[i]  = CreateFile(dn, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
      if (hd[i] == INVALID_HANDLE_VALUE)
        continue;
      DWORD                     outsize;
      STORAGE_PROPERTY_QUERY    spq;
      STORAGE_DEVICE_DESCRIPTOR sdd = { 0 };
      spq.PropertyId = StorageDeviceProperty;
      spq.QueryType  = PropertyStandardQuery;
      memset(&sdd, 0, sizeof(sdd));

      if (DeviceIoControl(hd[i], IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(STORAGE_PROPERTY_QUERY), &sdd, sizeof(STORAGE_DEVICE_DESCRIPTOR), &outsize, NULL))
      {
        bt[i] = sdd.BusType + 1;
        if (sdd.RemovableMedia)
          rem[i] = 2;
        else
          rem[i] = 1;
        if (sdd.BusType == 7)
          break;
      }
    }
    if (i == MAX_DISKS)
    {
      for (i = 1; i < MAX_DISKS; i++)
        if (hd[i] != INVALID_HANDLE_VALUE)
          break;
    }

    if (i == MAX_DISKS)
    {
      ic     = 0;
      bt[0]  = 0;
      rem[0] = 0;
      hd[0]  = CreateFile("\\\\.\\PhysicalDrive0", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
      if (hd[0] != INVALID_HANDLE_VALUE)
      {
        DWORD                     outsize;
        STORAGE_PROPERTY_QUERY    spq;
        STORAGE_DEVICE_DESCRIPTOR sdd = { 0 };
        spq.PropertyId = StorageDeviceProperty;
        spq.QueryType  = PropertyStandardQuery;
        memset(&sdd, 0, sizeof(sdd));
        if (DeviceIoControl(hd[0], IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(STORAGE_PROPERTY_QUERY), &sdd, sizeof(STORAGE_DEVICE_DESCRIPTOR), &outsize, NULL))
        {
          bt[0] = sdd.BusType + 1;
          if (sdd.RemovableMedia)
            rem[0] = 2;
          else
            rem[0] = 1;
        }
      }
      else
      {
        SetWindowRedraw(hdDISKS, TRUE);
        InvalidateRect(hdDISKS, NULL, TRUE);
        SendDlgItemMessage(hWnd, IDC_PARTS, CB_RESETCONTENT, 0, 0);
        SwitchToMBRorPBR(hWnd, FALSE);
        return;
      }
    }
    else
      ic = i;
  }
  sprintf(nn, "hd%d: ", ic);
  if (bt[ic] > 0)
    switch (bt[ic] - 1)
    {
    case 0:
      strcat(nn, "UNKNOWN ");
      break;
    case 1:
      strcat(nn, "SCSI ");
      break;
    case 2:
      strcat(nn, "ATAPI ");
      break;
    case 3:
      strcat(nn, "ATA ");
      break;
    case 4:
      strcat(nn, "1394 ");
      break;
    case 5:
      strcat(nn, "SSA ");
      break;
    case 6:
      strcat(nn, "FIBRE ");
      break;
    case 7:
      strcat(nn, "USB ");
      break;
    case 8:
      strcat(nn, "RAID ");
      break;
    case 9:
      strcat(nn, "iSCSI ");
      break;
    case 10:
      strcat(nn, "SAS ");
      break;
    case 11:
      strcat(nn, "SATA ");
      break;
    case 12:
      strcat(nn, "SD ");
      break;
    case 13:
      strcat(nn, "MMC ");
      break;
    default:
      strcat(nn, " ");
    }
  if (rem[ic] == 2)
    strcat(nn, "[R]  ");
  else
  if (rem[ic] == 1)
    strcat(nn, "[F]  ");
  STORAGE_PROPERTY_QUERY     storagePropertyQuery;
  PSTORAGE_DEVICE_DESCRIPTOR descrip;
  DWORD                      lpBytesReturned;
  char                       buffer[10000];
  char                       vendorID[10000];
  char                       modelNumber[10000];

  strcpy(prinfo[ic], "");
  ZeroMemory(&storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY));
  storagePropertyQuery.PropertyId = StorageDeviceProperty;
  storagePropertyQuery.QueryType  = PropertyStandardQuery;
  FillMemory(buffer, sizeof(buffer), 0);
  lpBytesReturned = 0;
  if (DeviceIoControl(hd[ic], IOCTL_STORAGE_QUERY_PROPERTY, &storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY),
                      buffer, sizeof(buffer), (LPDWORD) &lpBytesReturned, NULL))
  {
    descrip = (PSTORAGE_DEVICE_DESCRIPTOR) &buffer;
    flipAndCodeBytes(buffer, descrip->VendorIdOffset, 0, vendorID);
    strcat(prinfo[ic], vendorID);
    strcat(prinfo[ic], " ");
    flipAndCodeBytes(buffer, descrip->ProductIdOffset, 0, modelNumber);
    strcat(prinfo[ic], modelNumber);
    strcat(nn, (char *) prinfo[ic]);
  }
  else
    strcat(nn, LoadText(IDS_UNKNOWN));
  DISK_GEOMETRY_EX lpOutBuffer;
  char             mu[4] = "GB";
  DOUBLE           sz;

  strcpy(hdsz[ic], "");
  hdspt[ic] = 63;
  hdbps[ic] = 512;
  hdh[ic]   = 255;
  if (DeviceIoControl(hd[ic], IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &lpOutBuffer, sizeof(DISK_GEOMETRY_EX),
                      (LPDWORD) &lpBytesReturned, NULL))
  {
    hdspt[ic] = lpOutBuffer.Geometry.SectorsPerTrack;
    hdbps[ic] = lpOutBuffer.Geometry.BytesPerSector;
    hdh[ic]   = lpOutBuffer.Geometry.TracksPerCylinder;
    sz        = 1.0 / 1073741824 * lpOutBuffer.DiskSize.QuadPart;
    if (sz < 1)
    {
      sz *= 1024;
      strcpy(mu, "MB");
    }
    else
    if (sz >= 1024)
    {
      sz /= 1024;
      strcpy(mu, "TB");
    }
    if (sz >= 100)
      sprintf(hdsz[ic], ", %.0f %s", sz, mu);
    else
    if (sz >= 10)
      sprintf(hdsz[ic], ", %.1f %s", sz, mu);
    else
    if (sz >= 1)
      sprintf(hdsz[ic], ", %.2f %s", sz, mu);
    else
    if (sz >= 0.1)
      sprintf(hdsz[ic], ", %.3f %s", sz, mu);
    if (strcmp(hdsz[ic], ""))
      strcat(nn, hdsz[ic]);
  }
  char                  c;
  HANDLE                hv;
  STORAGE_DEVICE_NUMBER d;
  char                  buf[64] = " ";

  for (i = 0; i < MAX_DISKS; i++)
    strcpy(hdl[i], "");
  for (c = 'C'; c <= 'Z'; c++)
  {
    sprintf(dn, "\\\\.\\%c:", c);
    hv = CreateFile(dn, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    if (hv == INVALID_HANDLE_VALUE)
      continue;
    if (!DeviceIoControl(hv, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &d, sizeof(d), (LPDWORD) &lpBytesReturned, 0))
    {
      CloseHandle(hv);
      continue;
    }
    if (d.DeviceType == FILE_DEVICE_DISK)
    {
      buf[0] = c;
      strcat(hdl[d.DeviceNumber], buf);
    }
    CloseHandle(hv);
  }
  for (i = 0; i < strlen(hdl[ic]); i++)
    sprintf(&buf[3 * i], ", %c", hdl[ic][i]);
  memcpy(buf, " [", 2);
  if (strlen(hdl[ic]) == 0)
    memcpy(&buf[2], "]\0", 2);
  else
    memcpy(&buf[3 * strlen(hdl[ic])], "]\0", 2);
  strcat(nn, buf);

  SendMessage(hdDISKS, CB_ADDSTRING, 0, (LPARAM) &nn);
  icp = 0;
  SendMessage(hdDISKS, CB_SETCURSEL, (WPARAM) 0, 0);
  SetWindowRedraw(hdDISKS, TRUE);
  InvalidateRect(hdDISKS, NULL, TRUE);
  if (IsDlgButtonChecked(hWnd, IDC_ISDISK) == BST_CHECKED)
    RefreshPart(hWnd, TRUE);
  CloseHandle(hd[ic]);
  return;

 jp1:

  hdDISKS = GetDlgItem(hWnd, IDC_DISKS);
  SetWindowRedraw(hdDISKS, FALSE);
  strcpy(dn, "\\\\.\\PhysicalDrive/");
  for (i = 0; i < MAX_DISKS; i++)
  {
    if (ic < 10)
      ++dn[17];
    else if (ic > 10)
      ++dn[18];
    else
      *(unsigned short *) &dn[17] = 0x3031;      //or use dn[17]='1',dn[18]='0',dn[19]=0;
    if (i == ic)
      continue;
    if (hd[i] == INVALID_HANDLE_VALUE)
      continue;
    if (hd[i] == 0)
    {
      hd[i] = CreateFile(dn, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
      if (hd[i] == INVALID_HANDLE_VALUE)
        continue;
    }
    sprintf(nn, "hd%d: ", i);
    DWORD                     outsize;
    STORAGE_PROPERTY_QUERY    spq;
    STORAGE_DEVICE_DESCRIPTOR sdd = { 0 };
    spq.PropertyId = StorageDeviceProperty;
    spq.QueryType  = PropertyStandardQuery;
    memset(&sdd, 0, sizeof(sdd));

    if (DeviceIoControl(hd[i], IOCTL_STORAGE_QUERY_PROPERTY, &spq, sizeof(STORAGE_PROPERTY_QUERY), &sdd, sizeof(STORAGE_DEVICE_DESCRIPTOR), &outsize, NULL))
    {
      bt[i] = sdd.BusType + 1;
      if (sdd.RemovableMedia)
        rem[i] = 2;
      else
        rem[i] = 1;
    }
    if (bt[i] > 0)
      switch (bt[i] - 1)
      {
      case 0:
        strcat(nn, " ");
        break;
      case 1:
        strcat(nn, "SCSI ");
        break;
      case 2:
        strcat(nn, "ATAPI ");
        break;
      case 3:
        strcat(nn, "ATA ");
        break;
      case 4:
        strcat(nn, "1394 ");
        break;
      case 5:
        strcat(nn, "SSA ");
        break;
      case 6:
        strcat(nn, "FIBRE ");
        break;
      case 7:
        strcat(nn, "USB ");
        break;
      case 8:
        strcat(nn, "RAID ");
        break;
      case 9:
        strcat(nn, "iSCSI ");
        break;
      case 10:
        strcat(nn, "SAS ");
        break;
      case 11:
        strcat(nn, "SATA ");
        break;
      case 12:
        strcat(nn, "SD ");
        break;
      case 13:
        strcat(nn, "MMC ");
        break;
      default:
        strcat(nn, " ");
      }
    if (rem[i] == 2)
      strcat(nn, "[R]  ");
    else
    if (rem[i] == 1)
      strcat(nn, "[F]  ");
    STORAGE_PROPERTY_QUERY     storagePropertyQuery;
    PSTORAGE_DEVICE_DESCRIPTOR descrip;
    DWORD                      lpBytesReturned;
    char                       buffer[10000];
    char                       vendorID[10000];
    char                       modelNumber[10000];

    strcpy(prinfo[i], "");
    ZeroMemory(&storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY));
    storagePropertyQuery.PropertyId = StorageDeviceProperty;
    storagePropertyQuery.QueryType  = PropertyStandardQuery;
    FillMemory(buffer, sizeof(buffer), 0);
    lpBytesReturned = 0;
    if (DeviceIoControl(hd[i], IOCTL_STORAGE_QUERY_PROPERTY, &storagePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY),
                        buffer, sizeof(buffer), (LPDWORD) &lpBytesReturned, NULL))
    {
      descrip = (PSTORAGE_DEVICE_DESCRIPTOR) &buffer;
      flipAndCodeBytes(buffer, descrip->VendorIdOffset, 0, vendorID);
      strcat(prinfo[i], vendorID);
      strcat(prinfo[i], " ");
      flipAndCodeBytes(buffer, descrip->ProductIdOffset, 0, modelNumber);
      strcat(prinfo[i], modelNumber);
      strcat(nn, (char *) prinfo[i]);
    }
    else
      strcat(nn, LoadText(IDS_UNKNOWN));
    DISK_GEOMETRY_EX lpOutBuffer;
    char             mu[4] = "GB";
    DOUBLE           sz;

    strcpy(hdsz[i], "");
    hdspt[i] = 63;
    hdbps[i] = 512;
    hdh[i]   = 255;
    if (DeviceIoControl(hd[i], IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &lpOutBuffer, sizeof(DISK_GEOMETRY_EX),
                        (LPDWORD) &lpBytesReturned, NULL))
    {
      hdspt[i] = lpOutBuffer.Geometry.SectorsPerTrack;
      hdbps[i] = lpOutBuffer.Geometry.BytesPerSector;
      hdh[ic]  = lpOutBuffer.Geometry.TracksPerCylinder;
      sz       = 1.0 / 1073741824 * lpOutBuffer.DiskSize.QuadPart;
      if (sz < 1)
      {
        sz *= 1024;
        strcpy(mu, "MB");
      }
      else
      if (sz >= 1024)
      {
        sz /= 1024;
        strcpy(mu, "TB");
      }
      if (sz >= 100)
        sprintf(hdsz[i], ", %.0f %s", sz, mu);
      else
      if (sz >= 10)
        sprintf(hdsz[i], ", %.1f %s", sz, mu);
      else
      if (sz >= 1)
        sprintf(hdsz[i], ", %.2f %s", sz, mu);
      else
      if (sz >= 0.1)
        sprintf(hdsz[i], ", %.3f %s", sz, mu);
      if (strcmp(hdsz[i], ""))
        strcat(nn, hdsz[i]);
    }
    CloseHandle(hd[i]);
    int j;

    for (j = 0; j < strlen(hdl[i]); j++)
      sprintf(&buf[3 * j], ", %c", hdl[i][j]);
    memcpy(buf, " [", 2);
    if (strlen(hdl[i]) == 0)
      memcpy(&buf[2], "]\0", 2);
    else
      memcpy(&buf[3 * strlen(hdl[i])], "]\0", 2);
    strcat(nn, buf);

    HWND h = GetDlgItem(hWnd, IDC_DISKS);
    if (i < ic)
      SendMessage(h, CB_INSERTSTRING, icp++, (LPARAM) &nn);
    else
      SendMessage(h, CB_ADDSTRING, 0, (LPARAM) &nn);
  }
  SetWindowRedraw(hdDISKS, TRUE);
}

int GetFileName(HWND hWnd, char* fn, int len, BOOLEAN addqts)
{
  if (IsDlgButtonChecked(hWnd, IDC_ISDISK) == BST_CHECKED)
  {
    char buf[512];

    if (GetDlgItemText(hWnd, IDC_DISKS, buf, len) == 0)
    {
      SetStatusText(hWnd, LoadText(IDS_NO_DISKNAME), -1);
      PrintError(hWnd, IDS_NO_DISKNAME);
      return 1;
    }
    char *pc;
    pc = strstr(buf, ": ");
    if (!pc)
    {
      SetStatusText(hWnd, LoadText(IDS_NO_DISKNAME), -1);
      PrintError(hWnd, IDS_NO_DISKNAME);
      return 1;
    }
    strcpy(fn, "(hd   ");
    memcpy(&fn[3], &buf[2], pc - &buf[0] - 2);
    memcpy(&fn[pc - &buf[0] + 1], ")\0", 2);
    len = strlen(fn);
  }
  else if (IsDlgButtonChecked(hWnd, IDC_ISFILE) == BST_CHECKED)
  {
    if (GetDlgItemText(hWnd, IDC_FILENAME, fn, len) == 0)
    {
      SetStatusText(hWnd, LoadText(IDS_NO_FILENAME), -1);
      PrintError(hWnd, IDS_NO_FILENAME);
      return 1;
    }
    if (addqts)
    {
      char buf[MAX_PATH + 2];

      sprintf(buf, "\"%s\"", fn);
      strcpy(fn, &buf[0]);
      len += 2;
    }
  }
  else
  {
    SetStatusText(hWnd, LoadText(IDS_NO_DEVICETYPE), -1);
    PrintError(hWnd, IDS_NO_DEVICETYPE);
    return 1;
  }
  return 0;
}

static unsigned char ebuf[512], buf_mbr[512 * 4];

// Partition enumerator
// xe->cur is the current partition number, before the first call to xd_enum,
// it should be set to 0xFF
// xe->nxt is the target partition number, if it equals 0xFF, it means enumerate
// all partitions, otherwise, it means jump to the specific partition.
int
xd_enum_ex(xd_t * xd, xde_t * xe)
{
  int kk = 1, cc;

  for (cc = xe->cur;; )
  {
    if (cc == 0xFF)
    {
      unsigned long pt[4][2];
      int           i, j, np;

      memcpy(ebuf, buf_mbr, 512);

      if (valueat(ebuf, 0x1FE, unsigned short) != 0xAA55)
        return 1;
      np = 0;
      for (i = 0x1BE; i < 0x1FE; i += 16)
        if (ebuf[i + 4])
        {
          if ((pt[np][1] = valueat(ebuf, i + 12, unsigned long)) == 0)
            return 1;
          pt[np++][0] = valueat(ebuf, i + 8, unsigned long);
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

          tt       = pt[i][0];
          pt[i][0] = pt[k][0];
          pt[k][0] = tt;
          tt       = pt[i][1];
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
          valueat(ebuf, xe->nxt * 16 + 8 + 0x1BE, unsigned long);
        xe->len =
          valueat(ebuf, xe->nxt * 16 + 12 + 0x1BE, unsigned long);
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
              valueat(ebuf, cc * 16 + 8 + 0x1BE, unsigned long);
            xe->len =
              valueat(ebuf, cc * 16 + 12 + 0x1BE, unsigned long);
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
                  valueat(ebuf, i * 16 + 8 + 0x1BE, unsigned long);
    }
    else
    {
      // Is end of extended partition chain ?
      if (((ebuf[4 + 0x1CE] != 0x5) && (ebuf[4 + 0x1CE] != 0xF)) ||
          (valueat(ebuf, 8 + 0x1CE, unsigned long) == 0))
        return 1;
      xe->bse = xe->ebs + valueat(ebuf, 8 + 0x1CE, unsigned long);
    }
    {
      while (1)
      {
        if (xd_seek(xd, xe->bse))
          return 1;

        if (xd_read(xd, (char *) ebuf, 1))
          return 1;

        if (valueat(ebuf, 0x1FE, unsigned short) != 0xAA55)
          return 1;

        if ((ebuf[4 + 0x1BE] == 5) || (ebuf[4 + 0x1BE] == 0xF))
        {
          if (valueat(ebuf, 8 + 0x1BE, unsigned long) == 0)
            return 1;
          else
          {
            xe->bse =
              xe->ebs + valueat(ebuf, 8 + 0x1BE, unsigned long);
            continue;
          }
        }
        break;
      }
      kk = (ebuf[4 + 0x1BE] != 0);
      if ((kk) && ((xe->nxt == 0xFF) || (cc == xe->nxt)))
      {
        xe->cur  = cc;
        xe->dfs  = ebuf[4 + 0x1BE];
        xe->bse += valueat(ebuf, 8 + 0x1BE, unsigned long);
        xe->len  = valueat(ebuf, 12 + 0x1BE, unsigned long);
        return 0;
      }
    }
  }
}

void RefreshPart(HWND hWnd, BOOLEAN silent)
{
  char          fn[MAX_PATH];
  unsigned char buf_pbr[512];
  char          temp[128];
  char          strfstype[64];
  int           fs, len;
  xd_t          *xd;
  DOUBLE        sz;
  char          mu[3];
  BOOLEAN       changed = FALSE;

  SetWindowRedraw(GetDlgItem(hWnd, IDC_PARTS), FALSE);
  SendDlgItemMessage(hWnd, IDC_PARTS, CB_RESETCONTENT, 0, 0);

  if (GetFileName(hWnd, fn, sizeof(fn), FALSE))
  {
    if (IsDlgButtonChecked(hWnd, IDC_ISDISK) == BST_CHECKED)
      SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, IDC_DISKS), TRUE);
    else
    if (IsDlgButtonChecked(hWnd, IDC_ISFILE) == BST_CHECKED)
      SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, IDC_FILENAME), TRUE);
    SetWindowRedraw(GetDlgItem(hWnd, IDC_PARTS), TRUE);
    return;
  }
  xd = xd_open(fn, 0, 0);
  if (xd == NULL)
  {
    if (!silent)
    {
      SetStatusText(hWnd, LoadText(IDS_INVALID_FILE), -1);
      if (IsWindowVisible(hWnd))
        PrintError(hWnd, IDS_INVALID_FILE);
      else
        strcpy(MsgToShow, LoadText(IDS_INVALID_FILE));
      if (IsDlgButtonChecked(hWnd, IDC_ISDISK) == BST_CHECKED)
        SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, IDC_DISKS), TRUE);
      else
      if (IsDlgButtonChecked(hWnd, IDC_ISFILE) == BST_CHECKED)
        SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, IDC_FILENAME), TRUE);
    }
    SetWindowRedraw(GetDlgItem(hWnd, IDC_PARTS), TRUE);
    InvalidateRect(GetDlgItem(hWnd, IDC_PARTS), NULL, TRUE);
    return;
  }
  if (xd_read(xd, (char *) buf_mbr, sizeof(buf_mbr) >> 9))
  {
    if (!silent)
    {
      SetStatusText(hWnd, LoadText(IDS_INVALID_FILE), -1);
      if (IsWindowVisible(hWnd))
        PrintError(hWnd, IDS_INVALID_FILE);
      else
        strcpy(MsgToShow, LoadText(IDS_INVALID_FILE));
      if (IsDlgButtonChecked(hWnd, IDC_ISDISK) == BST_CHECKED)
        SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, IDC_DISKS), TRUE);
      else
      if (IsDlgButtonChecked(hWnd, IDC_ISFILE) == BST_CHECKED)
        SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, IDC_FILENAME), TRUE);
    }
    xd_close(xd);
    SetWindowRedraw(GetDlgItem(hWnd, IDC_PARTS), TRUE);
    InvalidateRect(GetDlgItem(hWnd, IDC_PARTS), NULL, TRUE);
    return;
  }
  GetDlgItemText(hWnd, IDC_FILESYSTEM, temp, sizeof(temp));
  if (!strcmp(temp, "MBR"))
    fs = FST_MBR;
  else
  if (!strcmp(temp, "GPT"))
    fs = FST_GPT;
  else
  if (!strcmp(temp, "NTFS"))
    fs = FST_NTFS;
  else
  if (!strcmp(temp, "exFAT"))
    fs = FST_EXFAT;
  else
  if (!strcmp(temp, "FAT32"))
    fs = FST_FAT32;
  else
  if (!strcmp(temp, "FAT12/16"))
    fs = FST_FAT16;
  else
  if (!strcmp(temp, "EXT2"))
    fs = FST_EXT2;
  else
  if (!strcmp(temp, "EXT3"))
    fs = FST_EXT3;
  else
  if (!strcmp(temp, "EXT4"))
    fs = FST_EXT4;
  else
    fs = get_fstype((unsigned char *) buf_mbr);

  if (fs == FST_MBR_FORCE_LBA)
    fs = FST_MBR;
  else
  if (fs == FST_MBR_BAD)
  {
    if (IsWindowVisible(hWnd))
      MessageBox(hWnd, LoadText(IDS_INVALID_MBR), "Warning", MB_OK | MB_ICONWARNING);
    else
      strcpy(MsgToShow, LoadText(IDS_INVALID_MBR));
    fs = FST_MBR;
  }
  p_fstype[0] = fs;
  p_start[0]  = 0;
  p_index[0]  = -1;
  p_size[0]   = -1;
  isFloppy    = (fs != FST_MBR) && (fs != FST_GPT);

  strcpy(temp, LoadText(IDS_WHOLE_DISK));
  len = strlen(temp);
  sprintf(&temp[len], " (%s)", fst2str(fs));
  SendDlgItemMessage(hWnd, IDC_PARTS, CB_ADDSTRING, 0, (LPARAM) temp);
  SendDlgItemMessage(hWnd, IDC_PARTS, CB_SETCURSEL, (WPARAM) 0, 0);
  SetWindowRedraw(GetDlgItem(hWnd, IDC_PARTS), TRUE);
  if (IsWindowVisible(hWnd))
    RedrawWindow(GetDlgItem(hWnd, IDC_PARTS), NULL, 0, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
  p_start[1] = -1;
  SwitchToMBRorPBR(hWnd, IsWindowVisible(hWnd));
  int i, j, k;
  if (fs == FST_MBR)
  {
    xde_t xe;
    if (IsDlgButtonChecked(hWnd, IDC_ORDER_PART) == BST_UNCHECKED)  //Grub4Dos view style
    {
      char          parts1[MAX_PARTS][128], parts2[MAX_PARTS][128];
      unsigned long ps[MAX_PARTS];
      char          satovl[MAX_PARTS][10];

      i      = 0;
      xe.cur = xe.nxt = 0xFF;
      while (!xd_enum_ex(xd, &xe))
      {
        i++;
        if (xe.btf == 0x80)
          strcpy(parts1[i - 1], "(B)");
        else
          strcpy(parts1[i - 1], "");
        switch (xe.dfs)
        {
        case 0x7:
        {
          if (xd_seek(xd, xe.bse) || xd_read(xd, (char *) buf_pbr, sizeof(buf_pbr) >> 9))
          {
            strcat(parts1[i - 1], "NTFS/exFAT");
            break;
          }
          if (buf_pbr[0] == 0xEB)
          {
            if ((buf_pbr[1] == 0x52) && (!strncmp((char *) &buf_pbr[0x3], "NTFS", 4)))
            {
              p_fstype[i] = FST_NTFS;
              strcat(parts1[i - 1], "NTFS");
            }
            else
            if ((buf_pbr[1] == 0x76) && (!strncmp((char *) &buf_pbr[0x3], "EXFAT", 5)))
            {
              p_fstype[i] = FST_EXFAT;
              strcat(parts1[i - 1], "exFAT");
            }
          }
          break;
        }
        case 0x17:
        {
          if (xd_seek(xd, xe.bse) || xd_read(xd, (char *) buf_pbr, sizeof(buf_pbr) >> 9))
          {
            strcat(parts1[i - 1], "(H)NTFS/exFAT");
            break;
          }
          if (buf_pbr[0] == 0xEB)
          {
            if ((buf_pbr[1] == 0x52) && (!strncmp((char *) &buf_pbr[0x3], "NTFS", 4)))
            {
              p_fstype[i] = FST_NTFS;
              strcat(parts1[i - 1], "(H)NTFS");
            }
            else
            if ((buf_pbr[1] == 0x76) && (!strncmp((char *) &buf_pbr[0x3], "EXFAT", 5)))
            {
              p_fstype[i] = FST_EXFAT;
              strcat(parts1[i - 1], "(H)exFAT");
            }
          }
          break;
        }
        case 0xB:
          p_fstype[i] = FST_FAT32;
          strcat(parts1[i - 1], "FAT32");
          break;
        case 0xC:
          p_fstype[i] = FST_FAT32;
          strcat(parts1[i - 1], "FAT32X");
          break;
        case 0x1B:
          p_fstype[i] = FST_FAT32;
          strcat(parts1[i - 1], "(H)FAT32");
          break;
        case 0x1C:
          p_fstype[i] = FST_FAT32;
          strcat(parts1[i - 1], "(H)FAT32X");
          break;
        case 0x1:
          p_fstype[i] = FST_FAT16;
          strcat(parts1[i - 1], "FAT12");
          break;
        case 0x4:
          p_fstype[i] = FST_FAT16;
          strcat(parts1[i - 1], "FAT16");
          break;
        case 0x6:
          p_fstype[i] = FST_FAT16;
          strcat(parts1[i - 1], "FAT16B");
          break;
        case 0xE:
          p_fstype[i] = FST_FAT16;
          strcat(parts1[i - 1], "FAT16X");
          break;
        case 0x11:
          p_fstype[i] = FST_FAT16;
          strcat(parts1[i - 1], "(H)FAT12");
          break;
        case 0x14:
          p_fstype[i] = FST_FAT16;
          strcat(parts1[i - 1], "(H)FAT16");
          break;
        case 0x16:
          p_fstype[i] = FST_FAT16;
          strcat(parts1[i - 1], "(H)FAT16B");
          break;
        case 0x1E:
          p_fstype[i] = FST_FAT16;
          strcat(parts1[i - 1], "(H)FAT16X");
          break;
        case 0x83:
        {
          p_fstype[i] = FST_EXT2;
          if (xd_seek(xd, xe.bse) || xd_read(xd, (char *) buf_mbr, sizeof(buf_mbr) >> 9))
          {
            strcat(parts1[i - 1], "EXT2/3/4");
            break;
          }
          if (valueat(buf_mbr, 0x400 + 0xE0, unsigned long) == 0)
            strcat(parts1[i - 1], "EXT2");
          else
          {
            unsigned long sfrc;

            sfrc = valueat(buf_mbr, 0x400 + 0x64, unsigned long);
            if ((sfrc & 0x8) || (sfrc & 0x40))
            {
              p_fstype[i] = FST_EXT4;
              strcat(parts1[i - 1], "EXT4");
            }
            else
            {
              p_fstype[i] = FST_EXT3;
              strcat(parts1[i - 1], "EXT3");
            }
          }
          break;
        }
        case 0x5:
          strcat(parts1[i - 1], "Extended");
          break;
        case 0xF:
          strcat(parts1[i - 1], "ExtendedX");
          break;
        case 0x82:
          strcat(parts1[i - 1], "Swap");
          break;
        case 0xA5:
          strcat(parts1[i - 1], "FBSD");
          break;
        default:
          p_fstype[i] = FST_OTHER;
          strcat(parts1[i - 1], "Other");
        }

        p_start[i] = xe.bse;
        p_index[i] = xe.cur;
        p_size[i]  = xe.len;
        ps[i - 1]  = xe.bse;
        if ((xe.dfs == 0x5) || (xe.dfs == 0xf))
          ps[i - 1]--;
        sz = 1.0 / 2097152 * xe.len;
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
        if (sz >= 100)
          sprintf(parts2[i - 1], "%.0f %s", sz, mu);
        else
        if (sz >= 10)
          sprintf(parts2[i - 1], "%.1f %s", sz, mu);
        else
        if (sz >= 1)
          sprintf(parts2[i - 1], "%.2f %s", sz, mu);
        else
        if (sz >= 0.1)
          sprintf(parts2[i - 1], "%.3f %s", sz, mu);
        else
        if (sz >= 0.01)
          sprintf(parts2[i - 1], "%.4f %s", sz, mu);
      }
      for (j = 0; j < i; j++)
        strcpy(satovl[j], "");
      if ((strstr(fn, "(hd") == fn) && (strchr(fn, ')') == &fn[strlen(fn) - 1]))
      {
        ic = strtoul(&fn[3], NULL, 0);
        for (j = 0; j < strlen(hdl[ic]); j++)
        {
          HANDLE hv;
          char   dn[5];

          sprintf(dn, "\\\\.\\%c:", hdl[ic][j]);
          hv = CreateFile(dn, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
          if (hv == INVALID_HANDLE_VALUE)
            continue;
          PARTITION_INFORMATION_EX PartInfo;
          unsigned long            returned_bytes;

          if (DeviceIoControl(hv, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0, &PartInfo, sizeof(PARTITION_INFORMATION_EX),
                              &returned_bytes, FALSE))
            for (k = 0; k < i; k++)
              if ((unsigned long) (PartInfo.StartingOffset.QuadPart / hdbps[ic]) == ps[k])
              {
                sprintf(satovl[k], "   [%c]", hdl[ic][j]);
                break;
              }
          CloseHandle(hv);
        }
      }
      int m;

      m = 0;
      for (j = 0; j < i; j++)
      {
        if ((strlen(parts1[j]) + strlen(parts2[j]) + 3) > m)
          m = strlen(parts1[j]) + strlen(parts2[j]) + 3;
      }
      char buf[128];
      int  mm;

      for (j = 0; j < i; j++)
      {
        sprintf(buf, "p%d:   ", p_index[j + 1]);
        strcat(buf, parts1[j]);
        mm = (int) (1.6 * (m - (strlen(parts1[j]) + strlen(parts2[j])))) + 1;
        for (k = 1; k <= mm; k++)
          strcat(buf, " ");
        strcat(buf, parts2[j]);
        strcat(buf, satovl[j]);
        strcpy(parts1[j], buf);
      }
      for (j = 0; j < i; j++)
        SendDlgItemMessage(hWnd, IDC_PARTS, CB_ADDSTRING, 0, (LPARAM) parts1[j]);
    }
    else          //partition manager view style
    {
      int           trmx_in[MAX_PARTS], trmx_out[MAX_PARTS];
      unsigned long ps[MAX_PARTS];
      int           exchmx;
      unsigned long exchps;
      char          parts_in1[MAX_PARTS][128], parts_in2[MAX_PARTS][128], parts_out1[MAX_PARTS][128], parts_out2[MAX_PARTS][128];
      unsigned char p_fstype_in[MAX_PARTS + 1];
      unsigned long p_start_in[MAX_PARTS + 1], p_size_in[MAX_PARTS + 1];
      int           p_index_in[MAX_PARTS + 1];
      unsigned char dfs_in[MAX_PARTS], dfs_out[MAX_PARTS];
      byte          aftextnd = 0;
      int           m, mm;
      char          satovl[MAX_PARTS][10];

      i      = 0;
      xe.cur = xe.nxt = 0xFF;
      while (!xd_enum_ex(xd, &xe))
      {
        trmx_in[i] = i;
        ps[i]      = xe.bse;
        if ((xe.dfs == 0x5) || (xe.dfs == 0xf))
          ps[i]--;
        i++;
      }
      do
      {
        changed = FALSE;
        for (j = 0; j < (i - 1); j++)
          if (ps[j] > ps[j + 1])
          {
            exchps         = ps[j];
            ps[j]          = ps[j + 1];
            ps[j + 1]      = exchps;
            exchmx         = trmx_in[j];
            trmx_in[j]     = trmx_in[j + 1];
            trmx_in[j + 1] = exchmx;
            changed        = TRUE;
          }
      } while (changed);
      for (j = 0; j < i; j++)
        trmx_out[trmx_in[j]] = j;
      i      = 0;
      xe.cur = xe.nxt = 0xFF;
      while (!xd_enum_ex(xd, &xe))
      {
        i++;
        if (xe.btf == 0x80)
          strcpy(strfstype, "(A)");
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
            {
              p_fstype_in[i] = FST_NTFS;
              strcat(strfstype, "NTFS");
            }
            else
            if ((buf_pbr[1] == 0x76) && (!strncmp((char *) &buf_pbr[0x3], "EXFAT", 5)))
            {
              p_fstype_in[i] = FST_EXFAT;
              strcat(strfstype, "exFAT");
            }
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
            {
              p_fstype_in[i] = FST_NTFS;
              strcat(strfstype, "(H)NTFS");
            }
            else
            if ((buf_pbr[1] == 0x76) && (!strncmp((char *) &buf_pbr[0x3], "EXFAT", 5)))
            {
              p_fstype_in[i] = FST_EXFAT;
              strcat(strfstype, "(H)exFAT");
            }
          }
          break;
        }
        case 0xB:
          p_fstype_in[i] = FST_FAT32;
          strcat(strfstype, "FAT32");
          break;
        case 0xC:
          p_fstype_in[i] = FST_FAT32;
          strcat(strfstype, "FAT32X");
          break;
        case 0x1B:
          p_fstype_in[i] = FST_FAT32;
          strcat(strfstype, "(H)FAT32");
          break;
        case 0x1C:
          p_fstype_in[i] = FST_FAT32;
          strcat(strfstype, "(H)FAT32X");
          break;
        case 0x1:
          p_fstype_in[i] = FST_FAT16;
          strcat(strfstype, "FAT12");
          break;
        case 0x4:
          p_fstype_in[i] = FST_FAT16;
          strcat(strfstype, "FAT16");
          break;
        case 0x6:
          p_fstype_in[i] = FST_FAT16;
          strcat(strfstype, "FAT16B");
          break;
        case 0xE:
          p_fstype_in[i] = FST_FAT16;
          strcat(strfstype, "FAT16X");
          break;
        case 0x11:
          p_fstype_in[i] = FST_FAT16;
          strcat(strfstype, "(H)FAT12");
          break;
        case 0x14:
          p_fstype_in[i] = FST_FAT16;
          strcat(strfstype, "(H)FAT16");
          break;
        case 0x16:
          p_fstype_in[i] = FST_FAT16;
          strcat(strfstype, "(H)FAT16B");
          break;
        case 0x1E:
          p_fstype_in[i] = FST_FAT16;
          strcat(strfstype, "(H)FAT16X");
          break;
        case 0x83:
        {
          p_fstype_in[i] = FST_EXT2;
          if (xd_seek(xd, xe.bse) || xd_read(xd, (char *) buf_mbr, sizeof(buf_mbr) >> 9))
          {
            strcat(strfstype, "EXT2/3/4");
            break;
          }
          if (valueat(buf_mbr, 0x400 + 0xE0, unsigned long) == 0)
            strcat(strfstype, "EXT2");
          else
          {
            unsigned long sfrc;

            sfrc = valueat(buf_mbr, 0x400 + 0x64, unsigned long);
            if ((sfrc & 0x8) || (sfrc & 0x40))
            {
              p_fstype[i] = FST_EXT4;
              strcat(strfstype, "EXT4");
            }
            else
            {
              p_fstype[i] = FST_EXT3;
              strcat(strfstype, "EXT3");
            }
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
          p_fstype_in[i] = FST_OTHER;
          strcat(strfstype, "Other");
        }
        p_start_in[i] = xe.bse;
        p_index_in[i] = xe.cur;
        p_size_in[i]  = xe.len;
        dfs_in[i - 1] = xe.dfs;
        sz            = 1.0 / 2097152 * xe.len;
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
        strcpy(parts_in1[i - 1], strfstype);
        if (sz >= 100)
          sprintf(parts_in2[i - 1], "%.0f %s", sz, mu);
        else
        if (sz >= 10)
          sprintf(parts_in2[i - 1], "%.1f %s", sz, mu);
        else
        if (sz >= 1)
          sprintf(parts_in2[i - 1], "%.2f %s", sz, mu);
        else
        if (sz >= 0.1)
          sprintf(parts_in2[i - 1], "%.3f %s", sz, mu);
        else
        if (sz >= 0.01)
          sprintf(parts_in2[i - 1], "%.4f %s", sz, mu);
      }
      for (j = 0; j < i; j++)
        strcpy(satovl[j], "");
      if ((strstr(fn, "(hd") == fn) && (strchr(fn, ')') == &fn[strlen(fn) - 1]))
      {
        ic = strtoul(&fn[3], NULL, 0);
        for (j = 0; j < strlen(hdl[ic]); j++)
        {
          HANDLE hv;
          char   dn[5];

          sprintf(dn, "\\\\.\\%c:", hdl[ic][j]);
          hv = CreateFile(dn, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
          if (hv == INVALID_HANDLE_VALUE)
            continue;
          PARTITION_INFORMATION_EX PartInfo;
          unsigned long            returned_bytes;

          if (DeviceIoControl(hv, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0, &PartInfo, sizeof(PARTITION_INFORMATION_EX),
                              &returned_bytes, FALSE))
            for (k = 0; k < i; k++)
              if ((unsigned long) (PartInfo.StartingOffset.QuadPart / hdbps[ic]) == ps[k])
              {
                sprintf(satovl[k], "   [%c]", hdl[ic][j]);
                break;
              }
          CloseHandle(hv);
        }
      }
      m = 0;
      for (j = 0; j < i; j++)
      {
        if ((strlen(parts_in1[j]) + strlen(parts_in2[j]) + 3) > m)
          m = strlen(parts_in1[j]) + strlen(parts_in2[j]) + 3;
        strcpy(parts_out1[trmx_out[j]], parts_in1[j]);
        strcpy(parts_out2[trmx_out[j]], parts_in2[j]);
        p_start[trmx_out[j] + 1]  = p_start_in[j + 1];
        p_index[trmx_out[j] + 1]  = p_index_in[j + 1];
        p_fstype[trmx_out[j] + 1] = p_fstype_in[j + 1];
        p_size[trmx_out[j] + 1]   = p_size_in[j + 1];
        dfs_out[trmx_out[j]]      = dfs_in[j];
      }
      for (j = 0; j < i; j++)
      {
        if ((dfs_out[j] == 0x5) || (dfs_out[j] == 0xf))
        {
          aftextnd = 1;
          strcpy(parts_in1[j], "        ");
          strcat(parts_in1[j], parts_out1[j]);
          mm = (int) (1.6 * (m - (strlen(parts_out1[j]) + strlen(parts_out2[j])))) + 1;
          for (k = 1; k <= mm; k++)
            strcat(parts_in1[j], " ");
          strcat(parts_in1[j], parts_out2[j]);
          continue;
        }
        sprintf(temp, "p%d:   ", j - aftextnd + 1);
        strcpy(parts_in1[j], temp);
        strcat(parts_in1[j], parts_out1[j]);
        mm = (int) (1.6 * (m - (strlen(parts_out1[j]) + strlen(parts_out2[j])))) + 1;
        for (k = 1; k <= mm; k++)
          strcat(parts_in1[j], " ");
        strcat(parts_in1[j], parts_out2[j]);
        strcat(parts_in1[j], satovl[j]);
      }
      for (j = 0; j < i; j++)
        SendDlgItemMessage(hWnd, IDC_PARTS, CB_ADDSTRING, 0, (LPARAM) parts_in1[j]);
    }
  }
  else                          //GPT
  {
  }
  xd_close(xd);
  i++;
  for (; i < (MAX_PARTS + 1); i++)
    p_fstype[i] = 0;
}

void SwitchToInstallOrEdit(HWND hWnd, BOOLEAN isInstall, BOOLEAN isMBRorGPT)
{
  char buf[64] = "";
  char *pch;

  if (isMBRorGPT && (p_fstype[0] == FST_GPT))
  {
    strcpy(buf, LoadText(IDS_UNSUPPORTED));
    buf[0] = toupper(buf[0]);
    SetStatusText(hWnd, buf, -1);
  }
  else
  {
    strcpy(buf, "Grub4Dos ");
    if (isMBRorGPT)
      strcat(buf, "MBR/BS");
    else
      strcat(buf, "PBR");

    if (!isInstall)
    {
      if (strcmp(det_ver, ""))
      {
        strcat(buf, det_ver);
        pch = strstr(det_ver, LoadText(IDS_ENHANCED_VERSION));
      }
      else
        pch = NULL;
      strcat(buf, " ");
      strcat(buf, LoadText(IDS_INSTALLED));
      if (pch || ((!isMBRorGPT) && (!strstr(det_ver, LoadText(IDS_OBSOLETE_VERSION)))))
        SetStatusText(hWnd, buf, 1);
      else
        SetStatusText(hWnd, buf, 0);
    }
    else
    {
      pch = strstr(det_ver, LoadText(IDS_UNSUPPORTED_VERSION));
      if (pch)
      {
        strcat(buf, det_ver);
        strcat(buf, " ");
        strcat(buf, LoadText(IDS_INSTALLED));
      }
      else
      {
        strcat(buf, " ");
        strcat(buf, LoadText(IDS_NOT_INSTALLED));
      }
      SetStatusText(hWnd, buf, -1);
    }
  }

  GetDlgItemText(hWnd, IDC_GROUPBOX2, buf, sizeof(buf));
  if (isInstall)
  {
    char bufcmp[64];

    strcpy(bufcmp, LoadText(IDS_OPTIONS));
    strcat(bufcmp, " [");
    strcat(bufcmp, LoadText(IDS_INSTALL_MODE));
    strcat(bufcmp, "]");
    if (!strcmp(buf, bufcmp))
      return;
    CheckDlgButton(hWnd, IDC_NO_BACKUP_MBR, BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_SILENT, BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_DISABLE_FLOPPY, BST_CHECKED);
    CheckDlgButton(hWnd, IDC_DISABLE_OSBR, BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_PREVMBR_FIRST, BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_CHS_NO_TUNE, BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_COPY_BPB, BST_UNCHECKED);
    SetDlgItemText(hWnd, IDC_BOOTFILE, "GRLDR");
    SetDlgItemText(hWnd, IDC_EXTRA, "");
    SetDlgItemText(hWnd, IDC_HOTKEY, "");
    SetDlgItemText(hWnd, IDC_TIMEOUT, "0");
    strcpy(btnInstName, LoadText(IDS_INSTALL));
    SetDlgItemText(hWnd, IDC_GROUPBOX2, bufcmp);
    char buf[64];

    if (isMBRorGPT)
    {
      sprintf(buf, LoadText(IDS_BR_TO_FILE), "MBR");
      SetDlgItemText(hWnd, IDC_SAVEMBR, buf);
    }
    else
    {
      sprintf(buf, LoadText(IDS_BR_TO_FILE), "PBR");
      SetDlgItemText(hWnd, IDC_SAVEMBR, buf);
    }
  }
  else
  {
    CheckDlgButton(hWnd, IDC_NO_BACKUP_MBR, BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_SILENT, BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_DISABLE_FLOPPY, BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_DISABLE_OSBR, BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_PREVMBR_FIRST, BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_CHS_NO_TUNE, BST_UNCHECKED);
    CheckDlgButton(hWnd, IDC_COPY_BPB, BST_UNCHECKED);
    SetDlgItemText(hWnd, IDC_BOOTFILE, "GRLDR");
    SetDlgItemText(hWnd, IDC_EXTRA, "");
    SetDlgItemText(hWnd, IDC_HOTKEY, "");
    SetDlgItemText(hWnd, IDC_TIMEOUT, "0");
    char buf[64];

    if (isMBRorGPT)
    {
      sprintf(buf, LoadText(IDS_COPY_TO_FILE), "MBR");
      SetDlgItemText(hWnd, IDC_SAVEMBR, buf);
    }
    else
    {
      sprintf(buf, LoadText(IDS_BR_TO_FILE), "PBR");
      SetDlgItemText(hWnd, IDC_SAVEMBR, buf);
    }
    char bufcmp[64];

    strcpy(bufcmp, LoadText(IDS_OPTIONS));
    strcat(bufcmp, " [");
    strcat(bufcmp, LoadText(IDS_EDIT_MODE));
    strcat(bufcmp, "]");
    if (!strcmp(buf, bufcmp))
      return;
    strcpy(btnInstName, LoadText(IDS_UPDATE));
    SetDlgItemText(hWnd, IDC_GROUPBOX2, bufcmp);
  }
  if ((IsDlgButtonChecked(hWnd, IDC_RESTORE_PREVMBR) == BST_CHECKED) || (IsDlgButtonChecked(hWnd, IDC_RESTOREMBR) == BST_CHECKED))
    SetDlgItemText(hWnd, IDC_INSTALL, LoadText(IDS_RESTORE));
  else
  {
    if (IsDlgButtonChecked(hWnd, IDC_SAVEMBR) == BST_CHECKED)
    {
      char    buf[64] = "";
      int     idx;
      BOOLEAN isMBRorGPT;

      idx        = ComboBox_GetCurSel(GetDlgItem(hWnd, IDC_PARTS));
      isMBRorGPT = (idx < 0) || ((idx == 0) && ((p_fstype[0] == FST_MBR) || (p_fstype[0] == FST_GPT)));
      if ((strcmp(btnInstName, LoadText(IDS_INSTALL)) == 0) || !isMBRorGPT)
        strcpy(buf, LoadText(IDS_SAVE));
      else
        strcpy(buf, LoadText(IDS_COPY));
      strcat(buf, " && ");
      strcat(buf, btnInstName);
      SetDlgItemText(hWnd, IDC_INSTALL, buf);
    }
    else
    {
      if (IsDlgButtonChecked(hWnd, IDC_OUTPUT) == BST_CHECKED)
        SetDlgItemText(hWnd, IDC_INSTALL, LoadText(IDS_SAVE));
      else
        SetDlgItemText(hWnd, IDC_INSTALL, btnInstName);
    }
  }
}


void SwitchToMBRorPBR(HWND hWnd, BOOLEAN noRedraw)
{
  BOOLEAN isMBRorGPT = TRUE;
  int     idx        = -1;

  if (noRedraw)
    SetWindowRedraw(hWnd, FALSE);

  idx        = ComboBox_GetCurSel(GetDlgItem(hWnd, IDC_PARTS));
  isMBRorGPT = (idx < 0) || ((idx == 0) && ((p_fstype[0] == FST_MBR) || (p_fstype[0] == FST_GPT)));

  POINT cbClientPoint;
  RECT  cbScreenRect;
  int   left1, left2, left3, top1, top2, top3;

  GetWindowRect(GetDlgItem(hWnd, IDC_VERBOSE), &cbScreenRect);
  left1 = cbScreenRect.left;
  top1  = cbScreenRect.top;
  GetWindowRect(GetDlgItem(hWnd, IDC_PREVMBR_FIRST), &cbScreenRect);
  left2 = cbScreenRect.left;
  top2  = cbScreenRect.top;
  GetWindowRect(GetDlgItem(hWnd, IDC_CHS_NO_TUNE), &cbScreenRect);
  left3 = cbScreenRect.left;
  top3  = cbScreenRect.top;
  GetWindowRect(GetDlgItem(hWnd, IDC_SILENT), &cbScreenRect);
  if (!isMBRorGPT)
  {
    if (cbScreenRect.top != top1)
    {
      cbClientPoint.x = left3;
      cbClientPoint.y = top1;
      ScreenToClient(hWnd, &cbClientPoint);
      MoveWindow(GetDlgItem(hWnd, IDC_SILENT), cbClientPoint.x, cbClientPoint.y, cbScreenRect.right - cbScreenRect.left, cbScreenRect.bottom - cbScreenRect.top, !noRedraw);
    }
  }
  else
  {
    if (cbScreenRect.top != top2)
    {
      cbClientPoint.x = left3;
      cbClientPoint.y = top2;
      ScreenToClient(hWnd, &cbClientPoint);
      MoveWindow(GetDlgItem(hWnd, IDC_SILENT), cbClientPoint.x, cbClientPoint.y, cbScreenRect.right - cbScreenRect.left, cbScreenRect.bottom - cbScreenRect.top, !noRedraw);
    }
  }
  if ((idx >= 0) && ((p_fstype[0] == FST_MBR) || (p_fstype[0] == FST_GPT)))
  {
    GetWindowRect(GetDlgItem(hWnd, IDC_SKIP), &cbScreenRect);
    if (idx == 0)
    {
      if (cbScreenRect.top != top3)
      {
        cbClientPoint.x = left2;
        cbClientPoint.y = top3;
        ScreenToClient(hWnd, &cbClientPoint);
        MoveWindow(GetDlgItem(hWnd, IDC_SKIP), cbClientPoint.x, cbClientPoint.y, cbScreenRect.right - cbScreenRect.left, cbScreenRect.bottom - cbScreenRect.top, !noRedraw);
      }
    }
    else
    {
      if (cbScreenRect.top != top1)
      {
        cbClientPoint.x = left2;
        cbClientPoint.y = top1;
        ScreenToClient(hWnd, &cbClientPoint);
        MoveWindow(GetDlgItem(hWnd, IDC_SKIP), cbClientPoint.x, cbClientPoint.y, cbScreenRect.right - cbScreenRect.left, cbScreenRect.bottom - cbScreenRect.top, !noRedraw);
      }
    }
  }
  if (isMBRorGPT && (idx <= 0))
  {
    GetWindowRect(GetDlgItem(hWnd, IDC_BOOTFILE), &cbScreenRect);
    if (cbScreenRect.left != left3)
    {
      cbClientPoint.x = left3;
      cbClientPoint.y = cbScreenRect.top;
      ScreenToClient(hWnd, &cbClientPoint);
      MoveWindow(GetDlgItem(hWnd, IDC_BOOTFILE), cbClientPoint.x, cbClientPoint.y, cbScreenRect.right - cbScreenRect.left, cbScreenRect.bottom - cbScreenRect.top, !noRedraw);
      int left;

      GetWindowRect(GetDlgItem(hWnd, IDC_RESTOREMBR), &cbScreenRect);
      left = cbScreenRect.left + 10;
      GetWindowRect(GetDlgItem(hWnd, IDC_LBOOTFILE), &cbScreenRect);
      cbClientPoint.x = left;
      cbClientPoint.y = cbScreenRect.top;
      ScreenToClient(hWnd, &cbClientPoint);
      MoveWindow(GetDlgItem(hWnd, IDC_LBOOTFILE), cbClientPoint.x, cbClientPoint.y, cbScreenRect.right - cbScreenRect.left, cbScreenRect.bottom - cbScreenRect.top, !noRedraw);
    }
  }
  if ((isMBRorGPT && (idx > 0)) || (!isMBRorGPT))
  {
    GetWindowRect(GetDlgItem(hWnd, IDC_LBOOTFILE), &cbScreenRect);
    if (cbScreenRect.left != left1)
    {
      cbClientPoint.x = left1;
      cbClientPoint.y = cbScreenRect.top;
      ScreenToClient(hWnd, &cbClientPoint);
      MoveWindow(GetDlgItem(hWnd, IDC_LBOOTFILE), cbClientPoint.x, cbClientPoint.y, cbScreenRect.right - cbScreenRect.left, cbScreenRect.bottom - cbScreenRect.top, !noRedraw);
      int left;

      GetWindowRect(GetDlgItem(hWnd, IDC_EXTRA), &cbScreenRect);
      left = cbScreenRect.left;
      GetWindowRect(GetDlgItem(hWnd, IDC_BOOTFILE), &cbScreenRect);
      cbClientPoint.x = left;
      cbClientPoint.y = cbScreenRect.top;
      ScreenToClient(hWnd, &cbClientPoint);
      MoveWindow(GetDlgItem(hWnd, IDC_BOOTFILE), cbClientPoint.x, cbClientPoint.y, cbScreenRect.right - cbScreenRect.left, cbScreenRect.bottom - cbScreenRect.top, !noRedraw);
    }
  }
  ShowWindow(GetDlgItem(hWnd, IDC_NO_BACKUP_MBR), isMBRorGPT ? SW_SHOW : SW_HIDE);
  ShowWindow(GetDlgItem(hWnd, IDC_DISABLE_FLOPPY), isMBRorGPT ? SW_SHOW : SW_HIDE);
  ShowWindow(GetDlgItem(hWnd, IDC_DISABLE_OSBR), isMBRorGPT ? SW_SHOW : SW_HIDE);
  ShowWindow(GetDlgItem(hWnd, IDC_PREVMBR_FIRST), isMBRorGPT ? SW_SHOW : SW_HIDE);
  ShowWindow(GetDlgItem(hWnd, IDC_COPY_BPB), isMBRorGPT ? SW_SHOW : SW_HIDE);
  ShowWindow(GetDlgItem(hWnd, IDC_SKIP), ((p_fstype[0] == FST_MBR) || (p_fstype[0] == FST_GPT)) ? SW_SHOW : SW_HIDE);
  ShowWindow(GetDlgItem(hWnd, IDC_LTIMEOUT), isMBRorGPT ? SW_SHOW : SW_HIDE);
  ShowWindow(GetDlgItem(hWnd, IDC_TIMEOUT), isMBRorGPT ? SW_SHOW : SW_HIDE);
  ShowWindow(GetDlgItem(hWnd, IDC_LHOTKEY), isMBRorGPT ? SW_SHOW : SW_HIDE);
  ShowWindow(GetDlgItem(hWnd, IDC_HOTKEY), isMBRorGPT ? SW_SHOW : SW_HIDE);
  ShowWindow(GetDlgItem(hWnd, IDC_RESTORE_PREVMBR), isMBRorGPT ? SW_SHOW : SW_HIDE);
  ShowWindow(GetDlgItem(hWnd, IDC_OUTPUT), isMBRorGPT ? SW_SHOW : SW_HIDE);
  ShowWindow(GetDlgItem(hWnd, IDC_CHS_NO_TUNE), isMBRorGPT ? SW_SHOW : SW_HIDE);
  ShowWindow(GetDlgItem(hWnd, IDC_LOCK), isMBRorGPT ? SW_HIDE : SW_SHOW);
  ShowWindow(GetDlgItem(hWnd, IDC_UNMOUNT), isMBRorGPT ? SW_HIDE : SW_SHOW);

  if (isWin6orabove && (((p_fstype[0] == FST_MBR) && ((p_fstype[idx] == FST_NTFS) || (p_fstype[idx] == FST_EXFAT))) || ((p_fstype[0] == FST_NTFS) || (p_fstype[0] == FST_EXFAT))))
    CheckDlgButton(hWnd, IDC_LOCK, BST_CHECKED);
  else
    CheckDlgButton(hWnd, IDC_LOCK, BST_UNCHECKED);

  if (isMBRorGPT)
  {
    char buf[64];

    sprintf(buf, LoadText(IDS_BR_TO_FILE), "MBR");
    SetDlgItemText(hWnd, IDC_SAVEMBR, buf);
    sprintf(buf, LoadText(IDS_BR_FROM_FILE), "MBR");
    SetDlgItemText(hWnd, IDC_RESTOREMBR, buf);
    if (IsDlgButtonChecked(hWnd, IDC_RESTORE_PREVMBR) == BST_CHECKED)
      SetDlgItemText(hWnd, IDC_INSTALL, LoadText(IDS_RESTORE));
    if (IsDlgButtonChecked(hWnd, IDC_OUTPUT) == BST_CHECKED)
      SetDlgItemText(hWnd, IDC_INSTALL, LoadText(IDS_SAVE));
  }
  else
  {
    char buf[64];

    sprintf(buf, LoadText(IDS_BR_TO_FILE), "PBR");
    SetDlgItemText(hWnd, IDC_SAVEMBR, buf);
    sprintf(buf, LoadText(IDS_BR_FROM_FILE), "PBR");
    SetDlgItemText(hWnd, IDC_RESTOREMBR, buf);
    if (IsDlgButtonChecked(hWnd, IDC_RESTORE_PREVMBR) == BST_CHECKED)
      SetDlgItemText(hWnd, IDC_INSTALL, btnInstName);
    if (IsDlgButtonChecked(hWnd, IDC_OUTPUT) == BST_CHECKED)
      SetDlgItemText(hWnd, IDC_INSTALL, btnInstName);
  }
  GetOptCurBR(hWnd);
  if (noRedraw)
  {
    SetWindowRedraw(hWnd, TRUE);
    RECT updrect, cmprect;

    GetWindowRect(GetDlgItem(hWnd, IDC_LSTATUS), &cmprect);
    cbClientPoint.x = cmprect.left;
    cbClientPoint.y = cmprect.top;
    ScreenToClient(hWnd, &cbClientPoint);
    updrect.left = cbClientPoint.x;
    updrect.top  = cbClientPoint.y;
    GetWindowRect(GetDlgItem(hWnd, IDC_GROUPBOX3), &cmprect);
    cbClientPoint.x = cmprect.right;
    cbClientPoint.y = cmprect.bottom;
    ScreenToClient(hWnd, &cbClientPoint);
    updrect.right  = cbClientPoint.x;
    updrect.bottom = cbClientPoint.y;
    RedrawWindow(hWnd, &updrect, 0, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
    RedrawWindow(GetDlgItem(hWnd, IDC_INSTALL), NULL, 0, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW);
  }
}

char* str_lowcase(char* str)
{
  int i;

  for (i = 0; str[i]; i++)
    if ((str[i] >= 'A') && (str[i] <= 'Z'))
      str[i] += 'a' - 'A';

  return str;
}

// Grldr flags, this flag is used by grldr.mbr

#define GFG_DISABLE_FLOPPY    1
#define GFG_DISABLE_OSBR      2
#define GFG_DUCE              4
#define CFG_CHS_NO_TUNE       8         /*--chs-no-tune add by chenall* 2008-12-15*/
#define GFG_PREVMBR_LAST      128

void GetOptCurBR(HWND hWnd)
{
  strcpy(det_ver, "");
  int idx = -1;

  idx = ComboBox_GetCurSel(GetDlgItem(hWnd, IDC_PARTS));
  if ((idx < 0) || ((idx == 0) && (p_fstype[0] == FST_OTHER)))
    SwitchToInstallOrEdit(hWnd, TRUE, FALSE);
  else
  {
    char fn[MAX_PATH];

    if (IsDlgButtonChecked(hWnd, IDC_ISDISK) == BST_CHECKED)
    {
      char buf[512];

      if (GetDlgItemText(hWnd, IDC_DISKS, buf, sizeof(buf)) == 0)
      {
        SwitchToInstallOrEdit(hWnd, TRUE, (idx < 0) || ((idx == 0) && ((p_fstype[0] == FST_MBR) || (p_fstype[0] == FST_GPT))));
        return;
      }
      char *pc;
      pc = strstr(buf, ": ");
      if (!pc)
      {
        SwitchToInstallOrEdit(hWnd, TRUE, (idx < 0) || ((idx == 0) && ((p_fstype[0] == FST_MBR) || (p_fstype[0] == FST_GPT))));
        return;
      }
      strcpy(fn, "(hd   ");
      memcpy(&fn[3], &buf[2], pc - &buf[0] - 2);
      memcpy(&fn[pc - &buf[0] + 1], ")\0", 2);
    }
    else
    {
      if (IsDlgButtonChecked(hWnd, IDC_ISFILE) == BST_CHECKED)
      {
        if (GetDlgItemText(hWnd, IDC_FILENAME, fn, sizeof(fn)) == 0)
        {
          SwitchToInstallOrEdit(hWnd, TRUE, (idx < 0) || ((idx == 0) && ((p_fstype[0] == FST_MBR) || (p_fstype[0] == FST_GPT))));
          return;
        }
      }
      else
      {
        SwitchToInstallOrEdit(hWnd, TRUE, (idx < 0) || ((idx == 0) && ((p_fstype[0] == FST_MBR) || (p_fstype[0] == FST_GPT))));
        return;
      }
    }

    if ((idx == 0) && (p_fstype[0] == FST_MBR))       //MBR
    {
      int            g4d_mbr_ver = 0;
      int            slen        = sizeof(grub_mbr45cp);
      char           bsfp[1024];
      BOOLEAN        r_bsfp = FALSE;
      unsigned long  bs_ofs;
      int            sln = 0;
      byte           grub_mbr[slen + 1], cur_mbr[slen];
      xd_t           * xd;
      BOOLEAN        silent_boot, backup_mbr, disable_floppy, disable_osbr, prevmbr_last, chs_no_tune;
      unsigned short load_seg         = 8192;
      char           boot_file[64]    = "";
      char           boot_file_83[12] = "";
      int            gfg, time_out, hot_key, part_num, def_drive, def_part;
      char           key_name[64]   = "", lc_key_name[64] = "";
      char           extra_par[512] = "";

      hot_key     = time_out = gfg = part_num = def_drive = def_part = -1;
      silent_boot = backup_mbr = disable_floppy = disable_osbr = prevmbr_last = chs_no_tune = FALSE;

      xd = xd_open(fn, 0, 0);
      if (xd == NULL)
      {
        SwitchToInstallOrEdit(hWnd, TRUE, (idx < 0) || ((idx == 0) && (p_fstype[0] == FST_MBR)));
        return;
      }
      if (xd_read(xd, (char *) cur_mbr, sizeof(cur_mbr) >> 9))
      {
        xd_close(xd);
        SwitchToInstallOrEdit(hWnd, TRUE, (idx < 0) || ((idx == 0) && (p_fstype[0] == FST_MBR)));
        return;
      }
      int i;

      for (i = 0x170; i < 0x1F0; i++)
        if (!memcmp(&cur_mbr[i], "\r\nMissing ", 10))
        {
          if (!memcmp(&cur_mbr[i + 10], "helper.", 7))
          {
            g4d_mbr_ver = 2;
            break;
          }
          if (!memcmp(&cur_mbr[i + 10], "MBR-helper.", 11))
          {
            g4d_mbr_ver = 1;
            break;
          }
        }
      if (i >= 0x1F0)
      {
        xd_close(xd);
        goto not_g4d;
      }

      if ((!strncmp((char *) &cur_mbr[0x36], "FAT", 3) && ((cur_mbr[0x26] == 0x28) || (cur_mbr[0x26] == 0x29))) ||
          (!strncmp((char *) &cur_mbr[0x52], "FAT32", 5) && ((cur_mbr[0x42] == 0x28) || (cur_mbr[0x42] == 0x29))))
      {
        if (p_start[1] == -1)
        {
          int n;

          bs_ofs = 0xFFFFFFFF;
          for (n = 0x1BE; n < 0x1FE; n += 16)
            if (cur_mbr[n + 4])
            {
              if (bs_ofs > valueat(cur_mbr[n], 8, unsigned long))
                bs_ofs = valueat(cur_mbr[n], 8, unsigned long);
            }
          if ((!xd_seek(xd, bs_ofs)) && (!(xd_read(xd, bsfp, sizeof(bsfp) >> 9))))
          {
            int fst_bs = get_fstype((unsigned char *) bsfp);

            if (fst_bs == FST_FAT32)
              sln = 0x5A - 0xB;
            else
            if (fst_bs == FST_FAT16)
              sln = 0x3E - 0xB;
            else
              sln = 0;
            if (sln)
            {
              valueat(bsfp[0], 0x1C, unsigned long) = 0;
              valueat(bsfp[0], 0xE, unsigned short) = valueat(bsfp[0], 0xE, unsigned short) + bs_ofs;
              valueat(bsfp[0], 0x20, unsigned long) = valueat(bsfp[0], 0x20, unsigned long) + bs_ofs;
              r_bsfp                                = memcmp(&cur_mbr[0xB], &bsfp[0xB], sln) == 0;
            }
          }
        }
        else
          for (i = 1; i <= MAX_PARTS; i++)
            if ((p_index[i] == 0) && ((p_fstype[i] == FST_FAT32) || (p_fstype[i] == FST_FAT16)))
            {
              bs_ofs = p_start[i];
              if ((!xd_seek(xd, bs_ofs)) && (!(xd_read(xd, bsfp, sizeof(bsfp) >> 9))))
              {
                if (p_fstype[i] == FST_FAT32)
                  sln = 0x5A - 0xB;
                else
                if (p_fstype[i] == FST_FAT16)
                  sln = 0x3E - 0xB;
                else
                  sln = 0;
                if (sln)
                {
                  valueat(bsfp[0], 0x1C, unsigned long) = 0;
                  valueat(bsfp[0], 0xE, unsigned short) = valueat(bsfp[0], 0xE, unsigned short) + bs_ofs;
                  valueat(bsfp[0], 0x20, unsigned long) = valueat(bsfp[0], 0x20, unsigned long) + bs_ofs;
                  r_bsfp                                = memcmp(&cur_mbr[0xB], &bsfp[0xB], sln) == 0;
                }
              }
              break;
            }
      }
      xd_close(xd);

      for (i = 512; i < 1024; i++)
        if (cur_mbr[i] != 0)
        {
          backup_mbr = TRUE;
          break;
        }
      if (g4d_mbr_ver == 2)
        goto try_another_mbr_2;
      memcpy(grub_mbr, grub_mbr45cp, slen);
      if (r_bsfp)
        memcpy(&grub_mbr[0xB], &bsfp[0xB], sln);
      if (backup_mbr)
        memcpy(&grub_mbr[512], &cur_mbr[512], 512);
      else
        memset(&grub_mbr[512], 0, 512);
      if ((cur_mbr[0xA00 + (valueat(grub_mbr45cp, 0xA00 + 0x1EC, unsigned short) & 0x7FF) - 3] == 0) && (cur_mbr[0xA00 + (valueat(grub_mbr45cp, 0xA00 + 0x1EC, unsigned short) & 0x7FF) - 2] == 0) && (cur_mbr[0x400 + (valueat(grub_mbr45cp, 0x400 + 0x1EC, unsigned short) & 0x7FF) - 3] == 0) && (cur_mbr[0x400 + (valueat(grub_mbr45cp, 0x400 + 0x1EC, unsigned short) & 0x7FF) - 2] == 0) && (cur_mbr[0x600 + (valueat(grub_mbr45cp, 0x600 + 0x1EC, unsigned short) & 0x7FF) - 3] == 0) && (cur_mbr[0x600 + (valueat(grub_mbr45cp, 0x600 + 0x1EC, unsigned short) & 0x7FF) - 2] == 0) && (cur_mbr[0x800 + (valueat(grub_mbr45cp, 0x800 + 0x1EE, unsigned short) & 0x7FF) - 3] == 0) && (cur_mbr[0x800 + (valueat(grub_mbr45cp, 0x800 + 0x1EE, unsigned short) & 0x7FF) - 2] == 0))
      {
        for (i = 0x1EA7; i <= (0x1EA7 + 74); i++)
          if (cur_mbr[i] != 0)
            break;
        if (i > (0x1EA7 + 74))
        {
          for (i = 0x1F30; i <= (0x1F30 + 98); i++)
            if (cur_mbr[i] != 0)
              break;
          if (i > (0x1F30 + 98))
          {
            for (i = 0x22D2; i <= (0x22D2 + 32); i++)
              if (cur_mbr[i] != 0)
                break;
            if (i > (0x22D2 + 32))
            {
              memset(&grub_mbr[0xA00 + (valueat(grub_mbr45cp, 0xA00 + 0x1EC, unsigned short) & 0x7FF) - 3], 0, 2);
              memset(&grub_mbr[0x400 + (valueat(grub_mbr45cp, 0x400 + 0x1EC, unsigned short) & 0x7FF) - 3], 0, 2);
              memset(&grub_mbr[0x600 + (valueat(grub_mbr45cp, 0x600 + 0x1EC, unsigned short) & 0x7FF) - 3], 0, 2);
              memset(&grub_mbr[0x800 + (valueat(grub_mbr45cp, 0x800 + 0x1EE, unsigned short) & 0x7FF) - 3], 0, 2);
              memset(&grub_mbr[0x1EA7], 0, 74);
              memset(&grub_mbr[0x1F30], 0, 98);
              memset(&grub_mbr[0x22D2], 0, 32);
              silent_boot = TRUE;
            }
          }
        }
      }
      valueat(grub_mbr, 2 + 2, unsigned char)  = gfg = valueat(cur_mbr, 2 + 2, unsigned char);
      valueat(grub_mbr, 3 + 2, unsigned char)  = time_out = valueat(cur_mbr, 3 + 2, unsigned char);
      valueat(grub_mbr, 4 + 2, unsigned short) = hot_key = valueat(cur_mbr, 4 + 2, unsigned short);
      valueat(grub_mbr, 6 + 2, unsigned char)  = def_drive = valueat(cur_mbr, 6 + 2, unsigned char);
      if (def_drive < 255)
      {
        char buf[64];

        sprintf(buf, " --preferred-drive=%d", def_drive);
        strcat(extra_par, buf);
      }
      valueat(grub_mbr, 7 + 2, unsigned char) = def_part = valueat(cur_mbr, 7 + 2, unsigned char);
      if (def_part < 255)
      {
        char buf[64];

        sprintf(buf, " --preferred-partition=%d", def_part);
        strcat(extra_par, buf);
      }
      strcpy(key_name, (char *) &cur_mbr[0x1FE8]);
      strcpy(lc_key_name, key_name);
      str_lowcase(lc_key_name);
      if (strcmp(str_lowcase(get_keyname(hot_key)), lc_key_name) && (hot_key != 0) && strcmp(key_name, ""))
      {
        strcat(extra_par, " --key-name=");
        strcat(extra_par, key_name);
      }
      memcpy(&grub_mbr[0x1B8], &cur_mbr[0x1B8], 72);
      memcpy(&grub_mbr[0x1FE8], &cur_mbr[0x1FE8], 13);
      disable_floppy = gfg & GFG_DISABLE_FLOPPY;
      disable_osbr   = gfg & GFG_DISABLE_OSBR;
      if (gfg & GFG_DUCE)
        strcat(extra_par, " --duce");
      if (gfg & CFG_CHS_NO_TUNE)
        chs_no_tune = TRUE;
      prevmbr_last = gfg & GFG_PREVMBR_LAST;
      strncpy(boot_file, (char *) &cur_mbr[0xA00 + (valueat(grub_mbr45cp, 0xA00 + 0x1EC, unsigned short) & 0x7FF)], 24);
      if (!strcmp(boot_file, ""))
        goto try_another_mbr_1;
      else
      {
        char* pc;

        pc = strchr(&boot_file[0], '.');
        if (((pc) && (((pc == &boot_file[0]) || (pc - &boot_file[0] > 8) || (strlen(pc + 1) > 3)))) || ((!pc) && (strlen(boot_file) > 8)))
          goto try_another_mbr_1;
        str_upcase(boot_file);
        memset(boot_file_83, ' ', sizeof(boot_file_83) - 1);
        if (pc)
        {
          memcpy(boot_file_83, boot_file, pc - &boot_file[0]);
          memcpy(&boot_file_83[8], pc + 1, strlen(pc + 1));
        }
        else
          memcpy(boot_file_83, boot_file, strlen(boot_file));
        str_lowcase(boot_file);
        char buf[64];

        strncpy(buf, (char *) &cur_mbr[0x400 + (valueat(grub_mbr45cp, 0x400 + 0x1EC, unsigned short) & 0x7FF)], 24);
        if (strcmp(boot_file_83, buf))
          goto try_another_mbr_1;
        strncpy(buf, (char *) &cur_mbr[0x600 + (valueat(grub_mbr45cp, 0x600 + 0x1EC, unsigned short) & 0x7FF)], 24);
        if (strcmp(boot_file_83, buf))
          goto try_another_mbr_1;
        strncpy(buf, (char *) &cur_mbr[0x800 + (valueat(grub_mbr45cp, 0x800 + 0x1EE, unsigned short) & 0x7FF)], 11);
        if (strcmp(boot_file, buf))
          goto try_another_mbr_1;
        strncpy((char *) &grub_mbr[0xA00 + (valueat(grub_mbr45cp, 0xA00 + 0x1EC, unsigned short) & 0x7FF)], (char *) &cur_mbr[0xA00 + (valueat(grub_mbr45cp, 0xA00 + 0x1EC, unsigned short) & 0x7FF)], 23);
        strncpy((char *) &grub_mbr[0x400 + (valueat(grub_mbr45cp, 0x400 + 0x1EC, unsigned short) & 0x7FF)], (char *) &cur_mbr[0x400 + (valueat(grub_mbr45cp, 0x400 + 0x1EC, unsigned short) & 0x7FF)], 23);
        strncpy((char *) &grub_mbr[0x600 + (valueat(grub_mbr45cp, 0x600 + 0x1EC, unsigned short) & 0x7FF)], (char *) &cur_mbr[0x600 + (valueat(grub_mbr45cp, 0x600 + 0x1EC, unsigned short) & 0x7FF)], 23);
        strncpy((char *) &grub_mbr[0x800 + (valueat(grub_mbr45cp, 0x800 + 0x1EE, unsigned short) & 0x7FF)], (char *) &cur_mbr[0x800 + (valueat(grub_mbr45cp, 0x800 + 0x1EE, unsigned short) & 0x7FF)], 10);
        load_seg = valueat(cur_mbr, 0x400 + 0x1EA, unsigned short);
        if ((load_seg != valueat(cur_mbr, 0x600 + 0x1EA, unsigned short)) || (load_seg != valueat(cur_mbr, 0xA00 + 0x1EA, unsigned short)))
          goto try_another_mbr_1;
        valueat(grub_mbr, 0x400 + 0x1EA, unsigned short) = load_seg;
        valueat(grub_mbr, 0x600 + 0x1EA, unsigned short) = load_seg;
        valueat(grub_mbr, 0xA00 + 0x1EA, unsigned short) = load_seg;
        str_upcase(boot_file);
        if (!strcmp(boot_file, "GRLDR"))
          load_seg = 8192;
        else
        if (load_seg != 8192)
        {
          char buf[64];

          sprintf(buf, " --load-seg=%04x", load_seg);
          strcat(extra_par, buf);
        }
      }

      if (memcmp(grub_mbr, cur_mbr, slen) == 0)
      {
        sprintf(det_ver, " 0.4.5c [%s]", LoadText(IDS_ENHANCED_VERSION));
        SendDlgItemMessage(hWnd, IDC_CHOOSEVER, CB_SETCURSEL, (WPARAM) 0, 0);
        SwitchToInstallOrEdit(hWnd, FALSE, (idx < 0) || ((idx == 0) && (p_fstype[0] == FST_MBR)));
        if (!backup_mbr)
          CheckDlgButton(hWnd, IDC_NO_BACKUP_MBR, BST_CHECKED);
        if (disable_floppy)
          CheckDlgButton(hWnd, IDC_DISABLE_FLOPPY, BST_CHECKED);
        if (disable_osbr)
          CheckDlgButton(hWnd, IDC_DISABLE_OSBR, BST_CHECKED);
        if (!prevmbr_last)
          CheckDlgButton(hWnd, IDC_PREVMBR_FIRST, BST_CHECKED);
        if (silent_boot)
          CheckDlgButton(hWnd, IDC_SILENT, BST_CHECKED);
        if (chs_no_tune)
          CheckDlgButton(hWnd, IDC_CHS_NO_TUNE, BST_CHECKED);
        if (r_bsfp)
          CheckDlgButton(hWnd, IDC_COPY_BPB, BST_CHECKED);
        SetDlgItemText(hWnd, IDC_BOOTFILE, boot_file);
        if (strcmp(extra_par, ""))
          SetDlgItemText(hWnd, IDC_EXTRA, &extra_par[1]);
        if (((hot_key == 0x3920) && strcmp(lc_key_name, "space")) || (hot_key != 0x3920))
          SetDlgItemText(hWnd, IDC_HOTKEY, get_keyname(hot_key));
        if (time_out != 0)
        {
          char buf[64] = "";

          sprintf(buf, "%d", time_out);
          SetDlgItemText(hWnd, IDC_TIMEOUT, buf);
        }
        return;
      }

 try_another_mbr_1:

      slen = sizeof(grub_mbr45cnp);
      memcpy(grub_mbr, grub_mbr45cnp, slen);

      load_seg = 8192;
      strcpy(boot_file, "");
      strcpy(boot_file_83, "");
      strcpy(key_name, "");
      strcpy(lc_key_name, "");
      strcpy(extra_par, "");

      hot_key     = time_out = gfg = part_num = def_drive = def_part = -1;
      silent_boot = disable_floppy = disable_osbr = prevmbr_last = chs_no_tune = FALSE;

      if (r_bsfp)
        memcpy(&grub_mbr[0xB], &bsfp[0xB], sln);
      if (backup_mbr)
        memcpy(&grub_mbr[512], &cur_mbr[512], 512);
      else
        memset(&grub_mbr[512], 0, 512);

      if ((cur_mbr[0xA00 + (valueat(grub_mbr45cp, 0xA00 + 0x1EC, unsigned short) & 0x7FF) - 3] == 0) && (cur_mbr[0xA00 + (valueat(grub_mbr45cp, 0xA00 + 0x1EC, unsigned short) & 0x7FF) - 2] == 0) && (cur_mbr[0x400 + (valueat(grub_mbr45cp, 0x400 + 0x1EC, unsigned short) & 0x7FF) - 3] == 0) && (cur_mbr[0x400 + (valueat(grub_mbr45cp, 0x400 + 0x1EC, unsigned short) & 0x7FF) - 2] == 0) && (cur_mbr[0x600 + (valueat(grub_mbr45cp, 0x600 + 0x1EC, unsigned short) & 0x7FF) - 3] == 0) && (cur_mbr[0x600 + (valueat(grub_mbr45cp, 0x600 + 0x1EC, unsigned short) & 0x7FF) - 2] == 0) && (cur_mbr[0x800 + (valueat(grub_mbr45cp, 0x800 + 0x1EE, unsigned short) & 0x7FF) - 3] == 0) && (cur_mbr[0x800 + (valueat(grub_mbr45cp, 0x800 + 0x1EE, unsigned short) & 0x7FF) - 2] == 0))
      {
        for (i = 0x1EA7; i <= (0x1EA7 + 74); i++)
          if (cur_mbr[i] != 0)
            break;
        if (i > (0x1EA7 + 74))
        {
          for (i = 0x1F30; i <= (0x1F30 + 98); i++)
            if (cur_mbr[i] != 0)
              break;
          if (i > (0x1F30 + 98))
          {
            for (i = 0x22D2; i <= (0x22D2 + 32); i++)
              if (cur_mbr[i] != 0)
                break;
            if (i > (0x22D2 + 32))
            {
              memset(&grub_mbr[0xA00 + (valueat(grub_mbr45cp, 0xA00 + 0x1EC, unsigned short) & 0x7FF) - 3], 0, 2);
              memset(&grub_mbr[0x400 + (valueat(grub_mbr45cp, 0x400 + 0x1EC, unsigned short) & 0x7FF) - 3], 0, 2);
              memset(&grub_mbr[0x600 + (valueat(grub_mbr45cp, 0x600 + 0x1EC, unsigned short) & 0x7FF) - 3], 0, 2);
              memset(&grub_mbr[0x800 + (valueat(grub_mbr45cp, 0x800 + 0x1EE, unsigned short) & 0x7FF) - 3], 0, 2);
              memset(&grub_mbr[0x1EA7], 0, 74);
              memset(&grub_mbr[0x1F30], 0, 98);
              memset(&grub_mbr[0x22D2], 0, 32);
              silent_boot = TRUE;
            }
          }
        }
      }
      valueat(grub_mbr, 2, unsigned char)  = gfg = valueat(cur_mbr, 2, unsigned char);
      valueat(grub_mbr, 3, unsigned char)  = time_out = valueat(cur_mbr, 3, unsigned char);
      valueat(grub_mbr, 4, unsigned short) = hot_key = valueat(cur_mbr, 4, unsigned short);
      valueat(grub_mbr, 6, unsigned char)  = def_drive = valueat(cur_mbr, 6, unsigned char);
      if (def_drive < 255)
      {
        char buf[64];

        sprintf(buf, " --preferred-drive=%d", def_drive);
        strcat(extra_par, buf);
      }
      valueat(grub_mbr, 7, unsigned char) = def_part = valueat(cur_mbr, 7, unsigned char);
      if (def_part < 255)
      {
        char buf[64];

        sprintf(buf, " --preferred-partition=%d", def_part);
        strcat(extra_par, buf);
      }
      strcpy(key_name, (char *) &cur_mbr[0x1FE8]);
      strcpy(lc_key_name, key_name);
      str_lowcase(lc_key_name);
      if (strcmp(str_lowcase(get_keyname(hot_key)), lc_key_name) && (hot_key != 0) && strcmp(key_name, ""))
      {
        strcat(extra_par, " --key-name=");
        strcat(extra_par, key_name);
      }
      memcpy(&grub_mbr[0x1B8], &cur_mbr[0x1B8], 72);
      memcpy(&grub_mbr[0x1FE8], &cur_mbr[0x1FE8], 13);
      disable_floppy = gfg & GFG_DISABLE_FLOPPY;
      disable_osbr   = gfg & GFG_DISABLE_OSBR;
      if (gfg & GFG_DUCE)
        chs_no_tune = TRUE;
      if (gfg & CFG_CHS_NO_TUNE)
        strcat(extra_par, " --chs-no-tune");
      prevmbr_last = gfg & GFG_PREVMBR_LAST;
      strncpy(boot_file, (char *) &cur_mbr[0xA00 + (valueat(grub_mbr45cp, 0xA00 + 0x1EC, unsigned short) & 0x7FF)], 24);
      if (!strcmp(boot_file, ""))
        return;
      else
      {
        char* pc;

        pc = strchr(&boot_file[0], '.');
        if (((pc) && (((pc == &boot_file[0]) || (pc - &boot_file[0] > 8) || (strlen(pc + 1) > 3)))) || ((!pc) && (strlen(boot_file) > 8)))
          goto unsupported_045c;
        str_upcase(boot_file);
        memset(boot_file_83, ' ', sizeof(boot_file_83) - 1);
        if (pc)
        {
          memcpy(boot_file_83, boot_file, pc - &boot_file[0]);
          memcpy(&boot_file_83[8], pc + 1, strlen(pc + 1));
        }
        else
          memcpy(boot_file_83, boot_file, strlen(boot_file));
        str_lowcase(boot_file);
        char buf[64];

        strncpy(buf, (char *) &cur_mbr[0x400 + (valueat(grub_mbr45cp, 0x400 + 0x1EC, unsigned short) & 0x7FF)], 24);
        if (strcmp(boot_file_83, buf))
          goto unsupported_045c;
        strncpy(buf, (char *) &cur_mbr[0x600 + (valueat(grub_mbr45cp, 0x600 + 0x1EC, unsigned short) & 0x7FF)], 24);
        if (strcmp(boot_file_83, buf))
          goto unsupported_045c;
        strncpy(buf, (char *) &cur_mbr[0x800 + (valueat(grub_mbr45cp, 0x800 + 0x1EE, unsigned short) & 0x7FF)], 11);
        if (strcmp(boot_file, buf))
          goto unsupported_045c;
        strncpy((char *) &grub_mbr[0xA00 + (valueat(grub_mbr45cp, 0xA00 + 0x1EC, unsigned short) & 0x7FF)], (char *) &cur_mbr[0xA00 + (valueat(grub_mbr45cp, 0xA00 + 0x1EC, unsigned short) & 0x7FF)], 23);
        strncpy((char *) &grub_mbr[0x400 + (valueat(grub_mbr45cp, 0x400 + 0x1EC, unsigned short) & 0x7FF)], (char *) &cur_mbr[0x400 + (valueat(grub_mbr45cp, 0x400 + 0x1EC, unsigned short) & 0x7FF)], 23);
        strncpy((char *) &grub_mbr[0x600 + (valueat(grub_mbr45cp, 0x600 + 0x1EC, unsigned short) & 0x7FF)], (char *) &cur_mbr[0x600 + (valueat(grub_mbr45cp, 0x600 + 0x1EC, unsigned short) & 0x7FF)], 23);
        strncpy((char *) &grub_mbr[0x800 + (valueat(grub_mbr45cp, 0x800 + 0x1EE, unsigned short) & 0x7FF)], (char *) &cur_mbr[0x800 + (valueat(grub_mbr45cp, 0x800 + 0x1EE, unsigned short) & 0x7FF)], 10);
        load_seg = valueat(cur_mbr, 0x400 + 0x1EA, unsigned short);
        if ((load_seg != valueat(cur_mbr, 0x600 + 0x1EA, unsigned short)) || (load_seg != valueat(cur_mbr, 0xA00 + 0x1EA, unsigned short)))
          goto try_another_mbr_2;
        valueat(grub_mbr, 0x400 + 0x1EA, unsigned short) = load_seg;
        valueat(grub_mbr, 0x600 + 0x1EA, unsigned short) = load_seg;
        valueat(grub_mbr, 0xA00 + 0x1EA, unsigned short) = load_seg;
        str_upcase(boot_file);
        if (!strcmp(boot_file, "GRLDR"))
          load_seg = 8192;
        else
        if (load_seg != 8192)
        {
          char buf[64];

          sprintf(buf, " --load-seg=%04x", load_seg);
          strcat(extra_par, buf);
        }
      }

      if (memcmp(grub_mbr, cur_mbr, slen) == 0)
      {
        sprintf(det_ver, " 0.4.5c [%s]", LoadText(IDS_NORMAL_VERSION));
        SendDlgItemMessage(hWnd, IDC_CHOOSEVER, CB_SETCURSEL, (WPARAM) 0, 0);
        SwitchToInstallOrEdit(hWnd, FALSE, (idx < 0) || ((idx == 0) && (p_fstype[0] == FST_MBR)));
        if (!backup_mbr)
          CheckDlgButton(hWnd, IDC_NO_BACKUP_MBR, BST_CHECKED);
        if (disable_floppy)
          CheckDlgButton(hWnd, IDC_DISABLE_FLOPPY, BST_CHECKED);
        if (disable_osbr)
          CheckDlgButton(hWnd, IDC_DISABLE_OSBR, BST_CHECKED);
        if (!prevmbr_last)
          CheckDlgButton(hWnd, IDC_PREVMBR_FIRST, BST_CHECKED);
        if (silent_boot)
          CheckDlgButton(hWnd, IDC_SILENT, BST_CHECKED);
        if (chs_no_tune)
          CheckDlgButton(hWnd, IDC_CHS_NO_TUNE, BST_CHECKED);
        if (r_bsfp)
          CheckDlgButton(hWnd, IDC_COPY_BPB, BST_CHECKED);
        SetDlgItemText(hWnd, IDC_BOOTFILE, boot_file);
        if (strcmp(extra_par, ""))
          SetDlgItemText(hWnd, IDC_EXTRA, &extra_par[1]);
        if (((hot_key == 0x3920) && strcmp(lc_key_name, "space")) || (hot_key != 0x3920))
          SetDlgItemText(hWnd, IDC_HOTKEY, get_keyname(hot_key));
        if (time_out != 0)
        {
          char buf[64] = "";

          sprintf(buf, "%d", time_out);
          SetDlgItemText(hWnd, IDC_TIMEOUT, buf);
        }
        return;
      }

 unsupported_045c:

      sprintf(det_ver, " 0.4.5c [%s]", LoadText(IDS_UNSUPPORTED_VERSION));
      SendDlgItemMessage(hWnd, IDC_CHOOSEVER, CB_SETCURSEL, (WPARAM) 0, 0);
      SwitchToInstallOrEdit(hWnd, TRUE, (idx < 0) || ((idx == 0) && (p_fstype[0] == FST_MBR)));
      return;

 try_another_mbr_2:

      slen = sizeof(grub_mbr46anp);
      memcpy(grub_mbr, grub_mbr46anp, slen);

      strcpy(boot_file, "");
      strcpy(boot_file_83, "");
      strcpy(key_name, "");
      strcpy(lc_key_name, "");
      strcpy(extra_par, "");

      hot_key     = time_out = gfg = part_num = -1;
      silent_boot = disable_floppy = disable_osbr = prevmbr_last = chs_no_tune = FALSE;

      if (r_bsfp)
        memcpy(&grub_mbr[0xB], &bsfp[0xB], sln);
      if (backup_mbr)
        memcpy(&grub_mbr[512], &cur_mbr[512], 512);
      else
        memset(&grub_mbr[512], 0, 512);

      if ((memcmp(&cur_mbr[0x5E0], "\x0\x0", 2) == 0) && (memcmp(&cur_mbr[0x1040], "\x0\x0", 2) == 0))
      {
        for (i = 0x1BC1; i <= (0x1BC1 + 66); i++)
          if (cur_mbr[i] != 0)
            break;
        if (i > (0x1BC1 + 66))
        {
          for (i = 0x1CAD; i <= (0x1CAD + 12); i++)
            if (cur_mbr[i] != 0)
              break;
          if (i > (0x1CAD + 12))
          {
            for (i = 0x1C42; i <= (0x1C42 + 106); i++)
              if (cur_mbr[i] != 0)
                break;
            if (i > (0x1C42 + 106))
            {
              for (i = 0x1F74; i <= (0x1F74 + 32); i++)
                if (cur_mbr[i] != 0)
                  break;
              if (i > (0x1F74 + 32))
              {
                memset(&grub_mbr[0x5E0], 0, 2);
                memset(&grub_mbr[0x1040], 0, 2);
                memset(&grub_mbr[0x1BC1], 0, 66);
                memset(&grub_mbr[0x1CAD], 0, 12);
                memset(&grub_mbr[0x1C42], 0, 106);
                memset(&grub_mbr[0x1F74], 0, 32);
                silent_boot = TRUE;
              }
            }
          }
        }
      }

      valueat(grub_mbr, 0x5A, unsigned char)  = gfg = valueat(cur_mbr, 0x5A, unsigned char);
      valueat(grub_mbr, 0x5B, unsigned char)  = time_out = valueat(cur_mbr, 0x5B, unsigned char);
      valueat(grub_mbr, 0x5C, unsigned short) = hot_key = valueat(cur_mbr, 0x5C, unsigned short);
      valueat(grub_mbr, 0x5E, unsigned char)  = valueat(cur_mbr, 0x5E, unsigned char);
      valueat(grub_mbr, 0x5F, unsigned char)  = valueat(cur_mbr, 0x5F, unsigned char);
      strcpy(key_name, (char *) &cur_mbr[0x1CBA]);
      strcpy(lc_key_name, key_name);
      str_lowcase(lc_key_name);
      if (strcmp(str_lowcase(get_keyname(hot_key)), lc_key_name) && (hot_key != 0) && strcmp(key_name, ""))
      {
        strcat(extra_par, " --key-name=");
        strcat(extra_par, key_name);
      }
      memcpy(&grub_mbr[0x1B8], &cur_mbr[0x1B8], 72);
      memcpy(&grub_mbr[0x1CBA], &cur_mbr[0x1CBA], 13);
      disable_floppy = gfg & GFG_DISABLE_FLOPPY;
      disable_osbr   = gfg & GFG_DISABLE_OSBR;
      if (gfg & GFG_DUCE)
        strcat(extra_par, " --duce");
      if (gfg & CFG_CHS_NO_TUNE)
        chs_no_tune = TRUE;
      prevmbr_last = gfg & GFG_PREVMBR_LAST;
      strncpy(boot_file, (char *) &cur_mbr[0x5E3], 12);
      if (!strcmp(boot_file, ""))
        goto try_another_mbr_3;
      strncpy((char *) &grub_mbr[0x5E3], (char *) &cur_mbr[0x5E3], 12);

      if (memcmp(grub_mbr, cur_mbr, slen) == 0)
      {
        sprintf(det_ver, " 0.4.6a [%s]", LoadText(IDS_NORMAL_VERSION));
        SendDlgItemMessage(hWnd, IDC_CHOOSEVER, CB_SETCURSEL, (WPARAM) 1, 0);
        SwitchToInstallOrEdit(hWnd, FALSE, (idx < 0) || ((idx == 0) && (p_fstype[0] == FST_MBR)));
        if (!backup_mbr)
          CheckDlgButton(hWnd, IDC_NO_BACKUP_MBR, BST_CHECKED);
        if (disable_floppy)
          CheckDlgButton(hWnd, IDC_DISABLE_FLOPPY, BST_CHECKED);
        if (disable_osbr)
          CheckDlgButton(hWnd, IDC_DISABLE_OSBR, BST_CHECKED);
        if (!prevmbr_last)
          CheckDlgButton(hWnd, IDC_PREVMBR_FIRST, BST_CHECKED);
        if (silent_boot)
          CheckDlgButton(hWnd, IDC_SILENT, BST_CHECKED);
        if (chs_no_tune)
          CheckDlgButton(hWnd, IDC_CHS_NO_TUNE, BST_CHECKED);
        if (r_bsfp)
          CheckDlgButton(hWnd, IDC_COPY_BPB, BST_CHECKED);
        str_upcase(boot_file);
        SetDlgItemText(hWnd, IDC_BOOTFILE, boot_file);
        if (strcmp(extra_par, ""))
          SetDlgItemText(hWnd, IDC_EXTRA, &extra_par[1]);
        if (((hot_key == 0x3920) && strcmp(lc_key_name, "space")) || (hot_key != 0x3920))
          SetDlgItemText(hWnd, IDC_HOTKEY, get_keyname(hot_key));
        if (time_out != 0)
        {
          char buf[64] = "";

          sprintf(buf, "%d", time_out);
          SetDlgItemText(hWnd, IDC_TIMEOUT, buf);
        }

        return;
      }
      ;

 try_another_mbr_3:

      slen = sizeof(grub_mbr46ap);
      memcpy(grub_mbr, grub_mbr46ap, slen);

      strcpy(boot_file, "");
      strcpy(boot_file_83, "");
      strcpy(key_name, "");
      strcpy(lc_key_name, "");
      strcpy(extra_par, "");

      hot_key     = time_out = gfg = part_num = -1;
      silent_boot = disable_floppy = disable_osbr = prevmbr_last = chs_no_tune = FALSE;

      if (r_bsfp)
        memcpy(&grub_mbr[0xB], &bsfp[0xB], sln);
      if (backup_mbr)
        memcpy(&grub_mbr[512], &cur_mbr[512], 512);
      else
        memset(&grub_mbr[512], 0, 512);

      if ((memcmp(&cur_mbr[0x5E0], "\x0\x0", 2) == 0) && (memcmp(&cur_mbr[0x1040], "\x0\x0", 2) == 0))
      {
        for (i = 0x1BC1; i <= (0x1BC1 + 66); i++)
          if (cur_mbr[i] != 0)
            break;
        if (i > (0x1BC1 + 66))
        {
          for (i = 0x1CAD; i <= (0x1CAD + 12); i++)
            if (cur_mbr[i] != 0)
              break;
          if (i > (0x1CAD + 12))
          {
            for (i = 0x1C42; i <= (0x1C42 + 106); i++)
              if (cur_mbr[i] != 0)
                break;
            if (i > (0x1C42 + 106))
            {
              for (i = 0x1F74; i <= (0x1F74 + 32); i++)
                if (cur_mbr[i] != 0)
                  break;
              if (i > (0x1F74 + 32))
              {
                memset(&grub_mbr[0x5E0], 0, 2);
                memset(&grub_mbr[0x1040], 0, 2);
                memset(&grub_mbr[0x1BC1], 0, 66);
                memset(&grub_mbr[0x1CAD], 0, 12);
                memset(&grub_mbr[0x1C42], 0, 106);
                memset(&grub_mbr[0x1F74], 0, 32);
                silent_boot = TRUE;
              }
            }
          }
        }
      }

      valueat(grub_mbr, 0x5A, unsigned char)  = gfg = valueat(cur_mbr, 0x5A, unsigned char);
      valueat(grub_mbr, 0x5B, unsigned char)  = time_out = valueat(cur_mbr, 0x5B, unsigned char);
      valueat(grub_mbr, 0x5C, unsigned short) = hot_key = valueat(cur_mbr, 0x5C, unsigned short);
      valueat(grub_mbr, 0x5E, unsigned char)  = valueat(cur_mbr, 0x5E, unsigned char);
      valueat(grub_mbr, 0x5F, unsigned char)  = valueat(cur_mbr, 0x5F, unsigned char);
      strcpy(key_name, (char *) &cur_mbr[0x1CBA]);
      strcpy(lc_key_name, key_name);
      str_lowcase(lc_key_name);
      if (strcmp(str_lowcase(get_keyname(hot_key)), lc_key_name) && (hot_key != 0) && strcmp(key_name, ""))
      {
        strcat(extra_par, " --key-name=");
        strcat(extra_par, key_name);
      }
      memcpy(&grub_mbr[0x1B8], &cur_mbr[0x1B8], 72);
      memcpy(&grub_mbr[0x1CBA], &cur_mbr[0x1CBA], 13);
      disable_floppy = gfg & GFG_DISABLE_FLOPPY;
      disable_osbr   = gfg & GFG_DISABLE_OSBR;
      if (gfg & GFG_DUCE)
        strcat(extra_par, " --duce");
      if (gfg & CFG_CHS_NO_TUNE)
        chs_no_tune = TRUE;
      prevmbr_last = gfg & GFG_PREVMBR_LAST;
      strncpy(boot_file, (char *) &cur_mbr[0x5E3], 12);
      if (!strcmp(boot_file, ""))
        goto unsupported_046a;
      strncpy((char *) &grub_mbr[0x5E3], (char *) &cur_mbr[0x5E3], 12);

      if (memcmp(grub_mbr, cur_mbr, slen) == 0)
      {
        sprintf(det_ver, " 0.4.6a [%s]", LoadText(IDS_ENHANCED_VERSION));
        SendDlgItemMessage(hWnd, IDC_CHOOSEVER, CB_SETCURSEL, (WPARAM) 1, 0);
        SwitchToInstallOrEdit(hWnd, FALSE, (idx < 0) || ((idx == 0) && (p_fstype[0] == FST_MBR)));
        if (!backup_mbr)
          CheckDlgButton(hWnd, IDC_NO_BACKUP_MBR, BST_CHECKED);
        if (disable_floppy)
          CheckDlgButton(hWnd, IDC_DISABLE_FLOPPY, BST_CHECKED);
        if (disable_osbr)
          CheckDlgButton(hWnd, IDC_DISABLE_OSBR, BST_CHECKED);
        if (!prevmbr_last)
          CheckDlgButton(hWnd, IDC_PREVMBR_FIRST, BST_CHECKED);
        if (silent_boot)
          CheckDlgButton(hWnd, IDC_SILENT, BST_CHECKED);
        if (chs_no_tune)
          CheckDlgButton(hWnd, IDC_CHS_NO_TUNE, BST_CHECKED);
        if (r_bsfp)
          CheckDlgButton(hWnd, IDC_COPY_BPB, BST_CHECKED);
        str_upcase(boot_file);
        SetDlgItemText(hWnd, IDC_BOOTFILE, boot_file);
        if (strcmp(extra_par, ""))
          SetDlgItemText(hWnd, IDC_EXTRA, &extra_par[1]);
        if (((hot_key == 0x3920) && strcmp(lc_key_name, "space")) || (hot_key != 0x3920))
          SetDlgItemText(hWnd, IDC_HOTKEY, get_keyname(hot_key));
        if (time_out != 0)
        {
          char buf[64] = "";

          sprintf(buf, "%d", time_out);
          SetDlgItemText(hWnd, IDC_TIMEOUT, buf);
        }

        return;
      }
      ;

 unsupported_046a:

      sprintf(det_ver, " 0.4.6a [%s]", LoadText(IDS_UNSUPPORTED_VERSION));
      SendDlgItemMessage(hWnd, IDC_CHOOSEVER, CB_SETCURSEL, (WPARAM) 1, 0);
      SwitchToInstallOrEdit(hWnd, TRUE, TRUE);
      return;

 not_g4d:

      SwitchToInstallOrEdit(hWnd, TRUE, TRUE);
      return;
    }
    else
    if ((idx == 0) && (p_fstype[0] == FST_GPT))                   //GPT
    {
      sprintf(det_ver, "%s", LoadText(IDS_UNSUPPORTED));
      SwitchToInstallOrEdit(hWnd, TRUE, TRUE);
    }
    else                                          //PBR & FLOPPY
    {
      int     slen;
      BOOLEAN isFloppy   = (idx == 0) && ((p_fstype[0] != FST_MBR) && (p_fstype[0] != FST_GPT));
      BOOLEAN isObsolete = FALSE;

      switch (p_fstype[idx])
      {
      case FST_NTFS:
        slen = 2048;
        break;
      case FST_EXT2:
        slen = 2048;
        break;
      case FST_EXT3:
        slen = 2048;
        break;
      case FST_EXT4:
        slen = 2048;
        break;
      case FST_EXFAT:
        slen = 0x3000;
        break;
      default:
        slen = 512;
      }
      byte           cur_pbr[slen];
      byte           grub_pbr[slen + 1];
      xd_t           * xd;
      BOOLEAN        silent_boot;
      unsigned short load_seg;
      char           boot_file[64];
      char           extra_par[256];

      xd = xd_open(fn, 0, 0);
      if (xd == NULL)
      {
        SwitchToInstallOrEdit(hWnd, TRUE, (idx < 0) || ((idx == 0) && ((p_fstype[0] == FST_MBR) || (p_fstype[0] == FST_GPT))));
        return;
      }
      if (((p_start[idx] != xd->ofs) && (xd_seek(xd, p_start[idx]))) || (xd_read(xd, (char *) cur_pbr, sizeof(cur_pbr) >> 9)))
      {
        xd_close(xd);
        SwitchToInstallOrEdit(hWnd, TRUE, (idx < 0) || ((idx == 0) && ((p_fstype[0] == FST_MBR) || (p_fstype[0] == FST_GPT))));
        return;
      }
      xd_close(xd);

      silent_boot = FALSE;
      load_seg    = 8192;
      strcpy(boot_file, "");
      strcpy(extra_par, "");

      switch (p_fstype[idx])
      {
      case FST_NTFS:
      {
        memcpy(grub_pbr, &grub_mbr45cp[0xA00], slen);
        if (isFloppy)
          grub_pbr[0x57] = 255;
        else
        {
          grub_pbr[0x57]                         = p_index[idx];
          valueat(grub_pbr, 0x1C, unsigned long) = p_start[idx];
        }
        grub_pbr[2]                              = cur_pbr[2];
        load_seg                                 = valueat(cur_pbr, 0x1EA, unsigned short);
        valueat(grub_pbr, 0x1EA, unsigned short) = load_seg;
        memcpy(&grub_pbr[0x8], &cur_pbr[0x8], 0x54 - 0x8);
        int ofs;

        ofs = (valueat(grub_pbr, 0x1EC, unsigned short) & 0x7FF) - 3;
        if (ofs < (slen - 8))
        {
          if ((cur_pbr[ofs] == 0) && (cur_pbr[ofs + 1] == 0))
          {
            silent_boot       = TRUE;
            grub_pbr[ofs]     = 0;
            grub_pbr[ofs + 1] = 0;
          }
          if (cur_pbr[ofs + 3] == 0)
            goto try_another_pbr_1;
          byte *pc;

          pc = memchr(&cur_pbr[ofs + 3], 0, 30);
          if ((!pc) || ((pc - &cur_pbr[ofs + 3]) > 23))
            goto try_another_pbr_1;
          memcpy(boot_file, &cur_pbr[ofs + 3], pc - &cur_pbr[ofs + 3] + 1);
          memcpy(&grub_pbr[ofs + 3], &cur_pbr[ofs + 3], 23);
        }
        break;
      }
      case FST_FAT32:
      {
        isObsolete = strncmp((char *) &cur_pbr[0x3], "GRLDR", 5) == 0;
        if ((memcmp(&cur_pbr[0x3], "MSDOS", 5) != 0) && (memcmp(&cur_pbr[0x3], "MSWIN", 5) != 0) && !isObsolete)
          goto unknown_PBR;
        memcpy(grub_pbr, &grub_mbr45cp[0x400], slen);
        grub_pbr[0x2] = cur_pbr[0x2];
        if (isFloppy)
          grub_pbr[0x5D] = 255;
        else
        {
          grub_pbr[0x5D]                         = p_index[idx];
          valueat(grub_pbr, 0x1C, unsigned long) = p_start[idx];
        }
        memcpy(&grub_pbr[0x3], &cur_pbr[0x3], 0x5A - 0x3);
        load_seg                                 = valueat(cur_pbr, 0x1EA, unsigned short);
        valueat(grub_pbr, 0x1EA, unsigned short) = load_seg;
        int ofs;

        ofs = (valueat(grub_pbr, 0x1EC, unsigned short) & 0x7FF) - 3;
        if (ofs < (slen - 8))
        {
          if ((cur_pbr[ofs] == 0) && (cur_pbr[ofs + 1] == 0))
          {
            silent_boot       = TRUE;
            grub_pbr[ofs]     = 0;
            grub_pbr[ofs + 1] = 0;
          }
          if ((cur_pbr[ofs + 3] == 0) || (cur_pbr[ofs + 3] == ' ') || (cur_pbr[ofs + 3 + 11] != 0))
            goto try_another_pbr_1;
          char buf[12] = "";
          int  l;

          strncpy(buf, (char *) &cur_pbr[ofs + 3], 12);
          l = 7;
          while ((l >= 0) && (buf[l] == ' '))
            l--;
          if (l < 0)
            goto try_another_pbr_1;
          l++;
          memcpy(boot_file, buf, l);
          boot_file[l] = 0;
          l            = 10;
          while ((l >= 8) && (buf[l] == ' '))
            l--;
          buf[l + 1] = 0;
          if (l >= 8)
          {
            strcat(boot_file, ".");
            strcat(boot_file, &buf[8]);
          }
          memcpy(&grub_pbr[ofs + 3], &cur_pbr[ofs + 3], 11);
        }
        break;
      }
      case FST_FAT16:
      {
        isObsolete = strncmp((char *) &cur_pbr[0x3], "GRLDR", 5) == 0;
        if ((memcmp(&cur_pbr[0x3], "MSDOS", 5) != 0) && (memcmp(&cur_pbr[0x3], "MSWIN", 5) != 0) && !isObsolete)
          goto unknown_PBR;
        memcpy(grub_pbr, &grub_mbr45cp[0x600], slen);
        grub_pbr[0x2] = cur_pbr[0x2];
        if (isFloppy)
          grub_pbr[0x41] = 255;
        else
        {
          grub_pbr[0x41]                         = p_index[idx];
          valueat(grub_pbr, 0x1C, unsigned long) = p_start[idx];
        }
        memcpy(&grub_pbr[0x3], &cur_pbr[0x3], 0x3E - 0x3);
        load_seg                                 = valueat(cur_pbr, 0x1EA, unsigned short);
        valueat(grub_pbr, 0x1EA, unsigned short) = load_seg;
        int ofs;

        ofs = (valueat(grub_pbr, 0x1EC, unsigned short) & 0x7FF) - 3;
        if (ofs < (slen - 8))
        {
          if ((cur_pbr[ofs] == 0) && (cur_pbr[ofs + 1] == 0))
          {
            silent_boot       = TRUE;
            grub_pbr[ofs]     = 0;
            grub_pbr[ofs + 1] = 0;
          }
          if ((cur_pbr[ofs + 3] == 0) || (cur_pbr[ofs + 3] == ' ') || (cur_pbr[ofs + 3 + 11] != 0))
            goto try_another_pbr_1;
          char buf[12] = "";
          int  l;

          strncpy(buf, (char *) &cur_pbr[ofs + 3], 12);
          l = 7;
          while ((l >= 0) && (buf[l] == ' '))
            l--;
          if (l < 0)
            goto try_another_pbr_1;
          l++;
          memcpy(boot_file, buf, l);
          boot_file[l] = 0;
          l            = 10;
          while ((l >= 8) && (buf[l] == ' '))
            l--;
          buf[l + 1] = 0;
          if (l >= 8)
          {
            strcat(boot_file, ".");
            strcat(boot_file, &buf[8]);
          }
          memcpy(&grub_pbr[ofs + 3], &cur_pbr[ofs + 3], 11);
        }
        break;
      }
      case FST_EXT2:
      {
 all_ext:
        slen = 512;
        memcpy(grub_pbr, &grub_mbr45cp[0x800], slen);
        grub_pbr[2] = cur_pbr[2];
        if (isFloppy)
          grub_pbr[0x25] = 255;
        else
          grub_pbr[0x25] = p_index[idx];
        int           def_spt, def_hds;
        unsigned long def_tsc, def_ssc;
        char          buf[64];

        valueat(grub_pbr, 0x18, unsigned short) = def_spt = valueat(cur_pbr, 0x18, unsigned short);
        if ((def_spt > 0) && (def_spt != hdspt[ic]))
        {
          sprintf(buf, " --sectors-per-track=%d", def_spt);
          strcat(extra_par, buf);
        }
        valueat(grub_pbr, 0x1A, unsigned short) = def_hds = valueat(cur_pbr, 0x1A, unsigned short);
        if ((def_hds > 0) && (def_hds != hdh[ic]))
        {
          sprintf(buf, " --heads=%d", def_hds);
          strcat(extra_par, buf);
        }
        valueat(grub_pbr, 0x20, unsigned long) = def_tsc = valueat(cur_pbr, 0x20, unsigned long);
        if ((def_tsc > 0) && (def_tsc != (unsigned long) (p_size[idx] / hdbps[ic])))
        {
          sprintf(buf, " --total-sectors=%lu", def_tsc);
          strcat(extra_par, buf);
        }
        valueat(grub_pbr, 0x1C, unsigned long) = def_ssc = valueat(cur_pbr, 0x1C, unsigned long);
        if ((def_ssc > 0) && (def_ssc != ((unsigned long) p_start[idx])))
        {
          sprintf(buf, " --start-sector=%lu", def_ssc);
          strcat(extra_par, buf);
        }
        valueat(grub_pbr, 0x26, unsigned int)  = valueat(cur_pbr[1024], 0x58, unsigned int);
        valueat(grub_pbr, 0x28, unsigned long) = valueat(cur_pbr[1024], 0x28, unsigned long);
        valueat(grub_pbr, 0x2C, unsigned long) = valueat(cur_pbr[1024], 0x14, unsigned long) + 1;
        unsigned long bpb;

        bpb                                    = 1024 << valueat(cur_pbr[1024], 0x18, unsigned long);
        valueat(grub_pbr, 0xE, unsigned int)   = bpb;
        valueat(grub_pbr, 0xD, unsigned short) = (unsigned short) (bpb / hdbps[ic]);
        valueat(grub_pbr, 0x14, unsigned long) = (unsigned long) bpb / 4;
        valueat(grub_pbr, 0x10, unsigned long) = (unsigned long) (bpb / 4) * (bpb / 4);
        load_seg                               = 8192;
        int ofs;

        ofs = (valueat(grub_pbr, 0x1EE, unsigned short) & 0x7FF) - 3;
        if (ofs < (slen - 8))
        {
          if ((cur_pbr[ofs] == 0) && (cur_pbr[ofs + 1] == 0))
          {
            silent_boot       = TRUE;
            grub_pbr[ofs]     = 0;
            grub_pbr[ofs + 1] = 0;
          }
          if (cur_pbr[ofs + 4] == 0)
            goto try_another_pbr_1;
          byte *pc;

          pc = memchr(&cur_pbr[ofs + 3], 0, 20);
          if ((!pc) || ((pc - &cur_pbr[ofs + 3]) > 11))
            goto unknown_PBR;
          memcpy(boot_file, &cur_pbr[ofs + 3], pc - &cur_pbr[ofs + 3] + 1);
          memcpy(&grub_pbr[ofs + 3], &cur_pbr[ofs + 3], 11);
        }
        break;
      }
      case FST_EXT3:
        goto all_ext;
      case FST_EXT4:
        goto all_ext;
      }

      if (load_seg != 8192)
      {
        char buf[64];

        sprintf(buf, " --load-seg=%04x", load_seg);
        strcat(extra_par, buf);
      }

      if (memcmp(grub_pbr, cur_pbr, slen) == 0)
      {
        if (isObsolete)
          sprintf(det_ver, " [%s]", LoadText(IDS_OBSOLETE_VERSION));
        else
          strcpy(det_ver, " 0.4.5c");
        SendDlgItemMessage(hWnd, IDC_CHOOSEVER, CB_SETCURSEL, (WPARAM) 0, 0);
        SwitchToInstallOrEdit(hWnd, FALSE, (idx < 0) || ((idx == 0) && ((p_fstype[0] == FST_MBR) || (p_fstype[0] == FST_GPT))));
        if (silent_boot)
          CheckDlgButton(hWnd, IDC_SILENT, BST_CHECKED);
        str_upcase(boot_file);
        SetDlgItemText(hWnd, IDC_BOOTFILE, boot_file);
        if (strcmp(extra_par, ""))
          SetDlgItemText(hWnd, IDC_EXTRA, &extra_par[1]);
        return;
      }
 try_another_pbr_1:

      silent_boot = FALSE;
      strcpy(boot_file, "");
      strcpy(extra_par, "");

      switch (p_fstype[idx])
      {
      case FST_NTFS:
      {
        memcpy(grub_pbr, &grub_pbr46al[0xC00], slen);
        grub_pbr[2] = cur_pbr[2];
        memcpy(&grub_pbr[0x8], &cur_pbr[0x8], 0x54 - 0x8);
        if ((cur_pbr[0x1E0] == 0) && (cur_pbr[0x1E1] == 0))
        {
          silent_boot     = TRUE;
          grub_pbr[0x1E0] = 0;
          grub_pbr[0x1E1] = 0;
        }
        if (cur_pbr[0x1E3] == 0)
          goto unknown_PBR;
        byte *pc;

        pc = memchr(&cur_pbr[0x1E3], 0, 20);
        if ((!pc) || ((pc - &cur_pbr[0x1E3]) > 11))
          goto unknown_PBR;
        memcpy(boot_file, &cur_pbr[0x1E3], pc - &cur_pbr[0x1E3] + 1);
        memcpy(&grub_pbr[0x1E3], &cur_pbr[0x1E3], 11);
        break;
      }
      case FST_EXFAT:
      {
        slen = 1024;
        memcpy(grub_pbr, &grub_pbr46al[0x800], slen);
        memcpy(&grub_pbr[slen], &cur_pbr[slen], 0x200 * 11 - slen);
        memcpy(&grub_pbr[0x200 * 12], &grub_pbr46al[0x800], slen);
        memcpy(&grub_pbr[0x200 * 12 + slen], &cur_pbr[0x200 * 12 + slen], 0x200 * 11 - slen);
        grub_pbr[2] = cur_pbr[2];
        memcpy(&grub_pbr[0x8], &cur_pbr[0x8], 0x78 - 0x8);
        memcpy(&grub_pbr[0x200 * 12 + 0x8], &cur_pbr[0x200 * 12 + 0x8], 0x78 - 0x8);

        if ((cur_pbr[0x1E0] == 0) && (cur_pbr[0x1E1] == 0) && (cur_pbr[0x1800 + 0x1E0] == 0) && (cur_pbr[0x1800 + 0x1E1] == 0))
        {
          silent_boot                  = TRUE;
          grub_pbr[0x1E0]              = 0;
          grub_pbr[0x1E1]              = 0;
          grub_pbr[0x200 * 12 + 0x1E0] = 0;
          grub_pbr[0x200 * 12 + 0x1E1] = 0;
        }
        if (memcmp(&cur_pbr[0x1E3], &cur_pbr[0x200 * 12 + 0x1E3], 12) != 0)
          goto unknown_PBR;
        if (cur_pbr[0x1E4] == 0)
          goto unknown_PBR;
        byte *pc;

        pc = memchr(&cur_pbr[0x1E3], 0, 20);
        if ((!pc) || ((pc - &cur_pbr[0x1E3]) > 12))
          goto unknown_PBR;
        strncpy(boot_file, (char *) &cur_pbr[0x1E3], pc - &cur_pbr[0x1E3] + 1);
        memcpy(&grub_pbr[0x1E3], &cur_pbr[0x1E3], 12);
        memcpy(&grub_pbr[0x200 * 12 + 0x1E3], &cur_pbr[0x200 * 12 + 0x1E3], 12);
        int    i;
        UINT32 checksum = 0;
        for (i = 0; i < (0x200 * 11); i++)
        {
          if (i == 106 || i == 107 || i == 112)
            continue;
          checksum = ((checksum << 31) | (checksum >> 1)) + grub_pbr[i];
        }
        for (i = 0x200 * 11; i < (0x200 * 12); i += 4)
          valueat(grub_pbr, i, UINT32) = checksum;
        checksum = 0;
        for (i = 0x200 * 12; i < (0x200 * 23); i++)
        {
          if (i == ((0x200 * 12) + 106) || i == ((0x200 * 12) + 107) || i == ((0x200 * 12) + 112))
            continue;
          checksum = ((checksum << 31) | (checksum >> 1)) + grub_pbr[i];
        }
        for (i = 0x200 * 23; i < (0x200 * 24); i += 4)
          valueat(grub_pbr, i, UINT32) = checksum;
        slen = 0x3000;
        break;
      }
      case FST_FAT32:
      {
        memcpy(grub_pbr, grub_pbr46al, slen);
        memcpy(&grub_pbr[0x3], &cur_pbr[0x3], 0x5A - 0x3);
        if ((cur_pbr[0x1E0] == 0) && (cur_pbr[0x1E1] == 0))
        {
          silent_boot     = TRUE;
          grub_pbr[0x1E0] = 0;
          grub_pbr[0x1E1] = 0;
        }
        if ((cur_pbr[0x1E3] == 0) || (cur_pbr[0x1E3] == ' ') || (cur_pbr[0x1E3 + 11] != 0))
          goto unknown_PBR;
        char buf[12] = "";
        int  l;

        strncpy(buf, (char *) &cur_pbr[0x1E3], 12);
        l = 7;
        while ((l >= 0) && (buf[l] == ' '))
          l--;
        if (l < 0)
          goto unknown_PBR;
        l++;
        memcpy(boot_file, buf, l);
        boot_file[l] = 0;
        l            = 10;
        while ((l >= 8) && (buf[l] == ' '))
          l--;
        buf[l + 1] = 0;
        if (l >= 8)
        {
          strcat(boot_file, ".");
          strcat(boot_file, &buf[8]);
        }
        memcpy(&grub_pbr[0x1E3], &cur_pbr[0x1E3], 11);
        break;
      }
      case FST_FAT16:
      {
        memcpy(grub_pbr, &grub_pbr46al[0x200], slen);
        grub_pbr[0x2] = cur_pbr[0x2];
        memcpy(&grub_pbr[0x3], &cur_pbr[0x3], 0x3E - 0x3);
        if ((cur_pbr[0x1E3] == 0) || (cur_pbr[0x1E3] == ' ') || (cur_pbr[0x1E3 + 11] != 0))
          goto unknown_PBR;
        char buf[12] = "";
        int  l;

        strncpy(buf, (char *) &cur_pbr[0x1E3], 12);
        l = 7;
        while ((l >= 0) && (buf[l] == ' '))
          l--;
        if (l < 0)
          goto unknown_PBR;
        l++;
        memcpy(boot_file, buf, l);
        boot_file[l] = 0;
        l            = 10;
        while ((l >= 8) && (buf[l] == ' '))
          l--;
        buf[l + 1] = 0;
        if (l >= 8)
        {
          strcat(boot_file, ".");
          strcat(boot_file, &buf[8]);
        }
        memcpy(&grub_pbr[0x1E3], &cur_pbr[0x1E3], 11);
        break;
      }
      case FST_EXT2:
      {
 all_ext2:
        slen = 1024;
        memcpy(grub_pbr, &grub_pbr46al[0x400], slen);
        if ((cur_pbr[0x1E0] == 0) && (cur_pbr[0x1E1] == 0))
        {
          silent_boot     = TRUE;
          grub_pbr[0x1E0] = 0;
          grub_pbr[0x1E1] = 0;
        }
        if (cur_pbr[0x1E3] == 0)
          goto unknown_PBR;
        byte *pc;

        pc = memchr(&cur_pbr[0x1E3], 0, 20);
        if ((!pc) || ((pc - &cur_pbr[0x1E3]) > 11))
          goto unknown_PBR;
        memcpy(boot_file, &cur_pbr[0x1E3], pc - &cur_pbr[0x1E3] + 1);
        memcpy(&grub_pbr[0x1E3], &cur_pbr[0x1E3], 11);
        break;
      }
      case FST_EXT3:
        goto all_ext2;
      case FST_EXT4:
        goto all_ext2;
      }

      if (memcmp(grub_pbr, cur_pbr, slen) == 0)
      {
        if (isObsolete)
          sprintf(det_ver, " [%s]", LoadText(IDS_OBSOLETE_VERSION));
        else
          strcpy(det_ver, " 0.4.6a");
        SendDlgItemMessage(hWnd, IDC_CHOOSEVER, CB_SETCURSEL, (WPARAM) 1, 0);
        SwitchToInstallOrEdit(hWnd, FALSE, (idx < 0) || ((idx == 0) && ((p_fstype[0] == FST_MBR) || (p_fstype[0] == FST_GPT))));
        if (silent_boot)
          CheckDlgButton(hWnd, IDC_SILENT, BST_CHECKED);
        str_upcase(boot_file);
        SetDlgItemText(hWnd, IDC_BOOTFILE, boot_file);
        if (strcmp(extra_par, ""))
          SetDlgItemText(hWnd, IDC_EXTRA, &extra_par[1]);
        return;
      }

 unknown_PBR:

      SwitchToInstallOrEdit(hWnd, TRUE, (idx < 0) || ((idx == 0) && ((p_fstype[0] == FST_MBR) || (p_fstype[0] == FST_GPT))));
    }
  }
}

void ChangeBtnWidth(HWND hWnd, int btnID, int val)
{
  RECT  buttonScreenRect;
  GetWindowRect(GetDlgItem(hWnd, btnID), &buttonScreenRect);
  POINT buttonClientPoint;
  buttonClientPoint.x = buttonScreenRect.left;
  buttonClientPoint.y = buttonScreenRect.top;
  ScreenToClient(hWnd, &buttonClientPoint);
  MoveWindow(GetDlgItem(hWnd, btnID), buttonClientPoint.x - val / 2, buttonClientPoint.y, buttonScreenRect.right - buttonScreenRect.left + val, buttonScreenRect.bottom - buttonScreenRect.top, TRUE);
}


BOOL CALLBACK DialogOutputProc(HWND hOWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
  switch (wMsg)
  {
  case WM_CLOSE:
  {
    EndDialog(hOWnd, 0);
    break;
  }
  case WM_CONTEXTMENU:
  {
    if ((HWND) wParam != GetDlgItem(hOWnd, IDC_RICHEDIT))
      break;
    HMENU           hMenu;
    CHARRANGE       cr;
    int             len;
    GETTEXTLENGTHEX tls;

    hMenu = CreatePopupMenu();
    SendMessage((HWND) wParam, EM_EXGETSEL, 0, (LPARAM) &cr);
    if (comp_tab_output[IDC_COPY - IDC_COUNT_MAIN])
    {
      if (cr.cpMax > cr.cpMin)
        InsertMenu(hMenu, 0, MF_STRING | MF_BYPOSITION, IDC_COPY, (LPCSTR) comp_tab_output[IDC_COPY - IDC_COUNT_MAIN]);
      else
        InsertMenu(hMenu, 0, MF_STRING | MF_BYPOSITION | MF_GRAYED, IDC_COPY, (LPCSTR) comp_tab_output[IDC_COPY - IDC_COUNT_MAIN]);
    }
    else
    {
      if (cr.cpMax > cr.cpMin)
        InsertMenu(hMenu, 0, MF_STRING | MF_BYPOSITION, IDC_COPY, (LPCSTR) "Copy");
      else
        InsertMenu(hMenu, 0, MF_STRING | MF_BYPOSITION | MF_GRAYED, IDC_COPY, (LPCSTR) "Copy");
    }
    tls.flags    = GTL_NUMCHARS;
    tls.codepage = CP_ACP;
    len          = SendMessage((HWND) wParam, EM_GETTEXTLENGTHEX, (WPARAM) &tls, (LPARAM) 0);
    if (comp_tab_output[IDC_SELALL - IDC_COUNT_MAIN])
    {
      if ((cr.cpMin == 0) && (cr.cpMax == (len + 1)))
        InsertMenu(hMenu, 1, MF_STRING | MF_BYPOSITION | MF_GRAYED, IDC_SELALL, (LPCSTR) comp_tab_output[IDC_SELALL - IDC_COUNT_MAIN]);
      else
        InsertMenu(hMenu, 1, MF_STRING | MF_BYPOSITION, IDC_SELALL, (LPCSTR) comp_tab_output[IDC_SELALL - IDC_COUNT_MAIN]);
    }
    else
    {
      if ((cr.cpMin == 0) && (cr.cpMax == (len + 1)))
        InsertMenu(hMenu, 1, MF_STRING | MF_BYPOSITION | MF_GRAYED, IDC_SELALL, (LPCSTR) "Select all");
      else
        InsertMenu(hMenu, 1, MF_STRING | MF_BYPOSITION, IDC_SELALL, (LPCSTR) "Select all");
    }
    TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hOWnd, NULL);
    DestroyMenu(hMenu);
    break;
  }
  case WM_INITDIALOG:
  {
    HICON hIcon;
    char  title[30];
    char  buf[30];
    HWND  hWnd;
    int   i;

    for (i = IDC_COUNT_MAIN; i < (IDC_COUNT_MAIN + IDC_COUNT_OUTPUT); i++)
      if (comp_tab_output[i - IDC_COUNT_MAIN])
        SetDlgItemText(hOWnd, i, comp_tab_output[i - IDC_COUNT_MAIN]);
    hWnd = GetParent(hOWnd);
    if (lParam == 0)
      GetDlgItemText(hWnd, IDC_TEST, buf, sizeof(buf));
    else
      GetDlgItemText(hWnd, IDC_INSTALL, buf, sizeof(buf));
    sprintf(title, "%s %s", LoadText(IDS_OUTPUT), buf);
    SendMessage(hOWnd, WM_SETTEXT, 0, (LPARAM) title);
    hIcon = LoadIcon(hInst, MAKEINTRESOURCE(ICO_MAIN));
    SendMessage(hOWnd, WM_SETICON, ICON_BIG, (LPARAM) hIcon);
    RECT winParScreenRect, winScreenRect;

    GetWindowRect(hWnd, &winParScreenRect);
    GetWindowRect(hOWnd, &winScreenRect);
    MoveWindow(hOWnd, (int) (winParScreenRect.left + (winParScreenRect.right - winParScreenRect.left) / 2 - (winScreenRect.right - winScreenRect.left) / 2), (int) (winScreenRect.top + (winScreenRect.bottom - winScreenRect.top) / 2 - (winScreenRect.bottom - winScreenRect.top) / 2), winScreenRect.right - winScreenRect.left, winScreenRect.bottom - winScreenRect.top, FALSE);

    char fn[512], *pb, sfn[512];
    char temp[64];
    int  len;
    char osdrv[7] = "";

    if (lParam == 1)
    {
      DWORD                 nr;
      char                  vl[24];
      STORAGE_DEVICE_NUMBER d1;
      HANDLE                hd;
      char                  osdrvl[3] = "\1\1";

      if (GetEnvironmentVariable("SystemDrive", osdrvl, sizeof(osdrvl)))
        for (i = (byte) 'C'; i <= (byte) 'Z'; i++)
          if ((char) i == osdrvl[0])
          {
            sprintf(vl, "\\\\.\\%c:", (char) i);
            hd = CreateFile(vl, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
            if (hd == INVALID_HANDLE_VALUE)
              break;

            if (!DeviceIoControl(hd, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &d1, sizeof(d1), &nr, 0))
            {
              CloseHandle(hd);
              break;
            }
            if (d1.DeviceType == FILE_DEVICE_DISK)
              sprintf(osdrv, "(hd%d)", (int) d1.DeviceNumber);
            CloseHandle(hd);
            break;
          }
    }

    GetModuleFileName(hInst, fn, sizeof(fn));
    strcpy(sfn, fn);
    pb = strrchr(sfn, '\\');
    if (!pb)
      pb = sfn;
    else
      pb++;
    strcpy(pb, "grubinst.exe\0");
    pb = strrchr(fn, '\\');
    if (!pb)
      pb = fn;
    else
      pb++;
    strcpy(pb, "grubinst.exe ");
    len = strlen(fn);

    GetDlgItemText(hWnd, IDC_CHOOSEVER, temp, sizeof(temp));
    sprintf(&fn[len], "--g4d-version=%s ", temp);
    len += strlen(&fn[len]);
    if (lParam == 0)
    {
      strcpy(&fn[len], "--read-only ");
      len += strlen(&fn[len]);
    }
    GetDlgItemText(hWnd, IDC_FILESYSTEM, temp, sizeof(temp));
    if (!strcmp(temp, "MBR"))
    {
	  int idx    = ComboBox_GetCurSel(GetDlgItem(hWnd, IDC_PARTS));
	  if (idx <= 0)
        strcpy(&fn[len], "--file-system-type=MBR ");
	  else
	  {
		  switch (p_fstype[idx])
		  {
		    case FST_NTFS:
			   strcpy(&fn[len], "--file-system-type=NTFS ");
			   break;
			case FST_EXFAT:
			   strcpy(&fn[len], "--file-system-type=exFAT ");
			   break;
			case FST_FAT32:
			   strcpy(&fn[len], "--file-system-type=FAT32 ");
			   break;			   
			case FST_FAT16:
			   strcpy(&fn[len], "--file-system-type=FAT16 ");
			   break;			   
			case FST_EXT2:
			   strcpy(&fn[len], "--file-system-type=EXT2 ");
			   break;
			case FST_EXT3:
			   strcpy(&fn[len], "--file-system-type=EXT3 ");
			   break;			   
			case FST_EXT4:
			   strcpy(&fn[len], "--file-system-type=EXT4 ");
			   break;			   
		  }
	  }
      len += strlen(&fn[len]);
    }
    else
    if (!strcmp(temp, "GPT"))
    {
      strcpy(&fn[len], "--file-system-type=GPT ");
      len += strlen(&fn[len]);
    }
    else
    if (!strcmp(temp, "NTFS"))
    {
      strcpy(&fn[len], "--file-system-type=NTFS ");
      len += strlen(&fn[len]);
    }
    else
    if (!strcmp(temp, "exFAT"))
    {
      strcpy(&fn[len], "--file-system-type=exFAT ");
      len += strlen(&fn[len]);
    }
    else
    if (!strcmp(temp, "FAT32"))
    {
      strcpy(&fn[len], "--file-system-type=FAT32 ");
      len += strlen(&fn[len]);
    }
    else
    if (!strcmp(temp, "FAT12/16"))
    {
      strcpy(&fn[len], "--file-system-type=FAT16 ");
      len += strlen(&fn[len]);
    }
    else
    if (!strcmp(temp, "EXT2"))
    {
      strcpy(&fn[len], "--file-system-type=EXT2 ");
      len += strlen(&fn[len]);
    }
    if (!strcmp(temp, "EXT3"))
    {
      strcpy(&fn[len], "--file-system-type=EXT3 ");
      len += strlen(&fn[len]);
    }
    if (!strcmp(temp, "EXT4"))
    {
      strcpy(&fn[len], "--file-system-type=EXT4 ");
      len += strlen(&fn[len]);
    }
    if (IsDlgButtonChecked(hWnd, IDC_OUTPUT) == BST_CHECKED)
    {
      strcpy(&fn[len], "--output ");
      len += strlen(&fn[len]);
    }
    if (IsDlgButtonChecked(hWnd, IDC_VERBOSE) == BST_CHECKED)
    {
      strcpy(&fn[len], "--verbose ");
      len += strlen(&fn[len]);
    }
    if (IsWindowVisible(GetDlgItem(hWnd, IDC_LOCK)) && (IsDlgButtonChecked(hWnd, IDC_LOCK) == BST_CHECKED))
    {
      strcpy(&fn[len], "--lock ");
      len += strlen(&fn[len]);
    }
    if (IsWindowVisible(GetDlgItem(hWnd, IDC_UNMOUNT)) && (IsDlgButtonChecked(hWnd, IDC_UNMOUNT) == BST_CHECKED))
    {
      strcpy(&fn[len], "--unmount ");
      len += strlen(&fn[len]);
    }
    if (IsWindowVisible(GetDlgItem(hWnd, IDC_NO_BACKUP_MBR)) && (IsDlgButtonChecked(hWnd, IDC_NO_BACKUP_MBR) == BST_CHECKED))
    {
      strcpy(&fn[len], "--no-backup-mbr ");
      len += strlen(&fn[len]);
    }
    if (IsWindowVisible(GetDlgItem(hWnd, IDC_DISABLE_FLOPPY)) && (IsDlgButtonChecked(hWnd, IDC_DISABLE_FLOPPY) == BST_CHECKED))
    {
      strcpy(&fn[len], "--mbr-disable-floppy ");
      len += strlen(&fn[len]);
    }
    if (IsWindowVisible(GetDlgItem(hWnd, IDC_DISABLE_OSBR)) && (IsDlgButtonChecked(hWnd, IDC_DISABLE_OSBR) == BST_CHECKED))
    {
      strcpy(&fn[len], "--mbr-disable-osbr ");
      len += strlen(&fn[len]);
    }
    if (IsWindowVisible(GetDlgItem(hWnd, IDC_PREVMBR_FIRST)) && (IsDlgButtonChecked(hWnd, IDC_PREVMBR_FIRST) == BST_CHECKED))
    {
      strcpy(&fn[len], "--boot-prevmbr-first ");
      len += strlen(&fn[len]);
    }
    if (isFloppy)
    {
      strcpy(&fn[len], "--floppy ");
      len += strlen(&fn[len]);
    }
    if (IsWindowVisible(GetDlgItem(hWnd, IDC_COPY_BPB)) && (IsDlgButtonChecked(hWnd, IDC_COPY_BPB) == BST_CHECKED))
    {
      strcpy(&fn[len], "--copy-bpb ");
      len += strlen(&fn[len]);
    }
    if (IsDlgButtonChecked(hWnd, IDC_SKIP) == BST_CHECKED)
    {
      strcpy(&fn[len], "--skip-mbr-test ");
      len += strlen(&fn[len]);
    }
    if (IsDlgButtonChecked(hWnd, IDC_SILENT) == BST_CHECKED)
    {
      strcpy(&fn[len], "--silent-boot ");
      len += strlen(&fn[len]);
    }
    if (IsWindowVisible(GetDlgItem(hWnd, IDC_CHS_NO_TUNE)) && (IsDlgButtonChecked(hWnd, IDC_CHS_NO_TUNE) == BST_CHECKED))
    {
      strcpy(&fn[len], "--chs-no-tune ");
      len += strlen(&fn[len]);
    }
    if (IsWindowVisible(GetDlgItem(hWnd, IDC_TIMEOUT)))
    {
      if (GetDlgItemText(hWnd, IDC_TIMEOUT, temp, sizeof(temp)) != 0)
      {
        sprintf(&fn[len], "--time-out=%s ", temp);
        len += strlen(&fn[len]);
      }
      else
      {
        sprintf(&fn[len], "--time-out=0 ");
        len += strlen(&fn[len]);
      }
    }
    if (IsWindowVisible(GetDlgItem(hWnd, IDC_HOTKEY)) && (GetDlgItemText(hWnd, IDC_HOTKEY, temp, sizeof(temp)) != 0))
    {
      sprintf(&fn[len], "--hot-key=%s ", temp);
      len += strlen(&fn[len]);
    }
    if ((GetDlgItemText(hWnd, IDC_BOOTFILE, temp, sizeof(temp)) != 0) && (strcmp(temp, "GRLDR")))
    {
      sprintf(&fn[len], "--boot-file=%s ", temp);
      len += strlen(&fn[len]);
    }
    if (IsDlgButtonChecked(hWnd, IDC_RESTOREMBR) == BST_CHECKED)
    {
      strcpy(&fn[len], "\"--restore=");
      len += strlen(&fn[len]);
      if (GetDlgItemText(hWnd, IDC_SAVEFILE, &fn[len], sizeof(fn) - len) == 0)
      {
        PrintError(hWnd, IDS_NO_RESTOREFILE);
        EndDialog(hOWnd, 0);
        SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, IDC_SAVEFILE), TRUE);
        break;
      }
      len      += strlen(&fn[len]);
      fn[len++] = '"';
      fn[len++] = ' ';
      fn[len]   = 0;
    }
    else if (IsWindowVisible(GetDlgItem(hWnd, IDC_RESTORE_PREVMBR)) && (IsDlgButtonChecked(hWnd, IDC_RESTORE_PREVMBR) == BST_CHECKED))
    {
      strcpy(&fn[len], "--restore-prevmbr ");
      len += strlen(&fn[len]);
    }
    else
    {
      int slen;

      if (IsDlgButtonChecked(hWnd, IDC_SAVEMBR) == BST_CHECKED)
      {
        strcpy(&fn[len], "\"--save=");
        slen = strlen(&fn[len]);
        if (GetDlgItemText(hWnd, IDC_SAVEFILE, &fn[len + slen], sizeof(fn) - len - slen) != 0)
        {
          len      += strlen(&fn[len]);
          fn[len++] = '"';
          fn[len++] = ' ';
          fn[len]   = 0;
        }
        else
        {
          PrintError(hWnd, IDS_NO_SAVEFILE);
          EndDialog(hOWnd, 0);
          SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, IDC_SAVEFILE), TRUE);
          break;
        }
      }
    }
    if (GetDlgItemText(hWnd, IDC_PARTS, temp, sizeof(temp)) != 0)
    {
      int idx;

      idx = ComboBox_GetCurSel(GetDlgItem(hWnd, IDC_PARTS));
      if (idx > 0)
      {
        sprintf(&fn[len], "--install-partition=%d ", p_index[idx]);
        len += strlen(&fn[len]);
      }
    }
    if (GetDlgItemText(hWnd, IDC_EXTRA, &fn[len], sizeof(fn) - len) != 0)
    {
      len      += strlen(&fn[len]);
      fn[len++] = ' ';
      fn[len]   = 0;
    }
    if (IsDlgButtonChecked(hWnd, IDC_OUTPUT) == BST_UNCHECKED)
    {
      if (GetFileName(hWnd, &fn[len], sizeof(fn) - len, TRUE))
      {
        EndDialog(hOWnd, 0);
        if (IsDlgButtonChecked(hWnd, IDC_ISDISK) == BST_CHECKED)
          SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, IDC_DISKS), TRUE);
        else
        if (IsDlgButtonChecked(hWnd, IDC_ISFILE) == BST_CHECKED)
          SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, IDC_FILENAME), TRUE);
        break;
      }
      if ((lParam == 1) && (!strcmp(&fn[len], osdrv)))
      {
        char buf[128];
        int  idx = -1;

        idx = ComboBox_GetCurSel(GetDlgItem(hWnd, IDC_PARTS));
        if ((idx < 0) || ((idx == 0) && ((p_fstype[0] == FST_MBR) || (p_fstype[0] == FST_GPT))))
          sprintf(buf, LoadText(IDS_OSDRIVE), "MBR/BS");
        else
          sprintf(buf, LoadText(IDS_OSDRIVE), "PBR");
        if (MessageBox(hWnd, buf, "Warning", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
        {
          EndDialog(hOWnd, 0);
          break;
        }
      }
    }
    else
    {
      strcpy(&fn[len], "\"");
      len += strlen(&fn[len]);
      if (GetDlgItemText(hWnd, IDC_SAVEFILE, &fn[len], sizeof(fn) - len) == 0)
      {
        PrintError(hWnd, IDS_NO_SAVEFILE);
        EndDialog(hOWnd, 0);
        SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, IDC_SAVEFILE), TRUE);
        break;
      }
      len      += strlen(&fn[len]);
      fn[len++] = '"';
      fn[len]   = 0;
    }

    FILE * file;
    file = fopen(sfn, "r");
    if (file)
    {
      fclose(file);

      char                outtxt[10240] = "";
      char                buf[1024];
      SECURITY_ATTRIBUTES sa;
      SECURITY_DESCRIPTOR sd;
      STARTUPINFO         si;
      PROCESS_INFORMATION pi;
      HANDLE              newstdout, read_stdout;

      InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
      SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
      sa.lpSecurityDescriptor = &sd;
      sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
      sa.bInheritHandle       = TRUE;
      CreatePipe(&read_stdout, &newstdout, &sa, 0);
      GetStartupInfo(&si);
      si.dwFlags     = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
      si.wShowWindow = SW_HIDE;
      si.hStdOutput  = newstdout;
      si.hStdError   = newstdout;
      if (lParam == 0)
      {
        sprintf(outtxt, "%s \"grubinst.exe", LoadText(IDS_STARTING));
        strcpy(&outtxt[22], &fn[strlen(sfn)]);
        strcpy(&outtxt[22 + strlen(fn) - strlen(sfn)], "\"...\r\n\r\n");
      }

      if (!CreateProcess(NULL, fn, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
      {
        CloseHandle(newstdout);
        CloseHandle(read_stdout);
        char errmsg[512];
        sprintf(errmsg, "%s\r\n%s: %d", LoadText(IDS_EXEC_ERROR), LoadText(IDS_ERROR_CODE), (int) GetLastError());
        MessageBox(hWnd, errmsg, NULL, MB_OK | MB_ICONERROR);
        {
          EndDialog(hOWnd, 0);
          break;
        }
      }

      unsigned long exit = 0;
      unsigned long bread;
      unsigned long avail;

      memset(buf, 0, sizeof(buf));

      for (;; )
      {
        GetExitCodeProcess(pi.hProcess, &exit);
        PeekNamedPipe(read_stdout, buf, 1023, &bread, &avail, NULL);
        if (bread != 0)
        {
          memset(buf, 0, sizeof(buf));
          if (avail > 1023)
          {
            while (bread >= 1023)
            {
              ReadFile(read_stdout, buf, 1023, &bread, NULL);
              strcat(outtxt, buf);
              memset(buf, 0, sizeof(buf));
            }
          }
          else
          {
            ReadFile(read_stdout, buf, 1023, &bread, NULL);
            strcat(outtxt, buf);
          }
        }
        if (exit != STILL_ACTIVE)
          break;
      }
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);
      CloseHandle(newstdout);
      CloseHandle(read_stdout);


      SETTEXTEX stx;
      stx.flags    = ST_DEFAULT;
      stx.codepage = CP_ACP;
      SendDlgItemMessage(hOWnd, IDC_RICHEDIT, EM_SETTEXTEX, (WPARAM) &stx, (LPARAM) &outtxt);
      if (lParam == 1)
        AlreadyScanned = FALSE;
    }
    else
    {
      MessageBox(hWnd, LoadText(IDS_NO_GRUBINST), NULL, MB_OK | MB_ICONERROR);
      EndDialog(hOWnd, 0);
    }
    break;
  }
  case WM_ACTIVATE:
  {
    if (wParam == WA_ACTIVE)
    {
      SendMessage(GetDlgItem(hOWnd, IDC_RICHEDIT), EM_SETSEL, (WPARAM) -1, (LPARAM) 0);
      if (!AlreadyScanned)
      {
        AlreadyScanned = TRUE;
        GetOptCurBR(GetParent(hOWnd));
      }
    }
    break;
  }
  case WM_COMMAND:
  {
    switch (wParam & 0xFFFF)
    {
    case IDC_CLOSE:
    {
      EndDialog(hOWnd, 0);
      break;
    }
    case IDC_COPY:
    {
      SendDlgItemMessage(hOWnd, IDC_RICHEDIT, WM_COPY, 0, (LPARAM) 0);
      break;
    }
    case IDC_SELALL:
    {
      SendMessage(GetDlgItem(hOWnd, IDC_RICHEDIT), EM_SETSEL, (WPARAM) 0, (LPARAM) -1);
      break;
    }
    }
  }
  default:
    return FALSE;
  }
  return TRUE;
}

typedef struct _DEV_BROADCAST_DEVICEINTERFACE
{
  DWORD dbcc_size;
  DWORD dbcc_devicetype;
  DWORD dbcc_reserved;
  GUID  dbcc_classguid;
  TCHAR dbcc_name[1];
} DEV_BROADCAST_DEVICEINTERFACE, *PDEV_BROADCAST_DEVICEINTERFACE;

WINUSERAPI HDEVNOTIFY WINAPI RegisterDeviceNotificationW(HANDLE, LPVOID, DWORD);
WINUSERAPI BOOL WINAPI UnregisterDeviceNotification(HANDLE);

void SetStatusText(HWND hWnd, char* text, signed char installed)
{
  switch (installed)
  {
  case 1:
    status_color = RGB(0, 161, 24);
    break;
  case -1:
    status_color = RGB(161, 24, 0);
    break;
  default:
    status_color = GetSysColor(COLOR_WINDOWTEXT);
  }
  Edit_SetText(GetDlgItem(hWnd, IDC_STATUS), text);
}

BOOL CALLBACK DialogMainProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
  switch (wMsg)
  {
  case WM_POSTSHOWJOB:
  {
    if (((HWND) wParam != hWnd) || (lParam != 3))
      break;
    if (strcmp(MsgToShow, "") != 0)
      MessageBox(hWnd, MsgToShow, "Warning", MB_OK | MB_ICONWARNING);

    RefreshDisk(hWnd, FALSE);

    DEV_BROADCAST_DEVICEINTERFACE dbi;
    int                           DBT_DEVTYP_DEVICEINTERFACE  = 0x00000005; // device interface class
    GUID                          GUID_DEVINTERFACE_DISK      = { 0x53F56307, 0xB6BF, 0x11D0, { 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b } };
    int                           DEVICE_NOTIFY_WINDOW_HANDLE = 0x00000000;

    ZeroMemory(&dbi, sizeof(dbi));
    dbi.dbcc_size       = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    dbi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    dbi.dbcc_classguid  = GUID_DEVINTERFACE_DISK;
    usbnotification     = RegisterDeviceNotificationW(hWnd, &dbi, DEVICE_NOTIFY_WINDOW_HANDLE);
    break;
  }
  case WM_CTLCOLORSTATIC:
  {
    HDC hdcStatic = (HDC) wParam;
    if ((HWND) lParam != GetDlgItem(hWnd, IDC_STATUS))
    {
      SetBkColor(hdcStatic, GetSysColor(COLOR_BTNFACE));
      break;
    }
    SetTextColor(hdcStatic, status_color);
    SetBkColor(hdcStatic, GetSysColor(COLOR_BTNFACE));
    if (status_brush == NULL)
      status_brush = GetSysColorBrush(COLOR_BTNFACE);
    return (INT_PTR) status_brush;
  }
  case WM_CLOSE:
  {
    UnregisterDeviceNotification(usbnotification);
    EndDialog(hWnd, 0);
    break;
  }
  case WM_INITDIALOG:
  {
    HICON hIcon;
    char  title[30];
    int   i;

    for (i = 0; i < IDC_COUNT_MAIN; i++)
      if (comp_tab_main[i])
        SetDlgItemText(hWnd, i, comp_tab_main[i]);
    GetDlgItemText(hWnd, IDC_INSTALL, btnInstName, sizeof(btnInstName));
    sprintf(title, "%s " VERSION, LoadText(IDS_MAIN));
    SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM) title);
    hIcon = LoadIcon(hInst, MAKEINTRESOURCE(ICO_MAIN));
    SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM) hIcon);
    SetDlgItemText(hWnd, IDC_TIMEOUT, "0");
    SendDlgItemMessage(hWnd, IDC_FILESYSTEM, CB_ADDSTRING, 0, (LPARAM) LoadText(IDS_AUTODETECT));
    SendDlgItemMessage(hWnd, IDC_FILESYSTEM, CB_ADDSTRING, 0, (LPARAM) "MBR");
    SendDlgItemMessage(hWnd, IDC_FILESYSTEM, CB_ADDSTRING, 0, (LPARAM) "GPT");
    SendDlgItemMessage(hWnd, IDC_FILESYSTEM, CB_ADDSTRING, 0, (LPARAM) "NTFS");
    SendDlgItemMessage(hWnd, IDC_FILESYSTEM, CB_ADDSTRING, 0, (LPARAM) "exFAT");
    SendDlgItemMessage(hWnd, IDC_FILESYSTEM, CB_ADDSTRING, 0, (LPARAM) "FAT32");
    SendDlgItemMessage(hWnd, IDC_FILESYSTEM, CB_ADDSTRING, 0, (LPARAM) "FAT12/16");
    SendDlgItemMessage(hWnd, IDC_FILESYSTEM, CB_ADDSTRING, 0, (LPARAM) "EXT2");
    SendDlgItemMessage(hWnd, IDC_FILESYSTEM, CB_ADDSTRING, 0, (LPARAM) "EXT3");
    SendDlgItemMessage(hWnd, IDC_FILESYSTEM, CB_ADDSTRING, 0, (LPARAM) "EXT4");
    SendDlgItemMessage(hWnd, IDC_FILESYSTEM, CB_SETCURSEL, (WPARAM) 0, 0);
    SendDlgItemMessage(hWnd, IDC_BOOTFILE, CB_ADDSTRING, 0, (LPARAM) LoadText(IDS_G4D_LOADER));
    SendDlgItemMessage(hWnd, IDC_BOOTFILE, CB_ADDSTRING, 0, (LPARAM) LoadText(IDS_G2_LOADER));
    SetDlgItemText(hWnd, IDC_BOOTFILE, "GRLDR");
    CheckDlgButton(hWnd, IDC_VERBOSE, BST_CHECKED);
    SendDlgItemMessage(hWnd, IDC_CHOOSEVER, CB_ADDSTRING, 0, (LPARAM) "0.4.5c");
    SendDlgItemMessage(hWnd, IDC_CHOOSEVER, CB_ADDSTRING, 0, (LPARAM) "0.4.6a");
    SendDlgItemMessage(hWnd, IDC_CHOOSEVER, CB_SETCURSEL, (WPARAM) 0, 0);
    CheckDlgButton(hWnd, IDC_VERBOSE, BST_CHECKED);
    CheckDlgButton(hWnd, IDC_DISABLE_FLOPPY, BST_CHECKED);
    CheckRadioButton(hWnd, IDC_ISDISK, IDC_ISFILE, IDC_ISDISK);
    RefreshDisk(hWnd, TRUE);
    PostMessage(hWnd, WM_POSTSHOWJOB, (WPARAM) hWnd, (LPARAM) 3);
    break;
  }
  case WM_DEVICECHANGE:
  {
    PDEV_BROADCAST_HDR    lpdb                       = (PDEV_BROADCAST_HDR) lParam;
    PDEV_BROADCAST_VOLUME lpdv                       = (PDEV_BROADCAST_VOLUME) lParam;
    int                   DBT_DEVTYP_DEVICEINTERFACE = 0x00000005; // device interface class

    if (((wParam == DBT_DEVICEREMOVECOMPLETE) && (lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)) || ((wParam == DBT_DEVICEARRIVAL) && (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) && (lpdv->dbcv_flags == 0x0000)))
    {
      RefreshDisk(hWnd, TRUE);
      RefreshDisk(hWnd, FALSE);
    }
  }
  case WM_COMMAND:
  {
    switch (wParam & 0xFFFF)
    {
    case IDC_TEST:
      if (LoadLibrary("riched20.dll"))
        DialogBoxParam(NULL, MAKEINTRESOURCE(DLG_OUTPUT), hWnd, DialogOutputProc, 0);
      else
        MessageBox(hWnd, LoadText(IDS_NO_RICHED20), "Error", MB_OK | MB_ICONERROR);
      break;
    case IDC_INSTALL:
      if (LoadLibrary("riched20.dll"))
        DialogBoxParam(NULL, MAKEINTRESOURCE(DLG_OUTPUT), hWnd, DialogOutputProc, 1);
      else
        MessageBox(hWnd, LoadText(IDS_NO_RICHED20), "Error", MB_OK | MB_ICONERROR);
      break;
    case IDC_QUIT:
      EndDialog(hWnd, 0);
      break;
    case IDC_ISDISK:
    {
      if (wParam >> 16 == BN_CLICKED)
      {
        char temp[512] = "";
        GetDlgItemText(hWnd, IDC_DISKS, temp, sizeof(temp));
        if (strcmp(temp, ""))
          RefreshPart(hWnd, TRUE);
        else
        {
          SendDlgItemMessage(hWnd, IDC_PARTS, CB_RESETCONTENT, 0, 0);
          SwitchToMBRorPBR(hWnd, TRUE);
        }
      }
      break;
    }
    case IDC_ISFILE:
    {
      if (wParam >> 16 == BN_CLICKED)
      {
        char temp[512] = "";
        GetDlgItemText(hWnd, IDC_FILENAME, temp, sizeof(temp));
        if (strcmp(temp, ""))
          RefreshPart(hWnd, TRUE);
        else
        {
          SendDlgItemMessage(hWnd, IDC_PARTS, CB_RESETCONTENT, 0, 0);
          SwitchToMBRorPBR(hWnd, TRUE);
        }
      }
      break;
    }
    case IDC_REFRESH_DISK:
    {
      RefreshDisk(hWnd, TRUE);
      RefreshDisk(hWnd, FALSE);
      break;
    }
    case IDC_FILESYSTEM:
    {
      if (wParam >> 16 == CBN_SELENDOK)
      {
        char temp[512] = "";

        if (IsDlgButtonChecked(hWnd, IDC_ISDISK) == BST_CHECKED)
          GetDlgItemText(hWnd, IDC_DISKS, temp, sizeof(temp));
        else
        if (IsDlgButtonChecked(hWnd, IDC_ISFILE) == BST_CHECKED)
          GetDlgItemText(hWnd, IDC_FILENAME, temp, sizeof(temp));
        if (strcmp(temp, ""))
          RefreshPart(hWnd, TRUE);
      }
      break;
    }
    case IDC_PARTS:
    {
      if (wParam >> 16 == CBN_SELENDOK)
        SwitchToMBRorPBR(hWnd, TRUE);
      break;
    }
    case IDC_ORDER_PART:
    {
      if (wParam >> 16 == BN_CLICKED)
        RefreshPart(hWnd, FALSE);
      break;
    }
    case IDC_BOOTFILE:
    {
      if (wParam >> 16 == CBN_SELCHANGE)
      {
        char buf[64];
        HWND cbhwnd;
        int  idx;

        cbhwnd = GetDlgItem(hWnd, IDC_BOOTFILE);
        idx    = ComboBox_GetCurSel(cbhwnd);
        switch (idx)
        {
        case 0:
          strcpy(buf, "GRLDR");
          break;
        case 1:
          strcpy(buf, "G2LDR");
          break;
        default:
          return TRUE;
        }
        SendMessage(cbhwnd, CB_ADDSTRING, 0, (LPARAM) &buf);
        SendMessage(cbhwnd, CB_SETCURSEL, (WPARAM) 2, 0);
      }
      else
      if (wParam >> 16 == CBN_DROPDOWN)
      {
        int  c;
        HWND cbhwnd;

        cbhwnd = GetDlgItem(hWnd, IDC_BOOTFILE);
        c      = SendMessage(cbhwnd, CB_GETCOUNT, 0, 0);
        while (c > 2)
        {
          SendMessage(cbhwnd, CB_DELETESTRING, (WPARAM) (c - 1), 0);
          c = SendMessage(cbhwnd, CB_GETCOUNT, 0, 0);
        }
      }
      else
      if (wParam >> 16 == CBN_EDITCHANGE)
      {
        char buf[64];
        int  selstart, selend;

        SendMessage(GetDlgItem(hWnd, IDC_BOOTFILE), CB_GETEDITSEL, (WPARAM) &selstart, (LPARAM) &selend);
        GetDlgItemText(hWnd, IDC_BOOTFILE, buf, sizeof(buf));
        str_upcase(buf);
        SetDlgItemText(hWnd, IDC_BOOTFILE, buf);
        SendMessage(GetDlgItem(hWnd, IDC_BOOTFILE), CB_SETEDITSEL, (WPARAM) 0, MAKELPARAM(selstart, selend));
      }
      break;
    }
    case IDC_BROWSE:
    {
      OPENFILENAME ofn;
      char         fn[MAX_PATH];

      memset(&ofn, 0, sizeof(ofn));
      GetDlgItemText(hWnd, IDC_FILENAME, fn, sizeof(fn));
      ofn.lStructSize = sizeof(ofn);
      ofn.hwndOwner   = hWnd;
      ofn.hInstance   = hInst;
      ofn.lpstrFile   = fn;
      ofn.nMaxFile    = sizeof(fn);
      ofn.Flags       = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
      char buf[256];
      int  len;

      sprintf(buf, "%s (*.dsk, *.img, *.vhd)%c*.dsk;*.img;*.vhd%c", LoadText(IDS_RAW_DISK), '\0', '\0');
      len = strlen(LoadText(IDS_RAW_DISK)) + 41;
      sprintf(&buf[len], "%s (*.*)%c*.*%c%c", LoadText(IDS_ALL_FILES), '\0', '\0', '\0');
      ofn.lpstrFilter = buf;
      if (GetOpenFileName(&ofn))
      {
        SetDlgItemText(hWnd, IDC_FILENAME, fn);
        RefreshPart(hWnd, TRUE);
        InvalidateRect(hWnd, NULL, TRUE);
        SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, IDC_FILENAME), TRUE);
        SendMessage(GetDlgItem(hWnd, IDC_FILENAME), EM_SETSEL, (WPARAM) -1, (LPARAM) 0);
      }
      break;
    }
    case IDC_DISKS:
    {
      if (wParam >> 16 == CBN_SELENDOK)
      {
        CheckRadioButton(hWnd, IDC_ISDISK, IDC_ISFILE, IDC_ISDISK);
        RefreshPart(hWnd, TRUE);
      }
      break;
    }
    case IDC_RESTOREMBR:
    {
      if (wParam >> 16 == BN_CLICKED)
      {
        if (IsDlgButtonChecked(hWnd, IDC_RESTOREMBR) == BST_CHECKED)
        {
          if (IsDlgButtonChecked(hWnd, IDC_SAVEMBR) == BST_CHECKED)
          {
            ChangeBtnWidth(hWnd, IDC_INSTALL, -DiffWidth);
            CheckDlgButton(hWnd, IDC_SAVEMBR, BST_UNCHECKED);
          }
          CheckDlgButton(hWnd, IDC_RESTORE_PREVMBR, BST_UNCHECKED);
          CheckDlgButton(hWnd, IDC_OUTPUT, BST_UNCHECKED);
          SetDlgItemText(hWnd, IDC_INSTALL, LoadText(IDS_RESTORE));
        }
        else
          SetDlgItemText(hWnd, IDC_INSTALL, btnInstName);
      }
      break;
    }
    case IDC_RESTORE_PREVMBR:
    {
      if (wParam >> 16 == BN_CLICKED)
      {
        if (IsDlgButtonChecked(hWnd, IDC_RESTORE_PREVMBR) == BST_CHECKED)
        {
          if (IsDlgButtonChecked(hWnd, IDC_SAVEMBR) == BST_CHECKED)
          {
            ChangeBtnWidth(hWnd, IDC_INSTALL, -DiffWidth);
            CheckDlgButton(hWnd, IDC_SAVEMBR, BST_UNCHECKED);
          }
          CheckDlgButton(hWnd, IDC_RESTOREMBR, BST_UNCHECKED);
          CheckDlgButton(hWnd, IDC_OUTPUT, BST_UNCHECKED);
          SetDlgItemText(hWnd, IDC_INSTALL, LoadText(IDS_RESTORE));
        }
        else
          SetDlgItemText(hWnd, IDC_INSTALL, btnInstName);
      }
      break;
    }
    case IDC_SAVEMBR:
    {
      if (wParam >> 16 == BN_CLICKED)
      {
        if (IsDlgButtonChecked(hWnd, IDC_SAVEMBR) == BST_CHECKED)
        {
          char    buf[64] = "";
          int     idx;
          BOOLEAN isMBRorGPT;

          idx        = ComboBox_GetCurSel(GetDlgItem(hWnd, IDC_PARTS));
          isMBRorGPT = (idx < 0) || ((idx == 0) && ((p_fstype[0] == FST_MBR) || (p_fstype[0] == FST_GPT)));
          if ((strcmp(btnInstName, LoadText(IDS_INSTALL)) == 0) || !isMBRorGPT)
            strcpy(buf, LoadText(IDS_SAVE));
          else
            strcpy(buf, LoadText(IDS_COPY));
          strcat(buf, " && ");
          strcat(buf, btnInstName);
          CheckDlgButton(hWnd, IDC_RESTORE_PREVMBR, BST_UNCHECKED);
          CheckDlgButton(hWnd, IDC_RESTOREMBR, BST_UNCHECKED);
          CheckDlgButton(hWnd, IDC_OUTPUT, BST_UNCHECKED);
          ChangeBtnWidth(hWnd, IDC_INSTALL, DiffWidth);
          SetDlgItemText(hWnd, IDC_INSTALL, buf);
        }
        else
        {
          SetDlgItemText(hWnd, IDC_INSTALL, btnInstName);
          ChangeBtnWidth(hWnd, IDC_INSTALL, -DiffWidth);
        }
      }
      break;
    }
    case IDC_OUTPUT:
    {
      if (wParam >> 16 == BN_CLICKED)
      {
        if (IsDlgButtonChecked(hWnd, IDC_OUTPUT) == BST_CHECKED)
        {
          CheckDlgButton(hWnd, IDC_RESTORE_PREVMBR, BST_UNCHECKED);
          CheckDlgButton(hWnd, IDC_RESTOREMBR, BST_UNCHECKED);
          if (IsDlgButtonChecked(hWnd, IDC_SAVEMBR) == BST_CHECKED)
          {
            ChangeBtnWidth(hWnd, IDC_INSTALL, -DiffWidth);
            CheckDlgButton(hWnd, IDC_SAVEMBR, BST_UNCHECKED);
          }
          SetDlgItemText(hWnd, IDC_INSTALL, LoadText(IDS_SAVE));
        }
        else
          SetDlgItemText(hWnd, IDC_INSTALL, btnInstName);
      }
      break;
    }
    case IDC_FILENAME:
    {
      if (wParam >> 16 == EN_CHANGE)
      {
        CheckRadioButton(hWnd, IDC_ISDISK, IDC_ISFILE, IDC_ISFILE);
        SendDlgItemMessage(hWnd, IDC_PARTS, CB_RESETCONTENT, 0, 0);
      }
      break;
    }
    case IDC_BROWSE_SAVE:
    {
      OPENFILENAME ofn;
      char         fn[512];
      BOOLEAN      isMBRorGPT = TRUE;

      isMBRorGPT = ComboBox_GetCurSel(GetDlgItem(hWnd, IDC_PARTS)) <= 0;
      memset(&ofn, 0, sizeof(ofn));
      GetDlgItemText(hWnd, IDC_SAVEFILE, fn, sizeof(fn));
      ofn.lStructSize = sizeof(ofn);
      ofn.hwndOwner   = hWnd;
      ofn.hInstance   = hInst;
      ofn.lpstrFile   = fn;
      ofn.nMaxFile    = sizeof(fn);
      char buf[256];
      int  len;

      sprintf(buf, "%s (*.bin)%c*.bin%c", LoadText(IDS_BIN_FILES), '\0', '\0');
      len = strlen(LoadText(IDS_BIN_FILES)) + 15;
      if (isMBRorGPT)
      {
        sprintf(&buf[len], "%s (*.mbr)%c*.mbr%c", LoadText(IDS_MBR_FILES), '\0', '\0');
        len += strlen(LoadText(IDS_MBR_FILES)) + 15;
      }
      else
      {
        sprintf(&buf[len], "%s (*.pbr)%c*.pbr%c", LoadText(IDS_PBR_FILES), '\0', '\0');
        len += strlen(LoadText(IDS_PBR_FILES)) + 15;
      }
      sprintf(&buf[len], "%s (*.*)%c*.*%c%c", LoadText(IDS_ALL_FILES), '\0', '\0', '\0');
      ofn.lpstrFilter = buf;
      ofn.Flags       = OFN_HIDEREADONLY;
      if (IsDlgButtonChecked(hWnd, IDC_RESTOREMBR) == BST_CHECKED)
      {
        ofn.Flags |= OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        if (GetOpenFileName(&ofn))
        {
          SetDlgItemText(hWnd, IDC_SAVEFILE, fn);
          SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, IDC_SAVEFILE), TRUE);
          SendMessage(GetDlgItem(hWnd, IDC_SAVEFILE), EM_SETSEL, (WPARAM) -1, (LPARAM) 0);
        }
      }
      else
      if (IsDlgButtonChecked(hWnd, IDC_OUTPUT) == BST_CHECKED)
      {
        ofn.Flags |= OFN_OVERWRITEPROMPT;
        if (!strcmp(fn, ""))
        {
          if (isMBRorGPT)
            strcpy(fn, "grldr.mbr");
          else
            strcpy(fn, "grldr.pbr");
          ofn.nFilterIndex = 2;
        }
        if (GetSaveFileName(&ofn))
        {
          SetDlgItemText(hWnd, IDC_SAVEFILE, fn);
          SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, IDC_SAVEFILE), TRUE);
          SendMessage(GetDlgItem(hWnd, IDC_SAVEFILE), EM_SETSEL, (WPARAM) -1, (LPARAM) 0);
        }
      }
      else
      {
        ofn.Flags |= OFN_OVERWRITEPROMPT;
        if (!strcmp(fn, ""))
        {
          if (isMBRorGPT)
            strcpy(fn, "MBR.bin");
          else
            strcpy(fn, "PBR.bin");
        }
        if (GetSaveFileName(&ofn))
        {
          SetDlgItemText(hWnd, IDC_SAVEFILE, fn);
          if (IsDlgButtonChecked(hWnd, IDC_SAVEMBR) == BST_UNCHECKED)
          {
            ChangeBtnWidth(hWnd, IDC_INSTALL, DiffWidth);
            CheckDlgButton(hWnd, IDC_SAVEMBR, BST_CHECKED);
            char buf[64];

            sprintf(buf, "%s && %s", LoadText(IDS_SAVE), btnInstName);
            SetDlgItemText(hWnd, IDC_INSTALL, buf);
          }
          SendMessage(hWnd, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(hWnd, IDC_SAVEFILE), TRUE);
          SendMessage(GetDlgItem(hWnd, IDC_SAVEFILE), EM_SETSEL, (WPARAM) -1, (LPARAM) 0);
        }
      }
      break;
    }
    }
    break;
  }
  default:
    return FALSE;
  }
  return TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  HWND hWnd = NULL;
  char title[30];

  hInst = hInstance;
  if (!strncmp(lpCmdLine, "--lang=", 7))
    lng_ext = &lpCmdLine[7];
  ChangeText(hWnd);

  sprintf(title, "%s " VERSION, LoadText(IDS_MAIN));
  hWnd = FindWindow("#32770", title);
  if (hWnd)
  {
    if (IsIconic(hWnd))
      SendMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
    SetForegroundWindow(hWnd);
    return 0;
  }

  isWin6orabove = (DWORD) (LOBYTE(LOWORD(GetVersion()))) >= 6;

  DialogBoxParam(hInst, MAKEINTRESOURCE(DLG_MAIN), NULL, DialogMainProc, 0);
  return 0;
}
#endif       //win32 end

#ifdef LINUX
int main(void)
{
  return 0;
}
#endif