/* thank goodness gcc will place the above 8 bytes at the end of the b.out
 * file. Do not insert any other asm lines here.
 */
/* it seems gcc use end and/or _end for __bss_end */
#if 1
	#define	__BSS_END	end
#else
	#define	__BSS_END	_end
#endif

#if 1
	#define	__BSS_START	__bss_start
#elif 1
	#define	__BSS_START	edata
#else
	#define	__BSS_START	_edata
#endif
#define PROG_PSP (char*)(((int)&__BSS_END + 16) & ~0x0F)
extern int __BSS_END;
extern int __BSS_START;

unsigned long long GRUB = 0x534f443442555247LL;/* this is needed, see the following comment. */
/* gcc treat the following as data only if a global initialization like the
 * above line occurs.
 */

/* The 40-byte space is structured. */
asm(".long main");		/* actually not used for now */
asm(".long .text");		/* actually not used for now */
asm(".long etext");		/* actually not used for now */
asm(".long .data");		/* actually not used for now */
asm(".long edata");		/* actually not used for now */
asm(".long __bss_start");	/* actually not used for now */
asm(".long .bss");		/* actually not used for now */
asm(".long end");		/* this is the process length */
asm(".ascii \"main\"");
asm(".ascii \"_end\"");

/* Don't insert any code/data here! */

/* these 16 bytes can be used for any purpose. */
asm(".long 0");
asm(".long 0");
asm(".long 0");
asm(ASM_BUILD_DATE);	//asm(".long 0");

/* Don't insert any code/data here! */

/* a valid executable file for grub4dos must end with these 8 bytes */
asm(".long 0x03051805");
asm(".long 0xBCBAA7BA");

/* thank goodness gcc will place the above 8 bytes at the end of the program
 * file. Do not insert any other asm lines here.
 */