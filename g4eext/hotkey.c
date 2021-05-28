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
 * gcc -Wl,--build-id=none -m64 -mno-sse -nostdlib -Wno-int-to-pointer-cast -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE hotkey.c -o hotkey.o 2>&1|tee build.log
 * gcc -Wl,--build-id=none -m64 -mno-sse -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE hotkey.c -o hotkey.o 2>&1|tee build.log
 * disassemble:                 objdump -d hotkey.o > hotkey.s
 * confirm no relocation:       readelf -r hotkey.o > r.s
 *                              readelf -a hotkey.o > a.s
 * generate executable:         objcopy -O binary hotkey.o hotkey
 *
 */
//extern int (*hotkey_func)(char *titles,int flags,int flags1);		标题,标记,标记1=4B400000 | 第一入口*100 | 项目号
//hotkey_func(0,0,-1);
//hotkey_func(0,-1,(0x4B40<<16)|(first_entry << 8) | entryno);


#include "grub4dos.h"
#define DEBUG 0
//#include "grubprog.h"

//在此定义静态变量、结构。不能包含全局变量，否则编译可以通过，但是不能正确执行。
//不能在main前定义子程序，在main后可以定义子程序。
typedef struct
{
  unsigned short code;		//键代码
  char name[10];					//键名称
} __attribute__ ((packed)) key_tab_t;	//键盘表

typedef struct 
{
	int key_code;             //键代码
	unsigned short title_num;	//标题号
} __attribute__ ((packed)) hotkey_t;	//热键

typedef struct
{
    int key_code;	//键代码
    char *cmd;		//命令
} __attribute__ ((packed)) hotkey_c;	//热键字符

union
{
	int flags;
	struct
	{
		unsigned char sel;		//选择 
		unsigned char first;	//首先
		unsigned short flag;	//标记
	} k;
} __attribute__ ((packed)) cur_menu_flag;	//当前菜单标记

typedef struct
{
    int cmd_pos;					//命令位置
    hotkey_c hk_cmd[64];	//热键字符数组
    char cmd[4096];				//命令
} __attribute__ ((packed)) hkey_data_t;	//热键数据

#define HOTKEY_MAGIC 0X79654B48						//魔术
#define HOTKEY_FLAGS_AUTO_HOTKEY (1<<12)	//热键标志	自动热键		1000
#define HOTKEY_FLAGS_AUTO_HOTKEY1 (1<<11)	//热键标志	自动热键1		800
#define HOTKEY_FLAGS_NOT_CONTROL (1<<13)	//热键标志	不控制			2000    -nc
#define HOTKEY_FLAGS_NOT_BOOT	 (1<<14)		//热键标志	不启动			4000    -nb
#define BUILTIN_CMDLINE		0x1	/* Run in the command-line.  	运行在命令行=1	*/
#define BUILTIN_MENU			(1 << 1)/* Run in the menu.  			运行在菜单=2		*/

static key_tab_t key_table[] = {						//键盘表
  {0x0031, "1"},
  {0x0032, "2"},
  {0x0033, "3"},
  {0x0034, "4"},
  {0x0035, "5"},
  {0x0036, "6"},
  {0x0037, "7"},
  {0x0038, "8"},
  {0x0039, "9"},
  {0x0030, "0"},
  {0x002d, "-"},
  {0x003d, "="},
  {0x0071, "q"},
  {0x0077, "w"},
  {0x0065, "e"},
  {0x0072, "r"},
  {0x0074, "t"},
  {0x0079, "y"},
  {0x0075, "u"},
  {0x0069, "i"},
  {0x006f, "o"},
  {0x0070, "p"},
  {0x005b, "["},
  {0x005d, "]"},
  {0x0061, "a"},
  {0x0073, "s"},
  {0x0064, "d"},
  {0x0066, "f"},
  {0x0067, "g"},
  {0x0068, "h"},
  {0x006a, "j"},
  {0x006b, "k"},
  {0x006c, "l"},
  {0x003b, ";"},
  {0x0027, "'"},
  {0x0060, "`"},
  {0x005c, "\\"},
  {0x007a, "z"},
  {0x0078, "x"},
  {0x0063, "c"},
  {0x0076, "v"},
  {0x0062, "b"},
  {0x006e, "n"},
  {0x006d, "m"},
  {0x002c, ","},
  {0x002e, "."},
  {0x002f, "/"},
  {0x3b00, "f1"},
  {0x3c00, "f2"},
  {0x3d00, "f3"},
  {0x3e00, "f4"},
  {0x3f00, "f5"},
  {0x4000, "f6"},
  {0x4100, "f7"},
  {0x4200, "f8"},
  {0x4300, "f9"},
  {0x4400, "f10"},
  {0x5200, "ins"},
  {0x5300, "del"},
  {0x8500, "f11"},
  {0x8600, "f12"},
  {0x0000, 0}
};

static unsigned short allow_key[9] = {	//方向键
/*KEY_ENTER       */0x000D,
/*KEY_HOME        */0x4700,
/*KEY_UP          */0x4800,
/*KEY_PPAGE       */0x4900,
/*KEY_LEFT        */0x4B00,
/*KEY_RIGHT       */0x4D00,
/*KEY_END         */0x4F00,
/*KEY_DOWN        */0x5000,
/*KEY_NPAGE       */0x5100
};
static int my_app_id = 0;
static int _checkkey_ = 0;
static char keyname_buf[16];
static int get_keycode (char *key, int flags);
static int check_hotkey(char **title,int flags);
static char *get_keyname (int code);
static int check_allow_key(unsigned short key);
/* gcc treat the following as data only if a global initialization like the		gcc仅在发生与上述行类似的全局初始化时才将以下内容视为数据。
 * above line occurs.
 */
static hkey_data_t hotkey_data = {0};	//HOTKEY 数据保留区
static int hotkey_cmd_flag = HOTKEY_MAGIC;			//程序尾部标志
static char *HOTKEY_PROG_MEMORY;
static grub_size_t g4e_data = 0;
static void get_G4E_image(void);


/* this is needed, see the comment in grubprog.h 		这是必需的，请参阅grubprog.h中的注释 */
#include "grubprog.h"
/* Do not insert any other asm lines here. 请勿在此处插入任何其他asm行 */
//不能包含子程序，不能包含全局变量，否则编译可以通过，但是不能正确执行。

//与使用于grub4dos不同，按键值由形参带入，内部不需要获得键。
//1. 由菜单或命令行启用热键，并获得参数。此时：arg一般非0，flags=102，flags1及key任意。如：hotkey -A [F4] "commandlien" [F3] "reboot"
//2. 菜单初始化时搜索菜单项中的热键。此时：arg=0，flags=0，flags1=ffffffff,key=0。如：title 运行 ^KonBoot 2.5  如：title [R]重启
//3. 处理按键。此时：arg=0，flags=ffffffff，flags1=4b400000，key=键值。
//返回: -1=刷新;		2字节=热键项目号;		3字节&40=0/1=启动/检查更新
grub_size_t main(char *arg,int flags,int flags1,int key);
grub_size_t main(char *arg,int flags,int flags1,int key)
{
  get_G4E_image();
  if (! g4e_data)
    return 0;
  
  int i;
	char *base_addr;
	static hotkey_t *hotkey;
	unsigned short hotkey_flags;
	unsigned short *p_hotkey_flags;
	base_addr = (char *)(menu_mem + 512); //eb98980   menu_mem=eb98780
	hotkey = (hotkey_t*)base_addr;
	p_hotkey_flags = (unsigned short*)(base_addr + 504);  //eb98d7c
	cur_menu_flag.flags = flags1;

	if (flags == HOTKEY_MAGIC)	//首次加载热键时，以及首次处理热键参数时被调用
	{
		if (arg && *(int *)arg == 0x54494E49)	//INIT 初始化数据  首次加载热键时被调用
		{
			hotkey_data.hk_cmd[0].cmd = hotkey_data.cmd;
			hotkey_data.cmd_pos = 0;
		}
		return (grub_size_t)&hotkey_data;//返回热键数据地址
	}

	if (flags == -1)	//处理按键
	{
		int c;
		hotkey_c *hkc = hotkey_data.hk_cmd;

		if (my_app_id != HOTKEY_MAGIC/* || (!hotkey->key_code && !hkc->key_code)*/)
			return key;

		hotkey_flags = *p_hotkey_flags;
    c = key & 0xf00ffff;
		if (!c || check_allow_key(c))	//检测当前的按键码是否方向键或回车键, 是则返回
			return c;

		for(i=0;i<64;++i)
		{
			if (!hkc[i].key_code)
				break;

			if (hkc[i].key_code == c)
			{
				grub_error_t err_old = errnum;
				if (hkc[i].cmd[0] == '@')		//静默方式运行(没有任何显示)
			    builtin_cmd(NULL,hkc[i].cmd + 1,BUILTIN_CMDLINE);
				else
				{
					putchar_hooked = 0;
					setcursor (1); /* show cursor and disable splashimage */
					if (current_term->SETCOLORSTATE)
				    current_term->SETCOLORSTATE(COLOR_STATE_STANDARD);
					cls();
					if (debug > 0)
				    printf(" Hotkey Boot: %s\n",hkc[i].cmd);
			    builtin_cmd(NULL,hkc[i].cmd,BUILTIN_CMDLINE);
				}
				if (putchar_hooked == 0 && errnum > ERR_NONE && errnum < MAX_ERR_NUM)
				{
			    printf("\nError %u\n",errnum);
			    getkey();
				}
				errnum = err_old;
				return -1;
			}
		}

		for (;hotkey->key_code;++hotkey)
		{
			if (hotkey->key_code == c)
				return (hotkey_flags|hotkey->title_num)<<16;
		}

		if ((hotkey_flags & HOTKEY_FLAGS_AUTO_HOTKEY) && (char)c > 0x20)
		{
			char h = tolower(c&0xff);	//小写
			char *old_c_hotkey = (char*)(p_hotkey_flags+1);
			char *old_t = old_c_hotkey + 1;
			int find = -1,n = *old_t;
			int cur_sel = -1;
			char **titles;

			if (c >= 'A' && c <= 'Z')	//按下Shift键
				return h;
			if (c & 0xf000000)	//如果按下Shift或Ctrl或Alt键
				goto chk_control;
			if (cur_menu_flag.k.flag == 0x4B40)
				cur_sel = cur_menu_flag.k.sel + cur_menu_flag.k.first;
			titles = (char **)(base_addr + 512);

			if (*old_c_hotkey != h)
			{//不同按键清除记录
        *old_c_hotkey = h;
				n = *old_t = 0;
			}

			for(i=0;i<256;++i)
			{
				if (!*titles)
					break;
				if (check_hotkey(titles,h))	//从菜单标题中提取热键代码
				{//第几次按键跳到第几个匹配的菜单(无匹配转第一个)
					if (cur_sel != -1)
					{
						if (cur_sel < i)
						{
							find = i;
							break;
						}
					}
					else if (n-- == 0)
					{
						find = i;
						break;
					}
					if (find == -1) find = i;
				}
				++titles;
			}
			if (find != -1)
			{
				if (find != i) *old_t = 1;
				else (*old_t)++;
				return (hotkey_flags|HOTKEY_FLAGS_NOT_BOOT|find)<<16;
			}
			if (h > '9')
				return 0;
		}
		chk_control:
		if ((hotkey_flags & HOTKEY_FLAGS_NOT_CONTROL) || (c & 0xf000000))	//不控制, 或者按了控制键/上档键/替换键又没有查到
			return 0;
		return c;
	}
	else if (flags == 0)  //搜索菜单项中的热键
	{
		char **titles;
		titles = (char **)(base_addr + 512);  //菜单项地址
		for(i=0;i<126;++i)
		{
			if (!*titles) //菜单项结束
				break;
			if (hotkey->key_code = check_hotkey(titles,0))  //搜索热键
			{
				hotkey->title_num = i;  //热键对应的菜单项号
				++hotkey;
			}
			++titles; //下一菜单项
		}
		hotkey->key_code = 0;
		return 1;
	}

	if (debug > 0)
    printf("Hotkey for grub4efi by chenall (renew by yaya),%s\n",__DATE__);

  if ((flags & BUILTIN_CMDLINE) && (!arg || !*arg)) //帮助信息
  {
    printf("Usage:\n\thotkey -nb\tonly selected menu when press menu hotkey\n\thotkey -nc\tdisable control key\n");
    printf("      \thotkey -A\tselect the menu item with the first letter of the menu\n");
    printf("      \thotkey [HOTKEY] \"COMMAND\"\tregister new hotkey\n\thotkey [HOTKEY]\tDisable Registered hotkey HOTKEY\n");
    printf("      \te.g.\thotkey -A [F3] \"reboot\" [Ctrl+d] \"commandlien\"\n");
    printf("      \te.g.\ttitle [F4] Boot Win 8\n");
    printf("      \te.g.\ttitle Boot ^Win 10\n\n");
    printf("      \tsetmenu --hotkey=[COLOR]\tset hotkey color.\n");
    printf("      \tCommand keys such as p, b, c and e will only work if SHIFT is pressed when hotkey -A\n");
  }
	hotkey_flags = 1<<15;
	while (*arg == '-') //处理热键参数中的'-'号.  如: hotkey -A, -u, -nb, -nc
	{
		++arg;
		if (*(unsigned short*)arg == 0x626E) //nb not boot
			hotkey_flags |= HOTKEY_FLAGS_NOT_BOOT;
		else if (*(unsigned short*)arg == 0x636E) //nc not control
			hotkey_flags |= HOTKEY_FLAGS_NOT_CONTROL;
		else if (*arg == 'A')
		{
			hotkey_flags |= HOTKEY_FLAGS_AUTO_HOTKEY;
		}
		else if (*arg == 'u')
		{
			HOTKEY_FUNC = 0;
			return builtin_cmd("delmod","hotkey",flags);
		}
		arg = wee_skip_to(arg,0);
	}

	if (!HOTKEY_FUNC) //首次加载热键，初始化
	{
		int buff_len = 0x4000;
		*p_hotkey_flags = hotkey_flags;
		//HOTKEY程序驻留内存，直接复制自身代码到指定位置。
		//本程序由command加载, 执行完毕就释放了，因此需要为热键单独分配内存。
		//开启HOTKEY支持，即设置hotkey_func函数地址。
		my_app_id = HOTKEY_MAGIC;
		char *p = malloc(buff_len);
		HOTKEY_FUNC = (grub_size_t)p;
		memmove((void*)HOTKEY_FUNC,&main,buff_len);
		//获取程序执行时的路径的文件名。
    ((grub_size_t(*)(char*,int,int,int))HOTKEY_FUNC)("INIT",HOTKEY_MAGIC,0,0);//获取新HOTKEY数据位置并作一些初使化
		if (debug > 0)
		{
			printf("Hotkey Installed!\n");
		}
	}
	else
		*p_hotkey_flags |= hotkey_flags;

	if (arg)  //处理热键参数的中括号
	{
    //本程序由command加载时, 热键数据是空白, 必须由热键备份内存获取.
    hkey_data_t *hkd =(hkey_data_t *)((grub_size_t(*)(char*,int,int,int))HOTKEY_FUNC)(NULL,HOTKEY_MAGIC,0,0); //热键数据地址
		hotkey_c *hkc = hkd->hk_cmd;

		while (*arg && *arg <= ' ')
			++arg;
		int key_code,cmd_len;
		int exist_key = -1;     //存在的键
		int disabled_key = -1;  //禁用的键
		if (*arg != '[')//显示当前已注册热键
		{
			if (!(flags & BUILTIN_CMDLINE) || debug < 1)//必须在命令行下并且DEBUG 非 OFF 模式才会显示
		    return 1;
			if (debug > 1)
		    printf("hotkey_data_addr: 0x%X\n",hkd);
			if (hkc->key_code)
		    printf("Current registered hotkey:\n");
			while(hkc->key_code)
			{
		    if (hkc->key_code != -1)
		    {
					if (debug > 1)
						printf("0x%X ",hkc->cmd);
					printf("%s=>%s\n",get_keyname(hkc->key_code),hkc->cmd);
		    }
		    ++hkc;
			}
			return -1;
		}

test:
		key_code = check_hotkey(&arg,0);  //获得热键参数中的中括号. 如: hotkey [F4] "commandlien"

		if (!key_code)  //没有热键结束
			return 0;
		while(*arg) //移动到中括号后, 取命令
		{
			if (*arg++ == ']')
		    break;
		}
		while (*arg && *arg <= ' ') //跳过空格
			++arg;

		if (*arg == '"')  //取命令
		{
#if 1
      cmd_len = 0;
      arg++;
      char *p = arg;
      while (*p++ != '"') //计算命令尺寸
        cmd_len++;
      *(--p) = 0; //把末尾的'"'变成0, 命令终止符
#else
			cmd_len = strlen(arg);
			++arg;
			--cmd_len;
			while(cmd_len--)
			{
		    if (arg[cmd_len] == '"')
		    {
    			arg[cmd_len] = 0;
					break;
		    }
			}
#endif
		}

#if 1
    cmd_len++;  //命令尺寸加终止符
#else
		cmd_len = strlen(arg) + 1;
#endif

		for(i=0;i<64;++i)
		{
			if (!hkc[i].key_code)           //无键代码, 退出
		    break;
			if (hkc[i].key_code == key_code)//键代码相等, 存在的键=i
		    exist_key = i;
			else if (hkc[i].key_code == -1) //如果键代码=-1
		    disabled_key = i;             //禁用的键=i
		}
		if (disabled_key != -1 && exist_key == -1)  //如果有禁用的键,也有存在的键
		{//有禁用的热键,直接使用该位置
			exist_key = disabled_key;
			hkc[exist_key].key_code = key_code;
		}
		if (i==64 && exist_key == -1)
		{
			printf("Max 64 hotkey cmds limit!");
			return 0;
		}
#if 0   //不能处理第二个中括号
		if (exist_key != -1)//已经存在
		{
			if (strlen(hkc[exist_key].cmd) >= cmd_len)//新的命令长度没有超过旧命令长度
				i = -1;
		}
		else//新增热键
#endif
		{
			exist_key = i;
			hkc[i].key_code = key_code;
		}
		if (cmd_len <= 1) //如果命令尺寸<=1
		{
			hkc[exist_key].key_code = -1; //设置禁用键代码
			return 1;
		}
		if (hkd->cmd_pos + cmd_len >= sizeof(hkd->cmd))
		{
			printf("error: not enough space!\n");
			return 0;
		}	    
		if (i >= 0)//需要更新地址
		{
			hkc[exist_key].cmd = hkd->cmd + hkd->cmd_pos;
			hkd->cmd_pos += cmd_len;//命令数据区
		}
		memmove(hkc[exist_key].cmd,arg,cmd_len ); //保存命令
		if (debug > 0)
			printf("%d [%s] registered!\n",exist_key,get_keyname(key_code));
    while (*arg >= ' ')  //检测终止符,回车换行
			arg++;
    if (*arg) //如果是回车换行,结束
      return 1;
    arg++;  //跳过终止符
    while (*arg >= ' ' && *arg != '[') //探测中括号
      arg++;
    if (*arg == '[')
      goto test;
    return 1;
	}
	return 0;
}

/*
	检测当前的按键码是否方向键或回车键
*/
static int check_allow_key(unsigned short key)
{
	int i;
	for (i=0;i<9;++i)
	{
		if (key == allow_key[i])
			return key;
	}
	return 0;
}

/*
	从菜单标题中提取热键代码
	返回热键对应的按键码。
	flags 非0时判断菜单的首字母.-A参数.
*/
static int check_hotkey(char **title,int flags)
{
	char *arg = *title;
	int code;
	while (*arg && *arg <= ' ')
		++arg;

	if (flags)	//如果标记非0
		return tolower(*arg) == flags;
	if (*arg == '^') //处理菜单项第一字符热键. 如: title ^Ctrl+d xxxxxx
	{
		++arg;
		sprintf(keyname_buf,"%.15s",arg);//最多复制15个字符
		nul_terminate(keyname_buf);//在空格处截断
		if ((code = get_keycode(keyname_buf,1)))
		{
			//设置新的菜单标题位置
			arg+=strlen(keyname_buf);
			if (*arg == 0)
				--arg;
			*arg = *(*title);
			*title = arg;
			return code;
		}
	}
	else if (*arg == '[') //处理菜单项第一个中括号. 如: title [F4] xxxxx;  title [Ctrl+d] xxxxx;
	{
		int i;
		++arg;
		while (*arg == ' ' || *arg == '\t')
			++arg;
		for(i = 0;i<15;++arg)
		{
			if (!*arg || *arg == ' ' || *arg == ']' )
			{
				break;
			}
			keyname_buf[i++] = *arg;
		}
		while (*arg == ' ')
			++arg;
		if (*arg != ']')
			return 0;
		keyname_buf[i] = 0;
		code = get_keycode(keyname_buf,0);
		return code;
	}
  arg = *title;  //2021-05-22新功能。支持菜单中的任意英文字母作为热键。扫描菜单项中的'^',把其后的字母设置为热键
  while (*arg)  //处理菜单项中任意位置的热键. 如: title Boot ^Windows 10
  {
    if (*arg == '^') 
      return *(arg+1) | 0x20; //转小写
    arg++;
  }
	return 0;
}

static int
get_keycode (char *key, int flags)	//获得键代码
{
  int idx, i;
  char *str;

  if (*(unsigned short*)key == 0x7830)/* == 0x*/
	{
		unsigned long long code;
		if (!safe_parse_maxint(&key,&code))
			return 0;
//		return (((long)code <= 0) || ((long)code >= 0xFFFF)) ? 0 : (int)code;
    return (((long long)code <= 0) || ((long long)code >= 0xFFFF)) ? 0 : (int)code;
	}
  str = key;
  idx = 0;
  if (!strnicmp (str, "shift", 5))    //比较字符串s1和s2的前n个字符但不区分大小写
	{
		idx = 1;
		str += 6;   //跳过shift+
	}
  else if (!strnicmp (str, "ctrl", 4))
	{
		idx = 2;
		str += 5;
	}
  else if (!strnicmp (str, "alt", 3))
	{
		idx = 4;
		str += 4;
	}
  
  if (flags && !idx) // 2021-05-22不再支持类似 ^F!, ^w 等，排除之。
    return 0;
  for (i = 0; key_table[i].name[0]; i++)  //在键表中搜索'+'号后的字母
	{
		if (!stricmp(str,key_table[i].name))  //比较字符串s1和s2。不区分大小写. 是shift+x, ctrl+x, alt+x 的组合.
		{
//			return key_table[i].code[idx];
			return key_table[i].code | idx << 24; 
		}
	}
  return 0;
}

static char *
//get_keyname (unsigned short code)
get_keyname (int code)	//获得键名称
{
  int i;
  int j = (code & 0xf000000) >> 24;
  char *p = 0;

  for (i = 0; *(key_table[i].name); i++)
	{
    if (key_table[i].code == (code & 0xffff))
    {
    switch (j)
    {
      case 0:
        p = (char *)"";
        break;
      case 1:
        p = (char *)"Shift-";
        break;
      case 2:
        p = (char *)"Ctrl-";
        break;
      case 4:
        p = (char *)"Alt-";
        break;
    }
    sprintf (keyname_buf, "%s%s", p, key_table[i].name);
    return keyname_buf;
    }
	}

  sprintf (keyname_buf, "0x%x", code);
  return keyname_buf;
}

static void get_G4E_image(void)
{
  grub_size_t i;

  //在内存0-0x9ffff, 搜索特定字符串"GRUB4EFI"，获得GRUB_IMGE
  for (i = 0x9F100; i >= 0; i -= 0x1000)
  {
    if (*(unsigned long long *)i == 0x4946453442555247)	//比较数据
    {
      g4e_data = *(grub_size_t *)(i+16); //GRUB4DOS_for_UEFI入口
      return;
    }
  }
  return;
}
