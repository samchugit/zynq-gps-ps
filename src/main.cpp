#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "gps.h"

#define RECV_MAX 1000

extern uint8_t prompt_i[];

int main()
{
    uint8_t ch = 0;
    ChanReset();
    ChanStart(ch, 1);
    ChanTask();

#ifdef CHANNEL_TEST
    for (int i = 0; i < 76; i++)
    {
        Info("Injectint {}th data", i);
        DataInject(ch, prompt_i + i * RECV_MAX);
        TestBitSync(ch);
        TestBitSampling(ch);
    }
#endif

    return 0;
}