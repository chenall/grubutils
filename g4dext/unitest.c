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

gcc -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE unitest.c

 * disassemble:			objdump -d a.out 
 * confirm no relocation:	readelf -r a.out
 * generate executable:		objcopy -O binary a.out unitest
 *
 */
/*
 * to get this source code & binary: http://grub4dos-chenall.google.com
 * For more information.Please visit web-site at http://chenall.net
 * 2010-03-05
 */
#include "font/font.h"
#include "grub4dos.h"

static FL_HEADER _fl_header;
static DWORD g_dwCharInfo = 0;    // 存储当前字符的检索信息。  bit0～bit25：存放点阵信息的起始地址。 bit26～bit31：存放像素宽度。
static void DisplayChar(int nYSize, int nBytesPerLine, unsigned char *pFontData);
#define		BASE_FONT_ADDR		0x600000

int GRUB = 0x42555247;/* this is needed, see the following comment. */
/* gcc treat the following as data only if a global initialization like the
 * above line occurs.
 */
asm(".long 0x534F4434");
asm(ASM_BUILD_DATE);
/* a valid executable file for grub4dos must end with these 8 bytes */
asm(".long 0x03051805");
asm(".long 0xBCBAA7BA");

/* thank goodness gcc will place the above 8 bytes at the end of the b.out
 * file. Do not insert any other asm lines here.
 */

int
main ()
{
	void *p = &main;
	char *arg = p - (*(int *)(p - 8));
	int flags = (*(int *)(p - 12));
		if (strlen(arg) <= 0)
	{
		return 1;
	}

	while(arg[0]==' ' || arg[0]=='\t') arg++;

	if (strlen(arg) > 256)
	return !(errnum = ERR_WONT_FIT);

	if (open(arg))
	{
		read ((unsigned long long)(unsigned int)(char *) RAW_ADDR (BASE_FONT_ADDR), -1, 0xedde0d90);
		close();

		if (InitFont())
		{
				unsigned long long ch;
				arg=skip_to(0,arg);
				if (! safe_parse_maxint(&arg,&ch))
					return 0;
				ch &= 0xffff;
			if (ch)
			{
				ReadCharDistX(ch);
				int dwOffset;
				WORD bytesPerLine = 0;
				dwOffset = ReadCharDotArray(ch, &bytesPerLine);
				DisplayChar(_fl_header.YSize, bytesPerLine, (char *)BASE_FONT_ADDR + dwOffset);
				return 0;
			}
		}
		return 1;
	}
	else
	{
		printf ("load fontfile failed!\n");
		return 0;
	}
}

static int ReadFontHeader(PFL_HEADER pfl_header)
{	
	memmove(pfl_header,(char *)BASE_FONT_ADDR,sizeof(FL_HEADER));
	//检测表示头
	if(_fl_header.magic[0] != 'U' && _fl_header.magic[1] != 'F' && _fl_header.magic[2] != 'L')
	{
		printf("Cann't support file format!\n");
		return 0;
	}

	_fl_header.pSection = (FL_SECTION_INF *)(BASE_FONT_ADDR + 0x10 );

	return 1;	
}

/***************************************************************
功能： 初始化字体。 即打开字体文件，且读取信息头。
参数： pFontFile--字库文件名      
***************************************************************/
int InitFont()
{
	memset(&_fl_header , 0, sizeof(FL_HEADER));

	return ReadFontHeader(&_fl_header);	
}

/********************************************************************
功能： 获取当前字符的像素宽度, 且将索引信息存入一个全局变量：g_dwCharInfo。
        根据索引信息，即同时能获取当前字符的点阵信息的起始地址。
参数： wCode -- 当字库为unicode编码格式时，则将wCode当unicode编码处理。
                否则反之（MBCS)。
********************************************************************/
int ReadCharDistX(WORD wCode)
{
	if('U' == _fl_header.magic[0])     //unicode 编码
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

	if(g_dwCharInfo > 0)
	{		
		DWORD nDataLen = *bytesPerLine * _fl_header.YSize;			
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
	
	for(i = 0;i<_fl_header.nSection;i++)
	{
		if(wCode >= _fl_header.pSection[i].First && wCode <= _fl_header.pSection[i].Last)
			break;		
	}
	if(i >= _fl_header.nSection)	
		return 0;	

	offset = BASE_FONT_ADDR + _fl_header.pSection[i].OffAddr+ FONT_INDEX_TAB_SIZE*(wCode - _fl_header.pSection[i].First );
	g_dwCharInfo = (*(DWORD *)offset);

	return GET_FONT_WIDTH(g_dwCharInfo);
}

/*****************************************************************
功能：根据点阵信息显示一个字模
参数：
	  nYSize--字符的高度
	  nBytesPerLine--每行占的字节数（公式为：（像素宽＋７）／８＝？）
      pFontData--字符的点阵信息。
*****************************************************************/
static void DisplayChar(int nYSize, int nBytesPerLine, unsigned char *pFontData)
{
	int i, j;
	printf("bytesPerLine = %d\n", nBytesPerLine);

	for(i = 0; i < nYSize; i++)	
	{
		for(j = 0; j < nBytesPerLine; j++)
		{
			int x=0;
			for(x=0;x<8;x++)
				putchar( (pFontData[i * nBytesPerLine + j] & (1 << 7 - x))?'*':' ');
		}
		printf("\n");
	}
}

