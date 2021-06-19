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

#define G4D_ARG(x)  (memcmp(arg, x, strlen(x)) == 0 ? 1 : 0)
#define G4D_VAL(x, y) (strnicmp(x, y, strlen (y)) == 0 ? 1 : 0)

enum var_type
{
  VAR_STR = 0,
  VAR_WSTR,
  VAR_UINT,
  VAR_HEX,
  VAR_INVALID = -1
};

static efi_system_table_t *st = 0;
static efi_runtime_services_t *rt = 0;
static efi_guid_t var_guid = EFI_GLOBAL_VARIABLE_GUID;

static grub_size_t g4e_data = 0;

static int print_help (const char *errmsg);
static unsigned long strtoul (const char *nptr, char **endptr, int base);

static grub_uint8_t *utf16_to_utf8 (grub_uint8_t *dest,
                                    const grub_uint16_t *src, grub_size_t size);

static efi_status_t get_variable (const char *var, const efi_guid_t *guid,
                                  grub_size_t *datasize_out, void **data_out);

static void get_G4E_image (void);

#include <grubprog.h>

static int main (char *arg,int key);
static int main (char *arg,int key)
{
  efi_status_t status;
  enum var_type efi_var_type = VAR_WSTR;
  char *p;
  char *data = NULL, *out = NULL;
  grub_size_t datasize = 0;

  get_G4E_image ();
  if (! g4e_data)
    return 0;
  st = grub_efi_system_table;
  rt = st->runtime_services;

  if (!*arg || G4D_ARG("--help"))
    return print_help (NULL);
  if (G4D_ARG("--type="))
  {
    p = arg + 7;
    if (G4D_VAL (p, "UINT"))
      efi_var_type = VAR_UINT;
    else if (G4D_VAL (p, "WSTR"))
      efi_var_type = VAR_WSTR;
    else if (G4D_VAL (p, "STR"))
      efi_var_type = VAR_STR;
    else
      return print_help ("invalid TYPE");
    arg = skip_to (0, arg);
  }
  if (G4D_ARG("--guid="))
  {
    p = arg + 7;
    if (strlen(p) < 36 ||
        p[8] != '-' || p[13] != '-' || p[18] != '-' || p[23] != '-')
      return print_help ("invalid GUID");
    p[8] = 0;
    var_guid.data1 = strtoul (p, NULL, 16);
    p[13] = 0;
    var_guid.data2 = strtoul (p + 9, NULL, 16);
    p[18] = 0;
    var_guid.data3 = strtoul (p + 14, NULL, 16);
    var_guid.data4[7] = strtoul (p + 34, NULL, 16);
    p[34] = 0;
    var_guid.data4[6] = strtoul (p + 32, NULL, 16);
    p[32] = 0;
    var_guid.data4[5] = strtoul (p + 30, NULL, 16);
    p[30] = 0;
    var_guid.data4[4] = strtoul (p + 28, NULL, 16);
    p[28] = 0;
    var_guid.data4[3] = strtoul (p + 26, NULL, 16);
    p[26] = 0;
    var_guid.data4[2] = strtoul (p + 24, NULL, 16);
    p[23] = 0;
    var_guid.data4[1] = strtoul (p + 21, NULL, 16);
    p[21] = 0;
    var_guid.data4[0] = strtoul (p + 19, NULL, 16);
    arg = p + 36;
    arg = skip_to (0, arg);
  }
  if (!*arg)
    return print_help ("invalid VARIABLE");
  get_variable (arg, &var_guid, &datasize, (void **)&data);
  if (!data || !datasize)
  {
    printf ("ERROR: %s not found.\n", arg);
    return 0;
  }

  switch (efi_var_type)
  {
    case VAR_STR:
      out = zalloc (datasize + 1);
      if (!out)
      {
        printf ("ERROR: out of memory.\n");
        goto fail;
      }
      memmove (out, data, datasize);
      break;
    case VAR_WSTR:
      out = zalloc (datasize / 2 * 3 + 1);
      if (!out)
      {
        printf ("ERROR: out of memory.\n");
        goto fail;
      }
      utf16_to_utf8 ((grub_uint8_t *)out, (grub_uint16_t *)data, datasize);
      break;
    case VAR_UINT:
      out = zalloc (22); // uint64 184,467,440,737,095,551,615 
      if (!out)
      {
        printf ("ERROR: out of memory.\n");
        goto fail;
      }
      if (datasize == 8) // uint64
        sprintf (out, "%llu", (unsigned long long) *(grub_uint64_t *)data);
      else if (datasize == 4) // uint32
        sprintf (out, "%u", *(grub_uint32_t *)data);
      else if (datasize == 2) // uint16
        sprintf (out, "%u", *(grub_uint16_t *)data);
      else
        sprintf (out, "%u", *(grub_uint8_t *)data);
      break;
    default:
      printf ("ERROR: invalid TYPE.\n");
      goto fail;
  }

  printf ("%s\n", out);
  sprintf (ADDR_RET_STR, "%s", out);

  free (data);
  free (out);
  return 1;
fail:
  free (data);
  free (out);
  return 0;
}

static int print_help (const char *errmsg)
{
  if (errmsg)
    printf ("ERROR: %s.\n", errmsg);
  printf ("Usage: getenv [--type=TYPE] [--guid=GUID] EFI_VAR\nOptions:\n");
  printf ("--type=TYPE Parse VARIABLE as specific type.\n");
  printf ("  avaliable types:\n");
  printf ("    UINT    unsigned int\n");
  printf ("    WSTR    wide-character string [default]\n");
  printf ("    STR     string\n");
  printf ("--guid=GUID Specify GUID of VARIABLE to query.\n");
  printf ("  default GUID: 8BE4DF61-93CA-11D2-AA0D-00E098032B8C\n");
  printf ("--help      Display this message.\n");
  return errmsg ? 0 : 1;
}

static unsigned long strtoul (const char *nptr, char **endptr, int base)
{
  unsigned long val = 0;
  int negate = 0;
  unsigned int digit;
  /* Skip any leading whitespace */
  while (isspace (*nptr))
  {
    nptr++;
  }
  /* Parse sign, if present */
  if (*nptr == '+')
  {
    nptr++;
  }
  else if (*nptr == '-')
  {
    nptr++;
    negate = 1;
  }
  /* Parse base */
  if (base == 0)
  {
    /* Default to decimal */
    base = 10;
    /* Check for octal or hexadecimal markers */
    if (*nptr == '0')
    {
      nptr++;
      base = 8;
      if ((*nptr | 0x20) == 'x')
      {
        nptr++;
        base = 16;
      }
    }
  }
  /* Parse digits */
  for (; ; nptr++)
  {
    digit = *nptr;
    if (digit >= 'a')
    {
      digit = (digit - 'a' + 10);
    }
    else if (digit >= 'A')
    {
      digit = (digit - 'A' + 10);
    }
    else if (digit <= '9')
    {
      digit = (digit - '0');
    }
    if (digit >= (unsigned int) base)
    {
      break;
    }
    val = ((val * base) + digit);
  }
  /* Record end marker, if applicable */
  if (endptr)
  {
    *endptr = ((char *) nptr);
  }
  /* Return value */
  return (negate ? -val : val);
}

#define GRUB_UINT8_1_LEADINGBIT 0x80
#define GRUB_UINT8_2_LEADINGBITS 0xc0
#define GRUB_UINT8_3_LEADINGBITS 0xe0
#define GRUB_UINT8_4_LEADINGBITS 0xf0
#define GRUB_UINT8_5_LEADINGBITS 0xf8
#define GRUB_UINT8_6_LEADINGBITS 0xfc
#define GRUB_UINT8_7_LEADINGBITS 0xfe

#define GRUB_UINT8_1_TRAILINGBIT 0x01
#define GRUB_UINT8_2_TRAILINGBITS 0x03
#define GRUB_UINT8_3_TRAILINGBITS 0x07
#define GRUB_UINT8_4_TRAILINGBITS 0x0f
#define GRUB_UINT8_5_TRAILINGBITS 0x1f
#define GRUB_UINT8_6_TRAILINGBITS 0x3f

#define GRUB_MAX_UTF8_PER_UTF16 4
/* You need at least one UTF-8 byte to have one UTF-16 word.
   You need at least three UTF-8 bytes to have 2 UTF-16 words (surrogate pairs).
 */
#define GRUB_MAX_UTF16_PER_UTF8 1
#define GRUB_MAX_UTF8_PER_CODEPOINT 4

#define GRUB_UCS2_LIMIT 0x10000
#define GRUB_UTF16_UPPER_SURROGATE(code) \
  (0xD800 | ((((code) - GRUB_UCS2_LIMIT) >> 10) & 0x3ff))
#define GRUB_UTF16_LOWER_SURROGATE(code) \
  (0xDC00 | (((code) - GRUB_UCS2_LIMIT) & 0x3ff))

/* Process one character from UTF8 sequence. 
   At beginning set *code = 0, *count = 0. Returns 0 on failure and
   1 on success. *count holds the number of trailing bytes.  */
static int
utf8_process (grub_uint8_t c, grub_uint32_t *code, int *count)
{
  if (*count)
  {
    if ((c & GRUB_UINT8_2_LEADINGBITS) != GRUB_UINT8_1_LEADINGBIT)
    {
      *count = 0;
      /* invalid */
      return 0;
    }
    else
    {
      *code <<= 6;
      *code |= (c & GRUB_UINT8_6_TRAILINGBITS);
      (*count)--;
      /* Overlong.  */
      if ((*count == 1 && *code <= 0x1f) || (*count == 2 && *code <= 0xf))
      {
        *code = 0;
        *count = 0;
        return 0;
      }
      return 1;
    }
  }

  if ((c & GRUB_UINT8_1_LEADINGBIT) == 0)
  {
    *code = c;
    return 1;
  }
  if ((c & GRUB_UINT8_3_LEADINGBITS) == GRUB_UINT8_2_LEADINGBITS)
  {
    *count = 1;
    *code = c & GRUB_UINT8_5_TRAILINGBITS;
    /* Overlong */
    if (*code <= 1)
    {
      *count = 0;
      *code = 0;
      return 0;
    }
    return 1;
  }
  if ((c & GRUB_UINT8_4_LEADINGBITS) == GRUB_UINT8_3_LEADINGBITS)
  {
    *count = 2;
    *code = c & GRUB_UINT8_4_TRAILINGBITS;
    return 1;
  }
  if ((c & GRUB_UINT8_5_LEADINGBITS) == GRUB_UINT8_4_LEADINGBITS)
  {
    *count = 3;
    *code = c & GRUB_UINT8_3_TRAILINGBITS;
    return 1;
  }
  return 0;
}

/* Convert a (possibly null-terminated) UTF-8 string of at most SRCSIZE
   bytes (if SRCSIZE is -1, it is ignored) in length to a UTF-16 string.
   Return the number of characters converted. DEST must be able to hold
   at least DESTSIZE characters. If an invalid sequence is found, return -1.
   If SRCEND is not NULL, then *SRCEND is set to the next byte after the
   last byte used in SRC.  */
static grub_size_t
utf8_to_utf16 (grub_uint16_t *dest, grub_size_t destsize,
               const grub_uint8_t *src, grub_size_t srcsize, const grub_uint8_t **srcend)
{
  grub_uint16_t *p = dest;
  int count = 0;
  grub_uint32_t code = 0;

  if (srcend)
    *srcend = src;

  while (srcsize && destsize)
  {
    int was_count = count;
    if (srcsize != (grub_size_t)-1)
      srcsize--;
    if (!utf8_process (*src++, &code, &count))
    {
      code = '?';
      count = 0;
      /* Character c may be valid, don't eat it.  */
      if (was_count)
        src--;
    }
    if (count != 0)
      continue;
    if (code == 0)
      break;
    if (destsize < 2 && code >= GRUB_UCS2_LIMIT)
      break;
    if (code >= GRUB_UCS2_LIMIT)
    {
      *p++ = GRUB_UTF16_UPPER_SURROGATE (code);
      *p++ = GRUB_UTF16_LOWER_SURROGATE (code);
      destsize -= 2;
    }
    else
    {
      *p++ = code;
      destsize--;
    }
  }

  if (srcend)
    *srcend = src;
  return p - dest;
}

static grub_uint8_t *
utf16_to_utf8 (grub_uint8_t *dest, const grub_uint16_t *src, grub_size_t size)
{
  grub_uint32_t code_high = 0;

  while (size--)
  {
    grub_uint32_t code = *src++;

    if (code_high)
    {
      if (code >= 0xDC00 && code <= 0xDFFF)
      {
        /* Surrogate pair.  */
        code = ((code_high - 0xD800) << 10) + (code - 0xDC00) + 0x10000;

        *dest++ = (code >> 18) | 0xF0;
        *dest++ = ((code >> 12) & 0x3F) | 0x80;
        *dest++ = ((code >> 6) & 0x3F) | 0x80;
        *dest++ = (code & 0x3F) | 0x80;
      }
      else
      {
        /* Error...  */
        *dest++ = '?';
        /* *src may be valid. Don't eat it.  */
        src--;
      }
      code_high = 0;
    }
    else
    {
      if (code <= 0x007F)
        *dest++ = code;
      else if (code <= 0x07FF)
      {
        *dest++ = (code >> 6) | 0xC0;
        *dest++ = (code & 0x3F) | 0x80;
      }
      else if (code >= 0xD800 && code <= 0xDBFF)
      {
        code_high = code;
        continue;
      }
      else if (code >= 0xDC00 && code <= 0xDFFF)
      {
        /* Error... */
        *dest++ = '?';
      }
      else if (code < 0x10000)
      {
        *dest++ = (code >> 12) | 0xE0;
        *dest++ = ((code >> 6) & 0x3F) | 0x80;
        *dest++ = (code & 0x3F) | 0x80;
      }
      else
      {
        *dest++ = (code >> 18) | 0xF0;
        *dest++ = ((code >> 12) & 0x3F) | 0x80;
        *dest++ = ((code >> 6) & 0x3F) | 0x80;
        *dest++ = (code & 0x3F) | 0x80;
      }
    }
  }
  return dest;
}

static efi_status_t
get_variable (const char *var, const efi_guid_t *guid,
              grub_size_t *datasize_out, void **data_out)
{
  efi_status_t status;
  efi_uintn_t datasize = 0;
  efi_char16_t *var16;
  void *data;
  grub_size_t len, len16;

  *data_out = NULL;
  *datasize_out = 0;

  len = strlen (var);
  len16 = len * GRUB_MAX_UTF16_PER_UTF8;
  var16 = malloc ((len16 + 1) * sizeof (var16[0]));
  if (!var16)
    return EFI_OUT_OF_RESOURCES;
  len16 = utf8_to_utf16 (var16, len16, (grub_uint8_t *) var, len, NULL);
  var16[len16] = 0;

  status = rt->get_variable (var16, guid, NULL, &datasize, NULL);

  if (status != EFI_BUFFER_TOO_SMALL || !datasize)
  {
    free (var16);
    return status;
  }

  data = malloc (datasize);
  if (!data)
  {
    free (var16);
    return EFI_OUT_OF_RESOURCES;
  }

  status = rt->get_variable (var16, guid, NULL, &datasize, data);
  free (var16);

  if (status == EFI_SUCCESS)
  {
    *data_out = data;
    *datasize_out = datasize;
    return status;
  }

  free (data);
  return status;
}

static void get_G4E_image (void)
{
  grub_size_t i;

  /* search "GRUB4EFI" in 0-0x9ffff */
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
