// MODULE FUSION_DT_PLASMA — Implementation
// Voir fusion_dt_plasma.h pour la portee scientifique et les references.
// Toute valeur numerique produite ici provient d'un calcul reel execute,
// horodate CLOCK_MONOTONIC nanoseconde et journalise (exigence projet).

#include "fusion_dt_plasma.h"
#include "../debug/forensic_logger.h"
#include "../debug/ultra_forensic_logger.h"
#include "../debug/memory_tracker.h"
#include "../vorax/vorax_operations.h"
#include "../lum/lum_memory_tracer.h"

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define FUSION_DT_MODULE_NAME "fusion_dt_plasma"

// ---------------------------------------------------------------------------
// Hash FNV-1a 64 bits : tracabilite BIT-LEVEL de l'etat plasma.
// Chaque octet (donc chaque bit) de la representation memoire participe au
// hash ; la modification d'un seul bit de l'etat change la valeur journalisee.
// ---------------------------------------------------------------------------
uint64_t fusion_dt_state_hash(const fusion_dt_state_t* state) {
    if (!state) return 0;
    const uint8_t* bytes = (const uint8_t*)state;
    uint64_t hash = 0xcbf29ce484222325ULL;      // FNV offset basis
    for (size_t i = 0; i < sizeof(fusion_dt_state_t); i++) {
        hash ^= (uint64_t)bytes[i];
        hash *= 0x100000001b3ULL;               // FNV prime
    }
    return hash;
}

// ---------------------------------------------------------------------------
// Reactivite D-T <sigma-v> — Bosch & Hale, Nuclear Fusion 32 (1992) 611.
// Parametrisation R-matrix, precision annoncee < 0.25% sur 0.2-100 keV.
// Retour en m^3/s (la publication donne des cm^3/s : conversion x 1e-6).
// ---------------------------------------------------------------------------
double fusion_dt_reactivity_bosch_hale(double T_keV) {
    if (T_keV < FUSION_DT_T_MIN_KEV || T_keV > FUSION_DT_T_MAX_KEV) {
        forensic_log(FORENSIC_LEVEL_WARNING, __func__,
                     "T=%.4f keV hors domaine Bosch-Hale [%.1f, %.1f] keV",
                     T_keV, FUSION_DT_T_MIN_KEV, FUSION_DT_T_MAX_KEV);
        return 0.0;
    }

    // Coefficients Bosch-Hale pour T(d,n)4He
    static const double BG      = 34.3827;      // sqrt(keV) — parametre de Gamow
    static const double MRC2    = 1124656.0;    // masse reduite * c^2 (keV)
    static const double C1      = 1.17302e-9;
    static const double C2      = 1.51361e-2;
    static const double C3      = 7.51886e-2;
    static const double C4      = 4.60643e-3;
    static const double C5      = 1.35000e-2;
    static const double C6      = -1.06750e-4;
    static const double C7      = 1.36600e-5;

    double num   = T_keV * (C2 + T_keV * (C4 + T_keV * C6));
    double den   = 1.0 + T_keV * (C3 + T_keV * (C5 + T_keV * C7));
    double theta = T_keV / (1.0 - num / den);
    double xi    = cbrt((BG * BG) / (4.0 * theta));
    double sigma_v_cm3 = C1 * theta * sqrt(xi / (MRC2 * T_keV * T_keV * T_keV))
                         * exp(-3.0 * xi);
    return sigma_v_cm3 * 1e-6;  // cm^3/s -> m^3/s
}

// Puissance fusion volumique, plasma 50/50 : n_D = n_T = n_e/2
double fusion_dt_power_density(double n_e_m3, double T_keV) {
    double sv = fusion_dt_reactivity_bosch_hale(T_keV);
    double n_half = 0.5 * n_e_m3;
    return n_half * n_half * sv * FUSION_DT_E_TOTAL_J;
}

// Pertes bremsstrahlung volumiques — NRL Plasma Formulary
double fusion_dt_bremsstrahlung(double n_e_m3, double T_keV, double z_eff) {
    if (T_keV <= 0.0) return 0.0;
    return FUSION_DT_BREMS_COEFF * z_eff * n_e_m3 * n_e_m3 * sqrt(T_keV);
}

// n*tau_E minimal pour ignition : chauffage alpha >= brems + transport
// (n^2/4)<sv>E_alpha >= C_B Zeff n^2 sqrt(T) + 3 n T_J / tau_E
double fusion_dt_lawson_n_tau(double T_keV, double z_eff) {
    double sv = fusion_dt_reactivity_bosch_hale(T_keV);
    if (sv <= 0.0) return INFINITY;
    double alpha_term = 0.25 * sv * FUSION_DT_E_ALPHA_J;
    double brems_term = FUSION_DT_BREMS_COEFF * z_eff * sqrt(T_keV);
    double net = alpha_term - brems_term;
    if (net <= 0.0) return INFINITY;  // brems > chauffage alpha : ignition impossible
    double T_J = T_keV * FUSION_DT_KEV_TO_JOULE;
    return 3.0 * T_J / net;
}

// ---------------------------------------------------------------------------
// Balayage de Lawson : recherche des temperatures optimales d'ignition.
// Chaque point est journalise dans le ring buffer nanoseconde (NANO).
// ---------------------------------------------------------------------------
fusion_dt_lawson_result_t* fusion_dt_lawson_scan(double T_start_keV, double T_end_keV,
                                                 double T_step_keV, double z_eff) {
    if (T_step_keV <= 0.0 || T_end_keV <= T_start_keV) {
        forensic_log(FORENSIC_LEVEL_ERROR, __func__, "Parametres de balayage invalides");
        return NULL;
    }

    FORENSIC_LOG_MODULE_START(FUSION_DT_MODULE_NAME, "lawson_scan");
    FORENSIC_TIMING_START(scan_timer);

    size_t count = (size_t)((T_end_keV - T_start_keV) / T_step_keV) + 1;
    fusion_dt_lawson_result_t* result =
        TRACKED_MALLOC(sizeof(fusion_dt_lawson_result_t));
    if (!result) return NULL;
    memset(result, 0, sizeof(*result));

    result->points = TRACKED_MALLOC(count * sizeof(fusion_dt_lawson_point_t));
    if (!result->points) {
        TRACKED_FREE(result);
        return NULL;
    }

    double best_triple = INFINITY, best_ntau = INFINITY;
    double best_T_triple = 0.0, best_T_ntau = 0.0;

    for (size_t i = 0; i < count; i++) {
        double T = T_start_keV + (double)i * T_step_keV;
        fusion_dt_lawson_point_t* pt = &result->points[i];
        pt->T_keV = T;
        pt->reactivity_m3_s = fusion_dt_reactivity_bosch_hale(T);
        pt->n_tau_required_s_m3 = fusion_dt_lawson_n_tau(T, z_eff);
        pt->triple_product_keV_s_m3 = pt->n_tau_required_s_m3 * T;

        // Capture nanoseconde par nanoseconde (ring buffer, sans I/O bloquante)
        FORENSIC_LOG_NANO(FUSION_DT_MODULE_NAME, "lawson_T_keV", T);
        FORENSIC_LOG_NANO(FUSION_DT_MODULE_NAME, "lawson_ntau", pt->n_tau_required_s_m3);

        if (pt->triple_product_keV_s_m3 < best_triple) {
            best_triple = pt->triple_product_keV_s_m3;
            best_T_triple = T;
        }
        if (pt->n_tau_required_s_m3 < best_ntau) {
            best_ntau = pt->n_tau_required_s_m3;
            best_T_ntau = T;
        }
    }

    result->point_count = count;
    result->T_optimal_keV = best_T_triple;
    result->triple_product_min = best_triple;
    result->n_tau_min_s_m3 = best_ntau;
    result->T_ntau_optimal_keV = best_T_ntau;
    result->success = isfinite(best_triple);

    FORENSIC_TIMING_END(scan_timer);
    uint64_t scan_ns = FORENSIC_TIMING_CALC_NS(scan_timer);

    snprintf(result->message, sizeof(result->message),
             "Balayage %zu points en %llu ns : triple produit min %.4e keV.s/m^3 a T=%.2f keV",
             count, (unsigned long long)scan_ns, best_triple, best_T_triple);

    FORENSIC_LOG_MODULE_METRIC(FUSION_DT_MODULE_NAME, "lawson_T_optimal_keV", best_T_triple);
    FORENSIC_LOG_MODULE_METRIC(FUSION_DT_MODULE_NAME, "lawson_triple_min", best_triple);
    FORENSIC_LOG_MODULE_METRIC(FUSION_DT_MODULE_NAME, "lawson_scan_duration_ns", (double)scan_ns);
    FORENSIC_LOG_MODULE_END(FUSION_DT_MODULE_NAME, "lawson_scan", result->success);

    forensic_log(FORENSIC_LEVEL_INFO, __func__, "%s", result->message);
    return result;
}

void fusion_dt_lawson_result_destroy(fusion_dt_lawson_result_t** result_ptr) {
    if (!result_ptr || !*result_ptr) return;
    if ((*result_ptr)->points) TRACKED_FREE((*result_ptr)->points);
    TRACKED_FREE(*result_ptr);
    *result_ptr = NULL;
}

// ---------------------------------------------------------------------------
// Cycle de vie du moteur plasma
// ---------------------------------------------------------------------------
fusion_dt_plasma_t* fusion_dt_plasma_create(const fusion_dt_config_t* config) {
    if (!config || config->n_e_m3 <= 0.0 || config->tau_E_s <= 0.0 ||
        config->volume_m3 <= 0.0 || config->dt_s <= 0.0) {
        forensic_log(FORENSIC_LEVEL_ERROR, __func__, "Configuration plasma invalide");
        return NULL;
    }

    fusion_dt_plasma_t* plasma = TRACKED_MALLOC(sizeof(fusion_dt_plasma_t));
    if (!plasma) return NULL;
    memset(plasma, 0, sizeof(*plasma));

    plasma->magic_number = FUSION_DT_PLASMA_MAGIC;
    plasma->config = *config;
    if (plasma->config.log_every == 0) plasma->config.log_every = 1;
    plasma->memory_address = (void*)plasma;
    plasma->creation_timestamp_ns = lum_get_timestamp();

    if (plasma->config.enable_ash_dynamics && plasma->config.tau_he_ratio <= 0.0) {
        plasma->config.tau_he_ratio = 5.0;  // valeur litterature par defaut
    }

    // Etat initial : W = 3 n T (electrons + ions, Te = Ti, combustible pur)
    plasma->state.time_s = 0.0;
    plasma->state.T_keV = config->T_keV;
    plasma->state.W_J_m3 = 3.0 * config->n_e_m3 * config->T_keV * FUSION_DT_KEV_TO_JOULE;
    plasma->state.tau_E_eff_s = config->tau_E_s;
    plasma->state.n_fuel_m3 = config->n_e_m3;
    plasma->state.n_helium_m3 = 0.0;
    plasma->state.z_eff_dynamic = config->z_eff;
    plasma->state.step_index = 0;
    plasma->state_hash = fusion_dt_state_hash(&plasma->state);

    forensic_log(FORENSIC_LEVEL_INFO, __func__,
                 "Plasma cree: n=%.3e m^-3 T=%.2f keV tau_E=%.2f s V=%.1f m^3 "
                 "P_aux=%.2f MW hash_initial=0x%016llX",
                 config->n_e_m3, config->T_keV, config->tau_E_s, config->volume_m3,
                 config->p_aux_W / 1e6, (unsigned long long)plasma->state_hash);
    return plasma;
}

void fusion_dt_plasma_destroy(fusion_dt_plasma_t** plasma_ptr) {
    if (!plasma_ptr || !*plasma_ptr) return;
    fusion_dt_plasma_t* plasma = *plasma_ptr;
    if (plasma->magic_number != FUSION_DT_PLASMA_MAGIC) {
        forensic_log(FORENSIC_LEVEL_ERROR, __func__,
                     "Double-free ou corruption detectee (magic=0x%08X)",
                     plasma->magic_number);
        return;
    }
    if (plasma->lum_group_deuterium) lum_group_destroy(plasma->lum_group_deuterium);
    if (plasma->lum_group_tritium)   lum_group_destroy(plasma->lum_group_tritium);
    if (plasma->lum_group_alpha)     lum_group_destroy(plasma->lum_group_alpha);
    plasma->magic_number = FUSION_DT_PLASMA_DESTROYED;
    TRACKED_FREE(plasma);
    *plasma_ptr = NULL;
}

// ---------------------------------------------------------------------------
// Degradation du confinement avec la puissance de chauffage — IPB98(y,2) :
// tau_E proportionnel a P^-0.69 (ITER Physics Basis, Nucl. Fusion 39 (1999)).
// C'est cette degradation qui stabilise les points de fonctionnement pilotes
// (analyses POPCON) : sans elle, un plasma 0-D est thermiquement instable
// entre la branche froide et l'excursion vers T > 100 keV.
// ---------------------------------------------------------------------------
static double fusion_dt_tau_eff(const fusion_dt_plasma_t* plasma, double p_heat_W) {
    double tau = plasma->config.tau_E_s;
    double exponent = plasma->config.tau_scaling_exponent;
    if (exponent > 0.0 && plasma->config.p_ref_W > 0.0) {
        double ratio = p_heat_W / plasma->config.p_ref_W;
        if (ratio < 1e-3) ratio = 1e-3;  // garde-fou numerique (P_heat -> 0)
        tau *= pow(ratio, -exponent);
    }
    return tau;
}

// ---------------------------------------------------------------------------
// Systeme dynamique couple (energie + cendres d'helium) :
//   dW/dt    = p_alpha + p_aux - p_brems - W/tau_eff
//   dn_He/dt = R - n_He/tau_He           (si cendres actives, sinon 0)
// avec R = (n_fuel/2)^2 <sigma-v>, n_fuel = n_e - 2 n_He (quasi-neutralite,
// n_e maintenue constante par injection de combustible), tau_He = ratio*tau_eff,
// Z_eff = (n_fuel + 4 n_He)/n_e, W = (3/2)(n_e + n_ions) T.
// Mode simple (cendres off) : n_fuel = n_ions = n_e => W = 3 n T (inchange).
// ---------------------------------------------------------------------------
typedef struct {
    double dW_dt;
    double dnHe_dt;
    // Grandeurs intermediaires reutilisees pour la mise a jour d'etat
    double T_keV;
    double n_fuel;
    double z_eff;
    double reaction_rate_m3_s;   // R (reactions/m^3/s)
    double p_alpha;
    double p_brems;
    double tau_eff;
} fusion_dt_derivs_t;

static fusion_dt_derivs_t fusion_dt_derivatives(const fusion_dt_plasma_t* plasma,
                                                double W, double n_He) {
    const fusion_dt_config_t* cfg = &plasma->config;
    fusion_dt_derivs_t d;
    memset(&d, 0, sizeof(d));

    double n_e = cfg->n_e_m3;
    double n_fuel = n_e;
    double n_ions = n_e;
    double z_eff = cfg->z_eff;
    if (cfg->enable_ash_dynamics) {
        n_fuel = n_e - 2.0 * n_He;
        if (n_fuel < 0.0) n_fuel = 0.0;
        n_ions = n_fuel + n_He;
        z_eff = (n_fuel + 4.0 * n_He) / n_e;  // somme n_i Z_i^2 / n_e
    }

    double T_keV = W / (1.5 * (n_e + n_ions) * FUSION_DT_KEV_TO_JOULE);
    // Clamp au domaine Bosch-Hale : hors domaine, reactivite nulle (physique :
    // negligeable sous 0.2 keV ; au-dela de 100 keV la simulation est arretee
    // en amont par la detection d'excursion thermique).
    double T_clamped = T_keV;
    if (T_clamped < FUSION_DT_T_MIN_KEV) T_clamped = FUSION_DT_T_MIN_KEV;
    if (T_clamped > FUSION_DT_T_MAX_KEV) T_clamped = FUSION_DT_T_MAX_KEV;

    double sv = fusion_dt_reactivity_bosch_hale(T_clamped);
    double R = 0.25 * n_fuel * n_fuel * sv;
    double p_alpha = R * FUSION_DT_E_ALPHA_J;
    double p_brems = fusion_dt_bremsstrahlung(n_e, T_clamped, z_eff);
    double p_aux_density = cfg->p_aux_W / cfg->volume_m3;
    double p_heat_W = (p_alpha + p_aux_density) * cfg->volume_m3;
    double tau_eff = fusion_dt_tau_eff(plasma, p_heat_W);

    d.dW_dt = p_alpha + p_aux_density - p_brems - W / tau_eff;
    d.dnHe_dt = cfg->enable_ash_dynamics
                    ? (R - n_He / (cfg->tau_he_ratio * tau_eff))
                    : 0.0;
    d.T_keV = T_keV;
    d.n_fuel = n_fuel;
    d.z_eff = z_eff;
    d.reaction_rate_m3_s = R;
    d.p_alpha = p_alpha;
    d.p_brems = p_brems;
    d.tau_eff = tau_eff;
    return d;
}

bool fusion_dt_plasma_step(fusion_dt_plasma_t* plasma) {
    if (!plasma || plasma->magic_number != FUSION_DT_PLASMA_MAGIC) {
        forensic_log(FORENSIC_LEVEL_ERROR, __func__, "Moteur plasma invalide");
        return false;
    }

    FORENSIC_TIMING_START(step_timer);

    double W = plasma->state.W_J_m3;
    double n_He = plasma->state.n_helium_m3;
    double W_prev = W;
    double dt = plasma->config.dt_s;

    // Integration RK4 classique sur le systeme couple (W, n_He)
    fusion_dt_derivs_t k1 = fusion_dt_derivatives(plasma, W, n_He);
    fusion_dt_derivs_t k2 = fusion_dt_derivatives(plasma, W + 0.5 * dt * k1.dW_dt,
                                                  n_He + 0.5 * dt * k1.dnHe_dt);
    fusion_dt_derivs_t k3 = fusion_dt_derivatives(plasma, W + 0.5 * dt * k2.dW_dt,
                                                  n_He + 0.5 * dt * k2.dnHe_dt);
    fusion_dt_derivs_t k4 = fusion_dt_derivatives(plasma, W + dt * k3.dW_dt,
                                                  n_He + dt * k3.dnHe_dt);
    W += (dt / 6.0) * (k1.dW_dt + 2.0 * k2.dW_dt + 2.0 * k3.dW_dt + k4.dW_dt);
    n_He += (dt / 6.0) * (k1.dnHe_dt + 2.0 * k2.dnHe_dt + 2.0 * k3.dnHe_dt + k4.dnHe_dt);
    if (W < 0.0) W = 0.0;
    if (n_He < 0.0) n_He = 0.0;

    // Mise a jour de l'etat complet (grandeurs coherentes avec (W, n_He))
    fusion_dt_derivs_t now = fusion_dt_derivatives(plasma, W, n_He);
    fusion_dt_state_t* s = &plasma->state;
    s->W_J_m3 = W;
    s->n_helium_m3 = n_He;
    s->n_fuel_m3 = now.n_fuel;
    s->z_eff_dynamic = now.z_eff;
    s->T_keV = now.T_keV;
    s->reactivity_m3_s = fusion_dt_reactivity_bosch_hale(
        now.T_keV < FUSION_DT_T_MIN_KEV ? FUSION_DT_T_MIN_KEV :
        (now.T_keV > FUSION_DT_T_MAX_KEV ? FUSION_DT_T_MAX_KEV : now.T_keV));
    s->p_fusion_W_m3 = now.reaction_rate_m3_s * FUSION_DT_E_TOTAL_J;
    s->p_alpha_W_m3 = now.p_alpha;
    s->p_brems_W_m3 = now.p_brems;
    s->tau_E_eff_s = now.tau_eff;
    s->p_transport_W_m3 = W / now.tau_eff;
    double p_fus_total_W = s->p_fusion_W_m3 * plasma->config.volume_m3;
    s->q_factor = (plasma->config.p_aux_W > 0.0)
                      ? p_fus_total_W / plasma->config.p_aux_W
                      : INFINITY;
    s->time_s += dt;
    s->step_index++;

    // Tracabilite bit-level : hash de l'etat complet apres le pas
    plasma->state_hash = fusion_dt_state_hash(s);

    FORENSIC_TIMING_END(step_timer);
    uint64_t step_ns = FORENSIC_TIMING_CALC_NS(step_timer);

    if (s->step_index % plasma->config.log_every == 0) {
        FORENSIC_LOG_NANO(FUSION_DT_MODULE_NAME, "T_keV", s->T_keV);
        FORENSIC_LOG_NANO(FUSION_DT_MODULE_NAME, "W_J_m3", s->W_J_m3);
        FORENSIC_LOG_NANO(FUSION_DT_MODULE_NAME, "p_fusion_W_m3", s->p_fusion_W_m3);
        FORENSIC_LOG_NANO(FUSION_DT_MODULE_NAME, "q_factor", s->q_factor);
        FORENSIC_LOG_NANO(FUSION_DT_MODULE_NAME, "tau_E_eff_s", s->tau_E_eff_s);
        if (plasma->config.enable_ash_dynamics) {
            FORENSIC_LOG_NANO(FUSION_DT_MODULE_NAME, "n_helium_m3", s->n_helium_m3);
            FORENSIC_LOG_NANO(FUSION_DT_MODULE_NAME, "z_eff_dynamic", s->z_eff_dynamic);
        }
        FORENSIC_LOG_NANO(FUSION_DT_MODULE_NAME, "step_compute_ns", (double)step_ns);
        // Hash 64 bits journalise en hexadecimal (sans perte, hors ring double)
        forensic_log(FORENSIC_LEVEL_DEBUG, __func__,
                     "step=%llu t=%.6f s T=%.4f keV W=%.6e J/m^3 hash=0x%016llX (%llu ns)",
                     (unsigned long long)s->step_index, s->time_s, s->T_keV,
                     s->W_J_m3, (unsigned long long)plasma->state_hash,
                     (unsigned long long)step_ns);
    }

    // Detection d'anomalie energetique (comportement non programme)
    ultra_forensic_check_anomaly_energy(FUSION_DT_MODULE_NAME, W_prev, W,
                                        1.0 / FUSION_DT_KEV_TO_JOULE);
    return true;
}

// ---------------------------------------------------------------------------
// Estimation nette electrique — HYPOTHESES EXPLICITES (a auditer) :
//   rendement thermique->electrique eta_th = 0.33 (Rankine classique)
//   gain de couverture tritigene M_blanket = 1.15 (multiplication neutrons)
//   rendement des systemes de chauffage eta_aux = 0.40 (recirculation)
// ---------------------------------------------------------------------------
static double fusion_dt_p_net_electric_MW(double p_fus_MW, double p_aux_W) {
    const double eta_th = 0.33, m_blanket = 1.15, eta_aux = 0.40;
    return eta_th * m_blanket * p_fus_MW - (p_aux_W / 1e6) / eta_aux;
}

// ---------------------------------------------------------------------------
// Simulation de combustion complete
// ---------------------------------------------------------------------------
fusion_dt_burn_result_t fusion_dt_plasma_run_burn(fusion_dt_plasma_t* plasma) {
    fusion_dt_burn_result_t result;
    memset(&result, 0, sizeof(result));

    if (!plasma || plasma->magic_number != FUSION_DT_PLASMA_MAGIC) {
        snprintf(result.message, sizeof(result.message), "Moteur plasma invalide");
        return result;
    }

    FORENSIC_LOG_MODULE_START(FUSION_DT_MODULE_NAME, "burn_simulation");
    FORENSIC_LOG_HW_SAMPLE(FUSION_DT_MODULE_NAME);
    FORENSIC_TIMING_START(burn_timer);

    double energy_J = 0.0;
    double W_prev = plasma->state.W_J_m3;
    uint64_t steps = 0;
    bool runaway = false;

    for (uint64_t i = 0; i < plasma->config.max_steps; i++) {
        if (!fusion_dt_plasma_step(plasma)) break;
        steps++;
        energy_J += plasma->state.p_fusion_W_m3 * plasma->config.volume_m3
                    * plasma->config.dt_s;

        // Excursion thermique : sortie du domaine de validite Bosch-Hale.
        // On journalise l'anomalie et on arrete proprement (pas de resultat
        // extrapole hors du domaine physique valide du modele).
        if (plasma->state.T_keV > FUSION_DT_T_MAX_KEV) {
            FORENSIC_LOG_ANOMALY(FUSION_DT_MODULE_NAME,
                                 "excursion_thermique_T_sup_100keV",
                                 plasma->state.T_keV);
            runaway = true;
            break;
        }

        // Regime stationnaire : |dW| relatif < 1e-12 sur un pas
        double dW = fabs(plasma->state.W_J_m3 - W_prev);
        if (W_prev > 0.0 && dW / W_prev < 1e-12 && i > 100) {
            forensic_log(FORENSIC_LEVEL_INFO, __func__,
                         "Regime stationnaire atteint au pas %llu",
                         (unsigned long long)i);
            break;
        }
        W_prev = plasma->state.W_J_m3;
    }

    FORENSIC_TIMING_END(burn_timer);
    uint64_t burn_ns = FORENSIC_TIMING_CALC_NS(burn_timer);

    const fusion_dt_state_t* s = &plasma->state;
    result.success = (steps > 0);
    result.thermal_runaway_detected = runaway;
    result.T_final_keV = s->T_keV;
    result.q_factor_final = s->q_factor;
    result.p_fusion_final_MW = s->p_fusion_W_m3 * plasma->config.volume_m3 / 1e6;
    result.energy_produced_MJ = energy_J / 1e6;
    result.steps_executed = steps;
    result.total_compute_time_ns = burn_ns;
    result.state_hash_final = plasma->state_hash;
    result.triple_product_keV_s_m3 =
        plasma->config.n_e_m3 * s->T_keV *
        (s->tau_E_eff_s > 0.0 ? s->tau_E_eff_s : plasma->config.tau_E_s);
    // Ignition : le chauffage alpha couvre seul toutes les pertes
    result.ignited = (s->p_alpha_W_m3 >= s->p_brems_W_m3 + s->p_transport_W_m3);

    result.p_net_electric_est_MW =
        fusion_dt_p_net_electric_MW(result.p_fusion_final_MW,
                                    plasma->config.p_aux_W);

    snprintf(result.message, sizeof(result.message),
             "%llu pas en %llu ns : T=%.2f keV Q=%.2f P_fus=%.1f MW ignition=%s",
             (unsigned long long)steps, (unsigned long long)burn_ns,
             result.T_final_keV, result.q_factor_final, result.p_fusion_final_MW,
             result.ignited ? "OUI" : "NON");

    FORENSIC_LOG_MODULE_METRIC(FUSION_DT_MODULE_NAME, "burn_T_final_keV", result.T_final_keV);
    FORENSIC_LOG_MODULE_METRIC(FUSION_DT_MODULE_NAME, "burn_q_factor", result.q_factor_final);
    FORENSIC_LOG_MODULE_METRIC(FUSION_DT_MODULE_NAME, "burn_p_fusion_MW", result.p_fusion_final_MW);
    FORENSIC_LOG_MODULE_METRIC(FUSION_DT_MODULE_NAME, "burn_energy_MJ", result.energy_produced_MJ);
    FORENSIC_LOG_MODULE_METRIC(FUSION_DT_MODULE_NAME, "burn_steps", (double)steps);
    FORENSIC_LOG_MODULE_METRIC(FUSION_DT_MODULE_NAME, "burn_compute_ns", (double)burn_ns);
    FORENSIC_LOG_HW_SAMPLE(FUSION_DT_MODULE_NAME);
    FORENSIC_LOG_MODULE_END(FUSION_DT_MODULE_NAME, "burn_simulation", result.success);

    forensic_log(FORENSIC_LEVEL_INFO, __func__, "%s", result.message);
    return result;
}

// ---------------------------------------------------------------------------
// Projection LUM/VORAX : le bilan de particules D + T -> alpha + n est
// represente par des groupes LUM et la conservation est verifiee par
// vorax_check_conservation (unite de presence conservee par vorax_fuse).
// ---------------------------------------------------------------------------
bool fusion_dt_plasma_sync_lums(fusion_dt_plasma_t* plasma, size_t lum_population) {
    if (!plasma || plasma->magic_number != FUSION_DT_PLASMA_MAGIC || lum_population == 0) {
        forensic_log(FORENSIC_LEVEL_ERROR, __func__, "Parametres invalides");
        return false;
    }

    FORENSIC_LOG_MODULE_START(FUSION_DT_MODULE_NAME, "lum_sync");
    FORENSIC_TIMING_START(sync_timer);

    // Facteur d'echelle : ions reels representes par un LUM
    double ions_total = plasma->config.n_e_m3 * plasma->config.volume_m3;
    plasma->ions_per_lum = ions_total / (2.0 * (double)lum_population);

    // Nettoyage d'une synchronisation precedente
    if (plasma->lum_group_deuterium) { lum_group_destroy(plasma->lum_group_deuterium); plasma->lum_group_deuterium = NULL; }
    if (plasma->lum_group_tritium)   { lum_group_destroy(plasma->lum_group_tritium);   plasma->lum_group_tritium = NULL; }
    if (plasma->lum_group_alpha)     { lum_group_destroy(plasma->lum_group_alpha);     plasma->lum_group_alpha = NULL; }

    lum_group_t* group_d = lum_group_create(lum_population);
    lum_group_t* group_t = lum_group_create(lum_population);
    if (!group_d || !group_t) {
        if (group_d) lum_group_destroy(group_d);
        if (group_t) lum_group_destroy(group_t);
        return false;
    }

    for (size_t i = 0; i < lum_population; i++) {
        // presence=1 : ion present ; position spatiale = index (1D symbolique)
        lum_t* lum_d = lum_create(1, (int32_t)i, 0, LUM_STRUCTURE_LINEAR);
        lum_t* lum_t_ion = lum_create(1, (int32_t)i, 1, LUM_STRUCTURE_LINEAR);
        if (lum_d) { lum_group_add(group_d, lum_d); lum_destroy(lum_d); }
        if (lum_t_ion) { lum_group_add(group_t, lum_t_ion); lum_destroy(lum_t_ion); }
    }

    // Operation VORAX native : fusion des populations D et T
    vorax_result_t* fuse_result = vorax_fuse(group_d, group_t);
    bool conservation_ok = false;
    if (fuse_result && fuse_result->success && fuse_result->result_group) {
        lum_group_t* inputs[2] = { group_d, group_t };
        lum_group_t* outputs[1] = { fuse_result->result_group };
        conservation_ok = vorax_check_conservation(inputs, 2, outputs, 1);

        FORENSIC_LOG_MODULE_METRIC(FUSION_DT_MODULE_NAME, "lum_fuse_time_ns",
                                   (double)fuse_result->execution_time_ns);
        FORENSIC_LOG_MODULE_METRIC(FUSION_DT_MODULE_NAME, "lum_conservation",
                                   conservation_ok ? 1.0 : 0.0);

        // Le groupe fusionne represente le plasma reactif (paires D-T)
        plasma->lum_group_alpha = fuse_result->result_group;
        fuse_result->result_group = NULL;  // transfert de propriete
    }

    plasma->lum_group_deuterium = group_d;
    plasma->lum_group_tritium = group_t;
    if (fuse_result) vorax_result_destroy(fuse_result);

    FORENSIC_TIMING_END(sync_timer);
    uint64_t sync_ns = FORENSIC_TIMING_CALC_NS(sync_timer);

    forensic_log(FORENSIC_LEVEL_INFO, __func__,
                 "Sync LUM: %zu LUMs/espece, %.3e ions/LUM, conservation=%s, %llu ns",
                 lum_population, plasma->ions_per_lum,
                 conservation_ok ? "VERIFIEE" : "ECHEC",
                 (unsigned long long)sync_ns);

    FORENSIC_LOG_MODULE_END(FUSION_DT_MODULE_NAME, "lum_sync", conservation_ok);
    return conservation_ok;
}

// ---------------------------------------------------------------------------
// Optimiseur de point de fonctionnement : balayage 2D (n_e, tau_E).
// Pour chaque design candidat, une simulation de combustion complete est
// executee AVEC cendres d'helium et degradation IPB98 (physique la plus
// complete du module), puis la production electrique nette est evaluee.
// La "solution" retournee est le design viable maximisant P_net.
// ---------------------------------------------------------------------------
fusion_dt_design_result_t* fusion_dt_optimize_operating_point(
    double n_min_m3, double n_max_m3, size_t n_points,
    double tau_min_s, double tau_max_s, size_t tau_points,
    double volume_m3, double p_aux_W) {

    if (n_points < 2 || tau_points < 2 || n_min_m3 <= 0.0 || tau_min_s <= 0.0 ||
        n_max_m3 <= n_min_m3 || tau_max_s <= tau_min_s || volume_m3 <= 0.0) {
        forensic_log(FORENSIC_LEVEL_ERROR, __func__, "Parametres d'optimisation invalides");
        return NULL;
    }

    FORENSIC_LOG_MODULE_START(FUSION_DT_MODULE_NAME, "optimize_operating_point");
    FORENSIC_LOG_HW_SAMPLE(FUSION_DT_MODULE_NAME);
    FORENSIC_TIMING_START(opt_timer);

    size_t total = n_points * tau_points;
    fusion_dt_design_result_t* result = TRACKED_MALLOC(sizeof(*result));
    if (!result) return NULL;
    memset(result, 0, sizeof(*result));
    result->points = TRACKED_MALLOC(total * sizeof(fusion_dt_design_point_t));
    if (!result->points) {
        TRACKED_FREE(result);
        return NULL;
    }
    memset(result->points, 0, total * sizeof(fusion_dt_design_point_t));

    double best_p_net = -INFINITY;
    size_t idx = 0, viable_count = 0;

    for (size_t i = 0; i < n_points; i++) {
        double n_e = n_min_m3 + (n_max_m3 - n_min_m3) * (double)i / (double)(n_points - 1);
        for (size_t j = 0; j < tau_points; j++) {
            double tau = tau_min_s + (tau_max_s - tau_min_s) * (double)j / (double)(tau_points - 1);

            fusion_dt_config_t cfg;
            memset(&cfg, 0, sizeof(cfg));
            cfg.n_e_m3 = n_e;
            cfg.T_keV = 9.0;              // demarrage chaud (apres phase de chauffage)
            cfg.tau_E_s = tau;
            cfg.volume_m3 = volume_m3;
            cfg.p_aux_W = p_aux_W;
            cfg.z_eff = 1.0;              // impuretes hors helium negligees (documente)
            cfg.dt_s = 2e-3;
            cfg.max_steps = 150000;       // 300 s simulees : equilibre des cendres
            cfg.log_every = 50000;        // limiter le volume de logs par design
            cfg.tau_scaling_exponent = 0.69;
            cfg.p_ref_W = 100e6;
            cfg.enable_ash_dynamics = true;
            cfg.tau_he_ratio = 5.0;

            fusion_dt_design_point_t* pt = &result->points[idx++];
            pt->n_e_m3 = n_e;
            pt->tau_E_s = tau;

            fusion_dt_plasma_t* plasma = fusion_dt_plasma_create(&cfg);
            if (!plasma) continue;
            fusion_dt_burn_result_t burn = fusion_dt_plasma_run_burn(plasma);

            pt->T_final_keV = burn.T_final_keV;
            pt->q_factor = burn.q_factor_final;
            pt->p_fusion_MW = burn.p_fusion_final_MW;
            pt->p_net_electric_MW = burn.p_net_electric_est_MW;
            pt->helium_fraction = plasma->state.n_helium_m3 / n_e;
            pt->viable = burn.success && !burn.thermal_runaway_detected &&
                         burn.T_final_keV >= 1.0 &&
                         burn.T_final_keV <= FUSION_DT_T_MAX_KEV;
            fusion_dt_plasma_destroy(&plasma);

            FORENSIC_LOG_NANO(FUSION_DT_MODULE_NAME, "design_n_e_m3", n_e);
            FORENSIC_LOG_NANO(FUSION_DT_MODULE_NAME, "design_tau_E_s", tau);
            FORENSIC_LOG_NANO(FUSION_DT_MODULE_NAME, "design_p_net_MW",
                              pt->viable ? pt->p_net_electric_MW : -1e9);

            if (pt->viable) {
                viable_count++;
                if (pt->p_net_electric_MW > best_p_net) {
                    best_p_net = pt->p_net_electric_MW;
                    result->best = *pt;
                }
            }
        }
    }

    FORENSIC_TIMING_END(opt_timer);
    result->total_compute_time_ns = FORENSIC_TIMING_CALC_NS(opt_timer);
    result->points_evaluated = total;
    result->points_viable = viable_count;
    result->success = (viable_count > 0) && isfinite(best_p_net);

    snprintf(result->message, sizeof(result->message),
             "%zu designs evalues (%zu viables) en %llu ns : optimum n=%.3e m^-3 "
             "tau_E=%.2f s -> P_net=%.1f MW (Q=%.1f, T=%.1f keV, He=%.1f%%)",
             total, viable_count,
             (unsigned long long)result->total_compute_time_ns,
             result->best.n_e_m3, result->best.tau_E_s,
             result->best.p_net_electric_MW, result->best.q_factor,
             result->best.T_final_keV, result->best.helium_fraction * 100.0);

    FORENSIC_LOG_MODULE_METRIC(FUSION_DT_MODULE_NAME, "opt_designs_evaluated", (double)total);
    FORENSIC_LOG_MODULE_METRIC(FUSION_DT_MODULE_NAME, "opt_designs_viable", (double)viable_count);
    FORENSIC_LOG_MODULE_METRIC(FUSION_DT_MODULE_NAME, "opt_best_p_net_MW", result->best.p_net_electric_MW);
    FORENSIC_LOG_MODULE_METRIC(FUSION_DT_MODULE_NAME, "opt_best_q_factor", result->best.q_factor);
    FORENSIC_LOG_HW_SAMPLE(FUSION_DT_MODULE_NAME);
    FORENSIC_LOG_MODULE_END(FUSION_DT_MODULE_NAME, "optimize_operating_point", result->success);

    forensic_log(FORENSIC_LEVEL_INFO, __func__, "%s", result->message);
    return result;
}

void fusion_dt_design_result_destroy(fusion_dt_design_result_t** result_ptr) {
    if (!result_ptr || !*result_ptr) return;
    if ((*result_ptr)->points) TRACKED_FREE((*result_ptr)->points);
    TRACKED_FREE(*result_ptr);
    *result_ptr = NULL;
}

// ---------------------------------------------------------------------------
// Integration lum_memory_tracer : snapshot bit a bit de l'etat plasma au
// format .lum natif (1 LUM = 1 bit), reconstruction, et preuve diff=0.
// C'est la traçabilite bit-level NATIVE du projet (magic LUMT, cycle C111).
// ---------------------------------------------------------------------------
bool fusion_dt_snapshot_bit_level(const fusion_dt_plasma_t* plasma,
                                  const char* out_path) {
    if (!plasma || plasma->magic_number != FUSION_DT_PLASMA_MAGIC || !out_path) {
        forensic_log(FORENSIC_LEVEL_ERROR, __func__, "Parametres invalides");
        return false;
    }

    FORENSIC_LOG_MODULE_START(FUSION_DT_MODULE_NAME, "bit_level_snapshot");

    lum_trace_stats_t stats;
    memset(&stats, 0, sizeof(stats));
    int rc = lum_memory_snapshot_buffer(&plasma->state, sizeof(fusion_dt_state_t),
                                        out_path, LUM_TRACE_GRANULARITY_BIT, &stats);
    if (rc != 0) {
        forensic_log(FORENSIC_LEVEL_ERROR, __func__,
                     "Echec snapshot bit-level (rc=%d) vers %s", rc, out_path);
        FORENSIC_LOG_MODULE_END(FUSION_DT_MODULE_NAME, "bit_level_snapshot", false);
        return false;
    }

    // Reconstruction puis preuve diff=0 (fonctions natives du projet)
    fusion_dt_state_t reconstructed;
    memset(&reconstructed, 0, sizeof(reconstructed));
    size_t bytes_restored = 0;
    rc = lum_memory_reconstruct(out_path, &reconstructed, sizeof(reconstructed),
                                &bytes_restored);
    uint64_t diff_count = UINT64_MAX;
    bool diff_zero = false;
    if (rc == 0) {
        diff_zero = (memcmp(&reconstructed, &plasma->state,
                            sizeof(fusion_dt_state_t)) == 0);
        diff_count = diff_zero ? 0 : UINT64_MAX;
        // Verification croisee par le validateur natif
        if (lum_memory_validate_diff_zero(out_path, &reconstructed,
                                          sizeof(reconstructed), &diff_count) == 0) {
            diff_zero = diff_zero && (diff_count == 0);
        }
    }

    FORENSIC_LOG_MODULE_METRIC(FUSION_DT_MODULE_NAME, "snapshot_lums_emitted",
                               (double)stats.total_lums_emitted);
    FORENSIC_LOG_MODULE_METRIC(FUSION_DT_MODULE_NAME, "snapshot_duration_ns",
                               (double)stats.snapshot_ns);
    FORENSIC_LOG_MODULE_METRIC(FUSION_DT_MODULE_NAME, "snapshot_diff_count",
                               (double)diff_count);

    forensic_log(FORENSIC_LEVEL_INFO, __func__,
                 "Snapshot bit-level: %llu LUMs (1 bit/LUM) en %llu ns -> %s, "
                 "reconstruction %zu octets, diff=%llu (%s)",
                 (unsigned long long)stats.total_lums_emitted,
                 (unsigned long long)stats.snapshot_ns, out_path,
                 bytes_restored, (unsigned long long)diff_count,
                 diff_zero ? "PREUVE DIFF=0 OK" : "ECHEC");

    FORENSIC_LOG_MODULE_END(FUSION_DT_MODULE_NAME, "bit_level_snapshot", diff_zero);
    return diff_zero;
}
