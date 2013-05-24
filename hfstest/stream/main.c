#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "cfile.h"

int main (int argc, char const *argv[])
{
    STREAM stream = {};
    ssize_t result = init_cfile_stream(&stream, "/.error", "r");
    result = stream_open(stream);
    if (result < 0) {
        perror("open");
        exit(1);
    }
    char* buf = malloc(1024);
    result = stream_read(stream, buf, 1024);
    if (result < 0) {
        perror("read");
//        exit(1);
    }
    
    result = stream_close(stream);
    if (result < 0 && errno) {
        perror("close");
//        exit(1);
    }
    buf[1023] = '\0';
    printf("buf: %s", buf);
    return 0;
}