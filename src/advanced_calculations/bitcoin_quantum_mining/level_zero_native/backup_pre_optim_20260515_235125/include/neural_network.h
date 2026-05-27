/* neural_network.h — Réseau neuronal biologiquement-fondé multi-échelle.
 *
 * Cycle C95 — squelette d'API publique. Voir CAHIER_DES_CHARGES_C95_MAITRE.md §3.
 *
 * 5 niveaux de granularité :
 *   L0 — Atomique (MD AMBER ff14SB) : zones critiques uniquement (~10⁴ atomes)
 *   L1 — Moléculaire (canaux Markov, récepteurs)
 *   L2 — Compartimental (équation câble multi-segments)
 *   L3 — Cellulaire (Izhikevich spiking)
 *   L4 — Réseau STDP (plasticité Hebbienne)
 *
 * Ce header expose les structures et fonctions principales. Les implémentations
 * sont dans les fichiers .c séparés par niveau.
 */
#ifndef LUMVORAX_NEURAL_NETWORK_H
#define LUMVORAX_NEURAL_NETWORK_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * L1 — Canaux ioniques (Hodgkin-Huxley étendu, modèle de Markov)
 * ============================================================================ */
typedef enum {
    ION_CHANNEL_NA_V15 = 0,    /* Na+ voltage-gated (canal Na_v 1.5) */
    ION_CHANNEL_K_V11,         /* K+  voltage-gated (Kv1.1) */
    ION_CHANNEL_K_LEAK,        /* K+  leak */
    ION_CHANNEL_CA_L,          /* Ca++ L-type */
    ION_CHANNEL_CL_GABA,       /* Cl- GABA-gated */
    ION_CHANNEL_COUNT
} ion_channel_type_t;

typedef struct {
    ion_channel_type_t type;
    double g_max;          /* conductance max (S/cm²) */
    double e_rev;          /* potentiel d'inversion (mV) */
    double m, h, n;        /* gating variables HH (sans dim) */
    double tau_m, tau_h;   /* constantes de temps (ms) */
} ion_channel_t;

/* Met à jour gating variables en dt ms à voltage V (mV). */
void ion_channel_step(ion_channel_t *ch, double V_mV, double dt_ms);

/* Courant (µA/cm²) traversant le canal au voltage V. */
double ion_channel_current(const ion_channel_t *ch, double V_mV);

/* ============================================================================
 * L1 — Récepteurs synaptiques (AMPA / NMDA / GABA-A)
 * ============================================================================ */
typedef enum {
    RECEPTOR_AMPA = 0,
    RECEPTOR_NMDA,
    RECEPTOR_GABA_A,
    RECEPTOR_COUNT
} receptor_type_t;

typedef struct {
    receptor_type_t type;
    double g_peak;        /* conductance pic (nS) */
    double tau_rise;      /* ms */
    double tau_decay;     /* ms */
    double e_rev_mV;
    double s;             /* fraction ouverte (0..1) */
} receptor_t;

void receptor_spike(receptor_t *r);                 /* presynaptic spike → s += */
void receptor_step(receptor_t *r, double dt_ms);    /* décroissance */
double receptor_current(const receptor_t *r, double V_post_mV);

/* ============================================================================
 * L3 — Neurone Izhikevich (spiking phénoménologique)
 *   dv/dt = 0.04 v² + 5 v + 140 - u + I
 *   du/dt = a (b v - u)
 *   v >= 30 → v = c, u += d
 * ============================================================================ */
typedef struct {
    double v;     /* membrane potential (mV) */
    double u;     /* recovery */
    double a, b, c, d;   /* paramètres (RS=régulier, FS=rapide, etc.) */
    int spiked;
} izh_neuron_t;

void izh_neuron_init(izh_neuron_t *n, double a, double b, double c, double d);
void izh_neuron_step(izh_neuron_t *n, double I_input, double dt_ms);

/* Presets standards */
void izh_preset_regular_spiking(izh_neuron_t *n);  /* RS : a=0.02 b=0.2 c=-65 d=8 */
void izh_preset_fast_spiking(izh_neuron_t *n);     /* FS : a=0.1  b=0.2 c=-65 d=2 */
void izh_preset_chattering(izh_neuron_t *n);       /* CH : a=0.02 b=0.2 c=-50 d=2 */

/* ============================================================================
 * L4 — Réseau STDP (Spike-Timing Dependent Plasticity)
 * ============================================================================ */
typedef struct {
    int n_neurons;
    int n_synapses;
    izh_neuron_t *neurons;
    int *src;         /* indices source des synapses */
    int *dst;         /* indices destination */
    double *weight;   /* poids synaptiques */
    double *trace_pre, *trace_post;  /* traces STDP */
    double tau_pre, tau_post;        /* ms */
    double A_plus, A_minus;          /* amplitudes potentiation/depression */
    double w_min, w_max;
} neural_network_t;

neural_network_t *nn_create(int n_neurons, int n_synapses);
void nn_destroy(neural_network_t *nn);
void nn_connect(neural_network_t *nn, int src, int dst, double w0);
void nn_step(neural_network_t *nn, const double *I_ext, double dt_ms);
void nn_apply_stdp(neural_network_t *nn);

/* ============================================================================
 * Sérialisation .lum (intégration LUM DBMS)
 * ============================================================================ */
int nn_serialize_lum(const neural_network_t *nn, const char *path);
neural_network_t *nn_deserialize_lum(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* LUMVORAX_NEURAL_NETWORK_H */
