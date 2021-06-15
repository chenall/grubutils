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

#define GRUB_ACPI_FADT_SIGNATURE "FACP"

struct grub_acpi_fadt
{
  struct grub_acpi_table_header hdr;
  grub_uint32_t facs_addr;
  grub_uint32_t dsdt_addr;
  grub_uint8_t somefields1[20];
  grub_uint32_t pm1a;
  grub_uint8_t somefields2[8];
  grub_uint32_t pmtimer;
  grub_uint8_t somefields3[32];
  grub_uint32_t flags;
  grub_uint8_t somefields4[16];
  grub_uint64_t facs_xaddr;
  grub_uint64_t dsdt_xaddr;
  grub_uint8_t somefields5[96];
} __attribute__ ((packed));

#define GRUB_ACPI_SLP_EN (1 << 13)
#define GRUB_ACPI_SLP_TYP_OFFSET 10

enum
{
  GRUB_ACPI_OPCODE_ZERO = 0, GRUB_ACPI_OPCODE_ONE = 1,
  GRUB_ACPI_OPCODE_NAME = 8, GRUB_ACPI_OPCODE_ALIAS = 0x06,
  GRUB_ACPI_OPCODE_BYTE_CONST = 0x0a,
  GRUB_ACPI_OPCODE_WORD_CONST = 0x0b,
  GRUB_ACPI_OPCODE_DWORD_CONST = 0x0c,
  GRUB_ACPI_OPCODE_STRING_CONST = 0x0d,
  GRUB_ACPI_OPCODE_SCOPE = 0x10,
  GRUB_ACPI_OPCODE_BUFFER = 0x11,
  GRUB_ACPI_OPCODE_PACKAGE = 0x12,
  GRUB_ACPI_OPCODE_METHOD = 0x14, GRUB_ACPI_OPCODE_EXTOP = 0x5b,
  GRUB_ACPI_OPCODE_ADD = 0x72,
  GRUB_ACPI_OPCODE_CONCAT = 0x73,
  GRUB_ACPI_OPCODE_SUBTRACT = 0x74,
  GRUB_ACPI_OPCODE_MULTIPLY = 0x77,
  GRUB_ACPI_OPCODE_DIVIDE = 0x78,
  GRUB_ACPI_OPCODE_LSHIFT = 0x79,
  GRUB_ACPI_OPCODE_RSHIFT = 0x7a,
  GRUB_ACPI_OPCODE_AND = 0x7b,
  GRUB_ACPI_OPCODE_NAND = 0x7c,
  GRUB_ACPI_OPCODE_OR = 0x7d,
  GRUB_ACPI_OPCODE_NOR = 0x7e,
  GRUB_ACPI_OPCODE_XOR = 0x7f,
  GRUB_ACPI_OPCODE_CONCATRES = 0x84,
  GRUB_ACPI_OPCODE_MOD = 0x85,
  GRUB_ACPI_OPCODE_INDEX = 0x88,
  GRUB_ACPI_OPCODE_CREATE_DWORD_FIELD = 0x8a,
  GRUB_ACPI_OPCODE_CREATE_WORD_FIELD = 0x8b,
  GRUB_ACPI_OPCODE_CREATE_BYTE_FIELD = 0x8c,
  GRUB_ACPI_OPCODE_TOSTRING = 0x9c,
  GRUB_ACPI_OPCODE_IF = 0xa0, GRUB_ACPI_OPCODE_ONES = 0xff
};

enum
{
  GRUB_ACPI_EXTOPCODE_MUTEX = 0x01,
  GRUB_ACPI_EXTOPCODE_EVENT_OP = 0x02,
  GRUB_ACPI_EXTOPCODE_OPERATION_REGION = 0x80,
  GRUB_ACPI_EXTOPCODE_FIELD_OP = 0x81,
  GRUB_ACPI_EXTOPCODE_DEVICE_OP = 0x82,
  GRUB_ACPI_EXTOPCODE_PROCESSOR_OP = 0x83,
  GRUB_ACPI_EXTOPCODE_POWER_RES_OP = 0x84,
  GRUB_ACPI_EXTOPCODE_THERMAL_ZONE_OP = 0x85,
  GRUB_ACPI_EXTOPCODE_INDEX_FIELD_OP = 0x86,
  GRUB_ACPI_EXTOPCODE_BANK_FIELD_OP = 0x87,
};

#define G4D_ARG(x)  (memcmp(arg, x, strlen(x)) == 0 ? 1 : 0)

static void acpi_shutdown (void);

static efi_system_table_t *st;
static grub_size_t g4e_data = 0;
static void get_G4E_image (void);

#include <grubprog.h>

static int main (char *arg,int key);
static int main (char *arg,int key)
{
  get_G4E_image ();
  if (! g4e_data)
    return 0;
  st = grub_efi_system_table;
  if (G4D_ARG ("--help") || G4D_ARG ("/?"))
  {
    printf ("Usage: g4d_off [OPTION]\nOptions:\n");
    printf ("--fw-only   Only use UEFI firmware to shutdown PC.\n");
    printf ("--acpi-fw   After ACPI soft off fails, try UEFI firmware.\n");
    printf ("--help      Display this message.\n");
    printf ("NOTE: as defaut, only use ACPI for shutdown PC.\n");
    printf ("      and all options can only be a single option.\n");
    return 1;
  }
  if (G4D_ARG ("--acpi-fw") || ! G4D_ARG ("--fw-only"))
    acpi_shutdown ();
  if (G4D_ARG ("--acpi-fw") || G4D_ARG ("--fw-only"))
  {
    st->runtime_services->reset_system (EFI_RESET_SHUTDOWN, EFI_SUCCESS, 0, NULL);
    return 0;
  }
  return 0;
}

static inline grub_uint32_t
decode_length (const grub_uint8_t *ptr, int *numlen)
{
  int num_bytes, i;
  grub_uint32_t ret;
  if (*ptr < 64)
  {
    if (numlen)
      *numlen = 1;
    return *ptr;
  }
  num_bytes = *ptr >> 6;
  if (numlen)
    *numlen = num_bytes + 1;
  ret = *ptr & 0xf;
  ptr++;
  for (i = 0; i < num_bytes; i++)
  {
    ret |= *ptr << (8 * i + 4);
    ptr++;
  }
  return ret;
}

static inline grub_uint32_t
skip_name_string (const grub_uint8_t *ptr, const grub_uint8_t *end)
{
  const grub_uint8_t *ptr0 = ptr;

  while (ptr < end && (*ptr == '^' || *ptr == '\\'))
    ptr++;
  switch (*ptr)
  {
    case '.':
      ptr++;
      ptr += 8;
      break;
    case '/':
      ptr++;
      ptr += 1 + (*ptr) * 4;
      break;
    case 0:
      ptr++;
      break;
    default:
      ptr += 4;
      break;
  }
  return ptr - ptr0;
}

static inline grub_uint32_t
skip_data_ref_object (const grub_uint8_t *ptr, const grub_uint8_t *end)
{
  switch (*ptr)
  {
    case GRUB_ACPI_OPCODE_PACKAGE:
    case GRUB_ACPI_OPCODE_BUFFER:
      return 1 + decode_length (ptr + 1, 0);
    case GRUB_ACPI_OPCODE_ZERO:
    case GRUB_ACPI_OPCODE_ONES:
    case GRUB_ACPI_OPCODE_ONE:
      return 1;
    case GRUB_ACPI_OPCODE_BYTE_CONST:
      return 2;
    case GRUB_ACPI_OPCODE_WORD_CONST:
      return 3;
    case GRUB_ACPI_OPCODE_DWORD_CONST:
      return 5;
    case GRUB_ACPI_OPCODE_STRING_CONST:
    {
      const grub_uint8_t *ptr0 = ptr;
      for (ptr++; ptr < end && *ptr; ptr++);
        if (ptr == end)
          return 0;
      return ptr - ptr0 + 1;
    }
    default:
      if (*ptr == '^' || *ptr == '\\' || *ptr == '_'
          || (*ptr >= 'A' && *ptr <= 'Z'))
        return skip_name_string (ptr, end);
      printf ("Unknown opcode 0x%x\n", *ptr);
      return 0;
  }
}

static inline grub_uint32_t
skip_term (const grub_uint8_t *ptr, const grub_uint8_t *end)
{
  grub_uint32_t add;
  const grub_uint8_t *ptr0 = ptr;

  switch(*ptr)
  {
    case GRUB_ACPI_OPCODE_ADD:
    case GRUB_ACPI_OPCODE_AND:
    case GRUB_ACPI_OPCODE_CONCAT:
    case GRUB_ACPI_OPCODE_CONCATRES:
    case GRUB_ACPI_OPCODE_DIVIDE:
    case GRUB_ACPI_OPCODE_INDEX:
    case GRUB_ACPI_OPCODE_LSHIFT:
    case GRUB_ACPI_OPCODE_MOD:
    case GRUB_ACPI_OPCODE_MULTIPLY:
    case GRUB_ACPI_OPCODE_NAND:
    case GRUB_ACPI_OPCODE_NOR:
    case GRUB_ACPI_OPCODE_OR:
    case GRUB_ACPI_OPCODE_RSHIFT:
    case GRUB_ACPI_OPCODE_SUBTRACT:
    case GRUB_ACPI_OPCODE_TOSTRING:
    case GRUB_ACPI_OPCODE_XOR:
      /*
       * Parameters for these opcodes: TermArg, TermArg Target, see ACPI
       * spec r5.0, page 828f.
       */
      ptr++;
      ptr += add = skip_term (ptr, end);
      if (!add)
        return 0;
      ptr += add = skip_term (ptr, end);
      if (!add)
        return 0;
      ptr += skip_name_string (ptr, end);
      break;
    default:
      return skip_data_ref_object (ptr, end);
  }
  return ptr - ptr0;
}

static inline grub_uint32_t
skip_ext_op (const grub_uint8_t *ptr, const grub_uint8_t *end)
{
  const grub_uint8_t *ptr0 = ptr;
  int add;
  switch (*ptr)
  {
    case GRUB_ACPI_EXTOPCODE_MUTEX:
      ptr++;
      ptr += skip_name_string (ptr, end);
      ptr++;
      break;
    case GRUB_ACPI_EXTOPCODE_EVENT_OP:
      ptr++;
      ptr += skip_name_string (ptr, end);
      break;
    case GRUB_ACPI_EXTOPCODE_OPERATION_REGION:
      ptr++;
      ptr += skip_name_string (ptr, end);
      ptr++;
      ptr += add = skip_term (ptr, end);
      if (!add)
    return 0;
      ptr += add = skip_term (ptr, end);
      if (!add)
    return 0;
      break;
    case GRUB_ACPI_EXTOPCODE_FIELD_OP:
    case GRUB_ACPI_EXTOPCODE_DEVICE_OP:
    case GRUB_ACPI_EXTOPCODE_PROCESSOR_OP:
    case GRUB_ACPI_EXTOPCODE_POWER_RES_OP:
    case GRUB_ACPI_EXTOPCODE_THERMAL_ZONE_OP:
    case GRUB_ACPI_EXTOPCODE_INDEX_FIELD_OP:
    case GRUB_ACPI_EXTOPCODE_BANK_FIELD_OP:
      ptr++;
      ptr += decode_length (ptr, 0);
      break;
    default:
      printf ("Unexpected extended opcode: 0x%x\n", *ptr);
      return 0;
  }
  return ptr - ptr0;
}

static int
get_sleep_type (grub_uint8_t *table, grub_uint8_t *ptr, grub_uint8_t *end,
                grub_uint8_t *scope, int scope_len)
{
  grub_uint8_t *prev = table;

  if (!ptr)
    ptr = table + sizeof (struct grub_acpi_table_header);
  while (ptr < end && prev < ptr)
  {
    int add;
    prev = ptr;
    switch (*ptr)
    {
      case GRUB_ACPI_OPCODE_EXTOP:
        ptr++;
        ptr += add = skip_ext_op (ptr, end);
        if (!add)
          return -1;
        break;
      case GRUB_ACPI_OPCODE_CREATE_DWORD_FIELD:
      case GRUB_ACPI_OPCODE_CREATE_WORD_FIELD:
      case GRUB_ACPI_OPCODE_CREATE_BYTE_FIELD:
      {
        ptr += 5;
        ptr += add = skip_data_ref_object (ptr, end);
        if (!add)
          return -1;
        ptr += 4;
        break;
      }
      case GRUB_ACPI_OPCODE_NAME:
        ptr++;
        if ((!scope || memcmp ((void *)scope, "\\", scope_len) == 0) &&
            (memcmp ((void *)ptr, "_S5_", 4) == 0 || memcmp ((void *)ptr, "\\_S5_", 4) == 0))
        {
          int ll;
          grub_uint8_t *ptr2 = ptr;
          ptr2 += skip_name_string (ptr, end);
          if (*ptr2 != 0x12)
          {
            printf ("Unknown opcode in _S5: 0x%x\n", *ptr2);
            return -1;
          }
          ptr2++;
          decode_length (ptr2, &ll);
          ptr2 += ll;
          ptr2++;
          switch (*ptr2)
          {
            case GRUB_ACPI_OPCODE_ZERO:
              return 0;
            case GRUB_ACPI_OPCODE_ONE:
              return 1;
            case GRUB_ACPI_OPCODE_BYTE_CONST:
              return ptr2[1];
            default:
              printf ("Unknown data type in _S5: 0x%x\n", *ptr2);
              return -1;
          }
        }
        ptr += add = skip_name_string (ptr, end);
        if (!add)
          return -1;
        ptr += add = skip_data_ref_object (ptr, end);
        if (!add)
          return -1;
        break;
      case GRUB_ACPI_OPCODE_ALIAS:
        ptr++;
        /* We need to skip two name strings */
        ptr += add = skip_name_string (ptr, end);
        if (!add)
          return -1;
        ptr += add = skip_name_string (ptr, end);
        if (!add)
          return -1;
        break;

      case GRUB_ACPI_OPCODE_SCOPE:
      {
        int scope_sleep_type;
        int ll;
        grub_uint8_t *name;
        int name_len;

        ptr++;
        add = decode_length (ptr, &ll);
        name = ptr + ll;
        name_len = skip_name_string (name, ptr + add);
        if (!name_len)
          return -1;
        scope_sleep_type = get_sleep_type (table, name + name_len,
                           ptr + add, name, name_len);
        if (scope_sleep_type != -2)
          return scope_sleep_type;
        ptr += add;
        break;
      }
      case GRUB_ACPI_OPCODE_IF:
      case GRUB_ACPI_OPCODE_METHOD:
      {
        ptr++;
        ptr += decode_length (ptr, 0);
        break;
      }
      default:
        printf ("Unknown opcode 0x%x\n", *ptr);
        return -1;
    }
  }
  return -2;
}

static void
acpi_shutdown (void)
{
  struct grub_acpi_rsdp_v20 *rsdp2 = NULL;
  struct grub_acpi_rsdp_v10 *rsdp1 = NULL;
  struct grub_acpi_table_header *rsdt;
  grub_uint32_t *entry_ptr;
  grub_uint32_t port = 0;
  int sleep_type = -1;
  unsigned i;
  efi_packed_guid_t acpi_guid = EFI_ACPI_TABLE_GUID;
  efi_packed_guid_t acpi20_guid = EFI_ACPI_20_TABLE_GUID;

  for (i = 0; i < st->num_table_entries; i++)
  {
    efi_packed_guid_t *guid = &st->configuration_table[i].vendor_guid;
    if (! memcmp ((void *)guid, (void *)&acpi20_guid, sizeof (efi_packed_guid_t)))
    {
      rsdp2 = st->configuration_table[i].vendor_table;
      break;
    }
  }

  if (rsdp2)
    rsdp1 = &(rsdp2->rsdpv1);
  else
  {
    for (i = 0; i < st->num_table_entries; i++)
    {
      efi_packed_guid_t *guid = &st->configuration_table[i].vendor_guid;
      if (! memcmp ((void *)guid, (void *)&acpi_guid, sizeof (efi_packed_guid_t)))
      {
        rsdp1 = st->configuration_table[i].vendor_table;
        break;
      }
    }
  }
  if (!rsdp1)
  {
    printf ("ACPI shutdown not supported.\n");
    return;
  }

  rsdt = (struct grub_acpi_table_header *) (grub_addr_t) rsdp1->rsdt_addr;
  for (entry_ptr = (grub_uint32_t *) (rsdt + 1);
       entry_ptr < (grub_uint32_t *) (((grub_uint8_t *) rsdt) + rsdt->length);
       entry_ptr++)
  {
    if (memcmp ((void *) (grub_addr_t) *entry_ptr, "FACP", 4) == 0)
    {
      struct grub_acpi_fadt *fadt
        = ((struct grub_acpi_fadt *) (grub_addr_t) *entry_ptr);
      struct grub_acpi_table_header *dsdt
        = (struct grub_acpi_table_header *) (grub_addr_t) fadt->dsdt_addr;
      grub_uint8_t *buf = (grub_uint8_t *) dsdt;

      port = fadt->pm1a;

      if (memcmp ((void *)dsdt->signature, "DSDT", sizeof (dsdt->signature)) == 0
          && sleep_type < 0)
        sleep_type = get_sleep_type (buf, NULL, buf + dsdt->length, NULL, 0);
    }
    else if (memcmp ((void *) (grub_addr_t) *entry_ptr, "SSDT", 4) == 0
             && sleep_type < 0)
    {
      struct grub_acpi_table_header *ssdt
        = (struct grub_acpi_table_header *) (grub_addr_t) *entry_ptr;
      grub_uint8_t *buf = (grub_uint8_t *) ssdt;

      sleep_type = get_sleep_type (buf, NULL, buf + ssdt->length, NULL, 0);
    }
  }

  if (port && sleep_type >= 0 && sleep_type < 8)
  {
    asm volatile ("outw %w0,%w1" : :"a" ((unsigned short int)
        (GRUB_ACPI_SLP_EN | (sleep_type << GRUB_ACPI_SLP_TYP_OFFSET))),
        "Nd" ((unsigned short int) (port & 0xffff)));
  }

  st->boot_services->stall (1000000);

  printf ("ACPI shutdown failed\n");
}

static void get_G4E_image (void)
{
  grub_size_t i;

  for (i = 0x9F100; i >= 0; i -= 0x1000)
  {
    if (*(unsigned long long *)i == 0x4946453442555247)
    {
      g4e_data = *(grub_size_t *)(i+16);
      return;
    }
  }
  return;
}
