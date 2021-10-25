#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "defs.h"
#include "axi-bram.h"

/**
 * @brief open physical memory /dev/mem
 * @return file descriptor if success, -1 if failed
 */
int __OpenPhysicalMem()
{
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0)
    {
#ifdef DEBUG_INFO
        printf("can not open /dev/mem\n");
#endif
        return -1;
    }
#ifdef DEBUG_INFO
    printf("/dev/mem is opened\n");
#endif
    return fd;
}

/**
 * @brief close physical memory /dev/mem
 * @param fd file descriptor of opened /dev/mem file
 * @return 0 if success, -1 if failed
 */
int __ClosePhysicalMem(int fd)
{
    int ret = close(fd);
    if (ret < 0)
    {
#ifdef DEBUG_INFO
        printf("can not close /dev/mem\n");
#endif
        return -1;
    }
#ifdef DEBUG_INFO
    printf("/dev/mem is closed\n");
#endif
    return 0;
}

/**
 * @brief map BRAM physical address to user space memory
 * @param fd file descriptor of opened /dev/mem file
 * @param bram_base_addr base address of BRAM
 * @param bram_depth depth of BRAM
 * @return map address if success, MAP_FAILED if failed
 */
u32 *__MapBram(int fd, u32 bram_base_addr, u32 bram_depth)
{
    u32 *bram32_vptr;
    bram32_vptr = (u32 *)mmap(NULL,
                              bram_depth,
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED,
                              fd,
                              bram_base_addr);
#ifdef DEBUG_INFO
    if (bram32_vptr == MAP_FAILED)
    {
        printf("mmap failed\n");
    }
    else
    {
        printf("mmap successful\n");
    }
#endif
    return bram32_vptr;
}

/**
 * @brief unmap BRAM physical address on user space memory
 * @param bram32_vptr BRAM mapped address
 * @param bram_depth depth of BRAM
 * @return 0 if success, -1 if failed
 */
int __UnmapBram(void *bram32_vptr, u32 bram_depth)
{
    int ret = munmap(bram32_vptr, bram_depth);
    if (ret < 0)
    {
#ifdef DEBUG_INFO
        printf("can not unmap BRAM\n");
#endif
        return -1;
    }
#ifdef DEBUG_INFO
    printf("BRAM is unmapped\n");
#endif
    return 0;
}

/**
 * @brief write a word(u32) to BRAM at offset
 * @param bram32_vptr BRAM mapped address
 * @param bram_depth depth of BRAM
 * @param data_offset offset of data
 * @param word the data to be write
 * @return 0 if success, -1 if failed
 */
int __BramWriteWord(u32 *bram32_vptr, u32 bram_depth, u32 data_offset, u32 word)
{
    if (data_offset < bram_depth)
    {
        bram32_vptr[data_offset] = word;
#ifdef DEBUG_INFO
        printf("word write to BRAM\n");
#endif
        return 0;
    }
#ifdef DEBUG_INFO
    printf("failed to write word to BRAM\n");
#endif
    return -1;
}

/**
 * @brief read a word(u32) from BRAM at offset
 * @param bram32_vptr BRAM mapped address
 * @param bram_depth depth of BRAM
 * @param data_offset offset of data
 * @return data if success, -1 if failed
 */
u32 __BramReadWord(u32 *bram32_vptr, u32 bram_depth, u32 data_offset)
{
    if (data_offset < bram_depth)
    {
#ifdef DEBUG_INFO
        printf("word read from BRAM\n");
#endif
        return bram32_vptr[data_offset];
    }
#ifdef DEBUG_INFO
    printf("failed to read word from BRAM\n");
#endif
    return -1;
}

/**
 * @brief write data to BRAM at offset
 * @param bram_base_addr base address of BRAM
 * @param bram_depth depth of BRAM
 * @param data_offset offset of data
 * @param data data to write
 * @param data_len length of data
 * @return 0 if success, -1 if overflow
 */
int BramWriteWords(u32 bram_base_addr, u32 bram_depth, u32 data_offset, u32 *data, u32 data_len)
{
    int fd = __OpenPhysicalMem();
    u32 *bram32_vptr = __MapBram(fd, bram_base_addr, bram_depth);
    u32 i;
    int ret = 0;

    if (data_offset + data_len < bram_depth)
    {
        for (i = 0; i < data_len; i++)
        {
            __BramWriteWord(bram32_vptr, bram_depth, data_offset + i, data[i]);
        }
#ifdef DEBUG_INFO
        printf("words write to BRAM\n");
#endif
        ret = 0;
    }
    else
    {
#ifdef DEBUG_INFO
        printf("failed to write words to BRAM\n");
#endif
        ret = -1;
    }

    __UnmapBram(bram32_vptr, bram_depth);
    __ClosePhysicalMem(fd);
    return ret;
}

/**
 * @brief read data from BRAM at offset
 * @param bram_base_addr base address of BRAM
 * @param bram_depth depth of BRAM
 * @param data_offset offset of data
 * @param data data to write
 * @param data_len length of data
 * @return 0 if success, -1 if overflow
 */
int BramReadWords(u32 bram_base_addr, u32 bram_depth, u32 data_offset, u32 *data, u32 data_len)
{
    int fd = __OpenPhysicalMem();
    u32 *bram32_vptr = __MapBram(fd, bram_base_addr, bram_depth);
    u32 i;
    int ret = 0;

    if (data_offset + data_len < bram_depth)
    {
        for (i = 0; i < data_len; i++)
        {
            data[i] = __BramReadWord(bram32_vptr, bram_depth, data_offset + i);
        }
#ifdef DEBUG_INFO
        printf("words read from BRAM\n");
#endif
        ret = 0;
    }
    else
    {
#ifdef DEBUG_INFO
        printf("failed to read words from BRAM\n");
#endif
        ret = -1;
    }

    __UnmapBram(bram32_vptr, bram_depth);
    __ClosePhysicalMem(fd);
    return ret;
}