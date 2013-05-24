//
//  hfs_pstruct.c
//  hfstest
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
#include <libkern/OSByteOrder.h>
#include <malloc/malloc.h>
#include <string.h>
#include <sys/stat.h>

#include "hfs_pstruct.h"
#include "hfs.h"
#include "hfs_macos_defs.h"


void _PrintString(const char* label, const char* value_format, ...)
{
    va_list argp;
    va_start(argp, value_format);
    char* str;
    vasprintf(&str, value_format, argp);
    out("  %-23s  %s\n", label, str);
    if (malloc_size(str))
        free(str);
    va_end(argp);
}

void _PrintAttributeString(const char* label, const char* value_format, ...)
{
    va_list argp;
    va_start(argp, value_format);
    char* str;
    vasprintf(&str, value_format, argp);
    out("  %-23s= %s\n", label, str);
    if (malloc_size(str))
        free(str);
    va_end(argp);
}

void _PrintSubattributeString(const char* str, ...)
{
    va_list argp;
    va_start(argp, str);
    char* s;
    vasprintf(&s, str, argp);
    out("  %-23s. %s\n", "", s);
    if (malloc_size(s))
        free(s);
    va_end(argp);
}

void _PrintUI8(const char* label, u_int8_t i)      { _PrintAttributeString(label, "%u", i); }
void _PrintUI8Hex(const char* label, u_int8_t i)   { _PrintAttributeString(label, "0x%X", i); }
void _PrintUI16(const char* label, u_int16_t i)    { _PrintAttributeString(label, "%u", i); }
void _PrintUI16Hex(const char* label, u_int16_t i) { _PrintAttributeString(label, "0x%X", i); }
void _PrintUI32(const char* label, u_int32_t i)    { _PrintAttributeString(label, "%u", i); }
void _PrintUI32Hex(const char* label, u_int32_t i) { _PrintAttributeString(label, "0x%X", i); }
void _PrintUI64(const char* label, u_int64_t i)    { _PrintAttributeString(label, "%llu", i); }
void _PrintUI64Hex(const char* label, u_int64_t i) { _PrintAttributeString(label, "0x%llX", i); }

void _PrintUI64DataLength(const char *label, u_int64_t size)
{
    long double bob = size;
    int count = 0;
    char *sizeNames[] = { "bytes", "KiB", "MiB", "GiB", "TiB", "EiB", "PiB", "ZiB", "YiB" };
    while (count < 9) {
        if (bob < 1024.) break;
        bob /= 1024.;
        count ++;
    }
    _PrintAttributeString(label, "%0.2Lf %s (%llu bytes)", bob, sizeNames[count], size);
}
void _PrintUI32DataLength(const char *label, u_int32_t size)
{
    _PrintUI64DataLength(label, (u_int64_t)size);
}

void _PrintUI8Binary(const char *label, u_int8_t i)
{
    char str[9] = {};
    str[8] = '\0';
    for (int j = 0; j<8; j++) str[7-j] = ((i >> j) & 0x01) ? '1' : '0'; str[8] = '\0';
    _PrintAttributeString(label, "%s", str);
}

void _PrintUI16Binary(const char *label, u_int16_t i)
{
    char str[17] = {};
    str[16] = '\0';
    for (int j = 0; j<16; j++) str[15-j] = ((i >> j) & 0x01) ? '1' : '0'; str[16] = '\0';
    _PrintAttributeString(label, "%s", str);
}

void _PrintUI32Binary(const char *label, u_int32_t i)
{
    char str[65] = {};
    str[64] = '\0';
    for (int j = 0; j<32; j++) str[31-j] = ((i >> j) & 0x01) ? '1' : '0'; str[32] = '\0';
    _PrintAttributeString(label, "%s", str);
}

void _PrintUI64Binary(const char *label, u_int64_t i)
{
    union U64 {
        u_int64_t i64;
        struct {
            // Named without respect to endian-ness.
            u_int32_t first;
            u_int32_t last;
        };
    };
    union U64 u = { i };
    char str1[33], str2[33];
    for (int j = 0; j<32; j++) str1[31-j] = ((u.first >> j) & 0x01) ? '1' : '0'; str1[32] = '\0';
    for (int j = 0; j<32; j++) str2[31-j] = ((u.last >> j) & 0x01) ? '1' : '0'; str2[32] = '\0';
    _PrintAttributeString(label, "%s\n%26s %s", str1, "", str2);
}

void _PrintHFSTimestamp(const char* label, u_int32_t timestamp)
{
    if (timestamp > MAC_GMT_FACTOR) {
        timestamp -= MAC_GMT_FACTOR;
    } else {
        timestamp = 0; // Cannot be negative.
    }
    
    time_t seconds = timestamp;
    struct tm *t = gmtime(&seconds);
    
    char* buf = malloc(50);
    bzero(buf, 50);
    strftime(buf, 50, "%c %Z\0", t);
    _PrintAttributeString(label, buf);
    free(buf);
}

void _PrintUI16Char(const char* label, u_int16_t i)
{
    _PrintAttributeString(label, "0x%x (%c%c)", i, (i>>8) & 0xFF, i & 0xFF);
}

void _PrintUI32Char(const char* label, u_int32_t i)
{
    _PrintAttributeString(label, "0x%x (%c%c%c%c)", i, i>>(3*8) & 0xFF, i>>(2*8) & 0xFF, i>>(1*8) & 0xFF, i & 0xFF);
}

void _PrintHFSUniStr255(const char* label, const HFSUniStr255 *record)
{
    char narrow[256];
    wchar_t wide[256];
    int len = MIN(record->length, 255);
    for (int i = 0; i < len; i++) {
        narrow[i] = wctob(record->unicode[i]);
        wide[i] = record->unicode[i];
    }
    narrow[len] = '\0';
    wide[len] = L'\0';
    
    wprintf(L"  %-23s= %ls\n", label, wide);
}



void PrintVolumeHeader(const HFSPlusVolumeHeader *vh)
{
    VisualizeData((char*)vh, sizeof(HFSPlusVolumeHeader));
    
    out("\n# HFS Plus Volume Header\n");
    _PrintUI16Char      ("signature",              vh->signature);
    _PrintUI16          ("version",                vh->version);
	_PrintUI32Binary    ("attributes",             vh->attributes);
    {
        u_int32_t attributes = vh->attributes;
        const int attribute_count = 12;
        int att_values[attribute_count] = {
            kHFSVolumeHardwareLockMask,
            kHFSVolumeUnmountedMask,
            kHFSVolumeSparedBlocksMask,
            kHFSVolumeNoCacheRequiredMask,
            kHFSBootVolumeInconsistentMask,
            kHFSCatalogNodeIDsReusedMask,
            kHFSVolumeJournaledMask,
            kHFSVolumeInconsistentMask,
            kHFSVolumeSoftwareLockMask,
            kHFSUnusedNodeFixMask,
            kHFSContentProtectionMask,
            kHFSMDBAttributesMask
        };
        char* att_names[attribute_count] = {
            "kHFSVolumeHardwareLockMask",
            "kHFSVolumeUnmountedMask",
            "kHFSVolumeSparedBlocksMask",
            "kHFSVolumeNoCacheRequiredMask",
            "kHFSBootVolumeInconsistentMask",
            "kHFSCatalogNodeIDsReusedMask",
            "kHFSVolumeJournaledMask",
            "kHFSVolumeInconsistentMask",
            "kHFSVolumeSoftwareLockMask",
            "kHFSUnusedNodeFixMask",
            "kHFSContentProtectionMask",
            "kHFSMDBAttributesMask"
        };
        for (int i = 0; i < attribute_count; i++) {
            if (attributes & att_values[i])
                _PrintSubattributeString("%s (%u)", att_names[i], att_values[i]);
        }
    }
    _PrintUI32Char      ("lastMountedVersion",      vh->lastMountedVersion);
	_PrintUI32          ("journalInfoBlock",        vh->journalInfoBlock);
    _PrintHFSTimestamp  ("createDate",              vh->createDate);
    _PrintHFSTimestamp  ("modifyDate",              vh->modifyDate);
    _PrintHFSTimestamp  ("backupDate",              vh->backupDate);
    _PrintHFSTimestamp  ("checkedDate",             vh->checkedDate);
    _PrintUI32          ("fileCount",               vh->fileCount);
	_PrintUI32          ("folderCount",             vh->folderCount);
	_PrintUI32DataLength("blockSize",               vh->blockSize);
	_PrintUI32          ("totalBlocks",             vh->totalBlocks);
	_PrintUI32          ("freeBlocks",              vh->freeBlocks);
    _PrintUI32          ("nextAllocation",          vh->nextAllocation);
    _PrintUI32          ("rsrcClumpSize",           vh->rsrcClumpSize);
    _PrintUI32          ("dataClumpSize",           vh->dataClumpSize);
    _PrintUI32          ("nextCatalogID",           vh->nextCatalogID);
    _PrintUI32          ("writeCount",              vh->writeCount);
    _PrintUI64Binary    ("encodingsBitmap",         vh->encodingsBitmap);
    {
        u_int64_t encodings = vh->encodingsBitmap;
        const int attribute_count = 39;
        int att_values[attribute_count] = {
            kTextEncodingMacRoman,
            kTextEncodingMacJapanese,
            kTextEncodingMacChineseTrad,
            kTextEncodingMacKorean,
            kTextEncodingMacArabic,
            kTextEncodingMacHebrew,
            kTextEncodingMacGreek,
            kTextEncodingMacCyrillic,
            kTextEncodingMacDevanagari,
            kTextEncodingMacGurmukhi,
            kTextEncodingMacGujarati,
            kTextEncodingMacOriya,
            kTextEncodingMacBengali,
            kTextEncodingMacTamil,
            kTextEncodingMacTelugu,
            kTextEncodingMacKannada,
            kTextEncodingMacMalayalam,
            kTextEncodingMacSinhalese,
            kTextEncodingMacBurmese,
            kTextEncodingMacKhmer,
            kTextEncodingMacThai,
            kTextEncodingMacLaotian,
            kTextEncodingMacGeorgian,
            kTextEncodingMacArmenian,
            kTextEncodingMacChineseSimp,
            kTextEncodingMacTibetan,
            kTextEncodingMacMongolian,
            kTextEncodingMacEthiopic,
            kTextEncodingMacCentralEurRoman,
            kTextEncodingMacVietnamese,
            kTextEncodingMacExtArabic,
            kTextEncodingMacSymbol,
            kTextEncodingMacDingbats,
            kTextEncodingMacTurkish,
            kTextEncodingMacCroatian,
            kTextEncodingMacIcelandic,
            kTextEncodingMacRomanian,
            49,
            48,
        };
        char* att_names[attribute_count] = {
            "kTextEncodingMacRoman",
            "kTextEncodingMacJapanese",
            "kTextEncodingMacChineseTrad",
            "kTextEncodingMacKorean",
            "kTextEncodingMacArabic",
            "kTextEncodingMacHebrew",
            "kTextEncodingMacGreek",
            "kTextEncodingMacCyrillic",
            "kTextEncodingMacDevanagari",
            "kTextEncodingMacGurmukhi",
            "kTextEncodingMacGujarati",
            "kTextEncodingMacOriya",
            "kTextEncodingMacBengali",
            "kTextEncodingMacTamil",
            "kTextEncodingMacTelugu",
            "kTextEncodingMacKannada",
            "kTextEncodingMacMalayalam",
            "kTextEncodingMacSinhalese",
            "kTextEncodingMacBurmese",
            "kTextEncodingMacKhmer",
            "kTextEncodingMacThai",
            "kTextEncodingMacLaotian",
            "kTextEncodingMacGeorgian",
            "kTextEncodingMacArmenian",
            "kTextEncodingMacChineseSimp",
            "kTextEncodingMacTibetan",
            "kTextEncodingMacMongolian",
            "kTextEncodingMacEthiopic",
            "kTextEncodingMacCentralEurRoman",
            "kTextEncodingMacVietnamese",
            "kTextEncodingMacExtArabic",
            "kTextEncodingMacSymbol",
            "kTextEncodingMacDingbats",
            "kTextEncodingMacTurkish",
            "kTextEncodingMacCroatian",
            "kTextEncodingMacIcelandic",
            "kTextEncodingMacRomanian",
            "kTextEncodingMacFarsi",
            "kTextEncodingMacUkrainian",
        };
        for (int i = 0; i < attribute_count; i++) {
            if (encodings & ((u_int64_t)1 << att_values[i]))
                _PrintSubattributeString("%s (%u)", att_names[i], att_values[i]);
        }
    }
    out("\n# Finder Info\n");

    // The HFS+ documentation says this is an 8-member array of UI32s.
    // It's a 32-member array of UI8s according to their struct.

    _PrintUI32          ("bootDirID",           OSReadBigInt32(&vh->finderInfo, 0));
    _PrintUI32          ("bootParentID",        OSReadBigInt32(&vh->finderInfo, 4));
    _PrintUI32          ("openWindowDirID",     OSReadBigInt32(&vh->finderInfo, 8));
    _PrintUI32          ("os9DirID",            OSReadBigInt32(&vh->finderInfo, 12));
    _PrintUI32Binary    ("reserved",            OSReadBigInt32(&vh->finderInfo, 16));
    _PrintUI32          ("osXDirID",            OSReadBigInt32(&vh->finderInfo, 20));
    _PrintUI64Hex       ("volID",               OSReadBigInt64(&vh->finderInfo, 24));
    
    out("\n# Allocation Bitmap File\n");
    PrintHFSPlusForkData(&vh->allocationFile);
    
    out("\n# Extents Overflow File\n");
    PrintHFSPlusForkData(&vh->extentsFile);
    
    out("\n# Catalog File\n");
    PrintHFSPlusForkData(&vh->catalogFile);
    
    out("\n# Attributes File\n");
    PrintHFSPlusForkData(&vh->attributesFile);
    
    out("\n# Startup File\n");
    PrintHFSPlusForkData(&vh->startupFile);
}

void PrintHFSPlusExtentRecordBuffer(const Buffer *buffer, unsigned int count, u_int32_t totalBlocks)
{
    debug("Buffer: cap: %zd; size: %zd; off:%zd\n", buffer->capacity, buffer->size, buffer->offset);
    
    _PrintAttributeString("extents", "%12s %12s %12s", "startBlock", "blockCount", "%% of file");
    int usedExtents = 0;
    int catalogBlocks = 0;

    for (int i = 0; i < count; i++) {
        HFSPlusExtentRecord *record = (HFSPlusExtentRecord*)(buffer->data + (sizeof(HFSPlusExtentRecord) * i));
        for (int i = 0; i < kHFSPlusExtentDensity; i++) {
            HFSPlusExtentDescriptor extent = (*record)[i];
            if (extent.startBlock == 0 && extent.blockCount == 0)
                break; // unused extent
            usedExtents++;
            catalogBlocks += extent.blockCount;
            if (totalBlocks) {
                _PrintString("", "%12u %12u %12.2f", extent.startBlock, extent.blockCount, (float)extent.blockCount/(float)totalBlocks*100.);
            } else {
                _PrintString("", "%12u %12u ?", extent.startBlock, extent.blockCount);
            }
        }
    }
    
    _PrintString("", "%d allocation blocks in %d extents total.", catalogBlocks, usedExtents);
    _PrintString("", "%0.2f blocks per extent on average.", ( (float)catalogBlocks / (float)usedExtents) );
}

void PrintExtentsSummary(const HFSFork *fork)
{
    // Create a Buffer and keep track of the count of structs in it for printing.
    Buffer buffer = buffer_alloc(sizeof(HFSPlusExtentRecord));
    buffer.capacity = 1024;
    unsigned int recordCount = 0;
    
    unsigned int blocks = 0;
    HFSPlusExtentRecord forkExtents;
    memcpy(forkExtents, fork->forkData.extents, sizeof(HFSPlusExtentRecord));
    
    HFSPlusExtentRecord *extents = &forkExtents;
    
    ssize_t result;
    
ADD_EXTENT:
    result = buffer_append(&buffer, (void*)extents, sizeof(HFSPlusExtentRecord));
    recordCount++;
    
    for (int i = 0; i < kHFSPlusExtentDensity; i++) blocks += (*extents)[i].blockCount;
    
    if ((*extents)[7].blockCount != 0) {
        u_int32_t record_start = 0;
        u_int8_t found = hfs_find_extent_record(extents, &record_start, fork, blocks);
        if (found && blocks < fork->forkData.totalBlocks) goto ADD_EXTENT;
    }
    
    PrintHFSPlusExtentRecordBuffer(&buffer, recordCount, fork->forkData.totalBlocks);
}

void PrintHFSPlusForkData(const HFSPlusForkData *fork)
{
    _PrintUI64DataLength ("logicalSize",    fork->logicalSize);
    _PrintUI32DataLength ("clumpSize",      fork->clumpSize);
    _PrintUI32           ("totalBlocks",    fork->totalBlocks);
    
    if (fork->totalBlocks) {
        Buffer extentsBuffer = buffer_alloc(sizeof(HFSPlusExtentRecord));
        buffer_set(&extentsBuffer, (char*)fork->extents, sizeof(HFSPlusExtentRecord));
        PrintHFSPlusExtentRecordBuffer(&extentsBuffer, 1, fork->totalBlocks);
        buffer_free(&extentsBuffer);
    }
}

void PrintBTNodeDescriptor(const BTNodeDescriptor *node)
{
    out("\n# B-Tree Node Descriptor\n");
    _PrintUI32   ("fLink",               node->fLink);
    _PrintUI32   ("bLink",               node->bLink);
    {
        u_int64_t attributes = node->kind;
        const int attribute_count = 4;
        int att_values[attribute_count] = {
            kBTLeafNode,
            kBTIndexNode,
            kBTHeaderNode,
            kBTMapNode
        };
        char* att_names[attribute_count] = {
            "kBTLeafNode",
            "kBTIndexNode",
            "kBTHeaderNode",
            "kBTMapNode"
        };
        for (int i = 0; i < attribute_count; i++) {
            if (attributes == att_values[i])
                _PrintAttributeString("kind", "%s (%u)", att_names[i], att_values[i]);
        }
    }
    _PrintUI8    ("height",             node->height);
    _PrintUI16   ("numRecords",         node->numRecords);
    _PrintUI16   ("reserved",           node->reserved);
}

#define PrintUI8(x, y)  _PrintUI8(#y, x->y)
#define PrintUI16(x, y) _PrintUI16(#y, x->y)
#define PrintUI32(x, y) _PrintUI32(#y, x->y)
#define PrintUI64(x, y) _PrintUI64(#y, x->y)

void PrintBTHeaderRecord(const BTHeaderRec *hr)
{
    out("\n# B-Tree Header Record\n");
    PrintUI16(hr, treeDepth);
	_PrintUI16   ("treeDepth",          hr->treeDepth);
	_PrintUI32   ("rootNode",           hr->rootNode);
	_PrintUI32   ("leafRecords",        hr->leafRecords);
	_PrintUI32   ("firstLeafNode",      hr->firstLeafNode);
	_PrintUI32   ("lastLeafNode",       hr->lastLeafNode);
	_PrintUI16   ("nodeSize",           hr->nodeSize);
	_PrintUI16   ("maxKeyLength",       hr->maxKeyLength);
	_PrintUI32   ("totalNodes",         hr->totalNodes);
	_PrintUI32   ("freeNodes",          hr->freeNodes);
	_PrintUI16   ("reserved1",          hr->reserved1);
	_PrintUI32   ("clumpSize",          hr->clumpSize);
    {
        u_int64_t attributes = hr->btreeType;
        const int attribute_count = 3;
        int att_values[attribute_count] = {
            0,
            128,
            255
        };
        char* att_names[attribute_count] = {
            "kHFSBTreeType",
            "kUserBTreeType",
            "kReservedBTreeType"
        };
        for (int i = 0; i < attribute_count; i++) {
            if (attributes == att_values[i])
                _PrintAttributeString("btreeType", "%s (%u)", att_names[i], att_values[i]);
        }
    }
	_PrintUI8Hex ("keyCompareType",     hr->keyCompareType);
	_PrintUI32Binary("attributes",      hr->attributes);

    {
        u_int64_t attributes = hr->attributes;
        const int attribute_count = 3;
        int att_values[attribute_count] = {
            kBTBadCloseMask,
            kBTBigKeysMask,
            kBTVariableIndexKeysMask
        };
        char* att_names[attribute_count] = {
            "kBTBadCloseMask",
            "kBTBigKeysMask",
            "kBTVariableIndexKeysMask"
        };
        for (int i = 0; i < attribute_count; i++) {
            if (attributes & att_values[i])
                _PrintSubattributeString("%s (%u)", att_names[i], att_values[i]);
        }
    }

//	out("Reserved3:        %u\n",    hr->reserved3[16]);
}

void PrintHFSPlusBSDInfo(const HFSPlusBSDInfo *record)
{
    PrintUI32(record, ownerID);
    PrintUI32(record, groupID);
    
    int flagMasks[] = {
        UF_NODUMP,
        UF_IMMUTABLE,
        UF_APPEND,
        UF_OPAQUE,
        UF_COMPRESSED,
        UF_TRACKED,
        UF_HIDDEN,
        SF_ARCHIVED,
        SF_IMMUTABLE,
        SF_APPEND,
    };
    
    char* flagNames[] = {
        "immutable",
        "appendOnly",
        "directoryOpaque",
        "compressed",
        "tracked",
        "hidden",
        "archived (superuser)",
        "immutable (superuser)",
        "append (superuser)",
    };
    
    _PrintUI8Binary("adminFlags", record->adminFlags);
    
    for (int i = 0; i < 10; i++) {
        u_int8_t flag = record->adminFlags;
        if (flag & flagMasks[i]) {
            _PrintSubattributeString("%s", flagNames[i]);
        }
    }
    
    _PrintUI8Binary("ownerFlags", record->ownerFlags);
    
    for (int i = 0; i < 10; i++) {
        u_int8_t flag = record->ownerFlags;
        if (flag & flagMasks[i]) {
            _PrintSubattributeString("%s", flagNames[i]);
        }
    }
    
    u_int16_t mode = record->fileMode;
    
    if (S_ISBLK(mode)) {
        _PrintAttributeString("fileMode (type)", "block special");
    }
    if (S_ISCHR(mode)) {
        _PrintAttributeString("fileMode (type)", "character special");
    }
    if (S_ISDIR(mode)) {
        _PrintAttributeString("fileMode (type)", "directory");
    }
    if (S_ISFIFO(mode)) {
        _PrintAttributeString("fileMode (type)", "named pipe");
    }
    if (S_ISREG(mode)) {
        _PrintAttributeString("fileMode (type)", "regular file");
    }
    if (S_ISLNK(mode)) {
        _PrintAttributeString("fileMode (type)", "symbolic link");
    }
    if (S_ISSOCK(mode)) {
        _PrintAttributeString("fileMode (type)", "socket");
    }
    if (S_ISWHT(mode)) {
        _PrintAttributeString("fileMode (type)", "whiteout");
    }
    
    _PrintUI16Binary("fileMode", mode);
    
    int modes[] = {
        S_ISUID,
        S_ISGID,
        S_ISTXT,
        
        S_IRUSR,
        S_IWUSR,
        S_IXUSR,
        
        S_IRGRP,
        S_IWGRP,
        S_IXGRP,
        
        S_IROTH,
        S_IWOTH,
        S_IXOTH,
    };
    char* names[] = {
        "setUID",
        "setGID",
        "sticky",
        
        "read (owner)",
        "write (owner)",
        "execute (owner)",
        
        "read (group)",
        "write (group)",
        "execute (group)",
        
        "read (others)",
        "write (others)",
        "execute (others)",
    };
    
    for (int i = 0; i < 12; i++) {
        u_int16_t mode = record->fileMode;
        if (mode & modes[i]) {
            _PrintSubattributeString("%s", names[i]);
        }
    }
    
    PrintUI32(record, special.linkCount);
}

void PrintFndrFileInfo(const FndrFileInfo *record)
{
    _PrintUI32Char("fdType", record->fdType);
    _PrintUI32Char("fdCreator", record->fdCreator);
    PrintUI16(record, fdFlags);
    PrintUI16(record, fdLocation.v);
    PrintUI16(record, fdLocation.h);
    PrintUI16(record, opaque);
}

void PrintFndrDirInfo(const FndrDirInfo *record)
{
    PrintUI16(record, frRect.top);
    PrintUI16(record, frRect.left);
    PrintUI16(record, frRect.bottom);
    PrintUI16(record, frRect.right);
    PrintUI8(record, frFlags);
    PrintUI16(record, frLocation.v);
    PrintUI16(record, frLocation.h);
    PrintUI16(record, opaque);
}

void PrintFndrOpaqueInfo(const FndrOpaqueInfo *record)
{
    // It's opaque. Provided for completeness, and just incase some properties are discovered.
}

void PrintHFSPlusCatalogFolder(const HFSPlusCatalogFolder *record)
{
    _PrintAttributeString("recordType", "kHFSPlusFolderRecord");
    _PrintUI16Binary("flags", record->flags); //TODO: Add flags.
    PrintUI16(record, valence);
    PrintUI32(record, folderID);
    _PrintHFSTimestamp("createDate", record->createDate);
    _PrintHFSTimestamp("contentModDate", record->contentModDate);
    _PrintHFSTimestamp("attributeModDate", record->attributeModDate);
    _PrintHFSTimestamp("accessDate", record->accessDate);
    _PrintHFSTimestamp("backupDate", record->backupDate);
    PrintHFSPlusBSDInfo(&record->bsdInfo);
    PrintFndrDirInfo(&record->userInfo);
    PrintFndrOpaqueInfo(&record->finderInfo);
    PrintUI32(record, textEncoding);
    PrintUI32(record, folderCount);
}

void PrintHFSPlusCatalogFile(const HFSPlusCatalogFile *record)
{
    _PrintAttributeString("recordType", "kHFSPlusFileRecord");
    _PrintUI16Binary("flags", record->flags); //TODO: Add the rest of the flags.
    if (record->flags & kHFSFileLockedMask)
        _PrintSubattributeString("kHFSFileLockedMask");
    if (record->flags & kHFSThreadExistsMask)
        _PrintSubattributeString("kHFSThreadExistsMask");
    PrintUI16(record, reserved1);
    PrintUI32(record, fileID);
    _PrintHFSTimestamp("createDate", record->createDate);
    _PrintHFSTimestamp("contentModDate", record->contentModDate);
    _PrintHFSTimestamp("attributeModDate", record->attributeModDate);
    _PrintHFSTimestamp("accessDate", record->accessDate);
    _PrintHFSTimestamp("backupDate", record->backupDate);
    PrintHFSPlusBSDInfo(&record->bsdInfo);
    PrintFndrFileInfo(&record->userInfo);
    PrintFndrOpaqueInfo(&record->finderInfo);
    PrintUI32(record, textEncoding);
    PrintUI32(record, reserved2);
    PrintHFSPlusForkData(&record->dataFork);
    PrintHFSPlusForkData(&record->resourceFork);
}

void PrintHFSPlusFolderThreadRecord(const HFSPlusCatalogThread *record)
{
    _PrintAttributeString("recordType", "kHFSPlusFolderThreadRecord");
    PrintHFSPlusCatalogThread(record);
}

void PrintHFSPlusFileThreadRecord(const HFSPlusCatalogThread *record)
{
    _PrintAttributeString("recordType", "kHFSPlusFileThreadRecord");
    PrintHFSPlusCatalogThread(record);
}

void PrintHFSPlusCatalogThread(const HFSPlusCatalogThread *record)
{
    //    PrintUI16(record, recordType);
    PrintUI16(record, reserved);
    PrintUI32(record, parentID);
    _PrintHFSUniStr255("nodeName", &record->nodeName);
}

void VisualizeHFSPlusExtentKey(const HFSPlusExtentKey *record, const char* label)
{
    out("\n  %s\n", label);
    out("  +-----------------------------------------------+\n");
    out("  | length | fork | pad | fileID     | startBlock |\n");
    out("  | %-6u | %-3u  | %-3u | %-10u | %-10u |\n",
           record->keyLength,
           record->forkType,
           record->pad,
           record->fileID,
           record->startBlock);
//    out("  | %04x   | %02x   | %02x  | %08x   | %08x   |\n",
//           record->keyLength,
//           record->forkType,
//           record->pad,
//           record->fileID,
//           record->startBlock);
    out("  +-----------------------------------------------+\n");
}

void PrintHFSPlusExtentKey(const HFSPlusExtentKey *record)
{
    PrintUI16(record, keyLength);
    PrintUI8(record, forkType);
    PrintUI8(record, pad);
    PrintUI32(record, fileID);
    PrintUI32(record, startBlock);
}

void PrintHFSPlusCatalogKey(const HFSPlusCatalogKey *record)
{
    PrintUI16(record, keyLength);
    PrintUI32(record, parentID);
    _PrintHFSUniStr255("key", &record->nodeName);
}

void VisualizeData(const char* data, size_t length)
{
    unsigned int offset = 0;
    while (length > offset) {
        const char* line = &data[offset];
        size_t lineMax = MIN((length - offset), 16);
        out("  |");
        for (int c = 0; c < lineMax; c++) {
            if ( (c % 4) == 0 ) out(" ");
            out("%02x ", line[c] & 0xFF);
        }
        out("| ");
        for (int c = 0; c < lineMax; c++) {
            if ( (c % 4) == 0 ) out(" ");
            char chr = line[c] & 0xFF;
            if (chr < 32)
                chr = '.';
            out("%c", chr);
        }
        out(" |\n");
        offset += 16;
    }
}


void PrintNode(const HFSBTreeNode* node)
{
    out("\n# Node %d (offset %d; length: %zd)\n", node->nodeNumber, node->nodeOffset, node->buffer.size);
    PrintBTNodeDescriptor(&node->nodeDescriptor);
    
    for (int recordNumber = 0; recordNumber < node->nodeDescriptor.numRecords; recordNumber++) {
        PrintNodeRecord(node, recordNumber);
    }
}

void PrintNodeRecord(const HFSBTreeNode* node, int recordNumber)
{
    if(recordNumber >= node->nodeDescriptor.numRecords) return;
    
    if (recordNumber != node->nodeDescriptor.numRecords) {
        u_int16_t offset1 = hfs_btree_get_record_offset(node, recordNumber);
        u_int16_t offset2 = hfs_btree_get_record_offset(node, recordNumber+1);
        if (offset1 >= offset2) {
            error("abort: Record offset is not smaller than the next offset. What are you trying to feed me?\n");
            exit(1);
        }
    }
    u_int16_t offset = hfs_btree_get_record_offset(node, recordNumber);
    if (offset > node->bTree.headerRecord.nodeSize) {
        error("Invalid record offset: points beyond end of node: %d\n", offset);
        return;
    }
    
    ssize_t record_size = hfs_btree_get_record_size(node, recordNumber);
    if (record_size < 1) {
        error("Invalid record size: %zd.\n", record_size);
        return;
    }
    
    Buffer record = hfs_btree_get_record(node, recordNumber);
    
    if (record.size == 0) {
        error("Invalid record: no data found.\n");
        return;
    }
    
    
    out("\n  # Record %d of %d (offset %zd; length: %zd)\n",
           recordNumber + 1,
           node->nodeDescriptor.numRecords,
           offset,
           hfs_btree_get_record_size(node, recordNumber));
    
    switch (node->nodeDescriptor.kind) {
        case kBTHeaderNode:
        {
            switch (recordNumber) {
                case 0:
                {
                    _PrintAttributeString("recordType", "BTHeaderRec");
                    BTHeaderRec header = *( (BTHeaderRec*) record.data );
                    PrintBTHeaderRecord(&header);
                    break;
                }
                case 1:
                {
                    _PrintAttributeString("recordType", "userData (reserved)");
                    break;
                }
                case 2:
                {
                    _PrintAttributeString("recordType", "mapData");
                    break;
                }
            }
            break;
        }
        case kBTMapNode:
        {
            break;
        }
            
        default:
        {
            // Handle all keyed node records
            Buffer key;
            Buffer value;
            
            ssize_t key_length = hfs_btree_decompose_keyed_record(node, &record, &key, &value);
            
            switch (node->bTree.fork.cnid) {
                case kHFSCatalogFileID:
                {
                    HFSPlusCatalogKey keyStruct = *( (HFSPlusCatalogKey*) key.data );
                    PrintHFSPlusCatalogKey(&keyStruct);
                    
                    if (key_length < kHFSPlusCatalogKeyMinimumLength || key_length > kHFSPlusCatalogKeyMaximumLength)
                        goto INVALID_KEY;
                    
                    break;
                };
                    
                case kHFSExtentsFileID:
                {
                    HFSPlusExtentKey keyStruct = *( (HFSPlusExtentKey*) key.data );
                    VisualizeHFSPlusExtentKey(&keyStruct, "Key");
                    
                    if (key_length != kHFSPlusExtentKeyMaximumLength)
                        goto INVALID_KEY;
                    
                    break;
                };
                    
                default:
                    // TODO: Attributes file support.
                    goto INVALID_KEY;
            }
            
            switch (node->nodeDescriptor.kind) {
                case kBTIndexNode:
                {
                    u_int32_t *pointer = (u_int32_t*) value.data;
                    _PrintUI32("nextNodeID", *pointer);
                    break;
                }
                    
                case kBTLeafNode:
                {
                    switch (node->bTree.fork.cnid) {
                        case kHFSCatalogFileID:
                        {
                            u_int16_t type =  *(u_int16_t*)(value.data);
                            
                            switch (type) {
                                case kHFSPlusFileRecord:
                                {
                                    HFSPlusCatalogFile recordValue = *( (HFSPlusCatalogFile*) value.data );
                                    PrintHFSPlusCatalogFile(&recordValue);
                                    break;
                                }
                                    
                                case kHFSPlusFolderRecord:
                                {
                                    HFSPlusCatalogFolder recordValue = *( (HFSPlusCatalogFolder*) value.data );
                                    PrintHFSPlusCatalogFolder(&recordValue);
                                    break;
                                }
                                    
                                case kHFSPlusFileThreadRecord:
                                {
                                    HFSPlusCatalogThread recordValue = *( (HFSPlusCatalogThread*) value.data );
                                    PrintHFSPlusFileThreadRecord(&recordValue);
                                    break;
                                }
                                    
                                case kHFSPlusFolderThreadRecord:
                                {
                                    HFSPlusCatalogThread recordValue = *( (HFSPlusCatalogThread*) value.data );
                                    PrintHFSPlusFolderThreadRecord(&recordValue);
                                    break;
                                }
                                    
                                default:
                                {
                                    _PrintAttributeString("recordType", "%d (invalid)", type);
                                    ;;
                                }
                            }
                            break;
                        }
                        case kHFSExtentsFileID:
                        {
                            PrintHFSPlusExtentRecordBuffer(&value, 1, 0);
                            break;
                        }
                    }
                    
                    break;
                }
                    
                default:
                {
                INVALID_KEY:
                    if ((recordNumber + 1) == node->nodeDescriptor.numRecords) {
                        _PrintAttributeString("recordType", "(free space)");
                    } else {
                        _PrintAttributeString("recordType", "(unknown record format)");
                        VisualizeData(record.data, record_size);
                    }
                }
            }
            
            buffer_free(&key);
            buffer_free(&value);
            
            break;
        }
    }
    
    buffer_free(&record);
}

void PrintTreeNode(const HFSBTree *tree, u_int32_t nodeID)
{
    HFSBTreeNode node = {};
    int8_t result = hfs_btree_get_node(&node, tree, nodeID);
    if (result == -1) {
        error("PrintTreeNode: node %d is invalid or out of range.\n", nodeID);
        return;
    }
    PrintNode(&node);
    hfs_btree_free_node(&node);
}
