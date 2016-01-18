/*
 *  Copyright (C) 2016  Tinybit(tinybit@tom.com) & chenall
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
    umbr 一个通用型的引导程序,程序很简单,只支持LBA,可以支持BIOS+GPT/MBR磁盘上启动
    功能:只是根据用户的定义把启动代码加载到指定位置然后启动就行了.
    0x10 - 0x4F 用户定义空间可以定义4个记录,
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
//#define	DEBUG
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
	.ASCII	"UMBR"
	.BYTE	1

	. = _start1 + 0x10
	//.BYTE 0xE9, 0x50, 0x3E, 0x00, 0x00, 0x00, 0xE0, 0x07, 0x40, 0xA8, 0xA8, 0x00, 0x00, 0x00, 0x00, 0x00
	. = _start1 + 0x20
	. = _start1 + 0x30
	. = _start1 + 0x40
	. = _start1 + 0x50

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


move_down_to_7000:

	pushw	%es
	pushw	$0x0700
	popw	%es		/* ES=0x0700 */

	xorw	%si, %si
	xorw	%di, %DI
#ifdef	DEBUG
	movw	$0x100, %cx	/* move 1 sector */
#else
	movw	$0x80, %cx	/* move 1 sector */
#endif
	cld
	repz movsl

	ljmp	$0x0700, $(3f - _start1)
3:
	popw	%es
	ret

1:
	/* we are loaded by BIOS, only the MBR sector is loaded. */

	/* CS:0000=_start1, SS:SP=0000:0580 */
	/* DS:0000=_start1, ES=0x07E0 */
	/* CS=DS=BX */

	/* since only one sector is loaded, we assume segment=0x600. */

	cmpw	$0x0700, %bx
	jbe	3f

	/* move this sector down to 0700:0000 */

	call	move_down_to_7000
	
	/* CS=0x700. */
3:

	/* setup a safe stack */

	movw	$0x3E00, %sp	/* SS:SP=0000:3E00 */
	cmpw	$0x03E0, %bx	/* BX=DS, BX=CS or BX > 0x600 */
	jnb	3f
	addw	%sp, %sp	/* SS:SP=0000:6000 */
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
	cmpb	$0x80,%DL
	JNS		90f
	movb	$0x80, %dl	/* try hard drive first */
90:
	movw	$0x7010,%SI
	movw	$4,%cx
100:
	movw	(%si),%AX
	testw	%AX,%AX
	JZ		110f			//校验信息为空跳过
	pushw	%DX				//备份dx的值
	PUSHW	%AX
	CALL	readdisk		//读取数据
	JC		110f			//磁盘读取失败
	CALL	_check_sum		//校验
	POPW	%DX
#ifdef DEBUG
	CALL	print_hex
#endif
	xorw	%dx,%AX
	popw	%DX				//恢复dx
	JNZ		110f			//校验失败继续下一个
	cmpw	$0x07c0,6(%si)	//如果是从0x7c00启动的就认为是PBR,直接修改0x1c处的值
	JNE		_make_boot
	pushl	8(%SI)
	popl	0x7C1C
_make_boot://用于远程跳转启动,具体的启动位置在DAP数据包中.DS:SI指向DAP数据包.
#ifdef DEBUG
	movw	$0xBBBB,%ax
	CALL	print_hex
#endif
	pushl	4(%SI)
//	pushw	6(%si)
//	pushw	4(%si)
	lret	
110:
#ifdef DEBUG
	CALL 	print_hex
#endif
	CALL	print_next
	addw	$0x10,%si
	loop	100b
Show_errmsg:
	movw	$(message_string - _start1), %SI
#ifdef DEBUG
//	incb	%cs:(%si)
#endif
	call	print_message	/* CS:SI points to message string */
	jmp Show_errmsg


//简单检验代码
_check_sum:
	pushw	%cx
	pushl	%ebx
	pushw	%es
	pushw	%si
	movw	6(%si),%es
	movw	2(%si),%cx
	movw	4(%SI),%SI
	xorl	%ebx,%ebx		//清除bx值用于后面的计算
	cmpw	$0x7F,%cx
	JBE		100f
	movw	$0x7f,%cx
100:
	shlw	$7,%CX
110:
	lodsl	%es:(%si)		//读入4个字节到EAX加速校验速度
	xorl	%eax,%ebx
	LOOP	110b
	movw	%bx,%AX
	shrl	$0x10,%EBX
	xorw	%bx,%ax
#ifdef DEBUG
	CALL 	dumpax
#endif
	popw	%si
	popw	%es
	popl	%ebx
	popw	%cx
	ret
#ifdef DEBUG
dumpax:
	movw	$4,%CX
	movw	$(crc_0 - _start1),%si
ascii_to_hex:
	ROL 	$4,%AX
	movb	%AL,%BL
	andb	$0xF,%BL
	ADDB	$0x30,%BL
	CMPb	$0x3a,%BL
	JL		111f
	addb	$0x7,%BL
111:
	movb	%BL,%CS:(%si)
	INCW	%SI
	decw	%CX
	JNZ		ascii_to_hex
	RET
#endif
/*BIOS INT13 扩展读 0x4200,调用之前先设置好DS:SI,和dl的值
 DS:SI disk address packet, 
params:
 	AH	42h = function number for extended read
	DL	drive index (e.g. 1st HDD = 80h)
	DS:SI	segment:offset pointer to the DAP, see below
return:
	CF	Set On Error, Clear If No Error
	AH	Return Code
*/
readdisk:
	pushw	%CX
	pushl	%EBX
	pushw	2(%si)
	pushw	6(%si)
	pushl	8(%si)
	xorl	%EBX,%EBX		
	movw	2(%si),%CX
9:
	cmpw	$0x80,%CX
	jl		10f
	movw	$0x7F,2(%si)	//一次最多读127个扇区
	JMP		12f
10:
	movw	%CX,2(%si)
12:
	movw	$0x10,(%si)
	movw	$0x4200, %ax
	call	int13
	JC		101f		//读取出错了
	movw	2(%si),%bx	//读取成功的扇区数
#ifdef DEBUG
	MOVW	%BX,%AX
	CALL	print_hex
#endif
	subw	%bx,%CX
	JLE		100f		//全部读完了
	addl	%ebx,8(%si)	//下一次读取扇区位置
	shlw	$5,%bx		
	addw	%bx,6(%SI)	//修改缓冲区偏移
	JMP		9b
100:
101:
	popl	8(%SI)
	popw	6(%SI)
	popw	2(%si)
	popl	%EBX
	popw	%cx
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
	popw	%DS
	ret
	/* prints string CS:SI (modifies AX BX SI) */

print_next:
	pushaw
	movw	$(try_next - _start1), %si
	incb	%cs:(%si)
#ifdef DEBUG
	movw	$(crc_err_string - _start1), %SI
#endif
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
	.ascii	"0Urr\r\n\0"
#ifdef DEBUG
crc_err_string:
	.ascii	"CRC:"
crc_0:
	.ascii	"0000"
#endif
try_next:
	.ascii	"0try\r\n\0"
	/* Make sure the above code does not occupy the partition table */
#ifndef DEBUG
	/* offset value here must be less than or equal to 0x1b8 */
	. = . - ((. - _start1) / 0x1b9)

	. = _start1 + 0x1be	/* The partition table */

#endif
	. = _start1 + 0x1fe	/* boot signature */
	.word	0xaa55
#ifdef DEBUG
print_hex:
	pushaw
	MOVW	%AX,%DX
	movw	$4,%cx
print_hex1:
	ROL 	$4,%DX
	movb	%DL,%AL
	andb	$0xF,%AL
	ADDB	$0x30,%AL
	CMPb	$0x3a,%AL
	JL		OUTP
	addb	$0x7,%al
OUTP:
	xorw	%bx, %bx
	movb	$0x0e, %ah
	int	$0x10		
	DECW	%CX
	CMPW	$0,%CX
	JA print_hex1
	MOVB	$0x20,%Al
	movb	$0x0e, %ah
	int	$0x10	

	movb	$0x00, %ah
	int	$0x16
	popaw
	RET
	. = _start1 + 0x300
	.word 	0
#endif