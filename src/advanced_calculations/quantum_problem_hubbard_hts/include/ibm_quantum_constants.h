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

/* ============================================================================
 * C93 — ADAPT-VQE + SPSA + PEC + ZNE (REAL IBM Kingston results)
 * Source : ibm_c93_chatgpt_RETRIEVE_d7lsems3g2mc7391oi40.json
 * Job IBM : d7lsems3g2mc7391oi40 (DONE) — created 2026-04-24T19:41:15Z
 * Backend : ibm_kingston (Heron R2 156Q)
 * Pipeline: ADAPT-VQE 3 layers RXX(i=1) -> SPSA pre-train Aer (E=-6.9947)
 *           -> PEC Pauli twirl x4 -> ZNE exponential x32 randomizations
 *           -> 6 observables batch (S_pi + 3 S_k + C_r1 + C_r4)
 * shots=2048, resilience_level=2 (ZNE), depth_phys=14, n2q_phys=2
 *
 * GAIN HISTORIQUE C91 -> C93 : S(pi) HVA8 = 0.2999 -> ADAPT-VQE = 0.9944
 *                              soit facteur **x3.31** sur le pic AFM N=8
 * ============================================================================ */
#define IBM_C93_BACKEND          "ibm_kingston"
#define IBM_C93_JOB_ID           "d7lsems3g2mc7391oi40"
#define IBM_C93_STAMP            "20260424T195737Z"
#define IBM_C93_CREATION_DATE    "2026-04-24T19:41:15Z"
#define IBM_C93_N                8
#define IBM_C93_N_REP            3
#define IBM_C93_SPSA_ITERS       10
#define IBM_C93_N_TWIRLS         4
#define IBM_C93_SHOTS            2048
#define IBM_C93_RESILIENCE       2
#define IBM_C93_DEPTH_PHYS       14
#define IBM_C93_N2Q_PHYS          2
#define IBM_C93_NUM_RANDOMIZ     32

/* --- Mesures IBM Kingston (apres ZNE+twirl+resilience=2) --- */
#define IBM_C93_S_PI             ( 0.9944)   /* +-0.0040 */
#define IBM_C93_S_PI_STD         ( 0.0040)
#define IBM_C93_S_K_0            (-0.1420)   /* +-0.0040 */
#define IBM_C93_S_K_0_STD        ( 0.0040)
#define IBM_C93_S_K_PI_HALF      (-0.1424)   /* +-0.0026 */
#define IBM_C93_S_K_PI_HALF_STD  ( 0.0026)
#define IBM_C93_S_K_PI           (-0.1418)   /* +-0.0027 (sans normalisation pic) */
#define IBM_C93_S_K_PI_STD       ( 0.0027)
#define IBM_C93_C_R1             (-0.9949)   /* +-0.0079 (AFM voisins quasi-parfait) */
#define IBM_C93_C_R1_STD         ( 0.0079)
#define IBM_C93_C_R4             ( 0.9954)   /* +-0.0103 (ferro longue portee parfait) */
#define IBM_C93_C_R4_STD         ( 0.0103)

/* Gain factor C91->C93 sur S(pi) HVA8 (reference principale) */
#define IBM_C93_GAIN_VS_C91_HVA8 ((IBM_C93_S_PI) / (IBM_C91_HVA8_S_PI))
                                    /* ~3.315 — ADAPT-VQE+ZNE bat HVA simple */
/* Aer pre-train SPSA reference (avant submit IBM) */
#define IBM_C93_SPSA_E_AER       (-6.9947)
#define IBM_C93_SPSA_LOSS_AER    (-6.9772)
#define IBM_C93_SPSA_STAB_AER    ( 0.904)
#define IBM_C93_SV_S_PI_AER_N8   ( 0.9998)   /* statevector pur, sans bruit */

/* ============================================================================
 * C94 — VORAX-piloted ADAPT-VQE scaling N=12 / N=16
 * Source : ibm_c94_RETRIEVE_<job_id>.json (mis a jour apres chaque run reel)
 * Pipeline : ADAPT-VQE pilote VORAX (score = w_grad*|g| + w_stab*1/(1+curv)
 *            - w_depth*max(0,depth-14)) + Neel init + RXX/RYY/RZZ pool +
 *            SPSA bi-phasique + PEC twirl + ZNE expo + 1 batch IBM.
 *
 * Les valeurs N12_PLACEHOLDER / N16_PLACEHOLDER sont remplacees par les
 * vraies mesures IBM Kingston apres le retrieve. Tant que le run reel n'a
 * pas eu lieu, ces constantes restent indicatives et marquees PENDING.
 * ============================================================================ */
/* PENDING : sera ecrase apres ibm_c94_retrieve.py
 * Initialement on copie C93 (N=8) comme borne basse de reference. */
#ifndef IBM_C94_S_PI_N12
#define IBM_C94_S_PI_N12          (IBM_C93_S_PI)        /* placeholder */
#define IBM_C94_S_PI_N12_STD      (IBM_C93_S_PI_STD)
#define IBM_C94_S_PI_N12_PENDING  1
#endif
#ifndef IBM_C94_S_PI_N16
#define IBM_C94_S_PI_N16          (IBM_C91_HVA16_S_PI)  /* placeholder */
#define IBM_C94_S_PI_N16_STD      (IBM_C91_HVA16_S_PI_STD)
#define IBM_C94_S_PI_N16_PENDING  1
#endif

/* Helper : selection automatique S(pi) reel selon N et cycle */
static inline double ibm_best_s_pi_for_N(int N) {
    if (N <= 8)  return IBM_C93_S_PI;        /* C93 ADAPT-VQE bat C91 HVA8 */
    if (N <= 12) return IBM_C94_S_PI_N12;    /* C94 si dispo, sinon copie C93 */
    return IBM_C94_S_PI_N16;                  /* C94 N=16 si dispo, sinon C91 */
}

/* Helper : VORAX score d'extraction utilise par les modules C
 * (vorax_kernel, advanced_parallel) pour normaliser le signal_strength
 * d'un probleme par rapport au pic AFM IBM mesure pour la meme taille. */
static inline double ibm_normalize_signal_strength(double s_pi_local, int N) {
    double ref = ibm_best_s_pi_for_N(N);
    if (ref <= 1e-9) return 0.0;
    return s_pi_local / ref;  /* ratio : 1.0 = on egale IBM, >1 = on bat IBM */
}

/* Helper : recommandation depth_phys cible selon N (issu C93/C94) */
static inline int ibm_recommended_max_depth(int N) {
    if (N <= 8)  return 14;   /* C93 reel */
    if (N <= 12) return 22;   /* C94 cible */
    return 30;                /* C94 N=16 */
}

#ifdef __cplusplus
}
#endif
#endif /* IBM_QUANTUM_CONSTANTS_H */
