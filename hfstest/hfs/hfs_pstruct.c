//
//  hfs_pstruct.c
//  hfstest
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
#include <libkern/OSByteOrder.h>

#include "hfs_pstruct.h"
#include "hfs.h"
#include "hfs_macos_defs.h"


/*
 * to_bsd_time - convert from Mac OS time (seconds since 1/1/1904)
 *		 to BSD time (seconds since 1/1/1970)
 */
time_t to_bsd_time(u_int32_t hfs_time)
{
	u_int32_t gmt = hfs_time;
	if (gmt > MAC_GMT_FACTOR)
		gmt -= MAC_GMT_FACTOR;
	else
		gmt = 0;	/* don't let date go negative! */
	return (time_t)gmt;
}

size_t string_from_hfs_time(char* buf, int buf_size, u_int32_t hfs_time) {
	time_t gmt = 0;
	gmt = to_bsd_time(hfs_time);
	struct tm *t = gmtime(&gmt);
	size_t len = strftime(buf, buf_size, "%c", t);
	return len;
}

void _PrintString(char* label, char* value_format, ...)
{
    va_list argp;
    va_start(argp, value_format);
    char* str;
    vasprintf(&str, value_format, argp);
    printf("  %-23s  %s\n", label, str);
    free(str);
    va_end(argp);
}

void _PrintAttributeString(char* label, char* value_format, ...)
{
    va_list argp;
    va_start(argp, value_format);
    char* str;
    vasprintf(&str, value_format, argp);
    printf("  %-23s= %s\n", label, str);
    free(str);
    va_end(argp);
}

void _PrintSubattributeString(char* str, ...)
{
    va_list argp;
    va_start(argp, str);
    char* s;
    vasprintf(&s, str, argp);
    printf("  %-23s. %s\n", "", s);
    free(s);
    va_end(argp);
}

void _PrintUI8(char* label, u_int8_t i)      { _PrintAttributeString(label, "%u", i); }
void _PrintUI8Hex(char* label, u_int8_t i)   { _PrintAttributeString(label, "0x%X", i); }
void _PrintUI16(char* label, u_int16_t i)    { _PrintAttributeString(label, "%u", i); }
void _PrintUI16Hex(char* label, u_int16_t i) { _PrintAttributeString(label, "0x%X", i); }
void _PrintUI32(char* label, u_int32_t i)    { _PrintAttributeString(label, "%u", i); }
void _PrintUI32Hex(char* label, u_int32_t i) { _PrintAttributeString(label, "0x%X", i); }
void _PrintUI64(char* label, u_int64_t i)    { _PrintAttributeString(label, "%llu", i); }
void _PrintUI64Hex(char* label, u_int64_t i) { _PrintAttributeString(label, "0x%llX", i); }

void _PrintUI64DataLength(char *label, u_int64_t size)
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
void _PrintUI32DataLength(char *label, u_int32_t size)
{
    _PrintUI64DataLength(label, (u_int64_t)size);
}

void _PrintUI32Binary(char *label, u_int32_t i)
{
    char str[65] = {};
    str[64] = '\0';
    for (int j = 0; j<32; j++) str[31-j] = ((i >> j) & 0x01) ? '1' : '0'; str[32] = '\0';
    _PrintAttributeString(label, "%s", str);
}

void _PrintUI64Binary(char *label, u_int64_t i)
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

void _PrintHFSTimestamp(char* label, u_int32_t timestamp)
{
    if (timestamp > MAC_GMT_FACTOR) {
        timestamp -= MAC_GMT_FACTOR;
    } else {
        timestamp = 0; // Cannot be negative.
    }
    
    time_t seconds = timestamp;
    struct tm *t = gmtime(&seconds);
    
    char* buf = malloc(50);
    strftime(buf, 50, "%c %Z\0", t);
    _PrintAttributeString(label, buf);
    free(buf);
}

void _PrintUI32Char(char* label, u_int32_t i)
{
    char str[5] = { i>>(3*8), i>>(2*8), i>>8, i, '\0' };
    _PrintAttributeString(label, "0x%X (%s)", i, str);
}

void _PrintUI16Char(char* label, u_int16_t i)
{
    char str[3] = { i>>8, i, '\0' };
    _PrintAttributeString(label, "0x%X (%s)", i, str);
}

void PrintVolumeHeader(HFSPlusVolumeHeader *vh)
{
    printf("\n# HFS Plus Volume Header\n");
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
    printf("\n# Finder Info\n");

    // The HFS+ documentation says this is an 8-member array of UI32s.
    // It's a 32-member array of UI8s according to their struct.

    _PrintUI32          ("bootDirID",           OSReadBigInt32(&vh->finderInfo, 0));
    _PrintUI32          ("bootParentID",        OSReadBigInt32(&vh->finderInfo, 4));
    _PrintUI32          ("openWindowDirID",     OSReadBigInt32(&vh->finderInfo, 8));
    _PrintUI32          ("os9DirID",            OSReadBigInt32(&vh->finderInfo, 12));
    _PrintUI32Binary    ("reserved",            OSReadBigInt32(&vh->finderInfo, 16));
    _PrintUI32          ("osXDirID",            OSReadBigInt32(&vh->finderInfo, 20));
    _PrintUI64Hex       ("volID",               OSReadBigInt64(&vh->finderInfo, 24));
    
    printf("\n# Allocation Bitmap File\n");
    PrintHFSPlusForkData(&vh->allocationFile);
    
    printf("\n# Extents Overflow File\n");
    PrintHFSPlusForkData(&vh->extentsFile);
    
    printf("\n# Catalog File\n");
    PrintHFSPlusForkData(&vh->catalogFile);
    
    printf("\n# Attributes File\n");
    PrintHFSPlusForkData(&vh->attributesFile);
    
    printf("\n# Startup File\n");
    PrintHFSPlusForkData(&vh->startupFile);
}

void PrintHFSPlusForkData(HFSPlusForkData *fork)
{
    _PrintUI64DataLength ("logicalSize",    fork->logicalSize);
    _PrintUI32DataLength ("clumpSize",      fork->clumpSize);
    _PrintUI32           ("totalBlocks",    fork->totalBlocks);
    if (fork->extents[0].blockCount > 0) {
        _PrintAttributeString("extents", "%12s %12s %12s", "startBlock", "blockCount", "%% of file");
        int usedExtents = 0;
        int catalogBlocks = 0;
        for (int i = 0; i < 8; i++) {
            HFSPlusExtentDescriptor extent = fork->extents[i];
            if (extent.blockCount == 0)
                break; // unused extent
            usedExtents++;
            catalogBlocks += extent.blockCount;
            _PrintString("", "%12u %12u %12.2f", extent.startBlock, extent.blockCount, (float)extent.blockCount/(float)fork->totalBlocks*100.);
        }
        _PrintString("", "%d allocation blocks in %d extents total.", catalogBlocks, usedExtents);
        _PrintString("", "%0.2f blocks per extent on average.", ( (float)catalogBlocks / (float)usedExtents) );
        if (catalogBlocks < fork->totalBlocks) {
//        TODO: Include these extents above when the b-tree reader is done.
            _PrintString("", "(there are more extents in the extents overflow file)");
        }
    }
}
void PrintBTNodeDescriptor(BTNodeDescriptor *node)
{
    printf("\n# B-Tree Node Descriptor\n");
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
void PrintBTHeaderRecord(BTHeaderRec *hr)
{
    printf("\n# B-Tree Header Record\n");
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

//	printf("Reserved3:        %u\n",    hr->reserved3[16]);
}

void PrintCatalogHeader(HFSVolume *hfs)
{
    HFSPlusVolumeHeader *vh = &hfs->vh;
    
    HFSFork catalog;
    catalog.hfs = *hfs;
    catalog.forkData = vh->catalogFile;
    
    HFSBTree tree;
    hfs_btree_init(&tree, &catalog);
    tree.fork.cnid = kHFSCatalogFileID;
    
    PrintBTNodeDescriptor(&tree.nodeDescriptor);
    
    if (tree.nodeDescriptor.numRecords > 0) {
        PrintBTHeaderRecord(&tree.headerRecord);
    }
    
    for (int j = 0; j<10; j++) {
        printf("\n  # Catalog Node %d\n", j);
        
        HFSBTreeNode node;
        ssize_t result = hfs_btree_get_node(&tree, &node, j);
        if (result == -1) {
            perror("couldn't read node");
            return;
        }
        
        PrintBTNodeDescriptor(&node.nodeDescriptor);
        
        if (node.nodeDescriptor.numRecords) {
            for (int i = 0; i<node.nodeDescriptor.numRecords; i++) {
                printf("    Record %d\n", i);
                hfs_btree_print_record(&node, i);
            }
        }
    }
}
