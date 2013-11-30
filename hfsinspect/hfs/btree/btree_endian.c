//
//  btree_endian.c
//  hfsinspect
//
//  Created by Adam Knight on 11/26/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "btree_endian.h"
#include "_endian.h"
#include "output.h"
#include "output_hfs.h"

void swap_BTNodeDescriptor(BTNodeDescriptor *record)
{
    Convert32(record->fLink);
    Convert32(record->bLink);
    
    // noswap: record->kind is a short
    if (record->kind < kBTLeafNode || record->kind > kBTMapNode)
        warning("invalid node type: %d", record->kind);
    
    // noswap: record->height is a short
    if (record->height > 16)
        warning("invalid node height: %d", record->kind);
    
    Convert16(record->numRecords);
    
    // noswap: record->reserved is reserved
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
    // noswap: record->reserved1
    Convert32(record->clumpSize);
    // noswap: record->btreeType is a short
    // noswap: record->keyCompareType is a short
    Convert32(record->attributes);
    // noswap: header->reserved3
}

int swap_BTreeNode(BTreeNodePtr node)
{
    Bytes nodeData = node->data;
    const BTreePtr bTree = node->bTree;
    
    // *** Verify Node ***
    
    // Check record offset 0 to see if we've done this before (last two bytes of the node). It's a constant 14.
    BTRecOffset sentinel = *(BTRecOffsetPtr)(node->data + node->nodeSize - sizeof(BTRecOffset));
    if ( sentinel == 14 ) { warning("Node is already swapped."); return 0; }
    
    // Verify that this is a node in the first place (swap error protection).
    sentinel = be16toh(sentinel);
    if ( sentinel != 14 ) {
        warning("Node is not a node (sentinel: %u != 14).", sentinel);
        errno = EINVAL;
        return -1;
    }
    
    // *** Node Descriptor ***
    
    BTNodeDescriptor *nodeDescriptor = (BTNodeDescriptor*)nodeData;
    swap_BTNodeDescriptor(nodeDescriptor);
    
    // Verify this is a valid node (additional protection against swap errors)
    if (nodeDescriptor->kind < kBTLeafNode || nodeDescriptor->kind > kBTMapNode) {
        warning("Invalid node kind: %d", nodeDescriptor->kind);
        errno = EINVAL;
        return -1;
    }
    
    // *** Record Offsets ***
    
    uint16_t numRecords = nodeDescriptor->numRecords;
    BTRecOffsetPtr offsets = ((BTRecOffsetPtr)(node->data + node->nodeSize)) - numRecords - 1;
    FOR_UNTIL(i, numRecords+1) Convert16(offsets[i]);
    
    // Validate offsets
    off_t record_min = sizeof(BTNodeDescriptor);
    off_t record_max = (node->nodeSize - ((numRecords+1) * sizeof(BTRecOffset)));
    
    {
        int prev = record_max;
        FOR_UNTIL(i, numRecords) {
            BTRecOffset recOffset = offsets[i];
            if (recOffset < record_min || recOffset > record_max) {
                warning("record %u points outside this node: %u (%u, %u)", i, recOffset, record_min, record_max);
            }
            if (i != 0 && recOffset > prev) {
                warning("record %u is out of order (%u > %u)", i, offsets[i-1], recOffset);
            }
            prev = recOffset;
        }
    }
    
    // *** Records *** (header records and key length of keyed records)
    
    // For keyed records, swap the key length if needed.
    bool swapKeys = (
                     // Has keys?
                     (nodeDescriptor->kind == kBTIndexNode || nodeDescriptor->kind == kBTLeafNode) &&
                     // Has 16-bit keys?
                     (bTree->headerRecord.attributes & kBTBigKeysMask)
                     );

    
    for (int recordNum = 0; recordNum < numRecords; recordNum++) {
        Bytes record = BTGetRecord(node, recordNum);
        BTreeKeyPtr key = (BTreeKey*)(record);
        if (swapKeys) {
            Convert16( key->length16 );
        }
        
        switch (nodeDescriptor->kind) {
            case kBTHeaderNode:
                if (recordNum == 0) swap_BTHeaderRec((BTHeaderRec*)record);
                break;
                
            case kBTIndexNode:
            {
                // Swap the node pointers.
                BTRecOffset keyLen = BTGetRecordKeyLength(node, recordNum);
                uint32_t *childPointer = (uint32_t*)(record + keyLen);
                Convert32(*childPointer);
            }
            
            default:
                break;
        }
    }
    return 1;
}
