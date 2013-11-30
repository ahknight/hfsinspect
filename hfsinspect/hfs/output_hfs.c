//
//  output_hfs.c
//  hfsinspect
//
//  Created by Adam Knight on 5/5/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "output_hfs.h"

#include "hfs_structs.h"
#include "hfs_catalog_ops.h"
#include "output.h"
#include "stringtools.h"
#include "hfs.h"
#include <sys/stat.h>

static HFS *volume = NULL;
void set_hfs_volume(HFS *v) { volume = v; }



#pragma mark Value Print Functions

void _PrintCatalogName(char* label, bt_nodeid_t cnid)
{
    hfs_wc_str name;
    if (cnid != 0)
        hfs_catalog_get_cnid_name(name, volume, cnid);
    else
        name[0] = L'\0';
    
    PrintAttribute(label, "%d (%ls)", cnid, name);
}

void _PrintHFSBlocks(const char *label, uint64_t blocks)
{
    char sizeLabel[50];
    (void)format_blocks(sizeLabel, blocks, volume->block_size, 50);
    PrintAttribute(label, sizeLabel);
}

void _PrintHFSTimestamp(const char* label, uint32_t timestamp)
{
    char buf[50];
    (void)format_hfs_timestamp(buf, timestamp, 50);
    PrintAttribute(label, buf);
}

void _PrintHFSChar(const char* label, const char* i, size_t nbytes)
{
    char str[50];
    char hex[50];
    
    (void)format_hfs_chars(str, i, nbytes, 50);
    (void)format_dump(hex, i, 16, nbytes, 50);
    
    PrintAttribute(label, "0x%s (%s)", hex, str);
}

void _PrintHFSUniStr255(const char* label, const HFSUniStr255 *record)
{
    hfs_wc_str wide;
    hfsuctowcs(wide, record);
    PrintAttribute(label, "\"%ls\" (%u)", wide, record->length);
}


#pragma mark Structure Print Functions

void PrintVolumeInfo(const HFS* hfs)
{
    if (hfs->vh.signature == kHFSPlusSigWord)
        BeginSection("HFS+ Volume Format (v%d)", hfs->vh.version);
    else if (hfs->vh.signature == kHFSXSigWord)
        BeginSection("HFSX Volume Format (v%d)", hfs->vh.version);
    else
        BeginSection("Unknown Volume Format"); // Curious.
    
    hfs_wc_str volumeName;
    int success = hfs_catalog_get_cnid_name(volumeName, hfs, kHFSRootFolderID);
    if (success)
        PrintAttribute("volume name", "%ls", volumeName);
    
    BTree catalog;
    hfs_get_catalog_btree(&catalog, hfs);
    
    if (hfs->vh.signature == kHFSXSigWord && catalog.headerRecord.keyCompareType == kHFSBinaryCompare) {
        PrintAttribute("case sensitivity", "case sensitive");
    } else {
        PrintAttribute("case sensitivity", "case insensitive");
    }
    
    HFSPlusVolumeFinderInfo* finderInfo = (HFSPlusVolumeFinderInfo*)&hfs->vh.finderInfo;
    if (finderInfo->bootDirID || finderInfo->bootParentID || finderInfo->os9DirID || finderInfo->osXDirID) {
        PrintAttribute("bootable", "yes");
    } else {
        PrintAttribute("bootable", "no");
    }
    EndSection();
}

void PrintHFSMasterDirectoryBlock(const HFSMasterDirectoryBlock* vcb)
{
    BeginSection("HFS Master Directory Block");
    
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
    PrintAttribute("drVN", "%s", name);
    
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
    EndSection();
}

void PrintVolumeHeader(const HFSPlusVolumeHeader *vh)
{
    BeginSection("HFS Plus Volume Header");
    PrintHFSChar        (vh, signature);
    PrintUI             (vh, version);

	PrintRawAttribute   (vh, attributes, 2);
    PrintUIFlagIfMatch  (vh->attributes, kHFSVolumeHardwareLockMask);
    PrintUIFlagIfMatch  (vh->attributes, kHFSVolumeUnmountedMask);
    PrintUIFlagIfMatch  (vh->attributes, kHFSVolumeSparedBlocksMask);
    PrintUIFlagIfMatch  (vh->attributes, kHFSVolumeNoCacheRequiredMask);
    PrintUIFlagIfMatch  (vh->attributes, kHFSBootVolumeInconsistentMask);
    PrintUIFlagIfMatch  (vh->attributes, kHFSCatalogNodeIDsReusedMask);
    PrintUIFlagIfMatch  (vh->attributes, kHFSVolumeJournaledMask);
    PrintUIFlagIfMatch  (vh->attributes, kHFSVolumeInconsistentMask);
    PrintUIFlagIfMatch  (vh->attributes, kHFSVolumeSoftwareLockMask);
    PrintUIFlagIfMatch  (vh->attributes, kHFSUnusedNodeFixMask);
    PrintUIFlagIfMatch  (vh->attributes, kHFSContentProtectionMask);
    PrintUIFlagIfMatch  (vh->attributes, kHFSMDBAttributesMask);
    
    PrintHFSChar        (vh, lastMountedVersion);
	PrintUI             (vh, journalInfoBlock);
    PrintHFSTimestamp   (vh, createDate);
    PrintHFSTimestamp   (vh, modifyDate);
    PrintHFSTimestamp   (vh, backupDate);
    PrintHFSTimestamp   (vh, checkedDate);
    PrintUI             (vh, fileCount);
	PrintUI             (vh, folderCount);
	PrintDataLength     (vh, blockSize);
	PrintHFSBlocks      (vh, totalBlocks);
	PrintHFSBlocks      (vh, freeBlocks);
    PrintUI             (vh, nextAllocation);
    PrintDataLength     (vh, rsrcClumpSize);
    PrintDataLength     (vh, dataClumpSize);
    PrintUI             (vh, nextCatalogID);
    PrintUI             (vh, writeCount);
    
    PrintRawAttribute   (vh, encodingsBitmap, 2);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacRoman);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacJapanese);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacChineseTrad);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacKorean);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacArabic);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacHebrew);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacGreek);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacCyrillic);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacDevanagari);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacGurmukhi);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacGujarati);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacOriya);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacBengali);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacTamil);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacTelugu);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacKannada);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacMalayalam);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacSinhalese);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacBurmese);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacKhmer);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacThai);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacLaotian);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacGeorgian);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacArmenian);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacChineseSimp);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacTibetan);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacMongolian);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacEthiopic);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacCentralEurRoman);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacVietnamese);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacExtArabic);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacSymbol);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacDingbats);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacTurkish);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacCroatian);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacIcelandic);
    PrintUIFlagIfSet    (vh->encodingsBitmap, kTextEncodingMacRomanian);
    if (vh->encodingsBitmap & ((uint64_t)1 << 49)) PrintAttribute(NULL, "%s (%u)", "kTextEncodingMacFarsi", 49);
    if (vh->encodingsBitmap & ((uint64_t)1 << 48)) PrintAttribute(NULL, "%s (%u)", "kTextEncodingMacUkrainian", 48);

    BeginSection("Finder Info");
    
    HFSPlusVolumeFinderInfo* finderInfo = (HFSPlusVolumeFinderInfo*)&vh->finderInfo;
    
    PrintCatalogName    (finderInfo, bootDirID);
    PrintCatalogName    (finderInfo, bootParentID);
    PrintCatalogName    (finderInfo, openWindowDirID);
    PrintCatalogName    (finderInfo, os9DirID);
    PrintRawAttribute   (finderInfo, reserved, 2);
    PrintCatalogName    (finderInfo, osXDirID);
    PrintRawAttribute   (finderInfo, volID, 16);
    
    EndSection();
    
    BeginSection("Allocation Bitmap File");
    PrintHFSPlusForkData(&vh->allocationFile, kHFSAllocationFileID, HFSDataForkType);
    EndSection();
    
    BeginSection("Extents Overflow File");
    PrintHFSPlusForkData(&vh->extentsFile, kHFSExtentsFileID, HFSDataForkType);
    EndSection();
    
    BeginSection("Catalog File");
    PrintHFSPlusForkData(&vh->catalogFile, kHFSCatalogFileID, HFSDataForkType);
    EndSection();
    
    BeginSection("Attributes File");
    PrintHFSPlusForkData(&vh->attributesFile, kHFSAttributesFileID, HFSDataForkType);
    EndSection();
    
    BeginSection("Startup File");
    PrintHFSPlusForkData(&vh->startupFile, kHFSStartupFileID, HFSDataForkType);
    EndSection();
}

void PrintExtentList(const ExtentList* list, uint32_t totalBlocks)
{
    PrintAttribute("extents", "%12s %12s %12s", "startBlock", "blockCount", "% of file");
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
            PrintAttribute("", "%12u %12u %12.2f", e->startBlock, e->blockCount, pct);
        } else {
            PrintAttribute("", "%12u %12u %12s", e->startBlock, e->blockCount, "?");
        }
    }
    
    char sumLine[50];
    memset(sumLine, '-', 38);
    PrintAttribute("", sumLine);
    
    if (totalBlocks) {
        PrintAttribute("", "%4d extents %12d %12.2f", usedExtents, catalogBlocks, total);
    } else {
        PrintAttribute("", "%12s %12d %12s", "", catalogBlocks, "?");
    }
    
//    PrintAttribute("", "%d allocation blocks in %d extents total.", catalogBlocks, usedExtents);
    PrintAttribute("", "%0.2f blocks per extent on average.", ( (float)catalogBlocks / (float)usedExtents) );
}

void PrintForkExtentsSummary(const HFSFork* fork)
{
    info("Printing extents summary");
    
    PrintExtentList(fork->extents, fork->totalBlocks);
}

void PrintHFSPlusForkData(const HFSPlusForkData *fork, uint32_t cnid, uint8_t forktype)
{
    if (forktype == HFSDataForkType) {
        PrintAttribute("fork", "data");
    } else if (forktype == HFSResourceForkType) {
        PrintAttribute("fork", "resource");
    }
    if (fork->logicalSize == 0) {
        PrintAttribute("logicalSize", "(empty)");
        return;
    }
    
    PrintDataLength (fork, logicalSize);
    PrintDataLength (fork, clumpSize);
    PrintHFSBlocks  (fork, totalBlocks);
    
    if (fork->totalBlocks) {
        HFSFork *hfsfork;
        if ( hfsfork_make(&hfsfork, volume, *fork, forktype, cnid) < 0 ) {
            critical("Could not create fork for fileID %u", cnid);
            return;
        }
        PrintForkExtentsSummary(hfsfork);
        hfsfork_free(hfsfork);
    }
}

void PrintBTNodeDescriptor(const BTNodeDescriptor *node)
{
    BeginSection("Node Descriptor");
    PrintUI(node, fLink);
    PrintUI(node, bLink);
    PrintConstIfEqual(node->kind, kBTLeafNode);
    PrintConstIfEqual(node->kind, kBTIndexNode);
    PrintConstIfEqual(node->kind, kBTHeaderNode);
    PrintConstIfEqual(node->kind, kBTMapNode);
    PrintUI(node, height);
    PrintUI(node, numRecords);
    PrintUI(node, reserved);
    EndSection();
}

void PrintBTHeaderRecord(const BTHeaderRec *hr)
{
    BeginSection("Header Record");
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
    
    PrintConstIfEqual(hr->btreeType, kBTHFSTreeType);
    PrintConstIfEqual(hr->btreeType, kBTUserTreeType);
    PrintConstIfEqual(hr->btreeType, kBTReservedTreeType);
    
    PrintConstHexIfEqual(hr->keyCompareType, kHFSCaseFolding);
    PrintConstHexIfEqual(hr->keyCompareType, kHFSBinaryCompare);
    
	PrintRawAttribute(hr, attributes, 2);
    PrintUIFlagIfMatch(hr->attributes, kBTBadCloseMask);
    PrintUIFlagIfMatch(hr->attributes, kBTBigKeysMask);
    PrintUIFlagIfMatch(hr->attributes, kBTVariableIndexKeysMask);
        
    EndSection();
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
    
    return strlen(modeString);
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
        uint8_t flag = record->adminFlags;
        if (flag & flagMasks[i]) {
            PrintAttribute(NULL, "%05o %s", flagMasks[i], flagNames[i]);
        }
    }
    
    PrintUIOct(record, ownerFlags);
    
    for (int i = 0; i < 10; i++) {
        uint8_t flag = record->ownerFlags;
        if (flag & flagMasks[i]) {
            PrintAttribute(NULL, "%05o %s", flagMasks[i], flagNames[i]);
        }
    }
        
    uint16_t mode = record->fileMode;
    
    char modeString[11];
    _genModeString(modeString, mode);
    PrintAttribute("fileMode", modeString);
    
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
    PrintHFSChar(record, fdType);
    PrintHFSChar(record, fdCreator);
    PrintRawAttribute(record, fdFlags, 2);
    PrintFinderFlags(record->fdFlags);
    PrintAttribute("fdLocation", "(%u, %u)", record->fdLocation.v, record->fdLocation.h);
    PrintUI(record, opaque);
}

void PrintFndrDirInfo(const FndrDirInfo *record)
{
    PrintAttribute("frRect", "(%u, %u, %u, %u)", record->frRect.top, record->frRect.left, record->frRect.bottom, record->frRect.right);
    PrintRawAttribute(record, frFlags, 2);
    PrintFinderFlags    (record->frFlags);
    PrintAttribute("frLocation", "(%u, %u)", record->frLocation.v, record->frLocation.h);
    PrintUI             (record, opaque);
}

void PrintFinderFlags(uint16_t record)
{
    uint16_t kIsOnDesk             = 0x0001;     /* Files and folders (System 6) */
    uint16_t kRequireSwitchLaunch  = 0x0020;     /* Old */
    uint16_t kColor                = 0x000E;     /* Files and folders */
    uint16_t kIsShared             = 0x0040;     /* Files only (Applications only) If */
    uint16_t kHasNoINITs           = 0x0080;     /* Files only (Extensions/Control */
    uint16_t kHasBeenInited        = 0x0100;     /* Files only.  Clear if the file */
    uint16_t kHasCustomIcon        = 0x0400;     /* Files and folders */
    uint16_t kIsStationery         = 0x0800;     /* Files only */
    uint16_t kNameLocked           = 0x1000;     /* Files and folders */
    uint16_t kHasBundle            = 0x2000;     /* Files only */
    uint16_t kIsInvisible          = 0x4000;     /* Files and folders */
    uint16_t kIsAlias              = 0x8000;     /* Files only */
    
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
    PrintAttribute("recordType", "kHFSPlusFolderRecord");
    
    PrintRawAttribute(record, flags, 2);
    PrintConstIfEqual(record->flags, kHFSFileLockedMask);
    PrintConstIfEqual(record->flags, kHFSThreadExistsMask);
    PrintConstIfEqual(record->flags, kHFSHasAttributesMask);
    PrintConstIfEqual(record->flags, kHFSHasSecurityMask);
    PrintConstIfEqual(record->flags, kHFSHasFolderCountMask);
    PrintConstIfEqual(record->flags, kHFSHasLinkChainMask);
    PrintConstIfEqual(record->flags, kHFSHasChildLinkMask);
    PrintConstIfEqual(record->flags, kHFSHasDateAddedMask);

    PrintUI(record, valence);
    PrintUI(record, folderID);
    PrintHFSTimestamp(record, createDate);
    PrintHFSTimestamp(record, contentModDate);
    PrintHFSTimestamp(record, attributeModDate);
    PrintHFSTimestamp(record, accessDate);
    PrintHFSTimestamp(record, backupDate);
    PrintHFSPlusBSDInfo(&record->bsdInfo);

    BeginSection("Finder Info");
    PrintFndrDirInfo(&record->userInfo);
    PrintFndrOpaqueInfo(&record->finderInfo);
    EndSection();
    
    PrintUI(record, textEncoding);
    PrintUI(record, folderCount);
}

void PrintHFSPlusCatalogFile(const HFSPlusCatalogFile *record)
{
    PrintAttribute("recordType", "kHFSPlusFileRecord");
    PrintRawAttribute(record, flags, 2);
    PrintUIFlagIfMatch(record->flags, kHFSFileLockedMask);
    PrintUIFlagIfMatch(record->flags, kHFSThreadExistsMask);
    PrintUIFlagIfMatch(record->flags, kHFSHasAttributesMask);
    PrintUIFlagIfMatch(record->flags, kHFSHasSecurityMask);
    PrintUIFlagIfMatch(record->flags, kHFSHasFolderCountMask);
    PrintUIFlagIfMatch(record->flags, kHFSHasLinkChainMask);
    PrintUIFlagIfMatch(record->flags, kHFSHasChildLinkMask);
    PrintUIFlagIfMatch(record->flags, kHFSHasDateAddedMask);

    PrintUI                 (record, reserved1);
    PrintUI                 (record, fileID);
    PrintHFSTimestamp       (record, createDate);
    PrintHFSTimestamp       (record, contentModDate);
    PrintHFSTimestamp       (record, attributeModDate);
    PrintHFSTimestamp       (record, accessDate);
    PrintHFSTimestamp       (record, backupDate);
    PrintHFSPlusBSDInfo     (&record->bsdInfo);
    
    BeginSection("Finder Info");
    PrintFndrFileInfo       (&record->userInfo);
    PrintFndrOpaqueInfo     (&record->finderInfo);
    EndSection();
    
    PrintUI                 (record, textEncoding);
    PrintUI                 (record, reserved2);
    
    BeginSection("Data Fork");
    PrintHFSPlusForkData(&record->dataFork, kHFSCatalogFileID, HFSDataForkType);
    EndSection();
    
    if (record->resourceFork.logicalSize) {
        BeginSection("Resource Fork");
        PrintHFSPlusForkData(&record->resourceFork, kHFSCatalogFileID, HFSResourceForkType);
        EndSection();
    }
}

void PrintHFSPlusFolderThreadRecord(const HFSPlusCatalogThread *record)
{
    PrintAttribute       ("recordType", "kHFSPlusFolderThreadRecord");
    PrintHFSPlusCatalogThread   (record);
}

void PrintHFSPlusFileThreadRecord(const HFSPlusCatalogThread *record)
{
    PrintAttribute       ("recordType", "kHFSPlusFileThreadRecord");
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
    PrintUIHex          (record, recordType);
    PrintHFSPlusForkData(&record->theFork, 0, 0);
}

void PrintHFSPlusAttrExtents(const HFSPlusAttrExtents* record)
{
    PrintUIHex          (record, recordType);
    PrintHFSPlusExtentRecord(&record->extents);
}

void PrintHFSPlusAttrData(const HFSPlusAttrData* record)
{
    PrintUIHex          (record, recordType);
    PrintUI             (record, attrSize);
    
    VisualizeData((char*)&record->attrData, record->attrSize);
}

void PrintHFSPlusAttrRecord(const HFSPlusAttrRecord* record)
{
    switch (record->recordType) {
        case kHFSPlusAttrInlineData:
            PrintHFSPlusAttrData(&record->attrData);
            break;
            
        case kHFSPlusAttrForkData:
            PrintHFSPlusAttrForkData(&record->forkData);
            break;
            
        case kHFSPlusAttrExtents:
            PrintHFSPlusAttrExtents(&record->overflowExtents);
            break;
            
        default:
            error("unknown attribute record type: %d", record->recordType);
            VisualizeData((char*)record, sizeof(HFSPlusAttrRecord));
            break;
    }
}

void PrintHFSPlusExtentRecord(const HFSPlusExtentRecord* record)
{
    ExtentList *list = extentlist_make();
    extentlist_add_record(list, *record);
    PrintExtentList(list, 0);
    extentlist_free(list);
}

void PrintJournalInfoBlock(const JournalInfoBlock *record)
{
    /*
     struct JournalInfoBlock {
     uint32_t       flags;
     uint32_t       device_signature[8];  // signature used to locate our device.
     uint64_t       offset;               // byte offset to the journal on the device
     uint64_t       size;                 // size in bytes of the journal
     uuid_string_t   ext_jnl_uuid;
     char            machine_serial_num[48];
     char    	reserved[JIB_RESERVED_SIZE];
     } __attribute__((aligned(2), packed));
     typedef struct JournalInfoBlock JournalInfoBlock;
     */
    
    BeginSection("Journal Info Block");
    PrintRawAttribute(record, flags, 2);
    PrintUIFlagIfMatch(record->flags, kJIJournalInFSMask);
    PrintUIFlagIfMatch(record->flags, kJIJournalOnOtherDeviceMask);
    PrintUIFlagIfMatch(record->flags, kJIJournalNeedInitMask);
    _PrintRawAttribute("device_signature", &record->device_signature[0], 32, 16);
    PrintDataLength(record, offset);
    PrintDataLength(record, size);

    char uuid_str[sizeof(uuid_string_t) + 1];
    strlcpy(uuid_str, &record->ext_jnl_uuid[0], sizeof(uuid_str));
    PrintAttribute("ext_jnl_uuid", uuid_str);
    
    char serial[49];
    strlcpy(serial, &record->machine_serial_num[0], 49);
    PrintAttribute("machine_serial_num", serial);
    
    // (uint32_t) reserved[32]
    
    EndSection();
}

void PrintJournalHeader(const journal_header *record)
{
    /*
     int32_t        magic;
     int32_t        endian;
     volatile off_t start;         // zero-based byte offset of the start of the first transaction
     volatile off_t end;           // zero-based byte offset of where free space begins
     off_t          size;          // size in bytes of the entire journal
     int32_t        blhdr_size;    // size in bytes of each block_list_header in the journal
     int32_t        checksum;
     int32_t        jhdr_size;     // block size (in bytes) of the journal header
     uint32_t       sequence_num;  // NEW FIELD: a monotonically increasing value assigned to all txn's
*/
    BeginSection("Journal Header");
    PrintHFSChar(record, magic);
    PrintUIHex(record, endian);
    PrintUI(record, start);
    PrintUI(record, end);
    PrintDataLength(record, size);
    PrintDataLength(record, blhdr_size);
    PrintUIHex(record, checksum);
    PrintDataLength(record, jhdr_size);
    PrintUI(record, sequence_num);
    EndSection();
}

void PrintVolumeSummary(const VolumeSummary *summary)
{
    BeginSection  ("Volume Summary");
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
    
    BeginSection        ("Data Fork");
    PrintForkSummary    (&summary->dataFork);
    EndSection();
    
    BeginSection        ("Resource Fork");
    PrintForkSummary    (&summary->resourceFork);
    EndSection();
    
    BeginSection  ("Largest Files");
    print("# %10s %10s", "Size", "CNID");
    for (int i = 9; i > 0; i--) {
        if (summary->largestFiles[i].cnid == 0) continue;
        
        char size[50];
        (void)format_size(size, summary->largestFiles[i].measure, false, 50);
        hfs_wc_str name = L"";
        hfs_catalog_get_cnid_name(name, volume, summary->largestFiles[i].cnid);
        print("%d %10s %10u %ls", 10-i, size, summary->largestFiles[i].cnid, name);
    }
    EndSection(); // largest files
    
    EndSection(); // volume summary
}

void PrintForkSummary(const ForkSummary *summary)
{
    PrintUI             (summary, count);
    PrintAttribute("fragmentedCount", "%llu (%0.2f)", summary->fragmentedCount, (float)summary->fragmentedCount/(float)summary->count);
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
        Print("%s: %s:%-6u; %s:%-4u; %s:%-3u; %s:%-10u; %s:%-10u",
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
        Print("%s", label);
        Print("+-----------------------------------------------+");
        Print("| %-6s | %-4s | %-3s | %-10s | %-10s |",
               "length",
               "fork",
               "pad",
               "fileID",
               "startBlock");
        Print("| %-6u | %-4u | %-3u | %-10u | %-10u |",
               record->keyLength,
               record->forkType,
               record->pad,
               record->fileID,
               record->startBlock);
        Print("+-----------------------------------------------+");
    }
}

void VisualizeHFSPlusCatalogKey(const HFSPlusCatalogKey *record, const char* label, bool oneLine)
{
    hfs_wc_str name;
    hfsuctowcs(name, &record->nodeName);
    if (oneLine) {
        Print("%s: %s:%-6u; %s:%-10u; %s:%-6u; %s:%-50ls",
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
        char* names     = "| %-6s | %-10s | %-6s | %-50s |";
        char* format    = "| %-6u | %-10u | %-6u | %-50ls |";
        char* line_f    =  "+%s+";
        char  dashes[90];
        memset(dashes, '-', 83);
        
        Print("%s", label);
        Print(line_f, dashes);
        Print(names, "length", "parentID", "length", "nodeName");
        Print(format, record->keyLength, record->parentID, record->nodeName.length, name);
        Print(line_f, dashes);
    }
}

void VisualizeHFSPlusAttrKey(const HFSPlusAttrKey *record, const char* label, bool oneLine)
{
    HFSUniStr255 hfsName;
    hfsName.length = record->attrNameLen;
    memset(&hfsName.unicode, 0, 255);
    memcpy(&hfsName.unicode, &record->attrName, record->attrNameLen * sizeof(uint16_t));
    
    hfs_wc_str name;
    hfsuctowcs(name, &hfsName);
    if (oneLine) {
        Print("%s: %s = %-6u; %s = %-10u; %s = %-10u; %s = %-6u; %s = %-50ls",
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
        char* names     = "| %-6s | %-10s | %-10s | %-6s | %-50s |";
        char* format    = "| %-6u | %-10u | %-10u | %-6u | %-50ls |";
        
        char* line_f    =  "+%s+";
        char dashes[100];
        memset(dashes, '-', 96);
        
        Print("%s", label);
        Print(line_f, dashes);
        Print(names, "length", "fileID", "startBlock", "length", "nodeName");
        Print(format, record->keyLength, record->fileID, record->startBlock, record->attrNameLen, name);
        Print(line_f, dashes);
    }
}

void VisualizeHFSBTreeNodeRecord(const BTreeRecord* record, const char* label)
{
    char* names     = "| %-9s | %-6s | %-6s | %-10s | %-12s |";
    char* format    = "| %-9u | %-6u | %-6u | %-10u | %-12u |";
    
    char* line_f    =  "+%s+";
    char dashes[60];
    memset(dashes, '-', 57);
    
    Print("%s", label);
    Print(line_f, dashes);
    Print(names, "record_id", "offset", "length", "key_length", "value_length");
    Print(format, record->recordID, record->offset, record->length, record->keyLength, record->valueLength);
    Print(line_f, dashes);
    Print("Key data:");
    VisualizeData((char*)record->key, record->keyLength);
    Print("Value data:");
    VisualizeData(record->value, record->valueLength);
}


#pragma mark Node Print Functions

void PrintTreeNode(const BTree *tree, uint32_t nodeID)
{
    debug("PrintTreeNode");
    
    BTreeNode* node;
    if ( btree_get_node(&node, tree, nodeID) < 0) {
        error("node %u is invalid or out of range.", nodeID);
        return;
    }
    PrintNode(node);
    btree_free_node(node);
}

void PrintNode(const BTreeNode* node)
{
    debug("PrintNode");
    
    BeginSection("Node %u (offset %llu; length: %zu)", node->nodeNumber, node->nodeOffset, node->dataLen);
    PrintBTNodeDescriptor(&node->nodeDescriptor);
    
    for (int recordNumber = 0; recordNumber < node->nodeDescriptor.numRecords; recordNumber++) {
        PrintNodeRecord(node, recordNumber);
    }
    EndSection();
}

void PrintFolderListing(uint32_t folderID)
{
    debug("Printing listing for folder ID %d", folderID);

    // CNID kind mode user group data rsrc name
    char lineStr[110];
    memset(lineStr, '-', 100);
    char* headerFormat  = "%-9s %-10s %-10s %-9s %-9s %-15s %-15s %s";
    char* rowFormat     = "%-9u %-10s %-10s %-9d %-9d %-15s %-15s %ls";
    
    // Get thread record
    BTreeNode* node = NULL;
    bt_recordid_t recordID = 0;
    HFSPlusCatalogKey key;
    key.parentID = folderID;
    key.nodeName.length = 0;
    key.nodeName.unicode[0] = '\0';
    key.nodeName.unicode[1] = '\0';
    
    BTree tree;
    hfs_get_catalog_btree(&tree, volume);
    
    if ( btree_search_tree(&node, &recordID, &tree, &key) < 1 ) {
        error("No thread record for %d found.");
    }
    
    debug("Found thread record %d:%d", node->nodeNumber, recordID);

    HFSPlusCatalogKey *recordKey = (HFSPlusCatalogKey*)node->records[recordID].key;
    HFSPlusCatalogThread *threadRecord = (HFSPlusCatalogThread*)node->records[recordID].value;
    hfs_wc_str name;
    hfsuctowcs(name, &threadRecord->nodeName);
    uint32_t threadID = recordKey->parentID;
    
    struct {
        unsigned    fileCount;
        unsigned    folderCount;
        unsigned    hardLinkCount;
        unsigned    symlinkCount;
        unsigned    dataForkCount;
        size_t      dataForkSize;
        unsigned    rsrcForkCount;
        size_t      rsrcForkSize;
    } folderStats = {0};
    
    // Start output
    BeginSection("Listing for %ls", name);
    Print(headerFormat, "CNID", "kind", "mode", "user", "group", "data", "rsrc", "name");

    // Loop over siblings until NULL
    BTreeRecord *record = &node->records[recordID];
    while (1) {
        while ( (record = hfs_catalog_next_in_folder(record)) != NULL ) {
            debug("Looking at record %d", record->recordID);
            HFSPlusCatalogRecord* catalogRecord = (HFSPlusCatalogRecord*)record->value;
            
            if (catalogRecord->record_type == kHFSPlusFileRecord || catalogRecord->record_type == kHFSPlusFolderRecord) {
                uint32_t cnid = 0;
                char kind[10];
                char mode[11];
                int user = -1, group = -1;
                char dataSize[20] = {'-', '\0'};
                char rsrcSize[20] = {'-', '\0'};
                
                recordKey = (HFSPlusCatalogKey*)record->key;
                hfsuctowcs(name, &recordKey->nodeName);
                
                if ( hfs_catalog_record_is_hard_link(catalogRecord) && hfs_catalog_record_is_alias(catalogRecord) ) {
                    folderStats.hardLinkCount++;
                    strlcpy(kind, "dir link", 10);
                } else if (hfs_catalog_record_is_hard_link(catalogRecord)) {
                    folderStats.hardLinkCount++;
                    strlcpy(kind, "hard link", 10);
                } else if (hfs_catalog_record_is_symbolic_link(catalogRecord)) {
                    folderStats.symlinkCount++;
                    strlcpy(kind, "symlink", 10);
                } else if (catalogRecord->record_type == kHFSPlusFolderRecord) {
                    folderStats.folderCount++;
                    strlcpy(kind, "folder", 10);
                } else {
                    folderStats.fileCount++;
                    strlcpy(kind, "file", 10);
                }
                
                // Files and folders share these attributes at the same locations.
                cnid        = catalogRecord->catalogFile.fileID;
                user        = catalogRecord->catalogFile.bsdInfo.ownerID;
                group       = catalogRecord->catalogFile.bsdInfo.groupID;
                _genModeString(mode, catalogRecord->catalogFile.bsdInfo.fileMode);
                
                if (catalogRecord->record_type == kHFSPlusFileRecord) {
                    if (catalogRecord->catalogFile.dataFork.logicalSize)
                        (void)format_size(dataSize, catalogRecord->catalogFile.dataFork.logicalSize, false, 50);
                    
                    if (catalogRecord->catalogFile.resourceFork.logicalSize)
                        (void)format_size(rsrcSize, catalogRecord->catalogFile.resourceFork.logicalSize, false, 50);
                    
                    if (catalogRecord->catalogFile.dataFork.totalBlocks > 0) {
                        folderStats.dataForkCount++;
                        folderStats.dataForkSize += catalogRecord->catalogFile.dataFork.logicalSize;
                    }

                    if (catalogRecord->catalogFile.resourceFork.totalBlocks > 0) {
                        folderStats.rsrcForkCount++;
                        folderStats.rsrcForkSize += catalogRecord->catalogFile.resourceFork.logicalSize;
                    }
                }
                
                Print(rowFormat, cnid, kind, mode, user, group, dataSize, rsrcSize, name);
            }
            recordID = record->recordID;
        }
        debug("Done with records in node %d", node->nodeNumber);
        
        // The NULL may have been an end-of-node mark.  Check for this and break if that's not the case.
        if ( (recordID + 1) < (node->recordCount - 1)) {
            debug("Search ended before the end of the node. Abandoning further searching. (%d/%d)", recordID + 1, node->recordCount - 1);
            break;
        }
        
        // Now we look at the next node in the chain and see if that's in it.
        uint32_t nextNode = node->nodeDescriptor.fLink;
        btree_free_node(node);
        
        if ( btree_get_node(&node, &tree, nextNode) < 0 ) {
            error("Failed to get node %d", nextNode);
            return;
        }
        
        HFSPlusCatalogKey *nextKey = (HFSPlusCatalogKey*)node->records[0].key;
        if (nextKey->parentID != threadID) {
            debug("First record's parent is %d; we're looking for %d or %d", nextKey->parentID, folderID, threadID);
            break;
        }
        record = &node->records[0];
        debug("Checking node %d", node->nodeNumber);
    }
    
    char dataTotal[50];
    char rsrcTotal[50];
    
    format_size(dataTotal, folderStats.dataForkSize, false, 50);
    format_size(rsrcTotal, folderStats.rsrcForkCount, false, 50);
    
    Print("%s", lineStr);
    Print(headerFormat, "", "", "", "", "", dataTotal, rsrcTotal, "");
    
    Print("%10s: %-10d %10s: %-10d %10s: %-10d",
           "Folders",
           folderStats.folderCount,
           "Data forks",
           folderStats.dataForkCount,
           "Hard links",
           folderStats.hardLinkCount
           );
    
    Print("%10s: %-10d %10s: %-10d %10s: %-10d",
           "Files",
           folderStats.fileCount,
           "RSRC forks",
           folderStats.rsrcForkCount,
           "Symlinks",
           folderStats.symlinkCount
           );
    
    btree_free_node(node);
    
    EndSection();
    
    debug("Done listing.");
}

void PrintNodeRecord(const BTreeNode* node, int recordNumber)
{
    debug("PrintNodeRecord");
    
    if(recordNumber >= node->nodeDescriptor.numRecords) return;
    
    const BTreeRecord *record = &node->records[recordNumber];
    
    uint16_t offset = record->offset;
    if (offset > node->bTree.headerRecord.nodeSize) {
        error("Invalid record offset: points beyond end of node: %u", offset);
        return;
    }
    
    if (record->length == 0) {
        error("Invalid record: no data found.");
        return;
    }
    
    if (node->nodeDescriptor.kind == kBTIndexNode) {
        if (node->bTree.treeID == kHFSExtentsFileID) {
            if (recordNumber == 0) {
                BeginSection("Extent Tree Index Records");
                Print("%-3s %-12s %-12s %-4s   %-12s %-12s", "#", "Node ID", "Key Length", "Fork", "File ID", "Start Block");
            }
            bt_nodeid_t next_node = *(bt_nodeid_t*)record->value;
            HFSPlusExtentKey* key = (HFSPlusExtentKey*)record->key;
            Print("%-3u %-12u %-12u 0x%02x   %-12u %-12u", recordNumber, next_node, key->keyLength, key->forkType, key->fileID, key->startBlock);
            return;
        } else if (node->bTree.treeID == kHFSCatalogFileID) {
            if (recordNumber == 0) {
                BeginSection("Catalog Tree Index Records");
                Print("%-3s %-12s %-5s %-12s %s", "#", "nodeID", "kLen", "parentID", "nodeName");
            }
            bt_nodeid_t next_node = *(bt_nodeid_t*)record->value;
            HFSPlusCatalogKey* key = (HFSPlusCatalogKey*)record->key;
            hfs_wc_str nodeName;
            hfsuctowcs(nodeName, &key->nodeName);
            Print("%-3u %-12u %-5u %-12u %ls", recordNumber, next_node, key->keyLength, key->parentID, nodeName);
            return;
        }

    }
    
    BeginSection("Record ID %u (%u/%u) (offset %u; length: %zd) (Node %d)",
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
                    PrintAttribute("recordType", "BTHeaderRec");
                    BTHeaderRec header = *( (BTHeaderRec*) record->record );
                    PrintBTHeaderRecord(&header);
                    break;
                }
                case 1:
                {
                    PrintAttribute("recordType", "userData (reserved)");
                    break;
                }
                case 2:
                {
                    PrintAttribute("recordType", "mapData");
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
            switch (node->bTree.treeID) {
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
                    HFSPlusAttrKey *keyStruct = (HFSPlusAttrKey*) record->key;
                    if (record->keyLength < kHFSPlusAttrKeyMinimumLength || record->keyLength > kHFSPlusAttrKeyMaximumLength)
                        goto INVALID_KEY;
                    
                    VisualizeHFSPlusAttrKey(keyStruct, "Attributes Key", 0);
                    
                    break;
                }
                    
                default:
                    // TODO: Attributes file support.
                    goto INVALID_KEY;
            }
            
            switch (node->nodeDescriptor.kind) {
                case kBTIndexNode:
                {
                    uint32_t *pointer = (uint32_t*) record->value;
                    PrintAttribute("nextNodeID", "%llu", *pointer);
                    break;
                }
                    
                case kBTLeafNode:
                {
                    switch (node->bTree.treeID) {
                        case kHFSCatalogFileID:
                        {
                            uint16_t type =  hfs_get_catalog_record_type(record);
                            
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
                                    PrintAttribute("recordType", "%u (invalid)", type);
                                    VisualizeData(record->value, record->valueLength);
                                    break;
                                }
                            }
                            break;
                        }
                        case kHFSExtentsFileID:
                        {
                            HFSPlusExtentRecord *extentRecord = (HFSPlusExtentRecord*)record->value;
                            ExtentList *list = extentlist_make();
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
                        PrintAttribute("recordType", "(free space)");
                    } else {
                        PrintAttribute("recordType", "(unknown b-tree/record format)");
                        VisualizeData(record->record, record->length);
                    }
                }
            }
            
            break;
        }
    }
    
    EndSection();
}

int format_hfs_timestamp(char* out, uint32_t timestamp, size_t length)
{
    if (timestamp > MAC_GMT_FACTOR) {
        timestamp -= MAC_GMT_FACTOR;
    } else {
        timestamp = 0; // Cannot be negative.
    }
    
    return format_time(out, timestamp, length);
}

int format_hfs_chars(char* out, const char* value, size_t nbytes, size_t length)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    while (nbytes--) *out++ = value[nbytes];
    *out++ = '\0';
#else
    memcpy(out, value, nbytes);
    out[nbytes] = '\0';
#endif
    return strlen(out);
}
