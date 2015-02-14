/* shared.h - definitions used in all GRUB-specific code */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004  Free Software Foundation, Inc.
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

/*
 *  Generic defines to use anywhere
 */

#ifndef GRUB_SHARED_HEADER
#define GRUB_SHARED_HEADER	1

/* Define if edata is defined */
#define HAVE_EDATA_SYMBOL 1

/* Define if end is defined */
#define HAVE_END_SYMBOL 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define if _edata is defined */
#define HAVE_USCORE_EDATA_SYMBOL 1

/* Define if end is defined */
#define HAVE_USCORE_END_SYMBOL 1

/* Define if _start is defined */
#define HAVE_USCORE_START_SYMBOL 1

/* Define if __bss_start is defined */
#define HAVE_USCORE_USCORE_BSS_START_SYMBOL 1

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Add an underscore to a C symbol in assembler code if needed. */
#ifdef HAVE_ASM_USCORE
# define EXT_C(sym) _ ## sym
#else
# define EXT_C(sym) sym
#endif

#define NO_DECOMPRESSION
#define BIOSDISK_FLAG_LBA_EXTENSION	0x1
//#include "extra.h"

/* 512-byte scratch area */
#define SCRATCHADDR  0x3fe00
#define SCRATCHSEG   0x3fe0

#define BUFFERLEN   0x8000
#define BUFFERADDR  0x30000
#define BUFFERSEG   0x3000

#define BOOT_PART_TABLE	0x07be

/*
 *  BIOS disk defines
 */
#define BIOSDISK_READ			0x0
#define BIOSDISK_WRITE			0x1

#define MAX_CMDLINE		1600	/* 0x640 */

#define FSYS_BUF		0x3E0000
#define FSYS_BUFLEN		0x008000

#define PART_TABLE_BUF		0x3E8000
#define PART_TABLE_BUFLEN	0x001000

#define PART_TABLE_TMPBUF	0x3E9000
#define PART_TABLE_TMPBUFLEN	0x000200

#define CMDLINE_BUF		0x3E9200
#define CMDLINE_BUFLEN		0x000640

#define COMPLETION_BUF		0x3E9840
#define COMPLETION_BUFLEN	0x000640

#define UNIQUE_BUF		0x3E9E80
#define UNIQUE_BUFLEN		0x000640

#define HISTORY_BUF		0x3EA4C0
#define HISTORY_BUFLEN		0x001F40

#define PASSWORD_BUF		0x3EC400
#define PASSWORD_BUFLEN		0x000200

#define DEFAULT_FILE_BUF	0x3EC600
#define DEFAULT_FILE_BUFLEN	0x000060

/*
 *  extended chainloader code address for switching to real mode
 */

#define HMA_ADDR		0x2B0000

#define SECTOR_SIZE		0x200
#define SECTOR_BITS		9

/*
 *  defines for use when switching between real and protected mode
 */

#define PROT_MODE_CSEG	40 /*0x8*/
#define PROT_MODE_DSEG  0x10
#define PSEUDO_RM_CSEG	0x18
#define PSEUDO_RM_DSEG	8 /*0x20*/
#define LOWMEM_DATASEG	0x51E

#define CR0_PE   0x00000001UL
#define CR0_WP   0x00010000UL
#define CR0_PG   0x80000000UL
#define CR4_PSE  0x00000010UL
#define CR4_PAE  0x00000020UL
#define CR4_PGE  0x00000080UL
#define PML4E_P  0x0000000000000001ULL
#define PDPTE_P  0x0000000000000001ULL
#define PDE_P    0x0000000000000001ULL
#define PDE_RW   0x0000000000000002ULL
#define PDE_US   0x0000000000000004ULL
#define PDE_PS   0x0000000000000080ULL
#define PDE_G    0x0000000000000100ULL
#define MSR_IA32_EFER 0xC0000080UL
#define IA32_EFER_LME 0x00000100ULL

/*
 * Assembly code defines
 *
 * "EXT_C" is assumed to be defined in the Makefile by the configure
 *   command.
 */

#define ENTRY(x) .globl EXT_C(x) ; EXT_C(x):
#define VARIABLE(x) ENTRY(x)


#define K_RDWR	0x60	/* keyboard data & cmds (read/write) */
#define K_STATUS	0x64	/* keyboard status */
#define K_CMD		0x64	/* keybd ctlr command (write-only) */

#define K_OBUF_FUL	0x01	/* output buffer full */
#define K_IBUF_FUL	0x02	/* input buffer full */

#define KC_CMD_WIN	0xd0	/* read  output port */
#define KC_CMD_WOUT	0xd1	/* write output port */
#define KB_OUTPUT_MASK  0xdd	/* enable output buffer full interrupt
				   enable data line
				   enable clock line */
#define KB_A20_ENABLE   0x02

#define KEY_IC		0x5200	/* insert char */

/* Remap some libc-API-compatible function names so that we prevent
   circularararity. */
#ifndef WITHOUT_LIBC_STUBS
#define memmove grub_memmove
#define memcpy grub_memmove	/* we don't need a separate memcpy */
#define memset grub_memset
#define isspace grub_isspace
#define printf grub_printf
#define sprintf grub_sprintf
#undef putchar
#define putchar grub_putchar
#define strncat grub_strncat
#define strstr grub_strstr
#define memcmp grub_memcmp
#define strcmp grub_strcmp
#define tolower grub_tolower
#define strlen grub_strlen
#define strcpy grub_strcpy
#endif /* WITHOUT_LIBC_STUBS */

#define PXE_TFTP_MODE	1
#define PXE_FAST_READ	1

/* see typedef gfx_data_t below */

#define DRIVE_MAP_SIZE		8
#define DRIVE_MAP_SLOT_SIZE	24

#define PC_SLICE_TYPE_NONE		0
#define PC_SLICE_TYPE_HIDDEN_FLAG	0x10

#define NUM_FSYS 3

/* Error codes */
#define ERR_NONE			0
#define ERR_BAD_FILENAME		1
#define ERR_BAD_FILETYPE		2
#define ERR_BAD_GZIP_DATA		3
#define ERR_BAD_GZIP_HEADER		4
#define ERR_BAD_PART_TABLE		5
#define ERR_BAD_VERSION			6
#define ERR_BELOW_1MB			7
#define ERR_BOOT_COMMAND		8
#define ERR_BOOT_FAILURE		9
#define ERR_BOOT_FEATURES		10
#define ERR_DEV_FORMAT			11
#define ERR_DEV_VALUES			12
#define ERR_EXEC_FORMAT			13
#define ERR_FILELENGTH			14
#define ERR_FILE_NOT_FOUND		15
#define ERR_FSYS_CORRUPT		16
#define ERR_FSYS_MOUNT			17
#define ERR_GEOM			18
#define ERR_NEED_LX_KERNEL		19
#define ERR_NEED_MB_KERNEL		20
#define ERR_NO_DISK			21
#define ERR_NO_PART			22
#define ERR_NUMBER_PARSING		23
#define ERR_OUTSIDE_PART		24
#define ERR_READ			25
#define ERR_SYMLINK_LOOP		26
#define ERR_UNRECOGNIZED		27
#define ERR_WONT_FIT			28
#define ERR_WRITE			29
#define ERR_BAD_ARGUMENT		30
#define ERR_UNALIGNED			31
#define ERR_PRIVILEGED			32
#define ERR_DEV_NEED_INIT		33
#define ERR_NO_DISK_SPACE		34
#define ERR_NUMBER_OVERFLOW		35

#define ERR_DEFAULT_FILE		36
#define ERR_DEL_MEM_DRIVE		37
#define ERR_DISABLE_A20			38
#define ERR_DOS_BACKUP			39
#define ERR_ENABLE_A20			40
#define ERR_EXTENDED_PARTITION		41
#define ERR_FILENAME_FORMAT		42
#define ERR_HD_VOL_START_0		43
#define ERR_INT13_ON_HOOK		44
#define ERR_INT13_OFF_HOOK		45
#define ERR_INVALID_BOOT_CS		46
#define ERR_INVALID_BOOT_IP		47
#define ERR_INVALID_FLOPPIES		48
#define ERR_INVALID_HARDDRIVES		49
#define ERR_INVALID_HEADS		50
#define ERR_INVALID_LOAD_LENGTH		51
#define ERR_INVALID_LOAD_OFFSET		52
#define ERR_INVALID_LOAD_SEGMENT	53
#define ERR_INVALID_SECTORS		54
#define ERR_INVALID_SKIP_LENGTH		55
#define ERR_INVALID_RAM_DRIVE		56
#define ERR_IN_SITU_FLOPPY		57
#define ERR_IN_SITU_MEM			58
#define ERR_MD_BASE			59
#define ERR_NON_CONTIGUOUS		60
#define ERR_NO_DRIVE_MAPPED		61
#define ERR_NO_HEADS			62
#define ERR_NO_SECTORS			63
#define ERR_PARTITION_TABLE_FULL	64
#define ERR_RD_BASE			65
#define ERR_SPECIFY_GEOM		66
#define ERR_SPECIFY_MEM			67
#define ERR_SPECIFY_RESTRICTION		68
#define ERR_MD5_FORMAT			69
#define ERR_WRITE_GZIP_FILE		70
#define ERR_FUNC_CALL			71
#define ERR_INTERNAL_CHECK		72
#define ERR_KERNEL_WITH_PROGRAM		73
#define ERR_HALT			74
#define ERR_PARTITION_LOOP		75

#define MAX_ERR_NUM			76

#ifndef ASM_FILE
/*
 *  Below this should be ONLY defines and other constructs for C code.
 */

#undef NULL
#define NULL         ((void *) 0)

#define IS_PC_SLICE_TYPE_EXTENDED(type) \
  ({ int _type = (type) & ~PC_SLICE_TYPE_HIDDEN_FLAG; \
     _type == 5 || _type == 0xf || _type == 0x85; })

int fat_mount (void);
unsigned long fat_read (unsigned long long buf, unsigned long long len, unsigned long write);
int fat_dir (char *dirname);

int ntfs_mount (void);
unsigned long ntfs_read (unsigned long long buf, unsigned long long len, unsigned long write);
int ntfs_dir (char *dirname);

int ext2fs_mount (void);
unsigned long ext2fs_read (unsigned long long buf, unsigned long long len, unsigned long write);
int ext2fs_dir (char *dirname);

struct fsys_entry
{
  char *name;
  int (*mount_func) (void);
  unsigned long (*read_func) (unsigned long long buf, unsigned long long len, unsigned long write);
  int (*dir_func) (char *dirname);
//  void (*close_func) (void);
//  unsigned long (*embed_func) (unsigned long *start_sector, unsigned long needed_sectors);
};

extern int print_possibilities;

extern unsigned long long fsmax;
extern struct fsys_entry fsys_table[NUM_FSYS + 1];

struct part_info {
	unsigned long part_start_relative;
	unsigned long part_len;
	unsigned char boot_indicator;
	unsigned char part_type;
	unsigned short part_num;
	unsigned long part_start_offset;
};

extern unsigned long boot_drive;
#define boot_drive (*(unsigned long *)((char *)(&boot_drive) - 0x300000))

extern unsigned long install_partition;
#define install_partition (*(unsigned long *)((char *)(&install_partition) - 0x300000))

extern unsigned long free_mem_start;
#define free_mem_start (*(unsigned long *)((char *)(&free_mem_start) - 0x300000))
extern unsigned long free_mem_end;
#define free_mem_end (*(unsigned long *)((char *)(&free_mem_end) - 0x300000))

struct mem_alloc_array
{
  unsigned long addr;
  unsigned long pid;
};

extern struct mem_alloc_array *mem_alloc_array_start;
#define mem_alloc_array_start (*(struct mem_alloc_array **)((char *)(&mem_alloc_array_start) - 0x300000))
extern unsigned long mem_alloc_array_end;
#define mem_alloc_array_end (*(unsigned long *)((char *)(&mem_alloc_array_end) - 0x300000))

/* instrumentation variables */
extern void (*disk_read_hook) (unsigned long long, unsigned long, unsigned long);
extern void (*disk_read_func) (unsigned long long, unsigned long, unsigned long);

extern unsigned long current_drive;
#define current_drive (*(unsigned long *)((char *)(&current_drive) - 0x300000))
extern unsigned long current_partition;
#define current_partition (*(unsigned long *)((char *)(&current_partition) - 0x300000))

extern int fsys_type;

extern inline unsigned long log2_tmp (unsigned long word);
extern unsigned long unicode_to_utf8 (unsigned short *filename, unsigned char *utf8, unsigned long n);

extern unsigned long long part_start;
#define part_start (*(unsigned long long *)((char *)(&part_start) - 0x300000))
extern unsigned long long part_length;
#define part_length (*(unsigned long long *)((char *)(&part_length) - 0x300000))

extern unsigned long current_slice;
#define current_slice (*(unsigned long *)((char *)(&current_slice) - 0x300000))

extern unsigned long buf_drive;
extern unsigned long long buf_track;

/* these are the current file position and maximum file position */
extern unsigned long long filepos;
#define filepos (*(unsigned long long *)((char *)(&filepos) - 0x300000))
extern unsigned long long filemax;
#define filemax (*(unsigned long long *)((char *)(&filemax) - 0x300000))
extern unsigned long long filesize;
#define filesize (*(unsigned long long *)((char *)(&filesize) - 0x300000))

/*
 *  Common BIOS/boot data.
 */

extern unsigned long saved_drive;
#define saved_drive (*(unsigned long *)((char *)(&saved_drive) - 0x300000))
extern unsigned long saved_partition;
#define saved_partition (*(unsigned long *)((char *)(&saved_partition) - 0x300000))
extern char saved_dir[256];
extern unsigned long ram_drive;
#define ram_drive (*(unsigned long *)((char *)(&ram_drive) - 0x300000))
extern unsigned long long rd_base;
#define rd_base (*(unsigned long long *)((char *)(&rd_base) - 0x300000))
extern unsigned long long rd_size;
#define rd_size (*(unsigned long long *)((char *)(&rd_size) - 0x300000))
extern unsigned long long saved_mem_higher;
extern unsigned long saved_mem_upper;
extern unsigned long saved_mem_lower;
extern unsigned long saved_mmap_addr;
extern unsigned long saved_mmap_length;

extern int filesystem_type;

extern unsigned long ROM_int15;
#define ROM_int15 (*(unsigned long *)((char *)(&ROM_int15) - 0x300000))

extern unsigned long ROM_int13;
#define ROM_int13 (*(unsigned long *)((char *)(&ROM_int13) - 0x300000))

extern unsigned long ROM_int13_dup;
#define ROM_int13_dup (*(unsigned long *)((char *)(&ROM_int13_dup) - 0x300000))

/*
 *  Error variables.
 */

extern unsigned long errnum;
#define errnum (*(unsigned long *)((char *)(&errnum) - 0x300000))
extern char *err_list[];

extern unsigned long memdisk_raw;
#define memdisk_raw (*(unsigned long *)((char *)(&memdisk_raw) - 0x300000))

extern unsigned long a20_keep_on;
#define a20_keep_on (*(unsigned long *)((char *)(&a20_keep_on) - 0x300000))

extern unsigned long safe_mbr_hook;
#define safe_mbr_hook (*(unsigned long *)((char *)(&safe_mbr_hook) - 0x300000))

extern unsigned long int13_scheme;
#define int13_scheme (*(unsigned long *)((char *)(&int13_scheme) - 0x300000))

extern int is64bit;
#define is64bit (*(int *)((char *)(&is64bit) - 0x300000))
extern int debug;
#define debug (*(int *)((char *)(&debug) - 0x300000))

/* Simplify declaration of entry_addr. */
typedef void (*entry_func) (int, int, int, int, int, int)
     __attribute__ ((noreturn));

extern entry_func entry_addr;

/* Enter the stage1.5/stage2 C code after the stack is set up. */
void cmain (void);

/* calls for direct boot-loader chaining */
int chain_stage1 (unsigned long segment_offset);
int console_getkey (void);
int console_checkkey (void);
unsigned long realmode_run (unsigned long regs_ptr);
#define chain_stage1 ((int (*)(unsigned long))((char *)(&chain_stage1) - 0x300000))
#define console_getkey ((int (*)(void))((char *)(&console_getkey) - 0x300000))
#define console_checkkey ((int (*)(void))((char *)(&console_checkkey) - 0x300000))
#define realmode_run ((unsigned long (*)(unsigned long))((char *)(&realmode_run) - 0x300000))

/* Return the data area immediately following our code. */
int get_code_end (void);

void console_putchar (int c);
#define console_putchar ((void (*)(int))((char *)(&console_putchar) - 0x300000))

/* Low-level disk I/O */
int biosdisk (int subfunc, int drive, unsigned long long sector, unsigned long nsec, int segment);

/* The table for a builtin.  */
struct builtin
{
  char *name;			/* The command name.  */
  int (*func) (char *);		/* The callback function.  */
};

/* All the builtins are registered in this.  */
extern struct builtin *builtin_table[];

extern char *mbr;

char *skip_to (char *cmdline);
struct builtin *find_command (char *command);
void print_cmdline_message (int forever);
void enter_cmdline (void);

/* C library replacement functions with identical semantics. */
#define grub_printf(...) grub_sprintf(NULL, __VA_ARGS__)
int grub_sprintf (char *buffer, const char *format, ...);
int grub_tolower (int c);
int grub_isspace (int c);
int grub_strncat (char *s1, const char *s2, int n);
void *grub_memcpy (void *to, const void *from, unsigned int n);
void *grub_memmove (void *to, const void *from, int len);
void *grub_memset (void *start, int c, int len);
int grub_strncat (char *s1, const char *s2, int n);
char *grub_strstr (const char *s1, const char *s2);
char *grub_strtok (char *s, const char *delim);
int grub_memcmp (const char *s1, const char *s2, int n);
int grub_strcmp (const char *s1, const char *s2);
int grub_strlen (const char *str);
char *grub_strcpy (char *dest, const char *src);

/* misc */
int get_cmdline (void);
int substring (const char *s1, const char *s2, int case_insensitive);
int nul_terminate (char *str);
int get_based_digit (int c, int base);
int safe_parse_maxint (char **str_ptr, unsigned long long *myint_ptr);
int parse_string (char *arg);
int memcheck (unsigned long long addr, unsigned long long len);
int grub_putstr (const char *str);
unsigned int grub_sleep (unsigned int seconds);

int rawread (unsigned long drive, unsigned long long sector, unsigned long byte_offset, unsigned long long byte_len, unsigned long long buf, unsigned long write);
int devread (unsigned long long sector, unsigned long byte_offset, unsigned long long byte_len, unsigned long long buf, unsigned long write);

/* Parse a device string and initialize the global parameters. */
char *set_device (char *device);
int open_device (void);
int open_partition (void);
int list_partitions (void);

/* Open a file or directory on the active device, using GRUB's
   internal filesystem support. */
int grub_open (char *filename);

/* Read LEN bytes into BUF from the file that was opened with
   GRUB_OPEN.  If LEN is -1, read all the remaining data in the file.  */
unsigned long long grub_read (unsigned long long buf, unsigned long long len, unsigned long write);

/* Display statistics on the current active device. */
void print_fsys_type (void);

/* Display device and filename completions. */
void print_a_completion (char *filename, int case_insensitive);
int print_completions (void);

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
/* 1C */ unsigned long hidden_sectors __attribute__ ((packed));/* any value */
/* 20 */ unsigned long total_sectors_long __attribute__ ((packed));/* FAT32=non-zero, NTFS=0, FAT12/16=any */
/* 24 */ unsigned long sectors_per_fat32 __attribute__ ((packed));/* FAT32=non-zero, NTFS=any, FAT12/16=any */
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
	/* +08 */ unsigned long start_lba __attribute__ ((packed));
	/* +0C */ unsigned long total_sectors __attribute__ ((packed));
	/* +10 */
    } P[4];
/* 1FE */ unsigned short boot_signature __attribute__ ((packed));/* 0xAA55 */
};

int probe_bpb (struct master_and_dos_boot_sector *BS);
int probe_mbr (struct master_and_dos_boot_sector *BS, unsigned long start_sector1, unsigned long sector_count1, unsigned long part_start1);

struct realmode_regs {
	unsigned long edi; // input and output
	unsigned long esi; // input and output
	unsigned long ebp; // input and output
	unsigned long esp; //stack pointer, input
	unsigned long ebx; // input and output
	unsigned long edx; // input and output
	unsigned long ecx; // input and output
	unsigned long eax;// input and output
	unsigned long gs; // input and output
	unsigned long fs; // input and output
	unsigned long es; // input and output
	unsigned long ds; // input and output
	unsigned long ss; //stack segment, input
	unsigned long eip; //instruction pointer, input
	unsigned long cs; //code segment, input
	unsigned long eflags; // input and output
};

struct drive_map_slot
{
	/* Remember to update DRIVE_MAP_SLOT_SIZE once this is modified.
	 * The struct size must be a multiple of 4.
	 */

	  /* X=max_sector bit 7: read only or fake write */
	  /* Y=to_sector  bit 6: safe boot or fake write */
	  /* ------------------------------------------- */
	  /* X Y: meaning of restrictions imposed on map */
	  /* ------------------------------------------- */
	  /* 1 1: read only=0, fake write=1, safe boot=0 */
	  /* 1 0: read only=1, fake write=0, safe boot=0 */
	  /* 0 1: read only=0, fake write=0, safe boot=1 */
	  /* 0 0: read only=0, fake write=0, safe boot=0 */

	unsigned char from_drive;
	unsigned char to_drive;		/* 0xFF indicates a memdrive */
	unsigned char max_head;
	unsigned char max_sector;	/* bit 7: read only */
					/* bit 6: disable lba */

	unsigned short to_cylinder;	/* max cylinder of the TO drive */
					/* bit 15:  TO  drive support LBA */
					/* bit 14:  TO  drive is CDROM(with big 2048-byte sector) */
					/* bit 13: FROM drive is CDROM(with big 2048-byte sector) */

	unsigned char to_head;		/* max head of the TO drive */
	unsigned char to_sector;	/* max sector of the TO drive */
					/* bit 7: in-situ */
					/* bit 6: fake-write or safe-boot */

	unsigned long long start_sector;
	//unsigned long start_sector_hi;	/* hi dword of the 64-bit value */
	unsigned long long sector_count;
	//unsigned long sector_count_hi;	/* hi dword of the 64-bit value */
};

//extern struct drive_map_slot bios_drive_map[DRIVE_MAP_SIZE + 1];
//extern struct drive_map_slot hooked_drive_map[DRIVE_MAP_SIZE + 1];
extern struct drive_map_slot bios_drive_map[];
extern struct drive_map_slot hooked_drive_map[];

//#define bios_drive_map ((struct drive_map_slot *)((char *)(&bios_drive_map) - 0x300000))
#define hooked_drive_map ((struct drive_map_slot *)((char *)(&hooked_drive_map) - 0x300000))

void linux_boot (void) __attribute__ ((noreturn));
#define linux_boot ((void (*)(void))((char *)(&linux_boot) - 0x300000))

extern char floppies_orig;
extern char harddrives_orig;

#define floppies_orig (*(char *)((char *)(&floppies_orig) - 0x300000))
#define harddrives_orig (*(char *)((char *)(&harddrives_orig) - 0x300000))

#endif /* ! ASM_FILE */

#endif /* ! GRUB_SHARED_HEADER */
