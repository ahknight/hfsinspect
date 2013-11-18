//
//  partitions.c
//  hfsinspect
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "partitions.h"
#include "output.h"

int partition_load(Volume *vol)
{
    if (gpt_test(vol)) {
        if (gpt_load(vol) < 0) {
            perror("gpt_load");
            return -1;
        }
    
    } else if (mbr_test(vol)) {
        if (mbr_load(vol) < 0) {
            perror("mbr_load");
            return -1;
        }
    
    } else {
        return -1;
    }
    
    return 0;
}

void partition_dump(Volume *vol)
{
    if (gpt_test(vol)) {
        gpt_load(vol);
        gpt_dump(vol);
        
    } else if (mbr_test(vol)) {
        mbr_load(vol);
        mbr_dump(vol);
    } else {
        warning("Unknown disk or partition type.");
        return;
    }
    
    BeginSection("Parsed Volume");
    vol_dump(vol);
    EndSection();
}
