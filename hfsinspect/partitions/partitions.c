//
//  partitions.c
//  hfsinspect
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "partitions.h"
#include "output.h"

PartitionOps* partops[] = {
    &gpt_ops,
    &apm_ops,
    &mbr_ops,
    &cs_ops,
    &((PartitionOps){NULL, NULL, NULL})
};

int partition_load(Volume *vol)
{
    info("Loading partitions.");
    
    PartitionOps** ops = (PartitionOps**)&partops;
    while ((*ops)->test != NULL) {
        if ((*ops)->test(vol) == 1) {
            if ((*ops)->load != NULL) {
                if (DEBUG && (*ops)->dump != NULL) {
                    (*ops)->dump(vol);
                }
                if ( (*ops)->load(vol) < 0) {
                    (ops)++;
                    continue; // try another
                } else {
                    break;
                }
            }
        }
        (ops)++;
    }
    
    // Recursive load
    FOR_UNTIL(i, vol->partition_count) {
        info("Looking for nested partitions on partition %u", i);
        if (vol->partitions[i]->type == kVolTypePartitionMap) {
            debug("Trying to load nested partitions on partition %u", i);
            if (partition_load(vol->partitions[i]) < 0) {
                error("error loading partition %u", i);
            }
        }
    }
    
    return 0;
}

int partition_dump(Volume *vol)
{
    partition_load(vol);
    
    BeginSection("Parsed Volumes");
    vol_dump(vol);
    EndSection();
    
    return 0;
}
