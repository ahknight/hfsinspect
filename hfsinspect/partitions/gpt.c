//
//  gpt.c
//  hfsinspect
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <stdio.h>
#include "gpt.h"
#include "hfs_endian.h"
#include "hfs_io.h"
#include "hfs_pstruct.h"

uuid_t* gpt_swap_uuid(uuid_t* uuid)
{
    // Because these UUIDs are fucked up.
    void* uuid_buf = uuid;
    *( (u_int32_t*) uuid + 0 ) = OSSwapInt32(*( (u_int32_t*)uuid_buf + 0 ));
    *( (u_int16_t*) uuid + 2 ) = OSSwapInt16(*( (u_int16_t*)uuid_buf + 2 ));
    *( (u_int16_t*) uuid + 3 ) = OSSwapInt16(*( (u_int16_t*)uuid_buf + 3 ));
    *( (u_int64_t*) uuid + 1 ) =             *( (u_int64_t*)uuid_buf + 1 );
    return uuid;
}

const char* gpt_partition_type_str(uuid_t uuid, PartitionHint* hint)
{
    for(int i=0; gpt_partition_types[i].name[0] != '\0'; i++) {
        uuid_t test;
        uuid_parse(gpt_partition_types[i].uuid, test);
        int result = uuid_compare(uuid, test);
        if ( result == 0 ) {
            if (hint != NULL) *hint = gpt_partition_types[i].hints;
            return gpt_partition_types[i].name;
        }
    }
    
    static uuid_string_t string;
    uuid_unparse(uuid, string);
    return string;
}

int gpt_get_header(HFSVolume* hfs, GPTHeader* header, MBR* mbr)
{
    if (hfs == NULL || header == NULL) { return EINVAL; }
    
    size_t read_size = hfs->block_size*2;
    char* buf = NULL;
    INIT_BUFFER(buf, read_size);
    
    ssize_t bytes;
    bytes = hfs_read_range(buf, hfs, read_size, 0);
    if (bytes < 0) {
        perror("read");
        return errno;
    }
    
    bool has_protective_mbr = false;
    MBR tmp = *(MBR*)(buf);
    char sig[2] = { 0x55, 0xaa };
    if (memcmp(&sig, &tmp.signature, 2) == 0) {
        // Valid MBR.
        info("MBR found.");
        
        if (mbr != NULL) *mbr = tmp;
        
        if (tmp.partitions[0].type == kMBRTypeGPTProtective) {
            has_protective_mbr = true;
            info("Protective MBR found");
        } else {
            // Invalid GPT protective MBR
            warning("Invalid protective MBR.");
        }
        
    } else {
        info("No protective MBR found.");
    }
    
    off_t offset = hfs->block_size;
    debug("default offset: %lf", offset);
    if (has_protective_mbr) {
        offset = hfs->block_size * tmp.partitions[0].first_sector_lba;
        debug("updated offset to %f", offset);
    }
    *header = *(GPTHeader*)(buf+offset);
    
    FREE_BUFFER(buf);
    
    return 0;
}

bool gpt_sniff(HFSVolume* hfs)
{
    GPTHeader header;
    int result = gpt_get_header(hfs, &header, NULL);
    if (result < 0) { perror("get gpt header"); return false; }
    
    result = memcmp(&header.signature, "EFI PART", 8);
    
    return (!result);
}


void gpt_print(HFSVolume* hfs)
{
    uuid_string_t uuid_str;
    uuid_t* uuid_ptr;
    
    MBR* mbr;
    INIT_BUFFER(mbr, sizeof(MBR));
    
    GPTHeader* header;
    INIT_BUFFER(header, sizeof(GPTHeader));
    
    int result = gpt_get_header(hfs, header, mbr);
    if (result == -1) { perror("get gpt header"); goto CLEANUP; }
    
    uuid_ptr = gpt_swap_uuid(&header->uuid);
    uuid_unparse(*uuid_ptr, uuid_str);
    
    if (mbr->partitions[0].type)
        mbr_print(hfs, mbr);
    
    PrintHeaderString("GPT Partition Map");
    PrintUIHex              (header, revision);
    PrintDataLength         (header, header_size);
    PrintUIHex              (header, header_crc32);
    PrintUIHex              (header, reserved);
    PrintUI                 (header, current_lba);
    PrintUI                 (header, backup_lba);
    PrintUI                 (header, first_lba);
    PrintUI                 (header, last_lba);
    _PrintDataLength        ("(size)", (header->last_lba * hfs->block_size) - (header->first_lba * hfs->block_size) );
    PrintAttributeString    ("uuid", uuid_str);
    PrintUI                 (header, partition_start_lba);
    PrintUI                 (header, partition_entry_count);
    PrintDataLength         (header, partition_entry_size);
    PrintUIHex              (header, partition_crc32);
    
    off_t offset = header->partition_start_lba * hfs->block_size;
    size_t size = header->partition_entry_count * header->partition_entry_size;
    
    void* buf;
    INIT_BUFFER(buf, size);
    
    result = hfs_read_range(buf, hfs, size, offset);
    if (result < 0) {
        perror("read");
        exit(1);
    }
    
    for(int i = 0; i < header->partition_entry_count; i++) {
        const char* type = NULL;
        
        GPTPartitionEntry partition = ((GPTPartitionEntry*)buf)[i];
        
        if (partition.first_lba == 0 && partition.last_lba == 0) break;
        
        PrintHeaderString("Partition: %d (%llu)", i + 1, offset);
        
        uuid_ptr = gpt_swap_uuid(&partition.type_uuid);
        uuid_unparse(*uuid_ptr, uuid_str);
        type = gpt_partition_type_str(*uuid_ptr, NULL);
        PrintAttributeString("type", "%s (%s)", type, uuid_str);
        
        uuid_ptr = gpt_swap_uuid(&partition.uuid);
        uuid_unparse(*uuid_ptr, uuid_str);
        PrintAttributeString("uuid", "%s", uuid_str);
        
        PrintAttributeString("first_lba", "%llu", partition.first_lba);
        PrintAttributeString("last_lba", "%llu", partition.last_lba);
        _PrintDataLength("(size)", (partition.last_lba * hfs->block_size) - (partition.first_lba * hfs->block_size) );
        _PrintRawAttribute("attributes", &partition.attributes, sizeof(partition.attributes), 2);
        wchar_t name[37] = {0};
        for(int c = 0; c < 37; c++) name[c] = partition.name[c];
        name[36] = '\0';
        PrintAttributeString("name", "%ls", name);
        
        offset += sizeof(GPTPartitionEntry);
    }
    FREE_BUFFER(buf);
    
CLEANUP:
    FREE_BUFFER(mbr);
    FREE_BUFFER(header);
}

// note: allocates space for partitions
bool gpt_partitions(HFSVolume* hfs, Partition partitions[128], unsigned* count) {
    GPTHeader* header;
    int result;
    off_t offset;
    size_t size;
    void* buf = NULL;
    bool err = false;
    
    INIT_BUFFER(header, sizeof(GPTHeader));
    result = gpt_get_header(hfs, header, NULL);
    if (result == -1) { err = true; goto EXIT; }
    
    offset = header->partition_start_lba * hfs->block_size;
    size = header->partition_entry_count * header->partition_entry_size;
    
    INIT_BUFFER(buf, size);
    result = hfs_read_range(buf, hfs, size, offset);
    if (result < 0) { err = true; goto EXIT; }
    
    uuid_string_t uuid_str;
    uuid_t* uuid_ptr;
    const char* type;
    type = NULL;
    PartitionHint hint;
    
    for(*count = 0; *count < header->partition_entry_count; (*count)++) {
        GPTPartitionEntry partition = ((GPTPartitionEntry*)buf)[*count];
        if (partition.first_lba == 0 && partition.last_lba == 0) break;
        
        uuid_ptr = gpt_swap_uuid(&partition.type_uuid);
        uuid_unparse(*uuid_ptr, uuid_str);
        type = gpt_partition_type_str(*uuid_ptr, &hint);
        
        partitions[*count].offset = partition.first_lba * hfs->block_size;
        partitions[*count].length = (partition.last_lba * hfs->block_size) - partitions[*count].offset;
        partitions[*count].hints  = hint;
    }
    
EXIT:
    if (err) perror("get GPT partiions");
    
    FREE_BUFFER(header);
    FREE_BUFFER(buf);
    FREE_BUFFER(partitions);
    
    return (!err);
}
