//
//  output_hfs.h
//  hfsinspect
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_output_hfs_h
#define hfsinspect_output_hfs_h

#include "hfs/hfs_structs.h"
#include "hfs/hfs_extentlist.h"
#include "hfs/Apple/vfs_journal.h"

#define MAC_GMT_FACTOR 2082844800UL

void set_hfs_volume                 (HFS *v);


void _PrintHFSBlocks                (const char* label, uint64_t blocks);
void _PrintHFSChar                  (const char* label, const char* i, size_t nbytes);
void _PrintHFSTimestamp             (const char* label, uint32_t timestamp);
void _PrintHFSUniStr255             (const char* label, const HFSUniStr255 *record);
void _PrintCatalogName              (char* label, bt_nodeid_t cnid);


#define PrintCatalogName(record, value)         _PrintCatalogName(#value, record->value)
#define PrintHFSBlocks(record, value)           _PrintHFSBlocks(#value, record->value)
#define PrintHFSChar(record, value)             _PrintHFSChar(#value, (char*)&(record->value), sizeof(record->value))
#define PrintHFSTimestamp(record, value)        _PrintHFSTimestamp(#value, record->value)



void PrintVolumeInfo                (const HFS* hfs);
void PrintHFSMasterDirectoryBlock   (const HFSMasterDirectoryBlock* vcb);
void PrintVolumeHeader              (const HFSPlusVolumeHeader *vh);
void PrintExtentList                (const ExtentList* list, uint32_t totalBlocks);
void PrintForkExtentsSummary        (const HFSFork* fork);
void PrintHFSPlusForkData           (const HFSPlusForkData *fork, uint32_t cnid, uint8_t forktype);
void PrintBTNodeDescriptor          (const BTNodeDescriptor *node);
void PrintBTHeaderRecord            (const BTHeaderRec *hr);
void PrintHFSPlusBSDInfo            (const HFSPlusBSDInfo *record);
void PrintFndrFileInfo              (const FndrFileInfo *record);
void PrintFndrDirInfo               (const FndrDirInfo *record);
void PrintFinderFlags               (uint16_t record);
void PrintFndrOpaqueInfo            (const FndrOpaqueInfo *record);
void PrintHFSPlusCatalogFolder      (const HFSPlusCatalogFolder *record);
void PrintHFSPlusCatalogFile        (const HFSPlusCatalogFile *record);
void PrintHFSPlusFolderThreadRecord (const HFSPlusCatalogThread *record);
void PrintHFSPlusFileThreadRecord   (const HFSPlusCatalogThread *record);
void PrintHFSPlusCatalogThread      (const HFSPlusCatalogThread *record);
void PrintJournalInfoBlock          (const JournalInfoBlock *record);
void PrintJournalHeader             (const journal_header *record);

void PrintVolumeSummary             (const VolumeSummary *summary);
void PrintForkSummary               (const ForkSummary *summary);

void VisualizeHFSPlusExtentKey      (const HFSPlusExtentKey *record, const char* label, bool oneLine);
void VisualizeHFSPlusCatalogKey     (const HFSPlusCatalogKey *record, const char* label, bool oneLine);
void VisualizeHFSPlusAttrKey        (const HFSPlusAttrKey *record, const char* label, bool oneLine);
void VisualizeHFSBTreeNodeRecord    (const char* label, BTHeaderRec headerRecord, const BTreeNodePtr node, BTRecNum recNum);

void PrintTreeNode                  (const BTreePtr tree, uint32_t nodeID);
void PrintNode                      (const BTreeNodePtr node);
void PrintFolderListing             (uint32_t folderID);
void PrintNodeRecord                (const BTreeNodePtr node, int recordNumber);

void PrintHFSPlusExtentRecord       (const HFSPlusExtentRecord* record);

int format_hfs_timestamp(char* out, uint32_t timestamp, size_t length);
int format_hfs_chars(char* out, const char* value, size_t nbytes, size_t length);

#endif
