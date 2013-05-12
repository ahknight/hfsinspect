//
//  hfs_endian.c
//  hfstest
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <stdlib.h>
#include <hfs/hfs_format.h>
#include "hfs_endian.h"

void swap_HFSPlusVolumeHeader(HFSPlusVolumeHeader *vh)
{
    vh->signature           = S16(vh->signature);
    vh->version             = S16(vh->version);
    vh->attributes          = S32(vh->attributes);
    vh->lastMountedVersion  = S32(vh->lastMountedVersion);
    vh->journalInfoBlock    = S32(vh->journalInfoBlock);
    
    vh->createDate          = S32(vh->createDate);
    vh->modifyDate          = S32(vh->modifyDate);
    vh->backupDate          = S32(vh->backupDate);
    vh->checkedDate         = S32(vh->checkedDate);
    
    vh->fileCount           = S32(vh->fileCount);
    vh->folderCount         = S32(vh->folderCount);
    
    vh->blockSize           = S32(vh->blockSize);
    vh->totalBlocks         = S32(vh->totalBlocks);
    vh->freeBlocks          = S32(vh->freeBlocks);
    
    vh->nextAllocation      = S32(vh->nextAllocation);
    vh->rsrcClumpSize       = S32(vh->rsrcClumpSize);
    vh->dataClumpSize       = S32(vh->dataClumpSize);
    vh->nextCatalogID       = S32(vh->nextCatalogID);
    
    vh->writeCount          = S32(vh->writeCount);
    vh->encodingsBitmap     = S64(vh->encodingsBitmap);
    
//    vh->finderInfo is an array of bytes; swap as-used.

    swap_HFSPlusForkData(&vh->allocationFile);
    swap_HFSPlusForkData(&vh->extentsFile);
    swap_HFSPlusForkData(&vh->catalogFile);
    swap_HFSPlusForkData(&vh->attributesFile);
    swap_HFSPlusForkData(&vh->startupFile);
}

void swap_HFSPlusForkData(HFSPlusForkData *fork)
{
    fork->logicalSize       = S64(fork->logicalSize);
    fork->totalBlocks       = S32(fork->totalBlocks);
    fork->clumpSize         = S32(fork->clumpSize);

    for (int i = 0; i < kHFSPlusExtentDensity; i++) {
        HFSPlusExtentDescriptor *extent_descriptor = &fork->extents[i];
        swap_HFSPlusExtentDescriptor(extent_descriptor);
    }
}

void swap_HFSPlusExtentDescriptor(HFSPlusExtentDescriptor *extent)
{
    extent->startBlock      = S32(extent->startBlock);
    extent->blockCount      = S32(extent->blockCount);
}

void swap_BTNodeDescriptor(BTNodeDescriptor *node)
{
    node->fLink             = S32(node->fLink);
    node->bLink             = S32(node->bLink);
//    node->kind is a short
//    node->height is a short
    node->numRecords        = S16(node->numRecords);
//    node->reserved is reserved
}

void swap_BTHeaderRec(BTHeaderRec *header)
{
    header->treeDepth       = S16(header->treeDepth);
    header->rootNode        = S32(header->rootNode);
    header->leafRecords     = S32(header->leafRecords);
    header->firstLeafNode   = S32(header->firstLeafNode);
    header->lastLeafNode    = S32(header->lastLeafNode);
    header->nodeSize        = S16(header->nodeSize);
    header->maxKeyLength    = S16(header->maxKeyLength);
    header->totalNodes      = S32(header->totalNodes);
    header->freeNodes       = S32(header->freeNodes);
//    header->reserved1
    header->clumpSize       = S32(header->clumpSize);
//    header->btreeType is a short
//    header->keyCompareType is a short
    header->attributes      = S32(header->attributes);
//    header->reserved3
}
