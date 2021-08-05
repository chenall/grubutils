

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

#define grub_cpuid(num,a,b,c,d) \
  asm volatile ("cpuid" \
                : "=a" (a), "=b" (b), "=c" (c), "=d" (d)  \
                : "0" (num))

//在此定义静态变量、结构。不能包含全局变量，否则编译可以通过，但是不能正确执行。
//不能在main前定义子程序，在main后可以定义子程序。
static grub_size_t g4e_data = 0;
static void get_G4E_image(void);

/* 这是必需的，请参阅grubprog.h中的注释 */
#include <grubprog.h>
/* 请勿在此处插入任何其他asm行 */
//不能包含子程序，不能包含全局变量，否则编译可以通过，但是不能正确执行。
static int main(char *arg,int key);
static int main(char *arg,int key)
{
  unsigned int eax, ebx, ecx, edx, leaf;
  unsigned long long num;
  get_G4E_image();
  if (! g4e_data)
    return 0;
  if (strcmp (arg, "--pae") == 0)
  {
    grub_cpuid (1, eax, ebx, ecx, edx);
    printf ("%s\n", (edx & (1 << 6))? "true" : "false");
    return sprintf (ADDR_RET_STR, "%u", (edx & (1 << 6))? 1 : 0);
  }
  if (strcmp (arg, "--vendor") == 0)
  {
    char vendor[13];
    grub_cpuid (0, eax, ebx, ecx, edx);
    memmove (&vendor[0], &ebx, 4);
    memmove (&vendor[4], &edx, 4);
    memmove (&vendor[8], &ecx, 4);
    vendor[12] = '\0';
    printf ("%s\n", vendor);
    return sprintf (ADDR_RET_STR, "%s", vendor);
  }
  if (strcmp (arg, "--vmx") == 0)
  {
    grub_cpuid (1, eax, ebx, ecx, edx);
    printf ("%s\n", (ecx & (1 << 5))? "true" : "false");
    return sprintf (ADDR_RET_STR, "%u", (ecx & (1 << 5))? 1 : 0);
  }
  if (strcmp (arg, "--hyperv") == 0)
  {
    grub_cpuid (1, eax, ebx, ecx, edx);
    printf ("%s\n", (ecx & (1 << 31))? "true" : "false");
    return sprintf (ADDR_RET_STR, "%u", (ecx & (1 << 31))? 1 : 0);
  }
  if (strcmp (arg, "--vmsign") == 0)
  {
    char vmsign[13];
    grub_cpuid (0x40000000, eax, ebx, ecx, edx);
    memmove (&vmsign[0], &ebx, 4);
    memmove (&vmsign[4], &ecx, 4);
    memmove (&vmsign[8], &edx, 4);
    vmsign[12] = '\0';
    printf ("%s\n", vmsign);
    return sprintf (ADDR_RET_STR, "%s", vmsign);
  }
  if (strcmp (arg, "--brand") == 0)
  {
    char brand[50];
    memset (brand, 0, 50);
    grub_cpuid (0x80000002, eax, ebx, ecx, edx);
    memmove (&brand[0], &eax, 4);
    memmove (&brand[4], &ebx, 4);
    memmove (&brand[8], &ecx, 4);
    memmove (&brand[12], &edx, 4);
    grub_cpuid (0x80000003, eax, ebx, ecx, edx);
    memmove (&brand[16], &eax, 4);
    memmove (&brand[20], &ebx, 4);
    memmove (&brand[24], &ecx, 4);
    memmove (&brand[28], &edx, 4);
    grub_cpuid (0x80000004, eax, ebx, ecx, edx);
    memmove (&brand[32], &eax, 4);
    memmove (&brand[36], &ebx, 4);
    memmove (&brand[40], &ecx, 4);
    memmove (&brand[44], &edx, 4);
    printf ("%s\n", brand);
    return sprintf (ADDR_RET_STR, "%s", brand);
  }
  if (memcmp (arg, "--eax=", 6) == 0)
  {
    char *p = arg + 6;
    if (safe_parse_maxint (&p, &num))
    {
      leaf = (unsigned int) num;
      grub_cpuid (leaf, eax, ebx, ecx, edx);
      return printf ("EAX=0x%08X EBX=0x%08X ECX=0x%08X EDX=0x%08X\n",
                     eax, ebx, ecx, edx);
    }
    return 0;
  }
  printf ("Usage: cpuid [OPTION]\nOptions:\n");
  printf ("  --pae      Check if CPU supports PAE.\n");
  printf ("  --vendor   Get CPU's manufacturer ID string.\n");
  printf ("  --vmx      Check if CPU supports Virtual Machine eXtensions.\n");
  printf ("  --hyperv   Check if hypervisor presents.\n");
  printf ("  --vmsign   Get hypervisor signature.\n");
  printf ("  --brand    Get CPU's processor brand string.\n");
  printf ("  --eax=NUM  Specify value of EAX register.\n");
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
