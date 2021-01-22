/* thank goodness gcc will place the above 8 bytes at the end of the b.out		谢天谢地，gcc将上述8个字节放在b.out文件的末尾。请勿在此处插入任何其他asm管路。 
 * file. Do not insert any other asm lines here.
 */
/* it seems gcc use end and/or _end for __bss_end 	似乎gcc使用end和/或end表示bss end */
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

unsigned long long GRUB = 0x534f443442555247LL;/* this is needed, see the following comment.  'GRUB4DOS'这是必需的，请参阅以下命令 */
/* gcc treat the following as data only if a global initialization like the		gcc仅在发生与上述行类似的全局初始化时才将以下内容视为数据。
 * above line occurs.
 */

/* The 40-byte space is structured. 40字节空间是结构化的 */
asm(".long main");		/* actually not used for now 实际上暂时不用 */ //
asm(".long .text");		/* actually not used for now */
asm(".long etext");		/* actually not used for now */
asm(".long .data");		/* actually not used for now */
asm(".long edata");		/* actually not used for now */
asm(".long __bss_start");	/* actually not used for now */
asm(".long .bss");		/* actually not used for now */
asm(".long end");		/* this is the process end 这是进程结束*/
asm(".ascii \"main_end\""); //新版本签名

/* Don't insert any code/data here! 不要在这里插入任何代码/数据！*/
/* these 16 bytes can be used for any purpose. 这16个字节可以用于任何目的*/
asm(".long 0");
asm(".long 0");
asm(".long 0");
asm(".long 0");

/* Don't insert any code/data here! 不要在这里插入任何代码/数据*/

/* a valid executable file for grub4dos must end with these 8 bytes     grub4dos的有效可执行文件必须以这8字节结尾 */
//grub4dos外部命令结束签名
asm(".long 0x03051805");
asm(".long 0xBCBAA7BA");

asm(".globl _start");
asm("_start:");
/* thank goodness gcc will place the above 8 bytes at the end of the program		谢天谢地，gcc会把上面的8个字节放在程序文件的末尾。请勿在此处插入任何其他asm管路。 
 * file. Do not insert any other asm lines here.
 */