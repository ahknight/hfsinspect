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
#include "hfs/Apple/hfs_macos_defs.h"

#pragma mark - Structures

typedef uint32_t VolType;
typedef char*    String;
typedef BytePtr  Bytes;

enum {
    kTypeUnknown                = 0,
    
    kVolTypePartitionMap        = 'PM  ',   // May contain subvolumes
    kVolTypeSystem              = 'SYST',   // Do not look at this volume (reserved space, etc.)
    kVolTypeUserData            = 'DATA',   // May contain a filesystem
    
    kSysSwapSpace               = 'SWAP',   // Swap space
    kSysFreeSpace               = 'FREE',   // Free space
    kSysEFI                     = 'EFI ',   // EFI reserved space
    kSysRecovery                = 'RCVR',   // Recovery OS installer
    kSysReserved                = 'RSVD',   // General system reserved space ("other")
    
    kPMTypeAPM                  = 'APM ',
    kPMTypeMBR                  = 'MBR ',
    kPMTypeGPT                  = 'GPT ',
    kPMCoreStorage              = 'CS  ',
    
    kFSTypeMFS                  = 'MFS ',
    kFSTypeHFS                  = 'HFS ',
    kFSTypeHFSPlus              = 'HFS+',
    kFSTypeWrappedHFSPlus       = 'HFSW',
    kFSTypeHFSX                 = 'HFSX',
    
    kFSTypeUFS                  = 'UFS ',
    kFSTypeZFS                  = 'ZFS ',
    kFSTypeBeFS                 = 'BeFS',
    kFSTypeExt2                 = 'ext2',
    kFSTypeExt3                 = 'ext3',
    kFSTypeExt4                 = 'ext4',
    kFSTypeBTFS                 = 'BTFS',
    
    kFSTypeFAT12                = 'FT12',
    kFSTypeFAT16                = 'FT16',
    kFSTypeFAT16B               = 'FT17',
    kFSTypeFAT32                = 'FT32',
    kFSTypeExFAT                = 'ExFT',
    kFSTypeNTFS                 = 'NTFS',
};

typedef struct Volume Volume;
struct Volume {
    int                 fd;                 // POSIX file descriptor
    FILE                *fp;                // C file handle
    char                source[PATH_MAX];   // path to source file
    mode_t              mode;               // mode of source file
    
    uint32_t            sector_size;         // block size (LBA size if the raw disk; FS blocks if a filesystem)
    uint64_t            sector_count;        // total blocks in this volume
    
    off_t               offset;             // offset in bytes on source
    size_t              length;             // length in bytes
    
    VolType             type;               // Major type of volume (partition map or filesystem)
    VolType             subtype;            // Minor type of volume (style of pmap or fs (eg. GPT or HFSPlus)
    char                desc[100];          // Human-readable description of the volume format.
    char                native_desc[100];   // Native description of the volume format, if any.
    
    unsigned            depth;              // How many containers deep this volue is found (0 = root)
    Volume*             parent_partition;   // the enclosing partition; NULL if the root partition map
    
    unsigned            partition_count;    // total count of sub-partitions; 0 if this is a data partition
    Volume*             partitions[128];    // partition records
};

// For partition/filesytem-specific volume load/modify operations.
typedef int (*volop) (Volume* vol);

typedef struct PartitionOps {
    char    name[32];
    volop   test;
    volop   load;
    volop   dump;
} PartitionOps;

#pragma mark - Functions

/** 
 Opens the source at the specified path and returns an allocated Volume* structure. Close and release the structure with vol_close().
 @param vol A pointer to a Volume struct.
 @param path The path to the source.
 @param mode The mode to pass to open(2). Only O_RDONLY and O_RDRW are supported.
 @param offset The offset on the source this volume starts at.
 @param length The length of the volume on the source. Reads past this will be treated as if they were reading past the end of a file and return zeroes. Pass in 0 for no length checking.
 @param block_size The size of the blocks on this volume. For devices, use the LBA block size. For filesystems, use the filesystem allocation block size. If 0 is passed in and path points to a device, ioctl(2) is used to determine the block size of underlying devices automatically.
 @return Zero on success, -1 on failure (check errno and reference open(2) for details).
 @see {@link vol_qopen}
 */
int vol_open(Volume* vol, const String path, int mode, off_t offset, size_t length, size_t block_size) _NONNULL;

/**
 Quickly open a whole source. Offset, length, and block_size are set to zero and auto-detected as needed.
 @param path The path to the source.
 @see {@link vol_open}
 */
Volume* vol_qopen(const String path) _NONNULL;

/**
 Read from a volume, adjusting for the volume's source offset and length.
 @param vol The Volume to read from.
 @param buf A buffer of sufficient size to hold the result.
 @param nbyte The number of bytes to read.
 @param offset The offset within the volume to read from. Do not compensate for the volume's physical location or block size -- that's what this function is for.
 @see read(2)
 */
ssize_t vol_read        (const Volume *vol, void* buf, size_t size, off_t offset) __attribute__((nonnull(1,2)));
ssize_t vol_read_blocks (const Volume *vol, void* buf, ssize_t block_count, ssize_t start_block) __attribute__((nonnull(1,2)));
ssize_t vol_read_raw    (const Volume *vol, void* buf, size_t nbyte, off_t offset) __attribute__((nonnull(1,2)));

/**
 Write to a volume, adjusting for the volume's source offset and length.
 @param vol The Volume to write to.
 @param buf A buffer containing the data to write.
 @param nbyte The number of bytes to write.
 @param offset The offset within the volume to write to. Do not compensate for the volume's physical location -- that's what this function is for.
 @see write(2)
 */
ssize_t vol_write(Volume *vol, const void* buf, size_t nbyte, off_t offset) __attribute__((nonnull(1,2)));

/**
 Closes the file descriptor and releases the memory used by the Volume structure.
 @param vol The Volume to close.
 @see close(2)
 */
int vol_close(Volume *vol) _NONNULL;

/**
 */
Volume* vol_make_partition(Volume* vol, uint16_t pos, off_t offset, size_t length) _NONNULL;
void vol_dump(Volume* vol) _NONNULL;

#endif
