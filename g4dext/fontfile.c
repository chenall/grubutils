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

gcc -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE fontfile.c

 * disassemble:			objdump -d a.out 
 * confirm no relocation:	readelf -r a.out
 * generate executable:		objcopy -O binary a.out fontfile
 *
 */
/*
 * to get this source code & binary: http://grub4dos-chenall.google.com
 * For more information.Please visit web-site at http://chenall.net
 * 2010-02-25
 * 2010-09-23 supported new version of grub4dos.
 */
#include "grub4dos.h"
//#include "fontfile.h"
#define A_NORMAL        0x7
#define A_REVERSE       0x70
#define VIDEOMEM 0xA0000
#define BASE_FONT_ADDR 0x400000

#define VSHADOW VSHADOW1
#define VSHADOW_SIZE 60000
#define VSHADOW1 ((unsigned char *)0x3A0000)
#define VSHADOW2 (VSHADOW1 + VSHADOW_SIZE)
#define VSHADOW4 (VSHADOW2 + VSHADOW_SIZE)
#define VSHADOW8 (VSHADOW4 + VSHADOW_SIZE)
#define text ((unsigned long *)0x3FC000)

static int dbcs_ending_byte = 0;

static int fontfile_func (char *arg ,int flags);
static void graphics_cursor (int set);
static void graphics_setxy (int col, int row);
static void graphics_scroll (void);
void graphics_putchar (int ch);
static unsigned short ushFontReaded = 0;	/* font loaded? */

static unsigned char chr[16 << 2];
static unsigned char mask[16];
static int graphics_old_cursor;
static int no_scroll = 0;

/* color state */
static int graphics_standard_color = A_NORMAL;
static int graphics_normal_color = A_NORMAL;
static int graphics_highlight_color = A_REVERSE;
static int graphics_helptext_color = A_NORMAL;
static int graphics_heading_color = A_NORMAL;
static int graphics_current_color = A_NORMAL;

int outline = 0;
static unsigned char by[32], chsa[16], chsb[16];
int disable_space_highlight = 0;
#define x0 0
#define x1 current_term->chars_per_line
#define y0 0
#define y1 current_term->max_lines

unsigned int *minifnt_start;
unsigned short *minifnt_flag = (unsigned short *)BASE_FONT_ADDR;

/* this is needed, see the comment in grubprog.h */
#include "grubprog.h"
/* Do not insert any other asm lines here. */

int
main (char *arg,int flags)
{
	void *p = &main;
	if (strlen(arg) <= 0)
	{
		return 1;
	}

	while(arg[0]==' ' || arg[0]=='\t') arg++;

	if (strlen(arg) > 256)
	return !(errnum = ERR_WONT_FIT);

	if (open(arg))
	{
		ushFontReaded = 1;
		read ((unsigned long long)(unsigned int)(char *) RAW_ADDR (BASE_FONT_ADDR), -1, 0xedde0d90);
		memmove((char *)BASE_FONT_ADDR + filemax ,p,(int)&GRUB - (int)p );
		close();
		if (*minifnt_flag)
		{
			minifnt_start = (unsigned int *)(BASE_FONT_ADDR + (*minifnt_flag) * 2);
		}
		return fontfile_func(arg,flags);
	}
	else
	{
		ushFontReaded = 0;
		printf ("load fontfile failed!\n");
		return 0;
	}
}

static inline void outb(unsigned short port, unsigned char val)
{
    __asm __volatile ("outb %0,%1"::"a" (val), "d" (port));
}

static void MapMask(int value) {
    outb(0x3c4, 2);
    outb(0x3c5, value);
}
#if 0
static int get_mini_fnt(void)
{
	if (! *minifnt_flag)
		return 1;

	unsigned short *_ch = (unsigned short *)BASE_FONT_ADDR + 1;
	unsigned short cur_ch;
	cur_ch = (text[fonty * x1 + fontx - 1] & 0xff) + ((text[fonty * x1 + fontx] & 0xff) << 8);
	
	int i;
	for (i=0;i < (*minifnt_flag);i++)
	{
		if ( *(_ch++) == cur_ch )
		{
			return (2 + 2 * (*minifnt_flag) + i * 32);
		}
	}
	return 0;
}
#endif
//#ifdef SUPPORT_GRAPHICS
/* Chinese Support by Gandalf
 *    These codes used for defining a proper fontfile
 */
static void
graphics_cursor (int set)
{

    unsigned char *pat, *mem, *ptr;
    int i, ch, offset, n, ch1, ch2 ,ch_flag;
    int invert = 0;

    if (set && no_scroll)
        return;

    offset = cursorY * x1 + fontx;

    ch = text[fonty * x1 + fontx] & 0xff;
    if (ch != ' ' || ! disable_space_highlight)
	invert = (text[fonty * x1 + fontx] & /*0xff00*/ 0xffff0000) != 0;


    pat = font8x16 + (ch << 4);

    dbcs_ending_byte = 0;
	ch_flag = 0;
	
    if (ushFontReaded && ch >= 0xa1)
    {

	/* here is one trick, look one line as a string with '\0' ended, so,
	 * if the last byte is the 1st byte of a Chinese Character,,, ,,, */
        for (n = 0; n < fontx; n++)
        {
            ch1 = text[fonty * x1 + n] & 0xff;
            ch2 = text[fonty * x1 + n+1] & 0xff;

            if ((ch1>=0xa1 && ch1<=0xfe)&&(ch2>=0xa1 && ch2<=0xfe))
            {
                if (n == fontx-1)
                {
					int dotpos = 0;

					if (*minifnt_flag)
					{
						unsigned short *_ch = (unsigned short *)BASE_FONT_ADDR + 1;
						unsigned short cur_ch;
						cur_ch = (ch2 << 8) + ch1;
						int i;

						for (i=0;i < (*minifnt_flag);i++)
						{
							if ( *(_ch++) == cur_ch )
							{
								dotpos = 2 + (*minifnt_flag << 1) + (i << 5);
								ch_flag = 1;
								break;
							}
						}

						if (! ch_flag) break;

					}
					else
					{
						dotpos = (ch1-0xa1)*94 + ch2 - 0xa1;
						dotpos<<=5;
					}
					
					dbcs_ending_byte = ch;
					memmove(by, (unsigned char *) RAW_ADDR (BASE_FONT_ADDR + dotpos), 32);

					for (i = 0; i <32; i ++)
					{
						if (i%2)
							chsb[i/2] = by[i];
						else
							chsa[i/2] = by[i];	
					}
				}
			n++;
			}
		}

		if (! dbcs_ending_byte)
		{
			if (fontx == x1 - 1)
			{
				putchar (' ');
				text[fonty * x1 + fontx] = ch;
				text[fonty * x1 + fontx] &= 0x00ff;
				if (graphics_current_color == graphics_highlight_color)//if (graphics_current_color & 0xf0)
					text[fonty * x1 + fontx] |= 0x10000;//0x100;
				return;
			}
			if (! *minifnt_flag)
				return;
		}
    }

    mem = (unsigned char*)VIDEOMEM + offset;
    
	if (dbcs_ending_byte) 
    {
        mem--;//mem = (unsigned char*)VIDEOMEM + offset -1;
	offset--;
        pat = chsa;
    }

write_char:

    if (set)
    {
        MapMask(15);
        ptr = mem;
        for (i = 0; i < 16; i++, ptr += x1)
       	{
            cursorBuf[i] = pat[i];
            *ptr = ~pat[i];
        }
        return;
    }

    if (outline)
      for (i = 0; i < 16; i++)
      {
        mask[i] = pat[i];
	if (i < 15)
		mask[i] |= pat[i+1];
	if (i > 0)
		mask[i] |= pat[i-1];
        mask[i] |= (mask[i] << 1) | (mask[i] >> 1);
	mask[i] = ~(mask[i]);
      }

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

	c1 = ((unsigned char*)VSHADOW1)[offset];
	c2 = ((unsigned char*)VSHADOW2)[offset];
	c4 = ((unsigned char*)VSHADOW4)[offset];
	c8 = ((unsigned char*)VSHADOW8)[offset];

	if (outline)
	{
		m = mask[i];

		c1 &= m;
		c2 &= m;
		c4 &= m;
		c8 &= m;
	}
	
	c1 |= p;
	c2 |= p;
	c4 |= p;
	c8 |= p;

	chr[i     ] = c1;
	chr[16 + i] = c2;
	chr[32 + i] = c4;
	chr[48 + i] = c8;
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
	offset = cursorY * x1 + fontx;
	pat = chsb;
	goto write_char;
    }
}
#if 0
void
graphics_gotoxy (int x, int y)
{
    graphics_cursor(0);

    graphics_setxy(x, y);

    graphics_cursor(1);
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
        graphics_gotoxy (x0, j - 1);

        for (i = x0; i < x1; i++)
       	{
            graphics_putchar (text[j * 80 + i]);
        }
    }

    /* last line should be blank */
    graphics_gotoxy (x0, y1 - 1);

    for (i = x0; i < x1; i++)
        graphics_putchar (' ');

    graphics_setxy (x0, y1 - 1);

    no_scroll = 0;
}

void
graphics_putchar (int ch)
{
    ch &= 0xff;
    //graphics_cursor(0);

    if (ch == '\n') {
        if (fonty + 1 < y1)
            graphics_gotoxy(fontx, fonty + 1);
        else
	{
	    graphics_cursor(0);
            graphics_scroll();
	    graphics_cursor(1);
	}
        //graphics_cursor(1);
        return;
    } else if (ch == '\r') {
        gotoxy(x0, fonty);
        //graphics_cursor(1);
        return;
    }

    //graphics_cursor(0);

    text[fonty * 80 + fontx] = ch;
    text[fonty * 80 + fontx] &= 0x00ff;
    if (graphics_current_color == graphics_highlight_color)//if (graphics_current_color & 0xf0)
        text[fonty * 80 + fontx] |= 0x10000;//0x100;

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
#endif
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
	int graphics_cursor_bak = graphics_CURSOR;
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

	/* FIXME: should we be explicitly switching the terminal as a 
	 * side effect here? */
	builtin_cmd("terminal", "graphics", flags);

	return 1;
}
