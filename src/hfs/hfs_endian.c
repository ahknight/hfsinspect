//
//  hfs_endian.c
//  hfsinspect
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs/hfs_endian.h"

#include "hfsinspect/output.h"
#include "hfsinspect/cdefs.h"
#include "hfs/types.h"
#include "hfs/btree/btree_endian.h"
#include "hfs/catalog.h"
#include "logging/logging.h"    // console printing routines

void swap_HFSExtentDescriptor(HFSExtentDescriptor* record)
{
    Swap(record->startBlock);
    Swap(record->blockCount);
}

void swap_HFSExtentRecord(HFSExtentRecord* record)
{
    FOR_UNTIL(i, kHFSExtentDensity) swap_HFSExtentDescriptor(record[i]);
}

void swap_HFSMasterDirectoryBlock(HFSMasterDirectoryBlock* record)
{
    Swap(record->drSigWord); /* == kHFSSigWord */
	Swap(record->drCrDate); /* date and time of volume creation */
	Swap(record->drLsMod); /* date and time of last modification */
	Swap(record->drAtrb); /* volume attributes */
	Swap(record->drNmFls); /* number of files in root folder */
	Swap(record->drVBMSt); /* first block of volume bitmap */
	Swap(record->drAllocPtr); /* start of next allocation search */
	Swap(record->drNmAlBlks); /* number of allocation blocks in volume */
	Swap(record->drAlBlkSiz); /* size (in bytes) of allocation blocks */
	Swap(record->drClpSiz); /* default clump size */
	Swap(record->drAlBlSt); /* first allocation block in volume */
	Swap(record->drNxtCNID); /* next unused catalog node ID */
	Swap(record->drFreeBks); /* number of unused allocation blocks */
    // noswap: record->drVN
	Swap(record->drVolBkUp); /* date and time of last backup */
	Swap(record->drVSeqNum); /* volume backup sequence number */
	Swap(record->drWrCnt); /* volume write count */
	Swap(record->drXTClpSiz); /* clump size for extents overflow file */
	Swap(record->drCTClpSiz); /* clump size for catalog file */
	Swap(record->drNmRtDirs); /* number of directories in root folder */
	Swap(record->drFilCnt); /* number of files in volume */
	Swap(record->drDirCnt);	/* number of directories in volume */
    // noswap: record->drFndrInfo[8]; /* info used by the Finder */
	Swap(record->drEmbedSigWord); /* embedded vol sig (née drVCSize) */
	swap_HFSExtentDescriptor(&record->drEmbedExtent);	/* embedded volume location and size (formerly drVBMCSize and drCtlCSize) */
	Swap(record->drXTFlSize);	/* size of extents overflow file */
	swap_HFSExtentRecord(&record->drXTExtRec);	/* extent record for extents overflow file */
	Swap(record->drCTFlSize);	/* size of catalog file */
	swap_HFSExtentRecord(&record->drCTExtRec);	/* extent record for catalog file */
}

void swap_HFSPlusVolumeHeader(HFSPlusVolumeHeader *record)
{
    Swap16(record->signature);
    Swap16(record->version);
    Swap32(record->attributes);
    Swap32(record->lastMountedVersion);
    Swap32(record->journalInfoBlock);
    
    Swap32(record->createDate);
    Swap32(record->modifyDate);
    Swap32(record->backupDate);
    Swap32(record->checkedDate);
    
    Swap32(record->fileCount);
    Swap32(record->folderCount);
    
    Swap32(record->blockSize);
    Swap32(record->totalBlocks);
    Swap32(record->freeBlocks);
    
    Swap32(record->nextAllocation);
    Swap32(record->rsrcClumpSize);
    Swap32(record->dataClumpSize);
    Swap32(record->nextCatalogID);
    
    Swap32(record->writeCount);
    Swap64(record->encodingsBitmap);
    
    HFSPlusVolumeFinderInfo* finderInfo = (HFSPlusVolumeFinderInfo*)&record->finderInfo;
    Swap32(finderInfo->bootDirID);
    Swap32(finderInfo->bootParentID);
    Swap32(finderInfo->openWindowDirID);
    Swap32(finderInfo->os9DirID);
    // noswap: reserved is reserved (uint32)
    Swap32(finderInfo->osXDirID);
    Swap64(finderInfo->volID);
    
    swap_HFSPlusForkData(&record->allocationFile);
    swap_HFSPlusForkData(&record->extentsFile);
    swap_HFSPlusForkData(&record->catalogFile);
    swap_HFSPlusForkData(&record->attributesFile);
    swap_HFSPlusForkData(&record->startupFile);
}

void swap_JournalInfoBlock(JournalInfoBlock* record)
{
    /*
     struct JournalInfoBlock {
     uint32_t	flags;
     uint32_t       device_signature[8];  // signature used to locate our device.
     uint64_t       offset;               // byte offset to the journal on the device
     uint64_t       size;                 // size in bytes of the journal
     uuid_string_t   ext_jnl_uuid;
     char            machine_serial_num[48];
     char    	reserved[JIB_RESERVED_SIZE];
     } __attribute__((aligned(2), packed));
     typedef struct JournalInfoBlock JournalInfoBlock;
     */
    Swap32(record->flags);
    FOR_UNTIL(i, 8) Swap32(record->device_signature[i]);
    Swap64(record->offset);
    Swap64(record->size);
    // noswap: uuid_string_t is a series of char
    // noswap: machine_serial_num is a series of char
    // noswap: reserved is reserved
}

void swap_HFSPlusForkData(HFSPlusForkData *record)
{
    Swap64(record->logicalSize);
    Swap32(record->totalBlocks);
    Swap32(record->clumpSize);
    swap_HFSPlusExtentRecord(record->extents);
}

void swap_HFSPlusExtentRecord(HFSPlusExtentDescriptor record[])
{
    FOR_UNTIL(i, kHFSPlusExtentDensity) swap_HFSPlusExtentDescriptor(&record[i]);
}

void swap_HFSPlusExtentDescriptor(HFSPlusExtentDescriptor *record)
{
    Swap32(record->startBlock);
    Swap32(record->blockCount);
}

void swap_HFSPlusExtentKey(HFSPlusExtentKey *record)
{
//    noswap: keyLength; swapped in swap_BTNode
    Swap32(record->fileID);
    Swap32(record->startBlock);
}

void swap_HFSUniStr255(HFSUniStr255 *unistr)
{
    Swap16(unistr->length);
    if (unistr->length > 256) {
        warning("Invalid HFSUniStr255 length: %d", unistr->length);
    }
    FOR_UNTIL(i, MIN(256, unistr->length)) Swap16(unistr->unicode[i]);
}

void swap_HFSPlusAttrKey(HFSPlusAttrKey *record)
{
//    noswap: keyLength; swapped in swap_BTNode
    Swap16(record->pad);
    Swap32(record->fileID);
    Swap32(record->startBlock);
    Swap16(record->attrNameLen);
    FOR_UNTIL(i, record->attrNameLen) Swap16(record->attrName[i]);
}

void swap_HFSPlusAttrData(HFSPlusAttrData* record)
{
    // noswap: Swap32(record->recordType); (previously swapped)
    // noswap: Swap32(record->reserved[0]);
    // noswap: Swap32(record->reserved[1]);
    Swap32(record->attrSize);
}

void swap_HFSPlusAttrForkData(HFSPlusAttrForkData* record)
{
    // noswap: Swap32(record->recordType); (previously swapped)
    // noswap: Swap32(record->reserved);
    swap_HFSPlusForkData(&record->theFork);
}

void swap_HFSPlusAttrExtents(HFSPlusAttrExtents* record)
{
    // noswap: Swap32(record->recordType); (previously swapped)
    // noswap: Swap32(record->reserved);
    swap_HFSPlusExtentRecord(record->extents);
}

void swap_HFSPlusAttrRecord(HFSPlusAttrRecord* record)
{
    record->recordType = be32toh(record->recordType);
    switch (record->recordType) {
        case kHFSPlusAttrInlineData:
            // InlineData is no more; use AttrData.
            swap_HFSPlusAttrData(&record->attrData);
            break;
            
        case kHFSPlusAttrForkData:
            swap_HFSPlusAttrForkData(&record->forkData);
            break;
            
        case kHFSPlusAttrExtents:
            swap_HFSPlusAttrExtents(&record->overflowExtents);
            break;

        default:
            critical("Unknown attribute record type: %d", record->recordType);
            break;
    }
}
