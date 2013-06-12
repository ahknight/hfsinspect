//
//  hfs_pstruct.c
//  hfsinspect
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "stringtools.h"

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
#include <stdbool.h>

#include "hfs_macos_defs.h"
#include "hfs_pstruct.h"
#include "hfs.h"
#include "vfs_journal.h"

#define _PrintUI(label, value)                  _PrintAttributeString(label, "%llu", (u_int64_t)value);
#define PrintUI(record, value)                  _PrintUI(#value, record->value)

#define _PrintUIOct(label, value)               _PrintAttributeString(label, "%06o", value)
#define PrintUIOct(record, value)               _PrintUIOct(#value, record->value)

#define _PrintUIHex(label, value)               _PrintAttributeString(label, "%#x", value)
#define PrintUIHex(record, value)               _PrintUIHex(#value, record->value)

#define PrintRawAttribute(record, value, base)  _PrintRawAttribute(#value, &(record->value), sizeof(record->value), base)
#define PrintBinaryDump(record, value)          PrintRawAttribute(record, value, 2)
#define PrintOctalDump(record, value)           PrintRawAttribute(record, value, 8)
#define PrintHexDump(record, value)             PrintRawAttribute(record, value, 16)

#define PrintDataLength(record, value)          _PrintDataLength(#value, (u_int64_t)record->value)
#define PrintHFSBlocks(record, value)           _PrintHFSBlocks(#value, record->value)
#define PrintHFSChar(record, value)             _PrintHFSChar(#value, &(record->value), sizeof(record->value))
#define PrintHFSTimestamp(record, value)        _PrintHFSTimestamp(#value, record->value)

#define PrintOctFlag(label, value)              _PrintSubattributeString("%06o (%s)", value, label)
#define PrintHexFlag(label, value)              _PrintSubattributeString("%s (%#x)", label, value)
#define PrintIntFlag(label, value)              _PrintSubattributeString("%s (%llu)", label, (u_int64_t)value)

#define PrintUIFlagIfSet(source, flag)          { if (((u_int64_t)(source)) & (((u_int64_t)1) << ((u_int64_t)(flag)))) PrintIntFlag(#flag, flag); }

#define PrintUIFlagIfMatch(source, flag)        { if ((source) & flag) PrintIntFlag(#flag, flag); }
#define PrintUIOctFlagIfMatch(source, flag)     { if ((source) & flag) PrintOctFlag(#flag, flag); }
#define PrintUIHexFlagIfMatch(source, flag)     { if ((source) & flag) PrintHexFlag(#flag, flag); }

#define PrintConstIfEqual(source, c)            { if ((source) == c)   PrintIntFlag(#c, c); }
#define PrintConstOctIfEqual(source, c)         { if ((source) == c)   PrintOctFlag(#c, c); }
#define PrintConstHexIfEqual(source, c)         { if ((source) == c)   PrintHexFlag(#c, c); }

void _PrintString               (const char* label, const char* value_format, ...);
void _PrintHeaderString         (const char* value_format, ...);
void _PrintAttributeString      (const char* label, const char* value_format, ...);
void _PrintSubattributeString   (const char* str, ...);

void _PrintRawAttribute         (const char* label, const void* map, size_t size, char base);

void _PrintDataLength           (const char* label, u_int64_t size);
void _PrintHFSBlocks            (const char* label, u_int64_t blocks);
void _PrintHFSChar              (const char* label, const void* i, size_t nbytes);
void _PrintHFSTimestamp         (const char* label, u_int32_t timestamp);
void _PrintHFSUniStr255         (const char* label, const HFSUniStr255 *record);

static HFSVolume volume = {0};
void set_hfs_volume(HFSVolume *v) { volume = *v; }


#pragma mark Line Print Functions

void _PrintString(const char* label, const char* value_format, ...)
{
    va_list argp;
    va_start(argp, value_format);
    char* str;
    vasprintf(&str, value_format, argp);
    printf("  %-23s  %s\n", label, str);
    if (malloc_size(str))
        free(str);
    va_end(argp);
}

void _PrintHeaderString(const char* value_format, ...)
{
    va_list argp;
    va_start(argp, value_format);
    char* str;
    vasprintf(&str, value_format, argp);
    printf("\n# %s\n", str);
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
    printf("  %-23s= %s\n", label, str);
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
    printf("  %-23s. %s\n", "", s);
    if (malloc_size(s))
        free(s);
    va_end(argp);
}

#pragma mark Value Print Functions

void _PrintHFSBlocks(const char *label, u_int64_t blocks)
{
    size_t displaySize = blocks * volume.vh.blockSize;
    char* sizeLabel = sizeString(displaySize);
    _PrintAttributeString(label, "%s (%d blocks)", sizeLabel, blocks);
    free(sizeLabel);
}

void _PrintDataLength(const char *label, u_int64_t size)
{
    size_t displaySize = size;
    char* sizeLabel = sizeString(displaySize);
    _PrintAttributeString(label, "%s (%lu bytes)", sizeLabel, size);
    free(sizeLabel);
}

void _PrintRawAttribute(const char* label, const void* map, size_t size, char base)
{
    unsigned segmentLength = 32;
    char* str = memstr(map, size, base);
    size_t len = strlen(str);
    
    for (int i = 0; i < len; i += segmentLength) {
        char* segment = strndup(&str[i], MIN(segmentLength, len - i));
        _PrintAttributeString(label, "%s%s", (base==16?"0x":""), segment);
        free(segment);
        if (i == 0) label = "";
    }
    
    free(str);
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

void _PrintHFSChar(const char* label, const void* i, size_t nbytes)
{
    // Iterates over the input (i) from left to right, copying bytes
    // in reverse order as chars in a string. Reverse order because we
    // presume the host is little-endian.
    char* str = malloc(nbytes + 1);
    for (size_t byte = 0; byte < nbytes; byte++) {
        str[byte] = ((char*)i)[(nbytes-1) - byte];
    }
    str[nbytes] = '\0';
    
    // Grab the hex representation of the input.
    char* hex = memstr(i, nbytes, 16);
    
    _PrintAttributeString(label, "0x%s (%s)", hex, str);
    
    // Cleanup
    free(hex);
    free(str);
}

void _PrintHFSUniStr255(const char* label, const HFSUniStr255 *record)
{
    wchar_t *wide = hfsuctowcs(record);
    wprintf(L"  %-23s= \"%ls\" (%u)\n", label, wide, record->length);
}


#pragma mark Structure Print Functions

void PrintVolumeHeader(const HFSPlusVolumeHeader *vh)
{
    //    VisualizeData((char*)vh, sizeof(HFSPlusVolumeHeader));
    
    _PrintHeaderString("HFS Plus Volume Header");
    PrintHFSChar    (vh,    signature);
    PrintUI         (vh,    version);

	PrintBinaryDump         (vh,                attributes);
    PrintUIFlagIfMatch      (vh->attributes,    kHFSVolumeHardwareLockMask);
    PrintUIFlagIfMatch      (vh->attributes,    kHFSVolumeUnmountedMask);
    PrintUIFlagIfMatch      (vh->attributes,    kHFSVolumeSparedBlocksMask);
    PrintUIFlagIfMatch      (vh->attributes,    kHFSVolumeNoCacheRequiredMask);
    PrintUIFlagIfMatch      (vh->attributes,    kHFSBootVolumeInconsistentMask);
    PrintUIFlagIfMatch      (vh->attributes,    kHFSCatalogNodeIDsReusedMask);
    PrintUIFlagIfMatch      (vh->attributes,    kHFSVolumeJournaledMask);
    PrintUIFlagIfMatch      (vh->attributes,    kHFSVolumeInconsistentMask);
    PrintUIFlagIfMatch      (vh->attributes,    kHFSVolumeSoftwareLockMask);
    PrintUIFlagIfMatch      (vh->attributes,    kHFSUnusedNodeFixMask);
    PrintUIFlagIfMatch      (vh->attributes,    kHFSContentProtectionMask);
    PrintUIFlagIfMatch      (vh->attributes,    kHFSMDBAttributesMask);
    
    PrintHFSChar            (vh,    lastMountedVersion);
	PrintUI                 (vh,    journalInfoBlock);
    PrintHFSTimestamp       (vh,    createDate);
    PrintHFSTimestamp       (vh,    modifyDate);
    PrintHFSTimestamp       (vh,    backupDate);
    PrintHFSTimestamp       (vh,    checkedDate);
    PrintUI                 (vh,    fileCount);
	PrintUI                 (vh,    folderCount);
	PrintDataLength         (vh,    blockSize);
	PrintHFSBlocks          (vh,    totalBlocks);
	PrintHFSBlocks          (vh,    freeBlocks);
    PrintUI                 (vh,    nextAllocation);
    PrintDataLength         (vh,    rsrcClumpSize);
    PrintDataLength         (vh,    dataClumpSize);
    PrintUI                 (vh,    nextCatalogID);
    PrintUI                 (vh,    writeCount);
    
    PrintBinaryDump         (vh,                    encodingsBitmap);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacRoman);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacJapanese);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacChineseTrad);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacKorean);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacArabic);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacHebrew);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacGreek);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacCyrillic);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacDevanagari);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacGurmukhi);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacGujarati);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacOriya);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacBengali);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacTamil);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacTelugu);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacKannada);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacMalayalam);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacSinhalese);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacBurmese);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacKhmer);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacThai);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacLaotian);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacGeorgian);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacArmenian);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacChineseSimp);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacTibetan);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacMongolian);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacEthiopic);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacCentralEurRoman);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacVietnamese);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacExtArabic);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacSymbol);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacDingbats);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacTurkish);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacCroatian);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacIcelandic);
    PrintUIFlagIfSet        (vh->encodingsBitmap,   kTextEncodingMacRomanian);
    if (vh->encodingsBitmap & ((u_int64_t)1 << 49)) _PrintSubattributeString("%s (%u)", "kTextEncodingMacFarsi", 49);
    if (vh->encodingsBitmap & ((u_int64_t)1 << 48)) _PrintSubattributeString("%s (%u)", "kTextEncodingMacUkrainian", 48);

    _PrintHeaderString("Finder Info");
    
    // The HFS+ documentation says this is an 8-member array of UI32s.
    // It's a 32-member array of UI8s according to their struct.
    
    _PrintUI            ("bootDirID",           OSReadBigInt32(&vh->finderInfo, 0));
    _PrintUI            ("bootParentID",        OSReadBigInt32(&vh->finderInfo, 4));
    _PrintUI            ("openWindowDirID",     OSReadBigInt32(&vh->finderInfo, 8));
    _PrintUI            ("os9DirID",            OSReadBigInt32(&vh->finderInfo, 12));
    {
        u_int32_t reserved = OSReadBigInt32(&vh->finderInfo, 16);
        _PrintRawAttribute("reserved", &reserved, sizeof(reserved), 2);
    }
    _PrintUI            ("osXDirID",            OSReadBigInt32(&vh->finderInfo, 20));
    _PrintUIHex         ("volID",               OSReadBigInt64(&vh->finderInfo, 24));
    
    if (vh->attributes & kHFSVolumeJournaledMask) {
        if (vh->journalInfoBlock) {
            JournalInfoBlock block = {0};
            bool success = hfs_get_JournalInfoBlock(&block, &volume);
            if (!success) critical("Could not get the journal info block!");
            PrintJournalInfoBlock(&block);
        } else {
            warning("Consistency error: volume attributes indicate it is journaled but the journal info block is empty!");
        }
    }

    _PrintHeaderString("Allocation Bitmap File");
    PrintHFSPlusForkData(&vh->allocationFile, kHFSAllocationFileID, HFSDataForkType);
    
    _PrintHeaderString("Extents Overflow File");
    PrintHFSPlusForkData(&vh->extentsFile, kHFSExtentsFileID, HFSDataForkType);
    
    _PrintHeaderString("Catalog File");
    PrintHFSPlusForkData(&vh->catalogFile, kHFSCatalogFileID, HFSDataForkType);
    
    _PrintHeaderString("Attributes File");
    PrintHFSPlusForkData(&vh->attributesFile, kHFSAttributesFileID, HFSDataForkType);
    
    _PrintHeaderString("Startup File");
    PrintHFSPlusForkData(&vh->startupFile, kHFSStartupFileID, HFSDataForkType);
}

void PrintExtentList(const ExtentList* list, u_int32_t totalBlocks)
{
    _PrintAttributeString("extents", "%12s %12s %12s", "startBlock", "blockCount", "% of file");
    int usedExtents = 0;
    int catalogBlocks = 0;
    float total = 0;
    
    Extent *e = NULL;
    
    TAILQ_FOREACH(e, list, extents) {
        usedExtents++;
        catalogBlocks += e->blockCount;
        
        if (totalBlocks) {
            float pct = (float)e->blockCount/(float)totalBlocks*100.;
            total += pct;
            _PrintString("", "%12u %12u %12.2f", e->startBlock, e->blockCount, pct);
        } else {
            _PrintString("", "%12u %12u %12s", e->startBlock, e->blockCount, "?");
        }
    }
    char* sumLine = repchar('-', 38);
    _PrintString("", sumLine);
    free(sumLine);
    if (totalBlocks) {
        _PrintString("", "%4d extents %12d %12.2f", usedExtents, catalogBlocks, total);
    } else {
        _PrintString("", "%12s %12d %12s", "", catalogBlocks, "?");
    }
    
//    _PrintString("", "%d allocation blocks in %d extents total.", catalogBlocks, usedExtents);
    _PrintString("", "%0.2f blocks per extent on average.", ( (float)catalogBlocks / (float)usedExtents) );
}

void PrintForkExtentsSummary(const HFSFork* fork)
{
    info("Printing extents summary");
    
    PrintExtentList(fork->extents, fork->forkData.totalBlocks);
}

void PrintHFSPlusForkData(const HFSPlusForkData *fork, u_int32_t cnid, u_int8_t forktype)
{
    if (forktype == HFSDataForkType) {
        _PrintAttributeString("fork", "data");
    } else if (forktype == HFSResourceForkType) {
        _PrintAttributeString("fork", "resource");
    }
    if (fork->logicalSize == 0) {
        _PrintAttributeString("logicalSize", "(empty)");
        return;
    }
    
    PrintDataLength (fork, logicalSize);
    PrintDataLength (fork, clumpSize);
    PrintHFSBlocks  (fork, totalBlocks);
    
    if (fork->totalBlocks) {
        HFSFork hfsfork = hfsfork_make(&volume, *fork, forktype, cnid);
        PrintForkExtentsSummary(&hfsfork);
        hfsfork_free(&hfsfork);
    }
}

void PrintBTNodeDescriptor(const BTNodeDescriptor *node)
{
    _PrintHeaderString("B-Tree Node Descriptor");
    PrintUI(node, fLink);
    PrintUI(node, bLink);
    {
        u_int64_t attributes = node->kind;
        int att_values[4] = {
            kBTLeafNode,
            kBTIndexNode,
            kBTHeaderNode,
            kBTMapNode
        };
        char* att_names[4] = {
            "kBTLeafNode",
            "kBTIndexNode",
            "kBTHeaderNode",
            "kBTMapNode"
        };
        for (int i = 0; i < 4; i++) {
            if (attributes == att_values[i])
                _PrintAttributeString("kind", "%s (%u)", att_names[i], att_values[i]);
        }
    }
    PrintUI(node, height);
    PrintUI(node, numRecords);
    PrintUI(node, reserved);
}

void PrintBTHeaderRecord(const BTHeaderRec *hr)
{
    _PrintHeaderString("B-Tree Header Record");
    PrintUI         (hr, treeDepth);
	PrintUI         (hr, rootNode);
	PrintUI         (hr, leafRecords);
	PrintUI         (hr, firstLeafNode);
	PrintUI         (hr, lastLeafNode);
	PrintDataLength (hr, nodeSize);
	PrintUI         (hr, maxKeyLength);
	PrintUI         (hr, totalNodes);
	PrintUI         (hr, freeNodes);
	PrintUI         (hr, reserved1);
	PrintDataLength (hr, clumpSize);
    {
        u_int64_t attributes = hr->btreeType;
        int att_values[3] = {
            0,
            128,
            255
        };
        char* att_names[3] = {
            "kHFSBTreeType",
            "kUserBTreeType",
            "kReservedBTreeType"
        };
        for (int i = 0; i < 3; i++) {
            if (attributes == att_values[i])
                _PrintAttributeString("btreeType", "%s (%u)", att_names[i], att_values[i]);
        }
    }
	PrintHexDump        (hr,    keyCompareType);
	PrintBinaryDump     (hr,    attributes);
    
    {
        u_int64_t attributes = hr->attributes;
        int att_values[3] = {
            kBTBadCloseMask,
            kBTBigKeysMask,
            kBTVariableIndexKeysMask
        };
        char* att_names[3] = {
            "kBTBadCloseMask",
            "kBTBigKeysMask",
            "kBTVariableIndexKeysMask"
        };
        for (int i = 0; i < 3; i++) {
            if (attributes & att_values[i])
                _PrintSubattributeString("%s (%u)", att_names[i], att_values[i]);
        }
    }
    
    //	printf("Reserved3:        %u\n",    hr->reserved3[16]);
}

void PrintHFSPlusBSDInfo(const HFSPlusBSDInfo *record)
{
    PrintUI(record, ownerID);
    PrintUI(record, groupID);
    
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
    
    PrintUIOct(record, adminFlags);
    
    for (int i = 0; i < 10; i++) {
        u_int8_t flag = record->adminFlags;
        if (flag & flagMasks[i]) {
            _PrintSubattributeString("%05o %s", flagMasks[i], flagNames[i]);
        }
    }
    
    PrintUIOct(record, ownerFlags);
    
    for (int i = 0; i < 10; i++) {
        u_int8_t flag = record->ownerFlags;
        if (flag & flagMasks[i]) {
            _PrintSubattributeString("%05o %s", flagMasks[i], flagNames[i]);
        }
    }
        
    u_int16_t mode = record->fileMode;
    char modeString[11] = "";
    
    if (S_ISBLK(mode)) {
        modeString[0] = 'b';
    }
    if (S_ISCHR(mode)) {
        modeString[0] = 'c';
    }
    if (S_ISDIR(mode)) {
        modeString[0] = 'd';
    }
    if (S_ISFIFO(mode)) {
        modeString[0] = 'p';
    }
    if (S_ISREG(mode)) {
        modeString[0] = '-';
    }
    if (S_ISLNK(mode)) {
        modeString[0] = 'l';
    }
    if (S_ISSOCK(mode)) {
        modeString[0] = 's';
    }
    if (S_ISWHT(mode)) {
        modeString[0] = 'x';
    }
    
    modeString[1] = (mode & S_IRUSR ? 'r' : '-');
    modeString[2] = (mode & S_IWUSR ? 'w' : '-');
    
    modeString[4] = (mode & S_IRGRP ? 'r' : '-');
    modeString[5] = (mode & S_IWGRP ? 'w' : '-');
    
    modeString[7] = (mode & S_IROTH ? 'r' : '-');
    modeString[8] = (mode & S_IWOTH ? 'w' : '-');
    
    if ((mode & S_ISUID) && !(mode & S_IXUSR)) {
        modeString[3] = 'S';
    } else if ((mode & S_ISUID) && (mode & S_IXUSR)) {
        modeString[3] = 's';
    } else if (!(mode & S_ISUID) && (mode & S_IXUSR)) {
        modeString[3] = 'x';
    } else {
        modeString[3] = '-';
    }

    if ((mode & S_ISGID) && !(mode & S_IXGRP)) {
        modeString[6] = 'S';
    } else if ((mode & S_ISGID) && (mode & S_IXGRP)) {
        modeString[6] = 's';
    } else if (!(mode & S_ISGID) && (mode & S_IXGRP)) {
        modeString[6] = 'x';
    } else {
        modeString[6] = '-';
    }

    if ((mode & S_ISTXT) && !(mode & S_IXOTH)) {
        modeString[9] = 'T';
    } else if ((mode & S_ISTXT) && (mode & S_IXOTH)) {
        modeString[9] = 't';
    } else if (!(mode & S_ISTXT) && (mode & S_IXOTH)) {
        modeString[9] = 'x';
    } else {
        modeString[9] = '-';
    }
    
    modeString[10] = '\0';
    
    _PrintAttributeString("modeString", modeString);
    
    PrintUIOct(record, fileMode);
    
    PrintConstOctIfEqual(mode & S_IFMT, S_IFBLK);
    PrintConstOctIfEqual(mode & S_IFMT, S_IFCHR);
    PrintConstOctIfEqual(mode & S_IFMT, S_IFDIR);
    PrintConstOctIfEqual(mode & S_IFMT, S_IFIFO);
    PrintConstOctIfEqual(mode & S_IFMT, S_IFREG);
    PrintConstOctIfEqual(mode & S_IFMT, S_IFLNK);
    PrintConstOctIfEqual(mode & S_IFMT, S_IFSOCK);
    PrintConstOctIfEqual(mode & S_IFMT, S_IFWHT);
    
    PrintUIOctFlagIfMatch(mode, S_ISUID);
    PrintUIOctFlagIfMatch(mode, S_ISGID);
    PrintUIOctFlagIfMatch(mode, S_ISTXT);
    
    PrintUIOctFlagIfMatch(mode, S_IRUSR);
    PrintUIOctFlagIfMatch(mode, S_IWUSR);
    PrintUIOctFlagIfMatch(mode, S_IXUSR);
    
    PrintUIOctFlagIfMatch(mode, S_IRGRP);
    PrintUIOctFlagIfMatch(mode, S_IWGRP);
    PrintUIOctFlagIfMatch(mode, S_IXGRP);
    
    PrintUIOctFlagIfMatch(mode, S_IROTH);
    PrintUIOctFlagIfMatch(mode, S_IWOTH);
    PrintUIOctFlagIfMatch(mode, S_IXOTH);
    

    PrintUI(record, special.linkCount);
}

void PrintFndrFileInfo(const FndrFileInfo *record)
{
    PrintHFSChar    (record, fdType);
    PrintHFSChar    (record, fdCreator);
    PrintBinaryDump     (record, fdFlags);
    PrintFinderFlags(record->fdFlags);
    _PrintAttributeString("fdLocation", "(%u, %u)", record->fdLocation.v, record->fdLocation.h);
    PrintUI         (record, opaque);
}

void PrintFndrDirInfo(const FndrDirInfo *record)
{
    _PrintAttributeString("frRect", "(%u, %u, %u, %u)", record->frRect.top, record->frRect.left, record->frRect.bottom, record->frRect.right);
    PrintBinaryDump         (record, frFlags);
    PrintFinderFlags    (record->frFlags);
    _PrintAttributeString("frLocation", "(%u, %u)", record->frLocation.v, record->frLocation.h);
    PrintUI             (record, opaque);
}

void PrintFinderFlags(u_int16_t record)
{
    u_int16_t kIsOnDesk             = 0x0001;     /* Files and folders (System 6) */
    u_int16_t kRequireSwitchLaunch  = 0x0020;     /* Old */
    u_int16_t kColor                = 0x000E;     /* Files and folders */
    u_int16_t kIsShared             = 0x0040;     /* Files only (Applications only) If */
    u_int16_t kHasNoINITs           = 0x0080;     /* Files only (Extensions/Control */
    u_int16_t kHasBeenInited        = 0x0100;     /* Files only.  Clear if the file */
    u_int16_t kHasCustomIcon        = 0x0400;     /* Files and folders */
    u_int16_t kIsStationery         = 0x0800;     /* Files only */
    u_int16_t kNameLocked           = 0x1000;     /* Files and folders */
    u_int16_t kHasBundle            = 0x2000;     /* Files only */
    u_int16_t kIsInvisible          = 0x4000;     /* Files and folders */
    u_int16_t kIsAlias              = 0x8000;     /* Files only */
    
    PrintUIFlagIfMatch(record, kIsOnDesk);
    PrintUIFlagIfMatch(record, kRequireSwitchLaunch);
    PrintUIFlagIfMatch(record, kColor);
    PrintUIFlagIfMatch(record, kIsShared);
    PrintUIFlagIfMatch(record, kHasNoINITs);
    PrintUIFlagIfMatch(record, kHasBeenInited);
    PrintUIFlagIfMatch(record, kHasCustomIcon);
    PrintUIFlagIfMatch(record, kIsStationery);
    PrintUIFlagIfMatch(record, kNameLocked);
    PrintUIFlagIfMatch(record, kHasBundle);
    PrintUIFlagIfMatch(record, kIsInvisible);
    PrintUIFlagIfMatch(record, kIsAlias);
}

void PrintFndrOpaqueInfo(const FndrOpaqueInfo *record)
{
    // It's opaque. Provided for completeness, and just incase some properties are discovered.
}

void PrintHFSPlusCatalogFolder(const HFSPlusCatalogFolder *record)
{
    _PrintAttributeString("recordType", "kHFSPlusFolderRecord");
    PrintBinaryDump(record, flags);
    if (record->flags & kHFSFileLockedMask)
        _PrintSubattributeString("kHFSFileLockedMask");
    if (record->flags & kHFSThreadExistsMask)
        _PrintSubattributeString("kHFSThreadExistsMask");
    if (record->flags & kHFSHasAttributesMask)
        _PrintSubattributeString("kHFSHasAttributesMask");
    if (record->flags & kHFSHasSecurityMask)
        _PrintSubattributeString("kHFSHasSecurityMask");
    if (record->flags & kHFSHasFolderCountMask)
        _PrintSubattributeString("kHFSHasFolderCountMask");
    if (record->flags & kHFSHasLinkChainMask)
        _PrintSubattributeString("kHFSHasLinkChainMask");
    if (record->flags & kHFSHasChildLinkMask)
        _PrintSubattributeString("kHFSHasChildLinkMask");
    if (record->flags & kHFSHasDateAddedMask)
        _PrintSubattributeString("kHFSHasDateAddedMask");
    PrintUI(record, valence);
    PrintUI(record, folderID);
    PrintHFSTimestamp(record, createDate);
    PrintHFSTimestamp(record, contentModDate);
    PrintHFSTimestamp(record, attributeModDate);
    PrintHFSTimestamp(record, accessDate);
    PrintHFSTimestamp(record, backupDate);
    PrintHFSPlusBSDInfo(&record->bsdInfo);
    PrintFndrDirInfo(&record->userInfo);
    PrintFndrOpaqueInfo(&record->finderInfo);
    PrintUI(record, textEncoding);
    PrintUI(record, folderCount);
}

void PrintHFSPlusCatalogFile(const HFSPlusCatalogFile *record)
{
    _PrintAttributeString("recordType", "kHFSPlusFileRecord");
    PrintBinaryDump(record, flags);
    if (record->flags & kHFSFileLockedMask)
        _PrintSubattributeString("kHFSFileLockedMask");
    if (record->flags & kHFSThreadExistsMask)
        _PrintSubattributeString("kHFSThreadExistsMask");
    if (record->flags & kHFSHasAttributesMask)
        _PrintSubattributeString("kHFSHasAttributesMask");
    if (record->flags & kHFSHasSecurityMask)
        _PrintSubattributeString("kHFSHasSecurityMask");
    if (record->flags & kHFSHasFolderCountMask)
        _PrintSubattributeString("kHFSHasFolderCountMask");
    if (record->flags & kHFSHasLinkChainMask)
        _PrintSubattributeString("kHFSHasLinkChainMask");
    if (record->flags & kHFSHasChildLinkMask)
        _PrintSubattributeString("kHFSHasChildLinkMask");
    if (record->flags & kHFSHasDateAddedMask)
        _PrintSubattributeString("kHFSHasDateAddedMask");
    PrintUI                 (record, reserved1);
    PrintUI                 (record, fileID);
    PrintHFSTimestamp       (record, createDate);
    PrintHFSTimestamp       (record, contentModDate);
    PrintHFSTimestamp       (record, attributeModDate);
    PrintHFSTimestamp       (record, accessDate);
    PrintHFSTimestamp       (record, backupDate);
    PrintHFSPlusBSDInfo     (&record->bsdInfo);
    PrintFndrFileInfo       (&record->userInfo);
    PrintFndrOpaqueInfo     (&record->finderInfo);
    PrintUI                 (record, textEncoding);
    PrintUI                 (record, reserved2);
    PrintHFSPlusForkData    (&record->dataFork, kHFSCatalogFileID, HFSDataForkType);
    PrintHFSPlusForkData    (&record->resourceFork, kHFSCatalogFileID, HFSResourceForkType);
}

void PrintHFSPlusFolderThreadRecord(const HFSPlusCatalogThread *record)
{
    _PrintAttributeString       ("recordType", "kHFSPlusFolderThreadRecord");
    PrintHFSPlusCatalogThread   (record);
}

void PrintHFSPlusFileThreadRecord(const HFSPlusCatalogThread *record)
{
    _PrintAttributeString       ("recordType", "kHFSPlusFileThreadRecord");
    PrintHFSPlusCatalogThread   (record);
}

void PrintHFSPlusCatalogThread(const HFSPlusCatalogThread *record)
{
    PrintUI             (record, reserved);
    PrintUI             (record, parentID);
    _PrintHFSUniStr255  ("nodeName", &record->nodeName);
}

void PrintJournalInfoBlock(const JournalInfoBlock *record)
{
    /*
     struct JournalInfoBlock {
     u_int32_t       flags;
     u_int32_t       device_signature[8];  // signature used to locate our device.
     u_int64_t       offset;               // byte offset to the journal on the device
     u_int64_t       size;                 // size in bytes of the journal
     uuid_string_t   ext_jnl_uuid;
     char            machine_serial_num[48];
     char    	reserved[JIB_RESERVED_SIZE];
     } __attribute__((aligned(2), packed));
     typedef struct JournalInfoBlock JournalInfoBlock;
     */
    
    _PrintHeaderString("Journal Info Block");
    PrintBinaryDump         (record, flags);
    PrintUIFlagIfMatch      (record->flags, kJIJournalInFSMask);
    PrintUIFlagIfMatch      (record->flags, kJIJournalOnOtherDeviceMask);
    PrintUIFlagIfMatch      (record->flags, kJIJournalNeedInitMask);
    _PrintRawAttribute      ("device_signature", &record->device_signature[0], 32, 16);
    PrintDataLength         (record, offset);
    PrintDataLength         (record, size);

    char* str = malloc(sizeof(uuid_string_t));
    str[sizeof(uuid_string_t) - 1] = '\0';
    memcpy(str, &record->ext_jnl_uuid[0], sizeof(uuid_string_t));
    _PrintAttributeString("ext_jnl_uuid", str);
    free(str); str=NULL;

    str = malloc(49);
    str[48] = '\0';
    memcpy(str, &record->machine_serial_num[0], 48);
    _PrintAttributeString("machine_serial_num", str);
    free(str); str=NULL;
    
    // (u_int32_t) reserved[32]
}

void PrintJournalHeader(const journal_header *record)
{
    
}

void PrintVolumeSummary(const VolumeSummary *summary)
{
    _PrintHeaderString  ("Volume Summary");
    PrintUI             (summary, nodeCount);
    PrintUI             (summary, recordCount);
    PrintUI             (summary, fileCount);
    PrintUI             (summary, folderCount);
    PrintUI             (summary, aliasCount);
    PrintUI             (summary, hardLinkFileCount);
    PrintUI             (summary, hardLinkFolderCount);
    PrintUI             (summary, symbolicLinkCount);
    PrintUI             (summary, invisibleFileCount);
    PrintUI             (summary, emptyFileCount);
    PrintUI             (summary, emptyDirectoryCount);
    
    _PrintHeaderString  ("Data Fork");
    PrintForkSummary    (&summary->dataFork);
    
    _PrintHeaderString  ("Resource Fork");
    PrintForkSummary    (&summary->resourceFork);
    
    _PrintHeaderString  ("Largest Files");
    print("# %10s %10s", "Size", "CNID");
    for (int i = 9; i > 0; i--) {
        char* size = sizeString(summary->largestFiles[i].measure);
        print("%d %10s %10u", 10-i, size, summary->largestFiles[i].cnid);
        free(size);
    }
}

void PrintForkSummary(const ForkSummary *summary)
{
    PrintUI             (summary, count);
    PrintUI             (summary, fragmentedCount);
    PrintHFSBlocks      (summary, blockCount);
    PrintDataLength     (summary, logicalSpace);
    PrintUI             (summary, extentRecords);
    PrintUI             (summary, extentDescriptors);
    PrintUI             (summary, overflowExtentRecords);
    PrintUI             (summary, overflowExtentDescriptors);
}

#pragma mark Structure Visualization Functions

void VisualizeHFSPlusExtentKey(const HFSPlusExtentKey *record, const char* label, bool oneLine)
{
    if (oneLine) {
        printf("%s: %s:%-6u; %s:%-4u; %s:%-3u; %s:%-10u; %s:%-10u\n",
               label,
               "length",
               record->keyLength,
               "fork",
               record->forkType,
               "pad",
               record->pad,
               "fileID",
               record->fileID,
               "startBlock",
               record->startBlock
               );
    } else {
        printf("\n  %s\n", label);
        printf("  +-----------------------------------------------+\n");
        printf("  | %-6s | %-4s | %-3s | %-10s | %-10s |\n",
               "length",
               "fork",
               "pad",
               "fileID",
               "startBlock");
        printf("  | %-6u | %-4u | %-3u | %-10u | %-10u |\n",
               record->keyLength,
               record->forkType,
               record->pad,
               record->fileID,
               record->startBlock);
        printf("  +-----------------------------------------------+\n");
    }
}

void VisualizeHFSPlusCatalogKey(const HFSPlusCatalogKey *record, const char* label, bool oneLine)
{
    wchar_t* name   = hfsuctowcs(&record->nodeName);
    if (oneLine) {
        printf("%s: %s:%-6u; %s:%-10u; %s:%-6u; %s:%-50ls\n",
               label,
               "length",
               record->keyLength,
               "parentID",
               record->parentID,
               "length",
               record->nodeName.length,
               "nodeName",
               name
               );
    } else {
        char* names     = " | %-6s | %-10s | %-6s | %-50s |\n";
        char* format    = " | %-6u | %-10u | %-6u | %-50ls |\n";
        
        char* line_f    =  " +%s+\n";
        char* dashes    = repchar('-', 83);
        
        printf("\n  %s\n", label);
        printf(line_f, dashes);
        printf(names, "length", "parentID", "length", "nodeName");
        printf(format, record->keyLength, record->parentID, record->nodeName.length, name);
        printf(line_f, dashes);
    }
    
    free(name);
}

void VisualizeHFSBTreeNodeRecord(const HFSBTreeNodeRecord* record, const char* label)
{
    char* names     = " | %-9s | %-6s | %-6s | %-10s | %-12s |\n";
    char* format    = " | %-9u | %-6u | %-6u | %-10u | %-12u |\n";
    
    char* line_f    =  " +%s+\n";
    char* dashes    = repchar('-', 57);
    
    printf("\n  %s\n", label);
    printf(line_f, dashes);
    printf(names, "record_id", "offset", "length", "key_length", "value_length");
    printf(format, record->recordID, record->offset, record->length, record->keyLength, record->valueLength);
    printf(line_f, dashes);
    printf("Key data:\n");
    VisualizeData(record->key, record->keyLength);
    printf("Value data:\n");
    VisualizeData(record->value, record->valueLength);
}

void VisualizeData(const char* data, size_t length)
{
    memdump(data, length, 16, 4, 4);
}


#pragma mark Node Print Functions

void PrintTreeNode(const HFSBTree *tree, u_int32_t nodeID)
{
    debug("PrintTreeNode");
    
    HFSBTreeNode node;
    int8_t result = hfs_btree_get_node(&node, tree, nodeID);
    if (result == -1) {
        error("node %u is invalid or out of range.", nodeID);
        return;
    }
    PrintNode(&node);
    hfs_btree_free_node(&node);
}

void PrintNode(const HFSBTreeNode* node)
{
    debug("PrintNode");
    
    _PrintHeaderString      ("Node %u (offset %llu; length: %zu)", node->nodeNumber, node->nodeOffset, node->buffer.size);
    PrintBTNodeDescriptor   (&node->nodeDescriptor);
    
    for (int recordNumber = 0; recordNumber < node->nodeDescriptor.numRecords; recordNumber++) {
        PrintNodeRecord     (node, recordNumber);
    }
}

void PrintNodeRecord(const HFSBTreeNode* node, int recordNumber)
{
    debug("PrintNodeRecord");
    
    if(recordNumber >= node->nodeDescriptor.numRecords) return;
    
    const HFSBTreeNodeRecord *record = &node->records[recordNumber];
    
    u_int16_t offset = record->offset;
    if (offset > node->bTree.headerRecord.nodeSize) {
        error("Invalid record offset: points beyond end of node: %u", offset);
        return;
    }
    
    if (record->length == 0) {
        error("Invalid record: no data found.");
        return;
    }
    
    _PrintHeaderString("  Record ID %u (%u/%u) (offset %u; length: %zd) (Node %d)",
                       recordNumber,
                       recordNumber + 1,
                       node->nodeDescriptor.numRecords,
                       offset,
                       record->length,
                       node->nodeNumber
                       );
    
    switch (node->nodeDescriptor.kind) {
        case kBTHeaderNode:
        {
            switch (recordNumber) {
                case 0:
                {
                    _PrintAttributeString("recordType", "BTHeaderRec");
                    BTHeaderRec header = *( (BTHeaderRec*) record->record );
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
            switch (node->bTree.fork.cnid) {
                case kHFSCatalogFileID:
                {
                    HFSPlusCatalogKey *keyStruct = (HFSPlusCatalogKey*) record->key;
                    VisualizeHFSPlusCatalogKey(keyStruct, "Catalog Key", 0);
                    
                    if (record->keyLength < kHFSPlusCatalogKeyMinimumLength || record->keyLength > kHFSPlusCatalogKeyMaximumLength)
                        goto INVALID_KEY;
                    
                    break;
                };
                    
                case kHFSExtentsFileID:
                {
                    HFSPlusExtentKey keyStruct = *( (HFSPlusExtentKey*) record->key );
                    VisualizeHFSPlusExtentKey(&keyStruct, "Extent Key", 0);
                    
                    if (record->keyLength != kHFSPlusExtentKeyMaximumLength)
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
                    u_int32_t *pointer = (u_int32_t*) record->value;
                    _PrintUI("nextNodeID", *pointer);
                    break;
                }
                    
                case kBTLeafNode:
                {
                    switch (node->bTree.fork.cnid) {
                        case kHFSCatalogFileID:
                        {
                            u_int16_t type =  *(u_int16_t*)(record->value);
                            
                            switch (type) {
                                case kHFSPlusFileRecord:
                                {
                                    PrintHFSPlusCatalogFile((HFSPlusCatalogFile*)record->value);
                                    break;
                                }
                                    
                                case kHFSPlusFolderRecord:
                                {
                                    PrintHFSPlusCatalogFolder((HFSPlusCatalogFolder*)record->value);
                                    break;
                                }
                                    
                                case kHFSPlusFileThreadRecord:
                                {
                                    PrintHFSPlusFileThreadRecord((HFSPlusCatalogThread*)record->value);
                                    break;
                                }
                                    
                                case kHFSPlusFolderThreadRecord:
                                {
                                    PrintHFSPlusFolderThreadRecord((HFSPlusCatalogThread*)record->value);
                                    break;
                                }
                                    
                                default:
                                {
                                    _PrintAttributeString("recordType", "%u (invalid)", type);
                                    break;
                                }
                            }
                            break;
                        }
                        case kHFSExtentsFileID:
                        {
                            HFSPlusExtentRecord *extentRecord = (HFSPlusExtentRecord*)record->value;
                            ExtentList *list = extentlist_alloc();
                            extentlist_add_record(list, *extentRecord);
                            PrintExtentList(list, 0);
                            extentlist_free(list);
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
                        VisualizeData(record->record, record->length);
                    }
                }
            }
            
            break;
        }
    }
}
