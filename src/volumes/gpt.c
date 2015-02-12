//
//  gpt.c
//  volumes
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <string.h>             // memcpy, strXXX, etc.
#if defined(__linux__)
    #include <bsd/string.h>     // strlcpy, etc.
#endif

#include <stdbool.h>
#include <math.h>
#include <errno.h>              // errno/perror

#include "gpt.h"
#include "output.h"
#include "_endian.h"
#include "crc32/crc32.h"
#include "logging/logging.h"    // console printing routines


void        _gpt_swap_uuid          (uuid_t* uuid_p, const uuid_t* uuid);
const char* _gpt_partition_type_str (uuid_t uuid, VolType* hint);
int         _gpt_valid_header       (GPTHeader header);
int         _gpt_valid_pmap         (GPTHeader header, const GPTPartitionRecord* pmap);
int         _gpt_load_header        (Volume* vol, GPTHeader* gpt, GPTPartitionRecord* entries) __attribute__((nonnull));
void        PrintGPTHeader          (const GPTHeader* header_p, const Volume* vol);
void        PrintGPTPartitions      (const GPTHeader* header_p, const GPTPartitionRecord* entries_p, Volume* vol);

#pragma mark - Internal

void _gpt_swap_uuid(uuid_t* uuid_p, const uuid_t* uuid)
{
    // Because these UUIDs are fucked up.
    *( (uint32_t*) uuid_p + 0 ) = be32toh(*( (uint32_t*)uuid + 0 ));
    *( (uint16_t*) uuid_p + 2 ) = be16toh(*( (uint16_t*)uuid + 2 ));
    *( (uint16_t*) uuid_p + 3 ) = be16toh(*( (uint16_t*)uuid + 3 ));
    *( (uint64_t*) uuid_p + 1 ) =         *( (uint64_t*)uuid + 1 );
}

const char* _gpt_partition_type_str(uuid_t uuid, VolType* hint)
{
    for(int i=0; gpt_partition_types[i].name[0] != '\0'; i++) {
        uuid_t test   = { "\0" };
        int    result = 0;

        uuid_parse(gpt_partition_types[i].uuid, test);
        result = uuid_compare(uuid, test);

        if ( result == 0 ) {
            if (hint != NULL) *hint = gpt_partition_types[i].voltype;
            return gpt_partition_types[i].name;
        }
    }

    static uuid_string_t string;
    uuid_unparse(uuid, string);
    return string;
}

int _gpt_valid_header(GPTHeader header)
{
    uint32_t crc        = 0;
    uint32_t header_crc = 0;

    if (memcmp(&header.signature, GPT_SIG, 8) != 0) {
        debug("Invalid GPT signature.");
        return false;
    }

    if (header.partitions_entry_size != sizeof(GPTPartitionEntry)) {
        warning("Invalid GPT partition entry size; reports a size of %u while it should be %zu.", header.partitions_entry_size, sizeof(GPTPartitionEntry));
    }

    if (header.partitions_entry_count > 128) {
        warning("Only GPT partition tables up to 128 partitions are supported; the %u addtional partitions will be ignored.", header.partitions_entry_count - 128);
        header.partitions_entry_count = 128;
    }

    if (header.header_size != sizeof(GPTHeader)) {
        debug("Invalid GPT header size; reports a size of %u while it should be %zu.", header.header_size, sizeof(GPTHeader));
        return false;
    }

    // The CRC is calculated with the checksum field zeroed.
    header_crc = header.crc;
    header.crc = 0;
    crc        = crc32(0, &header, header.header_size);

    if (crc != header_crc) {
        warning("Invalid or corrupt GPT header: bad CRC; expected %#010x; got %#010x.", header_crc, crc);
        return false;
    }

    debug("GPT header CRC OK!");
    return true;
}

int _gpt_valid_pmap(GPTHeader header, const GPTPartitionRecord* pmap)
{
    uint32_t crc        = 0;
    uint32_t header_crc = header.partition_table_crc;

    crc = crc32(crc, pmap, sizeof(GPTPartitionRecord));

    if (crc != header_crc) {
        debug("Invalid or corrupt GPT partition map: bad CRC; expected %#010x; got %#010x.", header_crc, crc);
        return false;
    }

    return true;
}

int _gpt_load_header(Volume* vol, GPTHeader* header_out, GPTPartitionRecord* entries_out)
{
    GPTHeader header           = {0};
    MBR       mbr              = {{0}};
    off_t     offset           = 0;
    size_t    length           = 0;
    uint32_t  first_sector_lba = 1;
    length = sizeof(GPTHeader);

    if (vol == NULL) { errno = EINVAL; return -1; }
    if ( (header_out == NULL) && (entries_out == NULL) ) { errno = EINVAL; return -1; }

    // Load the MBR.
    if ( mbr_load_header(vol, &mbr) < 0 )
        return -1;

    // Update the offset of the GPT table, just incase.
    if (mbr.partitions[0].type == kMBRTypeGPTProtective) {
        first_sector_lba = mbr.partitions[0].first_sector_lba;
    }

    // Nasty thing, GPT.  See, typically you'd grab the MBR, look for the
    // protective partition pointer then go get the GPT info.
    // Then 4k sectors arrived.  Now you have no idea how far in to look.
    // So, we read 16KB and search.  Upside: if we find it, we update the
    // sector size to indicate that since it's always at LBA 1. :)

    // Search for the GPT header with block sizes of 512, 1024, 2048, and 4096.
    for (int i = 9; i < 13; i++) {
        size_t block_size = pow(2, i);
        offset = block_size * first_sector_lba;
        debug("Looking for GPT header at offset %ju", (uintmax_t)offset);

        // Read the GPT header.
        if ( vol_read(vol, &header, length, offset) < 0 )
            return -1;

        // Look for the signature.
        if (memcmp(&header.signature, GPT_SIG, 8) == 0) {
            vol->sector_size = block_size;
            break;
        }
    }

    // Verify the header.
    if (_gpt_valid_header(header) != true)
        return -1;

    if (entries_out != NULL) {
        char*              buf     = NULL;
        GPTPartitionRecord entries = {{{0}}};

        // Determine start of partition array
        offset = header.partition_table_start_lba * vol->sector_size;
        length = header.partitions_entry_count * header.partitions_entry_size;

        // Read the partition array
        ALLOC(buf, length);
        if ( vol_read(vol, (uint8_t*)buf, length, offset) < 0 )
            return -1;

        // Verify the partition map.
        if (_gpt_valid_pmap(header, (const GPTPartitionRecord*)buf) != true)
            return -1;
        else
            debug("GPT partition map CRC OK!");

        // Iterate over the partitions and update the volume record
        FOR_UNTIL(i, header.partitions_entry_count) {
            GPTPartitionEntry partition;

            // Grab the next partition structure
            partition = ((GPTPartitionEntry*)buf)[i];
            if ((partition.first_lba == 0) && (partition.last_lba == 0))
                break;

            // Copy entry struct
            entries[i] = partition;
        }

        // Clean up
        FREE(buf);

        if (entries_out != NULL) memcpy(*entries_out, entries, sizeof(entries));
    }

    if (header_out != NULL) *header_out = header;

    return 0;
}

void PrintGPTHeader(const GPTHeader* header_p, const Volume* vol)
{
    uuid_string_t uuid_str = "";
    uuid_t        uuid     = {0};
    out_ctx*      ctx      = vol->ctx;

    _gpt_swap_uuid(&uuid, &header_p->uuid);
    uuid_unparse(uuid, uuid_str);

    PrintUIHex              (ctx, header_p, revision);
    PrintDataLength         (ctx, header_p, header_size);
    PrintUIHex              (ctx, header_p, crc);
    PrintUIHex              (ctx, header_p, reserved);
    PrintUI                 (ctx, header_p, current_lba);
    PrintUI                 (ctx, header_p, backup_lba);
    PrintUI                 (ctx, header_p, first_lba);
    PrintUI                 (ctx, header_p, last_lba);
    if (vol != NULL)
        _PrintDataLength (ctx, "(size)", (header_p->last_lba * vol->sector_size) - (header_p->first_lba * vol->sector_size) );

    PrintAttribute          (ctx, "uuid", uuid_str);
    PrintUI                 (ctx, header_p, partition_table_start_lba);
    PrintUI                 (ctx, header_p, partitions_entry_count);
    PrintDataLength         (ctx, header_p, partitions_entry_size);
    PrintUIHex              (ctx, header_p, partition_table_crc);
}

void PrintGPTPartitions(const GPTHeader* header_p, const GPTPartitionRecord* entries_p, Volume* vol)
{
    uuid_string_t uuid_str = "";
    uuid_t        uuid     = {0};
    wchar_t       name[37] = {0};
    const char*   type     = NULL;
    out_ctx*      ctx      = vol->ctx;

    FOR_UNTIL(i, header_p->partitions_entry_count) {
        GPTPartitionEntry partition = (*entries_p)[i];

        if ((partition.first_lba == 0) && (partition.last_lba == 0)) break;

        BeginSection(ctx, "Partition: %d (%llu)", i + 1, (partition.first_lba * vol->sector_size));

        _gpt_swap_uuid(&uuid, (const uuid_t*)&partition.type_uuid);
        uuid_unparse(uuid, uuid_str);
        type = _gpt_partition_type_str(uuid, NULL);
        PrintAttribute(ctx, "type", "%s (%s)", type, uuid);

        _gpt_swap_uuid(&uuid, (const uuid_t*)&partition.uuid);
        uuid_unparse(uuid, uuid_str);
        PrintAttribute(ctx, "uuid", "%s", uuid_str);

        PrintAttribute(ctx, "first_lba", "%llu", partition.first_lba);
        PrintAttribute(ctx, "last_lba", "%llu", partition.last_lba);
        _PrintDataLength(ctx, "(size)", (partition.last_lba * vol->sector_size) - (partition.first_lba * vol->sector_size) );
        _PrintRawAttribute(ctx, "attributes", &partition.attributes, sizeof(partition.attributes), 2);
        for(int c = 0; c < 37; c++) name[c] = partition.name[c];
        name[36] = '\0';
        PrintAttribute(ctx, "name", "%ls", name);

        EndSection(ctx);
    }
}

#pragma mark - External

/**
   Tests a volume to see if it contains a GPT partition map.
   @return Returns -1 on error (check errno), 0 for NO, 1 for YES.
 */
int gpt_test(Volume* vol)
{
    GPTHeader          header = {0};
    GPTPartitionRecord parts  = {{{0}}};

    debug("GPT test");

    _gpt_load_header(vol, &header, &parts);

    // Verify this is a valid GPT header.
    if ( _gpt_valid_header(header) != true )
        return false;

    return true;
}

/**
   Updates a volume with sub-volumes for any defined partitions.
   @return Returns -1 on error (check errno), 0 for success.
 */
int gpt_load(Volume* vol)
{
    GPTHeader          header  = {0};
    GPTPartitionRecord entries = {{{0}}};
    off_t              offset  = 0;
    size_t             length  = 0;

    vol->type    = kVolTypePartitionMap;
    vol->subtype = kPMTypeGPT;

    debug("GPT load");

    // Load GPT header
    if ( _gpt_load_header(vol, &header, &entries) < 0 )
        return -1;

    // Iterate over the partitions and update the volume record
    FOR_UNTIL(i, header.partitions_entry_count) {
        uuid_string_t     uuid_str    = "";
        uuid_t            uuid        = {0};
        wchar_t           wcname[100] = {'\0'};
        char              name[100]   = {'\0'};
        Volume*           p           = NULL;
        VolType           type        = {0};
        GPTPartitionEntry* partition;

        // Grab the next partition structure
        partition = &(entries[i]);
        if ((partition->first_lba == 0) && (partition->last_lba == 0))
            break;

        // Calculate the size of the partition
        offset = partition->first_lba * vol->sector_size;
        length = (partition->last_lba * vol->sector_size) - offset;

        // Add partition to volume
        p      = vol_make_partition(vol, i, offset, length);
        if (p == NULL) {
            perror("vol_make_partition");
            warning("subpartition creation failed");
            continue;
        }

        // Update partition type with hint
        _gpt_swap_uuid(&uuid, (const uuid_t*)&(partition->type_uuid));
        uuid_unparse(uuid, uuid_str);
        const char* desc = _gpt_partition_type_str(uuid, &type);
        p->type = type;

        FOR_UNTIL(j, j < 36 && partition->name[j] != '\0') {
            wcname[j] = partition->name[j];
        }
        
        wcstombs(name, wcname, 100);
        (void)strlcpy(p->desc, name, 36);
        (void)strlcpy(p->native_desc, desc, 100);
    }

    return 0;
}

/**
   Prints a description of the GPT structure and partition information to stdout.
   @return Returns -1 on error (check errno), 0 for success.
 */
int gpt_dump(Volume* vol)
{
    GPTHeader          header  = {0};
    GPTPartitionRecord entries = {{{0}}};
    out_ctx*           ctx     = vol->ctx;

    debug("GPT dump");

    if ( _gpt_load_header(vol, &header, &entries) < 0)
        return -1;

    BeginSection(ctx, "GPT Partition Map");

    PrintGPTHeader((const GPTHeader*)&header, vol);
    PrintGPTPartitions((const GPTHeader*)&header, (const GPTPartitionRecord*)&entries, vol);

    EndSection(ctx);

    return 0;
}

PartitionOps gpt_ops = {
    .name = "GPT",
    .test = gpt_test,
    .dump = gpt_dump,
    .load = gpt_load,
};
