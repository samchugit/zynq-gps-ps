#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "devmem3.h"
#include "ephemeris.h"
#include "gps.h"

const int RECV_MS = 1000;
const int NAV_FRAME = 300;
const int BIT_SYNC_MAX = 15;
const int BIT_SYNC_HIGH = 12;
const int BIT_SYNC_LOW = 5;

const uint8_t preambleUpright[] = {1, 0, 0, 0, 1, 0, 1, 1};
const uint8_t preambleInverse[] = {0, 1, 1, 1, 0, 1, 0, 0};

static uint32_t BusyFlags;

/**
 * @brief parity check for a word
 * @param p parity value of this word
 * @param word word to be checked
 * @param D29 29th bit of last word
 * @param D30 30th bit of last word
 * @return 0 if parity good
 */
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

/**
  recv_buf
  |- buf 1 ---------------------|- buf 2 -----------------------|
  0    1    2    3   ...   999  1000 1001 1002 1003 ...    1999
  ┌────┬────┬────┬── ... ──┬────┬────┬────┬────┬──  ...  ──┬────┐
  └────┴────┴────┴── ... ──┴────┴────┴────┴────┴──  ...  ──┴────┘
       ↑                             ↑
       bit_head                      bit_tail

  nav_buf
  0    1    2    3   ...   299  300  301 ...   349
  ┌────┬────┬────┬── ... ──┬────┬────┬── ... ──┬────┐
  └────┴────┴────┴── ... ──┴────┴────┴── ... ──┴────┘
                                ↑
                                nav_tail
 */

struct CHANNEL
{
    uint8_t ch; // channel id
    uint8_t sv; // PRN of the satellite

    uint8_t data_fetch_ok; // data fetch flag (1 for good)

    uint8_t recv_buf[RECV_MS * 2];
    uint16_t buf_tail;   // recv_buf data tail
    uint16_t bit_head;   // bit synced data head
    uint16_t bit_tail;   // bit synced data tail
    uint8_t bit_sync_ok; // bit synced flag (1 for good)

    uint8_t nav_buf[NAV_FRAME + RECV_MS / 20];
    uint16_t nav_tail;     // nav tail
    uint8_t frame_sync_ok; // frame synced flag (0 for good)

    void RecvReset();
    void NavReset();
    void Reset();
    void DataFetch();
    void BitSync();
    void BitSampling();
    uint16_t ParityCheck(uint8_t *buf, uint16_t *nbits);
    void FrameSync();
    void Service();
};

/**
 * @brief reset receive buffer of current channel
 */
void CHANNEL::RecvReset()
{
    memset(recv_buf, 0, RECV_MS * 2);
    data_fetch_ok = 0;
    buf_tail = 0;
    bit_head = 0;
    bit_tail = bit_head + RECV_MS;
    bit_sync_ok = 0;
}

/**
 * @brief reset nav buffer of current channel
 */
void CHANNEL::NavReset()
{
    memset(nav_buf, 0, NAV_FRAME + RECV_MS / 20);
    nav_tail = 0;
    frame_sync_ok = 1;
}

/**
 * @brief reset current channel
 */
void CHANNEL::Reset()
{
#ifdef CHANNEL_TEST
    sv = 0;
#endif
    RecvReset();
    NavReset();
}

/**
 * @brief fetch nav data from axi-reg (phy mem)
 */
void CHANNEL::DataFetch()
{
    static uint32_t rx_state_last;
    uint32_t rx_state;
    MemRead(0x50004400, &rx_state);

#ifdef LOG_DEBUG
    Debug("DataFetch rx_state: {}, rx_state_last: {}", rx_state, rx_state_last);
#endif

    if (rx_state_last == rx_state)
    {
#ifdef LOG_DEBUG
        Debug("DataFetch data not accepted", 0);
#endif
        data_fetch_ok = 0;
        return;
    }
#ifdef LOG_DEBUG
    Debug("DataFetch data accepted", 0);
#endif
    data_fetch_ok = 1;
    rx_state_last = rx_state;

    uint32_t recv[RECV_MS];
    switch (rx_state)
    {
    case 1:
        MemReadWords(0x500c0000, RECV_MS, recv);
        break;
    case 2:
        MemReadWords(0x50080000, RECV_MS, recv);
        break;
    default:
        break;
    }
    // TODO: temporary fix, uint32_t[RECV_MS] -> uint8_t[RECV_MS]
    for (uint16_t i = 0; i < RECV_MS; i++)
    {
        if (recv[i] == 0)
            recv_buf[buf_tail++] = 0;
        else
            recv_buf[buf_tail++] = 1;
    }
#ifdef LOG_DEBUG
    Debug("DataFetch buf_tail: {}", buf_tail);
    Debug("DataFetch updated recv_buf: {}", array2str(recv_buf, buf_tail));
#endif
}

/**
 * @brief find bit offset of 'recv_buf'
 * @return void
 */
void CHANNEL::BitSync()
{
    // Return if bit alright synced.
    if (bit_sync_ok == 1)
        return;

    uint8_t ip;
    uint8_t ip_last;
    uint8_t edges[20] = {0};

    // Find and index all edges.
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

    // Find the max & sec edge.
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

#ifdef LOG_DEBUG
    Debug("BitSync edge_total:{}, max_edge_num:{}, sec_edge_num:{}", edge_total, max_edge_num, sec_edge_num);
#endif

    // Judge whether bit synced.
    // Judgment takes edge_total, max_edge_num, sec_edge_num into account
    if (edge_total > BIT_SYNC_MAX && max_edge_num > BIT_SYNC_HIGH && sec_edge_num < BIT_SYNC_LOW)
    {
        bit_sync_ok = 1;
        bit_head += max_edge_idx;
        bit_tail += max_edge_idx;
    }
    else
        RecvReset();
}

/**
 * @brief resample 1kHz signal into 50bps NAV message
 * @return void
 */
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
            if (bit_sum > 10) // judge 20ms data
                nav_buf[nav_tail++] = 1;
            else
                nav_buf[nav_tail++] = 0;
            cnt = 0;
            bit_sum = 0;
        }
    }

    // Remove sampled signal from 'recv_buf'.
    if (buf_tail < RECV_MS)
        buf_tail = 0;
    else
        buf_tail -= RECV_MS;
    memcpy(recv_buf, recv_buf + RECV_MS, buf_tail);
    // clear frame synced flag
    frame_sync_ok = 1;
}

/**
 * @brief find preable and check parity of a subframe
 * @param buf input subframe buffer
 * @param nbits number of bits need to shift
 * @return 0 if parity good, 'nbits' if no preamble or parity failed
 */
uint16_t CHANNEL::ParityCheck(uint8_t *buf, uint16_t *nbits)
{
    uint8_t p[6];

    // Upright or inverted preamble, setting of parity bits resolves phase ambiguity.
    if (0 == memcmp(buf, preambleUpright, 8))
        p[4] = p[5] = 0;
    else if (0 == memcmp(buf, preambleInverse, 8))
        p[4] = p[5] = 1;
    else
        return *nbits = 1; // return if no preamble found

    // Parity check up to ten 30-bit words.
    uint16_t i;
    for (i = 0; i < 300; i += 30)
    {
        if (0 != parity(p, buf + i, p[4], p[5]))
            return *nbits = i + 30; // return if word parity check failed
    }

    // Subframe found and parity check good, depack subframe.
    Ephemeris[sv].Subframe(buf);
    *nbits = 300;
    return 0;
}

/**
 * @brief find frame in bit stream
 * @return void
 */
void CHANNEL::FrameSync()
{
    while (nav_tail >= 300) // enough for a subframe
    {
        uint16_t nbits;
        uint16_t frame_sync_ok = ParityCheck(nav_buf, &nbits);
#ifdef LOG_DEBUG
        Debug("Frame sync nbits:{}.", nbits);
#endif
        nav_tail -= nbits;
        memcpy(nav_buf, nav_buf + nbits, nav_tail); // shift 'nav_buf'
    }
}

void CHANNEL::Service()
{
#ifdef LOG_INFO
    Info("Enter channel {}: PRN {}.", ch, sv);
#endif

    const int POLLING = 250; // Poll 4 times per second
    const int TIMEOUT = 80;  // Bail after 20 seconds on LOS
    for (int watchdog = 0; watchdog < TIMEOUT; watchdog++)
    {
        TimerWait(POLLING);
        DataFetch();
        if (data_fetch_ok == 1)
        {
            BitSync();
            if (bit_sync_ok == 1)
            {
#ifdef LOG_INFO
                Info("Bit synced for channel {}: PRN {}. Bit offset {}ms.", ch, sv, bit_head);
#endif
                BitSampling();
#ifdef LOG_DEBUG
                Debug("Updated nav_buf: {}.", array2str(nav_buf, nav_tail));
#endif
                FrameSync();
                if (frame_sync_ok == 0)
                {
                    watchdog = 0;
#ifdef LOG_INFO
                    Info("Frame synced for channel {}: PRN {}.", ch, sv);
#endif
#ifdef LOG_DEBUG
                    Ephemeris[sv].PrintAll();
#endif
                }
            }
        }
    }

#ifdef LOG_INFO
    Info("Leave channel {}: PRN {}.", ch, sv);
#endif
}

static CHANNEL Chans[NUM_CHANS];

// TODO: temporary handling with channel reset
void ChanReset()
{
    uint8_t i;
    for (i = 0; i < NUM_CHANS; i++)
    {
        Chans[i].Reset();
    }
}

void ChanTask()
{ // one thread per channel
    static int inst;
    int ch = inst++; // which channel am I?
    Chans[ch].ch = ch;
    for (;;)
    {
        if (BusyFlags & (1 << ch))
            Chans[ch].Service(); // returns after loss of signal
        // NextTask();
    }
}

void ChanStart(uint8_t ch, uint8_t sv)
{
    Chans[ch].sv = sv;
    BusyFlags |= (1 << ch);
}

#ifdef CHANNEL_TEST
void DataInject(uint8_t ch, uint8_t *input)
{
    memcpy(Chans[ch].recv_buf + Chans[ch].buf_tail, input, RECV_MS);

    Debug("{}", array2str(input, RECV_MS));

    Chans[ch].buf_tail += RECV_MS;
}

void TestBitSync(uint8_t ch)
{
    Chans[ch].BitSync();
    if (Chans[ch].bit_sync_ok == 1)
    {
        Info("Bit synced, offset is {}", Chans[ch].bit_head);
    }
}

void TestBitSampling(uint8_t ch)
{
    if (Chans[ch].bit_sync_ok == 1 && Chans[ch].nav_tail < 350)
    {
        Chans[ch].BitSampling();
        Chans[ch].FrameSync();

        Debug("{}", array2str(Chans[ch].nav_buf, Chans[ch].nav_tail));
    }
}
#endif
