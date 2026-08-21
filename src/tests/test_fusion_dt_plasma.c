// TEST FUSION_DT_PLASMA — Validation scientifique et demonstration complete
// ==========================================================================
// Protocole en 5 phases, chaque phase journalisee (forensic + ultra-forensic):
//   Phase 1 : validation de la reactivite Bosch-Hale contre les valeurs
//             publiees (Bosch & Hale 1992 / NRL Plasma Formulary).
//   Phase 2 : balayage de Lawson 1-100 keV, export CSV, extraction des
//             points de fonctionnement optimaux (ignition).
//   Phase 3 : simulations de combustion 0-D — trois scenarios :
//             (a) type ITER (Q~10), (b) reacteur en ignition, (c) sous-critique.
//   Phase 4 : projection LUM/VORAX du bilan de particules + conservation.
//   Phase 5 : verification de la tracabilite bit-level (sensibilite du hash
//             a une perturbation d'un seul bit de l'etat).
// Tout resultat imprime provient d'un calcul reellement execute ici.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../physics/fusion_dt_plasma.h"
#include "../physics/fusion_dt_reactor.h"
#include "../debug/memory_tracker.h"
#include "../debug/forensic_logger.h"
#include "../debug/ultra_forensic_logger.h"

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, label)                                              \
    do {                                                                      \
        if (cond) {                                                           \
            g_tests_passed++;                                                 \
            printf("  [PASS] %s\n", label);                                   \
        } else {                                                              \
            g_tests_failed++;                                                 \
            printf("  [FAIL] %s\n", label);                                   \
        }                                                                     \
    } while (0)

// Ecart relatif
static double rel_err(double value, double reference) {
    return fabs(value - reference) / fabs(reference);
}

// ---------------------------------------------------------------------------
// PHASE 1 — Validation Bosch-Hale contre valeurs publiees
// References: Bosch & Hale, Nucl. Fusion 32 (1992) 611, table VIII (D-T).
// Valeurs en cm^3/s converties en m^3/s (x 1e-6).
// ---------------------------------------------------------------------------
static void phase1_validation_bosch_hale(void) {
    printf("\n=== PHASE 1 : VALIDATION REACTIVITE BOSCH-HALE (D-T) ===\n");
    FORENSIC_LOG_MODULE_START("fusion_dt_plasma", "phase1_validation");

    struct { double T_keV; double sv_ref_m3s; double tol; } refs[] = {
        {  1.0, 6.857e-27, 0.05 },   // 6.857e-21 cm3/s
        {  2.0, 2.977e-25, 0.05 },   // 2.977e-19 cm3/s
        {  5.0, 1.366e-23, 0.05 },   // 1.366e-17 cm3/s
        { 10.0, 1.136e-22, 0.05 },   // 1.136e-16 cm3/s
        { 20.0, 4.330e-22, 0.05 },   // 4.330e-16 cm3/s
        { 50.0, 8.702e-22, 0.05 },   // 8.702e-16 cm3/s
    };
    size_t n_refs = sizeof(refs) / sizeof(refs[0]);

    for (size_t i = 0; i < n_refs; i++) {
        double sv = fusion_dt_reactivity_bosch_hale(refs[i].T_keV);
        double err = rel_err(sv, refs[i].sv_ref_m3s);
        char label[128];
        snprintf(label, sizeof(label),
                 "<sigma-v>(%.0f keV) = %.4e m^3/s (ref %.4e, ecart %.2f%%)",
                 refs[i].T_keV, sv, refs[i].sv_ref_m3s, err * 100.0);
        TEST_ASSERT(err < refs[i].tol, label);
        FORENSIC_LOG_MODULE_METRIC("fusion_dt_plasma", "validation_ecart_relatif", err);
    }

    // Hors domaine : doit retourner 0 (et journaliser un WARNING)
    TEST_ASSERT(fusion_dt_reactivity_bosch_hale(0.05) == 0.0,
                "T=0.05 keV hors domaine -> reactivite nulle");
    TEST_ASSERT(fusion_dt_reactivity_bosch_hale(500.0) == 0.0,
                "T=500 keV hors domaine -> reactivite nulle");

    FORENSIC_LOG_MODULE_END("fusion_dt_plasma", "phase1_validation", g_tests_failed == 0);
}

// ---------------------------------------------------------------------------
// PHASE 2 — Balayage de Lawson et points de fonctionnement optimaux
// ---------------------------------------------------------------------------
static void phase2_lawson_scan(void) {
    printf("\n=== PHASE 2 : BALAYAGE DE LAWSON (IGNITION D-T) ===\n");

    fusion_dt_lawson_result_t* scan = fusion_dt_lawson_scan(1.0, 100.0, 0.05, 1.0);
    TEST_ASSERT(scan != NULL && scan->success, "Balayage de Lawson execute");
    if (!scan) return;

    printf("  %s\n", scan->message);
    printf("  Temperature optimale (triple produit min) : %.2f keV\n", scan->T_optimal_keV);
    printf("  Triple produit minimal n*T*tau_E          : %.4e keV.s/m^3\n", scan->triple_product_min);
    printf("  n*tau_E minimal                            : %.4e s/m^3 (a T=%.2f keV)\n",
           scan->n_tau_min_s_m3, scan->T_ntau_optimal_keV);

    // Verification contre la litterature :
    //   minimum du triple produit D-T attendu ~3e21 keV.s/m^3 vers 13-15 keV
    //   minimum de n*tau attendu ~1.5-2e20 s/m^3 vers 25-30 keV
    TEST_ASSERT(scan->T_optimal_keV > 10.0 && scan->T_optimal_keV < 20.0,
                "T optimale triple produit dans [10,20] keV (litterature ~13-15)");
    TEST_ASSERT(scan->triple_product_min > 1e21 && scan->triple_product_min < 6e21,
                "Triple produit min dans [1e21,6e21] keV.s/m^3 (litterature ~3e21)");
    TEST_ASSERT(scan->T_ntau_optimal_keV > 20.0 && scan->T_ntau_optimal_keV < 35.0,
                "T optimale n*tau dans [20,35] keV (litterature ~25-30)");

    // Export CSV complet pour audit externe
    mkdir("logs", 0755);
    mkdir("logs/fusion", 0755);
    FILE* csv = fopen("logs/fusion/lawson_scan.csv", "w");
    if (csv) {
        fprintf(csv, "T_keV,reactivity_m3_s,n_tau_required_s_m3,triple_product_keV_s_m3\n");
        for (size_t i = 0; i < scan->point_count; i++) {
            fprintf(csv, "%.3f,%.6e,%.6e,%.6e\n",
                    scan->points[i].T_keV, scan->points[i].reactivity_m3_s,
                    scan->points[i].n_tau_required_s_m3,
                    scan->points[i].triple_product_keV_s_m3);
        }
        fclose(csv);
        printf("  CSV exporte : logs/fusion/lawson_scan.csv (%zu points)\n",
               scan->point_count);
    }
    TEST_ASSERT(csv != NULL, "Export CSV du balayage");

    fusion_dt_lawson_result_destroy(&scan);
    TEST_ASSERT(scan == NULL, "Destruction securisee du resultat (double-free protege)");
}

// ---------------------------------------------------------------------------
// PHASE 3 — Simulations de combustion 0-D
// ---------------------------------------------------------------------------
static void phase3_burn_scenarios(void) {
    printf("\n=== PHASE 3 : SIMULATIONS DE COMBUSTION 0-D ===\n");

    // (a) Scenario type ITER : Q attendu de l'ordre de 10
    //     Parametres publies : n ~ 1.0e20 m^-3, tau_E ~ 3.7 s, V ~ 830 m^3,
    //     P_aux = 50 MW. La degradation du confinement IPB98(y,2)
    //     (tau_E ~ P^-0.69) stabilise le point de fonctionnement pilote
    //     (sans elle, le 0-D est thermiquement instable — physique POPCON).
    {
        printf("  --- Scenario (a) : type ITER, P_aux = 50 MW, IPB98(y,2) ---\n");
        fusion_dt_config_t cfg = {
            .n_e_m3 = 1.0e20, .T_keV = 8.5, .tau_E_s = 3.7, .volume_m3 = 830.0,
            .p_aux_W = 50e6, .z_eff = 1.6, .dt_s = 1e-3,
            .max_steps = 400000, .log_every = 1000,
            .tau_scaling_exponent = 0.69, .p_ref_W = 100e6
        };
        fusion_dt_plasma_t* plasma = fusion_dt_plasma_create(&cfg);
        TEST_ASSERT(plasma != NULL, "Creation moteur plasma ITER-like");
        if (plasma) {
            fusion_dt_burn_result_t r = fusion_dt_plasma_run_burn(plasma);
            printf("    %s\n", r.message);
            printf("    Triple produit : %.3e keV.s/m^3 | P_net_elec estimee : %.1f MW\n",
                   r.triple_product_keV_s_m3, r.p_net_electric_est_MW);
            printf("    Hash final etat (bit-level) : 0x%016llX\n",
                   (unsigned long long)r.state_hash_final);
            TEST_ASSERT(r.success, "Simulation ITER-like executee");
            TEST_ASSERT(r.q_factor_final > 5.0, "Q > 5 (ordre de grandeur ITER Q=10)");
            TEST_ASSERT(r.T_final_keV > 7.0 && r.T_final_keV < 15.0,
                        "T stationnaire dans [7,15] keV (ITER: ~9 keV)");
            TEST_ASSERT(!r.thermal_runaway_detected, "Pas d'excursion thermique");
            fusion_dt_plasma_destroy(&plasma);
            TEST_ASSERT(plasma == NULL, "Destruction securisee moteur (a)");
        }
    }

    // (b) Scenario reacteur en ignition : confinement renforce, P_aux coupee
    //     apres l'allumage n'est pas modelisee ici — on demarre chaud (12 keV)
    //     avec P_aux faible et tau_E genereux : le plasma doit s'auto-entretenir.
    {
        printf("  --- Scenario (b) : reacteur en ignition (tau_E = 6 s) ---\n");
        fusion_dt_config_t cfg = {
            .n_e_m3 = 1.5e20, .T_keV = 12.0, .tau_E_s = 6.0, .volume_m3 = 1000.0,
            .p_aux_W = 10e6, .z_eff = 1.4, .dt_s = 1e-3,
            .max_steps = 200000, .log_every = 1000
        };
        fusion_dt_plasma_t* plasma = fusion_dt_plasma_create(&cfg);
        TEST_ASSERT(plasma != NULL, "Creation moteur plasma ignition");
        if (plasma) {
            fusion_dt_burn_result_t r = fusion_dt_plasma_run_burn(plasma);
            printf("    %s\n", r.message);
            printf("    Triple produit : %.3e keV.s/m^3 | P_net_elec estimee : %.1f MW\n",
                   r.triple_product_keV_s_m3, r.p_net_electric_est_MW);
            TEST_ASSERT(r.success, "Simulation ignition executee");
            TEST_ASSERT(r.ignited || r.thermal_runaway_detected,
                        "Auto-entretien atteint (ignition ou excursion vers T>100 keV)");
            fusion_dt_plasma_destroy(&plasma);
        }
    }

    // (c) Scenario sous-critique : confinement insuffisant, le plasma doit
    //     s'effondrer vers l'equilibre chauffage externe / pertes (Q << 1).
    {
        printf("  --- Scenario (c) : sous-critique (tau_E = 0.1 s) ---\n");
        fusion_dt_config_t cfg = {
            .n_e_m3 = 5.0e19, .T_keV = 5.0, .tau_E_s = 0.1, .volume_m3 = 100.0,
            .p_aux_W = 20e6, .z_eff = 2.0, .dt_s = 1e-4,
            .max_steps = 100000, .log_every = 1000
        };
        fusion_dt_plasma_t* plasma = fusion_dt_plasma_create(&cfg);
        TEST_ASSERT(plasma != NULL, "Creation moteur plasma sous-critique");
        if (plasma) {
            fusion_dt_burn_result_t r = fusion_dt_plasma_run_burn(plasma);
            printf("    %s\n", r.message);
            TEST_ASSERT(r.success, "Simulation sous-critique executee");
            TEST_ASSERT(!r.ignited, "Pas d'ignition (attendu pour ce confinement)");
            TEST_ASSERT(r.q_factor_final < 1.0, "Q < 1 (attendu)");
            fusion_dt_plasma_destroy(&plasma);
        }
    }
}

// ---------------------------------------------------------------------------
// PHASE 4 — Projection LUM/VORAX et conservation
// ---------------------------------------------------------------------------
static void phase4_lum_vorax_bridge(void) {
    printf("\n=== PHASE 4 : PROJECTION LUM/VORAX + CONSERVATION ===\n");

    fusion_dt_config_t cfg = {
        .n_e_m3 = 1.0e20, .T_keV = 10.0, .tau_E_s = 3.7, .volume_m3 = 830.0,
        .p_aux_W = 50e6, .z_eff = 1.6, .dt_s = 1e-3,
        .max_steps = 1000, .log_every = 100
    };
    fusion_dt_plasma_t* plasma = fusion_dt_plasma_create(&cfg);
    TEST_ASSERT(plasma != NULL, "Creation moteur plasma pour projection LUM");
    if (!plasma) return;

    bool ok = fusion_dt_plasma_sync_lums(plasma, 10000);
    TEST_ASSERT(ok, "Conservation VORAX verifiee (10000 LUMs D + 10000 LUMs T)");
    if (plasma->lum_group_deuterium && plasma->lum_group_tritium && plasma->lum_group_alpha) {
        size_t nd = lum_group_size(plasma->lum_group_deuterium);
        size_t nt = lum_group_size(plasma->lum_group_tritium);
        size_t na = lum_group_size(plasma->lum_group_alpha);
        printf("  LUMs D=%zu, T=%zu, groupe fusionne=%zu, %.3e ions/LUM\n",
               nd, nt, na, plasma->ions_per_lum);
        TEST_ASSERT(na == nd + nt, "vorax_fuse conserve les unites de presence");
    }

    fusion_dt_plasma_destroy(&plasma);
    TEST_ASSERT(plasma == NULL, "Destruction securisee apres projection");
}

// ---------------------------------------------------------------------------
// PHASE 5 — Tracabilite bit-level : sensibilite du hash a 1 bit
// ---------------------------------------------------------------------------
static void phase5_bit_level_traceability(void) {
    printf("\n=== PHASE 5 : TRACABILITE BIT-LEVEL (SENSIBILITE 1 BIT) ===\n");

    fusion_dt_state_t state;
    memset(&state, 0, sizeof(state));
    state.T_keV = 10.0;
    state.W_J_m3 = 4.8e5;
    state.step_index = 42;

    uint64_t hash_original = fusion_dt_state_hash(&state);

    // Perturbation d'exactement UN bit (bit 0 du premier octet de T_keV)
    fusion_dt_state_t state_flipped = state;
    uint8_t* raw = (uint8_t*)&state_flipped.T_keV;
    raw[0] ^= 0x01;
    uint64_t hash_flipped = fusion_dt_state_hash(&state_flipped);

    printf("  Hash original  : 0x%016llX\n", (unsigned long long)hash_original);
    printf("  Hash apres flip d'1 bit : 0x%016llX\n", (unsigned long long)hash_flipped);
    TEST_ASSERT(hash_original != hash_flipped,
                "Un seul bit modifie => hash different (tracabilite bit-level)");

    // Determinisme : le meme etat produit toujours le meme hash
    TEST_ASSERT(fusion_dt_state_hash(&state) == hash_original,
                "Hash deterministe (rejouabilite forensique)");
}

// ---------------------------------------------------------------------------
// PHASE 6 — Cendres d'helium : la dilution borne l'excursion thermique
// Reprend le scenario ignition de la phase 3(b) qui, SANS cendres, part en
// excursion T > 100 keV. AVEC la dynamique des cendres (dn_He/dt = R - n_He/
// tau_He), l'accumulation d'helium dilue le combustible et doit borner T.
// ---------------------------------------------------------------------------
static void phase6_helium_ash_dynamics(void) {
    printf("\n=== PHASE 6 : CENDRES D'HELIUM (BURN-UP + DILUTION) ===\n");
    // NOTE PHYSIQUE (probleme rencontre et documente) : les cendres SEULES ne
    // bornent pas une excursion : l'emballement se developpe en ~5 s alors que
    // tau_He = 5*tau_E = 30 s. Dans un tokamak reel, c'est la degradation du
    // confinement IPB98 (instantanee avec P) qui freine d'abord, puis les
    // cendres qui diluent a long terme. On isole donc l'effet cendres en
    // comparant (IPB98 seul) vs (IPB98 + cendres) sur le meme scenario.

    fusion_dt_config_t base = {
        .n_e_m3 = 1.5e20, .T_keV = 12.0, .tau_E_s = 6.0, .volume_m3 = 1000.0,
        .p_aux_W = 10e6, .z_eff = 1.0, .dt_s = 1e-3,
        .max_steps = 400000, .log_every = 10000,
        .tau_scaling_exponent = 0.69, .p_ref_W = 100e6
    };

    // Run A : IPB98 seul (reference, composition fixe)
    fusion_dt_burn_result_t ra;
    {
        fusion_dt_plasma_t* pa = fusion_dt_plasma_create(&base);
        TEST_ASSERT(pa != NULL, "Creation moteur run A (IPB98 seul)");
        if (!pa) return;
        ra = fusion_dt_plasma_run_burn(pa);
        printf("    Run A (IPB98 seul)    : %s\n", ra.message);
        fusion_dt_plasma_destroy(&pa);
    }

    // Run B : IPB98 + cendres d'helium
    fusion_dt_config_t cfg_b = base;
    cfg_b.enable_ash_dynamics = true;
    cfg_b.tau_he_ratio = 5.0;
    fusion_dt_plasma_t* pb = fusion_dt_plasma_create(&cfg_b);
    TEST_ASSERT(pb != NULL, "Creation moteur run B (IPB98 + cendres)");
    if (!pb) return;
    fusion_dt_burn_result_t rb = fusion_dt_plasma_run_burn(pb);
    double he_frac = pb->state.n_helium_m3 / cfg_b.n_e_m3;
    double fuel_frac = pb->state.n_fuel_m3 / cfg_b.n_e_m3;
    printf("    Run B (IPB98 + cendres): %s\n", rb.message);
    printf("    Fraction helium : %.2f%% | combustible restant : %.2f%% | Z_eff dynamique : %.3f\n",
           he_frac * 100.0, fuel_frac * 100.0, pb->state.z_eff_dynamic);

    TEST_ASSERT(rb.success, "Simulation avec cendres executee");
    TEST_ASSERT(!rb.thermal_runaway_detected && rb.T_final_keV < 100.0,
                "Regime borne (IPB98 + dilution), pas d'excursion");
    TEST_ASSERT(he_frac > 0.001, "Accumulation d'helium mesurable (> 0.1%)");
    TEST_ASSERT(pb->state.z_eff_dynamic > 1.0,
                "Z_eff dynamique > 1 (pollution helium prise en compte)");
    TEST_ASSERT(rb.p_fusion_final_MW < ra.p_fusion_final_MW,
                "La dilution helium REDUIT la puissance fusion (effet physique attendu)");

    fusion_dt_plasma_destroy(&pb);
    TEST_ASSERT(pb == NULL, "Destruction securisee moteur cendres");
}

// ---------------------------------------------------------------------------
// PHASE 7 — Optimisation du point de fonctionnement (recherche de solution)
// Balayage 2D (densite x confinement) avec la physique complete (cendres +
// IPB98) : la "solution" est le design maximisant la production electrique
// nette. C'est le livrable energetique du module.
// ---------------------------------------------------------------------------
static void phase7_operating_point_optimization(void) {
    printf("\n=== PHASE 7 : OPTIMISATION DU POINT DE FONCTIONNEMENT ===\n");

    fusion_dt_design_result_t* opt = fusion_dt_optimize_operating_point(
        0.6e20, 1.6e20, 6,    // densite : 6 points
        2.0, 7.0, 6,          // confinement : 6 points
        830.0, 50e6);         // volume ITER-like, P_aux = 50 MW

    TEST_ASSERT(opt != NULL && opt->success, "Optimisation executee (36 designs)");
    if (!opt) return;

    printf("    %s\n", opt->message);
    printf("    MEILLEUR DESIGN TROUVE :\n");
    printf("      n_e = %.3e m^-3 | tau_E = %.2f s | T = %.2f keV\n",
           opt->best.n_e_m3, opt->best.tau_E_s, opt->best.T_final_keV);
    printf("      Q = %.2f | P_fusion = %.1f MW | P_net electrique = %.1f MW\n",
           opt->best.q_factor, opt->best.p_fusion_MW, opt->best.p_net_electric_MW);
    printf("      Fraction helium a l'equilibre = %.2f%%\n",
           opt->best.helium_fraction * 100.0);

    TEST_ASSERT(opt->points_viable > 0, "Au moins un design viable");
    TEST_ASSERT(opt->best.p_net_electric_MW > 0.0,
                "SOLUTION ENERGETIQUE : production electrique nette POSITIVE");
    TEST_ASSERT(opt->best.q_factor > 1.0, "Q > 1 au point optimal");
    TEST_ASSERT(opt->best.helium_fraction > 0.0 && opt->best.helium_fraction < 0.5,
                "Fraction helium physiquement plausible");

    // Export CSV de la grille complete pour audit externe
    mkdir("logs", 0755);
    mkdir("logs/fusion", 0755);
    FILE* csv = fopen("logs/fusion/design_scan.csv", "w");
    if (csv) {
        fprintf(csv, "n_e_m3,tau_E_s,T_final_keV,q_factor,p_fusion_MW,p_net_MW,he_fraction,viable\n");
        for (size_t i = 0; i < opt->points_evaluated; i++) {
            const fusion_dt_design_point_t* p = &opt->points[i];
            fprintf(csv, "%.4e,%.3f,%.3f,%.3f,%.2f,%.2f,%.5f,%d\n",
                    p->n_e_m3, p->tau_E_s, p->T_final_keV, p->q_factor,
                    p->p_fusion_MW, p->p_net_electric_MW, p->helium_fraction,
                    p->viable ? 1 : 0);
        }
        fclose(csv);
        printf("    CSV exporte : logs/fusion/design_scan.csv (%zu designs)\n",
               opt->points_evaluated);
    }
    TEST_ASSERT(csv != NULL, "Export CSV de la grille de designs");

    fusion_dt_design_result_destroy(&opt);
    TEST_ASSERT(opt == NULL, "Destruction securisee resultat optimisation");
}

// ---------------------------------------------------------------------------
// PHASE 8 — Snapshot bit-level natif (lum_memory_tracer, 1 LUM = 1 bit)
// L'etat plasma est encode au format .lum (magic LUMT), reconstruit, et la
// preuve diff=0 est etablie par les validateurs natifs du projet.
// ---------------------------------------------------------------------------
static void phase8_native_bit_level_snapshot(void) {
    printf("\n=== PHASE 8 : SNAPSHOT BIT-LEVEL NATIF (.lum, DIFF=0) ===\n");

    fusion_dt_config_t cfg = {
        .n_e_m3 = 1.0e20, .T_keV = 9.0, .tau_E_s = 3.7, .volume_m3 = 830.0,
        .p_aux_W = 50e6, .z_eff = 1.0, .dt_s = 1e-3,
        .max_steps = 500, .log_every = 100,
        .tau_scaling_exponent = 0.69, .p_ref_W = 100e6,
        .enable_ash_dynamics = true, .tau_he_ratio = 5.0
    };
    fusion_dt_plasma_t* plasma = fusion_dt_plasma_create(&cfg);
    TEST_ASSERT(plasma != NULL, "Creation moteur plasma pour snapshot");
    if (!plasma) return;

    for (int i = 0; i < 500; i++) fusion_dt_plasma_step(plasma);

    mkdir("logs", 0755);
    mkdir("logs/fusion", 0755);
    bool ok = fusion_dt_snapshot_bit_level(plasma, "logs/fusion/plasma_state_bit.lum");
    TEST_ASSERT(ok, "Snapshot .lum bit-level + reconstruction + PREUVE DIFF=0");

    // Coherence : le nombre attendu de LUMs est sizeof(etat) * 8 bits
    printf("    Etat plasma : %zu octets -> %zu LUMs attendus (1 bit/LUM)\n",
           sizeof(fusion_dt_state_t), sizeof(fusion_dt_state_t) * 8);

    fusion_dt_plasma_destroy(&plasma);
    TEST_ASSERT(plasma == NULL, "Destruction securisee apres snapshot");
}

// ---------------------------------------------------------------------------
// Machine ITER publiee, construite manuellement (pas via machine_derive)
// pour valider les formules geometriques et les contraintes une a une.
// Valeurs publiees : R=6.2 m, a=2.0 m, kappa=1.7, delta=0.33, B0=5.3 T,
// I_p=15 MA, n=1.0e20 m^-3, V~830 m^3, S~680 m^2, q95=3.0, f_GW~0.85,
// beta_N~1.8, P_LH~70-90 MW, tau_E~3.7 s (H98=1), charge murale ~0.6 MW/m2.
// ---------------------------------------------------------------------------
static fusion_dt_machine_t build_iter_machine(void) {
    fusion_dt_machine_t m;
    memset(&m, 0, sizeof(m));
    m.R_m = 6.2; m.a_m = 2.0; m.kappa = 1.7; m.delta = 0.33;
    m.B0_T = 5.3; m.I_p_MA = 15.0;
    m.volume_m3 = 2.0 * M_PI * M_PI * m.R_m * m.a_m * m.a_m * m.kappa;
    m.surface_m2 = 4.0 * M_PI * M_PI * m.R_m * m.a_m
                   * sqrt((1.0 + m.kappa * m.kappa) / 2.0);
    m.n_gw_m3 = m.I_p_MA / (M_PI * m.a_m * m.a_m) * 1e20;
    m.q95 = fusion_dt_q95_uckan(m.R_m, m.a_m, m.kappa, m.delta, m.B0_T, m.I_p_MA);
    m.b_coil_T = m.B0_T * m.R_m / (m.R_m - m.a_m - 1.4);
    return m;
}

// ---------------------------------------------------------------------------
// PHASE 9 — Validation geometrie + contraintes + IPB98(y,2) sur ITER publie
// ---------------------------------------------------------------------------
static void phase9_iter_published_validation(void) {
    printf("\n=== PHASE 9 : VALIDATION MACHINE ITER (VALEURS PUBLIEES) ===\n");

    fusion_dt_machine_t m = build_iter_machine();
    printf("    V=%.0f m^3 (publie ~830) | S=%.0f m^2 (publie ~680)\n",
           m.volume_m3, m.surface_m2);
    printf("    q95=%.2f (publie 3.0) | n_GW=%.3e (f_GW=%.2f, publie ~0.85)\n",
           m.q95, m.n_gw_m3, 1.0e20 / m.n_gw_m3);
    printf("    B_coil=%.1f T (publie 11.8) | P_LH=%.0f MW (publie 70-90)\n",
           m.b_coil_T, fusion_dt_p_lh_martin(1.0e20, m.B0_T, m.surface_m2));

    TEST_ASSERT(m.volume_m3 > 790.0 && m.volume_m3 < 870.0,
                "Volume plasma ITER dans [790,870] m^3 (publie ~830)");
    TEST_ASSERT(m.surface_m2 > 640.0 && m.surface_m2 < 720.0,
                "Surface plasma ITER dans [640,720] m^2 (publie ~680)");
    TEST_ASSERT(m.q95 > 2.85 && m.q95 < 3.15,
                "q95 Uckan dans [2.85,3.15] (regle de conception ITER: 3.0)");
    double fgw = 1.0e20 / m.n_gw_m3;
    TEST_ASSERT(fgw > 0.80 && fgw < 0.88,
                "Fraction de Greenwald dans [0.80,0.88] (publie ~0.85)");
    TEST_ASSERT(m.b_coil_T > 11.0 && m.b_coil_T < 12.5,
                "Champ au conducteur dans [11,12.5] T (publie 11.8)");
    double p_lh = fusion_dt_p_lh_martin(1.0e20, m.B0_T, m.surface_m2);
    TEST_ASSERT(p_lh > 70.0 && p_lh < 100.0,
                "Seuil L-H Martin dans [70,100] MW (publie 70-90)");

    // tau_E IPB98(y,2) complet a P_loss = 95 MW (regime nominal publie 3.7 s)
    fusion_dt_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.n_e_m3 = 1.0e20; cfg.T_keV = 8.9; cfg.volume_m3 = m.volume_m3;
    cfg.p_aux_W = 95e6; cfg.z_eff = 1.6; cfg.dt_s = 1e-3; cfg.max_steps = 1;
    cfg.log_every = 1;
    cfg.use_ipb98_full = true;
    cfg.ipb98_I_MA = m.I_p_MA; cfg.ipb98_B_T = m.B0_T; cfg.ipb98_R_m = m.R_m;
    cfg.ipb98_epsilon = m.a_m / m.R_m; cfg.ipb98_kappa = m.kappa;
    cfg.ipb98_M_amu = 2.5; cfg.ipb98_h98 = 1.0;
    fusion_dt_plasma_t* p = fusion_dt_plasma_create(&cfg);
    TEST_ASSERT(p != NULL, "Creation plasma ITER mode IPB98 complet");
    if (p) {
        printf("    tau_E IPB98(y,2) a P=95 MW : %.2f s (publie 3.7 a H98=1)\n",
               p->state.tau_E_eff_s);
        TEST_ASSERT(p->state.tau_E_eff_s > 2.7 && p->state.tau_E_eff_s < 4.2,
                    "tau_E IPB98 complet dans [2.7,4.2] s (publie ~3.7)");
        fusion_dt_plasma_destroy(&p);
    }
}

// ---------------------------------------------------------------------------
// PHASE 10 — Burn ITER auto-coherent : confinement PREDIT, plus impose
// ---------------------------------------------------------------------------
static void phase10_iter_self_consistent_burn(void) {
    printf("\n=== PHASE 10 : BURN ITER AUTO-COHERENT (IPB98 COMPLET + CENDRES) ===\n");

    fusion_dt_machine_t m = build_iter_machine();
    fusion_dt_material_catalog_t cat = fusion_dt_catalog_lts_iter();
    fusion_dt_reactor_point_t pt;
    bool ok = fusion_dt_reactor_evaluate(&cat, &m, 1.0e20 / m.n_gw_m3, 50e6, &pt);
    TEST_ASSERT(ok, "Evaluation reacteur ITER executee");
    if (!ok) return;

    printf("    T=%.2f keV | Q=%.2f | P_fus=%.0f MW | tau_E predit=%.2f s | He=%.2f%%\n",
           pt.T_final_keV, pt.q_factor, pt.p_fusion_MW, pt.tau_E_s,
           pt.he_fraction * 100.0);
    printf("    Contraintes: fGW=%.2f%s betaN=%.2f%s q95=%.2f%s mur=%.2f MW/m2%s "
           "P_LH=%.0f MW%s Bcoil=%.1f T%s\n",
           pt.f_greenwald, pt.c_greenwald ? "(OK)" : "(VIOLE)",
           pt.beta_n, pt.c_beta ? "(OK)" : "(VIOLE)",
           pt.machine.q95, pt.c_q95 ? "(OK)" : "(VIOLE)",
           pt.wall_load_MW_m2, pt.c_wall ? "(OK)" : "(VIOLE)",
           pt.p_lh_MW, pt.c_lh ? "(OK)" : "(VIOLE)",
           pt.machine.b_coil_T, pt.c_bcoil ? "(OK)" : "(VIOLE)");

    TEST_ASSERT(pt.T_final_keV > 6.0 && pt.T_final_keV < 13.0,
                "T equilibre ITER dans [6,13] keV (publie ~8.9)");
    TEST_ASSERT(pt.q_factor > 4.0 && pt.q_factor < 25.0,
                "Q ITER auto-coherent dans [4,25] (objectif publie 10)");
    TEST_ASSERT(pt.wall_load_MW_m2 > 0.35 && pt.wall_load_MW_m2 < 1.05,
                "Charge murale ITER dans [0.35,1.05] MW/m2 (publie ~0.6)");
    TEST_ASSERT(pt.beta_n > 1.2 && pt.beta_n < 2.6,
                "beta_N ITER dans [1.2,2.6] (publie ~1.8)");
    TEST_ASSERT(pt.viable, "ITER satisfait TOUTES les contraintes (design valide)");
}

// ---------------------------------------------------------------------------
// PHASE 11 — SOLUTION : optimisation contrainte pour 3 catalogues materiaux
// ---------------------------------------------------------------------------
static void phase11_constrained_material_optimization(void) {
    printf("\n=== PHASE 11 : OPTIMISATION CONTRAINTE — 3 CATALOGUES MATERIAUX ===\n");

    mkdir("logs", 0755);
    mkdir("logs/fusion", 0755);
    FILE* csv = fopen("logs/fusion/reactor_designs.csv", "w");
    if (csv) {
        fprintf(csv, "catalog,R_m,a_m,B0_T,Ip_MA,n_e_m3,fgw,p_aux_MW,T_keV,q_factor,"
                     "p_fusion_MW,p_net_MW,tau_E_s,beta_n,wall_MW_m2,p_lh_MW,"
                     "he_frac,viable\n");
    }

    fusion_dt_material_catalog_t catalogs[3];
    catalogs[0] = fusion_dt_catalog_lts_iter();
    catalogs[1] = fusion_dt_catalog_hts_rebco();
    catalogs[2] = fusion_dt_catalog_future_hypothetical();
    double R_min[3] = {5.0, 2.2, 2.0};
    double R_max[3] = {9.5, 6.0, 6.0};

    fusion_dt_reactor_point_t best[3];
    memset(best, 0, sizeof(best));

    for (int c = 0; c < 3; c++) {
        // fgw descend a 0.25 : les machines a tres haut champ (catalogue
        // futur) ont une densite de Greenwald si elevee que meme 40% de
        // celle-ci viole la limite de charge murale.
        fusion_dt_reactor_result_t* res = fusion_dt_reactor_optimize(
            &catalogs[c], R_min[c], R_max[c], 8, 0.25, 0.90, 6,
            3.1, 1.8, 0.4, 50e6);
        char label[160];
        snprintf(label, sizeof(label), "Optimisation [%s] executee avec designs viables",
                 catalogs[c].name);
        TEST_ASSERT(res != NULL && res->success, label);
        if (!res) continue;

        printf("    %s\n", res->message);
        best[c] = res->best;

        if (csv) {
            for (size_t i = 0; i < res->points_evaluated; i++) {
                const fusion_dt_reactor_point_t* p = &res->points[i];
                fprintf(csv, "%s,%.2f,%.2f,%.2f,%.1f,%.3e,%.2f,%.0f,%.2f,%.2f,"
                             "%.1f,%.1f,%.2f,%.2f,%.2f,%.1f,%.4f,%d\n",
                        catalogs[c].name, p->machine.R_m, p->machine.a_m,
                        p->machine.B0_T, p->machine.I_p_MA, p->n_e_m3,
                        p->f_greenwald, p->p_aux_W / 1e6, p->T_final_keV,
                        p->q_factor, p->p_fusion_MW, p->p_net_MW, p->tau_E_s,
                        p->beta_n, p->wall_load_MW_m2, p->p_lh_MW,
                        p->he_fraction, p->viable ? 1 : 0);
            }
        }

        snprintf(label, sizeof(label), "[%s] P_net > 0 (production nette reelle)",
                 catalogs[c].name);
        TEST_ASSERT(res->best.p_net_MW > 0.0, label);
        fusion_dt_reactor_result_destroy(&res);
    }
    if (csv) {
        fclose(csv);
        printf("    CSV exporte : logs/fusion/reactor_designs.csv\n");
    }
    TEST_ASSERT(csv != NULL, "Export CSV des designs reacteurs");

    // Les materiaux HTS permettent un reacteur NET-POSITIF plus compact que LTS
    // (c'est la these de SPARC/ARC, ici retrouvee par le calcul contraint)
    if (best[0].viable && best[1].viable) {
        printf("    Compacite: LTS R=%.2f m vs HTS R=%.2f m (ARC publie: 3.3 m)\n",
               best[0].machine.R_m, best[1].machine.R_m);
        TEST_ASSERT(best[1].machine.R_m < best[0].machine.R_m,
                    "HTS REBCO permet un reacteur net-positif plus compact que LTS");
    }
    // Le catalogue hypothetique quantifie le gain si la R&D materiaux aboutit
    if (best[1].viable && best[2].viable) {
        printf("    Gain materiaux futurs: P_net %.0f MW (HTS) -> %.0f MW (hypothetique)\n",
               best[1].p_net_MW, best[2].p_net_MW);
        TEST_ASSERT(best[2].p_net_MW >= best[1].p_net_MW,
                    "Les cibles materiaux futures augmentent P_net (exigences chiffrees)");
    }
}

// ---------------------------------------------------------------------------
int main(void) {
    // Sequence d'initialisation standard LUM/VORAX (cf. test_forensic_complete_system.c)
    memory_tracker_init();
    forensic_logger_init("logs/forensic/fusion_dt_plasma_execution.log");
    ultra_forensic_logger_init();

    printf("========================================================\n");
    printf("  LUM/VORAX — MODULE FUSION NUCLEAIRE D-T (PLASMA 0-D)\n");
    printf("  Tracabilite forensique nanoseconde + bit-level\n");
    printf("========================================================\n");

    uint64_t t_start_ns = lum_get_timestamp();

    phase1_validation_bosch_hale();
    phase2_lawson_scan();
    phase3_burn_scenarios();
    phase4_lum_vorax_bridge();
    phase5_bit_level_traceability();
    phase6_helium_ash_dynamics();
    phase7_operating_point_optimization();
    phase8_native_bit_level_snapshot();
    phase9_iter_published_validation();
    phase10_iter_self_consistent_burn();
    phase11_constrained_material_optimization();

    uint64_t t_end_ns = lum_get_timestamp();

    printf("\n========================================================\n");
    printf("  BILAN : %d PASS / %d FAIL — duree totale %llu ns\n",
           g_tests_passed, g_tests_failed,
           (unsigned long long)(t_end_ns - t_start_ns));
    printf("========================================================\n");

    // Vidage du ring buffer nanoseconde vers CSV + rapport de synthese
    ultra_forensic_flush_nano_ring();
    ultra_forensic_generate_summary_report();
    ultra_forensic_logger_destroy();
    forensic_logger_destroy();
    memory_tracker_report();
    memory_tracker_destroy();

    return (g_tests_failed == 0) ? 0 : 1;
}
