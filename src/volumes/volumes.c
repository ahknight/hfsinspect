//
//  volumes.c
//  volumes
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "volumes.h"
#include "output.h"
#include "logging/logging.h"    // console printing routines


static PartitionOps* partitionTypes[] = {
    &gpt_ops,
    &apm_ops,
    &cs_ops,
    &mbr_ops,
    &((PartitionOps){"", NULL, NULL, NULL})
};

int volumes_load(Volume* vol)
{
    info("Loading partitions.");

    PartitionOps** ops = (PartitionOps**)&partitionTypes;
    while ((*ops)->test != NULL) {
        info("Testing for a %s partition.", (*ops)->name);
        if ((*ops)->test(vol) == 1) {
            info("Detected a %s partition.", (*ops)->name);
//            if ((log_level >= L_DEBUG) && ((*ops)->dump != NULL)) {
//                (*ops)->dump(vol);
//            }
            if ((*ops)->load != NULL) {
                if ( (*ops)->load(vol) < 0) {
                    warning("Can't load %s partition: load returned failure.", (*ops)->name);
                } else {
                    debug("Loaded a %s partition.", (*ops)->name);
                    vol->ops = (*ops);
                    break;
                }
            } else {
                warning("Can't load %s partition: no load function.", (*ops)->name);
            }
        }
        (ops)++;
    }

    // Recursive load
    for(unsigned i = 0; i < vol->partition_count; i++) {
        info("Looking for nested partitions on partition %u", i);
        if ((vol->partitions[i] != NULL) && (vol->partitions[i]->type == kVolTypePartitionMap)) {
            debug("Trying to load nested partitions on partition %u", i);
            if (volumes_load(vol->partitions[i]) < 0) {
                error("error loading partition %u", i);
            }
        }
    }

    return 0;
}

int volumes_dump(Volume* vol)
{
    volumes_load(vol);

    BeginSection(vol->ctx, "Parsed Volumes");
    vol_dump(vol);
    EndSection(vol->ctx);

    return 0;
}

