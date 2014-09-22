#include "grub4dos.h"
static int inifile(char *section,char *item,char *value);
static unsigned long long pos;
static char *buff;
static int setenvi = 0;
int GRUB = 0x42555247;/* this is needed, see the following comment. */
static char *trim(char *string);
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

static int main (char *arg,int flags)
{
	char *section = NULL;
	char *item = NULL;
	char *value = NULL;
	int ret = 0;
	if (!(buff = malloc(32<<10)))
		return 0;
	if (!open(arg))
		goto quit;
	section = skip_to(0,arg);
	item = strstr(++section,"]");
	if (!item)
		goto quit;
	*item++ = 0;
	item = skip_to(0x200,item);
	if (substring("/B",item,1) != 1)
	{
		setenvi = 1;
		item = skip_to(0x200,item);
	}
	if (value = strstr(item,"="))
	{
		*value++ = 0;
		value = trim(value);
	}
	else
	{
		value=skip_to(0x201,item);
		if (!*value)
			value = NULL;
	}
	ret = inifile(section,item,value);
	quit:
	close();
	free(buff);
	return ret;
}

static char *trim(char *string)
{
	char *s;
	while (*string == ' ' || *string == '\t')
		++string;
	s = string;
	string += strlen(string)-1;
	while (*string == ' ' || *string == '\t')
	{
		--string;
	}
	string[1] = 0;
	return s;
}

static char *check_eof(char *line)
{
	if (line == NULL)
		return NULL;
	char *s1 = line;
	while (*line)
	{
		if (*line == ';')
		{
			*line = 0;
			break;
		}
		if (*line == '\"')
		{
			while (*++line && *line != '\"')
				;
		}
		if (*line)
			++line;
	}
	s1 = trim(s1);
	return *s1?s1:NULL;
}

static int read_line(char *buf)
{
	char *s1 = buf;
	pos = filepos;
	while (read((unsigned long long)(int)buf,1,GRUB_READ) == 1)
	{
		if (*buf == '\r' || *buf == '\n')
		{
			if (s1 == buf)
			{
				pos = filepos;
				continue;
			}
			break;
		}
		else if (!*buf)
		{
			filepos = filemax;
			break;
		}
		++buf;
	}
	*buf = 0;
	return buf - s1;
}

static int find_section(char *section)
{
	static char *s;
	int ret = 0;
	if (!*section)
		return 1;
	while ((read_line(buff)))
	{
		if (*buff != '[')
			continue;
		s = strstr(buff,"]");
		*s = 0;
		s = buff + 1;
		s = trim(s);
		if (substring(section,s,1) == 0)
		{
			ret = 1;
			break;
		}
	}
	return ret;
}

static int inifile(char *section,char *item,char *value)
{
	char *s1,*s2;
	int ret = 0;
	int len;
	int remove_section = (substring(item,"/remove",1) == 0);
	section = trim(section);
	item = trim(item);
	if (remove_section && !*section)
		return 0;
	while (find_section(section))
	{
		if (remove_section)
		{
			filepos = pos;
			ret = read((unsigned long long)(unsigned int)"[*",2,GRUB_WRITE);
			continue;
		}
		while (len = (read_line(buff)))
		{
			if (*buff == '[')
			{
				filepos = pos;
				break;
			}
			s1 = check_eof(buff);
			if (s1)
			{
				s2 = skip_to(0x201,s1);
				if (!*item)
				{
					if (setenvi)
					{
						ret = envi_cmd(s1,s2,0);
					}
					else
						ret = printf("%s=%s\r\n",s1,s2);
				}
				else if (substring(item,s1,1) == 0)
				{
					if (value)
					{
						filepos = pos;
						if (substring(value,"/d",1) == 0)
							ret = read((unsigned long long)(unsigned int)";",1,GRUB_WRITE);
						else
						{
							ret = sprintf(buff,"%s=%s",item,value);
							if (ret > len)
								return 0;
							ret = read((unsigned long long)(unsigned int)buff,ret,GRUB_WRITE);
							if (ret < len)
								read((unsigned long long)(unsigned int)";",1,GRUB_WRITE);
						}
					}
					else if (setenvi)
					{
						ret = envi_cmd(s1,s2,0);
					}
					else
						return printf("%s",s2);
				}
			}
		}
		if (!*section)
			return ret;
	}
	return ret;
}




