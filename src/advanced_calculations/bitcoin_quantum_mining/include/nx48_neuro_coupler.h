/* nx48_neuro_coupler.h — Couplage neural Izhikevich + STDP pour NX48 (C99 P0.1)
 *
 * Architecture 3 couches biologiquement fondée :
 *  Couche 1 : 8 neurones Izhikevich Regular Spiking (1 par feature NX48)
 *  Couche 2 : STDP full-mesh 56 synapses (Song-Miller-Abbott 2000)
 *  Couche 3 : Signal de modulation ∈ [-1, +1] (mean_rate normalisé)
 *
 * Le coupleur NE REMPLACE PAS NX48 : il produit un signal qui peut moduler
 * exploration_bias de ±15% maximum, préservant 100% des décisions NX48 existantes.
 *
 * Récompense dopaminergique : injection +50 pA si near_miss (≥ percentile 90 LZ).
 */
#ifndef NX48_NEURO_COUPLER_H
#define NX48_NEURO_COUPLER_H

#include <stddef.h>
#include <stdint.h>

#define NX48_COUPLER_N_NEURONS   8
#define NX48_COUPLER_N_SYNAPSES  (NX48_COUPLER_N_NEURONS * (NX48_COUPLER_N_NEURONS - 1)) /* 56 */
#define NX48_COUPLER_HISTORY     100   /* taille fenêtre percentile 90 LZ (Q1) */
#define NX48_COUPLER_DT_MS       1.0   /* pas de temps Izhikevich (ms) */
#define NX48_COUPLER_WINDOW_MS   50.0  /* fenêtre comptage spikes pour mean_rate */
#define NX48_COUPLER_REWARD_PA   50.0  /* injection +50 pA sur near-miss */
#define NX48_COUPLER_GAIN_PA     30.0  /* feature × 30 pA (Izhikevich 2003 RS) */

typedef struct {
    /* État neurones Izhikevich : v=membrane (mV), u=récup (mV) */
    double v[NX48_COUPLER_N_NEURONS];
    double u[NX48_COUPLER_N_NEURONS];
    /* Spike count par neurone sur la fenêtre courante (reset à chaque WINDOW_MS) */
    uint32_t spike_count[NX48_COUPLER_N_NEURONS];
    /* Spike count cumulatif (jamais reset) */
    uint64_t spike_total[NX48_COUPLER_N_NEURONS];
    /* Dernier temps spike (ms) pour STDP (-1 = jamais) */
    double last_spike_ms[NX48_COUPLER_N_NEURONS];
    /* Poids synaptiques full-mesh w[i][j] (i!=j), bornés [0, 1] */
    double w[NX48_COUPLER_N_NEURONS][NX48_COUPLER_N_NEURONS];
    /* Histoire LZ pour seuil percentile 90 dynamique (Q1) */
    int    lz_history[NX48_COUPLER_HISTORY];
    int    lz_history_idx;
    int    lz_history_filled;
    /* Compteurs */
    double t_ms;
    uint64_t step_count;
    uint64_t reward_count;
    /* Identifiant run */
    char run_id[64];
    /* Sérialisation */
    char jsonl_path[512];
} nx48_coupler_t;

/* Crée un coupleur. log_freq = 0.1 → log JSONL toutes les 10% des steps. */
nx48_coupler_t* nx48_coupler_create(const char* run_id, double log_freq);
void            nx48_coupler_destroy(nx48_coupler_t* c);

/* Effectue 1 step de couplage :
 *   - features : 8 valeurs ∈ [0,1] (normalisées NX48)
 *   - near_miss : 1 si LZ courant ≥ percentile 90 historique
 * Retourne le signal de modulation ∈ [-1, +1].
 *   < 0 → encourage exploration
 *   > 0 → encourage exploitation
 */
double nx48_coupler_step(nx48_coupler_t* c,
                         const double features[NX48_COUPLER_N_NEURONS],
                         int near_miss);

/* Met à jour l'historique LZ et retourne 1 si lz_current ≥ percentile 90 (Q1). */
int nx48_coupler_check_near_miss(nx48_coupler_t* c, int lz_current);

/* Sérialise état JSONL (append) avec checksum FNV1a-64. */
int nx48_coupler_log_jsonl(const nx48_coupler_t* c);

/* Statistiques pour rapport */
double nx48_coupler_mean_rate_hz(const nx48_coupler_t* c);
double nx48_coupler_weight_spread(const nx48_coupler_t* c);

#endif /* NX48_NEURO_COUPLER_H */
