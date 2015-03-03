//
//  path_info.c
//  hfsinspect
//
//  Created by Adam Knight on 4/1/14.
//  Copyright (c) 2014 Adam Knight. All rights reserved.
//

#include "operations.h"


void showPathInfo(HIOptions* options)
{
    debug("Finding records for path: %s", options->file_path);

    if (strlen(options->file_path) == 0) {
        error("Path is required.");
        exit(1);
    }

    FSSpec               spec          = {0};
    HFSPlusCatalogRecord catalogRecord = {0};

    if ( HFSPlusGetCatalogInfoByPath(&spec, &catalogRecord, options->file_path, options->hfs) < 0) {
        error("Path not found: %s", options->file_path);
        return;
    }
    hfs_str name = "";
    hfsuc_to_str(&name, &spec.name);
    debug("Showing catalog record for %d:%s.", spec.parentID, name);
    showCatalogRecord(options, spec, false);

    if (catalogRecord.record_type == kHFSFileRecord) {
        options->extract_HFSPlusCatalogFile = &catalogRecord.catalogFile;
    }
}

