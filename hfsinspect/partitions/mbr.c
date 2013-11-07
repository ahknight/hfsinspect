//
//  mbr.c
//  hfsinspect
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <stdio.h>
#include "mbr.h"
#include "partition_support.h"
#include "hfs_pstruct.h"
#include "hfs_endian.h"

void mbr_print(HFSVolume* hfs, MBR* mbr)
{
    PrintHeaderString("Master Boot Record");
    PrintAttributeString("signature", "%#x", S16(*(u_int16_t*)mbr->signature));
    
    FOR_UNTIL(i, 4) {
        MBRPartition* partition = &mbr->partitions[i];
        
        PrintHeaderString("Partition %d", i + 1);
        PrintUIHex  (partition, status);
        PrintUI     (partition, first_sector_head);
        PrintUI     (partition, first_sector_cylinder);
        PrintUI     (partition, first_sector_sector);
        PrintUIHex  (partition, type);
        PrintUI     (partition, last_sector_head);
        PrintUI     (partition, last_sector_cylinder);
        PrintUI     (partition, last_sector_sector);
        PrintUI     (partition, first_sector_lba);
        PrintUI     (partition, sector_count);
        _PrintDataLength("(size)", partition->sector_count*hfs->block_size);
    }
}
