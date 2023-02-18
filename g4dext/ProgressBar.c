/*
 *  HOTKEY  --  hotkey functionality for use with grub4dos menu.
 *  Copyright (C) 2015  chenall(chenall.cn@gmail.com)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * compile:
 *
 * gcc -Wl,--build-id=none -m32 -mno-sse -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE ProgressBar.c -o ProgressBar.o
 *
 * disassemble:                 objdump -d ProgressBar.o 
 * confirm no relocation:       readelf -r ProgressBar.o
 * generate executable:         objcopy -O binary ProgressBar.o ProgressBar
 *
 */

//这是使用定时器的一个实例，也是模板。
#include "grub4dos.h"
#define DEBUG 0

//在此定义静态变量、结构。不能包含全局变量，否则编译可以通过，但是不能正确执行。
//不能在main前定义子程序，在main后可以定义子程序。
#define BUILTIN_CMDLINE		0x1	/* Run in the command-line.  	运行在命令行=1	*/
#define BUILTIN_MENU			(1 << 1)/* Run in the menu.  			运行在菜单=2		*/

typedef struct
{
	unsigned int left;
	unsigned int top;
	unsigned int length;
	unsigned int width;
	unsigned long long color;
	unsigned int type;
	unsigned int pixel;
	unsigned int delay;
	unsigned int delay0;
	unsigned int size;
	unsigned int once;
} __attribute__ ((packed)) user_data;	//用户数据

static user_data data;	//数据保留区

/* this is needed, see the comment in grubprog.h 		这是必需的，请参阅grubprog.h中的注释 */
#include "grubprog.h"
/* Do not insert any other asm lines here. 请勿在此处插入任何其他asm行 */
//不能包含子程序，不能包含全局变量，否则编译可以通过，但是不能正确执行。

static int main(char *arg, int flags);
static int main(char *arg, int flags)
{
	user_data *gd;
	int len;
	unsigned int colF, colB;
	unsigned long long col;

	if (flags == timer)	//首次加载时，返回驻留内存用户数据地址
		return (int)&data;

	if (flags == 0xffffffff)	//定时器呼叫  处理定时到期指定内容
	{
		gd = &data;	//数据挂钩
		if (grub_timeout == -1)	//如果菜单关闭倒计时
		{
			colF = foreground;
			colB = background;
			if (current_term->SETCOLORSTATE)
				current_term->SETCOLORSTATE (COLOR_STATE_NORMAL);
			foreground = background;
			rectangle(gd->left, gd->top, gd->length, 0, gd->width | 0x80000000);	//清除进度条
			foreground = colF;
			background = colB;
			timer = 0;		//定时器停止			
			return 1;
		}
		
		gd->delay0++;	//延时计数(毫秒)
		if (gd->delay > gd->delay0)	//如果延时未到
			return 1;	//返回
		gd->delay0 = 0;	//延时计数归零
//		colF = foreground;
//		foreground = gd->color;
		col = current_color_64bit;
		current_color_64bit = gd->color;
		if (!gd->once)
		{
			rectangle(gd->left, gd->top, gd->length, gd->width, 1); //画矩形
			gd->once = 1;
		}

		if (gd->type == 1)				//从左到右
		{
			rectangle(gd->size, gd->top, 0, gd->width, gd->pixel);	//画进度线
			gd->size += gd->pixel;	//下一位置
			if (gd->size >= gd->left + gd->length)	//如果进度线画满，结束
				goto exit;
		}
		else if (gd->type == 2)		//从右到左
		{
			rectangle(gd->size, gd->top, 0, gd->width, gd->pixel);
			gd->size -= gd->pixel;
			if (gd->size <= gd->left)
				goto exit;
		}
		else if (gd->type == 3)		//从上到下
		{
			rectangle(gd->left, gd->size, gd->length, 0, gd->pixel);
			gd->size += gd->pixel;
			if (gd->size >= gd->top + gd->width)
				goto exit;
		}
		else if (gd->type == 4) 	//从下到上
		{
			rectangle(gd->left, gd->size, gd->length, 0, gd->pixel);
			gd->size -= gd->pixel;
			if (gd->size <= gd->top)
				goto exit;
		}

//		foreground = colF;
		current_color_64bit = col;
		return 1;

//进度线画满，结束
exit:
		timer = 0;		//定时器停止
		grub_timeout = 0;	//通知菜单中的延时到
//		foreground = colF;
		current_color_64bit = col;
		return 1;
	}

  if ((flags & BUILTIN_CMDLINE) && (!arg || !*arg)) //帮助信息
  {
    printf("ProgressBar for grub4efi by yaya,%s\n",__DATE__);
    printf("Usage:\n\tProgressBar left top length widthl color type\n");
    printf("      \tcolor: use 32-bit color\n");
    printf("      \ttype: 1. left to right; 2. right to left; 3. top to bottom; 4. bottom to top\n");
    printf("      \tother units are pixels\n");
    return 1;
  }

	if (!(flags & BUILTIN_MENU))	//不允许在命令行界面操作
		return 1;

	if (!timer) //首次加载程序，初始化
	{
		int buff_len = *(unsigned int *)(&main + filemax - 0x24) - *(unsigned int *)(&main + filemax - 0x3c);	//本程序尺寸
		if (buff_len > 0x4000)//文件太大加载失败。限制程序不可以超过16KB。
			return 0;

		//本程序由command临时加载, 执行完毕就释放了，因此需要为程序单独分配驻留内存，。。
		char *p = malloc (buff_len + 4095);	//分配内存
    if (!p)
      return 0;
		p = (char *)((int)(p + 4095) & ~4095);				//4k对齐
		timer = (int)p;								//将驻留内存地址赋给定时器指针
		memmove((void*)timer,&main,buff_len);	//将本程序内容移动到驻留内存

		char* arg_bak = arg;	//避免损害参数
    gd = (user_data *)((int(*)(char*,int))timer)(0,timer);//获取驻留内存用户数据位置
		arg = arg_bak;
		//此时仍然在临时内存，但是可以对驻留内存用户数据进行初始化操作。
	}

	if (arg)  //处理参数		填充驻留内存用户数据
	{
		unsigned long long val;
		if (safe_parse_maxint (&arg, &val))
			gd->left = val;														//左上角x
		arg = skip_to (0, arg);
		if (safe_parse_maxint (&arg, &val))
			gd->top = val;														//左上角y
		arg = skip_to (0, arg);
		if (safe_parse_maxint (&arg, &val))
			gd->length = val;													//水平长度
		arg = skip_to (0, arg);
		if (safe_parse_maxint (&arg, &val))
			gd->width = val;													//垂直长度
		arg = skip_to (0, arg);
		if (safe_parse_maxint (&arg, &val))
			gd->color = val;													//32位颜色
		arg = skip_to (0, arg);
		if (safe_parse_maxint (&arg, &val))
			gd->type = val;														//类型  1/2/3/4=水平从左到右/水平从右到左/垂直从上到下/垂直从下到上
		arg = skip_to (0, arg);

		//计算进度条尺寸
		if (gd->type == 1 || gd->type == 2)
			len = gd->length;
		else
			len = gd->width;
		//计算进度条延迟和进度条像素
		for (gd->pixel = 1; ; gd->pixel++)
		{
			if (gd->delay = (grub_timeout * 1000) / (len * gd->pixel))
				break;
		}
		grub_timeout = 200;		//调大延时，避免干扰本程序
		gd->delay0 = 0;		//延时计数归零

		//确定进度线起始位置
		if (gd->type == 1)				//从左到右
			gd->size = gd->left;
		else if (gd->type == 2)		//从右到左
			gd->size = gd->left + gd->length - gd->pixel - 1;
		else if (gd->type == 3)		//从上到下
			gd->size = gd->top;
		else if (gd->type == 4) 	//从下到上
			gd->size = gd->top + gd->width - gd->pixel - 1;

		gd->pixel <<= 1;	//程序执行有延时，这里进行弥补，但是。不精确。
		run_line ("setmenu --timeout=0=100=0",1);	//屏蔽菜单倒计时显示
		gd->once = 0;
	}

	return 1;
}

