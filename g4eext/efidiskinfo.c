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

struct usb_dev_request
{
  grub_uint8_t type;
  grub_uint8_t request;
  grub_uint16_t value;
  grub_uint16_t index;
  grub_uint16_t length;
} __attribute__ ((packed));

struct usb_desc_device
{
  grub_uint8_t length;
  grub_uint8_t type;
  grub_uint16_t usbrel;
  grub_uint8_t class;
  grub_uint8_t subclass;
  grub_uint8_t protocol;
  grub_uint8_t maxsize0;
  grub_uint16_t vendorid;
  grub_uint16_t prodid;
  grub_uint16_t devrel;
  grub_uint8_t strvendor;
  grub_uint8_t strprod;
  grub_uint8_t strserial;
  grub_uint8_t configcnt;
} __attribute__ ((packed));

struct usb_desc_config
{
  grub_uint8_t length;
  grub_uint8_t type;
  grub_uint16_t totallen;
  grub_uint8_t numif;
  grub_uint8_t config;
  grub_uint8_t strconfig;
  grub_uint8_t attrib;
  grub_uint8_t maxpower;
} __attribute__ ((packed));

struct usb_desc_if
{
  grub_uint8_t length;
  grub_uint8_t type;
  grub_uint8_t ifnum;
  grub_uint8_t altsetting;
  grub_uint8_t endpointcnt;
  grub_uint8_t class;
  grub_uint8_t subclass;
  grub_uint8_t protocol;
  grub_uint8_t strif;
} __attribute__ ((packed));

struct usb_desc_endp
{
  grub_uint8_t length;
  grub_uint8_t type;
  grub_uint8_t endp_addr;
  grub_uint8_t attrib;
  grub_uint16_t maxpacket;
  grub_uint8_t interval;
} __attribute__ ((packed));

typedef enum
{
  EFI_USB_DATA_IN,
  EFI_USB_DATA_OUT,
  EFI_USB_NO_DATA,
} efi_usb_data_direction;

typedef efi_status_t EFIAPI (*efi_async_usb_transfer_callback)
    (void *data, efi_uintn_t len, void *context, efi_uint32_t status);

struct efi_usb_io
{
  // IO transfer
  efi_status_t EFIAPI (*control_transfer) (struct efi_usb_io *this,
                                    struct usb_dev_request *request,
                                    efi_usb_data_direction direction,
                                    efi_uint32_t timeout,
                                    void *data,
                                    efi_uintn_t len,
                                    efi_uint32_t *status);

  efi_status_t EFIAPI (*bulk_transfer) (struct efi_usb_io *this,
                                 efi_uint8_t dev_endpoint,
                                 void *data,
                                 efi_uintn_t len,
                                 efi_uint32_t timeout,
                                 efi_uint32_t *status);

  efi_status_t EFIAPI (*async_interrupt_transfer) (struct efi_usb_io *this,
                        efi_uint8_t dev_endpoint,
                        efi_boolean_t is_new_transfer,
                        efi_uintn_t polling_interval,
                        efi_uintn_t len,
                        efi_async_usb_transfer_callback interrupt_call_back,
                        void *context);

  efi_status_t EFIAPI (*sync_interrupt_transfer) (struct efi_usb_io *this,
                                           efi_uint8_t dev_endpoint,
                                           void *data,
                                           efi_uintn_t *len,
                                           efi_uintn_t timeout,
                                           efi_uint32_t *status);

  efi_status_t EFIAPI (*isochronous_transfer) (struct efi_usb_io *this,
                                        efi_uint8_t dev_endpoint,
                                        void *data,
                                        efi_uintn_t len,
                                        efi_uint32_t *status);

  efi_status_t EFIAPI (*async_isochronous_transfer) (struct efi_usb_io *this,
                        efi_uint8_t dev_endpoint,
                        void *data,
                        efi_uintn_t len,
                        efi_async_usb_transfer_callback isochronous_call_back,
                        void *context);

  // Common device request
  efi_status_t EFIAPI (*get_device_desc) (struct efi_usb_io *this,
                                   struct usb_desc_device *device_desc);

  efi_status_t EFIAPI (*get_config_desc) (struct efi_usb_io *this,
                                   struct usb_desc_config *config_desc);

  efi_status_t EFIAPI (*get_if_desc) (struct efi_usb_io *this,
                               struct usb_desc_if *if_desc);

  efi_status_t EFIAPI (*get_endp_desc) (struct efi_usb_io *this,
                                 efi_uint8_t endpoint_index,
                                 struct usb_desc_endp *endp_desc);

  efi_status_t EFIAPI (*get_str_desc) (struct efi_usb_io *this,
                                efi_uint16_t lang_id,
                                efi_uint8_t string_id,
                                efi_char16_t **string);

  efi_status_t EFIAPI (*get_supported_lang) (struct efi_usb_io *this,
                                      efi_uint16_t **lang_id_table,
                                      efi_uint16_t *table_size);

  // Reset controller's parent port
  efi_status_t EFIAPI (*port_reset) (struct efi_usb_io *this);
};
typedef struct efi_usb_io efi_usb_io_t;

#define LANG_ID_ENGLISH 0x0409

static efi_system_table_t *st = 0;
static efi_handle_t *ih = 0;
static efi_boot_services_t *bs = 0;
static efi_guid_t bio_guid = EFI_BLOCK_IO_GUID;
static efi_guid_t dp_guid = EFI_DEVICE_PATH_GUID;
static efi_guid_t usb_guid = EFI_USB_IO_PROTOCOL_GUID;

static grub_size_t g4e_data = 0;

static int print_help (void);
static char *wcstostr (const efi_char16_t *str);
static void get_G4E_image (void);

enum cmd_arg
{
  CMD_ARG_PRINT,
  CMD_ARG_RM,
  CMD_ARG_RO,
  CMD_ARG_BS,
  CMD_ARG_USB,
  CMD_ARG_USB_SPEC,
  //CMD_ARG_USB_VENDOR,
  //CMD_ARG_USB_PROD,
};

#include <grubprog.h>

static int main (char *arg,int key);
static int main (char *arg,int key)
{
  efi_status_t status;
  struct grub_disk_data *d = NULL;
  efi_usb_io_t *usb_io = NULL;
  struct usb_desc_device dev_desc;
  efi_char16_t *str16;
  char *str;

  enum cmd_arg prog_arg = CMD_ARG_PRINT;

  get_G4E_image ();
  if (! g4e_data)
    return 0;
  if (*(unsigned int *)IMG(0x8278) < 20220315)
  {
    printf("Please use grub4efi version above 2022-03-15.\n");
    return 0;
  }
  st = grub_efi_system_table;
  ih = grub_efi_image_handle;
  bs = st->boot_services;

  if (!*arg || G4D_ARG("--help"))
    return print_help ();
  if (G4D_ARG("--rm"))
  {
    prog_arg = CMD_ARG_RM;
    arg = skip_to (0, arg);
  }
  else if (G4D_ARG("--ro"))
  {
    prog_arg = CMD_ARG_RO;
    arg = skip_to (0, arg);
  }
  else if (G4D_ARG("--bs"))
  {
    prog_arg = CMD_ARG_BS;
    arg = skip_to (0, arg);
  }
  else if (G4D_ARG("--usb-drive"))
  {
    prog_arg = CMD_ARG_USB;
    arg = skip_to (0, arg);
  }
  else if (G4D_ARG("--usb-spec"))
  {
    prog_arg = CMD_ARG_USB_SPEC;
    arg = skip_to (0, arg);
  }

  if (! *arg || *arg == ' ' || *arg == '\t' || ! set_device (arg))
    current_drive = saved_drive;

  d = get_device_by_drive (current_drive,0);
  if (! d)
  {
    printf ("ERROR: GRUB disk data not found.\n");
    return 0;
  }

  if (prog_arg == CMD_ARG_RM)
  {
    sprintf (ADDR_RET_STR, "%d", d->block_io->media->removable_media);
    goto exit;
  }
  if (prog_arg == CMD_ARG_RO)
  {
    sprintf (ADDR_RET_STR, "%d", d->block_io->media->read_only);
    goto exit;
  }
  if (prog_arg == CMD_ARG_BS)
  {
    sprintf (ADDR_RET_STR, "%d", d->block_io->media->block_size);
    goto exit;
  }

  if (prog_arg == CMD_ARG_PRINT)
  {
    printf ("DRIVE 0x%x\n", current_drive);
    printf ("Removable  = %s\n", d->block_io->media->removable_media ? "TRUE" : "FALSE");
    printf ("Read Only  = %s\n", d->block_io->media->read_only ? "TRUE" : "FALSE");
    printf ("Block Size = %d\n", d->block_io->media->block_size);
    //printf ("Last Block = %d\n", d->block_io->media->last_block);
  }

  status = bs->handle_protocol (d->device_handle, &usb_guid, (void **)&usb_io);
  if (prog_arg == CMD_ARG_USB)
  {
    sprintf (ADDR_RET_STR, "%d", (status == EFI_SUCCESS) ? 1 : 0);
    goto exit;
  }
  if (prog_arg == CMD_ARG_PRINT)
    printf ("USB        = %s\n", (status == EFI_SUCCESS) ? "TRUE" : "FALSE");
  if (status != EFI_SUCCESS)
  {
    sprintf (ADDR_RET_STR, "ERR");
    goto exit;
  }

  status = usb_io->get_device_desc (usb_io, &dev_desc);
  if (status != EFI_SUCCESS)
  {
    sprintf (ADDR_RET_STR, "ERR");
    goto exit;
  }
  if (prog_arg == CMD_ARG_PRINT)
  {
    printf ("USB Vendor  ID    = %04X\n", dev_desc.vendorid);
    printf ("USB Product ID    = %04X\n", dev_desc.prodid);
    printf ("USB Specification = %2x.%02x\n", dev_desc.usbrel >> 8, dev_desc.usbrel & 0xff);
  }
  if (prog_arg == CMD_ARG_USB_SPEC)
  {
    sprintf (ADDR_RET_STR, "%2x.%02x", dev_desc.usbrel >> 8, dev_desc.usbrel & 0xff);
    goto exit;
  }

  status = usb_io->get_str_desc (usb_io, LANG_ID_ENGLISH, dev_desc.strvendor, &str16);
  if (status != EFI_SUCCESS)
    goto exit;
  str = wcstostr (str16);
  printf ("USB Vendor Str    = %s\n", str);
  free (str);
  bs->free_pool (str16);

  status = usb_io->get_str_desc (usb_io, LANG_ID_ENGLISH, dev_desc.strprod, &str16);
  if (status != EFI_SUCCESS)
    goto exit;
  str = wcstostr (str16);
  printf ("USB Product Str   = %s\n", str);
  free (str);
  bs->free_pool (str16);

  status = usb_io->get_str_desc (usb_io, LANG_ID_ENGLISH, dev_desc.strserial, &str16);
  if (status != EFI_SUCCESS)
    goto exit;
  str = wcstostr (str16);
  printf ("USB Serial Number = %s\n", str);
  free (str);
  bs->free_pool (str16);

exit:
  return 1;
}

static int print_help (void)
{
  printf ("Usage: efidiskinfo OPTIONS DRIVE\nOptions:\n");
  printf ("--rm          check if DRIVE is removable.\n");
  printf ("--ro          check if DRIVE is read only.\n");
  printf ("--bs          get the block size of DRIVE.\n");
  printf ("--usb-drive   check if DRIVE is USB.\n");
  printf ("--usb-spec    get the USB Specification Release Number.\n");
  //printf ("--usb-vendor  get the USB Manufacturer String.\n");
  //printf ("--usb-prod    get the USB Product String.\n");
  printf ("--help        Display this message.\n");
  return 1;
}

static grub_size_t
wcslen (const efi_char16_t *str)
{
  grub_size_t len = 0;
  while (*(str++))
    len++;
  return len;
}

static char *
wcstostr (const efi_char16_t *str)
{
  char *ret = NULL;
  grub_size_t len = wcslen (str), i;
  ret = zalloc (len + 1);
  if (!ret)
    return NULL;
  for (i = 0; i < len; i++)
    ret[i] = str[i];
  return ret;
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
