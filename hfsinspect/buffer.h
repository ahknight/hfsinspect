//
//  buffer.h
//  buffer
//
//  Created by Adam Knight on 5/13/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#ifndef hfsinspect_buffer_h
#define hfsinspect_buffer_h

// For passing around structs, blocks, etc.
struct Buffer {
    char*       data;       // The actual data.
    size_t      size;       // Curent size of .data.
    size_t      capacity;   // The maximum amount of memory that can be allocated.
    size_t      offset;     // Position to write new data at (should follow writes)
};
typedef struct Buffer Buffer;


#pragma mark Lifetime Functions

// Returns a malloced Buffer of a given size.
Buffer  buffer_alloc    (size_t size);

// Frees a buffer, destroying all data.
void    buffer_free     (Buffer *buffer);

// Allocates and returns a new Buffer with a copy of a range of another Buffer.
Buffer  buffer_slice    (const Buffer *buffer, size_t size, size_t offset);


#pragma mark Sizing Functions

// Grows or shrinks the buffer, including updating capacity as needed.
size_t  buffer_resize       (Buffer *buffer, size_t new_size);

// Resyncs internal values to the current buffer allocation.
size_t  buffer_update       (Buffer *buffer);


#pragma mark Setting Data

// Zeroes the data in the buffer without changing buffer metadata.
void    buffer_zero     (Buffer *buffer);

// Clears a buffer and sets the offset to 0.
void    buffer_reset    (Buffer *buffer);

// Sets the content of a buffer, zeroing and replacing the existing data.
ssize_t buffer_set      (Buffer *buffer, const char* data, size_t length);

// Appends a block of data to a buffer; respects the current offset and will overwrite data.
ssize_t buffer_append   (Buffer *buffer, const char* data, size_t length);

// Cover for pread that reads into the buffer.
ssize_t buffer_pread    (int fh, Buffer *buffer, size_t size, size_t offset);

#endif
