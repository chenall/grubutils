1. License:

   This software, called WEE, is based on grub4dos, and licensed under GPLv2.
   See COPYING.

2. Goal:

   This software initially aims to install on the MBR track of the hard drive,
   though also possible to write to ROM by someone who is interested.

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

2. Compile:

	You need to compile under Linux. Just do a "make".

3. Install:

   wee63.mbr contains code that can be used as Master Boot Record.

   How to write wee63.mbr to the Master Boot Track of a hard disk?

   First, read the Windows disk signature and partition information bytes
   (72 bytes in total, from offset 0x01b8 to 0x01ff of the MBR sector), and
   put them on the same range from offset 0x01b8 to 0x01ff of the beginning
   sector of wee63.mbr.

   Optionally, if the MBR in the hard disk is a single sector MBR created by
   Microsoft FDISK, it may be copied onto the second sector of wee63.mbr.

   The second sector of wee63.mbr is called "previous MBR".

   No other steps needed, after all necessary changes stated above have been
   made, now simply write wee63.mbr on to the Master Boot Track. That's all.

   Note: The Master Boot Track means the first track of the hard drive.
   wee63.mbr will occupy a maximum of 63 sectors of the Master Boot Track.

   wee63.mbr can also be directly loaded and run by other boot loaders, such
   as grub4dos and bootmgr. Usually you want to test it this way and make sure
   it works fine. wee63.mbr cannot be directly loaded and run via BOOT.INI of
   NTLDR.

4. Bootup script:

   Before you write wee63.mbr to Master Boot Track, you might want to change
   the boot-up script at the end of wee63.mbr. The boot-up script is just like
   what is called preset-menu in grub4dos. The default script contains only
   one command of "echo weeeeeeee:)". Note that echo is not a builtin command.

   The boot-up script can be increased as you need it, but the whole wee63.mbr
   should not exceed 63 sectors.

   You may bypass the boot-up script by quickly pressing the key C at startup.

   You may single-step trace the boot-up script by quickly pressing the Insert
   key at startup, and you will get the opportunity of step-by-step
   confirmation on each command in the boot-up script.

5. Examples showing the usage of the builtin commands:

   (1) boot the MBR

	(hd0)+1

   (2) boot the bootsector of a partition

	(hd0,0)+1

	or equivalently

	root (hd0,0)
	+1

   (3) find the partition containing ntldr and boot the bootsector

	find --set-root /ntldr
	+1

	or equivalently

	find --set-root /ntldr
	()+1

   (4) find the partition containing ntldr and boot ntldr

	find --set-root /ntldr
	/ntldr

	Note: bootmgr can boot likewise. And so can GRLDR and GRUB.EXE of
	grub4dos.

   (5) boot Ubuntu Linux

	find --set-root /boot/vmlinuz
	/boot/vmlinuz /boot/initrd.img root=UUID=XXXXXXXXXXXXX ro acpi=noirq

	Note: The initrd argument should immediately follow the vmlinuz.
	Like ntldr and others, vmlinuz here is treated as an executable
	program of WEE.

   (6) find the partition containing io.sys and boot io.sys

	find --set-root /io.sys
	/io.sys

	Note: Only IO.SYS of Win9x can be supported. WinMe, MS-DOS 6.22 and all
	others are not bootable.

   (7) find the partition containing kernel.sys and boot kernel.sys

	find --set-root /kernel.sys
	/kernel.sys

	Note: KERNEL.SYS is the kernel of FreeDOS.

   (8) 32-bit programs designed for WEE

	echo Hello World!

	Note: The echo executable for grub4dos runs under WEE as well.

==============================================================================

Update 1 (2011-01-31):

		.com style real mode program support

	The supported real mode program is just like a DOS .com file. Its size
	must be between 512 Bytes and 512 KBytes(inclusive). Note that a DOS
	.com file does not exceed 64KB.

	The file will be loaded at 1000:0100. DOS style command tail is also
	supported. The command-tail count is a byte at 1000:0080. The command-
	tail begins at 1000:0081, ending with CR(0x0D). A word at 1000:0000
	will be cleared to zero. By contrast a DOS PSP will begin with "CD 20".
	DS and ES and CS will be set to 0x1000. IP set to 0x0100. If program
	file length is less than 0xFF00, then SS set to 0x1000 and SP set to
	0xFFFE. If the file length is no less than 0xFF00, then the default
	grub4dos stack(at somewhere between 0000:2000 and 0000:7000) is used.

	The program should not change the content of extended memory.
	The program should not change memory range from 0000:0800 to 0000:FFFF.
	The program should not change BIOS specific memory.

	The program may return to WEE by a far jump to 0000:8200.

	The program file must end with the following 8-byte signature:

			12  05  01  0C  8D  8F  84  85

	No grub4dos API can be used. So the program could only call BIOS.


