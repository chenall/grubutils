/*
 *  MBRCHECK  --  Check sanity of the MBR, running under grub4dos.
 *  Copyright (C) 2011  tinybit(tinybit@tom.com)
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

#include <grub4dos.h>

static int mbrcheck (struct master_and_dos_boot_sector *BS,
                     unsigned long start_sector1,
                     unsigned long sector_count1, unsigned long part_start1);
static char mbr_buf[512];
static unsigned long long L[8];
static unsigned long S[8];
static unsigned long H[8];
static unsigned long C[8];
static unsigned long X;
static unsigned long Y;
static unsigned long Cmax;
static unsigned long Hmax;
static unsigned long Smax;
static unsigned long Lmax;

static grub_size_t g4e_data = 0;
static void get_G4E_image(void);

/* this is needed, see the comment in grubprog.h */
#include <grubprog.h>
/* Do not insert any other asm lines here. */

static int main (char *arg, int key);
static int main (char *arg, int key)
{
  get_G4E_image ();
  if (! g4e_data)
    return 0;
  if (! *arg)
    arg = "(hd0)+1";
  if (! open (arg))
    return 0;
  if (read ((unsigned long long)(grub_addr_t)mbr_buf, 512, GRUB_READ) != 512)
  {
    close ();
    if (errnum == ERR_NONE)
      errnum = ERR_EXEC_FORMAT;
      return 0;
  }
  close ();

  return mbrcheck ((struct master_and_dos_boot_sector *)mbr_buf, 0, 1, 0);
}

/* on call:
 *         BS        points to the bootsector
 *         start_sector1    is the start_sector of the bootimage in the
 *                real disk, if unsure, set it to 0
 *         sector_count1    is the sector_count of the bootimage in the
 *                real disk, if unsure, set it to 1
 *         part_start1    is the part_start of the partition in which
 *                the bootimage resides, if unsure, set it to 0
 *  
 * on return:
 *         0        success
 *        otherwise    failure
 *
 */
static int
mbrcheck (struct master_and_dos_boot_sector *BS,
          unsigned long start_sector1,
          unsigned long sector_count1, unsigned long part_start1)
{
  unsigned long i, j;
  unsigned long lba_total_sectors = 0;
  unsigned long non_empty_entries;
  unsigned long HPC;
  unsigned long SPT;
  unsigned long solutions = 0;
  unsigned long ret_val;
  unsigned long active_partitions = 0;
  unsigned long best_HPC = 0;
  unsigned long best_SPT = 0;
  unsigned long best_bad_things = 0xFFFFFFFF;
  
  /* probe the partition table */
  
  Cmax = 0; Hmax = 0; Smax = 0; Lmax = 0;
  if (filemax < 512)
  {
    printf ("Error: filesize(=%d) less than 512.\n", (unsigned long)filemax);
    return 1;
  }
  /* first, check signature */
  if (BS->boot_signature != 0xAA55)
  {
    printf ("Warning!!! No boot signature 55 AA.\n");
  }

  /* check boot indicator (0x80 or 0) */
  non_empty_entries = 0; /* count non-empty entries */
  for (i = 0; i < 4; i++)
    {
      int *part_entry;
      /* the boot indicator must be 0x80 (bootable) or 0 (non-bootable) */
      if ((unsigned char)(BS->P[i].boot_indicator << 1))/* if neither 0x80 nor 0 */
      {
    printf ("Error: invalid boot indicator(0x%X) for entry %d.\n", (unsigned char)(BS->P[i].boot_indicator), i);
    ret_val = 2;
    goto err_print;
      }
      if ((unsigned char)(BS->P[i].boot_indicator) == 0x80)
    active_partitions++;
      if (active_partitions > 1)
      {
    printf ("Error: duplicate active flag at entry %d.\n", i);
    ret_val = 3;
    goto err_print;
      }
      /* check if the entry is empty, i.e., all the 16 bytes are 0 */
      part_entry = (int *)&(BS->P[i].boot_indicator);
      if (*part_entry++ || *part_entry++ || *part_entry++ || *part_entry)
      {
      non_empty_entries++;
      /* valid partitions never start at 0, because this is where the MBR
       * lives; and more, the number of total sectors should be non-zero.
       */
      if (! BS->P[i].start_lba)
      {
        printf ("Error: partition %d should not start at sector 0(the MBR sector).\n", i);
        ret_val = 4;
        goto err_print;
      }
      if (! BS->P[i].total_sectors)
      {
        printf ("Error: number of total sectors in partition %d should not be 0.\n", i);
        ret_val = 5;
        goto err_print;
      }
      if (lba_total_sectors < BS->P[i].start_lba+BS->P[i].total_sectors)
          lba_total_sectors = BS->P[i].start_lba+BS->P[i].total_sectors;
      /* the partitions should not overlap each other */
      for (j = 0; j < i; j++)
      {
        if ((BS->P[j].start_lba <= BS->P[i].start_lba) && (BS->P[j].start_lba + BS->P[j].total_sectors >= BS->P[i].start_lba + BS->P[i].total_sectors))
        continue;
        if ((BS->P[j].start_lba >= BS->P[i].start_lba) && (BS->P[j].start_lba + BS->P[j].total_sectors <= BS->P[i].start_lba + BS->P[i].total_sectors))
        continue;
        if ((BS->P[j].start_lba < BS->P[i].start_lba) ?
        (BS->P[i].start_lba - BS->P[j].start_lba < BS->P[j].total_sectors) :
        (BS->P[j].start_lba - BS->P[i].start_lba < BS->P[i].total_sectors))
        {
        printf ("Error: overlapped partitions %d and %d.\n", j, i);
        ret_val = 6;
        goto err_print;
        }
      }
      /* the starting cylinder number */
      C[i] = (BS->P[i].start_sector_cylinder >> 8) | ((BS->P[i].start_sector_cylinder & 0xc0) << 2);
      if (Cmax < C[i])
          Cmax = C[i];
      /* the starting head number */
      H[i] = BS->P[i].start_head;
      if (Hmax < H[i])
          Hmax = H[i];
      /* the starting sector number */
      X = ((BS->P[i].start_sector_cylinder) & 0x3f);
      if (Smax < X)
          Smax = X;
      /* the sector number should not be 0. */
      ///* partitions should not start at the first track, the MBR-track */
      if (! X /* || BS->P[i].start_lba < Smax */)
      {
        printf ("Error: starting S of entry %d should not be 0.\n", i);
        ret_val = 7;
        goto err_print;
      }
      S[i] = X;
      L[i] = BS->P[i].start_lba;// - X + 1;
      if (start_sector1 == part_start1)/* extended partition is pretending to be a whole drive */
        L[i] +=(unsigned long) part_start1;
      if (Lmax < L[i])
          Lmax = L[i];
    
      /* the ending cylinder number */
      C[i+4] = (BS->P[i].end_sector_cylinder >> 8) | ((BS->P[i].end_sector_cylinder & 0xc0) << 2);
      if (Cmax < C[i+4])
          Cmax = C[i+4];
      /* the ending head number */
      H[i+4] = BS->P[i].end_head;
      if (Hmax < H[i+4])
          Hmax = H[i+4];
      /* the ending sector number */
      Y = ((BS->P[i].end_sector_cylinder) & 0x3f);
      if (Smax < Y)
          Smax = Y;
      if (! Y)
      {
        printf ("Error: ending S of entry %d should not be 0.\n", i);
        ret_val = 8;
        goto err_print;
      }
      S[i+4] = Y;
      L[i+4] = BS->P[i].start_lba + BS->P[i].total_sectors;
      if (start_sector1 == part_start1)/* extended partition is pretending to be a whole drive */
        L[i+4] +=(unsigned long) part_start1;
      if (L[i+4] < Y)
      {
        printf ("Error: partition %d ended too near.\n", i);
        ret_val = 9;
        goto err_print;
      }
      if (L[i+4] > 0x100000000ULL)
      {
        printf ("Error: partition %d ended too far.\n", i);
        ret_val = 10;
        goto err_print;
      }
      //L[i+4] -= Y;
      //L[i+4] ++;
      L[i+4] --;
      if (Lmax < L[i+4])
          Lmax = L[i+4];
      }
      else
      {
      /* empty entry, zero out all the coefficients */
      C[i] = 0;
      H[i] = 0;
      S[i] = 0;
      L[i] = 0;
      C[i+4] = 0;
      H[i+4] = 0;
      S[i+4] = 0;
      L[i+4] = 0;
      }
    }    /* end for */
  if (non_empty_entries == 0)
  {
    printf ("Error: partition table is empty.\n");
    ret_val = 11;
    goto err_print;
  }

  /* This can serve as a solution if there would be no solution. */
  printf ("Initial estimation: Cmax=%d, Hmax=%d, Smax=%d\n", Cmax, Hmax, Smax);

  /* Try each HPC in Hmax+1 .. 256 and each SPT in Smax .. 63 */

  for (SPT = Smax; SPT <= 63; SPT++)
  {
    for (HPC = (Hmax == 255) ? Hmax : Hmax + 1; HPC <= 256; HPC++)
    {
      unsigned long bad_things = 0;

      /* Check if this combination of HPC and SPT is OK */
      for (i = 0; i < 8; i++)
      {
    if (L[i])
    {
      unsigned long C1, H1, S1;

      /* Calculate C/H/S from LBA */
      S1 = (((unsigned long)L[i]) % SPT) + 1;
      H1 = (((unsigned long)L[i]) / SPT) % HPC;
      C1 = ((unsigned long)L[i]) / (SPT * HPC);
      /* check sanity */
#if 1
      if (C1 <= 1023)
      {
        if (C1 == C[i] && H1 == H[i] && S1 == S[i])
        {
            continue; /* this is OK */
        }
        if (/*C1 > C[i]*/ C1 == C[i]+1 && C[i] == Cmax && L[i] == Lmax && (H1 != HPC-1 || S1 != SPT) && (((H[i] == HPC-1 || (HPC == 255 && H[i] == 255)) && S[i] == SPT) || (H[i] == H1 && S[i] == S1)))
        {
            /* HP USB Disk Storage Format Tool. Bad!! */
            bad_things++;
            continue; /* accept it. */
        }
      }
      else
      {
        if ((((C1 & 1023) == C[i] || 1023 == C[i]) && (S1 == S[i] || SPT == S[i]) && (H1 == H[i] || (HPC-1) == H[i])) || (1023 == C[i] && 255 == H[i] && 63 == S[i]))
        continue; /* this is OK */
      }
#else
      if ((C1 <= 1023) ?
        ((C1 == C[i] && H1 == H[i] && S1 == S[i]) || (/*C1 > C[i]*/ C1 == C[i]+1 && C[i] == Cmax && (((H[i] == HPC-1 || (HPC == 255 && H[i] == 255)) && S[i] == SPT) || (H[i] == H1 && S[i] == S1))) )
        :
        ((((C1 & 1023) == C[i] || 1023 == C[i]) && (S1 == S[i] || SPT == S[i]) && (H1 == H[i] || (HPC-1) == H[i])) || (1023 == C[i] && 255 == H[i] && 63 == S[i])))
        continue; /* this is OK */
#endif
      /* failed, try next combination */
      break;
    }
      }
      if (i >= 8) /* passed */
      {
    solutions++;
    if (HPC == 256)
        bad_things += 16;    /* not a good solution. */
    printf ("Solution %d(bad_things=%d): H=%d, S=%d.\n", solutions, bad_things, HPC, SPT);
    if (bad_things <= best_bad_things)
    {
        best_HPC = HPC;
        best_SPT = SPT;
        best_bad_things = bad_things;
    }
      }
    }
  }
  if (solutions == 0)
  {
    if ((Hmax == 254 || Hmax == 255) && Smax == 63)
    {
      printf ("Partition table is NOT GOOD and there is no solution.\n"
          "But there is a fuzzy solution: H=255, S=63.\n");
      ret_val = 0;    /* partition table probe success */
    }
    else
    {
      printf ("Sorry! No solution. Bad! Please report it.\n");
      ret_val = 12;
    }
  }
  else if (solutions == 1)
  {
    if (best_bad_things == 0)
    {
    printf ("Perfectly Good!\n");
    return 0;    /* partition table probe success */
    }
    else
    {
      printf ("Found 1 solution, but the partition table has problems.\n");
      ret_val = 0;    /* partition table probe success */
    }
  }
  else
  {
    if (best_bad_things == 0)
    {
    printf ("Total solutions: %d (too many). Found a good one:\n"
        , solutions);
    }
    else
    {
    printf ("Total solutions: %d (too many). The best one is:\n"
        , solutions);
    }
    printf ("H=%d, S=%d.\n", best_HPC, best_SPT);
    ret_val = 13;
  }

  /* print the partition table in calculated decimal LBA C H S */
  for (i = 0; i < 4; i++)
  {
    printf ("%10ld %4d %3d %2d    %10ld %4d %3d %2d\n"
        , (unsigned long long)L[i], C[i], H[i], S[i]
        , (unsigned long long)L[i+4], C[i+4], H[i+4], S[i+4]);
  }

err_print:

  /* print the partition table in Hex */
  for (i = 0; i < 4; i++)
  {
    printf ("%02X, %02X %02X %02X   %02X, %02X %02X %02X   %08X   %08X\n"
        , (unsigned char)(BS->P[i].boot_indicator)
        , BS->P[i].start_head
        , (unsigned char)(BS->P[i].start_sector_cylinder)
        , (unsigned char)(BS->P[i].start_sector_cylinder >> 8)
        , (unsigned char)(BS->P[i].system_indicator)
        , BS->P[i].end_head
        , (unsigned char)(BS->P[i].end_sector_cylinder)
        , (unsigned char)(BS->P[i].end_sector_cylinder >> 8)
        , BS->P[i].start_lba
        , BS->P[i].total_sectors);
  }

  return ret_val;
}

static void get_G4E_image (void)
{
  grub_size_t i;

  for (i = 0x40100; i <= 0x9f100 ; i += 0x1000)
  {
    if (*(unsigned long long *)i == 0x4946453442555247)
    {
      g4e_data = *(grub_size_t *)(i+16);
      return;
    }
  }
  return;
}
