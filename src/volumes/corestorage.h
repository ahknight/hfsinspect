//
//  corestorage.h
//  volumes
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef volumes_corestorage_h
#define volumes_corestorage_h

#include "volume.h"
#include <uuid/uuid.h>

#pragma mark - Structures

enum CSBlockTypes {
    kCSVolumeHeaderBlock = 0x10,
    kCSDiskLabelBlock    = 0x11,
};

// 64 bytes
typedef struct CSBlockHeader CSBlockHeader;
struct CSBlockHeader
{
    uint32_t checksum;              // 0
    uint32_t checksum_seed;         // 4
    uint16_t version;               // 8
    uint16_t block_type;            // 10

    uint32_t field_5;               // 12; sequence number?
    uint64_t generation;            // 16; revision number? counter?
    uint64_t field_7;               // 24; some block number
    uint64_t field_8;               // 32; this block number?
    uint64_t field_9;               // 40; some block number

    uint32_t block_size;            // 48
    uint32_t field_11;              // 52; some kind of flags
    uint64_t field_12;              // 56
} __attribute__((aligned(2), packed));

// 512 bytes
typedef struct CSVolumeHeader CSVolumeHeader;
struct CSVolumeHeader
{
    CSBlockHeader block_header;                 // 0

    uint64_t      physical_size;                // 64
    uint8_t       field_14[16];                 // 72

    uint16_t      signature;                    // 88; "CS"
    uint32_t      checksum_algo;                // 90; 1 = CRC-32C

    uint16_t      md_count;                     // 94
    uint32_t      md_block_size;                // 96
    uint32_t      md_size;                      // 100
    uint64_t      md_blocks[8];                 // 104

    uint32_t      encryption_key_size;          // 168
    uint32_t      encryption_key_algo;          // 172; 2 = AES-XTS
    char          encryption_key_data[128];     // 176

    uuid_t        physical_volume_uuid;         // 304; UUID in big-endian
    uuid_t        logical_volume_group_uuid;    // 320; UUID in big-endian

    uint8_t       field_26[176];                // 336
} __attribute__((aligned(2), packed));

// probably 512 bytes, though only 216+ are described below
typedef struct CSMetadataBlockType11 CSMetadataBlockType11;
struct CSMetadataBlockType11
{
    CSBlockHeader block_header;                 // 0

    uint32_t      metadata_size;                // 0
    uint32_t      field_2;                      // 4; "0x03"
    uint32_t      checksum;                     // 8
    uint32_t      checksum_seed;                // 12

    uint32_t      uncharted[35];                // 16

    uint32_t      vol_grps_desc_off;            // 156
    uint32_t      plist_offset;                 // 160
    uint32_t      plist_size;                   // 164
    uint32_t      plist_size_copy;              // 168
    uint32_t      field_10;
    uint64_t      secondary_vhb_off;            // 176
    uint64_t      ukwn_record_count;            // 184
    struct {
        uint64_t unknown_0;
        uint64_t unknown_1;
        uint64_t unknown_2;
    } ukwn_records[1];                          // 192
} __attribute__((aligned(2), packed));

typedef struct CSVolumeGroupsDescriptor {
    uint64_t field_1;                           // 0
    uint64_t field_2;                           // 8
    uint64_t field_3;                           // 16
    uint64_t field_4;                           // 24
    uint64_t field_5;                           // 32
    uint64_t field_6;                           // 40
    char     plist[4];                          // 48
} CSVolumeGroupsDescriptor;

#pragma mark - Functions

extern PartitionOps cs_ops;

void cs_print_block_header(CSBlockHeader* header) __attribute__((nonnull));

/**
   Tests a volume to see if it contains a CS partition map.
   @return Returns -1 on error (check errno), 0 for NO, 1 for YES.
 */
int cs_test(Volume* vol) __attribute__((nonnull));

int cs_get_volume_header(Volume* vol, CSVolumeHeader* header) __attribute__((nonnull));

/**
   Updates a volume with sub-volumes for any defined partitions.
   @return Returns -1 on error (check errno), 0 for success.
 */
int cs_load(Volume* vol) __attribute__((nonnull));

/**
   Prints a description of the CS structure and partition information to stdout.
   @return Returns -1 on error (check errno), 0 for success.
 */
int cs_dump(Volume* vol) __attribute__((nonnull));

#endif
