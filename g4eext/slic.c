/*
 *  G4E external commands
 *  Copyright (C) 2021  a1ive
 *
 *  G4EEXT is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  G4EEXT is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with G4EEXT.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub4dos.h>
#include <uefi.h>

#define GRUB_RSDP_SIGNATURE "RSD PTR "
#define GRUB_RSDP_SIGNATURE_SIZE 8

struct grub_acpi_rsdp_v10
{
  grub_uint8_t signature[GRUB_RSDP_SIGNATURE_SIZE];
  grub_uint8_t checksum;
  grub_uint8_t oemid[6];
  grub_uint8_t revision;
  grub_uint32_t rsdt_addr;
} __attribute__ ((packed));

struct grub_acpi_rsdp_v20
{
  struct grub_acpi_rsdp_v10 rsdpv1;
  grub_uint32_t length;
  grub_uint64_t xsdt_addr;
  grub_uint8_t checksum;
  grub_uint8_t reserved[3];
} __attribute__ ((packed));

struct grub_acpi_table_header
{
  grub_uint8_t signature[4];
  grub_uint32_t length;
  grub_uint8_t revision;
  grub_uint8_t checksum;
  grub_uint8_t oemid[6];
  grub_uint8_t oemtable[8];
  grub_uint32_t oemrev;
  grub_uint8_t creator_id[4];
  grub_uint32_t creator_rev;
} __attribute__ ((packed));

#define SLIC_LENGTH 0x176

static efi_system_table_t *st;

static grub_uint8_t acpi_byte_checksum (void *base, grub_size_t size);
static void *acpi_alloc (efi_uintn_t size);
static void acpi_print_str (const grub_uint8_t *str, grub_size_t n);
static struct grub_acpi_table_header *acpi_find_slic (struct grub_acpi_rsdp_v10 *rsdp);
static struct grub_acpi_rsdp_v10 *acpi_get_rsdpv1 (void);
static struct grub_acpi_rsdp_v20 *acpi_get_rsdpv2 (void);

static grub_size_t g4e_data = 0;
static void get_G4E_image(void);

#include <grubprog.h>

static int main(char *arg,int key);
static int main(char *arg,int key)
{
  grub_uint8_t v2 = 0;
  char slic_oemid[6];
  char slic_oemtable[8];
  grub_uint8_t slic_table[SLIC_LENGTH];
  struct grub_acpi_rsdp_v10 *rsdp = 0;
  struct grub_acpi_rsdp_v20 *rsdp2;
  struct grub_acpi_table_header *slic = 0;
  struct grub_acpi_table_header *rsdt;
  struct grub_acpi_table_header *xsdt;
  get_G4E_image();
  if (! g4e_data)
    return 0;
  st = grub_efi_system_table;
  if (!*arg)
    return printf ("Usage: slic FILE\n");
  open (arg);
  if (errnum)
  {
    printf ("Failed to open %s\n", arg);
    return 0;
  }
  if (filemax != SLIC_LENGTH)
  {
    close ();
    printf ("bad slic table %s length 0x%llx\n", arg, filemax);
    return 0;
  }
  read ((unsigned long long)(grub_size_t)slic_table, SLIC_LENGTH, GRUB_READ);
  close ();
  memmove (slic_oemid, ((struct grub_acpi_table_header *)slic_table)->oemid, 6);
  memmove (slic_oemtable,
           ((struct grub_acpi_table_header *)slic_table)->oemtable, 8);
  printf ("SLIC OEMID ");
  acpi_print_str (((struct grub_acpi_table_header *)slic_table)->oemid, 6);
  printf (" OEMTABLE ");
  acpi_print_str (((struct grub_acpi_table_header *)slic_table)->oemtable, 8);
  printf ("\n");

  rsdp = (struct grub_acpi_rsdp_v10 *) acpi_get_rsdpv2 ();
  if (! rsdp)
    rsdp = acpi_get_rsdpv1 ();
  if (! rsdp)
  {
    printf ("ACPI table not found.\n");
    return 0;
  }
  rsdp2 = (struct grub_acpi_rsdp_v20 *) rsdp;
  v2 = rsdp->revision;
  rsdt = (void *)(grub_addr_t)(rsdp->rsdt_addr);
  if (v2)
    xsdt = (struct grub_acpi_table_header *) (grub_addr_t) rsdp2->xsdt_addr;
  printf ("RSDT @0x%lx OEMID ", (unsigned long)rsdt);
  acpi_print_str (rsdt->oemid, 6);
  printf (" OEMTABLE ");
  acpi_print_str (rsdt->oemtable, 8);
  printf ("\n");

  slic = acpi_find_slic (rsdp);
  if (slic)
  {
    printf ("find slic in acpi table: 0x%lx\n", (unsigned long)slic);
    memmove (slic, slic_table, SLIC_LENGTH);
  }
  else
  {
    struct grub_acpi_table_header *old_rsdt = rsdt;
    slic = acpi_alloc (SLIC_LENGTH);
    if (!slic)
    {
      printf ("out of memory\n");
      return 0;
    }
    memmove (slic, slic_table, SLIC_LENGTH);
    rsdt = acpi_alloc (rsdt->length + 4);
    memmove (rsdt, old_rsdt, old_rsdt->length);

    {
      grub_uint32_t *desc = (grub_uint32_t *) (rsdt + 1);
      grub_uint32_t num = (rsdt->length - sizeof (*rsdt)) / 4;
      rsdt->length += sizeof (*desc);
      desc[num] = (grub_uint32_t)(grub_addr_t)slic;
      rsdp->rsdt_addr = (grub_uint32_t)(grub_addr_t)rsdt;
    }

    if (v2)
    {
      struct grub_acpi_table_header *old_xsdt = xsdt;
      xsdt = acpi_alloc (xsdt->length + 8);
      memmove (xsdt, old_xsdt, old_xsdt->length);

      grub_uint64_t *desc = (grub_uint64_t *) (xsdt + 1);
      grub_uint64_t num = (xsdt->length - sizeof (*xsdt)) / 8;
      xsdt->length += sizeof (*desc);
      desc[num] = (grub_uint32_t)(grub_addr_t)slic;
      rsdp2->xsdt_addr = (grub_uint64_t)(grub_addr_t)xsdt;
    }
  }
  memmove (rsdt->oemid, slic_oemid, 6);
  memmove (rsdt->oemtable, slic_oemtable, 8);
  rsdt->checksum = 0;
  rsdt->checksum = 1 + ~acpi_byte_checksum (rsdt, rsdt->length);
  rsdp->checksum = 0;
  rsdp->checksum = 1 + ~acpi_byte_checksum (rsdp, sizeof (struct grub_acpi_rsdp_v10));
  if (v2)
  {
    memmove (xsdt->oemid, slic_oemid, 6);
    memmove (xsdt->oemtable, slic_oemtable, 8);
    xsdt->checksum = 0;
    xsdt->checksum = 1 + ~acpi_byte_checksum (xsdt, xsdt->length);
    rsdp2->checksum = 0;
    rsdp2->checksum = 1 + ~acpi_byte_checksum (rsdp2, rsdp2->length);
  }

  return 1;
}

static grub_uint8_t
acpi_byte_checksum (void *base, grub_size_t size)
{
  grub_uint8_t *ptr;
  grub_uint8_t ret = 0;
  for (ptr = (grub_uint8_t *) base; ptr < ((grub_uint8_t *) base) + size;
       ptr++)
    ret += *ptr;
  return ret;
}

static void *acpi_alloc (efi_uintn_t size)
{
  void *data = NULL;
  efi_status_t status;
  status = st->boot_services->allocate_pool (EFI_ACPI_RECLAIM_MEMORY, size, &data);
  if (status != EFI_SUCCESS)
    return NULL;
  else
    return data;
}

static void
acpi_print_str (const grub_uint8_t *str, grub_size_t n)
{
  grub_size_t i;
  for (i = 0; i < n; i++)
    printf ("%c", str[i]);
}

static struct grub_acpi_rsdp_v10 *acpi_get_rsdpv1 (void)
{
  unsigned i;
  efi_packed_guid_t acpi_guid = EFI_ACPI_TABLE_GUID;

  for (i = 0; i < st->num_table_entries; i++)
  {
    efi_packed_guid_t *guid = &st->configuration_table[i].vendor_guid;
    if (! memcmp ((void *)guid, (void *)&acpi_guid, sizeof (efi_packed_guid_t)))
      return (struct grub_acpi_rsdp_v10 *) st->configuration_table[i].vendor_table;
  }
  return 0;
}

static struct grub_acpi_rsdp_v20 *acpi_get_rsdpv2 (void)
{
  unsigned i;
  efi_packed_guid_t acpi20_guid = EFI_ACPI_20_TABLE_GUID;

  for (i = 0; i < st->num_table_entries; i++)
  {
    efi_packed_guid_t *guid = &st->configuration_table[i].vendor_guid;
    if (! memcmp ((void *)guid, (void *)&acpi20_guid, sizeof (efi_packed_guid_t)))
      return (struct grub_acpi_rsdp_v20 *) st->configuration_table[i].vendor_table;
  }
  return 0;
}

static struct grub_acpi_table_header *
acpi_find_slic (struct grub_acpi_rsdp_v10 *rsdp)
{
  grub_uint32_t len;
  grub_uint32_t *desc;
  struct grub_acpi_table_header *t;
  t = (void *)(grub_addr_t)(rsdp->rsdt_addr);
  len = t->length - sizeof (*t);
  desc = (grub_uint32_t *) (t + 1);
  for (; len >= sizeof (*desc); desc++, len -= sizeof (*desc))
  {
    t = (struct grub_acpi_table_header *) (grub_addr_t) *desc;
    if (t == NULL)
      continue;
    if (memcmp (t->signature, "SLIC", sizeof (t->signature)) == 0)
      return t;
  }
  return NULL;
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
