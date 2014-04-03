#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <wchar.h>
#include <math.h>

//#include <sys/_types/_timespec.h>

#define psizeof(x) printf("%-12s %-3ld (%u bits)\n", #x, sizeof(x), (unsigned)sizeof(x) * 8 );

int main (int argc, char const *argv[])
{
    psizeof(char);
    psizeof(wchar_t);
    
    psizeof(bool);
    psizeof(short);
    psizeof(int);
    psizeof(long);
    psizeof(long long);
    psizeof(float);
    psizeof(double);
    psizeof(long double);
    psizeof(off_t);
    psizeof(size_t);
    psizeof(ssize_t);

    // 24, 40, 48, 56 are not implemented and are defined as optional in the standard
    // not sure who would actually implement them...
    psizeof(uint8_t);
    psizeof(uint16_t);
    // psizeof(uint24_t);
    psizeof(uint32_t);
    // psizeof(uint40_t);
    // psizeof(uint48_t);
    // psizeof(uint56_t);
    psizeof(uint64_t);
    
//    psizeof(struct timespec);  //128b
//    struct timespec timespec;
//    psizeof(timespec.tv_sec);  //64b
//    psizeof(timespec.tv_nsec); //64b
    
    return 0;
}
