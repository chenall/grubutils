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
 * gcc -Wl,--build-id=none -m64 -mno-sse -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE ProgressBar.c -o ProgressBar.o 2>&1|tee build.log
 * objcopy -O binary ProgressBar.o ProgressBar
 *
 * analysis:
 * objdump -d ProgressBar.o > ProgressBar.s
 * readelf -a ProgressBar.o > a.s
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
	unsigned int Inc_or_dec;
	unsigned int pixel;
	unsigned int delay;
	unsigned int delay0;
	unsigned int size;
	unsigned int once;
	unsigned int no_box;
} __attribute__ ((packed)) user_data;	//用户数据

static user_data data;	//数据保留区
static grub_size_t g4e_data = 0;
static void get_G4E_image(void);

/* this is needed, see the comment in grubprog.h 		这是必需的，请参阅grubprog.h中的注释 */
#include "grubprog.h"
/* Do not insert any other asm lines here. 请勿在此处插入任何其他asm行 */
//不能包含子程序，不能包含全局变量，否则编译可以通过，但是不能正确执行。

static int main(char *arg, int flags);
static int main(char *arg, int flags)
{
	user_data *gd;
	unsigned int len;
	unsigned long long col;

  get_G4E_image();
  if (! g4e_data)
    return 0;

  if (*(unsigned int *)IMG(0x8278) < 20230613)
  {
    printf("Please use grub4efi version above 2023-06-13.\n");
    return 0;
  }

	if (flags == 0)	//首次加载时，返回驻留内存用户数据地址
		return (grub_size_t)&data;

	if (flags == -1)	//定时器呼叫  处理定时到期指定内容
	{
		gd = &data;	//数据挂钩
		if (grub_timeout == -1)	//如果菜单关闭倒计时
		{
			if (!(cursor_state & 1))
			{
				col = current_color_64bit;
				if (current_term->SETCOLORSTATE)
					current_term->SETCOLORSTATE (COLOR_STATE_NORMAL);
				current_color_64bit >>= 32;
				rectangle(gd->left, gd->top, gd->length, 0, gd->width | 0x80000000);	//清除进度条  
				current_color_64bit = col;
			}
			if (timer)
			{
				free ((void *)timer);
				timer = 0;		//定时器停止
			}			
			return 1;
		}		
		
		gd->delay0++;	//延时计数(毫秒)
		if (gd->delay > gd->delay0)	//如果延时未到
			return 1;	//返回
		gd->delay0 = 0;	//延时计数归零
		col = current_color_64bit;
		current_color_64bit = gd->color;

		if (!gd->once)
		{
			if(!gd->no_box && !gd->Inc_or_dec)
				rectangle(gd->left, gd->top, gd->length, gd->width, 1); //画矩形
			else if (gd->Inc_or_dec)
				rectangle(gd->left, gd->top, gd->length, 0, gd->width);
			gd->once = 1;
		}

		if (gd->Inc_or_dec)
		{
			if (current_term->SETCOLORSTATE)
				current_term->SETCOLORSTATE (COLOR_STATE_NORMAL);
			current_color_64bit >>= 32;
		}

		if (gd->type == 1)				//从左到右
		{
			if (gd->size > gd->left + gd->length)	//如果进度线画满，结束
				goto exit;
			else if (gd->left + gd->length - gd->size >= gd->pixel)
				len = gd->pixel;
			else
				len = gd->left + gd->length - gd->size;

			rectangle(gd->size, gd->top, 0, gd->width, (gd->Inc_or_dec ? (len | 0x80000000) : len));	//画进度线
			gd->size += gd->pixel;	//下一位置
		}
		else if (gd->type == 2)		//从右到左
		{
			if (gd->size <= (gd->left - gd->pixel))
				goto exit;
			else if (gd->size >= gd->left)
				len = gd->size;
			else
				len = gd->left;

			rectangle(len, gd->top, 0, gd->width, (gd->Inc_or_dec ? (gd->pixel | 0x80000000) : gd->pixel));
			gd->size -= gd->pixel;										//2ba-1
		}
		else if (gd->type == 3)		//从上到下
		{
			if (gd->size > gd->top + gd->width)	
				goto exit;
			else if (gd->top + gd->width - gd->size >= gd->pixel)
				len = gd->pixel;
			else
				len = gd->top + gd->width - gd->size;

			rectangle(gd->left, gd->size, gd->length, 0, (gd->Inc_or_dec ? (len | 0x80000000) : len));
			gd->size += gd->pixel;
		}
		else if (gd->type == 4) 	//从下到上
		{
			if (gd->size <= gd->top - gd->pixel)
				goto exit;
			else if (gd->size >= gd->top)
				len = gd->size;
			else
				len = gd->top;

			rectangle(gd->left, len, gd->length, 0, (gd->Inc_or_dec ? (gd->pixel | 0x80000000) : gd->pixel));
			gd->size -= gd->pixel;
		}

		current_color_64bit = col;
		return 1;

//进度线画满，结束
exit:
		grub_timeout = 0;	//通知菜单中的延时到
		current_color_64bit = col;
		return 1;
	}

  if ((flags & BUILTIN_CMDLINE) && (!arg || !*arg)) //帮助信息
  {
    printf("ProgressBar for grub4efi by yaya,%s\n",__DATE__);
    printf("Usage:\n\tProgressBar [--no-box] left top length widthl color type\n");
    printf("      \tcolor: use 32-bit color\n");
    printf("      \ttype(0-3): 1. left to right; 2. right to left; 3. top to bottom; 4. bottom to top\n");
    printf("      \ttype(4-7): 0. increase progressively; 1. decrease progressively\n");
    printf("      \tother units are pixels\n");
    printf("      \t--no-box don't show box\n");
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
		timer = (grub_size_t)malloc (buff_len);			//分配内存，将驻留内存地址赋给定时器指针
		if (!timer)
      return 0;
		memmove((void*)timer,(void*)main,buff_len);	//将本程序内容移动到驻留内存
		char* arg_bak = arg;	//避免损害参数
    gd = (user_data *)((grub_size_t(*)(char*,grub_size_t))timer)(0,0);//获取驻留内存用户数据位置
		arg = arg_bak;
		//此时仍然在临时内存，但是可以对驻留内存用户数据进行初始化操作。
	}

	if (arg)  //处理参数		填充驻留内存用户数据
	{
		unsigned long long val;

		for (;;)
		{
			if (memcmp (arg, "--no-box", 8) == 0)
				gd->no_box = 1;
			else
				break;
			arg=skip_to (0, arg);
		}

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
		{
			gd->type = val & 0xf;											//类型  1/2/3/4=水平从左到右/水平从右到左/垂直从上到下/垂直从下到上
			gd->Inc_or_dec = (val & 0xf0) >> 4;				//增减  0/1=递增/递减
		}
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
		gd->delay0 = 0;		//延时计数归零

		//确定进度线起始位置
		if (gd->type == 1)				//从左到右
			gd->size = gd->left;
		else if (gd->type == 2)		//从右到左
			gd->size = gd->left + gd->length - gd->pixel;
		else if (gd->type == 3)		//从上到下
			gd->size = gd->top;
		else if (gd->type == 4) 	//从下到上
			gd->size = gd->top + gd->width - gd->pixel;

		gd->once = 0;
	}

	return 1;
}


static void get_G4E_image(void)
{
  grub_size_t i;

  //在内存0-0x9ffff, 搜索特定字符串"GRUB4EFI"，获得GRUB_IMGE
  for (i = 0x40100; i <= 0x9f100 ; i += 0x1000)
  {
    if (*(unsigned long long *)i == 0x4946453442555247)	//比较数据
    {
      g4e_data = *(grub_size_t *)(i+16); //GRUB4DOS_for_UEFI入口
      return;
    }
  }
  return;
}
