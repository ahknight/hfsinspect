//
//  apm.c
//  hfsinspect
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <stdio.h>
#include "apm.h"
#include "hfs_structs.h"
#include "hfs_pstruct.h"
#include "hfs_io.h"
#include "hfs_endian.h"

int apm_get_header(HFSVolume* hfs, APMHeader* header, unsigned partition_number)
{
    char* buf = valloc(4096);
    if ( buf == NULL ) {
        perror("valloc");
        return -1;
    }
    
    ssize_t bytes = hfs_read_range(buf, hfs, hfs->block_size, (hfs->block_size * partition_number));
    if (bytes < 0) {
        perror("read");
        return -1;
    }
    
    *header = *(APMHeader*)(buf);
    free(buf);
    
    swap_APMHeader(header);
    
    return 0;
}

int apm_get_volume_header(HFSVolume* hfs, APMHeader* header)
{
    return apm_get_header(hfs, header, 1);
}

bool apm_sniff(HFSVolume* hfs)
{
    APMHeader header;
    int result = apm_get_volume_header(hfs, &header);
    if (result == -1) { perror("get APM header"); return false; }
    result = memcmp(&header.signature, "MP", 2);
    
    return (!result);
    
    return false;
}

void apm_print(HFSVolume* hfs)
{
    unsigned partitionID = 1;
    
    APMHeader* header = malloc(sizeof(APMHeader));
    
    PrintHeaderString("Apple Partition Map");
    
    while (1) {
        int result = apm_get_header(hfs, header, partitionID);
        if (result == -1) { free(header); perror("get APM header"); return; }
        
        char str[33]; memset(str, '\0', 33);
        
        PrintHeaderString("Partition %d", partitionID);
        PrintHFSChar        (header, signature);
        PrintUI             (header, partition_count);
        PrintUI             (header, partition_start);
        PrintDataLength     (header, partition_length*hfs->block_size);
        
        memcpy(str, &header->name, 32); str[32] = '\0';
        PrintAttributeString("name", "%s", str);
        
        memcpy(str, &header->type, 32); str[32] = '\0';
        for (int i = 0; APMPartitionIdentifers[i].type[0] != '\0'; i++) {
            APMPartitionIdentifer identifier = APMPartitionIdentifers[i];
            if ( (strncasecmp((char*)&header->type, identifier.type, 32) == 0) ) {
                PrintAttributeString("type", "%s (%s)", identifier.name, identifier.type);
            }
        }
        
        PrintUI             (header, data_start);
        PrintDataLength     (header, data_length*hfs->block_size);
        
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
        PrintDataLength     (header, boot_code_length*hfs->block_size);
        PrintUI             (header, bootloader_address);
        PrintUI             (header, boot_code_entry);
        PrintUI             (header, boot_code_checksum);
        
        memcpy(str, &header->processor_type, 16); str[16] = '\0';
        PrintAttributeString("processor_type", "'%s'", str);
        
        if (partitionID >= header->partition_count) break; else partitionID++;
    }
}
