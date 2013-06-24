//
//  format_sniffers.c
//  hfsinspect
//
//  Created by Adam Knight on 6/21/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "format_sniffers.h"
#include "hfs_pstruct.h"
#include "hfs_io.h"
#include "hfs_endian.h"

const unsigned lba_size = 512;

bool sniff_and_print(HFSVolume* hfs)
{
    if (gpt_sniff(hfs)) {
        print_gpt(hfs);
    } else if (cs_sniff(hfs)) {
        print_cs(hfs);
    } else {
        warning("Unknown disk or partition type.");
        return false;
    }
    return true;
}

int sniff_partitions(const HFSVolume* hfs, range_ptr partitions, int* count)
{
    // Check disk type
    // Call format-specific method to get partition list
    // Extract ranges
    // Set reference params for the ranges array and count
    // Return the number of partitions found.
    return 0;
}

#pragma mark GPT

void decode_gpt_uuid(uuid_t* uuid)
{
    // Because these UUIDs are fucked up.
    void* uuid_buf = uuid;
    *( (u_int32_t*) uuid + 0 ) = OSSwapInt32(*( (u_int32_t*)uuid_buf + 0 ));
    *( (u_int16_t*) uuid + 2 ) = OSSwapInt16(*( (u_int16_t*)uuid_buf + 2 ));
    *( (u_int16_t*) uuid + 3 ) = OSSwapInt16(*( (u_int16_t*)uuid_buf + 3 ));
    *( (u_int64_t*) uuid + 1 ) =             *( (u_int64_t*)uuid_buf + 1 );
}

const char* partition_type_str(uuid_t uuid)
{
    for(int i=0; i < GPTPartitionTypesCount; i++) {
        uuid_t test;
        uuid_parse(types[i].uuid, test);
        int result = uuid_compare(uuid, test);
        if ( result == 0 ) return types[i].name;
    }
    
    static uuid_string_t string;
    uuid_unparse(uuid, string);
    return string;
}

int get_gpt_header(HFSVolume* hfs, GPTHeader* header)
{
    char* buf = valloc(4096);
    if ( buf == NULL ) {
        perror("valloc");
        return -1;
    }
    
    ssize_t bytes;
    bytes = hfs_read_range(buf, hfs, lba_size, lba_size);
    if (bytes < 0) {
        perror("read");
        return -1;
    }
    
    *header = *(GPTHeader*)(buf);
    free(buf);
    
    return 0;
}

bool gpt_sniff(HFSVolume* hfs)
{
    GPTHeader header;
    int result = get_gpt_header(hfs, &header);
    if (result == -1) { perror("get gpt header"); return false; }
    
    result = memcmp(&header.signature, "EFI PART", 8);
    
    return (!result);
}

void print_gpt(HFSVolume* hfs)
{
    GPTHeader* header = malloc(sizeof(GPTHeader));
    int result = get_gpt_header(hfs, header);
    if (result == -1) { perror("get gpt header"); return; }
    
    uuid_string_t   uuid_str;
    uuid_t*         uuid_ptr = &header->uuid;
    decode_gpt_uuid(uuid_ptr);
    uuid_unparse(*uuid_ptr, uuid_str);
    
    PrintHeaderString("GPT Partition Map");
    PrintUIHex(header, revision);
    PrintUIHex(header, header_size);
    PrintUIHex(header, header_crc32);
    PrintUIHex(header, reserved);
    PrintUIHex(header, current_lba);
    PrintUIHex(header, backup_lba);
    PrintUIHex(header, first_lba);
    PrintUIHex(header, last_lba);
    PrintAttributeString("uuid", uuid_str);
    PrintUIHex(header, partition_start_lba);
    PrintUIHex(header, partition_entry_count);
    PrintUIHex(header, partition_entry_size);
    PrintUIHex(header, partition_crc32);
    
    off_t offset = header->partition_start_lba * lba_size;
    size_t size = header->partition_entry_count * header->partition_entry_size;
    
    void* buf = valloc(size);
    result = hfs_read_range(buf, hfs, size, offset);
    if (result < 0) {
        perror("read");
        exit(1);
    }
    
    for(int i = 0; i < header->partition_entry_count; i++) {
        const char*     type = NULL;
        
        GPTPartitionEntry partition = *(GPTPartitionEntry*)((char*)buf + (i * header->partition_entry_size));
        
        if (partition.first_lba == 0 && partition.last_lba == 0) break;
        
        PrintHeaderString("Partition: %d (%llu)", i, offset);
        
        uuid_ptr = &partition.type_uuid;
        decode_gpt_uuid(uuid_ptr);
        uuid_unparse(*uuid_ptr, uuid_str);
        type = partition_type_str(*uuid_ptr);
        PrintAttributeString("type", "%s (%s)", type, uuid_str);
        
        uuid_ptr = &partition.uuid;
        decode_gpt_uuid(uuid_ptr);
        uuid_unparse(*uuid_ptr, uuid_str);
        PrintAttributeString("uuid", "%s", uuid_str);
        
        PrintAttributeString("first_lba", "%llu", partition.first_lba);
        PrintAttributeString("last_lba", "%llu", partition.last_lba);
        wchar_t name[37] = {0};
        for(int c = 0; c < 37; c++) name[c] = partition.name[c];
        name[36] = '\0';
        PrintAttributeString("name", "%ls", name);
        
        offset += sizeof(GPTPartitionEntry);
    }
}

#pragma mark Core Storage

int get_cs_volume_header(HFSVolume* hfs, CSVolumeHeader* header)
{
    char* buf = valloc(4096);
    if ( buf == NULL ) {
        perror("valloc");
        return -1;
    }
    
    ssize_t bytes = hfs_read_range(buf, hfs, lba_size, 0);
    if (bytes < 0) {
        perror("read");
        return -1;
    }
    
    *header = *(CSVolumeHeader*)(buf);
    free(buf);
    
    return 0;
}

bool cs_sniff(HFSVolume* hfs)
{
    CSVolumeHeader header;
    int result = get_cs_volume_header(hfs, &header);
    if (result == -1) { perror("get cs header"); return false; }
    result = memcmp(&header.signature, "CS", 2);
    
    return (!result);
}

void print_cs_block_header(CSBlockHeader* header)
{
    PrintHeaderString("Core Storage Block Header");
    
    PrintUIHex      (header, checksum);
    PrintUIHex      (header, checksum_seed);
    PrintUI         (header, version);
    PrintUIHex      (header, block_type);
    PrintUI         (header, block_number);
    PrintDataLength (header, block_size);
    PrintBinaryDump (header, flags);
}

void print_cs_block_11(CSMetadataBlockType11* block)
{
    print_cs_block_header(&block->block_header);

    PrintHeaderString("Core Storage Block Type 11");
    
    PrintDataLength(block, size);
    PrintUIHex(block, checksum);
    PrintUIHex(block, checksum_seed);
    PrintUI(block, volume_groups_descriptor_offset);
    PrintUI(block, xml_offset);
    PrintDataLength(block, xml_size);
    PrintUI(block, backup_volume_header_block);
    PrintUI(block, record_count);
    for (int i = 0; i < block->record_count; i++) {
        PrintHeaderString("Record %d", i);
        PrintAttributeString("index", "%llu", block->records[i].index);
        PrintAttributeString("start", "%llu", block->records[i].start);
        PrintAttributeString("end", "%llu", block->records[i].end);
    }
}

void print_cs(HFSVolume* hfs)
{
    CSVolumeHeader* header = malloc(sizeof(CSVolumeHeader));
    int result = get_cs_volume_header(hfs, header);
    if (result == -1) { perror("get cs header"); return; }
    
    print_cs_block_header(&header->block_header);
    
    PrintHeaderString("Core Storage Volume");
    
    PrintDataLength (header, physical_size);
    PrintHFSChar    (header, signature);
    PrintUI         (header, checksum_algo);
    
    PrintUI         (header, metadata_count);
    PrintDataLength (header, metadata_block_size);
    PrintDataLength (header, metadata_size);
    
    static int buf_size = 1024*12;
    void* buf = valloc(buf_size);
    for(int i = 0; i < header->metadata_count; i++) {
        memset(buf, 0, buf_size);
        u_int64_t block_number = header->metadata_blocks[i];
        if (block_number == 0) continue;
        size_t size = header->block_header.block_size;
        off_t offset = (header->metadata_blocks[i] * size);
        
        print("\n# Metadata Block %d (block %#llx; offset %llu)", i, block_number, offset);
        _PrintDataLength("offset", offset);
        
        ssize_t bytes = hfs_read_range(buf, hfs, buf_size, offset);
        if (bytes < 0) { perror("read"); return; }
        
        CSMetadataBlockType11* block = ((CSMetadataBlockType11*)buf);
        print_cs_block_11(block);
        VisualizeData(buf, buf_size);
    }
    free(buf); buf = NULL;
    
    PrintHeaderString("Encryption Key Info");
    PrintDataLength (header, encryption_key_size); //bytes
    PrintUI         (header, encryption_key_algo);
    _PrintRawAttribute("encryption_key_data", &header->encryption_key_data, MIN(header->encryption_key_size, 128), 16);
}
