// MODULE FUSION_DT_TRANSPORT — Implementation
// Solveur stationnaire 1-D cylindrique par quadrature directe :
//   q(r) = (1/r) int_0^r S(r') r' dr'      (flux de chaleur, W/m^2)
//   T(r) = T_edge + int_r^a q(r')/(n(r') chi) dr'
// avec calage de chi (bissection) pour atteindre <T> cible, et boucle de
// point fixe sources <-> profil (sous-relaxation 0.5).

#include "fusion_dt_transport.h"
#include "fusion_dt_plasma.h"
#include "../debug/forensic_logger.h"
#include "../debug/ultra_forensic_logger.h"

#include <math.h>
#include <string.h>

#define GRID_N FUSION_DT_TRANSPORT_GRID_N

// Moyenne volumique cylindrique : <X> = 2 int_0^1 X(rho) rho drho (trapezes)
static double volume_average(const double* x, const double* rho) {
    double sum = 0.0;
    for (int i = 0; i < GRID_N; i++) {
        double f0 = x[i] * rho[i], f1 = x[i + 1] * rho[i + 1];
        sum += 0.5 * (f0 + f1) * (rho[i + 1] - rho[i]);
    }
    return 2.0 * sum;
}

bool fusion_dt_transport_solve(double a_m, double n_avg_m3, double volume_m3,
                               double p_aux_W, double T_avg_target_keV,
                               fusion_dt_transport_result_t* result) {
    if (!result || a_m <= 0.0 || n_avg_m3 <= 0.0 || volume_m3 <= 0.0 ||
        T_avg_target_keV <= 0.5) {
        return false;
    }
    memset(result, 0, sizeof(*result));

    FORENSIC_LOG_MODULE_START("fusion_dt_transport", "solve_1p5d");
    FORENSIC_TIMING_START(solve_timer);

    double rho[GRID_N + 1], n_prof[GRID_N + 1], T[GRID_N + 1], S[GRID_N + 1];
    // Profil de densite AVEC PIEDESTAL (mode H reel) : la densite de bord est
    // finie (~35% de la moyenne), sinon le gradient de temperature diverge
    // artificiellement dans la derniere maille (n -> 0 => dT/dr -> inf) et le
    // solveur compense par un chi non physique (bug corrige en V6, cf.
    // RAPPORT 156). n(rho) = n_edge + (n0 - n_edge)(1-rho^2)^0.3, n0 choisi
    // pour conserver <n> : <(1-rho^2)^0.3> = 1/1.3.
    const double alpha_n = 0.3;
    double n_edge = 0.35 * n_avg_m3;
    double n0 = n_edge + (n_avg_m3 - n_edge) * (1.0 + alpha_n);

    for (int i = 0; i <= GRID_N; i++) {
        rho[i] = (double)i / (double)GRID_N;
        n_prof[i] = n_edge + (n0 - n_edge)
                    * pow(1.0 - rho[i] * rho[i] + 1e-12, alpha_n);
    }

    double T_edge = FUSION_DT_PEDESTAL_FRACTION * T_avg_target_keV;
    // Initialisation : profil parabolique standard normalise a la cible
    for (int i = 0; i <= GRID_N; i++) {
        double shape = pow(1.0 - rho[i] * rho[i] + 1e-12, 1.0);
        T[i] = T_edge + (2.2 * T_avg_target_keV - T_edge) * shape;
    }

    double p_aux_density = p_aux_W / volume_m3;   // chauffage uniforme (W/m^3)
    double chi = 1.0;                             // diffusivite initiale (m^2/s)
    int iter = 0;
    double T_avg = 0.0;

    for (iter = 1; iter <= FUSION_DT_TRANSPORT_MAX_ITER; iter++) {
        // --- Sources locales S(r) sur le profil courant ---
        for (int i = 0; i <= GRID_N; i++) {
            double T_loc = T[i];
            double sv = 0.0;
            if (T_loc >= FUSION_DT_T_MIN_KEV) {
                double T_eval = (T_loc > FUSION_DT_T_MAX_KEV)
                                    ? FUSION_DT_T_MAX_KEV : T_loc;
                sv = fusion_dt_reactivity_bosch_hale(T_eval);
            }
            double n_loc = n_prof[i];
            double p_alpha = 0.25 * n_loc * n_loc * sv * FUSION_DT_E_ALPHA_J;
            double p_brems = fusion_dt_bremsstrahlung(n_loc, T_loc, 1.0);
            S[i] = p_alpha + p_aux_density - p_brems;
        }

        // --- Flux de chaleur q(r) = (1/r) int_0^r S r' dr' (trapezes) ---
        double q[GRID_N + 1];
        q[0] = 0.0;
        double integral = 0.0;
        for (int i = 1; i <= GRID_N; i++) {
            double r0 = rho[i - 1] * a_m, r1 = rho[i] * a_m;
            integral += 0.5 * (S[i - 1] * r0 + S[i] * r1) * (r1 - r0);
            q[i] = integral / r1;
        }

        // --- Calage de chi par bissection : <T(chi)> = cible ---
        double chi_lo = 1e-3, chi_hi = 1e3;
        double T_try[GRID_N + 1];
        for (int b = 0; b < 60; b++) {
            double chi_mid = sqrt(chi_lo * chi_hi);   // bissection log
            // T(r) = T_edge + int_r^a q/(n chi) dr'
            T_try[GRID_N] = T_edge;
            for (int i = GRID_N - 1; i >= 0; i--) {
                double r0 = rho[i] * a_m, r1 = rho[i + 1] * a_m;
                double g0 = q[i] / (n_prof[i] * chi_mid);
                double g1 = q[i + 1] / (n_prof[i + 1] * chi_mid);
                double dT_J = 0.5 * (g0 + g1) * (r1 - r0);   // en joules
                T_try[i] = T_try[i + 1] + dT_J / FUSION_DT_KEV_TO_JOULE;
            }
            double avg = volume_average(T_try, rho);
            // chi plus grand => transport plus fort => T plus bas
            if (avg > T_avg_target_keV) chi_lo = chi_mid;
            else chi_hi = chi_mid;
            chi = chi_mid;
        }

        // --- Point fixe sous-relaxe sur le profil ---
        double max_rel = 0.0;
        for (int i = 0; i <= GRID_N; i++) {
            double T_new = 0.5 * T[i] + 0.5 * T_try[i];
            double rel = fabs(T_new - T[i]) / (T[i] + 1e-9);
            if (rel > max_rel) max_rel = rel;
            T[i] = T_new;
        }
        T_avg = volume_average(T, rho);

        FORENSIC_LOG_NANO("fusion_dt_transport", "iter_T_avg_keV", T_avg);
        FORENSIC_LOG_NANO("fusion_dt_transport", "iter_chi_m2s", chi);

        if (max_rel < 1e-6 && fabs(T_avg - T_avg_target_keV) / T_avg_target_keV < 0.02) {
            result->converged = true;
            break;
        }
    }

    // --- Bilan integral (verification de conservation) ---
    // Sources integrees (volume torique reel, proportionnel a la moyenne)
    double S_avg = volume_average(S, rho);
    double p_source_W = S_avg * volume_m3;
    // Flux sortant : q(a) * surface laterale equivalente (V = A_section * L
    // => surface d'echange = perimetre * L = 2/a * V pour un cylindre)
    double q_edge = 0.0;
    {
        double integral = 0.0;
        for (int i = 1; i <= GRID_N; i++) {
            double r0 = rho[i - 1] * a_m, r1 = rho[i] * a_m;
            integral += 0.5 * (S[i - 1] * r0 + S[i] * r1) * (r1 - r0);
        }
        q_edge = integral / a_m;
    }
    double p_outflux_W = q_edge * (2.0 / a_m) * volume_m3;

    result->T_avg_keV = T_avg;
    result->T0_keV = T[0];
    result->T_edge_keV = T_edge;
    result->peaking_T = (T_avg > 0.0) ? T[0] / T_avg : 0.0;
    result->chi_m2_s = chi;
    result->p_source_MW = p_source_W / 1e6;
    result->p_outflux_MW = p_outflux_W / 1e6;
    result->balance_error = (fabs(p_source_W) > 1.0)
        ? fabs(p_source_W - p_outflux_W) / fabs(p_source_W) : 1.0;
    result->iterations = iter;
    memcpy(result->T_profile_keV, T, sizeof(T));

    // --- Piquage fusion du profil PREDIT ---
    {
        double num[GRID_N + 1];
        for (int i = 0; i <= GRID_N; i++) {
            double T_eval = T[i];
            double sv = 0.0;
            if (T_eval >= FUSION_DT_T_MIN_KEV) {
                if (T_eval > FUSION_DT_T_MAX_KEV) T_eval = FUSION_DT_T_MAX_KEV;
                sv = fusion_dt_reactivity_bosch_hale(T_eval);
            }
            double nn = n_prof[i] / n_avg_m3;
            num[i] = nn * nn * sv;
        }
        double num_avg = volume_average(num, rho);
        double sv_avg = fusion_dt_reactivity_bosch_hale(
            T_avg > FUSION_DT_T_MAX_KEV ? FUSION_DT_T_MAX_KEV : T_avg);
        result->peaking_fusion = (sv_avg > 0.0) ? num_avg / sv_avg : 1.0;
    }

    FORENSIC_TIMING_END(solve_timer);
    uint64_t ns = FORENSIC_TIMING_CALC_NS(solve_timer);
    FORENSIC_LOG_MODULE_METRIC("fusion_dt_transport", "solve_ns", (double)ns);
    FORENSIC_LOG_MODULE_METRIC("fusion_dt_transport", "peaking_T", result->peaking_T);
    FORENSIC_LOG_MODULE_METRIC("fusion_dt_transport", "chi_m2_s", result->chi_m2_s);
    FORENSIC_LOG_MODULE_END("fusion_dt_transport", "solve_1p5d", result->converged);

    forensic_log(FORENSIC_LEVEL_INFO, __func__,
                 "1.5-D: <T>=%.2f keV T0=%.2f keV piquage_T=%.2f chi=%.2f m2/s "
                 "piquage_fus=%.2f bilan=%.2e (%d iter, %llu ns)",
                 result->T_avg_keV, result->T0_keV, result->peaking_T,
                 result->chi_m2_s, result->peaking_fusion,
                 result->balance_error, result->iterations,
                 (unsigned long long)ns);
    return true;
}
