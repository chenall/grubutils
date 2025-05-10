

/*
 * compile:
 * gcc -Wl,--build-id=none -m64 -mno-sse -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE ntboot.c -o ntboot.o 2>&1|tee build.log
 * objcopy -O binary ntboot.o ntboot
 *
 * analysis:
 * objdump -d ntboot.o > ntboot.s
 * readelf -a ntboot.o > a.s
 * bcd修改参考 ntloader-3.0.4  鸣谢a1ive！
 */

#include <grub4dos.h>
#include <uefi.h>
#include <bcd.h>
#include <reg.h>

//在此定义静态变量、结构。不能包含全局变量，否则编译可以通过，但是不能正确执行。
//不能在main前定义子程序，在main后可以定义子程序。
static grub_size_t g4e_data = 0;
static void get_G4E_image(void);

typedef __WCHAR_TYPE__ wchar_t;
typedef __WINT_TYPE__ wint_t;
char temp[256];
static void convert_path (char *str, int backslash);
static int islower (int c);
static int toupper (int c);
static unsigned long strtoul (const char *nptr, char **endptr, int base);
static void bcd_patch_data (void);
static enum reg_bool check_header(hive_t *h);
static void reg_find_root(hive_t *h, HKEY *Key);
static reg_err_t reg_enum_keys(hive_t *h, HKEY Key, uint32_t Index,
              wchar_t *Name, uint32_t NameLength);
static reg_err_t find_child_key (hive_t* h, HKEY parent,
                const grub_uint16_t* namebit, grub_size_t nblen, HKEY* key);
static reg_err_t reg_find_key(hive_t *h, HKEY Parent, const wchar_t *Path, HKEY *Key);
static reg_err_t reg_enum_values(hive_t *h, HKEY Key,
                uint32_t Index, wchar_t *Name, uint32_t NameLength, uint32_t *Type);
static reg_err_t
reg_query_value(hive_t *h, HKEY Key, const wchar_t *Name, void **Data,
                uint32_t *DataLength, uint32_t *Type);
static void clear_volatile (hive_t* h, HKEY key);
static reg_err_t reg_open_hive(hive_t *h);
static void process_cmdline (char *arg);
static int modify_bcd (char *filename, int flags);
static int strcasecmp (const char *str1, const char *str2);
//static void bcd_replace_suffix (const wchar_t *src, const wchar_t *dst);
static void *bcd_find_hive (hive_t *hive, HKEY objects,
               const wchar_t *guid, const wchar_t *keyname, uint32_t *len);
static void bcd_delete_key (hive_t *hive, HKEY objects,
                const wchar_t *guid, const wchar_t *keyname);
static void bcd_patch_bool (hive_t *hive, HKEY objects,
                const wchar_t *guid, const wchar_t *keyname, uint8_t val);
static void bcd_patch_u64 (hive_t *hive, HKEY objects,
               const wchar_t *guid, const wchar_t *keyname, uint64_t val);
static void bcd_patch_sz (hive_t *hive, HKEY objects,
              const wchar_t *guid, const wchar_t *keyname, const char *str);
static void bcd_patch_szw (hive_t *hive, HKEY objects,
               const wchar_t *guid, const wchar_t *keyname, const wchar_t *str);
static void bcd_patch_dp (hive_t *hive, HKEY objects, uint32_t boottype,
              const wchar_t *guid, const wchar_t *keyname);
static int32_t get_int32_size (uint8_t *data, uint64_t offset);
static reg_err_t reg_delete_key(hive_t *h, HKEY parent, HKEY key);
static enum reg_bool validate_bins(const uint8_t *data, grub_size_t len);
static grub_size_t wcslen (const wchar_t *str);
struct nt_args *args;

/* 这是必需的，请参阅grubprog.h中的注释 */
#include <grubprog.h>
/* 请勿在此处插入任何其他asm行 */
//不能包含子程序，不能包含全局变量，否则编译可以通过，但是不能正确执行。
static int main(char *arg,int flags);
static int main(char *arg,int flags)
{
  int i = 1, j, test = 0;
  char *filename, *buf = 0;
  char tmp[64] = {0};

  get_G4E_image();
  if (! g4e_data)
    return 0;

  if (*(unsigned int *)IMG(0x8278) < 20250210)
  {
    printf("Please use grub4efi version above 2023-07-14.\n");
    return 0;
  }
  if (memcmp (arg, "--test", 6) == 0)		//测试  网起时，修改主机(服务器)的'/boot/bcd'
  {
    test = 1;
    arg = skip_to(0x200,arg);
  }

  //初始化参数，默认值
  args = (struct nt_args *)(void *)zalloc (sizeof(struct nt_args));
//  args->textmode = NTARG_BOOL_FALSE;
//  args->testmode = NTARG_BOOL_FALSE;
  args->timeout = 0;
  args->hires = NTARG_BOOL_UNSET;     //0xff    未设置   类型是NTBOOT_WIM时，设置为真，否则为假
  args->hal = NTARG_BOOL_TRUE;        //1
  args->minint = NTARG_BOOL_UNSET;    //0xff    未设置   类型是NTBOOT_WIM时，设置为真，否则为假
//  args->novga = NTARG_BOOL_FALSE;     //0
//  args->novesa = NTARG_BOOL_FALSE;    //0
//  args->safemode = NTARG_BOOL_FALSE;  //0
//  args->altshell = NTARG_BOOL_FALSE;  //0
//  args->exportcd = NTARG_BOOL_FALSE;  //0
//  args->advmenu = NTARG_BOOL_FALSE;   //0
//  args->optedit = NTARG_BOOL_FALSE;   //0
//  args->nx = NX_OPTIN;                //0       选择入口
//  args->pae = PAE_DEFAULT;            //0       默认
//  args->safeboot = SAFE_MINIMAL;      //0       最小的
//  args->gfxmode = GFXMODE_1024X768;   //0       分辨率 1024X768
  args->imgofs = 65536;
//  args->winload[0] == '\0'                            类型是NTBOOT_WIM时，设置为BCD_LONG_WINLOAD，否则为BCD_SHORT_WINLOAD
  sprintf (args->loadopt, "%s", BCD_DEFAULT_CMDLINE); //禁用完整性检查
  sprintf (args->sysroot, "%s", BCD_DEFAULT_SYSROOT); //默认系统根  \\Windows
  
 
  filename = arg;
  arg = skip_to(0x200,arg);
  char *suffix = &filename[strlen (filename) - 3]; //取尾缀
  //判断尾缀是否为WIM/VHD/WIN
  if (substring(suffix,"wim",1) == 0)
    args->boottype = NTBOOT_WIM;                   //启动类型
  else if (substring(suffix,"vhd",1) == 0 || substring(suffix,"hdx",1) == 0)
    args->boottype = NTBOOT_VHD;
  else if (substring(suffix,"win",1) == 0)
    args->boottype = NTBOOT_WOS;
  else
    goto load_fail;  //是常规

  if (current_drive == 0x21 && args->boottype != NTBOOT_WIM) //网起只允许wim格式
    goto load_fail;  //无效
  process_cmdline (arg);  //处理命令行参数

  if (current_drive == 0x21)
    goto abc;
  //处理字符串中的空格
  for (i = 0, j = 0; i < strlen (filename); i++,j++)
  {
    if (filename[i] == '\\' && filename[i+1] == ' ')
    {
      filename[j] = ' ';
      i++;
    }
    else
      filename[j] = filename[i];
  }
  filename[j] = 0;

abc:
  if (!test)
  {
    int current_drive_back = current_drive;
    int current_partition_back = current_partition;
    sprintf(tmp,"map --mem --no-hook (md)0x%x+0x%x (hd)",bat_md_start,bat_md_count);  //加载尾续文件  map函数会改变当前驱动器号
    run_line (tmp,flags);     
    current_drive = current_drive_back;
    current_partition = current_partition_back;
  }
  else
  {
    memmove (args->filepath, filename, 256); //复制文件路径
    convert_path (args->filepath, 1);        //转换路径
    open ("/boot/bcd");  //打开bcd文件
    if (current_drive == 0x21)
      args->bcd = (void *)efi_pxe_buf;
    else
    {
      buf = zalloc (filemax+2);//3bc9a860
      read ((unsigned long long)(grub_size_t)buf, filemax, GRUB_READ);
      args->bcd = (void *)buf;
    }
    args->bcd_length = filemax;
  }

  if (saved_drive == 0x21)
  {
    current_drive = saved_drive;
    current_partition = saved_partition;
//    args->safeboot = SAFE_NETWORK;
//    args->safemode = NTARG_BOOL_TRUE;

    //创建内存盘
    char *f;
    if (filename[0] == '(')
      f = filename + 6;
    else if (filename[0] == '/')
      f = filename;

    sprintf(tmp,"cat --length=0 (tftp)%s",f);        //确定wim文件尺寸
    run_line (tmp,flags); 

    static efi_system_table_t *st;
    static efi_boot_services_t *bs;
    st = grub_efi_system_table;
    bs = st->boot_services;
    grub_efi_status_t status;
    status = bs->allocate_pages (EFI_ALLOCATE_ANY_PAGES,   
          EFI_RESERVED_MEMORY_TYPE,                        //保留内存类型        0
          (grub_efi_uintn_t)((filesize + 0xc00000)&(-4096ULL)) >> 12, (unsigned long long *)(grub_size_t)&efi_pxe_buf);	//调用(分配页面,分配类型->任意页面,存储类型->运行时服务数据(6),分配页,地址)  
    if (status != EFI_SUCCESS)	//如果失败
    {
      printf_errinfo ("out of map memory: %d\n",(int)status);
      return 0;
    }

    rd_base = efi_pxe_buf;
    rd_size = (filesize + 0xc00000)&(-4096ULL);
    unsigned long long len = filesize;
    memset ((void *)rd_base, 0, rd_size); 

    //格式化内存盘
    sprintf(tmp,"(hd-1,0)/fat mkfs /A:4096 mbr (rd)");
    run_line (tmp,flags);

    //创建目录
    run_line ("find --set-root --devices=h /fat", flags);    //设置根
    run_line ("/fat mkdir (rd)/boot",flags);
    run_line ("/fat mkdir (rd)/efi",flags);
    run_line ("/fat mkdir (rd)/efi/boot",flags);

    run_line ("/fat mkdir (rd)/efi/microsoft",flags);   //fat函数不能创建长文件名
    run_line ("/fat mkdir (rd)/efi/microsoft/boot",flags);
    run_line ("/fat mkdir (rd)/efi/microsoft/boot/fonts",flags);
    run_line ("/fat mkdir (rd)/efi/microsoft/boot/resources",flags);

    //复制文件
    run_line ("cat --length=0 /bcdnet",flags);          //获取文件尺寸
    run_line ("/fat mkfile size=* (rd)/bcd",flags);     //创建文件信息,确定起始簇,分配簇.
    run_line ("/fat copy /o /bcdnet (rd)/bcd",flags);   //复制具体内容
    args->bcd = (void *)(rd_base + (ext_data_1 << 9));  //bcd在内存地址
    args->bcd_length = filesize;                        //bcd尺寸
    printf_debug ("args->bcd=%x, rd_base=%x, ext_data_1=%x, args->bcd_length=%x\n",args->bcd,rd_base,ext_data_1,args->bcd_length);

    run_line ("cat --length=0 /bootx64.efi",flags);
    run_line ("/fat mkfile size=* (rd)/bootx64.efi",flags);
    run_line ("/fat copy /o /bootx64.efi (rd)/bootx64.efi",flags);

    run_line ("cat --length=0 /boot/boot.sdi",flags);
    run_line ("/fat mkfile size=* (rd)/boot/boot.sdi",flags);
    run_line ("/fat copy /o /boot/boot.sdi (rd)/boot/boot.sdi",flags);

    run_line ("/fat mkfile size=0 (rd)/efi/boot/bootx64.efi",flags);
    run_line ("/fat copy /o /efi/boot/bootx64.efi (rd)/efi/boot/bootx64.efi",flags);

    run_line ("cat --length=0 /efi/microsoft/boot/fonts/wgl4_boot.ttf",flags);
    run_line ("/fat mkfile size=* (rd)/efi/microsoft/boot/fonts/wgl4_boot.ttf",flags);
    run_line ("/fat copy /o /efi/microsoft/boot/fonts/wgl4_boot.ttf (rd)/efi/microsoft/boot/fonts/wgl4_boot.ttf",flags);

    run_line ("cat --length=0 /efi/microsoft/boot/fonts/segoe_slboot.ttf",flags);
    run_line ("/fat mkfile size=* (rd)/efi/microsoft/boot/fonts/segoe_slboot.ttf",flags);
    run_line ("/fat copy /o /efi/microsoft/boot/fonts/segoe_slboot.ttf (rd)/efi/microsoft/boot/fonts/segoe_slboot.ttf",flags);

    run_line ("cat --length=0 /efi/microsoft/boot/fonts/segmono_boot.ttf",flags);
    run_line ("/fat mkfile size=* (rd)/efi/microsoft/boot/fonts/segmono_boot.ttf",flags);
    run_line ("/fat copy /o /efi/microsoft/boot/fonts/segmono_boot.ttf (rd)/efi/microsoft/boot/fonts/segmono_boot.ttf",flags);

    run_line ("cat --length=0 /efi/microsoft/boot/resources/bootres.dll",flags);
    run_line ("/fat mkfile size=* (rd)/efi/microsoft/boot/resources/bootres.dll",flags);
    run_line ("/fat copy /o /efi/microsoft/boot/resources/bootres.dll (rd)/efi/microsoft/boot/resources/bootres.dll",flags);

    sprintf(tmp,"/fat mkfile size=0x%x (rd)/boot.wim",len);
    run_line (tmp,flags);
    bs->stall (10000); //延时10毫秒

    saved_drive = 0x21;
    saved_partition = 0xffffff;
    //读wim文件
    unsigned long long buf1;
    efi_pxe_buf = rd_base + (ext_data_1 << 9);
    printf_debug ("efi_pxe_buf=%x, rd_base=%x, ext_data_1=%x, filename=%s\n",efi_pxe_buf,rd_base,ext_data_1,filename);//6cd16000,6c7b3000,2b18
//    read (buf1, 0, GRUB_READ);
    *(char *)IMG(0x8205) |= 0x80; //如果0x8205位7置1，使用efi_pxe_buf，不要分配内存
    open (filename);        //读wim文件到efi_pxe_buf
    *(char *)IMG(0x8205) &= 0x7f; //恢复
    bs->stall (50000); //延时50毫秒

    //关闭wim文件
    close ();
    //正式创建虚拟硬盘
    run_line ("map --no-alloc (rd)+1 (hd)",flags);
    //加载引导文件并启动
    run_line ("chainloader (hd-1,0)/bootx64.efi",flags);
    
    if (debug >= 3)
      getkey();  //加载是因为缺少所需文件或包含错误。
    run_line ("boot",flags);
    return 0;
  }
  else
    modify_bcd (filename, flags); //修改bcd

  bcd_patch_data ();  //bcd修补程序数据
  if (debug >= 3)
    getkey();
    

  if (!test)
  {
    run_line ("chainloader (hd-1,0)/bootx64.efi",flags);    
    run_line ("boot",flags);
  }
  else
  {
    if (current_drive == 0x21)
      tftp_write ("/boot/bcd");  //保存bcd
    else
    {
      filepos = 0;
      read ((unsigned long long)(grub_size_t)buf, filemax, GRUB_WRITE);
    }
    close ();  //关闭bcd文件
    if (buf)
      free (buf);
  }
  if (debug >= 3)
    getkey();

  return 1;
load_fail:
  printf_errinfo ("load fail\n");
  return 0;
}

static void get_G4E_image(void)
{
  grub_size_t i;

  //在内存0-0x9ffff, 搜索特定字符串"GRUB4EFI"，获得GRUB_IMGE
  for (i = 0x40100; i <= 0x9f100 ; i += 0x1000)
  {
    if (*(unsigned long long *)i == 0x4946453442555247)	//比较数据 "GRUB4EFI"
    {
      g4e_data = *(grub_size_t *)(i+16); //GRUB4DOS_for_UEFI入口
      return;
    }
  }
  return;
}

static void
convert_path (char *str, int backslash)  //转换路径
{
  char *p = str;
  while (*p)
  {
    if (*p == '/' && backslash)
      *p = '\\';
    if (*p == ':')
      *p = ' ';
    if (*p == '\r' || *p == '\n')
      *p = '\0';
    p++;
  }
}

static uint8_t
convert_bool (const char *str)  //转换bool
{
  uint8_t value = NTARG_BOOL_FALSE;
  if (! str ||
      strcasecmp (str, "yes") == 0 ||
      strcasecmp (str, "on") == 0 ||
      strcasecmp (str, "true") == 0 ||
      strcasecmp (str, "1") == 0)
      value = NTARG_BOOL_TRUE;
  return value;
}

static void process_cmdline (char *arg)  //处理命令行参数
{
  char *tmp = arg;
  char *key;
  char *value;
  char chr;
  /* 分析命令行 */
  while (*tmp)
  {
    /* 跳过空白*/
    while (isspace (*tmp))
      tmp++;
    /*查找值（如果有）并结束此参数 */
    key = tmp;
    value = NULL;
    while (*tmp)
    {
      if (isspace (*tmp))
      {
        * (tmp++) = '\0';
        break;
      }
      else if (*tmp == '=')
      {
        * (tmp++) = '\0';
        value = tmp;
      }
      else
        tmp++;
    }
    /* 处理命令行参数*/
#if 0
    if (strcmp (key, "uuid") == 0)
    {
      sprintf (args->fsuuid, "%s", value);
    }    
    else if (strcmp (key, "wim") == 0)
    {
      sprintf (args->filepath, "%s", value);   //文件路径
      convert_path (args->filepath, 1);              //转换路径
      args->boottype = NTBOOT_WIM;                   //启动类型
    }
    else if (strcmp (key, "vhd") == 0)
    {
      sprintf (args->filepath, "%s", value);
      convert_path (args->filepath, 1);
      args->boottype = NTBOOT_VHD;
    }
    else if (strcmp (key, "ram") == 0)
    {
      sprintf (args->filepath, "%s", value);
      convert_path (args->filepath, 1);
      args->boottype = NTBOOT_RAM;
    }
    else if (strcmp (key, "file") == 0)
    {
      sprintf (args->filepath, "%s", value);
      convert_path (args->filepath, 1);
      char *ext = strrchr (value, '.');
      if (ext && strcasecmp(ext, ".wim") == 0)
        args->boottype = NTBOOT_WIM;
      else
        args->boottype = NTBOOT_VHD;
    }
    else if (strcmp (key, "text") == 0)       //文本
    {
      args->textmode = NTARG_BOOL_TRUE;
    }
    else if (strcmp (key, "testmode") == 0)   //测试
    {
      args->testmode = convert_bool (value);
    }
    else if (strcmp (key, "detecthal") == 0)  //检测
    {
      args->hal = convert_bool (value);
    }
    else if (strcmp (key, "novesa") == 0)     //非vesa
    {
      args->novesa = convert_bool (value);
    }
    else if (strcmp (key, "altshell") == 0)   //alt外壳
    {
      args->altshell = convert_bool (value);
      args->safemode = NTARG_BOOL_TRUE;
    }
    else if (strcmp (key, "exportascd") == 0) //导出ascd
    {
      args->exportcd = convert_bool (value);
    }
    else if (strcmp (key, "f8") == 0)
    {
      args->advmenu = NTARG_BOOL_TRUE;         //f8
      args->textmode = NTARG_BOOL_TRUE;
    }
    else if (strcmp (key, "edit") == 0)       //编辑
    {
      args->optedit = NTARG_BOOL_TRUE;
      args->textmode = NTARG_BOOL_TRUE;
    }
    else if (strcmp (key, "nx") == 0)         //NX
    {
      if (! value || strcasecmp (value, "OptIn") == 0)  //选择入口
        args->nx = NX_OPTIN;
      else if (strcasecmp (value, "OptOut") == 0)       //选择出口
        args->nx = NX_OPTOUT;
      else if (strcasecmp (value, "AlwaysOff") == 0)    //总是关闭
        args->nx = NX_ALWAYSOFF;
      else if (strcasecmp (value, "AlwaysOn") == 0)     //总是打开
        args->nx = NX_ALWAYSON;
    }
    else if (strcmp (key, "pae") == 0)    //PAE
    {
      if (! value || strcasecmp (value, "Default") == 0)//默认
        args->pae = PAE_DEFAULT;
      else if (strcasecmp (value, "Enable") == 0)       //使能
        args->pae = PAE_ENABLE;
      else if (strcasecmp (value, "Disable") == 0)      //禁止
        args->pae = PAE_DISABLE;
    }
    else if (strcmp (key, "novga") == 0)      //非vga
    {
      args->novga = convert_bool (value);
    }
    else if (strcmp (key, "initrdfile") == 0 ||
        strcmp (key, "initrd") == 0)          //初始化内存盘
    {
      sprintf (args->initrd_path, "%s", value);
      convert_path (args->initrd_path, 1);
    }
#endif      
    if (strcmp (key, "hires") == 0)           //最高分辨率
    {
      args->hires = convert_bool (value);
    }
    else if (strcmp (key, "minint") == 0)     //pe模式
    {
      args->minint = convert_bool (value);
    }
    else if (strcmp (key, "safemode") == 0)   //安全模式
    {
      args->safemode = NTARG_BOOL_TRUE;
      if (! value || strcasecmp (value, "Minimal") == 0)  //最小的
        args->safeboot = SAFE_MINIMAL;
      else if (strcasecmp (value, "Network") == 0)        //网络
        args->safeboot = SAFE_NETWORK;
      else if (strcasecmp (value, "DsRepair") == 0)       //修复
        args->safeboot = SAFE_DSREPAIR;
    }
    else if (strcmp (key, "gfxmode") == 0)    //设置分辨率
    {
      args->hires = NTARG_BOOL_NA;                        //无设置
      if (! value || strcasecmp (value, "1024x768") == 0) //默认
        args->gfxmode = GFXMODE_1024X768;
      else if (strcasecmp (value, "800x600") == 0)
        args->gfxmode = GFXMODE_800X600;
      else if (strcasecmp (value, "1024x600") == 0)
        args->gfxmode = GFXMODE_1024X600;
    }
    else if (strcmp (key, "imgofs") == 0)     //镜象偏移
    {
      char *endp;
      args->imgofs = strtoul (value, &endp, 0);
    }
    else if (strcmp (key, "loadopt") == 0)    //禁用完整性检查
    {
      sprintf (args->loadopt, "%s", value);
      convert_path (args->loadopt, 0);
    }
    else if (strcmp (key, "winload") == 0)    //win装栽
    {
      sprintf (args->winload, "%s", value);
      convert_path (args->winload, 1);
    }
    else if (strcmp (key, "sysroot") == 0)    //系统根
    {
      sprintf (args->sysroot, "%s", value);
      convert_path (args->sysroot, 1);
    }
    else
    {
      /* Ignore unknown arguments   忽略未知参数*/
    }

    /* Undo modifications to command line   撤消对命令行的修改*/
    if (chr)
      tmp[-1] = chr;
    if (value)
      value[-1] = '=';
  }
  //如果以下参数未设置，则初始化参数（属于默认参数）
  if (args->hires == NTARG_BOOL_UNSET)   //如果雇用未设置
  {
    if (args->boottype == NTBOOT_WIM)    //如果引导类型是WIM
    {
      args->hires = NTARG_BOOL_TRUE; //真
    }
    else
      args->hires = NTARG_BOOL_FALSE;//假
  }
  if (args->minint == NTARG_BOOL_UNSET)  //如果minint未设置
  {
    if (args->boottype == NTBOOT_WIM)    //如果引导类型是WIM
    {
      args->minint = NTARG_BOOL_TRUE; //真
    }
    else
      args->minint = NTARG_BOOL_FALSE;//假
  }
  if (args->winload[0] == '\0')          //如果winload为空
  {
    if (args->boottype == NTBOOT_WIM)    //如果引导类型是WIM
    {
      sprintf (args->winload, "%s", BCD_LONG_WINLOAD);    //长字符串
    }
    else
      sprintf (args->winload, "%s", BCD_SHORT_WINLOAD);   //短字符串
  }
}

static int modify_bcd (char *filename, int flags) //修改bcd名称，设置bcd磁盘信息及bcd位置/尺寸
{
  grub_uint64_t start_addr;
	efi_status_t status;
  struct grub_part_data *p;
  struct grub_disk_data *d;

  printf_debug("current_drive=%x, current_partition=%x, saved_drive=%x, saved_partition=%x\n",
          current_drive, current_partition, saved_drive, saved_partition);
  printf_debug("filename=%s\n",filename);//boot/imgs/10pe.wim
  //确定驱动器号
  if (filename[0] == '(')
  {
    char a = filename[1];
    filename = set_device (filename); //设置当前驱动器=输入驱动器号, 当前分区=输入分区号, 其余作为文件名 /efi/boot/bootx64.efi
    printf_debug("current_drive=%x, current_partition=%x, saved_drive=%x, saved_partition=%x\n",
          current_drive, current_partition, saved_drive, saved_partition);
    printf_debug("filename=%s\n",filename);//boot/imgs/10pe.wim
    if (a != ')')
    {
      p = get_partition_info (current_drive, current_partition);
      d = get_device_by_drive (current_drive,0);
      goto qwer;
    }
  }
  p = get_partition_info (saved_drive, saved_partition);
  d = get_device_by_drive (saved_drive,0);
qwer:
  printf_debug("partition_type=%x, partition_start=%x, partition_size=%x\n",
          p->partition_type,p->partition_start,p->partition_size);
  //复制文件路径
  memmove (args->filepath, filename, 256);              //复制文件路径
  convert_path (args->filepath, 1);                     //转换路径
  //确定分区类型,分区起始字节(或者分区uuid),磁盘id(或者磁盘uuid)
  if (p->partition_type != 0xee)                        //mbr分区
  {
    start_addr = p->partition_start << 9;
    memmove (args->partid, &start_addr, 8);             //分区起始字节
    args->partmap = 0x01;                               //分区类型
    memmove (args->diskid, p->partition_signature, 4);  //磁盘id
    printf_debug("MBR: partition_start=%x, partition_signature=%x\n",
            p->partition_start,*(unsigned int *)&p->partition_signature);
            
  }
  else	//gpt分区
  {
    memmove (args->partid, p->partition_signature, 16); //分区uuid
    args->partmap = 0x00;                               //分区类型
    memmove (args->diskid, d->disk_signature, 16);      //磁盘uuid
    printf_debug("GPT: diskid_signature=%x, partition_signature=%x\n",
            *(unsigned long long *)&d->disk_signature,*(unsigned long long *)&p->partition_signature);
  }
  //安装虚拟磁盘
  run_line ("find --set-root /fat", flags);    //设置根到(hd-1)
  printf_debug("current_drive=%x, current_partition=%x, saved_drive=%x, saved_partition=%x\n",
          current_drive, current_partition, saved_drive, saved_partition);
  status = vdisk_install (current_drive, current_partition);  //安装虚拟磁盘
  if (status != EFI_SUCCESS)
  {
    printf_errinfo ("Failed to install vdisk.(%x)\n",status);	//未能安装vdisk
    return 0;
  }

  //通过外部命令fat, 重命名  bcdxxx -> bcd
  sprintf (temp,"/fat ren /bcdset bcd"); //外部命令，已打包
  run_line((char *)temp,1);
  //确定bcd位置及尺寸
  sprintf(temp,"map --status=0x%x",current_drive);
  run_line(temp,1);
  run_line("blocklist /bcd",255);   //获得bcd在ntboot的偏移以及尺寸
  args->bcd = (void *)((*(long long *)ADDR_RET_STR + (grub_size_t)map_start_sector[0]) << 9);  //(ntboot在内存的扇区起始+bcd相对于ntboot的扇区偏移)转字节
  args->bcd_length = filemax;   //(bcd扇区尺寸)转字节

  printf_debug("bcd=%x, bcd_length=%x\n",
          args->bcd, args->bcd_length);
  printf_debug("current_drive=%x, current_partition=%x, saved_drive=%x, saved_partition=%x\n",
          current_drive, current_partition, saved_drive, saved_partition);
  if (debug >= 3)
    run_line("pause", 1);
  return 1;
}

static int islower (int c)  //是ASCII大写字符
{
    return ((c >= 'a') && (c <= 'z'));
}

static int toupper (int c)  //ASCII字符大写转小写
{
    if (islower (c))
        c -= ('a' - 'A');
    return c;
}

/**
 * Compare two strings, case-insensitively    比较两个字符串，不区分大小写
 *
 * @v str1    First string                    第一个字符串
 * @v str2    Second string                   第二个字符串
 * @ret diff    Difference                    返回不同
 */
static int strcasecmp (const char *str1, const char *str2)
{
  int c1;
  int c2;
  do
  {
    c1 = toupper (* (str1++));
    c2 = toupper (* (str2++));
  }
  while ((c1 != '\0') && (c1 == c2));
  return (c1 - c2);
}

/**
 * Convert a string to an unsigned integer        将字符串转换为无符号整数
 *
 * @v nptr    String                              字符串
 * @v endptr    End pointer to fill in (or NULL)  要填充的结束指针（或NULL）
 * @v base    Numeric base                        数字基数
 * @ret val   Value                               返回值
 */
static unsigned long strtoul (const char *nptr, char **endptr, int base)
{
  unsigned long val = 0;
  int negate = 0;
  unsigned int digit;
  /* Skip any leading whitespace 跳过任何前导空格*/
  while (isspace (*nptr))
  {
    nptr++;
  }
  /* Parse sign, if present 分析符号（如果存在）*/
  if (*nptr == '+')
  {
    nptr++;
  }
  else if (*nptr == '-')
  {
    nptr++;
    negate = 1;
  }
  /* Parse base 分析基础*/
  if (base == 0)
  {
    /* Default to decimal 默认为十进制*/
    base = 10;
    /* Check for octal or hexadecimal markers 检查八进制或十六进制标记*/
    if (*nptr == '0')
    {
      nptr++;
      base = 8;
      if ((*nptr | 0x20) == 'x')
      {
        nptr++;
        base = 16;
      }
    }
  }
  /* Parse digits 分析数字*/
  for (; ; nptr++)
  {
    digit = *nptr;
    if (digit >= 'a')
    {
      digit = (digit - 'a' + 10);
    }
    else if (digit >= 'A')
    {
      digit = (digit - 'A' + 10);
    }
    else if (digit <= '9')
    {
      digit = (digit - '0');
    }
    if (digit >= (unsigned int) base)
    {
      break;
    }
    val = ((val * base) + digit);
  }
  /* Record end marker, if applicable 记录结束标记（如适用）*/
  if (endptr)
  {
    *endptr = ((char *) nptr);
  }
  /* Return value 返回值*/
  return (negate ? -val : val);
}

////////////////////////////////////////////////////////////////////////////////////////////////
//bcd.c
#if 0
static void
bcd_replace_suffix (const wchar_t *src, const wchar_t *dst)  //替换后缀
{
    uint8_t *p = args->bcd;
    uint32_t ofs;
    const grub_size_t len = sizeof(wchar_t) * 5; // . E F I \0
    for (ofs = 0; ofs + len < args->bcd_length; ofs++)
    {
        if (memcmp (p + ofs, (const char *)src, len) == 0)
        {
            memmove (p + ofs, (const char *)dst, len);
            printf_debug ("...patched_suffix BCD at %#x (%ls->%ls)\n", ofs, src, dst);
        }
    }
}
#endif

char tmp0[32], tmp1[32], tmp2[32];
static void wchar_to_char (const wchar_t *guid, const wchar_t *keyname, const wchar_t *data);
static void wchar_to_char (const wchar_t *guid, const wchar_t *keyname, const wchar_t *data)
{
  int i;
  if (guid)
  {
    for (i=0; i<8; i++)
      tmp0[i] = *(char *)&guid[i];
    tmp0[i] = 0;
  }
  if (keyname)
  {
    for (i=0; i<8; i++)
      tmp1[i] = *(char *)&keyname[i];
    tmp1[i] = 0;
  }
  if (data)
  {
    for (i=0; i<8; i++)
      tmp2[i] = *(char *)&data[i];
    tmp2[i] = 0;
  }
}

static void *
bcd_find_hive (hive_t *hive, HKEY objects,
               const wchar_t *guid, const wchar_t *keyname,
               uint32_t *len)  //查找蜂巢
{
    HKEY entry, elements, key;
    void *data = NULL;
    uint32_t type;

    if (reg_find_key (hive, objects, guid, &entry) != REG_ERR_NONE) //reg查找键 objects
        printf_errinfo ("Can't find HKEY %ls\n", guid);
    if (reg_find_key (hive, entry, BCD_REG_HKEY, &elements)         //reg查找键 entry
        != REG_ERR_NONE)
        printf_errinfo ("Can't find HKEY %ls\n", BCD_REG_HKEY);
    if (reg_find_key (hive, elements, keyname, &key)                //reg查找键 elements
        != REG_ERR_NONE)
        printf_errinfo ("Can't find HKEY %ls\n", keyname);
    if (reg_query_value (hive, key, BCD_REG_HVAL,
        (void **)&data, len, &type) != REG_ERR_NONE)                //reg查询值 key
        printf_errinfo ("Can't find HVAL %ls\n", BCD_REG_HVAL);
    return data;
}

static void
bcd_delete_key (hive_t *hive, HKEY objects,
                const wchar_t *guid, const wchar_t *keyname)  //删除键
{
    HKEY entry, elements, key;
    if (reg_find_key (hive, objects, guid, &entry) != REG_ERR_NONE) //reg查找键 objects
        printf_errinfo ("Can't find HKEY %ls\n", guid);
    if (reg_find_key (hive, entry, BCD_REG_HKEY, &elements)         //reg查找键 entry
        != REG_ERR_NONE)
        printf_errinfo ("Can't find HKEY %ls\n", BCD_REG_HKEY);
    if (reg_find_key (hive, elements, keyname, &key)
        != REG_ERR_NONE)                                            //reg查找键 elements
        printf_errinfo ("Can't find HKEY %ls\n", keyname);
    if (reg_delete_key (hive, elements, key)                        //reg删除键 elements
        != REG_ERR_NONE)
        printf_errinfo ("Can't delete HKEY %ls\n", keyname);
}

static void
bcd_patch_bool (hive_t *hive, HKEY objects,
                const wchar_t *guid, const wchar_t *keyname,
                uint8_t val)  //修补字节
{
    uint8_t *data;
    uint32_t len;
    data = bcd_find_hive (hive, objects, guid, keyname, &len);      //查找蜂巢
    if (len != sizeof (uint8_t))
        printf_errinfo ("Invalid bool size %x\n", len);
    memmove (data, (const void *)&val, sizeof (uint8_t));           //修补1字节
    wchar_to_char (guid, keyname, 0);
    printf_debug ("...patched_bool %s->%s (%c)\n", tmp0, tmp1, val ? 'y' : 'n');
}

static void
bcd_patch_u64 (hive_t *hive, HKEY objects,
               const wchar_t *guid, const wchar_t *keyname,
               uint64_t val)  //修补8字节
{
    uint8_t *data;
    uint32_t len;
    data = bcd_find_hive (hive, objects, guid, keyname, &len);      //查找蜂巢
    if (len != sizeof (uint64_t))
        printf_errinfo ("Invalid u64 size %x\n", len);
    memmove (data, (const void *)&val, sizeof (uint64_t));          //修补8字节
    wchar_to_char (guid, keyname, 0);
    printf_debug ("...patched_u64 %s->%s (%x)\n", tmp0, tmp1, val);
}

static inline int
is_utf8_trailing_octet (uint8_t c)  //是utf8尾随八位字节
{
    /* 10 00 00 00 - 10 11 11 11 */
    return (c >= 0x80 && c <= 0xBF);
}

/* Convert UTF-8 to UCS-2.  */
uint16_t *
utf8_to_ucs2 (uint16_t *dest, grub_size_t destsize,
              const uint8_t *src)   //utf8 -> utf16
{
    grub_size_t i;
    for (i = 0; src[0] && i < destsize; i++)
    {
        if (src[0] <= 0x7F)
        {
            *dest++ = 0x007F & src[0];
            src += 1;
        }
        else if (src[0] <= 0xDF
            && is_utf8_trailing_octet (src[1]))
        {
            *dest++ = ((0x001F & src[0]) << 6)
                | (0x003F & src[1]);
            src += 2;
        }
        else if (src[0] <= 0xEF
            && is_utf8_trailing_octet (src[1])
            && is_utf8_trailing_octet (src[2]))
        {
            *dest++ = ((0x000F & src[0]) << 12)
                | ((0x003F & src[1]) << 6)
                | (0x003F & src[2]);
                src += 3;
        }
        else
        {
            *dest++ = 0;
            break;
        }
    }
    return dest;
}

/**
 * Get length of wide-character string  获取宽字符串的长度
 *
 * @v str		String
 * @ret len		Length (in characters)
 */
static grub_size_t
wcslen (const wchar_t *str)
{
    grub_size_t len = 0;

    while (*(str++))
        len++;
    return len;
}

static void
bcd_patch_sz (hive_t *hive, HKEY objects,
              const wchar_t *guid, const wchar_t *keyname,
              const char *str)  //修补utf8字(转ucs2)
{
    uint16_t *data;
    uint32_t len;
    data = bcd_find_hive (hive, objects, guid, keyname, &len);      //查找蜂巢
    memset (data, 0, len);                                          //清零
    utf8_to_ucs2 (data, len / sizeof (wchar_t), (uint8_t *) str);   //utf8 -> utf16
    wchar_to_char (guid, keyname, data);
    printf_debug ("...patched_sz %s->%s (%s)\n", tmp0, tmp1, tmp2);
}

static void
bcd_patch_szw (hive_t *hive, HKEY objects,
               const wchar_t *guid, const wchar_t *keyname,
               const wchar_t *str)  //修补宽字(蜂巢,对象,guid,键名称,源)
{
    uint16_t *data;
    uint32_t len;
    grub_size_t wlen = wcslen (str) * sizeof (wchar_t);             //获取宽字符串的长度
    data = bcd_find_hive (hive, objects, guid, keyname, &len);      //查找蜂巢
    if (wlen > len)
        printf_errinfo ("Invalid wchar size %zu\n", wlen);
    memset (data, 0, len);                                          //清零
    memmove (data, str, wlen);                                      //修补宽字
    wchar_to_char (guid, keyname, str);
    printf_debug ("...patched_szw %s->%s (%s)\n", tmp0, tmp1, tmp2);
}

static void
bcd_patch_dp (hive_t *hive, HKEY objects, uint32_t boottype,
              const wchar_t *guid, const wchar_t *keyname)  //修补路径
{
    uint8_t *data;
    uint32_t len, ofs;
    uint8_t sdi[] = GUID_BIN_RAMDISK;
    data = bcd_find_hive (hive, objects, guid, keyname, &len);      //查找蜂巢
    memset (data, 0, len);                                          //清零
    switch (boottype)   //引导类型
    {
        case NTBOOT_WIM:
        case NTBOOT_RAM:
        {
            if (len < 0x028a)
                printf_errinfo ("WIM device path (%ls->%ls) length error (%x)\n", //WIM设备路径（%ls->%ls）长度错误（%x）
                     guid, keyname, len);
            memmove (data + 0x0000, sdi, sizeof (sdi));
            data[0x0014] = 0x01;
            data[0x0018] = 0x7a;
            data[0x0019] = 0x02; // len - 0x10
            data[0x0020] = 0x03;
            data[0x0038] = 0x01;
            data[0x003c] = 0x52;
            data[0x003d] = 0x02; // len - 0x38
            data[0x0040] = 0x05;
            ofs = 0x0044;
            break;
        }
        case NTBOOT_VHD:
        {
            if (len < 0x02bc)
                printf_errinfo ("VHD device path (%ls->%ls) length error (%x)\n",
                     guid, keyname, len);
            data[0x0010] = 0x08;
            data[0x0018] = 0xac; data[0x0019] = 0x02; // len - 0x10
            data[0x0024] = 0x02;
            data[0x0027] = 0x12;
            data[0x0028] = 0x1e;
            data[0x0036] = 0x8e; data[0x0037] = 0x02; // len - 0x2e
            data[0x003e] = 0x06;
            data[0x005e] = 0x66; data[0x005f] = 0x02; // len - 0x56
            data[0x0066] = 0x05;
            data[0x006a] = 0x01;
            data[0x006e] = 0x52; data[0x006f] = 0x02; // len - 0x6a
            data[0x0072] = 0x05;
            ofs = 0x0076;
            break;
        }
        case NTBOOT_WOS:
        case NTBOOT_REC:
        {
            if (len < 0x0058)
                printf_errinfo ("OS device path (%ls->%ls) length error (%x)\n",
                     guid, keyname, len);
            ofs = 0x0010;
            break;
        }
        default:
            printf_errinfo ("Unsupported boot type %x\n", boottype);  //不支持的启动类型%x
    }

    /* os device */
    if (*(unsigned long long *)args->partid)    //如果输入partid, 本地启动wim/vhd
    {
      data[ofs + 0x00] = 0x06;                //06=disk  需要物理磁盘参数
      data[ofs + 0x08] = 0x48;
      memmove (data + ofs + 0x10, args->partid, 16);  //分区起始字节(mbr)/分区uuid(gpt)
      data[ofs + 0x24] = args->partmap;
      memmove (data + ofs + 0x28, args->diskid, 16);  //磁盘id(mbr)/磁盘uuid(gpt)
    }
    else                                        //如果没有输入partid，网起wim 
      data[ofs + 0x00] = 0x05;                //05=root   类似"find set-root"，自动搜索文件

    if (boottype == NTBOOT_WIM ||
        boottype == NTBOOT_VHD ||
        boottype == NTBOOT_RAM)
        utf8_to_ucs2 ((uint16_t *)(data + ofs + 0x48), MAX_PATH,
                      (uint8_t *)args->filepath);   //修改文件名
    printf_debug ("filepath=%s\n", args->filepath);//
    wchar_to_char (guid, keyname, 0);
    printf_debug ("...patched_dp %s->%s (boottype=%x)\n", tmp0, tmp1, boottype);  //已修补%s->%s（引导类型=%x）
}

static void
bcd_patch_data (void)  //修补数据
{
    const wchar_t *entry_guid;
    HKEY root, objects;
    hive_t hive =
    {
        .size = args->bcd_length,
        .data = args->bcd,
    };
    /* 打开BCD配置单元 */
    if (reg_open_hive (&hive) != REG_ERR_NONE)                  //reg打开蜂箱
        printf_errinfo ("BCD hive load error.\n");
    reg_find_root (&hive, &root);                               //reg查找根
    if (reg_find_key (&hive, root, BCD_REG_ROOT, &objects)
        != REG_ERR_NONE)                                        //reg查找键
        printf_errinfo ("Can't find HKEY %ls\n", BCD_REG_ROOT);
    printf_debug ("BCD hive load OK.\n");

    /* 检查引导类型 */
    switch (args->boottype)
    {
        case NTBOOT_VHD:
            entry_guid = GUID_VHDB;
            break;
        case NTBOOT_WOS:
            entry_guid = GUID_WOSB;
            break;
        case NTBOOT_WIM:
        case NTBOOT_RAM:
        default:
            entry_guid = GUID_WIMB;
    }

    /* 修补对象->{BootMgr} (T6系统启动管理器) */
    bcd_patch_szw (&hive, objects, GUID_BOOTMGR,
                   BCDOPT_OBJECT, entry_guid);            //修补宽字  默认启动选项=当前引导类型
    bcd_patch_szw (&hive, objects, GUID_BOOTMGR,
                   BCDOPT_ORDER, entry_guid);             //修补宽字  菜单显示顺序=当前引导类型
    bcd_patch_u64 (&hive, objects, GUID_BOOTMGR,
                   BCDOPT_TIMEOUT, args->timeout);        //修补8字节 超时=时间

    /* 修补对象->{Resume} (恢复) */
    bcd_patch_dp (&hive, objects, NTBOOT_REC,
                  GUID_HIBR, BCDOPT_APPDEV);              //修补路径  恢复=系统启动程序设备
    bcd_patch_dp (&hive, objects, NTBOOT_REC,
                  GUID_HIBR, BCDOPT_OSDDEV);              //修补路径  恢复=操作系统设备
    bcd_patch_sz (&hive, objects, GUID_HIBR,
                  BCDOPT_WINLOAD, BCD_DEFAULT_WINRESUME); //修补utf8字(转ucs2)  启动程序路径=默认WIN简历
    bcd_patch_sz (&hive, objects, GUID_HIBR,
                  BCDOPT_SYSROOT, BCD_DEFAULT_HIBERFIL);  //修补utf8字(转ucs2)  系统文件路径=冬眠
    /* 修补对象 */
    if (args->boottype == NTBOOT_RAM) //如果是 {Ramdisk} (内存盘)
    {
        bcd_delete_key (&hive, objects, GUID_RAMDISK, BCDOPT_SDIDEV);   //删除键  //如果是 {Ramdisk} (内存盘)sdi ramdisk设备
        bcd_delete_key (&hive, objects, GUID_RAMDISK, BCDOPT_SDIPATH);  //删除键  RAM磁盘,sdi ramdisk路径
        bcd_patch_bool (&hive, objects, GUID_RAMDISK,
                        BCDOPT_EXPORTCD, args->exportcd);         //修正字节  RAM磁盘,ramdisk导出为cd=值
        bcd_patch_u64 (&hive, objects, GUID_RAMDISK,
                       BCDOPT_IMGOFS, args->imgofs);              //修补8字节 RAM磁盘,ramdisk映像偏移=值
    }
    else                              //如果是非内存盘
    {
        bcd_delete_key (&hive, objects, GUID_RAMDISK, BCDOPT_EXPORTCD); //删除键   RAM磁盘,ramdisk导出为cd
        bcd_delete_key (&hive, objects, GUID_RAMDISK, BCDOPT_IMGOFS);   //删除键   RAM磁盘,ramdisk映像偏移
    }

    /* 修补对象->{Options}(选择) */
    bcd_patch_sz (&hive, objects, GUID_OPTN,
                  BCDOPT_CMDLINE, args->loadopt);     //修补utf8字(转ucs2)  加载选项字符串=禁用完整性检查
    bcd_patch_bool (&hive, objects, GUID_OPTN,
                    BCDOPT_TESTMODE, args->testmode); //修正字节            测试签名=0
    bcd_patch_bool (&hive, objects, GUID_OPTN,
                    BCDOPT_DETHAL, args->hal);        //修正字节            检测hal,表明系统加载程序=1
    bcd_patch_bool (&hive, objects, GUID_OPTN,
                    BCDOPT_WINPE, args->minint);      //修正字节            迷你pe模式=0xff 
    bcd_patch_bool (&hive, objects, GUID_OPTN,
                    BCDOPT_NOVGA, args->novga);       //修正字节            不是VGA=0
    bcd_patch_bool (&hive, objects, GUID_OPTN,
                    BCDOPT_NOVESA, args->novesa);     //修正字节            不是VESA=0
    bcd_patch_bool (&hive, objects, GUID_OPTN,
                    BCDOPT_ADVOPT, args->advmenu);    //修正字节            高级选项（f8）
    bcd_patch_bool (&hive, objects, GUID_OPTN,
                    BCDOPT_OPTEDIT, args->optedit);   //修正字节            编辑模式
    bcd_patch_bool (&hive, objects, GUID_OPTN,
                    BCDOPT_TEXT, args->textmode);     //修正字节            文本模式
    bcd_patch_u64 (&hive, objects, GUID_OPTN,
                   BCDOPT_NX, args->nx);              //修补8字节           nx=入口
    bcd_patch_u64 (&hive, objects, GUID_OPTN,
                   BCDOPT_PAE, args->pae);            //修补8字节           pae=默认

    /* 修补对象->[Resolution}(分辨率) */
    if (args->hires == NTARG_BOOL_NA)     //如果是无设置
    {
        bcd_delete_key (&hive, objects, GUID_OPTN, BCDOPT_HIGHRES); //删除键  选择，最高分辨率
        bcd_patch_u64 (&hive, objects, GUID_OPTN,
                       BCDOPT_GFXMODE, args->gfxmode);              //修补8字节 图形分辨率=指定分辨率
    }
    else                                  //如果是未设置
    {
        bcd_delete_key (&hive, objects, GUID_OPTN, BCDOPT_GFXMODE); //删除键  选择，图形分辨率
        bcd_patch_bool (&hive, objects, GUID_OPTN,
                        BCDOPT_HIGHRES, args->hires);               //修正字节  最高分辨率=设置值
    }

    if (args->safemode)                   //安全模式
    {
        bcd_patch_u64 (&hive, objects, GUID_OPTN,
                       BCDOPT_SAFEMODE, args->safeboot);            //修补8字节   安全引导=最小的L
        bcd_patch_bool (&hive, objects, GUID_OPTN,
                        BCDOPT_ALTSHELL, args->altshell);           //修正字节    安全模式备用外壳=0
    }
    else                                  //非安全模式
    {
        bcd_delete_key (&hive, objects, GUID_OPTN, BCDOPT_SAFEMODE); //删除键    安全引导
        bcd_delete_key (&hive, objects, GUID_OPTN, BCDOPT_ALTSHELL); //删除键    安全模式备用外壳
    }

    /* 修补对象->{Entry}(条目) */
    bcd_patch_dp (&hive, objects, args->boottype,
                  entry_guid, BCDOPT_APPDEV);                       //修补路径  引导类型，guid=系统启动程序设备
    bcd_patch_dp (&hive, objects, args->boottype,
                  entry_guid, BCDOPT_OSDDEV);                       //修补路径  引导类型，guid=操作系统设备
    bcd_patch_sz (&hive, objects, entry_guid,
                  BCDOPT_WINLOAD, args->winload);                   //修补utf8字(转ucs2)  guid，win装载=设置值
    bcd_patch_sz (&hive, objects, entry_guid,
                  BCDOPT_SYSROOT, args->sysroot);                   //修补utf8字(转ucs2)  guid，系统根=设置值
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//reg.c
#define offsetof(__type, __member) ((grub_size_t) &((__type *) NULL)->__member)

enum reg_bool
{
    false = 0,
    true  = 1,
};

static int32_t
get_int32_size (uint8_t *data, uint64_t offset)   //获取int32尺寸
{
    int32_t ret;
    memmove (&ret, data + offset, sizeof (int32_t));
    return -ret;
}

static enum reg_bool check_header(hive_t *h)  //校验头
{
    HBASE_BLOCK *base_block = (HBASE_BLOCK *)h->data;
    uint32_t csum;
    enum reg_bool dirty = false;

    if (base_block->Signature != HV_HBLOCK_SIGNATURE)
    {
        printf_debug("Invalid signature.\n");
        return false;
    }

    if (base_block->Major != HSYS_MAJOR)
    {
        printf_debug("Invalid major value.\n");
        return false;
    }

    if (base_block->Minor < HSYS_MINOR)
    {
        printf_debug("Invalid minor value.\n");
        return false;
    }

    if (base_block->Type != HFILE_TYPE_PRIMARY)
    {
        printf_debug("Type was not HFILE_TYPE_PRIMARY.\n");
        return false;
    }

    if (base_block->Format != HBASE_FORMAT_MEMORY)
    {
        printf_debug("Format was not HBASE_FORMAT_MEMORY.\n");
        return false;
    }

    if (base_block->Cluster != 1)
    {
        printf_debug("Cluster was not 1.\n");
        return false;
    }

    if (base_block->Sequence1 != base_block->Sequence2)
    {
        printf_debug("Sequence1 != Sequence2.\n");
        base_block->Sequence2 = base_block->Sequence1;
        dirty = true;
    }

    //检查校验和

    csum = 0;

    for (unsigned int i = 0; i < 127; i++)
        csum ^= ((uint32_t *)h->data)[i];

    if (csum == 0xffffffff)
        csum = 0xfffffffe;
    else if (csum == 0)
        csum = 1;

    if (csum != base_block->CheckSum)
    {
        printf_debug("Invalid checksum.\n");
        base_block->CheckSum = csum;
        dirty = true;
    }

    if (dirty)
    {
        printf_debug("Hive is dirty.\n");
    }

    return true;
}

static
void reg_find_root(hive_t *h, HKEY *Key)  //reg查找根
{
    HBASE_BLOCK *base_block = (HBASE_BLOCK *)h->data;
    *Key = 0x1000 + base_block->RootCell;
}

static reg_err_t
reg_enum_keys(hive_t *h, HKEY Key, uint32_t Index,
              wchar_t *Name, uint32_t NameLength)   //reg枚举键
{
    int32_t size;
    CM_KEY_NODE *nk;
    CM_KEY_FAST_INDEX *lh;
    enum reg_bool overflow = false;

    // FIXME - 确保没有缓冲区溢出（在这里和其他地方）
    // 查找父密钥节点

    size = get_int32_size (h->data, Key);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]))
        return REG_ERR_BAD_ARGUMENT;

    nk = (CM_KEY_NODE *)((uint8_t *)h->data + Key + sizeof(int32_t));

    if (nk->Signature != CM_KEY_NODE_SIGNATURE)
        return REG_ERR_BAD_ARGUMENT;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]) + nk->NameLength)
        return REG_ERR_BAD_ARGUMENT;

    // FIXME - 易失性密钥？

    if (Index >= nk->SubKeyCount || nk->SubKeyList == 0xffffffff)
        return REG_ERR_FILE_NOT_FOUND;

    // 转到键索引

    size = get_int32_size (h->data, 0x1000 + nk->SubKeyList);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_FAST_INDEX, List[0]))
        return REG_ERR_BAD_ARGUMENT;

    lh = (CM_KEY_FAST_INDEX *)((uint8_t *)h->data + 0x1000
                               + nk->SubKeyList + sizeof(int32_t));

    if (lh->Signature == CM_KEY_INDEX_ROOT)
    {
        CM_KEY_INDEX *ri = (CM_KEY_INDEX *)lh;

        for (grub_size_t i = 0; i < ri->Count; i++)
        {
            CM_KEY_FAST_INDEX *lh2 = (CM_KEY_FAST_INDEX *)((uint8_t *)h->data + 0x1000 + ri->List[i] +
                                             sizeof(int32_t));

            if (lh2->Signature == CM_KEY_INDEX_ROOT)
            {
                // 不要重复出现：CVE-2021-3622
                printf_debug("Reading nested CM_KEY_INDEX is not yet implemented\n");
                return REG_ERR_BAD_ARGUMENT;
            }
            else if (lh2->Signature != CM_KEY_HASH_LEAF
                     && lh2->Signature != CM_KEY_FAST_LEAF)
                return REG_ERR_BAD_ARGUMENT;

            if (lh2->Count > Index)
            {
                lh = lh2;
                break;
            }

            Index -= lh2->Count;
        }
    }
    else if (lh->Signature != CM_KEY_HASH_LEAF && lh->Signature != CM_KEY_FAST_LEAF)
        return REG_ERR_BAD_ARGUMENT;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_FAST_INDEX,
            List[0]) + (lh->Count * sizeof(CM_INDEX)))
        return REG_ERR_BAD_ARGUMENT;

    if (Index >= lh->Count)
        return REG_ERR_BAD_ARGUMENT;

    // 查找子密钥节点

    size = get_int32_size (h->data, 0x1000 + lh->List[Index].Cell);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]))
        return REG_ERR_BAD_ARGUMENT;

    CM_KEY_NODE *nk2 = (CM_KEY_NODE *)((uint8_t *)h->data + 0x1000
                        + lh->List[Index].Cell + sizeof(int32_t));

    if (nk2->Signature != CM_KEY_NODE_SIGNATURE)
        return REG_ERR_BAD_ARGUMENT;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]) + nk2->NameLength)
        return REG_ERR_BAD_ARGUMENT;

    if (nk2->Flags & KEY_COMP_NAME)
    {
        unsigned int i = 0;
        char *nkname = (char *)nk2->Name;

        for (i = 0; i < nk2->NameLength; i++)
        {
            if (i >= NameLength)
            {
                overflow = true;
                break;
            }

            Name[i] = nkname[i];
        }

        Name[i] = 0;
    }
    else
    {
        unsigned int i = 0;

        for (i = 0; i < nk2->NameLength / sizeof(wchar_t); i++)
        {
            if (i >= NameLength)
            {
                overflow = true;
                break;
            }

            Name[i] = nk2->Name[i];
        }

        Name[i] = 0;
    }

    return overflow ? REG_ERR_OUT_OF_MEMORY : REG_ERR_NONE;
}

static reg_err_t
find_child_key(hive_t *h, HKEY parent,
               const wchar_t *namebit, grub_size_t nblen, HKEY *key)   //查找子键
{
    int32_t size;
    CM_KEY_NODE *nk;
    CM_KEY_FAST_INDEX *lh;

    // 查找父密钥节点

    size = get_int32_size (h->data, parent);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]))
        return REG_ERR_BAD_ARGUMENT;

    nk = (CM_KEY_NODE *)((uint8_t *)h->data + parent + sizeof(int32_t));

    if (nk->Signature != CM_KEY_NODE_SIGNATURE)
        return REG_ERR_BAD_ARGUMENT;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]) + nk->NameLength)
        return REG_ERR_BAD_ARGUMENT;

    if (nk->SubKeyCount == 0 || nk->SubKeyList == 0xffffffff)
        return REG_ERR_FILE_NOT_FOUND;

    // 转到键索引

    size = get_int32_size (h->data, 0x1000 + nk->SubKeyList);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_FAST_INDEX, List[0]))
        return REG_ERR_BAD_ARGUMENT;

    lh = (CM_KEY_FAST_INDEX *)((uint8_t *)h->data + 0x1000
                                + nk->SubKeyList + sizeof(int32_t));

    if (lh->Signature != CM_KEY_HASH_LEAF && lh->Signature != CM_KEY_FAST_LEAF)
        return REG_ERR_BAD_ARGUMENT;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_FAST_INDEX,
            List[0]) + (lh->Count * sizeof(CM_INDEX)))
        return REG_ERR_BAD_ARGUMENT;

    // FIXME - 如果CM_KEY_HASH_LEAF，则检查哈希值

    for (unsigned int i = 0; i < lh->Count; i++)
    {
        CM_KEY_NODE *nk2;

        size = get_int32_size (h->data, 0x1000 + lh->List[i].Cell);

        if (size < 0)
            continue;

        if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]))
            continue;

        nk2 = (CM_KEY_NODE *)((uint8_t *)h->data + 0x1000
                              + lh->List[i].Cell + sizeof(int32_t));

        if (nk2->Signature != CM_KEY_NODE_SIGNATURE)
            continue;

        if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]) + nk2->NameLength)
            continue;

        // FIXME - 在这里使用字符串协议正确地进行比较？

        if (nk2->Flags & KEY_COMP_NAME)
        {
            unsigned int j;
            char *name = (char *)nk2->Name;

            if (nk2->NameLength != nblen)
                continue;

            for (j = 0; j < nk2->NameLength; j++)
            {
                wchar_t c1 = name[j];
                wchar_t c2 = namebit[j];

                if (c1 >= 'A' && c1 <= 'Z')
                    c1 = c1 - 'A' + 'a';

                if (c2 >= 'A' && c2 <= 'Z')
                    c2 = c2 - 'A' + 'a';

                if (c1 != c2)
                    break;
            }

            if (j != nk2->NameLength)
                continue;

            *key = 0x1000 + lh->List[i].Cell;

            return REG_ERR_NONE;
        }
        else
        {
            unsigned int j;

            if (nk2->NameLength / sizeof(wchar_t) != nblen)
                continue;

            for (j = 0; j < nk2->NameLength / sizeof(wchar_t); j++)
            {
                wchar_t c1 = nk2->Name[j];
                wchar_t c2 = namebit[j];

                if (c1 >= 'A' && c1 <= 'Z')
                    c1 = c1 - 'A' + 'a';

                if (c2 >= 'A' && c2 <= 'Z')
                    c2 = c2 - 'A' + 'a';

                if (c1 != c2)
                    break;
            }

            if (j != nk2->NameLength / sizeof(wchar_t))
                continue;

            *key = 0x1000 + lh->List[i].Cell;

            return REG_ERR_NONE;
        }
    }

    return REG_ERR_FILE_NOT_FOUND;
}

static reg_err_t
reg_find_key(hive_t *h, HKEY Parent, const wchar_t *Path, HKEY *Key)  //reg查找键
{
    reg_err_t Status;
    grub_size_t nblen;
    HKEY k;

    do
    {
        nblen = 0;
        while (Path[nblen] != '\\' && Path[nblen] != 0)
            nblen++;

        Status = find_child_key(h, Parent, Path, nblen, &k);  //查找子键
        if (Status)
            return Status;

        if (Path[nblen] == 0 || (Path[nblen] == '\\' && Path[nblen + 1] == 0))
        {
            *Key = k;
            return Status;
        }

        Parent = k;
        Path = &Path[nblen + 1];
    } while (true);
}

static reg_err_t
reg_delete_key(hive_t *h, HKEY parent, HKEY key)  //reg删除键
{
    int32_t size;
    CM_KEY_NODE *nk;
    CM_KEY_FAST_INDEX *lh;

    // 查找父密钥节点

    size = get_int32_size (h->data, parent);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]))
        return REG_ERR_BAD_ARGUMENT;

    nk = (CM_KEY_NODE *)((uint8_t *)h->data + parent + sizeof(int32_t));

    if (nk->Signature != CM_KEY_NODE_SIGNATURE)
        return REG_ERR_BAD_ARGUMENT;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]) + nk->NameLength)
        return REG_ERR_BAD_ARGUMENT;

    if (nk->SubKeyCount == 0 || nk->SubKeyList == 0xffffffff)
        return REG_ERR_FILE_NOT_FOUND;

    // 转到键索引

    size = get_int32_size (h->data, 0x1000 + nk->SubKeyList);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_FAST_INDEX, List[0]))
        return REG_ERR_BAD_ARGUMENT;

    lh = (CM_KEY_FAST_INDEX *)((uint8_t *)h->data + 0x1000
    + nk->SubKeyList + sizeof(int32_t));

    if (lh->Signature != CM_KEY_HASH_LEAF && lh->Signature != CM_KEY_FAST_LEAF)
        return REG_ERR_BAD_ARGUMENT;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_FAST_INDEX,
        List[0]) + (lh->Count * sizeof(CM_INDEX)))
        return REG_ERR_BAD_ARGUMENT;

    for (unsigned int i = 0; i < lh->Count; i++)
    {
        // key = 0x1000 + lh->List[i].Cell;
        if (key != 0x1000 + lh->List[i].Cell)
            continue;
        lh->Count--;
        for (unsigned j = i; j < lh->Count; j++)
            memmove (&lh->List[j].Cell, &lh->List[j + 1].Cell,
                    sizeof (CM_INDEX));
        nk->SubKeyCount--;
        return REG_ERR_NONE;
    }

    return REG_ERR_FILE_NOT_FOUND;
}

static reg_err_t
reg_enum_values(hive_t *h, HKEY Key,
                uint32_t Index, wchar_t *Name,
                uint32_t NameLength, uint32_t *Type)  //reg枚举值
{
    int32_t size;
    CM_KEY_NODE *nk;
    uint32_t *list;
    CM_KEY_VALUE *vk;
    enum reg_bool overflow = false;

    // 查找键节点

    size = get_int32_size (h->data, Key);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]))
        return REG_ERR_BAD_ARGUMENT;

    nk = (CM_KEY_NODE *)((uint8_t *)h->data + Key + sizeof(int32_t));

    if (nk->Signature != CM_KEY_NODE_SIGNATURE)
        return REG_ERR_BAD_ARGUMENT;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]) + nk->NameLength)
        return REG_ERR_BAD_ARGUMENT;

    if (Index >= nk->ValuesCount || nk->Values == 0xffffffff)
        return REG_ERR_FILE_NOT_FOUND;

    // 转到键索引

    size = get_int32_size (h->data, 0x1000 + nk->Values);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + (sizeof(uint32_t) * nk->ValuesCount))
        return REG_ERR_BAD_ARGUMENT;

    list = (uint32_t *)((uint8_t *)h->data + 0x1000 + nk->Values + sizeof(int32_t));

    // 查找值节点

    size = get_int32_size (h->data, 0x1000 + list[Index]);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_VALUE, Name[0]))
        return REG_ERR_BAD_ARGUMENT;

    vk = (CM_KEY_VALUE *)((uint8_t *)h->data + 0x1000
                          + list[Index] + sizeof(int32_t));

    if (vk->Signature != CM_KEY_VALUE_SIGNATURE)
        return REG_ERR_BAD_ARGUMENT;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_VALUE,
            Name[0]) + vk->NameLength)
        return REG_ERR_BAD_ARGUMENT;

    if (vk->Flags & VALUE_COMP_NAME)
    {
        unsigned int i = 0;
        char *nkname = (char *)vk->Name;

        for (i = 0; i < vk->NameLength; i++)
        {
            if (i >= NameLength)
            {
                overflow = true;
                break;
            }

            Name[i] = nkname[i];
        }

        Name[i] = 0;
    }
    else
    {
        unsigned int i = 0;

        for (i = 0; i < vk->NameLength / sizeof(wchar_t); i++)
        {
            if (i >= NameLength)
            {
                overflow = true;
                break;
            }

            Name[i] = vk->Name[i];
        }

        Name[i] = 0;
    }

    *Type = vk->Type;

    return overflow ? REG_ERR_OUT_OF_MEMORY : REG_ERR_NONE;
}

static reg_err_t
reg_query_value(hive_t *h, HKEY Key,
                const wchar_t *Name, void **Data,
                uint32_t *DataLength, uint32_t *Type)   //reg查询值
{
    int32_t size;
    CM_KEY_NODE *nk;
    uint32_t *list;
    unsigned int namelen = wcslen(Name);

    // 查找键节点

    size = get_int32_size (h->data, Key);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]))
        return REG_ERR_BAD_ARGUMENT;

    nk = (CM_KEY_NODE *)((uint8_t *)h->data + Key + sizeof(int32_t));

    if (nk->Signature != CM_KEY_NODE_SIGNATURE)
        return REG_ERR_BAD_ARGUMENT;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]) + nk->NameLength)
        return REG_ERR_BAD_ARGUMENT;

    if (nk->ValuesCount == 0 || nk->Values == 0xffffffff)
        return REG_ERR_FILE_NOT_FOUND;

    // 转到键索引

    size = get_int32_size (h->data, 0x1000 + nk->Values);

    if (size < 0)
        return REG_ERR_FILE_NOT_FOUND;

    if ((uint32_t)size < sizeof(int32_t) + (sizeof(uint32_t) * nk->ValuesCount))
        return REG_ERR_BAD_ARGUMENT;

    list = (uint32_t *)((uint8_t *)h->data + 0x1000 + nk->Values + sizeof(int32_t));

    // 查找值节点

    for (unsigned int i = 0; i < nk->ValuesCount; i++)
    {
        CM_KEY_VALUE *vk;

        size = get_int32_size (h->data, 0x1000 + list[i]);

        if (size < 0)
            continue;

        if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_VALUE, Name[0]))
            continue;

        vk = (CM_KEY_VALUE *)((uint8_t *)h->data + 0x1000 + list[i] + sizeof(int32_t));

        if (vk->Signature != CM_KEY_VALUE_SIGNATURE)
            continue;

        if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_VALUE,
                Name[0]) + vk->NameLength)
            continue;

        if (vk->Flags & VALUE_COMP_NAME)
        {
            unsigned int j;
            char *valname = (char *)vk->Name;

            if (vk->NameLength != namelen)
                continue;

            for (j = 0; j < vk->NameLength; j++)
            {
                wchar_t c1 = valname[j];
                wchar_t c2 = Name[j];

                if (c1 >= 'A' && c1 <= 'Z')
                    c1 = c1 - 'A' + 'a';

                if (c2 >= 'A' && c2 <= 'Z')
                    c2 = c2 - 'A' + 'a';

                if (c1 != c2)
                    break;
            }

            if (j != vk->NameLength)
                continue;
        }
        else
        {
            unsigned int j;

            if (vk->NameLength / sizeof(wchar_t) != namelen)
                continue;

            for (j = 0; j < vk->NameLength / sizeof(wchar_t); j++)
            {
                wchar_t c1 = vk->Name[j];
                wchar_t c2 = Name[j];

                if (c1 >= 'A' && c1 <= 'Z')
                    c1 = c1 - 'A' + 'a';

                if (c2 >= 'A' && c2 <= 'Z')
                    c2 = c2 - 'A' + 'a';

                if (c1 != c2)
                    break;
            }

            if (j != vk->NameLength / sizeof(wchar_t))
                continue;
        }

        if (vk->DataLength & CM_KEY_VALUE_SPECIAL_SIZE)   // data stored as data offset
        {
            grub_size_t datalen = vk->DataLength & ~CM_KEY_VALUE_SPECIAL_SIZE;
            uint8_t *ptr;
#if 0
            if (datalen == 4)
                ptr = (uint8_t *)&vk->Data;
            else if (datalen == 2)
                ptr = (uint8_t *)&vk->Data + 2;
            else if (datalen == 1)
                ptr = (uint8_t *)&vk->Data + 3;
#else
            if (datalen == 4 || datalen == 2 || datalen == 1)
                ptr = (uint8_t *)&vk->Data;
#endif
            else if (datalen == 0)
                ptr = NULL;
            else
                return REG_ERR_BAD_ARGUMENT;

            *Data = ptr;
        }
        else
        {
            size = get_int32_size (h->data, 0x1000 + vk->Data);

            if ((uint32_t)size < vk->DataLength)
                return REG_ERR_BAD_ARGUMENT;

            *Data = (uint8_t *)h->data + 0x1000 + vk->Data + sizeof(int32_t);
        }

        // FIXME - 处理长“数据块”值

        *DataLength = vk->DataLength & ~CM_KEY_VALUE_SPECIAL_SIZE;
        *Type = vk->Type;

        return REG_ERR_NONE;
    }

    return REG_ERR_FILE_NOT_FOUND;
}

static void clear_volatile(hive_t *h, HKEY key)   //清除不稳定
{
    int32_t size;
    CM_KEY_NODE *nk;
    uint16_t sig;

    size = get_int32_size (h->data, key);

    if (size < 0)
        return;

    if ((uint32_t)size < sizeof(int32_t) + offsetof(CM_KEY_NODE, Name[0]))
        return;

    nk = (CM_KEY_NODE *)((uint8_t *)h->data + key + sizeof(int32_t));

    if (nk->Signature != CM_KEY_NODE_SIGNATURE)
        return;

    nk->VolatileSubKeyList = 0xbaadf00d;
    nk->VolatileSubKeyCount = 0;

    if (nk->SubKeyCount == 0 || nk->SubKeyList == 0xffffffff)
        return;

    size = get_int32_size (h->data, 0x1000 + nk->SubKeyList);
    memmove (&sig, (uint8_t*)h->data + 0x1000 + nk->SubKeyList + sizeof(int32_t), sizeof(uint16_t));

    if (sig == CM_KEY_HASH_LEAF || sig == CM_KEY_FAST_LEAF)
    {
        CM_KEY_FAST_INDEX *lh =
            (CM_KEY_FAST_INDEX *)((uint8_t *)h->data + 0x1000
                                  + nk->SubKeyList + sizeof(int32_t));

        for (unsigned int i = 0; i < lh->Count; i++)
            clear_volatile(h, 0x1000 + lh->List[i].Cell);
    }
    else if (sig == CM_KEY_INDEX_ROOT)
    {
        CM_KEY_INDEX *ri =
            (CM_KEY_INDEX *)((uint8_t *)h->data + 0x1000
                             + nk->SubKeyList + sizeof(int32_t));

        for (unsigned int i = 0; i < ri->Count; i++)
            clear_volatile(h, 0x1000 + ri->List[i]);
    }
    else
    {
        printf_debug("Unhandled registry signature 0x%x.\n", sig);
    }
}

static enum reg_bool validate_bins(const uint8_t *data, grub_size_t len)   //验证容器
{
    grub_size_t off = 0;

    data += 0x1000;

    while (off < len)
    {
        const HBIN *hb = (HBIN *)data;
        if (hb->Signature != HV_HBIN_SIGNATURE)
        {
            printf_debug("Invalid hbin signature in hive_t at offset %zx.\n", off);
            return false;
        }

        if (hb->FileOffset != off)
        {
            printf_debug("hbin FileOffset in hive_t was 0x%x, expected %zx.\n", hb->FileOffset,
                 off);
            return false;
        }

        if (hb->Size > len - off)
        {
            printf_debug("hbin overrun in hive_t at offset %zx.\n", off);
            return false;
        }

        if (hb->Size & 0xfff)
        {
            printf_debug("hbin Size in hive_t at offset %zx was not multiple of 1000.\n", off);
            return false;
        }

        off += hb->Size;
        data += hb->Size;
    }

    return true;
}

static reg_err_t
reg_open_hive(hive_t *h)  //reg打开蜂箱
{
    if (!check_header(h))  //校验头
    {
        printf_debug("Header check failed.\n");
        return REG_ERR_BAD_ARGUMENT;
    }

    const HBASE_BLOCK *base_block = (HBASE_BLOCK *)h->data;

    // 对hivet进行健全性检查，以避免以后进行错误检查74
    if (!validate_bins((uint8_t *)h->data, base_block->Length))
    {
        return REG_ERR_BAD_ARGUMENT;
    }

    clear_volatile(h, 0x1000 + base_block->RootCell);

    return REG_ERR_NONE;
}

