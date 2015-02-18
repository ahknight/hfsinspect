//
//  main.c
//  hfsinspect
//
//  Created by Adam Knight on 5/4/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include <stdarg.h>
#include <getopt.h>         // getopt_long
#include <libgen.h>         // basename
#include <locale.h>         // setlocale
#include <errno.h>          // errno/perror
#include <sys/stat.h>       // stat
#include <sys/param.h>      // system definitions

#include <string.h>         // memcpy, strXXX, etc.
#if defined(__linux__)
    #include <bsd/string.h> // strlcpy, etc.
#endif

#if defined(BSD)
    #include <sys/mount.h>  //statfs
#endif

#include "volumes/volumes.h"
#include "hfs/hfs.h"
#include "hfs/output_hfs.h"
#include "hfsplus/hfsplus.h"
#include "logging/logging.h"    // console printing routines

#include "operations/operations.h"
#include "hfs/unicode.h"
#include "volumes/utilities.h"     // commonly-used utility functions


#pragma mark - Function Declarations

void print_version(void);
void print_usage(void);
void print_help(void);

char* deviceAtPath (char* path) __attribute__((nonnull));
bool  resolveDeviceAndPath (char* path_in, char* device_out, char* path_out) __attribute__((nonnull));

void fatal(char* format, ...) __attribute((format(printf,1,2), noreturn));
#define fatal(...) { fprintf(stderr, __VA_ARGS__); fputc('\n', stdout); print_usage(); exit(1); }


#pragma mark - Variable Definitions

static char PROGRAM_NAME[PATH_MAX];


#pragma mark - Function Definitions

#define EMPTY_STRING(x) ((x) == NULL || strlen((x)) == 0)

char* deviceAtPath(char* path)
{
#if defined(BSD)
    static struct statfs stats;
    int                  result = statfs(path, &stats);
    if (result < 0) {
        perror("statfs");
        return NULL;
    }

    debug("statfs: from %s to %s", stats.f_mntonname, stats.f_mntfromname);

    return stats.f_mntfromname;
#else
    warning("%s: not supported on this platform", __FUNCTION__);
    return NULL;
#endif
}

bool resolveDeviceAndPath(char* path_in, char* device_out, char* path_out)
{
#if defined(__BSD__)
    char* path = realpath(path_in, NULL);
    if (path == NULL) {
        die(1, path_in);
        return false;
    }

    struct statfs stats;
    int           result = statfs(path_in, &stats);
    if (result < 0) {
        perror("statfs");
        return false;
    }

    debug("Device: %s", stats.f_mntfromname);
    debug("Mounted at: %s", stats.f_mntonname);
    debug("Path: %s", path);

    char* device     = stats.f_mntfromname;
    char* mountPoint = stats.f_mntonname;

    if (strncmp(mountPoint, "/", PATH_MAX) != 0) {
        // Remove mount point from path name (rather blindly, yes)
        size_t len = strlen(mountPoint);
        path = &path[len];
    }

    // If we were given a directory that was the root of a mounted volume, set the path to the root of that volume.
    if ((path_in[strlen(path_in) - 1] == '/') && (strlen(path) == 0)) {
        path[0] = '/';
        path[1] = '\0';
    }

    (void)strlcpy(path_out, path, PATH_MAX);
    (void)strlcpy(device_out, device, PATH_MAX);

    debug("Path in: %s", path_in);
    debug("Device out: %s", device_out);
    debug("Path out: %s", path_out);

    return true;
#else
    warning("%s: not supported on this platform", __FUNCTION__);
    return false;
#endif
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"

__attribute__((noreturn))
void die(int val, char* format, ...)
{
    va_list args;
    va_start(args, format);
    char    str[1024];
    vsnprintf(str, 1024, format, args);
    va_end(args);

    if (errno > 0) {
        perror(str);
    } else {
        error(str);
    }

    exit(val);
}

#pragma GCC diagnostic pop

void loadBTree(HIOptions* options)
{
    // Load the tree
    if (options->tree_type == BTreeTypeCatalog) {
        if ( hfs_get_catalog_btree(&options->tree, options->hfs) < 0)
            die(1, "Could not get Catalog B-Tree!");

    } else if (options->tree_type == BTreeTypeExtents) {
        if ( hfs_get_extents_btree(&options->tree, options->hfs) < 0)
            die(1, "Could not get Extents B-Tree!");

    } else if (options->tree_type == BTreeTypeAttributes) {
        if ( hfs_get_attribute_btree(&options->tree, options->hfs) < 0)
            die(1, "Could not get Attribute B-Tree!");

    } else if (options->tree_type == BTreeTypeHotfiles) {
        if ( hfs_get_hotfiles_btree(&options->tree, options->hfs) < 0)
            die(1, "Hotfiles B-Tree not found. Target must be a hard drive with a copy of Mac OS X that has successfully booted to create this file (it will not be present on SSDs).");
    } else {
        die(1, "Unsupported tree: %s", options->tree_type);
    }
}

__attribute__((noreturn))
void show_version()
{
    fprintf(stdout, "%s %s\n", PROGRAM_NAME, "1.0");
    exit(0);
}

/* hfsdebug-lite args remaining...
   -0,        --freespace     display all free extents on the volume
   -e,        --examples      display some usage examples
   -f,        --fragmentation display all fragmented files on the volume
   -H,        --hotfiles      display the hottest files on the volume; requires
                                the -t (--top=TOP) option for the number to list
   -l TYPE,   --list=TYPE     specify an HFS+ B-Tree's leaf nodes' type, where
                                 TYPE is one of "file", "folder", "filethread", or
                                 "folderthread" for the Catalog B-Tree; one of
                                 "hfcfile" or "hfcthread" for the Hot Files B-Tree
                                 B-Tree; or "extents" for the Extents B-Tree
                                 You can use the type "any" if you wish to display
                                 all node types. Currently, "any" is the only
                                 type supported for the Attributes B-Tree
   -m,        --mountdata     display a mounted volume's in-memory data
   -S,        --summary_rsrc  calculate and display volume usage summary
   -t TOP,    --top=TOP       specify the number of most fragmented files
                                 to display (when used with the --fragmentation
                                 option), or the number of largest files to display
                                 (when used with the --summary option)
   -x FILTERDYLIB, --filter=FILTERDYLIB
                             run the filter implemented in the dynamic library
                                 whose path is FILTERDYLIB. Alternatively,
                                 FILTERDYLIB can be 'builtin:<name>', where <name>
                                 specifies one of the built-in filters: atime,
                                 crtime, dirhardlink, hardlink, mtime, and sxid
   -X FILTERARGS, --filter_args=FILTERARGS
                             pass this argument string to the filter callback
 */

void print_usage()
{
    char* help = "[-hv] [-d path | -p path | -V fspath] [-0DjlrSs] [-b btree [-n nid]] [-P path] [-F parent:name] [-o file] path";
    fprintf(stderr, "usage: %s %s\n", PROGRAM_NAME, help);
}

void print_help()
{
    char* help = "\n"
                 "    -h,         --help          Show help and quit. \n"
                 "    -v,         --version       Show version information and quit. \n"
                 "    -S          --si            Use base 1000 SI data size measurements instead of the traditional base 1024.\n"
                 "                --debug         Set to be overwhelmed with useless data.\n"
                 "\n"
                 "SOURCES: \n"
                 "hfsinspect will use the root filesystem by default, or the filesystem containing a target file in some cases. If you wish to\n"
                 "specify a specific device or volume, you can with the following options:\n"
                 "\n"
                 "    -d DEV,     --device DEV    Path to device or file containing a bare HFS+ filesystem (no partition map or HFS wrapper) \n"
                 "    -V VOLUME   --volume VOLUME Use the path to a mounted disk or any file on the disk to use a mounted volume. \n"
                 "    -p          --path          Locate the record for the given path on a mounted filesystem.\n"
                 "\n"
                 "INFO: \n"
                 "    By default, hfsinspect will just show you the volume header and quit.  Use the following options to get more specific data.\n"
                 "\n"
                 "    -l,         --list          If the specified FSOB is a folder, list the contents. \n"
                 "    -D,         --disk-info     Show any available information about the disk, including partitions and volume headers.\n"
                 "    -0,         --freespace     Show a summary of the used/free space and extent count based on the allocation file.\n"
                 "    -s,         --summary       Show a summary of the files on the disk.\n"
                 "    -r,         --volumeheader  Dump the volume header. \n"
                 "    -j,         --journal       Dump the volume's journal info block structure. \n"
                 "    -b NAME,    --btree NAME    Specify which HFS+ B-Tree to work with. Supported options: attributes, catalog, extents, or hotfiles. \n"
                 "    -n ID,      --node ID       Dump an HFS+ B-Tree node by ID (must specify tree with -b). \n"
                 "    -c CNID,    --cnid CNID     Lookup and display a record by its catalog node ID. \n"
                 "    -F FSSpec   --fsspec FSSpec Locate a record by Carbon-style FSSpec (parent:name).\n"
                 "    -P path     --fs-path path  Locate a record by path on the given device's filesystem.\n"
                 "    -y DIR      --yank          Yank all the filesystem files and put then in the specified directory.\n"
                 "\n"
                 "OUTPUT: \n"
                 "    You can optionally have hfsinspect dump any fork it finds as the result of an operation. This includes B-Trees or file forks.\n"
                 "    Use a command like \"-b catalog -o catalog.dump\" to extract the catalog file from the boot drive, for instance.\n"
                 "\n"
                 "    -o PATH,    --output PATH   Use with -b or -p to dump a raw data fork (when used with -b, will dump the HFS tree file).\n"
                 "\n";
    print_usage();
    fputs(help, stderr);
}

int main (int argc, char* const* argv)
{
    bool      use_decimal = false;
    HIOptions options     = {0};

#if defined(GC_ENABLED)         // GC_ENABLED
    GC_INIT();
#endif                          // GC_ENABLED

    SALLOC(options.hfs, sizeof(struct HFS));

    (void)strlcpy(PROGRAM_NAME, basename(argv[0]), PATH_MAX);

    if (argc == 1) { print_usage(); exit(1); }

    setlocale(LC_ALL, "");

#pragma mark Process Options

    /* options descriptor */
    struct option longopts[] = {
        { "version",        no_argument,            NULL,                   'v' },
        { "help",           no_argument,            NULL,                   'h' },
        { "si",             no_argument,            NULL,                   'S' },
        { "debug",          no_argument,            NULL,                   'B' },

        { "device",         required_argument,      NULL,                   'd' },
        { "volume",         required_argument,      NULL,                   'V' },
        { "path",           required_argument,      NULL,                   'p' },

        { "volumeheader",   no_argument,            NULL,                   'r' },
        { "journal",        no_argument,            NULL,                   'j' },
        { "list",           no_argument,            NULL,                   'l' },
        { "disk-info",      no_argument,            NULL,                   'D' },
        { "freespace",      no_argument,            NULL,                   '0' },
        { "summary",        no_argument,            NULL,                   's' },
        { "btree",          required_argument,      NULL,                   'b' },
        { "node",           required_argument,      NULL,                   'n' },
        { "cnid",           required_argument,      NULL,                   'c' },
        { "fsspec",         required_argument,      NULL,                   'F' },
        { "fs-path",        required_argument,      NULL,                   'P' },
        { "yank",           required_argument,      NULL,                   'y' },

        { "output",         required_argument,      NULL,                   'o' },
        { NULL,             0,                      NULL,                   0   }
    };

    /* short options */
    char*         shortopts = "0ShvjlrsDd:n:b:p:P:F:V:c:o:y:L";

    int           opt;
    while ((opt = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (opt) {
            case 'L':
            {
                log_level += 1;
                break;
            }

            case 'B':
            {
                log_level = L_DEBUG;
                break;
            }

            case 'S':
            {
                use_decimal = true;
                break;
            }

            case 'l':
            {
                set_mode(&options, HIModeListFolder);
                break;
            }

            case 'v':
            {
                show_version();
            }

            case 'h':
            {
                print_help();
                exit(0);
            }

            case 'j':
            {
                set_mode(&options, HIModeShowJournalInfo);
                break;
            }

            case 'D':
            {
                set_mode(&options, HIModeShowDiskInfo);
                break;
            }

            // Set device with path to file or device
            case 'd':
            {
                (void)strlcpy(options.device_path, optarg, PATH_MAX);
                break;
            }

            case 'V':
            {
                char* str = deviceAtPath(optarg);
                (void)strlcpy(options.device_path, str, PATH_MAX);
                if (options.device_path == NULL) fatal("Unable to determine device. Does the path exist?");
                break;
            }

            case 'r':
            {
                set_mode(&options, HIModeShowVolumeInfo);
                break;
            }

            case 'p':
            {
                set_mode(&options, HIModeShowPathInfo);
                char device[PATH_MAX] = {0}, file[PATH_MAX] = {0};

                bool success          = resolveDeviceAndPath(optarg, device, file);

                if (device != NULL) (void)strlcpy(options.device_path, device, PATH_MAX);
                if (file != NULL)   (void)strlcpy(options.file_path, file, PATH_MAX);

                if ( !success || !strlen(options.device_path ) ) fatal("Device could not be determined. Specify manually with -d/--device.");
                if ( !strlen(options.file_path) ) fatal("Path must be an absolute path from the root of the target filesystem.");

                break;
            }

            case 'P':
            {
                set_mode(&options, HIModeShowPathInfo);
                (void)strlcpy(options.file_path, optarg, PATH_MAX);
                if (options.file_path[0] != '/') fatal("Path given to -P/--fs-path must be an absolute path from the root of the target filesystem.");
                break;
            }

            case 'b':
            {
                set_mode(&options, HIModeShowBTreeInfo);
                if (strcmp(optarg, BTreeOptionAttributes) == 0) options.tree_type = BTreeTypeAttributes;
                else if (strcmp(optarg, BTreeOptionCatalog) == 0) options.tree_type = BTreeTypeCatalog;
                else if (strcmp(optarg, BTreeOptionExtents) == 0) options.tree_type = BTreeTypeExtents;
                else if (strcmp(optarg, BTreeOptionHotfiles) == 0) options.tree_type = BTreeTypeHotfiles;
                else fatal("option -b/--btree must be one of: attributes, catalog, extents, or hotfiles (not %s).", optarg);
                break;
            }

            case 'F':
            {
                set_mode(&options, HIModeShowCatalogRecord);

                memset(&options.record_filename, 0, sizeof(options.record_filename));
                options.record_parent = 0;

                char* option, * tofree;
                option                = tofree = strdup(optarg);
                char* parent   = strsep(&option, ":");
                char* filename = strsep(&option, ":");

                if (parent && strlen(parent)) sscanf(parent, "%u", &options.record_parent);
                if (filename) (void)strlcpy(options.record_filename, filename, PATH_MAX);

                SFREE(tofree);

                if ((options.record_filename == NULL) || (options.record_parent == 0)) fatal("option -F/--fsspec requires a parent ID and file (eg. 2:.journal)");

                break;
            }

            case 'y':
            {
                set_mode(&options, HIModeYankFS);
                (void)strlcpy(options.extract_path, optarg, PATH_MAX);
                break;
            }

            case 'o':
            {
                set_mode(&options, HIModeExtractFile);
                (void)strlcpy(options.extract_path, optarg, PATH_MAX);
                break;
            }

            case 'n':
            {
                set_mode(&options, HIModeShowBTreeNode);

                int count = sscanf(optarg, "%u", &options.node_id);
                if (count == 0) fatal("option -n/--node requires a numeric argument");

                break;
            }

            case 'c':
            {
                set_mode(&options, HIModeShowCNID);

                int count = sscanf(optarg, "%u", &options.cnid);
                if (count == 0) fatal("option -c/--cnid requires a numeric argument");

                break;
            }

            case 's':
            {
                set_mode(&options, HIModeShowSummary);
                break;
            }

            case '0':
            {
                set_mode(&options, HIModeFreeSpace);
                break;
            }

            case '?':  // Unknown option/Missing argument
            case ':':  // Value set or index returned
            case 0:
            default:
            {
                debug("unhandled argument '%c'", opt);
                print_usage();
                exit(1);
            }
        }
    }

#pragma mark Prepare Input and Outputs

    // If no device path was given, use the volume of the CWD.
    if ( EMPTY_STRING(options.device_path) ) {
        info("No device specified. Trying to use the root device.");

        char device[PATH_MAX];
        (void)strlcpy(device, deviceAtPath("."), PATH_MAX);
        if (device != NULL)
            (void)strlcpy(options.device_path, device, PATH_MAX);

        if ( EMPTY_STRING(options.device_path) ) {
            error("Device could not be determined. Specify manually with -d/--device.");
            exit(1);
        }
        debug("Found device %s", options.device_path);
    }

    // Load the device
    Volume* vol = NULL;
OPEN:
    vol = vol_qopen(options.device_path);
    if (vol == NULL) {
        if (errno == EBUSY) {
            // If the device is busy, see if we can use the raw disk instead (and aren't already).
            if (strstr(options.device_path, "/dev/disk") != NULL) {
                char* newDevicePath = NULL;
                SALLOC(newDevicePath, PATH_MAX + 1);
                (void)strlcat(newDevicePath, "/dev/rdisk", PATH_MAX);
                (void)strlcat(newDevicePath, &options.device_path[9], PATH_MAX);
                info("Device %s busy. Trying the raw disk instead: %s", options.device_path, newDevicePath);
                (void)strlcpy(options.device_path, newDevicePath, PATH_MAX);
                SFREE(newDevicePath);
                goto OPEN;
            }
            die(errno, "vol_qopen");

        } else {
            error("could not open %s", options.device_path);
            die(errno, "vol_qopen");
        }
    }

#pragma mark Disk Summary

    if (check_mode(&options, HIModeShowDiskInfo)) {
        volumes_dump(vol);
    } else {
        volumes_load(vol);
    }

#pragma mark Find HFS Volume

    // Device loaded. Find the first HFS+ filesystem.
    info("Looking for HFS filesystems on %s.", basename((char*)&vol->source));
    Volume* tmp = hfs_find(vol);
    if (tmp == NULL) {
        // No HFS Plus volumes found.
        die(1, "No HFS+ filesystems found.");
    } else {
        vol = tmp;
    }

    if (hfs_open(options.hfs, vol) < 0) {
        die(1, "hfs_open");
    }

    out_ctx* ctx = options.hfs->vol->ctx;
    ctx->decimal_sizes = use_decimal;

    uid_t    uid = 99;
    gid_t    gid = 99;

    // If extracting, determine the UID to become by checking the owner of the output directory (so we can create any requested files later).
    if (check_mode(&options, HIModeExtractFile) || check_mode(&options, HIModeYankFS)) {
        char* dir = strdup(dirname(options.extract_path));
        if ( !strlen(dir) ) {
            die(1, "Output file directory does not exist: %s", dir);
        }

        struct stat dirstat = {0};
        int         result  = stat(dir, &dirstat);
        if (result == -1) {
            die(errno, "stat");
        }

        uid = dirstat.st_uid;
        gid = dirstat.st_gid;
        SFREE(dir);
    }

#pragma mark Drop Permissions

    // If we're root, drop down.
    if (geteuid() == 0) {
        debug("Dropping privs");
        if (setegid(gid) != 0) die(1, "Failed to drop group privs.");
        if (seteuid(uid) != 0) die(1, "Failed to drop user privs.");
        info("Was running as root.  Now running as %u/%u.", geteuid(), getegid());
    }

#pragma mark Yank FS
    // Pull the essential files from the disk for inspection or transport.
    if (check_mode(&options, HIModeYankFS)) {
        if (options.hfs->vol == NULL) {
            error("No HFS+ volume detected; can't extract FS.");
            goto NOPE;
        }

        char path[PATH_MAX] = "";
        strlcpy(path, options.extract_path, PATH_MAX);

        int  result         = mkdir(options.extract_path, 0777);
        if ((result < 0) && (errno != EEXIST)) {
            die(0, "mkdir");
        }

        char headerPath         [PATH_MAX+1] = "";
        char allocationPath     [PATH_MAX+1] = "";
        char extentsPath        [PATH_MAX+1] = "";
        char catalogPath        [PATH_MAX+1] = "";
        char attributesPath     [PATH_MAX+1] = "";
        char startupPath        [PATH_MAX+1] = "";
        char hotfilesPath       [PATH_MAX+1] = "";
        char journalBlockPath   [PATH_MAX+1] = "";
        char journalPath        [PATH_MAX+1] = "";

        strlcpy(headerPath,         options.extract_path,           PATH_MAX);
        strlcpy(allocationPath,     headerPath,                     PATH_MAX);
        strlcpy(extentsPath,        headerPath,                     PATH_MAX);
        strlcpy(catalogPath,        headerPath,                     PATH_MAX);
        strlcpy(attributesPath,     headerPath,                     PATH_MAX);
        strlcpy(startupPath,        headerPath,                     PATH_MAX);
        strlcpy(hotfilesPath,       headerPath,                     PATH_MAX);
        strlcpy(journalBlockPath,   headerPath,                     PATH_MAX);
        strlcpy(journalPath,        headerPath,                     PATH_MAX);

        strlcat(headerPath,         "/header.block",                PATH_MAX);
        strlcat(allocationPath,     "/allocation.bmap",             PATH_MAX);
        strlcat(extentsPath,        "/extents.btree",               PATH_MAX);
        strlcat(catalogPath,        "/catalog.btree",               PATH_MAX);
        strlcat(attributesPath,     "/attributes.btree",            PATH_MAX);
        strlcat(startupPath,        "/startupFile",                 PATH_MAX);
        strlcat(hotfilesPath,       "/hotfiles.btree",              PATH_MAX);
        strlcat(journalBlockPath,   "/journal.block",               PATH_MAX);
        strlcat(journalPath,        "/journal.buf",                 PATH_MAX);

        HFSFork* fork        = NULL;

        // Volume Header Blocks
        unsigned sectorCount = 16;
        size_t   readSize    = vol->sector_size*sectorCount;
        void*    buf         = NULL;
        int      nbytes      = 0;

        SALLOC(buf, readSize);
        assert(buf != NULL);
        nbytes = vol_read(vol, (void*)buf, readSize, 0);
        if (nbytes < 0) die(0, "reading volume header");

        FILE* fp = fopen(headerPath, "w");
        (void)fwrite(buf, 1, readSize, fp);
        fclose(fp);

        struct {
            HFSPlusForkData forkData;
            bt_nodeid_t     cnid;
            char*           path;
        } files[5] = {
            // Extents Overflow
            { options.hfs->vh.extentsFile,     kHFSExtentsFileID,      extentsPath},
            // Catalog
            { options.hfs->vh.catalogFile,     kHFSCatalogFileID,      catalogPath},
            // Allocation Bitmap (HFS+)
            { options.hfs->vh.allocationFile,  kHFSAllocationFileID,   allocationPath},
            // Startup (HFS+)
            { options.hfs->vh.startupFile,     kHFSStartupFileID,      startupPath},
            // Attributes (HFS+)
            { options.hfs->vh.attributesFile,  kHFSAttributesFileID,   attributesPath},
        };

        for(unsigned i = 0; i < 5; i++) {
            if (files[i].forkData.logicalSize < 1) continue;

            hfsfork_make(&fork, options.hfs, files[i].forkData, HFSDataForkType, files[i].cnid);
            (void)extractFork(fork, files[i].path);
            hfsfork_free(fork);
            fork = NULL;
        }

        // Hotfiles
        HFSPlusCatalogRecord catalogRecord = {0};

        if ( HFSPlusGetCatalogInfoByPath(NULL, &catalogRecord, "/.hotfiles.btree", options.hfs) ) {
            hfsfork_make(&fork, options.hfs, catalogRecord.catalogFile.dataFork, HFSDataForkType, catalogRecord.catalogFile.fileID);
            (void)extractFork(fork, hotfilesPath);
            hfsfork_free(fork);
            fork = NULL;
        }

        // Journal Info Block
        if ( HFSPlusGetCatalogInfoByPath(NULL, &catalogRecord, "/.journal_info_block", options.hfs) ) {
            hfsfork_make(&fork, options.hfs, catalogRecord.catalogFile.dataFork, HFSDataForkType, catalogRecord.catalogFile.fileID);
            (void)extractFork(fork, journalBlockPath);
            hfsfork_free(fork);
            fork = NULL;
        }

        // Journal File
        if ( HFSPlusGetCatalogInfoByPath(NULL, &catalogRecord, "/.journal", options.hfs) ) {
            hfsfork_make(&fork, options.hfs, catalogRecord.catalogFile.dataFork, HFSDataForkType, catalogRecord.catalogFile.fileID);
            (void)extractFork(fork, journalPath);
            hfsfork_free(fork);
            fork = NULL;
        }
    }

NOPE:

#pragma mark Load Volume Data

    set_hfs_volume(options.hfs); // Set the context for certain output_hfs.h calls.

#pragma mark Volume Requests

    // Always detail what volume we're working on at the very least
//    PrintVolumeInfo(ctx, options.hfs);

    // Default to volume info if there are no other specifiers.
    if (options.mode == 0) set_mode(&options, HIModeShowVolumeInfo);

    // Volume summary
    if (check_mode(&options, HIModeShowSummary)) {
        debug("Printing summary.");
        VolumeSummary summary = generateVolumeSummary(&options);
        PrintVolumeSummary(ctx, &summary);
    }

    // Show volume info
    if (check_mode(&options, HIModeShowVolumeInfo)) {
        debug("Printing volume header.");
        PrintVolumeHeader(ctx, &options.hfs->vh);
    }

    // Journal info
    if (check_mode(&options, HIModeShowJournalInfo)) {
        if (options.hfs->vh.attributes & kHFSVolumeJournaledMask) {
            if (options.hfs->vh.journalInfoBlock != 0) {
                JournalInfoBlock block   = {0};
                bool             success = hfs_get_JournalInfoBlock(&block, options.hfs);
                if (!success) die(1, "Could not get the journal info block!");
                PrintJournalInfoBlock(ctx, &block);

                journal_header   header  = {0};
                success = hfs_get_journalheader(&header, &block, options.hfs);
                if (!success) die(1, "Could not get the journal header!");
                PrintJournalHeader(ctx, &header);

            } else {
                warning("Consistency error: volume attributes indicate it is journaled but the journal info block is empty!");
            }
        }
    }

#pragma mark Allocation Requests
    // Show volume's free space extents
    if (check_mode(&options, HIModeFreeSpace)) {
        debug("Show free space.");
        showFreeSpace(&options);
    }

#pragma mark Catalog Requests

    // Show a path's series of records
    if (check_mode(&options, HIModeShowPathInfo)) {
        debug("Show path info.");
        showPathInfo(&options);
    }

    // Show a catalog record by FSSpec
    if (check_mode(&options, HIModeShowCatalogRecord)) {
        debug("Finding catalog record for %d:%s", options.record_parent, options.record_filename);
        FSSpec spec = {
            .hfs      = options.hfs,
            .parentID = options.record_parent,
            .name     = strtohfsuc(options.record_filename)
        };
        showCatalogRecord(&options, spec, false);
    }

    // Show a catalog record by its CNID (file/folder ID)
    if (check_mode(&options, HIModeShowCNID)) {
        debug("Showing CNID %d", options.cnid);
        FSSpec spec = {
            .hfs      = options.hfs,
            .parentID = options.cnid,
            .name     = {0}
        };
        showCatalogRecord(&options, spec, true);
    }

#pragma mark B-Tree Requests

    // Show B-Tree info
    if (check_mode(&options, HIModeShowBTreeInfo)) {
        debug("Printing tree info.");

        loadBTree(&options);

        if (options.tree->treeID == kHFSCatalogFileID) {
            BeginSection(ctx, "Catalog B-Tree Header");

        } else if (options.tree->treeID == kHFSExtentsFileID) {
            BeginSection(ctx, "Extents B-Tree Header");

        } else if (options.tree->treeID == kHFSAttributesFileID) {
            BeginSection(ctx, "Attributes B-Tree Header");

        } else if (options.tree_type == BTreeTypeHotfiles) {
            BeginSection(ctx, "Hotfiles B-Tree Header");

        } else {
            die(1, "Unknown tree type: %d", options.tree_type);
        }

        PrintTreeNode(ctx, options.tree, 0);
        EndSection(ctx);
    }

    // Show a B-Tree node by ID
    if (check_mode(&options, HIModeShowBTreeNode)) {
        debug("Printing tree node.");

        loadBTree(&options);

        if (options.tree_type == BTreeTypeCatalog) {
            BeginSection(ctx, "Catalog B-Tree Node %d", options.node_id);

        } else if (options.tree_type == BTreeTypeExtents) {
            BeginSection(ctx, "Extents B-Tree Node %d", options.node_id);

        } else if (options.tree_type == BTreeTypeAttributes) {
            BeginSection(ctx, "Attributes B-Tree Node %d", options.node_id);

        } else if (options.tree_type == BTreeTypeHotfiles) {
            BeginSection(ctx, "Hotfiles B-Tree Node %d", options.node_id);

        } else {
            die(1, "Unknown tree type: %s", options.tree_type);
        }

        bool showHex = 0;

        if (showHex) {
            size_t length = options.tree->headerRecord.nodeSize;
            off_t  offset = length * options.node_id;
            void*  buf    = NULL;
            SALLOC(buf, length);
            fpread(options.tree->fp, buf, length, offset);
            VisualizeData(buf, length);
            SFREE(buf);

        } else {
            PrintTreeNode(ctx, options.tree, options.node_id);
        }

        EndSection(ctx);

    }

#pragma mark Extract File Requests

    // Extract any found files (not complete; FIXME: dropping perms breaks right now)
    if (check_mode(&options, HIModeExtractFile)) {
        if ((options.extract_HFSFork == NULL) && options.tree && (options.tree->treeID != 0)) {
            HFSFork* fork;
            hfsfork_get_special(&fork, options.hfs, options.tree->treeID);
            options.extract_HFSFork = fork;
        }

        // Extract any found data, if requested.
        if ((options.extract_path != NULL) && (strlen(options.extract_path) != 0)) {
            debug("Extracting data.");
//            if (options.extract_HFSPlusCatalogFile->fileID != 0) {
//                extractHFSPlusCatalogFile(options.hfs, options.extract_HFSPlusCatalogFile, options.extract_path);
//            } else
            if (options.extract_HFSFork != NULL) {
                extractFork(options.extract_HFSFork, options.extract_path);
            }
        }
    }

    // Clean up
    hfs_close(options.hfs); // also perorms vol_close(vol), though perhaps it shouldn't?

    debug("Clean exit.");

    return 0;
}

