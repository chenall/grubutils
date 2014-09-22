/***********************************************************
描述：	用c语言写的一个如何从MBCS编码格式的点阵字库中读取字符信息（像素宽 +点阵信息）
        至于容错性和效率方面还得使用者自行改善。谢谢您的参阅！
作者：  wujianguo
日期： 	20090516
MSN:    wujianguo19@hotmail.com
qq：    9599598
*************************************************************/
#include "../font.h"
#include "mbcs.h"

        
//extern FILE *g_prf;
extern FL_HEADER _fl_header;      
extern DWORD g_dwCharInfo;  

#define		IS_CJK(codepage)		(codepage & 0xf)   //判断是否为中日韩字符集。如是则默认为字符等宽。
#define		SET_BIT(n)				(1<<n)

enum{
	CP932 = SET_BIT(0),
	CP936 = SET_BIT(1),
	CP949 = SET_BIT(2),
	CP950 = SET_BIT(3),
	CP874 = SET_BIT(4),
	CP1250 = SET_BIT(5),
	CP1251 = SET_BIT(6),
	CP1252 = SET_BIT(7),
	CP1253 = SET_BIT(8),
	CP1254 = SET_BIT(9),
	CP1255 = SET_BIT(10),
	CP1256 = SET_BIT(11),
	CP1257 = SET_BIT(12),
	CP1258 = SET_BIT(13),
};

static long GetPosWithMbcs(WORD wCode, int nCodepage)
{
	long lIdx = -1;

	BYTE R = (wCode >> 8) & 0xFF;   //区码
	BYTE C = wCode & 0xFF;   //位码

	switch(nCodepage)
	{
	case CP932:
		if(R >= 0x81 && R <= 0x9F)
		{
			if(C >= 0x40 && C <= 0x7E)
				lIdx = (R-0x81)*188 + (C-0x40);  //188 = (0x7E-0x40+1)+(0xFC-0x80+1); 			
			else if(C >= 0x80 && C <= 0xFC)
				lIdx = (R-0x81)*188 + (C-0x80)+63;  // 63 = 0x7E-0x40+1;			
		}
		else if(R >= 0xE0 && R <= 0xFC)
		{
			if(C >= 0x40 && C <= 0x7E)
				lIdx = 5828 + (R-0xE0)*188 + (C-0x40);  // 5828 = 188 * (0x9F-0x81+1);
			else if(C >= 0x80 && C <= 0xFC)
				lIdx = 5828 + (R-0xE0)*188 + (C-0x80)+63;
		}
		break;

	case CP936:
		if((R >= 0xA1 && R <= 0xFE) && (C >= 0xA1 && C <= 0xFE))
			lIdx = (R-0xa1)*94 + (C-0xa1);  //94 = (0xFE-0xA1+1); 
		break;

	case CP949:
		if(R >= 0x81)
		{
			if(C >= 0x41 && C <= 0x7E)
				lIdx = ((R-0x81) * 188 + (C - 0x41));   // 188 = (0x7E-0x41+1)+(0xFE-0x81+1);
			else if(C >= 0x81 && C <= 0xFE)
				lIdx = ((R-0x81) * 188 + (C - 0x81) + 62);  // 62 = (0x7E-0x41+1);
		}
		break;

	case CP950:
		if(R >= 0xA1 && R <= 0xFE) 
		{
			if(C >= 0x40 && C <= 0x7E)
				lIdx = ((R-0xa1)*157+(C-0x40));   // 157 = (0x7E-0x40+1)+(0xFE-0xA1+1);
			else if(C >= 0xA1 && C <= 0xFE)
				lIdx = ((R-0xa1)*157+(C-0xa1)+63);  // 63 = (0x7E-0x40+1);
		}
		break;
	default:
		break;
	}
	return lIdx;
}

// 获取字符的像素宽度
DWORD ReadCharDistX_M(WORD wCode)
{
	PFL_HEADER pfl_header = &_fl_header;
	DWORD offset = 0;
	int   i = 0;
	g_dwCharInfo = 0;		

	if(!pfl_header->nSection)  // CJK (等宽字符). 文件格式：文件头+点阵数据。
	{
		long lIdx = GetPosWithMbcs(wCode, pfl_header->wCpFlag);
		if(lIdx != -1) 
		{
			g_dwCharInfo = sizeof(FL_HEADER) - sizeof(pfl_header->pSection) + lIdx * ((pfl_header->YSize + 7)/PIXELS_PER_BYTE * pfl_header->YSize);
			g_dwCharInfo |= (pfl_header->YSize << 26);
			return pfl_header->YSize;
		}
	}
	else   // 拉丁文 (非等宽字符).   文件格式： 文件头+索引表+点阵数据。
	{	
		if(wCode <= 0xff)
		{		
			offset = sizeof(FL_HEADER) - sizeof(pfl_header->pSection) + wCode * FONT_INDEX_TAB_SIZE;
			fseek(g_prf, offset, SEEK_SET);
			fread(&g_dwCharInfo, sizeof(DWORD), 1, g_prf);
			return GET_FONT_WIDTH(g_dwCharInfo);
		}
	}
	return 0;
}
