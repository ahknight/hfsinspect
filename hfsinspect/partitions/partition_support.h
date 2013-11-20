//
//  partition_support.h
//  hfsinspect
//
//  Created by Adam Knight on 6/21/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_partition_support_h
#define hfsinspect_partition_support_h

#pragma mark - Structures

typedef enum PartitionHint {
    kHintIgnore         = 'IGNR',             // Do not look at this partition (reserved space, etc.)
    kHintFilesystem     = 'FS  ',             // May have a filesystem
    kHintPartitionMap   = 'PM  ',             // May have a partition map
} PartitionHint;

typedef struct Partition {
    off_t               offset;             // offset in bytes on device
    size_t              length;             // length in bytes
    PartitionHint       hints;              // PartitionHint flags
} Partition;

#endif
