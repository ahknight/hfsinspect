//
//  corestorage.c
//  hfsinspect
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <stdio.h>
#include "corestorage.h"
#include "hfs_io.h"
#include "hfs_pstruct.h"

int cs_get_volume_header(HFSVolume* hfs, CSVolumeHeader* header)
{
    char* buf = valloc(4096);
    if ( buf == NULL ) {
        perror("valloc");
        return -1;
    }
    
    ssize_t bytes = hfs_read_range(buf, hfs, hfs->block_size, 0);
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
    int result = cs_get_volume_header(hfs, &header);
    if (result == -1) { perror("get cs header"); return false; }
    result = memcmp(&header.signature, "CS", 2);
    
    return (!result);
}

void cs_print_block_header(CSBlockHeader* header)
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

void cs_print_block_11(CSMetadataBlockType11* block)
{
    cs_print_block_header(&block->block_header);
    
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

void cs_print(HFSVolume* hfs)
{
    CSVolumeHeader* header = malloc(sizeof(CSVolumeHeader));
    int result = cs_get_volume_header(hfs, header);
    if (result == -1) { perror("get cs header"); return; }
    
    cs_print_block_header(&header->block_header);
    
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
        cs_print_block_11(block);
        VisualizeData(buf, buf_size);
    }
    free(buf); buf = NULL;
    
    PrintHeaderString("Encryption Key Info");
    PrintDataLength (header, encryption_key_size); //bytes
    PrintUI         (header, encryption_key_algo);
    _PrintRawAttribute("encryption_key_data", &header->encryption_key_data, MIN(header->encryption_key_size, 128), 16);
}

