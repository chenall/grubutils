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


/*//////////////////////////////////////////////////////////////////////////////////////////////////////
compile and generate executable mod:	

step 1:
	gcc -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE g4d_off.c
	
step 2: 
	objcopy -O binary a.out g4d_off.mod

    and then the resultant g4d_off.mod will be grub4dos executable.

///////////////////////////////////////////////////////////////////////////////////////////////////////

	this executable mode running in grub4dos 0.4.5 or above, it will use acpi or apm bios interface to 
	shutdown PC. 

	the executable mode also can be compresses by gzip and runing like this:
		(hd0,0)/g4d_off.mod.gz --help
/////////////////////////////////////////////////////////////////////////////////////////////////////*/

#define		U_8				unsigned char
#define		U_16			unsigned short
#define		U_32			unsigned long
#define		U_64			unsigned long long
#define		NULL			((void*)0)
#define		MAX_32BIT_VALUE	0xffffffff
#define		MAX_X86IO_SPACE	0xffff
#define		ACPI_SDTH		system_description_table_header
#define		ACPI_FLG		(pm1_ctr_tpy.pm1_cover.acpi_flg)
#define		PM1_CNT			(pm1_ctr_tpy.pm1_cover.pm1_top)
#define		PM1A			(pm1_ctr_tpy.pm1_cover.cover_reserved_1)
#define		PM1B			(pm1_ctr_tpy.pm1_cover.cover_reserved_2)
#define		PM1A_BIT_BLK	(pm1_ctr_tpy.pm1_ctr_reg_group.pm1a_blk)
#define		PM1B_BIT_BLK	(pm1_ctr_tpy.pm1_ctr_reg_group.pm1b_blk)

//#define		G4D_ARG(x)		(memcmp((void*)&main - *(U_32*)(&main - 8), x, strlen(x)) == 0 ? 1 : 0)
#define		G4D_ARG(x)		(memcmp(arg, x, strlen(x)) == 0 ? 1 : 0)
#define 	TABLE_CHECK32(addr,signature) 	(header_check32(((void*)(addr)), (signature)) \
											&& check_sum(((void*)(addr)),(((struct ACPI_SDTH*)(addr))->length)))
											
#define		REG_INW(port, val) 	__asm__ __volatile__ ("inw %%dx, %%ax" : "=a" (val) : "d"(port))
#define		REG_OUTW(port, val) __asm__ __volatile__ ("outw %%ax, %%dx" : :"a"(val), "d"(port))
#define		REG_OUTB(port, val)	__asm__ __volatile__ ("outb %%al, %%dx" : :"a"(val), "d"(port))


/*following is grub4dos internal function*/										
#define 	strlen 		((int (*)(const char *))((*(int **)0x8300)[12]))
#define		memcmp 		((int (*)(const char *, const char *, int))((*(int **)0x8300)[22]))
#define 	cls 		((void (*)(void))((*(int **)0x8300)[6]))
#define 	putchar 	((void (*)(int))((*(int **)0x8300)[2]))
#define 	sprintf 	((int (*)(char *, const char *, ...))((*(int **)0x8300)[0]))
#define 	printf(...) sprintf(NULL, __VA_ARGS__)
#define 	memmove 	((void *(*)(void *, const void *, int))((*(int **)0x8300)[23]))				

struct rsdp_revision_0
{
	unsigned char		signture[8] ; 
	unsigned char		checksum ; 
	unsigned char		oemid[6]; 
	unsigned char		revision ; 
	unsigned long		rsdtaddress ; 
} __attribute__ ((packed)) ; 


struct rsdp_table	/* acpi revision 2 above */
{
	unsigned char		signture[8] ; 
	unsigned char		checksum; 
	unsigned char		oemid[6]; 
	unsigned char		revision; 
	unsigned long		rsdtaddress; 
	unsigned long		length; 
	unsigned long long	xsdtaddress; 
	unsigned long		extended_checksum; 
	unsigned char		reservd[3]; 
} __attribute__ ((packed)); 


struct system_description_table_header
{
	unsigned char		signature[4]; 
	unsigned long		length; 
	unsigned char		revision; 
	unsigned char		checksum; 
	unsigned char		oemid[6]; 
	unsigned char		oem_table_id[8]; 
	unsigned char		oem_revison[4]; 
	unsigned char		creator_id[4]; 
	unsigned char		creator_revision[4]; 
} __attribute__ ((packed)); 


struct simplify_fadt_table 	/* refer to acpi 4.0 sepction */
{
	struct system_description_table_header	fadt_header;
	unsigned char		ignor_1[4]; 
	unsigned long		dsdt; 
	unsigned char		ignore_2[4]; 
	unsigned long		smi_cmd; 
	unsigned char		acpi_enable; 
	unsigned char		acpi_disable; 
	unsigned char		ignore_3[10]; 
	unsigned long		pm1a_cnt_blk; 
	unsigned long		pm1b_cnt_blk; 
	unsigned char		ignore_4[17]; 
	unsigned char		pm1_cnt_len; 
	unsigned char		ignore_5[50]; 
	unsigned long long	x_dsdt;
	unsigned char		ignore_6[24];
	unsigned char		x_pm1a_cnt_blk[12]; 
	unsigned char		x_pm1b_cnt_blk [12];
	unsigned char		ignore_7 [48];
} __attribute__ ((packed)); 

struct gas_addr_format
{
	unsigned char		address_space_id;
	unsigned char		register_bit_width;
	unsigned char		register_bit_offset;
	unsigned char		reserved;
	unsigned long long  address;
} __attribute__ ((packed)) ; 


struct pm1_control_registers
{
	unsigned short		sci_en : 1; 
	unsigned short		bm_rld : 1; 
	unsigned short		gbl_rls : 1; 
	unsigned short		reserved_1 :6; 
	unsigned short		igneore : 1; 
	unsigned short		slp_typx : 3; 
	unsigned short		slp_en : 1; 
	unsigned short		reserved_2 : 2; 
} __attribute__ ((packed)); 


static void *	get_rsdp 		(void);
static void * 	get_fadt		(void); 
static void *	get_dsdt 		(void);
static int		get_s5_sly 		(struct ACPI_SDTH* dsdt);
static void *	scan_entry 		(unsigned long long parent_table, const char parent_signature[],
								  const char entry_signature[], unsigned long address_wide); 
static inline int   check_sum			(void *entry, unsigned long length);
static inline void 	header_print 		(void *entry);
static inline int   header_check32		(void *entry, const char signature[]);
static inline void  * scan_rsdp			(void *entry, unsigned long cx); 
static void		    write_pm1_reg		(void);
static int    	    write_smi_reg		(int set_acpi_state);
static int 			read_pm1_reg 		(void);
static int			translate_acpi		(int en_or_disable); 
static void			func_write_pm_reg	(U_32 pm_cnt_io);
static int			func_read_pm_reg	(U_32 pm_cnt_io);
static void			display_debug		(void);
static void 		try_apm				(void);
void 				wait_io_delay 		(unsigned long millisecond);
static union 
{
	struct {
		struct	pm1_control_registers	pm1a_blk; 
		struct	pm1_control_registers	pm1b_blk; 
	} __attribute__ ((packed)) pm1_ctr_reg_group; 
	
	struct {
		unsigned short	cover_reserved_1; 
		unsigned short	cover_reserved_2; 	
		unsigned short	pm1_top;
		unsigned short	acpi_flg;
	}__attribute__ ((packed)) pm1_cover;
	
} __attribute__ ((packed)) volatile pm1_ctr_tpy; 

static struct	rsdp_table *						rsdp_addr = NULL; 
static struct 	system_description_table_header *	rsdt_addr = NULL; 
static struct	system_description_table_header *	xsdt_addr = NULL; 
static struct	system_description_table_header *	dsdt_addr = NULL; 
static struct	simplify_fadt_table *				fadt_addr = NULL; 

		 
/* this is needed, see the comment in grubprog.h */
#include "grubprog.h"
/* Do not insert any other asm lines here. */

int main (char *arg,int flags)
{
	if (G4D_ARG("--help") || G4D_ARG("/?")) {
		cls;
	printf("\n\n\
   USAGE:	[path] g4d_off.gz [--apm-only | --acpi-apm]\n\n\
		--apm-only : only use APM bios to shutdown PC.\n\
		--acpi-apm : after ACPI soft off fails, try APM bios.\n\
		--force-sci: possible need enable SCI for some acpi bios .\n\
		--display  : display some ACPI message about system.\n\
		--help  /? : display this message.\n\n\
    NOTE: 	as defaut, only use ACPI for shutdow PC.\n\
			and all options can only be a single option.\n\n\
  thanks:	Mr.xianglang, Mr.rockrock99, Mr.tinybit, Mr.chenall and you.\n\n\
     ver: - final\n\n");
	return 0;
	}
	
	if (! get_rsdp()) goto error;
	if (! get_fadt()) goto error;
	if (! get_dsdt()) goto error;
	translate_acpi(1);
	
	if (G4D_ARG("--display")) {/* this is debug message */
		display_debug();
		goto error;
	}

	/* acpi_soft_off */
	if (!G4D_ARG("--apm-only") && ACPI_FLG ) {
		if (!G4D_ARG("--force-sci")) translate_acpi(0);
		if (get_s5_sly(dsdt_addr)) write_pm1_reg();
		printf("PM1_CNT vlaue %x writed\nACPI soft off fails! please see help for more info.\n", PM1_CNT);
	}
error:
	translate_acpi(0);
	wait_io_delay(200);
	
	/* if call APM bios, we don't return to grub4dos */
	if (G4D_ARG("--apm-only") || G4D_ARG("--acpi-apm")) try_apm(); 
	return 1;
}


static void * get_rsdp ( void )
{
	void * p;
	
	if (scan_rsdp((void*)0xf0000, (0xfffff-0xf0000)/16)) goto found; 
	if (scan_rsdp((void*)0xe0000, (0xeffff-0xe0000)/16)) goto found; 
	p = (void*) ((U_32)*(U_16*)0x40e << 4) ; 
	if (p > (void*)0x90000 && (scan_rsdp(p, 1024/16) )) goto found; 
	return rsdp_addr = NULL; 
found:
	printf("\nrsdp address at : 0x%x, revision is %x\n",rsdp_addr, rsdp_addr->revision);
	return rsdp_addr ; 
}


static inline void * scan_rsdp ( void *entry, unsigned long cx )
{
	for (; cx != 0 ; cx --, entry += 16) {
		if (memcmp(entry, "RSD PTR ", 8) != 0) continue ; 
		if ((((struct rsdp_table*)entry)->revision == 0) 
				? check_sum(entry, sizeof(struct rsdp_revision_0))
				: check_sum(entry, ((struct rsdp_table*)entry)->length))
			return rsdp_addr = entry ; 
	}
	return rsdp_addr = NULL; 
}


static inline int check_sum ( void *entry, unsigned long length )
{
	int	sum = 0; 
	char *	p = entry;
	
	for (; p < (char*)entry + length ; p++)
		sum = ( *p + sum ) & 0xff ; 
	if (sum) {
		printf("check sum error :\n");
		header_print(entry);
		return 0 ;
	} else 
		return 1; 
}


void header_print (void * entry)
{
	int j;
	char *p = entry;
	
	for (j = 4; j; j--)
		putchar(*p++);	
	printf(" table at: 0x%x, length is 0x%x, revision is %x, OEMID is ", entry, 
			((struct ACPI_SDTH*)entry)->length, ((struct ACPI_SDTH*)entry)->revision) ; 
	for (p =((struct ACPI_SDTH*)entry)->oemid,j = 6; j; j--)
		putchar(*p++);
	printf("\n");
	return;
}


static inline int header_check32 ( void *entry, const char signature[] )
{	
	if (memcmp(entry, signature, strlen(signature) ) == 0 
		&& ((struct ACPI_SDTH*)entry)->revision 
		&& ((struct ACPI_SDTH*)entry)->length >= sizeof(struct ACPI_SDTH))
		return 1 ; 
	else {
			header_print(entry);
			return 0 ;
	}
}


static void * get_fadt (void)
{	
	if (! rsdp_addr) {
		printf("acpi entry : rsdp not found\n");
		goto error;
	}
	if (TABLE_CHECK32(rsdp_addr->rsdtaddress, "RSDT")) {		
		header_print(rsdt_addr = (struct ACPI_SDTH*)rsdp_addr->rsdtaddress); 
		fadt_addr = (struct simplify_fadt_table*) scan_entry(rsdp_addr->rsdtaddress, "RSDT", "FACP", 4); 
	}		
	if (! fadt_addr && rsdp_addr->revision )
		fadt_addr = (struct simplify_fadt_table*) scan_entry(rsdp_addr->xsdtaddress, "XSDT", "FACP", 8); 
	if (fadt_addr) {
		header_print((void*)fadt_addr);
		return fadt_addr;
	}
error:
	return fadt_addr = NULL;
}


static void * scan_entry ( unsigned long long parent_table, const char parent_signature[],
							const char entry_signature[], unsigned long address_wide )
{
	struct ACPI_SDTH * parent_start; 
	struct ACPI_SDTH * parent_end;
	struct ACPI_SDTH * entry_start; 
	
	if (! parent_table) goto error;
	if (parent_table > MAX_32BIT_VALUE) {
		ACPI_FLG = 64;
		goto error_64 ;
	} else 
		parent_start = (void *)(U_32)parent_table; 
	parent_end = (void*)parent_start + parent_start->length;
	
	for (entry_start =  (void*)parent_end - address_wide;
		(void*)entry_start >= (void*)parent_start + sizeof(struct ACPI_SDTH);
		entry_start = (void *)entry_start - address_wide) {
		if (TABLE_CHECK32((*(U_32*)entry_start), entry_signature)) 
			goto found;
	}
error:
	ACPI_FLG = 0;
error_64:
	printf("FADT not found");
	return NULL ;
found:
	ACPI_FLG = 32;
	return (void*)(*(U_32*)entry_start);

}


static void * get_dsdt (void)
{
	void * dsdt;

	if (fadt_addr) {
		if (fadt_addr->fadt_header.revision > 2 && fadt_addr->x_dsdt < MAX_32BIT_VALUE) 
			dsdt = (void*)(U_32)(fadt_addr->x_dsdt);
		else dsdt = (void*)fadt_addr->dsdt;
		if (TABLE_CHECK32(dsdt, "DSDT")) {
			header_print(dsdt);
			return dsdt_addr = dsdt ;
		}
	}
error:
	printf("dsdt not found");
	return dsdt_addr = NULL;
}


static int get_s5_sly ( struct ACPI_SDTH* dsdt )
{ 
	unsigned char *p = (void*)dsdt + sizeof(struct ACPI_SDTH);
	unsigned char *s5_p;
	int i = 0;
	
	if (!dsdt) goto error;
	for (p; p < (U_8*)dsdt + dsdt->length; p++) {
		if (*(U_32*)p != 0x5f35535f) continue; /* _S5_ */
		/* Sn name possible have root path "\" */
		if ( (*(p - 1) == 0x8 || *(p - 2) == 0x8) && *(p + 4) == 0x12) 
			goto name_found;
	}
	printf("S5 name space not found");
error:
	return 0;
	
name_found:
	if (*(p - 1) == 0x8 ) s5_p = p - 1; else s5_p = p - 2;
	printf("\nS5 name space start at 0x%x\nS5 dump :  ", s5_p);
	for (s5_p; s5_p < p + 5 + *(p+5); s5_p++) {
		if (*s5_p < 0x10) printf("0");
		printf("%x ",*s5_p);
	}
	putchar('\n');
	
	PM1A = PM1B = 0;
	p  += 7;
	if (*p == 0x0a) PM1A_BIT_BLK.slp_typx = *++p;
	else PM1A_BIT_BLK.slp_typx = *p;
	if (!fadt_addr->pm1b_cnt_blk)
		goto found;
	p += 1;
	if (*p == 0x0a) PM1B_BIT_BLK.slp_typx = *++p;
	else PM1B_BIT_BLK.slp_typx = *p;
found:
	return 1; 
}


static int translate_acpi (int en_or_disable) /* input: 0 for disable ,1 for enable; */
{
	if (!fadt_addr) goto error;
	write_smi_reg(en_or_disable);
	if (!read_pm1_reg()) {
		ACPI_FLG = 0 ;
		goto error;
	}
    if (PM1_CNT & 1) {
		printf("\n\n	enter ACPI mode \n");
		return 1;
	} else
		printf("\n	enter legacy mode\n");
error:
	return 0;
}


static int write_smi_reg (int set_acpi_state)
{
	if (fadt_addr->smi_cmd) {
		set_acpi_state == 1 ? (set_acpi_state = fadt_addr->acpi_enable) 
							: (set_acpi_state = fadt_addr->acpi_disable);
	} else
		goto error;
	if (set_acpi_state) {
		REG_OUTB(fadt_addr->smi_cmd, set_acpi_state); 
		wait_io_delay(1);
		return 1;
	}
error:
	printf("don't support SMI interface\n");
	return 0;
}


void wait_io_delay (U_32 millisecond)
{
	int cx;
	for (cx = millisecond * 500000; millisecond != 0; millisecond--) {
	 __asm__ ( "nop\n\t"  " nop\n\t");
	}
	return;
}

static int read_pm1_reg (void)
{
	if (!fadt_addr->pm1a_cnt_blk) {
		printf("pm1a_cnt port error");
		return 0;
	}
	PM1_CNT = PM1A = PM1B = 0;
	PM1A = func_read_pm_reg(fadt_addr->pm1a_cnt_blk);	
	if (fadt_addr->pm1b_cnt_blk) 
	  PM1B = func_read_pm_reg(fadt_addr->pm1b_cnt_blk);
	PM1_CNT = PM1A | PM1B;
	return 1;
}

	
static int func_read_pm_reg (U_32 pm_cnt_io)
{	
	int i;
	REG_INW(pm_cnt_io, i);
	wait_io_delay(1);
	return i;
}


static void write_pm1_reg (void)
{
	PM1_CNT &= 0X63F9;
	PM1_CNT |= PM1A ;
	PM1_CNT |= 0X2000;
	
	func_write_pm_reg(fadt_addr->pm1a_cnt_blk);
	
	if (fadt_addr->pm1b_cnt_blk) {
		PM1_CNT |=	PM1B;
		func_write_pm_reg(fadt_addr->pm1b_cnt_blk);
	}
	return;
}


static void func_write_pm_reg (U_32 pm_cnt_io) 
{
	REG_OUTW(pm_cnt_io, PM1_CNT);
	wait_io_delay(50);
	return;
}	


static void display_debug (void)
{
		get_s5_sly(dsdt_addr);
		
		int	tmp_facs;
		int pm1a_evt;
		int gts_ptr = 0;
	
		{ /* find _GTS */
			unsigned char *p = (void*)dsdt_addr + sizeof(struct ACPI_SDTH);
			for (; p < (unsigned char*)dsdt_addr + dsdt_addr->length; p++) {
				if (memcmp(p, "_GTS", 4) == 0) {
					gts_ptr = 1; break;
				}
			}
		}
		if (gts_ptr) printf("_GTS found\n");
		else printf("\nGTS not found\n");
		tmp_facs = *(int*)((U_8*)fadt_addr+0x24);
		printf("facs at      : %x	global_lock is : %x\n", tmp_facs, *(int*)(tmp_facs+16) );
		printf("pm1a_evt_len : %x", *(U_8*)((U_8*)fadt_addr+88) );
		tmp_facs = *(int*)((U_8*)fadt_addr+56);
		REG_INW(tmp_facs, pm1a_evt);
		printf("	pm1a_statS reg : %x", pm1a_evt );
		REG_INW(tmp_facs+2, pm1a_evt);
		printf("	pm1a_enabe reg : %x \n", pm1a_evt );
		printf("SMI port     : %x	enable_acpi    : %x	disable_acpi   : %x\n", fadt_addr->smi_cmd, \
		fadt_addr->acpi_enable, fadt_addr->acpi_disable);
		printf("pm1a_cnt_len : %x	", *(U_8*)((U_8*)fadt_addr+89) );
		printf("pm1a_cnt_blk   : %x	pm1b_cnt_blk   : %x\n", fadt_addr->pm1a_cnt_blk, fadt_addr->pm1b_cnt_blk);
		printf("address FLAG : 0x%x\n", ACPI_FLG);
		return;
}
	
	
static void try_apm (void)
{
	printf("\n	try APM....if fail, don't return!!!!");
	
	U_32 code_start = 0;
	U_32 code_len = 0;
	
	__asm__("call 9f\n\t");
	__asm__ __volatile__ (
			"1:\n\t"
			/* 0x8 is a selecotor about 16 bit data segment,whitch in grub4dos gdt */
			"movw $0x8, %ax\n\t"
			"movw %ax, %ds\n\t"
			"movw %ax, %es\n\t"
			"movw %ax, %fs\n\t"
			"movw %ax, %gs\n\t"
			"movw %ax, %ss\n\t"
			"ljmp $0x18, $((2f - 1b) + 0x800)\n\t" /* jmp to 16 bit code segment */
			
			".code16\n\t"
			
			"2:\n\t"
			"movl %cr0, %eax\n\t"
			"andw $0xfffe, %ax\n\t"
			"movl %eax, %cr0\n\t"				 /* enter real mode */
			"ljmp $0, $((3f - 1b) + 0x800)\n\t"  /* reload CS sengment, flush cup cache */
			
			"3:\n\t"
			"xorl %eax, %eax\n\t"
			"movw %ax, %ds\n\t"
			"movw %ax, %es\n\t"
			"movw %ax, %fs\n\t"
			"movw %ax, %gs\n\t"
			"movw %ax, %ss\n\t"
			"movw $0x400, %sp\n\t"		
			
			/* ***********APM code start at here ************ */

				/*connect real mode interface of APM */		
				"movw $0x5301, %ax\n\t"
				"xor %bx, %bx\n\t"
				"int $0x15\n\t"
				
				/* set apm driver vision 1.1+ */
				"movw $0x530E, %ax\n\t"
				"xorw %bx, %bx\n\t"
				"movw $0x0101, %cx\n\t"
				"int $0x15\n\t"
				
				/* TURN OFF SYSTEM BY APM 1.1+ */
				"movw $0x5307, %ax\n\t"
				"movw $1, %bx\n\t"
				"movw $3, %cx\n\t"
				"int $0x15\n\t"
				
				/* if call APM bios fail , halt cpu , now no return*/
				"4:\n\t"
				"hlt\n\t"
				"jmp 4b\n\t"
	); /* end of APM real mode code */
	
	__asm__(
		".code32\n\t"
		"9:	\n\t"
				"popl %0\n\t"
				"movl $(9b - 1b), %1\n\t"
				:"=m"(code_start), "=m"(code_len)
	);

	/* copy code to 0x800 */
	memmove((char*)0x800, (char*)code_start, code_len);
	
	/* run these copyed real mode code at 0x800 */
	__asm__ ("ljmp $0x28, $0x800");
}
