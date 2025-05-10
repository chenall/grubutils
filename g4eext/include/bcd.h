/*
 *  ntloader  --  Microsoft Windows NT6+ loader
 *  Copyright (C) 2025  A1ive.
 *
 *  ntloader is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, either version 3 of the License,
 *  or (at your option) any later version.
 *
 *  ntloader is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ntloader.  If not, see <http://www.gnu.org/licenses/>.
 *  来源：a1ive开发的ntloader-3.0.4
 */

#ifndef _BCD_BOOT_H
#define _BCD_BOOT_H 1

//#include <ntloader.h>
#include <stdint.h>

#define GUID_WIMB L"{19260817-6666-8888-abcd-000000000000}"     //WIM
#define GUID_VHDB L"{19260817-6666-8888-abcd-000000000001}"     //VHD
#define GUID_WOSB L"{19260817-6666-8888-abcd-000000000002}"     //WOS
#define GUID_HIBR L"{19260817-6666-8888-abcd-000000000003}"     //恢复
#define GUID_OPTN L"{19260817-6666-8888-abcd-100000000000}"     //选择

#define GUID_BOOTMGR L"{9dea862c-5cdd-4e70-acc1-f32b344d4795}"  //NT6系统   Windows 启动管理器
#define GUID_RAMDISK L"{ae5534e0-a924-466c-b836-758539a3ee3a}"  //RAM磁盘
#define GUID_MEMDIAG L"{b2721d73-1db4-4c62-bf78-c548a880142d}"  //MEM镜像   Windows 内存测试程序
#define GUID_OSNTLDR L"{466f5a88-0af2-4f76-9038-095b170dc21c}"  //NT5系统
#define GUID_RELDRST L"{1afa9c49-16ab-4a5c-901b-212802da9460}"  //恢复加载器
#define GUID_BTLDRST L"{6efb52bf-1766-41db-a6b3-0ee5eff72bd7}"  //启动加载器

//BIN虚拟盘
#define GUID_BIN_RAMDISK \
    { \
        0xe0, 0x34, 0x55, 0xae, 0x24, 0xa9, 0x6c, 0x46, \
        0xb8, 0x36, 0x75, 0x85, 0x39, 0xa3, 0xee, 0x3a \
    }

#define BCD_REG_ROOT L"Objects"     //注册表  根
#define BCD_REG_HKEY L"Elements"    //注册表  键
#define BCD_REG_HVAL L"Element"     //注册表  值

#define BCDOPT_APPDEV   L"11000001" // application device         系统启动程序设备
#define BCDOPT_WINLOAD  L"12000002" // winload & winresume        启动程序路径
#define BCDOPT_TITLE    L"12000004" //                            程序名称
#define BCDOPT_LANG     L"12000005" //                            首选区域设置
#define BCDOPT_CMDLINE  L"12000030" //                            加载选项字符串
#define BCDOPT_INHERIT  L"14000006" // options                    options
#define BCDOPT_GFXMODE  L"15000052" // graphicsresolution         图形分辨率
#define BCDOPT_ADVOPT   L"16000040" // advanced options           高级选项
#define BCDOPT_OPTEDIT  L"16000041" // options edit               选项编辑
#define BCDOPT_TEXT     L"16000046" // graphicsmodedisabled       图形模式已禁用
#define BCDOPT_TESTMODE L"16000049" // testsigning                测试签名
#define BCDOPT_HIGHRES  L"16000054" // highest resolution         最高分辨率
#define BCDOPT_OSDDEV   L"21000001" // os device                  操作系统设备
#define BCDOPT_SYSROOT  L"22000002" // os root & hiberfil         系统文件路径
#define BCDOPT_OBJECT   L"23000003" // resume / default object    恢复/默认对象
#define BCDOPT_ORDER    L"24000001" // menu display order         BCD对象应该显示的顺序
#define BCDOPT_TIMEOUT  L"25000004" // menu timeout               菜单超时
#define BCDOPT_NX       L"25000020" // NX
#define BCDOPT_PAE      L"25000021" // PAE
#define BCDOPT_SAFEMODE L"25000080" // safeboot                   安全模式
#define BCDOPT_DETHAL   L"26000010" // detect hal                 检测hal    表明系统加载程序
#define BCDOPT_DISPLAY  L"26000020" // menu display ui            菜单显示ui
#define BCDOPT_WINPE    L"26000022" // minint winpe mode          迷你pe模式
#define BCDOPT_NOVESA   L"26000042" //                            不是VESA
#define BCDOPT_NOVGA    L"26000043" //                            不是VGA
#define BCDOPT_ALTSHELL L"26000081" // safeboot alternate shell   安全模式备用外壳
#define BCDOPT_SOS      L"26000091" //                            紧急呼救信号
#define BCDOPT_SDIDEV   L"31000003" // sdi ramdisk device         sdi ramdisk设备
#define BCDOPT_SDIPATH  L"32000004" // sdi ramdisk path           sdi ramdisk路径
#define BCDOPT_IMGOFS   L"35000001" // ramdisk image offset       ramdisk映像偏移
#define BCDOPT_EXPORTCD L"36000006" // ramdisk export as cd       ramdisk导出为cd

#define NX_OPTIN     0x00   //NX  选择入口
#define NX_OPTOUT    0x01   //NX  选择出口
#define NX_ALWAYSOFF 0x02   //NX  总是关闭
#define NX_ALWAYSON  0x03   //NX  总是打开

#define PAE_DEFAULT  0x00   //PAE 默认
#define PAE_ENABLE   0x01   //PAE 使能信号
#define PAE_DISABLE  0x02   //PAE 禁止信号

#define SAFE_MINIMAL  0x00  //安全 最小的
#define SAFE_NETWORK  0x01  //安全 网络
#define SAFE_DSREPAIR 0x02  //安全 修复

#define GFXMODE_1024X768 0x00 //分辨率 1024X768
#define GFXMODE_800X600  0x01 //分辨率 800X600
#define GFXMODE_1024X600 0x02 //分辨率 1024X600

#define BCD_DEFAULT_CMDLINE "/DISABLE_INTEGRITY_CHECKS"             //BCD  禁用完整性检查

#define BCD_LONG_WINLOAD "\\Windows\\System32\\boot\\winload.efi"   //BCD  长WINLOAD

#define BCD_SHORT_WINLOAD "\\Windows\\System32\\winload.efi"        //BCD  短WINLOAD

#define BCD_DEFAULT_WINRESUME "\\Windows\\System32\\winresume.efi"  //BCD  默认WIN简历

#define BCD_DEFAULT_SYSROOT "\\Windows"                             //BCD  默认系统根

#define BCD_DEFAULT_HIBERFIL "\\hiberfil.sys"                       //BCD  默认HIBERFIL

#define MAX_PATH 255

#define NTBOOT_WIM 0x00
#define NTBOOT_VHD 0x01
#define NTBOOT_WOS 0x02
#define NTBOOT_REC 0x03   //恢复
#define NTBOOT_RAM 0x04
#if 0
#define NTBOOT_APP 0x05
#define NTBOOT_MEM 0x06
#endif

#define NTARG_BOOL_TRUE  1
#define NTARG_BOOL_FALSE 0
#define NTARG_BOOL_UNSET 0xff   //未设置
#define NTARG_BOOL_NA    0x0f

struct nt_args
{
    uint8_t textmode;
    uint8_t testmode;
    uint8_t hires;
    uint8_t hal;
    uint8_t minint;
    uint8_t novga;
    uint8_t novesa;
    uint8_t safemode;
    uint8_t altshell;
    uint8_t exportcd;
    uint8_t advmenu;
    uint8_t optedit;

    uint64_t nx;
    uint64_t pae;
    uint64_t timeout;
    uint64_t safeboot;
    uint64_t gfxmode;
    uint64_t imgofs;

    char loadopt[128];
    char winload[64];
    char sysroot[32];

    uint32_t boottype;
//    char fsuuid[17];
    uint8_t partid[16]; //分区ID    MBR: 分区起始字节;  GPT:  分区uuid
    uint8_t diskid[16]; //磁盘ID    MBR: 磁盘(分区)id;  GPT:  磁盘uuid
    uint8_t partmap;    //分区类型  MBR: 1;             GPT:  0
    char filepath[MAX_PATH + 1];

//    char initrd_path[MAX_PATH + 1];
    void *bcd;
    uint32_t bcd_length;
//    void *bootmgr;
//    uint32_t bootmgr_length;
};

#endif
