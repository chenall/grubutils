wee 是一个微型的grub4dos用于安装到硬盘mbr上，可以用来代替之前的grldr.mbr方案。

支持的文件系统： FAT12/16/32/NTFS EXT2/3/4

weesetup是为了方便安装wee63.mbr而写的程序。支持自定义脚本。

注意： 如果你的bios不支持EBIOS请不要安装（一般情况下较新的电脑都会支持），具体原因请看下面的WEE说明。

   WEE access disk sectors only using EBIOS(int13/AH=42h), and never using
   CHS mode BIOS call(int13/AH=02h). So, if the BIOS does not support EBIOS
   on a drive, then WEE will not be able to access that drive.

   WEE supports FAT12/16/32, NTFS and ext2/3/4, and no other file systems are
   supported.

   WEE can boot up IO.SYS(Win9x), KERNEL.SYS(FreeDOS), VMLINUZ(Linux), NTLDR/
   BOOTMGR(Windows), GRLDR(grub4dos). And GRUB.EXE(grub4dos) is also bootable
   because it is of a valid Linux kernel format.

   Any single sector boot record file(with 55 AA at offset 0x1FE) can boot
   as well.

   Besides, WEE can run 32-bit programs written for it.
2011-03-07
	1.升级内置wee63.mbr到最新版。
	2.修改默认的内置脚本菜单。

2011-02-28
	1.升级内置wee63.mbr到最新版.

2011-02-17
	1.升级内置wee63.mbr到最新版
	2.添加-l参数，显示磁盘列表。
	3.阻止在使用了fbinst的磁盘上使用该程序。
	

weesetup v1.2.
Usage:
        weesetup [OPTIONS] DEVICE

OPTIONS:
        -i wee63.mbr            Use a custom wee63.mbr file.

        -o outfile              Export new wee63.mbr to outfile.

        -s scriptfile           Import script from scriptfile.

        -m mbrfile              Read mbr from mbrfile(must use with option -o).

        -f                      Force install.
        -u                      Update.
        -b                      Backup mbr to second sector(default is nt6mbr).
	-l			List all disks in system and exit.

Report bugs to website:
        http://code.google.com/p/grubutils/issues

Thanks:
        wee63.mbr (minigrub for mbr by tinybit)
                http://bbs.znpc.net/viewthread.php?tid=5838
        wee63setup.c by SvOlli,xdio.c by bean


wee 安装程序v1.2
用法:
        weesetup [参数] 磁盘
参数:
        -i wee63.mbr            使用外置的wee63.mbr。

        -o outfile              导出新的WEE63.MBR文件到outfile.

        -s scriptfile           导入wee脚本文件.

        -m mbrfile              从mbrfile获取mbr信息（必须配合参数-o使用）.

        -f                      强制安装.
	-u			更新wee.
        -b                      备份旧的mbr到第二扇区（默认不备份，而是直接使用内置的nt6mbr）.
	-l			显示所在硬盘列表

请到以下网址报告BUG:
        http://code.google.com/p/grubutils/issues

感谢:
        wee63.mbr (minigrub for mbr by tinybit)
                http://bbs.znpc.net/viewthread.php?tid=5838
        wee63setup.c by SvOlli,xdio.c by bean

更多资料请访问我的搏客:
	http://chenall.net/post/weesetup/

编译方法：

1.首先从SVN下载GRUBUTILS源码
svn co svn://svn.gna.org/svn/grubutil grubutil

2.在grubutil目录下新建一个子目录weesetup，然后把weesetup的源码复制过去.

在weesetup目录下执行make，就可以在bin目录中生成一个weesetup.exe

或直接从googlecode上下载。
svn co http://grubutils.googlecode.com/svn/grubutils grubutils

cd grubutils
cd weesetup
make


附:内置的wee63.mbr默认脚本内容
find --set-root /boot/grub/grldr
/boot/grub/grldr
timeout 1
default 0

title 1. Windows
find --set-root --active
+1
find --set-root /bootmgr
/bootmgr
find --set-root /ntldr
/ntldr

title 2. GRUB4DOS
find --set-root /BOOT/GRUB/GRLDR
/BOOT/GRUB/GRLDR
find --set-root /BOOT/GRUB.EXE
/BOOT/GRUB.EXE
find --set-root /BOOT/GRLDR
/BOOT/GRLDR
find --set-root /grldr
/grldr

title 3. Plop Boot Manager 
find --set-root /BOOT/GRUB/PLPBT.BIN
/BOOT/GRUB/PLPBT.BIN

title 4. Vboot
find --set-root /vbootldr
/vbootldr

title 5. Burg
find --set-root /buldr
/buldr