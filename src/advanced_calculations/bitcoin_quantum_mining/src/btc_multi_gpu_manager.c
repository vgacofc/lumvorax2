/*
 * LumVorax C256 - Multi-GPU Manager Implementation
 * Détection et gestion de plusieurs GPUs Intel Gen9
 */

#include "btc_multi_gpu_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <time.h>
#include <errno.h>
#include <xf86drm.h>
#include <i915_drm.h>
#include <pthread.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * UTILITAIRES
 * ═══════════════════════════════════════════════════════════════════════════ */

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * DÉTECTION DEVICES DRM
 * ═══════════════════════════════════════════════════════════════════════════ */

int btc_multi_gpu_scan_devices(char devices[][DRM_DEVICE_PATH_MAX], int max_devices) {
    int count = 0;
    
    /* Scanner /dev/dri/ pour renderD* */
    DIR* dir = opendir("/dev/dri");
    if (!dir) {
        fprintf(stderr, "[MULTI-GPU] ERROR: Cannot open /dev/dri: %s\n", strerror(errno));
        return -1;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL && count < max_devices) {
        /* Chercher renderD128, renderD129, etc. */
        if (strncmp(entry->d_name, "renderD", 7) == 0) {
            snprintf(devices[count], DRM_DEVICE_PATH_MAX, "/dev/dri/%s", entry->d_name);
            count++;
        }
    }
    
    closedir(dir);
    return count;
}

bool btc_multi_gpu_is_gen9(const char* device_path, uint32_t* device_id_out) {
    /* Ouvrir device */
    int fd = open(device_path, O_RDWR);
    if (fd < 0) {
        return false;
    }
    
    /* Vérifier driver i915 */
    struct drm_version version = {0};
    char name[32] = {0};
    version.name = name;
    version.name_len = sizeof(name);
    
    if (ioctl(fd, DRM_IOCTL_VERSION, &version) < 0) {
        close(fd);
        return false;
    }
    
    if (strcmp(name, "i915") != 0) {
        close(fd);
        return false;
    }
    
    /* Obtenir device ID via getparam */
    struct drm_i915_getparam gp = {0};
    int device_id = 0;
    gp.param = I915_PARAM_CHIPSET_ID;
    gp.value = &device_id;
    
    if (ioctl(fd, DRM_IOCTL_I915_GETPARAM, &gp) < 0) {
        close(fd);
        return false;
    }
    
    if (device_id_out) {
        *device_id_out = (uint32_t)device_id;
    }
    
    close(fd);
    
    /* Vérifier si Gen9 (Skylake/Kabylake)
     * Gen9 device IDs: 0x1900-0x19FF (Skylake), 0x5900-0x59FF (Kabylake)
     */
    bool is_gen9 = ((device_id >= 0x1900 && device_id <= 0x19FF) ||
                    (device_id >= 0x5900 && device_id <= 0x59FF));
    
    return is_gen9;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * INITIALISATION
 * ═══════════════════════════════════════════════════════════════════════════ */

int btc_multi_gpu_init(multi_gpu_context_t** ctx_out) {
    if (!ctx_out) {
        return -1;
    }
    
    /* Allouer contexte */
    multi_gpu_context_t* ctx = calloc(1, sizeof(multi_gpu_context_t));
    if (!ctx) {
        fprintf(stderr, "[MULTI-GPU] ERROR: Cannot allocate context\n");
        return -1;
    }
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  LumVorax C256 - Multi-GPU Manager                      ║\n");
    printf("║  Détection GPUs Intel Gen9                              ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    
    /* Scanner devices DRM */
    char devices[MAX_GPUS][DRM_DEVICE_PATH_MAX];
    int device_count = btc_multi_gpu_scan_devices(devices, MAX_GPUS);
    
    if (device_count < 0) {
        free(ctx);
        return -1;
    }
    
    printf("[MULTI-GPU] Devices DRM détectés: %d\n", device_count);
    
    /* Analyser chaque device */
    ctx->gpu_count = 0;
    for (int i = 0; i < device_count; i++) {
        gpu_info_t* gpu = &ctx->gpus[ctx->gpu_count];
        
        /* Copier chemin device */
        strncpy(gpu->device_path, devices[i], DRM_DEVICE_PATH_MAX - 1);
        gpu->fd = -1;
        
        /* Vérifier si Gen9 */
        uint32_t device_id = 0;
        if (!btc_multi_gpu_is_gen9(devices[i], &device_id)) {
            printf("[MULTI-GPU] %s: Non-Gen9, ignoré\n", devices[i]);
            continue;
        }
        
        gpu->device_id = device_id;
        gpu->is_gen9 = true;
        gpu->is_available = true;
        
        /* Obtenir nom device */
        switch (device_id) {
            case 0x5916: strcpy(gpu->device_name, "Intel UHD Graphics 620"); break;
            case 0x5917: strcpy(gpu->device_name, "Intel UHD Graphics 620"); break;
            case 0x591B: strcpy(gpu->device_name, "Intel HD Graphics 630"); break;
            case 0x591D: strcpy(gpu->device_name, "Intel HD Graphics P630"); break;
            case 0x5912: strcpy(gpu->device_name, "Intel HD Graphics 630"); break;
            case 0x1916: strcpy(gpu->device_name, "Intel HD Graphics 520"); break;
            case 0x1912: strcpy(gpu->device_name, "Intel HD Graphics 530"); break;
            default: snprintf(gpu->device_name, sizeof(gpu->device_name), 
                             "Intel Gen9 GPU [0x%04X]", device_id);
        }
        
        printf("[MULTI-GPU] GPU %d: %s\n", ctx->gpu_count, gpu->device_path);
        printf("            Device: %s (0x%04X)\n", gpu->device_name, device_id);
        
        ctx->gpu_count++;
    }
    
    if (ctx->gpu_count == 0) {
        printf("[MULTI-GPU] ⚠️  Aucun GPU Gen9 détecté\n");
        free(ctx);
        return 0;
    }
    
    printf("\n[MULTI-GPU] ✓ %d GPU(s) Gen9 détecté(s)\n", ctx->gpu_count);
    
    ctx->initialized = true;
    ctx->start_time_ns = get_timestamp_ns();
    *ctx_out = ctx;
    
    return ctx->gpu_count;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * GESTION GPUS
 * ═══════════════════════════════════════════════════════════════════════════ */

const gpu_info_t* btc_multi_gpu_get_info(const multi_gpu_context_t* ctx, int gpu_index) {
    if (!ctx || gpu_index < 0 || gpu_index >= ctx->gpu_count) {
        return NULL;
    }
    return &ctx->gpus[gpu_index];
}

int btc_multi_gpu_activate(multi_gpu_context_t* ctx, int gpu_index) {
    if (!ctx || gpu_index < 0 || gpu_index >= ctx->gpu_count) {
        return -1;
    }
    
    gpu_info_t* gpu = &ctx->gpus[gpu_index];
    
    if (gpu->fd >= 0) {
        printf("[MULTI-GPU] GPU %d déjà activé\n", gpu_index);
        return 0;
    }
    
    /* Ouvrir device DRM */
    gpu->fd = open(gpu->device_path, O_RDWR);
    if (gpu->fd < 0) {
        fprintf(stderr, "[MULTI-GPU] ERROR: Cannot open %s: %s\n",
                gpu->device_path, strerror(errno));
        gpu->is_available = false;
        return -1;
    }
    
    /* Vérifier driver */
    struct drm_version version = {0};
    char name[32] = {0};
    version.name = name;
    version.name_len = sizeof(name);
    
    if (ioctl(gpu->fd, DRM_IOCTL_VERSION, &version) < 0) {
        fprintf(stderr, "[MULTI-GPU] ERROR: Cannot get driver version\n");
        close(gpu->fd);
        gpu->fd = -1;
        gpu->is_available = false;
        return -1;
    }
    
    strncpy(gpu->driver_name, name, sizeof(gpu->driver_name) - 1);
    gpu->driver_major = version.version_major;
    gpu->driver_minor = version.version_minor;
    gpu->driver_patch = version.version_patchlevel;
    
    printf("[MULTI-GPU] ✓ GPU %d activé: %s %d.%d.%d\n",
           gpu_index, gpu->driver_name,
           gpu->driver_major, gpu->driver_minor, gpu->driver_patch);
    
    ctx->active_gpu_count++;
    return 0;
}

int btc_multi_gpu_deactivate(multi_gpu_context_t* ctx, int gpu_index) {
    if (!ctx || gpu_index < 0 || gpu_index >= ctx->gpu_count) {
        return -1;
    }
    
    gpu_info_t* gpu = &ctx->gpus[gpu_index];
    
    if (gpu->fd >= 0) {
        close(gpu->fd);
        gpu->fd = -1;
        ctx->active_gpu_count--;
        printf("[MULTI-GPU] GPU %d désactivé\n", gpu_index);
    }
    
    return 0;
}

int btc_multi_gpu_activate_all(multi_gpu_context_t* ctx) {
    if (!ctx) {
        return -1;
    }
    
    int activated = 0;
    for (int i = 0; i < ctx->gpu_count; i++) {
        if (btc_multi_gpu_activate(ctx, i) == 0) {
            activated++;
        }
    }
    
    printf("[MULTI-GPU] %d/%d GPU(s) activé(s)\n", activated, ctx->gpu_count);
    return activated;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * DISPATCH MULTI-GPU
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Structure pour thread GPU */
typedef struct {
    multi_gpu_context_t* ctx;
    int gpu_index;
    uint32_t nonce_start;
    uint32_t nonce_count;
    uint8_t block_header[80];
    uint32_t best_nonce;
    uint32_t leading_zeros;
    int result;
} gpu_thread_data_t;

/* Fonction thread GPU (placeholder - nécessite intégration avec btc_gen9_native_runner) */
static void* gpu_mining_thread(void* arg) {
    gpu_thread_data_t* data = (gpu_thread_data_t*)arg;
    
    printf("[MULTI-GPU] Thread GPU %d: nonces [%u, %u) (%u nonces)\n",
           data->gpu_index, data->nonce_start,
           data->nonce_start + data->nonce_count, data->nonce_count);
    
    /* TODO: Intégrer avec btc_gen9_execute_mining()
     * Pour l'instant, simulation */
    data->best_nonce = data->nonce_start;
    data->leading_zeros = 0;
    data->result = 0;
    
    /* Simuler temps calcul */
    usleep(100000); // 100ms
    
    return NULL;
}

int btc_multi_gpu_dispatch(multi_gpu_context_t* ctx,
                           const multi_gpu_dispatch_config_t* config,
                           multi_gpu_result_t* result) {
    if (!ctx || !config || !result) {
        return -1;
    }
    
    if (ctx->active_gpu_count == 0) {
        fprintf(stderr, "[MULTI-GPU] ERROR: No active GPUs\n");
        return -1;
    }
    
    printf("\n[MULTI-GPU] Dispatch sur %d GPU(s)\n", ctx->active_gpu_count);
    printf("[MULTI-GPU] Nonces: [%u, %u) (%u total)\n",
           config->nonce_start,
           config->nonce_start + config->nonce_count,
           config->nonce_count);
    
    /* Calculer distribution nonces */
    uint32_t nonces_per_gpu = config->nonce_count / ctx->active_gpu_count;
    uint32_t nonces_remainder = config->nonce_count % ctx->active_gpu_count;
    
    /* Créer threads GPU */
    pthread_t threads[MAX_GPUS];
    gpu_thread_data_t thread_data[MAX_GPUS];
    int thread_count = 0;
    uint32_t current_nonce = config->nonce_start;
    
    for (int i = 0; i < ctx->gpu_count; i++) {
        if (ctx->gpus[i].fd < 0) continue;
        
        gpu_thread_data_t* data = &thread_data[thread_count];
        data->ctx = ctx;
        data->gpu_index = i;
        data->nonce_start = current_nonce;
        data->nonce_count = nonces_per_gpu + (thread_count < nonces_remainder ? 1 : 0);
        memcpy(data->block_header, config->block_header, 80);
        
        pthread_create(&threads[thread_count], NULL, gpu_mining_thread, data);
        
        current_nonce += data->nonce_count;
        thread_count++;
    }
    
    /* Attendre tous les threads */
    uint64_t start_time = get_timestamp_ns();
    
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    
    uint64_t end_time = get_timestamp_ns();
    double elapsed_sec = (double)(end_time - start_time) / 1e9;
    
    /* Trouver meilleur résultat */
    result->best_nonce = 0;
    result->leading_zeros = 0;
    result->gpu_index = -1;
    
    for (int i = 0; i < thread_count; i++) {
        if (thread_data[i].leading_zeros > result->leading_zeros) {
            result->leading_zeros = thread_data[i].leading_zeros;
            result->best_nonce = thread_data[i].best_nonce;
            result->gpu_index = thread_data[i].gpu_index;
        }
    }
    
    result->total_hashes = config->nonce_count;
    result->elapsed_sec = elapsed_sec;
    result->hashrate_mhs = ((double)config->nonce_count / elapsed_sec) / 1e6;
    
    /* Mettre à jour statistiques */
    ctx->total_hashes_all_gpus += config->nonce_count;
    ctx->global_hashrate_mhs = result->hashrate_mhs;
    
    printf("[MULTI-GPU] ✓ Dispatch terminé: %.2f MH/s\n", result->hashrate_mhs);
    
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * STATISTIQUES
 * ═══════════════════════════════════════════════════════════════════════════ */

int btc_multi_gpu_get_stats(const multi_gpu_context_t* ctx,
                            uint64_t* total_hashes_out,
                            double* hashrate_mhs_out) {
    if (!ctx) {
        return -1;
    }
    
    if (total_hashes_out) {
        *total_hashes_out = ctx->total_hashes_all_gpus;
    }
    
    if (hashrate_mhs_out) {
        *hashrate_mhs_out = ctx->global_hashrate_mhs;
    }
    
    return 0;
}

void btc_multi_gpu_print_info(const multi_gpu_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  Multi-GPU Information                                   ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    
    printf("GPUs détectés: %d\n", ctx->gpu_count);
    printf("GPUs actifs: %d\n\n", ctx->active_gpu_count);
    
    for (int i = 0; i < ctx->gpu_count; i++) {
        const gpu_info_t* gpu = &ctx->gpus[i];
        
        printf("GPU %d:\n", i);
        printf("  Device: %s\n", gpu->device_path);
        printf("  Name: %s\n", gpu->device_name);
        printf("  Device ID: 0x%04X\n", gpu->device_id);
        printf("  Driver: %s %d.%d.%d\n",
               gpu->driver_name, gpu->driver_major,
               gpu->driver_minor, gpu->driver_patch);
        printf("  Status: %s\n", gpu->fd >= 0 ? "Active" : "Inactive");
        printf("  Gen9: %s\n", gpu->is_gen9 ? "Yes" : "No");
        printf("\n");
    }
}

void btc_multi_gpu_print_stats(const multi_gpu_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  Multi-GPU Statistics                                    ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
    
    printf("Total hashes (all GPUs): %lu\n", ctx->total_hashes_all_gpus);
    printf("Global hashrate: %.2f MH/s\n", ctx->global_hashrate_mhs);
    printf("\n");
    
    for (int i = 0; i < ctx->gpu_count; i++) {
        const gpu_info_t* gpu = &ctx->gpus[i];
        if (gpu->fd < 0) continue;
        
        printf("GPU %d (%s):\n", i, gpu->device_name);
        printf("  Hashes: %lu\n", gpu->total_hashes);
        printf("  Hashrate: %.2f MH/s\n", gpu->current_hashrate_mhs);
        printf("\n");
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * CLEANUP
 * ═══════════════════════════════════════════════════════════════════════════ */

void btc_multi_gpu_cleanup(multi_gpu_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    printf("[MULTI-GPU] Cleanup...\n");
    
    /* Fermer tous les GPUs */
    for (int i = 0; i < ctx->gpu_count; i++) {
        btc_multi_gpu_deactivate(ctx, i);
    }
    
    free(ctx);
    printf("[MULTI-GPU] ✓ Cleanup terminé\n");
}

// Made with ❤️ by Bob - LumVorax C256

// Made with Bob
