//
//  corestorage.h
//  hfsinspect
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs_structs.h"

#ifndef hfsinspect_corestorage_h
#define hfsinspect_corestorage_h

#pragma mark - Structures

// 64 bytes
typedef struct CSBlockHeader CSBlockHeader;
struct CSBlockHeader
{
    u_int32_t       checksum;
    u_int32_t       checksum_seed;
    u_int16_t       version;
    u_int16_t       block_type;
    u_int16_t       block_size;
    u_int16_t       reserved1;
    
    u_int64_t       reserved2;
    u_int64_t       reserved3;
    
    u_int64_t       block_number;
    u_int64_t       reserved4;
    
    u_int32_t       lba_size;
    u_int32_t       flags;
    u_int64_t       reserved5;
} __attribute__((aligned(2), packed));

// 512 bytes
typedef struct CSVolumeHeader CSVolumeHeader;
struct CSVolumeHeader
{
    CSBlockHeader   block_header;
    
    u_int64_t       physical_size;
    u_int64_t       reserved8;
    
    u_int64_t       reserved9;
    u_int16_t       signature; //"CS" offset 88
    u_int32_t       checksum_algo;
    u_int16_t       metadata_count;
    
    u_int32_t       metadata_block_size;
    u_int32_t       metadata_size;
    u_int64_t       metadata_blocks[8];
    
    u_int32_t       encryption_key_size; //bytes
    u_int32_t       encryption_key_algo;
    u_int8_t        encryption_key_data[128];
    uuid_t          physical_volume_uuid;
    uuid_t          logical_volume_group_uuid;
    u_int8_t        reserved10[176];
} __attribute__((aligned(2), packed));

// probably 512 bytes
typedef struct CSMetadataBlockType11 CSMetadataBlockType11;
struct CSMetadataBlockType11
{
    CSBlockHeader   block_header;
    
    u_int32_t       size;
    u_int32_t       reserved1; //3
    u_int32_t       checksum;
    u_int32_t       checksum_seed;
    
    u_int32_t       reserved2[35];
    
    u_int32_t       volume_groups_descriptor_offset;
    u_int32_t       xml_offset;
    u_int32_t       xml_size;
    u_int32_t       xml_size_copy;
    u_int32_t       reserved3;
    u_int64_t       backup_volume_header_block;
    u_int64_t       record_count;
    struct {
        u_int64_t   index;
        u_int64_t   start;
        u_int64_t   end;
    }               records[10];
    u_int32_t       reserved4[4];
} __attribute__((aligned(2), packed));


#pragma mark - Functions

int         cs_get_volume_header    (HFSVolume* hfs, CSVolumeHeader* header);
bool        cs_sniff                (HFSVolume* hfs);
void        cs_print_block_header   (CSBlockHeader* header);
void        cs_print                (HFSVolume* hfs);

#endif
