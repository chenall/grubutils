/*
 * to get this source code & binary: http://grub4dos-chenall.google.com
 * For more information.Please visit web-site at http://chenall.net
 * 2010-09-22
 */
#include "grub4dos.h"

int GRUB = 0x42555247;/* this is needed, see the following comment. */
/* gcc treat the following as data only if a global initialization like the
 * above line occurs.
 */

asm(".long 0x534F4434");

/* a valid executable file for grub4dos must end with these 8 bytes */
asm(ASM_BUILD_DATE);
asm(".long 0x03051805");
asm(".long 0xBCBAA7BA");

/* thank goodness gcc will place the above 8 bytes at the end of the b.out
 * file. Do not insert any other asm lines here.
 */

#define vga ((unsigned short *)0xb8000)
#define SNAKE 0x2efe
#define SNAKEH 0xAefe
#define FOOD  0xffe
#define LEFT 0x4b00
#define RIGHT 0x4d00
#define DOWN 0x5000
#define UP 0x4800
#define ESC 0x011b
#define PAUSE 0x1970
#define SPEED (s<<4)
#define FRAME_COLOR 0xe
#define snake ((struct snakes *)0x500000)
struct snakes
{
    int pos;
    struct snakes *up;
};//圆形链表.

static struct snakes *snake_head;//指向蛇头的指针(snake_head->up 指向蛇尾).
static int score,level; //分数,等级;
static int add_food(void);//添加食物函数.
static void wait(int seconds);//延时
static int write_xy(int x,int y,int color,char *msg);//直接写屏
static void draw_frame(void);//初始化界面
static int play(void);

static int i,j,z;
static unsigned long s;
static unsigned long speed; //速度控制

int
main()
{
	//获取太概的延时设定.
	unsigned long date,time,time1;
	get_datetime(&date,&time);
	while (get_datetime(&date,&time1),time1 == time);
	for(s=0;s<-1L;s++)
	{
		get_datetime(&date,&time);
		if (time != time1) break;
	}
	s >>= 4;
   	draw_frame();
	write_xy(0,1,7,"  LEVEL 00      SCORE 00000000");
    while(1)
    {
		write_xy(5,14,7,"Press ESC to QUIT");
		write_xy(5,15,7," OR S to Start");
		write_xy(3,18,0xe,"Make by chenall 2010-09-22");
		i = getkey();
		if (i == ESC) break;
		else if (i == 0x1F73) 
		{
			if (play() == 1) write_xy(8,10,0x8f,"Game Over!");
		}
    }
    return 1;

}
static int rand(void)
{
	unsigned long date,time;
	get_datetime(&date,&time);
	time >>= 8;
	time &= 0xff;
	time *= 123;
	return time;
}

static int add_food(void)
{
	int pos;
	struct snakes *snake_;
	char s_buff[10];
	pos = (rand()%19+4)*80+rand()%29+1;

	for (snake_=snake_head->up;snake_ != snake_head;snake_ = snake_->up)
	{
		if (pos == snake_->pos)
		{
			wait(s);
			return add_food();
		}
	}
	vga[pos] = FOOD;
	sprintf(s_buff,"%06d",score+=1);
	write_xy(22,1,13,s_buff);
	if (score%5 == 0)
	{
		sprintf(s_buff,"%02d",level+=1);
		write_xy(8,1,12,s_buff);
		if (speed > s) speed -= s;
	}
	return 0;
}

static int write_xy(int x,int y,int color,char *msg)
{
	int pos = x+y*80;
	while(*msg)
	{
		if (color)
		{
			vga[pos] = (color << 8);
		}
		else
		{
			vga[pos] = vga[pos] & 0xff00;
		}
		vga[pos++] |= *msg++ & 0xff;
	}
	return 1;
}

static void wait(int seconds)
{
    int i;
    unsigned long date,time;
    for(i=0;i< seconds;i++)
    get_datetime(&date,&time);
}

static void draw_frame(void)
{
	for (i=80;i<80*25;i++) vga[i]=0x0720;//清除屏幕.
	//画墙壁(边框);
	for (i=0;i<32;i++) write_xy(i,3,FRAME_COLOR,"\xdc");  //顶部框
	for (i=4;i<25;i++) //中间的框
	{
			write_xy(0,i,FRAME_COLOR,"\xdb");
			write_xy(31,i,FRAME_COLOR,"\xdb");
	}
	for (i=0;i<32;i++) write_xy(i,24,FRAME_COLOR,"\xdf");     //尾部
	write_xy(22,1,13,"00000000");
	write_xy(8,1,12,"00");
	return;
}

static int play(void)
{
    int pos;
    int direction;
    int snake_node;
//    struct snakes snake[128];
    score = level = 0;
    speed = SPEED;
    direction = RIGHT;
    snake_node = 3;
    memset(snake,0,512);
	draw_frame();
    write_xy(2,1,15,"LEVEL");
    write_xy(16,1,15,"SCORE");
    snake[0].pos=14*80+10;
    snake[0].up=&snake[1];
    snake[1].pos=15*80+10;
    snake[1].up=&snake[2];
    snake[2].pos=16*80+10;
    snake[2].up=&snake[0];
    snake_head=&snake[2];
    pos=snake_head->pos;
    add_food();
    while(1)
    {
        wait(speed);
        if (checkkey () >= 0)
        {
         z = getkey();
         if (z == ESC) return -1;
         else if (z == PAUSE)
         {
            write_xy(10,2,0x8E,"Paused");
            while (getkey() != PAUSE);
            write_xy(10,2,0x7,"      ");
         }
         else if (z == 0x1E61)
				add_food();
		else if ((z == DOWN && direction != UP)
            || (z == UP && direction != DOWN)
            || (z == RIGHT && direction != LEFT)
            || (z == LEFT && direction != RIGHT))
        direction = z;
        }
        switch(direction)
        {
            case RIGHT:
                pos++;
                break;
            case UP:
                pos -= 80;
                break;
            case LEFT:
                pos--;
                break;
            case DOWN:
                pos += 80;
                break;
        }

         if (vga[pos] == FOOD)
        {
            snake[snake_node].pos=pos;
            snake[snake_node].up = snake_head -> up;
            snake_head->up = &snake[snake_node];
            vga[snake_head->pos] = SNAKE;
            snake_head = &snake[snake_node];
            vga[pos] = SNAKEH;
            snake_node++;
            if (add_food())
                return 0;
            continue;
        }
        else if ((char)(vga[pos] & 0xff) != 0x20)
        {
            return 1;
        }
        vga[snake_head->pos] = SNAKE;
        snake_head = snake_head->up;
        vga[snake_head->pos] = 0x720;
        snake_head->pos = pos;
        vga[snake_head->pos] = SNAKEH;
    }

}
