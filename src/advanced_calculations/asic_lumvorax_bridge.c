/* asic_lumvorax_bridge.c — Pont ASIC ↔ VORAX (C99 Q4)
 *
 * Convertit le vecteur de sortie d'un ASIC simulé (16 dimensions ψ_eff)
 * vers le format VORAX et l'injecte dans la fonction de loss du minier
 * NX48-BTC. Le but : permettre à un futur ASIC dédié LUMVORAX de
 * remplacer la simulation logicielle NX48 par un signal hardware.
 *
 * Format ASIC :
 *   asic_vector_t {
 *     double psi_real[16];   // partie réelle ψ
 *     double psi_imag[16];   // partie imaginaire ψ
 *     double energy_meV;     // énergie de calcul mesurée
 *     double temperature_K;  // température die
 *     uint64_t timestamp_ns; // horodatage hardware
 *   }
 *
 * Format VORAX :
 *   vorax_loss_input_t {
 *     double features[8];         // features dérivées du vecteur ASIC
 *     double weight;              // poids dans la loss globale
 *     double regularization_l2;   // régularisation L2 sur ψ
 *   }
 *
 * Loss VORAX = α·||features||² + β·energy + γ·||ψ||₂²
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <time.h>

#define ASIC_PSI_DIM      16
#define ASIC_FEATURE_DIM   8

typedef struct {
    double   psi_real[ASIC_PSI_DIM];
    double   psi_imag[ASIC_PSI_DIM];
    double   energy_meV;
    double   temperature_K;
    uint64_t timestamp_ns;
} asic_vector_t;

typedef struct {
    double features[ASIC_FEATURE_DIM];
    double weight;
    double regularization_l2;
} vorax_loss_input_t;

/* Convertit un vecteur ASIC vers une entrée de loss VORAX.
 *   features[0..7] = (|ψ_0|², |ψ_1|², ..., |ψ_7|²) repliés sur 16→8 par moyennage par paire.
 *   poids = 1.0 / (1.0 + temp_K / 300.0)  -- les ASIC chauds sont moins fiables
 *   reg_l2 = ||ψ||₂² classique
 */
int asic_to_vorax(const asic_vector_t* asic, vorax_loss_input_t* vorax) {
    if (!asic || !vorax) return -1;
    /* Calcule |ψ_k|² = re² + im² pour les 16 amplitudes */
    double mag_sq[ASIC_PSI_DIM];
    double total_l2 = 0.0;
    for (int k = 0; k < ASIC_PSI_DIM; k++) {
        mag_sq[k] = asic->psi_real[k] * asic->psi_real[k] +
                    asic->psi_imag[k] * asic->psi_imag[k];
        total_l2 += mag_sq[k];
    }
    /* Replie 16→8 par moyennage par paires (k, k+8) */
    for (int j = 0; j < ASIC_FEATURE_DIM; j++) {
        vorax->features[j] = (mag_sq[j] + mag_sq[j + ASIC_FEATURE_DIM]) * 0.5;
    }
    /* Poids dépendant de la température */
    double temp_norm = (asic->temperature_K > 0.0) ? asic->temperature_K / 300.0 : 1.0;
    vorax->weight = 1.0 / (1.0 + temp_norm);
    vorax->regularization_l2 = total_l2;
    return 0;
}

/* Calcule la loss VORAX globale à partir d'un batch d'entrées ASIC.
 * loss = α · Σ_b w_b · ||features_b||² + β · Σ_b w_b · energy_b + γ · Σ_b reg_l2_b
 */
double vorax_compute_loss_batch(const vorax_loss_input_t* batch,
                                const double* energies,
                                int batch_size,
                                double alpha, double beta, double gamma) {
    if (!batch || batch_size <= 0) return 0.0;
    double sum_features = 0.0, sum_energy = 0.0, sum_reg = 0.0;
    for (int b = 0; b < batch_size; b++) {
        double f_norm_sq = 0.0;
        for (int j = 0; j < ASIC_FEATURE_DIM; j++) {
            f_norm_sq += batch[b].features[j] * batch[b].features[j];
        }
        sum_features += batch[b].weight * f_norm_sq;
        if (energies) sum_energy += batch[b].weight * energies[b];
        sum_reg += batch[b].regularization_l2;
    }
    return alpha * sum_features + beta * sum_energy + gamma * sum_reg;
}

#ifdef ASIC_BRIDGE_STANDALONE
int main(void) {
    /* Test : 4 vecteurs ASIC simulés */
    asic_vector_t batch_asic[4];
    vorax_loss_input_t batch_vorax[4];
    double energies[4];

    srand(42);
    printf("=== Test ASIC → VORAX bridge (C99 Q4) ===\n");
    for (int b = 0; b < 4; b++) {
        for (int k = 0; k < ASIC_PSI_DIM; k++) {
            batch_asic[b].psi_real[k] = ((double)rand() / RAND_MAX) - 0.5;
            batch_asic[b].psi_imag[k] = ((double)rand() / RAND_MAX) - 0.5;
        }
        /* Normalise ψ */
        double norm = 0;
        for (int k = 0; k < ASIC_PSI_DIM; k++) {
            norm += batch_asic[b].psi_real[k] * batch_asic[b].psi_real[k] +
                    batch_asic[b].psi_imag[k] * batch_asic[b].psi_imag[k];
        }
        norm = sqrt(norm);
        for (int k = 0; k < ASIC_PSI_DIM; k++) {
            batch_asic[b].psi_real[k] /= norm;
            batch_asic[b].psi_imag[k] /= norm;
        }
        batch_asic[b].energy_meV = -1.5 - (double)b * 0.1;
        batch_asic[b].temperature_K = 273.0 + (double)b * 10.0;
        batch_asic[b].timestamp_ns = (uint64_t)time(NULL) * 1000000000ULL + b;
        energies[b] = batch_asic[b].energy_meV;
        if (asic_to_vorax(&batch_asic[b], &batch_vorax[b]) != 0) {
            fprintf(stderr, "ERR conversion b=%d\n", b);
            return 1;
        }
        printf("  batch[%d] T=%.1fK E=%.3fmeV w=%.4f reg_l2=%.4f\n",
               b, batch_asic[b].temperature_K, batch_asic[b].energy_meV,
               batch_vorax[b].weight, batch_vorax[b].regularization_l2);
    }

    double loss = vorax_compute_loss_batch(batch_vorax, energies, 4,
                                           1.0,   /* alpha */
                                           0.5,   /* beta  */
                                           0.01); /* gamma */
    printf("\nLoss VORAX globale = %.6f (alpha=1.0, beta=0.5, gamma=0.01)\n", loss);
    if (isnan(loss) || isinf(loss)) {
        fprintf(stderr, "ERREUR: loss NaN/Inf\n");
        return 2;
    }
    printf("✓ ASIC↔VORAX bridge opérationnel\n");
    return 0;
}
#endif
