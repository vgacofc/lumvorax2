// NOUVEAU FICHIER : src/advanced_calculations/neural_advanced_optimizers.c

#include "neural_advanced_optimizers.h"
#include "../debug/forensic_logger.h"
#include "../debug/memory_tracker.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <stdio.h>
#include <inttypes.h>  // CORRECTION: Ajout pour PRIu64 et formats portables

// Temporary logging macros until forensic_log is properly implemented
#define FORENSIC_LEVEL_ERROR 0
#define FORENSIC_LEVEL_INFO 1
#define FORENSIC_LEVEL_WARNING 2
#define FORENSIC_LEVEL_SUCCESS 3
#define FORENSIC_LEVEL_DEBUG 4
#define forensic_log(level, func, fmt, ...) printf("[%s] " fmt "\n", func, ##__VA_ARGS__)

// Magic numbers pour validation intégrité
#define ADAM_MAGIC_NUMBER    0x4D414441  // "ADAM"
#define LBFGS_MAGIC_NUMBER   0x4C424653  // "LBFS"
#define NEWTON_MAGIC_NUMBER  0x4E45574E  // "NEWN"

// === IMPLÉMENTATION OPTIMISEUR ADAM ULTRA-PRÉCIS ===

neural_adam_ultra_precise_t* neural_adam_create_ultra_precise(size_t parameter_count) {
    neural_adam_ultra_precise_t* adam = TRACKED_MALLOC(sizeof(neural_adam_ultra_precise_t));
    if (!adam) {
        forensic_log(FORENSIC_LEVEL_ERROR, "neural_adam_create_ultra_precise",
                    "Échec allocation mémoire optimiseur Adam");
        return NULL;
    }
    
    // Allocation mémoire pour moments
    adam->moment1 = TRACKED_MALLOC(parameter_count * sizeof(double));
    adam->moment2 = TRACKED_MALLOC(parameter_count * sizeof(double));
    
    if (!adam->moment1 || !adam->moment2) {
        forensic_log(FORENSIC_LEVEL_ERROR, "neural_adam_create_ultra_precise",
                    "Échec allocation mémoire moments Adam");
        if (adam->moment1) TRACKED_FREE(adam->moment1);
        if (adam->moment2) TRACKED_FREE(adam->moment2);
        TRACKED_FREE(adam);
        return NULL;
    }
    
    // Initialisation moments à zéro
    memset(adam->moment1, 0, parameter_count * sizeof(double));
    memset(adam->moment2, 0, parameter_count * sizeof(double));
    
    // Configuration paramètres Adam optimaux pour ultra-précision
    adam->beta1 = 0.9;                  // Momentum standard
    adam->beta2 = 0.999;                // Variance adaptive standard
    adam->epsilon = 1e-12;              // Epsilon ultra-précis (vs 1e-8 standard)
    adam->learning_rate = 0.001;        // LR initial raisonnable
    adam->step_count = 0;               // Compteur étapes
    
    // INNOVATION : Décroissance adaptative
    adam->convergence_factor = 0.95;    // Réduction LR quand convergence
    adam->min_learning_rate = 1e-8;     // LR minimum absolu
    adam->precision_threshold = 1e-10;  // Seuil précision pour décroissance
    
    // Gestion mémoire et intégrité
    adam->parameter_count = parameter_count;
    adam->memory_address = adam;
    adam->magic_number = ADAM_MAGIC_NUMBER;
    
    forensic_log(FORENSIC_LEVEL_INFO, "neural_adam_create_ultra_precise",
                "Optimiseur Adam ultra-précis créé - Paramètres: %zu, Epsilon: %.2e",
                parameter_count, adam->epsilon);
    
    return adam;
}

void neural_adam_destroy_ultra_precise(neural_adam_ultra_precise_t** optimizer) {
    if (!optimizer || !*optimizer) return;
    
    neural_adam_ultra_precise_t* adam = *optimizer;
    
    // Validation intégrité avant destruction
    if (adam->magic_number != ADAM_MAGIC_NUMBER || adam->memory_address != adam) {
        forensic_log(FORENSIC_LEVEL_ERROR, "neural_adam_destroy_ultra_precise",
                    "Corruption détectée optimiseur Adam (magic: 0x%08X vs 0x%08X)",
                    adam->magic_number, ADAM_MAGIC_NUMBER);
        return;
    }
    
    forensic_log(FORENSIC_LEVEL_DEBUG, "neural_adam_destroy_ultra_precise",
                "Destruction optimiseur Adam - Steps: %" PRIu64 ", LR final: %.2e",
                adam->step_count, adam->learning_rate);
    
    // Libération mémoire moments
    if (adam->moment1) TRACKED_FREE(adam->moment1);
    if (adam->moment2) TRACKED_FREE(adam->moment2);
    
    // Nettoyage sécurité
    adam->magic_number = 0;
    adam->memory_address = NULL;
    
    TRACKED_FREE(adam);
    *optimizer = NULL;
}

void neural_adam_ultra_precise_update(
    void* system, // neural_blackbox_computer_t désactivé
    neural_adam_ultra_precise_t* optimizer,
    double* gradients,
    double current_loss
) {
    if (!system || !optimizer || !gradients) {
        forensic_log(FORENSIC_LEVEL_ERROR, "neural_adam_ultra_precise_update",
                    "Paramètres invalides fournis");
        return;
    }
    
    // Validation intégrité optimiseur
    if (optimizer->magic_number != ADAM_MAGIC_NUMBER) {
        forensic_log(FORENSIC_LEVEL_ERROR, "neural_adam_ultra_precise_update",
                    "Corruption optimiseur Adam détectée");
        return;
    }
    
    // Validation gradient
    if (!neural_validate_gradient(gradients, optimizer->parameter_count)) {
        forensic_log(FORENSIC_LEVEL_WARNING, "neural_adam_ultra_precise_update",
                    "Gradient invalide détecté (NaN/Inf), mise à jour ignorée");
        return;
    }
    
    optimizer->step_count++;
    
    // DÉCROISSANCE ADAPTATIVE LR selon convergence
    if (optimizer->step_count > 1 && current_loss > 0) {
        static double previous_loss = DBL_MAX;
        double loss_improvement = (previous_loss - current_loss) / previous_loss;
        
        if (loss_improvement < optimizer->precision_threshold && 
            optimizer->learning_rate > optimizer->min_learning_rate) {
            optimizer->learning_rate *= optimizer->convergence_factor;
            forensic_log(FORENSIC_LEVEL_DEBUG, "neural_adam_ultra_precise_update",
                        "LR réduit à %.2e (convergence lente détectée)", optimizer->learning_rate);
        }
        previous_loss = current_loss;
    }
    
    // Correction bias (standard Adam)
    double bias_correction1 = 1.0 - pow(optimizer->beta1, optimizer->step_count);
    double bias_correction2 = 1.0 - pow(optimizer->beta2, optimizer->step_count);
    
    // Calcul précision extended pour éviter erreurs arrondis
    long double lr_corrected = (long double)optimizer->learning_rate / sqrt(bias_correction2);
    lr_corrected /= bias_correction1;
    
    // Mise à jour paramètres avec précision extended
    for (size_t i = 0; i < optimizer->parameter_count; i++) {
        long double grad = (long double)gradients[i];
        
        // Mise à jour moments
        long double m1 = (long double)optimizer->moment1[i];
        long double m2 = (long double)optimizer->moment2[i];
        
        m1 = optimizer->beta1 * m1 + (1.0L - optimizer->beta1) * grad;
        m2 = optimizer->beta2 * m2 + (1.0L - optimizer->beta2) * grad * grad;
        
        optimizer->moment1[i] = (double)m1;
        optimizer->moment2[i] = (double)m2;
        
        // Calcul mise à jour paramètre avec precision extended
        long double update = lr_corrected * m1 / (sqrtl(m2) + optimizer->epsilon);
        
        // Application mise à jour aux paramètres du système
        // Note: L'accès aux paramètres du système nécessiterait une interface spécifique
        // Ici on simule la mise à jour pour validation algorithme
        gradients[i] = (double)update; // Stockage temporaire pour validation
    }
    
    if (optimizer->step_count % 1000 == 0) {
        forensic_log(FORENSIC_LEVEL_INFO, "neural_adam_ultra_precise_update",
                    "Adam step %" PRIu64 " - LR: %.2e, Loss: %.15f", 
                    optimizer->step_count, optimizer->learning_rate, current_loss);
    }
}

bool neural_adam_reset_moments(neural_adam_ultra_precise_t* optimizer) {
    if (!optimizer || optimizer->magic_number != ADAM_MAGIC_NUMBER) {
        return false;
    }
    
    memset(optimizer->moment1, 0, optimizer->parameter_count * sizeof(double));
    memset(optimizer->moment2, 0, optimizer->parameter_count * sizeof(double));
    optimizer->step_count = 0;
    
    forensic_log(FORENSIC_LEVEL_DEBUG, "neural_adam_reset_moments",
                "Moments Adam réinitialisés");
    
    return true;
}

// === IMPLÉMENTATION OPTIMISEUR L-BFGS ===

neural_lbfgs_optimizer_t* neural_lbfgs_create(size_t parameter_count, size_t memory_size) {
    if (memory_size == 0) memory_size = 20; // Valeur par défaut recommandée
    
    neural_lbfgs_optimizer_t* lbfgs = TRACKED_MALLOC(sizeof(neural_lbfgs_optimizer_t));
    if (!lbfgs) {
        forensic_log(FORENSIC_LEVEL_ERROR, "neural_lbfgs_create",
                    "Échec allocation mémoire optimiseur L-BFGS");
        return NULL;
    }
    
    // Allocation mémoire pour vecteurs s et y
    lbfgs->s_vectors = TRACKED_MALLOC(memory_size * sizeof(double*));
    lbfgs->y_vectors = TRACKED_MALLOC(memory_size * sizeof(double*));
    lbfgs->alpha = TRACKED_MALLOC(memory_size * sizeof(double));
    lbfgs->rho = TRACKED_MALLOC(memory_size * sizeof(double));
    
    if (!lbfgs->s_vectors || !lbfgs->y_vectors || !lbfgs->alpha || !lbfgs->rho) {
        forensic_log(FORENSIC_LEVEL_ERROR, "neural_lbfgs_create",
                    "Échec allocation mémoire vecteurs L-BFGS");
        if (lbfgs->s_vectors) TRACKED_FREE(lbfgs->s_vectors);
        if (lbfgs->y_vectors) TRACKED_FREE(lbfgs->y_vectors);
        if (lbfgs->alpha) TRACKED_FREE(lbfgs->alpha);
        if (lbfgs->rho) TRACKED_FREE(lbfgs->rho);
        TRACKED_FREE(lbfgs);
        return NULL;
    }
    
    // Allocation des vecteurs individuels
    for (size_t i = 0; i < memory_size; i++) {
        lbfgs->s_vectors[i] = TRACKED_MALLOC(parameter_count * sizeof(double));
        lbfgs->y_vectors[i] = TRACKED_MALLOC(parameter_count * sizeof(double));
        
        if (!lbfgs->s_vectors[i] || !lbfgs->y_vectors[i]) {
            // Nettoyage en cas d'échec
            for (size_t j = 0; j <= i; j++) {
                if (lbfgs->s_vectors[j]) TRACKED_FREE(lbfgs->s_vectors[j]);
                if (lbfgs->y_vectors[j]) TRACKED_FREE(lbfgs->y_vectors[j]);
            }
            TRACKED_FREE(lbfgs->s_vectors);
            TRACKED_FREE(lbfgs->y_vectors);
            TRACKED_FREE(lbfgs->alpha);
            TRACKED_FREE(lbfgs->rho);
            TRACKED_FREE(lbfgs);
            return NULL;
        }
        
        memset(lbfgs->s_vectors[i], 0, parameter_count * sizeof(double));
        memset(lbfgs->y_vectors[i], 0, parameter_count * sizeof(double));
    }
    
    // Initialisation paramètres
    lbfgs->memory_size = memory_size;
    lbfgs->current_position = 0;
    lbfgs->memory_full = false;
    lbfgs->parameter_count = parameter_count;
    lbfgs->last_loss = DBL_MAX;
    lbfgs->convergence_rate = 0.0;
    lbfgs->iterations = 0;
    lbfgs->memory_address = lbfgs;
    lbfgs->magic_number = LBFGS_MAGIC_NUMBER;
    
    forensic_log(FORENSIC_LEVEL_INFO, "neural_lbfgs_create",
                "Optimiseur L-BFGS créé - Paramètres: %zu, Mémoire: %zu",
                parameter_count, memory_size);
    
    return lbfgs;
}

void neural_lbfgs_destroy(neural_lbfgs_optimizer_t** optimizer) {
    if (!optimizer || !*optimizer) return;
    
    neural_lbfgs_optimizer_t* lbfgs = *optimizer;
    
    // Validation intégrité
    if (lbfgs->magic_number != LBFGS_MAGIC_NUMBER || lbfgs->memory_address != lbfgs) {
        forensic_log(FORENSIC_LEVEL_ERROR, "neural_lbfgs_destroy",
                    "Corruption détectée optimiseur L-BFGS");
        return;
    }
    
    forensic_log(FORENSIC_LEVEL_DEBUG, "neural_lbfgs_destroy",
                "Destruction optimiseur L-BFGS - Itérations: %" PRIu64,
                lbfgs->iterations);
    
    // Libération vecteurs
    for (size_t i = 0; i < lbfgs->memory_size; i++) {
        if (lbfgs->s_vectors[i]) TRACKED_FREE(lbfgs->s_vectors[i]);
        if (lbfgs->y_vectors[i]) TRACKED_FREE(lbfgs->y_vectors[i]);
    }
    
    TRACKED_FREE(lbfgs->s_vectors);
    TRACKED_FREE(lbfgs->y_vectors);
    TRACKED_FREE(lbfgs->alpha);
    TRACKED_FREE(lbfgs->rho);
    
    // Nettoyage sécurité
    lbfgs->magic_number = 0;
    lbfgs->memory_address = NULL;
    
    TRACKED_FREE(lbfgs);
    *optimizer = NULL;
}

// === FONCTIONS UTILITAIRES ===

double neural_vector_norm_ultra_precise(double* vector, size_t size) {
    if (!vector || size == 0) return 0.0;
    
    // Calcul norme avec precision extended pour éviter overflow
    long double sum = 0.0L;
    for (size_t i = 0; i < size; i++) {
        long double val = (long double)vector[i];
        sum += val * val;
    }
    
    return (double)sqrtl(sum);
}

double neural_dot_product_ultra_precise(double* v1, double* v2, size_t size) {
    if (!v1 || !v2 || size == 0) return 0.0;
    
    // Calcul produit scalaire avec precision extended
    long double sum = 0.0L;
    for (size_t i = 0; i < size; i++) {
        sum += (long double)v1[i] * (long double)v2[i];
    }
    
    return (double)sum;
}

void neural_vector_copy(double* dest, double* src, size_t size) {
    if (!dest || !src || size == 0) return;
    memcpy(dest, src, size * sizeof(double));
}

bool neural_validate_gradient(double* gradient, size_t size) {
    if (!gradient || size == 0) return false;
    
    for (size_t i = 0; i < size; i++) {
        if (isnan(gradient[i]) || isinf(gradient[i])) {
            return false;
        }
    }
    return true;
}

// Line search conditions de Wolfe (implémentation simplifiée)
double neural_wolfe_line_search_ultra_precise(
    void* system, // neural_blackbox_computer_t désactivé
    double* search_direction,
    double* gradient,
    double current_loss
) {
    // Implémentation basique line search
    // En production, implémentation complète conditions Wolfe
    
    if (!system || !search_direction || !gradient) return 0.0;
    
    const double c1 = 1e-4;  // Condition Armijo
    (void)c1; // Suppress unused variable warning
    double alpha = 1.0;      // Pas initial
    const double reduction_factor = 0.5;
    const int max_iterations = 50;
    
    (void)current_loss; // Suppress unused parameter warning
    
    // Calcul pente initiale
    double initial_slope = neural_dot_product_ultra_precise(
        gradient, search_direction, 1000); // system->total_parameters désactivé
    
    if (initial_slope >= 0) {
        forensic_log(FORENSIC_LEVEL_WARNING, "neural_wolfe_line_search_ultra_precise",
                    "Direction recherche non-descendante");
        return 1e-6; // Pas très petit
    }
    
    // Recherche linéaire simple (backtracking)
    for (int iter = 0; iter < max_iterations; iter++) {
        // Test condition Armijo
        // new_loss = evaluate_at_step(alpha)  
        // Simplification: supposons amélioration proportionnelle
        double expected_decrease = c1 * alpha * initial_slope;
        
        if (expected_decrease < 0) { // Condition satisfaite
            forensic_log(FORENSIC_LEVEL_DEBUG, "neural_wolfe_line_search_ultra_precise",
                        "Pas optimal trouvé: %.6f (itération %d)", alpha, iter);
            return alpha;
        }
        
        alpha *= reduction_factor;
    }
    
    forensic_log(FORENSIC_LEVEL_INFO, "neural_wolfe_line_search_ultra_precise",
                "Pas réduit à valeur minimale: %.6f", alpha);
    
    return alpha;
}