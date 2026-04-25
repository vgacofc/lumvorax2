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
#define NX48_COUPLER_I_GAIN_PA    30.0  /* 0..1 feature → 0..30 pA stimulant */
#define NX48_COUPLER_REWARD_PA    50.0  /* injection sur near-miss BTC */

typedef struct {
    neural_network_t *net;          /* Réseau Izhikevich + STDP (alloué) */
    uint64_t          step_count;   /* Nb appels nx48_coupler_step() */
    uint64_t          spike_count;  /* Total spikes émis depuis init */
    double            mean_rate_hz; /* Taux moyen tous neurones, fenêtre récente */
    double            last_modulation; /* Dernier signal renvoyé ∈ [-1,+1] */
    int               coupler_active;  /* 1 = ON, 0 = OFF (A/B testing) */
    char              run_id[64];      /* identifiant run pour logs */
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
 *   = (mean_rate_hz - 20.0) / 20.0   borné dans [-1,+1]
 *
 * Convention : 20 Hz = neutre (régime regular spiking sain).
 * < 20 Hz → exploration insuffisante (injecter exploration_bias↑)
 * > 20 Hz → suractivation (calmer, exploitation↑)
 *
 * NX48 reste libre d'utiliser ou ignorer ce signal. */
double nx48_coupler_step(nx48_coupler_t *c,
                         const double features[NX48_COUPLER_N_NEURONS],
                         int near_miss_event);

/* Sérialise l'état (poids STDP, traces, compteurs) pour audit forensique. */
int nx48_coupler_serialize_jsonl(const nx48_coupler_t *c, const char *path);

#ifdef __cplusplus
}
#endif

#endif /* LUMVORAX_NX48_NEURO_COUPLER_H */
