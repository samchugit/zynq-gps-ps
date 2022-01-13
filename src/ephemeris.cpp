#include <math.h>
#include <stdio.h>

#include "gps.h"
#include "ephemeris.h"

EPHEM Ephemeris[NUM_SATS];

static double TimeFromEpoch(double t, double t_ref)
{
    t -= t_ref;
    if (t > 302400)
        t -= 604800;
    else if (t < -302400)
        t += 604800;
    return t;
}

union PACK
{
    PACK(int a, int b = 0, int c = 0, int d = 0) { byte[3] = a, byte[2] = b, byte[1] = c, byte[0] = d; }
    uint8_t byte[4];
    unsigned u32;
    signed s32;
    unsigned u(int n) { return u32 >> (32 - n); }
    signed s(int n) { return s32 >> (32 - n); }
};

void EPHEM::Subframe1(uint8_t *nav)
{
    week = PACK(nav[6], nav[7]).u(10);
    t_gd = pow(2, -31) * PACK(nav[20]).s(8);
    IODC = PACK(nav[21]).u(8);
    t_oc = (1 << 4) * PACK(nav[22], nav[23]).u(16);
    a_f[2] = pow(2, -55) * PACK(nav[24]).s(8);
    a_f[1] = pow(2, -43) * PACK(nav[25], nav[26]).s(16);
    a_f[0] = pow(2, -31) * PACK(nav[27], nav[28], nav[29]).s(22);
}

void EPHEM::Subframe2(uint8_t *nav)
{
    IODE2 = PACK(nav[6]).u(8);
    C_rs = pow(2, -5) * PACK(nav[7], nav[8]).s(16);
    dn = pow(2, -43) * PACK(nav[9], nav[10]).s(16) * PI;
    M_0 = pow(2, -31) * PACK(nav[11], nav[12], nav[13], nav[14]).s(32) * PI;
    C_uc = pow(2, -29) * PACK(nav[15], nav[16]).s(16);
    e = pow(2, -33) * PACK(nav[17], nav[18], nav[19], nav[20]).u(32);
    C_us = pow(2, -29) * PACK(nav[21], nav[22]).s(16);
    sqrtA = pow(2, -19) * PACK(nav[23], nav[24], nav[25], nav[26]).u(32);
    t_oe = (1 << 4) * PACK(nav[27], nav[28]).u(16);
}

void EPHEM::Subframe3(uint8_t *nav)
{
    C_ic = pow(2, -29) * PACK(nav[6], nav[7]).s(16);
    OMEGA_0 = pow(2, -31) * PACK(nav[8], nav[9], nav[10], nav[11]).s(32) * PI;
    C_is = pow(2, -29) * PACK(nav[12], nav[13]).s(16);
    i_0 = pow(2, -31) * PACK(nav[14], nav[15], nav[16], nav[17]).s(32) * PI;
    C_rc = pow(2, -5) * PACK(nav[18], nav[19]).s(16);
    omega = pow(2, -31) * PACK(nav[20], nav[21], nav[22], nav[23]).s(32) * PI;
    OMEGA_dot = pow(2, -43) * PACK(nav[24], nav[25], nav[26]).s(24) * PI;
    IODE3 = PACK(nav[27]).u(8);
    IDOT = pow(2, -43) * PACK(nav[28], nav[29]).s(14) * PI;
}

// Ionospheric delay
void EPHEM::LoadPage18(uint8_t *nav)
{
    alpha[0] = pow(2, -30) * PACK(nav[7]).s(8);
    alpha[1] = pow(2, -27) * PACK(nav[8]).s(8);
    alpha[2] = pow(2, -24) * PACK(nav[9]).s(8);
    alpha[3] = pow(2, -24) * PACK(nav[10]).s(8);
    beta[0] = pow(2, 11) * PACK(nav[11]).s(8);
    beta[1] = pow(2, 14) * PACK(nav[12]).s(8);
    beta[2] = pow(2, 16) * PACK(nav[13]).s(8);
    beta[3] = pow(2, 16) * PACK(nav[14]).s(8);
}

void EPHEM::Subframe4(uint8_t *nav)
{
    if (PACK(nav[6]).u(8) == 0x78)
        LoadPage18(nav);
}

double EPHEM::EccentricAnomaly(double t_k)
{
    // Semi-major axis
    A = sqrtA * sqrtA;

    // Computed mean motion (rad/sec)
    double n_0 = sqrt(MU / (A * A * A));

    // Corrected mean motion
    double n = n_0 + dn;

    // Mean anomaly
    double M_k = M_0 + n * t_k;

    // Solve Kepler's Equation for Eccentric Anomaly
    double E_k = M_k;
    for (;;)
    {
        double temp = E_k;
        E_k = M_k + e * sin(E_k);
        if (fabs(E_k - temp) < 1e-10)
            break;
    }

    return E_k;
}

// Get satellite position at time t
void EPHEM::GetXYZ(double *x, double *y, double *z, double t)
{
    // Time from ephemeris reference epoch
    double t_k = TimeFromEpoch(t, t_oe);

    // Eccentric Anomaly
    double E_k = EccentricAnomaly(t_k);

    // True Anomaly
    double v_k = atan2(
        sqrt(1 - e * e) * sin(E_k),
        cos(E_k) - e);

    // Argument of Latitude
    double AOL = v_k + omega;

    // Second Harmonic Perturbations
    double du_k = C_us * sin(2 * AOL) + C_uc * cos(2 * AOL); // Argument of Latitude Correction
    double dr_k = C_rs * sin(2 * AOL) + C_rc * cos(2 * AOL); // Radius Correction
    double di_k = C_is * sin(2 * AOL) + C_ic * cos(2 * AOL); // Inclination Correction

    // Corrected Argument of Latitude; Radius & Inclination
    double u_k = AOL + du_k;
    double r_k = A * (1 - e * cos(E_k)) + dr_k;
    double i_k = i_0 + di_k + IDOT * t_k;

    // Positions in orbital plane
    double x_kp = r_k * cos(u_k);
    double y_kp = r_k * sin(u_k);

    // Corrected longitude of ascending node
    double OMEGA_k = OMEGA_0 + (OMEGA_dot - OMEGA_E) * t_k - OMEGA_E * t_oe;

    // Earth-fixed coordinates
    *x = x_kp * cos(OMEGA_k) - y_kp * cos(i_k) * sin(OMEGA_k);
    *y = x_kp * sin(OMEGA_k) + y_kp * cos(i_k) * cos(OMEGA_k);
    *z = y_kp * sin(i_k);
}

double EPHEM::GetClockCorrection(double t)
{
    // Time from ephemeris reference epoch
    double t_k = TimeFromEpoch(t, t_oe);

    // Eccentric Anomaly
    double E_k = EccentricAnomaly(t_k);

    // Relativistic correction
    double t_R = F * e * sqrtA * sin(E_k);

    // Time from clock correction epoch
    t = TimeFromEpoch(t, t_oc);

    // 20.3.3.3.3.1 User Algorithm for SV Clock Correction
    return a_f[0] + a_f[1] * pow(t, 1) + a_f[2] * pow(t, 2) + t_R - t_gd;
}

bool EPHEM::Valid()
{
    return IODC != 0 && IODC == IODE2 && IODC == IODE3;
}

void EPHEM::PrintAll()
{
    Debug("Ephemeris *************************", 0);
    Debug("Subframe 1 ------------------------", 0);
    Debug("  week:     {}", week);
    Debug("  IODC:     {}", IODC);
    Debug("  t_oc:     {}", t_oc);
    Debug("  t_gd:     {}", t_gd);
    Debug("  a_f0:     {}", a_f[0]);
    Debug("  a_f1:     {}", a_f[1]);
    Debug("  a_f2:     {}", a_f[2]);
    Debug("Subframe 2 ------------------------", 0);
    Debug("  IODE2:    {}", IODE2);
    Debug("  t_oe:     {}", t_oe);
    Debug("  C_rs:     {}", C_rs);
    Debug("  dn:       {}", dn);
    Debug("  M_0:      {}", M_0);
    Debug("  C_uc:     {}", C_uc);
    Debug("  e:        {}", e);
    Debug("  C_us:     {}", C_us);
    Debug("  sqrtA:    {}", sqrtA);
    Debug("  A:        {}", A);
    Debug("Subframe 3 ------------------------", 0);
    Debug("  IODE3:    {}", IODE3);
    Debug("  C_ic:     {}", C_ic);
    Debug("  OMEGA_0:  {}", OMEGA_0);
    Debug("  C_is:     {}", C_is);
    Debug("  i_0:      {}", i_0);
    Debug("  C_rc:     {}", C_rc);
    Debug("  omega:    {}", omega);
    Debug("  OMEGA_dot:{}", OMEGA_dot);
    Debug("  IDOT:     {}", IDOT);
    Debug("Ephemeris END**********************", 0);
}

// called from channel tasks
void EPHEM::Subframe(uint8_t *buf)
{
    uint8_t nav[30];
    uint8_t id = (buf[49] << 2) + (buf[50] << 1) + buf[51];

    for (int i = 0; i < 30; buf += 6)
    {
        for (int j = 0; j < 3; j++)
        {
            int byte = 0;
            for (int k = 0; k < 8; k++)
                byte += byte + *buf++;
            nav[i++] = byte;
        }
    }

    tow = PACK(nav[3], nav[4], nav[5]).u(17);

    switch (id)
    {
    case 1:
        Subframe1(nav);
        break;
    case 2:
        Subframe2(nav);
        break;
    case 3:
        Subframe3(nav);
        break;
        // case 4:
        //     Subframe4(nav);
        //     break;
        // case 5:
        //     Subframe5(nav);
        //     break;
    }
}
