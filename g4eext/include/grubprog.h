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
asm(".long main");          //程序入口           4000e8
asm(".long .text");         //程序段起始         4000e8
asm(".long etext");         //扩展程序段起始     402da4
asm(".long .data");         //数据段起始         403890
asm(".long edata");         //扩展数据段起始     4038e8
asm(".long __bss_start");   //bss段起始          4038e8  __bss_start - .text = 代码尺寸
asm(".long .bss");          //                   403900
asm(".long end");           //进程结束           4039b8  end -  .text = 程序实际需求空间尺寸
asm(".ascii \"uefi_end\""); //uefi版本签名

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