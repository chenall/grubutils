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
static efi_runtime_services_t *rt = 0;

static grub_size_t g4e_data = 0;
static void get_G4E_image (void);

#include <grubprog.h>

static int main (char *arg,int key);
static int main (char *arg,int key)
{
  efi_uint64_t osis = 0;
  efi_uint64_t osi = 0;
  efi_uintn_t size = sizeof (efi_uint64_t);
  efi_guid_t global = EFI_GLOBAL_VARIABLE_GUID;
  efi_status_t status;

  get_G4E_image ();
  if (! g4e_data)
    return 0;
  st = grub_efi_system_table;
  rt = st->runtime_services;

  /* check whether fwsetup is supported */
  status = rt->get_variable (L"OsIndicationsSupported", &global, NULL, &size, &osis);
  if (status != EFI_SUCCESS)
  {
    printf ("failed to get variable OsIndicationsSupported.\n");
    return 0;
  }
  if (! (osis & EFI_OS_INDICATIONS_BOOT_TO_FW_UI))
  {
    printf ("fwsetup is not supported.\n");
    return 0;
  }
  /* save old OsIndications variable */
  status = rt->get_variable (L"OsIndications", &global, NULL, &size, &osi);
  if (status != EFI_SUCCESS)
    printf ("failed to get variable OsIndications.\n");
  osi |= EFI_OS_INDICATIONS_BOOT_TO_FW_UI;
  /* set OsIndications variable */
  status = rt->set_variable (L"OsIndications", &global,
                             EFI_VARIABLE_NON_VOLATILE |
                             EFI_VARIABLE_BOOTSERVICE_ACCESS |
                             EFI_VARIABLE_RUNTIME_ACCESS, size, &osi);
  if (status != EFI_SUCCESS)
  {
    printf ("failed to set variable OsIndications.\n");
    return 0;
  }
  /* perform cold reset */
//  rt->reset_system (EFI_RESET_WARM, EFI_SUCCESS, 0, NULL);
  rt->reset_system (EFI_RESET_COLD, EFI_SUCCESS, 0, NULL);

  return 1;
}

static void get_G4E_image (void)
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
