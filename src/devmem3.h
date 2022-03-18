#ifndef _DEVMEM3_H
#define _DEVMEM3_H 1

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <inttypes.h>
#include <stdio.h>

// #define DEVMEM_DEBUG

int MemRead(off_t target, uint32_t *readval);
int MemWrite(off_t target, uint32_t writeval);
void MemReadWords(off_t addr, uint32_t length, uint32_t *data);
void MemWriteWords(off_t addr, uint32_t length, uint32_t *data);

#ifdef __cplusplus
} // closing brace for extern "C"
#endif // __cplusplus

#endif // DEVMEM3_H