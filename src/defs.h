#ifndef __DEFS_H
#define __DEFS_H

// #define DEBUG_INFO

/**
 * `BRAM_BASE_ADDR' is the offset address of BRAM Controller
 * `BRAM_DEPTH' is the memory depth of the BRAM
 * BRAM word length is 4bytes = 32bits
 */
#define BRAM_BASE_ADDR 0x40000000
#define BRAM_DEPTH 0x2000

#ifndef __u64_defined
#define __u64_defined
typedef unsigned long long u64;
#endif

#ifndef __u32_defined
#define __u32_defined
typedef unsigned long u32;
#endif

#endif /* __DEFS_H */