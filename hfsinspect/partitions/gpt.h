//
//  gpt.h
//  hfsinspect
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "volume.h"
#include "partition_support.h"
#include "mbr.h"

#ifndef hfsinspect_gpt_h
#define hfsinspect_gpt_h

#pragma mark - Structures

/*
 http://developer.apple.com/library/mac/#technotes/tn2166/_index.html
 
 GPTHeader is at LBA 1. GPTPartitionEntry array starts at header->partition_start_lba (usually LBA2).
 Backup header is at the next-to-last LBA of the disk. The partition array usually precedes this.
 
 There may be up to 128 partition records in the array (16KiB).
 */

// 92 bytes
typedef struct GPTHeader {
    u_int64_t     signature;
    u_int32_t     revision;
    u_int32_t     header_size;
    u_int32_t     header_crc32;
    u_int32_t     reserved;
    u_int64_t     current_lba;
    u_int64_t     backup_lba;
    u_int64_t     first_lba;
    u_int64_t     last_lba;
    uuid_t        uuid;
    u_int64_t     partition_start_lba;
    u_int32_t     partition_entry_count;
    u_int32_t     partition_entry_size;
    u_int32_t     partition_crc32;
} GPTHeader;

// 128 bytes
typedef struct GPTPartitionEntry {
    uuid_t      type_uuid;
    uuid_t      uuid;
    u_int64_t   first_lba;
    u_int64_t   last_lba;
    u_int64_t   attributes;
    u_int16_t   name[36];
} GPTPartitionEntry;

typedef GPTPartitionEntry GPTPartitionRecord[128];

typedef struct GPTPartitionName {
    uuid_string_t   uuid;
    char            name[100];
    PartitionHint   hints;
} GPTPartitionName;

static GPTPartitionName gpt_partition_types[] __attribute__((unused)) = {
    {"00000000-0000-0000-0000-000000000000", "Unused",                          kHintIgnore},
    
    {"024DEE41-33E7-11D3-9D69-0008C781F39F", "MBR Partition Scheme",            kHintPartitionMap},
    {"C12A7328-F81F-11D2-BA4B-00A0C93EC93B", "EFI System Partition",            kHintIgnore},
    {"21686148-6449-6E6F-744E-656564454649", "BIOS Boot Partition",             kHintIgnore},
    {"D3BFE2DE-3DAF-11DF-BA40-E3A556D89593", "Intel Fast Flash",                kHintIgnore},
    
    {"48465300-0000-11AA-AA11-00306543ECAC", "Mac OS Extended (HFS+)",          kHintFilesystem},
    {"55465300-0000-11AA-AA11-00306543ECAC", "Apple UFS",                       kHintIgnore},
    {"6A898CC3-1DD2-11B2-99A6-080020736631", "Apple ZFS",                       kHintIgnore},
    {"52414944-0000-11AA-AA11-00306543ECAC", "Apple RAID Partition",            kHintIgnore},
    {"52414944-5F4F-11AA-AA11-00306543ECAC", "Apple RAID Partition (offline)",  kHintIgnore},
    {"426F6F74-0000-11AA-AA11-00306543ECAC", "OS X Recovery Partition",         kHintFilesystem},
    {"4C616265-6C00-11AA-AA11-00306543ECAC", "Apple Label",                     kHintFilesystem},
    {"5265636F-7665-11AA-AA11-00306543ECAC", "Apple TV Recovery Partition",     kHintFilesystem},
    {"53746F72-6167-11AA-AA11-00306543ECAC", "Core Storage Volume",             kHintPartitionMap},
};

#pragma mark - Functions

uuid_t*     gpt_swap_uuid           (uuid_t* uuid);
const char* gpt_partition_type_str  (uuid_t uuid, PartitionHint* hint);

/**
 Tests a volume to see if it contains a GPT partition map.
 @return Returns -1 on error (check errno), 0 for NO, 1 for YES.
 */
int gpt_test(Volume *vol);

int gpt_load_header(Volume *vol, GPTHeader *gpt, GPTPartitionRecord *entries);

/**
 Updates a volume with sub-volumes for any defined partitions.
 @return Returns -1 on error (check errno), 0 for success.
 */
int gpt_load(Volume *vol);

/**
 Prints a description of the GPT structure and partition information to stdout.
 @return Returns -1 on error (check errno), 0 for success.
 */
int gpt_dump(Volume *vol);

#endif
