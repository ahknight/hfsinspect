//
//  main.c
//  hfstest
//
//  Created by Adam Knight on 5/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <hfs/hfs_format.h>
#include "hfs.h"
#include "hfs_pstruct.h"

int main(int argc, const char * argv[])
{
	const char * path = argv[1];
	
    HFSVolume hfs;
    
    if (-1 == hfs_open(&hfs, path)) {
        return errno;
    }
    
    hfs_load(&hfs);
    
    PrintVolumeHeader(&hfs.vh);
    	
    PrintCatalogHeader(&hfs);
    
	hfs_close(&hfs);

    return 0;
}

