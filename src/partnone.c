/*

    File: partnone.c

    Copyright (C) 1998-2008 Christophe GRENIER <grenier@cgsecurity.org>

    This software is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write the Free Software Foundation, Inc., 51
    Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
 
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <ctype.h>      /* tolower */
#include "types.h"
#include "common.h"
#include "testdisk.h"
#include "fnctdsk.h"
#include "analyse.h"
#include "lang.h"
#include "intrf.h"
#include "chgtype.h"
#include "savehdr.h"
#include "bfs.h"
#include "bsd.h"
#include "cramfs.h"
#include "ext2.h"
#include "fat.h"
#include "fatx.h"
#include "hfs.h"
#include "hfsp.h"
#include "jfs_superblock.h"
#include "jfs.h"
#include "luks.h"
#include "lvm.h"
#include "md.h"
#include "netware.h"
#include "ntfs.h"
#include "rfs.h"
#include "sun.h"
#include "sysv.h"
#include "swap.h"
#include "ufs.h"
#include "xfs.h"
#include "log.h"
#include "partnone.h"

static int check_part_none(disk_t *disk_car, const int verbose,partition_t *partition,const int saveheader);
static int get_geometry_from_nonembr(const unsigned char *buffer, const int verbose, CHSgeometry_t *geometry);
static list_part_t *read_part_none(disk_t *disk_car, const int verbose, const int saveheader);
static list_part_t *init_part_order_none(const disk_t *disk_car, list_part_t *list_part);
static void set_next_status_none(const disk_t *disk_car, partition_t *partition);
static int test_structure_none(list_part_t *list_part);
static int is_part_known_none(const partition_t *partition);
static void init_structure_none(const disk_t *disk_car,list_part_t *list_part, const int verbose);
static const char *get_partition_typename_none_aux(const unsigned int part_type_none);
static int set_part_type_none(partition_t *partition, unsigned int part_type);

static const struct systypes none_sys_types[] = {
  {UP_BEOS,	"BeFS"},
  {UP_CRAMFS,	"CramFS"},
  {UP_EXT2,	"ext2"},
  {UP_EXT3,	"ext3"},
  {UP_EXT4,	"ext4"},
/*  {UP_EXTENDED,	"Extended"}, */
  {UP_FAT12,	"FAT12"},
  {UP_FAT16,	"FAT16"},
  {UP_FAT32,	"FAT32"},
  {UP_FREEBSD,	"FreeBSD"},
  {UP_HFS,	"HFS"},
  {UP_HFSP,	"HFS+"},
  {UP_HFSX,	"HFSX"},
  {UP_HPFS,	"HPFS"},
  {UP_JFS,	"JFS"},
  {UP_LINSWAP,	"Linux SWAP"},
  {UP_LINSWAP2,	"Linux SWAP 2"},
  {UP_LUKS,	"Linux LUKS"},
  {UP_LVM,	"Linux LVM"},
  {UP_LVM2,	"Linux LVM2"},
  {UP_MD,	"Linux md 0.9 RAID"},
  {UP_MD1,	"Linux md 1.x RAID"},
  {UP_NETWARE,	"Netware"},
  {UP_NTFS,	"NTFS"},
  {UP_OPENBSD,	"OpenBSD"},
  {UP_OS2MB,	"OS2 Multiboot"},
  {UP_RFS,	"ReiserFS 3.5"},
  {UP_RFS2,	"ReiserFS 3.6"},
  {UP_RFS3,	"ReiserFS 3.x"},
  {UP_RFS4,	"ReiserFS 4"},
  {UP_SUN,	"Sun"},
  {UP_SYSV4,	"SysV 4"},
  {UP_UFS,	"UFS"},
  {UP_UFS2,	"UFS 2"},
  {UP_UNK,	"Unknown"},
  {UP_XFS,	"XFS"},
  {UP_XFS2,	"XFS 2"},
  {UP_XFS3,	"XFS 3"},
  {UP_XFS4,	"XFS 4"},
  { 0,NULL }
};

arch_fnct_t arch_none=
{
  .part_name="None",
  .part_name_option="partition_none",
  .msg_part_type=NULL,
  .read_part=read_part_none,
  .write_part=NULL,
  .init_part_order=init_part_order_none,
  .get_geometry_from_mbr=get_geometry_from_nonembr,
  .check_part=check_part_none,
  .write_MBR_code=NULL,
  .add_partition=NULL,
  .set_prev_status=set_next_status_none,
  .set_next_status=set_next_status_none,
  .test_structure=test_structure_none,
  .get_part_type=get_part_type_none,
  .set_part_type=set_part_type_none,
  .init_structure=init_structure_none,
  .erase_list_part=NULL,
  .get_partition_typename=get_partition_typename_none,
  .is_part_known=is_part_known_none
};

unsigned int get_part_type_none(const partition_t *partition)
{
  return partition->upart_type;
}

int get_geometry_from_nonembr(const unsigned char *buffer, const int verbose, CHSgeometry_t *geometry)
{
  {
    /* Ugly hack to get geometry from FAT and NTFS */
    const struct fat_boot_sector *fat_header=(const struct fat_boot_sector *)buffer;
    if(le16(fat_header->marker)==0xAA55)
    {
      if(le16(fat_header->secs_track)>0 && le16(fat_header->secs_track)<=63 &&
          le16(fat_header->heads)>0 && le16(fat_header->heads)<=255)
      {
        geometry->sectors_per_head=le16(fat_header->secs_track);
        geometry->heads_per_cylinder=le16(fat_header->heads);
      }
    }
  }
  return 0;
}

list_part_t *read_part_none(disk_t *disk_car, const int verbose, const int saveheader)
{
  int insert_error=0;
  unsigned char *buffer_disk;
  list_part_t *list_part;
  partition_t *partition;
  int res=0;
  partition=partition_new(&arch_none);
  buffer_disk=(unsigned char *)MALLOC(16*DEFAULT_SECTOR_SIZE);
  partition->part_size=disk_car->disk_size;
  if(recover_MD_from_partition(disk_car, partition, verbose)==0)
    res=1;
  else
    partition_reset(partition,&arch_none);
  if(res<=0)
    res=search_type_128(buffer_disk,disk_car,partition,verbose,0);
  if(res<=0)
    res=search_type_64(buffer_disk,disk_car,partition,verbose,0);
  if(res<=0)
    res=search_type_8(buffer_disk,disk_car,partition,verbose,0);
  if(res<=0)
    res=search_type_16(buffer_disk,disk_car,partition,verbose,0);
  if(res<=0)
    res=search_type_2(buffer_disk,disk_car,partition,verbose,0);
  if(res<=0)
    res=search_type_1(buffer_disk,disk_car,partition,verbose,0);
  if(res<=0)
    res=search_type_0(buffer_disk,disk_car,partition,verbose,0);
  free(buffer_disk);
  if(res<=0)
    partition_reset(partition,&arch_none);
  partition->part_size=disk_car->disk_size;
  partition->order=NO_ORDER;
  partition->status=STATUS_PRIM;
  screen_buffer_reset();
  disk_car->arch->check_part(disk_car,verbose,partition,saveheader);
  aff_part_buffer(AFF_PART_ORDER|AFF_PART_STATUS,disk_car,partition);
  list_part=insert_new_partition(NULL, partition, 0, &insert_error);
  if(insert_error>0)
    free(partition);
  return list_part;
}

static list_part_t *init_part_order_none(const disk_t *disk_car, list_part_t *list_part)
{
  return list_part;
}


static void set_next_status_none(const disk_t *disk_car, partition_t *partition)
{
}

static int test_structure_none(list_part_t *list_part)
{
  return 0;
}

static int set_part_type_none(partition_t *partition, unsigned int part_type)
{
  partition->upart_type=(upart_type_t)part_type;
  return 0;
}

static int is_part_known_none(const partition_t *partition)
{
  return 1;
}

static void init_structure_none(const disk_t *disk_car,list_part_t *list_part, const int verbose)
{
  list_part_t *element;
  for(element=list_part;element!=NULL;element=element->next)
  {
    element->part->status=STATUS_PRIM;
  }
}

static int check_part_none(disk_t *disk_car,const int verbose,partition_t *partition, const int saveheader)
{
  int ret=0;
  switch(partition->upart_type)
  {
    case UP_BEOS:
      ret=check_BeFS(disk_car,partition,verbose);
      break;
    case UP_CRAMFS:
      ret=check_cramfs(disk_car,partition,verbose);
      break;
    case UP_EXT2:
    case UP_EXT3:
    case UP_EXT4:
      ret=check_EXT2(disk_car,partition,verbose);
      break;
    case UP_EXTENDED:
      break;
    case UP_FAT12:
    case UP_FAT16:
    case UP_FAT32:
      ret=check_FAT(disk_car,partition,verbose);
      break;
    case UP_FATX:
      ret=check_FATX(disk_car,partition,verbose);
      break;
    case UP_FREEBSD:
      ret=check_BSD(disk_car,partition,verbose,BSD_MAXPARTITIONS);
      break;
    case UP_HFS:
      ret=check_HFS(disk_car,partition,verbose);
      break;
    case UP_HFSP:
    case UP_HFSX:
      ret=check_HFSP(disk_car,partition,verbose);
      break;
    case UP_HPFS:
      ret=check_HPFS(disk_car,partition,verbose);
      break;
    case UP_JFS:
      ret=check_JFS(disk_car,partition,verbose);
      break;
    case UP_LINSWAP:
    case UP_LINSWAP2:
      ret=check_Linux_SWAP(disk_car,partition,verbose);
      break;
    case UP_LUKS:
    ret=check_LUKS(disk_car, partition, verbose);
      break;
    case UP_LVM:
      ret=check_LVM(disk_car,partition,verbose);
      break;
    case UP_LVM2:
      ret=check_LVM2(disk_car,partition,verbose);
      break;
    case UP_NETWARE:
      ret=check_netware(disk_car,partition,verbose);
      break;
    case UP_NTFS:
      ret=check_NTFS(disk_car,partition,verbose,0);
      if(ret!=0)
      { screen_buffer_add("Invalid NTFS boot\n"); }
      break;
    case UP_OPENBSD:
      ret=check_BSD(disk_car,partition,verbose,OPENBSD_MAXPARTITIONS);
      break;
    case UP_OS2MB:
      ret=check_OS2MB(disk_car,partition,verbose);
      break;
    case UP_MD:
    case UP_MD1:
      ret=check_MD(disk_car,partition,verbose);
      if(ret!=0)
      { screen_buffer_add("Invalid RAID superblock\n"); }
      break;
    case UP_RFS:
    case UP_RFS2:
    case UP_RFS3:
    case UP_RFS4:
      ret=check_rfs(disk_car,partition,verbose);
      break;
    case UP_SUN:
      ret=check_sun_i386(disk_car,partition,verbose);
      break;
    case UP_SYSV4:
      ret=check_sysv(disk_car,partition,verbose);
      break;
    case UP_UFS:
    case UP_UFS2:
      ret=check_ufs(disk_car,partition,verbose);
      break;
    case UP_XFS:
    case UP_XFS2:
    case UP_XFS3:
    case UP_XFS4:
      ret=check_xfs(disk_car,partition,verbose);
      break;
    case UP_UNK:
      break;
    default:
      if(verbose>0)
      {
        log_info("check_part_none %u type %02X: no test\n",partition->order,partition->upart_type);
      }
      break;
  }
  return ret;
}

static const char *get_partition_typename_none_aux(const unsigned int part_type_none)
{
  int i;
  for (i=0; none_sys_types[i].name!=NULL; i++)
    if (none_sys_types[i].part_type == part_type_none)
      return none_sys_types[i].name;
  return NULL;
}

const char *get_partition_typename_none(const partition_t *partition)
{
  return get_partition_typename_none_aux(partition->upart_type);
}
