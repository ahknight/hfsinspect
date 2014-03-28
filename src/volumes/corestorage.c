//
//  corestorage.c
//  volumes
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "volumes/corestorage.h"
#include "misc/output.h"
#include "misc/stringtools.h"
#include "hfs/output_hfs.h"
#include "crc32c/crc32c.h"

void cs_dump_all_blocks(Volume* vol, CSVolumeHeader* vh);

uint32_t cs_crc32c(uint32_t seed, const Byte* base, uint32_t length)
{
    // All CRCs exempt the first 8 bytes as they hold the seed and CRC themselves.
    return crc32c(seed, (base+8), (length-8));
}

int cs_verify_block(const CSVolumeHeader* vh, const Byte* block, size_t nbytes)
{
    if (vh->checksum_algo == 1) {
        const CSBlockHeader* bh = (CSBlockHeader*)block;
        if (bh->version == 0) return -1;
        if (bh->checksum == 0) return 0;
        nbytes = bh->block_size;
        
        uint32_t crc = cs_crc32c(bh->checksum_seed, block, nbytes);
        
        if (crc != bh->checksum) {
            warning("corrupt CS block: CRC: %#x; expected: %#x\n", crc, bh->checksum);
            return -1;
        }
        
        return 0;
    }
    
    return -1;
}

int cs_get_volume_header(Volume* vol, CSVolumeHeader* header)
{
    size_t buf_size = sizeof(CSVolumeHeader);
    Byte buf[buf_size]; ZERO_STRUCT(buf);
    
    if ( vol_read(vol, buf, buf_size, 0) < 0 )
        return -1;
    
    memcpy(header, buf, buf_size);
    
    return cs_verify_block(header, buf, buf_size);
}

ssize_t cs_get_metadata_block(Byte** buf, const Volume* vol, const CSVolumeHeader* header, unsigned block)
{
    CSBlockHeader* bh = NULL;
    size_t buf_size = header->md_block_size;
    off_t offset = 0;
    ssize_t bytes = 0;

READ:
    offset = block * (size_t)header->md_block_size;
    bytes = vol_read(vol, *buf, buf_size, offset);
    if (bytes < 0) { return -1; }
    
    bh = (CSBlockHeader*)*buf;
    if (bh->version != 1)
        return -1;
    
    if (bh->block_size > buf_size) {
        buf_size = bh->block_size;
        if (malloc_size(*buf) < buf_size) {
            *buf = realloc(*buf, buf_size);
            if (*buf == NULL) {
                perror("realloc");
                exit(1);
            }
        }
        goto READ;
    }

    if (cs_verify_block(header, *buf, buf_size) < 0)
        return -1;
    
    return bytes;
}

int cs_test(Volume* vol)
{
    debug("CS test");
    
    CSVolumeHeader header;
    
    if ( cs_get_volume_header(vol, &header) < 0) { perror("get cs header"); return -1; }
    
    if ( memcmp(&header.signature, "CS", 2) == 0 ) { debug("Found a CS pmap."); /* cs_dump(vol); */ return 1; }
    
    return 0;
}

void cs_print_block_header(CSBlockHeader* header)
{
    BeginSection("CS Block Header");
    
    PrintUIHex      (header, checksum);
    PrintUIHex      (header, checksum_seed);
    PrintUI         (header, version);
    PrintUIHex      (header, block_type);
    
    PrintUIHex      (header, field_5);
    PrintUIHex      (header, generation);
    PrintUIHex      (header, field_7);
    PrintUIHex      (header, field_8);
    PrintUIHex      (header, field_9);
    
    PrintDataLength (header, block_size);
    PrintUIHex      (header, field_11);
    PrintUIHex      (header, field_12);

    EndSection();
}

void cs_print_volume_header(CSVolumeHeader* header)
{
    BeginSection("CS Volume Header");
    
    cs_print_block_header(&header->block_header);
    
    PrintDataLength (header, physical_size);
    PrintRawAttribute(header, field_14, 16);
    
    PrintHFSChar    (header, signature);
    PrintUI         (header, checksum_algo);
    
    PrintUI         (header, md_count);
    PrintDataLength (header, md_block_size);
    PrintDataLength (header, md_size);
    FOR_UNTIL(i, 8) {
        if (header->md_blocks[i]) {
            char title[50] = "";
            sprintf(title, "md_blocks[%u]", i);
            PrintAttribute(title, "%#x", header->md_blocks[i]);
        }
    }
    
    PrintDataLength (header, encryption_key_size);
    PrintUI         (header, encryption_key_algo);
    
    char str[2048] = "";
    memstr(str, 16, &header->encryption_key_data[0], header->encryption_key_size, 1024);
    PrintAttribute("encryption_key_data", str);
    
    memstr(str, 16, (char*)&header->physical_volume_uuid, 16, 1024);
    PrintAttribute("physical_volume_uuid", str);
    
    memstr(str, 16, (char*)&header->logical_volume_group_uuid, 16, 1024);
    PrintAttribute("logical_volume_group_uuid", str);
    
    //    memstr(str, 16, (char*)&header->reserved10[0], 176, 1024);
    //    VisualizeData((char*)&header->reserved10, 176);
    //    PrintAttribute("reserved10", str);
    
    EndSection();
}

void cs_print_block_11(CSMetadataBlockType11* block)
{
    BeginSection("CS Metadata Block (Type 0x11)");
    
    cs_print_block_header(&block->block_header);
    
    PrintDataLength (block, metadata_size);
    PrintUIHex      (block, field_2);
    PrintUIHex      (block, checksum);
    PrintUIHex      (block, checksum_seed);
    
    PrintUIHex      (block, vol_grps_desc_off);
    PrintUIHex      (block, plist_offset);
    PrintDataLength (block, plist_size);
    PrintDataLength (block, plist_size_copy);
    
    PrintUIHex      (block, field_10);
    
    PrintUIHex      (block, secondary_vhb_off);
    PrintUI         (block, ukwn_record_count);
    
    BeginSection("Unknown Records");
    Print("%-12s %-12s %-12s", "Generation", "Curly", "Moe");
    for (int i = 0; i < block->ukwn_record_count; i++) {
        uint64_t *record = (void*)&block->ukwn_records[i];
        Print("%-#12x %-#12x %-#12x", record[0], record[1], record[2]);
    }
    EndSection(); // records
    
    EndSection(); // block
}

int cs_dump(Volume* vol)
{
    debug("CS dump");
    
    char _header[1024] = "";
    CSVolumeHeader *header = (CSVolumeHeader*)&_header;
    
    if ( cs_get_volume_header(vol, header) == -1)
        return -1;
    
    BeginSection("Core Storage Volume");
    cs_print_volume_header(header);

    size_t block_size = header->md_block_size;
    Byte* buf = calloc(4, block_size);
    
    FOR_UNTIL(i, header->md_count) {
        uint64_t block_number = header->md_blocks[i];
        if (block_number == 0) continue;
        
        memset(buf, 0, malloc_size(buf));
        ssize_t bytes = cs_get_metadata_block(&buf, vol, header, block_number);
        if (bytes < 0) continue;
        
        CSBlockHeader *block_header = (CSBlockHeader *)buf;
        
        BeginSection("CS Metadata Block %d (block %#llx; offset %#llx)", i, block_number, block_number * header->md_block_size);
        
        switch (block_header->block_type) {
            case kCSVolumeHeaderBlock:
                cs_print_volume_header((void*)buf);
                break;
                
            case kCSDiskLabelBlock:
                cs_print_block_11((void*)buf);
//                VisualizeData((void*)buf, header->md_block_size);
                break;
                
            default:
                cs_print_block_header(block_header);
//                VisualizeData((void*)buf, header->md_block_size);
                break;
        }
        
        {
            size_t size = header->md_size;
            Byte* block = valloc(size);
            ssize_t nbytes = vol_read(vol, block, size, block_number * header->md_block_size);
            if (nbytes) {
                VisualizeData((char*)block, size);
            }
        }
        
        EndSection();
    }
    
    free(buf); buf = NULL;
    
//    BeginSection("Block Dump");
//    cs_dump_all_blocks(vol, header);
//    EndSection();
    
    EndSection(); // volume

    return 0;
}

//scavenges for possible metadata blocks at the end of the volume.
void cs_dump_all_blocks(Volume* vol, CSVolumeHeader* vh)
{
    size_t block_size = vh->md_block_size;
    Byte* buf = calloc(1, block_size);
    uint64_t block_number = vh->md_blocks[0];
    uint64_t total_blocks = (vh->physical_size / vh->md_block_size);
    
    for (;block_number < total_blocks; block_number++) {
        memset(buf, 0, malloc_size(buf));
        
        ssize_t bytes = cs_get_metadata_block(&buf, vol, vh, block_number);
        if (bytes < 0) {
            fprintf(stderr, "Invalid block at %#jx\n", (intmax_t)block_number);
            continue;
        }
        
        CSBlockHeader *block_header = (CSBlockHeader *)buf;
        
        BeginSection("CS Metadata Block %d (block %#jx)", -1, (intmax_t)block_number);
        
        switch (block_header->block_type) {
            case kCSVolumeHeaderBlock:
                cs_print_volume_header((void*)buf);
                break;
                
            case kCSDiskLabelBlock:
                cs_print_block_11((void*)buf);
                break;
                
            default:
                cs_print_block_header(block_header);
                VisualizeData((void*)buf, vh->md_block_size);
                break;
        }
        
        EndSection();
    }
    
    free(buf); buf = NULL;
}

PartitionOps cs_ops = {
    .name = "Core Storage",
    .test = cs_test,
    .dump = cs_dump,
    .load = NULL,
};
