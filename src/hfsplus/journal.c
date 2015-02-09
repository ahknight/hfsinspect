//
//  journal.c
//  hfsinspect
//
//  Created by Adam Knight on 11/30/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "hfsplus/journal.h"

#include <string.h>         // memcpy, strXXX, etc.
#if defined(__linux__)
#include <bsd/string.h>     // strlcpy, etc.
#endif

#include "hfs/output_hfs.h"

void PrintJournalInfoBlock(out_ctx* ctx, const JournalInfoBlock* record)
{
    /*
       struct JournalInfoBlock {
         uint32_t       flags;
         uint32_t       device_signature[8];  // signature used to locate our device.
         uint64_t       offset;               // byte offset to the journal on the device
         uint64_t       size;                 // size in bytes of the journal
         uuid_string_t   ext_jnl_uuid;
         char            machine_serial_num[48];
         char     reserved[JIB_RESERVED_SIZE];
       } __attribute__((aligned(2), packed));
       typedef struct JournalInfoBlock JournalInfoBlock;
     */

    BeginSection(ctx, "Journal Info Block");
    PrintRawAttribute(ctx, record, flags, 2);
    PrintUIFlagIfMatch(ctx, record->flags, kJIJournalInFSMask);
    PrintUIFlagIfMatch(ctx, record->flags, kJIJournalOnOtherDeviceMask);
    PrintUIFlagIfMatch(ctx, record->flags, kJIJournalNeedInitMask);
    _PrintRawAttribute(ctx, "device_signature", &record->device_signature[0], 32, 16);
    PrintDataLength(ctx, record, offset);
    PrintDataLength(ctx, record, size);

    char uuid_str[sizeof(uuid_string_t) + 1];
    (void)strlcpy(uuid_str, &record->ext_jnl_uuid[0], sizeof(uuid_str));
    PrintAttribute(ctx, "ext_jnl_uuid", uuid_str);

    char serial[49];
    (void)strlcpy(serial, &record->machine_serial_num[0], 49);
    PrintAttribute(ctx, "machine_serial_num", serial);

    // (uint32_t) reserved[32]

    EndSection(ctx);
}

void PrintJournalHeader(out_ctx* ctx, const journal_header* record)
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
    BeginSection(ctx, "Journal Header");
    PrintUIChar(ctx, record, magic);
    PrintUIHex(ctx, record, endian);
    PrintUI(ctx, record, start);
    PrintUI(ctx, record, end);
    PrintDataLength(ctx, record, size);
    PrintDataLength(ctx, record, blhdr_size);
    PrintUIHex(ctx, record, checksum);
    PrintDataLength(ctx, record, jhdr_size);
    PrintUI(ctx, record, sequence_num);
    EndSection(ctx);
}

