/*
 *  ntloader  --  Microsoft Windows NT6+ loader
 *  Copyright (C) 2021  A1ive.
 *
 *  ntloader is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ntloader is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ntloader.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _BCD_BOOT_H
#define _BCD_BOOT_H 1

//#include <ntboot.h>
#include <stdint.h>

#define BCD_DP_MAGIC "GNU GRUB2 NTBOOT"

#define GUID_OSENTRY L"{19260817-6666-8888-abcd-000000000000}"  //操作系统入口
#define GUID_REENTRY L"{19260817-6666-8888-abcd-000000000001}"  //重新进入

#define GUID_BOOTMGR L"{9dea862c-5cdd-4e70-acc1-f32b344d4795}"  //NT6系统
#define GUID_RAMDISK L"{ae5534e0-a924-466c-b836-758539a3ee3a}"  //RAM磁盘
#define GUID_MEMDIAG L"{b2721d73-1db4-4c62-bf78-c548a880142d}"  //MEM镜像
#define GUID_OSNTLDR L"{466f5a88-0af2-4f76-9038-095b170dc21c}"  //NT5系统

#define BCD_REG_ROOT L"Objects"     //注册表  根
#define BCD_REG_HKEY L"Elements"    //注册表  键
#define BCD_REG_HVAL L"Element"     //注册表  值

#define BCDOPT_REPATH   L"12000002" //BCDOPT  重新路径
#define BCDOPT_REHIBR   L"22000002" //BCDOPT  重组

#define BCDOPT_WINLOAD  L"12000002" //BCDOPT  WIN装载
#define BCDOPT_CMDLINE  L"12000030" //BCDOPT  命令行
#define BCDOPT_TESTMODE L"16000049" //BCDOPT  测试模式
#define BCDOPT_HIGHEST  L"16000054" //BCDOPT  最高视频模式
#define BCDOPT_SYSROOT  L"22000002" //BCDOPT  系统根
#define BCDOPT_TIMEOUT  L"25000004" //BCDOPT  超时   {bootmgr}
#define BCDOPT_NX       L"25000020" //BCDOPT  NX
#define BCDOPT_PAE      L"25000021" //BCDOPT  PAE
#define BCDOPT_DETHAL   L"26000010" //BCDOPT  
#define BCDOPT_DISPLAY  L"26000020" //BCDOPT  显示菜单    {bootmgr}
#define BCDOPT_WINPE    L"26000022" //BCDOPT  进入PE
#define BCDOPT_NOVESA   L"26000042" //BCDOPT  不是VESA
#define BCDOPT_NOVGA    L"26000043" //BCDOPT  不是VGA
#define BCDOPT_SOS      L"26000091" //BCDOPT  紧急呼救信号
#define BCDOPT_IMGOFS   L"35000001" //BCDOPT  IMG仿真  {ramdiskoptions}

#define NX_OPTIN     0x00   //NX  选择入口
#define NX_OPTOUT    0x01   //NX  选择出口
#define NX_ALWAYSOFF 0x02   //NX  总是关闭
#define NX_ALWAYSON  0x03   //NX  总是打开
#define PAE_DEFAULT  0x00   //NX  违约
#define PAE_ENABLE   0x01   //NX  使能信号
#define PAE_DISABLE  0x02   //NX  禁止信号

#define BCD_DEFAULT_CMDLINE "DDISABLE_INTEGRITY_CHECKS"             //BCD  默认CMD行

#define BCD_LONG_WINLOAD "\\Windows\\System32\\boot\\winload.efi"   //BCD  长WINLOAD

#define BCD_SHORT_WINLOAD "\\Windows\\System32\\winload.efi"        //BCD  短WINLOAD

#define BCD_DEFAULT_WINRESUME "\\Windows\\System32\\winresume.efi"  //BCD  违约WINRESUME

#define BCD_DEFAULT_SYSROOT "\\Windows"                             //BCD  默认系统根

#define BCD_DEFAULT_HIBERFIL "\\hiberfil.sys"                       //BCD  默认HIBERFIL

struct bcd_disk_info  //bcd磁盘信息
{
  uint8_t partid[16];       //分区ID    MBR: 分区起始字节;  GPT:  分区uuid
  uint32_t unknown;         //保留
  uint32_t partmap;         //分区类型  MBR: 1;             GPT:  0
  uint8_t diskid[16];       //磁盘ID    MBR: 磁盘(分区)id;  GPT:  磁盘uuid
} __attribute__ ((packed));

enum bcd_type //bcd类型
{
  BOOT_WIN,
  BOOT_WIM,
  BOOT_VHD,
} __attribute__ ((packed));

struct nt_args
{
//  int quiet;
//  int pause;
//  int pause_quiet;
//  int text_mode;
//  int linear;
//  char uuid[17];
  char path[256];
//  char initrd_path[256];

  char test_mode[6];
  char hires[6];
  char detecthal[6];
  char minint[6];
  char novesa[6];
  char novga[6];
  char nx[10];
  char pae[8];
  char loadopt[128];
  char winload[64];
  char sysroot[32];

//  char sb[6];
//  int bgrt;
//  int win7;
//  int vgashim;

  enum bcd_type type;
  struct bcd_disk_info info;
//  void *bcd_data;
  grub_uint64_t bcd_data;
  grub_uint32_t bcd_len;
  grub_uint16_t path16[256];
} __attribute__ ((packed));

#endif
