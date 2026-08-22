// MODULE FUSION_DT_REACTOR — Implementation
// Voir fusion_dt_reactor.h pour les references de chaque contrainte.
// Toute valeur produite provient d'une simulation de combustion reellement
// executee (moteur fusion_dt_plasma, IPB98(y,2) complete, cendres d'helium),
// journalisee par l'infrastructure forensique nanoseconde du projet.

#include "fusion_dt_reactor.h"
#include "fusion_dt_profiles.h"
#include "../debug/forensic_logger.h"
#include "../debug/ultra_forensic_logger.h"
#include "../debug/memory_tracker.h"

#include <math.h>
#include <string.h>
#include <stdio.h>

#define FUSION_DT_REACTOR_MODULE "fusion_dt_reactor"

// ---------------------------------------------------------------------------
// Catalogues materiaux
// ---------------------------------------------------------------------------
fusion_dt_material_catalog_t fusion_dt_catalog_lts_iter(void) {
    fusion_dt_material_catalog_t c;
    memset(&c, 0, sizeof(c));
    snprintf(c.name, sizeof(c.name), "LTS_Nb3Sn_EUROFER (etat ITER)");
    c.b_coil_max_T = 12.0;        // Nb3Sn : ~11.8 T au conducteur ITER
    c.d_gap_m = 1.4;              // couverture + ecran neutronique ITER
    c.wall_load_max_MW_m2 = 1.0;  // aciers EUROFER, ~1 MW/m2 en continu
    c.beta_n_max = 2.8;           // Troyon standard
    c.q95_min = 3.0;              // regle de conception ITER
    c.greenwald_frac_max = 0.90;
    c.eta_th = 0.33;              // cycle Rankine classique
    c.eta_aux = 0.40;
    c.div_limit_MW_m = 17.0;      // divertor tungstene classe ITER (~15-17 MW/m)
    c.f_rad_max = 0.70;           // semis Ne/Ar demontre (ASDEX-U, JET)
    c.gamma_cd = 0.30;            // NBI/ECCD etat de l'art
    c.tbr_local = 1.30;           // couverture HCPB (EU-DEMO, publie ~1.30)
    c.blanket_coverage = 0.85;    // 15% perdus (ports + divertor) -> TBR ~1.10
    c.hypothetical = false;
    return c;
}

fusion_dt_material_catalog_t fusion_dt_catalog_hts_rebco(void) {
    fusion_dt_material_catalog_t c;
    memset(&c, 0, sizeof(c));
    snprintf(c.name, sizeof(c.name), "HTS_REBCO_W (etat SPARC/ARC)");
    c.b_coil_max_T = 23.0;        // rubans REBCO : ~23 T demontres (CFS 2021)
    c.d_gap_m = 0.85;             // couverture liquide compacte type FLiBe
    c.wall_load_max_MW_m2 = 2.5;  // tungstene + refroidissement avance
    c.beta_n_max = 3.0;
    c.q95_min = 3.0;
    c.greenwald_frac_max = 0.90;
    c.eta_th = 0.40;              // cycle a sels fondus haute temperature
    c.eta_aux = 0.40;
    c.div_limit_MW_m = 20.0;      // divertor avance (geometrie longue jambe)
    c.f_rad_max = 0.80;           // semis pousse type DEMO
    c.gamma_cd = 0.35;            // LHCD haute efficacite (design ARC)
    c.tbr_local = 1.42;           // couverture liquide FLiBe+Be immersive
    c.blanket_coverage = 0.92;    // couverture quasi-complete -> TBR ~1.31 (ARC: 1.3)
    c.hypothetical = false;
    return c;
}

fusion_dt_material_catalog_t fusion_dt_catalog_future_hypothetical(void) {
    fusion_dt_material_catalog_t c;
    memset(&c, 0, sizeof(c));
    snprintf(c.name, sizeof(c.name), "FUTUR_HYPOTHETIQUE (non demontre)");
    c.b_coil_max_T = 32.0;        // HYPOTHESE : REBCO ameliore / hybrides
    c.d_gap_m = 0.70;             // HYPOTHESE : couverture ultra-compacte
    c.wall_load_max_MW_m2 = 4.0;  // HYPOTHESE : composites W-SiC avances
    c.beta_n_max = 3.5;           // HYPOTHESE : controle MHD actif
    c.q95_min = 3.0;
    c.greenwald_frac_max = 0.90;
    c.eta_th = 0.45;              // HYPOTHESE : cycles supercritiques
    c.eta_aux = 0.50;
    c.div_limit_MW_m = 25.0;      // HYPOTHESE : divertor metal liquide
    c.f_rad_max = 0.85;           // HYPOTHESE : controle radiatif avance
    c.gamma_cd = 0.45;            // HYPOTHESE : CD haute efficacite
    c.tbr_local = 1.50;           // HYPOTHESE : couverture enrichie 6Li optimisee
    c.blanket_coverage = 0.93;    // HYPOTHESE : integration poussee -> TBR ~1.40
    c.hypothetical = true;
    return c;
}

// ---------------------------------------------------------------------------
// Formules publiees
// ---------------------------------------------------------------------------
double fusion_dt_q95_uckan(double R_m, double a_m, double kappa, double delta,
                           double B0_T, double I_p_MA) {
    if (R_m <= 0.0 || a_m <= 0.0 || I_p_MA <= 0.0) return 0.0;
    double eps = a_m / R_m;
    double kappa_term =
        (1.0 + kappa * kappa * (1.0 + 2.0 * delta * delta
                                - 1.2 * delta * delta * delta)) / 2.0;
    double shape = (1.17 - 0.65 * eps) / ((1.0 - eps * eps) * (1.0 - eps * eps));
    return (5.0 * a_m * a_m * B0_T / (R_m * I_p_MA)) * kappa_term * shape;
}

double fusion_dt_p_lh_martin(double n_e_m3, double B0_T, double surface_m2) {
    double n20 = n_e_m3 / 1e20;
    if (n20 <= 0.0 || B0_T <= 0.0 || surface_m2 <= 0.0) return INFINITY;
    return 0.0488 * pow(n20, 0.717) * pow(B0_T, 0.803) * pow(surface_m2, 0.941);
}

// ---------------------------------------------------------------------------
// Derivation machine : B0 au max permis par le conducteur, I_p au max permis
// par q95_min (courant maximal = meilleur confinement IPB98 en I^0.93).
// ---------------------------------------------------------------------------
void fusion_dt_machine_derive(fusion_dt_machine_t* machine,
                              const fusion_dt_material_catalog_t* catalog,
                              double R_m, double aspect_ratio,
                              double kappa, double delta, double q95_target) {
    if (!machine || !catalog) return;
    memset(machine, 0, sizeof(*machine));
    if (q95_target < catalog->q95_min) q95_target = catalog->q95_min;

    machine->R_m = R_m;
    machine->a_m = R_m / aspect_ratio;
    machine->kappa = kappa;
    machine->delta = delta;

    // C6 : champ sur l'axe limite par le champ au conducteur
    double leg = R_m - machine->a_m - catalog->d_gap_m;
    if (leg <= 0.05) leg = 0.05;   // machine geometriquement impossible sinon
    machine->B0_T = catalog->b_coil_max_T * leg / R_m;
    machine->b_coil_T = machine->B0_T * R_m / leg;

    // C3 : courant fixe par la cible q95 (inversion de la formule d'Uckan)
    double q95_unit = fusion_dt_q95_uckan(R_m, machine->a_m, kappa, delta,
                                          machine->B0_T, 1.0);
    machine->I_p_MA = q95_unit / q95_target;
    machine->q95 = fusion_dt_q95_uckan(R_m, machine->a_m, kappa, delta,
                                       machine->B0_T, machine->I_p_MA);

    // Geometrie toroide
    machine->volume_m3 = 2.0 * M_PI * M_PI * R_m * machine->a_m * machine->a_m * kappa;
    machine->surface_m2 = 4.0 * M_PI * M_PI * R_m * machine->a_m
                          * sqrt((1.0 + kappa * kappa) / 2.0);

    // C1 : densite de Greenwald
    machine->n_gw_m3 = machine->I_p_MA / (M_PI * machine->a_m * machine->a_m) * 1e20;
}

// Combustion reelle d'un design a p_aux donne (IPB98 complete + cendres)
static bool fusion_dt_reactor_burn(const fusion_dt_machine_t* machine,
                                   double n_e_m3, double p_aux_W,
                                   double profile_peaking,
                                   fusion_dt_reactor_point_t* out,
                                   double* p_brems_total_W,
                                   fusion_dt_burn_result_t* burn_out) {
    fusion_dt_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.n_e_m3 = n_e_m3;
    cfg.T_keV = 9.0;
    cfg.volume_m3 = machine->volume_m3;
    cfg.p_aux_W = p_aux_W;
    cfg.z_eff = 1.0;
    cfg.dt_s = 2e-3;
    cfg.max_steps = 150000;          // 300 s simulees (equilibre des cendres)
    cfg.log_every = 50000;
    cfg.enable_ash_dynamics = true;
    cfg.tau_he_ratio = 5.0;
    cfg.use_ipb98_full = true;
    cfg.ipb98_I_MA = machine->I_p_MA;
    cfg.ipb98_B_T = machine->B0_T;
    cfg.ipb98_R_m = machine->R_m;
    cfg.ipb98_epsilon = machine->a_m / machine->R_m;
    cfg.ipb98_kappa = machine->kappa;
    cfg.ipb98_M_amu = 2.5;
    cfg.ipb98_h98 = 1.0;             // conservateur : conforme base mondiale
    cfg.profile_peaking = profile_peaking;  // V5 : calcule par profils radiaux

    fusion_dt_plasma_t* plasma = fusion_dt_plasma_create(&cfg);
    if (!plasma) return false;
    *burn_out = fusion_dt_plasma_run_burn(plasma);

    out->T_final_keV = burn_out->T_final_keV;
    out->q_factor = burn_out->q_factor_final;
    out->p_fusion_MW = burn_out->p_fusion_final_MW;
    out->tau_E_s = plasma->state.tau_E_eff_s;
    out->he_fraction = plasma->state.n_helium_m3 / n_e_m3;
    out->p_heat_MW = (plasma->state.p_alpha_W_m3 * machine->volume_m3
                      + p_aux_W) / 1e6;
    *p_brems_total_W = plasma->state.p_brems_W_m3 * machine->volume_m3;
    fusion_dt_plasma_destroy(&plasma);
    return true;
}

// ---------------------------------------------------------------------------
// Evaluation d'un design : combustion reelle a l'equilibre + contraintes
// ---------------------------------------------------------------------------
bool fusion_dt_reactor_evaluate(const fusion_dt_material_catalog_t* catalog,
                                const fusion_dt_machine_t* machine,
                                double greenwald_fraction, double p_aux_W,
                                bool steady_state,
                                fusion_dt_reactor_point_t* out) {
    if (!catalog || !machine || !out || greenwald_fraction <= 0.0) return false;
    memset(out, 0, sizeof(*out));
    out->machine = *machine;
    out->p_aux_W = p_aux_W;
    out->n_e_m3 = greenwald_fraction * machine->n_gw_m3;

    double p_brems_W = 0.0;
    fusion_dt_burn_result_t burn;
    memset(&burn, 0, sizeof(burn));
    // Premier burn avec le piquage de calibration ITER (1.3)
    out->peaking_used = 1.3;
    if (!fusion_dt_reactor_burn(machine, out->n_e_m3, p_aux_W,
                                out->peaking_used, out, &p_brems_W, &burn))
        return false;

    // V5 : piquage recalcule par integration radiale reelle a la temperature
    // d'equilibre du design (forme calculee, amplitude ancree ITER — cf.
    // fusion_dt_profiles.h)
    double peaking_calc = fusion_dt_profiles_effective_peaking(
        out->T_final_keV, FUSION_DT_ALPHA_N_DEFAULT, FUSION_DT_ALPHA_T_DEFAULT);

    // ---- V4 : stationnarite (bootstrap + generation de courant) ----
    // beta_p = 2 mu0 <p> / B_p^2, B_p = mu0 I_p / l_p (perimetre poloidal)
    // f_bs = 0.7 sqrt(eps) beta_p (Wesson), I_CD = I_p (1 - f_bs)
    // P_CD = n20 R I_CD / gamma_CD
    {
        double eps = machine->a_m / machine->R_m;
        double l_p = 2.0 * M_PI * machine->a_m
                     * sqrt((1.0 + machine->kappa * machine->kappa) / 2.0);
        double B_p = 4.0e-7 * M_PI * machine->I_p_MA * 1e6 / l_p;
        double p_pa = 2.0 * out->n_e_m3 * out->T_final_keV * FUSION_DT_KEV_TO_JOULE;
        out->beta_p = 2.0 * 4.0e-7 * M_PI * p_pa / (B_p * B_p);
        out->f_bootstrap = 0.7 * sqrt(eps) * out->beta_p;
        if (out->f_bootstrap > 0.95) out->f_bootstrap = 0.95;
        out->i_cd_MA = machine->I_p_MA * (1.0 - out->f_bootstrap);
        double n20 = out->n_e_m3 / 1e20;
        out->p_cd_MW = n20 * machine->R_m * (out->i_cd_MA * 1e6)
                       / catalog->gamma_cd / 1e6;
    }

    // Coherence stationnaire + piquage : la puissance CD chauffe aussi le
    // plasma, et le piquage radial reel differe du 1.3 initial. On re-simule
    // la combustion (une iteration suffit : dependances faibles en T).
    // Machines pulsees (ITER) : courant par transformateur, p_aux inchange.
    bool need_reburn_cd = steady_state && out->p_cd_MW * 1e6 > p_aux_W * 1.05;
    bool need_reburn_peaking = fabs(peaking_calc - out->peaking_used) > 0.04;
    if (need_reburn_cd || need_reburn_peaking) {
        double p_aux2 = need_reburn_cd ? out->p_cd_MW * 1e6 : p_aux_W;
        out->peaking_used = peaking_calc;
        if (fusion_dt_reactor_burn(machine, out->n_e_m3, p_aux2,
                                   out->peaking_used, out, &p_brems_W, &burn)) {
            out->p_aux_W = p_aux2;
            // Recalcul bootstrap avec la temperature re-equilibree
            double eps = machine->a_m / machine->R_m;
            double l_p = 2.0 * M_PI * machine->a_m
                         * sqrt((1.0 + machine->kappa * machine->kappa) / 2.0);
            double B_p = 4.0e-7 * M_PI * machine->I_p_MA * 1e6 / l_p;
            double p_pa = 2.0 * out->n_e_m3 * out->T_final_keV
                          * FUSION_DT_KEV_TO_JOULE;
            out->beta_p = 2.0 * 4.0e-7 * M_PI * p_pa / (B_p * B_p);
            out->f_bootstrap = 0.7 * sqrt(eps) * out->beta_p;
            if (out->f_bootstrap > 0.95) out->f_bootstrap = 0.95;
            out->i_cd_MA = machine->I_p_MA * (1.0 - out->f_bootstrap);
            double n20 = out->n_e_m3 / 1e20;
            out->p_cd_MW = n20 * machine->R_m * (out->i_cd_MA * 1e6)
                           / catalog->gamma_cd / 1e6;
        }
    }

    // Recirculation reelle : le plus grand des deux postes (le systeme CD
    // fournit aussi du chauffage — on ne compte pas double). En mode pulse,
    // seule la puissance de chauffage recircule (P_CD informatif).
    out->p_recirc_MW = (steady_state && out->p_cd_MW > out->p_aux_W / 1e6)
                           ? out->p_cd_MW : out->p_aux_W / 1e6;

    // P_net avec les rendements DU CATALOGUE et la recirculation stationnaire
    out->p_net_MW = catalog->eta_th * 1.15 * out->p_fusion_MW
                    - out->p_recirc_MW / catalog->eta_aux;

    // ---- Contraintes ----
    out->f_greenwald = out->n_e_m3 / machine->n_gw_m3;
    out->c_greenwald = (out->f_greenwald <= catalog->greenwald_frac_max + 1e-9);

    // C2 Troyon : beta = 2 mu0 <p> / B^2, <p> = 2 n T (electrons + ions)
    {
        double p_pa = 2.0 * out->n_e_m3 * out->T_final_keV * FUSION_DT_KEV_TO_JOULE;
        double beta = 2.0 * 4.0e-7 * M_PI * p_pa / (machine->B0_T * machine->B0_T);
        out->beta_n = beta * 100.0 * machine->a_m * machine->B0_T / machine->I_p_MA;
        out->c_beta = (out->beta_n <= catalog->beta_n_max);
    }

    // Tolerance 3% : la formule d'Uckan est une approximation (+-5%) de
    // l'equilibre de Grad-Shafranov (ITER exact : q95=3.00, Uckan : 2.996).
    out->c_q95 = (machine->q95 >= catalog->q95_min * 0.97);

    // C4 charge murale : 80% de l'energie de fusion part en neutrons
    out->wall_load_MW_m2 = 0.8 * out->p_fusion_MW / machine->surface_m2;
    out->c_wall = (out->wall_load_MW_m2 <= catalog->wall_load_max_MW_m2);

    // C5 seuil L-H : le chauffage total doit maintenir le mode H
    out->p_lh_MW = fusion_dt_p_lh_martin(out->n_e_m3, machine->B0_T,
                                         machine->surface_m2);
    out->c_lh = (out->p_heat_MW >= out->p_lh_MW);

    out->c_bcoil = (machine->b_coil_T <= catalog->b_coil_max_T + 1e-9);

    // C7 : domaine de confiance du modele (les pertes synchrotron ~B^2 T^2
    // non modelisees rendent les regimes T > 25 keV artificiellement
    // favorables ; les reacteurs reels operent a 8-25 keV).
    out->c_regime = (burn.T_final_keV <= 25.0);

    // C8 (V4) : evacuation de puissance au divertor. La puissance conduite a
    // la separatrice P_sep = P_chauffage - P_brems doit respecter
    // P_sep (1 - f_rad) / R <= limite divertor ; f_rad est controlable par
    // semis d'impuretes jusqu'a f_rad_max (ASDEX-U/JET/DEMO).
    {
        out->p_sep_MW = out->p_heat_MW - p_brems_W / 1e6;
        if (out->p_sep_MW < 0.0) out->p_sep_MW = 0.0;
        double p_div_bare = out->p_sep_MW / machine->R_m;  // MW/m sans semis
        out->f_rad_required = (p_div_bare > catalog->div_limit_MW_m)
            ? 1.0 - catalog->div_limit_MW_m / p_div_bare
            : 0.0;
        out->c_divertor = (out->f_rad_required <= catalog->f_rad_max + 1e-9);
    }

    // V6 : indice economique physique — energie magnetique stockee (premier
    // poste de cout d'un tokamak, proportionnelle a la masse d'aimants)
    {
        const double mu0 = 4.0e-7 * M_PI;
        double r_env = machine->a_m + catalog->d_gap_m;   // rayon enveloppe bobines
        double v_field = 2.0 * M_PI * M_PI * machine->R_m * r_env * r_env
                         * machine->kappa;
        out->e_mag_GJ = machine->B0_T * machine->B0_T / (2.0 * mu0)
                        * v_field / 1e9;
        out->cost_index_GJ_MW = (out->p_net_MW > 0.0)
            ? out->e_mag_GJ / out->p_net_MW : INFINITY;
    }

    // C9 (V5) : auto-suffisance en tritium. TBR = TBR_local du concept de
    // couverture x couverture geometrique. Consommation : chaque MW de
    // fusion brule 0.0561 kg de tritium par an (3.548e17 reactions/s/MW
    // x 5.007e-27 kg/triton x 3.156e7 s/an).
    {
        out->tbr = catalog->tbr_local * catalog->blanket_coverage;
        out->tritium_burn_kg_year = 0.0561 * out->p_fusion_MW;
        out->tritium_margin_kg_year = (out->tbr - 1.0) * out->tritium_burn_kg_year;
        out->c_tbr = (out->tbr >= 1.05);
    }

    bool burn_ok = burn.success && !burn.thermal_runaway_detected &&
                   burn.T_final_keV >= 4.0 &&
                   burn.T_final_keV <= FUSION_DT_T_MAX_KEV;
    out->viable = burn_ok && out->c_greenwald && out->c_beta && out->c_q95 &&
                  out->c_wall && out->c_lh && out->c_bcoil && out->c_regime &&
                  out->c_divertor && out->c_tbr;
    return true;
}

// ---------------------------------------------------------------------------
// Optimisation contrainte
// ---------------------------------------------------------------------------
fusion_dt_reactor_result_t* fusion_dt_reactor_optimize(
    const fusion_dt_material_catalog_t* catalog,
    double R_min_m, double R_max_m, size_t R_points,
    double fgw_min, double fgw_max, size_t fgw_points,
    double aspect_ratio, double kappa, double delta, double p_aux_W) {

    if (!catalog || R_points < 2 || fgw_points < 2 || R_min_m <= 0.0 ||
        R_max_m <= R_min_m || fgw_min <= 0.0 || fgw_max <= fgw_min) {
        forensic_log(FORENSIC_LEVEL_ERROR, __func__, "Parametres invalides");
        return NULL;
    }

    FORENSIC_LOG_MODULE_START(FUSION_DT_REACTOR_MODULE, "constrained_optimize");
    FORENSIC_LOG_HW_SAMPLE(FUSION_DT_REACTOR_MODULE);
    FORENSIC_TIMING_START(opt_timer);

    // P_aux est une dimension d'optimisation : les petits reacteurs sont
    // penalises par la recirculation d'un chauffage surdimensionne
    // (ARC opere a 38 MW quand ITER en demande 50).
    const double p_aux_fracs[3] = {0.6, 0.8, 1.0};
    const size_t p_aux_points = 3;
    // q95 est un levier de conception : ITER 3.0, ARC 7.2. Les machines a
    // tres haut champ reduisent leur courant pour rester dans la fenetre
    // (T <= 25 keV, mur) — sans ce levier, les hauts champs sont invendables.
    const double q95_mults[3] = {1.0, 1.6, 2.4};
    const size_t q95_points = 3;

    size_t total = R_points * fgw_points * p_aux_points * q95_points;
    fusion_dt_reactor_result_t* result = TRACKED_MALLOC(sizeof(*result));
    if (!result) return NULL;
    memset(result, 0, sizeof(*result));
    result->catalog = *catalog;
    result->points = TRACKED_MALLOC(total * sizeof(fusion_dt_reactor_point_t));
    if (!result->points) {
        TRACKED_FREE(result);
        return NULL;
    }
    memset(result->points, 0, total * sizeof(fusion_dt_reactor_point_t));

    double best_p_net = -INFINITY;
    size_t idx = 0, viable_count = 0;

    for (size_t i = 0; i < R_points; i++) {
        double R = R_min_m + (R_max_m - R_min_m) * (double)i / (double)(R_points - 1);
        for (size_t q = 0; q < q95_points; q++) {
        fusion_dt_machine_t machine;
        fusion_dt_machine_derive(&machine, catalog, R, aspect_ratio, kappa, delta,
                                 catalog->q95_min * q95_mults[q]);

        for (size_t j = 0; j < fgw_points; j++) {
            double fgw = fgw_min + (fgw_max - fgw_min) * (double)j / (double)(fgw_points - 1);
            for (size_t k = 0; k < p_aux_points; k++) {
                double p_aux = p_aux_W * p_aux_fracs[k];
                fusion_dt_reactor_point_t* pt = &result->points[idx++];
                if (!fusion_dt_reactor_evaluate(catalog, &machine, fgw, p_aux,
                                                true, pt))
                    continue;

                FORENSIC_LOG_NANO(FUSION_DT_REACTOR_MODULE, "reactor_R_m", R);
                FORENSIC_LOG_NANO(FUSION_DT_REACTOR_MODULE, "reactor_fgw", fgw);
                FORENSIC_LOG_NANO(FUSION_DT_REACTOR_MODULE, "reactor_p_net_MW",
                                  pt->viable ? pt->p_net_MW : -1e9);

                if (pt->viable) {
                    viable_count++;
                    if (pt->p_net_MW > best_p_net) {
                        best_p_net = pt->p_net_MW;
                        result->best = *pt;
                    }
                }
            }
        }
        }
    }

    FORENSIC_TIMING_END(opt_timer);
    result->total_compute_time_ns = FORENSIC_TIMING_CALC_NS(opt_timer);
    result->points_evaluated = total;
    result->points_viable = viable_count;
    result->success = (viable_count > 0) && isfinite(best_p_net);

    const fusion_dt_reactor_point_t* b = &result->best;
    snprintf(result->message, sizeof(result->message),
             "[%s] %zu designs (%zu viables) en %llu ns. Optimum: R=%.2f m "
             "a=%.2f m B0=%.2f T Ip=%.1f MA q95=%.1f n=%.2e T=%.1f keV "
             "tau_E=%.2f s Q=%.1f P_fus=%.0f MW P_net=%.0f MW mur=%.2f MW/m2 "
             "betaN=%.2f fGW=%.2f P_recirc=%.0f MW f_bs=%.2f P_CD=%.0f MW "
             "f_rad=%.2f TBR=%.2f T_marge=%.1f kg/an piquage=%.2f He=%.1f%%",
             catalog->name, total, viable_count,
             (unsigned long long)result->total_compute_time_ns,
             b->machine.R_m, b->machine.a_m, b->machine.B0_T, b->machine.I_p_MA,
             b->machine.q95, b->n_e_m3, b->T_final_keV, b->tau_E_s, b->q_factor,
             b->p_fusion_MW, b->p_net_MW, b->wall_load_MW_m2, b->beta_n,
             b->f_greenwald, b->p_recirc_MW, b->f_bootstrap, b->p_cd_MW,
             b->f_rad_required, b->tbr, b->tritium_margin_kg_year,
             b->peaking_used, b->he_fraction * 100.0);

    FORENSIC_LOG_MODULE_METRIC(FUSION_DT_REACTOR_MODULE, "opt_viable", (double)viable_count);
    FORENSIC_LOG_MODULE_METRIC(FUSION_DT_REACTOR_MODULE, "opt_best_p_net_MW",
                               result->success ? b->p_net_MW : -1e9);
    FORENSIC_LOG_HW_SAMPLE(FUSION_DT_REACTOR_MODULE);
    FORENSIC_LOG_MODULE_END(FUSION_DT_REACTOR_MODULE, "constrained_optimize",
                            result->success);
    forensic_log(FORENSIC_LEVEL_INFO, __func__, "%s", result->message);
    return result;
}

void fusion_dt_reactor_result_destroy(fusion_dt_reactor_result_t** result_ptr) {
    if (!result_ptr || !*result_ptr) return;
    if ((*result_ptr)->points) TRACKED_FREE((*result_ptr)->points);
    TRACKED_FREE(*result_ptr);
    *result_ptr = NULL;
}
