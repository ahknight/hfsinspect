//
//  hfs_pstruct.h
//  hfsinspect
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hfs_pstruct_h
#define hfsinspect_hfs_pstruct_h

#include "hfs_structs.h"
#include "hfs_macos_defs.h"
#include "hfs_extentlist.h"

#define MAC_GMT_FACTOR 2082844800UL

void set_hfs_volume                 (HFSVolume *v);


void _PrintRawAttribute         (const char* label, const void* map, size_t size, char base);
void _PrintDataLength           (const char* label, u_int64_t size);
void _PrintHFSBlocks            (const char* label, u_int64_t blocks);
void _PrintHFSChar              (const char* label, const void* i, size_t nbytes);
void _PrintHFSTimestamp         (const char* label, u_int32_t timestamp);
void _PrintHFSUniStr255         (const char* label, const HFSUniStr255 *record);
void _PrintCatalogName          (char* label, hfs_node_id cnid);

#define _PrintUI(label, value)                  PrintAttributeString(label, "%llu", (u_int64_t)value);
#define PrintUI(record, value)                  _PrintUI(#value, record->value)

#define _PrintUIOct(label, value)               PrintAttributeString(label, "%06o", value)
#define PrintUIOct(record, value)               _PrintUIOct(#value, record->value)

#define _PrintUIHex(label, value)               PrintAttributeString(label, "%#x", value)
#define PrintUIHex(record, value)               _PrintUIHex(#value, record->value)

#define PrintRawAttribute(record, value, base)  _PrintRawAttribute(#value, &(record->value), sizeof(record->value), base)
#define PrintBinaryDump(record, value)          PrintRawAttribute(record, value, 2)
#define PrintOctalDump(record, value)           PrintRawAttribute(record, value, 8)
#define PrintHexDump(record, value)             PrintRawAttribute(record, value, 16)

#define PrintCatalogName(record, value)         _PrintCatalogName(#value, record->value)
#define PrintDataLength(record, value)          _PrintDataLength(#value, (u_int64_t)record->value)
#define PrintHFSBlocks(record, value)           _PrintHFSBlocks(#value, record->value)
#define PrintHFSChar(record, value)             _PrintHFSChar(#value, &(record->value), sizeof(record->value))
#define PrintHFSTimestamp(record, value)        _PrintHFSTimestamp(#value, record->value)

#define PrintOctFlag(label, value)              PrintSubattributeString("%06o (%s)", value, label)
#define PrintHexFlag(label, value)              PrintSubattributeString("%s (%#x)", label, value)
#define PrintIntFlag(label, value)              PrintSubattributeString("%s (%llu)", label, (u_int64_t)value)

#define PrintUIFlagIfSet(source, flag)          { if (((u_int64_t)(source)) & (((u_int64_t)1) << ((u_int64_t)(flag)))) PrintIntFlag(#flag, flag); }

#define PrintUIFlagIfMatch(source, flag)        { if ((source) & flag) PrintIntFlag(#flag, flag); }
#define PrintUIOctFlagIfMatch(source, flag)     { if ((source) & flag) PrintOctFlag(#flag, flag); }
#define PrintUIHexFlagIfMatch(source, flag)     { if ((source) & flag) PrintHexFlag(#flag, flag); }

#define PrintConstIfEqual(source, c)            { if ((source) == c)   PrintIntFlag(#c, c); }
#define PrintConstOctIfEqual(source, c)         { if ((source) == c)   PrintOctFlag(#c, c); }
#define PrintConstHexIfEqual(source, c)         { if ((source) == c)   PrintHexFlag(#c, c); }


void PrintString                    (const char* label, const char* value_format, ...);
void PrintHeaderString              (const char* value_format, ...);
void PrintAttributeString           (const char* label, const char* value_format, ...);
void PrintSubattributeString        (const char* str, ...);

void PrintVolumeInfo                (const HFSVolume* hfs);
void PrintHFSMasterDirectoryBlock   (const HFSMasterDirectoryBlock* vcb);
void PrintVolumeHeader              (const HFSPlusVolumeHeader *vh);
void PrintExtentList                (const ExtentList* list, u_int32_t totalBlocks);
void PrintForkExtentsSummary        (const HFSFork* fork);
void PrintHFSPlusForkData           (const HFSPlusForkData *fork, u_int32_t cnid, u_int8_t forktype);
void PrintBTNodeDescriptor          (const BTNodeDescriptor *node);
void PrintBTHeaderRecord            (const BTHeaderRec *hr);
void PrintHFSPlusBSDInfo            (const HFSPlusBSDInfo *record);
void PrintFndrFileInfo              (const FndrFileInfo *record);
void PrintFndrDirInfo               (const FndrDirInfo *record);
void PrintFinderFlags               (u_int16_t record);
void PrintFndrOpaqueInfo            (const FndrOpaqueInfo *record);
void PrintHFSPlusCatalogFolder      (const HFSPlusCatalogFolder *record);
void PrintHFSPlusCatalogFile        (const HFSPlusCatalogFile *record);
void PrintHFSPlusFolderThreadRecord (const HFSPlusCatalogThread *record);
void PrintHFSPlusFileThreadRecord   (const HFSPlusCatalogThread *record);
void PrintHFSPlusCatalogThread      (const HFSPlusCatalogThread *record);
void PrintJournalInfoBlock          (const JournalInfoBlock *record);

void PrintVolumeSummary             (const VolumeSummary *summary);
void PrintForkSummary               (const ForkSummary *summary);

void VisualizeHFSPlusExtentKey      (const HFSPlusExtentKey *record, const char* label, bool oneLine);
void VisualizeHFSPlusCatalogKey     (const HFSPlusCatalogKey *record, const char* label, bool oneLine);
void VisualizeHFSPlusAttrKey        (const HFSPlusAttrKey *record, const char* label, bool oneLine);
void VisualizeHFSBTreeNodeRecord    (const HFSBTreeNodeRecord* record, const char* label);
void VisualizeData                  (const char* data, size_t length);

void PrintTreeNode                  (const HFSBTree *tree, u_int32_t nodeID);
void PrintNode                      (const HFSBTreeNode* node);
void PrintFolderListing             (u_int32_t folderID);
void PrintNodeRecord                (const HFSBTreeNode* node, int recordNumber);

void PrintHFSPlusExtentRecord       (const HFSPlusExtentRecord* record);

#endif
