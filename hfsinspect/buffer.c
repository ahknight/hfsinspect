//
//  buffer.c
//  buffer
//
//  Created by Adam Knight on 5/13/13.
//  Copyright (c) 2013 Adam Knight. All rights reserved.
//

#include "buffer.h"
#include <unistd.h>
#include <sys/param.h>  //MIN/MAX

#pragma mark Lifetime Functions

// Returns a malloced Buffer of a given size.
Buffer buffer_alloc(size_t size)
{
    Buffer buffer;
    buffer.capacity = size;
    buffer.offset = 0;
    buffer.data = malloc(size);
    buffer.size = malloc_size(buffer.data);
    
    buffer_zero(&buffer);
    
    if (getenv("DEBUG_BUFFER")) print_trace("%p: buffer_alloc\n", buffer.data);
    
    return buffer;
}
    
// Frees a buffer, destroying all data.
void buffer_free(Buffer *buffer)
{
    if (getenv("DEBUG_BUFFER")) print_trace("%p: buffer_free\n", buffer->data);
    
    if(malloc_size(buffer->data)) {
        buffer_zero(buffer);
        
        free(buffer->data);
        buffer->data = NULL;
        buffer->capacity = 0;
        buffer->size = 0;
    } else {
        error("DOUBLE FREE!");
    }
}

// Allocates and returns a new Buffer with a copy of a range of another Buffer.
Buffer buffer_slice(const Buffer *buffer, size_t size, size_t offset)
{
    //FIXME: Range check problem. Need to cover if size and/or offset are too large (three states).
    if ((size + offset) > malloc_size(buffer->data)) {
        size = malloc_size(buffer->data) - offset;
    }
    
    Buffer new_buffer = buffer_alloc(size);
    buffer_set(&new_buffer, buffer->data+offset, size);
    return new_buffer;
}


#pragma mark Sizing Functions

size_t buffer_resize(Buffer *buffer, size_t new_size)
{
    // FIXME: If growing, zero area past the current size after completion.
    
    if (new_size != malloc_size(buffer->data)) {
        buffer->data = realloc(buffer->data, new_size);
        buffer_update(buffer);
    }
    
    return buffer->size;
}

size_t buffer_update(Buffer *buffer)
{
    buffer->size = malloc_size(buffer->data);
    buffer->capacity = MAX(buffer->size, buffer->capacity);
    
    return buffer->size;
}


#pragma mark Setting Data

// Zeroes the data in the buffer without changing buffer metadata.
void buffer_zero(Buffer *buffer)
{
    memset(buffer->data, '\0', malloc_size(buffer->data));
}

// Clears a buffer and sets the offset to 0.
void buffer_reset(Buffer *buffer)
{
    buffer_zero(buffer);
    buffer->offset = 0;
}

// Sets the content of a buffer, zeroing and replacing the existing data.
ssize_t buffer_set(Buffer *buffer, const char* data, size_t length)
{
    buffer_reset(buffer);
    return buffer_append(buffer, data, length);
}

// Appends a block of data to a buffer; respects the current offset and will overwrite data.
ssize_t buffer_append(Buffer *buffer, const char* data, size_t length)
{
    size_t new_size = (buffer->offset + length);
    
    // Will this read put us past the specified growth limit?
    if(buffer->capacity < new_size) {
        errno = ENOBUFS;
        return -1;
    }
    
    // The new size is less than capacity. Do we need to grow to handle it?
    if( new_size > malloc_size(buffer->data) ) {
        size_t result = buffer_resize(buffer, new_size);
        if (result < new_size) {
            errno = ENOBUFS;
            return -1;
        }
    }
    
    // Append the data
    void* destination = buffer->data + buffer->offset;
    memcpy(destination, data, length);

    // Kick the can
    buffer->offset += length;
    
    return length;
}

// Cover for pread that reads into the buffer
ssize_t buffer_pread(int fh, Buffer *buffer, size_t size, size_t offset) {
    char* data = malloc(size);
    
    ssize_t result = 0;
//    debug("Seeking to %zd", offset);
    result = lseek(fh, offset, SEEK_SET);
    if (result < 0) {
        perror("lseek");
        critical("seek error");
    } else {
//        debug("Reading %zd bytes", size);
        result = read(fh, data, size);
        if (result < 0) {
            perror("buffer_read");
            critical("read error");
        } else {
            result = buffer_append(buffer, data, (size_t)result);
        }
    }
    
    free(data);
    return result;
}
