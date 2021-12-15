
class EPHEM
{
private:
    // Subframe 1
    unsigned week, IODC, t_oc;
    double t_gd, a_f[3];

    // Subframe 2
    unsigned IODE2, t_oe;
    double C_rs, dn, M_0, C_uc, e, C_us, sqrtA, A;

    // Subframe 3
    unsigned IODE3;
    double C_ic, OMEGA_0, C_is, i_0, C_rc, omega, OMEGA_dot, IDOT;

    // Subframe 4, page 18 - Ionospheric delay
    double alpha[4], beta[4];
    void LoadPage18(uint8_t *nav);

    void Subframe1(uint8_t *nav);
    void Subframe2(uint8_t *nav);
    void Subframe3(uint8_t *nav);
    void Subframe4(uint8_t *nav);
    //  void Subframe5(uint8_t *nav);

    double EccentricAnomaly(double t_k);

public:
    unsigned tow;

    void Subframe(uint8_t *buf);
    bool Valid();
    double GetClockCorrection(double t);
    void GetXYZ(double *x, double *y, double *z, double t);
    void PrintAll();
};

extern EPHEM Ephemeris[];
