//
//  partition_support.h
//  hfsinspect
//
//  Created by Adam Knight on 6/21/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_partition_support_h
#define hfsinspect_partition_support_h

#include "hfs_structs.h"
#include "range.h"


#pragma mark - Structures

typedef enum PartitionHint {
    kHintIgnore         = 0x01,   // Do not look at this partition (reserved space, etc.)
    kHintFilesystem     = 0x02,   // May have a filesystem
    kHintPartitionMap   = 0x04,   // May have a partition map
} PartitionHint;

typedef struct Partition {
    off_t           offset;
    size_t          length;
    PartitionHint   hints;
} Partition;


#pragma mark MBR

typedef struct MBRPartition MBRPartition;
struct MBRPartition {
    u_int8_t        status;
    unsigned        first_sector_head : 8;
    unsigned        first_sector_sector : 5;
    unsigned        first_sector_cylinder : 10;
    u_int8_t        type;
    unsigned        last_sector_head : 8;
    unsigned        last_sector_sector : 5;
    unsigned        last_sector_cylinder : 10;
    u_int32_t       first_sector_lba;
    u_int32_t       sector_count;
} __attribute__((aligned(2), packed));

typedef struct MBR {
    char            bootstrap[446];
    MBRPartition    partitions[4];
    char            signature[2];
} MBR;

static unsigned kMBRTypeGPTProtective __attribute__((unused))   = 0xEE;
static unsigned kMBRTypeAppleBoot __attribute__((unused))       = 0xAB;
static unsigned kMBRTypeAppleHFS __attribute__((unused))        = 0xAF;

#pragma mark GPT

/*
 http://developer.apple.com/library/mac/#technotes/tn2166/_index.html
 
 GPTHeader is at LBA 1. GPTPartitionEntry array starts at header->partition_start_lba (usually LBA2).
 Backup header is at the next-to-last LBA of the disk. The partition array usually precedes this.
 
 There may be up to 128 partition records in the array (16KiB).
 
 We presume 512-byte blocks, but shouldn't.  Unsure at this point how to get the "real" LBA size of
 a device but 512 works at the moment (stat block size?).
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

struct GPTPartitionName { uuid_string_t uuid; char name[100]; PartitionHint hints; };
typedef struct GPTPartitionName GPTPartitionName;

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


#pragma mark Apple Partition Map

// 136 bytes (512 reserved on-disk)
typedef struct APMHeader APMHeader;
struct APMHeader {
    u_int16_t       signature;
    u_int16_t       reserved1;
    u_int32_t       partition_count;
    u_int32_t       partition_start;
    u_int32_t       partition_length;
    char            name[32];
    char            type[32];
    u_int32_t       data_start;
    u_int32_t       data_length;
    u_int32_t       status;
    u_int32_t       boot_code_start;
    u_int32_t       boot_code_length;
    u_int32_t       bootloader_address;
    u_int32_t       reserved2;
    u_int32_t       boot_code_entry;
    u_int32_t       reserved3;
    u_int32_t       boot_code_checksum;
    char            processor_type[16];
//    char            reserved4[376];
};

typedef struct APMPartitionIdentifer { char type[32]; char name[100]; PartitionHint hints; } APMPartitionIdentifer;
static APMPartitionIdentifer APMPartitionIdentifers[] __attribute__((unused)) = {
    {"Apple_Boot",                  "OS X Open Firmware 3.x booter",    kHintIgnore},
    {"Apple_Boot_RAID",             "RAID partition",                   kHintIgnore},
    {"Apple_Bootstrap",             "secondary loader",                 kHintIgnore},
    {"Apple_Driver",                "device driver",                    kHintIgnore},
    {"Apple_Driver43",              "SCSI Manager 4.3 device driver",   kHintIgnore},
    {"Apple_Driver43_CD",           "SCSI CD-ROM device driver",        kHintIgnore},
    {"Apple_Driver_ATA",            "ATA device driver",                kHintIgnore},
    {"Apple_Driver_ATAPI",          "ATAPI device driver",              kHintIgnore},
    {"Apple_Driver_IOKit",          "IOKit device driver",              kHintIgnore},
    {"Apple_Driver_OpenFirmware",   "Open Firmware device driver",      kHintIgnore},
    {"Apple_Extra",                 "unused",                           kHintIgnore},
    {"Apple_Free",                  "free space",                       kHintIgnore},
    {"Apple_FWDriver",              "FireWire device driver",           kHintIgnore},
    {"Apple_HFS",                   "HFS/HFS+",                         kHintFilesystem},
    {"Apple_HFSX",                  "HFS+/HFSX",                        kHintFilesystem},
    {"Apple_Loader",                "secondary loader",                 kHintIgnore},
    {"Apple_MFS",                   "Macintosh File System",            kHintIgnore},
    {"Apple_Partition_Map",         "partition map",                    kHintPartitionMap},
    {"Apple_Patches",               "patch partition",                  kHintIgnore},
    {"Apple_PRODOS",                "ProDOS",                           kHintIgnore},
    {"Apple_Rhapsody_UFS",          "UFS",                              kHintIgnore},
    {"Apple_Scratch",               "empty",                            kHintIgnore},
    {"Apple_Second",                "secondary loader",                 kHintIgnore},
    {"Apple_UFS",                   "UFS",                              kHintIgnore},
    {"Apple_UNIX_SVR2",             "UNIX file system",                 kHintIgnore},
    {"Apple_Void",                  "dummy partition (empty)",          kHintIgnore},
    {"Be_BFS",                      "BeOS BFS",                         kHintIgnore},
    
    {"", "", 0}
};

static u_int32_t kAPMStatusValid                __attribute__((unused)) = 0x00000001;
static u_int32_t kAPMStatusAllocated            __attribute__((unused)) = 0x00000002;
static u_int32_t kAPMStatusInUse                __attribute__((unused)) = 0x00000004;
static u_int32_t kAPMStatusBootInfo             __attribute__((unused)) = 0x00000008;
static u_int32_t kAPMStatusReadable             __attribute__((unused)) = 0x00000010;
static u_int32_t kAPMStatusWritable             __attribute__((unused)) = 0x00000020;
static u_int32_t kAPMStatusPositionIndependent  __attribute__((unused)) = 0x00000040;
static u_int32_t kAPMStatusChainCompatible      __attribute__((unused)) = 0x00000100;
static u_int32_t kAPMStatusRealDriver           __attribute__((unused)) = 0x00000200;
static u_int32_t kAPMStatusChainDriver          __attribute__((unused)) = 0x00000400;
static u_int32_t kAPMStatusAutoMount            __attribute__((unused)) = 0x40000000;
static u_int32_t kAPMStatusIsStartup            __attribute__((unused)) = 0x80000000;


#pragma mark - Functions

bool        sniff_and_print         (HFSVolume* hfs);


#pragma mark GPT

uuid_t*     swap_gpt_uuid           (uuid_t* uuid);
const char* partition_type_str      (uuid_t uuid, PartitionHint* hint);
int         get_gpt_header          (HFSVolume* hfs, GPTHeader* header, MBR* mbr);
bool        gpt_sniff               (HFSVolume* hfs);
void        print_mbr               (HFSVolume* hfs, MBR* mbr);
void        print_gpt               (HFSVolume* hfs);
bool        gpt_partitions          (HFSVolume* hfs, Partition partitions[128], unsigned* count);


#pragma mark Core Storage

int         get_cs_volume_header    (HFSVolume* hfs, CSVolumeHeader* header);
bool        cs_sniff                (HFSVolume* hfs);
void        print_cs_block_header   (CSBlockHeader* header);
void        print_cs                (HFSVolume* hfs);


#pragma mark - Apple Partition Map

bool        apm_sniff               (HFSVolume* hfs);
void        print_apm               (HFSVolume* hfs);

#endif
