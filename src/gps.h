#ifndef _GPS_H
#define _GPS_H 1

#include <inttypes.h>
#include "logger.h"

// #define CHANNEL_TEST
#define LOG_DEBUG
#define LOG_INFO
#define LOG_WARN
#define LOG_ERROR
#define LOG_CRITICAL

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

///////////////////////////////////////////////////////////////////////////////
// Parameters

#define NUM_SATS 32
#define NUM_CHANS 12

///////////////////////////////////////////////////////////////////////////////
// Frequencies

#define L1 1575.42e6 // L1 carrier
#define FC 2.6e6     // Carrier @ 2nd IF
#define FS 10e6      // Sampling rate
#define CPS 1.023e6  // Chip rate
#define BPS 50.0     // NAV data rate

///////////////////////////////////////////////////////////////////////////////
// Official GPS constants

const double PI = 3.1415926535898;

const double MU = 3.986005e14;          // WGS 84: earth's gravitational constant for GPS user
const double OMEGA_E = 7.2921151467e-5; // WGS 84: earth's rotation rate

const double C = 2.99792458e8; // Speed of light

const double F = -4.442807633e-10; // -2*sqrt(MU)/pow(C,2)

//////////////////////////////////////////////////////////////
// Coroutines
unsigned EventCatch(unsigned);
void EventRaise(unsigned);
void NextTask();
void CreateTask(void (*entry)());
unsigned Microseconds(void);
void TimerWait(unsigned ms);

//////////////////////////////////////////////////////////////
// Search

// int  SearchInit();
// void SearchFree();
// void SearchTask();
// void SearchEnable(int sv);
// int  SearchCode(int sv, unsigned int g1);

//////////////////////////////////////////////////////////////
// Tracking

void ChanReset();
void ChanTask();
void ChanStart(uint8_t ch, uint8_t sv);
#ifdef CHANNEL_TEST
void DataInject(uint8_t ch, uint8_t *input);
void TestBitSync(uint8_t ch);
void TestBitSampling(uint8_t ch);
#endif

//////////////////////////////////////////////////////////////
// Solution

// void SolveTask();

#endif // _GPS_H