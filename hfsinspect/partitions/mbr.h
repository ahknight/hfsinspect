//
//  mbr.h
//  hfsinspect
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_mbr_h
#define hfsinspect_mbr_h

#include "hfs_structs.h"

#pragma mark - Structures

typedef struct MBRPartition MBRPartition;
struct MBRPartition {
    u_int8_t        status;
    unsigned        first_sector_head : 8;
    unsigned        first_sector_sector : 5;
    unsigned        first_sector_cylinder : 10;
    u_int8_t        type;
    unsigned        last_sector_head : 8;
    unsigned        last_sector_sector : 5;
    unsigned        last_sector_cylinder : 10;
    u_int32_t       first_sector_lba;
    u_int32_t       sector_count;
} __attribute__((aligned(2), packed));

typedef struct MBR {
    char            bootstrap[446];
    MBRPartition    partitions[4];
    char            signature[2];
} MBR;

static unsigned kMBRTypeGPTProtective __attribute__((unused))   = 0xEE;
static unsigned kMBRTypeAppleBoot __attribute__((unused))       = 0xAB;
static unsigned kMBRTypeAppleHFS __attribute__((unused))        = 0xAF;

#pragma mark - Functions

void mbr_print(HFSVolume* hfs, MBR* mbr);

#endif
