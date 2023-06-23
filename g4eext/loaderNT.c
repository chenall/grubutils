

/*
 * compile:
 * gcc -Wl,--build-id=none -m64 -mno-sse -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE loaderNT.c -o loaderNT.o 2>&1|tee build.log
 * objcopy -O binary loaderNT.o loaderNT
 *
 * analysis:
 * objdump -d loaderNT.o > loaderNT.s
 * readelf -a loaderNT.o > a.s
 */
//

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
static struct nt_args args;
struct nt_args *nt_cmdline;
char temp[256];

static void convert_path (char *str, int backslash);
static void bcd_print_hex (const void *data, grub_size_t len);
static void bcd_replace_hex (const void *search, grub_uint32_t search_len,
                 const void *replace, grub_uint32_t replace_len, int count);
static inline int grub_utf8_process (uint8_t c, uint32_t *code, int *count);
static inline grub_size_t grub_utf8_to_utf16 (grub_uint16_t *dest, grub_size_t destsize,
                    const grub_uint8_t *src, grub_size_t srcsize, const grub_uint8_t **srcend);
static void bcd_patch_path (void);
static inline int islower (int c);
static inline int toupper (int c);
static inline int towupper (wint_t c);
int wcscasecmp (const wchar_t *str1, const wchar_t *str2);
static void bcd_patch_hive (reg_hive_t *hive, const wchar_t *keyname, void *val);
int strcasecmp (const char *str1, const char *str2);
static void bcd_parse_bool (reg_hive_t *hive, const wchar_t *keyname, const char *s);
unsigned long strtoul (const char *nptr, char **endptr, int base);
static void bcd_parse_u64 (reg_hive_t *hive, const wchar_t *keyname, const char *s);
static void bcd_parse_str (reg_hive_t *hive, const wchar_t *keyname,
               grub_uint8_t resume, const char *s);
static void bcd_patch_data (void);
static grub_size_t reg_wcslen (const grub_uint16_t *s);
static enum reg_bool check_header(hive_t *h);
static void close_hive (reg_hive_t *this);
static void find_root (reg_hive_t *this, HKEY* key);
static reg_err_t enum_keys (reg_hive_t *this, HKEY key, grub_uint32_t index,
           grub_uint16_t *name, grub_uint32_t name_len);
static reg_err_t find_child_key (hive_t* h, HKEY parent,
                const grub_uint16_t* namebit, grub_size_t nblen, HKEY* key);
static reg_err_t find_key (reg_hive_t* this, HKEY parent, const grub_uint16_t* path, HKEY* key);
static reg_err_t enum_values (reg_hive_t *this, HKEY key,
             grub_uint32_t index, grub_uint16_t* name, grub_uint32_t name_len, grub_uint32_t* type);
static reg_err_t query_value_no_copy (reg_hive_t *this, HKEY key,
                     const grub_uint16_t* name, void** data,
                     grub_uint32_t* data_len, grub_uint32_t* type);
static reg_err_t query_value (reg_hive_t *this, HKEY key,
             const grub_uint16_t* name, void* data,
             grub_uint32_t* data_len, grub_uint32_t* type);
static void steal_data (reg_hive_t *this, void** data, grub_uint32_t* size);
static void clear_volatile (hive_t* h, HKEY key);
reg_err_t open_hive (void *file, grub_size_t len, reg_hive_t **hive);
void process_cmdline (char *arg);
static int modify_bcd (char *filename, char *bcdname, int flags);



/* 这是必需的，请参阅grubprog.h中的注释 */
#include <grubprog.h>
/* 请勿在此处插入任何其他asm行 */
//不能包含子程序，不能包含全局变量，否则编译可以通过，但是不能正确执行。
static int main(char *arg,int flags);
static int main(char *arg,int flags)
{
  char bcdname[] = "BCDWIM";
  int i = 1, j;
  char *filename = arg;

  get_G4E_image();
  if (! g4e_data)
    return 0;

  if (*(unsigned int *)IMG(0x8278) < 20220315)
  {
    printf("Please use grub4efi version above 2022-03-15.\n");
    return 0;
  }
  memset((void *)&args, 0, sizeof(args));
  arg = skip_to(0x200,filename);
  char *suffix = &filename[strlen (filename) - 3]; //取尾缀

  //判断尾缀是否为WIM/VHD/WIN
//  if ((suffix[0] | 0x20) == 'w' && (suffix[1] | 0x20) == 'i' && (suffix[2] | 0x20) == 'm')
  if (substring(suffix,"wim",1) == 0)
  {
    args.type = BOOT_WIM;
  }
//  else if ((suffix[0] | 0x20) == 'v' && (suffix[1] | 0x20) == 'h' && (suffix[2] | 0x20) == 'd')
  else if (substring(suffix,"vhd",1) == 0)
  {
    args.type = BOOT_VHD;
    sprintf (bcdname, "BCDVHD");
  }
//  else if ((suffix[0] | 0x20) == 'w' && (suffix[1] | 0x20) == 'i' && (suffix[2] | 0x20) == 'n')
  else if (substring(suffix,"win",1) == 0)
  {
    args.type = BOOT_WIN;
    sprintf (bcdname, "BCDWIN");
  }
  else
    goto load_fail;  //是常规

  process_cmdline (arg);  //处理命令行参数
  //处理字符串中的空格
  char *filename1 = temp;
  for (i = 0, j = 0; i < strlen (filename); i++,j++)
  {
    if (filename[i] == '\\' && filename[i+1] == ' ')
    {
      filename1[j] = ' ';
      i++;
    }
    else
      filename1[j] = filename[i];
  }
  filename1[j] = 0;  
  filename = filename1;

  modify_bcd (filename, bcdname, flags); //修改bcd
  nt_cmdline = (struct nt_args *)&args;
  bcd_patch_data ();
  run_line ("chainloader /bootx64.efi",flags);
  return 1;
load_fail:
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

void process_cmdline (char *arg)  //处理命令行参数
{
  char *tmp = arg;
  char *key;
  char *value;

  /* Do nothing if we have no command line 如果没有命令行，什么也不做 */
  if ((arg == NULL) || (arg[0] == '\0'))
    return;
  /* Parse command line 分析命令行 */
  while (*tmp)
  {
    /* Skip whitespace 跳过空白*/
    while (isspace (*tmp))
      tmp++;
    /* Find value (if any) and end of this argument 查找值（如果有）并结束此参数 */
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
    /* Process this argument 处理这个参数*/
#if 0
    if (strcmp (key, "text") == 0)        //文本
      args.text_mode = 1;
    else if (strcmp (key, "quiet") == 0)  //安静
      args.quiet = 1;
    else if (strcmp (key, "linear") == 0) //线性
      args.linear = 1;
    else if (strcmp (key, "pause") == 0)  //暂停
    {
      args.pause = 1;
      if (value && (strcmp (value, "quiet") == 0))
        args.pause_quiet = 1;
    }
    else if (strcmp (key, "uuid") == 0)   //uuid
    {
      if (! value || ! value[0])
        die ("Argument \"uuid\" needs a value\n");
      strncpy (args.uuid, value, 17);
    }
    else if (strcmp (key, "file") == 0)   //文件
    {
      char ext;
      if (! value || ! value[0])
        die ("Argument \"file\" needs a value\n");
      strncpy (args.path, value, 256);
      convert_path (args.path, 1);        //转换路径
      ext = value[strlen (value) - 1];
      if (ext == 'm' || ext == 'M')
        args.type = BOOT_WIM;
      else
        args.type = BOOT_VHD;
    }
    else if (strcmp (key, "initrdfile") == 0 || strcmp (key, "initrd") == 0)  //initrd
    {
      if (! value || ! value[0])
        die ("Invalid initrd path\n");
      strncpy (args.initrd_path, value, 256);
      convert_path (args.initrd_path, 1);
    }
    else if (strcmp (key, "bgrt") == 0)   //bgrt
      args.bgrt = 1;
#if __x86_64__
    else if (strcmp (key, "win7") == 0)   //win7
      args.win7 = 1;
    else if (strcmp (key, "vgashim") == 0)//vga垫片
      args.vgashim = 1;
#endif
    else if (strcmp (key, "secureboot") == 0) //安全引导
    {
      if (! value || ! value[0])
        sprintf (args.sb, "off");
      else
        sprintf (args.sb, "%s", value);
    }
#endif
    if (strcmp (key, "nx") == 0)   //nx
    {
      if (! value || ! value[0])
        sprintf (args.nx, "OptIn");
      else
        sprintf (args.nx, "%s", value);
    }
    else if (strcmp (key, "pae") == 0)  //pae
    {
      if (! value || ! value[0])
        sprintf (args.pae, "Default");
      else
        sprintf (args.pae, "%s", value);
    }
    else if (strcmp (key, "loadopt") == 0)  //加载选项 
    {
      if (! value || ! value[0])
        sprintf (args.loadopt, "DDISABLE_INTEGRITY_CHECKS");
      else
      {
        sprintf (args.loadopt, "%s", value);
        convert_path (args.loadopt, 0);
      }
    }
    else if (strcmp (key, "winload") == 0)  //winload
    {
      if (! value || ! value[0])
        sprintf (args.winload, "\\Windows\\System32\\boot\\winload.efi");
      else
      {
        sprintf (args.winload, "%s", value);
        convert_path (args.winload, 1);
      }
    }
    else if (strcmp (key, "sysroot") == 0)  //系统根
    {
      if (! value || ! value[0])
        sprintf (args.sysroot, "\\Windows");
      else
      {
        sprintf (args.sysroot, "%s", value);
        convert_path (args.sysroot, 1);
      }
    }
#if 0
    else if (key == arg)
    {
      /* Ignore unknown initial arguments, which may
       * be the program name.  忽略未知的初始参数，可能是程序名。
       */
    }
#endif
    else if (strcmp (key, "minint") == 0)   //迷你
    {
      if (! value || ! value[0])
        sprintf (args.minint, "true");
      else
        sprintf (args.minint, "%s", value);
    }
    else if (strcmp (key, "testmode") == 0) //测试模式 
    {
      if (! value || ! value[0])
        sprintf (args.test_mode, "true");
      else
        sprintf (args.test_mode, "%s", value);
    }
    else if (strcmp (key, "hires") == 0)    //强制最高分辨率，默认关。如果包含hires关键字而无值，则默认开。0/1=关/开
    {
      if (! value || ! value[0])
        sprintf (args.hires, "true");
      else
        sprintf (args.hires, "%s", value);
    }
    else if (strcmp (key, "detecthal") == 0)//探长 
    {
      if (! value || ! value[0])
        sprintf (args.detecthal, "true");
      else
        sprintf (args.detecthal, "%s", value);
    }
    else if (strcmp (key, "novga") == 0)  //novga
    {
      if (! value || ! value[0])
        sprintf (args.novga, "true");
      else
        sprintf (args.novga, "%s", value);
    }
    else if (strcmp (key, "novesa") == 0)  //没有vesa 
    {
      if (! value || ! value[0])
        sprintf (args.novesa, "true");
      else
        sprintf (args.novesa, "%s", value);
    }
    else
      printf_errinfo ("Unrecognised argument \"%s%s%s\"\n", key,
           (value ? "=" : ""), (value ? value : ""));
  }
}

static int modify_bcd (char *filename, char *bcdname, int flags) //修改bcd
{
  grub_uint64_t start_addr;
	efi_status_t status;
  struct grub_part_data *p;
  struct grub_disk_data *d;
  char file[] = "/bootx64.efi";
  printf_debug("current_drive=%x, current_partition=%x, saved_drive=%x, saved_partition=%x\n",
          current_drive, current_partition, saved_drive, saved_partition);
  printf_debug("filename=%s\n",filename);

  if (filename[0] == '(')
  {
    char a = filename[1];
    filename = set_device (filename); //设置当前驱动器=输入驱动器号, 当前分区=输入分区号, 其余作为文件名 /efi/boot/bootx64.efi
    printf_debug("current_drive=%x, current_partition=%x, saved_drive=%x, saved_partition=%x\n",
          current_drive, current_partition, saved_drive, saved_partition);
    printf_debug("filename=%s\n",filename);
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

  memmove (args.path, filename, 256);  //复制文件路径
  convert_path (args.path, 1);              //转换路径
  if (p->partition_type != 0xee)	//mbr分区
  {
    start_addr = p->partition_start << 9;
    memmove (args.info.partid, &start_addr, 8);              //分区起始字节
    args.info.partmap = 0x01;
    memmove (args.info.diskid, &p->partition_signature, 4);  //磁盘id
    printf_debug("MBR: partition_start=%x, partition_signature=%x\n",
            p->partition_start,*(unsigned int *)&p->partition_signature);
  }
  else	//gpt分区
  {
    memmove (args.info.partid, &p->partition_signature, 16); //分区uuid
    args.info.partmap = 0x00;
    memmove (args.info.diskid, &d->disk_signature, 16);      //磁盘uuid
    printf_debug("GPT: partition_signature=%s\n",args.info.partid);
  }

  run_line ("find --set-root /loaderNT", flags);    //设置根到(hd-1)
  printf_debug("current_drive=%x, current_partition=%x, saved_drive=%x, saved_partition=%x\n",
          current_drive, current_partition, saved_drive, saved_partition);
  d = get_device_by_drive (current_drive,0);
  p = get_partition_info (current_drive, current_partition);
  status = vdisk_install (current_drive, current_partition);  //安装虚拟磁盘
  if (status != EFI_SUCCESS)
  {
    printf_errinfo ("Failed to install vdisk.(%x)\n",status);	//未能安装vdisk
    return 0;
  }

//重命名  bcdwim -> bcd
#if 0 //通过外部命令fat
  sprintf (temp,"/fat ren /%s bcd",bcdname); //外部命令，已打包
  run_line((char *)temp,1);
  query_block_entries = -1;
  run_line("blocklist /bcd",1);   //获得bcd在ntboot的偏移以及尺寸
  args.bcd_data = (start_sector + map_start_sector[0]) << 9;  //(ntboot在内存的扇区起始+bcd相对于ntboot的扇区偏移)转字节
  args.bcd_len = map_num_sectors[0] << 9;   //(bcd扇区尺寸)转字节
#else //通过FAT12/16文件结构
  grub_size_t i; 
  char *short_name, *long_name;
  grub_uint32_t file_len, file_off;
  grub_uint64_t read_addr;
  printf_debug("start_sector=%x, partition_start=%x\n",
          d->start_sector,p->partition_start);
  start_addr ^= start_addr;
  start_addr = (d->start_sector + p->partition_start) << 9; //BCD 所在卷在内存的基地址字节
  char *addr = (char *)start_addr;            //BCD 所在卷基地址字节指针
  read_addr = start_addr + ((*(grub_uint16_t *)(addr + 0xe) + ((*(grub_uint16_t *)(addr + 0x16)) << 1)) << 9); //FAT16主目录字节地址
  char *dir = (char *)read_addr;  //目录指针

  for (i = 0; i < 0x200; i += 0x20, dir += 0x20)
  {
    if (dir[0] == 0xe5 || dir[0] == 0)  //已删除或空项目
      continue;

    if (memcmp (dir, bcdname, strlen(bcdname)) == 0) //如果文件名相等
    {
      file_off = ((*(grub_uint16_t *)&dir[0x1a] - 2) * (*(grub_uint8_t *)&addr[0xd])  //(文件簇号－2)×每簇扇区数
          /*+ (read_addr >> 9)*/                                                      //主目录表起始相对逻辑扇区
          + ((*(grub_uint16_t *)&addr[0x11]) >> 4))                                   //0x20×主目录表文件数÷每扇区字节数
          << 9;                                  //文件起始在卷的字节偏移  88add  1115ba00
      file_len = *(grub_uint32_t *)&dir[0x1c];   //文件字节尺寸
      break;
    }
  }
  printf_debug("i=%x\n",i);
  if (i >= 0x200)
    return 0; 

  //修改短文件名
  short_name = (char *)(grub_size_t)(i+read_addr);
  short_name[3] = ' ';
  short_name[4] = ' ';
  short_name[5] = ' ';
  //修改长文件名
  long_name = short_name - 32;
  if (long_name[11] == 0x0f)
  {
    long_name[7] = 0;
    long_name[9] = 0xff;
    long_name[10] = 0xff;
    long_name[14] = 0xff;
    long_name[15] = 0xff;
    long_name[16] = 0xff;
    long_name[17] = 0xff;
    long_name[13] = 0x6e;
  } 

  args.bcd_data = read_addr + file_off;//10e54400
  args.bcd_len = file_len;
  printf_debug("bcd_data=%x, bcd_len=%x\n",
          args.bcd_data, args.bcd_len);
  printf_debug("current_drive=%x, current_partition=%x, saved_drive=%x, saved_partition=%x\n",
          current_drive, current_partition, saved_drive, saved_partition);
#endif 

  return 1;
}

static void convert_path (char *str, int backslash)  //转换路径(路径,反斜杠)
{
  char *p = str;
  while (*p)
  {
    if (*p == '/' && backslash) //如果是'/',并且反斜杠存在
      *p = '\\';                //修改为'\\'
    if (*p == ':')              //如果是':'
      *p = ' ';                 //修改为' '
    if (*p == '\r' || *p == '\n') //如果是回车换行
      *p = '\0';                  //修改为0
    p++;
  }
}


static void
bcd_print_hex (const void *data, grub_size_t len)
{
  const grub_uint8_t *p = data;
  grub_size_t i;
  for (i = 0; i < len; i++)
    printf_debug ("%02x ", p[i]);
}

static void
bcd_replace_hex (const void *search, grub_uint32_t search_len,
                 const void *replace, grub_uint32_t replace_len, int count) //bcd替换十六进制数据 
{
  grub_uint8_t *p = (grub_uint8_t *)nt_cmdline->bcd_data;
  grub_uint32_t offset;
  int cnt = 0;
  for (offset = 0; offset + search_len < nt_cmdline->bcd_len; offset++)
  {
    if (memcmp (p + offset, search, search_len) == 0)
    {
      cnt++;
      if (debug > 3)
      {
        printf_debug ("0x%08x ", offset);
        bcd_print_hex (search, search_len);
        printf_debug ("\n---> ");
        bcd_print_hex (replace, replace_len);
        printf_debug ("\n");
      }

      memmove (p + offset, replace, replace_len);
      printf_debug ("...patched BCD at %x len %x\n", offset, replace_len);
      if (count && cnt == count)
        break;
    }
  }
}

#define GRUB_UINT8_1_LEADINGBIT 0x80
#define GRUB_UINT8_2_LEADINGBITS 0xc0
#define GRUB_UINT8_3_LEADINGBITS 0xe0
#define GRUB_UINT8_4_LEADINGBITS 0xf0
#define GRUB_UINT8_5_LEADINGBITS 0xf8
#define GRUB_UINT8_6_LEADINGBITS 0xfc
#define GRUB_UINT8_7_LEADINGBITS 0xfe

#define GRUB_UINT8_1_TRAILINGBIT 0x01
#define GRUB_UINT8_2_TRAILINGBITS 0x03
#define GRUB_UINT8_3_TRAILINGBITS 0x07
#define GRUB_UINT8_4_TRAILINGBITS 0x0f
#define GRUB_UINT8_5_TRAILINGBITS 0x1f
#define GRUB_UINT8_6_TRAILINGBITS 0x3f

#define GRUB_MAX_UTF8_PER_UTF16 4
/* You need at least one UTF-8 byte to have one UTF-16 word.
   You need at least three UTF-8 bytes to have 2 UTF-16 words (surrogate pairs).
 */
#define GRUB_MAX_UTF16_PER_UTF8 1
#define GRUB_MAX_UTF8_PER_CODEPOINT 4

#define GRUB_UCS2_LIMIT 0x10000
#define GRUB_UTF16_UPPER_SURROGATE(code) \
  (0xD800 | ((((code) - GRUB_UCS2_LIMIT) >> 10) & 0x3ff))
#define GRUB_UTF16_LOWER_SURROGATE(code) \
  (0xDC00 | (((code) - GRUB_UCS2_LIMIT) & 0x3ff))
  
/* Process one character from UTF8 sequence. 
   At beginning set *code = 0, *count = 0. Returns 0 on failure and
   1 on success. *count holds the number of trailing bytes.  */
static inline int
grub_utf8_process (uint8_t c, uint32_t *code, int *count)
{
  if (*count)
  {
    if ((c & GRUB_UINT8_2_LEADINGBITS) != GRUB_UINT8_1_LEADINGBIT)
    {
      *count = 0;
      /* invalid */
      return 0;
    }
    else
    {
      *code <<= 6;
      *code |= (c & GRUB_UINT8_6_TRAILINGBITS);
      (*count)--;
      /* Overlong.  */
      if ((*count == 1 && *code <= 0x1f) || (*count == 2 && *code <= 0xf))
      {
        *code = 0;
        *count = 0;
        return 0;
      }
      return 1;
    }
  }

  if ((c & GRUB_UINT8_1_LEADINGBIT) == 0)
  {
    *code = c;
    return 1;
  }
  if ((c & GRUB_UINT8_3_LEADINGBITS) == GRUB_UINT8_2_LEADINGBITS)
  {
    *count = 1;
    *code = c & GRUB_UINT8_5_TRAILINGBITS;
    /* Overlong */
    if (*code <= 1)
    {
      *count = 0;
      *code = 0;
      return 0;
    }
    return 1;
  }
  if ((c & GRUB_UINT8_4_LEADINGBITS) == GRUB_UINT8_3_LEADINGBITS)
  {
    *count = 2;
    *code = c & GRUB_UINT8_4_TRAILINGBITS;
    return 1;
  }
  if ((c & GRUB_UINT8_5_LEADINGBITS) == GRUB_UINT8_4_LEADINGBITS)
  {
    *count = 3;
    *code = c & GRUB_UINT8_3_TRAILINGBITS;
    return 1;
  }
  return 0;
}
/* Convert a (possibly null-terminated) UTF-8 string of at most SRCSIZE
   bytes (if SRCSIZE is -1, it is ignored) in length to a UTF-16 string.
   Return the number of characters converted. DEST must be able to hold
   at least DESTSIZE characters. If an invalid sequence is found, return -1.
   If SRCEND is not NULL, then *SRCEND is set to the next byte after the
   last byte used in SRC.  */
static inline grub_size_t
grub_utf8_to_utf16 (grub_uint16_t *dest, grub_size_t destsize,
                    const grub_uint8_t *src, grub_size_t srcsize,
                    const grub_uint8_t **srcend)
{
  grub_uint16_t *p = dest;
  int count = 0;
  grub_uint32_t code = 0;

  if (srcend)
    *srcend = src;

  while (srcsize && destsize)
  {
    int was_count = count;
    if (srcsize != (grub_size_t)-1)
      srcsize--;
    if (!grub_utf8_process (*src++, &code, &count))
    {
      code = '?';
      count = 0;
      /* Character c may be valid, don't eat it.  */
      if (was_count)
        src--;
    }
    if (count != 0)
      continue;
    if (code == 0)
      break;
    if (destsize < 2 && code >= GRUB_UCS2_LIMIT)
      break;
    if (code >= GRUB_UCS2_LIMIT)
    {
      *p++ = GRUB_UTF16_UPPER_SURROGATE (code);
      *p++ = GRUB_UTF16_LOWER_SURROGATE (code);
      destsize -= 2;
    }
    else
    {
      *p++ = code;
      destsize--;
    }
  }

  if (srcend)
    *srcend = src;
  return p - dest;
}

static void
bcd_patch_path (void)  //bcd修补路径
{
  const char *search = "\\PATH_SIGN";
  grub_size_t len;
  len = 2 * (strlen (nt_cmdline->path) + 1);  //wim/vhd的:  /路径/文件名
  /* UTF-8 to UTF-16le */
  grub_utf8_to_utf16 (nt_cmdline->path16, len,
                      (grub_uint8_t *)nt_cmdline->path, -1, NULL);  //转换为utf16

  bcd_replace_hex (search, strlen (search), nt_cmdline->path16, len, 0);  //bcd替换十六进制数据
}

static inline int islower (int c)
{
  return ((c >= 'a') && (c <= 'z'));
}

static inline int toupper (int c)
{
  if (islower (c))
  {
    c -= ('a' - 'A');
  }
  return c;
}

static inline int towupper (wint_t c)
{
  return toupper (c);
}

/**
 * Compare two wide-character strings, case-insensitively
 *
 * @v str1    First string
 * @v str2    Second string
 * @ret diff    Difference
 */
int wcscasecmp (const wchar_t *str1, const wchar_t *str2)
{
  int c1;
  int c2;
  do
  {
    c1 = towupper (* (str1++));
    c2 = towupper (* (str2++));
  }
  while ((c1 != L'\0') && (c1 == c2));
  return (c1 - c2);
}

static void
bcd_patch_hive (reg_hive_t *hive, const wchar_t *keyname, void *val) //bcd补丁配置单元 
{
  HKEY root, objects, osloader, elements, key;
  grub_uint8_t *data = NULL;
  grub_uint32_t data_len = 0, type;

  hive->find_root (hive, &root);  //根 
  //hive->find_key (hive, root, (const grub_uint16_t*)BCD_REG_ROOT, &objects);  //L"Objects"
  hive->find_key (hive, root, BCD_REG_ROOT, &objects);  //L"Objects"  目标
  if (wcscasecmp (keyname, BCDOPT_TIMEOUT) == 0)  //L"25000004"  超时
    hive->find_key (hive, objects, GUID_BOOTMGR, &osloader);  //L"{9dea862c-5cdd-4e70-acc1-f32b344d4795}"
  else if (wcscasecmp (keyname, BCDOPT_DISPLAY) == 0) //L"26000020"  显示 
    hive->find_key (hive, objects, GUID_BOOTMGR, &osloader);  //L"{9dea862c-5cdd-4e70-acc1-f32b344d4795}"
  else if (wcscasecmp (keyname, BCDOPT_IMGOFS) == 0)  //L"35000001"  ramdisk 选项
    hive->find_key (hive, objects, GUID_RAMDISK, &osloader);  //L"{ae5534e0-a924-466c-b836-758539a3ee3a}"
  else
    hive->find_key (hive, objects, GUID_OSENTRY, &osloader);  //L"{19260817-6666-8888-abcd-000000000000}"
  hive->find_key (hive, osloader, BCD_REG_HKEY, &elements); //L"Elements"  元素
  hive->find_key (hive, elements, keyname, &key);
  hive->query_value_no_copy (hive, key, BCD_REG_HVAL, //L"Element"
                             (void **)&data, &data_len, &type);
  memmove (data, val, data_len);
  printf_debug ("...patched %x len %x\n", data, data_len);
}

/**
 * Compare two strings, case-insensitively
 *
 * @v str1    First string
 * @v str2    Second string
 * @ret diff    Difference
 */
int strcasecmp (const char *str1, const char *str2)
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

char tmp[32];
static void wchar_to_char (const wchar_t *keyname);
static void wchar_to_char (const wchar_t *keyname)
{
  int i;
  for (i=0; i<8; i++)
    tmp[i] = *(char *)&keyname[i];
  tmp[i] = 0;
}

static void
bcd_parse_bool (reg_hive_t *hive, const wchar_t *keyname, const char *s)  //bcd解析布尔
{
  grub_uint8_t val = 0;
  if (strcasecmp (s, "yes") == 0 || strcasecmp (s, "on") == 0 ||
      strcasecmp (s, "true") == 0 || strcasecmp (s, "1") == 0)
    val = 1;
  wchar_to_char (keyname);
  printf_debug ("...patching key %s value %x\n", tmp, val);
  bcd_patch_hive (hive, keyname, &val);
}

/**
 * Convert a string to an unsigned integer
 *
 * @v nptr    String
 * @v endptr    End pointer to fill in (or NULL)
 * @v base    Numeric base
 * @ret val   Value
 */
unsigned long strtoul (const char *nptr, char **endptr, int base)
{
  unsigned long val = 0;
  int negate = 0;
  unsigned int digit;
  /* Skip any leading whitespace */
  while (isspace (*nptr))
  {
    nptr++;
  }
  /* Parse sign, if present */
  if (*nptr == '+')
  {
    nptr++;
  }
  else if (*nptr == '-')
  {
    nptr++;
    negate = 1;
  }
  /* Parse base */
  if (base == 0)
  {
    /* Default to decimal */
    base = 10;
    /* Check for octal or hexadecimal markers */
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
  /* Parse digits */
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
  /* Record end marker, if applicable */
  if (endptr)
  {
    *endptr = ((char *) nptr);
  }
  /* Return value */
  return (negate ? -val : val);
}

static void
bcd_parse_u64 (reg_hive_t *hive, const wchar_t *keyname, const char *s)
{
  grub_uint64_t val = 0;
  val = strtoul (s, NULL, 0);
  bcd_patch_hive (hive, (const wchar_t *)keyname, &val);
}

static void
bcd_parse_str (reg_hive_t *hive, const wchar_t *keyname,
               grub_uint8_t resume, const char *s)  //bcd解析字符串 
{
  HKEY root, objects, osloader, elements, key;
  grub_uint16_t *data = NULL;
  grub_uint32_t data_len = 0, type;
  wchar_to_char (keyname);
  printf_debug ("...patching key %s value %s\n", tmp, s);

  hive->find_root (hive, &root);  //根
  hive->find_key (hive, root, BCD_REG_ROOT, &objects);  //L"Objects"  选项

  if (resume)  //恢复
    hive->find_key (hive, objects, GUID_REENTRY, &osloader);  //L"{19260817-6666-8888-abcd-000000000001}"  再入境
  else 
    hive->find_key (hive, objects, GUID_OSENTRY, &osloader);  //L"{19260817-6666-8888-abcd-000000000000}"  OS入口

  hive->find_key (hive, osloader, BCD_REG_HKEY, &elements); //L"Elements"  元素
  hive->find_key (hive, elements, keyname, &key);
  hive->query_value_no_copy (hive, key, BCD_REG_HVAL, //L"Element"
                             (void **)&data, &data_len, &type);
  memset (data, 0, data_len);
  grub_utf8_to_utf16 (data, data_len, (grub_uint8_t *)s, -1, NULL);
}

static void
bcd_patch_data (void)  //bcd修补程序数据 
{
  static const wchar_t a[] = L".exe";
  static const wchar_t b[] = L".efi";
  reg_hive_t *hive = NULL;  //寄存器蜂巢

  if (open_hive ((void *)nt_cmdline->bcd_data, nt_cmdline->bcd_len, &hive) || !hive)
    printf_errinfo ("BCD hive load error.\n"); //BCD配置单元加载错误
  else
    printf_debug ("BCD hive load OK.\n");    //BCD配置单元加载成功

  if (nt_cmdline->type != BOOT_WIN)   //wim/vhd
    bcd_patch_path ();  //bcd修补路径  填充wim/vhd文件的:  /路径/文件名

  bcd_replace_hex (BCD_DP_MAGIC, strlen (BCD_DP_MAGIC),     //"GNU GRUB2 NTBOOT"
                   &nt_cmdline->info, sizeof (struct bcd_disk_info), 0); //bcd替换十六进制数据  填充填充wim/vhd/win文件的: 磁盘uuid, 分区起始

  /* display menu 显示菜单,开
   * default:   no 默认关*/
  bcd_parse_bool (hive, BCDOPT_DISPLAY, "yes");  //bcd解析布尔
  /* timeout      超时,关
   * default:   1 默认1秒*/
  bcd_parse_u64 (hive, BCDOPT_TIMEOUT, "0");
  /* testsigning  测试签名,按输入
   * default:   no 默认关*/
  if (nt_cmdline->test_mode[0])
    bcd_parse_bool (hive, BCDOPT_TESTMODE, nt_cmdline->test_mode);  //16000049
  else
    bcd_parse_bool (hive, BCDOPT_TESTMODE, "no");
  /* force highest resolution 强制最高分辨率,按输入
   * default:   no 默认关*/
  if (nt_cmdline->hires[0])
    bcd_parse_bool (hive, BCDOPT_HIGHEST, nt_cmdline->hires);
  else
    bcd_parse_bool (hive, BCDOPT_HIGHEST, "no");
  /* detect hal and kernel  检测hal和内核,按输入
   * default:   yes 默认是*/
  if (nt_cmdline->detecthal[0])
    bcd_parse_bool (hive, BCDOPT_DETHAL, nt_cmdline->detecthal);
  else
    bcd_parse_bool (hive, BCDOPT_DETHAL, "yes");
  /* winpe mode   WinPE模式,按输入
   * default:
   *      OS  - no
   *      VHD - no
   *      WIM - yes */
  if (nt_cmdline->minint[0])
    bcd_parse_bool (hive, BCDOPT_WINPE, nt_cmdline->minint);  //26000022
  else
  {
    if (nt_cmdline->type == BOOT_WIM)
      bcd_parse_bool (hive, BCDOPT_WINPE, "yes");
    else
      bcd_parse_bool (hive, BCDOPT_WINPE, "no");
  }
  /* disable vesa 禁用vesa,按输入
   * default:   no 默认关*/
  if (nt_cmdline->novesa[0])
    bcd_parse_bool (hive, BCDOPT_NOVESA, nt_cmdline->novesa);
  else
    bcd_parse_bool (hive, BCDOPT_NOVESA, "no");
  /* disable vga 禁用vga,按输入
   * default:   no 默认关*/
  if (nt_cmdline->novga[0])
    bcd_parse_bool (hive, BCDOPT_NOVGA, nt_cmdline->novga);
  else
    bcd_parse_bool (hive, BCDOPT_NOVGA, "no");
  /* nx policy  nx策略,按输入
   * default:   OptIn */
  if (nt_cmdline->nx[0])
  {
    uint64_t nx = 0;
    if (strcasecmp (nt_cmdline->nx, "OptIn") == 0)
      nx = NX_OPTIN;
    else if (strcasecmp (nt_cmdline->nx, "OptOut") == 0)
      nx = NX_OPTOUT;
    else if (strcasecmp (nt_cmdline->nx, "AlwaysOff") == 0)
      nx = NX_ALWAYSOFF;
    else if (strcasecmp (nt_cmdline->nx, "AlwaysOn") == 0)
      nx = NX_ALWAYSON;
    bcd_patch_hive (hive, (const wchar_t *)BCDOPT_NX, &nx);
  }
  /* pae        pae策略,按输入
   * default:   Default */
  if (nt_cmdline->pae[0])
  {
    grub_uint64_t pae = 0;
    if (strcasecmp (nt_cmdline->pae, "Default") == 0)     //默认
      pae = PAE_DEFAULT;
    else if (strcasecmp (nt_cmdline->pae, "Enable") == 0) //使能
      pae = PAE_ENABLE;
    else if (strcasecmp (nt_cmdline->pae, "Disable") == 0)//禁止
      pae = PAE_DISABLE;
    bcd_patch_hive (hive, BCDOPT_PAE, &pae);
  }
  /* load options 装载选项
   * default:   DDISABLE_INTEGRITY_CHECKS 可删除完整性检查*/
  if (nt_cmdline->loadopt[0])
    bcd_parse_str (hive, BCDOPT_CMDLINE, 0, nt_cmdline->loadopt); //L"12000030"
  else
    bcd_parse_str (hive, BCDOPT_CMDLINE, 0, BCD_DEFAULT_CMDLINE); //BCD默认命令行  DDISABLE_INTEGRITY_CHECKS 
  /* winload.efi
   * default:
   *      OS  - \\Windows\\System32\\winload.efi
   *      VHD - \\Windows\\System32\\winload.efi
   *      WIM - \\Windows\\System32\\boot\\winload.efi */
  if (nt_cmdline->winload[0])
    bcd_parse_str (hive, BCDOPT_WINLOAD, 0, nt_cmdline->winload);     //win
  else
  {
    if (nt_cmdline->type == BOOT_WIM)
      bcd_parse_str (hive, BCDOPT_WINLOAD, 0, BCD_LONG_WINLOAD);  //wim
    else
      bcd_parse_str (hive, BCDOPT_WINLOAD, 0, BCD_SHORT_WINLOAD); //vhd
  }
  /* windows system root  按输入
   * default:   \\Windows */
  if (nt_cmdline->sysroot[0])
    bcd_parse_str (hive, BCDOPT_SYSROOT, 0, nt_cmdline->sysroot);
  else
    bcd_parse_str (hive, BCDOPT_SYSROOT, 0, BCD_DEFAULT_SYSROOT); //"\\Windows"
  /* windows resume windows恢复*/
  if (nt_cmdline->type == BOOT_WIN)
  {
    bcd_parse_str (hive, BCDOPT_REPATH, 1, BCD_DEFAULT_WINRESUME);  //L"12000002"  "\\Windows\\System32\\winresume.efi"
    bcd_parse_str (hive, BCDOPT_REHIBR, 1, BCD_DEFAULT_HIBERFIL);   //L"22000002"  "\\hiberfil.sys"
  }
  
  if (grub_efi_system_table)
    bcd_replace_hex (a, 8, b, 8, 0);
  else
    bcd_replace_hex (b, 8, a, 8, 0);
}

#define _CR(RECORD, TYPE, FIELD) \
    ((TYPE *) ((char *) (RECORD) - (char *) &(((TYPE *) 0)->FIELD)))

#define _offsetof(TYPE, MEMBER) ((grub_size_t) &((TYPE *)NULL)->MEMBER)

#pragma GCC diagnostic ignored "-Wcast-align"

enum reg_bool
{
  false = 0,
  true  = 1,
};

static grub_size_t
reg_wcslen (const grub_uint16_t *s) //注册会员
{
  grub_size_t i = 0;
  while (s[i] != 0)
    i++;
  return i;
}

static enum reg_bool check_header(hive_t *h) //注册布尔检查头 
{
  HBASE_BLOCK* base_block = (HBASE_BLOCK*)h->data;
  uint32_t csum;

  if (base_block->Signature != HV_HBLOCK_SIGNATURE)
  {
    printf_debug ("Invalid signature.\n");
    return false;
  }

  if (base_block->Major != HSYS_MAJOR)
  {
    printf_debug ("Invalid major value.\n");
    return false;
  }

  if (base_block->Minor < HSYS_MINOR)
  {
    printf_debug ("Invalid minor value.\n");
    return false;
  }

  if (base_block->Type != HFILE_TYPE_PRIMARY)
  {
    printf_debug ("Type was not HFILE_TYPE_PRIMARY.\n");
    return false;
  }

  if (base_block->Format != HBASE_FORMAT_MEMORY)
  {
    printf_debug ("Format was not HBASE_FORMAT_MEMORY.\n");
    return false;
  }

  if (base_block->Cluster != 1)
  {
    printf_debug ("Cluster was not 1.\n");
    return false;
  }

  // FIXME - should apply LOG file if sequences don't match  如果序列不匹配，则应应用 LOG 文件
  if (base_block->Sequence1 != base_block->Sequence2)
  {
    printf_debug ("Sequence1 did not match Sequence2.\n");
    return false;
  }

  // check checksum  校验和 
  csum = 0;
  unsigned int i;

  for (i = 0; i < 127; i++)
  {
    csum ^= ((uint32_t*)h->data)[i];
  }
  if (csum == 0xffffffff)
    csum = 0xfffffffe;
  else if (csum == 0)
    csum = 1;

  if (csum != base_block->CheckSum)
  {
    printf_debug ("Invalid checksum.\n");
    return false;
  }
  return true;
}

static void close_hive (reg_hive_t *this) //关闭蜂巢
{
  hive_t *h = _CR(this, hive_t, public);
  memset (h, 0, sizeof (hive_t));
}

static void find_root (reg_hive_t *this, HKEY* key)  //查找根
{
  hive_t *h = _CR(this, hive_t, public);
  HBASE_BLOCK *base_block = (HBASE_BLOCK *)h->data;

  *key = 0x1000 + base_block->RootCell;
}

static reg_err_t
enum_keys (reg_hive_t *this, HKEY key, grub_uint32_t index,
           grub_uint16_t *name, grub_uint32_t name_len)  //枚举键
{
  hive_t *h = _CR(this, hive_t, public);
  grub_int32_t size;
  CM_KEY_NODE* nk;
  CM_KEY_FAST_INDEX* lh;
  CM_KEY_NODE* nk2;
  enum reg_bool overflow = false;
  unsigned int i;

  // FIXME - make sure no buffer overruns (here and elsewhere)
  // find parent key node

  size = -*(grub_int32_t*)((grub_uint8_t*)h->data + key);

  if (size < 0)
    return REG_ERR_FILE_NOT_FOUND;

  if ((grub_uint32_t)size < sizeof(grub_int32_t) + _offsetof(CM_KEY_NODE, Name[0]))
    return REG_ERR_BAD_ARGUMENT;

  nk = (CM_KEY_NODE*)((grub_uint8_t*)h->data + key + sizeof(grub_int32_t));

  if (nk->Signature != CM_KEY_NODE_SIGNATURE)
    return REG_ERR_BAD_ARGUMENT;

  if ((grub_uint32_t)size < sizeof(grub_int32_t)
      + _offsetof(CM_KEY_NODE, Name[0]) + nk->NameLength)
    return REG_ERR_BAD_ARGUMENT;

  // FIXME - volatile keys?

  if (index >= nk->SubKeyCount || nk->SubKeyList == 0xffffffff)
    return REG_ERR_FILE_NOT_FOUND;

  // go to key index

  size = -*(grub_int32_t*)((grub_uint8_t*)h->data + 0x1000 + nk->SubKeyList);

  if (size < 0)
    return REG_ERR_FILE_NOT_FOUND;

  if ((grub_uint32_t)size < sizeof(grub_int32_t) + _offsetof(CM_KEY_FAST_INDEX, List[0]))
    return REG_ERR_BAD_ARGUMENT;

  lh = (CM_KEY_FAST_INDEX*)((grub_uint8_t*)h->data + 0x1000
                            + nk->SubKeyList + sizeof(grub_int32_t));

  if (lh->Signature != CM_KEY_HASH_LEAF && lh->Signature != CM_KEY_LEAF)
    return REG_ERR_BAD_ARGUMENT;

  if ((grub_uint32_t)size < sizeof(grub_int32_t) + _offsetof(CM_KEY_FAST_INDEX, List[0])
      + (lh->Count * sizeof(CM_INDEX)))
    return REG_ERR_BAD_ARGUMENT;

  if (index >= lh->Count)
    return REG_ERR_BAD_ARGUMENT;

  // find child key node

  size = -*(grub_int32_t*)((grub_uint8_t*)h->data + 0x1000 + lh->List[index].Cell);

  if (size < 0)
    return REG_ERR_FILE_NOT_FOUND;

  if ((grub_uint32_t)size < sizeof(grub_int32_t) + _offsetof(CM_KEY_NODE, Name[0]))
    return REG_ERR_BAD_ARGUMENT;

  nk2 = (CM_KEY_NODE*)((grub_uint8_t*)h->data + 0x1000
                       + lh->List[index].Cell + sizeof(grub_int32_t));

  if (nk2->Signature != CM_KEY_NODE_SIGNATURE)
    return REG_ERR_BAD_ARGUMENT;

  if ((grub_uint32_t)size < sizeof(grub_int32_t) 
      + _offsetof(CM_KEY_NODE, Name[0]) + nk2->NameLength)
    return REG_ERR_BAD_ARGUMENT;

  if (nk2->Flags & KEY_COMP_NAME)
  {
    char* nkname = (char*)nk2->Name;
    for (i = 0; i < nk2->NameLength; i++)
    {
      if (i >= name_len)
      {
        overflow = true;
        break;
      }
      name[i] = nkname[i];
    }
    name[i] = 0;
  }
  else
  {
    for (i = 0; i < nk2->NameLength / sizeof(grub_uint16_t); i++)
    {
      if (i >= name_len)
      {
        overflow = true;
        break;
      }
      name[i] = nk2->Name[i];
    }
    name[i] = 0;
  }

  return overflow ? REG_ERR_OUT_OF_MEMORY : REG_ERR_NONE;
}

static reg_err_t
find_child_key (hive_t* h, HKEY parent,
                const grub_uint16_t* namebit, grub_size_t nblen, HKEY* key)  //找到子键 
{
  grub_int32_t size;
  CM_KEY_NODE* nk;
  CM_KEY_FAST_INDEX* lh;

  // find parent key node
  size = -*(grub_int32_t*)((grub_uint8_t*)h->data + parent);
  if (size < 0)
    return REG_ERR_FILE_NOT_FOUND; //3

  if ((grub_uint32_t)size < sizeof(grub_int32_t) + _offsetof(CM_KEY_NODE, Name[0]))
    return REG_ERR_BAD_ARGUMENT;  //6
  nk = (CM_KEY_NODE*)((grub_uint8_t*)h->data + parent + sizeof(grub_int32_t));
  if (nk->Signature != CM_KEY_NODE_SIGNATURE)
    return REG_ERR_BAD_ARGUMENT;  //6
  if ((grub_uint32_t)size < sizeof(grub_int32_t)
      + _offsetof(CM_KEY_NODE, Name[0]) + nk->NameLength)
    return REG_ERR_BAD_ARGUMENT;  //6
  if (nk->SubKeyCount == 0 || nk->SubKeyList == 0xffffffff)
    return REG_ERR_FILE_NOT_FOUND; //3
  // go to key index
  size = -*(grub_int32_t*)((grub_uint8_t*)h->data + 0x1000 + nk->SubKeyList);
  if (size < 0)
    return REG_ERR_FILE_NOT_FOUND; //3
  if ((grub_uint32_t)size < sizeof(grub_int32_t) + _offsetof(CM_KEY_FAST_INDEX, List[0]))
    return REG_ERR_BAD_ARGUMENT;  //6

  lh = (CM_KEY_FAST_INDEX*)((grub_uint8_t*)h->data + 0x1000
                            + nk->SubKeyList + sizeof(grub_int32_t));

  if (lh->Signature != CM_KEY_HASH_LEAF && lh->Signature != CM_KEY_LEAF)
    return REG_ERR_BAD_ARGUMENT;  //6
  if ((grub_uint32_t)size < sizeof(grub_int32_t)
      + _offsetof(CM_KEY_FAST_INDEX, List[0]) + (lh->Count * sizeof(CM_INDEX)))
    return REG_ERR_BAD_ARGUMENT;  //6
  // FIXME - check against hashes
  unsigned int i;
  for (i = 0; i < lh->Count; i++)
  {
    CM_KEY_NODE* nk2;
    size = -*(grub_int32_t*)((grub_uint8_t*)h->data + 0x1000 + lh->List[i].Cell);
    if (size < 0)
      continue;
    if ((grub_uint32_t)size < sizeof(grub_int32_t) + _offsetof(CM_KEY_NODE, Name[0]))
      continue;

    nk2 = (CM_KEY_NODE*)((grub_uint8_t*)h->data
                         + 0x1000 + lh->List[i].Cell + sizeof(grub_int32_t));
    if (nk2->Signature != CM_KEY_NODE_SIGNATURE)
      continue;
    if ((grub_uint32_t)size < sizeof(grub_int32_t)
        + _offsetof(CM_KEY_NODE, Name[0]) + nk2->NameLength)
      continue;
    // FIXME - use string protocol here to do comparison properly?
    if (nk2->Flags & KEY_COMP_NAME)
    {
      unsigned int j;
      char* name = (char*)nk2->Name;

      if (nk2->NameLength != nblen)
        continue;

      for (j = 0; j < nk2->NameLength; j++)
      {
        grub_uint16_t c1 = name[j];
        grub_uint16_t c2 = namebit[j];
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
      if (nk2->NameLength / sizeof(grub_uint16_t) != nblen)
        continue;

      for (j = 0; j < nk2->NameLength / sizeof(grub_uint16_t); j++)
      {
        grub_uint16_t c1 = nk2->Name[j];
        grub_uint16_t c2 = namebit[j];

        if (c1 >= 'A' && c1 <= 'Z')
          c1 = c1 - 'A' + 'a';
        if (c2 >= 'A' && c2 <= 'Z')
          c2 = c2 - 'A' + 'a';
        if (c1 != c2)
          break;
      }
      if (j != nk2->NameLength / sizeof(grub_uint16_t))
        continue;
      *key = 0x1000 + lh->List[i].Cell;
      return REG_ERR_NONE;
    }
  }
  return REG_ERR_FILE_NOT_FOUND; //3
}

static reg_err_t
find_key (reg_hive_t* this, HKEY parent, const grub_uint16_t* path, HKEY* key)  //查找键
{
  reg_err_t errno;
  hive_t* h = _CR(this, hive_t, public);
  grub_size_t nblen;
  HKEY k;

  do
  {
    nblen = 0;
    while (path[nblen] != '\\' && path[nblen] != 0)
    {
      nblen++;
    }
    errno = find_child_key (h, parent, path, nblen, &k);
    if (errno)
      return errno;
    if (path[nblen] == 0 || (path[nblen] == '\\' && path[nblen + 1] == 0))
    {
      *key = k;
      return errno;
    }

    parent = k;
    path = &path[nblen + 1];
  }
  while (1);
}

static reg_err_t
enum_values (reg_hive_t *this, HKEY key,
             grub_uint32_t index, grub_uint16_t* name, grub_uint32_t name_len, grub_uint32_t* type)  //枚举值 
{
  hive_t* h = _CR(this, hive_t, public);
  grub_int32_t size;
  CM_KEY_NODE* nk;
  grub_uint32_t* list;
  CM_KEY_VALUE* vk;
  enum reg_bool overflow = false;
  unsigned int i;

  // find key node
  size = -*(grub_int32_t*)((grub_uint8_t*)h->data + key);

  if (size < 0)
    return REG_ERR_FILE_NOT_FOUND;

  if ((grub_uint32_t)size < sizeof(grub_int32_t) + _offsetof(CM_KEY_NODE, Name[0]))
    return REG_ERR_BAD_ARGUMENT;

  nk = (CM_KEY_NODE*)((grub_uint8_t*)h->data + key + sizeof(grub_int32_t));

  if (nk->Signature != CM_KEY_NODE_SIGNATURE)
    return REG_ERR_BAD_ARGUMENT;

  if ((grub_uint32_t)size < sizeof(grub_int32_t) 
      + _offsetof(CM_KEY_NODE, Name[0]) + nk->NameLength)
    return REG_ERR_BAD_ARGUMENT;

  if (index >= nk->ValuesCount || nk->Values == 0xffffffff)
    return REG_ERR_FILE_NOT_FOUND;

  // go to key index
  size = -*(grub_int32_t*)((grub_uint8_t*)h->data + 0x1000 + nk->Values);

  if (size < 0)
    return REG_ERR_FILE_NOT_FOUND;

  if ((grub_uint32_t)size < sizeof(grub_int32_t) + (sizeof(grub_uint32_t) * nk->ValuesCount))
    return REG_ERR_BAD_ARGUMENT;

  list = (grub_uint32_t*)((grub_uint8_t*)h->data + 0x1000 + nk->Values + sizeof(grub_int32_t));

  // find value node
  size = -*(grub_int32_t*)((grub_uint8_t*)h->data + 0x1000 + list[index]);

  if (size < 0)
    return REG_ERR_FILE_NOT_FOUND;

  if ((grub_uint32_t)size < sizeof(grub_int32_t) + _offsetof(CM_KEY_VALUE, Name[0]))
    return REG_ERR_BAD_ARGUMENT;

  vk = (CM_KEY_VALUE*)((grub_uint8_t*)h->data + 0x1000 + list[index] + sizeof(grub_int32_t));

  if (vk->Signature != CM_KEY_VALUE_SIGNATURE)
    return REG_ERR_BAD_ARGUMENT;

  if ((grub_uint32_t)size < sizeof(grub_int32_t)
      + _offsetof(CM_KEY_VALUE, Name[0]) + vk->NameLength)
    return REG_ERR_BAD_ARGUMENT;

  if (vk->Flags & VALUE_COMP_NAME)
  {
    
    char* nkname = (char*)vk->Name;
    for (i = 0; i < vk->NameLength; i++)
    {
      if (i >= name_len)
      {
        overflow = true;
        break;
      }
      name[i] = nkname[i];
    }
    name[i] = 0;
  }
  else
  {
    for (i = 0; i < vk->NameLength / sizeof(grub_uint16_t); i++)
    {
      if (i >= name_len)
      {
        overflow = true;
        break;
      }
      name[i] = vk->Name[i];
    }
    name[i] = 0;
  }
  *type = vk->Type;
  return overflow ? REG_ERR_OUT_OF_MEMORY : REG_ERR_NONE;
}

static reg_err_t
query_value_no_copy (reg_hive_t *this, HKEY key,
                     const grub_uint16_t* name, void** data,
                     grub_uint32_t* data_len, grub_uint32_t* type)  //查询值无副本 
{
  hive_t* h = _CR(this, hive_t, public);
  grub_int32_t size;
  CM_KEY_NODE* nk;
  grub_uint32_t* list;
  unsigned int namelen = reg_wcslen(name);

  // find key node
  size = -*(grub_int32_t*)((grub_uint8_t*)h->data + key);
  if (size < 0)
    return REG_ERR_FILE_NOT_FOUND;
  if ((grub_uint32_t)size < sizeof(grub_int32_t) + _offsetof(CM_KEY_NODE, Name[0]))
    return REG_ERR_BAD_ARGUMENT;

  nk = (CM_KEY_NODE*)((grub_uint8_t*)h->data + key + sizeof(grub_int32_t));
  if (nk->Signature != CM_KEY_NODE_SIGNATURE)
    return REG_ERR_BAD_ARGUMENT;

  if ((grub_uint32_t)size < sizeof(grub_int32_t)
      + _offsetof(CM_KEY_NODE, Name[0]) + nk->NameLength)
    return REG_ERR_BAD_ARGUMENT;

  if (nk->ValuesCount == 0 || nk->Values == 0xffffffff)
    return REG_ERR_FILE_NOT_FOUND;

  // go to key index
  size = -*(grub_int32_t*)((grub_uint8_t*)h->data + 0x1000 + nk->Values);
  if (size < 0)
    return REG_ERR_FILE_NOT_FOUND;

  if ((grub_uint32_t)size < sizeof(grub_int32_t) + (sizeof(grub_uint32_t) * nk->ValuesCount))
    return REG_ERR_BAD_ARGUMENT;

  list = (grub_uint32_t*)((grub_uint8_t*)h->data + 0x1000 + nk->Values + sizeof(grub_int32_t));

  // find value node
  unsigned int i;
  for (i = 0; i < nk->ValuesCount; i++)
  {
    CM_KEY_VALUE* vk;
    size = -*(grub_int32_t*)((grub_uint8_t*)h->data + 0x1000 + list[i]);

    if (size < 0)
      continue;

    if ((grub_uint32_t)size < sizeof(grub_int32_t) + _offsetof(CM_KEY_VALUE, Name[0]))
      continue;

    vk = (CM_KEY_VALUE*)((grub_uint8_t*)h->data + 0x1000 + list[i] + sizeof(grub_int32_t));
    if (vk->Signature != CM_KEY_VALUE_SIGNATURE)
      continue;
    if ((grub_uint32_t)size < sizeof(grub_int32_t)
        + _offsetof(CM_KEY_VALUE, Name[0]) + vk->NameLength)
      continue;

    if (vk->Flags & VALUE_COMP_NAME)
    {
      unsigned int j;
      char* valname = (char*)vk->Name;

      if (vk->NameLength != namelen)
        continue;

      for (j = 0; j < vk->NameLength; j++)
      {
        grub_uint16_t c1 = valname[j];
        grub_uint16_t c2 = name[j];

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

      if (vk->NameLength / sizeof(grub_uint8_t) != namelen)
        continue;
      for (j = 0; j < vk->NameLength / sizeof(grub_uint16_t); j++)
      {
        grub_uint16_t c1 = vk->Name[j];
        grub_uint16_t c2 = name[j];

        if (c1 >= 'A' && c1 <= 'Z')
          c1 = c1 - 'A' + 'a';
        if (c2 >= 'A' && c2 <= 'Z')
          c2 = c2 - 'A' + 'a';
        if (c1 != c2)
          break;
      }

      if (j != vk->NameLength / sizeof(grub_uint8_t))
        continue;
    }

    if (vk->DataLength & CM_KEY_VALUE_SPECIAL_SIZE)
    { // data stored as data offset
      grub_size_t datalen = vk->DataLength & ~CM_KEY_VALUE_SPECIAL_SIZE;
      grub_uint8_t *ptr = NULL;

      if (datalen == 4 || datalen == 2 || datalen == 1)
        ptr = (grub_uint8_t*)&vk->Data;
      else if (datalen != 0)
        return REG_ERR_BAD_ARGUMENT;
      *data = ptr;
    }
    else
    {
      size = -*(grub_int32_t*)((grub_uint8_t*)h->data + 0x1000 + vk->Data);
      if ((grub_uint32_t)size < vk->DataLength)
        return REG_ERR_BAD_ARGUMENT;

      *data = (grub_uint8_t*)h->data + 0x1000 + vk->Data + sizeof(grub_int32_t);
    }

    // FIXME - handle long "data block" values
    *data_len = vk->DataLength & ~CM_KEY_VALUE_SPECIAL_SIZE;
    *type = vk->Type;
    return REG_ERR_NONE;
  }

  return REG_ERR_FILE_NOT_FOUND;
}

static reg_err_t
query_value (reg_hive_t *this, HKEY key,
             const grub_uint16_t* name, void* data,
             grub_uint32_t* data_len, grub_uint32_t* type)  //查询值 
{
  reg_err_t errno;
  void* out;
  grub_uint32_t len;
  errno = query_value_no_copy (this, key, name, &out, &len, type);
  if (errno)
    return errno;
  if (len > *data_len)
  {
    memmove(data, out, *data_len);
    *data_len = len;
    return REG_ERR_OUT_OF_MEMORY;
  }
  memmove(data, out, len);
  *data_len = len;
  return REG_ERR_NONE;
}

static void
steal_data (reg_hive_t *this, void** data, grub_uint32_t* size)  //窃取数据 
{
  hive_t* h = _CR(this, hive_t, public);
  *data = h->data;
  *size = h->size;
  h->data = NULL;
  h->size = 0;
}

static void clear_volatile (hive_t* h, HKEY key)  //清除易挥发的
{
  grub_int32_t size;
  CM_KEY_NODE* nk;
  grub_uint16_t sig;
  unsigned int i;

  size = -*(grub_int32_t*)((grub_uint8_t*)h->data + key);

  if (size < 0)
    return;

  if ((grub_uint32_t)size < sizeof(grub_int32_t) + _offsetof(CM_KEY_NODE, Name[0]))
    return;

  nk = (CM_KEY_NODE*)((grub_uint8_t*)h->data + key + sizeof(grub_int32_t));

  if (nk->Signature != CM_KEY_NODE_SIGNATURE)
    return;

  nk->VolatileSubKeyList = 0xbaadf00d;
  nk->VolatileSubKeyCount = 0;

  if (nk->SubKeyCount == 0 || nk->SubKeyList == 0xffffffff)
    return;

  size = -*(grub_int32_t*)((grub_uint8_t*)h->data + 0x1000 + nk->SubKeyList);

  sig = *(grub_uint16_t*)((grub_uint8_t*)h->data + 0x1000 + nk->SubKeyList + sizeof(grub_int32_t));

  if (sig == CM_KEY_HASH_LEAF)
  {
    CM_KEY_FAST_INDEX* lh =
        (CM_KEY_FAST_INDEX*)((grub_uint8_t*)h->data + 0x1000
                             + nk->SubKeyList + sizeof(grub_int32_t));
    for (i = 0; i < lh->Count; i++)
    {
      clear_volatile(h, 0x1000 + lh->List[i].Cell);
    }
  }
  else if (sig == CM_KEY_LEAF)
  {
    CM_KEY_FAST_INDEX* lf =
        (CM_KEY_FAST_INDEX*)((grub_uint8_t*)h->data + 0x1000
                             + nk->SubKeyList + sizeof(grub_int32_t));

    for (i = 0; i < lf->Count; i++)
    {
      clear_volatile(h, 0x1000 + lf->List[i].Cell);
    }
  }
  else if (sig == CM_KEY_INDEX_ROOT)
  {
    CM_KEY_INDEX* ri = (CM_KEY_INDEX*)((grub_uint8_t*)h->data + 0x1000
                                       + nk->SubKeyList + sizeof(grub_int32_t));

    for (i = 0; i < ri->Count; i++)
    {
      clear_volatile(h, 0x1000 + ri->List[i]);
    }
  }
  else
  {
    printf_debug ("Unhandled registry signature 0x%x.\n", sig);
  }
}

static hive_t static_hive;

reg_err_t
open_hive (void *file, grub_size_t len, reg_hive_t **hive)  //打开蜂巢
{
  hive_t *h = &static_hive;
  memset (h, 0, sizeof (hive_t));
  h->size = len;
  h->data = file;

  if (!check_header(h))
  {
    printf_debug ("Header check failed.\n");
    return REG_ERR_BAD_ARGUMENT;
  }

  clear_volatile (h, 0x1000 + ((HBASE_BLOCK*)h->data)->RootCell);

  h->public.reg_close = close_hive;
  h->public.find_root = find_root;
  h->public.enum_keys = enum_keys;
  h->public.find_key = find_key;
  h->public.enum_values = enum_values;
  h->public.query_value = query_value;
  h->public.steal_data = steal_data;
  h->public.query_value_no_copy = query_value_no_copy;
  *hive = &h->public;

  return REG_ERR_NONE;
}

#pragma GCC diagnostic error "-Wcast-align"
