//
//  partition_support.c
//  hfsinspect
//
//  Created by Adam Knight on 6/21/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "partition_support.h"
#include "hfs_pstruct.h"
#include "hfs_io.h"
#include "hfs_endian.h"

// Someday this will need to know if it's 512b or 4k, but for now, this works >99% of the time.
#define lba_size hfs->block_size

bool sniff_and_print(HFSVolume* hfs)
{
    if (gpt_sniff(hfs)) {
        print_gpt(hfs);
        
    } else if (cs_sniff(hfs)) {
        print_cs(hfs);
        
    } else if (apm_sniff(hfs)) {
        print_apm(hfs);
        
    } else {
        warning("Unknown disk or partition type.");
        return false;
    }
    return true;
}

int sniff_partitions(const HFSVolume* hfs, range_ptr partitions, int* count)
{
    // Check disk type
    // Call format-specific method to get partition list
    // Extract ranges
    // Set reference params for the ranges array and count
    // Return the number of partitions found.
    return 0;
}


#pragma mark GPT

uuid_t* swap_gpt_uuid(uuid_t* uuid)
{
    // Because these UUIDs are fucked up.
    void* uuid_buf = uuid;
    *( (u_int32_t*) uuid + 0 ) = OSSwapInt32(*( (u_int32_t*)uuid_buf + 0 ));
    *( (u_int16_t*) uuid + 2 ) = OSSwapInt16(*( (u_int16_t*)uuid_buf + 2 ));
    *( (u_int16_t*) uuid + 3 ) = OSSwapInt16(*( (u_int16_t*)uuid_buf + 3 ));
    *( (u_int64_t*) uuid + 1 ) =             *( (u_int64_t*)uuid_buf + 1 );
    return uuid;
}

const char* partition_type_str(uuid_t uuid, PartitionHint* hint)
{
    for(int i=0; gpt_partition_types[i].name[0] != '\0'; i++) {
        uuid_t test;
        uuid_parse(gpt_partition_types[i].uuid, test);
        int result = uuid_compare(uuid, test);
        if ( result == 0 ) {
            if (hint != NULL) *hint = gpt_partition_types[i].hints;
            return gpt_partition_types[i].name;
        }
    }
    
    static uuid_string_t string;
    uuid_unparse(uuid, string);
    return string;
}

int get_gpt_header(HFSVolume* hfs, GPTHeader* header, MBR* mbr)
{
    if (hfs == NULL || header == NULL) { return EINVAL; }
    
    size_t read_size = lba_size*2;
    char* buf = NULL;
    INIT_BUFFER(buf, read_size);
        
    ssize_t bytes;
    bytes = hfs_read_range(buf, hfs, read_size, 0);
    if (bytes < 0) {
        perror("read");
        return errno;
    }
    
    bool has_protective_mbr = false;
    MBR tmp = *(MBR*)(buf);
    char sig[2] = { 0x55, 0xaa };
    if (memcmp(&sig, &tmp.signature, 2) == 0) {
        // Valid MBR.
        info("MBR found.");
        
        if (mbr != NULL) *mbr = tmp;
        
        if (tmp.partitions[0].type == kMBRTypeGPTProtective) {
            has_protective_mbr = true;
            info("Protective MBR found");
        } else {
            // Invalid GPT protective MBR
            warning("Invalid protective MBR.");
        }
        
    } else {
        info("No protective MBR found.");
    }
    
    off_t offset = lba_size;
    debug("default offset: %lf", offset);
    if (has_protective_mbr) {
        offset = lba_size * tmp.partitions[0].first_sector_lba;
        debug("updated offset to %f", offset);
    }
    *header = *(GPTHeader*)(buf+offset);
    
    FREE_BUFFER(buf);
    
    return 0;
}

bool gpt_sniff(HFSVolume* hfs)
{
    GPTHeader header;
    int result = get_gpt_header(hfs, &header, NULL);
    if (result < 0) { perror("get gpt header"); return false; }
    
    result = memcmp(&header.signature, "EFI PART", 8);
    
    return (!result);
}

void print_mbr(HFSVolume* hfs, MBR* mbr)
{
    PrintHeaderString("Master Boot Record");
    PrintAttributeString("signature", "%#x", S16(*(u_int16_t*)mbr->signature));
    
    FOR_UNTIL(i, 4) {
        MBRPartition* partition = &mbr->partitions[i];
        
        PrintHeaderString("Partition %d", i + 1);
        PrintUIHex  (partition, status);
        PrintUI     (partition, first_sector_head);
        PrintUI     (partition, first_sector_cylinder);
        PrintUI     (partition, first_sector_sector);
        PrintUIHex  (partition, type);
        PrintUI     (partition, last_sector_head);
        PrintUI     (partition, last_sector_cylinder);
        PrintUI     (partition, last_sector_sector);
        PrintUI     (partition, first_sector_lba);
        PrintUI     (partition, sector_count);
        _PrintDataLength("(size)", partition->sector_count*lba_size);
    }
}

void print_gpt(HFSVolume* hfs)
{
    uuid_string_t uuid_str;
    uuid_t* uuid_ptr;
    
    MBR* mbr;
    INIT_BUFFER(mbr, sizeof(MBR));
    
    GPTHeader* header;
    INIT_BUFFER(header, sizeof(GPTHeader));
    
    int result = get_gpt_header(hfs, header, mbr);
    if (result == -1) { perror("get gpt header"); goto CLEANUP; }
    
    uuid_ptr = swap_gpt_uuid(&header->uuid);
    uuid_unparse(*uuid_ptr, uuid_str);
    
    if (mbr->partitions[0].type)
        print_mbr(hfs, mbr);
    
    PrintHeaderString("GPT Partition Map");
    PrintUIHex              (header, revision);
    PrintDataLength         (header, header_size);
    PrintUIHex              (header, header_crc32);
    PrintUIHex              (header, reserved);
    PrintUI                 (header, current_lba);
    PrintUI                 (header, backup_lba);
    PrintUI                 (header, first_lba);
    PrintUI                 (header, last_lba);
    _PrintDataLength        ("(size)", (header->last_lba * lba_size) - (header->first_lba * lba_size) );
    PrintAttributeString    ("uuid", uuid_str);
    PrintUI                 (header, partition_start_lba);
    PrintUI                 (header, partition_entry_count);
    PrintDataLength         (header, partition_entry_size);
    PrintUIHex              (header, partition_crc32);
    
    off_t offset = header->partition_start_lba * lba_size;
    size_t size = header->partition_entry_count * header->partition_entry_size;
    
    void* buf;
    INIT_BUFFER(buf, size);
    
    result = hfs_read_range(buf, hfs, size, offset);
    if (result < 0) {
        perror("read");
        exit(1);
    }
    
    for(int i = 0; i < header->partition_entry_count; i++) {
        const char* type = NULL;
        
        GPTPartitionEntry partition = ((GPTPartitionEntry*)buf)[i];
        
        if (partition.first_lba == 0 && partition.last_lba == 0) break;
        
        PrintHeaderString("Partition: %d (%llu)", i + 1, offset);
        
        uuid_ptr = swap_gpt_uuid(&partition.type_uuid);
        uuid_unparse(*uuid_ptr, uuid_str);
        type = partition_type_str(*uuid_ptr, NULL);
        PrintAttributeString("type", "%s (%s)", type, uuid_str);
        
        uuid_ptr = swap_gpt_uuid(&partition.uuid);
        uuid_unparse(*uuid_ptr, uuid_str);
        PrintAttributeString("uuid", "%s", uuid_str);
        
        PrintAttributeString("first_lba", "%llu", partition.first_lba);
        PrintAttributeString("last_lba", "%llu", partition.last_lba);
        _PrintDataLength("(size)", (partition.last_lba * lba_size) - (partition.first_lba * lba_size) );
        _PrintRawAttribute("attributes", &partition.attributes, sizeof(partition.attributes), 2);
        wchar_t name[37] = {0};
        for(int c = 0; c < 37; c++) name[c] = partition.name[c];
        name[36] = '\0';
        PrintAttributeString("name", "%ls", name);
        
        offset += sizeof(GPTPartitionEntry);
    }
    FREE_BUFFER(buf);
    
CLEANUP:
    FREE_BUFFER(mbr);
    FREE_BUFFER(header);
}

// note: allocates space for partitions
bool gpt_partitions(HFSVolume* hfs, Partition partitions[128], unsigned* count) {
    GPTHeader* header;
    int result;
    off_t offset;
    size_t size;
    void* buf = NULL;
    bool err = false;
    
    INIT_BUFFER(header, sizeof(GPTHeader));
    result = get_gpt_header(hfs, header, NULL);
    if (result == -1) { err = true; goto EXIT; }
    
    offset = header->partition_start_lba * lba_size;
    size = header->partition_entry_count * header->partition_entry_size;
    
    INIT_BUFFER(buf, size);
    result = hfs_read_range(buf, hfs, size, offset);
    if (result < 0) { err = true; goto EXIT; }
    
    uuid_string_t uuid_str;
    uuid_t* uuid_ptr;
    const char* type;
    type = NULL;
    PartitionHint hint;

    for(*count = 0; *count < header->partition_entry_count; (*count)++) {
        GPTPartitionEntry partition = ((GPTPartitionEntry*)buf)[*count];
        if (partition.first_lba == 0 && partition.last_lba == 0) break;
        
        uuid_ptr = swap_gpt_uuid(&partition.type_uuid);
        uuid_unparse(*uuid_ptr, uuid_str);
        type = partition_type_str(*uuid_ptr, &hint);
        
        partitions[*count].offset = partition.first_lba * lba_size;
        partitions[*count].length = (partition.last_lba * lba_size) - partitions[*count].offset;
        partitions[*count].hints  = hint;
    }
    
EXIT:
    if (err) perror("get GPT partiions");
    
    FREE_BUFFER(header);
    FREE_BUFFER(buf);
    FREE_BUFFER(partitions);
    
    return (!err);
}


#pragma mark Core Storage

int get_cs_volume_header(HFSVolume* hfs, CSVolumeHeader* header)
{
    char* buf = valloc(4096);
    if ( buf == NULL ) {
        perror("valloc");
        return -1;
    }
    
    ssize_t bytes = hfs_read_range(buf, hfs, lba_size, 0);
    if (bytes < 0) {
        perror("read");
        return -1;
    }
    
    *header = *(CSVolumeHeader*)(buf);
    free(buf);
    
    return 0;
}

bool cs_sniff(HFSVolume* hfs)
{
    CSVolumeHeader header;
    int result = get_cs_volume_header(hfs, &header);
    if (result == -1) { perror("get cs header"); return false; }
    result = memcmp(&header.signature, "CS", 2);
    
    return (!result);
}

void print_cs_block_header(CSBlockHeader* header)
{
    PrintHeaderString("Core Storage Block Header");
    
    PrintUIHex      (header, checksum);
    PrintUIHex      (header, checksum_seed);
    PrintUI         (header, version);
    PrintUIHex      (header, block_type);
    PrintUI         (header, block_number);
    PrintDataLength (header, block_size);
    PrintBinaryDump (header, flags);
}

void print_cs_block_11(CSMetadataBlockType11* block)
{
    print_cs_block_header(&block->block_header);

    PrintHeaderString("Core Storage Block Type 11");
    
    PrintDataLength(block, size);
    PrintUIHex(block, checksum);
    PrintUIHex(block, checksum_seed);
    PrintUI(block, volume_groups_descriptor_offset);
    PrintUI(block, xml_offset);
    PrintDataLength(block, xml_size);
    PrintUI(block, backup_volume_header_block);
    PrintUI(block, record_count);
    for (int i = 0; i < block->record_count; i++) {
        PrintHeaderString("Record %d", i);
        PrintAttributeString("index", "%llu", block->records[i].index);
        PrintAttributeString("start", "%llu", block->records[i].start);
        PrintAttributeString("end", "%llu", block->records[i].end);
    }
}

void print_cs(HFSVolume* hfs)
{
    CSVolumeHeader* header = malloc(sizeof(CSVolumeHeader));
    int result = get_cs_volume_header(hfs, header);
    if (result == -1) { perror("get cs header"); return; }
    
    print_cs_block_header(&header->block_header);
    
    PrintHeaderString("Core Storage Volume");
    
    PrintDataLength (header, physical_size);
    PrintHFSChar    (header, signature);
    PrintUI         (header, checksum_algo);
    
    PrintUI         (header, metadata_count);
    PrintDataLength (header, metadata_block_size);
    PrintDataLength (header, metadata_size);
    
    static int buf_size = 1024*12;
    void* buf = valloc(buf_size);
    for(int i = 0; i < header->metadata_count; i++) {
        memset(buf, 0, buf_size);
        u_int64_t block_number = header->metadata_blocks[i];
        if (block_number == 0) continue;
        size_t size = header->block_header.block_size;
        off_t offset = (header->metadata_blocks[i] * size);
        
        print("\n# Metadata Block %d (block %#llx; offset %llu)", i, block_number, offset);
        _PrintDataLength("offset", offset);
        
        ssize_t bytes = hfs_read_range(buf, hfs, buf_size, offset);
        if (bytes < 0) { perror("read"); return; }
        
        CSMetadataBlockType11* block = ((CSMetadataBlockType11*)buf);
        print_cs_block_11(block);
        VisualizeData(buf, buf_size);
    }
    free(buf); buf = NULL;
    
    PrintHeaderString("Encryption Key Info");
    PrintDataLength (header, encryption_key_size); //bytes
    PrintUI         (header, encryption_key_algo);
    _PrintRawAttribute("encryption_key_data", &header->encryption_key_data, MIN(header->encryption_key_size, 128), 16);
}


#pragma mark - Apple Partition Map

int get_apm_header(HFSVolume* hfs, APMHeader* header, unsigned partition_number)
{
    char* buf = valloc(4096);
    if ( buf == NULL ) {
        perror("valloc");
        return -1;
    }
    
    ssize_t bytes = hfs_read_range(buf, hfs, lba_size, (lba_size * partition_number));
    if (bytes < 0) {
        perror("read");
        return -1;
    }
    
    *header = *(APMHeader*)(buf);
    free(buf);
    
    swap_APMHeader(header);
    
    return 0;
}

int get_apm_volume_header(HFSVolume* hfs, APMHeader* header)
{
    return get_apm_header(hfs, header, 1);
}

bool apm_sniff(HFSVolume* hfs)
{
    APMHeader header;
    int result = get_apm_volume_header(hfs, &header);
    if (result == -1) { perror("get APM header"); return false; }
    result = memcmp(&header.signature, "MP", 2);
    
    return (!result);
    
    return false;
}

void print_apm(HFSVolume* hfs)
{
    unsigned partitionID = 1;
    
    APMHeader* header = malloc(sizeof(APMHeader));
    
    PrintHeaderString("Apple Partition Map");

    while (1) {
        int result = get_apm_header(hfs, header, partitionID);
        if (result == -1) { perror("get APM header"); return; }
        
        char str[33]; memset(str, '\0', 33);
        
        PrintHeaderString("Partition %d", partitionID);
        PrintHFSChar        (header, signature);
        PrintUI             (header, partition_count);
        PrintUI             (header, partition_start);
        PrintDataLength     (header, partition_length*lba_size);
        
        memcpy(str, &header->name, 32); str[32] = '\0';
        PrintAttributeString("name", "%s", str);
        
        memcpy(str, &header->type, 32); str[32] = '\0';
        for (int i = 0; APMPartitionIdentifers[i].type[0] != '\0'; i++) {
            APMPartitionIdentifer identifier = APMPartitionIdentifers[i];
            if ( (strncasecmp((char*)&header->type, identifier.type, 32) == 0) ) {
                PrintAttributeString("type", "%s (%s)", identifier.name, identifier.type);
            }
        }
        
        PrintUI             (header, data_start);
        PrintDataLength     (header, data_length*lba_size);
        
        PrintRawAttribute   (header, status, 2);
        PrintUIFlagIfSet    (header->status, kAPMStatusValid);
        PrintUIFlagIfSet    (header->status, kAPMStatusAllocated);
        PrintUIFlagIfSet    (header->status, kAPMStatusInUse);
        PrintUIFlagIfSet    (header->status, kAPMStatusBootInfo);
        PrintUIFlagIfSet    (header->status, kAPMStatusReadable);
        PrintUIFlagIfSet    (header->status, kAPMStatusWritable);
        PrintUIFlagIfSet    (header->status, kAPMStatusPositionIndependent);
        PrintUIFlagIfSet    (header->status, kAPMStatusChainCompatible);
        PrintUIFlagIfSet    (header->status, kAPMStatusRealDriver);
        PrintUIFlagIfSet    (header->status, kAPMStatusChainDriver);
        PrintUIFlagIfSet    (header->status, kAPMStatusAutoMount);
        PrintUIFlagIfSet    (header->status, kAPMStatusIsStartup);
        
        PrintUI             (header, boot_code_start);
        PrintDataLength     (header, boot_code_length*lba_size);
        PrintUI             (header, bootloader_address);
        PrintUI             (header, boot_code_entry);
        PrintUI             (header, boot_code_checksum);
        
        memcpy(str, &header->processor_type, 16); str[16] = '\0';
        PrintAttributeString("processor_type", "'%s'", str);
        
        if (partitionID >= header->partition_count) break; else partitionID++;
    }
}
