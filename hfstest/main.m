//
//  main.m
//  hfstest
//
//  Created by Adam Knight on 5/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <hfs/hfs_format.h>
#include "hfs_endian.h"
#include "hfs_pstruct.h"

int main(int argc, const char * argv[])
{
	const char * path = "/dev/disk2s1";
	
	int fd;             // File descriptor
	ssize_t size;       // Read result
	u_int32_t offset;   // Read offset
	
	HFSPlusVolumeHeader vh;
		
	fd = open(path, O_RDONLY);
	
	if (fd == -1) {
		printf("Error: %d", errno);
		return errno;
	}
	
    offset = 1024; // reserved area
	size = pread(fd, &vh, sizeof(vh), offset);
	
    printf("\n\nHFS Plus Volume Header (offset: %u):\n", offset);
    PrintVolumeHeader(&vh);
	
	HFSPlusExtentRecord *extentRecord = &vh.catalogFile.extents;
	offset = BE32(extentRecord[0]->startBlock) * BE32(vh.blockSize);
	
	BTNodeDescriptor node;
	size = pread(fd, &node, sizeof(node), offset);
	
    printf("\n\nCatalog B-Tree Node Descriptor (offset: %u):\n", offset);
    PrintBTNodeDescriptor(&node);
	
	BTHeaderRec header;
    offset += sizeof(node); // chain
	size = pread(fd, &header, sizeof(header), offset);
	
	printf("\n\nCatalog B-Tree Header Record (offset %u):\n", offset);
    PrintBTHeaderRecord(&header);    
    
	close(fd);

    return 0;
}

