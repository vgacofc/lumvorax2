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

/* C117-P4 — Unification timestamps : CLOCK_MONOTONIC_RAW (immune NTP slew).
 * Avant C117, log_encoder utilisait CLOCK_REALTIME (incohérent avec
 * lum_memory_tracer qui utilisait CLOCK_MONOTONIC). Depuis C117, tous les
 * timestamps lum_t.timestamp utilisent la même base monotonic raw, ce qui
 * permet de corréler/ordonner les évènements entre fichiers .lum sans
 * artéfact de saut horloge (NTP, leap second, manual time set). */
static uint64_t now_ns(void) {
    struct timespec ts;
#ifdef CLOCK_MONOTONIC_RAW
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) == 0)
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* C117-P3 — CRC32C (Castagnoli) — hardware SSE4.2 si dispo, sinon software.
 * Beaucoup plus robuste que FNV-1a/XOR contre collisions. Identique à celui
 * utilisé par lum_memory_tracer.c → garantit interopérabilité des checksums
 * entre les deux modules d'encodage LUM. */
static uint32_t fast_checksum(const void* data, size_t len) {
    uint32_t crc = ~0u;
    const uint8_t* p = (const uint8_t*)data;
#if defined(__SSE4_2__) && (defined(__x86_64__) || defined(__i386__))
    while (len >= 8) {
        crc = (uint32_t)__builtin_ia32_crc32di(crc, *(const uint64_t*)p);
        p += 8; len -= 8;
    }
    while (len >= 1) {
        crc = __builtin_ia32_crc32qi(crc, *p++);
        len--;
    }
#else
    /* Fallback software, polynôme reflected 0x82F63B78 (Castagnoli) */
    while (len--) {
        crc ^= *p++;
        for (int i = 0; i < 8; i++)
            crc = (crc >> 1) ^ (0x82F63B78u & -(int32_t)(crc & 1u));
    }
#endif
    return ~crc;
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

/* C118-Q6 — Court-circuit global LUM_LOGGING env var.
 * Si LUM_LOGGING=0 (lu une seule fois au premier appel), toutes les écritures
 * lum_log_writer_write_* deviennent des no-op. Permet de mesurer proprement
 * l'overhead réel du sous-système LUM (test A/B preconisé rapport C117 §9 / Q6).
 * Référence : RAPPORT_C117_ANALYSE §9 — overhead 4.5% jamais mesuré proprement. */
static int lum_logging_check_env(void) {
    static int cached = -1;
    if (cached == -1) {
        const char* env = getenv("LUM_LOGGING");
        cached = (env && env[0] == '0' && env[1] == '\0') ? 0 : 1;
    }
    return cached;
}

static int write_lum(lum_log_writer_t* w, lum_log_kind_t kind,
                     const void* payload, size_t payload_len) {
    if (!w) return -EINVAL;
    /* C118-Q6 : court-circuit si LUM_LOGGING=0 (mesure overhead A/B) */
    if (!lum_logging_check_env()) return 0;
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
    /* C116-P3 : payload_total_len stocké dans memory_address (uint64 via cast).
     * Simplifie le parseur de continuations : le parseur n'a plus besoin de
     * compter les lum_t suivants pour connaître la taille totale. */
    lum.memory_address = (void*)(uintptr_t)payload_len;
    lum.checksum = fast_checksum(payload, payload_len);
    lum.magic_number = LUM_LOG_MAGIC;

    /* Stocker payload jusqu'à sizeof(lum.padding) octets inline */
    size_t inline_len = (payload_len > sizeof(lum.padding)) ? sizeof(lum.padding)
                                                            : payload_len;
    memcpy(lum.padding, payload, inline_len);

    fwrite(&lum, sizeof(lum_t), 1, w->fp);
    /* C116-P3 FIX BUG : flush immédiat pour éviter perte de données si SIGTERM.
     * Sans ce flush, le buffer stdio peut rester non vidé → fichier .lum = 0 octets
     * (reproduit sur Ubuntu run C115 : btc_lum_log_*.lum = 0B). */
    fflush(w->fp);

    /* Si payload > inline, écrire le surplus en lum_t de continuation */
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
            /* C116-P3 : stocker l'offset dans memory_address pour reconstruire l'ordre */
            cont.memory_address = (void*)(uintptr_t)(payload_len - remaining);
            size_t chunk = (remaining > sizeof(cont.padding) + 12)
                           ? sizeof(cont.padding) + 12
                           : remaining;
            memcpy(&cont.position_x, p, chunk > 8 ? 8 : chunk);
            if (chunk > 8) memcpy(cont.padding, p + 8, chunk - 8);
            fwrite(&cont, sizeof(lum_t), 1, w->fp);
            fflush(w->fp);
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
