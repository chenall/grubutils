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
 * gcc -Wl,--build-id=none -m32 -mno-sse -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE hotkey.c -o hotkey.o
 *
 * disassemble:                 objdump -d hotkey.o 
 * confirm no relocation:       readelf -r hotkey.o
 * generate executable:         objcopy -O binary hotkey.o hotkey
 *
 */

#include "grub4dos.h"
#define DEBUG 0
typedef struct
{
  unsigned short code[4];
  char name[10];
} __attribute__ ((packed)) key_tab_t;
typedef struct 
{
	unsigned short key_code;
	unsigned short title_num;
} __attribute__ ((packed)) hotkey_t;

typedef struct
{
    int key_code;
    char *cmd;
} __attribute__ ((packed)) hotkey_c;

union
{
	int flags;
	struct
	{
		unsigned char sel;
		unsigned char first;
		unsigned short flag;
	} k;
} __attribute__ ((packed)) cur_menu_flag;

typedef struct
{
    int flags;
    int cmd_pos;
    hotkey_c hk_cmd[64];
    char cmd[4096];
} __attribute__ ((packed)) hkey_data_t;

#define CHECK_F11 0
#define HOTKEY_MAGIC 0X79654B48
#define HOTKEY_PROG_MEMORY	0x2000000-0x200000
#define HOTKEY_FLAGS_AUTO_HOTKEY (1<<12)
//#define HOTKEY_FLAGS_AUTO_HOTKEY1 (1<<11)
#define HOTKEY_FLAGS_NOT_CONTROL (1<<13)
#define HOTKEY_FLAGS_NOT_BOOT	 (1<<14)
#define BUILTIN_CMDLINE		0x1	/* Run in the command-line.  */
#define BUILTIN_MENU			(1 << 1)/* Run in the menu.  */
static key_tab_t key_table[] = {
//  {{0x011b, 0x011b, 0x011b, 0x0100}, "esc"},
  {{0x0231, 0x0221, 0x0000, 0x7800}, "1"},
  {{0x0332, 0x0340, 0x0300, 0x7900}, "2"},
  {{0x0433, 0x0423, 0x0000, 0x7a00}, "3"},
  {{0x0534, 0x0524, 0x0000, 0x7b00}, "4"},
  {{0x0635, 0x0625, 0x0000, 0x7c00}, "5"},
  {{0x0736, 0x075e, 0x071e, 0x7d00}, "6"},
  {{0x0837, 0x0826, 0x0000, 0x7e00}, "7"},
  {{0x0938, 0x092a, 0x0000, 0x7f00}, "8"},
  {{0x0a39, 0x0a28, 0x0000, 0x8000}, "9"},
  {{0x0b30, 0x0b29, 0x0000, 0x8100}, "0"},
  {{0x0c2d, 0x0c5f, 0x0c1f, 0x8200}, "-"},
  {{0x0d3d, 0x0d2b, 0x0000, 0x8300}, "="},
//  {{0x0e08, 0x0e08, 0x0e7f, 0x0e00}, "backspace"},
//  {{0x0f09, 0x0f00, 0x9400, 0xa500}, "tab"},
  {{0x1071, 0x1051, 0x1011, 0x1000}, "q"},
  {{0x1177, 0x1157, 0x1117, 0x1100}, "w"},
  {{0x1265, 0x1245, 0x1205, 0x1200}, "e"},
  {{0x1372, 0x1352, 0x1312, 0x1300}, "r"},
  {{0x1474, 0x1454, 0x1414, 0x1400}, "t"},
  {{0x1579, 0x1559, 0x1519, 0x1500}, "y"},
  {{0x1675, 0x1655, 0x1615, 0x1600}, "u"},
  {{0x1769, 0x1749, 0x1709, 0x1700}, "i"},
  {{0x186f, 0x184f, 0x180f, 0x1800}, "o"},
  {{0x1970, 0x1950, 0x1910, 0x1900}, "p"},
  {{0x1a5b, 0x1a7b, 0x1a1b, 0x1a00}, "["},
  {{0x1b5d, 0x1b7d, 0x1b1d, 0x1b00}, "]"},
//  {{0x1c0d, 0x1c0d, 0x1c0a, 0xa600}, "enter"},
  {{0x1e61, 0x1e41, 0x1e01, 0x1e00}, "a"},
  {{0x1f73, 0x1f53, 0x1f13, 0x1f00}, "s"},
  {{0x2064, 0x2044, 0x2004, 0x2000}, "d"},
  {{0x2166, 0x2146, 0x2106, 0x2100}, "f"},
  {{0x2267, 0x2247, 0x2207, 0x2200}, "g"},
  {{0x2368, 0x2348, 0x2308, 0x2300}, "h"},
  {{0x246a, 0x244a, 0x240a, 0x2400}, "j"},
  {{0x256b, 0x254b, 0x250b, 0x2500}, "k"},
  {{0x266c, 0x264c, 0x260c, 0x2600}, "l"},
  {{0x273b, 0x273a, 0x0000, 0x2700}, ";"},
  {{0x2827, 0x2822, 0x0000, 0x0000}, "'"},
  {{0x2960, 0x297e, 0x0000, 0x0000}, "`"},
  {{0x2b5c, 0x2b7c, 0x2b1c, 0x2600}, "\\"},
  {{0x2c7a, 0x2c5a, 0x2c1a, 0x2c00}, "z"},
  {{0x2d78, 0x2d58, 0x2d18, 0x2d00}, "x"},
  {{0x2e63, 0x2e43, 0x2e03, 0x2e00}, "c"},
  {{0x2f76, 0x2f56, 0x2f16, 0x2f00}, "v"},
  {{0x3062, 0x3042, 0x3002, 0x3000}, "b"},
  {{0x316e, 0x314e, 0x310e, 0x3100}, "n"},
  {{0x326d, 0x324d, 0x320d, 0x3200}, "m"},
  {{0x332c, 0x333c, 0x0000, 0x0000}, ","},
  {{0x342e, 0x343e, 0x0000, 0x0000}, "."},
//  {{0x352f, 0x352f, 0x9500, 0xa400}, "key-/"},
  {{0x352f, 0x353f, 0x0000, 0x0000}, "/"},
//  {{0x372a, 0x0000, 0x9600, 0x3700}, "key-*"},
//  {{0x3920, 0x3920, 0x3920, 0x3920}, "space"},
  {{0x3b00, 0x5400, 0x5e00, 0x6800}, "f1"},
  {{0x3c00, 0x5500, 0x5f00, 0x6900}, "f2"},
  {{0x3d00, 0x5600, 0x6000, 0x6a00}, "f3"},
  {{0x3e00, 0x5700, 0x6100, 0x6b00}, "f4"},
  {{0x3f00, 0x5800, 0x6200, 0x6c00}, "f5"},
  {{0x4000, 0x5900, 0x6300, 0x6d00}, "f6"},
  {{0x4100, 0x5a00, 0x6400, 0x6e00}, "f7"},
  {{0x4200, 0x5b00, 0x6500, 0x6f00}, "f8"},
  {{0x4300, 0x5c00, 0x6600, 0x7000}, "f9"},
  {{0x4400, 0x5d00, 0x6700, 0x7100}, "f10"},
/*
  {{0x4700, 0x4737, 0x7700, 0x9700}, "home"},
  {{0x4800, 0x4838, 0x8d00, 0x9800}, "up"},
  {{0x4900, 0x4939, 0x8400, 0x9900}, "pgup"},
  {{0x4a2d, 0x4a2d, 0x8e00, 0x4a00}, "key--"},
  {{0x4b00, 0x4b34, 0x7300, 0x9b00}, "left"},
  {{0x4c00, 0x4c35, 0x0000, 0x0000}, "key-5"},
  {{0x4d00, 0x4d36, 0x7400, 0x9d00}, "right"},
  {{0x4e2b, 0x4e2b, 0x0000, 0x4e00}, "key-+"},
  {{0x4f00, 0x4f31, 0x7500, 0x9f00}, "end"},
  {{0x5000, 0x5032, 0x9100, 0xa000}, "down"},
  {{0x5100, 0x5133, 0x7600, 0xa100}, "pgdn"},
*/
  {{0x5200, 0x5230, 0x9200, 0xa200}, "ins"},
  {{0x5300, 0x532e, 0x9300, 0xa300}, "del"},
  {{0x8500, 0x8700, 0x8900, 0x8b00}, "f11"},
  {{0x8600, 0x8800, 0x8a00, 0x8c00}, "f12"},
/*
  {{0x0000, 0x0000, 0x7200, 0x0000}, "prtsc"},
*/
  {{0x0000, 0x0000, 0x0000, 0x0000}, 0}
};

static unsigned short allow_key[9] = {
/*KEY_ENTER       */0x1C0D,
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
static int get_key(void);
static int check_hotkey(char **title,int flags);
static int check_f11(void);
static char *get_keyname (unsigned short code);
static int check_allow_key(unsigned short key);
/* gcc treat the following as data only if a global initialization like the
 * above line occurs.
 */
//static hkey_data_t hotkey_data = {0,{0,NULL}};//HOTKEY 数据保留区
static hkey_data_t hotkey_data;//HOTKEY 数据保留区  由.data移动到.bss
static int hotkey_cmd_flag = HOTKEY_MAGIC;//程序尾部标志

/* this is needed, see the comment in grubprog.h */
#include "grubprog.h"
/* Do not insert any other asm lines here. */

int main(char *arg,int flags,int flags1)
{
	int i;
	char *base_addr;
	char *magic;
	static hotkey_t *hotkey;
//	unsigned short hotkey_flags;
	int hotkey_flags;
//	unsigned short *p_hotkey_flags;
	int exist_key;
	int disabled_key;
	int delete_key;

	base_addr = (char *)(init_free_mem_start+512);
	hotkey = (hotkey_t*)base_addr;
//	p_hotkey_flags = (unsigned short*)(base_addr + 504);
	cur_menu_flag.flags = flags1;
	hkey_data_t *hkd;
	hotkey_c *hkc; 

	if (flags == HOTKEY_MAGIC)
	{
	    if (arg && *(int *)arg == 0x54494E49)//INIT 初始数数据
	    {
		hotkey_data.hk_cmd[0].cmd = hotkey_data.cmd;
		hotkey_data.cmd_pos = 0;
		hotkey_data.flags = 0;
	    }
	    return (int)&hotkey_data;
	}
	if (flags == -1)
	{
		int c;
		hotkey_c *hkc = hotkey_data.hk_cmd;
		if (my_app_id != HOTKEY_MAGIC/* || (!hotkey->key_code && !hkc->key_code)*/)
		{
		    return getkey();
		}
		hotkey_flags = hotkey_data.flags;
		#if CHECK_F11
		c = get_key();
		#else
		c = getkey();
		#endif
		#if DEBUG
		putchar_hooked = 0;
		gotoxy(70,0);
		printf("%d,%d\t",cur_menu_flag.k.first,cur_menu_flag.k.sel);
		#endif
		if (!c || check_allow_key(c))
			return c;
		for(i=0;i<64;++i)
		{
		    if (!hkc[i].key_code)
			break;
		    if (hkc[i].key_code == c)
		    {
			grub_error_t err_old = errnum;
			if (hkc[i].cmd[0] == '@')//静默方式运行(没有任何显示)
			{
				if (timer)
				{
					free ((void *)timer);
					timer = 0;
					grub_timeout = -1;
					cls();
				}
			    builtin_cmd(NULL,hkc[i].cmd + 1,BUILTIN_CMDLINE);
			}
			else
			{
				if (timer)
				{
					free ((void *)timer);
					timer = 0;
					grub_timeout = -1;
				}
			    if ((*(int*)0x8278) >= 20131014)//高版本的GRUB4DOS支持
			    {
				putchar_hooked = 0;
				setcursor (1); /* show cursor and disable splashimage */
				 if (current_term->SETCOLORSTATE)
				    current_term->SETCOLORSTATE(COLOR_STATE_STANDARD);
				cls();
				    printf_debug(" Hotkey Boot: %s\n",hkc[i].cmd);
			    }
			    builtin_cmd(NULL,hkc[i].cmd,BUILTIN_CMDLINE);
			}
			if (putchar_hooked == 0 && errnum > ERR_NONE && errnum < MAX_ERR_NUM)
			{
			    printf_errinfo("\nError %u\n",errnum);
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
			char h = tolower(c&0xff);
//			char *old_c = (char*)(p_hotkey_flags+1);
//			char *old_t = old_c + 1;
//			int find = -1,n = *old_t;
			char old_c;
			char old_t;
			int find = -1,n = old_t;
			int cur_sel = -1;
			char **titles;
			if (*(char*)0x417 & 3)
			{
				c = (c & 0xff00) | h ;
				goto chk_control;
			}
			if (cur_menu_flag.k.flag == 0x4B40)
				cur_sel = cur_menu_flag.k.sel + cur_menu_flag.k.first;
			titles = (char **)(base_addr + 512);
//			if (*old_c != h)
			if (old_c != h)
			{//不同按键清除记录
//				*old_c = h;
//				n = *old_t = 0;
				old_c = h;
				n = old_t = 0;
			}
			for(i=0;i<256;++i)
			{
				if (!*titles)
					break;
				if (check_hotkey(titles,h))
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
//				if (find != i) *old_t = 1;
//				else (*old_t)++;
				if (find != i) old_t = 1;
				else old_t++;
				return (hotkey_flags|HOTKEY_FLAGS_NOT_BOOT|find)<<16;
			}
			if (h > '9')
				return 0;
		}
		chk_control:
		if (hotkey_flags & HOTKEY_FLAGS_NOT_CONTROL)
			return 0;
		return c;
	}
	else if (flags == 0)
	{
		char **titles;
		titles = (char **)(base_addr + 512);
		for(i=0;i<126;++i)
		{
			if (!*titles)
				break;
			if (hotkey->key_code = check_hotkey(titles,0))
			{
				hotkey->title_num = i;
				++hotkey;
			}
			++titles;
		}
		hotkey->key_code = 0;
		return 1;
	}

  if (*arg == '-' && *(arg+1) == 'u')
  {
    if (HOTKEY_FUNC)
    {
      free ((void *)HOTKEY_FUNC);
      HOTKEY_FUNC = 0;
      hotkey_flags = 0;
    }
    return;
  }
  
	if ((flags & BUILTIN_CMDLINE) && (!arg || !*arg))
	{
    printf("Hotkey for grub4dos by chenall,%s\n",__DATE__);
    printf("Usage:\n\thotkey -nb\tonly selected menu when press menu hotkey\n\thotkey -nc\tdisable control key\n");
    printf("      \thotkey -A\tselect the menu item with the first letter of the menu\n");
    printf("      \thotkey -u\tunload hotkey.\n");
    printf("      \thotkey [HOTKEY] \"COMMAND\"\tregister new hotkey\n\thotkey [HOTKEY]\tDisable Registered hotkey HOTKEY\n");
    printf("      \te.g.\thotkey -A [F3] \"reboot\" [Ctrl+d] \"commandline\"\n");
    printf("      \te.g.\ttitle [F4] Boot Win 8\n");
    printf("      \te.g.\ttitle Boot ^Win 10\n\n");
    printf("      \tsetmenu --hotkey-color=[COLOR]\tset hotkey color.\n");
    printf("      \tCommand keys such as p, b, c and e will only work if SHIFT is pressed when hotkey -A\n");
        
    if (HOTKEY_FUNC && debug > 1)
    {
      hkd = ((hkey_data_t*(*)(char*,int))HOTKEY_FUNC)(NULL,HOTKEY_MAGIC);
      hkc = hkd->hk_cmd;
      printf("hotkey_data_addr: 0x%X\n",hkd);

      if (hkc->key_code)
        printf("Current registered hotkey:\n");
      while(hkc->key_code)
      {
        if (hkc->key_code != -1)
        {
          printf("0x%X ",hkc->cmd);
          printf("%s=>%s\n",get_keyname(hkc->key_code),hkc->cmd);
        }
        ++hkc;
      }  
    }
    
    return;
	}

	if (!HOTKEY_FUNC)
	{
		int buff_len = 0x4000;
		memset ((void *)&hotkey_data, 0, sizeof(hkey_data_t));
//		buff_len = (unsigned int)&__BSS_END - (unsigned int)&main;
		#if CHECK_F11
		if (check_f11())//检测BIOS是否支持F11,F12热键，如果有支持直接使用getkey函数取得按键码
		{
			_checkkey_ = 1;
			printf_debug("Current BIOS supported F11,F12 key.\n");
		}
		else
		{
			_checkkey_ = 0;
			printf_debug("Current BIOS Does not support F11,F12 key,try to hack it.\n");
		}
		#endif
		//HOTKEY程序驻留内存，直接复制自身代码到指定位置。
		my_app_id = HOTKEY_MAGIC;
		memmove((void*)HOTKEY_PROG_MEMORY,&main,buff_len);
		//开启HOTKEY支持，即设置hotkey_func函数地址。
		HOTKEY_FUNC = HOTKEY_PROG_MEMORY;
		char* arg_bak = arg;
		((int(*)(char*,int))HOTKEY_FUNC)("INIT",HOTKEY_MAGIC);//获取HOTKEY数据位置并作一些初使化
		arg = arg_bak;
		printf_debug("Hotkey Installed!\n");
	}

	if (arg)
	{
    hotkey_flags = 1<<15;
    while (*arg == '-')
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

      arg = wee_skip_to(arg,0);
    }
	    hkd = ((hkey_data_t*(*)(char*,int))HOTKEY_FUNC)(NULL,HOTKEY_MAGIC);
	    hkc = hkd->hk_cmd;
	    hkd->flags |= hotkey_flags;

	    while (*arg && *arg <= ' ')
		++arg;
    	    int key_code,cmd_len;
    	    if (*arg != '[')//显示当前已注册热键
    	    {
		return -1;
	    }
    if (arg[1] == ']')  //[] 删除热键数据
    {
      memset ((void *)hkd, 0, sizeof(hkey_data_t));
      arg +=2;
      while (*arg && *arg <= ' ') //跳过空格
        ++arg;
      goto test_end;
    }
test:
	    key_code = check_hotkey(&arg,0);
    	    if (!key_code)
		return 0;
       	    while(*arg)
	    {
		if (*arg++ == ']')
		    break;
	    }
	    while (*arg && *arg <= ' ')
		++arg;

    exist_key = -1;
    disabled_key = -1;
    delete_key = -1;
	    if (*arg == '"')
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
    else  //如果没有命令, 是删除指定热键
    {
      delete_key = 1;
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
		if (hkc[i].key_code == key_code)
		{
		    exist_key = i;
        break;                        //全部搜索,不能处理第二个中括号. 找到后必须退出.
		}
		else if (hkc[i].key_code == -1)
		    disabled_key = i;
	    }

    if (exist_key != -1 && delete_key != -1)    //如果键存在,并且要删除
    {
      hkc[exist_key].key_code = -1;             //设置禁用键代码
    }
	    if (disabled_key != -1 && exist_key == -1)
	    {//有禁用的热键,直接使用该位置
		exist_key = disabled_key;
		hkc[exist_key].key_code = key_code;
	    }

	    if (i==64 && exist_key == -1)
	    {
		printf_errinfo("Max 64 hotkey cmds limit!");
		return 0;
	    }
	    if (exist_key != -1)//已经存在
	    {
		if (strlen(hkc[exist_key].cmd) >= cmd_len-1)//新的命令长度没有超过旧命令长度
		   i = -1;
	    }
	    else//新增热键
	    {
    		exist_key = i;
		hkc[i].key_code = key_code;
	    }
	    
	    if (cmd_len <= 1)//禁用热键
	    {
		hkc[exist_key].key_code = -1;
		return 1;
	    }

	    if (hkd->cmd_pos + cmd_len >= sizeof(hkd->cmd))
	    {
		printf_errinfo("error: not enough space!\n");
		return 0;
	    }
	    
	    if (i >= 0)//需要更新地址
	    {
	        hkc[exist_key].cmd = hkd->cmd + hkd->cmd_pos;
	        hkd->cmd_pos += cmd_len;//命令数据区
	    }

	    memmove(hkc[exist_key].cmd,arg,cmd_len );

		printf_debug("%d [%s] registered!\n",exist_key,get_keyname(key_code));
    while (*arg >= ' ')  //检测终止符,回车换行
			arg++;
    if (*arg) //如果是回车换行,结束
      return 1;
    arg++;  //跳过终止符
    while (*arg >= ' ' && *arg != '[') //探测中括号
      arg++;
test_end:
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

static char inb(unsigned short port)
{
	char ret_val;
	__asm__ volatile ("inb %%dx,%%al" : "=a" (ret_val) : "d"(port));
	return ret_val;
}

static void outb(unsigned short port, char val)
{
	__asm__ volatile ("outb %%al,%%dx"::"a" (val), "d"(port));
}
#if CHECK_F11
/*检查BIOS是否支持F11作为热键*/
static int check_f11(void)
{
	int i;
	//要发送按键的指令。
	outb(0x64,0xd2);
	//发送一个按键扫描码
	outb(0x60,0x57);
	//发送一个按键中断码  扫描码（scan_code）+ 0x80 = 中断码（Break Code）
	outb(0x60,0xD7);
	//判断是否可以接收到这个按键，有的话就是有支持，否则不支持。
	for(i=0;i<3;++i)
	{
		if (checkkey() == 0x8500)
		{
			getkey();
			return 1;
		}
	}
	return 0;
}

static int get_key(void)
{
	if (_checkkey_ == 0)//BIOS不支持F11热键
	{
		while(checkkey()<0)
		{
		//没有检测到按键，可能是用户没有按键或者BIOS不支持
			unsigned char c = inb(0x60);
			//扫描码低于0X57的不处理，直接丢给BIOS。
			if (c < 0x57)
			{
				outb(0x64,0xd2);
				outb(0x60,c);
				break;
			}
			switch(c |= 0x80)//取中断码
			{
				case 0xd7: //F11
					return 0x8500;
				case 0xd8: //F12
					return 0x8600;
			}
		}
		//bios检测到按键，
	}
	return getkey();
}
#endif

/*
	从菜单标题中提取热键代码
	返回热键对应的按键码。
	flags 非0 时判断菜单的首字母.-A参数.
*/
static int check_hotkey(char **title,int flags)
{
	char *arg = *title;
	unsigned short code;
	while (*arg && *arg <= ' ')
		++arg;
	if (flags)
		return tolower(*arg) == flags;
	if (*arg == '^')
	{
		++arg;
		sprintf(keyname_buf,"%.15s",arg);//最多复制15个字符
		nul_terminate(keyname_buf);//在空格处截断
		if ((code = (unsigned short)get_keycode(keyname_buf,1)))
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
	else if (*arg == '[')
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
		code = (unsigned short)get_keycode(keyname_buf,0);
		return code;
	}
  arg = *title;  //2021-05-22新功能。支持菜单中的任意英文字母作为热键。扫描菜单项中的'^',把其后的字母设置为热键
  while (*arg)
  {
    if (*arg == '^')
    {
      ++arg;
      keyname_buf[0] = *arg;
      keyname_buf[1] = 0;
      if ((code = (unsigned short)get_keycode(keyname_buf,0)))
        return code;
    }
    arg++;
  }
	return 0;
}

static int
get_keycode (char *key, int flags)
{
  int idx, i;
  char *str;

  if (*(unsigned short*)key == 0x7830)/* == 0x*/
    {
      unsigned long long code;
      if (!safe_parse_maxint(&key,&code))
			return 0;
      return (((long)code <= 0) || ((long)code >= 0xFFFF)) ? 0 : (int)code;
    }
  str = key;
  idx = 0;
  if (!strnicmp (str, "shift", 5))
    {
      idx = 1;
      str += 6;
    }
  else if (!strnicmp (str, "ctrl", 4))
    {
      idx = 2;
      str += 5;
    }
  else if (!strnicmp (str, "alt", 3))
    {
      idx = 3;
      str += 4;
    }

  if (flags && !idx &&
          (str[0] != 'F' && str[1] != '\0') ||
          (str[0] == 'F' && (str[1] < '1' || str[1] > '9')) ||
          (str[0] == 'F' && str[2] != '\0' && str[2] != '0' && str[2] != '1' && str[2] != '2'))
    return 0;
  for (i = 0; key_table[i].name[0]; i++)
    {
      if (!stricmp(str,key_table[i].name))
      {
			return key_table[i].code[idx];
		}
    }
  return 0;
}


static char *
get_keyname (unsigned short code)
{
  int i;

  for (i = 0; *(key_table[i].name); i++)
    {
      int j;

      for (j = 0; j < 4; j++)
	{	 
	  if (key_table[i].code[j] == code)
	    {
	      char *p;
	      
	      switch (j)
		{
		case 0:
		  p = "";
		  break;
		case 1:
		  p = "Shift-";
		  break;
		case 2:
		  p = "Ctrl-";
		  break;
		case 3:
		  p = "Alt-";
		  break;
		}
	      sprintf (keyname_buf, "%s%s", p, key_table[i].name);
	      return keyname_buf;
	    }
	}
    }

  sprintf (keyname_buf, "0x%x", code);
  return keyname_buf;
}
