//
//  apm.c
//  volumes
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <errno.h>              // errno/perror
#include <string.h>             // memcpy, strXXX, etc.
#include <assert.h>

#include "apm.h"
#include "logging/logging.h"    // console printing routines

#include "volumes/_endian.h"
#include "hfsinspect/output.h"

#pragma mark Interface (Private)

void swap_APMHeader(APMHeader* record) __attribute__((nonnull));
int  apm_get_header(Volume* vol, APMHeader* header, unsigned partition_number) __attribute__((nonnull));
int  apm_get_volume_header(Volume* vol, APMHeader* header) __attribute__((nonnull));

#pragma mark Implementation

void swap_APMHeader(APMHeader* record)
{
    Swap16(record->signature);
    Swap16(record->reserved1);
    Swap32(record->partition_count);
    Swap32(record->partition_start);
    Swap32(record->partition_length);
//    char            name[32];
//    char            type[32];
    Swap32(record->data_start);
    Swap32(record->data_length);
    Swap32(record->status);
    Swap32(record->boot_code_start);
    Swap32(record->boot_code_length);
    Swap32(record->bootloader_address);
    Swap32(record->reserved2);
    Swap32(record->boot_code_entry);
    Swap32(record->reserved3);
    Swap32(record->boot_code_checksum);
//    char            processor_type[16];
}

int apm_get_header(Volume* vol, APMHeader* header, unsigned partition_number)
{
    size_t  block_size = vol->sector_size;

    ssize_t bytes      = vol_read(vol, (Bytes)header, sizeof(APMHeader), (block_size * partition_number));
    if (bytes < 0) {
        return -1;
    }

    swap_APMHeader(header);

    return 0;
}

int apm_get_volume_header(Volume* vol, APMHeader* header)
{
    return apm_get_header(vol, header, 1);
}

/**
   Tests a volume to see if it contains an APM partition map.
   Note that APM volumes can have block sizes of 512, 1K or 2K.
   @return Returns -1 on error (check errno), 0 for NO, 1 for YES.
 */
int apm_test(Volume* vol)
{
    debug("APM test");

    uint8_t* buf = NULL;
    ALLOC(buf, 4096);

    if ( vol_read(vol, buf, 4096, 0) < 0 )
        return -1;

    if ( memcmp(buf+512, APM_SIG, 2) == 0 ) {
        vol->sector_size = 512;
        return 1;

    } else if ( memcmp(buf+1024, APM_SIG, 2) == 0 ) {
        vol->sector_size = 1024;
        return 1;

    } else if ( memcmp(buf+2048, APM_SIG, 2) == 0 ) {
        vol->sector_size = 2048;
        return 1;
    }

    return 0;
}

/**
   Prints a description of the APM structure and partition information to stdout.
   @return Returns -1 on error (check errno), 0 for success.
 */
int apm_dump(Volume* vol)
{
    debug("APM dump");

    unsigned   partitionID = 1;

    APMHeader* header      = NULL;
    ALLOC(header, sizeof(APMHeader));

    BeginSection("Apple Partition Map");

    while (1) {
        int  result  = apm_get_header(vol, header, partitionID);
        if (result == -1) { FREE(header); perror("get APM header"); return -1; }

        char str[33] = "";

        BeginSection("Partition %d", partitionID);
        PrintUIChar         (header, signature);
        PrintUI             (header, partition_count);
        PrintUI             (header, partition_start);
        PrintDataLength     (header, partition_length*vol->sector_size);

        memcpy(str, &header->name, 32); str[32] = '\0';
        PrintAttribute("name", "%s", str);

        memcpy(str, &header->type, 32); str[32] = '\0';
        for (int i = 0; APMPartitionIdentifers[i].type[0] != '\0'; i++) {
            APMPartitionIdentifer identifier = APMPartitionIdentifers[i];
            if ( (strncasecmp((char*)&header->type, identifier.type, 32) == 0) ) {
                PrintAttribute("type", "%s (%s)", identifier.name, identifier.type);
            }
        }

        PrintUI             (header, data_start);
        PrintDataLength     (header, data_length*vol->sector_size);

        PrintRawAttribute   (header, status, 2);
        PrintUIFlagIfSet    (header->status, kAPMStatusValid);
        PrintUIFlagIfSet    (header->status, kAPMStatusAllocated);
        PrintUIFlagIfSet    (header->status, kAPMStatusInUse);
        PrintUIFlagIfSet    (header->status, kAPMStatusBootInfo);
        PrintUIFlagIfSet    (header->status, kAPMStatusReadable);
        PrintUIFlagIfSet    (header->status, kAPMStatusWritable);
        PrintUIFlagIfSet    (header->status, kAPMStatusPositionIndependent);
        PrintUIFlagIfSet    (header->status, kAPMStatusChainCompatible);
        PrintUIFlagIfSet    (header->status, kAPMStatusRealDriver);
        PrintUIFlagIfSet    (header->status, kAPMStatusChainDriver);
        PrintUIFlagIfSet    (header->status, kAPMStatusAutoMount);
        PrintUIFlagIfSet    (header->status, kAPMStatusIsStartup);

        PrintUI             (header, boot_code_start);
        PrintDataLength     (header, boot_code_length*vol->sector_size);
        PrintUI             (header, bootloader_address);
        PrintUI             (header, boot_code_entry);
        PrintUI             (header, boot_code_checksum);

        memcpy(str, &header->processor_type, 16); str[16] = '\0';
        PrintAttribute("processor_type", "'%s'", str);
        EndSection();

        if (partitionID >= header->partition_count) break; else partitionID++;
    }

    EndSection();

    return 0;
}

/**
   Updates a volume with sub-volumes for any defined partitions.
   @return Returns -1 on error (check errno), 0 for success.
 */
int apm_load(Volume* vol)
{
    debug("APM load");

    vol->type    = kVolTypePartitionMap;
    vol->subtype = kPMTypeAPM;

    APMHeader header      = {0};
    unsigned  partitionID = 1;

    while (1) {
        if ( apm_get_header(vol, &header, partitionID) < 0 )
            return -1;

        size_t  sector_size = vol->sector_size;
        off_t   offset      = header.partition_start * sector_size;
        size_t  length      = header.partition_length * sector_size;

        Volume* partition   = vol_make_partition(vol, partitionID - 1, offset, length);
        partition->sector_size  = sector_size;
        partition->sector_count = header.partition_length;

        memcpy(partition->desc, &header.name, 32);
        memcpy(partition->native_desc, &header.type, 32);

        for (int i = 0; APMPartitionIdentifers[i].type[0] != '\0'; i++) {
            APMPartitionIdentifer identifier = APMPartitionIdentifers[i];
            if ( (strncasecmp((char*)&header.type, identifier.type, 32) == 0) ) {
                partition->type = identifier.voltype;
            }
        }

        if (++partitionID >= header.partition_count) break;
    }

    return 0;
}

PartitionOps apm_ops = {
    .name = "Apple Partition Map",
    .test = apm_test,
    .dump = apm_dump,
    .load = apm_load,
};
