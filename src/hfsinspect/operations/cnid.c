//
//  cnid.c
//  hfsinspect
//
//  Created by Adam Knight on 4/1/14.
//  Copyright (c) 2014 Adam Knight. All rights reserved.
//

#include "operations.h"

void showCatalogRecord(HIOptions* options, FSSpec spec, bool followThreads)
{
    hfs_wc_str filename_str = {0};
    out_ctx*   ctx          = options->hfs->vol->ctx;

    hfsuctowcs(filename_str, &spec.name);
    debug("Finding catalog record for %d:%ls", spec.parentID, filename_str);

    HFSPlusCatalogRecord catalogRecord = {0};
    if ( HFSPlusGetCatalogRecordByFSSpec(&catalogRecord, spec) < 0 ) {
        error("FSSpec {%u, '%ls'} not found.", spec.parentID, filename_str);
        return;
    }

    int type = catalogRecord.record_type;

    if ((type == kHFSPlusFileThreadRecord) || (type == kHFSPlusFolderThreadRecord)) {
        if (followThreads) {
            debug("Following thread for %d:%ls", spec.parentID, filename_str);
            FSSpec s = {
                .hfs      = spec.hfs,
                .parentID = catalogRecord.catalogThread.parentID,
                .name     = catalogRecord.catalogThread.nodeName
            };
            showCatalogRecord(options, s, followThreads);
            return;

        } else {
            if ((type == kHFSPlusFolderThreadRecord) && check_mode(options, HIModeListFolder)) {
                PrintFolderListing(ctx, spec.parentID);
            } else {
                BTreeNodePtr node = NULL; BTRecNum recordID = 0;
                hfs_catalog_find_record(&node, &recordID, spec);
                PrintNodeRecord(ctx, node, recordID);
            }
        }

    } else if (type == kHFSPlusFileRecord) {
        // Check for hard link
        if ((catalogRecord.catalogFile.userInfo.fdCreator == kHardLinkFileType) && (catalogRecord.catalogFile.userInfo.fdType == kHFSPlusCreator)) {
            print("Record is a hard link.");
        }

        // Check for soft link
        if ((catalogRecord.catalogFile.userInfo.fdCreator == kSymLinkCreator) && (catalogRecord.catalogFile.userInfo.fdType == kSymLinkFileType)) {
            print("Record is a symbolic link.");
        }

        // Display record
        BTreeNodePtr node = NULL; BTRecNum recordID = 0;
        hfs_catalog_find_record(&node, &recordID, spec);
        PrintNodeRecord(ctx, node, recordID);

        // Set extract file
        options->extract_HFSPlusCatalogFile = &catalogRecord.catalogFile;

    } else if (type == kHFSPlusFolderRecord) {
        if (check_mode(options, HIModeListFolder)) {
            PrintFolderListing(ctx, catalogRecord.catalogFolder.folderID);
        } else {
            BTreeNodePtr node = NULL; BTRecNum recordID = 0;
            hfs_catalog_find_record(&node, &recordID, spec);
            PrintNodeRecord(ctx, node, recordID);
        }

    } else {
        error("Unknown record type: %d", type);
    }
}

