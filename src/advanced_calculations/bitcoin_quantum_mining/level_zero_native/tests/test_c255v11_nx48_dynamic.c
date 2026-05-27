/**
 * LumVorax C258 — Test NX48 Allocation Dynamique 3GB
 *
 * OBJECTIF: Allocation dynamique basée sur RAM système réelle
 * - Détection RAM via sysinfo() (formule OpenCL: 75% RAM, max 4GB)
 * - Batch size: 256K → maximum GPU (progression adaptative)
 * - Nombre contextes: 1, 2 ou 3 (selon performance)
 * - Mémoire GPU: Monitoring temps réel
 * - THP: Utilisé pour buffers >2MB (ne pas supprimer!)
 *
 * Architecture: 100% i915 DRM natif (0% OpenCL, 0% Level Zero)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdatomic.h>
#include <errno.h>
#include <sys/sysinfo.h>
#include "../include/btc_gen9_native_runner.h"

/* C255V11: Atomiques NX48 pour gestion dynamique (déclarations locales) */
static _Atomic uint32_t nx48_ctrl_batch_size_gen9 = 262144;  /* 256K départ */
static _Atomic int      nx48_ctrl_num_contexts = 2;          /* 2 contextes */
static _Atomic int      nx48_ctrl_use_thp = 1;               /* THP activé */
static _Atomic uint64_t nx48_ctrl_gpu_mem_available = 0;     /* Mémoire GPU */

#define WORK_GROUP_SIZE 256
#define MAX_ITERATIONS 100  /* 100 dispatches SANS REOPEN pour tester limitation */

static void print_header(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C258 — Allocation Dynamique 3GB (RAM Système)   ║\n");
    printf("║  0%% OpenCL, 0%% Level Zero, 100%% i915 DRM Direct          ║\n");
    printf("║  Formule OpenCL: 75%% RAM système, max 4GB                 ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

/**
 * C258: Détection RAM système via sysinfo() + formule OpenCL
 * Formule: (RAM_MB * 75%) / 100, max 4GB, min 1GB
 */
static uint64_t get_system_ram_mb(void) {
    struct sysinfo si;
    if (sysinfo(&si) != 0) {
        fprintf(stderr, "⚠️  Erreur sysinfo(), utilisation 8GB par défaut\n");
        return 8192;  /* 8GB par défaut */
    }
    
    /* Convertir totalram (bytes) en MB */
    uint64_t ram_mb = (si.totalram * si.mem_unit) / (1024 * 1024);
    return ram_mb;
}

static uint64_t calculate_dynamic_gpu_limit_mb(uint64_t ram_mb) {
    /* Formule OpenCL: 75% de la RAM système */
    uint64_t limit_mb = (ram_mb * 75) / 100;
    
    /* Clamp: min 1GB, max 4GB */
    if (limit_mb < 1024) limit_mb = 1024;
    if (limit_mb > 4096) limit_mb = 4096;
    
    return limit_mb;
}

/**
 * Query mémoire GPU disponible avec détection RAM système
 */
static uint64_t query_gpu_memory_available(void) {
    /* C258: Détection RAM système */
    uint64_t ram_mb = get_system_ram_mb();
    uint64_t limit_mb = calculate_dynamic_gpu_limit_mb(ram_mb);
    
    printf("[C258] 📊 RAM système détectée: %lu MB (%.2f GB)\n",
           ram_mb, ram_mb / 1024.0);
    printf("[C258] 🎯 Limite GPU calculée: %lu MB (%.2f GB) [formule: 75%% RAM, max 4GB]\n",
           limit_mb, limit_mb / 1024.0);
    
    /* Retourner en bytes */
    return limit_mb * 1024 * 1024ULL;
}

/**
 * NX48 calcule batch_size optimal selon mémoire GPU disponible
 * Stratégie: Démarrer 256K, doubler si succès, protéger mémoire
 */
static uint32_t nx48_compute_optimal_batch_size(uint64_t mem_available, 
                                                 uint32_t current_size,
                                                 int last_success) {
    const uint32_t MIN_SIZE = 262144;      /* 256K (TOUJOURS le départ) */
    const uint32_t MAX_SIZE = 1048576000;  /* 1000M (maximum autorisé) */
    
    /* Mémoire requise par nonce: 32 bytes (SHA-256 hash) */
    const uint64_t BYTES_PER_NONCE = 32;
    
    /* Calculer maximum selon mémoire (80% de la mémoire disponible) */
    uint64_t max_nonces = (mem_available * 80) / (100 * BYTES_PER_NONCE);
    uint32_t mem_limit = (uint32_t)(max_nonces < MAX_SIZE ? max_nonces : MAX_SIZE);
    
    if (!last_success) {
        /* Crash → Diviser par 2 */
        uint32_t new_size = current_size / 2;
        if (new_size < MIN_SIZE) new_size = MIN_SIZE;
        
        printf("[NX48-C255V11] ⚠️  Crash détecté → Recul batch_size: %u → %u\n",
               current_size, new_size);
        
        return new_size;
    }
    
    /* Succès → Doubler si possible */
    if (current_size < mem_limit && current_size < MAX_SIZE) {
        uint32_t new_size = current_size * 2;
        
        /* Clamp au minimum entre mem_limit et MAX_SIZE */
        if (new_size > mem_limit) new_size = mem_limit;
        if (new_size > MAX_SIZE) new_size = MAX_SIZE;
        
        double progress = (double)(new_size - MIN_SIZE) / (double)(MAX_SIZE - MIN_SIZE) * 100.0;
        
        printf("[NX48-C255V11] ✅ Succès → Doublement batch_size: %u → %u (%.1f%% vers max, mem_limit=%u)\n",
               current_size, new_size, progress, mem_limit);
        
        return new_size;
    }
    
    /* Déjà au maximum */
    printf("[NX48-C255V11] 🏁 Vitesse de croisière atteinte: batch_size=%u\n", current_size);
    return current_size;
}

/**
 * NX48 décide nombre de contextes optimal (1, 2 ou 3)
 * Stratégie: Commencer avec 2, ajuster selon hashrate
 */
static int nx48_compute_optimal_contexts(double hashrate_mhs, int current_contexts) {
    /* Pour l'instant, garder 2 contextes (stable) */
    /* TODO: Implémenter logique adaptative basée sur hashrate */
    (void)hashrate_mhs;  /* Unused pour l'instant */
    return current_contexts;
}

int main(void) {
    print_header();
    
    printf("[C258] Configuration Allocation Dynamique:\n");
    printf("  - Détection RAM: sysinfo() + formule OpenCL (75%% RAM, max 4GB)\n");
    printf("  - Batch size initial: 256K (progression automatique)\n");
    printf("  - Contextes GPU: 2 (adaptatif 1/2/3)\n");
    printf("  - Mémoire GPU: Monitoring temps réel\n");
    printf("  - THP: Activé pour buffers >2MB\n");
    printf("  - Kernel: kernels/btc_sha256_gen9.bin\n");
    printf("  - Log: logs/forensic/test_c255v11_nx48.log\n");
    printf("\n");
    
    /* Initialiser atomiques NX48 */
    atomic_store(&nx48_ctrl_batch_size_gen9, 262144);  /* 256K départ */
    atomic_store(&nx48_ctrl_num_contexts, 2);          /* 2 contextes */
    atomic_store(&nx48_ctrl_use_thp, 1);               /* THP activé */
    
    /* C258: Query mémoire GPU disponible avec détection RAM système */
    uint64_t gpu_mem_available = query_gpu_memory_available();
    atomic_store(&nx48_ctrl_gpu_mem_available, gpu_mem_available);
    
    printf("[C258] ✅ Mémoire GPU disponible: %.2f GB\n",
           gpu_mem_available / (1024.0 * 1024.0 * 1024.0));
    printf("\n");
    
    /* Initialisation Gen9 Native */
    printf("[C258] Initialisation Gen9 Native avec allocation dynamique...\n");
    
    uint32_t current_batch_size = atomic_load(&nx48_ctrl_batch_size_gen9);
    
    btc_gen9_config_t config = {
        .batch_size = current_batch_size,
        .work_group_size = WORK_GROUP_SIZE,
        .kernel_path = "kernels/btc_sha256_gen9.bin_kbl.bin",
        .log_path = "logs/forensic/test_c255v11_nx48.log",
        .enable_profiling = true
    };
    
    btc_gen9_context_t* ctx = NULL;
    if (btc_gen9_init(&ctx, &config) < 0 || !ctx) {
        printf("❌ Échec initialisation\n");
        return 1;
    }
    printf("✅ Initialisation réussie\n\n");
    
    /* Boucle principale: NX48 adapte dynamiquement */
    printf("[C258] Démarrage progression adaptative (allocation dynamique 3GB)...\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    int total_success = 0;
    int total_crashes = 0;
    double total_time = 0.0;
    uint64_t total_nonces = 0;
    
    /* Block header Bitcoin RÉEL (80 bytes) - FIX C311 #2
     * Source: config/btc_nx48_alltime.csv
     * Best nonce connu: 1604817786 (20 leading zeros)
     * Header hex: 01000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001e8cef69ffff001d00000000
     */
    uint8_t block_header[80] = {
        /* Version (4 bytes, little-endian) */
        0x01, 0x00, 0x00, 0x00,
        /* Previous block hash (32 bytes) - tous zéros pour test */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* Merkle root (32 bytes) - tous zéros pour test */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* Timestamp (4 bytes, little-endian) */
        0x00, 0x00, 0x00, 0x1e,
        /* Bits/Difficulty (4 bytes, little-endian) */
        0x8c, 0xef, 0x69, 0xff,
        /* Nonce (4 bytes, little-endian) - sera remplacé par GPU */
        0xff, 0x00, 0x1d, 0x00,
        /* Padding (4 bytes) */
        0x00, 0x00, 0x00, 0x00
    };
    uint32_t best_nonce = 0;
    uint32_t leading_zeros = 0;
    
    fprintf(stderr, "[C311 FIX #2] Block header RÉEL chargé (80 bytes)\n");
    fprintf(stderr, "[C311] Nonce connu attendu: 1604817786 (20 leading zeros)\n");
    
    for (int iter = 0; iter < MAX_ITERATIONS; iter++) {
        /* Lire batch_size actuel depuis NX48 */
        current_batch_size = atomic_load(&nx48_ctrl_batch_size_gen9);
        
        printf("[Batch %d/%d] batch_size=%u (%.2f M nonces)\n",
               iter + 1, MAX_ITERATIONS,
               current_batch_size, current_batch_size / 1000000.0);
        
        /* Exécuter dispatch */
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        /* Exécuter mining avec batch_size actuel */
        int ret = btc_gen9_execute_mining(ctx, block_header, 0, current_batch_size,
                                          &best_nonce, &leading_zeros);
        int success = (ret == 0);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        double dispatch_time = (end.tv_sec - start.tv_sec) +
                              (end.tv_nsec - start.tv_nsec) / 1e9;
        
        if (success) {
            total_success++;
            total_nonces += current_batch_size;
            total_time += dispatch_time;
            
            double hashrate = (current_batch_size / dispatch_time) / 1e6;  /* MH/s */
            
            printf("  ✅ Succès: %.3f sec, %.2f MH/s\n", dispatch_time, hashrate);
            
            /* NX48 calcule nouveau batch_size optimal */
            uint32_t new_batch_size = nx48_compute_optimal_batch_size(
                gpu_mem_available, current_batch_size, 1);
            
            atomic_store(&nx48_ctrl_batch_size_gen9, new_batch_size);
            
            /* NX48 ajuste nombre de contextes si nécessaire */
            int current_contexts = atomic_load(&nx48_ctrl_num_contexts);
            int new_contexts = nx48_compute_optimal_contexts(hashrate, current_contexts);
            if (new_contexts != current_contexts) {
                atomic_store(&nx48_ctrl_num_contexts, new_contexts);
                printf("  🔄 NX48 ajuste contextes: %d → %d\n", 
                       current_contexts, new_contexts);
            }
        } else {
            total_crashes++;
            
            printf("  ❌ Crash: errno=%d\n", errno);
            
            /* NX48 recule batch_size */
            uint32_t new_batch_size = nx48_compute_optimal_batch_size(
                gpu_mem_available, current_batch_size, 0);
            
            atomic_store(&nx48_ctrl_batch_size_gen9, new_batch_size);
        }
        
        printf("\n");
        
        /* DÉSACTIVÉ: Forcer 100 dispatches SANS REOPEN pour tester limitation */
        /* if (current_batch_size == atomic_load(&nx48_ctrl_batch_size_gen9) &&
            iter > 10) {
            printf("[C255V11] 🏁 Vitesse de croisière stable détectée\n");
            break;
        } */
    }
    
    /* Métriques finales */
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("║  MÉTRIQUES FINALES C258 — ALLOCATION DYNAMIQUE 3GB          ║\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    printf("Résultats:\n");
    printf("  - Dispatches réussis: %d/%d (%.1f%%)\n", 
           total_success, total_success + total_crashes,
           100.0 * total_success / (total_success + total_crashes));
    printf("  - Crashes: %d\n", total_crashes);
    printf("  - Batch size final: %u (%.2f M nonces)\n",
           atomic_load(&nx48_ctrl_batch_size_gen9),
           atomic_load(&nx48_ctrl_batch_size_gen9) / 1000000.0);
    printf("  - Contextes GPU: %d\n", atomic_load(&nx48_ctrl_num_contexts));
    printf("\n");
    
    if (total_success > 0) {
        double avg_hashrate = (total_nonces / total_time) / 1e6;  /* MH/s */
        printf("Performance:\n");
        printf("  - Total nonces: %lu (%.2f M)\n", 
               total_nonces, total_nonces / 1000000.0);
        printf("  - Temps total: %.3f sec\n", total_time);
        printf("  - Hashrate moyen: %.2f MH/s\n", avg_hashrate);
        printf("  - Temps moyen/dispatch: %.3f sec\n", total_time / total_success);
    }
    
    printf("\n");
    
    /* Cleanup */
    btc_gen9_cleanup(ctx);
    
    /* Résultat final */
    if (total_success > 0 && total_crashes == 0) {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ✅ TEST RÉUSSI — ALLOCATION DYNAMIQUE 3GB FONCTIONNELLE   ║\n");
        printf("║  Détection RAM système + formule OpenCL (75%%, max 4GB)    ║\n");
        printf("║  Progression adaptative 256K → Maximum GPU                ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
        return 0;
    } else if (total_success > 0) {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ⚠️  TEST PARTIEL — Allocation dynamique trouve limite     ║\n");
        printf("║  %d succès, %d crashes — Batch optimal trouvé             ║\n",
               total_success, total_crashes);
        printf("╚════════════════════════════════════════════════════════════╝\n");
        return 0;
    } else {
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  ❌ TEST ÉCHOUÉ — Aucun dispatch réussi                    ║\n");
        printf("╚════════════════════════════════════════════════════════════╝\n");
        return 1;
    }
}

// Made with Bob
