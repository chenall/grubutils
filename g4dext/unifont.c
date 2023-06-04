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
 * compile:

gcc -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE unifont.c

 * disassemble:			objdump -d a.out 
 * confirm no relocation:	readelf -r a.out
 * generate executable:		objcopy -O binary a.out unifont
 *
 */
/*
 * to get this source code & binary: http://grub4dos-chenall.google.com
 * For more information.Please visit web-site at http://chenall.net
 * 2010-03-07
 * last modify 2011-02-10
 */
#include "font/font.h"
#include "grub4dos.h"

//#include "fontfile.h"
#define A_NORMAL        0x7
#define A_REVERSE       0x70
#define VIDEOMEM 0xA0000
/* 程序运行时使用的内存 4M-6M之间
 * 其中前面4KB(0x400000-0x401000)作为全局变量存放位置
 * 0x401000 开始存放字库+程序自身
 */
#define VARIABLE_ADDR			0x400000	//变量存放起始地址
#define BASE_FONT_ADDR			0x401000	//字库文件存放地址
#define UNIFONT_LOAD_FLAG		0xBDC6AECB	//程序在内存中已经加载的标志

#define ushFontReaded			(*(WORD *)(VARIABLE_ADDR + 0x10))	//字体成功加载标志
#define g_dwCharInfo			(*(DWORD *)(VARIABLE_ADDR + 0x14))	// 存储当前字符的检索信息。  bit0～bit25：存放点阵信息的起始地址。 bit26～bit31：存放像素宽度。
#define FONT_WIDTH				(*(DWORD *)(VARIABLE_ADDR + 0x18))
#define ALL_FONT				(*(DWORD *)(VARIABLE_ADDR + 0x1C))
/*原始数据备份*/
#define graphics_cursor_bak		(*(DWORD *)(VARIABLE_ADDR + 0x100))
#define term_backup_orig_addr	((struct term_entry **)(VARIABLE_ADDR + 0x104))
#define term_backup				((struct term_entry *)(VARIABLE_ADDR + 0x108))

#define _fl_header				((PFL_HEADER)(VARIABLE_ADDR + 0x400))


#define VSHADOW VSHADOW1
#define VSHADOW_SIZE 60000
#define VSHADOW1 ((unsigned char *)0x3A0000)
#define VSHADOW2 (VSHADOW1 + VSHADOW_SIZE)
#define VSHADOW4 (VSHADOW2 + VSHADOW_SIZE)
#define VSHADOW8 (VSHADOW4 + VSHADOW_SIZE)
//#define text ((unsigned long *)0x3C5800)
#define TEXT ((unsigned long (*)[x1])0x3FC000)

static int fontfile_func (char *arg ,int flags);
static void graphics_cursor (int set);
static void graphics_setxy (int col, int row);
static void graphics_scroll (void);
static int unifont_unload(void);
void graphics_putchar (unsigned int ch);


//static unsigned char mask[16];
//static int graphics_old_cursor;
static int no_scroll = 0;

/* color state */
//static int graphics_standard_color = A_NORMAL;
//static int graphics_normal_color = A_NORMAL;
static int graphics_highlight_color = A_REVERSE;
//static int graphics_helptext_color = A_NORMAL;
//static int graphics_heading_color = A_NORMAL;
//static int graphics_current_color = A_NORMAL;

//int outline = 0;

//int disable_space_highlight = 0;
#define x0 0
#define x1 current_term->chars_per_line
#define y0 0
#define y1 current_term->max_lines

//static char *g_prf = 0x400000;


//int GRUB = 0x42555247;/* this is needed, see the following comment. */
/* gcc treat the following as data only if a global initialization like the
 * above line occurs.
 */
//asm(".long 0x534F4434");

/* a valid executable file for grub4dos must end with these 8 bytes */
//asm(ASM_BUILD_DATE);
//asm(".long 0x03051805");
//asm(".long 0xBCBAA7BA");

#include "grubprog.h"
/* thank goodness gcc will place the above 8 bytes at the end of the b.out
 * file. Do not insert any other asm lines here.
 */

int
main (char *arg,int flags)
{
	while(*arg == ' ' || *arg == '\t') arg++;
	if (memcmp(arg,"--unload",8) == 0)
	{
		unifont_unload();
		return 1;
	}
	if (memcmp(arg,"--all-font",10) == 0)
	{
		ALL_FONT = 1;
		arg = skip_to (0,arg);
	} else ALL_FONT = 0;
	if (strlen(arg) > 256)
	return !(errnum = ERR_WONT_FIT);

	if (open(arg))
	{
		if (filemax > 0x1FD000)
			return 0;
		if ((*(DWORD *)VARIABLE_ADDR) == UNIFONT_LOAD_FLAG)
		{
			unifont_unload();
		}
		read ((unsigned long long)(unsigned int)(char *) RAW_ADDR (BASE_FONT_ADDR), -1, 0xedde0d90);
		close();
		if (InitFont())
		{
//			memmove((char *)BASE_FONT_ADDR + filemax ,&main,(int)&GRUB - (int)&main );
			ushFontReaded = 1;
			FONT_WIDTH = 10;
			return fontfile_func(arg,flags);
		}
		return 0;

	}
	errnum = 0;
	printf ("UNIFONT %s\nUse:\nUNIFONT UNIFONT_FILE\tLoad unicode font file.\nUNIFONT --unload\t\tUnload.",__DATE__);
	return 1;
}

static inline void outb(unsigned short port, unsigned char val)
{
    __asm __volatile ("outb %0,%1"::"a" (val), "d" (port));
}

static void MapMask(int value) {
    outb(0x3c4, 2);
    outb(0x3c5, value);
}

/* Chinese Support by Gandalf
 *    These codes used for defining a proper fontfile
 */
static void
graphics_cursor (int set)
{

    unsigned char *pat, *mem, *ptr;
    unsigned i, ch, offset, n, ch1, ch2;
    int invert = 0;
	static int dbcs_ending_byte;
	static unsigned char by[16][2], chsa[16], chsb[16];
	static unsigned char chr[16 << 2];
    if (set && no_scroll)
        return;

    ch = TEXT[fonty][fontx];

	invert = (ch & 0xffff0000) != 0;

	dbcs_ending_byte = 0;
	offset = cursorY * x1 + fontx;
	pat = font8x16;
	ch1 = ch & 0xff;
	pat += (ch1 << 4);

	if (ushFontReaded && ((ch & 0xff00) || (ALL_FONT && ch1 > 0x20 /*&& ch1 != 218 && ch1 != 191 && ch1 != 192 && ch1 != 217 && ch1 != 196 && ch1 != 179*/)))
	{
		WORD bytesPerLine = 0;
		int dotpos;
		ReadCharDistX(ch &= 0xffff);
//		(*(DWORD *)(VARIABLE_ADDR + 0x20)) = GET_FONT_WIDTH(g_dwCharInfo);
		dotpos = ReadCharDotArray(ch, &bytesPerLine);
		if (dotpos)
		{
			dbcs_ending_byte = GET_FONT_WIDTH(g_dwCharInfo) > 8;
			pat = chsa;
			
			if(dbcs_ending_byte)
			{
				memmove(by, (unsigned char *) RAW_ADDR (BASE_FONT_ADDR + dotpos), 32);
				for (i = 0;i < 16; i++)
				{
					chsa[i] = by[i][0];
					chsb[i] = by[i][1];
				}
			}
			else
			{
				memmove(chsa, (unsigned char *) RAW_ADDR (BASE_FONT_ADDR + dotpos), 16);
			}
			dbcs_ending_byte = GET_FONT_WIDTH(g_dwCharInfo) > FONT_WIDTH;
		}
	}

	mem = (unsigned char*)VIDEOMEM + offset;

    if (set)
    {
        MapMask(15);
        ptr = mem;
        for (i = 0; i < 16; i++, ptr += x1)
       	{
//            cursorBuf[i] = pat[i];
            *ptr = ~pat[i];
        }
        return;
    }

write_char:

	for (i = 0; i < 16; i++, offset += x1)
	{
		unsigned char m, p, c1, c2, c4, c8;

		p = pat[i];

		if (invert)
		{
			p = ~p;
			chr[i     ] = p;
			chr[16 + i] = p;
			chr[32 + i] = p;
			chr[48 + i] = p;
			continue;
		}

		chr[i     ] = ((unsigned char*)VSHADOW1)[offset] | p;
		chr[i + 16] = ((unsigned char*)VSHADOW2)[offset] | p;
		chr[i + 32] = ((unsigned char*)VSHADOW4)[offset] | p;
		chr[i + 48] = ((unsigned char*)VSHADOW8)[offset] | p;
	}

    offset = 0;
    for (i = 1; i < 16; i <<= 1, offset += 16)
    {
        int j;

        MapMask(i);
        ptr = mem;
        for (j = 0; j < 16; j++, ptr += x1)
            *ptr = chr[j + offset];
    }

    MapMask(15);
    
	if (dbcs_ending_byte)
	{
		dbcs_ending_byte = 0;

		/* reset the mem position */
		mem++;//mem += 1;//mem = (unsigned char*)VIDEOMEM + offset;
		offset = cursorY * x1 + fontx + 1;
		pat = chsb;
		graphics_setxy(fontx + 1, fonty);
		goto write_char;
	}
}

static void
graphics_setxy (int col, int row)
{
    if (col >= x0 && col < x1)
    {
        fontx = col;
        cursorX = col << 3;
    }

    if (row >= y0 && row < y1)
    {
        fonty = row;
        cursorY = row << 4;
    }
}

static void
graphics_scroll (void)
{
	int i, j;

	/* we don't want to scroll recursively... that would be bad */
	if (no_scroll)
		return;
	no_scroll = 1;

	/* move everything up a line */
	for (j = y0 + 1; j < y1; j++)
	{
		gotoxy (x0, j - 1);

		for (i = x0; i < x1; i++)
		{
			graphics_putchar (TEXT[j][i]);
			if ((TEXT[j][i]) & 0xff00)
			{
				if ((TEXT[j][i+1]) == 0) i++;
			}
		}
	}

	/* last line should be blank */
	gotoxy (x0, y1 - 1);

	for (i = x0; i < x1; i++)
		graphics_putchar (' ');

	graphics_setxy (x0, y1 - 1);

	no_scroll = 0;
}

/*
	转换utf8字符串为unicode.
	ch指向utf8的第一个字符,n是长度.
	注: 本函数是配合scan_utf8函数使用的.
*/
int utf8_unicode(unsigned long *ch,int n)
{
	unsigned long *c1 = ch + 1;
	if (*ch == 196 && *ch == *(ch-1) && *c1 == 191)
	{
		return 0;
	}
	int c = *ch;
	int i;
	/*检测该utf8字符串长度是否为n*/
	for (i = 0; c & 0x80;++i)
	{
		c <<= 1;
	}

	if (i != n)
		return 0;

	c1 = ch + 1;
	c &= 0xff;
	c >>= n;
	for (i = n;i>1;--i)
	{
		c <<= 6;
		c |= *c1 & 0x3f;
		*c1++ = 0;
	}
	*ch = c | (*ch & 0xffff0000);
	return 1;
}

void scan_utf8(void)
{
	int i,ch;
	unsigned long *p_text = &TEXT[fonty][fontx];
	for (i=1; i<=6 ;--p_text)
	{
		ch = *p_text & 0xFFC0;
		/*
			根据UTF8的规则
			1.每个字符的最高位必须为1即0x80,
			2.首个字符前面至少两个1,即像110xxxxx;
			3.后面的字符必须是二进制10xxxxxx;
			
			因为这里是从字符的后面往前面判断的
			所以只要判断该字符的前两位为1即0xC0,那该UTF8的字符串就从这个地方开始.
		*/
		if (ch == 0xC0)
		{
			/*用utf8_unicode函数进行转换,长度为i,如果长度不符合则返回0*/
			if (utf8_unicode(p_text,i))
			{
				--i;
				if (fontx < i)
				{
					if (fonty == 0)
						return;
					i -= fontx;
					--fonty;
					fontx = x1 - 1;
					if (i == 1)
					{
						unsigned long utf8_tmp = *p_text;
						graphics_setxy(fontx,fonty);
						putchar(' ');
						TEXT[fonty][fontx] = utf8_tmp;
						return;
					}
				}
				fontx -= i;
				graphics_setxy(fontx,fonty);
			}
			return;
		}
		else if (ch != 0x80)
		{
			return;
		}
		++i;
	}
}

void
graphics_putchar (unsigned int ch)
{
    if (ch == '\n') {
        if (fonty + 1 < y1)
            current_term->GOTOXY(fontx, fonty + 1);
        else
	{
	    graphics_cursor(0);
            graphics_scroll();
	    graphics_cursor(1);
	}
        //graphics_cursor(1);
        return;
    } else if (ch == '\r') {
        current_term->GOTOXY(x0, fonty);
        //graphics_cursor(1);
        return;
    }
    else if (ch < 0)//如果ch小于(字符大于127如果没有使用unsigned char那传过来的参数就是负数了)
    {
		ch &= 0xff;
	}
	 else if (ch & 0xff00)
    {
		if (fontx+1>=x1) graphics_putchar(' ');
		TEXT[fonty][fontx+1] = 0;
    }
    TEXT[fonty][fontx] = ch;
    if (current_color == graphics_highlight_color)//if (graphics_current_color & 0xf0)
        TEXT[fonty][fontx+1] |= 0x10000;//0x100;

	if ((ch & 0xFF80) == 0x80)
		scan_utf8();

    graphics_cursor(0);

    if (fontx + 1 >= x1)
    {
        if (fonty + 1 < y1)
            graphics_setxy(x0, fonty + 1);
        else
	{
            graphics_setxy(x0, fonty);
            graphics_scroll();
	}
    } else {
		graphics_setxy(fontx + 1, fonty);
    }

    graphics_cursor(1);
}

static int fontfile_func (char *arg, int flags)
{
	int i;
	/* get rid of TERM_NEED_INIT from the graphics terminal. */
	for (i = 0; term_table[i].name; i++) {
		if (strcmp (term_table[i].name, "graphics") == 0) {
			term_table[i].flags &= ~(1 << 16);
			break;
		}
	}
	//graphics_set_splash (splashimage);
	if ((*(DWORD *)VARIABLE_ADDR) != UNIFONT_LOAD_FLAG)
	{
		*term_backup_orig_addr = &term_table[i];
		graphics_cursor_bak = graphics_CURSOR;
		memmove(term_backup, &term_table[i], sizeof(struct term_entry));
	}
	term_table[i].PUTCHAR = (void *)BASE_FONT_ADDR + filemax + (int)&graphics_putchar - (int)&main;
	graphics_CURSOR = BASE_FONT_ADDR + filemax + (int)&graphics_cursor - (int)&main;
	if (graphics_inited) {
		term_table[i].SHUTDOWN ();
		if (! term_table[i].STARTUP())
		{
			graphics_CURSOR = graphics_cursor_bak;
			return !(errnum = ERR_EXEC_FORMAT);
		}
		cls();
	}
	(*(DWORD *)VARIABLE_ADDR) = UNIFONT_LOAD_FLAG;
	/* FIXME: should we be explicitly switching the terminal as a 
	 * side effect here? */
	builtin_cmd("terminal", "graphics", flags);

	return 1;
}

/***************************************************************
功能： 初始化字体。 即打开字体文件，且读取信息头。
参数： pFontFile--字库文件名      
***************************************************************/
int InitFont()
{
	memset(_fl_header , 0, sizeof(FL_HEADER));
	memmove(_fl_header,(char *)BASE_FONT_ADDR,sizeof(FL_HEADER));
	//检测表头
	if(_fl_header->YSize != 16 || _fl_header->magic[0] != 'U' || _fl_header->magic[1] != 'F' || _fl_header->magic[2] != 'L')
	{
		printf("Cann't support file format!\n");
		return 0;
	}

	_fl_header->pSection = (FL_SECTION_INF *)(BASE_FONT_ADDR + 0x10);

	return 1;	
//		return ReadFontHeader(_fl_header);	
}

/********************************************************************
功能： 获取当前字符的像素宽度, 且将索引信息存入一个全局变量：g_dwCharInfo。
        根据索引信息，即同时能获取当前字符的点阵信息的起始地址。
参数： wCode -- 当字库为unicode编码格式时，则将wCode当unicode编码处理。
                否则反之（MBCS)。
********************************************************************/
int ReadCharDistX(WORD wCode)
{
	if('U' == _fl_header->magic[0])     //unicode 编码
		return ReadCharDistX_U(wCode);
	else
		return 0;//ReadCharDistX_M(wCode);
}

/**********************************************************************
功能： 获取点阵信息
参数： wCode 在这里预留，主要是因为前面有保存一个全局的g_dwCharInfo，也就知道了该字符的相应信息(宽度+点阵信息的起始地址)。
       fontArray 存放点阵信息
	   bytesPerLine 每一行占多少个字节。
**********************************************************************/
int ReadCharDotArray(WORD wCode, WORD *bytesPerLine)
{
	*bytesPerLine= (WORD)((GET_FONT_WIDTH(g_dwCharInfo))+7)/PIXELS_PER_BYTE;	
//	*bytesPerLine= (WORD)((GET_FONT_WIDTH(g_dwCharInfo))+7) >> 3;	

	if(g_dwCharInfo > 0)
	{
		DWORD nDataLen = *bytesPerLine * _fl_header->YSize;			
		/*因为只支持16X16格式所以*/
//		DWORD nDataLen = *bytesPerLine << 4;
		DWORD  dwOffset = GET_FONT_OFFADDR(g_dwCharInfo);    //获取字符点阵的地址信息(低26位)
		return dwOffset;
	}

	return 0;
}

// 获取字符的像素宽度
DWORD ReadCharDistX_U(WORD wCode)
{
	DWORD offset ;
	int   i;
	g_dwCharInfo = 0;
	
	for(i = 0;i<_fl_header->nSection;i++)
	{
		if(wCode >= _fl_header->pSection[i].First && wCode <= _fl_header->pSection[i].Last)
			break;		
	}
	if(i >= _fl_header->nSection)	
		return 0;	

	offset = BASE_FONT_ADDR + _fl_header->pSection[i].OffAddr+ FONT_INDEX_TAB_SIZE*(wCode - _fl_header->pSection[i].First );	
	g_dwCharInfo = (*(DWORD *)offset);

	return GET_FONT_WIDTH(g_dwCharInfo);
}

static int unifont_unload(void)
{
	if ((*(DWORD *)VARIABLE_ADDR) != UNIFONT_LOAD_FLAG)
		return 0;
	memmove(*term_backup_orig_addr, term_backup, sizeof(struct term_entry));
	if (graphics_cursor_bak) graphics_CURSOR = graphics_cursor_bak;
	(*(DWORD *)VARIABLE_ADDR) = 0;
	ushFontReaded = 0;
	return printf("UNIFONT Unload success!\n");
}
