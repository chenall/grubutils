#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc,char *argv[])
{
  int i;
  FILE *fp;
  FILE *f_mod = fopen("grub4dos.mod","wb");
  int ret;
  struct
  {
    char filename[12];
    unsigned long size;
  } grub_mod = {"\x05\x18\x05\x03\xBA\xA7\xBA\xBC",0};
  fwrite(&grub_mod,1,16,f_mod);
  char *buff=malloc(0xa000);
  unsigned long n;
  for(i = 1; i < argc; i++)
  {
    if ((fp=fopen(argv[i],"rb"))==NULL)
    {
        printf("Err open file %s\n",argv[i]);
        continue;
    }
    memset(&grub_mod,0,16);
    strncpy(grub_mod.filename,argv[i],11);
    n = fread(buff,1,0xa000,fp);
    fgetc(fp);
    if (feof(fp))
    {
        grub_mod.size=n;
        fwrite(&grub_mod,1,16,f_mod);
        fwrite(buff,1,n,f_mod);
    }
    else
        printf("Err read file %s,%lx\n",argv[i],n);

    fclose(fp);
  }
  fclose(f_mod);
  free(buff);
  return 1;
}


