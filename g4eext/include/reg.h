/*
 *  ntloader  --  Microsoft Windows NT6+ loader
 *  Copyright (C) 2021  A1ive.
 *
 *  ntloader is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ntloader is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ntloader.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Copyright (c) Mark Harmstone 2020
 *
 * This file is part of Quibble.
 *
 * Quibble is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public Licence as published by
 * the Free Software Foundation, either version 3 of the Licence, or
 * (at your option) any later version.
 *
 * Quibble is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public Licence for more details.
 *
 * You should have received a copy of the GNU Lesser General Public Licence
 * along with Quibble.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _WINREG_HEADER
#define _WINREG_HEADER 1

#include <stdint.h>

typedef uint32_t HKEY;

#define REG_NONE                        0x00000000
#define REG_SZ                          0x00000001
#define REG_EXPAND_SZ                   0x00000002
#define REG_BINARY                      0x00000003
#define REG_DWORD                       0x00000004
#define REG_DWORD_BIG_ENDIAN            0x00000005
#define REG_LINK                        0x00000006
#define REG_MULTI_SZ                    0x00000007
#define REG_RESOURCE_LIST               0x00000008
#define REG_FULL_RESOURCE_DESCRIPTOR    0x00000009
#define REG_RESOURCE_REQUIREMENTS_LIST  0x0000000a
#define REG_QWORD                       0x0000000b

#define HV_HBLOCK_SIGNATURE 0x66676572  // "regf"

#define HSYS_MAJOR 1
#define HSYS_MINOR 3
#define HFILE_TYPE_PRIMARY 0
#define HBASE_FORMAT_MEMORY 1

#define CM_KEY_LEAF             0x666c  // "lf"
#define CM_KEY_HASH_LEAF        0x686c  // "lh"
#define CM_KEY_INDEX_ROOT       0x6972  // "ri"
#define CM_KEY_NODE_SIGNATURE   0x6b6e  // "nk"
#define CM_KEY_VALUE_SIGNATURE  0x6b76  // "vk"

#define KEY_IS_VOLATILE                 0x0001
#define KEY_HIVE_EXIT                   0x0002
#define KEY_HIVE_ENTRY                  0x0004
#define KEY_NO_DELETE                   0x0008
#define KEY_SYM_LINK                    0x0010
#define KEY_COMP_NAME                   0x0020
#define KEY_PREDEF_HANDLE               0x0040
#define KEY_VIRT_MIRRORED               0x0080
#define KEY_VIRT_TARGET                 0x0100
#define KEY_VIRTUAL_STORE               0x0200

#define VALUE_COMP_NAME                 0x0001

// stupid name... this means "small enough not to warrant its own cell"
#define CM_KEY_VALUE_SPECIAL_SIZE       0x80000000

#define HIVE_FILENAME_MAXLEN 31

typedef struct
{
  uint32_t Signature;
  uint32_t Sequence1;
  uint32_t Sequence2;
  uint64_t TimeStamp;
  uint32_t Major;
  uint32_t Minor;
  uint32_t Type;
  uint32_t Format;
  uint32_t RootCell;
  uint32_t Length;
  uint32_t Cluster;
  uint16_t FileName[HIVE_FILENAME_MAXLEN + 1];
  uint32_t Reserved1[99];
  uint32_t CheckSum;
  uint32_t Reserved2[0x37E];
  uint32_t BootType;
  uint32_t BootRecover;
} __attribute__ ((packed)) HBASE_BLOCK; //在bcd起始

typedef struct
{
  uint16_t Signature;
  uint16_t Flags;
  uint64_t LastWriteTime;
  uint32_t Spare;
  uint32_t Parent;
  uint32_t SubKeyCount;
  uint32_t VolatileSubKeyCount;
  uint32_t SubKeyList;
  uint32_t VolatileSubKeyList;
  uint32_t ValuesCount;
  uint32_t Values;
  uint32_t Security;
  uint32_t Class;
  uint32_t MaxNameLen;
  uint32_t MaxClassLen;
  uint32_t MaxValueNameLen;
  uint32_t MaxValueDataLen;
  uint32_t WorkVar;
  uint16_t NameLength;
  uint16_t ClassLength;
  uint16_t Name[1];
} __attribute__ ((packed)) CM_KEY_NODE;

typedef struct
{
  uint32_t Cell;
  uint32_t HashKey;
} __attribute__ ((packed)) CM_INDEX;

typedef struct
{
  uint16_t Signature;
  uint16_t Count;
  CM_INDEX List[1];
} __attribute__ ((packed)) CM_KEY_FAST_INDEX;

typedef struct
{
  uint16_t Signature;
  uint16_t NameLength;
  uint32_t DataLength;
  uint32_t Data;
  uint32_t Type;
  uint16_t Flags;
  uint16_t Spare;
  uint16_t Name[1];
} __attribute__ ((packed)) CM_KEY_VALUE;

typedef struct
{
  uint16_t Signature;
  uint16_t Count;
  uint32_t List[1];
} __attribute__ ((packed)) CM_KEY_INDEX;

typedef enum
{
  REG_ERR_NONE = 0,
  REG_ERR_OUT_OF_MEMORY,
  REG_ERR_BAD_FILE_TYPE,
  REG_ERR_FILE_NOT_FOUND,
  REG_ERR_FILE_READ_ERROR,
  REG_ERR_BAD_FILENAME,
  REG_ERR_BAD_ARGUMENT,
  REG_ERR_BAD_SIGNATURE,
} reg_err_t;

typedef struct reg_hive
{
  void (*reg_close) (struct reg_hive *this);
  void (*find_root) (struct reg_hive *this, HKEY *key);
  reg_err_t (*enum_keys) (struct reg_hive *this, HKEY key,
                           uint32_t index, uint16_t *name, uint32_t name_len);
  reg_err_t (*find_key) (struct reg_hive *this, HKEY parent,
                          const uint16_t *path, HKEY *key);
  reg_err_t (*enum_values) (struct reg_hive *this, HKEY key,
                             uint32_t index, uint16_t *name, uint32_t name_len,
                             uint32_t *type);
  reg_err_t (*query_value) (struct reg_hive *this, HKEY key,
                             const uint16_t *name,
                             void *data, uint32_t *data_len, uint32_t *type);
  void (*steal_data) (struct reg_hive *this,
                      void **data, uint32_t *size);
  reg_err_t (*query_value_no_copy) (struct reg_hive *this, HKEY key,
                                     const uint16_t *name, void **data,
                                     uint32_t *data_len, uint32_t *type);
} reg_hive_t;

typedef struct
{
  reg_hive_t public;//         bcd函数
  grub_size_t size; //3000     bcd尺寸
  void* data;       //10ccac00 bcd起始
} hive_t;

reg_err_t open_hive (void *file, grub_size_t len, reg_hive_t ** hive);

#endif
