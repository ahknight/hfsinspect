//
//  hfs_endian.c
//  hfsinspect
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>

#include "hfs_structs.h"
#include "hfs_endian.h"
#include "hfs_pstruct.h"

#define Convert16(x)    x = OSSwapBigToHostInt16(x)
#define Convert32(x)    x = OSSwapBigToHostInt32(x)
#define Convert64(x)    x = OSSwapBigToHostInt64(x)

void swap_APMHeader(APMHeader* record)
{
    Convert16(record->signature);
    Convert16(record->reserved1);
	Convert32(record->partition_count);
	Convert32(record->partition_start);
	Convert32(record->partition_length);
//    char            name[32];
//    char            type[32];
	Convert32(record->data_start);
	Convert32(record->data_length);
	Convert32(record->status);
	Convert32(record->boot_code_start);
	Convert32(record->boot_code_length);
	Convert32(record->bootloader_address);
	Convert32(record->reserved2);
	Convert32(record->boot_code_entry);
	Convert32(record->reserved3);
	Convert32(record->boot_code_checksum);
//    char            processor_type[16];
}

void swap_HFSExtentDescriptor(HFSExtentDescriptor* record)
{
    Convert16(record->startBlock);
    Convert16(record->blockCount);
}

void swap_HFSExtentRecord(HFSExtentRecord* record)
{
    FOR_UNTIL(i, kHFSExtentDensity) swap_HFSExtentDescriptor(record[i]);
}

void swap_HFSMasterDirectoryBlock(HFSMasterDirectoryBlock* record)
{
    Convert16(record->drSigWord);	/* == kHFSSigWord */
	Convert32(record->drCrDate);	/* date and time of volume creation */
	Convert32(record->drLsMod);	/* date and time of last modification */
	Convert16(record->drAtrb);		/* volume attributes */
	Convert16(record->drNmFls);	/* number of files in root folder */
	Convert16(record->drVBMSt);	/* first block of volume bitmap */
	Convert16(record->drAllocPtr);	/* start of next allocation search */
	Convert16(record->drNmAlBlks);	/* number of allocation blocks in volume */
	Convert32(record->drAlBlkSiz);	/* size (in bytes) of allocation blocks */
	Convert32(record->drClpSiz);	/* default clump size */
	Convert16(record->drAlBlSt);	/* first allocation block in volume */
	Convert32(record->drNxtCNID);	/* next unused catalog node ID */
	Convert16(record->drFreeBks);	/* number of unused allocation blocks */
	Convert32(record->drVolBkUp);	/* date and time of last backup */
	Convert16(record->drVSeqNum);	/* volume backup sequence number */
	Convert32(record->drWrCnt);	/* volume write count */
	Convert32(record->drXTClpSiz);	/* clump size for extents overflow file */
	Convert32(record->drCTClpSiz);	/* clump size for catalog file */
	Convert16(record->drNmRtDirs);	/* number of directories in root folder */
	Convert32(record->drFilCnt);	/* number of files in volume */
	Convert32(record->drDirCnt);	/* number of directories in volume */
//	Convert32(record->drFndrInfo[8]);	/* information used by the Finder */
	Convert16(record->drEmbedSigWord);	/* embedded volume signature (formerly drVCSize) */
	swap_HFSExtentDescriptor(&record->drEmbedExtent);	/* embedded volume location and size (formerly drVBMCSize and drCtlCSize) */
	Convert32(record->drXTFlSize);	/* size of extents overflow file */
	swap_HFSExtentRecord(&record->drXTExtRec);	/* extent record for extents overflow file */
	Convert32(record->drCTFlSize);	/* size of catalog file */
	swap_HFSExtentRecord(&record->drCTExtRec);	/* extent record for catalog file */
}

void swap_HFSPlusVolumeHeader(HFSPlusVolumeHeader *record)
{
    Convert16(record->signature);
    Convert16(record->version);
    Convert32(record->attributes);
    Convert32(record->lastMountedVersion);
    Convert32(record->journalInfoBlock);
    
    Convert32(record->createDate);
    Convert32(record->modifyDate);
    Convert32(record->backupDate);
    Convert32(record->checkedDate);
    
    Convert32(record->fileCount);
    Convert32(record->folderCount);
    
    Convert32(record->blockSize);
    Convert32(record->totalBlocks);
    Convert32(record->freeBlocks);
    
    Convert32(record->nextAllocation);
    Convert32(record->rsrcClumpSize);
    Convert32(record->dataClumpSize);
    Convert32(record->nextCatalogID);
    
    Convert32(record->writeCount);
    Convert64(record->encodingsBitmap);
    
    HFSPlusVolumeFinderInfo* finderInfo = (HFSPlusVolumeFinderInfo*)&record->finderInfo;
    Convert32(finderInfo->bootDirID);
    Convert32(finderInfo->bootParentID);
    Convert32(finderInfo->openWindowDirID);
    Convert32(finderInfo->os9DirID);
    // reserved
    Convert32(finderInfo->osXDirID);
    Convert64(finderInfo->volID);
    
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
     u_int32_t	flags;
     u_int32_t       device_signature[8];  // signature used to locate our device.
     u_int64_t       offset;               // byte offset to the journal on the device
     u_int64_t       size;                 // size in bytes of the journal
     uuid_string_t   ext_jnl_uuid;
     char            machine_serial_num[48];
     char    	reserved[JIB_RESERVED_SIZE];
     } __attribute__((aligned(2), packed));
     typedef struct JournalInfoBlock JournalInfoBlock;
     */
    Convert32(record->flags);
    FOR_UNTIL(i, 8) Convert32(record->device_signature[i]);
    Convert64(record->offset);
    Convert64(record->size);
    // uuid_string_t is a series of char
    // machine_serial_num is a series of char
    // reserved is reserved
}

void swap_HFSPlusForkData(HFSPlusForkData *record)
{
    Convert64(record->logicalSize);
    Convert32(record->totalBlocks);
    Convert32(record->clumpSize);
    swap_HFSPlusExtentRecord(record->extents);
}

void swap_HFSPlusExtentRecord(HFSPlusExtentDescriptor record[])
{
    FOR_UNTIL(i, kHFSPlusExtentDensity) swap_HFSPlusExtentDescriptor(&record[i]);
}

void swap_HFSPlusExtentDescriptor(HFSPlusExtentDescriptor *record)
{
    Convert32(record->startBlock);
    Convert32(record->blockCount);
}

void swap_BTNodeDescriptor(BTNodeDescriptor *record)
{
    Convert32(record->fLink);
    Convert32(record->bLink);
    //    record->kind is a short
    //    record->height is a short
    Convert16(record->numRecords);
    //    record->reserved is reserved
}

void swap_BTHeaderRec(BTHeaderRec *record)
{
    Convert16(record->treeDepth);
    Convert32(record->rootNode);
    Convert32(record->leafRecords);
    Convert32(record->firstLeafNode);
    Convert32(record->lastLeafNode);
    Convert16(record->nodeSize);
    Convert16(record->maxKeyLength);
    Convert32(record->totalNodes);
    Convert32(record->freeNodes);
    //    record->reserved1
    Convert32(record->clumpSize);
    //    record->btreeType is a short
    //    record->keyCompareType is a short
    Convert32(record->attributes);
    //    header->reserved3
}

void swap_HFSPlusCatalogKey(HFSPlusCatalogKey *record)
{
    Convert16(record->keyLength);
    Convert32(record->parentID);
    swap_HFSUniStr255(&record->nodeName);
}

void swap_HFSPlusExtentKey(HFSPlusExtentKey *record)
{
    Convert16(record->keyLength);
    Convert32(record->fileID);
    Convert32(record->startBlock);
}

void swap_HFSPlusAttrKey(HFSPlusAttrKey *record)
{
    Convert16(record->keyLength);
    Convert32(record->fileID);
    Convert32(record->startBlock);
    Convert16(record->attrNameLen);
    FOR_UNTIL(i, record->attrNameLen) Convert16(record->attrName[i]);
}

void swap_HFSUniStr255(HFSUniStr255 *unistr)
{
    Convert16(unistr->length);
    FOR_UNTIL(i, unistr->length) Convert16(unistr->unicode[i]);
}

void swap_HFSPlusCatalogFolder(HFSPlusCatalogFolder *record)
{
    Convert16(record->recordType);
    Convert16(record->flags);
    Convert32(record->valence);
    Convert32(record->folderID);
    Convert32(record->createDate);
    Convert32(record->contentModDate);
    Convert32(record->attributeModDate);
    Convert32(record->accessDate);
    Convert32(record->backupDate);
    swap_HFSPlusBSDInfo(&record->bsdInfo);
    swap_FndrDirInfo(&record->userInfo);
    swap_FndrOpaqueInfo(&record->finderInfo);
    Convert32(record->textEncoding);
    Convert32(record->folderCount);
}

void swap_HFSPlusBSDInfo(HFSPlusBSDInfo *record)
{
    Convert32(record->ownerID);
    Convert32(record->groupID);
    // adminFlags is a short
    // ownerFlags is a short
    Convert16(record->fileMode);
    Convert32(record->special.iNodeNum);
}

void swap_FndrDirInfo(FndrDirInfo *record)
{
    Convert16(record->frRect.top);
    Convert16(record->frRect.left);
    Convert16(record->frRect.bottom);
    Convert16(record->frRect.right);
    // frFlags is a short
    Convert16(record->frLocation.v);
    Convert16(record->frLocation.h);
    Convert16(record->opaque);
}

void swap_FndrFileInfo(FndrFileInfo *record)
{
    Convert32(record->fdType);
    Convert32(record->fdCreator);
    Convert16(record->fdFlags);
    Convert16(record->fdLocation.v);
    Convert16(record->fdLocation.h);
    Convert16(record->opaque);
}

void swap_FndrOpaqueInfo(FndrOpaqueInfo *record)
{
    // A bunch of undocumented shorts.  Included for completeness.
    ;;
}

void swap_HFSPlusCatalogFile(HFSPlusCatalogFile *record)
{
    Convert16(record->recordType);
    Convert16(record->flags);
    Convert32(record->reserved1);
    Convert32(record->fileID);
    Convert32(record->createDate);
    Convert32(record->contentModDate);
    Convert32(record->attributeModDate);
    Convert32(record->accessDate);
    Convert32(record->backupDate);
    swap_HFSPlusBSDInfo(&record->bsdInfo);
    swap_FndrFileInfo(&record->userInfo);
    swap_FndrOpaqueInfo(&record->finderInfo);
    Convert32(record->textEncoding);
    Convert32(record->reserved2);
    
    swap_HFSPlusForkData(&record->dataFork);
    swap_HFSPlusForkData(&record->resourceFork);
}

void swap_HFSPlusCatalogThread(HFSPlusCatalogThread *record)
{
    Convert16(record->recordType);
    Convert32(record->reserved);
    Convert32(record->parentID);
    swap_HFSUniStr255(&record->nodeName);
}

void swap_HFSPlusAttrInlineData(HFSPlusAttrInlineData* record)
{
    // Marked as unused in headers; not found in kernel source outside of that.
    Convert32(record->recordType);
//    Convert32(record->reserved);
    Convert32(record->logicalSize);
}

void swap_HFSPlusAttrData(HFSPlusAttrData* record)
{
    Convert32(record->recordType);
//    Convert32(record->reserved[0]);
//    Convert32(record->reserved[1]);
    Convert32(record->attrSize);
}

void swap_HFSPlusAttrForkData(HFSPlusAttrForkData* record)
{
    Convert32(record->recordType);
//    Convert32(record->reserved);
    swap_HFSPlusForkData(&record->theFork);
}

void swap_HFSPlusAttrExtents(HFSPlusAttrExtents* record)
{
    Convert32(record->recordType);
//    Convert32(record->reserved);
    swap_HFSPlusExtentRecord(record->extents);
}

void swap_HFSPlusAttrRecord(HFSPlusAttrRecord* record)
{
    Convert32(record->recordType);
    if (record->recordType == kHFSPlusAttrInlineData) {
        swap_HFSPlusAttrData((HFSPlusAttrData*)record);
        
    } else if (record->recordType == kHFSPlusAttrForkData) {
        swap_HFSPlusAttrForkData((HFSPlusAttrForkData*)record);
        
    } else if (record->recordType == kHFSPlusAttrExtents) {
        swap_HFSPlusAttrExtents((HFSPlusAttrExtents*)record);
    } else {
        critical("Unknown attribute record type: %d", record->recordType);
    }
}

int swap_BTreeNode(HFSBTreeNode *node)
{
    // node->buffer is a 4-8KB block read from disk in BE format.
    // Figure out what it is and swap everything that needs swapping.
    // Good luck.
    
    // Check record offset 0 to see if we've done this before. It's a constant 14.
    u_int16_t sentinel = *(u_int16_t*)(node->buffer.data + node->bTree.headerRecord.nodeSize - sizeof(u_int16_t));
    if ( sentinel == 14 ) return 1;
    
    // Verify that this is a node in the first place (swap error protection).
    sentinel = S16(sentinel);
    if ( sentinel != 14 ) {
        debug("Node %u (offset %u) is not a node (sentinel: %u != 14).", node->nodeNumber, node->nodeOffset, sentinel);
        errno = EINVAL;
        return -1 ;
    }
    
    /* First, swap things universal to all nodes. */
    
    // Swap node descriptor
    BTNodeDescriptor *nodeDescriptor = (BTNodeDescriptor*)(node->buffer.data);
    swap_BTNodeDescriptor(nodeDescriptor);
    node->nodeDescriptor = *nodeDescriptor;
    
    // Verify this is a valid node (additional protection against swap errors)
    if (node->nodeDescriptor.kind < -1 || node->nodeDescriptor.kind > 2) {
        debug("Invalid node kind: %d", node->nodeDescriptor.kind);
        errno = EINVAL;
        return -1 ;
    }
    
    // Swap record offsets
    u_int16_t numRecords = node->nodeDescriptor.numRecords + 1;  // "+1" gets the free space record.
    u_int16_t *offsets = (u_int16_t*)(node->buffer.data + node->bTree.headerRecord.nodeSize - (sizeof(u_int16_t) * numRecords));
    
    FOR_UNTIL(i, numRecords) offsets[i] = S16(offsets[i]);
    
    // Record offsets
    // 0 1 2 3 4 5 | -- offset ID
    // 5 4 3 2 1 0 | -- record ID
    int lastRecordIndex = numRecords - 1;
    for (int offsetID = 0; offsetID < numRecords; offsetID++) {
        int recordID        = lastRecordIndex - offsetID;
        int nextOffsetID    = offsetID - 1;
        
        HFSBTreeNodeRecord *meta    = &node->records[lastRecordIndex - offsetID];
        meta->treeCNID              = node->bTree.fork.cnid;
        meta->nodeID                = node->nodeNumber;
        meta->node                  = node;
        meta->recordID              = recordID;
        meta->offset                = offsets[offsetID];
        meta->length                = offsets[nextOffsetID] - offsets[offsetID];
        meta->record                = node->buffer.data + meta->offset;
    }
    
    // Note for branching
    u_int32_t treeID = node->bTree.fork.cnid;
    
    for (int recordNum = 0; recordNum < node->nodeDescriptor.numRecords; recordNum++) {
        HFSBTreeNodeRecord *meta = &node->records[recordNum];

        if (node->nodeDescriptor.kind == kBTIndexNode || node->nodeDescriptor.kind == kBTLeafNode) {
/*
 Immediately following the keyLength is the key itself. The length of the key is determined by the node type and the B-tree attributes. In leaf nodes, the length is always determined by keyLength. In index nodes, the length depends on the value of the kBTVariableIndexKeysMask bit in the B-tree attributes in the header record. If the bit is clear, the key occupies a constant number of bytes, determined by the maxKeyLength field of the B-tree header record. If the bit is set, the key length is determined by the keyLength field of the keyed record.
 
 Following the key is the record's data. The format of this data depends on the node type, as explained in the next two sections. However, the data is always aligned on a two-byte boundary and occupies an even number of bytes. To meet the first alignment requirement, a pad byte must be inserted between the key and the data if the size of the keyLength field plus the size of the key is odd. To meet the second alignment requirement, a pad byte must be added after the data if the data size is odd.

 http://developer.apple.com/legacy/library/technotes/tn/tn1150.html#KeyedRecords
 */
            if (node->bTree.fork.cnid == kHFSAttributesFileID) VisualizeData(meta->record, meta->length);
            
            if( !(node->bTree.headerRecord.attributes & kBTBigKeysMask) ) {
                critical("Only HFS Plus B-Trees are supported.");
            }
            
            bool variableKeyLength = false;
            if (node->nodeDescriptor.kind == kBTLeafNode) {
                variableKeyLength = true;
            } else if (node->nodeDescriptor.kind == kBTIndexNode && node->bTree.headerRecord.attributes & kBTVariableIndexKeysMask) {
                variableKeyLength = true;
            }
            
            if (variableKeyLength) {
//                debug("Variable key length: %u", meta->keyLength);
                meta->keyLength = *(u_int16_t*)meta->record;
                meta->keyLength = S16(meta->keyLength);
//                debug("Variable key length: %u", meta->keyLength);
                
            } else {
//                debug("Max key length: %u", meta->keyLength);
                meta->keyLength = node->bTree.headerRecord.maxKeyLength;
//                debug("Max key length: %u", meta->keyLength);
            }
            
            meta->key = meta->record;
            
            // Each tree has a different key type.
            switch (node->bTree.fork.cnid) {
                case kHFSCatalogFileID:
                {
//                    debug("Swapping catalog key");
                    if (meta->keyLength < kHFSPlusCatalogKeyMinimumLength) {
                        meta->keyLength = kHFSPlusCatalogKeyMinimumLength;
                    } else if (meta->keyLength > kHFSPlusCatalogKeyMaximumLength) {
                        critical("Invalid key length for catalog record: %d", meta->keyLength);
                    }
                    swap_HFSPlusCatalogKey((HFSPlusCatalogKey*)meta->key);
                    break;
                }
                    
                case kHFSExtentsFileID:
                {
//                    debug("Swapping extent key");
                    if (meta->keyLength != kHFSPlusExtentKeyMaximumLength) {
                        critical("Invalid key length for extent record: %d", meta->keyLength);
                    }
                    swap_HFSPlusExtentKey((HFSPlusExtentKey*)meta->key);
                    break;
                }
                
                case kHFSAttributesFileID:
                {
                    warning("Swapping attribute key for record %u (key length %u)", recordNum, meta->keyLength);
                    if (meta->keyLength < kHFSPlusAttrKeyMinimumLength) {
                        meta->keyLength = kHFSPlusAttrKeyMinimumLength;
                    } else if (meta->keyLength > kHFSPlusAttrKeyMaximumLength) {
                        critical("Invalid key length for attribute record: %d", meta->keyLength);
                    }
                    swap_HFSPlusAttrKey((HFSPlusAttrKey*)meta->key);
                    break;
                }
                    
                   
                default:
                    critical("Unhandled tree type: %d", node->bTree.fork.cnid);
                    break;
            }
            
            if (meta->keyLength % 2) meta->keyLength++; // Round up to an even length.
            
            meta->value         = meta->key + sizeof(u_int16_t) + meta->keyLength;
            meta->valueLength   = meta->length - meta->keyLength;
        }
        
        switch (node->nodeDescriptor.kind) {
            case kBTIndexNode:
            {
                u_int32_t *pointer = (u_int32_t*)meta->value;
                *pointer = S32(*pointer);
                break;
            }
                
            case kBTLeafNode:
            {
                switch (node->bTree.fork.cnid) {
                    case kHFSCatalogFileID:
                    {
                        u_int16_t recordKind = *(u_int16_t*)meta->value;
                        recordKind = S16(recordKind);
                        HFSPlusCatalogRecord* catalogRecord = (HFSPlusCatalogRecord*)meta->value;
                        
                        switch (recordKind) {
                            case kHFSPlusFolderRecord:
                                swap_HFSPlusCatalogFolder(&catalogRecord->catalogFolder); break;
                                
                            case kHFSPlusFileRecord:
                                swap_HFSPlusCatalogFile(&catalogRecord->catalogFile); break;
                            
                            case kHFSPlusFolderThreadRecord:
                            case kHFSPlusFileThreadRecord:
                                swap_HFSPlusCatalogThread(&catalogRecord->catalogThread); break;
                                
                            default:
                                break;
                        }
                        break;
                    }
                        
                    case kHFSExtentsFileID:
                    {
                        swap_HFSPlusExtentRecord((HFSPlusExtentDescriptor*)meta->value);
                        break;
                    }
                    
                    case kHFSAttributesFileID:
                    {
                        swap_HFSPlusAttrRecord((HFSPlusAttrRecord*)meta->value);
                    }
                        
                    default:
                    {
                        critical("Unknown tree CNID: %d", treeID);
                        break;
                    }
                }
                break;
            }
                
            case kBTHeaderNode:
            {
                // Only swap the header.  Don't care about user and map.
                if (recordNum == 0) {
                    swap_BTHeaderRec((BTHeaderRec*)meta->record);
                }
                break;
            }
                
            case kBTMapNode:
            {
                debug("Ignoring map node.");
                break;
            }
                
            default:
            {
                critical("Unknown node kind: %d", node->nodeDescriptor.kind);
                break;
            }
        }
    }
    
    return 1;
}
