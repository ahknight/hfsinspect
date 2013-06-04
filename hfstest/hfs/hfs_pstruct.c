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
#include <stdbool.h>

#include "hfs_macos_defs.h"
#include "hfs_pstruct.h"
#include "hfs.h"


static HFSVolume volume = {};
void set_hfs_volume(HFSVolume *v) { volume = *v; }


// repeat character
char* _repchr(char c, int count)
{
    char* str = malloc(sizeof(c) * count);
    memset(str, c, count);
    str[count] = '\0';
    return str;
}

// repeat wide character
wchar_t* _repwc(wchar_t c, int count)
{
    wchar_t* str = malloc(sizeof(c) * count);
    memset(str, c, count);
    str[count] = '\0';
    return str;
}

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
        count++;
    }
    _PrintAttributeString(label, "%0.2Lf %s (%llu bytes)", bob, sizeNames[count], size);
}
void _PrintUI32DataLength(const char *label, u_int32_t size)
{
    _PrintUI64DataLength(label, (u_int64_t)size);
}

char* memstr(const char* buf, size_t length, u_int8_t base)
{
    // Base 1 would be interesting (ie. infinite). Past base 36 and we need to start picking printable character sets.
    if (base < 2 || base > 36) return NULL;
    
    // Determine how many characters will be needed in the output for each byte of input (256 bits, 8 bits to a byte).
    u_int8_t chars_per_byte = (256 / base / 8);
    
    // Patches bases over 32.
    if (chars_per_byte < 1) chars_per_byte = 1;
    
    // Size of the result string.
    size_t rlength = (length * chars_per_byte);
    char* result = malloc(rlength + 1);
    
    // Init the string's bytes to the character zero so that positions we don't write to have something printable.
    memset(result, '0', malloc_size(result));
    
    // We build the result string from the tail, so here's the index in that string, starting at the end.
    u_int8_t ridx = rlength - 1;
    for (size_t byte = 0; byte < length; byte++) {
        unsigned char c = buf[byte];    // Grab the current char from the input.
        while (c != 0 && ridx >= 0) {   // Iterate until we're out of input or output.
            u_int8_t idx = c % base;
            result[ridx] = (idx < 10 ? '0' + idx : 'a' + (idx-10)); // GO ASCII! 0-9 then a-z (up to base 36, see above).
            c /= base;                  // Shave off the processed portion.
            ridx--;                     // Move left in the result string.
        }
    }
    
    // Cap the string.
    result[rlength] = '\0';
    
    // Send it back.
    return result;
}

char* hrep(const char* buf, size_t length)
{
    return memstr(buf, length, 16);
}

#define PrintBinary(label, i) _PrintBinary(label, &i, sizeof(i))
void _PrintBinary(const char* label, const void* map, size_t size)
{
    char* str = memstr(map, size, 2);
    _PrintAttributeString(label, "%s", str);
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

#define PrintUIChar(label, i) _PrintUIChar(label, &i, sizeof(i))
void _PrintUIChar(const char* label, const void* i, size_t nbytes)
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
    char* hex = hrep(i, nbytes);
    
    _PrintAttributeString(label, "0x%s (%s)", hex, str);
    
    // Cleanup
    free(hex);
    free(str);
}

void _PrintHFSUniStr255(const char* label, const HFSUniStr255 *record)
{
//    char narrow[256];
//    wchar_t wide[256];
//    int len = MIN(record->length, 255);
//    for (int i = 0; i < len; i++) {
//        narrow[i] = wctob(record->unicode[i]);
//        wide[i] = record->unicode[i];
//    }
//    narrow[len] = '\0';
//    wide[len] = L'\0';
//    
    wchar_t *wide = hfsuctowcs(record);
    wprintf(L"  %-23s= \"%ls\" (%u)\n", label, wide, record->length);
}


void PrintVolumeHeader(const HFSPlusVolumeHeader *vh)
{
//    VisualizeData((char*)vh, sizeof(HFSPlusVolumeHeader));
    
    printf("\n# HFS Plus Volume Header\n");
    PrintUIChar         ("signature",              vh->signature);
    _PrintUI16          ("version",                vh->version);
	PrintBinary         ("attributes",             vh->attributes);
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
    PrintUIChar         ("lastMountedVersion",      vh->lastMountedVersion);
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
    PrintBinary         ("encodingsBitmap",         vh->encodingsBitmap);
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
    {
        u_int32_t reserved = OSReadBigInt32(&vh->finderInfo, 16);
        PrintBinary     ("reserved",            reserved);
    }
    _PrintUI32          ("osXDirID",            OSReadBigInt32(&vh->finderInfo, 20));
    _PrintUI64Hex       ("volID",               OSReadBigInt64(&vh->finderInfo, 24));
    
    printf("\n# Allocation Bitmap File\n");
    PrintHFSPlusForkData(&vh->allocationFile, kHFSAllocationFileID, HFSDataForkType);
    
    printf("\n# Extents Overflow File\n");
    PrintHFSPlusForkData(&vh->extentsFile, kHFSExtentsFileID, HFSDataForkType);
    
    printf("\n# Catalog File\n");
    PrintHFSPlusForkData(&vh->catalogFile, kHFSCatalogFileID, HFSDataForkType);
    
    printf("\n# Attributes File\n");
    PrintHFSPlusForkData(&vh->attributesFile, kHFSAttributeDataFileID, HFSDataForkType);
    
    printf("\n# Startup File\n");
    PrintHFSPlusForkData(&vh->startupFile, kHFSStartupFileID, HFSDataForkType);
}

void PrintHFSPlusExtentRecordBuffer(const Buffer *buffer, unsigned int count, u_int32_t totalBlocks)
{
//    debug("Buffer: cap: %zd; size: %zd; off:%zd", buffer->capacity, buffer->size, buffer->offset);
    
    _PrintAttributeString("extents", "%12s %12s %12s", "startBlock", "blockCount", "% of file");
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
        size_t record_start = 0;
        u_int8_t found = hfs_extents_find_record(extents, &record_start, fork, blocks);
        if (found && blocks < fork->forkData.totalBlocks) goto ADD_EXTENT;
    }
    
    PrintHFSPlusExtentRecordBuffer(&buffer, recordCount, fork->forkData.totalBlocks);
}

void PrintHFSPlusForkData(const HFSPlusForkData *fork, u_int32_t cnid, u_int8_t forktype)
{
    if (forktype == HFSDataForkType) {
        _PrintAttributeString("fork", "data");
    } else if (forktype == HFSResourceForkType) {
        _PrintAttributeString("fork", "resource");
    }
    _PrintUI64DataLength ("logicalSize",    fork->logicalSize);
    _PrintUI32DataLength ("clumpSize",      fork->clumpSize);
    _PrintUI32           ("totalBlocks",    fork->totalBlocks);
    
    if (fork->totalBlocks) {
        HFSFork hfsfork = make_hfsfork(&volume, *fork, forktype, cnid);
        PrintExtentsSummary(&hfsfork);
    }
}

void PrintBTNodeDescriptor(const BTNodeDescriptor *node)
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

#define PrintUI8(x, y)  _PrintUI8(#y, x->y)
#define PrintUI16(x, y) _PrintUI16(#y, x->y)
#define PrintUI32(x, y) _PrintUI32(#y, x->y)
#define PrintUI64(x, y) _PrintUI64(#y, x->y)

void PrintBTHeaderRecord(const BTHeaderRec *hr)
{
    printf("\n# B-Tree Header Record\n");
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
	_PrintUI8Hex    ("keyCompareType",      hr->keyCompareType);
	PrintBinary     ("attributes",          hr->attributes);

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
    
    PrintBinary("adminFlags", record->adminFlags);
    
    for (int i = 0; i < 10; i++) {
        u_int8_t flag = record->adminFlags;
        if (flag & flagMasks[i]) {
            _PrintSubattributeString("%s", flagNames[i]);
        }
    }
    
    PrintBinary("ownerFlags", record->ownerFlags);
    
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
    
    PrintBinary("fileMode", mode);
    
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
    PrintUIChar("fdType", record->fdType);
    PrintUIChar("fdCreator", record->fdCreator);
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
    PrintBinary("flags", record->flags);
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
    PrintBinary("flags", record->flags);
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
    PrintHFSPlusForkData(&record->dataFork, kHFSCatalogFileID, HFSDataForkType);
    PrintHFSPlusForkData(&record->resourceFork, kHFSCatalogFileID, HFSResourceForkType);
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
        char* dashes    = _repchr('-', 83);
        
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
    char* dashes    = _repchr('-', 57);
    
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

void memdump(const char* data, size_t length, u_int8_t base, u_int8_t gsize, u_int8_t gcount)
{
    int group_size = gsize;
    int group_count = gcount;
    
    int line_width = group_size * group_count;
    unsigned int offset = 0;
    while (length > offset) {
        const char* line = &data[offset];
        size_t lineMax = MIN((length - offset), line_width);
        printf("  %06u |", offset);
        for (int c = 0; c < lineMax; c++) {
            if ( (c % group_size) == 0 ) printf(" ");
            char* str = memstr(&line[c], 1, base);
            printf("%s ", str); free(str);
        }
        printf("|");
        for (int c = 0; c < lineMax; c++) {
            if ( (c % group_size) == 0 ) printf(" ");
            char chr = line[c] & 0xFF;
            if (chr < 32) // ASCII unprintable
                chr = '.';
            printf("%c", chr);
        }
        printf(" |\n");
        offset += line_width;
    }
}

void VisualizeData(const char* data, size_t length)
{
    memdump(data, length, 16, 4, 4);
}


void PrintNode(const HFSBTreeNode* node)
{
    printf("\n# Node %u (offset %llu; length: %zu)\n", node->nodeNumber, node->nodeOffset, node->buffer.size);
    PrintBTNodeDescriptor(&node->nodeDescriptor);
    
    for (int recordNumber = 0; recordNumber < node->nodeDescriptor.numRecords; recordNumber++) {
        PrintNodeRecord(node, recordNumber);
    }
}

void PrintNodeRecord(const HFSBTreeNode* node, int recordNumber)
{
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
    
    
    printf("\n  # Node %d - Record ID %u (%u/%u) (offset %u; length: %zd)\n",
           node->nodeNumber,
           recordNumber,
           recordNumber + 1,
           node->nodeDescriptor.numRecords,
           offset,
           record->length);
    
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
                    _PrintUI32("nextNodeID", *pointer);
                    break;
                }
                    
                case kBTLeafNode:
                {
                    switch (node->bTree.fork.cnid) {
                        case kHFSCatalogFileID:
                        {
                            u_int16_t type =  *(u_int16_t*)(record->value);
//                            _PrintAttributeString("recordType", "%d", type);
                            
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
                                    ;;
                                }
                            }
                            break;
                        }
                        case kHFSExtentsFileID:
                        {
                            PrintHFSPlusExtentRecordBuffer(record->value, 1, 0);
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

void PrintTreeNode(const HFSBTree *tree, u_int32_t nodeID)
{
    HFSBTreeNode node = {};
    int8_t result = hfs_btree_get_node(&node, tree, nodeID);
    if (result == -1) {
        error("node %u is invalid or out of range.", nodeID);
        return;
    }
    PrintNode(&node);
    hfs_btree_free_node(&node);
}
