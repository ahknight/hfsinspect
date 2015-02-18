//
//  output_hfs.h
//  hfsinspect
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_output_hfs_h
#define hfsinspect_output_hfs_h

#include "hfs/types.h"
#include "hfs/hfs_extentlist.h"

#include "volumes/output.h"

#define MAC_GMT_FACTOR 2082844800UL

#define _NONNULL       __attribute__((nonnull))

void set_hfs_volume(HFS* v) _NONNULL;
HFS* get_hfs_volume(void);

#define PrintCatalogName(ctx, record, value)  _PrintCatalogName(ctx, #value, record->value)
#define PrintHFSBlocks(ctx, record, value)    _PrintHFSBlocks(ctx, #value, record->value)
#define PrintHFSChar(ctx, record, value)      _PrintHFSChar(ctx, #value, (char*)&(record->value), sizeof(record->value))
#define PrintHFSTimestamp(ctx, record, value) _PrintHFSTimestamp(ctx, #value, record->value)

void _PrintCatalogName              (out_ctx* ctx, char* label, bt_nodeid_t cnid) _NONNULL;
void _PrintHFSBlocks                (out_ctx* ctx, const char* label, uint64_t blocks) _NONNULL;
void _PrintHFSChar                  (out_ctx* ctx, const char* label, const char* i, size_t nbytes) _NONNULL;
void _PrintHFSTimestamp             (out_ctx* ctx, const char* label, uint32_t timestamp) _NONNULL;

void PrintVolumeInfo                (out_ctx* ctx, const HFS* hfs) _NONNULL;
void PrintHFSMasterDirectoryBlock   (out_ctx* ctx, const HFSMasterDirectoryBlock* vcb) _NONNULL;
void PrintVolumeHeader              (out_ctx* ctx, const HFSPlusVolumeHeader* vh) _NONNULL;
void PrintExtentList                (out_ctx* ctx, const ExtentList* list, uint32_t totalBlocks) _NONNULL;
void PrintForkExtentsSummary        (out_ctx* ctx, const HFSFork* fork) _NONNULL;
void PrintHFSPlusForkData           (out_ctx* ctx, const HFSPlusForkData* fork, uint32_t cnid, uint8_t forktype) _NONNULL;
void PrintBTNodeDescriptor          (out_ctx* ctx, const BTNodeDescriptor* node) _NONNULL;
void PrintBTHeaderRecord            (out_ctx* ctx, const BTHeaderRec* hr) _NONNULL;
void PrintHFSPlusBSDInfo            (out_ctx* ctx, const HFSPlusBSDInfo* record) _NONNULL;
void PrintFndrFileInfo              (out_ctx* ctx, const FndrFileInfo* record) _NONNULL;
void PrintFndrDirInfo               (out_ctx* ctx, const FndrDirInfo* record) _NONNULL;
void PrintFinderFlags               (out_ctx* ctx, uint16_t record) _NONNULL;
void PrintFndrOpaqueInfo            (out_ctx* ctx, const FndrOpaqueInfo* record) _NONNULL;
void PrintHFSPlusCatalogFolder      (out_ctx* ctx, const HFSPlusCatalogFolder* record) _NONNULL;
void PrintHFSPlusCatalogFile        (out_ctx* ctx, const HFSPlusCatalogFile* record) _NONNULL;
void PrintHFSPlusFolderThreadRecord (out_ctx* ctx, const HFSPlusCatalogThread* record) _NONNULL;
void PrintHFSPlusFileThreadRecord   (out_ctx* ctx, const HFSPlusCatalogThread* record) _NONNULL;
void PrintHFSPlusCatalogThread      (out_ctx* ctx, const HFSPlusCatalogThread* record) _NONNULL;

void VisualizeHFSPlusExtentKey      (out_ctx* ctx, const HFSPlusExtentKey* record, const char* label, bool oneLine) _NONNULL;
void VisualizeHFSPlusCatalogKey     (out_ctx* ctx, const HFSPlusCatalogKey* record, const char* label, bool oneLine) _NONNULL;
void VisualizeHFSPlusAttrKey        (out_ctx* ctx, const HFSPlusAttrKey* record, const char* label, bool oneLine) _NONNULL;
void VisualizeHFSBTreeNodeRecord    (out_ctx* ctx, const char* label, BTHeaderRec headerRecord, const BTreeNodePtr node, BTRecNum recNum) _NONNULL;

void PrintTreeNode                  (out_ctx* ctx, const BTreePtr tree, uint32_t nodeID) _NONNULL;
void PrintNode                      (out_ctx* ctx, const BTreeNodePtr node) _NONNULL;
void PrintFolderListing             (out_ctx* ctx, uint32_t folderID) _NONNULL;
void PrintNodeRecord                (out_ctx* ctx, const BTreeNodePtr node, int recordNumber) _NONNULL;

void PrintHFSPlusExtentRecord       (out_ctx* ctx, const HFSPlusExtentRecord* record) _NONNULL;
void PrintHFSUniStr255              (out_ctx* ctx, const char* label, const HFSUniStr255* record) _NONNULL;

int format_hfs_timestamp            (out_ctx* ctx, char* out, uint32_t timestamp, size_t length) _NONNULL;
int format_hfs_chars                (out_ctx* ctx, char* out, const char* value, size_t nbytes, size_t length) _NONNULL;

#endif
