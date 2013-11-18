//
//  corestorage.h
//  hfsinspect
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_corestorage_h
#define hfsinspect_corestorage_h

#include "hfs_structs.h"

#pragma mark - Structures

// 64 bytes
typedef struct CSBlockHeader CSBlockHeader;
struct CSBlockHeader
{
    uint32_t       checksum;
    uint32_t       checksum_seed;
    uint16_t       version;
    uint16_t       block_type;
    uint16_t       block_size;
    uint16_t       reserved1;
    
    uint64_t       reserved2;
    uint64_t       reserved3;
    
    uint64_t       block_number;
    uint64_t       reserved4;
    
    uint32_t       lba_size;
    uint32_t       flags;
    uint64_t       reserved5;
} __attribute__((aligned(2), packed));

// 512 bytes
typedef struct CSVolumeHeader CSVolumeHeader;
struct CSVolumeHeader
{
    CSBlockHeader   block_header;
    
    uint64_t       physical_size;
    uint64_t       reserved8;
    
    uint64_t       reserved9;
    uint16_t       signature; //"CS" offset 88
    uint32_t       checksum_algo;
    uint16_t       metadata_count;
    
    uint32_t       metadata_block_size;
    uint32_t       metadata_size;
    uint64_t       metadata_blocks[8];
    
    uint32_t       encryption_key_size; //bytes
    uint32_t       encryption_key_algo;
    uint8_t        encryption_key_data[128];
    uuid_t          physical_volume_uuid;
    uuid_t          logical_volume_group_uuid;
    uint8_t        reserved10[176];
} __attribute__((aligned(2), packed));

// probably 512 bytes
typedef struct CSMetadataBlockType11 CSMetadataBlockType11;
struct CSMetadataBlockType11
{
    CSBlockHeader   block_header;
    
    uint32_t       size;
    uint32_t       reserved1; //3
    uint32_t       checksum;
    uint32_t       checksum_seed;
    
    uint32_t       reserved2[35];
    
    uint32_t       volume_groups_descriptor_offset;
    uint32_t       xml_offset;
    uint32_t       xml_size;
    uint32_t       xml_size_copy;
    uint32_t       reserved3;
    uint64_t       backup_volume_header_block;
    uint64_t       record_count;
    struct {
        uint64_t   index;
        uint64_t   start;
        uint64_t   end;
    }               records[10];
    uint32_t       reserved4[4];
} __attribute__((aligned(2), packed));


#pragma mark - Functions

int         cs_get_volume_header    (HFS* hfs, CSVolumeHeader* header);
bool        cs_sniff                (HFS* hfs);
void        cs_print_block_header   (CSBlockHeader* header);
void        cs_print                (HFS* hfs);

#endif
