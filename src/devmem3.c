#include "devmem3.h"
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef DEVMEM_DEBUG
#define FATAL                                                              \
    {                                                                      \
        fprintf(stderr, "Error at line %d, file %s: %s (%d).\n", __LINE__, \
                __FILE__, strerror(errno), errno);                         \
        return EXIT_FAILURE;                                               \
    }
#else
#define FATAL                \
    {                        \
        return EXIT_FAILURE; \
    }
#endif

/**
 * @brief read data from phy mem
 * @param[in] target target address
 * @param[out] readval reading data
 * @return EXIT_SUCCESS/EXIT_FAILURE
 */
int MemRead(off_t target, uint32_t *readval)
{
    int fd;
    void *map_base;
    void *virt_addr;
    off_t page_size = getpagesize();
    if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
        FATAL;

#ifdef DEVMEM_DEBUG
    fprintf(stderr, "/dev/mem opened.\n");
    fflush(stdout);
#endif

    map_base = mmap(0, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                    target & ~(page_size - 1));
    if (map_base == MAP_FAILED)
        FATAL;
    virt_addr = map_base + (target & (page_size - 1));

    *readval = *(volatile uint32_t *)virt_addr;

#ifdef DEVMEM_DEBUG
    fprintf(stdout, "At 0x%lX (%p) read 0x%X.\n", target, virt_addr, *readval);
    fflush(stdout);
#endif

    if (munmap(map_base, page_size) == -1)
        FATAL;
    close(fd);

    return EXIT_SUCCESS;
}

/**
 * @brief write data into phy mem
 * @param[in] target target address
 * @param[in] writeval data to be written
 * @return EXIT_SUCCESS/EXIT_FAILURE
 */
int MemWrite(off_t target, uint32_t writeval)
{
    uint32_t readback;
    int fd;
    void *map_base;
    void *virt_addr;
    off_t page_size = getpagesize();
    if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
        FATAL;

#ifdef DEVMEM_DEBUG
    fprintf(stderr, "/dev/mem opened.\n");
    fflush(stdout);
#endif

    map_base = mmap(0, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                    target & ~(page_size - 1));
    if (map_base == MAP_FAILED)
        FATAL;
    virt_addr = map_base + (target & (page_size - 1));

    *(volatile uint32_t *)virt_addr = writeval;
    readback = *(volatile uint32_t *)virt_addr;

#ifdef DEVMEM_DEBUG
    fprintf(stdout, "At 0x%lX (%p) write 0x%X, readback 0x%X.\n", target,
            virt_addr, writeval, readback);
    fflush(stdout);
#endif

    if (munmap(map_base, page_size) == -1)
        FATAL;
    close(fd);

    if (readback != writeval)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

/**
 * @brief read words block from phy mem
 * @param[in] addr block start address
 * @param[in] length block length to be read
 * @param[out] data data read to be read
 * @return void
 */
void MemReadWords(off_t addr, uint32_t length, uint32_t *data)
{
    off_t target;
    for (uint32_t i = 0; i < length; i++) {
        target = addr + i * sizeof(uint32_t);
        MemRead(target, data + i);
    }
}

/**
 * @brief write words block into phy mem
 * @param[in] addr block start address
 * @param[in] length block length to be written
 * @param[in] data data to be written
 * @return void
 */
void MemWriteWords(off_t addr, uint32_t length, uint32_t *data)
{
    off_t target;
    for (uint32_t i = 0; i < length; i++) {
        target = addr + i * sizeof(uint32_t);
        MemWrite(target, *(data + i));
    }
}