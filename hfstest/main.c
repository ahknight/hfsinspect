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

int main(int argc, const char * argv[])
{
	const char * path = argv[1]; //"/dev/disk2s1";
	
    HFSVolume hfs;
    
    if (-1 == hfs_open(&hfs, path)) {
        return errno;
    }
    
    hfs_load(&hfs);
    
    PrintVolumeHeader(&hfs.vh);
    	
    // PrintCatalogHeader(&vh);
    
//	HFSPlusExtentRecord *extentRecord = &hfs->vh.catalogFile.extents;
//    
//	off_t offset = BE32(extentRecord[0]->startBlock) * BE32(vh.blockSize);
//	
//	BTNodeDescriptor node;
//	size = pread(fd, &node, sizeof(node), offset);
//	
//    PrintBTNodeDescriptor(&node);
//	
//	BTHeaderRec header;
//    offset += sizeof(node); // chain
//	size = pread(fd, &header, sizeof(header), offset);
//	
//    PrintBTHeaderRecord(&header);    
//
    
	hfs_close(&hfs);

    return 0;
}

