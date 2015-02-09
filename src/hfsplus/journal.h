//
//  journal.h
//  hfsinspect
//
//  Created by Adam Knight on 11/30/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs/hfs.h"
#include "hfs/Apple/hfs_types.h"
#include "volumes/output.h"

void PrintJournalInfoBlock          (out_ctx* ctx, const JournalInfoBlock* record) __attribute__((nonnull));
void PrintJournalHeader             (out_ctx* ctx, const journal_header* record) __attribute__((nonnull));
