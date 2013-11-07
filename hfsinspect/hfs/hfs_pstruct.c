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
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>

#include "hfs_pstruct.h"
#include "hfs.h"
#include "vfs_journal.h"

static HFSVolume volume = {0};
void set_hfs_volume(HFSVolume *v) { volume = *v; }


#pragma mark Line Print Functions

void PrintString(const char* label, const char* value_format, ...)
{
    va_list argp;
    va_start(argp, value_format);
    char* str;
    vasprintf(&str, value_format, argp);
    printf("  %-23s  %s\n", label, str);
    FREE_BUFFER(str);
    va_end(argp);
}

void PrintHeaderString(const char* value_format, ...)
{
    va_list argp;
    va_start(argp, value_format);
    char* str;
    vasprintf(&str, value_format, argp);
    printf("\n# %s\n", str);
    FREE_BUFFER(str);
    va_end(argp);
}

void PrintAttributeString(const char* label, const char* value_format, ...)
{
    va_list argp;
    va_start(argp, value_format);
    char* str;
    vasprintf(&str, value_format, argp);
    printf("  %-23s= %s\n", label, str);
    FREE_BUFFER(str);
    va_end(argp);
}

void PrintSubattributeString(const char* str, ...)
{
    va_list argp;
    va_start(argp, str);
    char* s;
    vasprintf(&s, str, argp);
    printf("  %-23s. %s\n", "", s);
    FREE_BUFFER(s);
    va_end(argp);
}

#pragma mark Value Print Functions

void _PrintCatalogName(char* label, hfs_node_id cnid)
{
    wchar_t* name;
    if (cnid != 0)
        name = hfs_catalog_get_cnid_name(&volume, cnid);
    else
        name = L"";
    
    PrintAttributeString(label, "%d (%ls)", cnid, name);
    
    FREE_BUFFER(name);
}

void _PrintHFSBlocks(const char *label, u_int64_t blocks)
{
    size_t displaySize = blocks * volume.block_size;
    char* sizeLabel = sizeString(displaySize, false);
    PrintAttributeString(label, "%s (%d blocks)", sizeLabel, blocks);
    FREE_BUFFER(sizeLabel);
}

void _PrintDataLength(const char *label, u_int64_t size)
{
    size_t displaySize = size;
    char* sizeLabel;
    char* metricLabel;
    if (size > 1024*1024) {
        sizeLabel = sizeString(displaySize, false);
        metricLabel = sizeString(displaySize, true);
        PrintAttributeString(label, "%s/%s (%lu bytes)", sizeLabel, metricLabel, size);
        FREE_BUFFER(sizeLabel); FREE_BUFFER(metricLabel);
    } else if (size > 1024) {
        sizeLabel = sizeString(displaySize, false);
        PrintAttributeString(label, "%s (%lu bytes)", sizeLabel, size);
        FREE_BUFFER(sizeLabel);
    } else {
        PrintAttributeString(label, "%lu bytes", size);
    }
}

void _PrintRawAttribute(const char* label, const void* map, size_t size, char base)
{
    unsigned segmentLength = 32;
    char* str = memstr(map, size, base);
    size_t len = strlen(str);
    
    for (int i = 0; i < len; i += segmentLength) {
        char* segment = strndup(&str[i], MIN(segmentLength, len - i));
        PrintAttributeString(label, "%s%s", (base==16?"0x":""), segment);
        FREE_BUFFER(segment);
        if (i == 0) label = "";
    }
    
    FREE_BUFFER(str);
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
    
    char* buf;
    INIT_STRING(buf, 50);
    strftime(buf, 50, "%c %Z\0", t);
    PrintAttributeString(label, buf);
    FREE_BUFFER(buf);
}

void _PrintHFSChar(const char* label, const void* i, size_t nbytes)
{
    // Iterates over the input (i) from left to right, copying bytes
    // in reverse order as chars in a string. Reverse order because we
    // presume the host is little-endian.
    char* str;
    INIT_STRING(str, nbytes + 1);
    for (size_t byte = 0; byte < nbytes; byte++) {
        str[byte] = ((char*)i)[(nbytes-1) - byte];
    }
    str[nbytes] = '\0';
    
    // Grab the hex representation of the input.
    char* hex = memstr(i, nbytes, 16);
    
    PrintAttributeString(label, "0x%s (%s)", hex, str);
    
    // Cleanup
    FREE_BUFFER(hex);
    FREE_BUFFER(str);
}

void _PrintHFSUniStr255(const char* label, const HFSUniStr255 *record)
{
    wchar_t *wide = hfsuctowcs(record);
    wprintf(L"  %-23s= \"%ls\" (%u)\n", label, wide, record->length);
    FREE_BUFFER(wide);
}


#pragma mark Structure Print Functions

void PrintVolumeInfo(const HFSVolume* hfs)
{
    if (hfs->vh.signature == kHFSPlusSigWord)
        PrintHeaderString("HFS+ Volume Format (v%d)", hfs->vh.version);
    else if (hfs->vh.signature == kHFSXSigWord)
        PrintHeaderString("HFSX Volume Format (v%d)", hfs->vh.version);
    else
        PrintHeaderString("Unknown Volume Format"); // Curious.
    
    wchar_t* volumeName = hfs_catalog_get_cnid_name(hfs, kHFSRootFolderID);
    PrintAttributeString("volume name", "%ls", volumeName);
    FREE_BUFFER(volumeName);
    
    HFSBTree catalog = hfs_get_catalog_btree(hfs);
    
    if (hfs->vh.signature == kHFSXSigWord && catalog.headerRecord.keyCompareType == kHFSBinaryCompare) {
        PrintAttributeString("case sensitivity", "case sensitive");
    } else {
        PrintAttributeString("case sensitivity", "case insensitive");
    }
    
    HFSPlusVolumeFinderInfo* finderInfo = (HFSPlusVolumeFinderInfo*)&hfs->vh.finderInfo;
    if (finderInfo->bootDirID || finderInfo->bootParentID || finderInfo->os9DirID || finderInfo->osXDirID) {
        PrintAttributeString("bootable", "yes");
    } else {
        PrintAttributeString("bootable", "no");
    }
    
}

void PrintHFSMasterDirectoryBlock(const HFSMasterDirectoryBlock* vcb)
{
    PrintHeaderString("HFS Master Directory Block");
    
    PrintHFSChar(vcb, drSigWord);
    PrintHFSTimestamp(vcb, drCrDate);
    PrintHFSTimestamp(vcb, drLsMod);
    PrintRawAttribute(vcb, drAtrb, 2);
    PrintUI(vcb, drNmFls);
    PrintUI(vcb, drVBMSt);
    PrintUI(vcb, drAllocPtr);
    PrintUI(vcb, drNmAlBlks);
    PrintDataLength(vcb, drAlBlkSiz);
    PrintDataLength(vcb, drClpSiz);
    PrintUI(vcb, drAlBlSt);
    PrintUI(vcb, drNxtCNID);
    PrintUI(vcb, drFreeBks);
    
    char name[32]; memset(name, '\0', 32); memcpy(name, vcb->drVN, 31);
    PrintAttributeString("drVN", "%s", name);
    
    PrintHFSTimestamp(vcb, drVolBkUp);
    PrintUI(vcb, drVSeqNum);
    PrintUI(vcb, drWrCnt);
    PrintDataLength(vcb, drXTClpSiz);
    PrintDataLength(vcb, drCTClpSiz);
    PrintUI(vcb, drNmRtDirs);
    PrintUI(vcb, drFilCnt);
    PrintUI(vcb, drDirCnt);
    PrintUI(vcb, drFndrInfo[0]);
    PrintUI(vcb, drFndrInfo[1]);
    PrintUI(vcb, drFndrInfo[2]);
    PrintUI(vcb, drFndrInfo[3]);
    PrintUI(vcb, drFndrInfo[4]);
    PrintUI(vcb, drFndrInfo[5]);
    PrintUI(vcb, drFndrInfo[6]);
    PrintUI(vcb, drFndrInfo[7]);
    PrintHFSChar(vcb, drEmbedSigWord);
    PrintUI(vcb, drEmbedExtent.startBlock);
    PrintUI(vcb, drEmbedExtent.blockCount);
}

void PrintVolumeHeader(const HFSPlusVolumeHeader *vh)
{
    PrintHeaderString("HFS Plus Volume Header");
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
    if (vh->encodingsBitmap & ((u_int64_t)1 << 49)) PrintSubattributeString("%s (%u)", "kTextEncodingMacFarsi", 49);
    if (vh->encodingsBitmap & ((u_int64_t)1 << 48)) PrintSubattributeString("%s (%u)", "kTextEncodingMacUkrainian", 48);

    PrintHeaderString("Finder Info");
    
    HFSPlusVolumeFinderInfo* finderInfo = (HFSPlusVolumeFinderInfo*)&vh->finderInfo;
    
    PrintCatalogName        (finderInfo, bootDirID);
    PrintCatalogName        (finderInfo, bootParentID);
    PrintCatalogName        (finderInfo, openWindowDirID);
    PrintCatalogName        (finderInfo, os9DirID);
    PrintBinaryDump         (finderInfo, reserved);
    PrintCatalogName        (finderInfo, osXDirID);
    PrintHexDump            (finderInfo, volID);
    
    PrintHeaderString("Allocation Bitmap File");
    PrintHFSPlusForkData(&vh->allocationFile, kHFSAllocationFileID, HFSDataForkType);
    
    PrintHeaderString("Extents Overflow File");
    PrintHFSPlusForkData(&vh->extentsFile, kHFSExtentsFileID, HFSDataForkType);
    
    PrintHeaderString("Catalog File");
    PrintHFSPlusForkData(&vh->catalogFile, kHFSCatalogFileID, HFSDataForkType);
    
    PrintHeaderString("Attributes File");
    PrintHFSPlusForkData(&vh->attributesFile, kHFSAttributesFileID, HFSDataForkType);
    
    PrintHeaderString("Startup File");
    PrintHFSPlusForkData(&vh->startupFile, kHFSStartupFileID, HFSDataForkType);
}

void PrintExtentList(const ExtentList* list, u_int32_t totalBlocks)
{
    PrintAttributeString("extents", "%12s %12s %12s", "startBlock", "blockCount", "% of file");
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
            PrintString("", "%12u %12u %12.2f", e->startBlock, e->blockCount, pct);
        } else {
            PrintString("", "%12u %12u %12s", e->startBlock, e->blockCount, "?");
        }
    }
    
    char* sumLine = repchar('-', 38);
    PrintString("", sumLine);
    FREE_BUFFER(sumLine);
    
    if (totalBlocks) {
        PrintString("", "%4d extents %12d %12.2f", usedExtents, catalogBlocks, total);
    } else {
        PrintString("", "%12s %12d %12s", "", catalogBlocks, "?");
    }
    
//    PrintString("", "%d allocation blocks in %d extents total.", catalogBlocks, usedExtents);
    PrintString("", "%0.2f blocks per extent on average.", ( (float)catalogBlocks / (float)usedExtents) );
}

void PrintForkExtentsSummary(const HFSFork* fork)
{
    info("Printing extents summary");
    
    PrintExtentList(fork->extents, fork->totalBlocks);
}

void PrintHFSPlusForkData(const HFSPlusForkData *fork, u_int32_t cnid, u_int8_t forktype)
{
    if (forktype == HFSDataForkType) {
        PrintAttributeString("fork", "data");
    } else if (forktype == HFSResourceForkType) {
        PrintAttributeString("fork", "resource");
    }
    if (fork->logicalSize == 0) {
        PrintAttributeString("logicalSize", "(empty)");
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
    PrintHeaderString("B-Tree Node Descriptor");
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
                PrintAttributeString("kind", "%s (%u)", att_names[i], att_values[i]);
        }
    }
    PrintUI(node, height);
    PrintUI(node, numRecords);
    PrintUI(node, reserved);
}

void PrintBTHeaderRecord(const BTHeaderRec *hr)
{
    PrintHeaderString("B-Tree Header Record");
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
                PrintAttributeString("btreeType", "%s (%u)", att_names[i], att_values[i]);
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
                PrintSubattributeString("%s (%u)", att_names[i], att_values[i]);
        }
    }
    
    //	printf("Reserved3:        %u\n",    hr->reserved3[16]);
}

char* _genModeString(u_int16_t mode)
{
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
    
    return strdup(modeString);
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
            PrintSubattributeString("%05o %s", flagMasks[i], flagNames[i]);
        }
    }
    
    PrintUIOct(record, ownerFlags);
    
    for (int i = 0; i < 10; i++) {
        u_int8_t flag = record->ownerFlags;
        if (flag & flagMasks[i]) {
            PrintSubattributeString("%05o %s", flagMasks[i], flagNames[i]);
        }
    }
        
    u_int16_t mode = record->fileMode;
    
    char* modeString = _genModeString(mode);
    PrintAttributeString("fileMode", modeString);
    FREE_BUFFER(modeString);
    
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
    PrintBinaryDump (record, fdFlags);
    PrintFinderFlags(record->fdFlags);
    PrintAttributeString("fdLocation", "(%u, %u)", record->fdLocation.v, record->fdLocation.h);
    PrintUI         (record, opaque);
}

void PrintFndrDirInfo(const FndrDirInfo *record)
{
    PrintAttributeString("frRect", "(%u, %u, %u, %u)", record->frRect.top, record->frRect.left, record->frRect.bottom, record->frRect.right);
    PrintBinaryDump         (record, frFlags);
    PrintFinderFlags    (record->frFlags);
    PrintAttributeString("frLocation", "(%u, %u)", record->frLocation.v, record->frLocation.h);
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
    PrintAttributeString("recordType", "kHFSPlusFolderRecord");
    PrintBinaryDump(record, flags);
    if (record->flags & kHFSFileLockedMask)
        PrintSubattributeString("kHFSFileLockedMask");
    if (record->flags & kHFSThreadExistsMask)
        PrintSubattributeString("kHFSThreadExistsMask");
    if (record->flags & kHFSHasAttributesMask)
        PrintSubattributeString("kHFSHasAttributesMask");
    if (record->flags & kHFSHasSecurityMask)
        PrintSubattributeString("kHFSHasSecurityMask");
    if (record->flags & kHFSHasFolderCountMask)
        PrintSubattributeString("kHFSHasFolderCountMask");
    if (record->flags & kHFSHasLinkChainMask)
        PrintSubattributeString("kHFSHasLinkChainMask");
    if (record->flags & kHFSHasChildLinkMask)
        PrintSubattributeString("kHFSHasChildLinkMask");
    if (record->flags & kHFSHasDateAddedMask)
        PrintSubattributeString("kHFSHasDateAddedMask");
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
    PrintAttributeString("recordType", "kHFSPlusFileRecord");
    PrintBinaryDump(record, flags);
    if (record->flags & kHFSFileLockedMask)
        PrintSubattributeString("kHFSFileLockedMask");
    if (record->flags & kHFSThreadExistsMask)
        PrintSubattributeString("kHFSThreadExistsMask");
    if (record->flags & kHFSHasAttributesMask)
        PrintSubattributeString("kHFSHasAttributesMask");
    if (record->flags & kHFSHasSecurityMask)
        PrintSubattributeString("kHFSHasSecurityMask");
    if (record->flags & kHFSHasFolderCountMask)
        PrintSubattributeString("kHFSHasFolderCountMask");
    if (record->flags & kHFSHasLinkChainMask)
        PrintSubattributeString("kHFSHasLinkChainMask");
    if (record->flags & kHFSHasChildLinkMask)
        PrintSubattributeString("kHFSHasChildLinkMask");
    if (record->flags & kHFSHasDateAddedMask)
        PrintSubattributeString("kHFSHasDateAddedMask");
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
    PrintAttributeString       ("recordType", "kHFSPlusFolderThreadRecord");
    PrintHFSPlusCatalogThread   (record);
}

void PrintHFSPlusFileThreadRecord(const HFSPlusCatalogThread *record)
{
    PrintAttributeString       ("recordType", "kHFSPlusFileThreadRecord");
    PrintHFSPlusCatalogThread   (record);
}

void PrintHFSPlusCatalogThread(const HFSPlusCatalogThread *record)
{
    PrintUI             (record, reserved);
    PrintUI             (record, parentID);
    _PrintHFSUniStr255  ("nodeName", &record->nodeName);
}

void PrintHFSPlusAttrForkData(const HFSPlusAttrForkData* record)
{
    PrintUI                 (record, recordType);
    PrintHFSPlusForkData    (&record->theFork, 0, 0);
}

void PrintHFSPlusAttrExtents(const HFSPlusAttrExtents* record)
{
    PrintUI                 (record, recordType);
    PrintHFSPlusExtentRecord(&record->extents);
}

void PrintHFSPlusAttrData(const HFSPlusAttrData* record)
{
    PrintUI                 (record, recordType);
    PrintUI                 (record, attrSize);
}

void PrintHFSPlusAttrRecord(const HFSPlusAttrRecord* record)
{
    u_int32_t type = *(u_int32_t*)record;
    switch (type) {
        case kHFSPlusAttrInlineData:
            PrintHFSPlusAttrData((HFSPlusAttrData*)record);
            break;
        case kHFSPlusAttrForkData:
            PrintHFSPlusAttrForkData((HFSPlusAttrForkData*)record);
            break;
        case kHFSPlusAttrExtents:
            PrintHFSPlusAttrExtents((HFSPlusAttrExtents*)record);
            break;
            
        default:
            break;
    }
}

void PrintHFSPlusExtentRecord(const HFSPlusExtentRecord* record)
{
    ExtentList *list = extentlist_alloc();
    extentlist_add_record(list, *record);
    PrintExtentList(list, 0);
    extentlist_free(list);
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
    
    PrintHeaderString("Journal Info Block");
    PrintBinaryDump         (record, flags);
    PrintUIFlagIfMatch      (record->flags, kJIJournalInFSMask);
    PrintUIFlagIfMatch      (record->flags, kJIJournalOnOtherDeviceMask);
    PrintUIFlagIfMatch      (record->flags, kJIJournalNeedInitMask);
    _PrintRawAttribute      ("device_signature", &record->device_signature[0], 32, 16);
    PrintDataLength         (record, offset);
    PrintDataLength         (record, size);

    char* str;
    INIT_STRING(str, sizeof(uuid_string_t));
    str[sizeof(uuid_string_t) - 1] = '\0';
    memcpy(str, &record->ext_jnl_uuid[0], sizeof(uuid_string_t));
    PrintAttributeString("ext_jnl_uuid", str);
    FREE_BUFFER(str);

    INIT_STRING(str, 49);
    memcpy(str, &record->machine_serial_num[0], 48);
    str[48] = '\0';
    PrintAttributeString("machine_serial_num", str);
    FREE_BUFFER(str);
    
    // (u_int32_t) reserved[32]
}

void PrintJournalHeader(const journal_header *record)
{
    
}

void PrintVolumeSummary(const VolumeSummary *summary)
{
    PrintHeaderString  ("Volume Summary");
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
    
    PrintHeaderString  ("Data Fork");
    PrintForkSummary    (&summary->dataFork);
    
    PrintHeaderString  ("Resource Fork");
    PrintForkSummary    (&summary->resourceFork);
    
    PrintHeaderString  ("Largest Files");
    print("# %10s %10s", "Size", "CNID");
    for (int i = 9; i > 0; i--) {
        if (summary->largestFiles[i].cnid == 0) continue;
        
        char* size = sizeString(summary->largestFiles[i].measure, false);
        wchar_t* name = hfs_catalog_get_cnid_name(&volume, summary->largestFiles[i].cnid);
        print("%d %10s %10u %ls", 10-i, size, summary->largestFiles[i].cnid, name);
        FREE_BUFFER(size); FREE_BUFFER(name);
    }
}

void PrintForkSummary(const ForkSummary *summary)
{
    PrintUI             (summary, count);
    PrintAttributeString("fragmentedCount", "%llu (%0.2f)", summary->fragmentedCount, (float)summary->fragmentedCount/(float)summary->count);
//    PrintUI             (summary, fragmentedCount);
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
        
        FREE_BUFFER(dashes);
    }
    
    FREE_BUFFER(name);
}

void VisualizeHFSPlusAttrKey(const HFSPlusAttrKey *record, const char* label, bool oneLine)
{
    printf("%u, %u, %u, %u, %ls\n", record->keyLength, record->fileID, record->startBlock, record->attrNameLen, (wchar_t*)record->attrName);
    
    HFSUniStr255 hfsName;
    hfsName.length = record->attrNameLen;
    memset(&hfsName.unicode, 0, 255);
    memcpy(&hfsName.unicode, &record->attrName, record->attrNameLen * sizeof(u_int16_t));
    
    wchar_t* name   = hfsuctowcs(&hfsName);
    if (oneLine) {
        printf("%s: %s:%-6u; %s:%-10u; %s:%-10u; %s:%-6u; %s:%-50ls\n",
               label,
               "length",
               record->keyLength,
               "fileID",
               record->fileID,
               "startBlock",
               record->startBlock,
               "length",
               record->attrNameLen,
               "nodeName",
               name
               );
    } else {
        char* names     = " | %-6s | %-10s | %-10s | %-6s | %-50s |\n";
        char* format    = " | %-6u | %-10u | %-10u | %-6u | %-50ls |\n";
        
        char* line_f    =  " +%s+\n";
        char* dashes    = repchar('-', 100);
        
        printf("\n  %s\n", label);
        printf(line_f, dashes);
        printf(names, "length", "fileID", "startBlock", "length", "nodeName");
        printf(format, record->keyLength, record->fileID, record->startBlock, record->attrNameLen, name);
        printf(line_f, dashes);
        
        FREE_BUFFER(dashes);
    }
    
    FREE_BUFFER(name);
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
    
    FREE_BUFFER(dashes);
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
    
    PrintHeaderString      ("Node %u (offset %llu; length: %zu)", node->nodeNumber, node->nodeOffset, node->buffer.size);
    PrintBTNodeDescriptor   (&node->nodeDescriptor);
    
    for (int recordNumber = 0; recordNumber < node->nodeDescriptor.numRecords; recordNumber++) {
        PrintNodeRecord     (node, recordNumber);
    }
}

void PrintFolderListing(u_int32_t folderID)
{
    debug("Printing listing for folder ID %d", folderID);

    // CNID kind mode user group data rsrc name
    char* lineStr       = repchar('-', 100);
    char* headerFormat  = "%-9s %-10s %-10s %-9s %-9s %-15s %-15s %s\n";
    char* rowFormat     = "%-9u %-10s %-10s %-9d %-9d %-15s %-15s %ls\n";
    
    // Get thread record
    HFSBTreeNode node; memset(&node, 0, sizeof(node));
    hfs_record_id recordID = 0;
    HFSPlusCatalogKey key;
    key.parentID = folderID;
    key.nodeName.length = 0;
    key.nodeName.unicode[0] = '\0';
    key.nodeName.unicode[1] = '\0';
    
    HFSBTree tree = hfs_get_catalog_btree(&volume);
    
    int found = hfs_btree_search_tree(&node, &recordID, &tree, &key);
    if (found != 1) {
        error("No thread record for %d found.");
    }
    
    debug("Found thread record %d:%d", node.nodeNumber, recordID);

    HFSPlusCatalogKey *recordKey = (HFSPlusCatalogKey*)node.records[recordID].key;
    HFSPlusCatalogThread *threadRecord = (HFSPlusCatalogThread*)node.records[recordID].value;
    wchar_t* name = hfsuctowcs(&threadRecord->nodeName);
    u_int32_t threadID = recordKey->parentID;
    
    struct {
        unsigned    fileCount;
        unsigned    folderCount;
        unsigned    hardLinkCount;
        unsigned    symlinkCount;
        unsigned    dataForkCount;
        size_t      dataForkSize;
        unsigned    rsrcForkCount;
        size_t      rsrcForkSize;
    } folderStats;
    memset(&folderStats, 0, sizeof(folderStats));

    printf("Listing for %ls\n", name);
    FREE_BUFFER(name);
    
    printf(headerFormat, "CNID", "kind", "mode", "user", "group", "data", "rsrc", "name");

    // Loop over siblings until NULL
    HFSBTreeNodeRecord *record = &node.records[recordID];
    while (1) {
        while ( (record = hfs_catalog_next_in_folder(record)) != NULL ) {
            debug("Looking at record %d", record->recordID);
            HFSPlusCatalogRecord* catalogRecord = (HFSPlusCatalogRecord*)record->value;
            
            if (catalogRecord->record_type == kHFSPlusFileRecord || catalogRecord->record_type == kHFSPlusFolderRecord) {
                u_int32_t cnid = 0;
                char* kind = NULL;
                char* mode = NULL;
                int user = -1, group = -1;
                char* dataSize = NULL;
                char* rsrcSize = NULL;
                
                recordKey = (HFSPlusCatalogKey*)record->key;
                name = hfsuctowcs(&recordKey->nodeName);
                
                if ( hfs_catalog_record_is_hard_link(catalogRecord) && hfs_catalog_record_is_alias(catalogRecord) ) {
                    folderStats.hardLinkCount++;
                    kind = strdup("dir link");
                } else if (hfs_catalog_record_is_hard_link(catalogRecord)) {
                    folderStats.hardLinkCount++;
                    kind = strdup("hard link");
                } else if (hfs_catalog_record_is_symbolic_link(catalogRecord)) {
                    folderStats.symlinkCount++;
                    kind = strdup("symlink");
                } else if (catalogRecord->record_type == kHFSPlusFolderRecord) {
                    folderStats.folderCount++;
                    kind = strdup("folder");
                } else {
                    folderStats.fileCount++;
                    kind = strdup("file");
                }
                
                // Files and folders share these attributes at the same locations.
                cnid        = catalogRecord->catalogFile.fileID;
                user        = catalogRecord->catalogFile.bsdInfo.ownerID;
                group       = catalogRecord->catalogFile.bsdInfo.groupID;
                mode        = _genModeString(catalogRecord->catalogFile.bsdInfo.fileMode);
                
                if (catalogRecord->record_type == kHFSPlusFileRecord) {
                    dataSize    = sizeString(catalogRecord->catalogFile.dataFork.logicalSize, false);
                    rsrcSize    = sizeString(catalogRecord->catalogFile.resourceFork.logicalSize, false);
                    
                    if (catalogRecord->catalogFile.dataFork.totalBlocks > 0) {
                        folderStats.dataForkCount++;
                        folderStats.dataForkSize += catalogRecord->catalogFile.dataFork.logicalSize;
                    }

                    if (catalogRecord->catalogFile.resourceFork.totalBlocks > 0) {
                        folderStats.rsrcForkCount++;
                        folderStats.rsrcForkSize += catalogRecord->catalogFile.resourceFork.logicalSize;
                    }

                } else {
                    dataSize = strdup("-");
                    rsrcSize = strdup("-");
                }
                
                printf(rowFormat, cnid, kind, mode, user, group, dataSize, rsrcSize, name);
                
                FREE_BUFFER(name);
                FREE_BUFFER(mode);
                FREE_BUFFER(dataSize);
                FREE_BUFFER(rsrcSize);
                FREE_BUFFER(kind);
            }
            recordID = record->recordID;
        }
        debug("Done with records in node %d", node.nodeNumber);
        
        // The NULL may have been an end-of-node mark.  Check for this and break if that's not the case.
        if ( (recordID + 1) < (node.recordCount - 1)) {
            debug("Search ended before the end of the node. Abandoning further searching. (%d/%d)", recordID + 1, node.recordCount - 1);
            break;
        }
        
        // Now we look at the next node in the chain and see if that's in it.
        u_int32_t nextNode = node.nodeDescriptor.fLink;
        hfs_btree_free_node(&node);
        
        found = hfs_btree_get_node(&node, &tree, nextNode);
        if (found != 1) {
            error("Failed to get node %d", nextNode);
            return;
        }
        
        HFSPlusCatalogKey *nextKey = (HFSPlusCatalogKey*)node.records[0].key;
        if (nextKey->parentID != threadID) {
            debug("First record's parent is %d; we're looking for %d or %d", nextKey->parentID, folderID, threadID);
            break;
        }
        record = &node.records[0];
        debug("Checking node %d", node.nodeNumber);
    }
    
    char* dataTotal = sizeString(folderStats.dataForkSize, false);
    char* rsrcTotal = sizeString(folderStats.rsrcForkCount, false);
    
    printf("%s\n", lineStr);
    printf(headerFormat, "", "", "", "", "", dataTotal, rsrcTotal, "");
    
    printf("%10s: %-10d %10s: %-10d %10s: %-10d\n",
           "Folders",
           folderStats.folderCount,
           "Data forks",
           folderStats.dataForkCount,
           "Hard links",
           folderStats.hardLinkCount
           );
    
    printf("%10s: %-10d %10s: %-10d %10s: %-10d\n",
           "Files",
           folderStats.fileCount,
           "RSRC forks",
           folderStats.rsrcForkCount,
           "Symlinks",
           folderStats.symlinkCount
           );
    
    FREE_BUFFER(dataTotal);
    FREE_BUFFER(rsrcTotal);
    FREE_BUFFER(lineStr);
    hfs_btree_free_node(&node);
    
    debug("Done listing.");
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
    
    PrintHeaderString("  Record ID %u (%u/%u) (offset %u; length: %zd) (Node %d)",
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
                    PrintAttributeString("recordType", "BTHeaderRec");
                    BTHeaderRec header = *( (BTHeaderRec*) record->record );
                    PrintBTHeaderRecord(&header);
                    break;
                }
                case 1:
                {
                    PrintAttributeString("recordType", "userData (reserved)");
                    break;
                }
                case 2:
                {
                    PrintAttributeString("recordType", "mapData");
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
                
                case kHFSAttributesFileID:
                {
                    HFSPlusAttrKey keyStruct = *( (HFSPlusAttrKey*) record->key );
                    VisualizeHFSPlusAttrKey(&keyStruct, "Attributes Key", 0);
                    
                    if (record->keyLength < kHFSPlusAttrKeyMinimumLength || record->keyLength > kHFSPlusAttrKeyMaximumLength)
                        goto INVALID_KEY;
                    
                    break;
                }
                    
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
                            u_int16_t type =  hfs_get_catalog_record_type(record);
                            
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
                                    PrintAttributeString("recordType", "%u (invalid)", type);
                                    VisualizeData(record->value, record->valueLength);
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
                        case kHFSAttributesFileID:
                        {
                            PrintHFSPlusAttrRecord((HFSPlusAttrRecord*)record->value);
                        }
                    }
                    
                    break;
                }
                    
                default:
                {
                INVALID_KEY:
                    if ((recordNumber + 1) == node->nodeDescriptor.numRecords) {
                        PrintAttributeString("recordType", "(free space)");
                    } else {
                        PrintAttributeString("recordType", "(unknown b-tree/record format)");
                        VisualizeData(record->record, record->length);
                    }
                }
            }
            
            break;
        }
    }
}
