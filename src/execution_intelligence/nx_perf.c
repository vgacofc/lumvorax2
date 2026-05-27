/*
 * NX-Perf — Performance Monitoring System
 * Implémentation des compteurs de performance
 *
 * Conformité : STANDARD_NAMES.md v4.2
 * Cycle : C142
 * Date : 2026-04-30
 */

#define _GNU_SOURCE
#include "nx_perf.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <time.h>
#include <stdio.h>
#include <sys/syscall.h>

/* Wrapper pour perf_event_open syscall */
static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

/* Obtient le timestamp monotonic en nanosecondes */
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* Configure un compteur hardware */
static int setup_counter(nx_perf_counter_t* counter, nx_perf_counter_type_t type) {
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(struct perf_event_attr));
    
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.disabled = 1;
    pe.exclude_kernel = 0;
    pe.exclude_hv = 1;
    
    /* Map type vers événement hardware */
    switch (type) {
        case NX_PERF_CPU_CYCLES:
            pe.config = PERF_COUNT_HW_CPU_CYCLES;
            break;
        case NX_PERF_INSTRUCTIONS:
            pe.config = PERF_COUNT_HW_INSTRUCTIONS;
            break;
        case NX_PERF_CACHE_REFERENCES:
            pe.config = PERF_COUNT_HW_CACHE_REFERENCES;
            break;
        case NX_PERF_CACHE_MISSES:
            pe.config = PERF_COUNT_HW_CACHE_MISSES;
            break;
        case NX_PERF_BRANCH_INSTRUCTIONS:
            pe.config = PERF_COUNT_HW_BRANCH_INSTRUCTIONS;
            break;
        case NX_PERF_BRANCH_MISSES:
            pe.config = PERF_COUNT_HW_BRANCH_MISSES;
            break;
        case NX_PERF_BUS_CYCLES:
            pe.config = PERF_COUNT_HW_BUS_CYCLES;
            break;
        case NX_PERF_STALLED_CYCLES_FRONTEND:
            pe.config = PERF_COUNT_HW_STALLED_CYCLES_FRONTEND;
            break;
        case NX_PERF_STALLED_CYCLES_BACKEND:
            pe.config = PERF_COUNT_HW_STALLED_CYCLES_BACKEND;
            break;
        case NX_PERF_REF_CPU_CYCLES:
            pe.config = PERF_COUNT_HW_REF_CPU_CYCLES;
            break;
        default:
            return -1;
    }
    
    counter->type = type;
    counter->fd = perf_event_open(&pe, 0, -1, -1, 0);
    
    if (counter->fd < 0) {
        return -1;
    }
    
    return 0;
}

nx_perf_context_t* nx_perf_init(const nx_perf_config_t* config) {
    if (!config) {
        errno = EINVAL;
        return NULL;
    }
    
    nx_perf_context_t* ctx = calloc(1, sizeof(nx_perf_context_t));
    if (!ctx) return NULL;
    
    ctx->config = *config;
    ctx->is_active = false;
    
    /* Initialise les compteurs hardware si activés */
    if (config->enable_hardware_counters) {
        for (int i = 0; i < NX_PERF_COUNTER_MAX; i++) {
            if (setup_counter(&ctx->counters[i], i) < 0) {
                /* Certains compteurs peuvent ne pas être disponibles */
                ctx->counters[i].fd = -1;
            }
        }
    }
    
    /* Alloue le buffer hotspots si activé */
    if (config->enable_hotspot_detection) {
        ctx->hotspots = calloc(config->max_hotspots, sizeof(nx_perf_hotspot_t));
        if (!ctx->hotspots) {
            nx_perf_destroy(ctx);
            return NULL;
        }
    }
    
    return ctx;
}

int nx_perf_start(nx_perf_context_t* ctx) {
    if (!ctx) return -1;
    if (ctx->is_active) return 0;
    
    /* Active tous les compteurs */
    for (int i = 0; i < NX_PERF_COUNTER_MAX; i++) {
        if (ctx->counters[i].fd >= 0) {
            ioctl(ctx->counters[i].fd, PERF_EVENT_IOC_RESET, 0);
            ioctl(ctx->counters[i].fd, PERF_EVENT_IOC_ENABLE, 0);
        }
    }
    
    ctx->start_ts_ns = get_timestamp_ns();
    ctx->is_active = true;
    
    return 0;
}

int nx_perf_stop(nx_perf_context_t* ctx) {
    if (!ctx) return -1;
    if (!ctx->is_active) return 0;
    
    /* Lit les compteurs une dernière fois */
    nx_perf_read_counters(ctx);
    
    /* Désactive tous les compteurs */
    for (int i = 0; i < NX_PERF_COUNTER_MAX; i++) {
        if (ctx->counters[i].fd >= 0) {
            ioctl(ctx->counters[i].fd, PERF_EVENT_IOC_DISABLE, 0);
        }
    }
    
    ctx->is_active = false;
    
    /* Calcule les métriques finales */
    if (ctx->config.enable_semantic_analysis) {
        nx_perf_compute_semantic_metrics(ctx);
    }
    
    return 0;
}

int nx_perf_read_counters(nx_perf_context_t* ctx) {
    if (!ctx) return -1;
    
    for (int i = 0; i < NX_PERF_COUNTER_MAX; i++) {
        if (ctx->counters[i].fd >= 0) {
            ssize_t bytes = read(ctx->counters[i].fd, &ctx->counters[i].value, sizeof(uint64_t));
            if (bytes != sizeof(uint64_t)) {
                return -1;
            }
        }
    }
    
    return 0;
}

int nx_perf_compute_semantic_metrics(nx_perf_context_t* ctx) {
    if (!ctx) return -1;
    
    uint64_t cycles = ctx->counters[NX_PERF_CPU_CYCLES].value;
    uint64_t instructions = ctx->counters[NX_PERF_INSTRUCTIONS].value;
    uint64_t cache_refs = ctx->counters[NX_PERF_CACHE_REFERENCES].value;
    uint64_t cache_misses = ctx->counters[NX_PERF_CACHE_MISSES].value;
    uint64_t branches = ctx->counters[NX_PERF_BRANCH_INSTRUCTIONS].value;
    uint64_t branch_misses = ctx->counters[NX_PERF_BRANCH_MISSES].value;
    uint64_t stalled_frontend = ctx->counters[NX_PERF_STALLED_CYCLES_FRONTEND].value;
    uint64_t stalled_backend = ctx->counters[NX_PERF_STALLED_CYCLES_BACKEND].value;
    
    /* IPC (Instructions Per Cycle) */
    ctx->metrics.ipc = cycles > 0 ? (double)instructions / (double)cycles : 0.0;
    
    /* Cache miss rate */
    ctx->metrics.cache_miss_rate = cache_refs > 0 ? 
        (double)cache_misses / (double)cache_refs * 100.0 : 0.0;
    
    /* Branch miss rate */
    ctx->metrics.branch_miss_rate = branches > 0 ?
        (double)branch_misses / (double)branches * 100.0 : 0.0;
    
    /* Stall rates */
    ctx->metrics.frontend_stall_rate = cycles > 0 ?
        (double)stalled_frontend / (double)cycles * 100.0 : 0.0;
    
    ctx->metrics.backend_stall_rate = cycles > 0 ?
        (double)stalled_backend / (double)cycles * 100.0 : 0.0;
    
    /* CPU utilization (approximation) */
    uint64_t ref_cycles = ctx->counters[NX_PERF_REF_CPU_CYCLES].value;
    ctx->metrics.cpu_utilization = ref_cycles > 0 ?
        (double)cycles / (double)ref_cycles * 100.0 : 0.0;
    
    /* Utilise elapsed_ns pour calcul alternatif si nécessaire */
    (void)ctx->start_ts_ns; /* Évite warning unused */
    
    return 0;
}

int nx_perf_detect_hotspots(nx_perf_context_t* ctx) {
    if (!ctx || !ctx->hotspots) return -1;
    
    /* Cette fonction nécessiterait l'intégration avec un profiler
     * Pour l'instant, retourne 0 (pas de hotspots détectés)
     * Une implémentation complète utiliserait perf_event avec sampling
     */
    ctx->num_hotspots = 0;
    return 0;
}

int nx_perf_generate_report(const nx_perf_context_t* ctx, const char* output_path) {
    if (!ctx || !output_path) return -1;
    
    FILE* f = fopen(output_path, "w");
    if (!f) return -1;
    
    fprintf(f, "# NX-Perf Performance Report\n\n");
    fprintf(f, "## Hardware Counters\n\n");
    
    const char* counter_names[] = {
        "CPU Cycles",
        "Instructions",
        "Cache References",
        "Cache Misses",
        "Branch Instructions",
        "Branch Misses",
        "Bus Cycles",
        "Stalled Cycles Frontend",
        "Stalled Cycles Backend",
        "Reference CPU Cycles"
    };
    
    for (int i = 0; i < NX_PERF_COUNTER_MAX; i++) {
        if (ctx->counters[i].fd >= 0) {
            fprintf(f, "- %s: %lu\n", counter_names[i], ctx->counters[i].value);
        }
    }
    
    fprintf(f, "\n## Semantic Metrics\n\n");
    fprintf(f, "- IPC (Instructions Per Cycle): %.3f\n", ctx->metrics.ipc);
    fprintf(f, "- Cache Miss Rate: %.2f%%\n", ctx->metrics.cache_miss_rate);
    fprintf(f, "- Branch Miss Rate: %.2f%%\n", ctx->metrics.branch_miss_rate);
    fprintf(f, "- Frontend Stall Rate: %.2f%%\n", ctx->metrics.frontend_stall_rate);
    fprintf(f, "- Backend Stall Rate: %.2f%%\n", ctx->metrics.backend_stall_rate);
    fprintf(f, "- CPU Utilization: %.2f%%\n", ctx->metrics.cpu_utilization);
    
    if (ctx->num_hotspots > 0) {
        fprintf(f, "\n## Hotspots\n\n");
        for (uint32_t i = 0; i < ctx->num_hotspots; i++) {
            fprintf(f, "%d. %s (%.2f%% total)\n", 
                    i + 1, 
                    ctx->hotspots[i].function_name,
                    ctx->hotspots[i].percentage_total);
        }
    }
    
    fclose(f);
    return 0;
}

int nx_perf_get_semantic_metrics(const nx_perf_context_t* ctx, nx_perf_semantic_metrics_t* metrics) {
    if (!ctx || !metrics) return -1;
    
    *metrics = ctx->metrics;
    return 0;
}

int nx_perf_get_counter(const nx_perf_context_t* ctx, nx_perf_counter_type_t type, uint64_t* value) {
    if (!ctx || !value || type >= NX_PERF_COUNTER_MAX) return -1;
    
    *value = ctx->counters[type].value;
    return 0;
}

void nx_perf_destroy(nx_perf_context_t* ctx) {
    if (!ctx) return;
    
    if (ctx->is_active) {
        nx_perf_stop(ctx);
    }
    
    /* Ferme tous les file descriptors */
    for (int i = 0; i < NX_PERF_COUNTER_MAX; i++) {
        if (ctx->counters[i].fd >= 0) {
            close(ctx->counters[i].fd);
        }
    }
    
    if (ctx->hotspots) {
        free(ctx->hotspots);
    }
    
    free(ctx);
}

int nx_perf_benchmark_function(void (*func)(void*), void* arg, nx_perf_semantic_metrics_t* metrics) {
    if (!func) return -1;
    
    nx_perf_config_t config = {
        .enable_hardware_counters = true,
        .enable_semantic_analysis = true,
        .enable_hotspot_detection = false,
        .sampling_period = 0,
        .max_hotspots = 0,
        .output_path = ""
    };
    
    nx_perf_context_t* ctx = nx_perf_init(&config);
    if (!ctx) return -1;
    
    nx_perf_start(ctx);
    func(arg);
    nx_perf_stop(ctx);
    
    if (metrics) {
        nx_perf_get_semantic_metrics(ctx, metrics);
    }
    
    nx_perf_destroy(ctx);
    return 0;
}

// Made with Bob
