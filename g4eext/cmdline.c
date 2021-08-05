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

static efi_system_table_t *st = 0;
static efi_handle_t *ih = 0;
static efi_boot_services_t *bs = 0;
static efi_guid_t loaded_image_guid = EFI_LOADED_IMAGE_GUID;
static efi_loaded_image_t *loaded_image = 0;

static grub_size_t g4e_data = 0;

static grub_uint8_t *utf16_to_utf8 (grub_uint8_t *dest,
                                    const grub_uint16_t *src, grub_size_t size);
static void get_G4E_image (void);

#include <grubprog.h>

static int main (char *arg,int key);
static int main (char *arg,int key)
{
  efi_status_t status;
  grub_size_t cmdline_len = 0;
  efi_char16_t *wcmdline = 0;
  unsigned char *cmdline = 0;

  get_G4E_image ();
  if (! g4e_data)
    return 0;
  st = grub_efi_system_table;
  ih = grub_efi_image_handle;
  bs = st->boot_services;

  /* get loaded image */
  status = bs->open_protocol (ih, &loaded_image_guid,
                              (void **)&loaded_image, ih,
                              0, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (status != EFI_SUCCESS)
    return printf_errinfo ("loaded image protocol not found.\n");
  /* UTF-8 U+0000 ~ U+007F 1 byte  UTF-16 U+0000   ~ U+D7FF   2 bytes
   *       U+0080 ~ U+07FF 2 bytes        U+E000   ~ U+FFFF   2 bytes
   *       U+0800 ~ U+FFFF 3 bytes        U+010000 ~ U+10FFFF 4 bytes
   */
  cmdline_len = (loaded_image->load_options_size / 2) * 3 + 1;
  cmdline = zalloc (cmdline_len);
  if (!cmdline)
    printf_errinfo ("out of memory\n");

  wcmdline = loaded_image->load_options;
  utf16_to_utf8 (cmdline, wcmdline, cmdline_len);
  printf ("%s\n", cmdline);
  sprintf (ADDR_RET_STR, "%s", cmdline);
  free (cmdline);
  return 1;
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

static void get_G4E_image (void)
{
  grub_size_t i;

  /* search "GRUB4EFI" in 0-0x9ffff */
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
