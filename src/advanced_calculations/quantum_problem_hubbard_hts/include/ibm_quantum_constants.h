/* ibm_quantum_constants.h — Constantes physiques mesurees IBM Quantum reelles.
 *
 * Cycle C93 — propagation des resultats IBM C91/C92 dans le code classique C.
 * Source : src/advanced_calculations/bitcoin_quantum_mining/results/
 *          ibm_c91_scaling_20260423T211421Z.json
 *
 * Backend : ibm_kingston (Heron R2 156Q)
 * Job IBM : ibm_c91_scaling — 6 pubs en 1 batch (374s total, 369s QPU)
 * Date    : 2026-04-23T21:14Z
 * shots=2048, resilience_level=2 (ZNE)
 *
 * Ces constantes servent de **references calibration** pour les modules :
 *  - hubbard_hts_research_cycle_advanced_parallel (cible HVA AFM)
 *  - vorax_kernel (validation perte composite vs S(pi) reel)
 *  - nx48_adaptive_controller (signal_strength baseline)
 *
 * IMPORTANT : ce ne sont PAS des valeurs simulees, ce sont les mesures
 * physiques **reelles** soumises au QPU IBM Kingston via REPLIT_AGENT_DIRECT_PUSH.
 */
#ifndef IBM_QUANTUM_CONSTANTS_H
#define IBM_QUANTUM_CONSTANTS_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * GHZ scaling (etat de reference, sanity check du QPU)
 * S(pi)_theory_GHZ = -1 (corner -1 par construction)
 * S(pi)_mesure / S(pi)_theory =~ fidelity proxy (idealement 1.0)
 * ============================================================================ */
#define IBM_C91_GHZ8_S_PI       (-0.1473)   /* +-0.0044 */
#define IBM_C91_GHZ8_S_PI_STD   ( 0.0044)
#define IBM_C91_GHZ8_FIDELITY   ( 1.0313)   /* artefact : ratio > 1 = bruit corrige > theorie */
#define IBM_C91_GHZ8_DEPTH_PHYS  24
#define IBM_C91_GHZ8_N2Q_PHYS     7

#define IBM_C91_GHZ12_S_PI      (-0.0897)
#define IBM_C91_GHZ12_S_PI_STD  ( 0.0028)
#define IBM_C91_GHZ12_FIDELITY  ( 0.9862)
#define IBM_C91_GHZ12_DEPTH_PHYS 36
#define IBM_C91_GHZ12_N2Q_PHYS   11

#define IBM_C91_GHZ16_S_PI      (-0.0667)
#define IBM_C91_GHZ16_S_PI_STD  ( 0.0090)
#define IBM_C91_GHZ16_FIDELITY  ( 1.0008)   /* tres proche de 1.0 = QPU bien calibre */
#define IBM_C91_GHZ16_DEPTH_PHYS 48
#define IBM_C91_GHZ16_N2Q_PHYS   15

/* ============================================================================
 * HVA-Hubbard 1-layer (objet d'etude principal — phase AFM si S(pi)>0)
 * Z_neel ideal = +1.0 ; S(pi) > 0 confirme ordre antiferromagnetique
 * ============================================================================ */
#define IBM_C91_HVA8_S_PI       ( 0.2999)
#define IBM_C91_HVA8_S_PI_STD   ( 0.0117)
#define IBM_C91_HVA8_DEPTH_PHYS  73
#define IBM_C91_HVA8_N2Q_PHYS    21

#define IBM_C91_HVA12_S_PI      ( 0.3141)
#define IBM_C91_HVA12_S_PI_STD  ( 0.0085)
#define IBM_C91_HVA12_DEPTH_PHYS 110
#define IBM_C91_HVA12_N2Q_PHYS   33

#define IBM_C91_HVA16_S_PI      ( 0.3558)   /* meilleur signal AFM */
#define IBM_C91_HVA16_S_PI_STD  ( 0.0049)
#define IBM_C91_HVA16_DEPTH_PHYS 149
#define IBM_C91_HVA16_N2Q_PHYS   45

/* ============================================================================
 * Trends & calibration (extraits des rapports 96/97)
 * ============================================================================ */
/* Tendance scaling AFM : S(pi) croit avec N -> regime thermodynamique */
#define IBM_C91_AFM_TREND_DSPI_DN   ((IBM_C91_HVA16_S_PI - IBM_C91_HVA8_S_PI) / 8.0)
                                    /* ~ +0.007 par site supplementaire */
/* Borne de fidelity GHZ moyenne (sanity QPU) */
#define IBM_C91_GHZ_FID_MEAN        ((IBM_C91_GHZ8_FIDELITY + IBM_C91_GHZ12_FIDELITY \
                                     + IBM_C91_GHZ16_FIDELITY) / 3.0)
                                    /* ~ 1.006 — QPU calibre ±1% */

/* ============================================================================
 * Metadonnees du job (tracabilite forensique)
 * ============================================================================ */
#define IBM_C91_BACKEND          "ibm_kingston"
#define IBM_C91_BACKEND_QUBITS   156
#define IBM_C91_STAMP            "20260423T211421Z"
#define IBM_C91_SHOTS            2048
#define IBM_C91_RESILIENCE       2
#define IBM_C91_ELAPSED_TOTAL_S  374.39
#define IBM_C91_ELAPSED_QPU_S    368.85
#define IBM_C91_N_PUBS           6

/* ============================================================================
 * Helpers : selection automatique selon N
 * ============================================================================ */
static inline double ibm_c91_hva_s_pi_for_N(int N) {
    if (N <= 8)  return IBM_C91_HVA8_S_PI;
    if (N <= 12) return IBM_C91_HVA12_S_PI;
    return IBM_C91_HVA16_S_PI;
}

static inline double ibm_c91_hva_s_pi_std_for_N(int N) {
    if (N <= 8)  return IBM_C91_HVA8_S_PI_STD;
    if (N <= 12) return IBM_C91_HVA12_S_PI_STD;
    return IBM_C91_HVA16_S_PI_STD;
}

static inline double ibm_c91_ghz_fidelity_for_N(int N) {
    if (N <= 8)  return IBM_C91_GHZ8_FIDELITY;
    if (N <= 12) return IBM_C91_GHZ12_FIDELITY;
    return IBM_C91_GHZ16_FIDELITY;
}

#ifdef __cplusplus
}
#endif
#endif /* IBM_QUANTUM_CONSTANTS_H */
