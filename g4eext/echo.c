

/*
 * compile:
 * gcc -Wl,--build-id=none -m64 -mno-sse -nostdlib -fno-zero-initialized-in-bss -fno-function-cse -fno-jump-tables -Wl,-N -fPIE g4e_wb.c -o g4e_wb.o 2>&1|tee build.log
 * objcopy -O binary g4e_wb.o g4e_wb
 *
 * analysis:
 * objdump -d g4e_wb.o > g4e_wb.s
 * readelf -a g4e_wb.o > a.s
 */
//

#include <grub4dos.h>

//在此定义静态变量、结构。不能包含全局变量，否则编译可以通过，但是不能正确执行。
//不能在main前定义子程序，在main后可以定义子程序。
static grub_size_t g4e_data = 0;
static void EFIAPI get_G4E_image(void);

/* 这是必需的，请参阅grubprog.h中的注释 */
#include <grubprog.h>
/* 请勿在此处插入任何其他asm行 */
//不能包含子程序，不能包含全局变量，否则编译可以通过，但是不能正确执行。
static int EFIAPI main(char *arg,int key);
static int EFIAPI main(char *arg,int key)
{ 
  get_G4E_image();
  if (! g4e_data)
    return 0;

  return printf ("%s\n",arg);
}

static void EFIAPI get_G4E_image(void)
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
