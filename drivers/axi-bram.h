#ifndef __AXI_BRAM_H
#define __AXI_BRAM_H

#ifndef __u64_defined
#define __u64_defined
typedef unsigned long long u64;
#endif

#ifndef __u32_defined
#define __u32_defined
typedef unsigned long u32;
#endif

int OpenPhysicalMem();
int ClosePhysicalMem(int fd);
u32 *MapBram(int fd, u32 bram_base_addr, u32 bram_depth);
int UnmapBram(void *bram32_vptr, u32 bram_depth);
int BramWriteWord(u32 *bram32_vptr, u32 bram_depth, u32 bram_offset, u32 word);
u32 BramReadWord(u32 *bram32_vptr, u32 bram_depth, u32 bram_offset);
int BramWriteWords(u32 bram_base_addr, u32 bram_depth, u32 bram_offset, u32 *data, u32 data_len);
int BramReadWords(u32 bram_base_addr, u32 bram_depth, u32 bram_offset, u32 *data, u32 data_len);

#endif /* __AXI_BRAM_H */