//
//  journal.h
//  hfsinspect
//
//  Created by Adam Knight on 11/30/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfs/hfs.h"
#include "hfs/Apple/hfs_types.h"

void PrintJournalInfoBlock          (const JournalInfoBlock* record) __attribute__((nonnull));
void PrintJournalHeader             (const journal_header* record) __attribute__((nonnull));
