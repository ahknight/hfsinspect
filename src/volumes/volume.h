//
//  volume.h
//  volumes
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef volumes_volume_h
#define volumes_volume_h

#include <sys/param.h>          //PATH_MAX

#pragma mark - Structures

typedef uint32_t VolType;

enum {
    kTypeUnknown                = '\?\?\?\?',
    
    kVolTypeSystem              = 'IGNR',   // Do not look at this volume (reserved space, etc.)
    kVolTypePartitionMap        = 'CTNR',   // May contain subvolumes
    kVolTypeUserData            = 'DATA',   // May contain a filesystem

    kPMTypeAPM                  = 'MP  ',
    kPMTypeMBR                  = '55aa',
    kPMTypeGPT                  = 'EFIP',
    kPMCoreStorage              = 'CS  ',

    kFSTypeMFS                  = 'MFMa',
    kFSTypeHFS                  = 'BD  ',
    kFSTypeHFSPlus              = 'H+  ',
    kFSTypeWrappedHFSPlus       = 'BDH+',
    kFSTypeHFSX                 = 'HX  ',
};

typedef struct Volume Volume;
struct Volume {
    int                 fd;                 // POSIX file descriptor
    FILE                *fp;                // C file handle
    char                device[PATH_MAX];   // path to device file
    
    size_t              block_size;         // block size (LBA size if the raw disk; FS blocks if a filesystem)
    size_t              block_count;        // total blocks in this volume
    
    off_t               offset;             // offset in bytes on device
    size_t              length;             // length in bytes
    
    VolType             type;               // Major type of volume (partition map or filesystem)
    VolType             subtype;            // Minor type of volume (style of pmap or fs (eg. GPT or HFSPlus)
    char                desc[100];          // Human-readable description of the volume format.
    char                native_desc[100];   // Native description of the volume format, if any.
    
    uint32_t            depth;              // How many containers deep this volue is found (0 = root)
    Volume*             parent_partition;   // the enclosing partition; NULL if the root partition map
    
    unsigned            partition_count;    // total count of sub-partitions; 0 if this is a data partition
    Volume*             partitions[128];    // partition records
};

// For partition/filesytem-specific volume load/modify operations.
typedef int (*volop) (Volume* vol);

typedef struct PartitionOps {
    volop   test;
    volop   load;
    volop   dump;
} PartitionOps;

#pragma mark - Functions

/** 
 Opens the character device at the specified path and returns an allocated Volume* structure. Close and release the structure with vol_close().
 @param path The path to the character device.
 @param mode The mode to pass to open(2). Only O_RDONLY and O_RDRW are supported.
 @param offset The offset on the device this volume starts at.
 @param length The length of the volume on the device. Reads past this will be treated as if they were reading past the end of a file and return zeroes. Pass in 0 for no length checking.
 @param block_size The size of the blocks on this volume. For devices, use the LBA block size. For filesystems, use the filesystem allocation block size. If 0 is passed in, ioctl(2) is used to determine the block size of the underlying device automatically.
 @return A pointer to a Volume struct or NULL on failure (check errno and reference open(2) for details).
 @see {@link vol_qopen}
 */
Volume* vol_open(const char* path, int mode, off_t offset, size_t length, size_t block_size);

/**
 Quickly open a whole character device. Offset, length, and block_size are set to zero and auto-detected as needed.
 @param path The path to the character device.
 @see {@link vol_open}
 */
Volume* vol_qopen(const char* path);

/**
 Read from a volume, adjusting for the volume's device offset and length.
 @param vol The Volume to read from.
 @param buf A buffer of sufficient size to hold the result.
 @param nbyte The number of bytes to read.
 @param offset The offset within the volume to read from. Do not compensate for the volume's physical location or block size -- that's what this function is for.
 @see read(2)
 */
ssize_t vol_read        (const Volume *vol, void* buf, size_t size, off_t offset);
ssize_t vol_read_blocks (const Volume *vol, void* buf, ssize_t block_count, ssize_t start_block);
ssize_t vol_read_raw    (const Volume *vol, void* buf, size_t nbyte, off_t offset);

/**
 Write to a volume, adjusting for the volume's device offset and length.
 @param vol The Volume to write to.
 @param buf A buffer containing the data to write.
 @param nbyte The number of bytes to write.
 @param offset The offset within the volume to write to. Do not compensate for the volume's physical location -- that's what this function is for.
 @see write(2)
 */
ssize_t vol_write(Volume *vol, const void* buf, size_t nbyte, off_t offset);

/**
 Closes the file descriptor and releases the memory used by the Volume structure.
 @param vol The Volume to close.
 @see close(2)
 */
int vol_close(Volume *vol);

/**
 */
Volume* vol_make_partition(Volume* vol, uint16_t pos, off_t offset, size_t length);
void vol_dump(Volume* vol);

#endif
