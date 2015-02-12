//
//  corestorage.c
//  volumes
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//


#include <string.h>             // memcpy, strXXX, etc.

#if defined(__linux__)
    #include <malloc.h>
    #define malloc_size malloc_usable_size
#else
    #include <malloc/malloc.h>      // malloc_size
#endif

#include "volumes/corestorage.h"

#include "volumes/output.h"
#include "hfsinspect/stringtools.h"

#include "crc32c/crc32c.h"
#include "logging/logging.h"    // console printing routines
#include "memdmp/memdmp.h"


void cs_dump_all_blocks(Volume* vol, CSVolumeHeader* vh);

uint32_t cs_crc32c(uint32_t seed, const uint8_t* base, uint32_t length)
{
    // All CRCs exempt the first 8 bytes as they hold the seed and CRC themselves.
    return crc32c(seed, (base+8), (length-8));
}

int cs_verify_block(const CSVolumeHeader* vh, const uint8_t* block, size_t nbytes)
{
    if (vh->checksum_algo == 1) {
        const CSBlockHeader* bh  = (CSBlockHeader*)block;
        uint32_t             crc = 0;

        if (bh->version == 0) return -1;
        if (bh->checksum == 0) return 0;

        nbytes = bh->block_size;
        crc    = cs_crc32c(bh->checksum_seed, block, nbytes);

        if (crc != bh->checksum) {
            warning("corrupt CS block: CRC: %#x; expected: %#x\n", crc, bh->checksum);
            return -1;
        } else {
            debug("block checksum valid");
        }

        return 0;
//    } else {
//        debug("Unknown checksum algo: %u", vh->checksum_algo);
    }

    return -1;
}

int cs_get_volume_header(Volume* vol, CSVolumeHeader* header)
{
    // TODO: Refactor to not use dynamic arrays.
    size_t  buf_size = sizeof(CSVolumeHeader);
    uint8_t buf[buf_size];

    memset(&buf, 0, sizeof(buf));

    // FIXME: Need to copy only read bytes, not all requested bytes!
    if ( vol_read(vol, buf, buf_size, 0) < 0 )
        return -1;

    memcpy(header, buf, buf_size);

    return cs_verify_block(header, buf, buf_size);
}

ssize_t cs_get_metadata_block(uint8_t** buf, const Volume* vol, const CSVolumeHeader* header, unsigned block)
{
    CSBlockHeader* bh       = NULL;
    size_t         buf_size = header->md_block_size;
    off_t          offset   = 0;
    ssize_t        bytes    = 0;

READ:
    offset = block * (size_t)header->md_block_size;
    bytes  = vol_read(vol, *buf, buf_size, offset);
    if (bytes < 0)   return -1;
    bh     = (CSBlockHeader*)*buf;
    if (bh->version != 1)
        return -1;

    if (bh->block_size > buf_size) {
        buf_size = bh->block_size;
        if (malloc_size(*buf) < buf_size) {
            *buf = realloc(*buf, buf_size);
            if (*buf == NULL) {
                perror("realloc");
                abort();
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

    CSVolumeHeader header = {{0}};

    if ( cs_get_volume_header(vol, &header) < 0)   return -1;
    if ( memcmp(&header.signature, "CS", 2) == 0 ) { debug("Found a CS pmap."); /* cs_dump(vol); */ return 1; }

    return 0;
}

void PrintCSBlockHeader(out_ctx* ctx, CSBlockHeader* header)
{
    BeginSection(ctx, "CS Block Header");

    PrintUIHex      (ctx, header, checksum);
    PrintUIHex      (ctx, header, checksum_seed);
    PrintUI         (ctx, header, version);
    PrintUIHex      (ctx, header, block_type);

    PrintUIHex      (ctx, header, field_5);
    PrintUIHex      (ctx, header, generation);
    PrintUIHex      (ctx, header, field_7);
    PrintUIHex      (ctx, header, field_8);
    PrintUIHex      (ctx, header, field_9);

    PrintDataLength (ctx, header, block_size);
    PrintUIHex      (ctx, header, field_11);
    PrintUIHex      (ctx, header, field_12);

    EndSection(ctx);
}

void PrintCSVolumeHeader(out_ctx* ctx, CSVolumeHeader* header)
{
    BeginSection(ctx, "CS Volume Header");

    PrintCSBlockHeader(ctx, &header->block_header);

    PrintDataLength (ctx, header, physical_size);
    PrintRawAttribute(ctx, header, field_14, 16);

    PrintUIChar     (ctx, header, signature);
    PrintUI         (ctx, header, checksum_algo);

    PrintUI         (ctx, header, md_count);
    PrintDataLength (ctx, header, md_block_size);
    PrintDataLength (ctx, header, md_size);
    FOR_UNTIL(i, 8) {
        if (header->md_blocks[i]) {
            char title[50] = "";
            sprintf(title, "md_blocks[%u]", i);
            PrintAttribute(ctx, title, "%#x", header->md_blocks[i]);
        }
    }

    PrintDataLength (ctx, header, encryption_key_size);
    PrintUI         (ctx, header, encryption_key_algo);

    char str[2048] = "";
    memstr(str, 16, &header->encryption_key_data[0], header->encryption_key_size, 1024);
    PrintAttribute(ctx, "encryption_key_data", str);

    memstr(str, 16, (char*)&header->physical_volume_uuid, 16, 1024);
    PrintAttribute(ctx, "physical_volume_uuid", str);

    memstr(str, 16, (char*)&header->logical_volume_group_uuid, 16, 1024);
    PrintAttribute(ctx, "logical_volume_group_uuid", str);

    //    memstr(str, 16, (char*)&header->reserved10[0], 176, 1024);
    //    VisualizeData((char*)&header->reserved10, 176);
    //    PrintAttribute(ctx, "reserved10", str);

    EndSection(ctx);
}

void PrintCSMetadataBlockType11(out_ctx* ctx, CSMetadataBlockType11* block)
{
    BeginSection(ctx, "CS Metadata Block (Type 0x11)");

    PrintCSBlockHeader(ctx, &block->block_header);

    PrintDataLength (ctx, block, metadata_size);
    PrintUIHex      (ctx, block, field_2);
    PrintUIHex      (ctx, block, checksum);
    PrintUIHex      (ctx, block, checksum_seed);

    PrintUIHex      (ctx, block, vol_grps_desc_off);
    PrintUIHex      (ctx, block, plist_offset);
    PrintDataLength (ctx, block, plist_size);
    PrintDataLength (ctx, block, plist_size_copy);

    PrintUIHex      (ctx, block, field_10);

    PrintUIHex      (ctx, block, secondary_vhb_off);
    PrintUI         (ctx, block, ukwn_record_count);

    BeginSection(ctx, "Unknown Records");
    Print(ctx, "%-12s %-12s %-12s", "Generation", "Curly", "Moe");
    for (int i = 0; i < block->ukwn_record_count; i++) {
        uint64_t* record = (void*)&block->ukwn_records[i];
        Print(ctx, "%-#12x %-#12x %-#12x", record[0], record[1], record[2]);
    }
    EndSection(ctx); // records

    EndSection(ctx); // block
}

int cs_dump(Volume* vol)
{
    char            _header[1024] = "";
    CSVolumeHeader* header        = (CSVolumeHeader*)&_header;
    size_t          block_size    = 0;
    uint8_t*        buf           = NULL;
    out_ctx*        ctx           = vol->ctx;

    debug("CS dump");

    if ( cs_get_volume_header(vol, header) == -1)
        return -1;

    BeginSection(ctx, "Core Storage Volume");
    PrintCSVolumeHeader(ctx, header);

    block_size = header->md_block_size;
    ALLOC(buf, block_size);

    FOR_UNTIL(i, header->md_count) {
        uint64_t       block_number = header->md_blocks[i];
        ssize_t        bytes        = 0;
        CSBlockHeader* block_header = NULL;

        if (block_number == 0) continue;

        memset(buf, 0, malloc_size(buf));
        bytes        = cs_get_metadata_block(&buf, vol, header, block_number);

        if (bytes < 0) continue;

        block_header = (CSBlockHeader*)buf;

        BeginSection(ctx, "CS Metadata Block %d (block %#llx; offset %#llx)", i, block_number, block_number * header->md_block_size);

        switch (block_header->block_type) {
            case kCSVolumeHeaderBlock:
            {
                PrintCSVolumeHeader(ctx, (void*)buf);
                break;
            }

            case kCSDiskLabelBlock:
            {
                PrintCSMetadataBlockType11(ctx, (void*)buf);
//                VisualizeData((void*)buf, header->md_block_size);
                break;
            }

            default:
            {
                PrintCSBlockHeader(ctx, block_header);
//                VisualizeData((void*)buf, header->md_block_size);
                break;
            }
        }

        {
            size_t   size   = header->md_size;
            uint8_t* block  = valloc(size);
            ssize_t  nbytes = vol_read(vol, block, size, block_number * header->md_block_size);

            if (nbytes) {
                VisualizeData((char*)block, size);
            }
        }

        EndSection(ctx);
    }

    FREE(buf);

//    BeginSection(ctx, "Block Dump");
//    cs_dump_all_blocks(vol, header);
//    EndSection(ctx);

    EndSection(ctx); // volume

    return 0;
}

//scavenges for possible metadata blocks at the end of the volume.
void cs_dump_all_blocks(Volume* vol, CSVolumeHeader* vh)
{
    size_t   block_size   = vh->md_block_size;
    uint8_t* buf          = NULL;
    ALLOC(buf, block_size);
    uint64_t block_number = vh->md_blocks[0];
    uint64_t total_blocks = (vh->physical_size / vh->md_block_size);
    out_ctx* ctx          = vol->ctx;

    for (; block_number < total_blocks; block_number++) {
        memset(buf, 0, malloc_size(buf));

        ssize_t bytes = cs_get_metadata_block(&buf, vol, vh, block_number);
        if (bytes < 0) {
            fprintf(stderr, "Invalid block at %#jx\n", (intmax_t)block_number);
            continue;
        }

        CSBlockHeader* block_header = (CSBlockHeader*)buf;

        BeginSection(ctx, "CS Metadata Block %d (block %#jx)", -1, (intmax_t)block_number);

        switch (block_header->block_type) {
            case kCSVolumeHeaderBlock:
            {
                PrintCSVolumeHeader(ctx, (void*)buf);
                break;
            }

            case kCSDiskLabelBlock:
            {
                PrintCSMetadataBlockType11(ctx, (void*)buf);
                break;
            }

            default:
            {
                PrintCSBlockHeader(ctx, block_header);
                VisualizeData((void*)buf, vh->md_block_size);
                break;
            }
        }

        EndSection(ctx);
    }

    FREE(buf);
}

PartitionOps cs_ops = {
    .name = "Core Storage",
    .test = cs_test,
    .dump = cs_dump,
    .load = NULL,
};
