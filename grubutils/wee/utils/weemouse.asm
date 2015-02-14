; Cute Mouse Driver - a tiny mouse driver for DOS
;
; Copyright (c) 2010 Tinybit <tinybit@tom.com>
; Copyright (c) 1997-2002 Nagy Daniel <nagyd@users.sourceforge.net>
;
; BIOS wheel mouse support: Ported from Konstantin Koll's public
; domain code into Cute Mouse by Eric Auer 2007 (places marked -X-)
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA
; -X- no longer: 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Tinybit's notes:
;
; The driver now only supports BIOS PS2 mouse interface. And no wheel support,
; and no 3-button support.
;
; Please locate "org	100h" and start reading the source code there.
;
; Use jwasm to compile. Jwasm runs on many platforms including DOS/Linux.
;
;	jwasm -mt -mz  ctmouse.asm
;
; It will generate the ctmouse.EXE file.
;
;	jwasm -mt -bin ctmouse.asm
;
; It will generate the ctmouse.BIN file. You should rename it to ctmouse.COM.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

FOOLPROOF	 = 1		; check driver arguments validness

.386

;------------------------------------------------------------------------

OPCODE_MOV_AL	equ <db 0B0h>
OPCODE_MOV_CL	equ <db 0B1h>
OPCODE_MOV_DL	equ <db 0B2h>
OPCODE_MOV_BL	equ <db 0B3h>
OPCODE_MOV_AH	equ <db 0B4h>
OPCODE_MOV_CH	equ <db 0B5h>
OPCODE_MOV_DH	equ <db 0B6h>
OPCODE_MOV_BH	equ <db 0B7h>
OPCODE_MOV_AX	equ <db 0B8h>
OPCODE_MOV_CX	equ <db 0B9h>
OPCODE_MOV_DX	equ <db 0BAh>
OPCODE_MOV_BX	equ <db 0BBh>

OPCODE_ADD_AX	equ <db 05h>
OPCODE_AND_AX	equ <db 25h>
OPCODE_XOR_AX	equ <db 35h>
OPCODE_OR_AL	equ <db 0Ch>
OPCODE_AND_AL	equ <db 24h>
OPCODE_XOR_AL	equ <db 34h>
OPCODE_CMP_AL	equ <db 3Ch>

OPCODE_CMP_DI	equ <db 81h,0FFh>

;------------------------------------------------------------------------

_ARG_DI_	equ <word ptr [bp]>
_ARG_SI_	equ <word ptr [bp+2]>
_ARG_BP_	equ <word ptr [bp+4]>

_ARG_BX_	equ <word ptr [bp+8]>
_ARG_DX_	equ <word ptr [bp+10]>
_ARG_CX_	equ <word ptr [bp+12]>
_ARG_AX_	equ <word ptr [bp+14]>
_ARG_ES_	equ <word ptr [bp+16]>
_ARG_DS_	equ <word ptr [bp+18]>

nl		equ <13,10>
eos		equ <0>

POINT		struc
  X		dw 0
  Y		dw 0
POINT ends

PS2serv		macro	serv:req,errlabel ; :vararg
		mov	ax,serv
		int	15h

	ifnb <errlabel>
		jc	errlabel
		test	ah,ah
		jnz	errlabel
	endif
endm


;		SEGMENTS DEFINITION

; .model use16 tiny
assume ss:nothing

@TSRcode equ <DGROUP>
@TSRdata equ <DGROUP>
; TSR ends at TSRend, roughly line 3000 of the 4000 lines of ctmouse

TSRcref	equ <offset @TSRcode>	; offset relative TSR code group
TSRdref	equ <offset @TSRdata>	;	- " -	      data
coderef	equ <offset @code>	; offset relative main code group
dataref	equ <offset @data>	;	- " -	       data

.code
		org	0
TSRstart	label byte
		org	100h		; .COM style program
start:		jmp	real_start
TSRavail	label byte		; initialized data may come from here

		org	103h

swapmask	db	0		; 00000011b for lefthand

;----- old interrupt vectors -----

oldint33	dd	?		; old INT 33 handler address
oldint10	dd	?		; old INT 10 handler address

		org	110h
if 1
IDstring	db 'grub4dos',0		; lower case for installed in dos
else
IDstring	db 'GRUB4DOS',0		; upper case for installed in grub4dos
endif
szIDstring = $ - IDstring

		even
spritebuf	db	3*16 dup (?)	; copy of screen sprite in modes 4-6

;----- application state -----

		even
SaveArea = $

RedefArea = $

screenmask	dw	0011111111111111b	; user defined screen mask
		dw	0001111111111111b
		dw	0000111111111111b
		dw	0000011111111111b
		dw	0000001111111111b
		dw	0000000111111111b
		dw	0000000011111111b
		dw	0000000001111111b
		dw	0000000000111111b
		dw	0000000000011111b
		dw	0000000111111111b
		dw	0000000011111111b
		dw	0011000011111111b
		dw	1111100001111111b
		dw	1111100001111111b
		dw	1111110011111111b
cursormask	dw	0000000000000000b	; user defined cursor mask
		dw	0100000000000000b
		dw	0110000000000000b
		dw	0111000000000000b
		dw	0111100000000000b
		dw	0111110000000000b
		dw	0111111000000000b
		dw	0111111100000000b
		dw	0111111110000000b
		dw	0111110000000000b
		dw	0110110000000000b
		dw	0100011000000000b
		dw	0000011000000000b
		dw	0000001100000000b
		dw	0000001100000000b
		dw	0000000000000000b
nocursorcnt	dw	0FFFFh		; 0=show cursor, else hide cursor
		even
szDefArea = $ - RedefArea		; initialized by softreset_21


rangemax	POINT	<?,?>		; horizontal/vertical range max
upleft		POINT	<?,?>		; upper left of update region
lowright	POINT	<?,?>		; lower right of update region
pos		POINT	<?,?>		; virtual cursor position
granpos		POINT	<?,?>		; granulated virtual cursor position
UIR@		dd	?		; user interrupt routine address

		even
ClearArea = $

;sensround	POINT	<?,?>		; rounding error in applying
					;  sensitivity for mickeys
;rounderr	POINT	<?,?>		; same in conversion mickeys to pixels
		even
szClearArea1 = $ - ClearArea		; cleared by setpos_04

		even
szClearArea2 = $ - ClearArea		; cleared by setupvideo

callmask	db	?		; user interrupt routine call mask
mickeys		POINT	<?,?>		; mouse move since last access
BUTTLASTSTATE	struc
  counter	dw	?
  lastrow	dw	?
  lastcol	dw	?
BUTTLASTSTATE ends
buttpress	BUTTLASTSTATE <?,?,?>,<?,?,?>,<?,?,?>
buttrelease	BUTTLASTSTATE <?,?,?>,<?,?,?>,<?,?,?>
;wheel		BUTTLASTSTATE <?,?,?>	; wheel counter since last access
;wheelUIR	db	?		; wheel counter for UIR
		even
szClearArea3 = $ - ClearArea		; cleared by softreset_21
szSaveArea = $ - SaveArea

;		INITIALIZED DATA

		even
TSRdata		label byte

; ERRIF (TSRdata lt TSRavail) "TSR uninitialized data area too small!"

;----- driver and video state begins here -----

		even
granumask	POINT	<-1,-1>

textbuf		label	word
buffer@		dd	?		; pointer to screen sprite copy
;cursor@	dw	-1,0		; cursor sprite offset in videoseg;
					; -1=cursor not drawn
cursor@		dw	0,0		; cursor sprite offset in videoseg;
					; 0=upper left corner

videoseg	equ <cursor@[2]>	; 0=not supported video mode

UIRunlock	db	1		; 0=user intr routine is in progress
;videolock	db	1		; drawing: 1=ready,0=busy,-1=busy+queue
;newcursor	db	0		; 1=force cursor redraw

;----- table of pointers to registers values for RIL -----

REGSET		struc
  rgroup	dw	?
  regnum	db	?
  regval	db	?
REGSET ends

	db 0	; JWASM would use 0fch, "CLD" as pad byte??
		even
		dw	(vdata1end-vdata1)/(size REGSET)
vdata1		REGSET	<10h,1>,<10h,3>,<10h,4>,<10h,5>,<10h,8>,<08h,2>
vdata1end	label word
		dw	(vdata2end-vdata2)/(size REGSET)
vdata2		REGSET	<10h,1,0>,<10h,4,0>,<10h,5,1>,<10h,8,0FFh>,<08h,2,0Fh>
vdata2end	label byte

;		IRQ HANDLERS

		assume	ds:@TSRdata

PS2handler	proc
		assume	ds:nothing,es:nothing
		push	ds
		push	es
		pusha
		push	cs
		pop	ds
		assume	ds:@TSRdata

		mov	bp,sp

		; bp+30		buttons and flags
		; bp+28		X movement
		; bp+26		Y movement
		; bp+24		Z movement
		; bp+22		return CS
		; bp+20		return IP
		; bp+18		DS
		; bp+16		ES
		; bp+14		AX
		; bp+12		CX
		; bp+10		DX
		; bp+ 8		BX
		; bp+ 6		SP
		; bp+ 4		BP
		; bp+ 2		SI
		; bp		DI

		mov	al,[bp+30]	; buttons and flags
					; bit 0: left button pressed
					; bit 1: right button pressed
					; bit 2: reserved(or Middle?)
					; bit 3=1: reserved
					; bit 4: X is negative
					; bit 5: Y is negative
					; bit 6: X overflow
					; bit 7: Y overflow

		mov	bl,al		; backup, xchg will restore
		shl	al,3		; CF=Y sign bit, MSB=X sign

		sbb	ch,ch		; extend Y sign bit

		cbw			; extend X sign bit
		mov	al,[bp+28]	; AX=X movement
		xchg	bx,ax		; X to BX, buttons to AL
		mov	cl,[bp+26]	; CX=Y movement

		neg	cx		; reverse Y movement
		test	al,00000011b	; check the L and R buttons
		jpe	@@swapeven
		xor	al,[swapmask]	; buttons not same
@@swapeven:
		;call	mouseupdate	; AL flags AH wheel BX X CX Y
;			Update mouse status
;
; In:	AL			(new buttons state, 2 or 3 LSB used)
;	AH			(wheel movemement)
;	BX			(X mouse movement)
;	CX			(Y mouse movement)
; Out:	none
; Use:	callmask, granpos, mickeys, UIR@
; Modf:	AX, CX, DX, BX, SI, DI, wheel, wheelUIR, UIRunlock
; Call:	updateposition, updatebutton, drawcursor
;
;mouseupdate	proc
		and	ax,3
		xchg	di,ax			; keep btn, wheel state in DI

;----- update mickey counters and screen position

		xchg	ax,bx			; OPTIMIZE: instead MOV AX,BX
		xor	bx,bx
		call	updateposition

		xchg	ax,cx
		mov	bl,2
		call	updateposition
		or	cl,al			; bit 0=mickeys change flag

;----- update wheel movement

		;mov	ax,[mickeys.Y]
		;xchg	ax,di			; retrieve button, wheel info
		;xchg	dx,ax			; OPTIMIZE: instead MOV DX,AX
		mov	dx,di
		;mov	al,dh			; wheel: signed 4 bit value
		;xor	al,00001000b	; =8
		;sub	al,00001000b	; =8	; sign extension AL[0:3]->AL

;----- update buttons state

		mov	dh,dl			; DH=buttons new state
		xchg	dl,[buttstatus]
		xor	dl,dh			; DL=buttons change state

	jz @@btnfastz

		xor	bx,bx			; buttpress array index
		mov	al,00000010b		; mask for button 1
		call	updatebutton
		mov	al,00001000b		; mask for button 2
		call	updatebutton
		mov	al,00100000b		; mask for button 3
		call	updatebutton

@@btnfastz:


;----- call User Interrupt Routine (CX=events mask)

		dec	[UIRunlock]
	jnz @@uirnz
		; user proc not running
		and	cl,[callmask]
	;jmp short @@maskz
	jz @@maskz
		OPCODE_MOV_BX
buttstatus	db 0,0
		;xchg	bh,[wheelUIR]
		mov	ax,[granpos.X]
		mov	dx,[granpos.Y]
		xchg	ax,cx
		; set CX=DX=0 and DISKGEN work fine!!
		; so we are sure CX/DX are not used by DISKGEN.
		;mov	cx,0
		;mov	dx,0

		; set SI=DI=0 and EDIT/QEDIT/GHOST work fine!!
		; But DISKGEN uses SI/DI.
		mov	si,[mickeys.X]
		mov	di,[mickeys.Y]
		push	ds

		sti
		call	[UIR@]

		pop	ds
@@maskz:
		call	drawcursor
@@uirnz:

		inc	[UIRunlock]
;		ret
;mouseupdate	endp

		popa
		pop	es
		pop	ds
@@dummyPS2::	retf			; no-op callback handler
PS2handler		endp
		assume	ds:@TSRdata	; added...?

; In:	AX			(mouse movement)
;	BX			(offset X/offset Y)
; Out:	AX			(1 - mickey counter changed)
; Use:	rangemax, granumask
; Modf:	DX, SI, sensround, mickeys, rounderr, pos, granpos
;
updateposition	proc
		test	ax,ax
		jz	@@uposret
if 0
;		cmp	ax,255
;		jg	@@uposret
;		cmp	ax,-255
;		jl	@@uposret

;		cmp	ax,63
;		jg	@@newmickeys
;		cmp	ax,-63
;		jl	@@newmickeys

		cmp	ax,31
		jg	@@upgt31
		cmp	ax,-31
		jge	@@uple31
@@upgt31:
		; times 3
		;mov	dx,ax
		;add	ax,ax
		;add	ax,dx
		; times 4
		shl	ax,2
		jmp	@@newmickeys
@@uple31:
		cmp	ax,15
		jg	@@upgt15
		cmp	ax,-15
		jge	@@uple15
@@upgt15:
		; times 2
		;add	ax,ax
		; times 3
		mov	dx,ax
		add	ax,ax
		add	ax,dx
		jmp	@@newmickeys
@@uple15:
		cmp	ax,7
		jg	@@upgt7
		cmp	ax,-7
		jge	@@uple7
@@upgt7:
		; times 1.5
		;mov	dx,ax
		;add	ax,ax
		;add	ax,dx
		;sar	ax,1
		; times 2
		add	ax,ax
		;jmp	@@newmickeys
@@uple7:
else
		mov	si,ax
		jns	@@upns
		neg	ax
@@upns:
		shr	ax,3
		inc	ax
		imul	si		; DX changed
		;imul	si		; DX changed
		;shrd	ax,dx,3
endif

@@newmickeys:

		add	word ptr mickeys[bx],ax
		add	ax,word ptr pos[bx]

;----- cut new position by virtual ranges and save

@savecutpos::
		mov	dx,word ptr rangemax[bx]
		cmp	ax,dx
		jge	@@cutpos

		mov	dx,0
		cmp	ax,dx
		jg	@@cpg

@@cutpos:	xchg	ax,dx			; OPTIMIZE: instead MOV AX,DX
@@cpg:
		mov	word ptr pos[bx],ax	; new position
		and	al,byte ptr granumask[bx]
		mov	word ptr granpos[bx],ax	; new granulated position
@@uposdone:	mov	ax,1
@@uposret:	ret
updateposition	endp

; In:	AL			(unrolled press bit mask)
;	CL			(unrolled buttons change state)
;	DL			(buttons change state)
;	DH			(buttons new state)
;	BX			(buttpress array index)
; Out:	CL
;	DX			(shifted state)
;	BX			(next index)
; Use:	granpos
; Modf:	AX, SI, buttpress, buttrelease
;
updatebutton	proc
		shr	dx,1
	jnc @@ubnc
		; button changed
		mov	si,TSRdref:buttpress
		test	dl,dl
	js @@ubs
		; button not pressed
		add	al,al			; indicate that it released
		mov	si,TSRdref:buttrelease
@@ubs:
		inc	word ptr [si + bx + offset BUTTLASTSTATE.counter]
@lastpos::	or	cl,al
		mov	ax,[granpos.Y]
		mov	[si + bx + offset BUTTLASTSTATE.lastrow],ax
		mov	ax,[granpos.X]
		mov	[si + bx + offset BUTTLASTSTATE.lastcol],ax
@@ubnc:
		add	bx,size BUTTLASTSTATE	; next button
		ret
updatebutton	endp

;		END OF IRQ HANDLERS

;=============	INT 10 HANDLER =======================

int10handler	proc
		assume	ds:nothing,es:nothing
		cld
		test	ah,ah			; set video mode?
		jz	@@setmode
		cmp	ah,11h			; font manipulation function
		je	@@setnewfont
		cmp	ax,4F02h		; VESA set video mode?
		je	@@setmode
@@jmpold10:
		jmp	cs:[oldint10]
;		db	0EAh		; JMP FAR dword
;oldint10	dd	?


@@setnewfont:	cmp	al,10h
		jb	@@jmpold10
		cmp	al,20h
		jae	@@jmpold10
		;j	@@setmode

;===== set video mode or activate font

@@setmode:
		pushf
		call	cs:[oldint10]

		push	ds
		push	es
		pusha

		push	cs
		pop	ds
		assume	ds:@TSRdata

		call	setupvideo
@@exitINT10:
		popa
		pop	es
		pop	ds

		iret

int10handler	endp

;============== END OF INT 10 HANDLER =================

		assume	ds:@TSRdata
		assume	es:nothing		; -X-

;		Draw mouse cursor

drawcursor	proc
		; self-modify
		; let this instruction be 'ret'
		; to avoid re-entrant of this function
		; this must be the first instruction, so do not insert other
		; instruction here
		mov	byte ptr [drawcursor],0C3h	; C6, 06, ...

		mov	cx,[nocursorcnt]	; 0 or positive=show cursor
		test	cx,cx			; hide cursor?
		js	@@drawret		; yes, exit

		mov	cx,[videoseg]
		jcxz	@@drawret		; exit if nonstandard mode

		mov	ax,[granumask.Y]
		inc	ax
		mov	bx,[granpos.Y]	; cursor position Y
		mov	ax,[granpos.X]	; cursor position X
		jz	graphcursor		; jump if graphics mode
		;jz	@@drawret

;===== text mode cursor

		; AX/BX=(cursor position X/Y)
		xor	dx,dx
		mov	es,dx			; ES=0

		xchg	di,ax			; DI=X
		mov	al,[bitmapshift]
		dec	ax			; dec AL
		xchg	cx,ax			; save CX in AX, CL=AL
		sar	di,cl			; DI=column*2
		xchg	cx,ax			; restore CX from AX
		xchg	ax,bx			; AX=Y
		sar	ax,2			; AX=row*2=Y/4
		mov	dx,es:[44Ah]		; screen width

		imul	dx			; AX=row*screen width
		add	ax,es:[44Eh]		; video page offset
		add	di,ax

		; DI=text mode video memory offset(=row*[44Ah]*2+column*2)

		cmp	di,[cursor@]
		jz	@@drawret		; exit if position not changed

		push	di

		; Restore old screen contents
if 1
		les	di,dword ptr [cursor@]	; ES=video memory segment
						; DI=old position

		;inc	di
		;jz	@@restorescreenok	; cursor not drawn, need not restore
		; cursor drawn, now restore the old screen
		;sub	[cursor@],di	; mov [cursor@],-1 for cursor not drawn
		;dec	di

		mov	si,TSRdref:textbuf
		lodsw				; saved text after drawn
		cmp	ax,es:[di]		; changed?
		jnz	@@restorescreenok	; yes, do not restore
		movsw				; no, use text before drawn
@@restorescreenok:
else
		les	di,dword ptr [cursor@]	; ES=video memory segment
		inc	di
		jz	@@restorescreenok	; cursor not drawn, need not restore
		;dec	di
		xor	byte ptr es:[di],77h
@@restorescreenok:
endif
		pop	di

;----- draw software text mode cursor
if 1
		mov	[cursor@],di		; save cursor position
		mov	ax,es:[di]		; get text under cursor
		mov	textbuf[2],ax		; save text before drawn
		;and	ax,77FFh		; get fore/back ground color
		;xor	ax,7700h		; reverse color
		xor	ah,77h			; reverse color
		stosw				; draw to new position
		mov	textbuf[0],ax		; save text after drawn
else
		mov	[cursor@],di
		xor	byte ptr es:[di+1],77h
endif
@@drawret::
		; restore first byte of mov instruction to enable function
		mov	byte ptr [drawcursor],0C6h
		ret

drawcursor	endp



;		Draw graphics mode mouse cursor

graphcursor	proc
		;mov	si,16			; cursor height
		;add	si,bx
		lea	si,[bx+16]

		xchg	ax,bx
		xor	dx,dx
		neg	ax
	jge @@gcge
		neg	ax
		xchg	ax,dx
@@gcge:
		mov	[spritetop],ax
		mov	ax,[screenheight]
		cmp	si,ax
	jl @@gclt
		xchg	si,ax			; OPTIMIZE: instead MOV SI,AX
@@gclt:
		sub	si,dx			; =spriteheight
		push	si			;  =min(16-ax,screenheight-dx)
		;call	getgroffset
;	Return graphic mode video memory offset to line start
;
; In:	DX			(Y coordinate in pixels)
; Out:	DI			(video memory offset)
;	SI			(offset to next row)
; Use:	videoseg, scanline
; Modf:	AX, DX, ES
; Call:	@getoffsret
; Info:	4/5 (320x200x4)   byte offset = (y/2)*80 + (y%2)*2000h + (x*2)/8
;	  6 (640x200x2)   byte offset = (y/2)*80 + (y%2)*2000h + x/8
;	0Dh (320x200x16)  byte offset = y*40 + x/8, bit offset = 7 - (x % 8)
;	0Eh (640x200x16)  byte offset = y*80 + x/8, bit offset = 7 - (x % 8)
;	0Fh (640x350x4)   byte offset = y*80 + x/8, bit offset = 7 - (x % 8)
;	10h (640x350x16)  byte offset = y*80 + x/8, bit offset = 7 - (x % 8)
;	11h (640x480x2)   byte offset = y*80 + x/8, bit offset = 7 - (x % 8)
;	12h (640x480x16)  byte offset = y*80 + x/8, bit offset = 7 - (x % 8)
;	13h (320x200x256) byte offset = y*320 + x
;	HGC (720x348x2)   byte offset = (y%4)*2000h + (y/4)*90 + x/8
;						    bit offset = 7 - (x % 8)
;
;getgroffset	proc
		xor	di,di
		mov	es,di			; ES=0
		mov	ax,[scanline]
		mov	si,ax			; [nextrow]
		cmp	byte ptr videoseg[1],0A0h
		je	@getoffsret		; jump if not videomode 4-6
		mov	si,2000h
		sar	dx,1			; DX=Y/2
		jnc	@getoffsret
		mov	di,si			; DI=(Y%2)*2000h
		mov	si,-(2000h-80)
		;jmp short @getoffsret
@getoffsret:
		imul	dx			; AX=row*screen width
		add	ax,es:[44Eh]		; video page offset
		add	di,ax
;		ret
;getgroffset	endp

		pop	dx

; cx=force request, bx=X, di=line offset, si=nextrow, dx=spriteheight

		add	di,bx
		les	ax,dword ptr [cursor@]
		assume	es:nothing
if 0
		inc	ax
		jz	@@cpz
		dec	ax
@@cpz:
endif
;		inc	ax
;	jz @@cpz
		; cursor drawn
		OPCODE_CMP_DI
cursorpos	dw ?
	jnz @@cpnz2
		cmp	dx,[spriteheight]
		jz	@@drawret		; exit if position not changed
@@cpnz2:
		push	bx
		push	dx
		push	di
;		dec	ax
		xchg	di,ax			; OPTIMIZE: instead MOV DI,AX
		call	restoresprite
		pop	di
		pop	dx
;	jmp short @@cpnz
;@@cpz:
;		push	bx
;		call	updatevregs
;@@cpnz:
		pop	bx

; bx=X, di=line offset+bx, si=nextrow, dx=spriteheight, es=videoseg

		mov	[cursorpos],di
		mov	[spriteheight],dx
		mov	[nextrow],si
		sub	di,bx
		push	dx

;----- precompute sprite parameters

		push	bx
		;mov	cl,[bitmapshift]
		;mov	dx,[cursorwidth]
		OPCODE_MOV_CL
bitmapshift	db ?
		OPCODE_MOV_DX
cursorwidth	dw ?
		sar	bx,cl			; left sprite offset (signed)
		mov	ax,[scanline]
		add	dx,bx			; right sprite offset
		cmp	dx,ax
	jb @@pspb
		xchg	dx,ax			; DX=min(DX,scanline)
@@pspb:

		pop	ax			; =cursorX
		sub	cl,3			; mode 0Dh=1, other=0
		and	ax,[granumask.X]	; fix for mode 4/5
		sar	ax,cl			; sprite shift for non 13h modes
		neg	bx			; sprite shift for 13h mode
;	if_ lt					; if left sprite offset>0
	jge @@pspge
		add	dx,bx
		sub	di,bx
		mov	bl,0
		and	al,7			; shift in byte (X%8)
;	end_
@@pspge:

		inc	bx			; OPTIMIZE: BX instead BL
		sub	al,8			; if cursorX>0
		mov	bh,al			; ...then BH=-(8-X%8)
		push	bx			; ...else BH=-(8-X)=-(8+|X|)

;----- save screen sprite and draw cursor at new cursor position

		mov	[spritewidth],dx
		mov	[cursor@],di
		mov	al,0D6h			; screen source
		call	copysprite		; save new sprite

		OPCODE_MOV_BX
spritetop	dw ?
		pop	cx
		pop	ax			; CL/CH=sprite shift
						; AX=[spriteheight]
						; SI=[nextrow]
		add	bx,bx			; mask offset
@@spriteloop:
		push	ax
		push	cx
		push	bx
		push	si
		push	di
		mov	si,[spritewidth]
		mov	dx,screenmask[bx]
		mov	bx,cursormask[bx]
		call	makerow
		pop	di
		pop	si
		pop	bx
		pop	cx
		pop	ax
		add	di,si
		xor	si,[nextxor]		; for interlaced mode?
		inc	bx
		inc	bx
		dec	ax
		jnz	@@spriteloop

		;j	restorevregs
		;mov	bx,TSRdref:vdata1
		;call	writevregs
		;mov	ah,0F5h			; write register set
		;call	registerset

		; restore first byte of mov instruction to enable function
		mov	byte ptr [drawcursor],0C6h
		ret
graphcursor	endp

restoresprite	proc
		;call	updatevregs
		mov	al,0D7h			; screen destination
		;j	copysprite		; restore old sprite
restoresprite	endp

;		Copy screen sprite back and forth
;
; In:	AL			(0D6h/0D7h-screen source/dest.)
;	ES:DI			(pointer to video memory)
; Out:	CX = 0
;	BX = 0
; Use:	buffer@ (small in DS for CGA, larger for MCGA, in a000 for EGA...)
; Modf:	AX, DX
; Call:	none
;
copysprite	proc	C uses si di ds es	; TASM inserts PUSH here!
		assume	es:nothing
		cmp	al,0D6h
		pushf				; whether to swap pointers
		mov	NEXTOFFSCODE[1],al
		OPCODE_MOV_AX
nextrow		dw ?
		OPCODE_MOV_BX
spriteheight	dw ?

		push	ax
		mov	al,0BEh			; 1 for CGA / MCGA: MOV SI
		cmp	[bitmapshift],1	; 1 for MCGA mode 13h
		jz	@blitmode
		cmp	byte ptr videoseg[1],0A0h
		jne	@blitmode		; jump if CGA
		mov	al,0E8h			; 0 for EGA / VGA: CALL
@blitmode:	mov	BLITCODE[1],al
		cmp	al,0E8h
		jnz	@blitram		; if not EGA / VGA, skip GRC
		call	backup3ce		; backup 3ce/f regs
		push	dx
		mov	dx,3ceh
		mov	ax,word ptr SEQ5[1]	; mode reg, ax=nn05
		and	ah,0f4h			; read mode 0/1 plane/match...
		or	ah,1			; write mode 1: from latch :-)
		out	dx,ax
		mov	dx,3ceh
		mov	ax,0003h		; operator: COPY all planes :-)
		out	dx,ax
		pop	dx
@blitram:	pop	ax

		lds	si,[buffer@]
		assume	ds:nothing
		popf				; whether to swap pointers
	jnz @@blitnz
		push	ds
		push	es
		pop	ds
		pop	es			; DS:SI=screen
;		xchg	si,di			; ES:DI=buffer
		xchg	di,si	; JWASM and TASM use opposite encoding
@@blitnz:

;	countloop_ ,bx
@@spritewloop:
		OPCODE_MOV_CX
spritewidth	dw ?
		mov	dx,ax
		sub	dx,cx
		; instead of movsb inside a000:x, one could possibly do
		; out 3ce,4 read/stosb out 3ce,104 read/stosb out 3ce,204
		; read/stosb out 3ce,304 movsb -> copy planewise on EGA/VGA
		; ... but that would require a suitable target buffer!
		rep	movsb			; -X- now for CGA EGA VGA MCGA

NEXTOFFSCODE	db	01h,0d6h		; ADD SI,DX/ADD DI,DX
		OPCODE_XOR_AX
nextxor		dw ?
;	end_	; of outer countloop_
	dec bx
	jnz @@spritewloop

BLITCODE	db 90h				; for the label ;-)
		call restore3ce			; CALL or nop-ish MOV SI
		ret	; TASM inserts the right POP here!

copysprite	endp
		assume	ds:@TSRdata

;		Transform the cursor mask row to screen
;
; In:	DX = screenmask[row]	(which pixels will combine with screen color)
;	BX = cursormask[row]	(which pixels will be white/inverted, rather than black/transparent)
;	SI = [spritewidth]
;	CL			(sprite shift when mode 13h)
;	CH			(sprite shift when non 13h modes) (??)
;	ES:DI			(video memory pointer)
; Out:	none
; Use:	bitmapshift		(1 if mode 13h MCGA)
; Modf:	AX, CX, DX, BX, SI, DI
; Call:	none
;
makerow		proc
		assume	es:nothing
		cmp	[bitmapshift],1		; =1 for 13h mode
;	if_ eq
	jnz @@mrnz

;-----

;	 countloop_ ,si				; loop over x pixels
@@pixloop:
		shl	bx,cl			; if MSB=0
;		sbb	al,al			; ...then AL=0
	db 1ah, 0c0h	; JWASM and TASM use opposite encoding
		and	al,0Fh			; ...else AL=0Fh (WHITE color)
		shl	dx,cl
;	  if_ carry				; if most sign bit nonzero
	jnc @@mrnc
		xor	al,es:[di]
;	  end_
@@mrnc:
		stosb
		mov	cl,1
;	 end_ countloop
		dec si
		jnz @@pixloop
		ret
;	end_ if
@@mrnz:

;----- display cursor row in modes other than 13h

makerowno13:	call 	backup3ce

		mov	ax,0FFh
;	loop_					; shift masks left until ch++ is 0
@@m13zloop:
		add	dx,dx
		adc	al,al
		inc	dx			; al:dh:dl shifted screenmask
		add	bx,bx
		adc	ah,ah			; ah:bh:bl shifted cursormask
		inc	ch
;	until_ zero
	jnz @@m13zloop
;		xchg	dh,bl			; al:bl:dl - ah:bh:dh
		xchg	bl,dh	; JWASM and TASM use opposite encoding

;	countloop_ ,si
@@m13loop:
		push	dx
; ***		push	bx			; must be omitted, but why?
		mov	dx,es
		cmp	dh,0A0h
;	 if_ ne					; if not planar mode 0Dh-12h
	jz @@m13z
		and	al,es:[di]
		xor	al,ah
		stosb
;	 else_
	jmp short @@m13nz
@@m13z:
		xchg	cx,ax			; OPTIMIZE: instead MOV CX,AX
if 1	; OLD BUT WORKING
;		out_	3CEh,5,0		; set write mode 0: "color: reg2 mask: reg8"
		mov	dx,3ceh
		mov	ax,5
		out	dx,ax
;		out_	,3,8h			; data ANDed with latched data
		mov	ax,803h
		out	dx,ax
		xchg	es:[di],cl
;		out_	,3,18h			; data XORed with latched data
		mov	ax,1803h
		out	dx,ax
		xchg	es:[di],ch
else	; NEW BUT NO TRANSPARENCY
		mov	dx,3ceh			; graphics controller
		mov	ax,word ptr SEQ5[1]	; mode reg, ax=nn05
		and	ah,0f4h			; zap mode bits
		or	ah,2			; set write mode 2: "color: ram, mask: reg8"
		out	dx,ax
		mov	dx,3ceh			; graphics controller
		mov	ax, 803h		; 8 pan 0, mode AND (TODO: preserve pan)
		out	dx,ax			; set operation
		mov	al,8			; mask reg
		mov	ah,cl			; AND mask
		out	dx,ax			; set mask
		mov	byte ptr es:[di],12	; color
		mov	ax,1803h		; 18 pan 0, mode XOR (TODO: preserve pan)
		out	dx,ax			; set operation
		mov	al,8
		mov	ah,ch			; XOR mask
		out	dx,ax			; set mask
		mov	byte ptr es:[di],15	; color
endif
		inc	di
;	 end_
@@m13nz:
		xchg	ax,bx			; OPTIMIZE: instead MOV AX,BX
		pop	bx			; why was this push dx - pop bx?
; ***		pop	dx			; must be omitted, but why?
;	end_ countloop
	dec si
	jnz @@m13loop

restore3ce::	push	dx
		push	ax
		mov	dx,3ceh			; graphics controller
SEQ3		db	0b8h,3,0		; mov ax,0003
		out	dx,ax
SEQ4		db	0b8h,4,0		; mov ax,0004
		out	dx,ax
SEQ5		db	0b8h,5,0		; mov ax,0005
		out	dx,ax
SEQ8		db	0b8h,8,0		; mov ax,0008
		out	dx,ax
		pop	ax
		pop	dx
		ret

; backup 4 EGA+ graphics controller registers, can only read VGA, not from EGA

backup3ce::	push	dx
		push	ax
		mov	dx,3ceh		; graphics controller
		mov	al,3		; operator / pan, pan is 3 LSB
		out	dx,al
		inc	dx
		in	al,dx
		mov	SEQ3[2],al
		dec	dx
		mov	al,4		; plane: 2 LSM, only for read
		out	dx,al
		inc	dx
		in	al,dx
		mov	SEQ4[2],al
		dec	dx
		mov	al,5		; mode: read, write, memmode/depth
		out	dx,al
		inc	dx
		in	al,dx
		mov	SEQ5[2],al
		dec	dx
		mov	al,8		; bit mask (...)
		out	dx,al
		inc	dx
		in	al,dx
		mov	SEQ8[2],al
		pop	ax
		pop	dx
		ret

makerow		endp


		assume	es:nothing


;		INT 33 HANDLER SERVICES

; Use:	0:449h, 0:44Ah, 0:463h, 0:484h, 0:487h, 0:488h, 0:4A8h
; Modf:	RedefArea, screenheight, granumask, buffer@, videoseg, cursorwidth,
;	scanline, nextxor, bitmapshift, registerset, rangemax, ClearArea
; Call:	drawcursor, @savecutpos

setupvideo	proc

;----- set parameters for current video mode
; mode	 seg   screen  cell scan planar  VX/
;			    line	byte
;  0	B800h  640x200 16x8   -    -	  -
;  1	B800h  640x200 16x8   -    -	  -
;  2	B800h  640x200	8x8   -    -	  -
;  3	B800h  640x200	8x8   -    -	  -
;  4	B800h  320x200	2x1   80   no	  8
;  5	B800h  320x200	2x1   80   no	  8
;  6	B800h  640x200	1x1   80   no	  8
;  7	B000h  640x200	8x8   -    -	  -
; 0Dh	A000h  320x200	2x1   40  yes	 16
; 0Eh	A000h  640x200	1x1   80  yes	  8
; 0Fh	A000h  640x350	1x1   80  yes	  8
; 10h	A000h  640x350	1x1   80  yes	  8
; 11h	A000h  640x480	1x1   80  yes	  8
; 12h	A000h  640x480	1x1   80  yes	  8
; 6Ah	A000h  800x600	1x1  100  yes	  8
; 13h	A000h  320x200	2x1  320   no	  2
; other     0  640x200	1x1   -    -	  -
;
		mov	si,szClearArea2/2	; clear area 2

;----- setup video regs values for current video mode

		push	si
		xor	ax,ax
		mov	es,ax		; ES=0
		mov	al,es:[449h]	; current video mode

; mode 0-3
		mov	dx,0B8FFh		; B800h: [0-3]
		mov	cx,0304h		; 16x8: [0-1]
		mov	di,200			; x200: [4-6,0Dh-0Eh,13h]
		cmp	al,2
	jb @@settext
		dec	cx			; 8x8: [2-3,7]
		cmp	al,4
	jb @@settext
; mode 7
		cmp	al,7
		jne	@@checkgraph
		mov	dh,0B0h			; B000h: [7]
@@settext:
		; DH=seg_hi, DL=0FFh
		;
		mov	ch,1
		mov	bh,0F8h
		shl	dl,cl

		xor	ax,ax
		mov	es,ax			; ES=0
		add	al,es:[484h]		; screen height-1
						; Number of video rows (minus 1)
						; zero on old machines
		jz	@@stz
		inc	ax			; AL=screen height
		shl	ax,3
		xchg	di,ax			; OPTIMIZE: instead MOV DI,AX

@@stz:
		;mov	ax,es:[44Ah]		; textcolumns per row
		jmp short @@setcommon

; mode 4-6
@@checkgraph:	;mov	ah,0C3h			; RET opcode for [4-6,13h]
		;mov	cx,0303h		; sprite: 3 bytes/row
		;mov	dx,0B8FFh		; B800h: [4-6]/1x1: [6,0Eh-12h]
		;mov	di,200			; x200: [4-6,0Dh-0Eh,13h]
		mov	si,2000h xor -(2000h-80) ; [nextxor] for [4-6]
		mov	bx,TSRdref:spritebuf
		cmp	al,6
		je	@@setgraphics
		jb	@@set2x1

; in modes 0Dh-13h screen contents under cursor sprite will be
; saved at free space in video memory (A000h segment)

		mov	dh,0A0h			; A000h: [0Dh-13h]
		mov	bx,0A000h
		mov	es,bx
		xor	si,si			; [nextxor] for [0Dh-13h]
		cmp	al,6Ah			; SVGA
		je	@@mode6A
		cmp	al,13h
		ja	@@nonstandard
		je	@@mode13
; mode 8-0Dh
		cmp	al,0Dh
		jb	@@nonstandard
		;mov	ah,06h			; PUSH ES opcode for [0Dh-12h]
		mov	bx,3E82h		; 16002: [0Dh-0Eh]
		je	@@set320
; mode 0Eh-12h
		cmp	al,0Fh
		jb	@@setgraphics
		mov	di,350			; x350: [0Fh-10h]
		mov	bh,7Eh			; 32386: [0Fh-10h]
		cmp	al,11h
		jb	@@setgraphics
		mov	di,480			; x480: [11h-12h]
		mov	bh,9Eh			; 40578: [11h-12h]
		jmp short @@setgraphics
@@mode6A:
		mov	di,600			; x600: [6Ah]
		mov	bh,0EEh			; 61058: [6Ah]
		jmp short @@setgraphics
; mode 13h
@@mode13:	;mov	bl,0
		mov	bh,0FAh			; =320*200
		mov	cx,1000h		; sprite: 16 bytes/row

@@set320:	inc	cx			; OPTIMIZE: instead INC CL
@@set2x1:	dec	dx			; OPTIMIZE: instead MOV DL,-2

@@setgraphics:	nop				; -X-

		mov	word ptr [buffer@][0],bx
		mov	word ptr [buffer@][2],es

		mov	[nextxor],si
		;mov	byte ptr [registerset],ah
		jmp short @@setgcommon

@@nonstandard:	;mov	cl,3
		;mov	dl,0FFh
		;mov	di,200

;;+++++ for text modes: dh := 0B8h, j @@settext

		mov	dh,0			; no video segment

@@setgcommon:
		;mov	ax,640			; virtual screen width
		mov	bh,0FFh			; Y granularity
		;shr	ax,cl

@@setcommon:
		mov	[screenheight],di
		xor	ax,ax
		mov	es,ax
		mov	ax,es:[44Ah]		; textcolumns per row
		mov	[scanline],ax		; screen line width in bytes
		mov	[bitmapshift],cl	; log2(screen/memory ratio)
						; mode 13h=1, 0-1/0Dh=4, other=3
		mov	byte ptr [cursorwidth],ch ; cursor width in bytes
		mov	byte ptr [granumask.X],dl
		mov	byte ptr [granumask.Y],bh
		mov	byte ptr videoseg[1],dh
		shl	ax,cl
		pop	si

;----- set ranges and center cursor (AX=screenwidth, DI=screenheight)

		mov	cx,ax
		dec	ax
		mov	[rangemax.X],ax		; set right X range
		shr	cx,1			; X middle

		mov	dx,di
		dec	di
		mov	[rangemax.Y],di		; set lower Y range
		shr	dx,1			; Y middle

;----- set cursor position (CX=X, DX=Y, SI=area size to clear)

@setpos::	nop	;cli			; -X-
		push	ds
		pop	es

		mov	di,TSRdref:ClearArea
		xchg	si,cx
		xor	ax,ax
		rep	stosw

		;mov	word ptr [cursor@],-1
		xchg	ax,dx			; OPTIMIZE: instead MOV AX,DX
		mov	bx,offset POINT.Y
		call	@savecutpos
		xchg	ax,si			; OPTIMIZE: instead MOV AX,SI
		mov	bl,offset POINT.X
		jmp	@savecutpos
setupvideo	endp

; 00 - Reset driver and read status
;
; In:	none
; Out:	[AX] = 0/FFFFh		(not installed/installed)
;	[BX] = 2/3/FFFFh	(number of buttons)
;
resetdriver_00	proc
		mov	[_ARG_BX_],2
		mov	[callmask],ah		; AH=0, effectively disable UIR
		mov	ax,0FFFFh
		mov	[_ARG_AX_],ax		; FFFFh=success
		mov	[nocursorcnt],ax	; -1=hide cursor
		ret
resetdriver_00	endp

;------------------------------------------------------------------------------
; INT 33 0001 - Show Mouse Cursor
;	AX = 01
;	returns nothing
;	- increments the cursor flag;  the cursor is displayed if flag
;	  is zero;  default flag value is -1
;------------------------------------------------------------------------------
; 01 - (MS MOUSE v1.0+) - Show mouse cursor
;
; In:	none
; Out:	none
; Use:	none
; Modf:	AX, lowright.Y
; Call:	cursorstatus
;
showcursor_01	proc
		;neg	ax			; AL=AH=-1
		;mov	byte ptr lowright.Y[1],al ; place update region
		;jmp short cursorstatus		;  outside seen screen area
		mov	ax,[nocursorcnt]
		test	ax,ax
		js	@@showcurs
		ret
@@showcurs:
		inc	[nocursorcnt]
		ret
showcursor_01	endp

;------------------------------------------------------------------------------
; INT 33 0002 - Hide Mouse Cursor
;	AX = 02
;	returns nothing
;	- decrements cursor flag; hides cursor if flag is not zero
;------------------------------------------------------------------------------
;INT 33 0002 - HIDE MOUSE CURSOR
;	AX = 0002h
;Note:	multiple calls to hide the cursor will require multiple calls to
;	function 01h to unhide it.
;SeeAlso: AX=0001h,AX=0010h,INT 16/AX=FFFFh,INT 62/AX=007Bh
;SeeAlso: INT 6F/AH=08h"F_TRACK_OFF"
;------------------------------------------------------------------------------
; 02 - (MS MOUSE v1.0+) - Hide mouse cursor
;
; In:	none
; Out:	none
; Use:	none
; Modf:	AX
; Call:	cursorstatus
;
hidecursor_02	proc
		;dec	ax			; AL=1,AH=0
		;;j	cursorstatus
		dec	[nocursorcnt]
		ret
hidecursor_02	endp

if 0
; Hint:	request to cursor redraw (instead refresh) is useful in cases when
;	interrupt handlers try to hide, then show cursor while cursor
;	drawing is in progress
;
cursorstatus	proc
		add	al,[nocursorcnt]
		sub	ah,al			; exit if "already enabled"
		jz	@showret		;   or "counter overflow"
		mov	[nocursorcnt],al
		inc	ah			; jump if cursor changed
		jz	redrawcursor		;  between enabled/disabled
		ret
cursorstatus	endp
endif

;------------------------------------------------------------------------------
; INT 33 0003 - Get Mouse Position and Button Status
;	AX = 03
;
;	on return:
;	CX = horizontal (X) position  (0..639)
;	DX = vertical (Y) position  (0..199)
;	BX = button status:
;
;		|F-8|7|6|5|4|3|2|1|0|  Button Status
;		  |  | | | | | | | `---- left button (1 = pressed)
;		  |  | | | | | | `----- right button (1 = pressed)
;		  `------------------- unused
;
;	- values returned in CX, DX are the same regardless of video mode
;------------------------------------------------------------------------------
; INT 33 0003 - RETURN POSITION AND BUTTON STATUS
;	AX = 0003h
;Return: BX = button status (see #03168)
;	CX = column
;	DX = row
;Note:	in text modes, all coordinates are specified as multiples of the cell
;	size, typically 8x8 pixels
;SeeAlso: AX=0004h,AX=000Bh,INT 2F/AX=D000h"ZWmous"
;
;Bitfields for mouse button status:
;Bit(s)	Description	(Table 03168)
; 0	left button pressed if 1
; 1	right button pressed if 1
; 2	middle button pressed if 1 (Mouse Systems/Logitech/Genius)
;------------------------------------------------------------------------------
; 03 - (MS MOUSE v1.0+) - Get cursor position, buttons status and wheel counter
;
; In:	none
; Out:	[BL]			(buttons status)
;	[BH]			(wheel movement counter)
;	[CX]			(X - column)
;	[DX]			(Y - row)
; Use:	buttstatus, granpos
; Modf:	AX, CX, DX, wheel.counter
; Call:	@retBCDX
;
status_03	proc
		xor	ax,ax
		;xchg	ax,[wheel.counter]
		;mov	ah,al
		mov	al,[buttstatus]
		mov	cx,[granpos.X]
		mov	dx,[granpos.Y]
		;jmp	@retBCDX
@retBCDX::	mov	[_ARG_DX_],dx
@retBCX::	mov	[_ARG_CX_],cx
@retBX::	mov	[_ARG_BX_],ax
		ret
status_03	endp

;------------------------------------------------------------------------------
; INT 33 0004 - Set Mouse Cursor Position
;	AX = 4
;	CX = horizontal position
;	DX = vertical position
;	returns nothing
;
;	- default cursor position is at the screen center
;	- the position must be within the range of the current video mode
;	- the position may be rounded to fit screen mode resolution
;------------------------------------------------------------------------------
; INT 33 0004 - POSITION MOUSE CURSOR
;	AX = 0004h
;	CX = column
;	DX = row
;Note:	the row and column are truncated to the next lower multiple of the cell
;	size (typically 8x8 in text modes); however, some versions of the
;	Microsoft documentation incorrectly state that the coordinates are
;	rounded
;SeeAlso: AX=0003h,INT 62/AX=0081h,INT 6F/AH=10h"F_PUT_SPRITE"
;------------------------------------------------------------------------------
; 04 - (MS MOUSE v1.0+) - Position mouse cursor
;
; In:	CX			(X - column)
;	DX			(Y - row)
; Out:	none
; Use:	none
; Modf:	SI
; Call:	@setpos, refreshcursor
;
setpos_04	proc
		mov	si,szClearArea1/2	; clear area 1
		call	@setpos
		;j	refreshcursor
		;jmp	drawcursor
		ret
setpos_04	endp

if 0
; INT 33 0007 - Set Mouse Horizontal Min/Max Position
;	AX = 7
;	CX = minimum horizontal position
;	DX = maximum horizontal position
;	returns nothing
;
;	- restricts mouse horizontal movement to window
;	- if min value is greater than max value they are swapped
;
; 07 - (MS MOUSE v1.0+) - Set horizontal cursor range
;
; In:	CX			(min X)
;	DX			(max X)
; Out:	none
; Use:	none
; Modf:	BX
; Call:	@setnewrange
;
hrange_07	proc
		xor bx,bx	; MOVREG optimizes mov bx,offset POINT.X ...
		j	@setnewrange
hrange_07	endp

; INT 33 0008 - Set Mouse Vertical Min/Max Position
;	AX = 8
;	CX = minimum vertical position
;	DX = maximum vertical position
;	returns nothing
;
;	- restricts mouse vertical movement to window
;	- if min value is greater than max value they are swapped
;
; 08 - (MS MOUSE v1.0+) - Set vertical cursor range
;
; In:	CX			(min Y)
;	DX			(max Y)
; Out:	none
; Use:	pos
; Modf:	CX, DX, BX, rangemin, rangemax
; Call:	setpos_04
;
vrange_08	proc
		mov	bx,offset POINT.Y
@setnewrange::
		xchg	ax,cx		; OPTIMIZE: instead MOV AX,CX
		cmp	ax,dx
	jl @@snrl
		xchg	ax,dx
@@snrl:
		mov	word ptr rangemin[bx],ax
		mov	word ptr rangemax[bx],dx
		mov	cx,[pos.X]
		mov	dx,[pos.Y]
		;j	setpos_04
vrange_08	endp
endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 14 - (MS MOUSE v3.0+) - Exchange User Interrupt Routines
; 0C - (MS MOUSE v1.0+) - Define User Interrupt Routine
;
; In:	CX			(call mask)
;	ES:DX			(FAR routine)
; Out:	[CX]			(old call mask for function 14)
;	[ES:DX]			(old FAR routine for function 14)
; Use:	callmask, UIR@
; Modf:	AX
;
;	INT 33,14 - Swap Interrupt Subroutines
;	INT 33,C - Set User Defined Subroutine
;
;	AX = 14h or 0Ch
;	ES:DX = far pointer to user routine
;	CX = user interrupt mask:
;
;	|F-8|7|6|5|4|3|2|1|0| user interrupt mask in CX
;	  |  | | | | | | | `--- cursor position changed
;	  |  | | | | | | `---- left button pressed
;	  |  | | | | | `----- left button released
;	  |  | | | | `------ right button pressed
;	  |  | | | `------- right button released
;	  `--------------- unused
;
;	on return of function 14:
;	CX = previous user interrupt mask
;	ES:DX = far pointer to previous user interrupt
;	function 0C returns nothing
;
;	- routine at ES:DX is called if an event occurs and the
;	  corresponding bit specified in user mask is set
;	- routine at ES:DX receives parameters in the following
;	  registers:
;
;	  AX = condition mask causing call
;	  CX = horizontal cursor position
;	  DX = vertical cursor position
;	  DI = horizontal counts
;	  SI = vertical counts
;	  DS = mouse driver data segment
;	  BX = button state:
;
;	     |F-2|1|0|
;	       |  | `--- left button (1 = pressed)
;	       |  `---- right button (1 = pressed)
;	       `------ unused
;
;	- initial call mask and user routine should be restore on exit
;	  from user program
;	- user program may need to set DS to it's own segment

exchangeUIR_14	proc
		assume	es:nothing
		;mov	ah,0
		mov	al,[callmask]
		mov	[_ARG_CX_],ax
		mov	ax,word ptr UIR@[0]
		mov	[_ARG_DX_],ax
		mov	ax,word ptr UIR@[2]
		mov	[_ARG_ES_],ax
		;j	UIR_0C
exchangeUIR_14	endp

UIR_0C		proc
		assume	es:nothing
		mov	word ptr [UIR@][0],dx
		mov	word ptr [UIR@][2],es
		mov	[callmask],cl
		ret
UIR_0C		endp
;-----------------------------------------------------------------
;INT 33 000C - Define Interrupt Subroutine Parameters
;-----------------------------------------------------------------
;	AX = 000Ch
;	CX = call mask (see #03171)
;	ES:DX -> FAR routine (see #03172)
;SeeAlso: AX=0018h
;
;Bitfields for mouse call mask:
;Bit(s)	Description	(Table 03171)
; 0	call if mouse moves
; 1	call if left button pressed
; 2	call if left button released
; 3	call if right button pressed
; 4	call if right button released
; 5	call if middle button pressed (Mouse Systems/Logitech/Genius mouse)
; 6	call if middle button released (Mouse Systems/Logitech/Genius mouse)
; 7-15	unused
;Note:	some versions of the Microsoft documentation incorrectly state that CX
;	bit 0 means call if mouse cursor moves
;
;(Table 03172)
;Values interrupt routine is called with:
;	AX = condition mask (same bit assignments as call mask)
;	BX = button state
;	CX = cursor column
;	DX = cursor row
;	SI = horizontal mickey count
;	DI = vertical mickey count
;Note1:	some versions of the Microsoft documentation erroneously swap the
;	meanings of SI and DI
;Note2:	in text modes, the row and column will be reported as a multiple of
;	the character cell size, typically 8x8 pixels
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

if 0
debugprint	proc
		; print AL+20h
		pusha
		mov	dl,al
		;add	dl,20h
		mov	ah,2
		int	21h		; write character in DL to stdout
		popa
		ret
debugprint	endp
endif

;disablefunc	proc
;		mov	byte ptr [callmask],0
;disablefunc	endp

failfunc	proc
		mov	[_ARG_AX_],0FFFFh	; failure
		mov	[_ARG_BX_],0
		mov	[_ARG_CX_],0
failfunc	endp

nullfunc	proc
		ret
nullfunc	endp

;		INT 33 handler

		even
handler33table	dw TSRcref:resetdriver_00	; called 1st when EDIT loaded
	dw TSRcref:showcursor_01	; called frequently by EDIT
	dw TSRcref:hidecursor_02	; called frequently by EDIT
	dw TSRcref:status_03		; called continuously by QEDIT
	dw TSRcref:setpos_04		; called 3rd when EDIT loaded
	dw TSRcref:nullfunc		;pressdata_05
	dw TSRcref:nullfunc		;releasedata_06
	dw TSRcref:nullfunc		;hrange_07 called by QEDIT and GHOST
	dw TSRcref:nullfunc		;vrange_08 called by QEDIT and GHOST
	dw TSRcref:nullfunc		;graphcursor_09
	dw TSRcref:nullfunc		;textcursor_0A
	dw TSRcref:nullfunc		;mickeys_0B
	dw TSRcref:UIR_0C		; called 2nd when EDIT loaded
	dw TSRcref:nullfunc		;lightpenon_0D
	dw TSRcref:nullfunc		;lightpenoff_0E
	dw TSRcref:nullfunc		;sensitivity_0F
	dw TSRcref:nullfunc		;updateregion_10
	dw TSRcref:nullfunc		;wheelAPI_11
	dw TSRcref:nullfunc		;12 - large graphics cursor
	dw TSRcref:nullfunc		;doublespeed_13
	dw TSRcref:exchangeUIR_14	; called by QEDIT
	dw TSRcref:nullfunc		;storagereq_15
	dw TSRcref:nullfunc		;savestate_16
	dw TSRcref:nullfunc		;restorestate_17
	dw TSRcref:failfunc		;althandler_18
	dw TSRcref:failfunc		;althandler_19
	dw TSRcref:nullfunc		;sensitivity_1A
	dw TSRcref:nullfunc		;sensitivity_1B
	dw TSRcref:nullfunc		;1C - InPort mouse only
	dw TSRcref:failfunc		;1D - define display page #
	dw TSRcref:failfunc		;videopage_1E(not useful?)
	dw TSRcref:failfunc		;disabledrv_1F(failure)
	dw TSRcref:nullfunc		;enabledriver_20(success)
	dw TSRcref:resetdriver_00	;softreset_21 last call on exit EDIT
;
; GHOST will call function 00 on exit. EDIT and QEDIT will call 21 on exit.
; On startup, GHOST will call 00, 07, 08, 0C, 04. No further calls until exit
; and calling 21. GHOST uses its own graphic cursor routine to draw and move
; cursor.
;

handler33	proc
		assume	ds:nothing,es:nothing
		test	ah,ah
		jnz	@@h33nz
		;call	debugprint
		cmp	al,21h
		ja	@@h33nz
		push	ds

		;push	cs
		;pop	ds

		;cmp	al,21h
		;ja	language_23
		push	es
		pusha
		mov	si,cs
		mov	ds,si
		assume	ds:@TSRdata
		mov	si,ax			;!!! AX must be unchanged
		mov	bp,sp
		add	si,si
		cld
		call	handler33table[si]	; call by calculated offset
		popa
		pop	es
		pop	ds
		;iret
@@h33nz:
		;mov	byte ptr cs:[callmask],0
		iret
		assume	ds:@TSRdata

scanline	dw ?
screenheight	dw ?

handler33	endp

;		END OF INT 33 SERVICES

TSRend		label byte

E_error		db 5,'Error: Invalid option. Use /U to unload driver.',eos

E_notfound	db 5,'Error: PS/2 mouse not found!',eos

E_already	db 1,'Error: A mouse driver already present!',eos
E_nocute	db 1,'Error: Driver was not installed!',eos
E_notunload	db 2,'Error: Driver unload failed!',eos
S_unloaded	db 0,'PS/2 mouse driver unloaded.',eos

S_installed	db 'PS/2 mouse driver installed.',eos

real_start:
		cld
		xor	ax,ax
		mov	es,ax			; ES=0
		mov	bx,es:[0CCh]		; int33
		mov	ax,es:[0CEh]
		;mov	es,ax

		mov	word ptr [oldint33][0],bx
		mov	word ptr [oldint33][2],ax

		mov	bx,es:[40h]		; int10
		mov	ax,es:[42h]

		mov	word ptr [oldint10][0],bx
		mov	word ptr [oldint10][2],ax

		cmp	word ptr ds:[0], 0	; running under grub4dos?
		jne	@@dos1
		;------------- grub4dos begin ------------
		; change IDstring to upper case
		mov	dword ptr [IDstring], 42555247h
		mov	dword ptr [IDstring][4], 534F4434h
		;------------- grub4dos end --------------
@@dos1:

		; parse command line

		mov	si,80h			; offset PSP:cmdline_len
		lodsb
		cbw				; OPTIMIZE: instead MOV AH,0
		mov	bx,ax
		mov	[si+bx],ah		; OPTIMIZE: AH instead 0

		;--------------------------------------
@@clloop:
		lodsb
		test	al,al
		jz	@@clreturn		; end of command line
		cmp	al,' '
		jbe @@clloop
		;--------------------------------------

		cmp	al,'/'			; option character?
		lodsb
		mov	si, offset @data:E_error	; 'Invalid option'
		jne	@@exitmsg
		or	al,20h			; lowercase
		cmp	al,'u'
		jne	@@exitmsg

		; unload TSR

		; first, check if installed

		mov	es,word ptr oldint33[2]	; ES=oldint33 segment
		mov	si,TSRcref:IDstring
		mov	di,si
		mov	cx,szIDstring
		cld
		repe	cmpsb

		mov	si,dataref:E_nocute	; 'driver not installed'
		jne	@@exitmsg

		; ===== disabledrv_1F =====

		;	Disable PS/2
		; Modf:	AX, BX, DX, ES
		xor	bx,bx			; BH=0 to disable mouse
		PS2serv	0C200h			; enable/disable mouse
		mov	si,dataref:E_notunload	; 'Driver unload failed!'
		jc	@@exitmsg
		test	ah,ah
		jnz	@@exitmsg

		xor	bx,bx			; BX=0
		mov	es,bx			; ES=0
		PS2serv	0C207h			; clear mouse handler
		mov	si,dataref:E_notunload	; 'Driver unload failed!'
		jc	@@exitmsg
		test	ah,ah
		jnz	@@exitmsg

		;mov	bh,5			; 100 reports/second
		;PS2serv	0C202h		; set sample rate

		; get oldint33 segment
		push	cs
		pop	ds
		mov	cx,word ptr oldint33[2]

		; get INT 33 segment in AX
		xor	ax,ax
		mov	es,ax			; ES=0
		mov	ax,es:[0CEh]

		cmp	ax,cx			; is our int33?
		jne	@@exitmsg		; no

		; get INT 10 segment in AX
		mov	ax,es:[42h]

		cmp	ax,cx			; is our int10?
		jne	@@exitmsg		; no

		cmp	word ptr ds:[0], 0	; running under grub4dos?
		jne	@@dos2
		;------------- grub4dos begin ------------
		mov	ax,es:[413h]		; low mem size in K
		shl	ax,6			; mem end segment
		cmp	ax,cx			; is it last installed?
		jne	@@exitmsg		; no

		; Free memory
		add	word ptr es:[413h],((TSRend-TSRstart+1023)/1024)
	;	push	ds
	;	mov	ds,cx
	;	mov	ax,word ptr [oldint10][2]
	;	mov	bx,word ptr [oldint33][2]
	;	pop	ds
	;	cmp	ax,bx			; segment equal?
	;	jne	@@exitmsg		; no
	;	test	ax,63			; segment is KB aligned?
	;	jne	@@exitmsg		; no
		;------------- grub4dos end --------------
@@dos2:
		; restore old INT 10 and INT 33 handler
		push	ds
		mov	ds,cx
		mov	eax,[oldint10]
		mov	es:[40h],eax
		mov	eax,[oldint33]
		mov	es:[0CCh],eax
		pop	ds
		
		cmp	word ptr ds:[0], 0	; running under grub4dos?
		je	@@dos3end
		;jne	@@dos3
		;------------- grub4dos begin ------------
		;add	word ptr es:[413h],((TSRend-TSRstart+1023)>>10)
		;jmp	@@dos3end
		;------------- grub4dos end --------------
;@@dos3:
		;------------- dos begin ------------
		;call	FreeMem
		mov	es,word ptr oldint33[2]	; ES=oldint33 segment
		mov	ah,49h			; free memory
		int	21h
		;------------- dos end --------------
@@dos3end:
		mov	si,dataref:S_unloaded	; 'Driver unloaded.'

@@exitmsg:
		lodsb				; AL=return code
		cmp	word ptr ds:[0], 0	; running under grub4dos?
		jne	@@dos4
		;------------- grub4dos begin ------------
		; move to top of available memory
		call	sayASCIIZ
		;jmp	0000:8200
		db	0EAh		; JMP FAR dword
		dd	00008200h
		;------------- grub4dos end --------------
@@dos4:
		;------------- dos begin ------------
		mov	ah,4Ch			; terminate
		push	ax

		call	sayASCIIZ

		pop	ax
		int	21h
		;------------- dos end --------------

@@clreturn:

		; exit immediately if a driver is already installed

		xor	ax,ax	; reset old driver and get mouse install flag
		mov	cx,word ptr oldint33[2]
		jcxz	@@mdcxz
		pushf				;!!! Logitech MouseWare
		call	[oldint33]		;  Windows driver workaround
@@mdcxz:
		mov	si,dataref:E_already
		inc	ax			; AX=FFFFh means installed
		jz	@@exitmsg

		; find PS/2 mouse

		call	checkPS2
		push	cs
		pop	ds
		mov	si,dataref:E_notfound	; 'Error: mouse not found'
		jc	@@exitmsg

		cmp	word ptr ds:[0], 0	; running under grub4dos?
		je	@@grub4dos1

		;------------- dos begin ------------
		; release environment block

		mov	es,ds:[2Ch]		; DS=PSP
		mov	ah,49h			; free memory
		int	21h
		;------------- dos end --------------

@@grub4dos1:

if 1
		; set INT 33 and INT 10 handler
		mov	ax,cs
		shl	eax,16
		xor	dx,dx
		mov	es,dx			; ES=0
		mov	ax,@TSRcode:handler33
		mov	es:[0CCh],eax
		mov	ax,@TSRcode:int10handler
		mov	es:[40h],eax

		call	setupvideo

		push	cs
		pop	ds
		cmp	word ptr ds:[0], 0	; running under grub4dos?
		jne	@@dos5
		;------------- grub4dos begin ------------
		; move to top of available memory
		; get low mem size
		xor	ax,ax
		mov	es,ax			; ES=0
		mov	ax,es:[413h]		; low mem size in K
		shl	ax,6			; segment
		mov	dx,(TSRend-TSRstart)	; size of TSR
		mov	cx,dx
		add	dx,1023
		shr	dx,10			; code size in K
		shl	dx,6
		sub	ax,dx			; AX=new segment
		mov	dx,ax
		shr	dx,6			; DX=new low mem size in K
		mov	es:[413h],dx		; set it up
		mov	es,ax
		mov	si,cx
		dec	si
		mov	di,si
		std
		rep movsb
		cld
		push	ax
		shl	eax,16
		xor	dx,dx
		mov	es,dx			; ES=0
		mov	ax,@TSRcode:handler33
		mov	es:[0CCh],eax
		mov	ax,@TSRcode:int10handler
		mov	es:[40h],eax
		pop	es
		jmp	@@dos5end
		;------------- grub4dos end --------------
@@dos5:
		;------------- dos begin ------------
		; set INT 33 and INT 10 handler
		; mov	ax,cs
		push	cs
		pop	es
		;------------- dos end --------------
@@dos5end:
		;	Enable PS/2
		; Modf:	AX, BX, DX, ES
		; push	cs
		; pop	es
		mov	bx,TSRcref:PS2handler	; -X- real handler now
		PS2serv	0C207h			; set mouse handler in ES:BX
		mov	bh,1
		PS2serv	0C200h			; set mouse on (bh=1)
		mov	bh,1			; 20 reports/second
		PS2serv	0C202h			; set sample rate
endif

		push	cs
		pop	ds
		;push	ds
		;pop	es

		mov	si,dataref:S_installed	; 'driver installed'
		call    sayASCIIZ

		cmp	word ptr ds:[0], 0	; running under grub4dos?
		jne	@@dos6
		db	0EAh		; JMP FAR dword
		dd	00008200h
		;------------- grub4dos end --------------
@@dos6:
		;------------- dos begin ------------
		mov	ax,3100h			; TSR exit
		mov	dx,(TSRend-TSRstart+15)/16	; size of TSR
		int	21h
		;------------- dos end --------------

;				Check for PS/2
;
; In:	none
; Out:	Carry flag		(no PS/2 device found)
; Use:	none
; Modf:	AX, CX, BX, ES
; Call:	INT 11, INT 15/C2xx
; After reset with C201: "disabled, 100 Hz, 4 c/mm, 1:1", protocol unchanged
;
checkPS2	proc

		push	ds
		xor	ax,ax
		mov	ds,ax
		mov	ax,ds:[410h]		; get equipment list
		pop	ds
		test	al,4			; mouse installed?
		jz	@@noPS2			; jump if PS/2 not indicated
		mov	bh,3			; standard 3 byte packets
		PS2serv 0C205h,@@noPS2		; initialize mouse, bh=datasize
		mov	bh,3			; resolution: 2^bh counts/mm
		PS2serv 0C203h,@@noPS2		; set mouse resolution bh
		push	cs
		pop	es
		mov	bx,coderef:@@dummyPS2
		PS2serv	0C207h,@@noPS2		; can we set a handler?
		xor	bx,bx
		mov	es,bx
		PS2serv	0C207h			; clear mouse handler (ES:BX=0)

		; no handler yet, enabledriver_20 does int 15/C207, 15/C200
		clc
		ret

@@noPS2:	stc
		ret
checkPS2	endp

; In:	DS:SI			(null terminated string)
; Out:	none
; Use:	none
; Modf:	AX, BX, SI
; Call:	none
;
sayASCIIZ_	proc

@@sazloop:
		mov	bl, 07h		; graphics mode foreground color=white
		mov	ah,0Eh		; print char in AL
		int	10h		; via TTY mode
sayASCIIZ::
		lodsb			; get char
		cmp	al,0		; end of string?
	jnz @@sazloop
		ret

sayASCIIZ_	endp

		dd	0C010512h	; real
		dd	85848F8Dh	; mode
end start
