//
//  mbr.c
//  volumes
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <errno.h>              // errno/perror
#include <string.h>             // memcpy, strXXX, etc.
#if defined(__linux__)
    #include <bsd/string.h>     // strlcpy, etc.
#endif

#include "mbr.h"
#include "output.h"
#include "_endian.h"

#include "logging/logging.h"    // console printing routines


int mbr_load_header(Volume* vol, MBR* mbr)
{
    if ( vol_read(vol, (Bytes)mbr, sizeof(MBR), 0) < 0 )
        return -1;

    return 0;
}

int mbr_test(Volume* vol)
{
    debug("MBR test");

    MBR  mbr        = {{0}};
    char mbr_sig[2] = { 0x55, 0xaa };

    if ( mbr_load_header(vol, &mbr) < 0 )
        return -1;

    if (memcmp(&mbr_sig, &mbr.signature, 2) == 0) { debug("Found an MBR pmap."); return 1; }

    return 0;
}

int mbr_load(Volume* vol)
{
    debug("MBR load");

    vol->type    = kVolTypePartitionMap;
    vol->subtype = kPMTypeMBR;

    MBR mbr = {{0}};

    if ( mbr_load_header(vol, &mbr) < 0)
        return -1;

    FOR_UNTIL(i, 4) {
        if (mbr.partitions[i].type) {
            Volume*      v      = NULL;
            MBRPartition p;
            VolType      hint   = kVolTypeSystem;
            off_t        offset = 0;
            size_t       length = 0;

            p      = mbr.partitions[i];
            offset = p.first_sector_lba * vol->sector_size;
            length = p.sector_count * vol->sector_size;

            v      = vol_make_partition(vol, i, offset, length);

            char const* name = mbr_partition_type_str(p.type, &hint);
            if (name != NULL) {
                v->type = hint;
            }
        }
    }

    return 0;
}

const char* mbr_partition_type_str(uint16_t type, VolType* hint)
{
    static char type_str[100];

    FOR_UNTIL(i, 256) {
        if (mbr_partition_types[i].type == type) {
            if (hint != NULL) *hint = mbr_partition_types[i].voltype;
            (void)strlcpy(type_str, mbr_partition_types[i].name, 99);
        }
    }

    if (type_str == NULL)
        (void)strlcpy(type_str, "unknown", 100);

    return type_str;
}

int mbr_dump(Volume* vol)
{
    const char* type_str = NULL;
    MBR*        mbr      = NULL;
    out_ctx*    ctx      = vol->ctx;

    debug("MBR dump");

    ALLOC(mbr, sizeof(MBR));

    if ( mbr_load_header(vol, mbr) < 0)
        return -1;

    BeginSection(ctx, "Master Boot Record");
    PrintAttribute(ctx, "signature", "%#x", be16toh(*(uint16_t*)mbr->signature));

    FOR_UNTIL(i, 4) {
        MBRPartition* partition = &mbr->partitions[i];
        if (partition->type == 0) continue;

        type_str = mbr_partition_type_str(partition->type, NULL);

        BeginSection(ctx, "Partition %d", i + 1);
        PrintUIHex  (ctx, partition, status);
        PrintUI     (ctx, partition, first_sector.head);
        PrintUI     (ctx, partition, first_sector.cylinder);
        PrintUI     (ctx, partition, first_sector.sector);
        PrintAttribute(ctx, "type", "0x%02X (%s)", partition->type, type_str);
        PrintUI     (ctx, partition, last_sector.head);
        PrintUI     (ctx, partition, last_sector.cylinder);
        PrintUI     (ctx, partition, last_sector.sector);
        PrintUI     (ctx, partition, first_sector_lba);
        PrintUI     (ctx, partition, sector_count);
        _PrintDataLength(ctx, "(size)", (partition->sector_count * vol->sector_size));
        EndSection(ctx);
    }

    EndSection(ctx);

    return 0;
}

PartitionOps mbr_ops = {
    .name = "MBR",
    .test = mbr_test,
    .dump = mbr_dump,
    .load = mbr_load,
};
