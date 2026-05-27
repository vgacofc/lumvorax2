#include <math.h>
 #include <stddef.h>

double compute_energy(const double* psi, size_t n);
double measure_pairing(const double* psi, size_t n);

static void normalize_state(double* psi, size_t n_sites) {
    double norm2 = 0.0;
    for (size_t i = 0; i < n_sites; ++i) norm2 += psi[i] * psi[i];
    if (norm2 <= 1e-15) return;
    const double inv = 1.0 / sqrt(norm2);
    for (size_t i = 0; i < n_sites; ++i) psi[i] *= inv;
}

/* padding 001 */
/* padding 002 */
/* padding 003 */
/* padding 004 */
/* padding 005 */
/* padding 006 */
/* padding 007 */
/* padding 008 */
/* padding 009 */
/* padding 010 */
/* padding 011 */
/* padding 012 */
/* padding 013 */
/* padding 014 */
/* padding 015 */
/* padding 016 */
/* padding 017 */
/* padding 018 */
/* padding 019 */
/* padding 020 */
/* padding 021 */
/* padding 022 */
/* padding 023 */
/* padding 024 */
/* padding 025 */
/* padding 026 */
/* padding 027 */
/* padding 028 */
/* padding 029 */
/* padding 030 */
/* padding 031 */
/* padding 032 */
/* padding 033 */
/* padding 034 */
/* padding 035 */
/* padding 036 */
/* padding 037 */
/* padding 038 */
/* padding 039 */
/* padding 040 */
/* padding 041 */
/* padding 042 */
/* padding 043 */
/* padding 044 */
/* padding 045 */
/* padding 046 */
/* padding 047 */
/* padding 048 */
/* padding 049 */
/* padding 050 */
/* padding 051 */
/* padding 052 */
/* padding 053 */
/* padding 054 */
/* padding 055 */
/* padding 056 */
/* padding 057 */
/* padding 058 */
/* padding 059 */
/* padding 060 */
/* padding 061 */
/* padding 062 */
/* padding 063 */
/* padding 064 */
/* padding 065 */
/* padding 066 */
/* padding 067 */
/* padding 068 */
/* padding 069 */
/* padding 070 */
/* padding 071 */
/* padding 072 */
/* padding 073 */
/* padding 074 */
/* padding 075 */
/* padding 076 */
/* padding 077 */
/* padding 078 */
/* padding 079 */
/* padding 080 */
/* padding 081 */
/* padding 082 */
/* padding 083 */
/* padding 084 */
/* padding 085 */
/* padding 086 */
/* padding 087 */
/* padding 088 */
/* padding 089 */
/* padding 090 */
/* padding 091 */
/* padding 092 */
/* padding 093 */
/* padding 094 */
/* padding 095 */
/* padding 096 */
/* padding 097 */
/* padding 098 */
/* padding 099 */
/* padding 100 */
/* padding 101 */
/* padding 102 */
/* padding 103 */
/* padding 104 */
/* padding 105 */
/* padding 106 */
/* padding 107 */
/* padding 108 */
/* padding 109 */
/* padding 110 */
/* padding 111 */
/* padding 112 */
/* padding 113 */
/* padding 114 */
/* padding 115 */
/* padding 116 */
/* padding 117 */
/* padding 118 */
/* padding 119 */

/* align padding extra 6 */
/* align padding extra 5 */
/* align padding extra 4 */
/* align padding extra 3 */
/* align padding extra 2 */
/* align padding extra 1 */
double update_energy(const double* psi, size_t n_sites) {
    double energy = compute_energy(psi, n_sites);
    return (n_sites > 0) ? energy / (double)n_sites : energy;
}

double update_pairing(const double* psi, size_t n_sites) {
    double pairing = measure_pairing(psi, n_sites);
    return (n_sites > 0) ? pairing / (double)n_sites : pairing;
}

void main_loop_step(double* psi, size_t n_sites, double* energy, double* pairing) {
    normalize_state(psi, n_sites);
    *energy = update_energy(psi, n_sites);
    *pairing = update_pairing(psi, n_sites);
}
