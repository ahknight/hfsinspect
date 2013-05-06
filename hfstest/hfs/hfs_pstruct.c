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
#include "hfs_pstruct.h"
#include "hfs.h"
#include "hfs_endian.h"
#include "hfs_macos_defs.h"
void _PrintHFSUI8(char* label, u_int8_t i);
void _PrintHFSUI16(char* label, u_int16_t i);
void _PrintHFSUI32(char* label, u_int32_t i);
void _PrintHFSUI64(char* label, u_int64_t i);
void _PrintHFSTimestamp(char* label, u_int32_t timestamp);
void _PrintHFSChar16(char* label, u_int16_t i);
void _PrintHFSChar32(char* label, u_int32_t i);
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
	size_t len = strftime(buf, buf_size, "%c %Z", t);
	return len;
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
void _PrintHFSUI8(char* label, u_int8_t i)      { _PrintAttributeString(label, "%u", i); }
void _PrintHFSUI16(char* label, u_int16_t i)    { _PrintAttributeString(label, "0x%x (%u)", BE16(i), BE16(i)); }
void _PrintHFSUI32(char* label, u_int32_t i)    { _PrintAttributeString(label, "0x%x (%u)", BE32(i), BE32(i)); }
void _PrintHFSUI64(char* label, u_int64_t i)    { _PrintAttributeString(label, "0x%llx (%llu)", BE64(i), BE64(i)); }
void _PrintHFSBinary32(char *label, u_int32_t i)
{
    char str[65] = {};
    str[64] = '\0';
    for (int j = 0; j<32; j++) str[31-j] = ((i >> j) & 0x01) ? '1' : '0'; str[32] = '\0';
    _PrintAttributeString(label, "%s", str);
}
void _PrintHFSFileSize32(char *label, u_int32_t size)
{
    size = BE32(size);
    double bob = size;
    int count = 0;
    char *sizeNames[] = { "bytes", "KiB", "MiB", "GiB", "TiB", "EiB", "PiB", "ZiB", "YiB" };
    while (count < sizeof(sizeNames)) {
        if (bob < 1024.) break;
        bob /= 1024.;
        count ++;
    }
    _PrintAttributeString(label, "%0.2f %s (%u bytes)",  bob, sizeNames[count], size);
}
void _PrintHFSBinary64(char *label, u_int64_t i)
{
//    i = BE64(i);
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
void _PrintHFSFileSize64(char *label, u_int64_t size)
{
    size = BE64(size);
    long double bob = size;
    int count = 0;
    char *sizeNames[] = { "bytes", "KiB", "MiB", "GiB", "TiB", "EiB", "PiB", "ZiB", "YiB" };
    while (count < 9) {
        if (bob < 1024.) break;
        bob /= 1024.;
        count ++;
    }
    printf("  %-23s= %0.2Lf %s (%llu bytes)\n", label, bob, sizeNames[count], size);
}
void _PrintHFSTimestamp(char* label, u_int32_t timestamp)
{
    time_t time = BE32(timestamp);
    if (time > MAC_GMT_FACTOR) {
        time -= MAC_GMT_FACTOR;
    } else {
        time = 0; // Cannot be negative.
    }
    char buf[50];
    struct tm *t = gmtime(&time);
    strftime(buf, 50, "%c %Z", t);
    printf("  %-23s= %s\n", label, buf);
}
void _PrintHFSChar16(char* label, u_int16_t i)
{
    i = BE16(i);
    char str[3] = { i>>8, i, '\0' };
    printf("  %-23s= 0x%x (%s)\n", label, i, str);
}
void _PrintHFSChar32(char* label, u_int32_t i)
{
    i = BE32(i);
    char str[5] = { i>>(3*8), i>>(2*8), i>>8, i, '\0' };
    _PrintAttributeString(label, "0x%x (%s)", i, str);
}
void PrintVolumeHeader(HFSPlusVolumeHeader *vh)
{
    printf("\n# HFS Plus Volume Header\n");
    _PrintHFSChar16     ("signature",              vh->signature);
    _PrintHFSUI16       ("version",                vh->version);
	_PrintHFSBinary32   ("attributes",             vh->attributes);
    {
        u_int32_t attributes = BE32(vh->attributes);
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
    _PrintHFSChar32     ("lastMountedVersion",      vh->lastMountedVersion);
	_PrintHFSUI32       ("journalInfoBlock",        vh->journalInfoBlock);
    _PrintHFSTimestamp  ("createDate",              vh->createDate);
    _PrintHFSTimestamp  ("modifyDate",              vh->modifyDate);
    _PrintHFSTimestamp  ("backupDate",              vh->backupDate);
    _PrintHFSTimestamp  ("checkedDate",             vh->checkedDate);
    _PrintHFSUI32       ("fileCount",               vh->fileCount);
	_PrintHFSUI32       ("folderCount",             vh->folderCount);
	_PrintHFSFileSize32 ("blockSize",               vh->blockSize);
	_PrintHFSUI32       ("totalBlocks",             vh->totalBlocks);
	_PrintHFSUI32       ("freeBlocks",              vh->freeBlocks);
    _PrintHFSUI32       ("nextAllocation",          vh->nextAllocation);
    _PrintHFSUI32       ("rsrcClumpSize",           vh->rsrcClumpSize);
    _PrintHFSUI32       ("dataClumpSize",           vh->dataClumpSize);
    _PrintHFSUI32       ("nextCatalogID",           vh->nextCatalogID);
    _PrintHFSUI32       ("writeCount",              vh->writeCount);
    _PrintHFSBinary64   ("encodingsBitmap",         vh->encodingsBitmap);
    {
        u_int64_t encodings = BE64(vh->encodingsBitmap);
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
    // The HFS+ documentation says this is an 8-member array of UI32s.  It's a 32-member array of UI8s according to their struct.
    // If I cared, I'd copy their struct and fix it to reinterpret this properly.  I don't.  So I'll just assemble a big-endian
    // integer a couple of times to avoid having to add another APSL file to the project.
    u_int32_t be_int = 0; u_int8_t start = 0;
    be_int += vh->finderInfo[start+3]; be_int <<= 8;
    be_int += vh->finderInfo[start+2]; be_int <<= 8;
    be_int += vh->finderInfo[start+1]; be_int <<= 8;
    be_int += vh->finderInfo[start+0];
    _PrintHFSUI32("bootDirID", be_int);
    be_int = 0; start = 4;
    be_int += vh->finderInfo[start+3]; be_int <<= 8;
    be_int += vh->finderInfo[start+2]; be_int <<= 8;
    be_int += vh->finderInfo[start+1]; be_int <<= 8;
    be_int += vh->finderInfo[start+0];
    _PrintHFSUI32("bootParentID", be_int);
    be_int = 0; start = 8;
    be_int += vh->finderInfo[start+3]; be_int <<= 8;
    be_int += vh->finderInfo[start+2]; be_int <<= 8;
    be_int += vh->finderInfo[start+1]; be_int <<= 8;
    be_int += vh->finderInfo[start+0];
    _PrintHFSUI32("openWindowDirID", be_int);
    be_int = 0; start = 12;
    be_int += vh->finderInfo[start+3]; be_int <<= 8;
    be_int += vh->finderInfo[start+2]; be_int <<= 8;
    be_int += vh->finderInfo[start+1]; be_int <<= 8;
    be_int += vh->finderInfo[start+0];
    _PrintHFSUI32("os9DirID", be_int);
    be_int = 0; start = 16;
    be_int += vh->finderInfo[start+3]; be_int <<= 8;
    be_int += vh->finderInfo[start+2]; be_int <<= 8;
    be_int += vh->finderInfo[start+1]; be_int <<= 8;
    be_int += vh->finderInfo[start+0];
    _PrintHFSUI32("reserved", be_int);
    be_int = 0; start = 20;
    be_int += vh->finderInfo[start+3]; be_int <<= 8;
    be_int += vh->finderInfo[start+2]; be_int <<= 8;
    be_int += vh->finderInfo[start+1]; be_int <<= 8;
    be_int += vh->finderInfo[start+0];
    _PrintHFSUI32("osXDirID", be_int);
    u_int64_t volID = 0; start = 24;
    volID += vh->finderInfo[start+7]; volID <<= 8;
    volID += vh->finderInfo[start+6]; volID <<= 8;
    volID += vh->finderInfo[start+5]; volID <<= 8;
    volID += vh->finderInfo[start+4]; volID <<= 8;
    volID += vh->finderInfo[start+3]; volID <<= 8;
    volID += vh->finderInfo[start+2]; volID <<= 8;
    volID += vh->finderInfo[start+1]; volID <<= 8;
    volID += vh->finderInfo[start+0];
    _PrintHFSUI64("volID", volID);
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
    _PrintHFSFileSize64 ("logicalSize",    fork->logicalSize);
    _PrintHFSFileSize32 ("clumpSize",      fork->clumpSize);
    _PrintHFSUI32       ("totalBlocks",    fork->totalBlocks);
    if (BE32(fork->extents[0].blockCount) > 0) {
        printf("  %-23s=%12s %12s %12s\n","extents","startBlock", "blockCount", "%% of file");
        int usedExtents = 0;
        int catalogBlocks = 0;
        for (int i = 0; i < 8; i++) {
            HFSPlusExtentDescriptor extent = fork->extents[i];
            if (BE32(extent.blockCount) == 0)
                break; // unused extent
            usedExtents++;
            catalogBlocks += BE32(extent.blockCount);
            printf("%25s=%12u %12u %10.2f\n","", BE32(extent.startBlock), BE32(extent.blockCount), (float)BE32(extent.blockCount)/(float)BE32(fork->totalBlocks)*100.);
        }
        printf("  %-23s  %d allocation blocks in %d extents total.\n","", catalogBlocks, usedExtents);
        printf("  %-23s  %0.2f blocks per extent on average.\n","", catalogBlocks / (float)usedExtents);
        if (catalogBlocks < BE32(fork->totalBlocks)) {
//        TODO: Include these extents above when the b-tree reader is done.
            printf("%-25s  (there are more extents in the extents overflow file)", "");
        }
    }
}
void PrintBTNodeDescriptor(BTNodeDescriptor *node)
{
    printf("\n# B-Tree Node Descriptor\n");
    _PrintHFSUI32   ("fLink",               node->fLink);
    _PrintHFSUI32   ("bLink",               node->bLink);
    _PrintHFSUI8    ("Kind",                node->kind);
    _PrintHFSUI8    ("Height",              node->height);
    _PrintHFSUI16   ("Number of records",   node->numRecords);
    _PrintHFSUI16   ("Reserved",            node->reserved);
}
void PrintBTHeaderRecord(BTHeaderRec *hr)
{
    printf("\nB-Tree Header Record:\n");
	_PrintHFSUI16   ("treeDepth",       hr->treeDepth);
	_PrintHFSUI32   ("rootNode",        hr->rootNode);
	_PrintHFSUI32   ("leafRecords",     hr->leafRecords);
	_PrintHFSUI32   ("firstLeafNode",   hr->firstLeafNode);
	_PrintHFSUI32   ("lastLeafNode",    hr->lastLeafNode);
	_PrintHFSUI16   ("nodeSize",        hr->nodeSize);
	_PrintHFSUI16   ("maxKeyLength",    hr->maxKeyLength);
	_PrintHFSUI32   ("totalNodes",      hr->totalNodes);
	_PrintHFSUI32   ("freeNodes",       hr->freeNodes);
	_PrintHFSUI16   ("reserved1",       hr->reserved1);
	_PrintHFSUI32   ("clumpSize",       hr->clumpSize);
	_PrintHFSUI8    ("btreeType",       hr->btreeType);
	_PrintHFSUI8    ("keyCompareType",  hr->keyCompareType);
	_PrintHFSUI32   ("attributes",      hr->attributes);
//	printf("Reserved3:        %u\n", hr->reserved3[16]);
}
void PrintCatalogHeader(HFSVolume *hfs)
{
    HFSPlusVolumeHeader vh = hfs->vh;
    HFSFork catalog = { *hfs, (vh.catalogFile) };
	BTNodeDescriptor node;
	size_t size = hfs_readfork(&catalog, &node, sizeof(node), 0);
    if (size == -1) {
        perror("aborted read of catalog node");
        exit(errno);
    }
    PrintBTNodeDescriptor(&node);
	BTHeaderRec header;
	size = hfs_readfork(&catalog, &header, sizeof(header), sizeof(node));
    if (size == -1) {
        perror("aborted read of catalog header record");
        exit(errno);
    }
    PrintBTHeaderRecord(&header);
}