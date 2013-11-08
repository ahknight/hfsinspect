//
//  apm.h
//  hfsinspect
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs_structs.h"

#ifndef hfsinspect_apm_h
#define hfsinspect_apm_h

#pragma mark - Structures

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

bool        apm_sniff               (HFSVolume* hfs);
void        apm_print               (HFSVolume* hfs);

#endif
