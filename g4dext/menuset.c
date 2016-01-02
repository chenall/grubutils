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

gcc -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE menuset.c

 * disassemble:			objdump -d a.out 
 * confirm no relocation:	readelf -r a.out
 * generate executable:		objcopy -O binary a.out menuset
 *
 */
/*
 * to get this source code & binary: http://grub4dos-chenall.google.com
 * For more information.Please visit web-site at http://chenall.net
 * 2010-05-20
 */
#include "grub4dos.h"

struct broder {
	unsigned char disp_ul;
	unsigned char disp_ur;
	unsigned char disp_ll;
	unsigned char disp_lr;
	unsigned char disp_horiz;
	unsigned char disp_vert;
	unsigned char menu_box_x; /* line start */
	unsigned char menu_box_w; /* line width */
	unsigned char menu_box_y; /* first line number */
	unsigned char menu_box_h;
	unsigned char menu_box_b;
} __attribute__ ((packed));

/* this is needed, see the comment in grubprog.h */
#include "grubprog.h"
/* Do not insert any other asm lines here. */

int main (char *arg, int flags)
{
	struct broder tmp_broder = {218,191,192,217,196,179,2,76,2,0,0};
	char *p=(char *)&tmp_broder;
	int i;
	unsigned long long val;
	
	if (*arg)
	{
		memmove ((char *)&tmp_broder,menu_border,sizeof(tmp_broder));
	}
	
	for (i=0;i< 11 && *arg;i++)
	{
		if (safe_parse_maxint(&arg, &val))
		{
			val &= 0xff;
			if (val)
			{
				*p = (unsigned char)val ;
			}
		}
		else
		{
			(*p = *arg);
		}
		p++;
		arg = skip_to (0, arg);
	}
	if (tmp_broder.menu_box_h > current_term->max_lines - tmp_broder.menu_box_y)
	{
		tmp_broder.menu_box_h = 0;
	}
	if (tmp_broder.menu_box_w > current_term->chars_per_line-2) tmp_broder.menu_box_w = current_term->chars_per_line-2;
	if (tmp_broder.menu_box_x > current_term->chars_per_line-2) tmp_broder.menu_box_x = 2;
	if (tmp_broder.menu_box_x + tmp_broder.menu_box_w > current_term->chars_per_line-2) tmp_broder.menu_box_x = current_term->chars_per_line-2 - tmp_broder.menu_box_w;
	
	memmove (menu_border,(char *)&tmp_broder,sizeof(tmp_broder));
	return 1;
}
