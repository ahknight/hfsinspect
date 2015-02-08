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

#include "hfsinspect/output.h"
#include "hfs/output_hfs.h"

void PrintJournalInfoBlock(const JournalInfoBlock* record)
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

    BeginSection("Journal Info Block");
    PrintRawAttribute(record, flags, 2);
    PrintUIFlagIfMatch(record->flags, kJIJournalInFSMask);
    PrintUIFlagIfMatch(record->flags, kJIJournalOnOtherDeviceMask);
    PrintUIFlagIfMatch(record->flags, kJIJournalNeedInitMask);
    _PrintRawAttribute("device_signature", &record->device_signature[0], 32, 16);
    PrintDataLength(record, offset);
    PrintDataLength(record, size);

    char uuid_str[sizeof(uuid_string_t) + 1];
    (void)strlcpy(uuid_str, &record->ext_jnl_uuid[0], sizeof(uuid_str));
    PrintAttribute("ext_jnl_uuid", uuid_str);

    char serial[49];
    (void)strlcpy(serial, &record->machine_serial_num[0], 49);
    PrintAttribute("machine_serial_num", serial);

    // (uint32_t) reserved[32]

    EndSection();
}

void PrintJournalHeader(const journal_header* record)
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

