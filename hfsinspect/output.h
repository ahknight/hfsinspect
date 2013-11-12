//
//  output.h
//  hfsinspect
//
//  Created by Adam Knight on 11/10/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_output_h
#define hfsinspect_output_h

#define MEMBER(s, m) #m, "%llu", s->m

void _PrintRawAttribute         (const char* label, const void* map, size_t size, char base);
void _PrintDataLength           (const char* label, uint64_t size);

#define _PrintUI(label, value)                  PrintAttributeString(label, "%llu", (uint64_t)value);
#define PrintUI(record, value)                  _PrintUI(#value, record->value)

#define _PrintUIOct(label, value)               PrintAttributeString(label, "%06o", value)
#define PrintUIOct(record, value)               _PrintUIOct(#value, record->value)

#define _PrintUIHex(label, value)               PrintAttributeString(label, "%#x", value)
#define PrintUIHex(record, value)               _PrintUIHex(#value, record->value)

#define PrintRawAttribute(record, value, base)  _PrintRawAttribute(#value, &(record->value), sizeof(record->value), base)
#define PrintBinaryDump(record, value)          PrintRawAttribute(record, value, 2)
#define PrintOctalDump(record, value)           PrintRawAttribute(record, value, 8)
#define PrintHexDump(record, value)             PrintRawAttribute(record, value, 16)

#define PrintOctFlag(label, value)              PrintSubattributeString("%06o (%s)", value, label)
#define PrintHexFlag(label, value)              PrintSubattributeString("%s (%#x)", label, value)
#define PrintIntFlag(label, value)              PrintSubattributeString("%s (%llu)", label, (uint64_t)value)

#define PrintDataLength(record, value)          _PrintDataLength(#value, (uint64_t)record->value)

#define PrintUIFlagIfSet(source, flag)          { if (((uint64_t)(source)) & (((uint64_t)1) << ((uint64_t)(flag)))) PrintIntFlag(#flag, flag); }

#define PrintUIFlagIfMatch(source, flag)        { if ((source) & flag) PrintIntFlag(#flag, flag); }
#define PrintUIOctFlagIfMatch(source, flag)     { if ((source) & flag) PrintOctFlag(#flag, flag); }
#define PrintUIHexFlagIfMatch(source, flag)     { if ((source) & flag) PrintHexFlag(#flag, flag); }

#define PrintConstIfEqual(source, c)            { if ((source) == c)   PrintIntFlag(#c, c); }
#define PrintConstOctIfEqual(source, c)         { if ((source) == c)   PrintOctFlag(#c, c); }
#define PrintConstHexIfEqual(source, c)         { if ((source) == c)   PrintHexFlag(#c, c); }

void PrintString                    (const char* label, const char* value_format, ...);
void PrintHeaderString              (const char* value_format, ...);
void PrintAttributeString           (const char* label, const char* value_format, ...);
void PrintSubattributeString        (const char* str, ...);
void VisualizeData                  (const char* data, size_t length);


/*
 OutputSectionStart("HFS+ Volume Format (v%d)", 4);
 OutputAttribute("volume name", "Macintosh HD");
 OutputAttribute("case sensitivity", "case insensitive");
 OutputAttribute("bootable", "yes");

 OutputSectionStart("Record ID %d (%d/%d) (offset %d; length: %d) (Node %d)", 4);

OutputAttribute("fileMode", "100644", NULL);
OutputAttribute(NULL, "100000", "(S_IFREG)");

OutputAttributeSize("Size", 1024);

OutputAttributeRaw("fileMode",   record->fileMode,               8, 6, NULL);
OutputAttributeRaw(NULL,         (record->fileMode & S_IFMT),    8, 6, "S_IFREG");

# HFS+ Volume Format (v4)
  volume name            = Macintosh HD
  case sensitivity       = case insensitive
  bootable               = yes

    # Record ID 3 (4/9) (offset 870; length: 288) (Node 15776)

      Catalog Key
     +-----------------------------------------------------------------------------------+
     | length | parentID   | length | nodeName                                           |
     | 38     | 126366704  | 16     | attributes-btree                                   |
     +-----------------------------------------------------------------------------------+
      recordType             = kHFSPlusFileRecord
      flags                  = 00
                             . kHFSThreadExistsMask
                             . kHFSHasDateAddedMask
      reserved1              = 0
      fileID                 = 179319212
      createDate             = Mon Nov 11 19:21:06 2013 UTC
      contentModDate         = Mon Nov 11 19:22:29 2013 UTC
      attributeModDate       = Mon Nov 11 19:22:29 2013 UTC
      accessDate             = Mon Nov 11 21:27:12 2013 UTC
      backupDate             = Thu Jan  1 00:00:00 1970 UTC
      ownerID                = 501
      groupID                = 20
      adminFlags             = 000000
      ownerFlags             = 000000
      fileMode               = -rw-r--r--
      fileMode               = 100644
                             . 100000 (S_IFREG)
                             . 000400 (S_IRUSR)
                             . 000200 (S_IWUSR)
                             . 000040 (S_IRGRP)
                             . 000004 (S_IROTH)
 */

//int OutputSectionCurrent(const char** name);
//int OutputSectionStart(char* name);
//int OutputSectionEnd(void);
//
//void        OutputAttribute(char* name, char* value, char* alternate);
//void        OutputAttributeRaw(char* name, uint64_t value, uint8_t base, uint8_t padlen, char* alternate);
//void        OutputAttributeSize(char* name, size_t size);

#endif
