/* ============================================================================
 * LUM Log Encoder — Cycle C111
 * Implémentation : encode logs en séquence de lum_t (64 octets) append-only
 * ============================================================================ */
#include "lum_log_encoder.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define LUM_LOG_MAGIC 0x4C554D4Cu  /* 'LUML' */

struct lum_log_writer {
    FILE* fp;
    pthread_mutex_t mu;
    uint32_t next_id;
    uint64_t events_written;
};

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint32_t fast_checksum(const void* data, size_t len) {
    uint32_t h = 2166136261u; /* FNV-1a */
    const uint8_t* p = (const uint8_t*)data;
    for (size_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

lum_log_writer_t* lum_log_writer_open(const char* lum_path) {
    if (!lum_path) return NULL;
    lum_log_writer_t* w = (lum_log_writer_t*)calloc(1, sizeof(*w));
    if (!w) return NULL;
    w->fp = fopen(lum_path, "ab");
    if (!w->fp) {
        free(w);
        return NULL;
    }
    pthread_mutex_init(&w->mu, NULL);
    w->next_id = 1;
    w->events_written = 0;
    return w;
}

static int write_lum(lum_log_writer_t* w, lum_log_kind_t kind,
                     const void* payload, size_t payload_len) {
    if (!w) return -EINVAL;
    pthread_mutex_lock(&w->mu);

    lum_t lum;
    memset(&lum, 0, sizeof(lum));
    lum.id = w->next_id++;
    lum.presence = 1;
    lum.structure_type = (uint8_t)kind;
    lum.is_destroyed = 0;
    lum.reserved_flags = (uint8_t)(payload_len & 0xFF);
    lum.position_x = (int32_t)(w->events_written & 0xFFFFFFFFu);
    lum.position_y = (int32_t)(w->events_written >> 32);
    lum.timestamp = now_ns();
    lum.memory_address = NULL;
    lum.checksum = fast_checksum(payload, payload_len);
    lum.magic_number = LUM_LOG_MAGIC;

    /* Stocker payload jusqu'à 20 octets dans padding (lum_t = 64 octets total) */
    size_t inline_len = (payload_len > sizeof(lum.padding)) ? sizeof(lum.padding)
                                                            : payload_len;
    memcpy(lum.padding, payload, inline_len);

    fwrite(&lum, sizeof(lum_t), 1, w->fp);

    /* Si payload > 20 octets, écrire le surplus en lum_t supplémentaires */
    if (payload_len > inline_len) {
        const uint8_t* p = (const uint8_t*)payload + inline_len;
        size_t remaining = payload_len - inline_len;
        while (remaining > 0) {
            lum_t cont;
            memset(&cont, 0, sizeof(cont));
            cont.id = w->next_id++;
            cont.presence = 1;
            cont.structure_type = 0xFF; /* CONTINUATION marker */
            cont.timestamp = lum.timestamp;
            cont.magic_number = LUM_LOG_MAGIC;
            size_t chunk = (remaining > sizeof(cont.padding) + 12)
                           ? sizeof(cont.padding) + 12
                           : remaining;
            /* Utilise position_x/y + padding pour stocker payload (20+8=28 octets) */
            memcpy(&cont.position_x, p, chunk > 8 ? 8 : chunk);
            if (chunk > 8) memcpy(cont.padding, p + 8, chunk - 8);
            fwrite(&cont, sizeof(lum_t), 1, w->fp);
            p += chunk;
            remaining -= chunk;
        }
    }

    w->events_written++;
    pthread_mutex_unlock(&w->mu);
    return 0;
}

int lum_log_writer_write_text(lum_log_writer_t* w,
                               lum_log_kind_t kind,
                               const char* text) {
    if (!text) return -EINVAL;
    return write_lum(w, kind, text, strlen(text));
}

int lum_log_writer_write_metric(lum_log_writer_t* w,
                                 const char* metric_name,
                                 double value) {
    if (!metric_name) return -EINVAL;
    char buf[256];
    int n = snprintf(buf, sizeof(buf), "%s=%.17g", metric_name, value);
    if (n <= 0) return -EIO;
    return write_lum(w, LUM_LOG_METRIC, buf, (size_t)n);
}

int lum_log_writer_write_record(lum_log_writer_t* w,
                                 const char* record_name,
                                 uint64_t value) {
    if (!record_name) return -EINVAL;
    char buf[256];
    int n = snprintf(buf, sizeof(buf), "%s=%lu", record_name, (unsigned long)value);
    if (n <= 0) return -EIO;
    return write_lum(w, LUM_LOG_RECORD, buf, (size_t)n);
}

int lum_log_writer_close(lum_log_writer_t* w) {
    if (!w) return -EINVAL;
    pthread_mutex_lock(&w->mu);
    if (w->fp) {
        fflush(w->fp);
        fsync(fileno(w->fp));
        fclose(w->fp);
        w->fp = NULL;
    }
    pthread_mutex_unlock(&w->mu);
    pthread_mutex_destroy(&w->mu);
    free(w);
    return 0;
}
