//
//  output_hfs.c
//  hfsinspect
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <sys/stat.h>

#include <string.h>             // memcpy, strXXX, etc.
#if defined(__linux__)
    #include <bsd/string.h>     // strlcpy, etc.
#endif

#include "hfs/output_hfs.h"
#include "volumes/output.h"
#include "hfsinspect/stringtools.h"
#include "hfs/unicode.h"
#include "hfs/types.h"
#include "hfs/catalog.h"
#include "hfs/hfs.h"
#include "logging/logging.h"    // console printing routines


static HFS* volume_ = NULL;
void set_hfs_volume(HFS* v) { volume_ = v; }

#pragma mark Value Print Functions

void _PrintCatalogName(out_ctx* ctx, char* label, bt_nodeid_t cnid)
{
    hfs_wc_str name;
    if (cnid != 0)
        HFSPlusGetCNIDName(name, (FSSpec){volume_, cnid});
    else
        name[0] = L'\0';

    PrintAttribute(ctx, label, "%d (%ls)", cnid, name);
}

void _PrintHFSBlocks(out_ctx* ctx, const char* label, uint64_t blocks)
{
    char sizeLabel[50] = "";
    (void)format_blocks(ctx, sizeLabel, blocks, volume_->block_size, 50);
    PrintAttribute(ctx, label, sizeLabel);
}

void _PrintHFSTimestamp(out_ctx* ctx, const char* label, uint32_t timestamp)
{
    char buf[50];
    (void)format_hfs_timestamp(ctx, buf, timestamp, 50);
    PrintAttribute(ctx, label, buf);
}

void _PrintHFSChar(out_ctx* ctx, const char* label, const char* i, size_t nbytes)
{
    char str[50] = {0};
    char hex[50] = {0};

    (void)format_hfs_chars(ctx, str, i, nbytes, 50);
    (void)format_dump(ctx, hex, i, 16, nbytes, 50);

    PrintAttribute(ctx, label, "0x%s (%s)", hex, str);
}

void PrintHFSUniStr255(out_ctx* ctx, const char* label, const HFSUniStr255* record)
{
    hfs_wc_str wide;
    hfsuctowcs(wide, record);
    PrintAttribute(ctx, label, "\"%ls\" (%u)", wide, record->length);
}

#pragma mark Structure Print Functions

void PrintVolumeInfo(out_ctx* ctx, const HFS* hfs)
{
    if (hfs->vh.signature == kHFSPlusSigWord)
        BeginSection(ctx, "HFS+ Volume Format (v%d)", hfs->vh.version);
    else if (hfs->vh.signature == kHFSXSigWord)
        BeginSection(ctx, "HFSX Volume Format (v%d)", hfs->vh.version);
    else
        BeginSection(ctx, "Unknown Volume Format"); // Curious.

    hfs_wc_str volumeName = {0};
    int        success    = HFSPlusGetCNIDName(volumeName, (FSSpec){hfs, kHFSRootFolderID});
    if (success)
        PrintAttribute(ctx, "volume name", "%ls", volumeName);

    BTreePtr   catalog    = NULL;
    hfs_get_catalog_btree(&catalog, hfs);

    if ((hfs->vh.signature == kHFSXSigWord) && (catalog->headerRecord.keyCompareType == kHFSBinaryCompare)) {
        PrintAttribute(ctx, "case sensitivity", "case sensitive");
    } else {
        PrintAttribute(ctx, "case sensitivity", "case insensitive");
    }

    HFSPlusVolumeFinderInfo finderInfo = { .finderInfo = {hfs->vh.finderInfo} };
    if (finderInfo.bootDirID || finderInfo.bootParentID || finderInfo.os9DirID || finderInfo.osXDirID) {
        PrintAttribute(ctx, "bootable", "yes");
    } else {
        PrintAttribute(ctx, "bootable", "no");
    }
    EndSection(ctx);
}

void PrintHFSMasterDirectoryBlock(out_ctx* ctx, const HFSMasterDirectoryBlock* vcb)
{
    BeginSection(ctx, "HFS Master Directory Block");

    PrintHFSChar(ctx, vcb, drSigWord);
    PrintHFSTimestamp(ctx, vcb, drCrDate);
    PrintHFSTimestamp(ctx, vcb, drLsMod);
    PrintRawAttribute(ctx, vcb, drAtrb, 2);
    PrintUI(ctx, vcb, drNmFls);
    PrintUI(ctx, vcb, drVBMSt);
    PrintUI(ctx, vcb, drAllocPtr);
    PrintUI(ctx, vcb, drNmAlBlks);
    PrintDataLength(ctx, vcb, drAlBlkSiz);
    PrintDataLength(ctx, vcb, drClpSiz);
    PrintUI(ctx, vcb, drAlBlSt);
    PrintUI(ctx, vcb, drNxtCNID);
    PrintUI(ctx, vcb, drFreeBks);

    char name[32]; memset(name, '\0', 32); memcpy(name, vcb->drVN, 31);
    PrintAttribute(ctx, "drVN", "%s", name);

    PrintHFSTimestamp(ctx, vcb, drVolBkUp);
    PrintUI(ctx, vcb, drVSeqNum);
    PrintUI(ctx, vcb, drWrCnt);
    PrintDataLength(ctx, vcb, drXTClpSiz);
    PrintDataLength(ctx, vcb, drCTClpSiz);
    PrintUI(ctx, vcb, drNmRtDirs);
    PrintUI(ctx, vcb, drFilCnt);
    PrintUI(ctx, vcb, drDirCnt);
    PrintUI(ctx, vcb, drFndrInfo[0]);
    PrintUI(ctx, vcb, drFndrInfo[1]);
    PrintUI(ctx, vcb, drFndrInfo[2]);
    PrintUI(ctx, vcb, drFndrInfo[3]);
    PrintUI(ctx, vcb, drFndrInfo[4]);
    PrintUI(ctx, vcb, drFndrInfo[5]);
    PrintUI(ctx, vcb, drFndrInfo[6]);
    PrintUI(ctx, vcb, drFndrInfo[7]);
    PrintHFSChar(ctx, vcb, drEmbedSigWord);
    PrintUI(ctx, vcb, drEmbedExtent.startBlock);
    PrintUI(ctx, vcb, drEmbedExtent.blockCount);
    EndSection(ctx);
}

void PrintVolumeHeader(out_ctx* ctx, const HFSPlusVolumeHeader* vh)
{
    BeginSection(ctx, "HFS Plus Volume Header");
    PrintHFSChar        (ctx, vh, signature);
    PrintUI             (ctx, vh, version);

    PrintRawAttribute   (ctx, vh, attributes, 2);
    PrintUIFlagIfMatch  (ctx, vh->attributes, kHFSVolumeHardwareLockMask);
    PrintUIFlagIfMatch  (ctx, vh->attributes, kHFSVolumeUnmountedMask);
    PrintUIFlagIfMatch  (ctx, vh->attributes, kHFSVolumeSparedBlocksMask);
    PrintUIFlagIfMatch  (ctx, vh->attributes, kHFSVolumeNoCacheRequiredMask);
    PrintUIFlagIfMatch  (ctx, vh->attributes, kHFSBootVolumeInconsistentMask);
    PrintUIFlagIfMatch  (ctx, vh->attributes, kHFSCatalogNodeIDsReusedMask);
    PrintUIFlagIfMatch  (ctx, vh->attributes, kHFSVolumeJournaledMask);
    PrintUIFlagIfMatch  (ctx, vh->attributes, kHFSVolumeInconsistentMask);
    PrintUIFlagIfMatch  (ctx, vh->attributes, kHFSVolumeSoftwareLockMask);
    PrintUIFlagIfMatch  (ctx, vh->attributes, kHFSUnusedNodeFixMask);
    PrintUIFlagIfMatch  (ctx, vh->attributes, kHFSContentProtectionMask);
    PrintUIFlagIfMatch  (ctx, vh->attributes, kHFSMDBAttributesMask);

    PrintHFSChar        (ctx, vh, lastMountedVersion);
    PrintUI             (ctx, vh, journalInfoBlock);
    PrintHFSTimestamp   (ctx, vh, createDate);
    PrintHFSTimestamp   (ctx, vh, modifyDate);
    PrintHFSTimestamp   (ctx, vh, backupDate);
    PrintHFSTimestamp   (ctx, vh, checkedDate);
    PrintUI             (ctx, vh, fileCount);
    PrintUI             (ctx, vh, folderCount);
    PrintDataLength     (ctx, vh, blockSize);
    PrintHFSBlocks      (ctx, vh, totalBlocks);
    PrintHFSBlocks      (ctx, vh, freeBlocks);
    PrintUI             (ctx, vh, nextAllocation);
    PrintDataLength     (ctx, vh, rsrcClumpSize);
    PrintDataLength     (ctx, vh, dataClumpSize);
    PrintUI             (ctx, vh, nextCatalogID);
    PrintUI             (ctx, vh, writeCount);

    PrintRawAttribute   (ctx, vh, encodingsBitmap, 2);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacRoman);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacJapanese);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacChineseTrad);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacKorean);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacArabic);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacHebrew);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacGreek);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacCyrillic);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacDevanagari);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacGurmukhi);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacGujarati);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacOriya);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacBengali);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacTamil);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacTelugu);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacKannada);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacMalayalam);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacSinhalese);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacBurmese);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacKhmer);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacThai);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacLaotian);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacGeorgian);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacArmenian);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacChineseSimp);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacTibetan);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacMongolian);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacEthiopic);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacCentralEurRoman);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacVietnamese);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacExtArabic);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacSymbol);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacDingbats);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacTurkish);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacCroatian);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacIcelandic);
    PrintUIFlagIfSet    (ctx, vh->encodingsBitmap, kTextEncodingMacRomanian);
    if (vh->encodingsBitmap & ((uint64_t)1 << 49)) PrintAttribute(ctx, NULL, "%s (%u)", "kTextEncodingMacFarsi", 49);
    if (vh->encodingsBitmap & ((uint64_t)1 << 48)) PrintAttribute(ctx, NULL, "%s (%u)", "kTextEncodingMacUkrainian", 48);

    BeginSection(ctx, "Finder Info");

    HFSPlusVolumeFinderInfo* finderInfo = (void*)&vh->finderInfo;

    PrintCatalogName    (ctx, finderInfo, bootDirID);
    PrintCatalogName    (ctx, finderInfo, bootParentID);
    PrintCatalogName    (ctx, finderInfo, openWindowDirID);
    PrintCatalogName    (ctx, finderInfo, os9DirID);
    PrintRawAttribute   (ctx, finderInfo, reserved, 2);
    PrintCatalogName    (ctx, finderInfo, osXDirID);
    PrintRawAttribute   (ctx, finderInfo, volID, 16);

    EndSection(ctx);

    BeginSection(ctx, "Allocation Bitmap File");
    PrintHFSPlusForkData(ctx, &vh->allocationFile, kHFSAllocationFileID, HFSDataForkType);
    EndSection(ctx);

    BeginSection(ctx, "Extents Overflow File");
    PrintHFSPlusForkData(ctx, &vh->extentsFile, kHFSExtentsFileID, HFSDataForkType);
    EndSection(ctx);

    BeginSection(ctx, "Catalog File");
    PrintHFSPlusForkData(ctx, &vh->catalogFile, kHFSCatalogFileID, HFSDataForkType);
    EndSection(ctx);

    BeginSection(ctx, "Attributes File");
    PrintHFSPlusForkData(ctx, &vh->attributesFile, kHFSAttributesFileID, HFSDataForkType);
    EndSection(ctx);

    BeginSection(ctx, "Startup File");
    PrintHFSPlusForkData(ctx, &vh->startupFile, kHFSStartupFileID, HFSDataForkType);
    EndSection(ctx);
}

void PrintExtentList(out_ctx* ctx, const ExtentList* list, uint32_t totalBlocks)
{
    PrintAttribute(ctx, "extents", "%12s %12s %12s", "startBlock", "blockCount", "% of file");
    int     usedExtents   = 0;
    int     catalogBlocks = 0;
    float   total         = 0;

    Extent* e             = NULL;

    TAILQ_FOREACH(e, list, extents) {
        usedExtents++;
        catalogBlocks += e->blockCount;

        if (totalBlocks) {
            float pct = (float)e->blockCount/(float)totalBlocks*100.;
            total += pct;
            PrintAttribute(ctx, "", "%12u %12u %12.2f", e->startBlock, e->blockCount, pct);
        } else {
            PrintAttribute(ctx, "", "%12u %12u %12s", e->startBlock, e->blockCount, "?");
        }
    }

    char sumLine[50] = {'\0'};
    memset(sumLine, '-', 38);
    PrintAttribute(ctx, "", sumLine);

    if (totalBlocks) {
        PrintAttribute(ctx, "", "%4d extents %12d %12.2f", usedExtents, catalogBlocks, total);
    } else {
        PrintAttribute(ctx, "", "%12s %12d %12s", "", catalogBlocks, "?");
    }

//    PrintAttribute(ctx, "", "%d allocation blocks in %d extents total.", catalogBlocks, usedExtents);
    PrintAttribute(ctx, "", "%0.2f blocks per extent on average.", ( (float)catalogBlocks / (float)usedExtents) );
}

void PrintForkExtentsSummary(out_ctx* ctx, const HFSFork* fork)
{
    info("Printing extents summary");

    PrintExtentList(ctx, fork->extents, fork->totalBlocks);
}

void PrintHFSPlusForkData(out_ctx* ctx, const HFSPlusForkData* fork, uint32_t cnid, uint8_t forktype)
{
    if (forktype == HFSDataForkType) {
        PrintAttribute(ctx, "fork", "data");
    } else if (forktype == HFSResourceForkType) {
        PrintAttribute(ctx, "fork", "resource");
    }
    if (fork->logicalSize == 0) {
        PrintAttribute(ctx, "logicalSize", "(empty)");
        return;
    }

    PrintDataLength (ctx, fork, logicalSize);
    PrintDataLength (ctx, fork, clumpSize);
    PrintHFSBlocks  (ctx, fork, totalBlocks);

    if (fork->totalBlocks) {
        HFSFork* hfsfork;
        if ( hfsfork_make(&hfsfork, volume_, *fork, forktype, cnid) < 0 ) {
            critical("Could not create fork for fileID %u", cnid);
            return;
        }
        PrintForkExtentsSummary(ctx, hfsfork);
        hfsfork_free(hfsfork);
        hfsfork = NULL;
    }
}

void PrintBTNodeDescriptor(out_ctx* ctx, const BTNodeDescriptor* node)
{
    BeginSection(ctx, "Node Descriptor");
    PrintUI(ctx, node, fLink);
    PrintUI(ctx, node, bLink);
    PrintConstIfEqual(ctx, node->kind, kBTLeafNode);
    PrintConstIfEqual(ctx, node->kind, kBTIndexNode);
    PrintConstIfEqual(ctx, node->kind, kBTHeaderNode);
    PrintConstIfEqual(ctx, node->kind, kBTMapNode);
    PrintUI(ctx, node, height);
    PrintUI(ctx, node, numRecords);
    PrintUI(ctx, node, reserved);
    EndSection(ctx);
}

void PrintBTHeaderRecord(out_ctx* ctx, const BTHeaderRec* hr)
{
    BeginSection(ctx, "Header Record");
    PrintUI         (ctx, hr, treeDepth);
    PrintUI         (ctx, hr, rootNode);
    PrintUI         (ctx, hr, leafRecords);
    PrintUI         (ctx, hr, firstLeafNode);
    PrintUI         (ctx, hr, lastLeafNode);
    PrintDataLength (ctx, hr, nodeSize);
    PrintUI         (ctx, hr, maxKeyLength);
    PrintUI         (ctx, hr, totalNodes);
    PrintUI         (ctx, hr, freeNodes);
    PrintUI         (ctx, hr, reserved1);
    PrintDataLength (ctx, hr, clumpSize);

    PrintConstIfEqual(ctx, hr->btreeType, kBTHFSTreeType);
    PrintConstIfEqual(ctx, hr->btreeType, kBTUserTreeType);
    PrintConstIfEqual(ctx, hr->btreeType, kBTReservedTreeType);

    PrintConstHexIfEqual(ctx, hr->keyCompareType, kHFSCaseFolding);
    PrintConstHexIfEqual(ctx, hr->keyCompareType, kHFSBinaryCompare);

    PrintRawAttribute(ctx, hr, attributes, 2);
    PrintUIFlagIfMatch(ctx, hr->attributes, kBTBadCloseMask);
    PrintUIFlagIfMatch(ctx, hr->attributes, kBTBigKeysMask);
    PrintUIFlagIfMatch(ctx, hr->attributes, kBTVariableIndexKeysMask);

    EndSection(ctx);
}

int _genModeString(char* modeString, uint16_t mode)
{
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
#if defined(BSD)
    if (S_ISWHT(mode)) {
        modeString[0] = 'x';
    }
#endif

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

    if ((mode & S_ISVTX) && !(mode & S_IXOTH)) {
        modeString[9] = 'T';
    } else if ((mode & S_ISVTX) && (mode & S_IXOTH)) {
        modeString[9] = 't';
    } else if (!(mode & S_ISVTX) && (mode & S_IXOTH)) {
        modeString[9] = 'x';
    } else {
        modeString[9] = '-';
    }

    modeString[10] = '\0';

    return strlen(modeString);
}

void PrintHFSPlusBSDInfo(out_ctx* ctx, const HFSPlusBSDInfo* record)
{
    PrintUI(ctx, record, ownerID);
    PrintUI(ctx, record, groupID);

    int   flagMasks[] = {
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
        "no dump",
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

    PrintUIOct(ctx, record, adminFlags);

    for (unsigned i = 0; i < 10; i++) {
        uint8_t flag = record->adminFlags;
        if (flag & flagMasks[i]) {
            PrintAttribute(ctx, NULL, "%05o %s", flagMasks[i], flagNames[i]);
        }
    }

    PrintUIOct(ctx, record, ownerFlags);

    for (unsigned i = 0; i < 10; i++) {
        uint8_t flag = record->ownerFlags;
        if (flag & flagMasks[i]) {
            PrintAttribute(ctx, NULL, "%05o %s", flagMasks[i], flagNames[i]);
        }
    }

    uint16_t mode = record->fileMode;

    char     modeString[11];
    _genModeString(modeString, mode);
    PrintAttribute(ctx, "fileMode", modeString);

    PrintUIOct(ctx, record, fileMode);

    PrintConstOctIfEqual(ctx, mode & S_IFMT, S_IFBLK);
    PrintConstOctIfEqual(ctx, mode & S_IFMT, S_IFCHR);
    PrintConstOctIfEqual(ctx, mode & S_IFMT, S_IFDIR);
    PrintConstOctIfEqual(ctx, mode & S_IFMT, S_IFIFO);
    PrintConstOctIfEqual(ctx, mode & S_IFMT, S_IFREG);
    PrintConstOctIfEqual(ctx, mode & S_IFMT, S_IFLNK);
    PrintConstOctIfEqual(ctx, mode & S_IFMT, S_IFSOCK);
//    PrintConstOctIfEqual(ctx, mode & S_IFMT, S_IFWHT);

    PrintUIOctFlagIfMatch(ctx, mode, S_ISUID);
    PrintUIOctFlagIfMatch(ctx, mode, S_ISGID);
    PrintUIOctFlagIfMatch(ctx, mode, S_ISVTX);

    PrintUIOctFlagIfMatch(ctx, mode, S_IRUSR);
    PrintUIOctFlagIfMatch(ctx, mode, S_IWUSR);
    PrintUIOctFlagIfMatch(ctx, mode, S_IXUSR);

    PrintUIOctFlagIfMatch(ctx, mode, S_IRGRP);
    PrintUIOctFlagIfMatch(ctx, mode, S_IWGRP);
    PrintUIOctFlagIfMatch(ctx, mode, S_IXGRP);

    PrintUIOctFlagIfMatch(ctx, mode, S_IROTH);
    PrintUIOctFlagIfMatch(ctx, mode, S_IWOTH);
    PrintUIOctFlagIfMatch(ctx, mode, S_IXOTH);


    PrintUI(ctx, record, special.linkCount);
}

void PrintFndrFileInfo(out_ctx* ctx, const FndrFileInfo* record)
{
    PrintHFSChar(ctx, record, fdType);
    PrintHFSChar(ctx, record, fdCreator);
    PrintRawAttribute(ctx, record, fdFlags, 2);
    PrintFinderFlags(ctx, record->fdFlags);
    PrintAttribute(ctx, "fdLocation", "(%u, %u)", record->fdLocation.v, record->fdLocation.h);
    PrintUI(ctx, record, opaque);
}

void PrintFndrDirInfo(out_ctx* ctx, const FndrDirInfo* record)
{
    PrintAttribute(ctx, "frRect", "(%u, %u, %u, %u)", record->frRect.top, record->frRect.left, record->frRect.bottom, record->frRect.right);
    PrintRawAttribute(ctx, record, frFlags, 2);
    PrintFinderFlags    (ctx, record->frFlags);
    PrintAttribute(ctx, "frLocation", "(%u, %u)", record->frLocation.v, record->frLocation.h);
    PrintUI             (ctx, record, opaque);
}

void PrintFinderFlags(out_ctx* ctx, uint16_t record)
{
    uint16_t kIsOnDesk            = 0x0001;      /* Files and folders (System 6) */
    uint16_t kRequireSwitchLaunch = 0x0020;      /* Old */
    uint16_t kColor               = 0x000E;      /* Files and folders */
    uint16_t kIsShared            = 0x0040;      /* Files only (Applications only) If */
    uint16_t kHasNoINITs          = 0x0080;      /* Files only (Extensions/Control */
    uint16_t kHasBeenInited       = 0x0100;      /* Files only.  Clear if the file */
    uint16_t kHasCustomIcon       = 0x0400;      /* Files and folders */
    uint16_t kIsStationery        = 0x0800;      /* Files only */
    uint16_t kNameLocked          = 0x1000;      /* Files and folders */
    uint16_t kHasBundle           = 0x2000;      /* Files only */
    uint16_t kIsInvisible         = 0x4000;      /* Files and folders */
    uint16_t kIsAlias             = 0x8000;      /* Files only */

    PrintUIFlagIfMatch(ctx, record, kIsOnDesk);
    PrintUIFlagIfMatch(ctx, record, kRequireSwitchLaunch);
    PrintUIFlagIfMatch(ctx, record, kColor);
    PrintUIFlagIfMatch(ctx, record, kIsShared);
    PrintUIFlagIfMatch(ctx, record, kHasNoINITs);
    PrintUIFlagIfMatch(ctx, record, kHasBeenInited);
    PrintUIFlagIfMatch(ctx, record, kHasCustomIcon);
    PrintUIFlagIfMatch(ctx, record, kIsStationery);
    PrintUIFlagIfMatch(ctx, record, kNameLocked);
    PrintUIFlagIfMatch(ctx, record, kHasBundle);
    PrintUIFlagIfMatch(ctx, record, kIsInvisible);
    PrintUIFlagIfMatch(ctx, record, kIsAlias);
}

void PrintFndrOpaqueInfo(out_ctx* ctx, const FndrOpaqueInfo* record)
{
    // It's opaque. Provided for completeness, and just incase some properties are discovered.
}

void PrintHFSPlusCatalogFolder(out_ctx* ctx, const HFSPlusCatalogFolder* record)
{
    PrintAttribute(ctx, "recordType", "kHFSPlusFolderRecord");

    PrintRawAttribute(ctx, record, flags, 2);
    PrintConstIfEqual(ctx, record->flags, kHFSFileLockedMask);
    PrintConstIfEqual(ctx, record->flags, kHFSThreadExistsMask);
    PrintConstIfEqual(ctx, record->flags, kHFSHasAttributesMask);
    PrintConstIfEqual(ctx, record->flags, kHFSHasSecurityMask);
    PrintConstIfEqual(ctx, record->flags, kHFSHasFolderCountMask);
    PrintConstIfEqual(ctx, record->flags, kHFSHasLinkChainMask);
    PrintConstIfEqual(ctx, record->flags, kHFSHasChildLinkMask);
    PrintConstIfEqual(ctx, record->flags, kHFSHasDateAddedMask);

    PrintUI(ctx, record, valence);
    PrintUI(ctx, record, folderID);
    PrintHFSTimestamp(ctx, record, createDate);
    PrintHFSTimestamp(ctx, record, contentModDate);
    PrintHFSTimestamp(ctx, record, attributeModDate);
    PrintHFSTimestamp(ctx, record, accessDate);
    PrintHFSTimestamp(ctx, record, backupDate);
    PrintHFSPlusBSDInfo(ctx, &record->bsdInfo);

    BeginSection(ctx, "Finder Info");
    PrintFndrDirInfo(ctx, &record->userInfo);
    PrintFndrOpaqueInfo(ctx, &record->finderInfo);
    EndSection(ctx);

    PrintUI(ctx, record, textEncoding);
    PrintUI(ctx, record, folderCount);
}

void PrintHFSPlusCatalogFile(out_ctx* ctx, const HFSPlusCatalogFile* record)
{
    PrintAttribute(ctx, "recordType", "kHFSPlusFileRecord");
    PrintRawAttribute(ctx, record, flags, 2);
    PrintUIFlagIfMatch(ctx, record->flags, kHFSFileLockedMask);
    PrintUIFlagIfMatch(ctx, record->flags, kHFSThreadExistsMask);
    PrintUIFlagIfMatch(ctx, record->flags, kHFSHasAttributesMask);
    PrintUIFlagIfMatch(ctx, record->flags, kHFSHasSecurityMask);
    PrintUIFlagIfMatch(ctx, record->flags, kHFSHasFolderCountMask);
    PrintUIFlagIfMatch(ctx, record->flags, kHFSHasLinkChainMask);
    PrintUIFlagIfMatch(ctx, record->flags, kHFSHasChildLinkMask);
    PrintUIFlagIfMatch(ctx, record->flags, kHFSHasDateAddedMask);

    PrintUI                 (ctx, record, reserved1);
    PrintUI                 (ctx, record, fileID);
    PrintHFSTimestamp       (ctx, record, createDate);
    PrintHFSTimestamp       (ctx, record, contentModDate);
    PrintHFSTimestamp       (ctx, record, attributeModDate);
    PrintHFSTimestamp       (ctx, record, accessDate);
    PrintHFSTimestamp       (ctx, record, backupDate);
    PrintHFSPlusBSDInfo     (ctx, &record->bsdInfo);

    BeginSection(ctx, "Finder Info");
    PrintFndrFileInfo       (ctx, &record->userInfo);
    PrintFndrOpaqueInfo     (ctx, &record->finderInfo);
    EndSection(ctx);

    PrintUI                 (ctx, record, textEncoding);
    PrintUI                 (ctx, record, reserved2);

    BeginSection(ctx, "Data Fork");
    PrintHFSPlusForkData(ctx, &record->dataFork, kHFSCatalogFileID, HFSDataForkType);
    EndSection(ctx);

    if (record->resourceFork.logicalSize) {
        BeginSection(ctx, "Resource Fork");
        PrintHFSPlusForkData(ctx, &record->resourceFork, kHFSCatalogFileID, HFSResourceForkType);
        EndSection(ctx);
    }
}

void PrintHFSPlusFolderThreadRecord(out_ctx* ctx, const HFSPlusCatalogThread* record)
{
    PrintAttribute       (ctx, "recordType", "kHFSPlusFolderThreadRecord");
    PrintHFSPlusCatalogThread   (ctx, record);
}

void PrintHFSPlusFileThreadRecord(out_ctx* ctx, const HFSPlusCatalogThread* record)
{
    PrintAttribute       (ctx, "recordType", "kHFSPlusFileThreadRecord");
    PrintHFSPlusCatalogThread   (ctx, record);
}

void PrintHFSPlusCatalogThread(out_ctx* ctx, const HFSPlusCatalogThread* record)
{
    PrintUI             (ctx, record, reserved);
    PrintUI             (ctx, record, parentID);
    PrintHFSUniStr255  (ctx, "nodeName", &record->nodeName);
}

void PrintHFSPlusAttrForkData(out_ctx* ctx, const HFSPlusAttrForkData* record)
{
    PrintHFSPlusForkData(ctx, &record->theFork, 0, 0);
}

void PrintHFSPlusAttrExtents(out_ctx* ctx, const HFSPlusAttrExtents* record)
{
    PrintHFSPlusExtentRecord(ctx, &record->extents);
}

void PrintHFSPlusAttrData(out_ctx* ctx, const HFSPlusAttrData* record)
{
    PrintUI(ctx, record, attrSize);

    VisualizeData((char*)&record->attrData, record->attrSize);
}

void PrintHFSPlusAttrRecord(out_ctx* ctx, const HFSPlusAttrRecord* record)
{
    PrintConstHexIfEqual(ctx, record->recordType, kHFSPlusAttrInlineData);
    PrintConstHexIfEqual(ctx, record->recordType, kHFSPlusAttrForkData);
    PrintConstHexIfEqual(ctx, record->recordType, kHFSPlusAttrExtents);

    switch (record->recordType) {
        case kHFSPlusAttrInlineData:
        {
            PrintHFSPlusAttrData(ctx, &record->attrData);
            break;
        }

        case kHFSPlusAttrForkData:
        {
            PrintHFSPlusAttrForkData(ctx, &record->forkData);
            break;
        }

        case kHFSPlusAttrExtents:
        {
            PrintHFSPlusAttrExtents(ctx, &record->overflowExtents);
            break;
        }

        default:
        {
            error("unknown attribute record type: %d", record->recordType);
            VisualizeData((char*)record, sizeof(HFSPlusAttrRecord));
            break;
        }
    }
}

void PrintHFSPlusExtentRecord(out_ctx* ctx, const HFSPlusExtentRecord* record)
{
    ExtentList* list = extentlist_make();
    extentlist_add_record(list, *record);
    PrintExtentList(ctx, list, 0);
    extentlist_free(list);
}

void PrintVolumeSummary(out_ctx* ctx, const VolumeSummary* summary)
{
    BeginSection  (ctx, "Volume Summary");
    PrintUI             (ctx, summary, nodeCount);
    PrintUI             (ctx, summary, recordCount);
    PrintUI             (ctx, summary, fileCount);
    PrintUI             (ctx, summary, folderCount);
    PrintUI             (ctx, summary, aliasCount);
    PrintUI             (ctx, summary, hardLinkFileCount);
    PrintUI             (ctx, summary, hardLinkFolderCount);
    PrintUI             (ctx, summary, symbolicLinkCount);
    PrintUI             (ctx, summary, invisibleFileCount);
    PrintUI             (ctx, summary, emptyFileCount);
    PrintUI             (ctx, summary, emptyDirectoryCount);

    BeginSection        (ctx, "Data Fork");
    PrintForkSummary    (ctx, &summary->dataFork);
    EndSection(ctx);

    BeginSection        (ctx, "Resource Fork");
    PrintForkSummary    (ctx, &summary->resourceFork);
    EndSection(ctx);

    BeginSection  (ctx, "Largest Files");
    print("# %10s %10s", "Size", "CNID");
    for (unsigned i = 9; i > 0; i--) {
        if (summary->largestFiles[i].cnid == 0) continue;

        char       size[50];
        (void)format_size(ctx, size, summary->largestFiles[i].measure, 50);
        hfs_wc_str name = L"";
        HFSPlusGetCNIDName(name, (FSSpec){volume_, summary->largestFiles[i].cnid});
        print("%d %10s %10u %ls", 10-i, size, summary->largestFiles[i].cnid, name);
    }
    EndSection(ctx); // largest files

    EndSection(ctx); // volume summary
}

void PrintForkSummary(out_ctx* ctx, const ForkSummary* summary)
{
    PrintUI             (ctx, summary, count);
    PrintAttribute(ctx, "fragmentedCount", "%llu (%0.2f)", summary->fragmentedCount, (float)summary->fragmentedCount/(float)summary->count);
//    PrintUI             (ctx, summary, fragmentedCount);
    PrintHFSBlocks      (ctx, summary, blockCount);
    PrintDataLength     (ctx, summary, logicalSpace);
    PrintUI             (ctx, summary, extentRecords);
    PrintUI             (ctx, summary, extentDescriptors);
    PrintUI             (ctx, summary, overflowExtentRecords);
    PrintUI             (ctx, summary, overflowExtentDescriptors);
}

#pragma mark Structure Visualization Functions

void VisualizeHFSPlusExtentKey(out_ctx* ctx, const HFSPlusExtentKey* record, const char* label, bool oneLine)
{
    if (oneLine) {
        Print(ctx, "%s: %s:%-6u; %s:%-4u; %s:%-3u; %s:%-10u; %s:%-10u",
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
        Print(ctx, "%s", label);
        Print(ctx, "+-----------------------------------------------+");
        Print(ctx, "| %-6s | %-4s | %-3s | %-10s | %-10s |",
              "length",
              "fork",
              "pad",
              "fileID",
              "startBlock");
        Print(ctx, "| %-6u | %-4u | %-3u | %-10u | %-10u |",
              record->keyLength,
              record->forkType,
              record->pad,
              record->fileID,
              record->startBlock);
        Print(ctx, "+-----------------------------------------------+");
    }
}

void VisualizeHFSPlusCatalogKey(out_ctx* ctx, const HFSPlusCatalogKey* record, const char* label, bool oneLine)
{
    hfs_wc_str name;
    hfsuctowcs(name, &record->nodeName);
    if (oneLine) {
        Print(ctx, "%s: %s:%-6u; %s:%-10u; %s:%-6u; %s:%-50ls",
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
        char* names  = "| %-6s | %-10s | %-6s | %-50s |";
        char* format = "| %-6u | %-10u | %-6u | %-50ls |";
        char* line_f =  "+%s+";
        char  dashes[90];
        memset(dashes, '-', 83);

        Print(ctx, "%s", label);
        Print(ctx, line_f, dashes);
        Print(ctx, names, "length", "parentID", "length", "nodeName");
        Print(ctx, format, record->keyLength, record->parentID, record->nodeName.length, name);
        Print(ctx, line_f, dashes);
    }
}

void VisualizeHFSPlusAttrKey(out_ctx* ctx, const HFSPlusAttrKey* record, const char* label, bool oneLine)
{
    HFSUniStr255 hfsName;
    hfsName.length = record->attrNameLen;
    memset(&hfsName.unicode, 0, 255);
    memcpy(&hfsName.unicode, &record->attrName, record->attrNameLen * sizeof(uint16_t));

    hfs_wc_str name;
    hfsuctowcs(name, &hfsName);
    if (oneLine) {
        Print(ctx, "%s: %s = %-6u; %s = %-10u; %s = %-10u; %s = %-6u; %s = %-50ls",
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
        char* names      = "| %-6s | %-10s | %-10s | %-6s | %-50s |";
        char* format     = "| %-6u | %-10u | %-10u | %-6u | %-50ls |";

        char* line_f     =  "+%s+";
        char  dashes[97] = {'\0'};
        memset(dashes, '-', 96);
        dashes[96] = '\0';

        Print(ctx, "%s", label);
        Print(ctx, line_f, dashes);
        Print(ctx, names, "length", "fileID", "startBlock", "length", "nodeName");
        Print(ctx, format, record->keyLength, record->fileID, record->startBlock, record->attrNameLen, name);
        Print(ctx, line_f, dashes);
    }
}

void VisualizeHFSBTreeNodeRecord(out_ctx* ctx, const char* label, BTHeaderRec headerRecord, const BTreeNodePtr node, BTRecNum recNum)
{
    BTNodeRecord record = {0};
    BTGetBTNodeRecord(&record, node, recNum);

    char*        names  = "| %-9s | %-6s | %-6s | %-10s | %-12s |";
    char*        format = "| %-9u | %-6u | %-6u | %-10u | %-12u |";

    char*        line_f =  "+%s+";
    char         dashes[60];
    memset(dashes, '-', 57);

    Print(ctx, "%s", label);
    Print(ctx, line_f, dashes);
    Print(ctx, names, "record_id", "offset", "length", "key_length", "value_length");
    Print(ctx, format, recNum, record.offset, record.recordLen, record.keyLen, record.valueLen);
    Print(ctx, line_f, dashes);
    Print(ctx, "Key data:");
    VisualizeData((char*)record.key, MIN(record.recordLen, record.keyLen));
    Print(ctx, "Value data:");
    VisualizeData((char*)record.value, MIN(record.recordLen, record.valueLen));
}

#pragma mark Node Print Functions

void PrintTreeNode(out_ctx* ctx, const BTreePtr tree, uint32_t nodeID)
{
    debug("PrintTreeNode");

    BTreeNodePtr node = NULL;
    if ( BTGetNode(&node, tree, nodeID) < 0) {
        error("node %u is invalid or out of range.", nodeID);
        return;
    }
    if (node == NULL) {
        Print(ctx, "(unused node)");
        return;
    }
    PrintNode(ctx, node);
    btree_free_node(node);
}

void PrintNode(out_ctx* ctx, const BTreeNodePtr node)
{
    debug("PrintNode");

    BeginSection(ctx, "Node %u (offset %llu; length: %zu)", node->nodeNumber, node->nodeOffset, node->nodeSize);
    PrintBTNodeDescriptor(ctx, node->nodeDescriptor);

//    uint16_t *records = ((uint16_t*)(node->data + node->bTree->headerRecord.nodeSize) - node->nodeDescriptor->numRecords);
//    for (unsigned i = node->nodeDescriptor->numRecords-1; i >= 0; --i) {
//        char label[50] = "";
//        sprintf(label, "Record Offset #%u", node->nodeDescriptor->numRecords - i);
//        PrintAttribute(ctx, label, "%u", records[i]);
//    }

    for (unsigned recordNumber = 0; recordNumber < node->nodeDescriptor->numRecords; recordNumber++) {
        PrintNodeRecord(ctx, node, recordNumber);
    }
    EndSection(ctx);
}

void PrintFolderListing(out_ctx* ctx, uint32_t folderID)
{
    debug("Printing listing for folder ID %d", folderID);

    // CNID kind mode user group data rsrc name
    char         lineStr[110] = {0};
    memset(lineStr, '-', 100);
    char*        headerFormat = "%-9s %-10s %-10s %-9s %-9s %-15s %-15s %s";
    char*        rowFormat    = "%-9u %-10s %-10s %-9d %-9d %-15s %-15s %ls";

    // Search for thread record
    FSSpec       spec         = { .parentID = folderID };
    BTreeNodePtr node         = NULL;
    BTRecNum     recordID     = 0;

    if (hfs_catalog_find_record(&node, &recordID, spec) < 0) {
        error("No thread record for %d found.", folderID);
        return;
    }

    debug("Found thread record %d:%d", node->nodeNumber, recordID);

    // Get thread record
    HFSPlusCatalogKey*    recordKey     = NULL;
    HFSPlusCatalogRecord* catalogRecord = NULL;
    btree_get_record((void*)&recordKey, (void*)&catalogRecord, node, recordID);

    hfs_wc_str            name;
    hfsuctowcs(name, &catalogRecord->catalogThread.nodeName);
    uint32_t              threadID = recordKey->parentID;

    struct {
        uint64_t fileCount;
        uint64_t folderCount;
        uint64_t hardLinkCount;
        uint64_t symlinkCount;
        uint64_t dataForkCount;
        uint64_t dataForkSize;
        uint64_t rsrcForkCount;
        uint64_t rsrcForkSize;
    } folderStats = {0};

    // Start output
    BeginSection(ctx, "Listing for %ls", name);
    Print(ctx, headerFormat, "CNID", "kind", "mode", "user", "group", "data", "rsrc", "name");

    // Loop over siblings until NULL
    while (1) {
        // Locate next record, even if it's in the next node.
        if (++recordID >= node->recordCount) {
            debug("Done with records in node %d", node->nodeNumber);
            bt_nodeid_t nextNode = node->nodeDescriptor->fLink;
            BTreePtr    tree     = node->bTree;
            btree_free_node(node);
            if ( BTGetNode(&node, tree, nextNode) < 0) { error("couldn't get the next catalog node"); return; }
            recordID = 0;
        }

        btree_get_record((void*)&recordKey, (void*)&catalogRecord, node, recordID);

        if (threadID == recordKey->parentID) {
            debug("Looking at record %d", recordID);

            if ((catalogRecord->record_type == kHFSPlusFileRecord) || (catalogRecord->record_type == kHFSPlusFolderRecord)) {
                uint32_t cnid         = 0;
                char     kind[10];
                char     mode[11];
                int      user         = -1, group = -1;
                char     dataSize[20] = {'-', '\0'};
                char     rsrcSize[20] = {'-', '\0'};

                hfsuctowcs(name, &recordKey->nodeName);

                if ( HFSPlusCatalogRecordIsHardLink(catalogRecord) && HFSPlusCatalogRecordIsAlias(catalogRecord) ) {
                    folderStats.hardLinkCount++;
                    (void)strlcpy(kind, "dir link", 10);
                } else if (HFSPlusCatalogRecordIsHardLink(catalogRecord)) {
                    folderStats.hardLinkCount++;
                    (void)strlcpy(kind, "hard link", 10);
                } else if (HFSPlusCatalogRecordIsSymLink(catalogRecord)) {
                    folderStats.symlinkCount++;
                    (void)strlcpy(kind, "symlink", 10);
                } else if (catalogRecord->record_type == kHFSPlusFolderRecord) {
                    folderStats.folderCount++;
                    (void)strlcpy(kind, "folder", 10);
                } else {
                    folderStats.fileCount++;
                    (void)strlcpy(kind, "file", 10);
                }

                // Files and folders share these attributes at the same locations.
                cnid  = catalogRecord->catalogFile.fileID;
                user  = catalogRecord->catalogFile.bsdInfo.ownerID;
                group = catalogRecord->catalogFile.bsdInfo.groupID;
                _genModeString(mode, catalogRecord->catalogFile.bsdInfo.fileMode);

                if (catalogRecord->record_type == kHFSPlusFileRecord) {
                    if (catalogRecord->catalogFile.dataFork.logicalSize)
                        (void)format_size(ctx, dataSize, catalogRecord->catalogFile.dataFork.logicalSize, 50);

                    if (catalogRecord->catalogFile.resourceFork.logicalSize)
                        (void)format_size(ctx, rsrcSize, catalogRecord->catalogFile.resourceFork.logicalSize, 50);

                    if (catalogRecord->catalogFile.dataFork.totalBlocks > 0) {
                        folderStats.dataForkCount++;
                        folderStats.dataForkSize += catalogRecord->catalogFile.dataFork.logicalSize;
                    }

                    if (catalogRecord->catalogFile.resourceFork.totalBlocks > 0) {
                        folderStats.rsrcForkCount++;
                        folderStats.rsrcForkSize += catalogRecord->catalogFile.resourceFork.logicalSize;
                    }
                }

                Print(ctx, rowFormat, cnid, kind, mode, user, group, dataSize, rsrcSize, name);
            }
        } else {
            break;
        } // parentID == parentID
    }     // while(1)

    char dataTotal[50];
    char rsrcTotal[50];

    format_size(ctx, dataTotal, folderStats.dataForkSize, 50);
    format_size(ctx, rsrcTotal, folderStats.rsrcForkCount, 50);

    Print(ctx, "%s", lineStr);
    Print(ctx, headerFormat, "", "", "", "", "", dataTotal, rsrcTotal, "");

    Print(ctx, "%10s: %-10d %10s: %-10d %10s: %-10d",
          "Folders",
          folderStats.folderCount,
          "Data forks",
          folderStats.dataForkCount,
          "Hard links",
          folderStats.hardLinkCount
          );

    Print(ctx, "%10s: %-10d %10s: %-10d %10s: %-10d",
          "Files",
          folderStats.fileCount,
          "RSRC forks",
          folderStats.rsrcForkCount,
          "Symlinks",
          folderStats.symlinkCount
          );

    btree_free_node(node);

    EndSection(ctx);

    debug("Done listing.");
}

void PrintNodeRecord(out_ctx* ctx, const BTreeNodePtr node, int recordNumber)
{
    debug("PrintNodeRecord");

    if(recordNumber >= node->nodeDescriptor->numRecords) return;

    BTNodeRecord    _record = {0};
    BTNodeRecordPtr record  = &_record;
    BTGetBTNodeRecord(record, node, recordNumber);

    if (record->recordLen == 0) {
        error("Invalid record: no data found.");
        return;
    }

    if (node->nodeDescriptor->kind == kBTIndexNode) {
        if (node->bTree->treeID == kHFSExtentsFileID) {
            if (recordNumber == 0) {
                BeginSection(ctx, "Extent Tree Index Records");
                Print(ctx, "%-3s %-12s %-12s %-4s   %-12s %-12s", "#", "Node ID", "Key Length", "Fork", "File ID", "Start Block");
            }
            bt_nodeid_t       next_node = *(bt_nodeid_t*)record->value;
            HFSPlusExtentKey* key       = (HFSPlusExtentKey*)record->key;
            Print(ctx, "%-3u %-12u %-12u 0x%02x   %-12u %-12u", recordNumber, next_node, key->keyLength, key->forkType, key->fileID, key->startBlock);
            return;

        } else if (node->bTree->treeID == kHFSCatalogFileID) {
            if (recordNumber == 0) {
                BeginSection(ctx, "Catalog Tree Index Records");
                Print(ctx, "%-3s %-12s %-5s %-12s %s", "#", "nodeID", "kLen", "parentID", "nodeName");
            }
            bt_nodeid_t        next_node = *(bt_nodeid_t*)record->value;
            HFSPlusCatalogKey* key       = (HFSPlusCatalogKey*)record->key;
            hfs_wc_str         nodeName;
            hfsuctowcs(nodeName, &key->nodeName);
            Print(ctx, "%-3u %-12u %-5u %-12u %ls", recordNumber, next_node, key->keyLength, key->parentID, nodeName);
            return;
        }

    }

    BeginSection(ctx, "Record ID %u (%u/%u) (length: %zd) (Node %d)",
                 recordNumber,
                 recordNumber + 1,
                 node->nodeDescriptor->numRecords,
                 record->recordLen,
                 node->nodeNumber
                 );

    switch (node->nodeDescriptor->kind) {
        case kBTHeaderNode:
        {
            switch (recordNumber) {
                case 0:
                {
                    PrintAttribute(ctx, "recordType", "BTHeaderRec");
                    BTHeaderRec header = *( (BTHeaderRec*) record->record );
                    PrintBTHeaderRecord(ctx, &header);
                    break;
                }

                case 1:
                {
                    PrintAttribute(ctx, "recordType", "userData (reserved)");
                    VisualizeData(record->record, record->recordLen);
                    break;
                }

                case 2:
                {
                    PrintAttribute(ctx, "recordType", "mapData");
                    VisualizeData(record->record, record->recordLen);
                    break;
                }
            }
            break;
        }

        case kBTMapNode:
        {
            PrintAttribute(ctx, "recordType", "mapData");
            VisualizeData(record->record, record->recordLen);
            break;
        }

        default:
        {
            // Handle all keyed node records
            switch (node->bTree->treeID) {
                case kHFSCatalogFileID:
                {
                    HFSPlusCatalogKey* keyStruct = (HFSPlusCatalogKey*) record->key;
                    VisualizeHFSPlusCatalogKey(ctx, keyStruct, "Catalog Key", 0);

                    if ((record->keyLen < kHFSPlusCatalogKeyMinimumLength) || (record->keyLen > kHFSPlusCatalogKeyMaximumLength))
                        goto INVALID_KEY;

                    break;
                };

                case kHFSExtentsFileID:
                {
                    HFSPlusExtentKey keyStruct = *( (HFSPlusExtentKey*) record->key );
                    VisualizeHFSPlusExtentKey(ctx, &keyStruct, "Extent Key", 0);

                    if ( (record->keyLen - sizeof(keyStruct.keyLength)) != kHFSPlusExtentKeyMaximumLength)
                        goto INVALID_KEY;

                    break;
                };

                case kHFSAttributesFileID:
                {
                    HFSPlusAttrKey* keyStruct = (HFSPlusAttrKey*) record->key;
                    VisualizeHFSPlusAttrKey(ctx, keyStruct, "Attributes Key", 0);

                    if ((record->keyLen < kHFSPlusAttrKeyMinimumLength) || (record->keyLen > kHFSPlusAttrKeyMaximumLength))
                        goto INVALID_KEY;

                    break;
                }

                default:
                    goto INVALID_KEY;
            }

            switch (node->nodeDescriptor->kind) {
                case kBTIndexNode:
                {
                    uint32_t* pointer = (uint32_t*) record->value;
                    PrintAttribute(ctx, "nextNodeID", "%llu", *pointer);
                    break;
                }

                case kBTLeafNode:
                {
                    switch (node->bTree->treeID) {
                        case kHFSCatalogFileID:
                        {
                            uint16_t type = ((HFSPlusCatalogRecord*)record->value)->record_type;

                            switch (type) {
                                case kHFSPlusFileRecord:
                                {
                                    PrintHFSPlusCatalogFile(ctx, (HFSPlusCatalogFile*)record->value);
                                    break;
                                }

                                case kHFSPlusFolderRecord:
                                {
                                    PrintHFSPlusCatalogFolder(ctx, (HFSPlusCatalogFolder*)record->value);
                                    break;
                                }

                                case kHFSPlusFileThreadRecord:
                                {
                                    PrintHFSPlusFileThreadRecord(ctx, (HFSPlusCatalogThread*)record->value);
                                    break;
                                }

                                case kHFSPlusFolderThreadRecord:
                                {
                                    PrintHFSPlusFolderThreadRecord(ctx, (HFSPlusCatalogThread*)record->value);
                                    break;
                                }

                                default:
                                {
                                    PrintAttribute(ctx, "recordType", "%u (invalid)", type);
                                    VisualizeData(record->value, record->valueLen);
                                    break;
                                }
                            }
                            break;
                        }

                        case kHFSExtentsFileID:
                        {
                            HFSPlusExtentRecord* extentRecord = (HFSPlusExtentRecord*)record->value;
                            ExtentList*          list         = extentlist_make();
                            extentlist_add_record(list, *extentRecord);
                            PrintExtentList(ctx, list, 0);
                            extentlist_free(list);
                            break;
                        }

                        case kHFSAttributesFileID:
                        {
                            PrintHFSPlusAttrRecord(ctx, (HFSPlusAttrRecord*)record->value);
                        }
                    }

                    break;
                }

                default:
                {
INVALID_KEY:
                    if ((recordNumber + 1) == node->nodeDescriptor->numRecords) {
                        PrintAttribute(ctx, "recordType", "(free space)");
                    } else {
                        PrintAttribute(ctx, "recordType", "(unknown b-tree/record format)");
                        VisualizeData(record->record, record->recordLen);
                    }
                }
            }

            break;
        }
    }

    EndSection(ctx);
}

int format_hfs_timestamp(out_ctx* ctx, char* out, uint32_t timestamp, size_t length)
{
    if (timestamp > MAC_GMT_FACTOR) {
        timestamp -= MAC_GMT_FACTOR;
    } else {
        timestamp = 0; // Cannot be negative.
    }

    return format_time(ctx, out, timestamp, length);
}

int format_hfs_chars(out_ctx* ctx, char* out, const char* value, size_t nbytes, size_t length)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    while (nbytes--) *out++ = value[nbytes];
    *out++      = '\0';
#else
    memcpy(out, value, nbytes);
    out[nbytes] = '\0';
#endif
    return strlen(out);
}

