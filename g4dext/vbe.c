/*
 *  memcheck  --  Check implementation of int15/E820.
 *  Copyright (C) 2011  tinybit(tinybit@tom.com)
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

gcc -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE demo1.c -o demo1.o

 * disassemble:			objdump -d demo1.o
 * confirm no relocation:	readelf -r demo1.o
 * generate executable:		objcopy -O binary demo1.o demo1
 *
 */

#include "grub4dos.h"
#define memcpy memmove
#define uint32_t unsigned long
#define uint16_t unsigned short
#define uint8_t unsigned char
//#include "vbe.h"
  struct _VBEModeInfo_t
  {
    uint16_t ModeAttributes;
    uint8_t  WinAAttributes;
    uint8_t  WinBAttributes;
    uint16_t WinGranularity;
    uint16_t WinSize;
    uint16_t WinASegment;
    uint16_t WinBSegment;
    uint32_t WinFuncPtr;
    uint16_t BytesPerScanline;

    // VBE 1.2+
    uint16_t XResolution;
    uint16_t YResolution;
    uint8_t  XCharSize;
    uint8_t  YCharSize;
    uint8_t  NumberOfPlanes;
    uint8_t  BitsPerPixel;
    uint8_t  NumberOfBanks;
    uint8_t  MemoryModel;
    uint8_t  BankSize;
    uint8_t  NumberOfImagePages;
    uint8_t  Reserved1;

    // VBE 1.2+ Direct Color fields
    uint8_t  RedMaskSize;
    uint8_t  RedFieldPosition;
    uint8_t  GreenMaskSize;
    uint8_t  GreenFieldPosition;
    uint8_t  BlueMaskSize;
    uint8_t  BlueFieldPosition;
    uint8_t  RsvdMaskSize;
    uint8_t  RsvdFieldPosition;
    uint8_t  DirectColorModeInfo;

    // VBE 2.0+
    uint32_t PhysBasePtr;
    uint32_t Reserved2; // OffScreenMemOffset;
    uint16_t Reserved3; // OffScreenMemSize;

    // VBE 3.0+
    uint16_t LinBytesPerScanline;
    uint8_t  BnkNumberOfImagePages;
    uint8_t  LinNumberOfImagePages;
    uint8_t  LinRedMaskSize;
    uint8_t  LinRedFieldPosition;
    uint8_t  LinGreenMaskSize;
    uint8_t  LinGreenFieldPosition;
    uint8_t  LinBlueMaskSize;
    uint8_t  LinBlueFieldPosition;
    uint8_t  LinRsvdMaskSize;
    uint8_t  LinRsvdFieldPosition;
    uint32_t MaxPixelClock;

    uint8_t  Reserved4[189];
  } __attribute__ ((packed));
  typedef struct _VBEModeInfo_t VBEModeInfo_t;
  struct _VBEDriverInfo_t
  {
    uint8_t  VBESignature[4];
    uint16_t VBEVersion;
    uint32_t OEMStringPtr;
    uint32_t Capabilities;
    uint32_t VideoModePtr;
    uint16_t TotalMemory;

    // VBE 2.0 extensions
    uint16_t OemSoftwareRev;
    uint32_t OemVendorNamePtr;
    uint32_t OemProductNamePtr;
    uint32_t OemProductRevPtr;
    uint8_t  Reserved[222];
    uint8_t  OemDATA[256];
  } __attribute__ ((packed));
  typedef struct _VBEDriverInfo_t VBEDriverInfo_t;
  #define VBE_CAPS_DAC8      (((uint32_t)1) << 0)
  #define VBE_CAPS_NOTVGA    (((uint32_t)1) << 1)
  #define VBE_CAPS_BLANKFN9  (((uint32_t)1) << 2)
  #define VBE_MA_MODEHW (((uint32_t)1) << 0)
  #define VBE_MA_HASLFB (((uint32_t)1) << 7)
int GetModeInfo (uint16_t mode);
int SetMode (uint16_t mode);
void CloseMode ();

void gfx_Clear8  (uint32_t color);
void gfx_Clear16 (uint32_t color);
void gfx_Clear24 (uint32_t color);
void gfx_Clear32 (uint32_t color);
void SetPixel (long x, long y, uint32_t color);
static void (*gfx_FillRect)(long x1, long y1, long x2, long y2, uint32_t color);
static void gfx_FillRect24 (long x1, long y1, long x2, long y2, uint32_t color);
static void gfx_FillRect32 (long x1, long y1, long x2, long y2, uint32_t color);

/****************************************/
/* All global variables must be static! */
/****************************************/

static VBEDriverInfo_t drvInfo;
static VBEModeInfo_t modeInfo;
static uint32_t l_pixel;

void SetPixelBG (long x,long y,int a);
static int get_font(void);
static int _INT10_(uint32_t eax,uint32_t ebx,uint32_t ecx,uint32_t edx,uint32_t es,uint32_t edi);
static int vbe_init(int mode);
static void vbe_end(void);
static void vbe_cls(void);
static void vbe_gotoxy(int x, int y);
static int vbe_getxy(void);
static void vbe_scroll(void);
static void vbe_putchar (unsigned int c);
static int vbe_setcursor (int on);
static void vbe_cursor(int on);
static int vbe_standard_color = -1;
static int vbe_normal_color = 0x55ffff;
static int vbe_highlight_color = 0xff5555;
static int vbe_helptext_color = 0xff55ff;
static int vbe_heading_color = 0xffff55;
static int vbe_cursor_color = 0xff00ff;
static color_state vbe_color_state = COLOR_STATE_STANDARD;
static void vbe_setcolorstate (color_state state);
static void vbe_setcolor (int normal_color, int highlight_color, int helptext_color, int heading_color);
static struct 
{
   char name[16];
   unsigned short current_mode;
   unsigned short cursor;
   unsigned char *image;
   unsigned long BytesToLFB;
   unsigned long BytesPerChar;
} vbe = {"VESA_TEST",0,1,NULL,0,0};

static int vbe_vfont(char *file);
static unsigned vfont_size=0;
static uint8_t pre_ch;
static struct term_entry vesa;
static char mask[8]={0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
static short mask_u[16]={1<<0, 1<<1, 1<<2,1<<3, 1<<4, 1<<5, 1<<6,1<<7,1<<8,1<<9,1<<10,1<<11,1<<12,1<<13,1<<14,1<<15};
static char vfont[2048];
static int open_bmp(char *file);
static unsigned short *text = (unsigned short *)0x3FC000;
static unsigned long cursor_offset = 0;

struct realmode_regs {
	unsigned long edi; // as input and output
	unsigned long esi; // as input and output
	unsigned long ebp; // as input and output
	unsigned long esp; // stack pointer, as input
	unsigned long ebx; // as input and output
	unsigned long edx; // as input and output
	unsigned long ecx; // as input and output
	unsigned long eax;// as input and output
	unsigned long gs; // as input and output
	unsigned long fs; // as input and output
	unsigned long es; // as input and output
	unsigned long ds; // as input and output
	unsigned long ss; // stack segment, as input
	unsigned long eip; // instruction pointer, as input, 
	unsigned long cs; // code segment, as input
	unsigned long eflags; // as input and output
};
unsigned long long GRUB = 0x534f443442555247LL;/* this is needed, see the following comment. */
/* gcc treat the following as data only if a global initialization like the
 * above line occurs.
 */

//asm(".long 0x534F4434");

/* a valid executable file for grub4dos must end with these 8 bytes */
asm(ASM_BUILD_DATE);
asm(".long 0x03051805");
asm(".long 0xBCBAA7BA");

/* thank goodness gcc will place the above 8 bytes at the end of the b.out
 * file. Do not insert any other asm lines here.
 */

void demo1_Run();

/*******************************************************************/
/* main() must be the first function. Don't insert functions here! */
/*******************************************************************/

int main(char *arg,int flags)//( int argc, char* argv[] )
{
  uint16_t*	modeList;
  uint16_t	mode = 0;

	if ((int)&main != 0x50000)//init move program to 0x50000
	{
		if (strcmp(current_term->name,vbe.name) != 0)
		{
			int prog_len=(*(int *)(&main - 20));
			memmove((void*)0x50000,&main,prog_len);
		}
		return ((int (*)(char *,int))0x50000)(arg,flags);
	}

  // Display driver information
  if (! GetDriverInfo ())
  {
    printf( "ERROR: VBE driver not supported.\n" );
    return 0;
  }

	struct term_entry *pre_term = current_term;;
	if (strcmp(current_term->name,vbe.name) != 0)
	{
		vesa.name = vbe.name;
		vesa.flags = 0;
		vesa.PUTCHAR = vbe_putchar;
		vesa.CHECKKEY = current_term->CHECKKEY;
		vesa.GETKEY = current_term->GETKEY;
		vesa.GETXY = vbe_getxy;
		vesa.GOTOXY = vbe_gotoxy;
		vesa.CLS = vbe_cls;
		vesa.SETCOLORSTATE = vbe_setcolorstate;
		vesa.SETCOLOR = vbe_setcolor;
		vesa.SETCURSOR = vbe_setcursor;
		vesa.STARTUP = vbe_init;
		vesa.SHUTDOWN = vbe_end;
		font8x16 = (char*)get_font();
	}

	if (memcmp(arg,"demo",4) != 0)
	{
		while (*arg)
		{
			if (memcmp(arg,"vfont=",6) == 0)
			{
				arg = skip_to(1,arg);
				vbe_vfont(arg);
			}
			if (memcmp(arg,"mode=",5) == 0)
			{
				arg+=5;
				unsigned long long ll;
				if (!(safe_parse_maxint(&arg,&ll)))
					return 0;
				mode = (uint16_t)ll;
			}
			if (memcmp(arg,"bmp",3)== 0)
			{
				open_bmp(skip_to(1,arg));
			}
			arg = skip_to(0,arg);
		}
		if (!mode && !vbe.current_mode)
		{
			uint16_t* modeList = (uint16_t*)drvInfo.VideoModePtr;
			uint16_t auto_mode;
			while ((auto_mode = *modeList++) != 0xFFFF)
			{
				if (! GetModeInfo (auto_mode) ||
				   modeInfo.XResolution != 800 ||
				   modeInfo.YResolution != 600)
					continue;
				if (modeInfo.BitsPerPixel == 32)
				{
					mode = auto_mode;
					break;
				}
				else if (modeInfo.BitsPerPixel == 24)
				{
					mode = auto_mode;
				}
			}
		}
		if (!vbe_init (mode))
		{
			printf( "VBE mode %4x not supported\n", mode);
			getkey();
			return 0;
		}

		vesa.chars_per_line = modeInfo.XResolution>>3;
		vesa.max_lines = modeInfo.YResolution>>4;
		current_term = &vesa;
		if (vbe.image)
			memmove((char*)modeInfo.PhysBasePtr,vbe.image,modeInfo.YResolution*modeInfo.BytesPerScanline);
		printf("VBE for GRUB4DOS Current Mode: %dx%dx%d,%dx%d\n",modeInfo.XResolution,modeInfo.YResolution,modeInfo.BitsPerPixel,vesa.chars_per_line,vesa.max_lines);
//		getkey();
		return 1;
	}
	return 0;
}

static int vbe_vfont(char *file)
{
	unsigned int size;
	if (!open(file))
	{
		return !printf("Error: open_file\n");
	}
	filepos = 0x59;
	if (! read((unsigned long long)(unsigned int)&size,4,GRUB_READ) || size != 0x52515350)
	{
		close();
		return !printf("Err: fil");
	}
	filepos = 0x1ce;
	if (! read((unsigned long long)(unsigned int)&vfont_size,2,GRUB_READ))
	{
		close();
		return !printf("Error:read1\n");
	}
	vfont_size &= 0xFF;
	if (vfont_size > 64)
		vfont_size = 64;
	read((unsigned long long)(unsigned int)vfont,vfont_size*32,GRUB_READ);
	close();
	return 1;
}

static int _INT10_(uint32_t eax,uint32_t ebx,uint32_t ecx,uint32_t edx,uint32_t es,uint32_t edi)
{
	struct realmode_regs int_regs = {edi,0,0,-1,ebx,edx,ecx,eax,-1,-1,es,-1,-1,0xFFFF10CD,-1,-1};
	realmode_run((long)&int_regs);
	return (unsigned short)int_regs.eax;
}

static int vbe_setcursor (int on)
{
	vbe.cursor = on;
	vbe_cursor(on);
	return 0;
}

static int vbe_init(int mode)
{
   if (!mode)
   {
      mode = vbe.current_mode;
   }
   if (!GetModeInfo(mode) || !SetMode (mode))
      return 0;
   current_color = -1;
   vbe.current_mode = mode;
   vbe.BytesPerChar = modeInfo.BitsPerPixel>>3;
   fontx=0,fonty=0;
   vesa.chars_per_line = modeInfo.XResolution>>3;
   vesa.max_lines = modeInfo.YResolution>>4;
   return 1;
}

static void vbe_end(void)
{
	if (vbe.image)
		free(vbe.image);
	CloseMode ();
}

static void vbe_cls(void)
{
   if (vbe.image)
      memmove((char*)modeInfo.PhysBasePtr,vbe.image,modeInfo.YResolution * modeInfo.BytesPerScanline);
   else
   {
      uint32_t* l_lfb = (uint32_t*)modeInfo.PhysBasePtr;
      uint32_t size = (modeInfo.YResolution * modeInfo.BytesPerScanline)>>2;
      while(size--)
      {
         *l_lfb++=0;
      }
	}
	cursor_offset = 0;
	fontx = 0;
	fonty = 0;
}

static void vbe_cursor(int on)
{
   if (on)
      gfx_FillRect (fontx<<3,(fonty<<4)+15,(fontx+1)<<3,(fonty+1)<<4, vbe_cursor_color);
   else if (vbe.image)
   {
      uint8_t *lfb,*bg;
      lfb = (uint8_t*)modeInfo.PhysBasePtr + ((fonty<<4)+15)*modeInfo.BytesPerScanline + (fontx<<3)*vbe.BytesPerChar;
      bg = lfb - vbe.BytesToLFB;
      memmove(lfb,bg,vbe.BytesPerChar+1<<3);
      memmove(lfb+modeInfo.BytesPerScanline,bg+modeInfo.BytesPerScanline,vbe.BytesPerChar+1<<3);
   }
   else
      gfx_FillRect (fontx<<3,(fonty<<4)+15,(fontx+1)<<3,(fonty+1)<<4, 0);
}

static void vbe_gotoxy(int x, int y)
{
	int i;
	if (x > vesa.chars_per_line)
		x = vesa.chars_per_line;
	if (y > vesa.max_lines)
		y = vesa.max_lines;
   if (vbe.cursor)
	{
		vbe_cursor(0);
	}
	fontx = x;
	fonty = y;
	if (vbe.cursor)
	{
		vbe_cursor(1);
	}
	return;
}

static char inb(unsigned short port)
{
	char ret_val;
	__asm__ volatile ("inb %%dx,%%al" : "=a" (ret_val) : "d"(port));
	return ret_val;
}

void SetPixelBG(long x,long y,int a)
{
   uint8_t *lfb = (uint8_t*)modeInfo.PhysBasePtr + y*modeInfo.BytesPerScanline + x*vbe.BytesPerChar;
   if (vbe.image)
   {
      if (modeInfo.BitsPerPixel == 32)
         *(unsigned long *)lfb = *(unsigned long *)(lfb - vbe.BytesToLFB);
      else
         *(unsigned short *)lfb = *(unsigned short *)(lfb - vbe.BytesToLFB);
         lfb[2] = *(lfb - vbe.BytesToLFB + 2);
   }
   else
      *(unsigned long *)lfb = 0;
   if (a)
      *(unsigned long *)lfb = ~*(unsigned long *)lfb;
}

static void vbe_scroll(void)
{
	uint8_t* l_lfb = (uint8_t*)modeInfo.PhysBasePtr ;

	if(l_lfb)
	{
		uint32_t i,j;
		j = modeInfo.BytesPerScanline<<4;
		if (vbe.image)
		{
         uint8_t *ToBG,*FromBG,*l_lfb1;
         int k;
         ToBG = l_lfb - vbe.BytesToLFB;
         l_lfb1 = l_lfb + j;
         FromBG = l_lfb1 - vbe.BytesToLFB;
         for (i=0;i<fonty;++i)
         {
            memmove(l_lfb,ToBG,j);
            for (k = 0;k<modeInfo.BytesPerScanline<<4;++k)
            {
               if (l_lfb1[k] != FromBG[k])
               {
                  l_lfb[k] = l_lfb1[k];
               }
            }
            l_lfb += j;
            ToBG += j;
            FromBG +=j;
            l_lfb1 +=j;
         }
         memmove(l_lfb,ToBG,j);
		}
		else
		{
         for (i=0;i<fonty;++i)
         {
            memmove(l_lfb,l_lfb+j,j);
            l_lfb+=j;
         }
		}
	}
	return;
}

static int vbe_getxy(void)
{
	return (fonty << 8) | fontx;
}

static int get_font(void)
{
	struct realmode_regs int_regs = {0,0,0,-1,0x600,0,0,0x1130,-1,-1,-1,-1,-1,0xFFFF10CD,-1,-1};
	realmode_run((long)&int_regs);
	return (int_regs.es<<4)+int_regs.ebp;
}

/* unifont start at 24M */
#define UNIFONT_START		0x1800000

#define narrow_char_indicator	(*(unsigned long *)(UNIFONT_START + ('A' << 5)))
#define utf8_tmp ((char *)(UNIFONT_START + ('A' << 5) + 4))

static unsigned long utf8_unicode(unsigned long offset,int n)
{
	unsigned short *c1,*p_text = text + offset - n;
	unsigned long i,j,c;
	unsigned long ch;
	if (*p_text == 196 && p_text[1] == 191 && *(p_text - 1) == 196)
	{
		return 0;
	}
	c = *p_text;
	/*检测该utf8字符串长度是否为n*/
	for(i=0;i<=6;++i)
	{
		if (!(c & mask[i]))
			break;
	}
	if (i != (n+1))
		return 0;
	ch = ((unsigned char)*p_text)<<i;
	ch >>= i;
	for (i = 0;i<=n;++i)
	{
		ch <<= 6;
		ch |= *p_text & 0x3f;
		*p_text++ = 0;
	}
	text[offset-n] = ch;
	return n;
}

static unsigned long scan_utf8(unsigned long offset)
{
	int i,j,ch;
	unsigned short *p_text = text;
	j = (offset >= 6?6:offset);
	p_text += offset;
	for (i=0; i<j ;++i)
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
			return utf8_unicode(offset,i);
		}
		else if (ch != 0x80)
		{
			return 0;
		}
		--p_text;
	}
	return 0;
}

static void vbe_putchar (unsigned int c)
{
	if (fontx >= vesa.chars_per_line)
		fontx = 0,++fonty;
	if (fonty >= vesa.max_lines)
	{
		vbe_scroll();
		--fonty;
	}

	if ((char)c == '\n')
	{
		return vbe_gotoxy(fontx, fonty + 1);
	}
	else if ((char)c == '\r') {
		return vbe_gotoxy(0, fonty);
	}

	unsigned long offset = fonty * vesa.chars_per_line + fontx;
	text[offset] = (unsigned short)c;
	vbe_cursor(0);

	char *cha = font8x16 + ((unsigned char)c<<4);
	int i,j;
	uint8_t *lfb = (uint8_t*)modeInfo.PhysBasePtr + (fonty*modeInfo.BytesPerScanline<<4);

	if (narrow_char_indicator)
	{
		unsigned short unicode = scan_utf8(offset);
		char k;
		unsigned short *ch_u;
		if (unicode)
		{
			fontx -= unicode;
			lfb += fontx*vbe.BytesPerChar<<3;
			uint8_t *lfbx;
			unicode = text[offset-unicode];
			ch_u = (unsigned short *)UNIFONT_START + (unicode << 4);
			if ((*(unsigned char *)(0x100000 + unicode)) & 1)
			{
				k = 16;
			}
			else
			{
				k = 8;
				ch_u += 8;
			}
			for(j=0;j<k;++j)
			{
				lfbx = lfb;
				for (i=0;i<16;++i)
				{
					if (ch_u[j] & mask_u[i])
					{
						switch (modeInfo.BitsPerPixel)
						{
							case 24:
								*(uint16_t *)lfbx = (uint16_t)current_color;
								lfbx[2] = (uint8_t)(current_color >> 16);
								break;
							case 32:
								*(uint32_t *)lfbx = (uint32_t)current_color;
								break;
						}
					}
					else
						SetPixelBG (fontx*8+j,fonty*16+i,c>>16);
					lfbx += modeInfo.BytesPerScanline;
				}
				lfb +=vbe.BytesPerChar;
			}
			if (k == 16)
				++fontx;
		}
		else
		{
			for (j=0;j<16;++j)
			{
				for (i=0;i<8;++i)
				{
					if (cha[j] & mask[i])
						SetPixel (fontx*8+i,fonty*16+j,current_color);
					else
						SetPixelBG (fontx*8+i,fonty*16+j,c>>16);
				}
			}
		}
	}
	else
	{
		if (vfont_size)
		{
			if ((unsigned char)c >= 0x80 && (pre_ch - (unsigned char)c) == 0x40 && (unsigned char)c < 0x80+vfont_size)
			{
				uint8_t ch = pre_ch;
				if (!fontx && fonty)
				{
					vbe_gotoxy(vesa.chars_per_line - 1 ,fonty-1);
					vbe_putchar(' ');
				}
				else if (fontx)
					--fontx;

				cha = vfont + (ch - 0xC0)*32;

				for (j=0;j<16;++j)
				{
					for (i=0;i<8;++i)
					{
						if (cha[j] & mask[i])
							SetPixel (fontx*8+i,fonty*16+j,current_color);
						else
							SetPixelBG (fontx*8+i,fonty*16+j,c>>16);
					}
				}
				++fontx;
				cha += 16;
			}
			pre_ch = (uint8_t)c;
		}
		for (j=0;j<16;++j)
		{
			for (i=0;i<8;++i)
			{
				if (cha[j] & mask[i])
					SetPixel (fontx*8+i,fonty*16+j,current_color);
				else
					SetPixelBG (fontx*8+i,fonty*16+j,c>>16);
			}
		}
	}
	if (fontx + 1 >= vesa.chars_per_line)
	{
		++fonty;
		if (fonty >= vesa.max_lines)
		{
			vbe_scroll();
			--fonty;
		}
		vbe_gotoxy(0, fonty);
	}
	else
		vbe_gotoxy(fontx + 1, fonty);
	return;
}

//  /*
//   *  void gfx_SetPixel( *p_ctx, p_x, p_y, p_pixel )
//   *
//   *  Purpose:
//   *    Draws an 8/16/24-BPP pixel at the specified point
//   *    if within the boundaries of p_ctx->clip.
//   *
//   *  Returns:
//   *    Nothing
//   */
//void gfx_SetPixel (vbectx_t* p_ctx, long p_x, long p_y, uint32_t p_pixel)
//{
//	uint8_t* l_lfb;

//	if (p_ctx)
//	{
//		if( p_ctx->lfb )
//		{
//			if ( (p_x < p_ctx->clip.minx) || (p_y < p_ctx->clip.miny)
//				|| (p_x > p_ctx->clip.maxx) || (p_y > p_ctx->clip.maxy) )
//			{
//				return;
//			}
//			uint8_t PixelPerChar = p_ctx->bpp + 7 >> 3;
//			l_lfb = (uint8_t*)(p_ctx->lfb + (p_y * p_ctx->linesize) + p_x*PixelPerChar);
//			while(PixelPerChar--)
//			{
//				*l_lfb++ = (uint8_t)(p_pixel);
//				p_pixel >>= 8;
//			}
//		}
//	}
//}

static void vbe_setcolorstate (color_state state)
{
	switch (state)
	{
		case COLOR_STATE_STANDARD:
			current_color = vbe_standard_color;
			break;
		case COLOR_STATE_NORMAL:
			current_color = vbe_normal_color;
			break;
		case COLOR_STATE_HIGHLIGHT:
			current_color = vbe_highlight_color;
			break;
		case COLOR_STATE_HELPTEXT:
			current_color = vbe_helptext_color;
			break;
		case COLOR_STATE_HEADING:
			current_color = vbe_heading_color;
			break;
		default:
			current_color = vbe_standard_color;
			break;
	}
//	current_color = EncodePixel(current_color>>16,current_color>>8,current_color);
	vbe_color_state = state;
}


static void vbe_setcolor (int normal_color, int highlight_color, int helptext_color, int heading_color)
{
	if (normal_color == -1)
	{
		vbe_standard_color = highlight_color;
		if (vbe_color_state == COLOR_STATE_STANDARD)
         current_color = vbe_standard_color;
		vbe_cursor_color = helptext_color;
		return;
	}
	vbe_normal_color = normal_color;
	vbe_highlight_color = highlight_color;
	vbe_helptext_color = helptext_color;
	vbe_heading_color = heading_color;
	vbe_setcolorstate (vbe_color_state);
}

static uint32_t sys_RM16ToFlat32( uint32_t p_RMSegOfs )
{
	uint32_t hi, lo;
	hi = (p_RMSegOfs >> 16);
	lo = (uint16_t)p_RMSegOfs;
	return (hi << 4) + lo;
}

int GetDriverInfo ()
{
    char             l_vbe2Sig[4] = "VBE2";
    VBEDriverInfo_t* di;
    di = (VBEDriverInfo_t*)0x20000;
    memset (di, 0, 1024);
    memcpy (di, l_vbe2Sig, 4);
	// eax=0x4f00 es=0x2000 edi=0
	   if ((_INT10_(0x4F00,0,0,0,0x2000,0) == 0x4f)
       &&
        (memcmp/*strncmp*/(di->VBESignature, "VESA", 4) == 0) &&
        (di->VBEVersion >= 0x200) )
      {
        di->OEMStringPtr = sys_RM16ToFlat32(di->OEMStringPtr);
        di->VideoModePtr = sys_RM16ToFlat32(di->VideoModePtr);

        di->OemVendorNamePtr =  sys_RM16ToFlat32(di->OemVendorNamePtr);
        di->OemProductNamePtr = sys_RM16ToFlat32(di->OemProductNamePtr);
        di->OemProductRevPtr =  sys_RM16ToFlat32(di->OemProductRevPtr);

        memcpy (&drvInfo, di, sizeof(VBEDriverInfo_t));

        return 1;
      }

    return 0;
}

int GetModeInfo (uint16_t mode)
{
    VBEModeInfo_t* mi;

    if (!mode || mode == 0xFFFF)
      return 0;

    mi = (VBEModeInfo_t*)(0x20000 + 1024);
    memset (mi, 0, 1024);
	//eax=0x4f01 ecx=mode,es=0x2000,edi=1024
	if(_INT10_(0x4F01,0,mode,0,0x2000,1024) != 0x004F)
      return 0;

    // Mode must be supported
    if ((mi->ModeAttributes & VBE_MA_MODEHW) == 0)
      return 0;

    // LFB must be present
    if ((mi->ModeAttributes & VBE_MA_HASLFB) == 0)
      return 0;

    if (mi->PhysBasePtr == 0)
      return 0;

    memcpy (&modeInfo, mi, sizeof(VBEModeInfo_t));
    return 1;
}

  /*
   *    Set VBE LFB display mode. Ignores banked display modes.
   */
int SetMode (uint16_t mode)
{
   if (modeInfo.BitsPerPixel == 24)
      gfx_FillRect = gfx_FillRect24;
   else if (modeInfo.BitsPerPixel == 32)
      gfx_FillRect = gfx_FillRect32;
   else
      return 0;
   
   if (_INT10_(0x4F02,0x4000|mode,0,0,-1,0) != 0x004F)
      return 0;
   return 1;
}

void CloseMode ()
{
	_INT10_(3,0,0,0,-1,0);
}

  /*
   *    Clears the entire LFB
   */
void gfx_Clear24 (uint32_t color)
{
    uint32_t lfbsize = modeInfo.YResolution * modeInfo.BytesPerScanline;
  asm ("  pushl %edi");
  asm ("  cld");
  asm ("  movl    %0, %%edi" : :"m"(modeInfo.PhysBasePtr));
  asm ("  movl    %0, %%ecx" : :"m"(lfbsize));
  asm ("  movl    %0, %%eax" : :"m"(color));
  asm ("  testl   %edi, %edi");
  asm ("  jz      Clear24_Error");
  asm ("  cmpl    $3, %ecx");
  asm ("  jb      Clear24_Error");

  asm ("  movl    %eax, %edx"); /* Prepare last pixel*/
  asm ("  shll    $8, %edx");
  asm ("  andl    $0x00FFFFFF, %eax");
  asm ("  andl    $0xFF000000, %edx");
  asm ("  orl     %edx, %eax");

  asm ("Clear24_Loop:");
  asm ("  subl    $3, %ecx");
  asm ("  jbe     Clear24_Exit");

  /* Write 4 bytes, advance 3 bytes */
  asm ("  stosl");
  asm ("  decl    %edi");
  asm ("  jmp     Clear24_Loop");
  
  asm ("Clear24_Exit:");
  asm ("  roll    $8, %eax"); /* Write last pixel */
  asm ("  movl    %eax, -1(%edi)");
  asm ("Clear24_Error:");
  asm ("  popl %edi");
}

void gfx_Clear32 (uint32_t color)
{
    uint32_t lfbsize = modeInfo.YResolution * modeInfo.BytesPerScanline;
  asm ("  pushl %edi");
  asm ("  cld");
  asm ("  movl    %0, %%edi" : :"m"(modeInfo.PhysBasePtr));
  asm ("  movl    %0, %%ecx" : :"m"(lfbsize));
  asm ("  movl    %0, %%eax" : :"m"(color));
  asm ("  testl   %edi, %edi");
  asm ("  jz      Clear32_Error");
  asm ("  shrl    $2, %ecx");
  asm ("  rep stosl");
  asm ("Clear32_Error:");
  asm ("  popl %edi");
}

void SetPixel (long x, long y, uint32_t color)
{
	uint8_t* lfb;

	if (x < 0 || y < 0 || x >= modeInfo.XResolution || y >= modeInfo.YResolution)
		return;

	lfb = (uint8_t*)(modeInfo.PhysBasePtr + (y * modeInfo.BytesPerScanline) + (x * ((modeInfo.BitsPerPixel + 7) / 8)));
   switch (modeInfo.BitsPerPixel)
   {
      case 24:
         *(uint16_t *)lfb = (uint16_t)color;
         lfb[2] = (uint8_t)(color >> 16);
         break;
      case 32:
         *(uint32_t *)lfb = (uint32_t)color;
         break;
   }
}

  /*
   *    Draws a solid rectangle from (x1, y1) to (x2, y2)
   */
static void gfx_FillRect24 (long x1, long y1, long x2, long y2, uint32_t color)
{
    uint8_t* l_lfb;
    uint32_t l_width;
    uint32_t l_height;
    uint32_t l_skip;
    long  l_minx;
    long  l_miny;
    long  l_maxx;
    long  l_maxy;

    // Clip specified rectangle to context boundaries
    l_minx = 0;
    if( x1 >= l_minx )
      l_minx = x1;

    l_miny = 0;
    if( y1 >= l_miny )
      l_miny = y1;

    l_maxx = modeInfo.XResolution - 1;
    if( x2 <= l_maxx )
      l_maxx = x2;

    l_maxy = modeInfo.YResolution - 1;
    if( y2 <= l_maxy )
      l_maxy = y2;

    // Validate boundaries
    if( l_minx >= l_maxx )
      return;

    if( l_miny >= l_maxy )
      return;

    // Initialize loop variables
    l_width  = (l_maxx - l_minx) + 1;
    l_height = (l_maxy - l_miny) + 1;

    l_skip   = modeInfo.BytesPerScanline - (l_width * 3);

    // Calculate buffer offset
    l_lfb = (uint8_t*)(modeInfo.PhysBasePtr + (l_miny * modeInfo.BytesPerScanline) + (l_minx * 3));

    // Draw rectangle
    //gfx_FillRect24_asm( l_lfb, l_skip, l_width, l_height, color );
  asm ("  pushl %esi");
  asm ("  pushl %edi");
  asm ("  pushl %ebx");
  asm ("  cld");
  asm ("  movl    %0, %%edi" : :"m"(l_lfb));
  asm ("  movl    %0, %%edx" : :"m"(l_skip));
  asm ("  movl    %0, %%ecx" : :"m"(l_width));
  asm ("  movl    %0, %%ebx" : :"m"(l_height));
  asm ("  movl    %0, %%eax" : :"m"(color));
  asm ("  testl   %edi, %edi");
  asm ("  jz      FillRect24_Error");
  asm ("  movl    %ecx, %esi");
  asm ("  addl    %ecx, %ecx");
  asm ("  addl    %esi, %ecx");
  asm ("  movl    %ecx, %esi");

  asm ("FillRect24_Loop:");
  asm ("  cmpl    $3, %ecx");
  asm ("  jbe     FillRect24_ExitLoop");

  /* Write 4 bytes, advance 3 bytes */
  asm ("  stosl");
  asm ("  decl    %edi");

  asm ("  subl    $3, %ecx");
  asm ("  jmp     FillRect24_Loop");

  asm ("FillRect24_ExitLoop:");
  /* Write last pixel */
  asm ("  movb    3(%edi), %cl");
  asm ("  andl    $0x00FFFFFF, %eax");
  asm ("  shll    $24, %ecx");
  asm ("  orl     %ecx, %eax");
  asm ("  stosl");
  asm ("  decl    %edi");

  asm ("  movl    %esi, %ecx");
  asm ("  addl    %edx, %edi");
  asm ("  decl    %ebx");
  asm ("  jnz     FillRect24_Loop");
  asm ("FillRect24_Error:");
  asm ("  popl %ebx");
  asm ("  popl %edi");
  asm ("  popl %esi");
}

static void gfx_FillRect32 (long x1, long y1, long x2, long y2, uint32_t color)
{
    uint32_t* l_lfb;
    uint32_t  l_width;
    uint32_t  l_height;
    uint32_t  l_skip;
    long   l_minx;
    long   l_miny;
    long   l_maxx;
    long   l_maxy;

    // Clip specified rectangle to context boundaries
    l_minx = 0;
    if (l_minx < x1)
        l_minx = x1;

    l_miny = 0;
    if (l_miny < y1)
        l_miny = y1;

    l_maxx = modeInfo.XResolution - 1;
    if (l_maxx > x2)
        l_maxx = x2;

    l_maxy = modeInfo.YResolution - 1;
    if (l_maxy > y2)
        l_maxy = y2;

    // Validate boundaries
    if( l_minx >= l_maxx )
      return;

    if( l_miny >= l_maxy )
      return;

    // Initialize loop variables
    l_width  = (l_maxx - l_minx) + 1;
    l_height = (l_maxy - l_miny) + 1;

    l_skip   = modeInfo.BytesPerScanline - (l_width * 4);

    // Calculate buffer offset
    l_lfb = (uint32_t*)(modeInfo.PhysBasePtr + (l_miny * modeInfo.BytesPerScanline) + (l_minx * 4));

    // Draw rectangle
    //gfx_FillRect32_asm (l_lfb, l_skip, l_width, l_height, color);
  asm ("  pushl %esi");
  asm ("  pushl %edi");
  asm ("  pushl %ebx");
  asm ("  cld");
  asm ("  movl    %0, %%edi" : :"m"(l_lfb));
  asm ("  movl    %0, %%edx" : :"m"(l_skip));
  asm ("  movl    %0, %%ecx" : :"m"(l_width));
  asm ("  movl    %0, %%ebx" : :"m"(l_height));
  asm ("  movl    %0, %%eax" : :"m"(color));
  asm ("  testl   %edi, %edi");
  asm ("  jz      FillRect32_Error");
  asm ("  movl    %ecx, %esi");
  asm ("FillRect32_Loop:");
  asm ("  movl    %esi, %ecx");
  asm ("  rep stosl");
  asm ("  addl    %edx, %edi");
  asm ("  decl    %ebx");
  asm ("  jnz      FillRect32_Loop");
  asm ("FillRect32_Error:");
  asm ("  popl %ebx");
  asm ("  popl %edi");
  asm ("  popl %esi");

}

static int open_bmp(char *file)
{
   typedef unsigned long DWORD;
   typedef unsigned long LONG;
   typedef unsigned short WORD;
   struct { /* bmfh */ 
         WORD bfType;
         DWORD bfSize; 
         DWORD bfReserved1; 
         DWORD bfOffBits;
      } __attribute__ ((packed)) bmfh;
   struct { /* bmih */ 
      DWORD biSize; 
      LONG biWidth; 
      LONG biHeight; 
      WORD biPlanes; 
      WORD biBitCount; 
      DWORD biCompression; 
      DWORD biSizeImage; 
      LONG biXPelsPerMeter; 
      LONG biYPelsPerMeter; 
      DWORD biClrUsed; 
      DWORD biClrImportant;
   } __attribute__ ((packed)) bmih;
   unsigned long modeLineSize;
   unsigned char *mode_ptr;
   if (!open(file))
      return 0;
   if (! read((unsigned long long)(unsigned int)&bmfh,sizeof(bmfh),GRUB_READ) || bmfh.bfType != 0x4d42)
   {
      close();
      return !printf("Err: fil");
   }
   if (! read((unsigned long long)(unsigned int)&bmih,sizeof(bmih),GRUB_READ))
   {
      close();
      return !printf("Error:read1\n");
   }
   uint16_t*	modeList = (uint16_t*)drvInfo.VideoModePtr;
   uint16_t mode;
   while ((mode = *modeList++) != 0xFFFF)
   {
      if (! GetModeInfo (mode) || 
         modeInfo.XResolution != bmih.biWidth
      || modeInfo.YResolution != bmih.biHeight
      || modeInfo.BitsPerPixel < bmih.biBitCount)
         continue;
      if (! SetMode (mode))
      {
         printf( "VBE mode %4x not supported\n", mode);
      }
      mode_ptr = (unsigned char*)modeInfo.PhysBasePtr;
      modeLineSize = modeInfo.BytesPerScanline;
      break;
   }

   if (mode == 0xffff)
   {
      printf("not supported!");
      close();
      getkey();
      return 0;
   }

   unsigned int LineSize=(bmih.biWidth*(bmih.biBitCount>>3)+3)&~3;
   char *buff=malloc(bmfh.bfSize);
   int x = 0,y = bmih.biHeight-1;
   char *p,*p1;
   unsigned long BPB;
   unsigned long BPL;
   if (!read((unsigned long long)(unsigned int)buff,bmfh.bfSize,GRUB_READ))
   {
      close();
      return 0;
   }
   close();
   p = buff;
   BPB = bmih.biBitCount>>3;
   BPL = BPB * bmih.biWidth;
   vbe.image = malloc(modeInfo.BytesPerScanline*modeInfo.YResolution);
   vbe.BytesToLFB = modeInfo.PhysBasePtr - (unsigned long)vbe.image;
   for(y=bmih.biHeight-1;y>=0;--y)
   {
      p1 = vbe.image + modeInfo.BytesPerScanline * y;
      if (modeInfo.BitsPerPixel == 32 && BPB == 4)
      {
         memmove(p1,p,BPL);
      }
      else
      {
         for(x=0;x<bmih.biWidth;++x)
         {
            p1[0] = p[0];
            p1[1] = p[1];
            p1[2] = p[2];
            p1+=3,p+=3;
            if (modeInfo.BitsPerPixel==32)
            {
               *++p1=0;
            }
         }
      }
      p = buff +(bmih.biHeight-y)*LineSize;
   }
   free(buff);
   vbe.current_mode = mode;
}

