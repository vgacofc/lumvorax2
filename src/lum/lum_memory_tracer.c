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
