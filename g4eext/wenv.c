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
 * co mpile:
 * gcc -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -m64 -Wl,-N -fPIE ./g4dext/wenv.c 2>&1|tee build.log
 * disassemble:			objdump -d a.out > wenv.s
 * confirm no relocation:	readelf -a a.out > wenv-a.s
 * generate executable:		objcopy -O binary a.out wenv
 *
 */
/*
 2010-10-05 醉书生在svn版本9基础上修改
	变量名必须由非数字开头的字母、数字、下划线组成(不能以数字开头)
	变量名长度须完全匹配
		WENV set A12345678 非法，不会匹配  A1234567
		WENV set A1- 非法，不会匹配 A1
	set命令在debug未关闭时回显结果
	set命令变量名后必须有=，"WENV set TT" 将不会清掉TT
	
	reset命令复位?_WENV
	reset命令扩展批量清除
		WENV reset ABC 等效于 WENV set ABC=
		WENV reset ABC* 清理所有 ABC 开头的变量
		
	帮助信息整理，增加了reset，子命令出错只显示该命令帮助
	set/calc/run/read命令无参数时，显示相应命令语法
 */
/*
 * to get this source code & binary: http://grubutils.googlecode.com
 * For more information.Please visit web-site at http://chenall.net/grub4dos_wenv/
 * 2010-06-20
 2010-11-24
	1.添加子命令dir,支持通配符*,?
	比如,显示当前目录下所有.TXT文件.
	wenv dir /*.txt
	2.添加exec子命令(从CALL中分离出来的),用于调用外部命令.

 2010-11-23
	1.添加两个控制命令.continue和break.
	可以用于for循环中,continue跳过本次循环,break跳出循环.(可嵌套).
	另外break,也可以用于集合命令中,跳过后面的命令.

 2010-11-21
	1.添加三个伪变量
	@DATE		当前日期: 2010-11-21
	@TIME		当前时间: 17:05:30
	@RANDOM	产生一个随机数
	例子,显示今天日期. echo ${@DATE}
 2010-11-20
	1.修改主函数执行代码,优化整理架构.支持复合语句.
 2010-11-14
	1.echo自动添加的回车符改为\r\n
 2010-11-01
	1.echo字命令修改.添加两个参数.
	 1).-e 充许转义输出.
	 2).-n 输出不自动加回车符.
 2010-10-17
   1.BUG修正。
 2010-10-13
	1.尝试添加变量嵌套支持。
 2010-10-12
	1.添加简单的echo子命令。
	2.添加call子命令(就是run命令)
	
 2010-10-11
	1.添加子命令for(仿cmd模式),三种模式.(注意：语法要求比较严格请按照下面的格式使用)。
	  FOR /L %variable IN (start,step,end) DO wenv-command
	  FOR /F ["options"] %variable IN ( file-set ) DO wenv-command
	  注:file-set 前后必须有空格
	  FOR /F ["options"] %variable IN ("string") DO wenv-command
	  注:string前后必须有引号，并且紧跟()，否则都算非法。
	  支持的options
			 eol=c
			 delims=xxx
			 tokens=x,y,m-n
	 使用方法请参考cmd的for命令.
 2010-10-10
	1.修正由(tuxw)提出来的BUG，在使用变量提取时会失败的问题。
	2.重新调整程序架构，优化代码。
	3.支持命令序列。使用()里面的语句是一个序列。每个语句之前使用“ ; ”隔开.
	  例:wenv (set a=1 ; set c=2 ; set b=3).可以配合for命令使用.允许嵌套;
	4.set 命令也可以显示变量(按cmd的set命令使用习惯).如果后面没有=，则显示所有以输入的字符开头的变量.
	5.修改*的定义。(比较方便处理)
	  *ADDR$ 从内存地址中取字符串。*ADDR取一个数值。
	6.$$反义一个$(像批处理的%%),即两个$$处理时第一次会变成一个$,方便在for里面使用变量.
 2010-10-09
	1.初步添加for命令，目前支持以下两种操作.并且允许嵌套。(仿 CMD 的 for 命令)
	1). for /L $i IN (start,step,end) DO wenv-command
		 以增量形式从START到END的一个数字序列。
		 例子：
		 wenv for /L $a in (1,1,5) do run echo $a\n
		 wenv for /L $a in (5,-1,0) do run echo $a\n
	2). for /F $i IN ( file ) DO wenv-command
		 读取 FILE 的每一行。忽略以":"开头的行。
		 注：使用该功能file前后都必需要有空格隔开.
		 例子:
		 wenv for /F $a in ( /test.txt ) do run echo $a\n
	
	2.read 命令支持参数(类似批处理的参数功能)
	  其中$0 是代表自身. $1-$9 分别是第1个－第9个参数.
	这样可以配合for等命令实现一些强大的功能..
	3.修正check命令比较数字的BUG.
 2010-10-08
	1.修正cal ++VARIABLE没有成功的BUG。
	2.修正比较符BUG。
	3.变量提取支持${VARIABLE:X:-Y}的形式，-Y代表提取到-Y个字符(即倒数Y个字符都不要).
	4.比较操作使用check关键字，避免在比较时和关键命令混淆.
	5.添加内置变量?_GET用于GET命令，存放的时变量的长度。
	6.为了方便以后添加新功能，还有查错，重新整理了代码，每个命令使用独立的函数来实现（以前都是在同一个函数里面）
	7."check" 命令增加一个功能，后面可以加一个WENV命令方便使用。并且可以check连用一个例子:
	wenv set a=10
	wenv check ${a}>=10 check ${a}<=20 run echo a is between 10 and 20
 2010-10-07
	1.CALC支持自增/自减运算符++/--,和C语言的语法一样
	例子,注意执行的顺序.
	wenv calc a=b++ //结果a=b b=b+1
	wenv calc a=++b //结果b=b+1 a=b
 2010-10-06
	1.添加 >=,<= 比较符.
	2.支持按数字比较.
		如果要比较的第一个串是一个数字就按数字进行比较.
	3.数字比较时支持使用内存地址.*ADDR
 2010-10-05
	1.修正calc命令除法计算的bug.
	2.整合醉书生的修改版,并修复几个小bug.
	3.添加字符串比较功能. == <>
 2010-10-04
	1.添加字符串处理功能,和linux shell类似.
		1).${VAR:x:y}  从x开始提取y个字符,如果x为负数则从倒数x个开始提取.
		以下提取的内容都不包含STRING字符串.
		2).${VAR#STRING}	删除STRING前面的字符。
		3).${VAR##STRING} 删除STRING前面的字符，贪婪模式。
		4).${VAR%STRING}	删除STRING后面的字符。
		5).${VAR%%STRING}	删除STRING后面的字符，贪婪模式。
 2010-10-04
	1.函数调整.
 2010-09-30
	1.调整calc函数。
 2010-09-06
	1.修正一个逻辑错误（在释放变量的时候）。
	http://bbs.wuyou.com/viewthread.php?tid=159851&page=25#pid2033990
 2010-08-08 1.read 函数读取的文件允许使用":"符号注释.使用":"开头,这一行将不会执行.
	2.解决当使用read可能导致出错的问题.
 */
#include <grub4dos.h>
//#define DEBUG
#define MAX_ARG_LEN 1024
//用户可使用变量数
#define MAX_USER_VARS 60
#define MAX_VARS 64
#define MAX_VAR_LEN	8
#define MAX_ENV_LEN	512
#define MAX_BUFFER	(MAX_VARS * (MAX_VAR_LEN + MAX_ENV_LEN))
// 变量保存地址
//#define BASE_ADDR 0x45000
// 变量名
#define VAR ((char (*)[MAX_VAR_LEN])BASE_ADDR)
// 变量值
#define ENVI ((char (*)[MAX_ENV_LEN])(BASE_ADDR + MAX_VARS * MAX_VAR_LEN))
#define _WENV_ 60
#define WENV_RANDOM (*(unsigned long *)(ENVI[_WENV_]+0x20))
#define QUOTE_CHAR (*(unsigned long *)(ENVI[_WENV_]+0x30))
#define MAX_ARG_BUFF	0x1000
enum ENUM_CMD {CMD_HELP=-1, VAR_TIP=0, CMD_SET, CMD_GET, CMD_RESET, CMD_CALC, CMD_CALL, CMD_READ, CMD_CHECK, CMD_FOR, CMD_ECHO, CMD_DIR};

static int wenv_func(char *arg, int flags);		/* 功能入口 */
static int wenv_help_ex(enum ENUM_CMD cmd);		/* 显示帮助 */
static void trim_p(char **p_str);
#define wenv_help()     wenv_help_ex(CMD_HELP)
// 字符串比较
static int grub_memcmp(const char *s1, const char *s2, int n);
/* 
arg比较来源字符串,string要对比的字符串.
功能:
参数比较(类似于strcmp，但是这里作了扩展)
1.arg可以比string长;使用空格' ',制表符'\t',等于号'=' 分隔参数;
2.如果string字符串全部是小写字符,则不区分大小写进行比较.
*/
#define strcmp_ex(arg,string) grub_memcmp(arg,string,0)
static char *strstrn(const char *s1, const char *s2, const int n);	// 查找子串
// 环境变量功能函数
//static int envi_cmd(const char *var, char * const env, int flags);
#define set_envi(var, val)			envi_cmd(var, val, 0)
#define get_env(var, val)			envi_cmd(var, val, 1)
#define get_env_all()				envi_cmd(NULL, NULL, 2)
#define reset_env_all()				envi_cmd(NULL, NULL, 3)

static char *skip_next_next_cmd (char *cmd,int *status,int flags);
#define STATUS_AND	1
#define STATUS_OR		2
#define STATUS_ELSE	4
#define STATUS_NONE	0
#define STATUS_QUEUE	3

static int replace_str(char *in, char *out, int flags);	// 变量替换, 主要功能函数,低16位最大长度，高16位标志。
#define REPLACE_RTRIM	0x10000
#define REPLACE_LTRIM	0x20000
#define REPLACE_TRIM_QUOTE	0x40000
#define REPLACE_DEFAULT (REPLACE_RTRIM | REPLACE_LTRIM)
#define REPLACE_TRIM (REPLACE_RTRIM | REPLACE_LTRIM | REPLACE_TRIM_QUOTE)

static char *skip_next (int flags,char *arg);
#define SKIP_FLAGS_LINE		0x100
#define SKIP_FLAGS_NONE		0
#define SKIP_FLAGS_TERMINATE	0x200

static int read_val(char **str_ptr,unsigned long long *val);	// 读取一个数值(用于计算器)
static int ascii_unicode(const char *ch, char *addr);	// unicode编码转换
void case_convert(char *ch,char flag);
#define upper(string)	case_convert(string,'a')
#define lower(string)	case_convert(string,'A')
//static void lower(char *string);			// 小写转换
//static void upper(char *string);			// 大写转换
//static char* next_line(char *arg, char eol);				// 读下一命令行
static int strcpyn(char *dest,const char *src,int n);	// 复制字符串，最多n个字符
static int printfn(char *str,int n);				//最多显示n个字符
//子命令集
static int set_func(char *arg,int flags);
static int get_func(char *arg,int flags);
static int reset_func(char *arg,int flags);
static int calc_func(char *arg,int flags);			// 简单计算器模块, 参数是一个字符串
static int read_func(char *arg,int flags);				// 读外部文件命令集
static int call_func(char *arg,int flags);
static int exec_func(char *arg, int flags);
static int check_func(char *arg,int flags);
static int for_func(char *arg, int flags);
static int echo_func(char *arg, int flags);
static int config_func(char *arg, int flags);
static int dir_func (char *arg, int flags);
static int break_func(char *arg,int flags);
static int continue_func(char *arg,int flags);
static int replace(char *str ,const char *sub,const char *rep);
static int wenv_flags = 0x42555247;/* this is needed, see the following comment. */
#define WENV_FLAGS_BREAK 1
#define WENV_FLAGS_CONTINUE 2
/* gcc treat the following as data only if a global initialization like the
 * above line occurs.
 */
static struct cmd_list
{
  char *name;			//命令名
  int (*func) (char *, int);	//对应的功能函数
  int flags;			//标志
} *p_cmd_list = NULL;

static grub_size_t g4e_data = 0;
static void get_G4E_image(void);

/* this is needed, see the comment in grubprog.h */
#include <grubprog.h>
/* Do not insert any other asm lines here. */

int main(char *arg,int flags)
{
  get_G4E_image();
  if (! g4e_data)
    return 0;

//	if (*(int *)x8278 < 20101124)  
	if (*(int *)IMG(0x8278) < 20101124)
	{
		printf("grub4dos version does not support\n");
		return 0;
	}
	struct cmd_list wenv_cmd[16]=
	{
		{"set",set_func,(CMD_SET << 8)},
		{"get",get_func,CMD_GET << 8},
		{"reset",reset_func,CMD_RESET << 8},
		{"calc",calc_func,(CMD_CALC << 8) | 1},
		{"read",read_func,(CMD_CALC << 8) | 1},
		{"call",call_func,(CMD_CALL << 8) | 1},
		{"exec",exec_func,(CMD_CALL << 8) | 1},
		{"check",check_func,(CMD_CHECK << 8) | 1},
		{"for",for_func,(CMD_FOR << 8) | 1},
		{"echo",echo_func,CMD_FOR << 8},
		{"dir",dir_func,CMD_DIR << 8},
		{"config",config_func,0},
		{"break",break_func,0},
		{"continue",continue_func,0},
		{NULL,NULL,0}
	}; //必须在程序中进行初始化
	p_cmd_list = wenv_cmd;
	if (p_cmd_list == NULL)
		return ! (errnum = ERR_WONT_FIT);
	if( ! *arg || arg == NULL )
		return wenv_help();
	if( 0 != strcmp_ex(VAR[_WENV_], "?_WENV") )// 检查默认变量
		reset_env_all();
	#ifdef DEBUG
	printf("wenv_arg:%s\n",arg);
	#endif
	return wenv_func (arg , flags);
}
static char *check_Brackets(const char *arg)
{
	const char *p = arg++;
	int i = 1;
	while (i && *++p)//end of *p is ')'
	{
		if (*p == '(')
			i++;
		else if (*p == ')')
			i--;
		else if (*p == QUOTE_CHAR)
		{
			while (*++p && *p != QUOTE_CHAR)
				;
		}
	}
	if (i != 0 )
	{
		return NULL;
	}
	return (char *)p;
}

static char *skip_next (int flags,char *arg)
{
	if (flags & SKIP_FLAGS_LINE)//skip to next line
	{
		char eol = flags & 0xff;
		if (eol == '\0')
			eol = ':';
		while (*arg)
		{
			if (*arg == '\r' || *arg == '\n')
			{
				*arg++ = 0;
				while (*arg == '\r' || *arg == '\n' || *arg == ' ' || *arg == '\t')
					arg++;
				if (*arg != eol)
					break;
			}
			arg++;
		}
		return *arg?arg:0;
	}

	while (*arg && *arg != ' ' && *arg != '\t' &&
	! ((flags & 1) && *arg == '='))
	{
		if (*arg == QUOTE_CHAR)
		{
			while (*++arg && *arg != QUOTE_CHAR)
				;
		}
		else if (*arg == '\\')
		{
			++arg;
		}
		if (*arg)
			arg ++;
	}
	
	if ((flags & SKIP_FLAGS_TERMINATE) && *arg) //Nul_TERMINATE;
		*arg++ = '\0';
	while (*arg == ' ' || *arg == '\t' || ((flags & 1) && *arg == '='))
		arg ++;

	return arg;
}

static char *skip_next_next_cmd (char *cmd,int *status,int flags)
{
	*status = 0;
	if (cmd == NULL || *cmd == 0)
		return NULL;
	while (*cmd)
	{
		if (*cmd == '(')
			cmd = check_Brackets(cmd);
		cmd = skip_next (0, cmd);
		switch (*(unsigned int *)cmd)
		{//&=0x26;|=0x7c;]=0x5d;!=0x21; ;=0x3b
			case 0x20265d5d://	operator AND "]]&"
				*status = STATUS_AND;
				break;
			case 0x207c5d5d://	operator OR "]]|"
				*status = STATUS_OR;
				break;
			case 0x20215d5d://	]]!
				*status = STATUS_ELSE;
				break;
			case 0x205D5D5D://]]]
				*status = STATUS_QUEUE;
				break;
			default:
				if (*(unsigned short *)cmd != 0x203b)
					continue;
				*status = STATUS_NONE;
				break;
		}
		*(cmd - 1) = '\0';
		cmd = skip_next (0, cmd);
		if (flags == 0 || *status == flags)
		{
			break;
		}
	}
	return cmd;
}

static int wenv_func(char *arg, int flags)
{
	char *p = arg;
	char *p1;
	int ret = 0;
	int status = 0;
	int i;
	wenv_flags = 0;
	char *cmd_buff;
	if ((cmd_buff = malloc(0x800)) == NULL)
		return 0;

	while ( *p && (arg = p) != NULL)
	{
		if (*arg == '(')
		{
			p = check_Brackets(arg);
			if (p == NULL)
			{
				free(cmd_buff);
				return !(errnum = ERR_BAD_ARGUMENT);
			}
			*p++ = '\0';
			p = skip_next_next_cmd(p,&status,STATUS_NONE);
			ret = wenv_func(++arg,flags);
		}
		else
		{
			#ifdef DEBUG
				printf("wenv_arg:%s\n",arg);
			#endif
			*cmd_buff = '\0';
			p = skip_next_next_cmd(p,&status,STATUS_NONE);
			replace_str(arg,cmd_buff,REPLACE_TRIM | MAX_ENV_LEN); //替换变量
			arg = cmd_buff;
			for (i=0;p_cmd_list[i].name != NULL ;i++)
			{
				if (strcmp_ex(arg, p_cmd_list[i].name) == 0)
				{
					arg = skip_next(0, arg);
					if((unsigned char)*arg < (unsigned char)p_cmd_list[i].flags)
					{
						free(cmd_buff);
						return wenv_help_ex(p_cmd_list[i].flags >> 8);
					}
					break;
				}
			}
			if (p_cmd_list[i].func == NULL)
			{
				if (debug == 2)
					printf("wenv_arg:%s\n",arg);
				free(cmd_buff);
				return !(errnum = ERR_BAD_ARGUMENT);
			}
			ret = p_cmd_list[i].func(arg,flags);
		}
		if (status == STATUS_ELSE || wenv_flags)
			break;
		if (status == STATUS_NONE)
			continue;
		errnum = 0;
		if (status == STATUS_QUEUE || ((status == STATUS_AND) && ret) || ((status == STATUS_OR) && ret == 0))
			continue;
		p = skip_next_next_cmd(p,&status,STATUS_ELSE);
	}
	free(cmd_buff);
	return ret;
}

static int set_func(char *arg,int flags)
{
	if(*arg < 'A')
		return get_env_all();
	#ifdef DEBUG
	printf("wenv_set:%s:\n",arg);
	#endif
	int i;
	char var[MAX_VAR_LEN+1] = "\0";
	char value[MAX_ENV_LEN + 1] = "\0";
	
	for(i=0; i<MAX_VAR_LEN && *arg; ++i)
	{
		if( *arg == '=' 
		  || *arg < '0'
		  || (*arg > '9' && *arg != '_' && (unsigned char)((*arg | 0x20) - 'a') > 25)) //无效变量名
			break;
		#if 0
		if (! (*arg == '_'
			|| (*arg >= '0' && *arg <= '9')
			|| (*arg >= 'a' && *arg <= 'z')
			|| (*arg >= 'A' && *arg <= 'Z'))) //变量名只能使用字母,数字或下划线组成.
		{
			if( debug > 0 )
			{
				printf(" VARIABLE illegally\n\n");
				wenv_help_ex(1);
			}
			return 0;
		}
		#endif
		var[i] = *arg++;
	}
	while(*arg == ' ') arg++;//过滤空格
	if (*arg == '\0') //后面没有"="
	{
		return envi_cmd(var,NULL,2);
	}
	if (*arg != '=')//变量后面不是'='
	{
		return wenv_help_ex(1);
	}

	while (*++arg == ' ')
		;

	if(! *arg)
		return set_envi(var, NULL); //值为空清除变量
	int ucase=0;
	if (*arg=='$')
	{
		if ((arg[1] | 32) == 'u')
		{
			ucase = 1;
			arg +=3 ;
			while (*arg == ',' || *arg == ' ' || *arg == '\t') arg++;
		}
		else if ((arg[1] | 32) == 'l')
		{
			ucase = 2;
			arg +=3 ;
			while (*arg == ',' || *arg == ' ' || *arg == '\t') arg++;
		}
	}
	//memset(value, 0, MAX_ENV_LEN);
	if (grub_memcmp(arg, "$input,", 7) == 0)
	{
		arg += 7;
		struct get_cmdline_arg get_cmdline_str;
		get_cmdline_str.prompt = arg;
		get_cmdline_str.maxlen = MAX_ENV_LEN;
		get_cmdline_str.cmdline = value;
		get_cmdline_str.echo_char = 0;
		get_cmdline_str.readline = 1;
		if (get_cmdline_obsolete(get_cmdline_str))
			return 0;
		arg = value;
	}

	if (ucase == 1)
		upper(arg);
	else if (ucase == 2)
		lower(arg);
	if (debug > 0)
		printf("%s=%s\n",var,arg);
	if( set_envi(var, arg) )
	{
		return 1;
	}
	else
		return 0;
}
static int get_func(char *arg,int flags)
{
	int i;
	char t_var[MAX_VAR_LEN+1] = "\0";
	if(*arg < '?')
		return get_env_all();
	for (i=0;i<MAX_VAR_LEN;i++)
	{
		if ((arg[i] == '=') || (arg[i] < '0')) break;
		t_var[i] = arg[i];
	}
	if (arg[i] == '*') //显示包含指定字符的变量
	return envi_cmd(t_var,NULL,2);
	if (arg[i] && arg[i] != '=' && arg[i] != ' ') return 0;//超长
	char tmp_buf[MAX_ENV_LEN+1] = "\0";//变量缓存
	if(get_env(t_var, tmp_buf))
	{
		arg=skip_next(1,(char *)arg);
		if (*arg)
		{
			unsigned long long addr;
			if (! safe_parse_maxint(&arg,&addr))
				errnum = 0;
			else
			{
				return ascii_unicode(tmp_buf,(char *)(grub_size_t)addr);
			}
		}
		if (debug > 1)
			printf("%s = %s",t_var,tmp_buf);
		sprintf(tmp_buf,"%d",strlen(tmp_buf));
		set_envi ("?_GET",tmp_buf);
		return 1;
	}
	return 0;
}

static int call_func(char *arg,int flags)
{
	int ret;
	ret = builtin_cmd(arg, skip_next (0, arg), flags);
	sprintf(ENVI[_WENV_], "0x%X", ret);
	return ret;
}

static int reset_func(char *arg,int flags)
{
	if (*arg == '\0') return reset_env_all();
	int i = -1;
	#if 0
	char t_var[MAX_VAR_LEN+1] = "\0";
	for(i=0; i<MAX_VAR_LEN; ++i)
	{
		if (*arg < '0')
			break;
		t_var[i] = *arg++;
	}
	#endif
	i += strlen(arg);
	if (i >= MAX_VAR_LEN)//输入的变量名超过8个退出.
	{
		if(debug>0)
			printf(" VARIABLE illegally\n\n");
		return 0;
	}
	if (arg[i] == '*') //按前缀清除
	{
		int j, count=0;
		for(j=0; j < MAX_USER_VARS && VAR[j][0] != '\0'; j++)
		{
			if (VAR[j][0] == '@') //已经删除的
				continue;
			if (memcmp(VAR[j], arg , i) == 0)//匹配成功作删除标记
			{
				VAR[j][0] = '@';
				++count;
			}
		}
		if (debug > 0)
		printf("\treset counts = %d", count);
		return 1;
	}
	else
	{
		return set_envi(arg, NULL);
	}
}

static int check_func(char *arg,int flags)
{
	unsigned int op=-1; // 0:==, 1:<>, 2:>=, 3:<=, 判断优先级从小到大
	int i;
	char *p1,*p2;
	#ifdef DEBUG
		printf("check:%s\n",arg);
	#endif
	p1 = arg;
	p2 = p1;
	while(*p2)
	{
		if (*p2 == QUOTE_CHAR)
		{
			while (*++p2 && *p2 != QUOTE_CHAR)
				;
			if (*p2 == QUOTE_CHAR)
				p2++;
			else
				break;
		}
		if (*p2 == ' ') //碰到空格就截断。要比较空格必须放在""里面
		{
			*p2++ = '\0';
		}
		switch(*(unsigned short*)p2)
		{
			case 0x3D3D://==
				op = 0;
				break;
			case 0x3E3C://<>
				op = 1;
				break;
			case 0x3D3E://>=
				op = 2;
				break;
			case 0x3D3C://<=
				op = 3;
				break;
			case 0x2D3C://<-
				op = 4;
				break;
			default:
				p2++;
				continue;
		}
		*p2++ = '\0'; //有找到比较操作符了，直接截断。
		break;
	}
	if (op == -1)
	{
		errnum = ERR_BAD_ARGUMENT;
		return wenv_help_ex(7);
	}
	/*
	for (i = 1;*(p2-i) == ' ';i++)	// 滤掉操作符前面的空格
	{
		;
	}
	*(p2-i+1) = '\0';//截断
	//i = p2 - p1 - i;
	p2++;
	*/
	while(*++p2 == ' ')	// 滤掉操作符后面的空格
	{
		;
	}

	arg = skip_next(SKIP_FLAGS_TERMINATE, p2);
	//*(arg-1) = '\0';

//	trim_p(&p1);
//	trim_p(&p2);
	#ifdef DEBUG
		printf("%s <==> %s\n",p1,p2);
	#endif
	long long v1 = 0LL,v2=0LL;
	if(op == 4)
	{
		if( p1 = strstr(p2, p1))
		sprintf(ENVI[_WENV_], "0x%X", (p1 == NULL)? -1 : (int)(p1-p2));
	}
	else if ( !read_val(&p1,&v1) || !read_val(&p2,&v2) ) // 读数字失败就按字符串比较
	{
		v1 = 0LL;
		v2 = grub_memcmp (p2, p1, 0);
	}
	/* 不需要，因为read_val函数就已经过滤掉空格了
	else
	{
		while (*p2 == ' ') p2++;
	}
	*/
	errnum = 0; //清除错误信息
	switch(op)
	{
		case 0:
			i = (v1 == v2);
			break;
		case 1:
			i = (v1 != v2);
			break;
		case 2:
			i = (v1 >= v2);
			break;
		case 3:
			i = (v1 <= v2);
			break;
		case 4:
			i = (p1 != NULL);
			break;
	}
	if (*arg == '\0' || !i) //如果后面没有参数或者返回真为假，直接返回。
		return i;
	return wenv_func(arg,flags);
}

static int replace(char *str ,const char *sub,const char *rep)
{
	int istr;
	int isub;
	int irep = 0;
	int i;
	if (str == NULL || sub == NULL)
		return 0;
	istr = strlen(str);
	isub = strlen(sub);
	if (isub > istr || istr >= MAX_ENV_LEN)
		return 0;
	if (rep != NULL)
		irep = strlen(rep);
	char *_buff,*p_buff;
	if ((_buff = malloc(istr)) == NULL)
		return 0;
	p_buff = _buff;
	strcpy(_buff,str);
	istr = 0;
	while (*p_buff)
	{
		if (memcmp(p_buff,sub,isub) == 0)
		{
			if (irep > 0)
			{
				str += strcpyn(str,rep,irep);
			}
			p_buff += isub;
		}
		else
		{
			*str++ = *p_buff++;
		}
	}
	*str = '\0';
	free(_buff);
	return 1;
}
//删除字符串前后的空格和引号
static void trim_p(char **p_str)
{
	char *p=*p_str;
	while (*p == ' ')
		p++;
	*p_str = p;
	while (*p) p++;

	while (*--p == ' ')
		;
	if (*p != QUOTE_CHAR || **p_str != QUOTE_CHAR)
	{
		p++;
	}
	else
	{
		++(*p_str);
	}
	*p = '\0';
	return;
}
static int split_ex(char *str,char *delims,char **result,int tokens)
{
	if (*str == 0 || str == NULL || result == NULL)
		return 0;
	if (delims == NULL || *delims == 0 || tokens == 0)
	{
		*result = str;
		return 1;
	}
	char *p;
	char *s = str;
	p = delims;
	while (*p) //过滤字符前面的分隔符号。
	{
		if (*s == *p)
		{
			*s = '\0';
			while (*++s == *p);
		}
		p++;
	}
	if (tokens & 1) //设置第一个变量
	{
		*result++ = s++;
	}
	tokens >>= 1;
	while (*s)
	{
		p = delims;
		while (*p)//对比每个分隔符
		{
			if (*s == *p)
			{
				*s = '\0';
				while (*++s == *p) //过滤重复符号
					;
				*result = s;
			}
			p++;
		}
		if (*result == s) //有找到分隔符了
		{
			if (tokens & 1)
				*result++;
			else if (tokens == 0)//不再匹配退出
			{
				break;
			}
			tokens >>=1;
		}
		s++;
	}
	return 1;
}
static int for_func(char *arg, int flags)
{
	char *p;
	char *cmd;  //指向do 后面的命令的指针
	char *sub;  //临时变量  $X
	char *check_sub; //()里面的内容
	char command_buff[MAX_ARG_BUFF] = "\0"; //命令行缓存
	char rep[MAX_ENV_LEN];	 //要替换的内容缓存
	int x = 0;		//模式
	int ret = 0;		//命令返回值
	char delims[8] = {' ','\t'};//默认分融符
	char eol[2] = ":\"";
	int tokens = 1;
	if (strcmp_ex(arg,"/l") == 0) //操作符 /l 数字序列
		x = 1;
	else if (strcmp_ex(arg,"/f") == 0) //操作符 /f 读文件
		x = 2;
	else return 0;
	
	arg = skip_next (0,arg);
	if ((x & 2) && *arg == '\"')	//字符串分隔，设定提取参数
	{
		int i;
		arg++;
		while (*arg)
		{
			if (*arg == ' ')
				arg++;
			else if (*arg == '\"')
				break;
			else if (memcmp(arg,"tokens=",7) == 0)
			{
				unsigned char t;
				char *tp;
				arg += 7;
				tokens = 0;
				for(i=0;i<9;i++)
				{
					t = *arg++;
					t -= '1';
					if (t > 8)
						return 0;
					tokens |= 1 << t;
					if (*arg == '-')
					{
						unsigned char tt = *++arg;
						tt -= '1';
						if (tt > 8 || tt<t)
							return 0;
						while(t++<tt)
						{
							tokens |= 1 << t;
						}
						arg++;
					}
					if (*arg == ',')
						arg++;
					else
					{
						break;
					}
				}
				if (tokens == 0)
				{
					return !(errnum = ERR_BAD_ARGUMENT);
				}
			}
			else if (memcmp(arg,"delims=",7) == 0)
			{
				arg += 7;
				delims[0] = '\0';//删除默认分隔符
				delims[1] = '\0';
				for(i=0;i<7 && *arg;i++)
				{
					if (*arg == '\"' || (*arg == ' ' && arg[1] != '\"'))
						break;
					delims[i] = *arg++;
				}
			}
			else if (memcmp(arg,"eol=",4) == 0)
			{
				arg +=4;
				if (*arg == ' ' || *arg == '\"')
					eol[0] = '\0';
				else
					eol[0] = *arg++;
			}
			else if (memcmp(arg,"usebackq",8) == 0)
			{
				arg += 8;
				eol[1] = '`';
			}
			else
			{
				return !(errnum = ERR_BAD_ARGUMENT);
			}
		}

		#ifdef DEBUG
		if (debug >1)
			printf("delims:%s :: eol=%c :: tokens:%x\n",delims,eol,tokens);
		#endif
		if (*arg++ != '\"')
			return !(errnum = ERR_BAD_ARGUMENT);
		arg = skip_next (0,arg);
	}
	if (*arg != '%' || arg[2] != ' ')
		return !(errnum = ERR_BAD_ARGUMENT);
	sub = arg;	//检测临时变量是否正确
	arg = skip_next (0 , arg);
	sub[2] = '\0';
	if (strcmp_ex(arg, "in") != 0)	//关键字in
		return !(errnum = ERR_BAD_ARGUMENT);
	arg = skip_next(0, arg);
	if (*arg++ != '(')
		return !(errnum = ERR_BAD_ARGUMENT);
	while (*arg == ' ') arg++;
	check_sub = arg;
	arg = skip_next(0,arg);
	while (*arg && strcmp_ex(arg,"do") != 0) //关键字do
		arg = skip_next(0,arg);
	if (*arg == 0)
		return !(errnum = ERR_BAD_ARGUMENT);
	/*检测')'*/
	cmd = arg;
	while (*--cmd == ' ');
	if (*cmd-- != ')')
		return !(errnum = ERR_BAD_ARGUMENT);
	if (*check_sub == eol[1]) *cmd = 0;
	cmd = skip_next(0,arg);	//设置要执行命令开始的指针
	if (*cmd == '\0') return !(errnum = ERR_BAD_ARGUMENT);
	
	if (x == 1) //for /l
	{
		long long start;
		long long step;
		long long end;
		if (read_val(&check_sub,&start) == 0)
			return 0;
		while (*check_sub++ != ',');
		if (read_val(&check_sub,&step) == 0 || step == 0)
			return 0;
		while (*check_sub++ != ',');
		if (read_val(&check_sub,&end) == 0)
			return 0;
		for (;(step>0)?start<=end:start>=end; start += step)
		{
			strcpyn(command_buff,cmd,MAX_ENV_LEN);
			sprintf(rep,"%ld",start);
			if (replace (command_buff,sub,rep) == 0)
				return 0;
			ret = wenv_func(command_buff ,flags);
			if (wenv_flags == WENV_FLAGS_BREAK)
				break;
			if (errnum)
				return !errnum;
		}
	}
	 else	if (x == 2) //for /f
	{
		char *f_buf = (char*)0x610000; //默认文件读取缓存位置
		if (*check_sub == eol[1])
			f_buf = ++check_sub;
		else
		{
			if (! open(check_sub))
				return 0;
			if (read((unsigned long long)(grub_size_t)f_buf,0x100000,GRUB_READ) != filemax)
				return 0;
			close();
			f_buf[filemax] = 0;
		}
		if( *f_buf == eol[0] )
			f_buf = skip_next (eol[0] | SKIP_FLAGS_LINE,f_buf);
		char *p1;
		while ((p1 = f_buf))
		{
			char *s[10] = {NULL};	//临时变量，当前允许9个.%i %j %k
			int i;
			char t_sub[3] = {'%',sub[1]};
			//t_sub[1] = sub[1];
			f_buf = skip_next (eol[0] | SKIP_FLAGS_LINE,p1);
			#ifdef DEBUG
				printf("debug: %s\n",p1);
			#endif
			strcpyn(command_buff,cmd,MAX_ENV_LEN); //复制命令到缓冲区，因为要对命令进行字符串替换
			split_ex(p1,delims,s,tokens);//分隔字符串.
			for (i = 0;i<10;i++)
			{
				#ifdef DEBUG
				if (debug > 1)
					printf("debug:%d:%s\n",i,s[i]);
				#endif
				trim_p(&s[i]);
				if (replace (command_buff,t_sub,s[i]) == 0)
					return 0;
				t_sub[1]++;//下一个临时变量字符
			}
			#ifdef DEBUG
				printf("cmd: %s\n",command_buff);
			#endif
			ret = wenv_func(command_buff ,flags);
			if (wenv_flags == WENV_FLAGS_BREAK)
				break;
			if (errnum)
				return !errnum;
		}
	}
	wenv_flags = 0;
	return ret;
}
static int echo_func(char *arg, int flags)
{
	int i = 512;
	int echo_cr = 1;
	int echo_ec = 0;
	for (;;)
	{
		if (strcmp_ex(arg,"-n") == 0)
			echo_cr = 0;
		else if (strcmp_ex(arg,"-e") == 0)
			echo_ec = 1;
		else
			break;
		arg = skip_next (0,arg);
	}
	
	if (echo_ec)
		i = parse_string(arg); //格式化字符串
	printfn(arg,i);
	if (echo_cr)
	{
		putstr("\r\n");
	}
	return 1;
}
static int wenv_help_ex(enum ENUM_CMD cmd)
{
	switch(cmd)
	{//set get reset calc read run check for
		case CMD_HELP:  // information
			printf(" Using variables in GRUB4DOS, Compiled time: %s %s\n", __DATE__, __TIME__);
		case VAR_TIP:
			printf("\tVARIABLE is made of characters \"_/A-Z/a-z/0-9\"\n");
			printf("\tmax length=8, and the first character is not 0-9\n\n");
			if(cmd != CMD_HELP) break;
		case CMD_SET: // set
			printf(" WENV SET VARIABLE=[$<U|L>,] [$input,] STRING\n");
			printf("      SET VARIABLE=[[$<U|L>, ] <[[STRING] [${VARIABLE}]] | [*ADDR]>]\n");
			printf("      SET [PREFIX[*]]\n");
			if(cmd != CMD_HELP) break;
		case CMD_GET:   // get
			printf(" WENV GET [VARIABLE] | [PREFIX[*]]\n");
			if(cmd != CMD_HELP) break;
		case CMD_RESET: // reset
			printf(" WENV RESET [VARIABLE] | [PREFIX[*]]\n");
			if(cmd != CMD_HELP) break;
		case CMD_CALC:  // calc
			printf(" WENV CALC <${VAR}|*ADDR> [=<${VAR}|[*]INT...> [OPERATOR <${VAR}|[*]INT...>]]\n");
			if(cmd != CMD_HELP) break;
		case CMD_READ:  // read
			printf(" WENV READ <FILE>\n");
			if(cmd != CMD_HELP) break;
		case CMD_CALL:   // run
			printf(" WENV CALL <grub4dos-builtincmd | ${VARIABLE} | *ADDR>\n");
			printf(" WENV EXEC <command | ${VARIABLE} | *ADDR>\n");
			if(cmd != CMD_HELP) break;
		case CMD_CHECK: // check
			printf("\n WENV CHECK <string|${VAR}|[*]INTEGER> compare-op <string|${VAR}|[*]INTEGER>\n");
			printf("\tcompare-op support ==,<>,>=,<=\n");
			if(cmd != CMD_HELP) break;
		case CMD_FOR:	//for
			printf(" WENV FOR /L %ci IN (start,step,end) DO wenv-command\n",'%');
			printf("      FOR /F [\"options\"] %ci IN ( file ) DO wenv-command\n",'%');
			printf("      FOR /F [\"options\"] %ci IN (\"string\") DO wenv-command\n",'%');
			if(cmd != CMD_HELP) break;
		case CMD_ECHO:	//echo
			printf(" WENV ECHO [string]\n");
			if(cmd != CMD_HELP) break;
		default:
			printf("\nFor more information:  http://chenall.net/tag/grub4dos\n");
			printf("to get source and bin: http://grubutils.googlecode.com\n");
	}
	return 0;
}
static int var_expand(char **arg, char **out)
{
	if (**arg != '{' || arg == NULL)
		return 0;
	char ch[MAX_VAR_LEN + 1] = {0};
	char value[MAX_ENV_LEN + 1] = {0};
	char *p = *arg;
	int i;
	long long start = 0LL;//提取字符起始位置
	long long len = 0xFFFFFFFFULL;//提取字符串长度
	int str_flag = 0;
	p++;
	for (i=0;i<MAX_VAR_LEN+1;i++)
	{
		if (*p == '}') //提取变量成功.
		{
			break;
		}
		else if (*p == ':') //截取指定长度字符串关键字符
		{
			p++;
			if (! safe_parse_maxint (&p, &start)) //起始位置
			{
				break;//不符合要求,则不处理.如果后面刚好是一个"}"则相当于过滤掉了":"
			}
			if (*p == ':' && (p++,! safe_parse_maxint (&p, &len)))//长度,非必要的
			{
				break;//同上
			}
			break;//不再判断,直接退出.
		}
		else if (*p == '#')
		{
			if (*++p == '#')
			{
				p++;
				str_flag = 3;
			}
			else
				str_flag = 2;
			//if (*p == '*') p++;
			start = (long long)(grub_size_t)p;//设定起始位置(保存的是一个指针数值)
			len = 0;//字符串长度,后面需要跳过这些字符.
			while (*p && *p != '}')//跳过后面的字符
			{
				len++;
				p++;
			}
			break;
		}
		else if (*p == '%')
		{
			if (*++p == '%')
			{
				str_flag = 4;
				p++;
			}
			else
				str_flag = 5;
			start = (long long)(grub_size_t)p;//设定起始位置(保存的是一个指针数值);
			while (*p && *p != '}') p++;//跳过后面的字符
			//if (*(p-1) == '*') *(p-1) = '\0'; //截断
			break;
		}
		else if (*p < '0' || *p > 'z' || i >= MAX_VAR_LEN) //非法字符或变量超过8个字符
		{
			break;
		}
		ch[i] = *p++;
	}
	errnum = 0;
	if (*p == '$' && p[1] == '{')
	{
		char *p1 = value;
		i = p - *arg;
		strcpyn(p1,*arg,i);
		p1 += i;
		p++;
		if (var_expand(&p, &p1))
		{
			i = 1;
			while (i && *++p)
			{
				if (*p == '{')
					i++;
				if (*p == '}')
					i--;
				*p1++ = *p;
			}
			*arg = p;
			p1 = value;
			return var_expand(&p1,out);
		}
	}
	
	char _tmp[MAX_ENV_LEN] = "\0";
	char *rep = NULL;
	char *sub = _tmp;
	if (*p == '!')
	{
		int quote = 0;
		p++;
		while (*p != '}' && (*sub++ = *p++) )
		{
			if (*p == '=')
			{
				rep = sub++;
				p++;
			}
		}
		if (rep == NULL)
			return 0;
		*rep++ = 0;
		sub = _tmp;
	}
	else if (*p == ',')
	{
		if (*++p == '?')
		{
			str_flag = 8;
			p++;
		}
	}
	if (*p != '}')
	{
		return 0;
	}
	*arg = p;
	value[0] = '\0';//简易初始化.
	if (get_env(ch,value) || (str_flag & 8))
	{
		if (str_flag & 8)
		{
			i = strlen(value);
			sprintf(value,"%d",i);
			p = value;
		}
		else if (str_flag)
		{
			**arg = '\0';
			p = strstrn(value, (char *)(grub_size_t)start, str_flag & 1);
			**arg = '}';
			if (p == NULL)
				return 1;
			if (str_flag & 4)
			{
				len = p - value;
				p = value;
			}
			else
			{
				p += len;
				len = 0xFFFFFFFF;
			}
		}
		else if (rep != NULL)
		{
			replace(value,sub,rep);
			p = value;
		}
		else 
		{
			int str_len = strlen(value);
			if (start < 0LL) start += str_len;//确定起始位置
			if (len < 0LL) len += str_len - start;//确定结束位置
			p = value + start;
		}
		#ifdef DEBUG
		if (debug > 1)
			printf("%s\n",p);
		#endif
		while (*p && len >0)
		{
			*(*out)++ = *p++;
			len--;
		}
	}
	#ifdef DEBUG
	if (debug > 1)
	printf("%s = %s\n",ch,value);
	#endif
	return 1;
}
static int replace_str(char *in, char *out, int flags)
{
	unsigned int max_len = flags & 0xFFFF;
	if (max_len == 0)
		max_len = MAX_ENV_LEN;
	if (flags & REPLACE_LTRIM)
		while (*in == ' ') in++;//忽略前面的空格

	int quote = 0;
	if ((flags & REPLACE_TRIM_QUOTE) && *in == QUOTE_CHAR)
	{
		quote = 1;
		in++;
	}

	char *po = out;
	char *p = in;
	int i;
	unsigned long long addr;
	char ch[MAX_VAR_LEN + 1]="\0";
	char value[MAX_ENV_LEN + 1];
	while (*in && out - po <= max_len)
	{
		switch(*(p = in))
		{
			case '*':
				p++;
				if (! safe_parse_maxint (&p, &addr) || *p != '$')
				{
					errnum = 0;
					*out++ = *in++;
					continue;
				}
				in = ++p;
				p = (char *)(unsigned long)addr;
				while (*p && *p != '\r' && *p != '\n') *out++ = *p++;
				break;
			case '$':/*替换变量,如果这个变量不符合不替换保持原来的字符串*/
				p++;
				if (*p == '$') //两个$$只留下一个
				{
					in++;
					*out++ = *in++;
					continue;
				}
				if (var_expand(&p,&out))
					in = ++p;
				else
					*out++ = *in++;
				break;
			default:/* 默认不替换直接复制字符*/
				if (*p == QUOTE_CHAR && *p == p[1] ) //两个引号,替换为1个.
					in++;
				*out++ = *in++;
				break;
		}
	}
	out--;
	if (flags & REPLACE_RTRIM)
	{
		while(*out == ' ') //删除后面的空格
		{
			out--;
		}
	}

	if (!(quote && *out == QUOTE_CHAR))
		out++;
	*out = '\0'; //截断

	return 1;
}
static int strcpyn(char *dest,const char *src,int n)
{
#ifdef DEBUG
	if( NULL == dest || NULL == src || n <= 0
		|| dest >= src && (grub_size_t)(dest-src) < n
		|| dest <  src && (grub_size_t)(src-dest) < n )
	{
		printf("strcpyn: bad parameter\n");
		return;
	}
#endif
	const char *p = src;
	while(*src && n--)
	{
		*dest++ = *src++;
	}
	*dest = '\0';
	return (int)(src-p);
}
#if 0
static int envi_cmd(const char *var,char * const env,int flags)
{
#ifdef DEBUG
	if( flags < 0 || flags > 3 )
	{
		printf("envi_cmd: bad parameter\n");
		return 0;
	}
#endif
	//	char ch[MAX_VAR_LEN]="\0";
	if(flags == 3)//重置
	{
		#ifdef DEBUG
		memset((char *)BASE_ADDR,0,MAX_VARS * MAX_VAR_LEN);
		#else
		memset( (char *)BASE_ADDR, 0, MAX_BUFFER );
		#endif
		sprintf(VAR[_WENV_], "?_WENV");
		QUOTE_CHAR = '\"';
		return 1;
	}
	int i, j;
	char ch[MAX_VAR_LEN +1] = "\0"; //使用ch中转可以保证字符截取正确.
	strcpyn(ch,var,MAX_VAR_LEN);
	if (flags == 2)//显示所有变量信息或包含指定字符的变量
	{
		int count=0;
		for(i=0; i < MAX_USER_VARS && VAR[i][0]; ++i)
		{
			if (VAR[i][0] < 'A')
				continue;//非法或已删除变量.
			if (var != NULL && memcmp(VAR[i], ch, MAX_VAR_LEN))//包含特定字符的变量
			{/*
				for (j=0;j<MAX_VAR_LEN && var[j] != '\0';j++)
				{
					if (VAR[i][j] != var[j])
					{
						j = -1;
						break;
					}
				}
				if (j == -1) */continue; //不匹配退出
			}
			++count;
			j = printfn(VAR[i],MAX_VAR_LEN);
			
			#if 0
			for (j=0;j<MAX_VAR_LEN && VAR[i][j];j++)//不能直接PRINTF因为我们的变量索引表之间没有分隔符
					  putchar(VAR[i][j]);
			while(j++<=MAX_VAR_LEN) putchar(' ');
			#endif
			putchar('=');
			j = printfn(ENVI[i],60);
			#if 0
			for (j=0;j<60 && ENVI[i][j];j++)//同样的没有分隔,不能直接PRINTF
					  putchar(ENVI[i][j]);
			#endif
			
			if (ENVI[i][j]) printf(" ...");//如果要显示的字符太长了只显示60个字符.
			putchar('\n');
		}
		if (var != NULL && count == 0)
		{
			printf(" Variable %s not defined\n",var);
			return 0;
		}
		if (debug > 1)
		{
			printf("\t  counts = %d", count);
			if( NULL == var)
				printf("    max = %d", MAX_USER_VARS);
		}
		return 1;
	}
	j = 0xFF;
	/*遍历变量名表
	注: 添加 VAR[i][0] 判断,还有使用删除标记符"@"
		使用得我们不需要遍历整个表.
		//60以上有变量系统保留。
	*/
	if (ch[0]=='@')
	{
//		unsigned long date,time;
//		get_datetime(&date, &time);
    struct grub_datetime datetime;
		get_datetime(&datetime);

		if (memcmp(ch,"@DATE",6) == 0)
		{
//			sprintf(env,"%04X-%02X-%02X",(date >> 16),(char)(date >> 8),(char)date);
      sprintf(env,"%04X-%02X-%02X",datetime.year,datetime.month,datetime.day);
		}
		else if (memcmp(ch,"@TIME",6) == 0)
		{
//			sprintf(env,"%02X:%02X:%02X",(char)(time >> 24),(char)(time >> 16),(datetime.day));
      sprintf(env,"%02X:%02X:%02X",(datetime.hour),(datetime.minute),(datetime.second));
		}
		else if (memcmp(ch,"@RANDOM",8) == 0)
		{
//			WENV_RANDOM   =   (((WENV_RANDOM * time + date ) >> 16) & 0x7fff);
      WENV_RANDOM   =   (((WENV_RANDOM * ((datetime.hour << 16) + (datetime.minute << 8) + datetime.second) 
          (datetime.year << 16) + (datetime.month << 8) + datetime.day) >> 16) & 0x7fff);
			sprintf(env,"%d",WENV_RANDOM);
		}
		else
			return 0;
		return 1;
	}
	for(i=(ch[0]=='?')?60:0;i < MAX_VARS && VAR[i][0];i++)
	{
		if (memcmp(VAR[i], ch, MAX_VAR_LEN) == 0)//找到了
		{
			j = i;//如果完全匹配设置j = 1并退出循环,这时j会等于i,其它情况都不可能相等.
			break;
		}
		if (j == 0xFF && VAR[i][0] == '@') j = i;//记录一个可设变量的位置,如果是添加新的变量时使用.
	}
	
	//读取
	if (flags == 1)
	{//有找到匹配的变量时正常返回,否则返回0
		if (j == i)
		{
			strcpyn(env,ENVI[i],MAX_ENV_LEN);
			return *env;//返回得到的值的第一个字符(如果该变量的值为空则就是返回0);
		}
		return 0;
	}//如果flags = 1到这里就结束了,直接返回,后面不以不需要再判断.
	if (j == 0xFF && i >= MAX_VARS) //没有可用空间也没有找到匹配的记录.
	{
		if( debug > 0 ) // 这里最好给用户一个提示
			printf(" variables max=%d", MAX_USER_VARS);
		return 0;
	}
	if (env == NULL || env[0] == '\0') //env为空,删除变量
	{
		if(j == i)//有找到匹配的记录
		{
			VAR[i][0] = '@';//只简单设置删除标记,不做其它操作
			return 1;//直接返回
			//memset(ENVI[i],0,MAX_ENV_LEN);//清除变量的值.
		}
		return 1; // 变量不存在
	}
	
	if (j != i) //添加新的变量
	{
		i = (j<i?j:i);//优先使用已删除的空间.
		memmove(VAR[i] ,ch ,MAX_VAR_LEN);//添加变量名
	}
	//添加或修改
	memmove(ENVI[i],env,MAX_ENV_LEN);
	return 1;
}
#endif
static int read_val(char **str_ptr,unsigned long long *val)
{
		while (**str_ptr == ' ' || **str_ptr == '\t') (*str_ptr)++;
		char *arg = *str_ptr;
		unsigned long long *val_ptr = val;
		char *str_ptr1 = NULL;
		char *p = arg;
		if (arg[0] == arg[1] && (arg[0] == '+' || arg[0] == '-'))
		{
			str_ptr1 = arg;
			arg += 2;
		}
		if (*arg == '*')
		{
			p = arg;
			arg++;
		}
		
		if (! safe_parse_maxint_with_suffix (&arg, val, 0))
		{
	 return 0;
		}
		
		if (*p == '*')
		{
			val_ptr = (unsigned long long *)(grub_size_t)*val;
			*val = *val_ptr;
			if (str_ptr1 == NULL)
			{
				str_ptr1 = arg;
			}
			if (str_ptr1[0] == str_ptr1[1])
			{
				switch(str_ptr1[0])
				{
					case '+':
						(*val_ptr)++;
						str_ptr1 += 2;
						break;
					case '-':
						(*val_ptr)--;
						str_ptr1 += 2;
						break;
					default:
						break;
				}
			}
			 if (str_ptr1 < arg)//如果是前面的++/--
				*val = *val_ptr;
			 else
				arg = str_ptr1;
		}
		while (*arg == ' ') arg++;
		*str_ptr = arg;
		return (grub_size_t)val_ptr;
}
static int read_envi_val(char **str_ptr,char *envi,unsigned long long *val)
{
	int i;
	char *p = *str_ptr;
	char *p1 = &envi[8];
	char tmp = '\0';
	long long val_tmp = 0LL;
	memset(envi,0,MAX_VAR_LEN + MAX_ENV_LEN + 1);
	while (*p == ' ') p++;
	if (p[0] == p[1] && (p[0] == '+' || p[0] == '-'))
	{
		if (*p == '+')
			val_tmp = 1LL;
		else
			val_tmp = -1LL;
		p += 2;
	}
	if (*p == 0 || *p == '=' || (unsigned char)(*p - '0') < 9)
		return 0;
	for (i=0;i<MAX_VAR_LEN;i++)
	{
		if ((*p == '=') || (*p < 48)) break;
		envi[i] = *p++;
	}
	
	p1 = &envi[8];
	get_env(envi,p1);
	read_val(&p1,val);
	if (val_tmp != 0LL) //计算++X
	{
		*val += val_tmp;
		val_tmp = *val;
		sprintf(p1,"%ld",val_tmp);
		set_envi(envi,p1);
	}
	else if (p[0] == p[1] && (p[0] == '+' || p[0] == '-')) //计算x++
	{
		val_tmp = *val;
		if (*p == '+')
			val_tmp++;
		else
			val_tmp--;
		p += 2;
		sprintf(p1,"%ld",val_tmp);
		set_envi(envi,p1);
	}
	*str_ptr = p;
	return 1;
}
static int calc_func (char *arg,int flags)
{
	#ifdef DEBUG
	if (debug > 1)
		 printf("calc_arg:%s\n",arg);
	#endif
	char var[MAX_VAR_LEN + 1] = "\0";
	char envi[MAX_VAR_LEN + MAX_ENV_LEN + 1] = "\0";
	unsigned long long val1 = 0LL;
	unsigned long long val2 = 0LL;
	unsigned long long *p_result = &val1;
	unsigned long long var_val = 0LL;
//   char *p;
	char O;
	
	if (*arg == '*')
	{
		if ((p_result = (unsigned long long*)(grub_size_t)read_val(&arg, &val1)) == 0)
			return 0;
	}
	else
	{
		if (!read_val(&arg, &val1) && read_envi_val(&arg,envi,&val1) == 0)
		{
			return 0;
		}
	}
	while (*arg == ' ') arg++;
	if (*arg == '=')
	{
		arg++;
		strcpyn(var,envi,8);
		if (! read_val(&arg, &val1) && read_envi_val(&arg,envi,&val1) == 0)
	 return 0;
	}
	else 
	{
		envi[0] = 0;
		p_result = &val1;
	}
	for(;;)
	{
		while (*arg == ' ') arg++;//过滤空格
		if (*arg < 37) break;//非法字符
		val2 = 0ULL;
		O = *arg;
		arg++;
		if (O == '>' || O == '<')
		{
	 if (*arg != O)
		 return 0;
	 arg++;
		}
		
		if (read_val(&arg, &val2) == 0 && read_envi_val(&arg,envi,&val2) == 0)
	 return 0;
		switch(O)
		{
	 case '+':
		 val1 += val2;
		 break;
	 case '-':
		 val1 -= val2;
		 break;
	 case '*':
		 val1 *= val2;
		 break;
	 case '/':
		 val1 = ((long)val1 / (long)val2);
		 break;
	 case '%':
		 val1 = ((long)val1 % (long)val2);
		 break;
	 case '&':
		 val1 &= val2;
		 break;
	 case '|':
		 val1 |= val2;
		 break;
	 case '^':
		 val1 ^= val2;
		 break;
	 case '<':
		 val1 <<= val2;
		 break;
	 case '>':
		 val1 >>= val2;
		 break;
	 default:
		 return 0;
		}
	}
	if (p_result != &val1)
		*p_result = val1;
	if (debug > 1)
	  printf(" %ld (HEX:0x%lX)\n",*p_result,*p_result);
	
	if (var[0] != 0)
	{
		sprintf(&envi[8],"%ld",*p_result);
		set_envi(var,&envi[8]);
	}
	errnum = 0;
	return *p_result;
}
static int ascii_unicode(const char *ch,char *addr)
{
	int i = 0;
	while (*ch)
	{
		sprintf(&addr[i],"\\x%02X\\0",*ch++);
		i += 6;
	}
	sprintf(&addr[i],"\\0\\0\0");
	return 1;
}
// 大小写转换
void case_convert(char *ch,char flag)
{
    if (flag != 'a' && flag != 'A')
        return;
    while (*ch)
    {
        if ((unsigned char)(*ch-flag) < 26)
        {
            *ch ^= 0x20;
        }
        ++ch;
    }
}
#if 0
static void upper(char *string)
{
	while (*string)
	{
		if ((unsigned char)(*string - 'a') < 26) //字符如果在'a'-'z'之间
		{
			*string &= 0xDF;//0XDF=11011111B 把第5位设为0就是大写,好像用位操作可以加快运算速度.
		}
		string++;
	}
	return;
}
static void lower(char *string)
{
	while (*string)
	{
		if ((unsigned char)(*string - 'A') < 26)
		{
			*string |= 32;//32 = 00100000B,使用位操作只需要把第5位设为1就是小写了.
		}
		string++;
	}
	return;
}
#endif
/* 比较字符串前n个字符(s1不分大小写), 相等返回0
 * n>0 时，仅比较前n个字符
 * n=0 时，s1须以结束符、空格、制表符、等号分隔才会返回相等的结果
 */
static int grub_memcmp(const char *s1, const char *s2, int n)
{
	if( s1 == s2 )
		return 1;
	// 基础函数严谨一点比较好
#ifdef DEBUG
	if( NULL == s1 || NULL == s2 || n < 0
		|| s1 > s2 && (int)(s1-s2) < n
		|| s1 < s2 && (int)(s2-s1) < n )
	{
		printf("grub_memcmp: bad parameter\n");
		return;
	}
#endif
	int len = (n>0) ? n : strlen(s2);
	while(len--)
	{
		if  (*s1 == * s2 || ((unsigned char)(*s1 - 'A') < 26 && (*s1 | 32) == *s2))
		{
			s1++;
			s2++;
			continue;
		}
		else return (*s1 > *s2)?1:-1;
	}
		// 前面len字节已匹配，如n=0作额外的结束符检查
	if (n != 0 || (*s1 == '\0' || *s1 == ' ' || *s1 == '\t' || *s1 == '='))
		return 0;
	return 1;
}
/*
	static char *strstrn(const char *s1,const char *s2,const int n)
	功能：从字符串s1中寻找s2的位置（不比较结束符NULL).
	说明：如果没找到则返回NULL。n为0 时返回指向第一次出现s1位置的指针，否则返回最后一次出现的指针.
*/
static char *strstrn(const char *s1,const char *s2,const int n)
{
#ifdef DEBUG
	if( NULL == s1 || NULL == s2 || '\0' == *s1 || '\0' == *s2 )
	{
		printf("strstrn: bad parameters\n");
		return NULL;
	}
#endif
	int len1 = strlen(s1);
	int len2 = strlen(s2);
	if( len1 < len2 )
		return NULL;
	char *s = strstr(s1,s2);
	if( 0 == n )
		return s;
	const char *p = s1 + len1;
	do
	{
		p -= len2;
		if(p < s1)
			p = s1;
	} while( NULL == (s=(strstr(p, s2))) && p > s1 );
	return s;
}

static int printfn(char *str,int n)
{
	int i = 0;
	while (*str && n--)
	{
		putchar(*str++);
		i++;
	}
	return i;
}
/*
static char* next_line(char *arg, char eol)
{
	char *P=arg;
	_next_line:
	while(*P && *P != '\r' && *P != '\n')
		P++;
	if (*P == '\0')
		return P;
	else
		*P = 0;
	while(*++P)
	{
		if (*P == '\n' || *P == '\r' || *P == ' ' || *P == '\t')
			continue;
		else
			break;
	}
	if (*P == eol) //如果是注释符.
		goto _next_line;
	return P;
}
*/
/*从指定文件中读取WENV的命令序列，并依次执行*/
static int read_func(char *arg,int flags)
{
	if(! open(arg))
		return 0;
	if(filemax > 32*1024+512)	// 限制文件大小不能超过32KB+512字节
		return 0;
	//char *P=(char *)0x600000;	// 6M
  char *P = (char *)(grub_size_t)malloc(filemax+512);
  if (!P)
    return 0;

	char command_buff[MAX_ENV_LEN]; //命令行缓存
	if (read((unsigned long long)(grub_size_t)P,-1,GRUB_READ) != filemax)
		goto err;
	close();
	
	P[filemax] = 0;
	char *s[11] = {NULL};
	int i,j;
	s[0] = arg;

	for (i=1;i<10;i++)
	{
		arg = skip_next (SKIP_FLAGS_TERMINATE,arg);
		if (arg == NULL || *arg == 0)
			break;
		s[i] = arg;
	}

	if( *P == ':' )
		 P = skip_next(SKIP_FLAGS_LINE,P);
	int ret;
	char *P1;
	char sub[3] = "$0";
	
	while ((P1 = P))
	{
		P = skip_next(SKIP_FLAGS_LINE,P1);
		strcpy(command_buff,P1);
		for (j=0;j<i;j++)
		{
			sub[1] = j+48;
			if (replace (command_buff,sub,s[j]) == 0)
				goto err;
		}
		ret = wenv_func (command_buff,flags);
		if (errnum)
		{
			if (debug == 2)
				printf("command:%s\n",command_buff);
			break;
		}
	}
  free (P);
	return 1;
  
err:
  free (P);
  return 0;
}
static int show_config(int index)
{
	printf("WENV configure\n");
	if (index & 1)
		printf("QUOTE_CHAR:\t%c\n",QUOTE_CHAR);
	return 1;
}

static int config_func(char *arg, int flags)
{
	if (*arg == '\0' || arg == NULL)
		return show_config(-1);
	if (strcmp_ex(arg,"quote_char") == 0)
	{
		arg = skip_next(1, arg);
		if (*arg)
		{
			QUOTE_CHAR = *arg;
		}
		if (debug)
			show_config(1);
	}
	return 1;
}

static int dir_strstr(char **str1,char **str2)
{
	char *s1 = *str1;
	char *s2 = *str2;
	
	while (*s1)
	{
		s2 = *str2;
		while (*s2 && (*s2 == *s1 || *s2 == '?' || ((unsigned char)(*s1 - 'A') < 26 && (*s1 | 32) == *s2)))
			s1++,s2++;
		if (*s2 == '\0' || *s2 == '*')
		{
			*str1 = s1,*str2 = s2;
			return 1;
		}
		s1++;
	}
	return 0;
}

static int dir_cmp(char *s1,char *s2)
{
	while(*s1)
	{
		if (*s2 == '?' || *s1 == *s2 || ((unsigned char)(*s1 - 'A') < 26 && (*s1 | 32) == *s2))
		{
			s1++;
			s2++;
			continue;
		}
		else if (*s2 == '*')
		{
			s2++;
			if (*s2 == '\0')
				return 1;
			int i = dir_strstr(&s1,&s2);
			if (i == 0)
			{
				return 0;
			}
			continue;
		}
		break;
	}
	return *s1==*s2;
}

static int dir_func(char *arg, int flags)
{
	char cmd_buf[150];
	int i = strlen(arg);
	char *p = arg;

	if (i >= 128)
		return !(errnum = ERR_BAD_ARGUMENT);
	if (i)
	{
		arg += i;
		while (arg != p && *arg != '/')
			arg--;
		if (*arg != '/')
			p += i;
		else
			*arg++= '\0';
	}

	sprintf(cmd_buf,"ls %s/ > (md)0x3000+800",p);
	builtin_cmd(NULL,cmd_buf,flags);

//	p=(char*)0x600000;
  char *P = (char *)(grub_size_t)malloc(filemax+512);
  if (!P)
    return 0;
  
	while (*p == ' ')
		p++;
	char *name;

	i = 0;
	while (*(name = p))
	{
		p = skip_next (SKIP_FLAGS_TERMINATE,p);
//		parse_string(name);
		if (!*arg || dir_cmp(name,arg))
		{
			printf("%s\r\n",name);
			i = 1;
		}
	}
	if (i == 0)
		errnum = ERR_FILE_NOT_FOUND;
  free (P);
	return i;
}

static int exec_func(char *arg, int flags)
{
	int ret;
	ret = builtin_cmd("exec", arg, flags);
	sprintf(ENVI[_WENV_], "0x%X", ret);
	return ret;
}

static int break_func(char *arg,int flags)
{
	return (wenv_flags = WENV_FLAGS_BREAK);
}

static int continue_func(char *arg,int flags)
{
	return (wenv_flags = WENV_FLAGS_CONTINUE);
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
