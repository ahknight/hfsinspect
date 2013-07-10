[![Stories in Ready](http://badge.waffle.io/ahknight/hfsinspect.png)](http://waffle.io/ahknight/hfsinspect)  
# hfsinspect
## An open-source HFS+ filesystem explorer and debugger (in the spirit of hfsdebug)

This program is a work-in-progress and is quite buggy in its current state.  That said, it's read-only and can display some really interesting information already so have a run with it, help where you can, and file some suggestions on the site.

https://github.com/ahknight/hfsinspect

# Quick Guide

You'll need Xcode 4+ to build right now, and it does use OS X system headers (though I plan on changing that in the future).

Once built and the product located, go ahead and hit "hfsinspect --help" for a quick blurb about the options.

## Examples

    $ hfsinspect -d ./fstest -l -P /
    Listing for FS Test
    CNID      kind       mode       user      group     data            rsrc            name
    18        folder     d--------- 0         0         -               -               ␀␀␀␀HFS+ Private Data
    19        folder     dr-xr-xr-t 0         0         -               -               .HFS+ Private Directory Data
    20        folder     d-wx-wx-wt 501       20        -               -               .Trashes
    21        folder     drwx------ 501       20        -               -               .fseventsd
    16        file       ---------- 0         0         512.00 KiB      0.00 bytes      .journal
    17        file       ---------- 0         0         4.00 KiB        0.00 bytes      .journal_info_block
    ----------------------------------------------------------------------------------------------------
                                                        516.00 KiB      0.00 bytes
       Folders: 4          Data forks: 2          Hard links: 0
         Files: 2          RSRC forks: 0            Symlinks: 0

    $ hfsinspect -d ./fstest -P /

    #   Record ID 1 (2/20) (offset 124; length: 32) (Node 1)

      Catalog Key
     +-----------------------------------------------------------------------------------+
     | length | parentID   | length | nodeName                                           |
     | 6      | 2          | 0      |                                                    |
     +-----------------------------------------------------------------------------------+
      recordType             = kHFSPlusFolderThreadRecord
      reserved               = 0
      parentID               = 1
      nodeName               = "FS Test" (7)
    
    $ hfsinspect -d ./fstest -P /.journal

    #   Record ID 6 (7/20) (offset 676; length: 272) (Node 1)

      Catalog Key
     +-----------------------------------------------------------------------------------+
     | length | parentID   | length | nodeName                                           |
     | 22     | 2          | 8      | .journal                                           |
     +-----------------------------------------------------------------------------------+
      recordType             = kHFSPlusFileRecord
      flags                  = 00000000000000000000000000000010
                             . kHFSThreadExistsMask
      reserved1              = 0
      fileID                 = 16
      createDate             = Sun May 12 00:42:42 2013 UTC
      contentModDate         = Sun May 12 00:42:42 2013 UTC
      attributeModDate       = Thu Jan  1 00:00:00 1970 UTC
      accessDate             = Thu Jan  1 00:00:00 1970 UTC
      backupDate             = Thu Jan  1 00:00:00 1970 UTC
      ownerID                = 0
      groupID                = 0
      adminFlags             = 000000
      ownerFlags             = 000000
      fileMode               = ----------
      fileMode               = 100000
                             . 100000 (S_IFREG)
      special.linkCount      = 1
      fdType                 = 0x6A726E6C (jrnl)
      fdCreator              = 0x6866732B (hfs+)
      fdFlags                = 00000000000000000000000001010000
                             . kNameLocked (4096)
                             . kIsInvisible (16384)
      fdLocation             = (0, 0)
      opaque                 = 0
      textEncoding           = 0
      reserved2              = 0
      fork                   = data
      logicalSize            = 512.00 KiB (524288 bytes)
      clumpSize              = 0.00 bytes (0 bytes)
      totalBlocks            = 512.00 KiB (128 blocks)
      extents                =   startBlock   blockCount    % of file
                                          3          128       100.00
                               --------------------------------------
                                  1 extents          128       100.00
                               128.00 blocks per extent on average.
      fork                   = resource
      logicalSize            = (empty)

      $ hfsinspect -d ./fstest -v

      # HFSX Volume Format (v5)
        volume name            = FS Test
        case sensitivity       = case sensitive
        bootable               = no

      # HFS Plus Volume Header
        signature              = 0x4858 (HX)
        version                = 5
        attributes             = 00000000000000000000000000000000
                               = 00000000000000000010000000100001
                               . kHFSVolumeUnmountedMask (256)
                               . kHFSVolumeJournaledMask (8192)
                               . kHFSUnusedNodeFixMask (18446744071562067968)
                               . kHFSMDBAttributesMask (33664)
        lastMountedVersion     = 0x4846534A (HFSJ)
        journalInfoBlock       = 2
        createDate             = Sat May 11 19:42:41 2013 UTC
        modifyDate             = Sun May 12 06:24:19 2013 UTC
        backupDate             = Thu Jan  1 00:00:00 1970 UTC
        checkedDate            = Sun May 12 00:42:41 2013 UTC
        fileCount              = 5
        folderCount            = 4
        blockSize              = 4.00 KiB (4096 bytes)
        totalBlocks            = 100.00 MiB (25600 blocks)
        freeBlocks             = 97.13 MiB (24865 blocks)
        nextAllocation         = 4733
        rsrcClumpSize          = 64.00 KiB (65536 bytes)
        dataClumpSize          = 64.00 KiB (65536 bytes)
        nextCatalogID          = 25
        writeCount             = 6
        encodingsBitmap        = 00000000000000000000000000000000
                               = 00000000000000000000000000000000
                               = 00000000000000000000000000000000
                               = 00000000000000000000000000000001
                               . kTextEncodingMacRoman (0)

      # Finder Info
        bootDirID              = 0 ()
        bootParentID           = 0 ()
        openWindowDirID        = 0 ()
        os9DirID               = 0 ()
        reserved               = 00000000000000000000000000000000
                               = 00000000000000000000000000000000
        osXDirID               = 0 ()
        volID                  = 0x7EF9BE3682182A73

      # Journal Info Block
        flags                  = 00000000000000000000000000000000
                               = 00000000000000000000000000000001
                               . kJIJournalInFSMask (1)
        device_signature       = 0x00000000000000000000000000000000
                               = 0x00000000000000000000000000000000
        offset                 = 12.00 KiB (12288 bytes)
        size                   = 512.00 KiB (524288 bytes)
        ext_jnl_uuid           =
        machine_serial_num     =

      # Allocation Bitmap File
        fork                   = data
        logicalSize            = 4.00 KiB (4096 bytes)
        clumpSize              = 4.00 KiB (4096 bytes)
        totalBlocks            = 4.00 KiB (1 blocks)
        extents                =   startBlock   blockCount    % of file
                                            1            1       100.00
                                 --------------------------------------
                                    1 extents            1       100.00
                                 1.00 blocks per extent on average.

      # Extents Overflow File
        fork                   = data
        logicalSize            = 800.00 KiB (819200 bytes)
        clumpSize              = 800.00 KiB (819200 bytes)
        totalBlocks            = 800.00 KiB (200 blocks)
        extents                =   startBlock   blockCount    % of file
                                          131          200       100.00
                                 --------------------------------------
                                    1 extents          200       100.00
                                 200.00 blocks per extent on average.

      # Catalog File
        fork                   = data
        logicalSize            = 800.00 KiB (819200 bytes)
        clumpSize              = 800.00 KiB (819200 bytes)
        totalBlocks            = 800.00 KiB (200 blocks)
        extents                =   startBlock   blockCount    % of file
                                         2531          200       100.00
                                 --------------------------------------
                                    1 extents          200       100.00
                                 200.00 blocks per extent on average.

      # Attributes File
        fork                   = data
        logicalSize            = 800.00 KiB (819200 bytes)
        clumpSize              = 800.00 KiB (819200 bytes)
        totalBlocks            = 800.00 KiB (200 blocks)
        extents                =   startBlock   blockCount    % of file
                                          331          200       100.00
                                 --------------------------------------
                                    1 extents          200       100.00
                                 200.00 blocks per extent on average.

      # Startup File
        fork                   = data
        logicalSize            = (empty)

    $ ls -l file
    -rw-r--r--  1 ahknight  staff  10240 May 24 17:35 file
    
    $ hfsinspect -p file

    #   Record ID 14 (15/23) (offset 2252; length: 264) (Node 86272)

      Catalog Key
     +-----------------------------------------------------------------------------------+
     | length | parentID   | length | nodeName                                           |
     | 14     | 379620     | 4      | file                                               |
     +-----------------------------------------------------------------------------------+
      recordType             = kHFSPlusFileRecord
      flags                  = 00000000000000000000000010000010
                             . kHFSThreadExistsMask
                             . kHFSHasDateAddedMask
      reserved1              = 0
      fileID                 = 108228801
      createDate             = Fri May 24 22:35:34 2013 UTC
      contentModDate         = Fri May 24 22:35:34 2013 UTC
      attributeModDate       = Fri May 24 22:35:34 2013 UTC
      accessDate             = Tue Jun 18 03:47:49 2013 UTC
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
      special.linkCount      = 1
      fdType                 = 0x00000000 ()
      fdCreator              = 0x00000000 ()
      fdFlags                = 00000000000000000000000000000000
      fdLocation             = (0, 0)
      opaque                 = 0
      textEncoding           = 0
      reserved2              = 0
      fork                   = data
      logicalSize            = 10.00 KiB (10240 bytes)
      clumpSize              = 0.00 bytes (0 bytes)
      totalBlocks            = 12.00 KiB (3 blocks)
      extents                =   startBlock   blockCount    % of file
                                   95046227            3       100.00
                               --------------------------------------
                                  1 extents            3       100.00
                               3.00 blocks per extent on average.
      fork                   = resource
      logicalSize            = (empty)
    
    $ hfsinspect -p /Volumes/TARDIS/Backups.backupdb/Gallifrey/2013-05-29-011753/Hubert/ -l
    Listing for Hubert
    CNID      kind       mode       user      group     data            rsrc            name
    10929881  hard link  -r--r--r-- 11151100  10812491  0.00 bytes      0.00 bytes      .apdisk
    10929892  file       -rw-r--r-- 0         20        181.00 bytes    0.00 bytes      .com.apple.backupd.mvlist.plist
    10929882  hard link  -r--r--r-- 11151101  10812492  0.00 bytes      0.00 bytes      .com.apple.timemachine.donotpresent
    10929883  dir link   -r--r--r-- 0         0         0.00 bytes      464.00 bytes    .DocumentRevisions-V100
    10929884  hard link  -r--r--r-- 11151103  0         0.00 bytes      0.00 bytes      .DS_Store
    10929885  hard link  -r--r--r-- 11151104  10812495  0.00 bytes      0.00 bytes      .VolumeIcon.icns
    10929886  dir link   -r--r--r-- 11151105  10812496  0.00 bytes      464.00 bytes    [redacted]
    10929887  dir link   -r--r--r-- 11151106  0         0.00 bytes      464.00 bytes    Archived Music
    10929888  dir link   -r--r--r-- 11151107  10812498  0.00 bytes      464.00 bytes    Backups
    10929889  dir link   -r--r--r-- 11151108  0         0.00 bytes      464.00 bytes    [redacted]
    ----------------------------------------------------------------------------------------------------
                                                  181.00 bytes    5.00 bytes
    Folders: 0          Data forks: 1          Hard links: 9
      Files: 1          RSRC forks: 5            Symlinks: 0

    $ hfsinspect -c 18 -l
    Listing for �<90><80>�<90><80>�<90><80>�<90><80>HFS+ Private Data
    CNID      kind       mode       user      group     data            rsrc            name
    100043847 file       -rw-r--r-- 501       20        32.00 MiB       0.00 bytes      iNode100043847
    100049839 file       -r--r--r-- 501       20        1.69 KiB        0.00 bytes      iNode100049839
    100196873 file       -r--r--r-- 501       20        2.05 KiB        0.00 bytes      iNode100196873
    ...
    86494855  file       -rw-r--r-- 0         0         0.00 bytes      10.36 KiB       temp86494855
    86494857  file       -rw-r--r-- 0         0         0.00 bytes      111.80 KiB      temp86494857
    86494861  file       -rw-r--r-- 0         0         0.00 bytes      13.33 KiB       temp86494861
    ----------------------------------------------------------------------------------------------------
                                                      14.18 GiB       129.00 bytes
     Folders: 68         Data forks: 7663       Hard links: 0
       Files: 8613       RSRC forks: 129          Symlinks: 0

    $ hfsinspect -s -V /Volumes/Titania/
    # Volume Summary
      nodeCount              = 20780
      recordCount            = 858836
      fileCount              = 366108
      folderCount            = 71988
      aliasCount             = 4
      hardLinkFileCount      = 0
      hardLinkFolderCount    = 0
      symbolicLinkCount      = 311
      invisibleFileCount     = 293
      emptyFileCount         = 8678
      emptyDirectoryCount    = 0

    # Data Fork
      count                  = 357067
      fragmentedCount        = 7839
      blockCount             = 279.51 GiB (73271622 blocks)
      logicalSpace           = 278.83 GiB (299390329775 bytes)
      extentRecords          = 357067
      extentDescriptors      = 372671
      overflowExtentRecords  = 0
      overflowExtentDescriptors= 0

    # Resource Fork
      count                  = 6575
      fragmentedCount        = 1
      blockCount             = 341.55 MiB (87437 blocks)
      logicalSpace           = 327.25 MiB (343150709 bytes)
      extentRecords          = 6575
      extentDescriptors      = 6576
      overflowExtentRecords  = 0
      overflowExtentDescriptors= 0

    # Largest Files
    #       Size       CNID
    1   7.42 GiB     807918 MOSX.vdi
    2   2.22 GiB     810796 [redacted]
    3   1.62 GiB     807943 winxp-000001-s002.vmdk
    4   1.49 GiB     808013 Virtual Disk-s001.vmdk
    5   1.48 GiB     807964 winxp-s001.vmdk
    6   1.47 GiB     799434 [redacted]
    7   1.37 GiB     816888 [redacted]
    8   1.28 GiB     810661 [redacted]
    9   1.21 GiB     128733 [redacted]

Hopefully you get the idea.  Have fun!
