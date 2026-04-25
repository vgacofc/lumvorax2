/* nx48_neuro_coupler.h — COUPLAGE (≠ substitution) NX48 ↔ Izhikevich+STDP.
 *
 * Cycle C98 — Conformité STANDARD_NAMES.md §M-BTC18-C98 + prompt.txt v2.1.
 *
 * OBJECTIF UTILISATEUR (transcription verbatim, majuscules conservées) :
 *   « NE PAS SUBSTITUER le NX48 → Izhikevich+STDP, ME LE COUPLER AUX
 *     → Izhikevich+STDP. »
 *
 * ARCHITECTURE :
 *   NX48 reste le contrôleur principal autonome (8 features × 8 sous-neurones).
 *   Ce module ajoute un CO-PROCESSEUR spiking (Izhikevich + STDP) qui :
 *     1. Reçoit les NX48_BTC_N_FEATURES (8) features NX48 comme courants pA
 *     2. Excite 8 neurones Izhikevich en regular spiking
 *     3. STDP apprend les corrélations temporelles inter-features
 *     4. Renvoie un signal modulant scalaire ∈ [-1, +1] que NX48 utilise
 *        pour ajuster exploration_bias et delta_nonce_scale (modulation,
 *        PAS remplacement de la décision NX48)
 *
 * Ce design préserve à 100% la chaîne forensique NX48 et ajoute une couche
 * d'apprentissage Hebbien biologique sans toucher au pipeline mining.
 */
#ifndef LUMVORAX_NX48_NEURO_COUPLER_H
#define LUMVORAX_NX48_NEURO_COUPLER_H

#include <stdint.h>
#include "neural_network.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NX48_COUPLER_N_NEURONS    8     /* 1 neurone par feature NX48 BTC */
#define NX48_COUPLER_N_SYNAPSES   56    /* full-mesh sans self-loop = 8*7 */
#define NX48_COUPLER_DT_MS        0.5   /* pas Euler stable Izhikevich */
#define NX48_COUPLER_WINDOW_MS    50.0  /* fenêtre intégration spikes pour rate */

/* ── C99 calibration (correction critique C98) ──────────────────────────────
 * C98 : gain=30 pA → ~63 Hz observé → SUR-STIMULATION (cible RS = 20-40 Hz).
 * C99 : gain=15 pA → ~30 Hz attendu = milieu du régime RS optimal Izhikevich. */
#define NX48_COUPLER_I_GAIN_PA    15.0  /* 0..1 feature → 0..15 pA (C99 calibré) */
#define NX48_COUPLER_REWARD_PA    25.0  /* récompense réduite proportionnellement */

/* C99 : historique pour seuil near-miss DYNAMIQUE (percentile_90 glissant) */
#define NX48_COUPLER_LZ_HIST_SIZE 100   /* 100 derniers leading_zeros observés */

typedef struct {
    neural_network_t *net;          /* Réseau Izhikevich + STDP (alloué) */
    uint64_t          step_count;   /* Nb appels nx48_coupler_step() */
    uint64_t          spike_count;  /* Total spikes émis depuis init */
    double            mean_rate_hz; /* Taux moyen tous neurones, fenêtre récente */
    double            last_modulation; /* Dernier signal renvoyé ∈ [-1,+1] */
    int               coupler_active;  /* 1 = ON, 0 = OFF (A/B testing) */
    char              run_id[64];      /* identifiant run pour logs */

    /* ── C99 : seuil near-miss dynamique (percentile_90 glissant) ─────── */
    int               lz_history[NX48_COUPLER_LZ_HIST_SIZE];
    int               lz_history_count;       /* combien remplis */
    int               lz_history_idx;         /* position circulaire */
    int               near_miss_threshold;    /* p90 calculé à chaque update */
    uint64_t          near_miss_total;        /* compteur événements récompense */

    /* ── C99 : matrice 8x8 de décorrélation features (Q1 critique) ─── */
    double            decorr_W[NX48_COUPLER_N_NEURONS][NX48_COUPLER_N_NEURONS];

    /* ── C99 : poids EWMA pour stats poids synaptiques ──────────────── */
    double            w_spread_ema;   /* w_max - w_min EWMA */
    double            w_mean_ema;     /* moyenne EWMA */
} nx48_coupler_t;

/* ─── API publique ────────────────────────────────────────────────── */

/* Crée le coupleur (alloue net + connecte 56 synapses excitatrices uniformes).
 * w_init typique = 0.1 (faible mais non nul, STDP fera évoluer). */
nx48_coupler_t *nx48_coupler_create(const char *run_id, double w_init);

/* Libère toutes les ressources. */
void nx48_coupler_destroy(nx48_coupler_t *c);

/* Active/désactive le coupleur (pour A/B testing : ON vs OFF).
 * Quand OFF, nx48_coupler_step() retourne 0.0 sans calculer. */
void nx48_coupler_set_active(nx48_coupler_t *c, int active);

/* Pas de simulation : prend les 8 features NX48 (∈[0,1]) + un drapeau
 * near_miss_event (1 si un near-miss BTC vient d'arriver, sinon 0).
 * Retourne le signal de modulation scalaire ∈ [-1, +1] :
 *   = (mean_rate_hz - 30.0) / 30.0   borné dans [-1,+1]   (C99 : 30 Hz = neutre)
 *
 * Convention C99 : 30 Hz = neutre (milieu régime RS sain 20-40 Hz).
 * < 30 Hz → exploration insuffisante (mod < 0 → exploration↑)
 * > 30 Hz → suractivation (mod > 0 → exploitation↑)
 *
 * NX48 reste libre d'utiliser ou ignorer ce signal. */
double nx48_coupler_step(nx48_coupler_t *c,
                         const double features[NX48_COUPLER_N_NEURONS],
                         int near_miss_event);

/* C99 : version automatique qui calcule near_miss à partir du percentile_90
 * dynamique de l'historique LZ. À utiliser plutôt que nx48_coupler_step()
 * pour bénéficier du seuil adaptatif (recommandation expert). */
double nx48_coupler_step_auto(nx48_coupler_t *c,
                              const double features[NX48_COUPLER_N_NEURONS],
                              int current_leading_zeros);

/* Sérialise l'état (poids STDP, traces, compteurs) pour audit forensique. */
int nx48_coupler_serialize_jsonl(const nx48_coupler_t *c, const char *path);

/* C99 : log step JSONL ultra-léger (1 ligne par appel, pour analyse temps réel).
 * Format :
 *   {"step":N,"rate_hz":...,"mod":...,"w_spread":...,"near_miss":0|1,"lz":N}
 * À appeler à chaque pas si log_path != NULL. */
int nx48_coupler_log_step_jsonl(const nx48_coupler_t *c, const char *log_path,
                                int near_miss_event, int leading_zeros);

#ifdef __cplusplus
}
#endif

#endif /* LUMVORAX_NX48_NEURO_COUPLER_H */
