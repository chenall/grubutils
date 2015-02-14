/* builtins.c - the GRUB builtin commands */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004  Free Software Foundation, Inc.
 *  Copyright (C) 2010  Tinybit(tinybit@tom.com)
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

/* Include stdio.h before shared.h, because we can't define
   WITHOUT_LIBC_STUBS here.  */

#include "shared.h"

extern unsigned long chain_load_from;
extern unsigned long chain_load_to;
extern unsigned long chain_load_dword_len;
extern unsigned long chain_boot_IP;//0x7c00;
extern unsigned long chain_edx;
extern unsigned long chain_ebx;
void HMA_start (void) __attribute__ ((noreturn));

extern char *linux_data_real_addr;

/* For the Linux/i386 boot protocol version 2.03.  */
struct linux_kernel_header
{
  char code1[0x0020];
  unsigned short cl_magic;		/* Magic number 0xA33F */
  unsigned short cl_offset;		/* The offset of command line */
  char code2[0x01F1 - 0x0020 - 2 - 2];
  unsigned char setup_sects;		/* The size of the setup in sectors */
  unsigned short root_flags;		/* If the root is mounted readonly */
  unsigned short syssize;		/* obsolete */
  unsigned short swap_dev;		/* obsolete */
  unsigned short ram_size;		/* obsolete */
  unsigned short vid_mode;		/* Video mode control */
  unsigned short root_dev;		/* Default root device number */
  unsigned short boot_flag;		/* 0xAA55 magic number */
  unsigned short jump;			/* Jump instruction */
  unsigned long header;			/* Magic signature "HdrS" */
  unsigned short version;		/* Boot protocol version supported */
  unsigned long realmode_swtch;		/* Boot loader hook */
  unsigned long start_sys;		/* Points to kernel version string */
  unsigned char type_of_loader;		/* Boot loader identifier */
  unsigned char loadflags;		/* Boot protocol option flags */
  unsigned short setup_move_size;	/* Move to high memory size */
  unsigned long code32_start;		/* Boot loader hook */
  unsigned long ramdisk_image;		/* initrd load address */
  unsigned long ramdisk_size;		/* initrd size */
  unsigned long bootsect_kludge;	/* obsolete */
  unsigned short heap_end_ptr;		/* Free memory after setup end */
  unsigned short pad1;			/* Unused */
  char *cmd_line_ptr;			/* Points to the kernel command line */
  unsigned long initrd_addr_max;	/* The highest address of initrd */
} __attribute__ ((packed));

int read_dbr (void);
int command_func (char *arg/*, int flags*/);

/* The number of the history entries.  */
int num_history = 0;
int history;

int titles_count = 0;
int real_titles_count = 0;
int title_scripts[40];
char real_title_numbers[40];
int is_real_title;
int default_entry = 0;
int grub_timeout = -1;

char * get_history (void);
void add_history (void);

struct drive_map_slot bios_drive_map[DRIVE_MAP_SIZE + 1];

/* Get the NOth history. If NO is less than zero or greater than or
   equal to NUM_HISTORY, return NULL. Otherwise return a valid string.  */
char *
get_history (void)
{
  int j;
  char *p = (char *) HISTORY_BUF;
  if (history < 0 || history >= num_history)
	return 0;
  /* get history */
  for (j = 0; j < history; j++)
  {
	p += *(unsigned short *)p;
	if (p > (char *) HISTORY_BUF + HISTORY_BUFLEN)
	{
		num_history = j;
		return 0;
	}
  }

  return p + 2;
}

/* Add CMDLINE to the history buffer.  */
void
add_history (void)
{
  int j, len;
  char *p = (char *) HISTORY_BUF;
  char *cmdline = (char *) CMDLINE_BUF;

  if ((unsigned long)num_history >= 0x7FFFFFFFUL)
	return;
  if (*cmdline == 0 || *cmdline == 0x20)
	return;
  /* if string already in history, skip */

  for (j = 0; j < num_history; j++)
  {
	if (! grub_strcmp (p + 2, cmdline))
		return;
	p += *(unsigned short *)p;
	if (p > (char *) HISTORY_BUF + HISTORY_BUFLEN)
	{
		num_history = j;
		break;
	}
  }

  /* get cmdline length */
  len = grub_strlen (cmdline) + 3;
  p = (char *) HISTORY_BUF;
  if (((char *) HISTORY_BUF + HISTORY_BUFLEN) > (p + len))
	grub_memmove (p + len, p, ((char *) HISTORY_BUF + HISTORY_BUFLEN) - (p + len));
  *(unsigned short *)p = len;
  grub_strcpy (p + 2, cmdline);
  num_history++;
}

int
read_dbr (void)
{
	char tmp_str[64];

	grub_sprintf (tmp_str, "(%d,%d)+1",
		(unsigned char)current_drive,
		*(((unsigned char *)
		&current_partition)+2));
	if (! grub_open (tmp_str))
		return 0;//! (errnum = ERR_EXEC_FORMAT);
	if (grub_read (0x7C00, 512, 0xedde0d90) != 512)
		return ! (errnum = ERR_EXEC_FORMAT);

	if (*((unsigned long *) (0x7C00 + 0x1C)))
	    *((unsigned long *) (0x7C00 + 0x1C)) = part_start;
	return 1;
}

/* command */
int
command_func (char *arg/*, int flags*/)
{
  while (*arg == ' ' || *arg == '\t') arg++;
  if ((unsigned char)*arg < 0x20)
	return 0;

  /* open the command file. */
  {
	char *filename;
	char command_filename[256];

	grub_memmove (command_filename + 1, arg, sizeof(command_filename) - 2);
	command_filename[0] = '/';
	command_filename[255] = 0;
	nul_terminate (command_filename + 1);
	filename = command_filename + 1;

	if ((*arg >= 'a' && *arg <= 'z') || (*arg >= 'A' && *arg <= 'Z'))
		filename--;
	if (! grub_open (filename))
		return 0;
	if (((unsigned long *)(&filemax))[1] || (unsigned long)filemax < 9)
	{
		errnum = ERR_EXEC_FORMAT;
		goto fail;
	}
  }

  /* check if we have enough memory. */
  {
	unsigned long long memory_needed;

	arg = skip_to (arg);		/* get argument of command */
	memory_needed = filemax + ((grub_strlen (arg) + 16) & ~0xF) + 16 + 16;

	/* The amount of free memory is (free_mem_end - free_mem_start) */

	if (memory_needed > (free_mem_end - free_mem_start))
	{
		errnum = ERR_WONT_FIT;
		goto fail;
	}
  }

  /* Where is the free memory? build PSP, load the executable image. */
  {
	unsigned long pid = 255;
	unsigned long j;
	unsigned long psp_len;
	unsigned long prog_start;
	unsigned long long end_signature;

	for (j = 1;
		(unsigned long)&mem_alloc_array_start[j] < mem_alloc_array_end
		&& mem_alloc_array_start[j].addr; j++)
	{
	    if (pid < mem_alloc_array_start[j].pid)
		pid = mem_alloc_array_start[j].pid;
	}
	pid++;	/* new pid. */

	/* j - 1 is possibly for free memory start */
	--j;

	if ((mem_alloc_array_start[j].addr & 0x01))	/* no free memory. */
	{
		errnum = ERR_WONT_FIT;
		goto fail;
	}

	/* check sanity */
	if ((mem_alloc_array_start[j].addr & 0xFFFFFFF0) != free_mem_start)
	{
		errnum = ERR_INTERNAL_CHECK;
		goto fail;
	}

	psp_len = ((grub_strlen(arg) + 16) & ~0xF) + 16 + 16;
	prog_start = free_mem_start + psp_len;

	((unsigned long *)free_mem_start)[0] = psp_len;
	/* copy args into somewhere in PSP. */
	grub_memmove ((char *)(free_mem_start+16), arg, grub_strlen(arg)+1);
	/* PSP length in bytes. it is in both the starting dword and the
	 * ending dword of the PSP. */
	*(unsigned long *)(prog_start - 4) = psp_len;
	*(unsigned long *)(prog_start - 8) = psp_len - 16;	/* args */
	/* (prog_start - 16) is reserved for full pathname of the program. */
	if (grub_read (prog_start, -1ULL, 0xedde0d90) != filemax)
	{
		goto fail;
	}

	/* check exec signature. */
	end_signature = *(unsigned long long *)(prog_start + (unsigned long)filemax - 8);
	if (end_signature != 0xBCBAA7BA03051805ULL)
	{
		unsigned long boot_entry;
		unsigned long load_addr;
		unsigned long load_length;

		load_length = (unsigned long)filemax;
		/* check Linux */
		if (load_length < 5120 || load_length > 0x4000000)
			goto non_linux;
		{
		  //char *linux_data_real_addr;
		  char *linux_data_tmp_addr;
		  unsigned long cur_addr;
		  struct linux_kernel_header *lh = (struct linux_kernel_header *) prog_start;
		  int big_linux = 0;
		  int setup_sects = lh->setup_sects;
		  unsigned long data_len = 0;
		  unsigned long text_len = 0;
		  char *linux_bzimage_tmp_addr;

		  if (lh->boot_flag != 0xAA55 || setup_sects >= 0x40)
			goto non_linux;
		  if (lh->header == 0x53726448/* "HdrS" */ && lh->version >= 0x0200)
		  {
			big_linux = (lh->loadflags & 1/*LINUX_FLAG_BIG_KERNEL*/);
			lh->type_of_loader = 0x71/*LINUX_BOOT_LOADER_TYPE*/;

			/* Put the real mode part at as a high location as possible.  */
			linux_data_real_addr = (char *) (((*(unsigned short *)0x413) << 10) - 0x9400);
			/* But it must not exceed the traditional area.  */
			if (linux_data_real_addr > (char *) 0x90000)
			    linux_data_real_addr = (char *) 0x90000;

			if (lh->version >= 0x0201)
			{
			  lh->heap_end_ptr = (0x9000 - 0x200)/*LINUX_HEAP_END_OFFSET*/;
			  lh->loadflags |= 0x80/*LINUX_FLAG_CAN_USE_HEAP*/;
			}

			if (lh->version >= 0x0202)
			  lh->cmd_line_ptr = linux_data_real_addr + 0x9000/*LINUX_CL_OFFSET*/;
			else
			{
			  lh->cl_magic = 0xA33F/*LINUX_CL_MAGIC*/;
			  lh->cl_offset = 0x9000/*LINUX_CL_OFFSET*/;
			  lh->setup_move_size = 0x9400;
			}
		  } else {
			/* Your kernel is quite old...  */
			lh->cl_magic = 0xA33F/*LINUX_CL_MAGIC*/;
			lh->cl_offset = 0x9000/*LINUX_CL_OFFSET*/;
			if (setup_sects != 4)
				goto non_linux;
			linux_data_real_addr = (char *) 0x90000;
		  }

		  if (! setup_sects)
			setup_sects = 4;

		  data_len = setup_sects << 9;
		  text_len = load_length - data_len - 0x200;

		  //
		  // initrd
		  //
		  // ---------- new cur_addr ------------------------
		  //
		  // .........
		  //
		  // ---------- cur_addr ----------------------------
		  //
		  // a copy of bootsect+setup of vmlinuz
		  //
		  // ---------- linux_data_tmp_addr -----------------
		  //
		  // a copy of pmode code of vmlinuz, the "text"
		  //
		  // ---------- linux_bzimage_tmp_addr --------------
		  //
		  // vmlinuz
		  //
		  // ---------- prog_start --------------------------

		  linux_bzimage_tmp_addr = (char *)((prog_start + load_length + 15) & 0xFFFFFFF0);
		  linux_data_tmp_addr = (char *)(((unsigned long)(linux_bzimage_tmp_addr + text_len + 15)) & 0xFFFFFFF0);

		  if (! big_linux && text_len > linux_data_real_addr - (char *)0x10000)
		  {
			return !(errnum = ERR_WONT_FIT);
		  }
		  if (linux_data_real_addr + 0x9400 > ((char *) ((*(unsigned short *)0x413) << 10)))
		  {
			return !(errnum = ERR_WONT_FIT);
		  }

		  //grub_printf ("   [Linux-%s, setup=0x%x, size=0x%x]\n", (big_linux ? "bzImage" : "zImage"), data_len, text_len);

		  grub_memmove (linux_data_tmp_addr, (char *)prog_start, data_len + 0x200);
		  if (lh->header != 0x53726448/* "HdrS" */ || lh->version < 0x0200)
			/* Clear the heap space.  */
			grub_memset (linux_data_tmp_addr + ((setup_sects + 1) << 9), 0, (64 - setup_sects - 1) << 9);

		  cur_addr = (unsigned long)(linux_data_tmp_addr + 0x9400 + 0xfff);
		  cur_addr &= 0xfffff000;

		/******************* begin loading INITRD *******************/
		{
		  unsigned long len;
		  //unsigned long long moveto;
		  //unsigned long long tmp;
		  //unsigned long long top_addr;

		  lh = (struct linux_kernel_header *) linux_data_tmp_addr;//(cur_addr - 0x9400);

		  len = 0;

		  /* now arg points to arguments of Linux kernel, i.e., initrd, etc. */

		  if (*arg != '(' && *arg != '/')
		    goto done_initrd;

		  if (! grub_open (arg))
			goto fail;

		  if ( (filemax >> 32)	// ((unsigned long *)(&filemax))[1]
			|| (unsigned long)filemax < 512
			|| (unsigned long)filemax > ((0xfffff000 & free_mem_end) - cur_addr)/*0x40000000UL*/)
		  {
			errnum = ERR_WONT_FIT;
			goto fail;
		  }

		  cur_addr = free_mem_end - (unsigned long)filemax; /* load to top */
		  cur_addr &= 0xfffff000;

		  if ((len = grub_read ((unsigned long long)cur_addr, -1ULL, 0xedde0d90)) != filemax)
			goto fail;

		  //grub_printf ("   [Linux-initrd @ 0x%X, 0x%X bytes]\n", cur_addr, (unsigned long)len);

		  /* FIXME: Should check if the kernel supports INITRD.  */
		  lh->ramdisk_image = cur_addr;
		  lh->ramdisk_size = len;

		  arg = skip_to (arg);	/* points to the rest of command-line arguments */
		}
		/******************* end loading INITRD *******************/
done_initrd:

		  /* Copy command-line */
		  {
		    //char *src = arg;
		    char *dest = linux_data_tmp_addr + 0x9000;

		    while (dest < linux_data_tmp_addr + 0x93FF && *arg)
			*(dest++) = *(arg++);
		    *dest = 0;
		  }

		  grub_memmove (linux_bzimage_tmp_addr, (char *)(prog_start + data_len + 0x200), text_len);

		  if (errnum)
			return 0;

		  {
			unsigned int p;
			/* check grub.exe */
			for (p = (unsigned int)linux_bzimage_tmp_addr; p < (unsigned int)linux_bzimage_tmp_addr + 0x8000; p += 0x200)
			{
				if (((*(long long *)(void *)p & 0xFFFF00FFFFFFFFFFLL) == 0x02030000008270EALL) && ((*(long long *)(void *)(p + 0x12) & 0xFFFFFFFFFFLL) == 0x0037392E30LL))
				{
					if (*(long *)(void *)(p + 0x80) == 0xFFFFFFFF)//boot_drive
					{
						*(long *)(void *)(p + 0x80) = saved_drive;
						*(long *)(void *)(p + 0x08) = (saved_partition | 0xFFFF);
					}
					break;
				}
			}
		  }
		  ///* Ugly hack.  */
		  //linux_text_len = text_len;

		  //type = (big_linux ? KERNEL_TYPE_BIG_LINUX : KERNEL_TYPE_LINUX);
		  //goto success; 
		  //grub_printf ("   linux_data_real_addr=%X, linux_data_tmp_addr=%X\n", linux_data_real_addr, linux_data_tmp_addr);
		  /* real mode part is in low mem, and above 0x10000, no conflict loading it now. */
		  //return 1;
		  grub_memmove (linux_data_real_addr, linux_data_tmp_addr, 0x9400);
		  /* loading the kernel code */
		  //grub_memmove ((char *)(big_linux ? 0x100000 : 0x10000), linux_bzimage_tmp_addr, text_len);
		  chain_load_to = (big_linux ? 0x100000 : 0x10000);
		  chain_load_from = (unsigned long)linux_bzimage_tmp_addr;
		  chain_load_dword_len = ((text_len + 3) >> 2);
		  //return 1;
		  linux_boot();	/* no return */
		}
non_linux:
		/* check pure 16-bit real-mode program.
		 * The size should be in the range 512 to 512K
		 */
		if (load_length < 512 || load_length > 0x80000)
			return ! (errnum = ERR_EXEC_FORMAT);
		if (end_signature == 0x85848F8D0C010512ULL)	/* realmode */
		{
			unsigned long ret;
			struct realmode_regs regs;

			if (load_length + 0x10100 > ((*(unsigned short *)0x413) << 10))
				return ! (errnum = ERR_WONT_FIT);

			ret = grub_strlen (arg); /* command-tail count */
#define INSERT_LEADING_SPACE 0
#if INSERT_LEADING_SPACE
			if (ret > 125)
#else
			if (ret > 126)
#endif
				return ! (errnum = ERR_BAD_ARGUMENT);

			/* first, backup low 640K memory to address 2M */
			grub_memmove ((char *)0x200000, 0, 0xA0000);
			
			/* copy command-tail */
			if (ret)
#if INSERT_LEADING_SPACE
				grub_memmove ((char *)0x10082, arg, ret++);
#else
				grub_memmove ((char *)0x10081, arg, ret);
#endif

			/* setup offset 0x80 for command-tail count */
#if INSERT_LEADING_SPACE
			*(short *)0x10080 = (ret | 0x2000);
#else
			*(char *)0x10080 = ret;
#endif

			/* end the command-tail with CR */
			*(char *)(0x10081 + ret) = 0x0D;

			/* clear the beginning word of DOS PSP. the program
			 * check it and see it is running under grub4dos.
			 * a normal DOS PSP should begin with "CD 20".
			 */
			*(short *)0x10000 = 0;

			/* copy program to 1000:0100 */
			grub_memmove ((char *)0x10100, (char *)prog_start, load_length);

			/* setup DS, ES, CS:IP */
			regs.cs = regs.ds = regs.es = 0x1000;
			regs.eip = 0x100;

			/* setup FS, GS, EFLAGS and stack */
			regs.ss = regs.esp = regs.fs = regs.gs = regs.eflags = -1;

			/* for 64K .com style command, setup stack */
			if (load_length < 0xFF00)
			{
				regs.ss = 0x1000;
				regs.esp = 0xFFFE;
			}

			ret = realmode_run ((unsigned long)&regs);
			/* restore memory 0x10000 - 0xA0000 */
			grub_memmove ((char *)0x10000, (char *)0x210000, ((*(unsigned short *)0x413) << 10) - 0x10000);
			return ret;
		}
		if ((unsigned long)filemax <= (unsigned long)1024)
		{
			/* check 55 AA */
			if (*(unsigned short *)(prog_start + 510) != 0xAA55)
				return ! (errnum = ERR_EXEC_FORMAT);
			boot_entry = 0x00007C00; load_addr = boot_entry;
		} else if (*(unsigned char *)prog_start == (unsigned char)0xE9
			&& *(unsigned char *)(prog_start + 2) == 0x01
			&& (unsigned long)filemax > 0x30000)	/* NTLDR */
		{
			{
				int readdbr_ret = 0;
				if (!(readdbr_ret = read_dbr())) return readdbr_ret;
			}

			boot_entry = 0x20000000; load_addr = 0x20000;
		} else if (*(short *)prog_start == 0x3EEB
			&& *(short *)(prog_start + 0x1FF8) >= 0x40
			&& *(short *)(prog_start + 0x1FF8) <= 0x1B8
			&& *(long *)(prog_start + 0x2000) == 0x008270EA
			&& (*(long *)(prog_start
				+ *(short *)(prog_start + 0x1FF8)))
					== (*(long *)(prog_start + 0x1FFC))
			&& filemax >= 0x4000)	/* GRLDR */
		{
			boot_entry = 0x10000000; load_addr = 0x10000;
		} else if ((*(long long *)prog_start | 0xFF00LL) == 0x4749464E4F43FFEBLL && filemax > 0x4000)	/* FreeDOS */
		{
			//boot_drive = current_drive;
			//install_partition = current_partition;
			chain_boot_IP = 0x00600000;
			chain_load_to = 0x0600;
drdos:
			chain_load_from = prog_start;
			chain_load_dword_len = ((load_length + 3) >> 2);
			chain_edx = current_drive;
			((char *)(&chain_edx))[1] = ((char *)(&current_partition))[2];
			chain_ebx = chain_edx;
			/* begin Roy added 2011-05-08. load DBR to 0000:7C00 */
			{
				int readdbr_ret = 0;
				if (!(readdbr_ret = read_dbr())) return readdbr_ret;
			}
			/* end Roy added 2011-05-08. load DBR to 0000:7C00 */
			HMA_start();   /* no return */
		/* begin Roy added 2011-05-08. ReactOS support. */
		} else if (filemax >= 0x40000
			&& *((short *)(prog_start))==0x5A4D	 // MZ header
			&& *((short *)(prog_start+0x80))==0x4550 // PE header
			&& *((short *)(prog_start+0xDC))==0x1	//PE subsystem
			&& *((long *)(prog_start+0xA8))==0x1000	//Entry address
			&& *((long *)(prog_start+0xB4))==0x8000	//Base address
			) /* ReactOS freeldr.sys */
		{
			chain_boot_IP = 0x00009000;
			chain_load_to = 0x8000;
			chain_load_from = prog_start;
			chain_load_dword_len = ((load_length + 3) >> 2);
			chain_edx = current_drive;
			((char *)(&chain_edx))[1] = ((char *)(&current_partition))[2];
			chain_ebx = chain_edx;
			HMA_start();   /* no return */
		/* end Roy added 2011-05-08. ReactOS support. */
		} else if ((*(short *)prog_start) == 0x5A4D && filemax > 0x10000
			&& (*(long *)(prog_start + 0x1FE)) == 0x4A420000
			&& (*(short *)(prog_start + 0x7FE)) == 0x534D
			&& (*(short *)(prog_start + 0x800)) != 0x4D43	)	/* MS-DOS 7.x, not WinMe */
		{
			//boot_drive = current_drive;
			//install_partition = current_partition;

			/* to prevent MS from wiping int vectors 32-3F. */
			/* search these 12 bytes ... */
			/* 83 C7 08	add di,08	*/
			/* B9 0E 00	mov cx,0E	*/
			/* AB		stosw		*/
			/* 83 C7 02	add di,02	*/
			/* E2 FA	loop (-6)	*/
			/* ... replace with NOPs */
			unsigned long p = prog_start + 0x800;
			unsigned long q = prog_start + load_length - 16;
			while (p < q)
			{
				if ((*(long *)p == 0xB908C783) &&
				    (*(long long*)(p+4)==0xFAE202C783AB000ELL))
				{
					((long *)p)[2]=((long *)p)[1]=
						*(long *)p = 0x90909090;
					p += 12;
					continue; /* would occur 3 times */
				}
				p++;
			}

			chain_boot_IP = 0x00700000;
			chain_load_to = 0x0700;
			chain_load_from = prog_start + 0x800;
			chain_load_dword_len = ((load_length - 0x800 + 3) >> 2);
			chain_edx = current_drive;
			((char *)(&chain_edx))[1] = ((char *)(&current_partition))[2];
			chain_ebx = 0;
			HMA_start();   /* no return */
		/* begin Roy added 2010-12-18. DR-DOS support. */
		} else if ((*(long long *)prog_start==0x501E0100122E802ELL)
				/* added 2011-01-07. support packed drbio. */
			|| (*(long long *)(prog_start+6)==0x646F4D206C616552LL)
				/* added 2011-05-08. DRMK support. */
			|| ((*(long long *)prog_start | 0xFFFF02LL) ==
				0x4F43000000FFFFEBLL &&
				(*(((long long *)prog_start)+1) ==
				0x706D6F435141504DLL)))   /* DR-DOS */
		{
			chain_boot_IP = 0x00700000;
			chain_load_to = 0x0700;
			goto drdos;
		/* end Roy added 2010-12-18. DR-DOS support. */
		/* begin Roy added 2010-12-19. ROM-DOS support. */
		} else if ((*(long long *)(prog_start + 0x200)) == 0xCB5052C03342CA8CLL && (*(long *)(prog_start + 0x208) == 0x5441464B))   /* ROM-DOS */
		{
			chain_boot_IP = 0x10000000;
			chain_load_to = 0x10000;
			chain_load_from = prog_start + 0x200;
			chain_load_dword_len = ((load_length - 0x200 + 3) >> 2);
			*(unsigned long *)0x84 = current_drive | 0xFFFF0000;
			HMA_start();   /* no return */
		/* end Roy added 2010-12-19. ROM-DOS support. */
		} else
			return ! (errnum = ERR_EXEC_FORMAT);

		//boot_drive = current_drive;
		//install_partition = current_partition;
		grub_memmove ((char *)load_addr, (char *)prog_start, load_length);
		return chain_stage1 (boot_entry);
	}

	/* allocate all memory to the program. */
	mem_alloc_array_start[j].addr |= 0x01;	/* memory is now in use. */
	mem_alloc_array_start[j].pid = pid;	/* with this pid. */

	//psp_len += free_mem_start;
	free_mem_start = free_mem_end;	/* no free mem for other programs. */

	/* call the new program. */
	pid = ((int (*)(void))prog_start)();	/* pid holds return value. */

	/* on exit, release the memory. */
	for (j = 1;
		(unsigned long)&mem_alloc_array_start[j] < mem_alloc_array_end
		&& mem_alloc_array_start[j].addr; j++)
			;
	--j;
	mem_alloc_array_start[j].pid = 0;
	free_mem_start = (mem_alloc_array_start[j].addr &= 0xFFFFFFF0);
	return pid;
  }


fail:

	if (! errnum)
		errnum = ERR_EXEC_FORMAT;
	return 0;
}

static struct builtin builtin_command =
{
  "command",
  command_func,
};

//-----------------------------------------------
#if 0
int
parse_string (char *arg)
{
  int len;
  char *p;
  char ch;
  int quote;

  //nul_terminate (arg);

  for (quote = len = 0, p = arg; (ch = *p); p++)
  {
	if (ch == '\\')
	{
		if (quote)
		{
			*arg++ = ch;
			len++;
			quote = 0;
			continue;
		}
		quote = 1;
		continue;
	}
	if (quote)
	{
		if (ch == 't')
		{
			*arg++ = '\t';
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'r')
		{
			*arg++ = '\r';
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'n')
		{
			*arg++ = '\n';
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'a')
		{
			*arg++ = '\a';
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'b')
		{
			*arg++ = '\b';
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'f')
		{
			*arg++ = '\f';
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'v')
		{
			*arg++ = '\v';
			len++;
			quote = 0;
			continue;
		}
		if (ch >= '0' && ch <= '7')
		{
			/* octal */
			int val = ch - '0';

			if (p[1] >= '0' && p[1] <= '7')
			{
				val *= 8;
				p++;
				val += *p -'0';
				if (p[1] >= '0' && p[1] <= '7')
				{
					val *= 8;
					p++;
					val += *p -'0';
				}
			}
			*arg++ = val;
			len++;
			quote = 0;
			continue;
		}
		if (ch == 'x')
		{
			/* hex */
			int val;

			p++;
			ch = *p;
			if (ch >= '0' && ch <= '9')
				val = ch - '0';
			else if (ch >= 'A' && ch <= 'F')
				val = ch - 'A' + 10;
			else if (ch >= 'a' && ch <= 'f')
				val = ch - 'a' + 10;
			else
				return len;	/* error encountered */
			p++;
			ch = *p;
			if (ch >= '0' && ch <= '9')
				val = val * 16 + ch - '0';
			else if (ch >= 'A' && ch <= 'F')
				val = val * 16 + ch - 'A' + 10;
			else if (ch >= 'a' && ch <= 'f')
				val = val * 16 + ch - 'A' + 10;
			else
				p--;
			*arg++ = val;
			len++;
			quote = 0;
			continue;
		}
		if (ch)
		{
			*arg++ = ch;
			len++;
			quote = 0;
			continue;
		}
		return len;
	}
	*arg++ = ch;
	len++;
	quote = 0;
  }
  return len;
}


void hexdump(unsigned long ofs,char* buf,int len);

void hexdump(unsigned long ofs,char* buf,int len)
{
  while (len>0)
    {
      int cnt,k;

      grub_printf ("\n%08X: ", ofs);
      cnt=16;
      if (cnt>len)
        cnt=len;

      for (k=0;k<cnt;k++)
        {	  
          printf("%02X ", (unsigned long)(unsigned char)(buf[k]));
          if ((k!=15) && ((k & 3)==3))
            printf(" ");
        }

      for (;k<16;k++)
        {
          printf("   ");
          if ((k!=15) && ((k & 3)==3))
            printf(" ");
        }

      printf("; ");

      for (k=0;k<cnt;k++)
        printf("%c",(unsigned long)((((unsigned char)buf[k]>=32) && ((unsigned char)buf[k]!=127))?buf[k]:'.'));

      //printf("\n");

      ofs+=16;
      len-=cnt;
    }
}


/* cat */
static int
cat_func (char *arg/*, int flags*/)
{
  unsigned char c;
  unsigned char s[128];
  unsigned char r[128];
  unsigned long long Hex = 0;
  unsigned long long len, i, j;
  char *p;
  unsigned long long skip = 0;
  unsigned long long length = 0xffffffffffffffffULL;
  char *locate = 0;
  char *replace = 0;
  unsigned long long locate_align = 1;
  unsigned long long len_s;
  unsigned long long len_r = 0;
  unsigned long long ret = 0;

  for (;;)
  {
	if (grub_memcmp (arg, "--hex=", 6) == 0)
	{
		p = arg + 6;
		if (! safe_parse_maxint (&p, &Hex))
			return 0;
	}
    else if (grub_memcmp (arg, "--hex", 5) == 0)
      {
	Hex = 1;
      }
    else if (grub_memcmp (arg, "--skip=", 7) == 0)
      {
	p = arg + 7;
	if (! safe_parse_maxint (&p, &skip))
		return 0;
      }
    else if (grub_memcmp (arg, "--length=", 9) == 0)
      {
	p = arg + 9;
	if (! safe_parse_maxint (&p, &length))
		return 0;
      }
    else if (grub_memcmp (arg, "--locate=", 9) == 0)
      {
	p = locate = arg + 9;
	if (*p == '\"')
	{
	  while (*(++p) != '\"');
	  arg = ++p; // or: arg = p;
	}
      }
    else if (grub_memcmp (arg, "--replace=", 10) == 0)
      {
	p = replace = arg + 10;
	if (*replace == '*')
	{
        replace++;
        if (! safe_parse_maxint (&replace, &len_r))
			replace = p;
	} 
	if (*p == '\"')
	{
	  while (*(++p) != '\"');
	  arg = ++p;
	}
      }
    else if (grub_memcmp (arg, "--locate-align=", 15) == 0)
      {
	p = arg + 15;
	if (! safe_parse_maxint (&p, &locate_align))
		return 0;
	if ((unsigned long)locate_align == 0)
		return ! (errnum = ERR_BAD_ARGUMENT);
      }
    else
	break;
    arg = skip_to (arg);
  }
  if (! length)
  {
	if (grub_memcmp (arg,"()-1\0",5) == 0 )
	{
        if (! grub_open ("()+1"))
            return 0;
        filesize = filemax*(unsigned long long)part_start;
	} 
	else if (grub_memcmp (arg,"()\0",3) == 0 )
     {
        if (! grub_open ("()+1"))
            return 0;
        filesize = filemax*(unsigned long long)part_length;
     }
    else 
    {
       if (! grub_open (arg))
            return 0;
       filesize = filemax;
     }
		grub_printf ("\nFilesize is 0x%lX", (unsigned long long)filesize);
	ret = filemax;
	return ret;
  }
  if (! grub_open (arg))
    return 0; 
  if (length > filemax)
      length = filemax;
  filepos = skip;
  if (replace)
  {
	if (! len_r)
	{
		Hex = 0;
        if (*replace == '\"')
        {
        for (i = 0; i < 128 && (r[i] = *(++replace)) != '\"'; i++);
        }else{
        for (i = 0; i < 128 && (r[i] = *(replace++)) != ' ' && r[i] != '\t'; i++);
        }
        r[i] = 0;
        len_r = parse_string ((char *)r);
    }
    else
      {
		if (! Hex)
			Hex = -1ULL;
      }
  }
  if (locate)
  {
    if (*locate == '\"')
    {
      for (i = 0; i < 128 && (s[i] = *(++locate)) != '\"'; i++);
    }else{
      for (i = 0; i < 128 && (s[i] = *(locate++)) != ' ' && s[i] != '\t'; i++);
    }
    s[i] = 0;
    len_s = parse_string ((char *)s);
    if (len_s == 0 || len_s > 16)
	return ! (errnum = ERR_BAD_ARGUMENT);
    //j = skip;
    grub_memset ((char *)(SCRATCHADDR), 0, 32);
    for (j = skip; ; j += 16)
    {
	len = 0;
	if (j - skip < length)
	    len = grub_read ((unsigned long long)(SCRATCHADDR + 16), 16, 0xedde0d90);
	if (len < 16)
	    grub_memset ((char *)(SCRATCHADDR + 16 + (unsigned long)len), 0, 16 - len);

	if (j != skip)
	{
	    for (i = 0; i < 16; i++)
	    {
		unsigned long long k = j - 16 + i;
		if (locate_align == 1 || ! ((unsigned long)k % (unsigned long)locate_align))
		    if (! grub_memcmp ((char *)&s, (char *)(SCRATCHADDR + (unsigned long)i), len_s))
		    {
			/* print the address */
				grub_printf (" %lX", (unsigned long long)k);
			/* replace strings */
			if ((replace) && len_r)
			{
				unsigned long long filepos_bak;

				filepos_bak = filepos;
				filepos = k;
				if (Hex)
				  {
                    grub_read (len_r,(Hex == -1ULL)?8:Hex, 0x900ddeed);
					i = ((Hex == -1ULL)?8:Hex)+i;
                  }
				else
				 {
	 				/* write len_r bytes at string r to file!! */
                    grub_read ((unsigned long long)(unsigned int)(char *)&r, len_r, 0x900ddeed);
                    i = i+len_r;
                  }
				i--;
				filepos = filepos_bak;
			}
			ret++;
		    }
	    }
	}
	if (len == 0)
	    break;
	grub_memmove ((char *)SCRATCHADDR, (char *)(SCRATCHADDR + 16), 16);
    }
  }else if (Hex == (++ret))	/* a trick for (ret = 1, Hex == 1) */
  {
    for (j = skip; j - skip < length && (len = grub_read ((unsigned long long)(unsigned long)&s, 16, 0xedde0d90)); j += 16)
    {
		hexdump(j,(char*)&s,(len>length+skip-j)?(length+skip-j):len);
    }
  }else
    for (j = 0; j < length && grub_read ((unsigned long long)(unsigned long)&c, 1, 0xedde0d90); j++)
    {
#if 1
		console_putchar (c);
#else
	/* Because running "cat" with a binary file can confuse the terminal,
	   print only some characters as they are.  */
	if (grub_isspace (c) || (c >= ' ' && c <= '~'))
		grub_putchar (c);
	else
		grub_putchar ('?');
#endif
    }
  
  return ret;
}

static struct builtin builtin_cat =
{
  "cat",
  cat_func,
};
#endif

#if ! defined(MBRSECTORS127)

void set_int13_handler (struct drive_map_slot *map);
int unset_int13_handler (int check_status_only);

int map_func (char *);

static int
drive_map_slot_empty (struct drive_map_slot item)
{
	unsigned long *array = (unsigned long *)&item;
	
	unsigned long n = sizeof (struct drive_map_slot) / sizeof (unsigned long);
	
	while (n)
	{
		if (*array)
			return 0;
		array++;
		n--;
	}

	return 1;
	//if (*(unsigned long *)(&(item.from_drive))) return 0;
	//if (item.start_sector) return 0;
	//if (item.sector_count) return 0;
	//return 1;
}

#if 0
static int
drive_map_slot_equal (struct drive_map_slot a, struct drive_map_slot b)
{
	//return ! grub_memcmp ((void *)&(a.from_drive), (void *)&(b.from_drive), sizeof(struct drive_map_slot));
	return ! grub_memcmp ((void *)&a, (void *)&b, sizeof(struct drive_map_slot));
	//if (*(unsigned long *)(&(a.from_drive)) != *(unsigned long *)(&(b.from_drive))) return 0;
	//if (a.start_sector != b.start_sector) return 0;
	//if (a.sector_count != b.sector_count) return 0;
	//return 1;
}
#endif

/* map */
/* Map FROM_DRIVE to TO_DRIVE.  */
int
map_func (char *arg/*, int flags*/)
{
  char *to_drive;
  char *from_drive;
  unsigned long to, from;
  int i;
  
#if 0
    if (grub_memcmp (arg, "--status", 8) == 0)
      {
	int j;
	if (rd_base != -1ULL)
	    grub_printf ("\nram_drive=0x%X, rd_base=0x%lX, rd_size=0x%lX\n", ram_drive, rd_base, rd_size); 
	if (unset_int13_handler (1) && drive_map_slot_empty (bios_drive_map[0]))
	  {
	    grub_printf ("\nThe int13 hook is off. The drive map table is currently empty.\n");
	    return 1;
	  }

	  grub_printf ("\nFr To Hm Sm To_C _H _S   Start_Sector     Sector_Count   DHR"
		       "\n-- -- -- -- ---- -- -- ---------------- ---------------- ---\n");
	if (! unset_int13_handler (1))
	  for (i = 0; i < DRIVE_MAP_SIZE; i++)
	    {
		if (drive_map_slot_empty (hooked_drive_map[i]))
			break;
		for (j = 0; j < DRIVE_MAP_SIZE; j++)
		  {
		    if (drive_map_slot_equal(bios_drive_map[j], hooked_drive_map[i]))
			break;
		  }
		  grub_printf ("%02X %02X %02X %02X %04X %02X %02X %016lX %016lX %c%c%c\n", hooked_drive_map[i].from_drive, hooked_drive_map[i].to_drive, hooked_drive_map[i].max_head, hooked_drive_map[i].max_sector, hooked_drive_map[i].to_cylinder, hooked_drive_map[i].to_head, hooked_drive_map[i].to_sector, (unsigned long long)hooked_drive_map[i].start_sector, (unsigned long long)hooked_drive_map[i].sector_count, ((hooked_drive_map[i].to_cylinder & 0x4000) ? 'C' : hooked_drive_map[i].to_drive < 0x80 ? 'F' : hooked_drive_map[i].to_drive == 0xFF ? 'M' : 'H'), ((j < DRIVE_MAP_SIZE) ? '=' : '>'), ((hooked_drive_map[i].max_sector & 0x80) ? ((hooked_drive_map[i].to_sector & 0x40) ? 'F' : 'R') :((hooked_drive_map[i].to_sector & 0x40) ? 'S' : 'U')));
	    }
	for (i = 0; i < DRIVE_MAP_SIZE; i++)
	  {
	    if (drive_map_slot_empty (bios_drive_map[i]))
		break;
	    if (! unset_int13_handler (1))
	      {
		for (j = 0; j < DRIVE_MAP_SIZE; j++)
		  {
		    if (drive_map_slot_equal(hooked_drive_map[j], bios_drive_map[i]))
			break;
		  }
		if (j < DRIVE_MAP_SIZE)
			continue;
	      }
		  grub_printf ("%02X %02X %02X %02X %04X %02X %02X %016lX %016lX %c<%c\n", bios_drive_map[i].from_drive, bios_drive_map[i].to_drive, bios_drive_map[i].max_head, bios_drive_map[i].max_sector, bios_drive_map[i].to_cylinder, bios_drive_map[i].to_head, bios_drive_map[i].to_sector, (unsigned long long)bios_drive_map[i].start_sector, (unsigned long long)bios_drive_map[i].sector_count, ((bios_drive_map[i].to_cylinder & 0x4000) ? 'C' : bios_drive_map[i].to_drive < 0x80 ? 'F' : bios_drive_map[i].to_drive == 0xFF ? 'M' : 'H'), ((bios_drive_map[i].max_sector & 0x80) ? ((bios_drive_map[i].to_sector & 0x40) ? 'F' : 'R') :((bios_drive_map[i].to_sector & 0x40) ? 'S' : 'U')));
	  }
	return 1;
      }
#endif
    if (grub_memcmp (arg, "--hook", 6) == 0)
      {
	char *p;

	p = arg + 6;
	unset_int13_handler (0);
	if (drive_map_slot_empty (bios_drive_map[0]))
		return ! (errnum = ERR_NO_DRIVE_MAPPED);
	set_int13_handler (bios_drive_map);
	*(long *)PART_TABLE_BUF = buf_drive = buf_track = -1LL;
	return 1;
      }
    if (grub_memcmp (arg, "--unhook", 8) == 0)
      {
	unset_int13_handler (0);
	*(long *)PART_TABLE_BUF = buf_drive = buf_track = -1LL;
	return 1;
      }
  
  to_drive = arg;
  from_drive = skip_to (arg);

  /* Get the drive number for TO_DRIVE.  */
  set_device (to_drive);
  if (errnum)
    return 1;
  to = current_drive;

  /* Get the drive number for FROM_DRIVE.  */
  set_device (from_drive);
  if (errnum)
    return 1;
  from = current_drive;

  /* Search for an empty slot in BIOS_DRIVE_MAP.  */
  for (i = 0; i < DRIVE_MAP_SIZE; i++)
    {
      /* Perhaps the user wants to override the map.  */
      if (bios_drive_map[i].from_drive == from)
	break;
      
      if (drive_map_slot_empty (bios_drive_map[i]))
	break;
    }

  if (i == DRIVE_MAP_SIZE)
    {
      errnum = ERR_WONT_FIT;
      return 1;
    }

  if (to == from)
    /* If TO is equal to FROM, delete the entry.  */
    grub_memmove ((char *) &bios_drive_map[i], (char *) &bios_drive_map[i + 1],
		  sizeof (struct drive_map_slot) * (DRIVE_MAP_SIZE - i));
  else
  {
    (*(unsigned short *)(void *)(&(bios_drive_map[i]))) = from | (to << 8);
    grub_memset ((void *)((int)(&(bios_drive_map[i])) + 2), 0, DRIVE_MAP_SLOT_SIZE - 2);
  }
  
  return 0;
}

static struct builtin builtin_map =
{
  "map",
  map_func,
};
#endif

#if defined(MBRSECTORS127)

/* full-featured map implementation for ROM. */

//extern int rawread_ignore_memmove_overflow; /* defined in disk_io.c */
long query_block_entries;
unsigned long map_start_sector;
static unsigned long map_num_sectors = 0;

static unsigned long blklst_start_sector;
static unsigned long blklst_num_sectors;
static unsigned long blklst_num_entries;
static unsigned long blklst_last_length;
  
static unsigned long long start_sector, sector_count;

static void disk_read_start_sector_func (unsigned long long sector, unsigned long offset, unsigned long length);
static void
disk_read_start_sector_func (unsigned long long sector, unsigned long offset, unsigned long length)
{
      if (sector_count < 1)
	{
	  start_sector = sector;
	}
      sector_count++;
}

static void disk_read_blocklist_func (unsigned long long sector, unsigned long offset, unsigned long length);
  
  /* Collect contiguous blocks into one entry as many as possible,
     and print the blocklist notation on the screen.  */
static void
disk_read_blocklist_func (unsigned long long sector, unsigned long offset, unsigned long length)
{
      if (blklst_num_sectors > 0)
	{
	  if (blklst_start_sector + blklst_num_sectors == sector
	      && offset == 0 && blklst_last_length == 0x200)
	    {
	      blklst_num_sectors++;
	      blklst_last_length = length;
	      return;
	    }
	  else
	    {
	      if (query_block_entries >= 0)
	        {
		  if (blklst_last_length == 0x200)
		    grub_printf ("%s%ld+%d", (blklst_num_entries ? "," : ""),
			     (unsigned long long)(blklst_start_sector - part_start), blklst_num_sectors);
		  else if (blklst_num_sectors > 1)
		    grub_printf ("%s%ld+%d,%ld[0-%d]", (blklst_num_entries ? "," : ""),
			     (unsigned long long)(blklst_start_sector - part_start), (blklst_num_sectors-1),
			     (unsigned long long)(blklst_start_sector + blklst_num_sectors-1 - part_start), 
			     blklst_last_length);
		  else
		    grub_printf ("%s%ld[0-%d]", (blklst_num_entries ? "," : ""),
			     (unsigned long long)(blklst_start_sector - part_start), blklst_last_length);
	        }
	      blklst_num_entries++;
	      blklst_num_sectors = 0;
	    }
	}

      if (offset > 0)
	{
	  if (query_block_entries >= 0)
	  grub_printf("%s%ld[%d-%d]", (blklst_num_entries ? "," : ""),
		      (unsigned long long)(sector - part_start), offset, (offset + length));
	  blklst_num_entries++;
	}
      else
	{
	  blklst_start_sector = sector;
	  blklst_num_sectors = 1;
	  blklst_last_length = length;
	}
}

/* blocklist */
static int
blocklist_func (char *arg/*, int flags*/)
{
//#ifdef GRUB_UTIL
//  char *dummy = (char *) RAW_ADDR (0x100000);
//#else
//  char *dummy = (char *) RAW_ADDR (0x400000);
//#endif
  char *dummy = NULL;
  int err;
#ifndef NO_DECOMPRESSION
  int no_decompression_bak = no_decompression;
#endif
  
  blklst_start_sector = 0;
  blklst_num_sectors = 0;
  blklst_num_entries = 0;
  blklst_last_length = 0;

  map_start_sector = 0;
  map_num_sectors = 0;

  /* Open the file.  */
  if (! grub_open (arg))
    goto fail_open;

#ifndef NO_DECOMPRESSION
  if (compressed_file)
  {
    if (query_block_entries < 0)
    {
	/* compressed files are not considered contiguous. */
	query_block_entries = 3;
	goto fail_read;
    }

    grub_close ();
    no_decompression = 1;
    if (! grub_open (arg))
	goto fail_open;
  }
#endif /* NO_DECOMPRESSION */

  /* Print the device name.  */
  if (query_block_entries >= 0)
  {
	grub_printf ("(%cd%d", ((current_drive & 0x80) ? 'h' : 'f'), (current_drive & ~0x80));
  
	if ((current_partition & 0xFF0000) != 0xFF0000)
	    grub_printf (",%d", ((unsigned char)(current_partition >> 16)));
  
//	if ((current_partition & 0x00FF00) != 0x00FF00)
//	    grub_printf (",%c", ('a' + ((unsigned char)(current_partition >> 8))));
  
	grub_printf (")");
  }

//  rawread_ignore_memmove_overflow = 1;
  /* Read in the whole file to DUMMY.  */
  disk_read_hook = disk_read_blocklist_func;
  err = grub_read ((unsigned long long)(unsigned int)dummy, (query_block_entries < 0 ? 0x200 : -1ULL), 0xedde0d90);
  disk_read_hook = 0;
//  rawread_ignore_memmove_overflow = 0;
  if (! err)
    goto fail_read;

  /* The last entry may not be printed yet.  Don't check if it is a
   * full sector, since it doesn't matter if we read too much. */
  if (blklst_num_sectors > 0)
    {
      if (query_block_entries >= 0)
        grub_printf ("%s%ld+%d", (blklst_num_entries ? "," : ""),
		 (unsigned long long)(blklst_start_sector - part_start), blklst_num_sectors);
      blklst_num_entries++;
    }

  if (query_block_entries >= 0)
    console_putchar ('\n');
#if 0
  if (num_entries > 1)
    query_block_entries = num_entries;
  else
    {
      query_block_entries = 1;
      map_start_sector = start_sector;
      map_num_sectors = num_sectors;
    }
#endif
  if (query_block_entries < 0)
    {
      map_start_sector = blklst_start_sector;
      blklst_start_sector = 0;
      blklst_num_sectors = 0;
      blklst_num_entries = 0;
      blklst_last_length = 0;
//      rawread_ignore_memmove_overflow = 1;
      /* read in the last sector to DUMMY */
      filepos = filemax ? (filemax - 1) & (-(unsigned long long)0x200) : filemax;
      disk_read_hook = disk_read_blocklist_func;
      err = grub_read ((unsigned long long)(unsigned int)dummy, -1ULL, 0xedde0d90);
      disk_read_hook = 0;
//      rawread_ignore_memmove_overflow = 0;
      if (! err)
        goto fail_read;
      map_num_sectors = blklst_start_sector - map_start_sector + 1;
      query_block_entries = filemax ? 
	      map_num_sectors - ((filemax - 1) >> 9/*log2_tmp (buf_geom.sector_size)*//*>> SECTOR_BITS*/) : 2;
    }

fail_read:

  //grub_close ();

fail_open:

#ifndef NO_DECOMPRESSION
  no_decompression = no_decompression_bak;
#endif

  if (query_block_entries < 0)
    query_block_entries = 0;
  return ! errnum;
}

static struct builtin builtin_blocklist =
{
  "blocklist",
  blocklist_func,
};

static int
in_range (char *range, unsigned long long val)
{
  unsigned long long start_num;
  unsigned long long end_num;

  for (;;)
  {
    if (! safe_parse_maxint (&range, &start_num))
	break;
    if (val == start_num)
	return 1;
    if (*range == ',')
    {
	range++;
	continue;
    }
    if (*range != ':')
	break;

    range++;
    if (! safe_parse_maxint (&range, &end_num))
	break;
    if ((long long)val > (long long)start_num && (long long)val <= (long long)end_num)
	return 1;
    if (val > start_num && val <= end_num)
	return 1;
    if (*range != ',')
	break;

    range++;
  }

  errnum = 0;
  return 0;
}

int map_func (char *arg/*, int flags*/);
int unset_int13_handler (int check_status_only);
//extern int no_decompression;
int disable_map_info = 0;

static void print_bios_total_drives(void);
static void
print_bios_total_drives(void)
{	
	grub_printf ("\nfloppies_orig=%d, harddrives_orig=%d, floppies_curr=%d, harddrives_curr=%d\n", 
			((floppies_orig & 1)?(floppies_orig >> 6) + 1 : 0), harddrives_orig,
			(((*(char*)0x410) & 1)?((*(char*)0x410) >> 6) + 1 : 0), (*(char*)0x475));
}

void set_int13_handler (struct drive_map_slot *map);
int unset_int13_handler (int check_status_only);

int drive_map_slot_empty (struct drive_map_slot item);
int
drive_map_slot_empty (struct drive_map_slot item)
{
	unsigned long *array = (unsigned long *)&item;
	
	unsigned long n = sizeof (struct drive_map_slot) / sizeof (unsigned long);
	
	while (n)
	{
		if (*array)
			return 0;
		array++;
		n--;
	}

	return 1;
	//if (*(unsigned long *)(&(item.from_drive))) return 0;
	//if (item.start_sector) return 0;
	//if (item.sector_count) return 0;
	//return 1;
}
static int
drive_map_slot_equal (struct drive_map_slot a, struct drive_map_slot b)
{
	//return ! grub_memcmp ((void *)&(a.from_drive), (void *)&(b.from_drive), sizeof(struct drive_map_slot));
	return ! grub_memcmp ((void *)&a, (void *)&b, sizeof(struct drive_map_slot));
	//if (*(unsigned long *)(&(a.from_drive)) != *(unsigned long *)(&(b.from_drive))) return 0;
	//if (a.start_sector != b.start_sector) return 0;
	//if (a.sector_count != b.sector_count) return 0;
	//return 1;
}


unsigned long long map_mem_min = 0x100000;
unsigned long long map_mem_max = (-4096ULL);

#define INITRD_DRIVE	0x22

unsigned long probed_total_sectors;
unsigned long probed_total_sectors_round;
unsigned long probed_heads;
unsigned long probed_sectors_per_track;
unsigned long probed_cylinders;
unsigned long sectors_per_cylinder;

long long L[9]; /* L[n] == LBA[n] - S[n] + 1 */
long H[9];
short C[9];
short X;
short Y;
short Cmax;
long Hmax;
short Smax;
unsigned long Z;

/*  
 * return:
 * 		0		success
 *		otherwise	failure
 *			1	no "55 AA" at the end
 *
 */

int
probe_bpb (struct master_and_dos_boot_sector *BS)
{
  unsigned long i;

  /* first, check ext2 grldr boot sector */
  probed_total_sectors = BS->total_sectors_long;

  /* at 0D: (byte)Sectors per block. Valid values are 2, 4, 8, 16 and 32. */
  if (BS->sectors_per_cluster < 2 || 32 % BS->sectors_per_cluster)
	goto failed_ext2_grldr;

  /* at 0E: (word)Bytes per block.
   * Valid values are 0x400, 0x800, 0x1000, 0x2000 and 0x4000.
   */
  if (BS->reserved_sectors != BS->sectors_per_cluster * 0x200)
	goto failed_ext2_grldr;
  
  i = *(unsigned long *)((char *)BS + 0x2C);
  if (BS->reserved_sectors == 0x400 && i != 2)
	goto failed_ext2_grldr;

  if (BS->reserved_sectors != 0x400 && i != 1)
	goto failed_ext2_grldr;

  /* at 14: Pointers per block(number of blocks covered by an indirect block).
   * Valid values are 0x100, 0x200, 0x400, 0x800, 0x1000.
   */

  i = *(unsigned long *)((char *)BS + 0x14);
  if (i < 0x100 || 0x1000 % i)
	goto failed_ext2_grldr;
  
  /* at 10: Pointers in pointers-per-block blocks, that is, number of
   *        blocks covered by a double-indirect block.
   * Valid values are 0x10000, 0x40000, 0x100000, 0x400000 and 0x1000000.
   */

  if (*(unsigned long *)((char *)BS + 0x10) != i * i)
	goto failed_ext2_grldr;
  
  if (! BS->sectors_per_track || BS->sectors_per_track > 63)
	goto failed_ext2_grldr;
  
  if (BS->total_heads > 256 /* (BS->total_heads - 1) >> 8 */)
	goto failed_ext2_grldr;
  
  probed_heads = BS->total_heads;
  probed_sectors_per_track = BS->sectors_per_track;
  sectors_per_cylinder = probed_heads * probed_sectors_per_track;
  probed_cylinders = (probed_total_sectors + sectors_per_cylinder - 1) / sectors_per_cylinder;

  filesystem_type = 5;
  
  /* BPB probe success */
  return 0;

failed_ext2_grldr:
  probed_total_sectors = BS->total_sectors_short ? BS->total_sectors_short : (BS->total_sectors_long ? BS->total_sectors_long : (unsigned long)BS->total_sectors_long_long);
#if 0
  if (probed_total_sectors != sector_count && sector_count != 1 && (! (probed_total_sectors & 1) || probed_total_sectors != sector_count - 1))
	goto failed_probe_BPB;
#endif
  if (BS->bytes_per_sector != 0x200)
	return 1;
  if (! BS->sectors_per_cluster || 128 % BS->sectors_per_cluster)
	return 1;
//  if (! BS->reserved_sectors)	/* NTFS reserved_sectors is 0 */
//	goto failed_probe_BPB;
  if (BS->number_of_fats > (unsigned char)2 /* BS->number_of_fats && ((BS->number_of_fats - 1) >> 1) */)
	return 1;
  if (! BS->sectors_per_track || BS->sectors_per_track > 63)
	return 1;
  if (BS->total_heads > 256 /* (BS->total_heads - 1) >> 8 */)
	return 1;
  if (BS->media_descriptor < (unsigned char)0xF0)
	return 1;
  if (! BS->root_dir_entries && ! BS->total_sectors_short && ! BS->sectors_per_fat) /* FAT32 or NTFS */
	if (BS->number_of_fats && BS->total_sectors_long && BS->sectors_per_fat32)
	{
		filesystem_type = 3;
	}
	else if (! BS->number_of_fats && ! BS->total_sectors_long && ! BS->reserved_sectors && BS->total_sectors_long_long)
	{
		filesystem_type = 4;
	} else
		return 1;	/* unknown NTFS-style BPB */
  else if (BS->number_of_fats && BS->sectors_per_fat) /* FAT12 or FAT16 */
	if ((probed_total_sectors - BS->reserved_sectors - BS->number_of_fats * BS->sectors_per_fat - (BS->root_dir_entries * 32 + BS->bytes_per_sector - 1) / BS->bytes_per_sector) / BS->sectors_per_cluster < 0x0ff8 )
	{
		filesystem_type = 1;
	} else {
		filesystem_type = 2;
	}
  else
	return 1;	/* unknown BPB */
  
  probed_heads = BS->total_heads;
  probed_sectors_per_track = BS->sectors_per_track;
  sectors_per_cylinder = probed_heads * probed_sectors_per_track;
  probed_cylinders = (probed_total_sectors + sectors_per_cylinder - 1) / sectors_per_cylinder;

  /* BPB probe success */
  return 0;

}

/* on call:
 * 		BS		points to the bootsector
 * 		start_sector1	is the start_sector of the bootimage in the real disk, if unsure, set it to 0
 * 		sector_count1	is the sector_count of the bootimage in the real disk, if unsure, set it to 1
 * 		part_start1	is the part_start of the partition in which the bootimage resides, if unsure, set it to 0
 *  
 * on return:
 * 		0		success
 *		otherwise	failure
 *
 */

int
probe_mbr (struct master_and_dos_boot_sector *BS, unsigned long start_sector1, unsigned long sector_count1, unsigned long part_start1)
{
  unsigned long i, j;
  unsigned long lba_total_sectors = 0;
  
  /* probe the partition table */
  
  Cmax = 0; Hmax = 0; Smax = 0;
  for (i = 0; i < 4; i++)
    {
      int *part_entry;
      /* the boot indicator must be 0x80 (for bootable) or 0 (for non-bootable) */
      if ((unsigned char)(BS->P[i].boot_indicator << 1))/* if neither 0x80 nor 0 */
	return 1;
      /* check if the entry is empty, i.e., all the 16 bytes are 0 */
      part_entry = (int *)&(BS->P[i].boot_indicator);
      if (*part_entry++ || *part_entry++ || *part_entry++ || *part_entry)
      //if (*(long long *)&BS->P[i] || ((long long *)&BS->P[i])[1])
        {
	  /* valid partitions never start at 0, because this is where the MBR
	   * lives; and more, the number of total sectors should be non-zero.
	   */
	  if (! BS->P[i].start_lba || ! BS->P[i].total_sectors)
		return 2;
	  if (lba_total_sectors < BS->P[i].start_lba+BS->P[i].total_sectors)
	      lba_total_sectors = BS->P[i].start_lba+BS->P[i].total_sectors;
	  /* the partitions should not overlap each other */
	  for (j = 0; j < i; j++)
	  {
	    if ((BS->P[j].start_lba <= BS->P[i].start_lba) && (BS->P[j].start_lba + BS->P[j].total_sectors >= BS->P[i].start_lba + BS->P[i].total_sectors))
		continue;
	    if ((BS->P[j].start_lba >= BS->P[i].start_lba) && (BS->P[j].start_lba + BS->P[j].total_sectors <= BS->P[i].start_lba + BS->P[i].total_sectors))
		continue;
	    if ((BS->P[j].start_lba < BS->P[i].start_lba) ?
		(BS->P[i].start_lba - BS->P[j].start_lba < BS->P[j].total_sectors) :
		(BS->P[j].start_lba - BS->P[i].start_lba < BS->P[i].total_sectors))
		return 3;
	  }
	  /* the cylinder number */
	  C[i] = (BS->P[i].start_sector_cylinder >> 8) | ((BS->P[i].start_sector_cylinder & 0xc0) << 2);
	  if (Cmax < C[i])
		  Cmax = C[i];
	  H[i] = BS->P[i].start_head;
	  if (Hmax < H[i])
		  Hmax = H[i];
	  X = BS->P[i].start_sector_cylinder & 0x3f;/* the sector number */
	  if (Smax < X)
		  Smax = X;
	  /* the sector number should not be 0. */
	  ///* partitions should not start at the first track, the MBR-track */
	  if (! X /* || BS->P[i].start_lba < Smax */)
		return 4;
	  L[i] = BS->P[i].start_lba - X + 1;
	  if (start_sector1 == part_start1)/* extended partition is pretending to be a whole drive */
		L[i] +=(unsigned long) part_start1;
	
	  C[i+4] = (BS->P[i].end_sector_cylinder >> 8) | ((BS->P[i].end_sector_cylinder & 0xc0) << 2);
	  if (Cmax < C[i+4])
		  Cmax = C[i+4];
	  H[i+4] = BS->P[i].end_head;
	  if (Hmax < H[i+4])
		  Hmax = H[i+4];
	  Y = BS->P[i].end_sector_cylinder & 0x3f;
	  if (Smax < Y)
		  Smax = Y;
	  if (! Y)
		return 5;
	  L[i+4] = BS->P[i].start_lba + BS->P[i].total_sectors;
	  if (L[i+4] < Y)
		return 6;
	  L[i+4] -= Y;
	  if (start_sector1 == part_start1)/* extended partition is pretending to be a whole drive */
		L[i+4] +=(unsigned long) part_start1;
	  
   /* C[n] * (H_count * S_count) + H[n] * S_count = LBA[n] - S[n] + 1 = L[n] */

		/* C[n] * (H * S) + H[n] * S = L[n] */

	  /* Check the large disk partition -- Win98 */
	  if (Y == 63 && H[i+4] == Hmax && C[i+4] == Cmax
		&& (Hmax >= 254 || Cmax >= 1022)
		/* && C[i+4] == 1023 */
		&& (Cmax + 1) * (Hmax + 1) * 63 < L[i+4] + Y
		/* && C[i] * (Hmax + 1) * 63 + H[i] * 63 + X - 1 == BS->P[i].start_lba */
	     )
	  {
		if (C[i] * (Hmax+1) * 63 + H[i] * 63 > L[i])
			return 7;
		if (C[i] * (Hmax+1) * 63 + H[i] * 63 < L[i])
		{
		  /* calculate CHS numbers from start LBA */
		  if (X != ((unsigned long)L[i] % 63)+1 && X != 63)
			return 8;
		  if (H[i]!=((unsigned long)L[i]/63)%(Hmax+1) && H[i]!=Hmax)
			return 9;
		  if (C[i] != (((unsigned long)L[i]/63/(Hmax+1)) & 0x3ff) && C[i] != Cmax)
			return 10;
		}
		C[i] = 0; 
		H[i] = 1; 
		L[i] = 63;
		C[i+4] = 1; 
		H[i+4] = 0; 
		L[i+4] = (Hmax + 1) * 63;
	  }

	  /* Check the large disk partition -- Win2K */
	  else if (Y == 63 && H[i+4] == Hmax /* && C[i+4] == Cmax */
		&& (C[i+4] + 1) * (Hmax + 1) * 63 < L[i+4] + Y
		&& ! (((unsigned long)L[i+4] + Y) % ((Hmax + 1) * 63))
		&& ((((unsigned long)L[i+4] + Y) / ((Hmax + 1) * 63) - 1) & 0x3ff) == C[i+4]
	     )
	  {
		if (C[i] * (Hmax+1) * 63 + H[i] * 63 > L[i])
			return 11;
		if (C[i] * (Hmax+1) * 63 + H[i] * 63 < L[i])
		{
			if (((unsigned long)L[i] - H[i] * 63) % ((Hmax+1) * 63))
				return 12;
			if (((((unsigned long)L[i] - H[i] * 63) / ((Hmax+1) * 63)) & 0x3ff) != C[i])
				return 13;
		}
		C[i] = 0; 
		H[i] = 1; 
		L[i] = 63;
		C[i+4] = 1; 
		H[i+4] = 0; 
		L[i+4] = (Hmax + 1) * 63;
	  }

	  /* Maximum of C[n] * (H * S) + H[n] * S = 1023 * 255 * 63 + 254 * 63 = 0xFB03C1 */

	  else if (L[i+4] > 0xFB03C1) /* Large disk */
	  {
		/* set H/S to max */
		if (Hmax < 254)
		    Hmax = 254;
		Smax = 63;
		if ((unsigned long)L[i+4] % Smax)
		    return 114;
		if (H[i+4]!=((unsigned long)L[i+4]/63)%(Hmax+1) && H[i+4]!=Hmax)
		    return 115;
		if (C[i+4] != (((unsigned long)L[i+4]/63/(Hmax+1)) & 0x3ff) && C[i+4] != Cmax)
		    return 116;

		if (C[i] * (Hmax+1) * 63 + H[i] * 63 > L[i])
			return 117;
		if (C[i] * (Hmax+1) * 63 + H[i] * 63 < L[i])
		{
		  /* calculate CHS numbers from start LBA */
		  if (X != ((unsigned long)L[i] % 63)+1 && X != 63)
			return 118;
		  if (H[i]!=((unsigned long)L[i]/63)%(Hmax+1) && H[i]!=Hmax)
			return 119;
		  if (C[i] != (((unsigned long)L[i]/63/(Hmax+1)) & 0x3ff) && C[i] != Cmax)
			return 120;
		}
		C[i] = 0;
		H[i] = 1;
		L[i] = 63;
		C[i+4] = 1;
		H[i+4] = 0;
		L[i+4] = (Hmax + 1) * 63;
	  }
        }
      else
        {
	  /* empty entry, zero all the coefficients */
	  C[i] = 0;
	  H[i] = 0;
	  L[i] = 0;
	  C[i+4] = 0;
	  H[i+4] = 0;
	  L[i+4] = 0;
        }
    }

//  for (i = 0; i < 4; i++)
//    {
//grub_printf ("%d   \t%d   \t%d\n%d   \t%d   \t%d\n", C[i], H[i],(int)(L[i]), C[i+4], H[i+4],(int)(L[i+4]));
//    }
  for (i = 0; i < 8; i++)
    {
      if (C[i])
	break;
    }
  if (i == 8) /* all C[i] == 0 */
    {
      for (i = 0; i < 8; i++)
	{
	  if (H[i])
	    break;
	}
      if (i == 8) /* all H[i] == 0 */
	return 14;
      for (j = 0; j < i; j++)
	if (L[j])
	  return 15;
      if (! L[i])
	  return 16;
      //if (*(long *)((char *)&(L[i]) + 4))
      if (L[i] > 0x7fffffff)
	  return 17;
      if ((long)L[i] % H[i])
	  return 18;
      probed_sectors_per_track = (long)L[i] / H[i];
      if (probed_sectors_per_track > 63 || probed_sectors_per_track < Smax)
	  return 19;
      Smax = probed_sectors_per_track;
      for (j = i + 1; j < 8; j++)
        {
	  if (H[j])
	    {
              if (probed_sectors_per_track * H[j] != L[j])
	        return 20;
	    }
	  else if (L[j])
	        return 21;
	}
      probed_heads = Hmax + 1;
#if 0
      if (sector_count1 == 1)
	  probed_cylinders = 1;
      else
#endif
        {
	  L[8] = sector_count1; /* just set to a number big enough */
	  Z = sector_count1 / probed_sectors_per_track;
	  for (j = probed_heads; j <= 256; j++)
	    {
	      H[8] = Z % j;/* the remainder */
	      if (L[8] > H[8])
		{
		  L[8] = H[8];/* the least residue */
		  probed_heads = j;/* we got the optimum value */
		}
	    }
	  probed_cylinders = Z / probed_heads;
	  if (! probed_cylinders)
		  probed_cylinders = 1; 
	}
      sectors_per_cylinder = probed_heads * probed_sectors_per_track;
      probed_total_sectors_round = sectors_per_cylinder * probed_cylinders;
      probed_total_sectors = lba_total_sectors;
    }
  else
    {
	    if (i > 0)
	    {
		C[8] = C[i]; H[8] = H[i]; L[8] = L[i];
		C[i] = C[0]; H[i] = H[0]; L[i] = L[0];
		C[0] = C[8]; H[0] = H[8]; L[0] = L[8];
	    }
	    H[8] = 0; /* will store sectors per track */
	    for (i = 1; i < 8; i++)
	    {
		H[i] = C[0] * H[i] - C[i] * H[0];
		L[i] = C[0] * L[i] - C[i] * L[0];
		if (H[i])
		{
			if (H[i] < 0)
			  {
			    H[i] = - H[i];/* H[i] < 0x080000 */
			    L[i] = - L[i];
			  }
			/* Note: the max value of H[i] is 1024 * 256 * 2 = 0x00080000, 
			 * so L[i] should be less than 0x00080000 * 64 = 0x02000000 */
			if (L[i] <= 0 || L[i] > 0x7fffffff)
				return 22;
			L[8] = ((long)L[i]) / H[i]; /* sectors per track */
			if (L[8] * H[i] != L[i])
				return 23;
			if (L[8] > 63 || L[8] < Smax)
				return 24;
			Smax = L[8];
			if (H[8])
			  {
				/* H[8] is the old L[8] */
				if (L[8] != H[8])
					return 25;
			  }
			else /* H[8] is empty, so store L[8] for the first time */
				H[8] = L[8];
		}
		else if (L[i])
			return 26;
	    }
	    if (H[8])
	    {
		/* H[8] is sectors per track */
		L[0] -= H[0] * H[8];
		/* Note: the max value of H[8] is 63, the max value of C[0] is 1023,
		 * so L[0] should be less than 64 * 1024 * 256 = 0x01000000	 */
		if (L[0] <= 0 || L[0] > 0x7fffffff)
			return 27;
		
		/* L[8] is number of heads */
		L[8] = ((long)L[0]) / H[8] / C[0];
		if (L[8] * H[8] * C[0] != L[0])
			return 28;
		if (L[8] > 256 || L[8] <= Hmax)
			return 29;
		probed_sectors_per_track = H[8];
	    }
	    else /* fail to set L[8], this means all H[i]==0, i=1,2,3,4,5,6,7 */
	    {
		/* Now the only equation is: C[0] * H * S + H[0] * S = L[0] */
		for (i = 63; i >= Smax; i--)
		{
			L[8] = L[0] - H[0] * i;
			if (L[8] <= 0 || L[8] > 0x7fffffff)
				continue;
			Z = L[8];
			if (Z % (C[0] * i))
				continue;
			L[8] = Z / (C[0] * i);
			if (L[8] <= 256 && L[8] > Hmax)
				break;/* we have got the PROBED_HEADS */
		}
		if (i < Smax)
			return 30;
		probed_sectors_per_track = i;
	    }
	    probed_heads = L[8];
      sectors_per_cylinder = probed_heads * probed_sectors_per_track;
      probed_cylinders = (sector_count1 + sectors_per_cylinder - 1) / sectors_per_cylinder;
      if (probed_cylinders < Cmax + 1)
	      probed_cylinders = Cmax + 1;
      probed_total_sectors_round = sectors_per_cylinder * probed_cylinders;
      probed_total_sectors = lba_total_sectors;
    }

  
  filesystem_type = 0;	/* MBR device */
  
  /* partition table probe success */
  return 0;
}

/* map */
/* Map FROM_DRIVE to TO_DRIVE.  */
int
map_func (char *arg/*, int flags*/)
{
  char *to_drive;
  char *from_drive;
  unsigned long to, from, i = 0;
  int j;
  char *filename;
  char *p;
  unsigned long extended_part_start;
  unsigned long extended_part_length;

  int err;

  //struct master_and_dos_boot_sector *BS = (struct master_and_dos_boot_sector *) RAW_ADDR (0x8000);
#define	BS	((struct master_and_dos_boot_sector *)mbr)

  unsigned long long mem = -1ULL;
  int read_Only = 0;
  int fake_write = 0;
  int unsafe_boot = 0;
  int disable_lba_mode = 0;
  int disable_chs_mode = 0;
  unsigned long long sectors_per_track = -1ULL;
  unsigned long long heads_per_cylinder = -1ULL;
  //unsigned long long to_filesize = 0;
  unsigned long BPB_H = 0;
  unsigned long BPB_S = 0;
  int in_situ = 0;
  int add_mbt = -1;
  int prefer_top = 0;
  filesystem_type = -1;
  start_sector = sector_count = 0;
  
  for (;;)
  {
    if (grub_memcmp (arg, "--status", 8) == 0)
      {
#ifndef GRUB_UTIL
//	if (debug > 0)
	{
	  print_bios_total_drives();
	  //printf ("\nNumber of ATAPI CD-ROMs: %d\n", atapi_dev_count);
	}
#endif

	if (rd_base != -1ULL)
	    grub_printf ("\nram_drive=0x%X, rd_base=0x%lX, rd_size=0x%lX\n", ram_drive, rd_base, rd_size); 
	if (unset_int13_handler (1) && drive_map_slot_empty (bios_drive_map[0]))
	  {
	    grub_printf ("\nThe int13 hook is off. The drive map table is currently empty.\n");
	    return 1;
	  }

//struct drive_map_slot
//{
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

//	unsigned char from_drive;
//	unsigned char to_drive;		/* 0xFF indicates a memdrive */
//	unsigned char max_head;
//	unsigned char max_sector;	/* bit 7: read only */
					/* bit 6: disable lba */

//	unsigned short to_cylinder;	/* max cylinder of the TO drive */
					/* bit 15:  TO  drive support LBA */
					/* bit 14:  TO  drive is CDROM(with big 2048-byte sector) */
					/* bit 13: FROM drive is CDROM(with big 2048-byte sector) */

//	unsigned char to_head;		/* max head of the TO drive */
//	unsigned char to_sector;	/* max sector of the TO drive */
					/* bit 7: in-situ */
					/* bit 6: fake-write or safe-boot */

//	unsigned long start_sector;
//	unsigned long start_sector_hi;	/* hi dword of the 64-bit value */
//	unsigned long sector_count;
//	unsigned long sector_count_hi;	/* hi dword of the 64-bit value */
//};

	/* From To MaxHead MaxSector ToMaxCylinder ToMaxHead ToMaxSector StartLBA_lo StartLBA_hi Sector_count_lo Sector_count_hi Hook Type */
//	if (debug > 0)
	  grub_printf ("\nFr To Hm Sm To_C _H _S   Start_Sector     Sector_Count   DHR"
		       "\n-- -- -- -- ---- -- -- ---------------- ---------------- ---\n");
	if (! unset_int13_handler (1))
	  for (i = 0; i < DRIVE_MAP_SIZE; i++)
	    {
		if (drive_map_slot_empty (hooked_drive_map[i]))
			break;
		for (j = 0; j < DRIVE_MAP_SIZE; j++)
		  {
		    if (drive_map_slot_equal(bios_drive_map[j], hooked_drive_map[i]))
			break;
		  }
//		if (debug > 0)
		  grub_printf ("%02X %02X %02X %02X %04X %02X %02X %016lX %016lX %c%c%c\n", hooked_drive_map[i].from_drive, hooked_drive_map[i].to_drive, hooked_drive_map[i].max_head, hooked_drive_map[i].max_sector, hooked_drive_map[i].to_cylinder, hooked_drive_map[i].to_head, hooked_drive_map[i].to_sector, (unsigned long long)hooked_drive_map[i].start_sector, (unsigned long long)hooked_drive_map[i].sector_count, ((hooked_drive_map[i].to_cylinder & 0x4000) ? 'C' : hooked_drive_map[i].to_drive < 0x80 ? 'F' : hooked_drive_map[i].to_drive == 0xFF ? 'M' : 'H'), ((j < DRIVE_MAP_SIZE) ? '=' : '>'), ((hooked_drive_map[i].max_sector & 0x80) ? ((hooked_drive_map[i].to_sector & 0x40) ? 'F' : 'R') :((hooked_drive_map[i].to_sector & 0x40) ? 'S' : 'U')));
	    }
	for (i = 0; i < DRIVE_MAP_SIZE; i++)
	  {
	    if (drive_map_slot_empty (bios_drive_map[i]))
		break;
	    if (! unset_int13_handler (1))
	      {
		for (j = 0; j < DRIVE_MAP_SIZE; j++)
		  {
		    if (drive_map_slot_equal(hooked_drive_map[j], bios_drive_map[i]))
			break;
		  }
		if (j < DRIVE_MAP_SIZE)
			continue;
	      }
//	    if (debug > 0)
		  grub_printf ("%02X %02X %02X %02X %04X %02X %02X %016lX %016lX %c<%c\n", bios_drive_map[i].from_drive, bios_drive_map[i].to_drive, bios_drive_map[i].max_head, bios_drive_map[i].max_sector, bios_drive_map[i].to_cylinder, bios_drive_map[i].to_head, bios_drive_map[i].to_sector, (unsigned long long)bios_drive_map[i].start_sector, (unsigned long long)bios_drive_map[i].sector_count, ((bios_drive_map[i].to_cylinder & 0x4000) ? 'C' : bios_drive_map[i].to_drive < 0x80 ? 'F' : bios_drive_map[i].to_drive == 0xFF ? 'M' : 'H'), ((bios_drive_map[i].max_sector & 0x80) ? ((bios_drive_map[i].to_sector & 0x40) ? 'F' : 'R') :((bios_drive_map[i].to_sector & 0x40) ? 'S' : 'U')));
	  }
	return 1;
      }
    else if (grub_memcmp (arg, "--hook", 6) == 0)
      {
	unsigned long long first_entry = -1ULL;

	p = arg + 6;
	if (*p == '=')
	{
		p++;
		if (! safe_parse_maxint (&p, &first_entry))
			return 0;
		if (first_entry > 0xFF)
			return ! (errnum = ERR_BAD_ARGUMENT);
	}
	unset_int13_handler (0);
	//if (! unset_int13_handler (1))
	//	return ! (errnum = ERR_INT13_ON_HOOK);
	if (drive_map_slot_empty (bios_drive_map[0]))
	    //if (atapi_dev_count == 0)
		return ! (errnum = ERR_NO_DRIVE_MAPPED);
	if (first_entry <= 0xFF)
	{
	    /* setup first entry */
	    /* find first_entry in bios_drive_map */
	    for (i = 0; i < DRIVE_MAP_SIZE - 1; i++)
	    {
		if (drive_map_slot_empty (bios_drive_map[i]))
			break;	/* not found */
		if (bios_drive_map[i].from_drive == first_entry)
		{
			/* found */
			/* nothing to do if it is already the first entry */
			if (i == 0)
				break;
			/* backup this entry onto hooked_drive_map[0] */
			grub_memmove ((char *) &hooked_drive_map[0], (char *) &bios_drive_map[i], sizeof (struct drive_map_slot));
			/* move top entries downward */
			grub_memmove ((char *) &bios_drive_map[1], (char *) &bios_drive_map[0], sizeof (struct drive_map_slot) * i);
			/* restore this entry onto bios_drive_map[0] from hooked_drive_map[0] */
			grub_memmove ((char *) &bios_drive_map[0], (char *) &hooked_drive_map[0], sizeof (struct drive_map_slot));
			break;
		}
	    }
	}
	set_int13_handler (bios_drive_map);
	buf_drive = -1;
	buf_track = -1;
	return 1;
      }
    else if (grub_memcmp (arg, "--unhook", 8) == 0)
      {
	//if (unset_int13_handler (1))
	//	return ! (errnum = ERR_INT13_OFF_HOOK);
	unset_int13_handler (0);
//	int13_on_hook = 0;
	buf_drive = -1;
	buf_track = -1;
	return 1;
      }
    else if (grub_memcmp (arg, "--unmap=", 8) == 0)
      {
	int drive;
	char map_tmp[32];

	p = arg + 8;
	for (drive = 0xFF; drive >= 0; drive--)
	{
		if (drive != INITRD_DRIVE && in_range (p, drive)
		//#ifdef FSYS_FB
		//&& drive != FB_DRIVE
		//#endif
		)
		{
			/* unmap drive */
			sprintf (map_tmp, "(0x%X) (0x%X)", drive, drive);
			map_func (map_tmp/*, flags*/);
		}
	}
	buf_drive = -1;
	buf_track = -1;
	return 1;
      }
    else if (grub_memcmp (arg, "--rehook", 8) == 0)
      {
	//if (unset_int13_handler (1))
	//	return ! (errnum = ERR_INT13_OFF_HOOK);
	unset_int13_handler (0);
//	int13_on_hook = 0;
	if (drive_map_slot_empty (bios_drive_map[0]))
	    //if (atapi_dev_count == 0)
		return 1;//! (errnum = ERR_NO_DRIVE_MAPPED);
//	set_int13_handler (bios_drive_map);	/* backup bios_drive_map onto hooked_drive_map */
//	unset_int13_handler (0);	/* unhook it to avoid further access of hooked_drive_map by the call to map_func */
//	/* delete all memory mappings in hooked_drive_map */
//	for (i = 0; i < DRIVE_MAP_SIZE - 1; i++)
//	{
//	    while (hooked_drive_map[i].to_drive == 0xFF && !(hooked_drive_map[i].to_cylinder & 0x4000))
//	    {
//		grub_memmove ((char *) &hooked_drive_map[i], (char *) &hooked_drive_map[i + 1], sizeof (struct drive_map_slot) * (DRIVE_MAP_SIZE - i));
//	    }
//	}
	/* clear hooked_drive_map */
	grub_memset ((char *) hooked_drive_map, 0, sizeof (struct drive_map_slot) * (DRIVE_MAP_SIZE));

	/* re-create (topdown) all memory map entries in hooked_drive_map from bios_drive_map */
	for (j = 0; j < DRIVE_MAP_SIZE - 1; j++)
	{
	    unsigned long long top_start = 0;
	    unsigned long top_entry = DRIVE_MAP_SIZE;
	    /* find the top memory mapping in bios_drive_map */
	    for (i = 0; i < DRIVE_MAP_SIZE - 1; i++)
	    {
		if (bios_drive_map[i].to_drive == 0xFF && !(bios_drive_map[i].to_cylinder & 0x4000))
		{
			if (top_start < bios_drive_map[i].start_sector)
			{
			    top_start = bios_drive_map[i].start_sector;
			    top_entry = i;
			}
		}
	    }
	    if (top_entry >= DRIVE_MAP_SIZE)	/* not found */
		break;				/* end */
	    /* move it to hooked_drive_map, by a copy and a delete */
	    grub_memmove ((char *) &hooked_drive_map[j], (char *) &bios_drive_map[top_entry], sizeof (struct drive_map_slot));
	    grub_memmove ((char *) &bios_drive_map[top_entry], (char *) &bios_drive_map[top_entry + 1], sizeof (struct drive_map_slot) * (DRIVE_MAP_SIZE - top_entry));
	}
	/* now there should be no memory mappings in bios_drive_map. */

//	/* delete all memory mappings in bios_drive_map */
//	for (i = 0; i < DRIVE_MAP_SIZE - 1; i++)
//	{
//	    while (bios_drive_map[i].to_drive == 0xFF && !(bios_drive_map[i].to_cylinder & 0x4000))
//	    {
//		grub_memmove ((char *) &bios_drive_map[i], (char *) &bios_drive_map[i + 1], sizeof (struct drive_map_slot) * (DRIVE_MAP_SIZE - i));
//	    }
//	}
	/* re-create all memory mappings stored in hooked_drive_map */
	for (i = 0; i < DRIVE_MAP_SIZE - 1; i++)
	{
	    if (hooked_drive_map[i].to_drive == 0xFF && !(hooked_drive_map[i].to_cylinder & 0x4000))
	    {
		char tmp[128];
#ifndef NO_DECOMPRESSION
		int no_decompression_bak = no_decompression;
		int is64bit_bak = is64bit;
#endif
		sprintf (tmp, "--heads=%d --sectors-per-track=%d (md)0x%lX+0x%lX (0x%X)", (hooked_drive_map[i].max_head + 1), ((hooked_drive_map[i].max_sector) & 63), (unsigned long long)hooked_drive_map[i].start_sector, (unsigned long long)hooked_drive_map[i].sector_count, hooked_drive_map[i].from_drive);

		if (debug > 1)
		{
			printf ("Re-map the memdrive (0x%X):\n\tmap %s\n", hooked_drive_map[i].from_drive, tmp);
		}
		errnum = 0;
		disable_map_info = 1;
#ifndef NO_DECOMPRESSION
		if (hooked_drive_map[i].from_drive == INITRD_DRIVE)
		{
			no_decompression = 1;
			is64bit = 0;
		}
#endif
		map_func (tmp/*, flags*/);
#ifndef NO_DECOMPRESSION
		if (hooked_drive_map[i].from_drive == INITRD_DRIVE)
		{
			no_decompression = no_decompression_bak;
			is64bit = is64bit_bak;
		}
#endif
		disable_map_info = 0;

		if (errnum)
		{
			if (debug > 0)
			{
				printf ("Fatal: Error %d occurred while 'map %s'. Please report this bug.\n", errnum, tmp);
			}
			return 0;
		}

		//if (hooked_drive_map[i].from_drive == INITRD_DRIVE)
		//	linux_header->ramdisk_image = RAW_ADDR (initrd_start_sector << 9);
		/* change the map options */
		for (j = 0; j < DRIVE_MAP_SIZE - 1; j++)
		{
			if (bios_drive_map[j].from_drive == hooked_drive_map[i].from_drive && bios_drive_map[j].to_drive == 0xFF && !(bios_drive_map[j].to_cylinder & 0x4000))
			{
				bios_drive_map[j].max_head	= hooked_drive_map[i].max_head;
				bios_drive_map[j].max_sector	= hooked_drive_map[i].max_sector;
				bios_drive_map[j].to_cylinder	= hooked_drive_map[i].to_cylinder;
				bios_drive_map[j].to_head	= hooked_drive_map[i].to_head;
				bios_drive_map[j].to_sector	= hooked_drive_map[i].to_sector;
				break;
			}
		}
	    }
	}
	set_int13_handler (bios_drive_map);	/* hook it */
	buf_drive = -1;
	buf_track = -1;
	return 1;
      }
    else if (grub_memcmp (arg, "--floppies=", 11) == 0)
      {
	unsigned long long floppies;
	p = arg + 11;
	if (! safe_parse_maxint (&p, &floppies))
		return 0;
	if (floppies > 2)
		return ! (errnum = ERR_INVALID_FLOPPIES);
#ifndef GRUB_UTIL
	*((char *)0x410) &= 0x3e;
	if (floppies)
		*((char *)0x410) |= (floppies == 1) ? 1 : 0x41;
#endif
	return 1;
      }
    else if (grub_memcmp (arg, "--harddrives=", 13) == 0)
      {
	unsigned long long harddrives;
	p = arg + 13;
	if (! safe_parse_maxint (&p, &harddrives))
		return 0;
	if (harddrives > 127)
		return ! (errnum = ERR_INVALID_HARDDRIVES);
#ifndef GRUB_UTIL
	*((char *)0x475) = harddrives;
#endif
	return 1;
      }
    else if (grub_memcmp (arg, "--ram-drive=", 12) == 0)
      {
	unsigned long long tmp;
	p = arg + 12;
	if (! safe_parse_maxint (&p, &tmp))
		return 0;
	if (tmp > 254)
		return ! (errnum = ERR_INVALID_RAM_DRIVE);
	ram_drive = tmp;
	
	return 1;
      }
    else if (grub_memcmp (arg, "--rd-base=", 10) == 0)
      {
	unsigned long long tmp;
	p = arg + 10;
	if (! safe_parse_maxint/*_with_suffix*/ (&p, &tmp/*, 9*/))
		return 0;
//	if (tmp == 0xffffffff)
//		return ! (errnum = ERR_INVALID_RD_BASE);
	rd_base = tmp;
	
	return 1;
      }
    else if (grub_memcmp (arg, "--rd-size=", 10) == 0)
      {
	unsigned long long tmp;
	p = arg + 10;
	if (! safe_parse_maxint/*_with_suffix*/ (&p, &tmp/*, 9*/))
		return 0;
//	if (tmp == 0)
//		return ! (errnum = ERR_INVALID_RD_SIZE);
	rd_size = tmp;
	
	return 1;
      }
    else if (grub_memcmp (arg, "--memdisk-raw=", 14) == 0)
      {
	unsigned long long tmp;
	p = arg + 14;
	if (! safe_parse_maxint (&p, &tmp))
		return 0;
	memdisk_raw = tmp;
	return 1;
      }
    else if (grub_memcmp (arg, "--a20-keep-on=", 14) == 0)
      {
	unsigned long long tmp;
	p = arg + 14;
	if (! safe_parse_maxint (&p, &tmp))
		return 0;
	a20_keep_on = tmp;
	return 1;
      }
    else if (grub_memcmp (arg, "--safe-mbr-hook=", 16) == 0)
      {
	unsigned long long tmp;
	p = arg + 16;
	if (! safe_parse_maxint (&p, &tmp))
		return 0;
	safe_mbr_hook = tmp;
	return 1;
      }
    else if (grub_memcmp (arg, "--int13-scheme=", 15) == 0)
      {
	unsigned long long tmp;
	p = arg + 15;
	if (! safe_parse_maxint (&p, &tmp))
		return 0;
	int13_scheme = tmp;
	return 1;
      }
    else if (grub_memcmp (arg, "--mem-max=", 10) == 0)
      {
	unsigned long long num;
	p = arg + 10;
	if (! safe_parse_maxint/*_with_suffix*/ (&p, &num/*, 9*/))
		return 0;
	map_mem_max = (num<<9) & (-4096ULL); // convert to bytes, 4KB alignment, round down
	if (is64bit) {
	  if (map_mem_max==0)
	      map_mem_max = (-4096ULL);
	} else {
	  if (map_mem_max==0 || map_mem_max>(1ULL<<32))
	      map_mem_max = (1ULL<<32); // 4GB
	}
	grub_printf("map_mem_max = 0x%lX sectors = 0x%lX bytes\n",map_mem_max>>9,map_mem_max);
	return 1;
      }
    else if (grub_memcmp (arg, "--mem-min=", 10) == 0)
      {
	unsigned long long num;
	p = arg + 10;
	if (! safe_parse_maxint/*_with_suffix*/ (&p, &num/*, 9*/))
		return 0;
	map_mem_min = (((num<<9)+4095)&(-4096ULL)); // convert to bytes, 4KB alignment, round up
	if (map_mem_min < (1ULL<<20))
	    map_mem_min = (1ULL<<20); // 1MB
	grub_printf("map_mem_min = 0x%lX sectors = 0x%lX bytes\n",map_mem_min>>9,map_mem_min);
	return 1;
      }
    else if (grub_memcmp (arg, "--mem=", 6) == 0)
      {
	p = arg + 6;
	if (! safe_parse_maxint/*_with_suffix*/ (&p, &mem/*, 9*/))
		return 0;
	if (mem == -1ULL)
		mem = -2ULL;//return errnum = ERR_INVALID_MEM_RESERV;
      }
    else if (grub_memcmp (arg, "--mem", 5) == 0)
      {
	mem = 0;
      }
    else if (grub_memcmp (arg, "--top", 5) == 0)
      {
	prefer_top = 1;
      }
    else if (grub_memcmp (arg, "--read-only", 11) == 0)
      {
	if (read_Only || fake_write || unsafe_boot)
		return !(errnum = ERR_SPECIFY_RESTRICTION);
	read_Only = 1;
	unsafe_boot = 1;
      }
    else if (grub_memcmp (arg, "--fake-write", 12) == 0)
      {
	if (read_Only || fake_write || unsafe_boot)
		return !(errnum = ERR_SPECIFY_RESTRICTION);
	fake_write = 1;
	unsafe_boot = 1;
      }
    else if (grub_memcmp (arg, "--unsafe-boot", 13) == 0)
      {
	if (unsafe_boot)
		return !(errnum = ERR_SPECIFY_RESTRICTION);
	unsafe_boot = 1;
      }
    else if (grub_memcmp (arg, "--disable-chs-mode", 18) == 0)
      {
	disable_chs_mode = 1;
      }
    else if (grub_memcmp (arg, "--disable-lba-mode", 18) == 0)
      {
	disable_lba_mode = 1;
      }
    else if (grub_memcmp (arg, "--in-place", 10) == 0)
      {
	in_situ = 2;
      }
    else if (grub_memcmp (arg, "--in-situ", 9) == 0)
      {
	in_situ = 1;
      }
    else if (grub_memcmp (arg, "--heads=", 8) == 0)
      {
	p = arg + 8;
	if (! safe_parse_maxint (&p, &heads_per_cylinder))
		return 0;
	if ((unsigned long long)heads_per_cylinder > 256)
		return ! (errnum = ERR_INVALID_HEADS);
      }
    else if (grub_memcmp (arg, "--sectors-per-track=", 20) == 0)
      {
	p = arg + 20;
	if (! safe_parse_maxint (&p, &sectors_per_track))
		return 0;
	if ((unsigned long long)sectors_per_track > 63)
		return ! (errnum = ERR_INVALID_SECTORS);
      }
    else if (grub_memcmp (arg, "--add-mbt=", 10) == 0)
      {
	unsigned long long num;
	p = arg + 10;
	if (! safe_parse_maxint (&p, &num))
		return 0;
	add_mbt = num;
	if (add_mbt < -1 || add_mbt > 1)
		return 0;
      }
    else
	break;
    arg = skip_to (/*0, */arg);
  }
  
  to_drive = arg;
  from_drive = skip_to (/*0, */arg);

//  if (grub_memcmp (from_drive, "(hd+)", 5) == 0)
//  {
//	from = (unsigned char)(0x80 + (*(unsigned char *)0x475));
//  }
//  else
//  {
  /* Get the drive number for FROM_DRIVE.  */
  set_device (from_drive);
  if (errnum)
    return 0;
  from = current_drive;
//  }

  if (! (from & 0x80) && in_situ)
	return ! (errnum = ERR_IN_SITU_FLOPPY);
  /* Get the drive number for TO_DRIVE.  */
  filename = set_device (to_drive);

  if (errnum) {
	/* No device specified. Default to the root device. */
	current_drive = saved_drive;
	//current_partition = saved_partition;
	filename = 0;
	errnum = 0;
  }
  
  to = current_drive;
  //if (to == FB_DRIVE)
  //  to = (unsigned char)(fb_status >> 8)/* & 0xff*/;

  if (! (to & 0x80) && in_situ)
	return ! (errnum = ERR_IN_SITU_FLOPPY);

  /* if mem device is used, assume the --mem option */
  if (to == 0xffff || to == ram_drive || from == ram_drive)
  {
	if (mem == -1ULL)
		mem = 0;
  }

  if (mem != -1ULL && in_situ)
	return ! (errnum = ERR_IN_SITU_MEM);

  if (in_situ == 1)
  {
	unsigned long current_partition_bak;
       	char *filename_bak;
	char buf[16];

	current_partition_bak = current_partition;
	filename_bak = filename;
	
	/* read the first sector of drive TO */
	grub_sprintf (buf, "(%d)+1", to);
	if (! grub_open (buf))
		return 0;

	if (grub_read ((unsigned long long)SCRATCHADDR, SECTOR_SIZE, 0xedde0d90) != SECTOR_SIZE)
	{
		//grub_close ();

		/* This happens, if the file size is less than 512 bytes. */
		if (errnum == ERR_NONE)
			errnum = ERR_EXEC_FORMAT;
 
		return 0;
	}
	//grub_close ();

	current_partition = current_partition_bak;
	filename = filename_bak;

	/* try to find an empty entry in the partition table */
	for (i = 0; i < 4; i++)
	{
		if (! *(((char *)SCRATCHADDR) + 0x1C2 + i * 16))
			break;	/* consider partition type 00 as empty */
		if (! (*(((char *)SCRATCHADDR) + 0x1C0 + i * 16) & 63))
			break;	/* invalid start sector number of 0 */
		if (! (*(((char *)SCRATCHADDR) + 0x1C4 + i * 16) & 63))
			break;	/* invalid end sector number of 0 */
		if (! *(unsigned long *)(((char *)SCRATCHADDR) + 0x1C6 + i * 16))
			break;	/* invalid start LBA of 0 */
		if (! *(unsigned long *)(((char *)SCRATCHADDR) + 0x1CA + i * 16))
			break;	/* invalid sector count of 0 */
	}
	
	if (i >= 4)	/* No empty entry. The partition table is full. */
		return ! (errnum = ERR_PARTITION_TABLE_FULL);
  }
  
  if ((current_partition == 0xFFFFFF || (to >= 0x80 && to <= 0xFF)) && filename && (*filename == 0x20 || *filename == 0x09))
    {
      if (to == 0xffff /* || to == ram_drive */)
      {
	if (((long long)mem) <= 0)
	{
	  //grub_printf ("When mapping whole mem device at a fixed location, you must specify --mem to a value > 0.\n");
	  return ! (errnum = ERR_MD_BASE);
	}
	start_sector = (unsigned long long)mem;
	sector_count = 1;
	//goto map_whole_drive;
      }else if (to == ram_drive)
      {
	/* always consider this to be a fixed memory mapping */
	if ((rd_base & 0x1ff) || ! rd_base)
		return ! (errnum = ERR_RD_BASE);
	to = 0xffff;
	mem = (rd_base >> SECTOR_BITS);
	start_sector = (unsigned long long)mem;
	sector_count = 1;
	//goto map_whole_drive;
      }else{
        /* when whole drive is mapped, the geometry should not be specified. */
        if ((long long)heads_per_cylinder > 0 || (long long)sectors_per_track > 0)
	  return ! (errnum = ERR_SPECIFY_GEOM);
        /* when whole drive is mapped, the mem option should not be specified. 
         * but when we delete a drive map slot, the mem option means force.
         */
        if (mem != -1ULL && to != from)
	  return ! (errnum = ERR_SPECIFY_MEM);

        sectors_per_track = 1;/* 1 means the specified geometry will be ignored. */
        heads_per_cylinder = 1;/* can be any value but ignored since #sectors==1. */
        /* Note: if the user do want to specify geometry for whole drive map, then
         * use a command like this:
         * 
         * map --heads=H --sectors-per-track=S (hd0)+1 (hd1)
         * 
         * where S > 1
         */
        goto map_whole_drive;
      }
    }

  if (mem == -1ULL)
  {
    query_block_entries = -1; /* query block list only */
    blocklist_func (to_drive/*, flags*/);
    if (errnum)
	return 0;
    if (query_block_entries != 1 && mem == -1ULL)
	return ! (errnum = ERR_NON_CONTIGUOUS);

    start_sector = map_start_sector; /* in big 2048-byte sectors */
    //sector_count = map_num_sectors;
    sector_count = (filemax + 0x1ff) >> SECTOR_BITS; /* in small 512-byte sectors */

    if (start_sector == part_start && part_start && sector_count == 1)
	sector_count = part_length;
  }

  if ((to == 0xffff /* || to == ram_drive */) && sector_count == 1)
  {
    /* fixed memory mapping */
    grub_memmove ((void *) BS, (void *)(int)(start_sector << 9), SECTOR_SIZE);
  }else{
    if (! grub_open (to_drive))
	return 0;
    start_sector = sector_count = 0;
//    rawread_ignore_memmove_overflow = 1;
    disk_read_hook = disk_read_start_sector_func;
    /* Read the first sector of the emulated disk.  */
    err = grub_read ((unsigned long long)(unsigned long) BS, SECTOR_SIZE, 0xedde0d90);
    disk_read_hook = 0;
//    rawread_ignore_memmove_overflow = 0;
    if (err != SECTOR_SIZE && from != ram_drive)
    {
      //grub_close ();
      /* This happens, if the file size is less than 512 bytes. */
      if (errnum == ERR_NONE)
	  errnum = ERR_EXEC_FORMAT;
      return 0;
    }

    //to_filesize = gzip_filemax;
    sector_count = (filemax + 0x1ff) >> SECTOR_BITS; /* in small 512-byte sectors */
    if (start_sector == part_start /* && part_start */ && sector_count == 1)
    {
      if (mem != -1ULL)
      {
	char buf[32];

	//grub_close ();
	sector_count = part_length;
        grub_sprintf (buf, "(%d)%ld+%ld", to, (unsigned long long)part_start, (unsigned long long)part_length);
        if (! grub_open (buf))
		return 0;
        filepos = SECTOR_SIZE;
      } else if (part_start) {
	sector_count = part_length;
      }
    }

    //if (mem == -1ULL)
    //  grub_close ();
    if (to == 0xffff && sector_count == 1)
    {
      grub_printf ("For mem file in emulation, you should not specify sector_count to 1.\n");
      return 0;
    }
  }

  /* measure start_sector in small 512-byte sectors */
//  if (buf_geom.sector_size == 2048)
//	start_sector *= 4;

  if (filemax < 512 || BS->boot_signature != 0xAA55)
	goto geometry_probe_failed;
  
  /* probe the BPB */
  
  if (probe_bpb(BS))
	goto failed_probe_BPB;
  
  if (debug > 0 && ! disable_map_info)
    grub_printf ("%s BPB found %s 0xEB (jmp) leading the boot sector.\n", 
		filesystem_type == 1 ? "FAT12" :
		filesystem_type == 2 ? "FAT16" :
		filesystem_type == 3 ? "FAT32" :
		filesystem_type == 4 ? "NTFS"  :
		/*filesystem_type == 5 ?*/ "EXT2 GRLDR",
		(BS->dummy1[0] == (char)0xEB ? "with" : "but WITHOUT"));
  
  if (sector_count != 1)
    {
      if (debug > 0 && ! disable_map_info)
      {
	if (probed_total_sectors > sector_count)
	  grub_printf ("Warning: BPB total_sectors(%ld) is greater than the number of sectors in the whole disk image (%ld). The int13 handler will disable any read/write operations across the image boundary. That means you will not be able to read/write sectors (in absolute address, i.e., lba) %ld - %ld, though they are logically inside your file system.\n", (unsigned long long)probed_total_sectors, (unsigned long long)sector_count, (unsigned long long)sector_count, (unsigned long long)(probed_total_sectors - 1));
	else if (probed_total_sectors < sector_count && ! disable_map_info)
	  grub_printf ("info: BPB total_sectors(%ld) is less than the number of sectors in the whole disk image(%ld).\n", (unsigned long long)probed_total_sectors, (unsigned long long)sector_count);
      }
    }

  if (mem == -1ULL && (from & 0x80) && (to & 0x80) && (start_sector == part_start && part_start && sector_count == part_length)/* && BS->hidden_sectors >= probed_sectors_per_track */)
  {
	if (BS->hidden_sectors <= probed_sectors_per_track)
		BS->hidden_sectors = (unsigned long)part_start;
	extended_part_start = BS->hidden_sectors - probed_sectors_per_track;
	extended_part_length = probed_total_sectors + probed_sectors_per_track;
	if (debug > 0 && ! disable_map_info)
	  grub_printf ("\nBPB probed C/H/S = %d/%d/%d, BPB probed total sectors = %ld\n", probed_cylinders, probed_heads, probed_sectors_per_track, (unsigned long long)probed_total_sectors);
	BPB_H = probed_heads;
	BPB_S = probed_sectors_per_track;
	if (in_situ)
	{
		//start_sector += probed_sectors_per_track;
		//sector_count -= probed_sectors_per_track;
		goto geometry_probe_ok;
	}
		
	if (debug > 0 && ! disable_map_info)
	  grub_printf ("Try to locate extended partition (hd%d)%d+%d for the virtual (hd%d).\n", (to & 0x7f), extended_part_start, extended_part_length, (from & 0x7f));
	grub_sprintf ((char *)BS, "(hd%d)%d+%d", (to & 0x7f), extended_part_start, extended_part_length);
	//j = filesystem_type;	/* save filesystem_type */
	if (! grub_open ((char *)BS))
		return 0;

	//filesystem_type = j;	/* restore filesystem_type */
	/* Read the first sector of the emulated disk.  */
	if (grub_read ((unsigned long long)(unsigned int) BS, SECTOR_SIZE, 0xedde0d90) != SECTOR_SIZE)
	{
		//grub_close ();

		/* This happens, if the file size is less than 512 bytes. */
		if (errnum == ERR_NONE)
			errnum = ERR_EXEC_FORMAT;
 
		return 0;
	}
	//grub_close ();
	for (i = 0; i < 4; i++)
	{
		unsigned char sys = BS->P[i].system_indicator;
		if (sys == 0x05 || sys == 0x0f || sys == 0)
		{
			// *(long long *)&(BS->P[i].boot_indicator) = 0LL;
			((long long *)&(BS->P[i]))[0] = // 0LL;
			// *(long long *)&(BS->P[i].start_lba) = 0LL;
			((long long *)&(BS->P[i]))[1] = 0LL;
		}
	}
	part_start = extended_part_start;
	part_length = extended_part_length;
	start_sector = extended_part_start;
	sector_count = extended_part_length;
	
	/* when emulating a hard disk using a logical partition, the geometry should not be specified. */
	if ((long long)heads_per_cylinder > 0 || (long long)sectors_per_track > 0)
	  return ! (errnum = ERR_SPECIFY_GEOM);

	goto failed_probe_BPB;
  }
  //if (probed_cylinders * sectors_per_cylinder == probed_total_sectors)
	goto geometry_probe_ok;

failed_probe_BPB:
  /* probe the partition table */
  
  if (probe_mbr (BS, start_sector, sector_count, (unsigned long)part_start))
	goto geometry_probe_failed;

  if (sector_count != 1)
    {
      if (probed_total_sectors < probed_total_sectors_round && probed_total_sectors_round <= sector_count)
	  probed_total_sectors = probed_total_sectors_round;
      if (debug > 0 && ! disable_map_info)
      {
	if (probed_total_sectors > sector_count)
	  grub_printf ("Warning: total_sectors calculated from partition table(%ld) is greater than the number of sectors in the whole disk image (%ld). The int13 handler will disable any read/write operations across the image boundary. That means you will not be able to read/write sectors (in absolute address, i.e., lba) %ld - %ld, though they are logically inside your emulated virtual disk(according to the partition table).\n", (unsigned long long)probed_total_sectors, (unsigned long long)sector_count, (unsigned long long)sector_count, (unsigned long long)(probed_total_sectors - 1));
	else if (probed_total_sectors < sector_count)
	  grub_printf ("info: total_sectors calculated from partition table(%ld) is less than the number of sectors in the whole disk image(%ld).\n", (unsigned long long)probed_total_sectors, (unsigned long long)sector_count);
      }
    }
  
  goto geometry_probe_ok;

geometry_probe_failed:

  if (BPB_H || BPB_S)
  {
	//if (mem != -1ULL)
	//	grub_close ();
	return ! (errnum = ERR_EXTENDED_PARTITION);
  }

  /* possible ISO9660 image */
  if (filemax < 512 || BS->boot_signature != 0xAA55 || (unsigned char)from >= (unsigned char)0xA0)
  {
	if ((long long)heads_per_cylinder < 0)
		heads_per_cylinder = 0;
	if ((long long)sectors_per_track < 0)
		sectors_per_track = 0;
  }

  if ((long long)heads_per_cylinder < 0)
    {
	if (from & 0x80)
	{
		//if (mem != -1ULL)
		//	grub_close ();
		return ! (errnum = ERR_NO_HEADS);
	}
	/* floppy emulation */
	switch (sector_count)
	  {
		/* for the 5 standard floppy disks, offer a value */
		case  720: /*  360K */
		case 1440: /*  720K */
		case 2400: /* 1200K */
		case 2880: /* 1440K */
		case 5760: /* 2880K */
			heads_per_cylinder = 2; break;
		default:
			if (sector_count > 1 && sector_count < 5760 )
			  {
				heads_per_cylinder = 2; break;
			  }
			else
			  {
				//if (mem != -1ULL)
				//	grub_close ();
				return ! (errnum = ERR_NO_HEADS);
			  }
	  }
	if (debug > 0 && ! disable_map_info)
	  grub_printf ("\nAutodetect number-of-heads failed. Use default value %d\n", heads_per_cylinder);
    }
  else if (heads_per_cylinder == 0)
    {
	if (from & 0x80)
	  {
		/* hard disk emulation */
		heads_per_cylinder = 255; /* 256 */
	  }
	else
	  {
		/* floppy emulation */
		switch (sector_count)
		  {
			case 320: /* 160K */
			case 360: /* 180K */
			case 500: /* 250K */
				heads_per_cylinder = 1; break;
			case 1: /* a whole-disk emulation */
				heads_per_cylinder = 255; break;
			default:
				if (sector_count <= 63 * 1024 * 2 + 1)
					heads_per_cylinder = 2;
				else
					heads_per_cylinder = 255;
		  }
	  }
	if (debug > 0 && ! disable_map_info)
	  if (from < 0xA0)
	    grub_printf ("\nAutodetect number-of-heads failed. Use default value %d\n", heads_per_cylinder);
    }
  else
    {
	if (debug > 0 && ! disable_map_info)
	  grub_printf ("\nAutodetect number-of-heads failed. Use the specified %d\n", heads_per_cylinder);
    }

  if ((long long)sectors_per_track < 0)
    {
	if (from & 0x80)
	{
		//if (mem != -1ULL)
		//	grub_close ();
		return ! (errnum = ERR_NO_SECTORS);
	}
	/* floppy emulation */
	switch (sector_count)
	  {
		/* for the 5 standard floppy disks, offer a value */
		case  720: /*  360K */
		case 1440: /*  720K */
			sectors_per_track = 9; break;
		case 2400: /* 1200K */
			sectors_per_track = 15; break;
		case 2880: /* 1440K */
			sectors_per_track = 18; break;
		case 5760: /* 2880K */
			sectors_per_track = 36; break;
		default:
			if (sector_count > 1 && sector_count < 2880 )
			  {
				sectors_per_track = 18; break;
			  }
			else if (sector_count > 2880 && sector_count < 5760 )
			  {
				sectors_per_track = 36; break;
			  }
			else
			  {
				//if (mem != -1ULL)
				//	grub_close ();
				return ! (errnum = ERR_NO_SECTORS);
			  }
	  }
	if (debug > 0 && ! disable_map_info)
	  grub_printf ("\nAutodetect sectors-per-track failed. Use default value %d\n", sectors_per_track);
    }
  else if (sectors_per_track == 0)
    {
	if (sector_count == 1)
		sectors_per_track = 1;
	else if (from & 0x80)
	  {
		/* hard disk emulation */
		sectors_per_track = 63;
	  }
	else
	  {
		/* floppy emulation */
		switch (sector_count)
		  {
			case  400: /*  200K */
				sectors_per_track = 5; break;
			case  320: /*  160K */
			case  640: /*  320K */
			case 1280: /*  640K */
				sectors_per_track = 8; break;
			case  360: /*  180K */
			case  720: /*  360K */
			case 1440: /*  720K */
			case 1458: /*  729K */
			case 1476: /*  738K */
			case 1494: /*  747K */
			case 1512: /*  756K */
				sectors_per_track = 9; break;
			case  500: /*  250K */
			case  800: /*  400K */
			case  840: /*  420K */
			case 1000: /*  500K */
			case 1600: /*  800K */
			case 1620: /*  810K */
			case 1640: /*  820K */
			case 1660: /*  830K */
			case 1680: /*  840K */
				sectors_per_track = 10; break;
			case 1804: /*  902K */
				sectors_per_track = 11; break;
			case 1968: /*  984K */
				sectors_per_track = 12; break;
			case 2132: /* 1066K */
				sectors_per_track = 13; break;
			case 2400: /* 1200K */
			case 2430: /* 1215K */
			case 2460: /* 1230K */
			case 2490: /* 1245K */
			case 2520: /* 1260K */
				sectors_per_track = 15; break;
			case 2720: /* 1360K */
			case 2754: /* 1377K */
			case 2788: /* 1394K */
			case 2822: /* 1411K */
			case 2856: /* 1428K */
				sectors_per_track = 17; break;
			case 2880: /* 1440K */
			case 2916: /* 1458K */
			case 2952: /* 1476K */
			case 2988: /* 1494K */
			case 3024: /* 1512K */
				sectors_per_track = 18; break;
			case 3116: /* 1558K */
				sectors_per_track = 19; break;
			case 3200: /* 1600K */
			case 3240: /* 1620K */
			case 3280: /* 1640K */
			case 3320: /* 1660K */
				sectors_per_track = 20; break;
			case 3360: /* 1680K */
			case 3402: /* 1701K */
			case 3444: /* 1722K */
			case 3486: /* 1743K */
			case 3528: /* 1764K */
				sectors_per_track = 21; break;
			case 3608: /* 1804K */
				sectors_per_track = 22; break;
			case 3772: /* 1886K */
				sectors_per_track = 23; break;
			case 5760: /* 2880K */
				sectors_per_track = 36; break;
			case 6396: /* 3198K */
				sectors_per_track = 39; break;
			case 7216: /* 3608K */
				sectors_per_track = 44; break;
			case 7380: /* 3690K */
				sectors_per_track = 45; break;
			case 7544: /* 3772K */
				sectors_per_track = 46; break;
			default:
				if (sector_count <= 2881)
					sectors_per_track = 18;
				else if (sector_count <= 5761)
					sectors_per_track = 36;
				else
					sectors_per_track = 63;
		  }
	  }
	if (debug > 0 && ! disable_map_info)
	  if (from < 0xA0)
	    grub_printf ("\nAutodetect sectors-per-track failed. Use default value %d\n", sectors_per_track);
    }
  else
    {
	if (debug > 0 && ! disable_map_info)
	  grub_printf ("\nAutodetect sectors-per-track failed. Use the specified %d\n", sectors_per_track);
    }
  goto map_whole_drive;

geometry_probe_ok:
  
  if (! disable_map_info)
  {
    if (debug > 0)
      grub_printf ("\nprobed C/H/S = %d/%d/%d, probed total sectors = %ld\n", probed_cylinders, probed_heads, probed_sectors_per_track, (unsigned long long)probed_total_sectors);
    if (mem != -1ULL && ((long long)mem) <= 0)
    {
      if (((unsigned long long)(-mem)) < probed_total_sectors && probed_total_sectors > 1 && filemax >= 512)
	mem = - (unsigned long long)probed_total_sectors;
    }
  }
  if (BPB_H || BPB_S)
	if (BPB_H != probed_heads || BPB_S != probed_sectors_per_track)
	{
		//if (mem != -1ULL)
		//	grub_close ();
		return ! (errnum = ERR_EXTENDED_PARTITION);
	}
  
  if ((long long)heads_per_cylinder <= 0)
	heads_per_cylinder = probed_heads;
  else if (heads_per_cylinder != probed_heads)
    {
	if (debug > 0 && ! disable_map_info)
	  grub_printf ("\nWarning!! Probed number-of-heads(%d) is not the same as you specified.\nThe specified value %d takes effect.\n", probed_heads, heads_per_cylinder);
    }

  if ((long long)sectors_per_track <= 0)
	sectors_per_track = probed_sectors_per_track;
  else if (sectors_per_track != probed_sectors_per_track)
    {
	if (debug > 0 && ! disable_map_info)
	  grub_printf ("\nWarning!! Probed sectors-per-track(%d) is not the same as you specified.\nThe specified value %d takes effect.\n", probed_sectors_per_track, sectors_per_track);
    }

map_whole_drive:

  if (from != ram_drive)
  {
    /* Search for an empty slot in BIOS_DRIVE_MAP.  */
    for (i = 0; i < DRIVE_MAP_SIZE; i++)
    {
      /* Perhaps the user wants to override the map.  */
      if (bios_drive_map[i].from_drive == from)
	break;
      
      if (drive_map_slot_empty (bios_drive_map[i]))
	break;
    }

    if (i == DRIVE_MAP_SIZE)
    {
      //if (mem != -1ULL)
      //  grub_close ();
      errnum = ERR_WONT_FIT;
      return 0;
    }

    /* If TO == FROM and whole drive is mapped, and, no map options occur, then delete the entry.  */
    if (to == from && read_Only == 0 && fake_write == 0 && disable_lba_mode == 0
      && disable_chs_mode == 0 && start_sector == 0 && (sector_count == 0 ||
	/* sector_count == 1 if the user uses a special method to map a whole drive, e.g., map (hd1)+1 (hd0) */
     (sector_count == 1 && (long long)heads_per_cylinder <= 0 && (long long)sectors_per_track <= 1)))
	goto delete_drive_map_slot;
  }

  /* check whether TO is being mapped */
  //if (mem == -1)
  for (j = 0; j < DRIVE_MAP_SIZE; j++)
    {
      if (unset_int13_handler (1))
	  continue;	/* not hooked */
      if (to != hooked_drive_map[j].from_drive)
	  continue;
#if 0
	  if (! (hooked_drive_map[j].max_sector & 0x3F))
	      disable_chs_mode = 1;

	  /* X=max_sector bit 7: read only or fake write */
	  /* Y=to_sector  bit 6: safe boot or fake write */
	  /* ------------------------------------------- */
	  /* X Y: meaning of restrictions imposed on map */
	  /* ------------------------------------------- */
	  /* 1 1: read only=0, fake write=1, safe boot=0 */
	  /* 1 0: read only=1, fake write=0, safe boot=0 */
	  /* 0 1: read only=0, fake write=0, safe boot=1 */
	  /* 0 0: read only=0, fake write=0, safe boot=0 */

	  if (!(read_Only | fake_write | unsafe_boot))	/* no restrictions specified */
	  switch ((hooked_drive_map[j].max_sector & 0x80) | (hooked_drive_map[j].to_sector & 0x40))
	  {
		case 0xC0: read_Only = 0; fake_write = 1; unsafe_boot = 1; break;
		case 0x80: read_Only = 1; fake_write = 0; unsafe_boot = 1; break;
		case 0x00: read_Only = 0; fake_write = 0; unsafe_boot = 1; break;
	      /*case 0x40:*/
		default:   read_Only = 0; fake_write = 0; unsafe_boot = 0;
	  }

	  if (hooked_drive_map[j].max_sector & 0x40)
	      disable_lba_mode = 1;
#endif

	  to = hooked_drive_map[j].to_drive;
	  if (to == 0xFF && !(hooked_drive_map[j].to_cylinder & 0x4000))
		to = 0xFFFF;		/* memory device */
	  if (start_sector == 0 && (sector_count == 0 || (sector_count == 1 && (long long)heads_per_cylinder <= 0 && (long long)sectors_per_track <= 1)))
	  {
		sector_count = hooked_drive_map[j].sector_count;
		heads_per_cylinder = hooked_drive_map[j].max_head + 1;
		sectors_per_track = (hooked_drive_map[j].max_sector) & 0x3F;
	  }
	  start_sector += hooked_drive_map[j].start_sector;
      
	  /* If TO == FROM and whole drive is mapped, and, no map options occur, then delete the entry.  */
	  if (to == from && read_Only == 0 && fake_write == 0 && disable_lba_mode == 0
	      && disable_chs_mode == 0 && start_sector == 0 && (sector_count == 0 ||
	  /* sector_count == 1 if the user uses a special method to map a whole drive, e.g., map (hd1)+1 (hd0) */
	     (sector_count == 1 && (long long)heads_per_cylinder <= 0 && (long long)sectors_per_track <= 1)))
	    {
		/* yes, delete the FROM drive(with slot[i]), not the TO drive(with slot[j]) */
		if (from != ram_drive)
			goto delete_drive_map_slot;
	    }

	  break;
    }

  j = i;	/* save i into j */
//grub_printf ("\n debug 4 start_sector=%lX, part_start=%lX, part_length=%lX, sector_count=%lX, filemax=%lX\n", start_sector, part_start, part_length, sector_count, filemax);
  
#ifndef GRUB_UTIL
  /* how much memory should we use for the drive emulation? */
  if (mem != -1ULL)
    {
      unsigned long long start_byte;
      unsigned long long bytes_needed;
      unsigned long long base;
      unsigned long long top_end;
      
      bytes_needed = base = top_end = 0ULL;
//if (to == 0xff)
//{
//	
//}else{
      if (start_sector == part_start && part_start == 0 && sector_count == 1)
		sector_count = part_length;

      /* For GZIP disk image if uncompressed size >= 4GB, 
         high bits of filemax is wrong, sector_count is also wrong. */
      #if 0
      if ( compressed_file && (sector_count < probed_total_sectors) && filesystem_type > 0 )
      {   /* adjust high bits of filemax and sector_count */
	  unsigned long sizehi = (unsigned long)(probed_total_sectors >> (32-SECTOR_BITS));
	  if ( (unsigned long)filemax < (unsigned long)(probed_total_sectors << SECTOR_BITS) )
	      ++sizehi;
	  fsmax = filemax += (unsigned long long)sizehi << 32;
	  sector_count = filemax >> SECTOR_BITS;
	  grub_printf("Uncompressed size is probably %ld sectors\n",sector_count);
      }
		#endif
      if ( (long long)mem < 0LL && sector_count < (-mem) )
	bytes_needed = (-mem) << SECTOR_BITS;
      else
	bytes_needed = sector_count << SECTOR_BITS;

      /* filesystem_type
       *	 0		an MBR device
       *	 1		FAT12
       *	 2		FAT16
       *	 3		FAT32
       *	 4		NTFS
       *	-1		unknown filesystem(do not care)
       *	
       * Note: An MBR device is a whole disk image that has a partition table.
       */

      if (add_mbt<0)
	  add_mbt = (filesystem_type > 0 && (from & 0x80) && (from < 0xA0))? 1: 0; /* known filesystem without partition table */

      if (add_mbt)
	bytes_needed += sectors_per_track << SECTOR_BITS;	/* build the Master Boot Track */

      bytes_needed = ((bytes_needed+4095)&(-4096ULL));	/* 4KB alignment */
//}
      if ((to == 0xffff || to == ram_drive) && sector_count == 1)
	/* mem > 0 */
	bytes_needed = 0;
      //base = 0;

      start_byte = start_sector << SECTOR_BITS;
      if (to == ram_drive)
	start_byte += rd_base;
  /* we can list the total memory thru int15/E820. on the other hand, we can
   * see thru our drive map slots how much memory have already been used for
   * drive emulation. */
      /* the available memory can be calculated thru int15 and the slots. */
      /* total memory can always be calculated thru unhooked int15 */
      /* used memory can always be calculated thru drive map slots */
      /* the int15 handler:
       *   if not E820, return to the original int15
       *   for each E820 usable memory, check how much have been used in slot,
       *     and return the usable memory minus used memory.
       */
      /* once int13 hooked and at least 1 slot uses mem==1, also hook int15
       */

      if (map_mem_max > 0x100000000ULL && ! is64bit)
	  map_mem_max = 0x100000000ULL;
      if (map_mem_min < 0x100000ULL)
	  map_mem_min = 0x100000ULL;

      {
#if 1
	unsigned long ret;
	struct realmode_regs regs;

	regs.ebx = 0;	// start the iteration
	for (;;)
	{
	  unsigned long long tmpbase, tmpend, tmpmin;

	  regs.eax = 0xE820;
	  regs.ecx = 20;
	  regs.edx = 0x534D4150;	// SMAP
	  regs.es = 0;		// ES:DI = buffer
	  regs.edi = 0x580;	// ES:DI = buffer
	  regs.ds = regs.es = regs.fs = regs.gs = regs.eflags = -1;
	  regs.cs = -1;		// CS=0xFFFFFFFF to run an INT instruction
	  regs.eip = 0xFFFF15CD;	// run int15
	  ret = realmode_run ((unsigned long)&regs);
	  if (regs.eax != 0x534D4150)
	  {
		grub_printf ("\nFatal: Your BIOS has no support for System Memory Map(INT15/EAX=E820h).\nAs a result you cannot use the --mem option.\n");
		break;
	  }
	  if (regs.eflags & 1)
		break;

	      if (*(long *)(0x580 + 16) != 1)	// address type
		  continue;
	      tmpmin = (*(long long *)0x580 > map_mem_min) ? //base addr
			*(long long *)0x580 : map_mem_min;
	      tmpmin = ((tmpmin+4095)&(-4096ULL));/* 4KB alignment, round up */
	      tmpend = (*(long long *)0x580) + (*(long long *)(0x580 + 8));
	      if (tmpend > map_mem_max)
		  tmpend = map_mem_max;
	      tmpend &= (-4096ULL);	/* 4KB alignment, round down */
	      if (tmpend < bytes_needed)
		  continue;
	      tmpbase = tmpend - bytes_needed; // maximum possible base for this region
	      //grub_printf("range b %lx l %lx -- tm %lx te %lx tb %lx\n",map->BaseAddr,map->Length,tmpmin,tmpend,tmpbase);
	      if (((long long)mem) > 0)//force the base value
	      {
		  if (tmpbase >= (mem << 9))
		      tmpbase =  (mem << 9); // move tmpbase down to forced value
		  else // mem base exceed maximum possible base for this region
		      continue;
	      }
	      if (tmpbase < tmpmin) // base fall below minimum address for this region
		  continue;

	      for (i = 0; i < DRIVE_MAP_SIZE; i++)
	        {
      
		  if (drive_map_slot_empty (bios_drive_map[i]))
			break;

		  /* TO_DRIVE == 0xFF indicates a MEM map */
		  if (bios_drive_map[i].to_drive == 0xFF && !(bios_drive_map[i].to_cylinder & 0x4000))
	            {
		      unsigned long long drvbase, drvend;
		      drvbase = (bios_drive_map[i].start_sector << SECTOR_BITS);
		      drvend  = (bios_drive_map[i].sector_count << SECTOR_BITS) + drvbase;
		      drvend  = ((drvend+4095)&(-4096ULL));/* 4KB alignment, round up */
		      drvbase &= (-4096ULL);	/* 4KB alignment, round down */
		      //grub_printf("drv %02x: db %lx de %lx -- tb %lx te %lx\n",bios_drive_map[i].from_drive,drvbase,drvend,tmpbase,tmpend);
		      if (tmpbase < drvend && drvbase < tmpend)
			{ // overlapped address, move tmpend and tmpbase down
			  tmpend = drvbase;
			  if (tmpend >= bytes_needed)
			  {
			      if ((long long)mem <= 0)
			      { // decrease tmpbase
				  tmpbase = tmpend - bytes_needed;
				  if (tmpbase >= tmpmin)
				  { // recheck new tmpbase 
				      i = -1; continue;
				  }
			      }
			      else 
			      { // force base value, cannot decrease tmpbase
				  /* changed "<" to "<=". -- tinybit */
				  if (tmpbase <= tmpend - bytes_needed)
				  { // there is still enough space below drvbase
				      continue;
				  }
			      }
			  }
			  /* not enough room for this memory area */
			//  grub_printf("region can't be used bn %lx te %lx  tb %lx\n",bytes_needed,tmpend,tmpbase);
			  tmpbase = 0; break;
			}
		    }
	        } /* for (i = 0; i < DRIVE_MAP_SIZE; i++) */

	      if (tmpbase >= tmpmin)
	      {
		/* check if the memory area overlaps the (md)... or (rd)... file. */
//		if ( (to != 0xffff && to != ram_drive)	/* TO is not in memory */
//		  || ((long long)mem) > 0	/* TO is in memory but is mapped at a fixed location */
//		  || !compressed_file		/* TO is in memory and is normally mapped and uncompressed */
//		/* Now TO is in memory and is normally mapped and compressed */
//		  || tmpbase + bytes_needed <= start_byte  /* XXX: destination is below the gzip image */
//		  || tmpbase >= start_byte + to_filesize) /* XXX: destination is above the gzip image */
//		{
		    if (prefer_top)
		    {
			if (base < tmpbase)
			{
			    base = tmpbase; top_end = tmpend;
			}
			continue; // find available region with highest address
		    }
		    else
		    {
			base = tmpbase; top_end = tmpend;
			break; // use the first available region
		    }
//		}
//		else
//		    /* destination overlaps the gzip image */
//		    /* fail and try next memory block. */
//		    continue;
	      }
	      //if (to == 0xff)
		//bytes_needed = 0;

	  if (! regs.ebx)
		break;
	}
#else
      if (mbi.flags & MB_INFO_MEM_MAP)
        {
          struct AddrRangeDesc *map = (struct AddrRangeDesc *) saved_mmap_addr;
          unsigned long end_addr = saved_mmap_addr + saved_mmap_length;

          for (; end_addr > (unsigned long) map; map = (struct AddrRangeDesc *) (((int) map) + 4 + map->size))
	    {
	      unsigned long long tmpbase, tmpend, tmpmin;
	      if (map->Type != MB_ARD_MEMORY)
		  continue;
	      tmpmin = (map->BaseAddr > map_mem_min) ?
			map->BaseAddr : map_mem_min;
	      tmpmin = ((tmpmin+4095)&(-4096ULL));/* 4KB alignment, round up */
	      tmpend = (map->BaseAddr + map->Length < map_mem_max) ?
			map->BaseAddr + map->Length : map_mem_max;
	      tmpend &= (-4096ULL);	/* 4KB alignment, round down */
	      if (tmpend < bytes_needed)
		  continue;
	      tmpbase = tmpend - bytes_needed; // maximum possible base for this region
	      //grub_printf("range b %lx l %lx -- tm %lx te %lx tb %lx\n",map->BaseAddr,map->Length,tmpmin,tmpend,tmpbase);
	      if (((long long)mem) > 0)//force the base value
	      {
		  if (tmpbase >= (mem << 9))
		      tmpbase =  (mem << 9); // move tmpbase down to forced value
		  else // mem base exceed maximum possible base for this region
		      continue;
	      }
	      if (tmpbase < tmpmin) // base fall below minimum address for this region
		  continue;

	      for (i = 0; i < DRIVE_MAP_SIZE; i++)
	        {
      
		  if (drive_map_slot_empty (bios_drive_map[i]))
			break;

		  /* TO_DRIVE == 0xFF indicates a MEM map */
		  if (bios_drive_map[i].to_drive == 0xFF && !(bios_drive_map[i].to_cylinder & 0x4000))
	            {
		      unsigned long long drvbase, drvend;
		      drvbase = (bios_drive_map[i].start_sector << SECTOR_BITS);
		      drvend  = (bios_drive_map[i].sector_count << SECTOR_BITS) + drvbase;
		      drvend  = ((drvend+4095)&(-4096ULL));/* 4KB alignment, round up */
		      drvbase &= (-4096ULL);	/* 4KB alignment, round down */
		      //grub_printf("drv %02x: db %lx de %lx -- tb %lx te %lx\n",bios_drive_map[i].from_drive,drvbase,drvend,tmpbase,tmpend);
		      if (tmpbase < drvend && drvbase < tmpend)
			{ // overlapped address, move tmpend and tmpbase down
			  tmpend = drvbase;
			  if (tmpend >= bytes_needed)
			  {
			      if ((long long)mem <= 0)
			      { // decrease tmpbase
				  tmpbase = tmpend - bytes_needed;
				  if (tmpbase >= tmpmin)
				  { // recheck new tmpbase 
				      i = -1; continue;
				  }
			      }
			      else 
			      { // force base value, cannot decrease tmpbase
				  /* changed "<" to "<=". -- tinybit */
				  if (tmpbase <= tmpend - bytes_needed)
				  { // there is still enough space below drvbase
				      continue;
				  }
			      }
			  }
			  /* not enough room for this memory area */
			//  grub_printf("region can't be used bn %lx te %lx  tb %lx\n",bytes_needed,tmpend,tmpbase);
			  tmpbase = 0; break;
			}
		    }
	        } /* for (i = 0; i < DRIVE_MAP_SIZE; i++) */

	      if (tmpbase >= tmpmin)
	      {
		/* check if the memory area overlaps the (md)... or (rd)... file. */
//		if ( (to != 0xffff && to != ram_drive)	/* TO is not in memory */
//		  || ((long long)mem) > 0	/* TO is in memory but is mapped at a fixed location */
//		  || !compressed_file		/* TO is in memory and is normally mapped and uncompressed */
//		/* Now TO is in memory and is normally mapped and compressed */
//		  || tmpbase + bytes_needed <= start_byte  /* XXX: destination is below the gzip image */
//		  || tmpbase >= start_byte + to_filesize) /* XXX: destination is above the gzip image */
//		{
		    if (prefer_top)
		    {
			if (base < tmpbase)
			{
			    base = tmpbase; top_end = tmpend;
			}
			continue; // find available region with highest address
		    }
		    else
		    {
			base = tmpbase; top_end = tmpend;
			break; // use the first available region
		    }
//		}
//		else
//		    /* destination overlaps the gzip image */
//		    /* fail and try next memory block. */
//		    continue;
	      }
	      //if (to == 0xff)
		//bytes_needed = 0;

	    } /* for (; end_addr > (int) map; map = (struct AddrRangeDesc *) (((int) map) + 4 + map->size)) */

        } /* if (mbi.flags & MB_INFO_MEM_MAP) */
      else
	  grub_printf ("\nFatal: Your BIOS has no support for System Memory Map(INT15/EAX=E820h).\nAs a result you cannot use the --mem option.\n");
#endif
      }

      if (base < map_mem_min)
      {
	  //grub_close ();
	  return ! (errnum = ERR_WONT_FIT);
      }

      if (((long long)mem) > 0)
      {
	  base = ((unsigned long long)mem << 9);
	  sector_count = (top_end - base) >> SECTOR_BITS;
      }else{
	  sector_count = bytes_needed >> SECTOR_BITS;
	  // now sector_count may be > part_length, reading so many sectors could cause failure
      }

      bytes_needed = base;
      if (add_mbt)	/* no partition table */
	bytes_needed += sectors_per_track << SECTOR_BITS;	/* build the Master Boot Track */

      /* bytes_needed points to the first partition, base points to MBR */
      
//#undef MIN_EMU_BASE
      //      if (! grub_open (filename1))
      //	return 0;
	
      /* the first sector already read at BS */
      #if 0
      if ((to != 0xffff && to != ram_drive) || ((long long)mem) <= 0)
	{
		#endif
	  /* if image is in memory and not compressed, we can simply move it. */
	  if ((to == 0xffff || to == ram_drive) /*&& !compressed_file*/)
	  {
	    if (bytes_needed != start_byte)
		grub_memmove ((void *)(int)bytes_needed, (void *)(int)start_byte, filemax);
	  } else {
	    unsigned long long read_result;
	    grub_memmove ((void *)(int)bytes_needed, (void *)(unsigned int)BS, SECTOR_SIZE);
	    /* read the rest of the sectors */
	    if (filemax > SECTOR_SIZE)
	    {
	      read_result = grub_read ((bytes_needed + SECTOR_SIZE), -1ULL, 0xedde0d90);
	      if (read_result != filemax - SECTOR_SIZE)
	      {
		//if ( !probed_total_sectors || read_result<(probed_total_sectors<<SECTOR_BITS) )
		//{
		unsigned long long required = (probed_total_sectors << SECTOR_BITS) - SECTOR_SIZE;
		/* read again only required sectors */
		if ( ! probed_total_sectors 
		     || required >= filemax - SECTOR_SIZE
		     || ( (filepos = SECTOR_SIZE), /* re-read from the second sector. */
			  grub_read (bytes_needed + SECTOR_SIZE, required, 0xedde0d90) != required
			)
		   )
		{
		    //grub_close ();
		    if (errnum == ERR_NONE)
			errnum = ERR_READ;
		    return 0;
		}
		//}
	      }
	    }
	  }
	  //grub_close ();
	 #if 0
	}
      else if (/*(to == 0xffff || to == ram_drive) && */!compressed_file)
	{
	    if (bytes_needed != start_byte)
		grub_memmove64 (bytes_needed, start_byte, filemax);
		grub_close ();
	}
	else
     {
		grub_memmove64 (bytes_needed, (unsigned long long)(unsigned int)BS, SECTOR_SIZE);
        grub_read (bytes_needed + SECTOR_SIZE, -1ULL, 0xedde0d90);
        grub_close ();
      }
	#endif
      start_sector = base >> SECTOR_BITS;
      to = 0xFFFF/*GRUB_INVALID_DRIVE*/;

      if (add_mbt)	/* no partition table */
      {
	unsigned long sectors_per_cylinder1, cylinder, sector_rem;
	unsigned long head, sector;

	/* First sector of disk image has already been read to buffer BS/mbr */
	
	/* modify the BPB drive number. required for FAT12/16/32/NTFS */
	if (filesystem_type != -1)
	if (!*(char *)((int)BS + ((filesystem_type == 3) ? 0x41 : 0x25)))
		*(char *)((int)BS + ((filesystem_type == 3) ? 0x40 : 0x24)) = from;
	
	/* modify the BPB hidden sectors. required for FAT12/16/32/NTFS/EXT2 */
	if (filesystem_type != -1 || *(unsigned long *)((int)BS + 0x1c) == (unsigned long)part_start)
	    *(unsigned long *)((int)BS + 0x1c) = sectors_per_track;
	
	grub_memmove ((void *)(int)bytes_needed, (void *)(unsigned int)BS, SECTOR_SIZE);
	
	/* clear BS/mbr buffer */
	grub_memset(mbr, 0, SECTOR_SIZE);
	
	/* clear MS magic number */
	//*(long *)((int)mbr + 0x1b8) = 0; 
	/* Let MS magic number = virtual BIOS drive number */
	*(long *)((int)mbr + 0x1b8) = (unsigned char)from; 
	/* build the partition table */
	*(long *)((int)mbr + 0x1be) = 0x00010180L; 
	*(char *)((int)mbr + 0x1c2) = //((filesystem_type == -1 || filesystem_type == 5)? 0x83 : filesystem_type == 4 ? 0x07 : 0x0c); 
		filesystem_type == 1 ? 0x0E /* FAT12 */ :
		filesystem_type == 2 ? 0x0E /* FAT16 */ :
		filesystem_type == 3 ? 0x0C /* FAT32 */ :
		filesystem_type == 4 ? 0x07 /* NTFS */  :
		/*filesystem_type == 5 ?*/ 0x83 /* EXT2 */;

	//sector_count += sectors_per_track;	/* already incremented above */

	/* calculate the last sector's C/H/S value */
	sectors_per_cylinder1 = sectors_per_track * heads_per_cylinder;
	cylinder = ((unsigned long)(sector_count - 1)) / sectors_per_cylinder1;	/* XXX: 64-bit div */
	sector_rem = ((unsigned long)(sector_count - 1)) % sectors_per_cylinder1; /* XXX: 64-bit mod */
	head = sector_rem / (unsigned long)sectors_per_track;
	sector = sector_rem % (unsigned long)sectors_per_track;
	sector++;
	*(char *)((int)mbr + 0x1c3) = head;
	*(unsigned short *)((int)mbr + 0x1c4) = sector | (cylinder << 8) | ((cylinder >> 8) << 6);
	*(unsigned long *)((int)mbr + 0x1c6) = sectors_per_track; /* start LBA */
	*(unsigned long *)((int)mbr + 0x1ca) = sector_count - sectors_per_track; /* sector count */
	*(long long *)((int)mbr + 0x1ce) = 0LL; 
	*(long long *)((int)mbr + 0x1d6) = 0LL; 
	*(long long *)((int)mbr + 0x1de) = 0LL; 
	*(long long *)((int)mbr + 0x1e6) = 0LL; 
	*(long long *)((int)mbr + 0x1ee) = 0LL; 
	*(long long *)((int)mbr + 0x1f6) = 0LL; 
	*(unsigned short *)((int)mbr + 0x1fe) = 0xAA55;
	
	/* compose a master boot record routine */
	*(char *)((int)mbr) = 0xFA;	/* cli */
	*(unsigned short *)((int)mbr + 0x01) = 0xC033; /* xor AX,AX */
	*(unsigned short *)((int)mbr + 0x03) = 0xD08E; /* mov SS,AX */
	*(long *)((int)mbr + 0x05) = 0xFB7C00BC; /* mov SP,7C00 ; sti */
	*(long *)((int)mbr + 0x09) = 0x07501F50; /* push AX; pop DS */
						  /* push AX; pop ES */
	*(long *)((int)mbr + 0x0d) = 0x7C1CBEFC; /* cld; mov SI,7C1C */
	*(long *)((int)mbr + 0x11) = 0x50061CBF; /* mov DI,061C ; push AX */
	*(long *)((int)mbr + 0x15) = 0x01E4B957; /* push DI ; mov CX, 01E4 */
	*(long *)((int)mbr + 0x19) = 0x1ECBA4F3; /* repz movsb;retf;push DS */
	*(long *)((int)mbr + 0x1d) = 0x537C00BB; /* mov BX,7C00 ; push BX */
	*(long *)((int)mbr + 0x21) = 0x520180BA; /* mov DX,0180 ; push DX */
	*(long *)((int)mbr + 0x25) = 0x530201B8; /* mov AX,0201 ; push BX */
	*(long *)((int)mbr + 0x29) = 0x5F13CD41; /* inc CX; int 13; pop DI */
	*(long *)((int)mbr + 0x2d) = 0x5607BEBE; /* mov SI,07BE ; push SI */
	*(long *)((int)mbr + 0x31) = 0xCBFA5A5D; /* pop BP;pop DX;cli;retf */
	grub_memmove ((void*)(int)base, (void *)(unsigned int)mbr, SECTOR_SIZE);
      }
      else if ((from & 0x80) && (from < 0xA0))	/* the virtual drive is hard drive */
      {
	/* First sector of disk image has already been read to buffer BS/mbr */
	if (*(long *)((int)mbr + 0x1b8) == 0)
	  /* Let MS magic number = virtual BIOS drive number */
	  *(long *)((int)mbr + 0x1b8) = (unsigned char)from; 
	else if ((*(long *)((int)mbr + 0x1b8) & 0xFFFFFF00) == 0)
	  *(long *)((int)mbr + 0x1b8) |= (from << 8); 
	grub_memmove ((void*)(int)base, (void *)(unsigned int)mbr, SECTOR_SIZE);
      }

      /* if FROM is (rd), no mapping is established. but the image will be
       * loaded into memory, and (rd) will point to it. Note that Master
       * Boot Track and MBR code have been built as above when needed
       * for ram_drive > 0x80.
       */
      if (from == ram_drive)
      {
	rd_base = base;
	rd_size = filemax;//(sector_count << SECTOR_BITS);
	if (add_mbt)
		rd_size += sectors_per_track << SECTOR_BITS;	/* build the Master Boot Track */
	return 1;
      }
    }
#endif	/* GRUB_UTIL */

  if (in_situ)
	bios_drive_map[j].to_cylinder = 
		filesystem_type == 1 ? 0x0E /* FAT12 */ :
		filesystem_type == 2 ? 0x0E /* FAT16 */ :
		filesystem_type == 3 ? 0x0C /* FAT32 */ :
		filesystem_type == 4 ? 0x07 /* NTFS */  :
		/*filesystem_type == 5 ?*/ 0x83 /* EXT2 */;
  
//  /* if TO_DRIVE is whole floppy, skip the geometry lookup. */
//  if (start_sector == 0 && sector_count == 0 && to < 0x04)
//  {
//	tmp_geom.flags &= ~BIOSDISK_FLAG_LBA_EXTENSION;
//	tmp_geom.sector_size = 512;
//	tmp_geom.cylinders = 80;	/* does not care */
//	tmp_geom.heads = 2;		/* does not care */
//	tmp_geom.sectors = 18;		/* does not care */
//  }
//  else
//  /* Get the geometry. This ensures that the drive is present.  */
//  if (to != PXE_DRIVE && get_diskinfo (to, &tmp_geom))
//  {
//	return ! (errnum = ERR_NO_DISK);
//  }

  i = j;	/* restore i from j */
  
  bios_drive_map[i].from_drive = from;
  bios_drive_map[i].to_drive = (unsigned char)to; /* to_drive = 0xFF if to == 0xffff */

  /* if CHS disabled, let MAX_HEAD != 0 to ensure a non-empty slot */
  bios_drive_map[i].max_head = disable_chs_mode | (heads_per_cylinder - 1);
  bios_drive_map[i].max_sector = (disable_chs_mode ? 0 : in_situ ? 1 : sectors_per_track) | ((read_Only | fake_write) << 7) | (disable_lba_mode << 6);
  if (from >= 0xA0 /*&& tmp_geom.sector_size != 2048*/) /* FROM is cdrom and TO is not cdrom. */
	bios_drive_map[i].max_sector |= 0x0F; /* can be any value > 1, indicating an emulation. */

  /* bit 12 is for "TO drive is bifurcate" */

  if (! in_situ)
	bios_drive_map[i].to_cylinder =
		(//(tmp_geom.flags & BIOSDISK_FLAG_BIFURCATE) ? ((to & 0x100) >> 1) :
		((/*tmp_geom.flags &*/ BIOSDISK_FLAG_LBA_EXTENSION) << 15)) |
		//((tmp_geom.sector_size == 2048) << 14) |
		((from >= 0xA0) << 13) |	/* assume cdrom if from_drive is 0xA0 or greater */
		//((!!(tmp_geom.flags & BIOSDISK_FLAG_BIFURCATE)) << 12) |
		((filesystem_type > 0) << 11) |	/* has a known boot sector type */
		(/*(tmp_geom.cylinders - 1 > 0x3FF) ?*/ 0x3FF /*: (tmp_geom.cylinders - 1)*/);
  
  bios_drive_map[i].to_head = 254/*tmp_geom.heads - 1*/;

  bios_drive_map[i].to_sector = 63/*tmp_geom.sectors*/ | ((fake_write | ! unsafe_boot) << 6);
  if (in_situ)
	bios_drive_map[i].to_sector |= 0x80;
  
  bios_drive_map[i].start_sector = start_sector;
  //bios_drive_map[i].start_sector_hi = 0;	/* currently only low 32 bits are used. */
//  initrd_start_sector = start_sector;

  bios_drive_map[i].sector_count = sector_count;//(sector_count & 0xfffffffe) | fake_write | ! unsafe_boot;
  //bios_drive_map[i].sector_count_hi = 0;	/* currently only low 32 bits are used. */

#ifndef GRUB_UTIL
  /* increase the floppies or harddrives if needed */
  if (from & 0x80)
  {
	if (*((char *)0x475) == (char)(from - 0x80))
	{
		*((char *)0x475) = (char)(from - 0x80 + 1);
		if (debug > 0 && ! disable_map_info)
			print_bios_total_drives();
	}
  }else{
	if ((((*(char*)0x410) & 1)?((*(char*)0x410) >> 6) + 1 : 0) == ((char)from))
	{
		*((char *)0x410) &= 0x3e;
		*((char *)0x410) |= (((char)from) ? 0x41 : 1);
		if (debug > 0 && ! disable_map_info)
			print_bios_total_drives();
	}
  }
#endif	/* GRUB_UTIL */
  return 1;
  
delete_drive_map_slot:
  
//  if (bios_drive_map[i].to_drive == 0xFF && !(bios_drive_map[i].to_cylinder & 0x4000))
//  for (j = DRIVE_MAP_SIZE - 1; j > i ; j--)
//    {
//      if (bios_drive_map[j].to_drive == 0xFF && !(bios_drive_map[j].to_cylinder & 0x4000))
//	{
//	  if (mem == -1)
//		return ! (errnum = ERR_DEL_MEM_DRIVE);
//	  else /* force delete all subsequent MEM map slots */
//		grub_memmove ((char *) &bios_drive_map[j], (char *) &bios_drive_map[j + 1], sizeof (struct drive_map_slot) * (DRIVE_MAP_SIZE - j));
//	}
//    }

  grub_memmove ((char *) &bios_drive_map[i], (char *) &bios_drive_map[i + 1], sizeof (struct drive_map_slot) * (DRIVE_MAP_SIZE - i));
  if (mem != -1ULL)
	  //grub_close ();

#undef	BS

#ifndef GRUB_UTIL
  /* Search for the Max floppy drive number in the drive map table.  */
  from = 0;
  for (i = 0; i < DRIVE_MAP_SIZE; i++)
    {
      if (drive_map_slot_empty (bios_drive_map[i]))
	break;
      if (! (bios_drive_map[i].from_drive & 0xE0) && from < bios_drive_map[i].from_drive + 1)
	from = bios_drive_map[i].from_drive + 1;
    }

  /* max possible floppies that can be handled in BIOS Data Area is 4 */
  if (from > 4)
	from = 4;
  if (from <= ((floppies_orig & 1) ? (floppies_orig >> 6) + 1 : 0))
	if ((((*(char*)0x410) & 1) ? ((*(char*)0x410) >> 6) + 1 : 0) > ((floppies_orig & 1) ? (floppies_orig >> 6) + 1 : 0))
	{
		/* decrease the floppies */
		(*(char*)0x410) = floppies_orig;
		if (debug > 0)
			print_bios_total_drives();
	}

  /* Search for the Max hard drive number in the drive map table.  */
  from = 0;
  for (i = 0; i < DRIVE_MAP_SIZE; i++)
    {
      if (drive_map_slot_empty (bios_drive_map[i]))
	break;
      if ((bios_drive_map[i].from_drive & 0xE0) == 0x80 && from < bios_drive_map[i].from_drive - 0x80 + 1)
	from = bios_drive_map[i].from_drive - 0x80 + 1;
    }

  if (from <= harddrives_orig)
	if ((*(char*)0x475) > harddrives_orig)
	{
		/* decrease the harddrives */
		(*(char*)0x475) = harddrives_orig;
		if (debug > 0)
			print_bios_total_drives();
	}

#endif	/* ! GRUB_UTIL */
  return 1;
}

static struct builtin builtin_map =
{
  "map",
  map_func,
};
#endif

#if 1

extern int attempt_mount (void);
extern unsigned long request_fstype;
extern unsigned long dest_partition;
extern unsigned long entry;

static int
real_root_func (char *arg, int attempt_mnt)
{
  char *next;
  unsigned long i, tmp_drive = 0;
  unsigned long tmp_partition = 0;
  char ch;

  /* Get the drive and the partition.  */
  if (! *arg || *arg == ' ' || *arg == '\t')
    {
	current_drive = saved_drive;
	current_partition = saved_partition;
	next = 0; /* If ARG is empty, just print the current root device.  */
    }
  else
    {
	/* Call set_device to get the drive and the partition in ARG.  */
	if (! (next = set_device (arg)))
	    return 0;
    }

  if (next)
  {
	/* check the length of the root prefix, i.e., NEXT */
	for (i = 0; i < sizeof (saved_dir); i++)
	{
		ch = next[i];
		if (ch == 0 || ch == 0x20 || ch == '\t')
			break;
		if (ch == '\\')
		{
			//saved_dir[i] = ch;
			i++;
			ch = next[i];
			if (! ch || i >= sizeof (saved_dir))
			{
				i--;
				//saved_dir[i] = 0;
				break;
			}
		}
	}

	if (i >= sizeof (saved_dir))
	{
		errnum = ERR_WONT_FIT;
		return 0;
	}

	tmp_partition = current_partition;
	tmp_drive = current_drive;
  }

  errnum = ERR_NONE;

  /* Ignore ERR_FSYS_MOUNT.  */
  if (attempt_mnt)
    {
      if (! ((request_fstype = 1), (dest_partition = current_partition), open_partition ()) && errnum != ERR_FSYS_MOUNT)
	return 0;

      //if (fsys_type != NUM_FSYS || ! next)
      {
	grub_printf ("(0x%02X,%d):%lx,%lx:%02X,%02X:%s\n"
		, (long)current_drive
		, (unsigned long)(((unsigned short *)(&current_partition))[1])
		, (unsigned long long)part_start
		, (unsigned long long)part_length
		, (long)(((unsigned char *)(&current_partition))[0])
		, (long)(((unsigned char *)(&current_partition))[1])
		, fsys_table[fsys_type].name
	    );
      }
      //else
	//return ! (errnum = ERR_FSYS_MOUNT);
    }
  
  if (next)
  {
	saved_partition = tmp_partition;
	saved_drive = tmp_drive;

	/* copy root prefix to saved_dir */
	for (i = 0; i < sizeof (saved_dir); i++)
	{
		ch = next[i];
		if (ch == 0 || ch == 0x20 || ch == '\t')
			break;
		if (ch == '\\')
		{
			saved_dir[i] = ch;
			i++;
			ch = next[i];
			if (! ch || i >= sizeof (saved_dir))
			{
				i--;
				saved_dir[i] = 0;
				break;
			}
		}
		saved_dir[i] = ch;
	}

	if (saved_dir[i-1] == '/')
	{
		saved_dir[i-1] = 0;
	} else
		saved_dir[i] = 0;
  }

  if (*saved_dir)
	grub_printf ("%s\n", saved_dir);

  /* Clear ERRNUM.  */
  errnum = 0;
  /* If ARG is empty, then return TRUE for harddrive, and FALSE for floppy */
  return next ? 1 : (saved_drive & 0x80);
}

static int
root_func (char *arg/*, int flags*/)
{
  return real_root_func (arg, 1);
}

static struct builtin builtin_root =
{
  "root",
  root_func,
};

static int
rootnoverify_func (char *arg/*, int flags*/)
{
  return real_root_func (arg, 0);
}

static struct builtin builtin_rootnoverify =
{
  "rootnoverify",
  rootnoverify_func,
};

#endif




#if 1

//static int real_root_func (char *arg1, int attempt_mnt);
/* find */
/* Search for the filename ARG in all of partitions and optionally make that
 * partition root("--set-root", Thanks to Chris Semler <csemler@mail.com>).
 */
static int
find_func (char *arg/*, int flags*/)
{
  struct builtin *builtin1 = 0;
  int ret;
  char *filename;
  unsigned long drive;
  unsigned long tmp_drive = saved_drive;
  unsigned long tmp_partition = saved_partition;
  unsigned long got_file = 0;
  char *set_root = 0;
//  unsigned long ignore_cd = 0;
  unsigned long ignore_floppies = 0;
  unsigned long ignore_oem = 0;
  unsigned long active = 0;
  //char *in_drives = NULL;	/* search in drive list */
  char root_found[16];
  
  for (;;)
  {
    if (grub_memcmp (arg, "--set-root=", 11) == 0)
      {
	set_root = arg + 11;
	if (*set_root && *set_root != ' ' && *set_root != '\t' && *set_root != '/')
		return ! (errnum = ERR_FILENAME_FORMAT);
      }
    else if (grub_memcmp (arg, "--set-root", 10) == 0)
      {
	set_root = "";
      }
//    else if (grub_memcmp (arg, "--ignore-cd", 11) == 0)
//      {
//	ignore_cd = 1;
//      }
    else if (grub_memcmp (arg, "--ignore-floppies", 17) == 0)
      {
	ignore_floppies = 1;
      }
    else if (grub_memcmp (arg, "--ignore-oem", 12) == 0)
      {
	ignore_oem = 1;
      }
    else if (grub_memcmp (arg, "--active", 8) == 0)
      {
	active = 1;
      }
    else
	break;
    arg = skip_to (arg);
  }
  
  /* The filename should NOT have a DEVICE part. */
  filename = set_device (arg);
  if (filename)
	return ! (errnum = ERR_FILENAME_FORMAT);

  if (*arg == '/' || *arg == '+' || (*arg >= '0' && *arg <= '9'))
  {
    filename = arg;
    arg = skip_to (arg);
  } else {
    filename = 0;
  }

  /* arg points to command. */

  if (*arg)
  {
    builtin1 = find_command (arg);
//    if ((int)builtin1 != -1)
//    if (! builtin1 /*|| ! (builtin1->flags & flags)*/)
//    {
//	errnum = ERR_UNRECOGNIZED;
//	return 0;
//    }
    if ((int)builtin1/* != -1*/)
	arg = skip_to (arg);	/* get argument of command */
    else
	builtin1 = &builtin_command;
  }

  errnum = 0;

  /* Hard disks. Search in hard disks first, since floppies are slow */
#define FIND_DRIVES (*((char *)0x475))
  for (drive = 0x80; /*drive < 0x80 + FIND_DRIVES*/; drive++)
    {
      //unsigned long part = 0xFFFFFF;
      //unsigned long start, len, offset, ext_offset1;
      //unsigned long type, entry1;
      unsigned long type;
      unsigned long i;

      struct part_info *pi;

      if (drive >= 0x80 + FIND_DRIVES)
#undef FIND_DRIVES
      {
	if (ignore_floppies)
		break;
	drive = 0;	/* begin floppy */
      }
#define FIND_DRIVES (((*(char*)0x410) & 1)?((*(char*)0x410) >> 6) + 1 : 0)
      if (drive < 0x80)
      {
	if (drive >= FIND_DRIVES)
		break;	/* end floppy */
      }
#undef FIND_DRIVES

      pi = (struct part_info *)(PART_TABLE_BUF);

      current_drive = drive;
      list_partitions ();
      for (i = 1; pi[i].part_num != 0xFFFF; i++)
      /* while ((	next_partition_drive		= drive,
		next_partition_dest		= 0xFFFFFF,
		next_partition_partition	= &part,
		next_partition_type		= &type,
		next_partition_start		= &start,
		next_partition_len		= &len,
		next_partition_offset		= &offset,
		next_partition_entry		= &entry1,
		next_partition_ext_offset	= &ext_offset1,
		next_partition_buf		= mbr,
		next_partition ())) */
	{
	  if (active)
	  {
		/* the first partition is 255, so we must exclude it. */
		if ((char)(pi[i].part_num) > 3)	/* not primary */
			break;
		if (pi[i].boot_indicator != 0x80) /* not active */
			continue;
	  }
	  type = pi[i].part_type;
	  if (type != PC_SLICE_TYPE_NONE
		  && ! (ignore_oem == 1 && (type & ~PC_SLICE_TYPE_HIDDEN_FLAG) == 0x02) 
	      //&& ! IS_PC_SLICE_TYPE_BSD (type)
	      && ! IS_PC_SLICE_TYPE_EXTENDED (type))
	    {
	      ((unsigned short *)(&current_partition))[1] = (pi[i].part_num);
	      if (filename == 0)
		{
		  saved_drive = current_drive;
		  saved_partition = current_partition;

		  ret = builtin1 ? (builtin1->func) (arg/*, flags*/) : 1;
		  if (ret)
		  {
			grub_sprintf (root_found, "(0x%X,%d)", drive, pi[i].part_num);
		        grub_printf ("%s ", root_found);
			got_file = 1;
		        if (set_root && (drive < 0x80 || pi[i].part_num != 0xFF))
				goto found;
		  }
		}
	      else if (open_device ())
		{
		  saved_drive = current_drive;
		  saved_partition = current_partition;
		  if (grub_open (filename))
		    {
		      ret = builtin1 ? (builtin1->func) (arg/*, flags*/) : 1;
		      if (ret)
		      {
			grub_sprintf (root_found, "(0x%X,%d)", drive, pi[i].part_num);
			grub_printf ("%s ", root_found);
		        got_file = 1;
		        if (set_root && (drive < 0x80 || pi[i].part_num != 0xFF))
				goto found;
		      }
		    }
		}
	    }

	  /* We want to ignore any error here.  */
	  errnum = ERR_NONE;
	}

	if (got_file && set_root)
		goto found;

      /* next_partition always sets ERRNUM in the last call, so clear
	 it.  */
      errnum = ERR_NONE;
    }

#if 0
  /* CD-ROM.  */
  if (cdrom_drive != GRUB_INVALID_DRIVE && ! ignore_cd)
    {
      current_drive = cdrom_drive;
      current_partition = 0xFFFFFF;
      
      if (filename == 0)
	{
	  saved_drive = current_drive;
	  saved_partition = current_partition;
	  ret = builtin1 ? (builtin1->func) (arg/*, flags*/) : 1;
	  if (ret)
	  {
	    grub_sprintf (root_found, "(cd)");
		grub_printf (" %s\n", root_found);
	    got_file = 1;
	    if (set_root)
		goto found;
	  }
	}
      else if (open_device ())
	{
	  saved_drive = current_drive;
	  saved_partition = current_partition;
	  if (grub_open (filename))
	    {
	      ret = builtin1 ? (builtin1->func) (arg/*, flags*/) : 1;
	      if (ret)
	      {
	        grub_sprintf (root_found, "(cd)");
		  grub_printf (" %s\n", root_found);
	        got_file = 1;
	        if (set_root)
		  goto found;
	      }
	    }
	}

      errnum = ERR_NONE;
    }

  /* Floppies.  */
#define FIND_DRIVES (((*(char*)0x410) & 1)?((*(char*)0x410) >> 6) + 1 : 0)
  if (! ignore_floppies)
  {
    for (drive = 0; drive < 0 + FIND_DRIVES; drive++)
#undef FIND_DRIVES
    {
      current_drive = drive;
      current_partition = 0xFFFFFF;
      
      if (filename == 0)
	{
	  saved_drive = current_drive;
	  saved_partition = current_partition;
	  ret = builtin1 ? (builtin1->func) (arg/*, flags*/) : 1;
	  if (ret)
	  {
	    grub_sprintf (root_found, "(fd%d)", drive);
		grub_printf ("\n%s", root_found);
	    got_file = 1;
	    if (set_root)
		goto found;
	  }
	}
      else if (((request_fstype = 1), (dest_partition = current_partition), open_partition ()))
	{
	  saved_drive = current_drive;
	  saved_partition = current_partition;
	  if (grub_open (filename))
	    {
	      ret = builtin1 ? (builtin1->func) (arg/*, flags*/) : 1;
	      if (ret)
	      {
	        grub_sprintf (root_found, "(fd%d)", drive);
		  grub_printf ("\n%s", root_found);
	        got_file = 1;
	        if (set_root)
		  goto found;
	      }
	    }
	}

      errnum = ERR_NONE;
    }
  }
#endif

found:
  saved_drive = tmp_drive;
  saved_partition = tmp_partition;

  if (got_file)
    {
	errnum = ERR_NONE;
	if (set_root)
	{
		int j;

		//return real_root_func (root_found, 1);
		saved_drive = current_drive;
		saved_partition = current_partition;
		/* copy root prefix to saved_dir */
		for (j = 0; j < sizeof (saved_dir); j++)
		{
		    char ch;

		    ch = set_root[j];
		    if (ch == 0 || ch == 0x20 || ch == '\t')
			break;
		    if (ch == '\\')
		    {
			saved_dir[j] = ch;
			j++;
			ch = set_root[j];
			if (! ch || j >= sizeof (saved_dir))
			{
				j--;
				saved_dir[j] = 0;
				break;
			}
		    }
		    saved_dir[j] = ch;
		}

		if (saved_dir[j-1] == '/')
		{
		    saved_dir[j-1] = 0;
		} else
		    saved_dir[j] = 0;
	}
	return 1;
    }

  errnum = ERR_FILE_NOT_FOUND;
  return 0;
}

static struct builtin builtin_find =
{
  "find",
  find_func,
};

#endif

struct builtin builtin_exit =
{
  "exit",
  0/* we have no exit_func!! */,
};

struct builtin builtin_title =
{
  "title",
  0/* we have no title_func!! */,
};

static int
default_func (char *arg/*, int flags*/)
{
  unsigned long long ull;
  if (! safe_parse_maxint (&arg, &ull))
    return 0;

  default_entry = ull;
  return 1;
}

static struct builtin builtin_default =
{
  "default",
  default_func,
};

static int
timeout_func (char *arg/*, int flags*/)
{
  unsigned long long ull;
  if (! safe_parse_maxint (&arg, &ull))
    return 0;

  grub_timeout = ull;
  return 1;
}

static struct builtin builtin_timeout =
{
  "timeout",
  timeout_func,
};

static int
pause_func (char *arg/*, int flags*/)
{
  char *p;
  unsigned long long wait = -1;
  unsigned long i;

  for (;;)
  {
    if (grub_memcmp (arg, "--wait=", 7) == 0)
      {
        p = arg + 7;
        if (! safe_parse_maxint (&p, &wait))
                return 0;
      }
    else
        break;
    arg = skip_to (/*0, */arg);
  }

  printf("%s\n", arg);

  if ((unsigned long)wait != -1)
  {
    /* calculate ticks */
    i = ((unsigned long)wait) * 91;
    wait = i / 5;
  }
  for (i = 0; i <= (unsigned long)wait; i++)
  {
    /* Check if there is a key-press.  */
    if (console_checkkey () != -1)
	return console_getkey ();
  }

  return 1;
}

static struct builtin builtin_pause =
{
  "pause",
  pause_func,
};

void hexdump(unsigned long ofs, char* buf, unsigned long len);

void hexdump(unsigned long ofs, char* buf, unsigned long len)
{
  //buf &= 0xFFFFFFF0;
  ofs &= 0xFFFFFFF0;
  buf = (char *)ofs;
  len += 15;
  len &= 0xFFFFFFF0;
  while (len>0)
    {
      unsigned long cnt, k;

      grub_printf ("%08X: ", ofs);
      cnt=16;
//      if (cnt>len)
//        cnt=len;

      for (k=0;k<cnt;k++)
        {	  
          printf("%02X ", (unsigned long)(unsigned char)(buf[k]));
          if ((k!=15) && ((k & 3)==3))
            printf(" ");
        }

//      for (;k<16;k++)
//        {
//          printf("   ");
//          if ((k!=15) && ((k & 3)==3))
//            printf(" ");
//        }

      printf("; ");

      for (k=0;k<cnt;k++)
        printf("%c",(unsigned long)((((unsigned char)buf[k]>=32) && ((unsigned char)buf[k]!=127))?buf[k]:'.'));

      printf("\n");

      ofs+=16;
      buf+=16;
      len-=cnt;
    }
}

static char *hexdump_buf = 0;

extern int hexdump_func (char *arg/*, int flags*/);

int
hexdump_func (char *arg/*, int flags*/)
{
  char *p;
  unsigned long long tmp;
  unsigned long len = 0x80;

  if (arg && *arg)
  {
    p = arg;
    if (! safe_parse_maxint (&p, &tmp))
	return 0;
    hexdump_buf = (char *)(int)tmp;
    arg = skip_to (/*0, */arg);
    if (*arg)
    {
        p = arg;
        if (! safe_parse_maxint (&p, &tmp))
		return 0;
	len = tmp;
    }
  }

  hexdump((unsigned long)hexdump_buf, hexdump_buf, len);
  hexdump_buf += len;

  return 1;
}

static struct builtin builtin_hexdump =
{
  "hexdump",
  hexdump_func,
};

static char *asm_buf = (char *)0x1000000;	/* 16M */

static int
asm_func (char *arg/*, int flags*/)
{
  char *p;
  char *go_buf = (char *)0x1000000;	/* 16M */
  unsigned long long tmp;

  if (arg && *arg)
  {
    p = arg;
    if (safe_parse_maxint (&p, &tmp))
    {
	/* the pointer */
	go_buf = asm_buf = (char *)(int)tmp;
	arg = skip_to (/*0, */arg);
    }
    if (*arg)
    {
	unsigned long pseudo = *(unsigned long *)arg;

	p = arg = skip_to (arg);
	pseudo &= 0x00DFFFFF;
	if (pseudo == 0x6264) /* db */
	{
		for (; *p; p = skip_to (p))
		{
			if (! safe_parse_maxint (&p, &tmp))
				return 0;
			*(asm_buf++) = tmp;
		}
	}
	else if (pseudo == 0x6f67) /* go */
	{
		go_buf = ((char * (*)(void))go_buf)();
		printf ("%08X\n", go_buf);
	}
    }
  }
  else
	printf ("%08X\n", asm_buf);

  return !(errnum = 0);
}

static struct builtin builtin_asm =
{
  "asm",
  asm_func,
};

/* The table of builtin commands. Sorted in dictionary order.  */
struct builtin *builtin_table[] =
{
  &builtin_asm,
#if defined(MBRSECTORS127)
  &builtin_blocklist,
#endif
  //&builtin_cat,
  &builtin_command,
  &builtin_default,
  &builtin_exit,
  &builtin_find,
  &builtin_hexdump,
#if 1 || defined(MBRSECTORS127)
  &builtin_map,
#endif
  &builtin_pause,
  &builtin_root,
  &builtin_rootnoverify,
  &builtin_timeout,
  &builtin_title,
  0
};



