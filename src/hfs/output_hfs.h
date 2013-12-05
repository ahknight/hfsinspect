//
//  output_hfs.h
//  hfsinspect
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_output_hfs_h
#define hfsinspect_output_hfs_h

#include "hfs/hfs_types.h"
#include "hfs/hfs_extentlist.h"
#include "hfs/Apple/vfs_journal.h"

#define MAC_GMT_FACTOR 2082844800UL

void set_hfs_volume                 (HFS *v) _NONNULL;


void _PrintHFSBlocks                (const char* label, uint64_t blocks) _NONNULL;
void _PrintHFSChar                  (const char* label, const char* i, size_t nbytes) _NONNULL;
void _PrintHFSTimestamp             (const char* label, uint32_t timestamp) _NONNULL;
void _PrintHFSUniStr255             (const char* label, const HFSUniStr255 *record) _NONNULL;
void _PrintCatalogName              (char* label, bt_nodeid_t cnid) _NONNULL;


#define PrintCatalogName(record, value)         _PrintCatalogName(#value, record->value)
#define PrintHFSBlocks(record, value)           _PrintHFSBlocks(#value, record->value)
#define PrintHFSChar(record, value)             _PrintHFSChar(#value, (char*)&(record->value), sizeof(record->value))
#define PrintHFSTimestamp(record, value)        _PrintHFSTimestamp(#value, record->value)



void PrintVolumeInfo                (const HFS* hfs) _NONNULL;
void PrintHFSMasterDirectoryBlock   (const HFSMasterDirectoryBlock* vcb) _NONNULL;
void PrintVolumeHeader              (const HFSPlusVolumeHeader *vh) _NONNULL;
void PrintExtentList                (const ExtentList* list, uint32_t totalBlocks) _NONNULL;
void PrintForkExtentsSummary        (const HFSFork* fork) _NONNULL;
void PrintHFSPlusForkData           (const HFSPlusForkData *fork, uint32_t cnid, uint8_t forktype) _NONNULL;
void PrintBTNodeDescriptor          (const BTNodeDescriptor *node) _NONNULL;
void PrintBTHeaderRecord            (const BTHeaderRec *hr) _NONNULL;
void PrintHFSPlusBSDInfo            (const HFSPlusBSDInfo *record) _NONNULL;
void PrintFndrFileInfo              (const FndrFileInfo *record) _NONNULL;
void PrintFndrDirInfo               (const FndrDirInfo *record) _NONNULL;
void PrintFinderFlags               (uint16_t record) _NONNULL;
void PrintFndrOpaqueInfo            (const FndrOpaqueInfo *record) _NONNULL;
void PrintHFSPlusCatalogFolder      (const HFSPlusCatalogFolder *record) _NONNULL;
void PrintHFSPlusCatalogFile        (const HFSPlusCatalogFile *record) _NONNULL;
void PrintHFSPlusFolderThreadRecord (const HFSPlusCatalogThread *record) _NONNULL;
void PrintHFSPlusFileThreadRecord   (const HFSPlusCatalogThread *record) _NONNULL;
void PrintHFSPlusCatalogThread      (const HFSPlusCatalogThread *record) _NONNULL;
void PrintJournalInfoBlock          (const JournalInfoBlock *record) _NONNULL;
void PrintJournalHeader             (const journal_header *record) _NONNULL;

void PrintVolumeSummary             (const VolumeSummary *summary) _NONNULL;
void PrintForkSummary               (const ForkSummary *summary) _NONNULL;

void VisualizeHFSPlusExtentKey      (const HFSPlusExtentKey *record, const char* label, bool oneLine) _NONNULL;
void VisualizeHFSPlusCatalogKey     (const HFSPlusCatalogKey *record, const char* label, bool oneLine) _NONNULL;
void VisualizeHFSPlusAttrKey        (const HFSPlusAttrKey *record, const char* label, bool oneLine) _NONNULL;
void VisualizeHFSBTreeNodeRecord    (const char* label, BTHeaderRec headerRecord, const BTreeNodePtr node, BTRecNum recNum) _NONNULL;

void PrintTreeNode                  (const BTreePtr tree, uint32_t nodeID) _NONNULL;
void PrintNode                      (const BTreeNodePtr node) _NONNULL;
void PrintFolderListing             (uint32_t folderID) _NONNULL;
void PrintNodeRecord                (const BTreeNodePtr node, int recordNumber) _NONNULL;

void PrintHFSPlusExtentRecord       (const HFSPlusExtentRecord* record) _NONNULL;

int format_hfs_timestamp(char* out, uint32_t timestamp, size_t length) _NONNULL;
int format_hfs_chars(char* out, const char* value, size_t nbytes, size_t length) _NONNULL;

#endif
