#include <grub4dos.h>
#include <uefi.h>
#include <bcd.h>
#include <reg.h>

//在此定义静态变量、结构。不能包含全局变量，否则编译可以通过，但是不能正确执行。
//不能在main前定义子程序，在main后可以定义子程序。

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
//至少需要一个UTF-8字节才能有一个UTF-16字。
//至少需要三个UTF-8字节才能有两个UTF-16字(代理项对)。

#define GRUB_MAX_UTF16_PER_UTF8 1
#define GRUB_MAX_UTF8_PER_CODEPOINT 4

#define GRUB_UCS2_LIMIT 0x10000
#define GRUB_UTF16_UPPER_SURROGATE(code) \
  (0xD800 | ((((code) - GRUB_UCS2_LIMIT) >> 10) & 0x3ff))
#define GRUB_UTF16_LOWER_SURROGATE(code) \
  (0xDC00 | (((code) - GRUB_UCS2_LIMIT) & 0x3ff))


static grub_size_t g4e_data = 0;
static void get_G4E_image(void);
static void bcd_patch_path (void);
char temp[256];
char tmp[64];
grub_uint16_t path16[256];
static efi_system_table_t *st;


/* 这是必需的，请参阅grubprog.h中的注释 */
#include <grubprog.h>
/* 请勿在此处插入任何其他asm行 */
//不能包含子程序，不能包含全局变量，否则编译可以通过，但是不能正确执行。


//本程序用于修改网起bcd文件中的wim启动镜像文件名。bcd位于"/boot/"
static int main(char *arg,int flags);
static int main(char *arg,int flags)
{
  int i = 1, j;
  char *filename = temp;

  get_G4E_image();
  if (! g4e_data)
    return 0;

  for (i = 0, j = 0; i < strlen (arg); i++,j++)
  {
    if (arg[i] == '\\' && arg[i+1] == ' ')  //处理arg中字符串中的空格
    {
      filename[j] = ' ';
      i++;
    }
    else if (arg[i] == '/') //处理arg中的分隔符‘/'，转'\'
    {
      filename[j] = '\\';
    }
    else
      filename[j] = arg[i];
  }
  filename[j] = 0;

  open ("/boot/bcd");       //打开bcd文件
  bcd_patch_path ();        //修改bcd中的启动文件名
  if (debug >= 3)
    getkey();
  tftp_write ("/boot/bcd"); //保存bcd
  close ();                 //关闭bcd文件

  st = grub_efi_system_table;
  st->boot_services->stall (500000); //延迟500毫秒，避免启动wim文件失败

  return 1;
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
  
//处理UTF8序列中的一个字符。在开始设置*code=0，*count=0。
//失败时返回0，成功时返回1。count保存尾随字节数。
static inline int
grub_utf8_process (uint8_t c, uint32_t *code, int *count);
static inline int
grub_utf8_process (uint8_t c, uint32_t *code, int *count)
{
  if (*count)
  {
    if ((c & GRUB_UINT8_2_LEADINGBITS) != GRUB_UINT8_1_LEADINGBIT)
    {
      *count = 0;
      //无效的
      return 0;
    }
    else
    {
      *code <<= 6;
      *code |= (c & GRUB_UINT8_6_TRAILINGBITS);
      (*count)--;
      //过长的
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
    //过长的
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

//将长度最多为SRCSIZE字节的UTF-8字符串（可能以null结尾）转换为UTF-16字符串。
//返回转换的字符数。DEST必须至少能够容纳DESTSIZE字符。如果发现无效序列，则返回-1。
//如果SRCEND不为NULL，则*SRCEND设置为SRC中使用的最后一个字节之后的下一个字节。
static inline grub_size_t
grub_utf8_to_utf16 (grub_uint16_t *dest, grub_size_t destsize,
                    const grub_uint8_t *src, grub_size_t srcsize,
                    const grub_uint8_t **srcend);
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
    if (srcsize != (grub_uint32_t)-1)
      srcsize--;
    if (!grub_utf8_process (*src++, &code, &count))
    {
      code = '?';
      count = 0;
      //字符c可能有效，不要回收它
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

static void bcd_print_hex (const void *data, grub_size_t len);
static void
bcd_print_hex (const void *data, grub_size_t len) //打印十六进制
{
  const grub_uint8_t *p = data;
  grub_uint32_t i;
  for (i = 0; i < len; i++)
  {
    if (p[i] == '\0')
      continue;
    else
      printf_debug ("%c", p[i]);
  }
}

static void
bcd_replace_hex (const grub_uint64_t search, const void *replace, grub_uint32_t replace_len, int count);
static void
bcd_replace_hex (const grub_uint64_t search, const void *replace, grub_uint32_t replace_len, int count) //bcd替换十六进制数据 
{
  char *p = (char *)efi_pxe_buf;
  grub_uint32_t offset;
  int cnt = 0;

  for (offset = 0; offset < filemax; offset++)
  {
    if (*(grub_uint64_t *)(p + offset) == search) //wim文件中有3处相符，但是第一处无用
    {
      if (*(char *)(p + offset + 0x40) == '\\')     //找到真正需要修改之处
      {
        offset += 0x40;
        cnt++;
        printf_debug ("0x%08x ", offset);
        bcd_print_hex (replace, replace_len); //打印十六进制
        printf_debug ("\n");
        memmove (p + offset, replace, replace_len);
        printf_debug ("...patched BCD at %x len %x\n", offset, replace_len);
        if (count && cnt == count)
          break;
      }
    }
  }
}

static void
bcd_patch_path (void)  //bcd修补路径
{
  const grub_uint64_t search = 0x48;
  char *p = temp;
  grub_uint32_t len;

  len = 2 * (strlen (p) + 1);  //wim:  /路径/文件名
  grub_utf8_to_utf16 (path16, len, p, -1, NULL);  //utf8转换为utf16

  bcd_replace_hex (search, path16, len, 0);  //bcd替换十六进制数据
}
