//
//  format_sniffers.h
//  hfsinspect
//
//  Created by Adam Knight on 6/21/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_format_sniffers_h
#define hfsinspect_format_sniffers_h

#include "hfs_structs.h"
#include "range.h"

#pragma mark - Structures

#pragma mark GPT

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

typedef struct GPTPartitionEntry {
    uuid_t      type_uuid;
    uuid_t      uuid;
    u_int64_t   first_lba;
    u_int64_t   last_lba;
    u_int64_t   attributes;
    u_int16_t   name[36];
} GPTPartitionEntry;

struct GPTPartitionName { uuid_string_t uuid; char name[100]; };
typedef struct GPTPartitionName GPTPartitionName;

#define GPTPartitionTypesCount 14
static GPTPartitionName types[GPTPartitionTypesCount] __attribute__((unused)) = {
    {"00000000-0000-0000-0000-000000000000", "Unused"},
    
    {"024DEE41-33E7-11D3-9D69-0008C781F39F", "MBR Partition Scheme"},
    {"C12A7328-F81F-11D2-BA4B-00A0C93EC93B", "EFI System Partition"},
    {"21686148-6449-6E6F-744E-656564454649", "BIOS Boot Partition"},
    {"D3BFE2DE-3DAF-11DF-BA40-E3A556D89593", "Intel Fast Flash"},
    
    {"48465300-0000-11AA-AA11-00306543ECAC", "Mac OS Extended (HFS+)"},
    {"55465300-0000-11AA-AA11-00306543ECAC", "Apple UFS"},
    {"6A898CC3-1DD2-11B2-99A6-080020736631", "Apple ZFS"},
    {"52414944-0000-11AA-AA11-00306543ECAC", "Apple RAID Partition"},
    {"52414944-5F4F-11AA-AA11-00306543ECAC", "Apple RAID Partition (offline)"},
    {"426F6F74-0000-11AA-AA11-00306543ECAC", "OS X Recovery Partition"},
    {"4C616265-6C00-11AA-AA11-00306543ECAC", "Apple Label"},
    {"5265636F-7665-11AA-AA11-00306543ECAC", "Apple TV Recovery Partition"},
    {"53746F72-6167-11AA-AA11-00306543ECAC", "Core Storage Volume"},
};

#pragma mark Core Storage

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

bool        sniff_and_print         (HFSVolume* hfs);

#pragma mark GPT

void        decode_gpt_uuid         (uuid_t* uuid);
const char* partition_type_str      (uuid_t uuid);
int         get_gpt_header          (HFSVolume* hfs, GPTHeader* header);
bool        gpt_sniff               (HFSVolume* hfs);
void        print_gpt               (HFSVolume* hfs);
int         get_cs_volume_header    (HFSVolume* hfs, CSVolumeHeader* header);

#pragma mark Core Storage

bool        cs_sniff                (HFSVolume* hfs);
void        print_cs_block_header   (CSBlockHeader* header);
void        print_cs                (HFSVolume* hfs);

#endif
