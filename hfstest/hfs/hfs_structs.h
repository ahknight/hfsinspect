//
//  hfs_structs.h
//  hfstest
//
//  Created by Adam Knight on 5/6/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfstest_hfs_structs_h
#define hfstest_hfs_structs_h

struct HFSVolume {
    int fd;                 // File descriptor
    HFSPlusVolumeHeader vh; // Volume header
};
typedef struct HFSVolume HFSVolume;

struct HFSFork {
    struct HFSVolume hfs;           // File system descriptor
    HFSPlusForkData forkData;
};
typedef struct HFSFork HFSFork;



#endif
