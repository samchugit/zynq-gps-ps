#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "ephemeris.h"
#include "gps.h"

const int RECV_MS = 1000;
const int NAV_FRAME = 300;
const int BIT_SYNC_MAX = 25;
const int BIT_SYNC_HIGH = 20;
const int BIT_SYNC_LOW = 10;

const uint8_t preambleUpright[] = {1, 0, 0, 0, 1, 0, 1, 1};
const uint8_t preambleInverse[] = {0, 1, 1, 1, 0, 1, 0, 0};

static int parity(uint8_t *p, uint8_t *word, uint8_t D29, uint8_t D30)
{
    uint8_t *d = word - 1;
    for (int i = 1; i < 25; i++)
        d[i] ^= D30;
    p[0] = D29 ^ d[1] ^ d[2] ^ d[3] ^ d[5] ^ d[6] ^ d[10] ^ d[11] ^ d[12] ^ d[13] ^ d[14] ^ d[17] ^ d[18] ^ d[20] ^ d[23];
    p[1] = D30 ^ d[2] ^ d[3] ^ d[4] ^ d[6] ^ d[7] ^ d[11] ^ d[12] ^ d[13] ^ d[14] ^ d[15] ^ d[18] ^ d[19] ^ d[21] ^ d[24];
    p[2] = D29 ^ d[1] ^ d[3] ^ d[4] ^ d[5] ^ d[7] ^ d[8] ^ d[12] ^ d[13] ^ d[14] ^ d[15] ^ d[16] ^ d[19] ^ d[20] ^ d[22];
    p[3] = D30 ^ d[2] ^ d[4] ^ d[5] ^ d[6] ^ d[8] ^ d[9] ^ d[13] ^ d[14] ^ d[15] ^ d[16] ^ d[17] ^ d[20] ^ d[21] ^ d[23];
    p[4] = D30 ^ d[1] ^ d[3] ^ d[5] ^ d[6] ^ d[7] ^ d[9] ^ d[10] ^ d[14] ^ d[15] ^ d[16] ^ d[17] ^ d[18] ^ d[21] ^ d[22] ^ d[24];
    p[5] = D29 ^ d[3] ^ d[5] ^ d[6] ^ d[8] ^ d[9] ^ d[10] ^ d[11] ^ d[13] ^ d[15] ^ d[19] ^ d[22] ^ d[23] ^ d[24];
    return memcmp(d + 25, p, 6);
}

/*
  recv_buf
  |- buf 1 ---------------------|- buf 2 -----------------------|
  0    1    2    3   ...   999  1000 1001 1002 1003 ...    1999
  ┌────┬────┬────┬── ... ──┬────┬────┬────┬────┬──  ...  ──┬────┐
  └────┴────┴────┴── ... ──┴────┴────┴────┴────┴──  ...  ──┴────┘
       ↑                             ↑
       bit_head                      bit_tail
 */

struct CHANNEL
{
    uint8_t sv;

    uint8_t recv_buf[RECV_MS * 2];
    uint16_t buf_tail;  // recv_buf data tail
    uint16_t bit_head;  // bit synced data head
    uint16_t bit_tail;  // bit synced data tail
    uint8_t bit_synced; // bit synced flag

    uint8_t nav_buf[NAV_FRAME + RECV_MS / 20];
    uint16_t nav_wp; // nav buf write position
    uint16_t nav_rp; // nav buf read position

    void Reset();
    void BitSync();
    void BitSampling();
    uint16_t ParityCheck(uint8_t *buf, uint16_t *nbits);
    void FrameSync();
};

void CHANNEL::Reset()
{
    memset(recv_buf, 0, RECV_MS * 2);
    buf_tail = 0;
    bit_head = 0;
    bit_tail = bit_head + RECV_MS;
    bit_synced = 0;

    memset(nav_buf, 0, NAV_FRAME + RECV_MS / 20);
    nav_wp = 0;
    nav_rp = 0;
}

void CHANNEL::BitSync()
{
    // bit alright synced
    if (bit_synced == 1)
        return;

    // start bit sync
    uint8_t ip;
    uint8_t ip_last;
    uint8_t edges[20] = {0};

    ip = recv_buf[0];
    uint16_t ms_cnt;
    for (ms_cnt = 0; ms_cnt < RECV_MS; ms_cnt++)
    {
        ip_last = ip;
        ip = recv_buf[ms_cnt];
        uint8_t code_cnt = ms_cnt % 20;
        if (1 == (ip ^ ip_last))
            edges[code_cnt]++;
    }

    uint8_t edge_total = 0;
    uint8_t max_edge_num = 0;
    uint8_t max_edge_idx = 0;
    uint8_t sec_edge_num = 0;
    uint8_t sec_edge_idx = 0;
    for (int i = 0; i < 20; i++)
    {
        edge_total += edges[i];
        if (edges[i] > max_edge_num)
        {
            sec_edge_num = max_edge_num;
            sec_edge_idx = max_edge_idx;
            max_edge_num = edges[i];
            max_edge_idx = i;
        }
        else if (edges[i] > sec_edge_num)
        {
            sec_edge_num = edges[i];
            sec_edge_idx = i;
        }
    }

    // edge enough && max edge enough && sec edge low enough
    if (edge_total > BIT_SYNC_MAX && max_edge_num > BIT_SYNC_HIGH && sec_edge_num < BIT_SYNC_LOW)
    {
        bit_synced = 1;
        bit_head += max_edge_idx;
        bit_tail += max_edge_idx;
    }
    else
    {
        Reset();
    }
}

void CHANNEL::BitSampling()
{
    uint8_t cnt = 0;
    uint8_t bit_sum = 0;

    uint16_t i;
    for (i = bit_head; i < bit_tail; i++)
    {
        bit_sum += recv_buf[i];
        if (++cnt >= 20)
        {
            if (bit_sum > 10)
                nav_buf[nav_wp++] = 1;
            else
                nav_buf[nav_wp++] = 0;
            cnt = 0;
            bit_sum = 0;
        }
    }

    buf_tail -= RECV_MS;
    memcpy(recv_buf, recv_buf + RECV_MS, buf_tail);
}

uint16_t CHANNEL::ParityCheck(uint8_t *buf, uint16_t *nbits)
{
    uint8_t p[6];

    // upright or inverted preamble, setting of parity bits resolves phase ambiguity.
    if (0 == memcmp(buf, preambleUpright, 8))
        p[4] = p[5] = 0;
    else if (0 == memcmp(buf, preambleInverse, 8))
        p[4] = p[5] = 1;
    else
        return *nbits = 1;

    // parity check up to ten 30-bit words ...
    uint16_t i;
    for (i = 0; i < 300; i += 30)
    {
        if (0 != parity(p, buf + i, p[4], p[5]))
            return *nbits = i + 30;
    }

    Ephemeris[sv].Subframe(buf);
    *nbits = 300;
    return 0;
}

void CHANNEL::FrameSync()
{
    while (nav_wp >= 300) // enough for a subframe
    {
        uint16_t nbits;
        uint16_t parity_ok = ParityCheck(nav_buf, &nbits);
        nav_wp -= nbits;
        memcpy(nav_buf, nav_buf + nbits, nav_wp);
    }
}

static CHANNEL Chans[NUM_CHANS];

void ChanReset()
{
    uint8_t i;
    for (i = 0; i < NUM_CHANS; i++)
    {
        Chans[i].Reset();
    }
}
