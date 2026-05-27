/**
 * @file lum_poh.c
 * @brief Implémentation Proof of History (PoH) - Horloge Cryptographique
 * 
 * Architecture haute performance :
 * - Thread dédié pour génération continue
 * - SHA-256 optimisé (OpenSSL)
 * - Buffer circulaire lock-free pour historique
 * - Précision nanoseconde
 * 
 * Performance cible : 1M ticks/sec (vs Solana ~400K)
 * 
 * @version 1.0.0
 * @date 2026-05-02
 */

#include "lum_poh.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

/* ============================================================================
 * FONCTIONS UTILITAIRES
 * ============================================================================ */

/**
 * @brief Obtenir timestamp nanoseconde (CLOCK_MONOTONIC_RAW)
 */
uint64_t lum_poh_get_timestamp_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/**
 * @brief Sleep précis en nanosecondes
 */
static void sleep_ns(uint64_t ns) {
    struct timespec req = {
        .tv_sec = ns / 1000000000ULL,
        .tv_nsec = ns % 1000000000ULL
    };
    nanosleep(&req, NULL);
}

/* ============================================================================
 * INITIALISATION / DESTRUCTION
 * ============================================================================ */

lum_poh_t* lum_poh_init(uint32_t target_tps, bool enable_recording) {
    lum_poh_t* poh = (lum_poh_t*)calloc(1, sizeof(lum_poh_t));
    if (!poh) {
        return NULL;
    }
    
    // Configuration
    poh->target_ticks_per_second = target_tps > 0 ? target_tps : LUM_POH_TICKS_PER_SECOND;
    poh->enable_recording = enable_recording;
    
    // Initialiser hash genesis (SHA-256 de "LumVorax PoH Genesis")
    const char* genesis = "LumVorax PoH Genesis";
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, genesis, strlen(genesis));
    SHA256_Final(poh->current_hash, &ctx);
    
    // État initial
    poh->sequence = 0;
    poh->slot = 0;
    poh->tick_in_slot = 0;
    poh->total_ticks = 0;
    poh->start_time_ns = lum_poh_get_timestamp_ns();
    poh->ticks_per_second = 0.0;
    poh->running = false;
    
    // Buffer circulaire pour historique
    if (enable_recording) {
        poh->entries_capacity = LUM_POH_MAX_ENTRIES;
        poh->entries = (lum_poh_entry_t*)calloc(poh->entries_capacity, sizeof(lum_poh_entry_t));
        if (!poh->entries) {
            free(poh);
            return NULL;
        }
        poh->entries_head = 0;
        poh->entries_tail = 0;
    } else {
        poh->entries = NULL;
        poh->entries_capacity = 0;
    }
    
    // Mutex
    if (pthread_mutex_init(&poh->mutex, NULL) != 0) {
        free(poh->entries);
        free(poh);
        return NULL;
    }
    
    return poh;
}

void lum_poh_free(lum_poh_t* poh) {
    if (!poh) return;
    
    // Arrêter thread si actif
    if (poh->running) {
        lum_poh_stop(poh);
    }
    
    pthread_mutex_destroy(&poh->mutex);
    free(poh->entries);
    free(poh);
}

/* ============================================================================
 * GÉNÉRATION POH
 * ============================================================================ */

/**
 * @brief Générer tick PoH (SHA-256 séquentiel)
 * 
 * Algorithme :
 * 1. hash_new = SHA-256(hash_prev || sequence)
 * 2. sequence++
 * 3. Enregistrer dans buffer si activé
 */
void lum_poh_tick_internal(lum_poh_t* poh) {
    SHA256_CTX ctx;
    uint8_t new_hash[LUM_POH_HASH_SIZE];
    
    // SHA-256(hash_prev || sequence)
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, poh->current_hash, LUM_POH_HASH_SIZE);
    SHA256_Update(&ctx, &poh->sequence, sizeof(uint64_t));
    SHA256_Final(new_hash, &ctx);
    
    // Mettre à jour état
    memcpy(poh->current_hash, new_hash, LUM_POH_HASH_SIZE);
    poh->sequence++;
    poh->total_ticks++;
    
    // Calculer slot et tick
    poh->slot = lum_poh_sequence_to_slot(poh->sequence);
    poh->tick_in_slot = lum_poh_sequence_to_tick(poh->sequence);
    
    // Enregistrer dans buffer circulaire
    if (poh->enable_recording && poh->entries) {
        lum_poh_entry_t* entry = &poh->entries[poh->entries_tail];
        memcpy(entry->hash, poh->current_hash, LUM_POH_HASH_SIZE);
        entry->sequence = poh->sequence;
        entry->timestamp_ns = lum_poh_get_timestamp_ns();
        entry->slot = poh->slot;
        entry->tick_in_slot = poh->tick_in_slot;
        
        // Avancer tail (circulaire)
        poh->entries_tail = (poh->entries_tail + 1) % poh->entries_capacity;
        
        // Si buffer plein, avancer head
        if (poh->entries_tail == poh->entries_head) {
            poh->entries_head = (poh->entries_head + 1) % poh->entries_capacity;
        }
    }
}

/**
 * @brief Thread générateur PoH
 * 
 * Génère ticks en continu à la fréquence cible
 */
void* lum_poh_generator_thread(void* arg) {
    lum_poh_t* poh = (lum_poh_t*)arg;
    
    // OPTIMISATION: Batch processing pour réduire overhead
    #define BATCH_SIZE 100  // Générer 100 ticks par batch
    
    // Calculer intervalle entre batches (nanosecondes)
    uint64_t batch_interval_ns = (1000000000ULL * BATCH_SIZE) / poh->target_ticks_per_second;
    
    uint64_t next_batch_time = lum_poh_get_timestamp_ns();
    uint64_t metrics_update_time = next_batch_time;
    
    while (poh->running) {
        uint64_t now = lum_poh_get_timestamp_ns();
        
        // Générer batch de ticks si temps écoulé
        if (now >= next_batch_time) {
            pthread_mutex_lock(&poh->mutex);
            
            // Générer BATCH_SIZE ticks d'un coup
            for (int i = 0; i < BATCH_SIZE && poh->running; i++) {
                lum_poh_tick_internal(poh);
            }
            
            pthread_mutex_unlock(&poh->mutex);
            
            next_batch_time += batch_interval_ns;
            
            // Rattraper retard si nécessaire
            if (next_batch_time < now) {
                next_batch_time = now + batch_interval_ns;
            }
        }
        
        // Mettre à jour métriques toutes les secondes
        if (now - metrics_update_time >= 1000000000ULL) {
            pthread_mutex_lock(&poh->mutex);
            uint64_t elapsed_ns = now - poh->start_time_ns;
            if (elapsed_ns > 0) {
                poh->ticks_per_second = (double)poh->total_ticks * 1000000000.0 / (double)elapsed_ns;
            }
            pthread_mutex_unlock(&poh->mutex);
            metrics_update_time = now;
        }
        
        // Sleep adaptatif (plus long entre batches)
        uint64_t sleep_time = next_batch_time > now ? (next_batch_time - now) / 2 : 10000;
        if (sleep_time > 0 && sleep_time < 10000000) {  // Max 10ms
            sleep_ns(sleep_time);
        }
    }
    
    #undef BATCH_SIZE
    return NULL;
}

/* ============================================================================
 * CONTRÔLE THREAD
 * ============================================================================ */

bool lum_poh_start(lum_poh_t* poh) {
    if (!poh || poh->running) {
        return false;
    }
    
    poh->running = true;
    poh->start_time_ns = lum_poh_get_timestamp_ns();
    
    if (pthread_create(&poh->thread, NULL, lum_poh_generator_thread, poh) != 0) {
        poh->running = false;
        return false;
    }
    
    return true;
}

void lum_poh_stop(lum_poh_t* poh) {
    if (!poh || !poh->running) {
        return;
    }
    
    poh->running = false;
    pthread_join(poh->thread, NULL);
}

/* ============================================================================
 * API PUBLIQUE
 * ============================================================================ */

bool lum_poh_get_current(lum_poh_t* poh, lum_poh_entry_t* entry) {
    if (!poh || !entry) {
        return false;
    }
    
    pthread_mutex_lock(&poh->mutex);
    
    memcpy(entry->hash, poh->current_hash, LUM_POH_HASH_SIZE);
    entry->sequence = poh->sequence;
    entry->timestamp_ns = lum_poh_get_timestamp_ns();
    entry->slot = poh->slot;
    entry->tick_in_slot = poh->tick_in_slot;
    
    pthread_mutex_unlock(&poh->mutex);
    
    return true;
}

bool lum_poh_mix_transaction(lum_poh_t* poh, const uint8_t* tx_hash, lum_poh_proof_t* proof) {
    if (!poh || !tx_hash || !proof) {
        return false;
    }
    
    pthread_mutex_lock(&poh->mutex);
    
    // Mixer transaction dans hash PoH
    SHA256_CTX ctx;
    uint8_t mixed_hash[LUM_POH_HASH_SIZE];
    
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, poh->current_hash, LUM_POH_HASH_SIZE);
    SHA256_Update(&ctx, tx_hash, LUM_POH_HASH_SIZE);
    SHA256_Final(mixed_hash, &ctx);
    
    // Mettre à jour hash courant
    memcpy(poh->current_hash, mixed_hash, LUM_POH_HASH_SIZE);
    
    // Générer preuve
    memcpy(proof->tx_hash, tx_hash, LUM_POH_HASH_SIZE);
    memcpy(proof->poh_hash, poh->current_hash, LUM_POH_HASH_SIZE);
    proof->sequence = poh->sequence;
    proof->timestamp_ns = lum_poh_get_timestamp_ns();
    proof->slot = poh->slot;
    
    pthread_mutex_unlock(&poh->mutex);
    
    return true;
}

bool lum_poh_verify_proof(lum_poh_t* poh, const lum_poh_proof_t* proof) {
    if (!poh || !proof) {
        return false;
    }
    
    // Vérifier que la séquence existe dans l'historique
    lum_poh_entry_t entry;
    if (!lum_poh_get_entry(poh, proof->sequence, &entry)) {
        return false;
    }
    
    // Vérifier que le hash correspond
    if (memcmp(entry.hash, proof->poh_hash, LUM_POH_HASH_SIZE) != 0) {
        return false;
    }
    
    // Vérifier que le slot correspond
    if (entry.slot != proof->slot) {
        return false;
    }
    
    return true;
}

bool lum_poh_get_entry(lum_poh_t* poh, uint64_t sequence, lum_poh_entry_t* entry) {
    if (!poh || !entry || !poh->enable_recording || !poh->entries) {
        return false;
    }
    
    pthread_mutex_lock(&poh->mutex);
    
    // Rechercher dans buffer circulaire
    bool found = false;
    uint64_t idx = poh->entries_head;
    
    while (idx != poh->entries_tail) {
        if (poh->entries[idx].sequence == sequence) {
            memcpy(entry, &poh->entries[idx], sizeof(lum_poh_entry_t));
            found = true;
            break;
        }
        idx = (idx + 1) % poh->entries_capacity;
    }
    
    pthread_mutex_unlock(&poh->mutex);
    
    return found;
}

void lum_poh_get_metrics(lum_poh_t* poh, double* ticks_per_second, 
                         uint64_t* total_ticks, double* uptime_seconds) {
    if (!poh) {
        return;
    }
    
    pthread_mutex_lock(&poh->mutex);
    
    if (ticks_per_second) {
        *ticks_per_second = poh->ticks_per_second;
    }
    
    if (total_ticks) {
        *total_ticks = poh->total_ticks;
    }
    
    if (uptime_seconds) {
        uint64_t elapsed_ns = lum_poh_get_timestamp_ns() - poh->start_time_ns;
        *uptime_seconds = (double)elapsed_ns / 1000000000.0;
    }
    
    pthread_mutex_unlock(&poh->mutex);
}

// Made with Bob
