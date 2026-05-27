/* nx48_coupler_bridge.h — Pont d'intégration NX48↔Izhikevich+STDP (C99).
 *
 * Cycle C99 — branchement minimal et NON-INTRUSIF du coupleur dans le pipeline
 * NX48 BTC. Permet l'A/B testing sans casser la chaîne forensique existante.
 *
 * USAGE :
 *   1. Initialisation 1 fois par run :
 *        bridge = nx48_bridge_create(run_id, use_neural, log_path);
 *   2. À chaque update NX48 (typiquement après nx48_btc_compute_features) :
 *        double mod = nx48_bridge_modulate(bridge, features, current_lz);
 *        // mod ∈ [-1,+1] ; le caller décide d'appliquer ou non.
 *   3. Le bridge log automatiquement chaque appel en JSONL si log_path != NULL.
 *   4. Libération en fin de run :
 *        nx48_bridge_destroy(bridge);
 *
 * Le bridge encapsule TOUTE l'API du coupleur — nx48_btc_controller.c n'a
 * besoin que de cet header (pas de neural_network.h ni de nx48_neuro_coupler.h).
 */
#ifndef LUMVORAX_NX48_COUPLER_BRIDGE_H
#define LUMVORAX_NX48_COUPLER_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nx48_bridge_s nx48_bridge_t;

/* Crée le pont. use_neural=0 → pont fantôme (modulate retourne toujours 0).
 * log_path=NULL → pas de logging JSONL. */
nx48_bridge_t *nx48_bridge_create(const char *run_id,
                                   int use_neural,
                                   const char *log_path);

/* Libère le pont et flush le dernier état JSONL (audit). */
void nx48_bridge_destroy(nx48_bridge_t *b);

/* Modulation : prend les 8 features NX48 BTC (features[NX48_BTC_N_FEATURES])
 * + le current_leading_zeros (utilisé pour le seuil near-miss DYNAMIQUE p90).
 * Retourne signal ∈ [-1, +1] (0 si désactivé).
 *
 * Le caller (nx48_btc_update) doit appliquer la modulation par exemple :
 *   exploration_bias *= (1.0 + 0.30 * mod);  // ±30 % d'influence (C99 relevé)
 *   delta_nonce_scale *= (1.0 - 0.20 * mod); // exploration↑ quand mod<0
 */
double nx48_bridge_modulate(nx48_bridge_t *b,
                            const double features[8],
                            int current_leading_zeros);

/* Statistiques compactes pour intégration dans logs CSV NX48. */
double nx48_bridge_get_rate_hz(const nx48_bridge_t *b);
double nx48_bridge_get_w_spread(const nx48_bridge_t *b);
int    nx48_bridge_get_threshold(const nx48_bridge_t *b);
unsigned long long nx48_bridge_get_step_count(const nx48_bridge_t *b);
unsigned long long nx48_bridge_get_near_miss_total(const nx48_bridge_t *b);

#ifdef __cplusplus
}
#endif

#endif /* LUMVORAX_NX48_COUPLER_BRIDGE_H */
