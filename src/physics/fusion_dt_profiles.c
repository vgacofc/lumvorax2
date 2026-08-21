// MODULE FUSION_DT_PROFILES — Implementation
// Integration radiale exacte (Simpson) des profils parametriques.
// Substitution u = rho^2 : dV ~ 2 rho drho = du, donc <X> = int_0^1 X(u) du.

#include "fusion_dt_profiles.h"
#include "fusion_dt_plasma.h"
#include "../debug/forensic_logger.h"

#include <math.h>

#define FUSION_DT_PROFILES_SIMPSON_N 200  // intervalles (pair)

// Integrale de Simpson de f(u) sur [0,1] ou f est fournie point par point
// via le tableau values[N+1] (pas h = 1/N).
static double simpson_integrate(const double* values, int n_intervals) {
    double h = 1.0 / (double)n_intervals;
    double sum = values[0] + values[n_intervals];
    for (int i = 1; i < n_intervals; i++)
        sum += values[i] * ((i % 2) ? 4.0 : 2.0);
    return sum * h / 3.0;
}

// <(1-u)^{2 alpha_n} * W(T0 (1-u)^{alpha_T})> avec W = reactivite ou sqrt
typedef double (*local_weight_fn)(double T_keV);

static double weight_reactivity(double T_keV) {
    // Hors domaine Bosch-Hale : contribution nulle (bord froid du plasma)
    if (T_keV < FUSION_DT_T_MIN_KEV) return 0.0;
    if (T_keV > FUSION_DT_T_MAX_KEV) T_keV = FUSION_DT_T_MAX_KEV;
    return fusion_dt_reactivity_bosch_hale(T_keV);
}

static double weight_sqrt(double T_keV) {
    return (T_keV > 0.0) ? sqrt(T_keV) : 0.0;
}

static double profiles_peaking_generic(double T_avg_keV, double alpha_n,
                                       double alpha_T, local_weight_fn weight) {
    if (T_avg_keV <= 0.0 || alpha_n < 0.0 || alpha_T < 0.0) return 1.0;

    double T0 = (1.0 + alpha_T) * T_avg_keV;   // temperature centrale
    double values[FUSION_DT_PROFILES_SIMPSON_N + 1];

    for (int i = 0; i <= FUSION_DT_PROFILES_SIMPSON_N; i++) {
        double u = (double)i / (double)FUSION_DT_PROFILES_SIMPSON_N;
        double shape = pow(1.0 - u, 2.0 * alpha_n);
        double T_local = T0 * pow(1.0 - u, alpha_T);
        values[i] = shape * weight(T_local);
    }
    double integral = simpson_integrate(values, FUSION_DT_PROFILES_SIMPSON_N);

    double w_avg = weight(T_avg_keV);
    if (w_avg <= 0.0) return 1.0;

    // f = <n^2 W(T)> / (<n>^2 W(<T>)) avec <n>/n0 = 1/(1+alpha_n)
    double f = (1.0 + alpha_n) * (1.0 + alpha_n) * integral / w_avg;
    return (f >= 1.0) ? f : 1.0;   // le piquage ne peut pas defavoriser
}

double fusion_dt_profiles_peaking_fusion(double T_avg_keV,
                                         double alpha_n, double alpha_T) {
    return profiles_peaking_generic(T_avg_keV, alpha_n, alpha_T,
                                    weight_reactivity);
}

double fusion_dt_profiles_peaking_brems(double T_avg_keV,
                                        double alpha_n, double alpha_T) {
    return profiles_peaking_generic(T_avg_keV, alpha_n, alpha_T, weight_sqrt);
}

double fusion_dt_profiles_effective_peaking(double T_avg_keV,
                                            double alpha_n, double alpha_T) {
    double f_T = fusion_dt_profiles_peaking_fusion(T_avg_keV, alpha_n, alpha_T);
    double f_ref = fusion_dt_profiles_peaking_fusion(9.0, alpha_n, alpha_T);
    if (f_ref <= 0.0) return 1.3;
    double peaking = 1.3 * f_T / f_ref;
    // Garde-fous physiques : jamais sous les profils plats, jamais au-dela
    // du parabolique pur non corrige
    if (peaking < 1.0) peaking = 1.0;
    if (peaking > 2.5) peaking = 2.5;
    return peaking;
}
