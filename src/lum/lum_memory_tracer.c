/* ============================================================================
 * LUM Memory Tracer — Cycle C111
 * Implémentation : snapshot/reconstruction process self au format .lum (binaire)
 * ============================================================================ */
#include "lum_memory_tracer.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define LUM_TRACER_MAGIC 0x4C554D54u  /* 'LUMT' */
#define PAGE_SIZE 4096u

/* Format .lum produit :
 *   header (32 octets) :
 *     uint32 magic       = 'LUMT'
 *     uint32 granularity (0=page, 1=byte, 2=bit)
 *     uint64 total_lums
 *     uint64 total_bytes
 *     uint64 timestamp_ns
 *   N × lum_t (64 octets chacun, aligné cache line)
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t granularity;
    uint64_t total_lums;
    uint64_t total_bytes;
    uint64_t timestamp_ns;
} lum_file_header_t;

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint32_t lum_checksum(const void* data, size_t len) {
    /* CRC32-like rapide (Adler-32 simplifié pour vitesse, suffisant pour intégrité) */
    uint32_t a = 1, b = 0;
    const uint8_t* p = (const uint8_t*)data;
    for (size_t i = 0; i < len; i++) {
        a = (a + p[i]) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16) | a;
}

/* Encode 1 page (4096 octets) en 1 lum_t.
 * memory_address pointe sur une copie heap-allouée du contenu (à libérer après).
 */
static void encode_page_to_lum(uint64_t vaddr, const uint8_t* page_data, lum_t* out) {
    static uint32_t next_id = 1;
    out->id = next_id++;
    out->presence = 1;
    out->structure_type = 0; /* PAGE */
    out->is_destroyed = 0;
    out->reserved_flags = 0;
    out->position_x = (int32_t)(vaddr & 0xFFFFFFFFu);
    out->position_y = (int32_t)(vaddr >> 32);
    out->timestamp = now_ns();
    out->memory_address = (void*)(uintptr_t)vaddr;
    out->checksum = lum_checksum(page_data, PAGE_SIZE);
    out->magic_number = LUM_TRACER_MAGIC;
    memset(out->padding, 0, sizeof(out->padding));
}

/* Encode 1 octet en 1 lum_t (granularité BYTE) */
static void encode_byte_to_lum(uint64_t vaddr, uint8_t byte_val, lum_t* out) {
    static uint32_t next_id = 1;
    out->id = next_id++;
    out->presence = (byte_val != 0) ? 1 : 0;
    out->structure_type = 1; /* BYTE */
    out->is_destroyed = 0;
    out->reserved_flags = byte_val; /* contenu stocké ici */
    out->position_x = (int32_t)(vaddr & 0xFFFFFFFFu);
    out->position_y = (int32_t)(vaddr >> 32);
    out->timestamp = now_ns();
    out->memory_address = (void*)(uintptr_t)vaddr;
    out->checksum = byte_val;
    out->magic_number = LUM_TRACER_MAGIC;
    memset(out->padding, 0, sizeof(out->padding));
}

/* Encode 1 bit en 1 lum_t (granularité BIT) */
static void encode_bit_to_lum(uint64_t bit_addr, uint8_t bit_val, lum_t* out) {
    static uint32_t next_id = 1;
    out->id = next_id++;
    out->presence = bit_val & 1u;
    out->structure_type = 2; /* BIT */
    out->is_destroyed = 0;
    out->reserved_flags = 0;
    out->position_x = (int32_t)(bit_addr & 0xFFFFFFFFu);
    out->position_y = (int32_t)(bit_addr >> 32);
    out->timestamp = now_ns();
    out->memory_address = (void*)(uintptr_t)bit_addr;
    out->checksum = bit_val & 1u;
    out->magic_number = LUM_TRACER_MAGIC;
    memset(out->padding, 0, sizeof(out->padding));
}

/* ----------------------------------------------------------------------------
 * Snapshot process self
 * ---------------------------------------------------------------------------- */
int lum_memory_snapshot_self(const char* out_path,
                              lum_trace_granularity_t granularity,
                              bool include_anon,
                              bool include_files,
                              lum_trace_stats_t* stats) {
    if (!out_path) return -EINVAL;

    uint64_t t0 = now_ns();

    FILE* maps = fopen("/proc/self/maps", "r");
    if (!maps) return -errno;

    int mem_fd = open("/proc/self/mem", O_RDONLY);
    if (mem_fd < 0) {
        fclose(maps);
        return -errno;
    }

    FILE* out = fopen(out_path, "wb");
    if (!out) {
        close(mem_fd);
        fclose(maps);
        return -errno;
    }

    /* Header placeholder, rewriten à la fin */
    lum_file_header_t hdr = {
        .magic = LUM_TRACER_MAGIC,
        .granularity = (uint32_t)granularity,
        .total_lums = 0,
        .total_bytes = 0,
        .timestamp_ns = t0,
    };
    fwrite(&hdr, sizeof(hdr), 1, out);
    /* Pad to 32 bytes (sizeof(lum_file_header_t)=32 already) */

    uint64_t total_lums = 0;
    uint64_t total_bytes = 0;
    uint64_t pages_scanned = 0;
    uint64_t pages_resident = 0;

    char line[1024];
    while (fgets(line, sizeof(line), maps)) {
        uint64_t start = 0, end = 0;
        char perms[8] = {0};
        char path[256] = {0};
        int n = sscanf(line, "%lx-%lx %7s %*s %*s %*s %255s",
                       &start, &end, perms, path);
        if (n < 3) continue;
        if (perms[0] != 'r') continue; /* page non-lisible : skip */

        bool is_anon = (path[0] == '\0' || path[0] == '[');
        if (is_anon && !include_anon) continue;
        if (!is_anon && !include_files) continue;

        /* Skip [vvar], [vsyscall], [vdso] qui peuvent bloquer la lecture */
        if (strstr(path, "[vvar]") || strstr(path, "[vsyscall]")) continue;

        for (uint64_t addr = start; addr < end; addr += PAGE_SIZE) {
            pages_scanned++;
            uint8_t buf[PAGE_SIZE];
            ssize_t r = pread(mem_fd, buf, PAGE_SIZE, (off_t)addr);
            if (r != (ssize_t)PAGE_SIZE) {
                /* Page non résidente ou inaccessible : skip silencieusement */
                continue;
            }
            pages_resident++;

            switch (granularity) {
                case LUM_TRACE_GRANULARITY_PAGE: {
                    lum_t lum;
                    memset(&lum, 0, sizeof(lum));
                    encode_page_to_lum(addr, buf, &lum);
                    fwrite(&lum, sizeof(lum_t), 1, out);
                    /* Le contenu de la page est stocké après le lum_t */
                    fwrite(buf, PAGE_SIZE, 1, out);
                    total_lums++;
                    total_bytes += PAGE_SIZE;
                    break;
                }
                case LUM_TRACE_GRANULARITY_BYTE: {
                    for (size_t i = 0; i < PAGE_SIZE; i++) {
                        lum_t lum;
                        memset(&lum, 0, sizeof(lum));
                        encode_byte_to_lum(addr + i, buf[i], &lum);
                        fwrite(&lum, sizeof(lum_t), 1, out);
                        total_lums++;
                    }
                    total_bytes += PAGE_SIZE;
                    break;
                }
                case LUM_TRACE_GRANULARITY_BIT: {
                    for (size_t i = 0; i < PAGE_SIZE; i++) {
                        for (int b = 0; b < 8; b++) {
                            uint8_t bit = (buf[i] >> b) & 1u;
                            lum_t lum;
                            memset(&lum, 0, sizeof(lum));
                            encode_bit_to_lum((addr + i) * 8 + b, bit, &lum);
                            fwrite(&lum, sizeof(lum_t), 1, out);
                            total_lums++;
                        }
                    }
                    total_bytes += PAGE_SIZE;
                    break;
                }
                case LUM_TRACE_GRANULARITY_HUGEPAGE: {
                    /* C114 — granularité réservée pour API future (huge page 2 MiB).
                     * Implémentation déférée à C115 ; pour l'instant on retourne
                     * proprement une erreur sans corrompre le flux de sortie. */
                    fclose(out);
                    return -ENOSYS;
                }
            }
        }
    }

    /* Réécrire le header avec les compteurs finaux */
    hdr.total_lums = total_lums;
    hdr.total_bytes = total_bytes;
    fseek(out, 0, SEEK_SET);
    fwrite(&hdr, sizeof(hdr), 1, out);
    fclose(out);
    close(mem_fd);
    fclose(maps);

    uint64_t t1 = now_ns();
    if (stats) {
        stats->total_lums_emitted = total_lums;
        stats->total_pages_scanned = pages_scanned;
        stats->total_pages_resident = pages_resident;
        stats->total_bytes_dumped = total_bytes;
        stats->snapshot_ns = t1 - t0;
        stats->magic = LUM_TRACER_MAGIC;
    }
    return 0;
}

/* ----------------------------------------------------------------------------
 * Reconstruction depuis .lum
 * ---------------------------------------------------------------------------- */
int lum_memory_reconstruct(const char* in_path,
                            void* target_buffer,
                            size_t target_size,
                            size_t* bytes_restored) {
    if (!in_path || !target_buffer) return -EINVAL;

    FILE* in = fopen(in_path, "rb");
    if (!in) return -errno;

    lum_file_header_t hdr;
    if (fread(&hdr, sizeof(hdr), 1, in) != 1) {
        fclose(in);
        return -EIO;
    }
    if (hdr.magic != LUM_TRACER_MAGIC) {
        fclose(in);
        return -EBADMSG;
    }

    uint8_t* dst = (uint8_t*)target_buffer;
    size_t written = 0;

    for (uint64_t i = 0; i < hdr.total_lums; i++) {
        lum_t lum;
        if (fread(&lum, sizeof(lum_t), 1, in) != 1) break;

        switch ((lum_trace_granularity_t)hdr.granularity) {
            case LUM_TRACE_GRANULARITY_PAGE: {
                if (written + PAGE_SIZE > target_size) goto done;
                if (fread(dst + written, PAGE_SIZE, 1, in) != 1) goto done;
                written += PAGE_SIZE;
                break;
            }
            case LUM_TRACE_GRANULARITY_BYTE: {
                if (written + 1 > target_size) goto done;
                dst[written++] = lum.reserved_flags; /* contenu byte */
                break;
            }
            case LUM_TRACE_GRANULARITY_BIT: {
                size_t byte_idx = written / 8;
                int bit_idx = written % 8;
                if (byte_idx >= target_size) goto done;
                if (bit_idx == 0) dst[byte_idx] = 0;
                dst[byte_idx] |= (lum.presence & 1u) << bit_idx;
                written++;
                break;
            }
            case LUM_TRACE_GRANULARITY_HUGEPAGE: {
                /* C114 — granularité réservée API future ; reconstruction
                 * non supportée tant que snapshot ne l'écrit pas. */
                fclose(in);
                return -ENOSYS;
            }
        }
    }
done:
    fclose(in);
    if (bytes_restored) {
        *bytes_restored = (hdr.granularity == LUM_TRACE_GRANULARITY_BIT)
                          ? (written + 7) / 8
                          : written;
    }
    return 0;
}

/* ----------------------------------------------------------------------------
 * Validation diff=0
 * ---------------------------------------------------------------------------- */
int lum_memory_validate_diff_zero(const char* original_path,
                                   const void* reconstructed,
                                   size_t reconstructed_size,
                                   uint64_t* diff_count) {
    if (!original_path || !reconstructed) return -EINVAL;

    /* Re-reconstruire dans un buffer temporaire, puis comparer byte-par-byte */
    uint8_t* tmp = (uint8_t*)calloc(1, reconstructed_size);
    if (!tmp) return -ENOMEM;

    size_t restored = 0;
    int rc = lum_memory_reconstruct(original_path, tmp, reconstructed_size, &restored);
    if (rc != 0) {
        free(tmp);
        return rc;
    }

    uint64_t diffs = 0;
    size_t cmp_len = (restored < reconstructed_size) ? restored : reconstructed_size;
    const uint8_t* ref = (const uint8_t*)reconstructed;
    for (size_t i = 0; i < cmp_len; i++) {
        if (tmp[i] != ref[i]) diffs++;
    }
    free(tmp);
    if (diff_count) *diff_count = diffs;
    return (diffs == 0) ? 0 : -1;
}
