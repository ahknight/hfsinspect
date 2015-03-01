//
//  hotfiles.h
//  hfsinspect
//
//  Created by Adam Knight on 12/3/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_hotfiles_h
#define hfsinspect_hotfiles_h

#include "hfs/btree/btree.h"
#include "hfs/types.h"

#define HFC_MAGIC               0xFF28FF26
#define HFC_VERSION             1
#define HFC_DEFAULT_DURATION    (3600 * 60)
#define HFC_MINIMUM_TEMPERATURE 16
#define HFC_MAXIMUM_FILESIZE    (10 * 1024 * 1024)
#define HFS_TAG                 "CLUSTERED HOT FILES B-TREE     ";
#define HFC_LOOKUPTAG           0xFFFFFFFF
#define HFC_KEYLENGTH           (sizeof(HotFileKey) - sizeof(uint32_t))

struct HotFilesInfo {
    uint32_t magic;             /* must be HFC_MAGIC */
    uint32_t version;           /* must be HFC_VERSION */
    uint32_t duration;          /* duration of sample period */
    uint32_t timebase;          /* recording period start time */
    uint32_t timeleft;          /* recording period stop time */
    uint32_t threshold;
    uint32_t maxfileblks;
    uint32_t maxfilecnt;
    uint8_t  tag[32];           /* "An implementation should not attempt to verify or change this field." --TN1150 */
};
typedef struct HotFilesInfo HotFilesInfo;

struct HotFileKey {
    uint16_t keyLength;         /* always 10 */
    uint8_t  forkType;          /* always 0 */
    uint8_t  pad;               /* always 0 */
    uint32_t temperature;
    uint32_t fileID;
};
typedef struct HotFileKey HotFileKey;

/*
   The data for a thread record is the temperature of that fork, stored as a UInt32.
   Hot file records use all of the key fields as described above. The data for a hot file record is 4 bytes. The data in a hot file record is not meaningful. To aid in debugging, Mac OS X version 10.3 typically stores the first four bytes of the file name (encoded in UTF-8), or the ASCII text "????".
   --TN1150
 */
struct HotFileThreadRecord {
    union {
        uint32_t temperature;
        char     text[4];
    };
};
typedef struct HotFileThreadRecord HotFileThreadRecord;

#define HFCMakeKey(forkType, temperature, fileID) \
    (HotFileKey){ .keyLength = HFC_KEYLENGTH, .forkType = forkType, .pad = 0, .temperature = temperature, .fileID = fileID }

#define HFCMakeLookupKey(fileID, forkType) \
    (HotFileKey){ .keyLength = HFC_KEYLENGTH, .forkType = forkType, .pad = 0, .temperature = HFC_LOOKUPTAG, .fileID = fileID }


int hfs_get_hotfiles_btree(BTreePtr* tree, const HFSPlus* hfs) __attribute__((nonnull));
int hfs_hotfiles_get_node(BTreeNodePtr* node, const BTreePtr bTree, bt_nodeid_t nodeNum) __attribute__((nonnull));

uint32_t HFCGetFileTemperature(HFSPlus* hfs, bt_nodeid_t fileID, hfs_forktype_t forkType) __attribute__((nonnull));

#endif
