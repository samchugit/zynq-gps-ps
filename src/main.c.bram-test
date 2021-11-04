#include <stdio.h>
#include "defs.h"
#include "axi-bram.h"

int main()
{
    int len = 20;
    u32 data[len];
    u32 read[len];
    int i;

    for (i = 0; i < len; i++)
    {
        data[i] = i + 10;
        printf("%2dth data, data_write: 0x%lx\t\t\n", i, data[i]);
    }

    BramWriteWords(BRAM_BASE_ADDR, BRAM_DEPTH, 0, data, len);

    BramReadWords(BRAM_BASE_ADDR, BRAM_DEPTH, 0, read, len);

    for (i = 0; i < len; i++)
    {
        printf("%2dth data, data_read: 0x%lx\t\t\n", i, read[i]);
    }

    return 0;
}