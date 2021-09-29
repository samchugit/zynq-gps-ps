#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/mman.h>

#ifndef __u32_defined
#define __u32_defined
typedef unsigned long u32;
#endif

// #define DEBUG_INFO
#define READ 1
#define WRITE 2

#define DEFAULT_BRAM_BASE_ADDR 0x40000000
#define DEFAULT_BRAM_DEPTH 0x2000
#define DEFAULT_DATA_OFFSET 0x0
#define DEFAULT_DATA_LEN 0x1

/**
 * @brief open physical memory /dev/mem
 * @return file descriptor if success, -1 if failed
 */
int OpenPhysicalMem()
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
int ClosePhysicalMem(int fd)
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
u32 *MapBram(int fd, u32 bram_base_addr, u32 bram_depth)
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
int UnmapBram(void *bram32_vptr, u32 bram_depth)
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
int BramWriteWord(u32 *bram32_vptr, u32 bram_depth, u32 data_offset, u32 word)
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
u32 BramReadWord(u32 *bram32_vptr, u32 bram_depth, u32 data_offset)
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
    int fd = OpenPhysicalMem();
    u32 *bram32_vptr = MapBram(fd, bram_base_addr, bram_depth);
    u32 i;
    int ret = 0;

    if (data_offset + data_len < bram_depth)
    {
        for (i = 0; i < data_len; i++)
        {
            BramWriteWord(bram32_vptr, bram_depth, data_offset + i, data[i]);
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

    UnmapBram(bram32_vptr, bram_depth);
    ClosePhysicalMem(fd);
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
    int fd = OpenPhysicalMem();
    u32 *bram32_vptr = MapBram(fd, bram_base_addr, bram_depth);
    u32 i;
    int ret = 0;

    if (data_offset + data_len < bram_depth)
    {
        for (i = 0; i < data_len; i++)
        {
            data[i] = BramReadWord(bram32_vptr, bram_depth, data_offset + i);
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

    UnmapBram(bram32_vptr, bram_depth);
    ClosePhysicalMem(fd);
    return ret;
}

void usage(void)
{
    fprintf(stderr, "Usage: axi-bram read|write <options> [<data>]\n"
                    "       axi-bram read -b 0x1000 -d 0x100 -o 0x10 -l 0x10\n"
                    "       axi-bram write -b 1000 -d 100 -o 10 -l 2 abcdef12 12abcdef\n"
                    "Options:\n"
                    "  -b <bram_base_addr>  BRAM base address in hex, default is 0x%X\n"
                    "  -d <bram_depth>      BRAM depth in hex, default is 0x%X\n"
                    "  -o <data_offset>     Data offset in hex, default is 0x%X\n"
                    "  -l <data_length>     Data length in hex\n",
            DEFAULT_BRAM_BASE_ADDR, DEFAULT_BRAM_DEPTH, DEFAULT_DATA_OFFSET);
    return;
}

int main(int argc, char *argv[])
{
    int flag = 0;
    int opt;
    u32 bram_base_addr = DEFAULT_BRAM_BASE_ADDR;
    u32 bram_depth = DEFAULT_BRAM_DEPTH;
    u32 data_offset = DEFAULT_DATA_OFFSET;
    u32 data_len = DEFAULT_DATA_LEN;

    if (argc < 4)
    {
        usage();
        exit(1);
    }
    if (strcmp(argv[1], "read") == 0)
    {
        flag = READ;
    }
    else if (strcmp(argv[1], "write") == 0)
    {
        flag = WRITE;
    }

    while ((opt = getopt(argc, argv, "b:d:o:l:v")) != -1)
    {
        switch (opt)
        {
        case 'b':
            sscanf(optarg, "%lx", &bram_base_addr);
            break;
        case 'd':
            sscanf(optarg, "%lx", &bram_depth);
            break;
        case 'o':
            sscanf(optarg, "%lx", &data_offset);
            break;
        case 'l':
            sscanf(optarg, "%lx", &data_len);
            break;
        case '?':
            usage();
            exit(1);
        default:
            break;
        }
    }

    u32 i;
    u32 addr;
    u32 data[data_len];

    if (flag == READ)
    {
        printf("Reading 0x%lx words from BRAM:\n", data_len);
        BramReadWords(bram_base_addr, bram_depth, data_offset, data, data_len);
        for (i = 0; i < data_len; i++)
        {
            addr = bram_base_addr + 4 * (data_offset + i);
            printf("  0x%08lX: 0x%08lX\n", addr, data[i]);
        }
    }
    else if (flag == WRITE)
    {
        printf("Writing 0x%lx words to BRAM:\n", data_len);
        for (i = 0; i < data_len; i++)
        {
            addr = bram_base_addr + 4 * (data_offset + i);
            data[i] = strtoul(argv[argc - data_len + i], NULL, 16);
            printf("  0x%08lX: 0x%08lX\n", addr, data[i]);
        }
        BramWriteWords(bram_base_addr, bram_depth, data_offset, data, data_len);
    }
    else
    {
        usage();
        exit(1);
    }
    return 0;
}