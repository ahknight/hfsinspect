//
//  gpt.h
//  volumes
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef volumes_gpt_h
#define volumes_gpt_h

#include "volume.h"
#include "mbr.h"
#include <uuid/uuid.h>

#ifndef _UUID_STRING_T
#define _UUID_STRING_T
typedef char	uuid_string_t[37];
#endif /* _UUID_STRING_T */

#pragma mark - Structures

#define GPT_SIG "EFI PART"

/*
 http://developer.apple.com/library/mac/#technotes/tn2166/_index.html
 
 GPTHeader is at LBA 1. GPTPartitionEntry array starts at header->partition_start_lba (usually LBA2).
 Backup header is at the next-to-last LBA of the disk. The partition array usually precedes this.
 
 There may be up to 128 partition records in the array (16KiB).
 */

// 92 bytes
typedef struct GPTHeader {
    uint64_t     signature;
    uint32_t     revision;
    uint32_t     header_size;
    uint32_t     crc;
    uint32_t     reserved;
    uint64_t     current_lba;
    uint64_t     backup_lba;
    uint64_t     first_lba;
    uint64_t     last_lba;
    uuid_t       uuid;
    uint64_t     partition_table_start_lba;
    uint32_t     partitions_entry_count;
    uint32_t     partitions_entry_size;
    uint32_t     partition_table_crc;
} __attribute__((packed, aligned(2))) GPTHeader;

// 128 bytes
typedef struct GPTPartitionEntry {
    uuid_t      type_uuid;
    uuid_t      uuid;
    uint64_t   first_lba;
    uint64_t   last_lba;
    uint64_t   attributes;
    uint16_t   name[36];
} __attribute__((packed, aligned(2))) GPTPartitionEntry;

typedef GPTPartitionEntry GPTPartitionRecord[128];

typedef struct GPTPartitionName {
    uuid_string_t   uuid;
    char            name[100];
    VolType         voltype;
    VolType         volsubtype;
} GPTPartitionName;

static GPTPartitionName gpt_partition_types[] __attribute__((unused)) = {
    {"00000000-0000-0000-0000-000000000000", "Unused",                          kVolTypeSystem,         kSysFreeSpace},
    
    {"024DEE41-33E7-11D3-9D69-0008C781F39F", "MBR Partition Scheme",            kVolTypePartitionMap,   kPMTypeMBR},
    {"C12A7328-F81F-11D2-BA4B-00A0C93EC93B", "EFI System Partition",            kVolTypeSystem,         kSysEFI},
    {"21686148-6449-6E6F-744E-656564454649", "BIOS Boot Partition",             kVolTypeSystem,         kSysReserved},
    {"D3BFE2DE-3DAF-11DF-BA40-E3A556D89593", "Intel Fast Flash",                kVolTypeSystem,         kSysReserved},
    
    {"48465300-0000-11AA-AA11-00306543ECAC", "Mac OS Extended (HFS+)",          kVolTypeUserData,       kFSTypeHFSPlus},
    {"55465300-0000-11AA-AA11-00306543ECAC", "Apple UFS",                       kVolTypeUserData,       kFSTypeUFS},
    {"6A898CC3-1DD2-11B2-99A6-080020736631", "Apple ZFS",                       kVolTypeUserData,       kFSTypeZFS},
    {"52414944-0000-11AA-AA11-00306543ECAC", "Apple RAID Partition",            kVolTypeUserData,       kTypeUnknown},
    {"52414944-5F4F-11AA-AA11-00306543ECAC", "Apple RAID Partition (offline)",  kVolTypeUserData,       kTypeUnknown},
    {"426F6F74-0000-11AA-AA11-00306543ECAC", "OS X Recovery Partition",         kVolTypeUserData,       kSysRecovery},
    {"4C616265-6C00-11AA-AA11-00306543ECAC", "Apple Label",                     kVolTypeUserData,       kTypeUnknown},
    {"5265636F-7665-11AA-AA11-00306543ECAC", "Apple TV Recovery Partition",     kVolTypeUserData,       kSysRecovery},
    {"53746F72-6167-11AA-AA11-00306543ECAC", "Core Storage Volume",             kVolTypePartitionMap,   kPMCoreStorage},
    {{0}, {0}, 0, 0},
};

static uint64_t kGPTRequired        __attribute__((unused)) = 0x0000000000000001; //0 - "required", "system disk"
static uint64_t kGPTNoBlockIO       __attribute__((unused)) = 0x0000000000000002; //1 - "not mapped", "no block IO"
static uint64_t kGPTLegacyBIOSBootable  __attribute__((unused)) = 0x0000000000000004; //2

// http://msdn.microsoft.com/en-us/library/windows/desktop/aa365449(v=vs.85).aspx
static uint64_t kGPTMSReadOnly      __attribute__((unused)) = 0x1000000000000000; //60
static uint64_t kGPTMSShadowCopy    __attribute__((unused)) = 0x2000000000000000; //61
static uint64_t kGPTMSHidden        __attribute__((unused)) = 0x4000000000000000; //62
static uint64_t kGPTMSNoAutomount   __attribute__((unused)) = 0x8000000000000000; //63


#pragma mark - Functions

extern PartitionOps gpt_ops;

void        gpt_swap_uuid           (uuid_t *uuid_p, const uuid_t* uuid) __attribute__((nonnull));
const char* gpt_partition_type_str  (uuid_t uuid, VolType* hint);

/**
 Tests a volume to see if it contains a GPT partition map.
 @return Returns -1 on error (check errno), 0 for NO, 1 for YES.
 */
int gpt_test(Volume *vol) __attribute__((nonnull));

/**
 Updates a volume with sub-volumes for any defined partitions.
 @return Returns -1 on error (check errno), 0 for success.
 */
int gpt_load(Volume *vol) __attribute__((nonnull));

/**
 Prints a description of the GPT structure and partition information to stdout.
 @return Returns -1 on error (check errno), 0 for success.
 */
int gpt_dump(Volume *vol) __attribute__((nonnull));

#endif
