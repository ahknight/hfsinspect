#include <stdlib.h>
#include <stdio.h>
#include "../src/misc/_endian.h"

int main (int argc, char const *argv[])
{
    uint16_t a = 1;
    printf("N16: %u\n", a);
    Swap16(a);
    printf("S16: %u\n", a);
    
    uint32_t b = 1;
    printf("N32: %u\n", b);
    Swap32(b);
    printf("S32: %u\n", b);
    
    uint64_t c = 1;
    printf("N64: %llu\n", c);
    Swap64(c);
    printf("S64: %llu\n", c);
    
    return 0;
}