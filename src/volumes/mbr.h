//
//  mbr.h
//  volumes
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef volumes_mbr_h
#define volumes_mbr_h

#include "volume.h"

#include <stdint.h>
#include <stdbool.h>


#pragma mark - Structures

#pragma pack(push, 1)
#pragma options align=packed

typedef struct MBRCHS {
    uint8_t         head;
    uint16_t        sector : 6;
    uint16_t        cylinder : 10;
} MBRCHS;

#define empty_MBRCHS ((MBRCHS){ .head = 0, .sector = 0, .cylinder = 0})

typedef struct MBRPartition {
    uint8_t         status;
    MBRCHS          first_sector;
    uint8_t         type;
    MBRCHS          last_sector;
    uint32_t        first_sector_lba;
    uint32_t        sector_count;
} MBRPartition;

#define empty_MBRPartition ((MBRPartition){ \
    .status = 0, \
    .first_sector = empty_MBRCHS, \
    .type = 0, \
    .last_sector = empty_MBRCHS, \
    .first_sector_lba = 0, \
    .sector_count = 0 \
    })

typedef struct MBR {
    uint8_t         bootstrap[440];
    uint32_t        disk_signature;
    uint16_t        reserved0;
    MBRPartition    partitions[4];
    uint8_t         signature[2];
} MBR;

#define empty_MBR ((MBR){ \
    .bootstrap = {0}, \
    .disk_signature = 0, \
    .reserved0 = 0, \
    .partitions = { empty_MBRPartition, empty_MBRPartition, empty_MBRPartition, empty_MBRPartition }, \
    .signature = {0} \
    })

static unsigned kMBRTypeGPTProtective __attribute__((unused))   = 0xEE;
static unsigned kMBRTypeAppleBoot __attribute__((unused))       = 0xAB;
static unsigned kMBRTypeAppleHFS __attribute__((unused))        = 0xAF;

struct MBRPartitionName {
    uint16_t        type;
    bool            lba;
    char            name[100];
    VolType         voltype;
    VolType         volsubtype;
};
typedef struct MBRPartitionName MBRPartitionName;

static MBRPartitionName mbr_partition_types[] __attribute__((unused)) = {
    {0x00, 0, "Free",                           kVolTypeSystem,         kSysFreeSpace},
    {0x01, 0, "MS FAT12",                       kVolTypeUserData,       kFSTypeFAT12},
    {0x04, 0, "MS FAT16",                       kVolTypeUserData,       kFSTypeFAT16},
    {0x05, 0, "Extended partition (CHS)",       kVolTypePartitionMap,   kTypeUnknown},
    {0x06, 0, "MS FAT16B",                      kVolTypeUserData,       kFSTypeFAT16B},
    {0x07, 0, "MS NTFS/exFAT; IBM IFS/HPFS",    kVolTypeUserData,       kFSTypeExFAT},
    {0x08, 0, "MS FAT12/16 CHS",                kVolTypeUserData,       kFSTypeFAT12},
    {0x0b, 0, "MS FAT32 CHS",                   kVolTypeUserData,       kFSTypeFAT32},
    {0x0c, 1, "MS FAT32X LBA",                  kVolTypeUserData,       kFSTypeFAT32},
    {0x0e, 1, "MS FAT16X LBA",                  kVolTypeUserData,       kFSTypeFAT16},
    {0x0f, 1, "Extended partition (LBA)",       kVolTypePartitionMap,   kTypeUnknown},
    
    {0x82, 1, "Linux Swap",                     kVolTypeSystem,         kSysSwapSpace},
    {0x83, 1, "Linux Filesystem",               kVolTypeUserData,       kTypeUnknown},
    {0x85, 1, "Linux Extended Boot Record",     kVolTypeSystem,         kSysReserved},
    {0x8e, 1, "Linux LVM",                      kVolTypeSystem,         kSysReserved},
    
    {0x93, 1, "Linux Hidden",                   kVolTypeSystem,         kSysReserved},
    
    {0xa8, 1, "Apple UFS",                      kVolTypeUserData,       kFSTypeUFS},
    {0xab, 1, "Apple Boot",                     kVolTypeSystem,         kSysReserved},
    {0xaf, 1, "Apple HFS",                      kVolTypeUserData,       kFSTypeHFS},
    
    {0xee, 1, "GPT Protective MBR",             kVolTypePartitionMap,   kPMTypeGPT},
    {0xef, 1, "EFI system partition",           kVolTypeSystem,         kSysEFI},
    {0, 0, {'\0'}, 0, 0},
};

#pragma pack(pop)

#pragma mark - Functions

extern PartitionOps mbr_ops;

/**
 Tests a volume to see if it contains an MBR partition map.
 @return Returns -1 on error (check errno), 0 for NO, 1 for YES.
 */
int mbr_test(Volume *vol) __attribute__((nonnull));

int mbr_load_header(Volume *vol, MBR* mbr) __attribute__((nonnull));

/**
 Updates a volume with sub-volumes for any defined partitions.
 @return Returns -1 on error (check errno), 0 for success.
 */
int mbr_load(Volume *vol) __attribute__((nonnull));

/**
 Prints a description of the MBR structure and partition information to stdout.
 @return Returns -1 on error (check errno), 0 for success.
 */
int mbr_dump(Volume *vol) __attribute__((nonnull));

const char* mbr_partition_type_str(uint16_t type, VolType* hint);

#endif
