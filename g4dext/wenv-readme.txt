                    WENV 使用说明 (readme)
                    
        （由zhaohj起草, chenall 整理于 2010-10-12, tuxw 修改于 2010-10-16）
最近修改： 2010-11-24

WENV 是一个基于GRUB4DOS的程序，可以用它来增强GRUB4DOS的脚本能力,来现一些不可思议的功能。
WENV 最初只支持简单变量和运行GRUB4DOS命令。到了现在已经支持各式各样的处理。

可以把让它当成是GRUB4DOS的增强形SHELL，方便各位grub4dos fans编写高效强大的菜单。

在这里要感谢无论启动论坛上众多朋友的支持，如果没有你们，WENV还只是一个小玩具。
http://bbs.wuyou.com/viewthread.php?tid=159851

取之于网络，共享于网络。

本说明文件来源于无忧论坛zhaohj提供的README.TXT,我经过修改整理而成。
http://bbs.wuyou.com/viewthread.php?tid=159851&page=46#pid2056205

如有发现错误，请通知我，谢谢。

WENV目前支持的命令列表(wenv-commands):

    SET         设置/修改/新增/删除/显示变量
    GET         显示变量/判断变量是否存在
    FOR         类似于CMD的FOR命令，具体见后面介绍
    CALL        执行一个GRUB4DOS内部命令
    CALC        简易计算器（没有优先级从左往右计算，不支持括号）
    ECHO        在屏幕上显示信息
    READ        分析一个文件并执行里面的命令（必须是WENV支持的命令），并且支持参数。
    CHECK       判断/检测相当于if命令，后面可以跟WENV-COMMAND（当判断的返回值为真时执行）
    RESET       清除变量/重置

内置变量:
?_WENV          保存上一次wenv call运行的返回值
?_GET           保存上一次wenv get variable变量的长度
@DATE		当前日期YYYY-MM-DD
@TIME		当前时间hh:mm:ss
@RANDOM		产生一个随机数字

一：SET 设置/修改/新增/删除变量
    
    1.WENV SET [variable=[string]]

    variable  指定环境变量名,区分大小写 （目前最多允许用户定义60个变量）
              不超过8个字符.可以使用字母(a-z/A-Z)、数字(0-9)、下划线(_)。不可以使用数字开头。
    string    指定要指派给变量的一系列字符串。不能超过512字节。
    
    无等号时显示指定名称的所有变量的值。

    SET P
    
    会显示所有以字母 P 打头的变量
    
    2.等号后面可以跟"$U,"或"$L,"
        $U，全转变为大写；
        $L，全转变为小写；
    
        例子: WENV SET a=$U,aBcDeF  得到 a=ABCDEF
    3.关键字 $input，来获取从键盘输入的字符。
        例子：获取从键盘输入的字符串，并转换为大写赋值给srspath
        WENV set srspath=$u,$input,please input SRS driver path:
    
    4.一些组合例子:

        WENV SET variable=$U,string
        WENV SET variable=$U,$input,Prompt
        

二、 GET 显示变量/判断变是是否存在

    GET 命令可以显示指定的变量,并设置?_GET为该变量的字符串长度。
    
    WENV get xxx

    返回变量xxx的值的字符串长度.
    
    注: 如果变量不存在会返回一个失败的值0，可以用于判断变量是否存在
    如: 当变量abc不存在时显示字符串"Variable abc not defined"
        wenv get abc || echo Variable abc not defined

三、FOR 类拟于CMD的for
   比较强大的一个命令，可以分析字符串，文本文件。
   语法:
      FOR /L %variable IN (start,step,end) DO wenv-command
      该集表示以增量形式从开始到结束的一个数字序列。因此，(1,1,5)将产生序列
      1 2 3 4 5，(5,-1,1)将产生序列(5 4 3 2 1)
      
      FOR /F ["options"] %variable IN ( file ) DO wenv-command
      FOR /F ["options"] %variable IN ("string") DO wenv-command
   如果有 usebackq 选项:
      FOR /F ["options"] %variable IN (`string`) DO wenv-command
      options
       eol=c           - 指一个行注释字符的结尾(就一个)
       delims=xxx      - 指分隔符集。这个替换了空格和跳格键的
                         默认分隔符集。
       tokens=x,y,m-n  - 指每行的哪一个符号被传递到每个迭代
                         的 for 本身。这会导致额外变量名称的分配。m-n
                         格式为一个范围。通过 nth 符号指定 mth。
       usebackq        - 反向引号
       
     几个例子 分析myfile.txt的每一行，使用";"作为注释符,使用","分隔字符串，显示每行的第二和第三个字符串
      FOR /F "eol=; tokens=2,3 delims=," %i in ( myfile.txt ) do echo %i %j
      
      显示从1-10的数字
      FOR /L %i in (1,1,10) do echo %i
      
      以下语句会显示 1 2 3 6
      FOR /f "tokens=1-3,6" %i in ("1 2 3 4 5 6 7 8 9 0") do echo %i %j %k %l
      
      可以参考CMD的for命令帮助说明.
四、CALL 命令
    WENV call grub4dos-builtincmd

    CALL 命令用于执行GRUB4DOS内部命令,使得可以动态执行一些GRUB4DOS的命令.
    
    可以使用变量或*ADDR$代替命令的任意部份。
    例子：
    
    wenv set a=(hd0)
    wenv call map ${a} (hd1)
    扩展了变量${a} 相当于执行map (hd0) (hd1)
    wenv call write (fd0)/aa.txt *0x60000$

    这个会扩展*0x60000$,把从内存地址0X60000处的字符复制到命令行
    
    如果内存地址0x60000处有字符串abcdef
    
    那扩展后就是write (fd0)/aa.txt abcdef

    甚至还可以直接 wenv call ${mycmd}
    会扩展变量mycmd，如果它是一个GRUB4DOS内部命令，则会得到执行，否则返回失败值。

    注意：不能用CALL运行无返回值的 Grub4DOS 命令。比如 WENV call configfile ... 可能会出错。
五、calc 命令 用于grub4dos的简单计算，从左到右计算，不支持优先级
    WENV calc [[VAR | [*]INTEGER]=] [*]INTEGER OPERATOR [[*] INTEGER]
    
    calc 命令可以直接操作内存中的数据，请使用*ADDR的形式。

    =前面如果是一个非数字字符串,则会把计算结果赋值给变量；
    OPERATOR:包含+、-、*、/、&(与)、|(或)、^(位异或)、>>、<<
    *INTEGER表示内存地址的值；
    如：
    WENV calc a=*60000+1        表示把内存地址0x60000的值再加1赋值给变量a
    WENV calc a=1<<10           左移10位，即2的10次方，等于1024
    WENV calc a=b++             a=b,b=b+1
    WENV calc a=++b             b=b+1,a=b
    WENV calc a=b--             a=b,b=b-1
    WENV calc a=--b             b=b-1,a=b

    WENV calc a++               变量a的值加1;（如果不存在，会自动增加一个变量a）
    
    注：calc可以对变量进行计算，但一般情况下请使用如下的例子获得更快的运算速度。
    wenv calc a=${b}+${c}
    以上的例子不会对变量b和c进行修改。
    
    wenv calc a=${b}+c++
    这个例子会修改变量c的值

六、echo 命令
    WENV ECHO strings
      默认不使用转义,输入的是什么就输出什么,并且未尾自动加回车符.

     1).-e 允许转义输出.
        比如\n是一个回车。
     2).-n 输出不自动加回车符.    

七、READ 命令
    WENV READ wenv-bat-file parameters
    
    用于执行一个WENV的批处理文件，该批处理文件里面必须是 wenv-commands。
    忽略以冒号":"开头的行。
    
    在批处理中可以使用参数$0-$9
    其中: $0 代表批处理文件自身。$1-$9 分别代表第一个参数到第九个参数。
    
    参数之间使用空格分隔。
    
    一个例子：
    wenv.bat文件内容如下
    echo $0
    echo $1
    echo $2
    使用命令WENV READ /wenv.bat abcd 12345
    将会在屏幕上显示如下内容
    /wenv.bat
    abcd
    12345

八：CHECK 判断命令相当于IF命令
    1.WENV check string1 compare-op string2 wenv-command
    
    比较符号可以使用其中一个: ==、<>、>=、<=
    
    where compare-op may be one of:
    == – equal
    <> – not equal
    <= – less than or equal
    >= – greater than or equal

    注: 注意比较时,后面的字符串忽略小写对比.
    如:
    abc==ABC 值为真
    aBc==abc 值为假
    abc==aBc 值为真.
    只要前面部分某个字符是大写，后面部分对应的一定要大写才能匹配

九、RESET   清除变量
    WENV RESET      清除所有变量
    WENV RESET P    清除变量P
    WENV RESET P*   清除所有P开头的变量
其它说明:

    1、要使用命令序列，可以把要执行的命令放在()之内
       多个命令之间使用" ; "(前后各有一个空格)分隔。

       格式: (wenv-command1 ; wenv-command2 ; wenv-command3)

       应用的例子:
    
                  wenv (set a=1 ; set b=2 ; set c=3)
                  wenv for /l %i in (1,1,5) do (set a=%i ; echo %i)

   2、从内存中复制字符串 *ADDR$
      说明：程序执行时会从内存地址ADDR处复制一个字符串替换命令行的*ADDR$

      例子：显示内存地址0x60000处的字符串信息
      wenv echo *0x60000$
      执行时先复制内存0x60000处的字符串到命令行替换后再执行。

   3、连续运行多条命令,命令与命令之间使用以下字符隔开。
    ]]]  无条件执行。
    ]]&  当前一条命令返回真时执行后面的语句.否则直接返回。
    ]]|  当前一条命令返回假时执行后面的语句.否则直接返回。

通用字符串处理(以下方法可以应用于任何地方)
   
   1: ${VAR:X:Y}
    提取第X个字符后面的Y个字符,如果X为负数则从倒数第X个开始提取Y长度的字符；
    如果Y的值为空则提取第X个字符后面的所有有字符；
    如果Y的值为负数，则去掉倒数Y个字符；
    WENV set a=ABCDabcd1234
    WENV set b=${a:4:4}     得到b=abcd
    WENV set b=${a:-8:4}    得到b=abcd
    WENV set b=${a:4:-4}    得到b=abcd
    WENV set b=${a:-8:-4}   得到b=abcd
    
   2: ${VAR#STRING}	删除STRING前面的字符.
    WENV set a=ABCD;abcd;1234
    WENV set b=${a#;}       得到b=abcd;1234
    
   3: ${VAR##STRING}	删除STRING前面的字符，贪婪模式
    WENV set a=ABCD;abcd;1234
    WENV set b=${a##;}      得到b=1234
    
   4: ${VAR%STRING}	删除STRING后面的字符.
    WENV set a=ABCD;abcd;1234
    WENV set b=${a%;}       得到b=ABCD;abcd
    
   5: ${VAR%%STRING}	删除STRING后面的字符,贪婪模式
    WENV set a=ABCD;abcd;1234
    WENV set b=${a%%;}       得到b=ABCD
   6: 支持嵌套变量。
   像下面的例子：
    wenv echo ${${${a:0:1}:0:1}}

一些应用举例：
        1)根据提示从键盘输入，全转为大写后保存给srspath变量
        WENV set srspath=$u,$input,please input SRS driver path:
        2)设置变量a,并把值全转为大写
        WENV set a=ABCDabcd1234
        WENV set a=$u,${a}
        得到a=ABCDABCD1234
        3)清除某个变量
        WENV set a=        =后为空，清除变量a
        4)清除某些字符开头的所有变量
        WENV reset a*      清除a开头的所有变量
        WENV reset sr*     清除sr开头的所有变量
        5)清除所有变量
        WENV reset
        6)显示所有变量信息(不包括内置变量)
        WENV set
        7)显示所有以prefix开头的变量
        WENV set prefix

wenv call 的例子:
    如：
    WENV set imgpath=/SRS_f6/srs_f6.IMG
    WENV set imgpath=$u,${imgpath}
    WENV call map --mem ${imgpath} (fd0)
    map --hook

    可以动态执行命令，如：
    write (md)0x300+1 map --status\0
    WENV call *0x60000$
    
wenv check 的例子:
    如：WENV set a=abc
        WENV check ${a}==abc && echo ${a}=abc
        WENV calc a=123
        WENV check ${a}==123 && echo ${a}=123
        WENV check ${a}<=150 check ${a}>=100 && echo ${a}>=100.and.${a}<=150
    上面实现了IF语句功能.