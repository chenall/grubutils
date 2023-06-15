/*
 * The C code for a grub4dos executable may have defines as follows:
 * 用于编写外部命令的函数定义。
*/

#ifndef GRUB4DOS_2015_02_15
#define GRUB4DOS_2015_02_15

#include <uefi.h>

#undef NULL
#define NULL         ((void *) 0)
#define	IMG(x)	((x) - 0x8200 + g4e_data)
#define	SYSVAR(x)	(*(unsigned long long *)((*(unsigned long long *)IMG(0x8308)) + (x<<3)))
#define	SYSVAR_2(x)	(*(unsigned long long *)(*(unsigned long long *)((*(unsigned long long *)IMG(0x8308)) + (x<<3))))
#define	SYSFUN(x)	(*(unsigned long long *)((*(unsigned long long *)IMG(0x8300)) + (x<<3)))

#define L( x ) _L ( x )
#define _L( x ) L ## x

#if defined (__GNUC__) && (__GNUC__ > 8)
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#endif


typedef __WCHAR_TYPE__     wchar_t;

typedef unsigned char      grub_uint8_t;
typedef unsigned short     grub_uint16_t;
typedef unsigned int       grub_uint32_t;
typedef unsigned long long grub_uint64_t;
typedef signed char        grub_int8_t;
typedef short              grub_int16_t;
typedef int                grub_int32_t;
typedef long long          grub_int64_t;

typedef unsigned int        grub_efi_uint32_t;
typedef char                grub_efi_boolean_t;
typedef unsigned char       grub_efi_uint8_t;
typedef unsigned long long  grub_efi_uint64_t;

typedef unsigned long long grub_disk_addr_t;
typedef unsigned long long grub_off_t;


#if defined(__i386__)
  typedef unsigned int        grub_size_t;
  typedef int                 grub_ssize_t;
#else
  typedef unsigned long long  grub_size_t;
  typedef long long           grub_ssize_t;
#endif

typedef grub_size_t   grub_addr_t;
typedef grub_size_t   grub_efi_lba_t;
typedef grub_size_t   grub_efi_status_t;
typedef grub_size_t   grub_efi_uintn_t;

/* Error codes (descriptions are in common.c) */
typedef enum
{
  ERR_NONE = 0,
  ERR_BAD_FILENAME,
  ERR_BAD_FILETYPE,
  ERR_BAD_GZIP_DATA,
  ERR_BAD_GZIP_HEADER,
  ERR_BAD_PART_TABLE,
  ERR_BAD_VERSION,
  ERR_BELOW_1MB,
  ERR_BOOT_COMMAND,
  ERR_BOOT_FAILURE,
  ERR_BOOT_FEATURES,
  ERR_DEV_FORMAT,
  ERR_DEV_VALUES,
  ERR_EXEC_FORMAT,
  ERR_FILELENGTH,
  ERR_FILE_NOT_FOUND,
  ERR_FSYS_CORRUPT,
  ERR_FSYS_MOUNT,
  ERR_GEOM,
  ERR_NEED_LX_KERNEL,
  ERR_NEED_MB_KERNEL,
  ERR_NO_DISK,
  ERR_NO_PART,
  ERR_NUMBER_PARSING,
  ERR_OUTSIDE_PART,
  ERR_READ,
  ERR_SYMLINK_LOOP,
  ERR_UNRECOGNIZED,
  ERR_WONT_FIT,
  ERR_WRITE,
  ERR_BAD_ARGUMENT,
  ERR_UNALIGNED,
  ERR_PRIVILEGED,
  ERR_DEV_NEED_INIT,
  ERR_NO_DISK_SPACE,
  ERR_NUMBER_OVERFLOW,
  ERR_DEFAULT_FILE,
  ERR_DEL_MEM_DRIVE,
  ERR_DISABLE_A20,
  ERR_DOS_BACKUP,
  ERR_ENABLE_A20,
  ERR_EXTENDED_PARTITION,
  ERR_FILENAME_FORMAT,
  ERR_HD_VOL_START_0,
  ERR_INT13_ON_HOOK,
  ERR_INT13_OFF_HOOK,
  ERR_INVALID_BOOT_CS,
  ERR_INVALID_BOOT_IP,
  ERR_INVALID_FLOPPIES,
  ERR_INVALID_HARDDRIVES,
  ERR_INVALID_HEADS,
  ERR_INVALID_LOAD_LENGTH,
  ERR_INVALID_LOAD_OFFSET,
  ERR_INVALID_LOAD_SEGMENT,
  ERR_INVALID_SECTORS,
  ERR_INVALID_SKIP_LENGTH,
  ERR_INVALID_RAM_DRIVE,
  ERR_IN_SITU_FLOPPY,
  ERR_IN_SITU_MEM,
  ERR_MD_BASE,
  ERR_NON_CONTIGUOUS,
  ERR_MANY_FRAGMENTS,
  ERR_NO_DRIVE_MAPPED,
  ERR_NO_HEADS,
  ERR_NO_SECTORS,
  ERR_PARTITION_TABLE_FULL,
  ERR_RD_BASE,
  ERR_SPECIFY_GEOM,
  ERR_SPECIFY_MEM,
  ERR_SPECIFY_RESTRICTION,
  ERR_MD5_FORMAT,
  ERR_WRITE_GZIP_FILE,
  ERR_FUNC_CALL,
  ERR_WRITE_TO_NON_MEM_DRIVE,
  ERR_INTERNAL_CHECK,
  ERR_KERNEL_WITH_PROGRAM,
  ERR_HALT,
  ERR_PARTITION_LOOP,
  ERR_NOT_ENOUGH_MEMORY,
  ERR_NO_VBE_BIOS,
  ERR_BAD_VBE_SIGNATURE,
  ERR_LOW_VBE_VERSION,
  ERR_NO_VBE_MODES,
  ERR_SET_VBE_MODE,
  ERR_SET_VGA_MODE,
  ERR_LOAD_SPLASHIMAGE,
  ERR_UNIFONT_FORMAT,
  ERR_DIVISION_BY_ZERO,

  MAX_ERR_NUM,

  /* these are for batch scripts and must be > MAX_ERR_NUM */
  ERR_BAT_GOTO,
  ERR_BAT_CALL,
  ERR_BAT_BRACE_END, 
} grub_error_t;


#define GRUB_READ 0xedde0d90
#define GRUB_WRITE 0x900ddeed
#define SKIP_LINE		0x100
#define SKIP_NONE		0
#define SKIP_WITH_TERMINATE	0x200
#define DRIVE_MAP_SIZE	8
#define PXE_DRIVE		0x21
#define INITRD_DRIVE	0x22
#define FB_DRIVE		0x23
#define SECTOR_SIZE		0x200
#define SECTOR_BITS		9
#define BIOSDISK_FLAG_BIFURCATE		0x4
#define MB_ARD_MEMORY	1
#define MB_INFO_MEM_MAP	0x00000040


#define install_partition (*(unsigned long long *)IMG(0x8208))
#define HOTKEY_FUNC (*(grub_size_t*)IMG(0x8260))    //热键函数
#define grub_timeout (*(int *)IMG(0x827c))          //倒计时
#define boot_drive (*(unsigned long long *)IMG(0x8280))
#define pxe_yip (*(unsigned int *)IMG(0x8284))
#define pxe_sip (*(unsigned int *)IMG(0x8288))
#define pxe_gip (*(unsigned int *)IMG(0x828C))
#define filesize (*(unsigned long long *)IMG(0x8290))
#define saved_partition (*(unsigned int *)IMG(0x829C))
#define saved_drive (*(unsigned int *)IMG(0x82A0))
#define no_decompression (*(unsigned int *)IMG(0x82A4))
#define part_start (*(unsigned long long *)IMG(0x82A8))
#define part_length (*(unsigned long long *)IMG(0x82B0))
#define fb_status (*(unsigned int *)IMG(0x82B8))
#define is64bit (*(unsigned int *)IMG(0x82BC))
#define cursor_state (*(unsigned int *)IMG(0x82C4))
#define ram_drive (*(unsigned int *)IMG(0x82CC))
#define rd_base (*(unsigned long long *)IMG(0x82D0))
#define rd_size (*(unsigned long long *)IMG(0x82D8))
#define addr_system_functions (*(unsigned int *)IMG(0x8300))	//地址
#define free_mem_start (*(unsigned int *)IMG(0x82F0))
#define free_mem_end (*(unsigned int *)IMG(0x82F4))
#define saved_mmap_addr (*(unsigned int *)IMG(0x82F8))
#define saved_mmap_length (*(unsigned int *)IMG(0x82FB))
#define errnum (*(grub_error_t *)IMG(0x8314))
#define current_drive (*(unsigned int *)IMG(0x8318))
#define current_partition (*(unsigned int *)IMG(0x831C))
#define filemax (*(unsigned long long *)IMG(0x8320))
#define filepos (*(unsigned long long *)IMG(0x8328))
#define debug (*(int *)IMG(0x8330))
#define current_slice (*(unsigned int *)IMG(0x8334))
#define buf_track	(*(unsigned long long *)IMG(0x8340))
#define buf_drive	(*(int *)IMG(0x8348))
#define menu_mem (*(grub_size_t*)IMG(0x8388))     //菜单地址
#define timer (*(grub_size_t *)IMG(0x8350))         //外部定时器

#define next_partition_drive		(SYSVAR(0))
#define next_partition_dest		(SYSVAR(1))
#define next_partition_partition	((int *)(SYSVAR_2(2)))
#define next_partition_type		((int *)(SYSVAR_2(3)))
#define next_partition_start		((long long *)(SYSVAR_2(4)))
#define next_partition_len		((long long *)(SYSVAR_2(5)))
#define next_partition_offset		((long long *)(SYSVAR_2(6)))
#define next_partition_entry		((int *)(SYSVAR_2(7)))
#define next_partition_ext_offset	((int *)(SYSVAR_2(8)))
#define next_partition_buf		((char *)(SYSVAR_2(9)))
#define quit_print		(SYSVAR(10))
#define image   ((struct efi_loaded_image *)(SYSVAR_2(12)))
#define filesystem_type (SYSVAR(13))
#define grub_efi_image_handle ((void *)(SYSVAR_2(14)))
#define grub_efi_system_table ((void *)(SYSVAR_2(15)))
#define buf_geom ((struct geometry *)(SYSVAR_2(16)))
#define tmp_geom ((struct geometry *)(SYSVAR_2(17)))
#define CONFIG_ENTRIES ((char *)(SYSVAR_2(18)))
#define current_term ((struct term_entry *)(SYSVAR(19)))
#define fsys_table (SYSVAR(20))
#define fsys_type (SYSVAR(21))
#define NUM_FSYS (SYSVAR(22))
#define graphics_inited (SYSVAR(23))
#define BASE_ADDR ((char *)(SYSVAR_2(24)))
#define fontx (SYSVAR(26))
#define fonty (SYSVAR(27))
#define graphics_CURSOR ((void *)(SYSVAR_2(28)))
#define menu_border ((struct border *)(SYSVAR_2(29)))
#define ADDR_RET_STR ((char *)(SYSVAR_2(31)))
/* If the variable is a string, then:  ADDR_RET_STR = var;
   If the variable is a numeric value, then:  sprintf (ADDR_RET_STR,"0x%lx",var); */
#define current_color (SYSVAR(42))
#define current_color_64bit (SYSVAR(43))
#define foreground (SYSVAR(43))
#define background (SYSVAR(44))
#define p_get_cmdline_str (SYSVAR(45))
#define splashimage_loaded (SYSVAR(46))
#define putchar_hooked (SYSVAR(47))


#define sprintf ((int (*)(char *, const char *, ...))(SYSFUN(0)))
#define printf(...) sprintf(NULL, __VA_ARGS__)
#define printf_debug(...) sprintf((char*)2, __VA_ARGS__)
#define printf_errinfo(...) sprintf((char*)3, __VA_ARGS__)
#define printf_warning(...) sprintf((char*)2, __VA_ARGS__)
#define putstr ((void (*)(const char *))(SYSFUN(1)))
#define putchar ((void (*)(int))(SYSFUN(2)))
#define get_cmdline_obsolete ((int (*)(struct get_cmdline_arg cmdline))(SYSFUN(3)))
#define getxy ((int (*)(void))(SYSFUN(4)))
#define gotoxy ((void (*)(int, int))(SYSFUN(5)))
#define cls ((void (*)(void))(SYSFUN(6)))
#define wee_skip_to ((char *(*)(char *, int))(SYSFUN(7)))
#define nul_terminate ((int (*)(char *))(SYSFUN(8)))
#define safe_parse_maxint_with_suffix ((int (*)(char **str_ptr, unsigned long long *myint_ptr, int unitshift))(SYSFUN(9)))
#define safe_parse_maxint(str_ptr, myint_ptr) safe_parse_maxint_with_suffix(str_ptr, myint_ptr, 0)
#define substring ((int (*)(const char *s1, const char *s2, int case_insensitive))(SYSFUN(10)))
#define strstr ((char *(*)(const char *s1, const char *s2))(SYSFUN(11)))
#define strlen ((int (*)(const char *str))(SYSFUN(12)))
#define strtok ((char *(*)(char *s, const char *delim))(SYSFUN(13)))
#define strncat ((int (*)(char *s1, const char *s2, int n))(SYSFUN(14)))
#define strcpy ((char *(*)(char *dest, const char *src))(SYSFUN(16)))
#define efidisk_readwrite ((int *(*)(int drive, grub_disk_addr_t sector, grub_size_t size, char *buf, int read_write))(SYSFUN(17)))
#define blockio_read_write ((grub_size_t *(*)(efi_block_io_t *this, efi_uint32_t media_id, efi_lba_t lba, efi_uintn_t len, void *buf, int read_write))(SYSFUN(18)))
#define getkey ((int (*)(void))(SYSFUN(19)))
#define checkkey ((int (*)(void))(SYSFUN(20)))
#define memcmp ((int (*)(const char *s1, const char *s2, int n))(SYSFUN(22)))
#define memmove ((void *(*)(void *to, const void *from, int len))(SYSFUN(23)))
#define memset ((void *(*)(void *start, int c, int len))(SYSFUN(24)))
#define get_partition_info ((void *(*)(int drive, int partition))(SYSFUN(25)))
#define open ((int (*)(char *))(SYSFUN(26)))
#define read ((unsigned long long (*)(unsigned long long, unsigned long long, unsigned int))(SYSFUN(27)))
#define close ((void (*)(void))(SYSFUN(28)))
#define get_device_by_drive ((struct grub_disk_data *(*)(unsigned int drive, unsigned int map))(SYSFUN(29)))
#define disk_read_hook ((void(**)(unsigned long long buf, unsigned long long len, unsigned int write))(SYSFUN(31)))
#define devread ((int (*)(unsigned long long sector, unsigned long long byte_offset, unsigned long long byte_len, unsigned long long buf, unsigned int write))(SYSFUN(32)))
#define devwrite ((int (*)devwrite (unsigned long long sector, unsigned long long sector_len, unsigned long long buf))(SYSFUN(33)))
#define next_partition ((int (*)(void))(SYSFUN(34)))
#define open_device ((int (*)(void))(SYSFUN(35)))
#define real_open_partition ((int (*)(int))(SYSFUN(36)))
#define set_device ((char *(*)(char *))(SYSFUN(37)))
#define run_line ((int (*)(char *heap, int flags))(SYSFUN(38)))
#define vdisk_install ((efi_status_t (*)(int drive, int partition))(SYSFUN(39)))
#define parse_string ((int (*)(char *))(SYSFUN(41)))
#define hexdump ((void (*)(unsigned long long,char*,int))(SYSFUN(42)))
#define skip_to ((char *(*)(int after_equal, char *cmdline))(SYSFUN(43)))
#define builtin_cmd ((int (*)(char *cmd, char *arg, int flags))(SYSFUN(44)))
#define get_datetime ((void (*)(struct grub_datetime *datetime))(SYSFUN(45)))
#define find_command ((struct builtin *(*)(char *))(SYSFUN(46)))
#define memalign ((void * (*)(grub_size_t, grub_size_t))(SYSFUN(48)))
#define zalloc ((void *(*)(unsigned int))(SYSFUN(49)))
#define malloc ((void *(*)(unsigned int))(SYSFUN(50)))
#define free ((void (*)(void *ptr))(SYSFUN(51)))
#define realmode_run ((int (*)(int regs_ptr))(SYSFUN(53)))
#define grub_dir ((int (*)(char *))(SYSFUN(61)))
#define print_a_completion ((void (*)(char *, int))(SYSFUN(62)))
#define print_completions ((int (*)(int, int))(SYSFUN(63)))
#define lba_to_chs ((void (*)(unsigned int lba, unsigned int *cl, unsigned int *ch, unsigned int *dh))(SYSFUN(64)))
#define probe_bpb ((int (*)(struct master_and_dos_boot_sector *BS))(SYSFUN(65)))
#define probe_mbr ((int (*)(struct master_and_dos_boot_sector *BS, unsigned int start_sector1, unsigned int sector_count1, unsigned int part_start1))(SYSFUN(66)))
#define unicode_to_utf8 ((unsigned int (*)(unsigned short *, unsigned char *, unsigned int))(SYSFUN(67)))
#define rawread ((int (*)(unsigned int, unsigned long long, unsigned int, unsigned long long, unsigned long long, unsigned int))(SYSFUN(68,int)))
#define rawwrite ((int (*)(unsigned int, unsigned int, char *))(SYSFUN(69)))
#define setcursor ((int (*)(int))(SYSFUN(70)))
#define tolower ((int (*)(int))(SYSFUN(71)))
#define isspace ((int (*)(int))(SYSFUN(72)))
#define sleep ((unsigned int (*)(unsigned int))(SYSFUN(73)))
#define mem64 ((int (*)(int, unsigned long long, unsigned long long, unsigned long long))(SYSFUN(74)))
#define envi_cmd ((int (*)(const char*, char *const, int))(SYSFUN(75)))

/* strncmpx 增强型字符串比较函数 by chenall 2011-12-13
	int strncmpx(const char *s1,const char *s2, unsigned int n, int case_insensitive)
	比较两个字符串s1,s2.长度: n,
	如果n等于0，则只比较到字符串结束位置。否则比较指定长度n.不管字符串是否结束。
	如果case_insensitive非0，比较字母时不区分大小写。
	可以替换strcmp/memcmp等字符串比较函数
	返回值: s1-s2
		当s1<s2时，返回值<0
		当s1=s2时，返回值=0
		当s1>s2时，返回值>0
*/
#define strncmpx ((int (*)(const char*, const char*, unsigned int,int))(SYSFUN(76)))
/*
	以下几个字符串比较都是使用strncmpx来实现
*/
//比较字符串s1和s2。
#define strcmp(s1,s2) strncmpx(s1,s2,0,0)
//比较字符串s1和s2。不区分大小写
#define stricmp(s1,s2) strncmpx(s1,s2,0,1)
#define strcmpi stricmp
//比较字符串s1和s2的前n个字符。
#define strncmp(s1,s2,n) strncmpx(s1,s2,n,0)
//比较字符串s1和s2的前n个字符但不区分大小写。
#define strnicmp(s1,s2,n) strncmpx(s1,s2,n,1)
#define strncmpi strnicmp

#define rectangle ((void (*)(int left, int top, int length, int width, int line))(SYSFUN(77)))
#define get_cmdline ((int (*)(void))(SYSFUN(78)))

#define RAW_ADDR(x) (x)

struct get_cmdline_arg
{
	char *cmdline;
	char *prompt;
	int maxlen;
	int echo_char;
	int readline;
} __attribute__ ((packed));

struct geometry
{
  /* The total number of sectors */
  unsigned long long total_sectors;
  /* Device sector size */
  unsigned int sector_size;
  /* Power of sector size 2 */
  unsigned int log2_sector_size;
};

/* fsys.h */
struct fsys_entry
{
  char *name;
  int (*mount_func) (void);
  unsigned int (*read_func) (unsigned long long buf, unsigned long long len, unsigned int write);
  int (*dir_func) (char *dirname);
  void (*close_func) (void);
  unsigned int (*embed_func) (unsigned int *start_sector, unsigned int needed_sectors);
};
/* fsys.h */

/* shared.h */
struct master_and_dos_boot_sector {
/* 00 */ char dummy1[0x0b]; /* at offset 0, normally there is a short JMP instuction(opcode is 0xEB) */
/* 0B */ unsigned short bytes_per_sector __attribute__ ((packed));/* seems always to be 512, so we just use 512 */
/* 0D */ unsigned char sectors_per_cluster;/* non-zero, the power of 2, i.e., 2^n */
/* 0E */ unsigned short reserved_sectors __attribute__ ((packed));/* FAT=non-zero, NTFS=0? */
/* 10 */ unsigned char number_of_fats;/* NTFS=0; FAT=1 or 2  */
/* 11 */ unsigned short root_dir_entries __attribute__ ((packed));/* FAT32=0, NTFS=0, FAT12/16=non-zero */
/* 13 */ unsigned short total_sectors_short __attribute__ ((packed));/* FAT32=0, NTFS=0, FAT12/16=any */
/* 15 */ unsigned char media_descriptor;/* range from 0xf0 to 0xff */
/* 16 */ unsigned short sectors_per_fat __attribute__ ((packed));/* FAT32=0, NTFS=0, FAT12/16=non-zero */
/* 18 */ unsigned short sectors_per_track __attribute__ ((packed));/* range from 1 to 63 */
/* 1A */ unsigned short total_heads __attribute__ ((packed));/* range from 1 to 256 */
/* 1C */ unsigned int hidden_sectors __attribute__ ((packed));/* any value */
/* 20 */ unsigned int total_sectors_long __attribute__ ((packed));/* FAT32=non-zero, NTFS=0, FAT12/16=any */
/* 24 */ unsigned int sectors_per_fat32 __attribute__ ((packed));/* FAT32=non-zero, NTFS=any, FAT12/16=any */
/* 28 */ unsigned long long total_sectors_long_long __attribute__ ((packed));/* NTFS=non-zero, FAT12/16/32=any */
/* 30 */ char dummy2[0x18e];

    /* Partition Table, starting at offset 0x1BE */
/* 1BE */ struct {
	/* +00 */ unsigned char boot_indicator;
	/* +01 */ unsigned char start_head;
	/* +02 */ unsigned short start_sector_cylinder __attribute__ ((packed));
	/* +04 */ unsigned char system_indicator;
	/* +05 */ unsigned char end_head;
	/* +06 */ unsigned short end_sector_cylinder __attribute__ ((packed));
	/* +08 */ unsigned int start_lba __attribute__ ((packed));
	/* +0C */ unsigned int total_sectors __attribute__ ((packed));
	/* +10 */
    } P[4];
/* 1FE */ unsigned short boot_signature __attribute__ ((packed));/* 0xAA55 */
  };

struct drive_map_slot
{
	/* Remember to update DRIVE_MAP_SLOT_SIZE once this is modified.
	 * The struct size must be a multiple of 4.
	 */
	unsigned char from_drive;
	unsigned char to_drive;						/* 0xFF indicates a memdrive */
	unsigned char max_head;
  
	unsigned char :7;
	unsigned char read_only:1;          //位7
  
	unsigned short to_log2_sector:4;    //位0-3
	unsigned short from_log2_sector:4;  //位4-7
	unsigned short :2;
	unsigned short fragment:1;          //位10
	unsigned short :2;
	unsigned short from_cdrom:1;        //位13
	unsigned short to_cdrom:1;          //位14
	unsigned short :1;
  
	unsigned char to_head;
	unsigned char to_sector;
	unsigned long long start_sector;
	unsigned long long sector_count;
} __attribute__ ((packed));


 /* shared.h */

typedef enum
{
	COLOR_STATE_STANDARD,
	/* represents the user defined colors for normal text */
	COLOR_STATE_NORMAL,
	/* represents the user defined colors for highlighted text */
	COLOR_STATE_HIGHLIGHT,
	/* represents the user defined colors for help text */
	COLOR_STATE_HELPTEXT,
	/* represents the user defined colors for heading line */
	COLOR_STATE_HEADING
} color_state;

struct term_entry
{
  /* The name of a terminal.  */
  const char *name;
  /* The feature flags defined above.  */
  unsigned int flags;
  unsigned short chars_per_line;
  /* Default for maximum number of lines if not specified */
  unsigned short max_lines;
  /* Put a character.  */
  void (*PUTCHAR) (int c);
  /* Check if any input character is available.  */
  int (*CHECKKEY) (void);
  /* Get a character.  */
  int (*GETKEY) (void);
  /* Get the cursor position. The return value is ((X << 8) | Y).  */
  int (*GETXY) (void);
  /* Go to the position (X, Y).  */
  void (*GOTOXY) (int x, int y);
  /* Clear the screen.  */
  void (*CLS) (void);
  /* Set the current color to be used */
  void (*SETCOLORSTATE) (color_state state);
  /* Set the normal color and the highlight color. The format of each
     color is VGA's.  */
  void (*SETCOLOR) (unsigned int state,unsigned long long color[]);
  /* Turn on/off the cursor.  */
  int (*SETCURSOR) (int on);

  /* function to start a terminal */
  int (*STARTUP) (void);
  /* function to use to shutdown a terminal */
  void (*SHUTDOWN) (void);
};

/* The table for a builtin.  */
struct builtin
{
  /* The command name.  */
  char *name;
  /* The callback function.  */
  int (*func) (char *, int);
  /* The combination of the flags defined above.  */
  int flags;
  /* The short version of the documentation.  */
  char *short_doc;
  /* The int version of the documentation.  */
  char *long_doc;
};

struct grub_datetime
{
  unsigned short year;
  unsigned char month;
  unsigned char day;
  unsigned char hour;
  unsigned char minute;
  unsigned char second;
	unsigned char pad1;
};

//#include <stdarg.h> 可变参数定义
#ifndef _VA_LIST
typedef __builtin_va_list va_list;
#define _VA_LIST
#endif
#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_end(ap)          __builtin_va_end(ap)
#define va_arg(ap, type)    __builtin_va_arg(ap, type)
 
/* GCC always defines __va_copy, but does not define va_copy unless in c99 mode
 * or -ansi is not specified, since it was not part of C90.
 */
#define __va_copy(d,s) __builtin_va_copy(d,s)
#ifndef __GNUC_VA_LIST
#define __GNUC_VA_LIST 1
typedef __builtin_va_list __gnuc_va_list;
#endif

typedef void *efi_handle_t;

struct grub_disk_data  //efi磁盘数据	(软盘,硬盘,光盘)    注：efidiskinfo.c使用
{
  efi_handle_t device_handle;               //句柄
  efi_block_io_t *block_io;                 //块输入输出      修订,媒体,重置,读块,写块,清除块
  struct grub_disk_data *next;              //下一个
  unsigned char drive;                      //from驱动器
  unsigned char to_drive;                   //to驱动器                  原生磁盘为0
  unsigned char from_log2_sector;           //from每扇区字节2的幂
  unsigned char to_log2_sector;             //to每扇区字节2的幂         原生磁盘为0
  unsigned long long start_sector;          //起始扇区                  原生磁盘为0  from在to的起始扇区  每扇区字节=(1 << to_log2_sector)
  unsigned long long sector_count;          //扇区计数                  原生磁盘为0  from在to的扇区数    每扇区字节=(1 << to_log2_sector)
  unsigned long long total_sectors;         //总扇区数                  from驱动器的总扇区数  每扇区字节=(1 << from_log2_sector)
  unsigned char disk_signature[16];         //磁盘签名                  软盘/光盘或略  启动wim/vhd需要  mbr类型同分区签名,gpt类型则异样  原生磁盘为0
  unsigned short to_block_size;             //to块尺寸                  原生磁盘为0
  unsigned char partmap_type;               //硬盘分区类型    1/2=MBR/GPT
  unsigned char fragment;                   //碎片
  unsigned char read_only;                  //只读
  unsigned char disk_type;                  //磁盘类型        0/1/2=光盘/硬盘/软盘
  unsigned short fill;                      //填充
  void *vdisk;                              //虚拟磁盘指针
}  __attribute__ ((packed));

struct grub_part_data  //efi分区数据	(硬盘)    注：loaderNT.c使用
{
	struct grub_part_data *next;  				  //下一个
	unsigned char	drive;									  //驱动器
	unsigned char	partition_type;					  //MBR分区ID         EE是gpt分区类型     光盘:
	unsigned char	partition_activity_flag;  //MBR分区活动标志   80活动              光盘:
	unsigned char partition_entry;				  //分区入口                              光盘: 启动目录确认入口   
	unsigned int partition_ext_offset;		  //扩展分区偏移                          光盘: 启动目录扇区地址
	unsigned int partition;							    //当前分区                              光盘: ffff
	unsigned long long partition_offset;	  //分区偏移
	unsigned long long partition_start;		  //分区起始扇区                          光盘: 引导镜像是硬盘时，分区起始扇区
	unsigned long long partition_size;			//分区扇区尺寸                          光盘: 引导镜像是硬盘时，分区扇区尺寸
	unsigned char partition_signature[16];  //分区签名                              光盘: 
	unsigned int boot_start;                //                                      光盘: 引导镜像在光盘的起始扇区(1扇区=2048字节)
	unsigned int boot_size;                 //                                      光盘: 引导镜像的扇区数(1扇区=512字节)
	efi_handle_t part_handle;               //句柄
	unsigned char partition_number;         //入口号           未使用
	unsigned char partition_boot;           //启动分区         /efi/boot/bootx64.efi文件所在分区
} __attribute__ ((packed));

#endif
