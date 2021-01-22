#include <grub4dos.h>

#define grub_cpuid(num,a,b,c,d) \
  asm volatile ("xchgl %%ebx, %1; cpuid; xchgl %%ebx, %1" \
                : "=a" (a), "=r" (b), "=c" (c), "=d" (d)  \
                : "0" (num))

#include <grubprog.h>

int main (char *arg,int key)
{
  unsigned int eax, ebx, ecx, edx, leaf;
  unsigned long long num;
  if (strcmp (arg, "--pae") == 0)
  {
    grub_cpuid (1, eax, ebx, ecx, edx);
    return printf ("%s\n", (edx & (1 << 6))? "true" : "false");
  }
  if (strcmp (arg, "--vendor") == 0)
  {
    char vendor[13];
    grub_cpuid (0, eax, ebx, ecx, edx);
    memmove (&vendor[0], &ebx, 4);
    memmove (&vendor[4], &edx, 4);
    memmove (&vendor[8], &ecx, 4);
    vendor[12] = '\0';
    return printf ("%s\n", vendor);
  }
  if (strcmp (arg, "--vmx") == 0)
  {
    grub_cpuid (1, eax, ebx, ecx, edx);
    return printf ("%s\n", (ecx & (1 << 5))? "true" : "false");
  }
  if (strcmp (arg, "--hyperv") == 0)
  {
    grub_cpuid (1, eax, ebx, ecx, edx);
    return printf ("%s\n", (ecx & (1 << 31))? "true" : "false");
  }
  if (strcmp (arg, "--vmsign") == 0)
  {
    char vmsign[13];
    grub_cpuid (0x40000000, eax, ebx, ecx, edx);
    memmove (&vmsign[0], &ebx, 4);
    memmove (&vmsign[4], &ecx, 4);
    memmove (&vmsign[8], &edx, 4);
    vmsign[12] = '\0';
    return printf ("%s\n", vmsign);
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
    return printf ("%s\n", brand);
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
