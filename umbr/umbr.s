/*
 *  Copyright (C) 2010  Tinybit(tinybit@tom.com) & chenall
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
    umbr 一个通用型的mbr多功能引导程序,程序很简单,只支持LBA,主要用于在GPT磁盘上启动
    功能:只是根据用户的定义把启动代码加载到指定位置然后启动就行了.
    0x10 - 0x5F 用户定义空间可以定义4个记录,
    格式如下(参考DiskAddressPacket,前两个字节数据用于校验,后面的一样):
    WORD CRC;
　　WORD BlockCount;　　// 启动代码块数(以扇区为单位)
　　DWORD BufferAddr;　　// 传输缓冲地址(segment:offset),也是该代码的启动地址
　　QWORD BlockNum;　　　// 启动代码在磁盘上的位置(LBA)
    一个记录例子:
    C763 2B02 0000E007 22000000 00000000
    C763 是第一个扇区的校验码(具体校验方法在参考源码), 2B02 就是0X22B 启动代码占用的扇区数,,0000E007,,加载启动位置07E0:0000,22000000 00000000,启动代码在磁盘中的起始扇区

    本代码修改自tinybit编写的wee63.mbr中的wee63start.S文件
 */

	.file	"umbr.S"

	.text

	.globl	start, _start

start:
_start:
_start1:

	/* Tell GAS to generate 16-bit real mode instructions */

	.code16

	. = _start1 + 0x00

	/* No cli, we use stack! BIOS or caller usually sets SS:SP=0000:0400 */

	/* Acer will hang up on USB boot if the leading byte is 0xEB(jmp).
	 * You might want to use 0x90 0x90 or 0x33 0xC0(xor AX,AX) instead.
	 */
	jmp	1f

	. = _start1 + 0x02

	.byte	0x90	/* MS uses it to indicate a valid floppy */

	. = _start1 + 0x08
	.byte 0xea
	.long 0x07C00000

	. = _start1 + 0x60

1:
	cli
	xorw	%bx, %bx
	movw	%bx, %ss
	movw	$0x580, %sp		/* temp safe stack space */
	call	1f
1:
	popw	%bx			/* Instruction Pointer of 1b */
	subw	$(1b - _start1), %bx	/* CS:BX=_start1 */

	shrw	$4, %bx
	movw	%cs, %ax
	addw	%ax, %bx		/* BX:0000=_start1 */

	/* we are booted by BIOS, or whole image already loaded */

	/* Let CS:0000=_start1 */
	pushw	%bx			/* BX:0000=_start1 */

	#;pushw	$(1f - _start1)
	.byte	0x6A, (1f - _start1)

	lret
	. = . - (. - _start1) / 0x80
1:
	/* CS:0000=_start1, SS:SP=0000:0580 */
/* begin characteristics distinguish this sector from others */
	movw	%bx, %ds
	pushw	$0x07E0
	popw	%es	/* ES=0x07E0 */

/* end characteristics distinguish this sector from others */
	/* DS:0000=_start1, ES=0x07E0 */
	jmp	1f


move_down_to_7C00:

	pushw	%es
	pushw	$0x07C0
	popw	%es		/* ES=0x07C0 */

	xorw	%si, %si
	xorw	%di, %di
	movw	$0x80, %cx	/* move 1 sector */
	cld
	repz movsl

	ljmp	$0x07C0, $(3f - _start1)
3:
	popw	%es
	ret

1:
	/* we are loaded by BIOS, only the MBR sector is loaded. */

	/* CS:0000=_start1, SS:SP=0000:0580 */
	/* DS:0000=_start1, ES=0x07E0 */
	/* CS=DS=BX */

	/* since only one sector is loaded, we assume segment=0x7C0. */

	cmpw	$0x07C0, %bx
	jbe	3f

	/* move this sector down to 07C0:0000 */

	call	move_down_to_7C00
	
	/* CS=0x7C0. */
3:

	/* setup a safe stack */

	movw	$0x3E00, %sp	/* SS:SP=0000:3E00 */
	cmpw	$0x03E0, %bx	/* BX=DS, BX=CS or BX > 0x7C0 */
	jnb	3f
	addw	%sp, %sp	/* SS:SP=0000:7C00 */
3:
	sti	/* now we have enough stack room. */

	/* clear the destination sector */
	xorw	%ax, %ax
	movw	%ax, %ds	/* let DS=0 to match SS=0 */
	xorw	%di, %di
	movw	$0x100, %cx
	cld
	repz stosw

//启动代码开始
	movb	$0x80, %dl	/* try hard drive first */
	movw	$0x7c10,%si
	movw	$4,%cx

100:
	call	_load
	testw	%ax,%ax
	jnz	_make_boot
	addw	$0x10,%si
	call	print_next
	loop	100b
Show_errmsg:
	movw	$(message_string - _start1), %si
	incb	%cs:(%si)
	call	print_message	/* CS:SI points to message string */
	jmp Show_errmsg

_make_boot://用于远程跳转启动,具体的启动位置在DAP数据包中.DS:SI指向DAP数据包.
#define BOOT_TYPE 0

#if (BOOT_TYPE==1)
	//第一个版本使用以下方法需要17个字节
	pushw	4(%si)		//FF7404
	pushw	6(%si)		//FF7406
	popw	0x7c0b		//8F060B7C
	popw	0x7c09		//8F06097C
	jmp	_boot		//E9XXXX
#endif

#if (BOOT_TYPE==2)
	//第二个版本以用以下方法需要15个字节(PS: 使用AX可以节省两个字节)
	movw	4(%si),%ax	//8B4404
	movw	%ax,0x7c09	//A3097c
	movw	6(%si),%ax	//8B4406
	movw	%ax,0x7c0b	//A30B7C
	jmp	_boot		//E9XXXX
#endif

#if (BOOT_TYPE==3)
	//使用MOVSW方式,编译后需要13个字节
	pushw	%ds
	popw	%es
	addw	$4,%si
	movw	$0x7c09,%di
	movsw
	movsw
	jmp	_boot		//转到0x7c08
#endif

#if (BOOT_TYPE==0)
	//使用lret只需要7个字节,并且不需要使用前面的EA指令,应该没有什么副作用吧
	pushw	6(%si)
	pushw	4(%si)
	lret
#endif

_load:
	xorw	%ax,%ax
	cmpw	(%si),%ax	//判断校验信息为空
	je	112f
	pushw	%dx		//保存DX
	pushw	(%si)		//保存检验信息
	pushw	2(%si)		//保存要读取的扇区数量
	movw	$0x1,2(%si)	//首先只读取第一个扇区用于校验
	call	readdisk
	popw	2(%si)		//恢复扇区数量
	popw	%dx		//恢复校验信息到dx寄存器
	call	_check_sum	//数据简单校验
	popw	%dx		//恢复EDX
	testw	%ax,%ax
	jz	112f
	call	readdisk
	incw	%ax		//只是让EAX返回值非0
112:
	ret

//简单检验代码
_check_sum:
	pushw	%cx
	pushw	%bx
	pushw	%es
	pushw	%si
	movw	6(%si),%es
	movw	4(%si),%si
	xorw	%bx,%bx
	movw	$0xDB,%cx
110:
	lodsw	%es:(%si),%ax
	xorw	%ax,%bx
	loop 110b
	movw	%bx,%ax
111:
	cmpw	%bx,%dx
	je	112f
	xorw	%ax,%ax
112:
	popw	%si
	popw	%es
	popw	%bx
	popw	%cx
	ret

//BIOS INT13 扩展读 0x4200,调用之前先设置好DS:SI,和dl的值
// DS:SI disk address packet, 
readdisk:
	movw	$0x10,(%si)
	movw	$0x4200, %ax
	call	int13
	ret

int13:
	pushw	%ds
	pushw	%es
//	pushw	%bx
	pushw	%dx
	pushw	%si
	pushw	%di
	pushw	%bp
	stc
	int	$0x13
	popw	%bp
	popw	%di
	popw	%si
	popw	%dx
//	popw	%bx
	popw	%es
	popw	%ds
	ret
	/* prints string CS:SI (modifies AX BX SI) */

print_next:
	pushaw
	movw	$(try_next - _start1), %si
	incb	%cs:(%si)
	call	print_message
	popaw
	ret

3:
	xorw	%bx, %bx	/* video page 0 */
	movb	$0x0e, %ah	/* print char in AL */
	int	$0x10		/* via TTY mode */

print_message:
	lodsb	%cs:(%si), %al	/* get token */
	cmpb	$0, %al		/* end of string? */
	jne	3b
	movb	$0x00, %ah
	int	$0x16
	ret

message_string:
	.ascii	"0Urr!\r\n\0"
try_next:
	.ascii	"0try\r\n\0"
	/* Make sure the above code does not occupy the partition table */

	/* offset value here must be less than or equal to 0x1b8 */
	. = . - ((. - _start1) / 0x1b9)

	. = _start1 + 0x1be	/* The partition table */

	. = _start1 + 0x1fe	/* boot signature */

	.word	0xaa55

