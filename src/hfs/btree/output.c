#include "output.h"
#include "volumes/output.h"

void PrintBTNodeRecord(out_ctx* ctx, BTNodeRecordPtr record)
{
    BeginSection(ctx, "Node %u, Record %u (Tree %u)", record->node->nodeNumber, record->recNum, record->node->bTree->treeID);
    PrintUI(ctx, record, offset);
    PrintUI(ctx, record, recordLen);
    PrintUI(ctx, record, keyLen);
    PrintRawAttribute(ctx, record, key, 16);
    PrintUI(ctx, record, valueLen);
    EndSection(ctx);
}

