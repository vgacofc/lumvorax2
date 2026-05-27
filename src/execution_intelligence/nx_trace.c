/*
 * NX-Trace — Execution Intelligence System (EIS)
 * Implémentation du traçage sémantique
 *
 * Conformité : STANDARD_NAMES.md v4.2
 * Cycle : C142
 * Date : 2026-04-30
 */

#define _GNU_SOURCE
#include "nx_trace.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <time.h>
#include <stdio.h>
#include <sched.h>

/* CRC32C (Castagnoli) pour intégrité */
static uint32_t crc32c_table[256];
static bool crc32c_initialized = false;

static void init_crc32c_table(void) {
    if (crc32c_initialized) return;
    
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ ((crc & 1) ? 0x82F63B78 : 0);
        }
        crc32c_table[i] = crc;
    }
    crc32c_initialized = true;
}

static uint32_t compute_crc32c(const void* data, size_t len) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < len; i++) {
        crc = crc32c_table[(crc ^ bytes[i]) & 0xFF] ^ (crc >> 8);
    }
    
    return ~crc;
}

/* Obtient le timestamp monotonic en nanosecondes */
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* Obtient le TID actuel */
static uint32_t get_thread_id(void) {
    return (uint32_t)syscall(SYS_gettid);
}

/* Obtient le CPU actuel */
static uint32_t get_cpu_id(void) {
    return (uint32_t)sched_getcpu();
}

nx_trace_context_t* nx_trace_init(const nx_trace_config_t* config) {
    if (!config) {
        errno = EINVAL;
        return NULL;
    }
    
    init_crc32c_table();
    
    nx_trace_context_t* ctx = calloc(1, sizeof(nx_trace_context_t));
    if (!ctx) return NULL;
    
    ctx->config = *config;
    ctx->buffer_size = config->buffer_size > 0 ? config->buffer_size : 4096;
    
    ctx->buffer = calloc(ctx->buffer_size, sizeof(nx_trace_event_t));
    if (!ctx->buffer) {
        free(ctx);
        return NULL;
    }
    
    ctx->buffer_index = 0;
    ctx->total_events = 0;
    ctx->is_active = false;
    ctx->output_fd = -1;
    
    return ctx;
}

int nx_trace_start(nx_trace_context_t* ctx) {
    if (!ctx) return -1;
    if (ctx->is_active) return 0;
    
    /* Ouvre le fichier de sortie */
    if (ctx->config.output_path[0] != '\0') {
        ctx->output_fd = open(ctx->config.output_path,
                             O_WRONLY | O_CREAT | O_TRUNC,
                             0644);
        if (ctx->output_fd < 0) {
            return -1;
        }
        
        /* Écrit le HEADER LUM standard (64 bytes) */
        uint8_t header[64] = {0};
        
        /* Magic "LUM\0" */
        header[0] = 'L';
        header[1] = 'U';
        header[2] = 'M';
        header[3] = '\0';
        
        /* Version (uint32_t à offset 4) */
        uint32_t version = 1;
        memcpy(&header[4], &version, sizeof(uint32_t));
        
        /* Count sera mis à jour dans nx_trace_stop() */
        /* Réservé à offset 8 (uint64_t) */
        
        /* CRC32C du header (à offset 56) */
        uint32_t header_crc = compute_crc32c(header, 56);
        memcpy(&header[56], &header_crc, sizeof(uint32_t));
        
        /* Écrit le header */
        if (write(ctx->output_fd, header, 64) != 64) {
            close(ctx->output_fd);
            ctx->output_fd = -1;
            return -1;
        }
    }
    
    ctx->start_ts_ns = get_timestamp_ns();
    ctx->is_active = true;
    
    return 0;
}

int nx_trace_stop(nx_trace_context_t* ctx) {
    if (!ctx) return -1;
    if (!ctx->is_active) return 0;
    
    /* Flush les événements restants */
    nx_trace_flush(ctx);
    
    ctx->is_active = false;
    
    if (ctx->output_fd >= 0) {
        /* Met à jour le count dans le header */
        uint64_t count = ctx->total_events;
        lseek(ctx->output_fd, 8, SEEK_SET);  /* Offset 8 = count */
        ssize_t written = write(ctx->output_fd, &count, sizeof(uint64_t));
        (void)written;  /* Ignore warning */
        
        /* Recalcule et met à jour le CRC32C du header */
        uint8_t header[64];
        lseek(ctx->output_fd, 0, SEEK_SET);
        ssize_t bytes_read = read(ctx->output_fd, header, 64);
        (void)bytes_read;  /* Ignore warning */
        uint32_t header_crc = compute_crc32c(header, 56);
        lseek(ctx->output_fd, 56, SEEK_SET);
        written = write(ctx->output_fd, &header_crc, sizeof(uint32_t));
        (void)written;  /* Ignore warning */
        
        /* Calcule le SHA-256 witness de tout le fichier */
        lseek(ctx->output_fd, 0, SEEK_SET);
        
        /* TODO: Implémenter SHA-256 du fichier complet */
        /* Pour l'instant, on écrit un placeholder */
        uint8_t sha256_witness[32] = {0};
        lseek(ctx->output_fd, 0, SEEK_END);
        written = write(ctx->output_fd, sha256_witness, 32);
        (void)written;  /* Ignore warning */
        
        close(ctx->output_fd);
        ctx->output_fd = -1;
    }
    
    return 0;
}

int nx_trace_record(nx_trace_context_t* ctx, const nx_trace_event_t* event) {
    if (!ctx || !event) return -1;
    if (!ctx->is_active) return -1;
    
    /* Échantillonnage */
    if (ctx->config.sampling_rate > 1) {
        if (ctx->total_events % ctx->config.sampling_rate != 0) {
            ctx->total_events++;
            return 0;
        }
    }
    
    /* Copie l'événement dans le buffer */
    nx_trace_event_t* slot = &ctx->buffer[ctx->buffer_index];
    memcpy(slot, event, sizeof(nx_trace_event_t));
    
    /* Calcule le CRC32C */
    slot->crc32c = compute_crc32c(slot, sizeof(nx_trace_event_t) - sizeof(uint32_t));
    
    ctx->buffer_index++;
    ctx->total_events++;
    
    /* Flush si buffer plein */
    if (ctx->buffer_index >= ctx->buffer_size) {
        return nx_trace_flush(ctx);
    }
    
    return 0;
}

int nx_trace_record_semantic(nx_trace_context_t* ctx, const char* label, uint64_t memory_addr) {
    if (!ctx || !label) return -1;
    
    nx_trace_event_t event = {0};
    event.ts_ns = get_timestamp_ns();
    event.thread_id = get_thread_id();
    event.cpu_id = get_cpu_id();
    event.memory_address = memory_addr;
    event.event_type = NX_EVENT_CUSTOM;
    
    strncpy(event.semantic_label, label, sizeof(event.semantic_label) - 1);
    
    return nx_trace_record(ctx, &event);
}

int nx_trace_flush(nx_trace_context_t* ctx) {
    if (!ctx) return -1;
    if (ctx->buffer_index == 0) return 0;
    if (ctx->output_fd < 0) return 0;
    
    size_t bytes_to_write = ctx->buffer_index * sizeof(nx_trace_event_t);
    ssize_t written = write(ctx->output_fd, ctx->buffer, bytes_to_write);
    
    if (written < 0) {
        return -1;
    }
    
    int events_written = ctx->buffer_index;
    ctx->buffer_index = 0;
    
    return events_written;
}

void nx_trace_destroy(nx_trace_context_t* ctx) {
    if (!ctx) return;
    
    if (ctx->is_active) {
        nx_trace_stop(ctx);
    }
    
    if (ctx->buffer) {
        free(ctx->buffer);
    }
    
    free(ctx);
}

int nx_trace_get_stats(const nx_trace_context_t* ctx, uint64_t* total_events, float* buffer_usage) {
    if (!ctx) return -1;
    
    if (total_events) {
        *total_events = ctx->total_events;
    }
    
    if (buffer_usage) {
        *buffer_usage = (float)ctx->buffer_index / (float)ctx->buffer_size * 100.0f;
    }
    
    return 0;
}

int nx_trace_reconstruct_causal_graph(const char* trace_file, const char* output_dot) {
    if (!trace_file || !output_dot) return -1;
    
    int fd = open(trace_file, O_RDONLY);
    if (fd < 0) return -1;
    
    FILE* dot = fopen(output_dot, "w");
    if (!dot) {
        close(fd);
        return -1;
    }
    
    fprintf(dot, "digraph causal_graph {\n");
    fprintf(dot, "  rankdir=TB;\n");
    fprintf(dot, "  node [shape=box];\n\n");
    
    nx_trace_event_t event;
    uint64_t event_id = 0;
    
    while (read(fd, &event, sizeof(event)) == sizeof(event)) {
        /* Vérifie l'intégrité */
        uint32_t expected_crc = compute_crc32c(&event, sizeof(event) - sizeof(uint32_t));
        if (expected_crc != event.crc32c) {
            fprintf(stderr, "CRC mismatch at event %lu\n", event_id);
            continue;
        }
        
        /* Ajoute le nœud */
        fprintf(dot, "  event_%lu [label=\"%s\\nts=%lu ns\\nTID=%u\"];\n",
                event_id, event.semantic_label, event.ts_ns, event.thread_id);
        
        /* Ajoute l'arête si parent existe */
        if (event.causal_parent[0] != '\0') {
            fprintf(dot, "  %s -> event_%lu;\n", event.causal_parent, event_id);
        }
        
        event_id++;
    }
    
    fprintf(dot, "}\n");
    
    fclose(dot);
    close(fd);
    
    return (int)event_id;
}

int nx_trace_replay(const char* trace_file, 
                    void (*callback)(const nx_trace_event_t*, void*),
                    void* user_data) {
    if (!trace_file || !callback) return -1;
    
    int fd = open(trace_file, O_RDONLY);
    if (fd < 0) return -1;
    
    nx_trace_event_t event;
    int count = 0;
    
    while (read(fd, &event, sizeof(event)) == sizeof(event)) {
        /* Vérifie l'intégrité */
        uint32_t expected_crc = compute_crc32c(&event, sizeof(event) - sizeof(uint32_t));
        if (expected_crc != event.crc32c) {
            fprintf(stderr, "CRC mismatch at event %d\n", count);
            continue;
        }
        
        callback(&event, user_data);
        count++;
    }
    
    close(fd);
    return count;
}

// Made with Bob
