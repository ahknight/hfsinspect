//
//  btree_endian.c
//  hfsinspect
//
//  Created by Adam Knight on 11/26/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs/btree/btree_endian.h"
#include "volumes/_endian.h"

#include "logging/logging.h"    // console printing routines


void swap_BTNodeDescriptor(BTNodeDescriptor* record)
{
    Swap32(record->fLink);
    Swap32(record->bLink);

    if ((signed)record->fLink < -1)
        warning("Invalid forward link: %d", (signed)record->fLink);

    if ((signed)record->bLink < -1)
        warning("Invalid backward link: %d", (signed)record->bLink);

    // noswap: record->kind is a short
    if ((record->kind < kBTLeafNode) || (record->kind > kBTMapNode))
        warning("invalid node type: %d", record->kind);

    // noswap: record->height is a short
    if (record->height > 16)
        warning("invalid node height: %d", record->kind);

    Swap16(record->numRecords);

    // noswap: record->reserved is reserved
}

void swap_BTHeaderRec(BTHeaderRec* record)
{
    Swap16(record->treeDepth);
    Swap32(record->rootNode);
    Swap32(record->leafRecords);
    Swap32(record->firstLeafNode);
    Swap32(record->lastLeafNode);
    Swap16(record->nodeSize);
    Swap16(record->maxKeyLength);
    Swap32(record->totalNodes);
    Swap32(record->freeNodes);
    // noswap: record->reserved1
    Swap32(record->clumpSize);
    // noswap: record->btreeType is a short
    // noswap: record->keyCompareType is a short
    Swap32(record->attributes);
    // noswap: header->reserved3
}

int swap_BTreeNode(BTreeNodePtr node)
{
    char*          nodeData = node->data;
    const BTreePtr bTree    = node->bTree;

    // *** Verify Node ***

    // Check record offset 0 to see if we've done this before (last two bytes of the node). It's a constant 14.
    BTRecOffset sentinel = *(BTRecOffsetPtr)(node->data + node->nodeSize - sizeof(BTRecOffset));
    if ( sentinel == 14 ) { warning("Node is already swapped."); return 0; }

    // Verify that this is a node in the first place (swap error protection).
    Swap16(sentinel);
    if ( sentinel != 14 ) {
        warning("node %u: node is not a node (sentinel: %u != 14).", node->nodeNumber, sentinel);
        errno = EINVAL;

        bool is_blank = true;
        for (unsigned i = 0; i < node->nodeSize; i++) {
            if ( ((char*)node->data + i) != 0 ) {
                debug("node %u: node has data at index %u", node->nodeNumber, i);
                is_blank = false;
                break;
            }
        }

        if (is_blank) {
            warning("node %u: node->data is blank", node->nodeNumber);
        }

        return -1;
    }

    // *** Node Descriptor ***

    BTNodeDescriptor* nodeDescriptor = (BTNodeDescriptor*)nodeData;
    swap_BTNodeDescriptor(nodeDescriptor);

    // Verify this is a valid node (additional protection against swap errors)
    if ((nodeDescriptor->kind < kBTLeafNode) || (nodeDescriptor->kind > kBTMapNode)) {
        warning("node %u: invalid node kind: %d", node->nodeNumber, nodeDescriptor->kind);
        errno = EINVAL;
        return -1;
    }

    // *** Record Offsets ***

    uint16_t     numRecords  = nodeDescriptor->numRecords;

    BTRecOffsetPtr offsets    = ((BTRecOffsetPtr)(node->data + node->nodeSize)) - numRecords - 1;
    for(unsigned i = 0; i < (numRecords+1); i++) Swap16(offsets[i]);

    // Validate offsets
    off_t record_min = sizeof(BTNodeDescriptor);
    off_t record_max = (node->nodeSize - ((numRecords+1) * sizeof(BTRecOffset)));

    {
        int prev = record_max;
        for(unsigned i = 0; i < numRecords; i++) {
            BTRecOffset recOffset = offsets[i];
            if ((recOffset < record_min) || (recOffset > record_max)) {
                warning("node %u: record %u points outside this node: %u (%jd, %jd)", node->nodeNumber, i, recOffset, (intmax_t)record_min, (intmax_t)record_max);
            }
            if ((i != 0) && (recOffset > prev)) {
                warning("node %u: record %u is out of order (%u > %u)", node->nodeNumber, i, offsets[i-1], recOffset);
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


    // Handle keyed nodes
    for (unsigned recordNum = 0; recordNum < numRecords; recordNum++) {
        void*    record = BTGetRecord(node, recordNum);
        BTreeKeyPtr key    = (BTreeKey*)(record);
        if (swapKeys) {
            Swap16( key->length16 );
        }

        switch (nodeDescriptor->kind) {
            case kBTHeaderNode:
            {
                if (recordNum == 0) swap_BTHeaderRec((BTHeaderRec*)record);
                break;
            }

            case kBTIndexNode:
            {
                // Swap the node pointers.
                BTRecOffset keyLen       = BTGetRecordKeyLength(node, recordNum);
                uint32_t*   childPointer = (void*)((char*)record + keyLen);
                Swap32(*childPointer);
                break;
            }

            case kBTMapNode:
            {
                debug("Map node %u: fLink: %u", node->nodeNumber, nodeDescriptor->fLink);
                debug("Map node %u: numRecords: %u", node->nodeNumber, nodeDescriptor->numRecords);
                break;
            }

            case kBTLeafNode:
            {
//                debug("Leaf node: %u", node->nodeNumber);
                break;
            }

            default:
            {
                break;
            }
        }
    }
    return 1;
}

