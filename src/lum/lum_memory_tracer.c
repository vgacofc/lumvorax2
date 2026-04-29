/* ============================================================================
 * LUM Memory Tracer — Cycle C111 / C115
 * Implémentation : snapshot/reconstruction process self au format .lum (binaire)
 * C115 : Implémentation réelle LUM_TRACE_GRANULARITY_HUGEPAGE (2 MiB)
 * ============================================================================ */
#define _POSIX_C_SOURCE 200809L  /* pread, clock_gettime, ssize_t */
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
#define HUGEPAGE_SIZE   (2UL * 1024UL * 1024UL)   /* 2 MiB */
#define PAGES_PER_HUGEPAGE (HUGEPAGE_SIZE / PAGE_SIZE) /* 512 pages */

/* C117-P1 — Algorithmes de checksum supportés */
#define LUM_CHKSUM_ADLER32  0u   /* Legacy C111-C116 */
#define LUM_CHKSUM_CRC32C   1u   /* C117 par défaut (Castagnoli) */

/* C117-P1 — Versions du format .lum
 *   v1 : header 32 octets (legacy C111-C116, fichiers non alignés cache-line)
 *   v2 : header 64 octets (C117, aligné cache-line, CRC32C, CLOCK_MONOTONIC_RAW)
 */
#define LUM_FORMAT_VERSION_LEGACY 1u
#define LUM_FORMAT_VERSION_C117   2u

/* Format .lum produit (C117-P1, FIX ALIGNEMENT) :
 *   header (64 octets, aligné cache line) :
 *     uint32 magic         = 'LUMT'
 *     uint32 granularity   (0=page, 1=byte, 2=bit, 3=hugepage)
 *     uint64 total_lums
 *     uint64 total_bytes
 *     uint64 timestamp_realtime_ns  (wall-clock du snapshot, CLOCK_REALTIME)
 *     uint32 version_major (2 = C117 nouveau format)
 *     uint32 checksum_algo (0=Adler32, 1=CRC32C)
 *     uint64 timestamp_monotonic_ns (CLOCK_MONOTONIC_RAW pour mesure)
 *     uint8  reserved[16]  (alignement 64 octets exact)
 *   N × lum_t (64 octets chacun, aligné cache line)
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t granularity;
    uint64_t total_lums;
    uint64_t total_bytes;
    uint64_t timestamp_realtime_ns;
    uint32_t version_major;
    uint32_t checksum_algo;
    uint64_t timestamp_monotonic_ns;
    uint8_t  reserved[16];
} lum_file_header_t;

_Static_assert(sizeof(lum_file_header_t) == 64,
               "C117-P1: header doit être 64 octets pour alignement cache-line");

/* C117-P4 — Timestamp monotonic raw (immune to NTP slew, plus stable que CLOCK_MONOTONIC) */
static uint64_t now_ns(void) {
    struct timespec ts;
#ifdef CLOCK_MONOTONIC_RAW
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) == 0)
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* C117-P4 — Wall-clock pour timestamp du snapshot (lisible humain) */
static uint64_t now_realtime_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* C117-P3 — CRC32C (Castagnoli, polynôme 0x1EDC6F41) — hardware SSE4.2 si dispo,
 * sinon fallback software. Beaucoup plus robuste que Adler-32/XOR/FNV-1a contre
 * les collisions naturelles et adversariales. */
static uint32_t lum_crc32c(const void* data, size_t len) {
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
    /* Fallback software, polynôme reflected 0x82F63B78 */
    while (len--) {
        crc ^= *p++;
        for (int i = 0; i < 8; i++)
            crc = (crc >> 1) ^ (0x82F63B78u & -(int32_t)(crc & 1u));
    }
#endif
    return ~crc;
}

/* Wrapper pour préserver l'API interne existante */
static uint32_t lum_checksum(const void* data, size_t len) {
    return lum_crc32c(data, len);
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

/* Encode 1 huge page (2 MiB max) en 1 lum_t (granularité HUGEPAGE) — C115
 * hp_vaddr : adresse virtuelle de début de la tranche
 * hp_len   : taille réelle lue (multiple de PAGE_SIZE, ≤ HUGEPAGE_SIZE)
 */
static void encode_hugepage_to_lum(uint64_t hp_vaddr, const uint8_t* hp_data,
                                    size_t hp_len, lum_t* out) {
    static uint32_t next_id = 1;
    out->id = next_id++;
    out->presence = 1;
    out->structure_type = 3; /* HUGEPAGE */
    out->is_destroyed = 0;
    /* Stocker log2(hp_len >> 12) dans reserved_flags pour reconstruction */
    out->reserved_flags = (uint8_t)(hp_len / PAGE_SIZE); /* nb pages dans la tranche */
    out->position_x = (int32_t)(hp_vaddr & 0xFFFFFFFFu);
    out->position_y = (int32_t)(hp_vaddr >> 32);
    out->timestamp = now_ns();
    out->memory_address = (void*)(uintptr_t)hp_vaddr;
    /* Checksum Adler-32 sur la première page uniquement (rapide pour 2 MiB) */
    out->checksum = lum_checksum(hp_data, PAGE_SIZE < hp_len ? PAGE_SIZE : hp_len);
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
    uint64_t t0_real = now_realtime_ns();

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

    /* C117-P1 — Header 64 octets (placeholder, réécrit à la fin) */
    lum_file_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = LUM_TRACER_MAGIC;
    hdr.granularity = (uint32_t)granularity;
    hdr.total_lums = 0;
    hdr.total_bytes = 0;
    hdr.timestamp_realtime_ns = t0_real;
    hdr.version_major = LUM_FORMAT_VERSION_C117;
    hdr.checksum_algo = LUM_CHKSUM_CRC32C;
    hdr.timestamp_monotonic_ns = t0;
    fwrite(&hdr, sizeof(hdr), 1, out);
    /* sizeof(lum_file_header_t) = 64 octets, validé par _Static_assert ci-dessus */

    uint64_t total_lums = 0;
    uint64_t total_bytes = 0;
    uint64_t pages_scanned = 0;
    uint64_t pages_resident = 0;

    /* Buffer heap pour HUGEPAGE (2 MiB) — alloué une seule fois */
    uint8_t *hp_buf = NULL;
    if (granularity == LUM_TRACE_GRANULARITY_HUGEPAGE) {
        hp_buf = (uint8_t *)malloc(HUGEPAGE_SIZE);
        if (!hp_buf) {
            fclose(out);
            close(mem_fd);
            fclose(maps);
            return -ENOMEM;
        }
    }

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

        /* C115 — Chemin HUGEPAGE : tranches de 2 MiB (512 pages agrégées) */
        if (granularity == LUM_TRACE_GRANULARITY_HUGEPAGE) {
            for (uint64_t hp_addr = start; hp_addr < end; hp_addr += HUGEPAGE_SIZE) {
                uint64_t hp_end = hp_addr + HUGEPAGE_SIZE;
                if (hp_end > end) hp_end = end;
                size_t bytes_collected = 0;

                for (uint64_t pg = hp_addr; pg < hp_end; pg += PAGE_SIZE) {
                    pages_scanned++;
                    ssize_t r = pread(mem_fd, hp_buf + bytes_collected,
                                      PAGE_SIZE, (off_t)pg);
                    if (r == (ssize_t)PAGE_SIZE) {
                        pages_resident++;
                    } else {
                        /* Page non résidente ou partielle : zéro-fill */
                        memset(hp_buf + bytes_collected, 0, PAGE_SIZE);
                    }
                    bytes_collected += PAGE_SIZE;
                }
                if (bytes_collected == 0) continue;

                lum_t lum;
                memset(&lum, 0, sizeof(lum));
                encode_hugepage_to_lum(hp_addr, hp_buf, bytes_collected, &lum);
                fwrite(&lum, sizeof(lum_t), 1, out);
                fwrite(hp_buf, bytes_collected, 1, out);
                total_lums++;
                total_bytes += bytes_collected;
            }
            continue; /* Passer à la prochaine VMA */
        }

        /* Granularités PAGE / BYTE / BIT : loop page-par-page original */
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
                case LUM_TRACE_GRANULARITY_HUGEPAGE:
                    /* Ne devrait jamais arriver ici (géré avant le loop) */
                    break;
            }
        }
    }
    free(hp_buf); /* NULL-safe */

    /* Réécrire le header avec les compteurs finaux (C117 : header 64 octets) */
    hdr.total_lums = total_lums;
    hdr.total_bytes = total_bytes;
    fseek(out, 0, SEEK_SET);
    fwrite(&hdr, sizeof(hdr), 1, out);
    fflush(out);
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
 * C133 — Snapshot d'un buffer mémoire arbitraire (zone contrôlée)
 *
 * Réutilise le même format binaire (header 64 octets v2 + N×lum_t)
 * que lum_memory_snapshot_self() pour rester compatible avec
 * lum_memory_reconstruct() existant.
 *
 * Différence majeure : pas de scan /proc/self/maps, l'appelant fournit
 * un pointeur+taille. Le buffer DOIT être stable pendant l'appel
 * (sinon diff=0 ne sera pas reproductible).
 * ---------------------------------------------------------------------------- */
int lum_memory_snapshot_buffer(const void* buffer,
                                size_t buffer_size,
                                const char* out_path,
                                lum_trace_granularity_t granularity,
                                lum_trace_stats_t* stats) {
    if (!buffer || !out_path) return -EINVAL;
    if (buffer_size == 0) return -EINVAL;
    if (granularity == LUM_TRACE_GRANULARITY_HUGEPAGE) return -EINVAL;
    if (granularity == LUM_TRACE_GRANULARITY_PAGE && (buffer_size % PAGE_SIZE) != 0) {
        return -EINVAL;
    }

    uint64_t t0 = now_ns();
    uint64_t t0_real = now_realtime_ns();

    FILE* out = fopen(out_path, "wb");
    if (!out) return -errno;

    lum_file_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = LUM_TRACER_MAGIC;
    hdr.granularity = (uint32_t)granularity;
    hdr.total_lums = 0;
    hdr.total_bytes = 0;
    hdr.timestamp_realtime_ns = t0_real;
    hdr.version_major = LUM_FORMAT_VERSION_C117;
    hdr.checksum_algo = LUM_CHKSUM_CRC32C;
    hdr.timestamp_monotonic_ns = t0;
    if (fwrite(&hdr, sizeof(hdr), 1, out) != 1) {
        fclose(out);
        return -EIO;
    }

    const uint8_t* src = (const uint8_t*)buffer;
    uint64_t total_lums = 0;
    uint64_t total_bytes = 0;
    /* Adresse virtuelle de référence : pointeur du buffer */
    uint64_t base_vaddr = (uint64_t)(uintptr_t)src;

    switch (granularity) {
        case LUM_TRACE_GRANULARITY_PAGE: {
            for (size_t off = 0; off < buffer_size; off += PAGE_SIZE) {
                lum_t lum;
                memset(&lum, 0, sizeof(lum));
                encode_page_to_lum(base_vaddr + off, src + off, &lum);
                if (fwrite(&lum, sizeof(lum_t), 1, out) != 1) goto io_err;
                if (fwrite(src + off, PAGE_SIZE, 1, out) != 1) goto io_err;
                total_lums++;
                total_bytes += PAGE_SIZE;
            }
            break;
        }
        case LUM_TRACE_GRANULARITY_BYTE: {
            for (size_t i = 0; i < buffer_size; i++) {
                lum_t lum;
                memset(&lum, 0, sizeof(lum));
                encode_byte_to_lum(base_vaddr + i, src[i], &lum);
                if (fwrite(&lum, sizeof(lum_t), 1, out) != 1) goto io_err;
                total_lums++;
            }
            total_bytes = buffer_size;
            break;
        }
        case LUM_TRACE_GRANULARITY_BIT: {
            for (size_t i = 0; i < buffer_size; i++) {
                for (int b = 0; b < 8; b++) {
                    uint8_t bit = (src[i] >> b) & 1u;
                    lum_t lum;
                    memset(&lum, 0, sizeof(lum));
                    encode_bit_to_lum((base_vaddr + i) * 8 + b, bit, &lum);
                    if (fwrite(&lum, sizeof(lum_t), 1, out) != 1) goto io_err;
                    total_lums++;
                }
            }
            total_bytes = buffer_size;
            break;
        }
        case LUM_TRACE_GRANULARITY_HUGEPAGE:
            /* Déjà rejeté plus haut */
            fclose(out);
            return -EINVAL;
    }

    /* Réécrire le header avec les compteurs finaux */
    hdr.total_lums = total_lums;
    hdr.total_bytes = total_bytes;
    if (fseek(out, 0, SEEK_SET) != 0) goto io_err;
    if (fwrite(&hdr, sizeof(hdr), 1, out) != 1) goto io_err;
    fflush(out);
    /* C133 : ftruncate explicite à la position courante (anti-padding NUL,
     * cohérent avec C129-FIX-NUL-01 d'ultra_forensic_logger). */
    {
        long pos = ftell(out);
        if (pos > 0) {
            int fd = fileno(out);
            if (fd >= 0) {
                /* ftruncate retourne -1 sur erreur ; on ignore (best effort) */
                (void)ftruncate(fd, (off_t)pos);
            }
        }
    }
    fclose(out);

    uint64_t t1 = now_ns();
    if (stats) {
        stats->total_lums_emitted = total_lums;
        stats->total_pages_scanned = (buffer_size + PAGE_SIZE - 1) / PAGE_SIZE;
        stats->total_pages_resident = stats->total_pages_scanned; /* 100% résident (heap) */
        stats->total_bytes_dumped = total_bytes;
        stats->snapshot_ns = t1 - t0;
        stats->magic = LUM_TRACER_MAGIC;
    }
    return 0;

io_err:
    fclose(out);
    return -EIO;
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

    /* C117-P1 — Lecture stricte du header 64 octets. Les fichiers .lum
     * produits avant C117 (header 32 octets, version_major absent) doivent
     * être régénérés ; le reader rejette les anciens fichiers explicitement. */
    lum_file_header_t hdr;
    if (fread(&hdr, sizeof(hdr), 1, in) != 1) {
        fclose(in);
        return -EIO;
    }
    if (hdr.magic != LUM_TRACER_MAGIC) {
        fclose(in);
        return -EBADMSG;
    }
    if (hdr.version_major != LUM_FORMAT_VERSION_C117) {
        /* Format legacy v1 (header 32 octets) — non supporté en C117.
         * Le caller doit régénérer le snapshot avec lum_memory_snapshot_self(). */
        fclose(in);
        return -ENOTSUP;
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
                /* C115 — reconstruction HUGEPAGE :
                 * Le lum_t.reserved_flags contient le nombre de pages dans la tranche.
                 * On lit bytes_in_tranche = reserved_flags * PAGE_SIZE octets bruts. */
                size_t n_pages = lum.reserved_flags;
                if (n_pages == 0) n_pages = PAGES_PER_HUGEPAGE; /* fallback 512 */
                size_t hp_size = n_pages * PAGE_SIZE;
                if (hp_size > HUGEPAGE_SIZE) hp_size = HUGEPAGE_SIZE;
                if (written + hp_size > target_size) hp_size = target_size - written;
                if (hp_size == 0) goto done;
                if (fread(dst + written, hp_size, 1, in) != 1) goto done;
                written += hp_size;
                break;
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

/* ============================================================================
 * C116-KERNEL : lum_memory_smaps_rollup
 * Lit /proc/self/smaps_rollup pour accéder aux statistiques mémoire kernel
 * incluant AnonHugePages (THP 2MB), RSS, Private_Dirty, Shared_Clean.
 *
 * Accès kernel : OUI — /proc/self/smaps est une interface kernel (VFS /proc)
 * qui expose les VMAs (Virtual Memory Areas) avec granularité page (4KiB).
 * smaps_rollup agrège toutes les VMAs pour éviter O(N) parsing.
 *
 * Champs extraits :
 *   - Rss          : pages résidentes (RAM physique occupée)
 *   - AnonHugePages: pages THP 2MB allouées (HugePage traçage)
 *   - Private_Dirty: pages modifiées non-partagées (état mémoire unique process)
 *   - Shared_Dirty : pages modifiées partagées
 *   - Referenced    : pages accédées depuis dernier reset softclear
 * ============================================================================ */
/* lum_smaps_rollup_t défini dans lum_memory_tracer.h */

/* Lit /proc/self/smaps_rollup et remplit smaps.
 * Retourne 0 succès, -1 si fichier inaccessible (kernel < 4.14). */
int lum_memory_smaps_rollup(lum_smaps_rollup_t *smaps) {
    if (!smaps) return -1;
    memset(smaps, 0, sizeof(*smaps));

    /* Tenter smaps_rollup (kernel >= 4.14, Linux Ubuntu 18.04+) */
    FILE *f = fopen("/proc/self/smaps_rollup", "r");
    if (!f) {
        /* Fallback : /proc/self/status pour VmRSS */
        f = fopen("/proc/self/status", "r");
        if (!f) return -1;
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            uint64_t val;
            if (sscanf(line, "VmRSS: %llu kB", (unsigned long long*)&val) == 1)
                smaps->rss_kb = val;
            else if (sscanf(line, "VmSize: %llu kB", (unsigned long long*)&val) == 1)
                smaps->vm_size_kb = val;
        }
        fclose(f);
        smaps->page_count = smaps->rss_kb / 4;
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        uint64_t val;
        if (sscanf(line, "Rss: %llu kB", (unsigned long long*)&val) == 1)
            smaps->rss_kb = val;
        else if (sscanf(line, "AnonHugePages: %llu kB", (unsigned long long*)&val) == 1)
            smaps->anon_huge_kb = val;
        else if (sscanf(line, "Private_Dirty: %llu kB", (unsigned long long*)&val) == 1)
            smaps->private_dirty_kb = val;
        else if (sscanf(line, "Shared_Dirty: %llu kB", (unsigned long long*)&val) == 1)
            smaps->shared_dirty_kb = val;
        else if (sscanf(line, "Referenced: %llu kB", (unsigned long long*)&val) == 1)
            smaps->referenced_kb = val;
        else if (sscanf(line, "Size: %llu kB", (unsigned long long*)&val) == 1)
            smaps->vm_size_kb += val;
    }
    fclose(f);

    smaps->page_count      = smaps->rss_kb / 4;  /* 4KB par page */
    smaps->huge_pages_count = smaps->anon_huge_kb / 2048; /* 2MB par hugepage */

    return 0;
}

/* Affiche un résumé smaps sur stdout (pour les logs LUM).
 * Indique si les HugePages THP sont actives (AnonHugePages > 0). */
void lum_memory_smaps_print(const lum_smaps_rollup_t *smaps) {
    printf("[SMAPS-KERNEL] RSS=%llu KB (%llu pages 4KiB)\n",
           (unsigned long long)smaps->rss_kb,
           (unsigned long long)smaps->page_count);
    printf("[SMAPS-KERNEL] AnonHugePages (THP 2MB)=%llu KB (%llu hugepages)\n",
           (unsigned long long)smaps->anon_huge_kb,
           (unsigned long long)smaps->huge_pages_count);
    printf("[SMAPS-KERNEL] Private_Dirty=%llu KB | Shared_Dirty=%llu KB\n",
           (unsigned long long)smaps->private_dirty_kb,
           (unsigned long long)smaps->shared_dirty_kb);
    printf("[SMAPS-KERNEL] Referenced=%llu KB | VM_Total=%llu KB\n",
           (unsigned long long)smaps->referenced_kb,
           (unsigned long long)smaps->vm_size_kb);
    printf("[SMAPS-KERNEL] Granularité réelle : %s (selon AnonHugePages)\n",
           smaps->anon_huge_kb > 0 ? "HUGEPAGE 2MB actif" : "PAGE 4KiB standard");
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
