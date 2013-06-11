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
    
    //    record->finderInfo is an array of bytes; swap as-used.
    
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
    for (int i = 0; i< 8; i++) {
        Convert32(record->device_signature[i]);
    }
    Convert64(record->offset);
    Convert64(record->size);
    // uuid_string_t is a series of char
    // machine_serial_num is a series of char
    // reserved is untouchable
}

void swap_HFSPlusForkData(HFSPlusForkData *record)
{
    Convert64(record->logicalSize);
    Convert32(record->totalBlocks);
    Convert32(record->clumpSize);
    swap_HFSPlusExtentRecord(record->extents);
}

void swap_HFSPlusExtentRecord(HFSPlusExtentDescriptor record[8])
{
    for (int i = 0; i < kHFSPlusExtentDensity; i++) {
        HFSPlusExtentDescriptor *descriptor = &record[i];
        swap_HFSPlusExtentDescriptor(descriptor);
    }
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

void swap_HFSUniStr255(HFSUniStr255 *unistr)
{
    Convert16(unistr->length);
    for (int i = 0; i < unistr->length; i++)
        Convert16(unistr->unicode[i]);
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
    for (int i = 0; i < numRecords; i++) {
        u_int16_t *offset = &offsets[i];
        *offset = S16(*offset);
    }
    
    // Record offsets
    // 0 1 2 3 4 5 | -- offset ID
    // 5 4 3 2 1 0 | -- record ID
    int lastRecordIndex = numRecords - 1;
    for (int offsetID = 0; offsetID < numRecords; offsetID++) {
        int recordID        = lastRecordIndex - offsetID;
        int nextOffsetID    = offsetID - 1;
        
        HFSBTreeNodeRecord *meta    = &node->records[lastRecordIndex - offsetID];
        meta->recordID             = recordID;
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
                meta->keyLength = *(u_int16_t*)meta->record;
                meta->keyLength = S16(meta->keyLength);
            } else {
                meta->keyLength = node->bTree.headerRecord.maxKeyLength;
            }
            
            meta->key = meta->record;
            
            // Each tree has a different key type.
            switch (node->bTree.fork.cnid) {
                case kHFSCatalogFileID:
                {
                    if (meta->keyLength < kHFSPlusCatalogKeyMinimumLength) {
                        meta->keyLength = kHFSPlusCatalogKeyMinimumLength;
                    } else if (meta->keyLength > kHFSPlusCatalogKeyMaximumLength) {
                        critical("Invalid key length for catalog record: %d", meta->keyLength);
                    }
                    swap_HFSPlusCatalogKey(meta->key);
                    break;
                }
                    
                case kHFSExtentsFileID:
                {
                    if (meta->keyLength != kHFSPlusExtentKeyMaximumLength) {
                        critical("Invalid key length for extent record: %d", meta->keyLength);
                    }
                    swap_HFSPlusExtentKey(meta->key);
                    break;
                }
                    
                default:
                    critical("Unhandled tree type: %d", node->bTree.fork.cnid);
                    break;
            }
            
            if (meta->keyLength % 2) meta->keyLength++; // Round up to an even length.
            
            meta->value         = meta->key + sizeof(u_int16_t) + meta->keyLength;
            meta->valueLength  = meta->length - meta->keyLength;
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
                        
                        if (recordKind == kHFSPlusFolderRecord) {
                            swap_HFSPlusCatalogFolder((HFSPlusCatalogFolder*)meta->value);
                            
                        } else if (recordKind == kHFSPlusFileRecord) {
                            swap_HFSPlusCatalogFile((HFSPlusCatalogFile*)meta->value);
                            
                        } else if (recordKind == kHFSPlusFolderThreadRecord) {
                            swap_HFSPlusCatalogThread((HFSPlusCatalogThread*)meta->value);
                            
                        } else if (recordKind == kHFSPlusFileThreadRecord) {
                            swap_HFSPlusCatalogThread((HFSPlusCatalogThread*)meta->value);
                            
                        }
                        break;
                    }
                        
                    case kHFSExtentsFileID:
                    {
                        swap_HFSPlusExtentRecord(*(HFSPlusExtentRecord*)meta->value);
                        break;
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
                    swap_BTHeaderRec(meta->record);
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
//        PrintNodeRecord(node, recordNum);
    }
    
    return 1;
}
