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

#define REG_NONE                        0x00000000  //注册表 无
#define REG_SZ                          0x00000001  //注册表 尺寸
#define REG_EXPAND_SZ                   0x00000002  //注册表 扩展尺寸
#define REG_BINARY                      0x00000003  //注册表 二进制
#define REG_DWORD                       0x00000004  //注册表 双字
#define REG_DWORD_BIG_ENDIAN            0x00000005  //注册表 双字大端法
#define REG_LINK                        0x00000006  //注册表 链接
#define REG_MULTI_SZ                    0x00000007  //注册表 多种尺寸
#define REG_RESOURCE_LIST               0x00000008  //注册表 资源列表
#define REG_FULL_RESOURCE_DESCRIPTOR    0x00000009  //注册表 完整资源描述符
#define REG_RESOURCE_REQUIREMENTS_LIST  0x0000000a  //注册表 资源需求列表
#define REG_QWORD                       0x0000000b  //注册表 四字

#define HV_HBLOCK_SIGNATURE 0x66676572  // "regf"   签名

#define HSYS_MAJOR 1                    // 1        主
#define HSYS_MINOR 3                    // 3        次
#define HFILE_TYPE_PRIMARY 0            // 0        文件类型 初级
#define HBASE_FORMAT_MEMORY 1           // 1        基本格式 存储器

#define CM_KEY_LEAF             0x666c  // "lf"     CM键 快速索引
#define CM_KEY_HASH_LEAF        0x686c  // "lh"     CM键 哈希叶
#define CM_KEY_INDEX_ROOT       0x6972  // "ri"     CM键 索引根
#define CM_KEY_NODE_SIGNATURE   0x6b6e  // "nk"     CM键 节点
#define CM_KEY_VALUE_SIGNATURE  0x6b76  // "vk"     CM键 值

#define KEY_IS_VOLATILE                 0x0001      //键  易失
#define KEY_HIVE_EXIT                   0x0002      //键  蜂箱出口
#define KEY_HIVE_ENTRY                  0x0004      //键  蜂箱入口
#define KEY_NO_DELETE                   0x0008      //键  不删除
#define KEY_SYM_LINK                    0x0010      //键  符号链接
#define KEY_COMP_NAME                   0x0020      //键  组件名称
#define KEY_PREDEF_HANDLE               0x0040      //键  预定义句柄
#define KEY_VIRT_MIRRORED               0x0080      //键  虚拟镜像
#define KEY_VIRT_TARGET                 0x0100      //键  虚拟目标
#define KEY_VIRTUAL_STORE               0x0200      //键  虚拟存储

#define VALUE_COMP_NAME                 0x0001      //值  组件名称

// stupid name... this means "small enough not to warrant its own cell"
#define CM_KEY_VALUE_SPECIAL_SIZE       0x80000000  //CM键值特殊尺寸

#define HIVE_FILENAME_MAXLEN 31                     //配置单元文件名最大长度

typedef struct
{
  uint32_t Signature;                           //签名    regf
  uint32_t Sequence1;                           //序列1   7
  uint32_t Sequence2;                           //序列2   7
  uint64_t TimeStamp;                           //时间戳
  uint32_t Major;                               //主要    1
  uint32_t Minor;                               //次要    3
  uint32_t Type;                                //类型    0
  uint32_t Format;                              //格式    1
  uint32_t RootCell;                            //根单元  20
  uint32_t Length;                              //尺寸    200
  uint32_t Cluster;                             //簇      1
  uint16_t FileName[HIVE_FILENAME_MAXLEN + 1];  //文件名  31宽字符+1结止符
  uint32_t Reserved1[99];                       //保留
  uint32_t CheckSum;                            //检查和  724ba366     在1fc
  uint32_t Reserved2[0x37E];                    //保留
  uint32_t BootType;                            //引导类型  0          在ff8
  uint32_t BootRecover;                         //引导恢复  0          在ffc
} __attribute__ ((packed)) HBASE_BLOCK; //数据库_块    在bcd起始    尺寸=1000

typedef struct
{
  uint16_t Signature;                           //签名            nk              nk              nk
  uint16_t Flags;                               //标记            2c              20              20
  uint64_t LastWriteTime;                       //最后写时间
  uint32_t Spare;                               //备用            2               2               2
  uint32_t Parent;                              //父              cc0             b08             b08
  uint32_t SubKeyCount;                         //子键计数        2               0               0
  uint32_t VolatileSubKeyCount;                 //易失性子键计数  0               0               0
  uint32_t SubKeyList;                          //子键列表        268             -1             -1             子键列表位置,存储着CM键快速索引
  uint32_t VolatileSubKeyList;                  //易失性子键列表  -1              -1             -1
  uint32_t ValuesCount;                         //值              0               1               1
  uint32_t Values;                              //值计数          -1              750             740
  uint32_t Security;                            //安全            1c40            348             1c40
  uint32_t Class;                               //种类            -1              -1              -1
  uint32_t MaxNameLen;                          //最大名称尺寸    16              0               0
  uint32_t MaxClassLen;                         //最大种类尺寸    0               0               0
  uint32_t MaxValueNameLen;                     //最大值名称尺寸  0               e               e
  uint32_t MaxValueDataLen;                     //最大值数据尺寸  0               12              aa
  uint32_t WorkVar;                             //工作变量        0               0               0
  uint16_t NameLength;                          //名称尺寸        c               8               0
  uint16_t ClassLength;                         //种类尺寸        0               0               0
  uint16_t Name[1];                             //名称            NewStoreRoot    12000004        11000001
} __attribute__ ((packed)) CM_KEY_NODE; //CM键节点

typedef struct
{
  uint32_t Cell;                                //单元
  uint32_t HashKey;                             //哈希键
} __attribute__ ((packed)) CM_INDEX;  //CM索引

typedef struct
{
  uint16_t Signature;                           //签名        lf
  uint16_t Count;                               //计数        2
  CM_INDEX List[1];                             //CM索引      208/Desc  110/Obje
} __attribute__ ((packed)) CM_KEY_FAST_INDEX; //CM键快速索引

typedef struct
{
  uint16_t Signature;                           //签名        vk                    vk          vk
  uint16_t NameLength;                          //名称尺寸    7                     4           7
  uint32_t DataLength;                          //数据尺寸    18                    80000004    AA
  uint32_t Data;                                //数据        2A0                   10100002    C98
  uint32_t Type;                                //类型        1                     4           3
  uint16_t Flags;                               //标记        1                     1           1
  uint16_t Spare;                               //备用        0                     0           0
  uint16_t Name[1];                             //名称        KeyName(BCD00000001)  TypeH(320)  Element
} __attribute__ ((packed)) CM_KEY_VALUE;  //CM键值

typedef struct
{
  uint16_t Signature;                           //签名
  uint16_t Count;                               //计数
  uint32_t List[1];                             //列表
} __attribute__ ((packed)) CM_KEY_INDEX;  //CM键索引

typedef enum
{
  REG_ERR_NONE = 0,         //无错误
  REG_ERR_OUT_OF_MEMORY,    //内存不足
  REG_ERR_BAD_FILE_TYPE,    //错误的文件类型
  REG_ERR_FILE_NOT_FOUND,   //找不到文件
  REG_ERR_FILE_READ_ERROR,  //读文件错误
  REG_ERR_BAD_FILENAME,     //错误的文件名
  REG_ERR_BAD_ARGUMENT,     //错误的论点
  REG_ERR_BAD_SIGNATURE,    //错误的签名
} reg_err_t;

typedef struct reg_hive
{
  void (*reg_close) (struct reg_hive *this);                                  //关闭注册表
  void (*find_root) (struct reg_hive *this, HKEY *key);                       //查找根
  reg_err_t (*enum_keys) (struct reg_hive *this, HKEY key,
                           uint32_t index, uint16_t *name, uint32_t name_len);//枚举键
  reg_err_t (*find_key) (struct reg_hive *this, HKEY parent,
                          const uint16_t *path, HKEY *key);                   //查找键
  reg_err_t (*enum_values) (struct reg_hive *this, HKEY key,
                             uint32_t index, uint16_t *name, uint32_t name_len,
                             uint32_t *type);                                 //枚举值
  reg_err_t (*query_value) (struct reg_hive *this, HKEY key,
                             const uint16_t *name,
                             void *data, uint32_t *data_len, uint32_t *type); //查询值
  void (*steal_data) (struct reg_hive *this,
                      void **data, uint32_t *size);                           //取数据
  reg_err_t (*query_value_no_copy) (struct reg_hive *this, HKEY key,
                                     const uint16_t *name, void **data,
                                     uint32_t *data_len, uint32_t *type);     //查询值不复制
} reg_hive_t; //注册表蜂箱

typedef struct
{
  reg_hive_t public;//         bcd函数
  grub_size_t size; //3000     bcd尺寸
  void* data;       //10ccac00 bcd数据
} hive_t; //bcd蜂箱

reg_err_t open_hive (void *file, grub_size_t len, reg_hive_t ** hive);  //打开蜂箱

#endif
