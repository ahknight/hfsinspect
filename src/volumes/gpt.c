//
//  gpt.c
//  volumes
//
//  Created by Adam Knight on 11/7/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "gpt.h"
#include "misc/_endian.h"
#include "misc/output.h"
#include "misc/stringtools.h"

uuid_t* gpt_swap_uuid(uuid_t* uuid)
{
    // Because these UUIDs are fucked up.
    void* uuid_buf = uuid;
    *( (uint32_t*) uuid + 0 ) = bswap32(*( (uint32_t*)uuid_buf + 0 ));
    *( (uint16_t*) uuid + 2 ) = bswap16(*( (uint16_t*)uuid_buf + 2 ));
    *( (uint16_t*) uuid + 3 ) = bswap16(*( (uint16_t*)uuid_buf + 3 ));
    *( (uint64_t*) uuid + 1 ) =         *( (uint64_t*)uuid_buf + 1 );
    return uuid;
}

const char* gpt_partition_type_str(uuid_t uuid, VolType* hint)
{
    for(int i=0; gpt_partition_types[i].name[0] != '\0'; i++) {
        uuid_t test;
        uuid_parse(gpt_partition_types[i].uuid, test);
        int result = uuid_compare(uuid, test);
        if ( result == 0 ) {
            if (hint != NULL) *hint = gpt_partition_types[i].voltype;
            return gpt_partition_types[i].name;
        }
    }
    
    static uuid_string_t string;
    uuid_unparse(uuid, string);
    return string;
}

int gpt_load_header(Volume *vol, GPTHeader *header_out, GPTPartitionRecord *entries_out)
{
    if (vol == NULL) { errno = EINVAL; return -1; }
    if ( (header_out == NULL)  && (entries_out == NULL) ) { errno = EINVAL; return -1; }
    
    off_t offset = 0;
    size_t length = 0;
    GPTHeader header; ZERO_STRUCT(header);
    
    {
        MBR mbr; ZERO_STRUCT(mbr);
        offset = vol->sector_size;
        length = sizeof(GPTHeader);
        
        if ( mbr_load_header(vol, &mbr) < 0 )
            return -1;
        
        if (mbr.partitions[0].type == kMBRTypeGPTProtective) {
            vol->sector_size = 512;
            offset = (mbr.partitions[0].first_sector_lba * vol->sector_size);
        }
        
        if ( vol_read(vol, (Bytes)&header, length, offset) < 0 )
            return -1;
    }
    
    if (entries_out != NULL) {
        char* buf = NULL;
        GPTPartitionRecord entries; ZERO_STRUCT(entries);

        // Determine start of partition array
        offset = header.partition_start_lba * vol->sector_size;
        length = header.partition_entry_count * header.partition_entry_size;
        
        // Read the partition array
        ALLOC(buf, length);
        if ( vol_read(vol, (Bytes)buf, length, offset) < 0 )
            return -1;
        
        // Iterate over the partitions and update the volume record
        FOR_UNTIL(i, header.partition_entry_count) {
            GPTPartitionEntry partition;
            
            // Grab the next partition structure
            partition = ((GPTPartitionEntry*)buf)[i];
            if (partition.first_lba == 0 && partition.last_lba == 0)
                break;
            
            // Copy entry struct
            entries[i] = partition;
        }
        
        // Clean up
        FREE(buf);

        if (entries_out != NULL) memcpy(*entries_out, entries, sizeof(GPTPartitionRecord));
    }
    
    if (header_out != NULL) *header_out = header;
    
    return 0;
}

/**
 Tests a volume to see if it contains a GPT partition map.
 @return Returns -1 on error (check errno), 0 for NO, 1 for YES.
 */
int gpt_test(Volume *vol)
{
    debug("GPT test");
    
    // Nasty thing, GPT.  See, typically you'd grab the MBR, look for the
    // protective partition pointer then go get the GPT info.
    // Then 4k sectors arrived.  Now you have no idea how far in to look.
    // So, we read 16KB and search.  Upside: if we find it, we update the
    // sector size to indicate that since it's always at LBA 1. :)
    
    size_t maxLBA = 16*1024;
    size_t haystackSize = maxLBA + sizeof(GPTHeader);
    char* haystack = calloc(1, haystackSize);
    ssize_t nbytes = 0;
    if ( (nbytes = vol_read(vol, (Bytes)haystack, haystackSize, 0)) < 0) {
        FREE(haystack);
        return -1;
    }
    
    char* needle = "EFI PART";
    char* found = memmem(haystack, haystackSize, needle, 8);
    
    if (found == NULL) { FREE(haystack); return false; }
    
    // found is a pointer to the location of the string, which starts the block.
    // Calculate the LBA used to format the drive using this information.
    // Presume LBAs are in the range 512 to 16K..
    size_t blockSize = (found - haystack);
    if (blockSize >= 512 && blockSize <= haystackSize) {
        debug("Updating volume sector size from %d to %zu.", vol->sector_size, blockSize);
        vol->sector_size = blockSize;
        if (vol->length && vol->sector_count)
            vol->sector_count = (vol->length / vol->sector_size);
        info("GPT volume has a sector size of %d and %lld sectors for a total of %zu bytes.", vol->sector_size, vol->sector_count, vol->length);
    }
    
    FREE(haystack);
    found = NULL;
    
    return true;

//    // How it SHOULD work.
//    GPTHeader header = {0};
//    if ( gpt_load_header(vol, &header, NULL) < 0 )
//        return -1;
//    
//    if (memcmp(&header.signature, "EFI PART", 8) == 0) { debug("Found a GPT pmap."); return 1; }
//    
//    return 0;
}

/**
 Updates a volume with sub-volumes for any defined partitions.
 @return Returns -1 on error (check errno), 0 for success.
 */
int gpt_load(Volume *vol)
{
    debug("GPT load");
    
    vol->type = kVolTypePartitionMap;
    vol->subtype = kPMTypeGPT;
    
    GPTHeader header; ZERO_STRUCT(header);
    GPTPartitionRecord entries; ZERO_STRUCT(entries);
    off_t offset = 0;
    size_t length = 0;
    
    // Load GPT header
    if ( gpt_load_header(vol, &header, &entries) < 0 )
        return -1;
    
    // Iterate over the partitions and update the volume record
    FOR_UNTIL(i, header.partition_entry_count) {
        uuid_string_t uuid_str;
        uuid_t* uuid_ptr = NULL;
//        ContainerHint hint;
        GPTPartitionEntry partition;
        
        // Grab the next partition structure
        partition = entries[i];
        if (partition.first_lba == 0 && partition.last_lba == 0)
            break;
        
        // Calculate the size of the partition
        offset = partition.first_lba * vol->sector_size;
        length = (partition.last_lba * vol->sector_size) - offset;
        
        // Add partition to volume
        Volume *p = vol_make_partition(vol, i, offset, length);
        
        // Update partition type with hint
        uuid_ptr = gpt_swap_uuid(&partition.type_uuid);
        uuid_unparse(*uuid_ptr, uuid_str);
        VolType type;
        const char * desc = gpt_partition_type_str(*uuid_ptr, &type);
        p->type = type;
        
        wchar_t wcname[100] = {'\0'};
        int i = 0;
        while (i < 36 && partition.name[i] != '\0') {
            wcname[i] = partition.name[i]; i++;
        }
        char name[100] = {'\0'};
        wcstombs(name, wcname, 100);
        (void)strlcpy(p->desc, name, 36);
        (void)strlcpy(p->native_desc, desc, 100);
//        (void)strlcpy(p->native_desc, uuid_str, 100);
    }
    
    return 0;
}

/**
 Prints a description of the GPT structure and partition information to stdout.
 @return Returns -1 on error (check errno), 0 for success.
 */
int gpt_dump(Volume *vol)
{
    debug("GPT dump");
    
    uuid_string_t uuid_str;
    uuid_t* uuid_ptr;
    
    GPTHeader header; ZERO_STRUCT(header);
    GPTPartitionRecord entries; ZERO_STRUCT(entries);
    
    if ( gpt_load_header(vol, &header, &entries) < 0)
        return -1;
    
    uuid_ptr = gpt_swap_uuid(&header.uuid);
    uuid_unparse(*uuid_ptr, uuid_str);
    
    GPTHeader *header_p = &header;
    
    BeginSection("GPT Partition Map");
    PrintUIHex              (header_p, revision);
    PrintDataLength         (header_p, header_size);
    PrintUIHex              (header_p, header_crc32);
    PrintUIHex              (header_p, reserved);
    PrintUI                 (header_p, current_lba);
    PrintUI                 (header_p, backup_lba);
    PrintUI                 (header_p, first_lba);
    PrintUI                 (header_p, last_lba);
    _PrintDataLength        ("(size)", (header.last_lba * vol->sector_size) - (header.first_lba * vol->sector_size) );
    PrintAttribute    ("uuid", uuid_str);
    PrintUI                 (header_p, partition_start_lba);
    PrintUI                 (header_p, partition_entry_count);
    PrintDataLength         (header_p, partition_entry_size);
    PrintUIHex              (header_p, partition_crc32);
    
    FOR_UNTIL(i, header.partition_entry_count) {
        const char* type = NULL;
        
        GPTPartitionEntry partition = entries[i];
        
        if (partition.first_lba == 0 && partition.last_lba == 0) break;
        
        BeginSection("Partition: %d (%llu)", i + 1, (partition.first_lba * vol->sector_size));
        
        uuid_ptr = gpt_swap_uuid(&partition.type_uuid);
        uuid_unparse(*uuid_ptr, uuid_str);
        type = gpt_partition_type_str(*uuid_ptr, NULL);
        PrintAttribute("type", "%s (%s)", type, uuid_str);
        
        uuid_ptr = gpt_swap_uuid(&partition.uuid);
        uuid_unparse(*uuid_ptr, uuid_str);
        PrintAttribute("uuid", "%s", uuid_str);
        
        PrintAttribute("first_lba", "%llu", partition.first_lba);
        PrintAttribute("last_lba", "%llu", partition.last_lba);
        _PrintDataLength("(size)", (partition.last_lba * vol->sector_size) - (partition.first_lba * vol->sector_size) );
        _PrintRawAttribute("attributes", &partition.attributes, sizeof(partition.attributes), 2);
        wchar_t name[37] = {0};
        for(int c = 0; c < 37; c++) name[c] = partition.name[c];
        name[36] = '\0';
        PrintAttribute("name", "%ls", name);
        EndSection();
    }
    
    EndSection();
    
    return 0;
}

PartitionOps gpt_ops = {
    .test = gpt_test,
    .dump = gpt_dump,
    .load = gpt_load,
};
