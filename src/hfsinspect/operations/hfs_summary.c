//
//  hfs_summary.c
//  hfsinspect
//
//  Created by Adam Knight on 4/1/14.
//  Copyright (c) 2014 Adam Knight. All rights reserved.
//

#include "operations.h"
#include "volumes/utilities.h"     // commonly-used utility functions


int compare_ranked_files(const void* a, const void* b)
{
    const Rank* A      = (Rank*)a;
    const Rank* B      = (Rank*)b;

    int         result = cmp(A->measure, B->measure);

    return result;
}

VolumeSummary generateVolumeSummary(HIOptions* options)
{
    /*
       Walk the leaf catalog nodes and gather various stats about the volume as a whole.
     */

    VolumeSummary summary = {0};
    HFS*          hfs     = options->hfs;

    BTreePtr      catalog = NULL;
    hfs_get_catalog_btree(&catalog, hfs);
    hfs_cnid_t    cnid    = catalog->headerRecord.firstLeafNode;

    while (1) {
        BTreeNodePtr node = NULL;
        if ( BTGetNode(&node, catalog, cnid) < 0) {
            perror("get node");
            die(1, "There was an error fetching node %d", cnid);
        }

        // Process node
        debug("Processing node %d", cnid); summary.nodeCount++;

        FOR_UNTIL(recNum, node->nodeDescriptor->numRecords) {

            summary.recordCount++;

            BTreeKeyPtr           recordKey   = NULL;
            uint8_t*              recordValue = NULL;
            btree_get_record(&recordKey, &recordValue, node, recNum);

            HFSPlusCatalogRecord* record      = (HFSPlusCatalogRecord*)recordValue;
            switch (record->record_type) {
                case kHFSPlusFileRecord:
                {
                    summary.fileCount++;

                    HFSPlusCatalogFile* file = &record->catalogFile;

                    // hard links
                    if (HFSPlusCatalogFileIsHardLink(record)) { summary.hardLinkFileCount++; continue; }
                    if (HFSPlusCatalogFolderIsHardLink(record)) { summary.hardLinkFolderCount++; continue; }

                    // symlink
                    if (HFSPlusCatalogRecordIsSymLink(record)) { summary.symbolicLinkCount++; continue; }

                    // alias
                    if (HFSPlusCatalogRecordIsAlias(record)) { summary.aliasCount++; continue; }

                    // invisible
                    if (file->userInfo.fdFlags & kIsInvisible) { summary.invisibleFileCount++; continue; }

                    // file sizes
                    if ((file->dataFork.logicalSize == 0) && (file->resourceFork.logicalSize == 0)) { summary.emptyFileCount++; continue; }

                    if (file->dataFork.logicalSize)
                        generateForkSummary(options, &summary.dataFork, file, &file->dataFork, HFSDataForkType);

                    if (file->resourceFork.logicalSize)
                        generateForkSummary(options, &summary.resourceFork, file, &file->resourceFork, HFSResourceForkType);

                    size_t fileSize = file->dataFork.logicalSize + file->resourceFork.logicalSize;

                    if (summary.largestFiles[0].measure < fileSize) {
                        summary.largestFiles[0].measure = fileSize;
                        summary.largestFiles[0].cnid    = file->fileID;
                        qsort(summary.largestFiles, 10, sizeof(Rank), compare_ranked_files);
                    }

                    break;
                }

                case kHFSPlusFolderRecord:
                {
                    summary.folderCount++;

                    HFSPlusCatalogFolder* folder = &record->catalogFolder;
                    if (folder->valence == 0) summary.emptyDirectoryCount++;

                    break;
                }

                default:
                {
                    break;
                }
            }

        }

        // Follow link
        cnid = node->nodeDescriptor->fLink;
        if (cnid == 0)   break;        // End Of Line

        // Console Status
        static int      count     = 0; // count of records so far
        static unsigned iter_size = 0; // update the status after this many records
        if (iter_size == 0) {
            iter_size = (catalog->headerRecord.leafRecords/100000);
            if (iter_size == 0) iter_size = 100;
        }

        count += node->nodeDescriptor->numRecords;
        //        if (count >= 100000) break;

        if ((count % iter_size) == 0) {
            // Update status
            size_t space     = summary.dataFork.logicalSpace + summary.resourceFork.logicalSpace;
            char   size[128] = {0};
            (void)format_size(options->hfs->vol->ctx, size, space, 128);

            fprintf(stdout, "\r%0.2f%% (files: %ju; directories: %ju; size: %s)",
                    ((float)count / (float)catalog->headerRecord.leafRecords) * 100.,
                    (intmax_t)summary.fileCount,
                    (intmax_t)summary.folderCount,
                    size
                    );
            fflush(stdout);
        }

        btree_free_node(node);
    }

    return summary;
}

void generateForkSummary(HIOptions* options, ForkSummary* forkSummary, const HFSPlusCatalogFile* file, const HFSPlusForkData* fork, hfs_forktype_t type)
{
    forkSummary->count++;

    forkSummary->blockCount   += fork->totalBlocks;
    forkSummary->logicalSpace += fork->logicalSize;

    forkSummary->extentRecords++;

    if (fork->extents[1].blockCount > 0) forkSummary->fragmentedCount++;

    for (int i = 0; i < kHFSPlusExtentDensity; i++) {
        if (fork->extents[i].blockCount > 0) forkSummary->extentDescriptors++; else break;
    }

    HFSFork* hfsfork;
    if ( hfsfork_make(&hfsfork, options->hfs, *fork, type, file->fileID) < 0 ) {
        die(1, "Could not create fork reference for fileID %u", file->fileID);
    }

    ExtentList* extents = hfsfork->extents;

    unsigned    counter = 1;

    Extent*     e       = NULL;
    TAILQ_FOREACH(e, extents, extents) {
        forkSummary->overflowExtentDescriptors++;
        if (counter && (counter == kHFSPlusExtentDensity) ) {
            forkSummary->overflowExtentRecords++;
            counter = 1;
        } else {
            counter++;
        }
    }

    hfsfork_free(hfsfork);
}

