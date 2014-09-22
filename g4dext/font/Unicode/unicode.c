/***********************************************************
描述：	用c语言写的一个如何从unicode编码格式的点阵字库中读取字符信息（像素宽 +点阵信息）
        至于容错性和效率方面还得使用者自行改善。谢谢您的参阅！
作者：  wujianguo
日期： 	20090516
MSN:    wujianguo19@hotmail.com
qq：    9599598
*************************************************************/

#include "../font.h"
#include "unicode.h"


        
extern FILE *g_prf;
extern FL_HEADER _fl_header;      
extern DWORD g_dwCharInfo;   

//int ReadFontSection()
//{
//	PFL_HEADER pfl_header = &_fl_header;

//	ReleaseSection();

//	pfl_header->pSection = (FL_SECTION_INF *)malloc(pfl_header->nSection*sizeof(FL_SECTION_INF));
//	if(pfl_header->pSection == NULL)
//	{
//		printf("Malloc fail!\n");
//		return 0;
//	}
//	pfl_header->pSection = 0x400010;
//	fread(pfl_header->pSection, pfl_header->nSection*sizeof(FL_SECTION_INF), 1, g_prf);	
//	return 1;
//}

//void ReleaseSection()
//{
//	if(_fl_header.pSection != NULL)
//	{
//		free(_fl_header.pSection);
//		_fl_header.pSection = NULL;
//	}
//}

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
	
	offset = _fl_header.pSection[i].OffAddr+ FONT_INDEX_TAB_SIZE*(wCode - _fl_header.pSection[i].First );	
//	fseek(g_prf, offset, SEEK_SET);
//	fread(&g_dwCharInfo, sizeof(DWORD), 1, g_prf);	
	g_dwCharInfo = (*(DWORD *)offset);

	return GET_FONT_WIDTH(g_dwCharInfo);
}





