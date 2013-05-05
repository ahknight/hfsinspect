//
//  hfs_pstruct.c
//  hfstest
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "hfs_pstruct.h"
#include "hfs_endian.h"

/*
 * to_bsd_time - convert from Mac OS time (seconds since 1/1/1904)
 *		 to BSD time (seconds since 1/1/1970)
 */
time_t to_bsd_time(u_int32_t hfs_time)
{
	u_int32_t gmt = hfs_time;
	
	if (gmt > MAC_GMT_FACTOR)
		gmt -= MAC_GMT_FACTOR;
	else
		gmt = 0;	/* don't let date go negative! */
	
	return (time_t)gmt;
}

size_t string_from_hfs_time(char* buf, int buf_size, u_int32_t hfs_time) {
	time_t gmt = 0;
	gmt = to_bsd_time(hfs_time);
	struct tm *t = gmtime(&gmt);
	size_t len = strftime(buf, buf_size, "%c %Z", t);
	return len;
}

void _PrintHFSUI8(char* label, u_int8_t i)
{
    printf("%-40s%u", label, i);
}

void _PrintHFSUI16(char* label, u_int16_t i)
{
    printf("%-40s%u", label, BE16(i));
}

void _PrintHFSUI32(char* label, u_int32_t i)
{
    printf("%-40s%u", label, BE32(i));
}

void _PrintHFSUI64(char* label, u_int64_t i)
{
    printf("%-40s%llu", label, BE64(i));
}

void _PrintHFSTimestamp(char* label, u_int32_t timestamp)
{
    time_t time = BE32(timestamp);
    if (time > MAC_GMT_FACTOR) {
        time -= MAC_GMT_FACTOR;
    } else {
        time = 0; // Cannot be negative.
    }
    char buf[50];
    struct tm *t = gmtime(&time);
    strftime(buf, 50, "%c %Z", t);
    
    printf("%-40s%s", label, buf);
}

void _PrintHFSChar16(char* label, u_int16_t i)
{
    printf("%-40s%u", label, i);
}

void _PrintHFSChar32(char* label, u_int32_t i)
{
    printf("%-40s%u", label, i);
}


void PrintVolumeHeader(HFSPlusVolumeHeader *vh)
{
    u_int16_t sig = BE16(vh->signature);
    char sig_str[3] = { sig>>8, sig, '\0' };
    
    u_int32_t mnt = BE32(vh->lastMountedVersion);
    char mnt_str[5] = { mnt>>(3*8), mnt>>(2*8), mnt>>8, mnt, '\0' };
    
	printf("Signature:            %s\n", sig_str);
	printf("Version:              %u\n", BE16(vh->version));
	printf("Attributes:	          %u\n", BE32(vh->attributes));
	printf("Last Mounted Version: %s\n", mnt_str);
	printf("Journal Info Block:   %u\n", BE32(vh->journalInfoBlock));
	
	int buf_size = 50;
	char* buf = malloc(buf_size);
	
	string_from_hfs_time(buf, buf_size, BE32(vh->createDate));
	printf("Date Created:         %s\n", buf);
	
	string_from_hfs_time(buf, buf_size, BE32(vh->modifyDate));
	printf("Date Modified:        %s\n", buf);
	
	string_from_hfs_time(buf, buf_size, BE32(vh->backupDate));
	printf("Last Backup:          %s\n", buf);
	
	string_from_hfs_time(buf, buf_size, BE32(vh->checkedDate));
	printf("Last Check:           %s\n", buf);
	
	printf("File Count:           %u\n", BE32(vh->fileCount));
	printf("Folder Count:         %u\n", BE32(vh->folderCount));
    
	printf("Block Size:           %u\n", BE32(vh->blockSize));
	printf("Total Blocks:         %u\n", BE32(vh->totalBlocks));
	printf("Free Blocks:          %u\n", BE32(vh->freeBlocks));
	
	free(buf);
}

void PrintBTNodeDescriptor(BTNodeDescriptor *node)
{
    printf("fLink:              %u\n", BE32(node->fLink));
    printf("bLink:              %u\n", BE32(node->bLink));
    printf("Kind:               %u\n", node->kind);
    printf("Height:             %u\n", node->height);
    printf("Number of records:  %u\n", BE16(node->numRecords));
    printf("Reserved:           %u\n", BE16(node->reserved));
}

void PrintBTHeaderRecord(BTHeaderRec *hr)
{
	printf("Tree depth:       %u\n", BE16(hr->treeDepth));		/* maximum height (usually leaf nodes) */
	printf("Root node:        %u\n", BE32(hr->rootNode));		/* node number of root node */
	printf("Leaf records:     %u\n", BE32(hr->leafRecords));		/* number of leaf records in all leaf nodes */
	printf("First leaf node:  %u\n", BE32(hr->firstLeafNode));		/* node number of first leaf node */
	printf("Last leaf node:   %u\n", BE32(hr->lastLeafNode));		/* node number of last leaf node */
	printf("Node size:        %u\n", BE16(hr->nodeSize));		/* size of a node, in bytes */
	printf("Max Key Length:   %u\n", BE16(hr->maxKeyLength));		/* reserved */
	printf("Total Nodes:      %u\n", BE32(hr->totalNodes));		/* total number of nodes in tree */
	printf("Free Nodes:       %u\n", BE32(hr->freeNodes));		/* number of unused (free) nodes in tree */
	printf("Reserved1:        %u\n", BE16(hr->reserved1));		/* unused */
	printf("Clump Size:       %u\n", BE32(hr->clumpSize));		/* reserved */
	printf("BTree Type:       %u\n", hr->btreeType);		/* reserved */
	printf("Key Compare Type: %u\n", hr->keyCompareType);		/* Key string Comparison Type */
	printf("Attributes:       %u\n", BE32(hr->attributes));		/* persistent attributes about the tree */
//	printf("Reserved3:        %u\n", BE32(hr->reserved3[16]));		/* reserved */
}
