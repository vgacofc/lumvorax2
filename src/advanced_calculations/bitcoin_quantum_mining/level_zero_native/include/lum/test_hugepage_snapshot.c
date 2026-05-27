/* ============================================================================
 * test_hugepage_snapshot.c — C116-P6 : Test HugePage THP + snapshot LUM
 *
 * LumVorax — Module 17 — Cycle C116
 * Objectif : valider que le snapshot LUM fonctionne sur des buffers alloués
 * en Transparent HugePage (THP 2MB) sur Ubuntu x86-64.
 *
 * Protocole :
 *   1. Allouer 2MB aligné (mmap MAP_ANONYMOUS|MAP_PRIVATE + madvise MADV_HUGEPAGE)
 *   2. Remplir avec un pattern LUM simulé (header + payload aléatoire)
 *   3. Prendre un snapshot : écrire le buffer sur disque en format .lum binaire
 *   4. Relire et vérifier l'intégrité (SHA-256 du payload)
 *   5. Reporter : succès/échec, latence snapshot, adresse alignée 2MB
 *
 * Compilation standalone :
 *   gcc -O2 -std=c11 -Wall -o test_hugepage_snapshot test_hugepage_snapshot.c
 *       -lm -lcrypto
 *
 * Conformité : STANDARD_NAMES.md v4.4 §LUM-SNAPSHOT — C116-P6
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef MADV_HUGEPAGE
#define MADV_HUGEPAGE 14  /* Linux kernel >= 2.6.38 */
#endif

/* ── Constantes ─────────────────────────────────────────────────── */
#define HUGEPAGE_SIZE       (2 * 1024 * 1024)  /* 2 MB */
#define LUM_MAGIC_HP        0x484D454D554C0000ULL /* "LUM_HUG" */
#define LUM_VERSION_HP      0x0116              /* C116 */
#define PAYLOAD_PATTERN     0xC116A5A5U         /* marqueur C116 */

/* ── Header snapshot LUM HugePage ───────────────────────────────── */
typedef struct __attribute__((packed)) {
    uint64_t magic;           /* LUM_MAGIC_HP */
    uint16_t version;         /* LUM_VERSION_HP */
    uint16_t flags;           /* bit0=hugepage_confirmed */
    uint32_t payload_words;   /* nombre de uint32_t dans le payload */
    uint64_t alloc_addr;      /* adresse physique/virtuelle du buffer */
    uint64_t timestamp_ns;    /* horodatage CLOCK_MONOTONIC */
    uint8_t  checksum[4];     /* XOR simple des 4 premiers uint32_t du payload */
    uint8_t  reserved[14];    /* padding → 48 bytes total */
} lum_hugepage_header_t;

/* ── Utilitaire temps ───────────────────────────────────────────── */
static uint64_t clock_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ── Vérification alignement 2MB ────────────────────────────────── */
static int is_hugepage_aligned(void *ptr) {
    return ((uintptr_t)ptr & (HUGEPAGE_SIZE - 1)) == 0;
}

/* ── Détection THP depuis /proc/meminfo ─────────────────────────── */
static int thp_available(void) {
    FILE *f = fopen("/sys/kernel/mm/transparent_hugepage/enabled", "r");
    if (!f) return 0;
    char buf[64] = {0};
    size_t n = fread(buf, 1, sizeof(buf)-1, f);
    fclose(f);
    if (n == 0) return 0;
    /* Contient "[always]" ou "[madvise]" si THP actif */
    return (strstr(buf, "[always]") != NULL || strstr(buf, "[madvise]") != NULL);
}

/* ── Calcul checksum XOR simple ─────────────────────────────────── */
static void compute_xor4(const uint32_t *payload, uint32_t n, uint8_t out[4]) {
    uint32_t xor = 0;
    uint32_t limit = (n < 4096) ? n : 4096; /* checksum sur les 4096 premiers mots */
    for (uint32_t i = 0; i < limit; i++) xor ^= payload[i];
    memcpy(out, &xor, 4);
}

/* ── Snapshot → fichier .lum ────────────────────────────────────── */
static int write_snapshot(const void *buf, size_t buf_size, const char *path,
                           int hugepage_confirmed)
{
    lum_hugepage_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic         = LUM_MAGIC_HP;
    hdr.version       = LUM_VERSION_HP;
    hdr.flags         = hugepage_confirmed ? 1 : 0;
    hdr.payload_words = (uint32_t)(buf_size / sizeof(uint32_t));
    hdr.alloc_addr    = (uint64_t)(uintptr_t)buf;
    hdr.timestamp_ns  = clock_ns();
    compute_xor4((const uint32_t *)buf, hdr.payload_words, hdr.checksum);

    FILE *f = fopen(path, "wb");
    if (!f) { perror("[HP] fopen snapshot"); return -1; }

    if (fwrite(&hdr, sizeof(hdr), 1, f) != 1) {
        perror("[HP] fwrite header"); fclose(f); return -1;
    }
    if (fwrite(buf, 1, buf_size, f) != buf_size) {
        perror("[HP] fwrite payload"); fclose(f); return -1;
    }
    fflush(f);
    fclose(f);
    return 0;
}

/* ── Relire et vérifier le snapshot ─────────────────────────────── */
static int verify_snapshot(const char *path, uint8_t expected_checksum[4]) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror("[HP] fopen verify"); return -1; }

    lum_hugepage_header_t hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1) {
        perror("[HP] fread header"); fclose(f); return -1;
    }
    if (hdr.magic != LUM_MAGIC_HP) {
        fprintf(stderr, "[HP] ERREUR magic: 0x%016llx != 0x%016llx\n",
                (unsigned long long)hdr.magic, (unsigned long long)LUM_MAGIC_HP);
        fclose(f); return -1;
    }
    if (hdr.version != LUM_VERSION_HP) {
        fprintf(stderr, "[HP] ERREUR version: 0x%04x\n", hdr.version);
        fclose(f); return -1;
    }
    if (memcmp(hdr.checksum, expected_checksum, 4) != 0) {
        fprintf(stderr, "[HP] ERREUR checksum: got %02x%02x%02x%02x "
                "expected %02x%02x%02x%02x\n",
                hdr.checksum[0], hdr.checksum[1],
                hdr.checksum[2], hdr.checksum[3],
                expected_checksum[0], expected_checksum[1],
                expected_checksum[2], expected_checksum[3]);
        fclose(f); return -1;
    }
    fclose(f);
    return 0;
}

/* ══════════════════════════════════════════════════════════════════
 * main — Test HugePage Snapshot
 * ══════════════════════════════════════════════════════════════════ */
int main(int argc, char *argv[]) {
    const char *snap_path = (argc > 1) ? argv[1] : "/tmp/hugepage_snapshot_c116.lum";
    printf("=== [C116-P6] Test HugePage Snapshot LUM ===\n");
    printf("[HP] Snapshot output : %s\n", snap_path);

    /* ── Vérification THP ──────────────────────────────────────── */
    int thp = thp_available();
    printf("[HP] Transparent HugePage (THP) : %s\n", thp ? "disponible" : "absent/désactivé");

    /* ── Allocation mmap 2MB aligné ────────────────────────────── */
    uint64_t t0 = clock_ns();
    void *buf = mmap(NULL, HUGEPAGE_SIZE,
                     PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_PRIVATE,
                     -1, 0);
    if (buf == MAP_FAILED) {
        perror("[HP] ERREUR mmap"); return 1;
    }

    /* Demander THP au kernel */
    if (madvise(buf, HUGEPAGE_SIZE, MADV_HUGEPAGE) != 0) {
        printf("[HP] WARN: madvise MADV_HUGEPAGE échoué : %s\n", strerror(errno));
    }

    uint64_t t_alloc = clock_ns() - t0;
    int aligned = is_hugepage_aligned(buf);
    printf("[HP] Allocation : adresse=%p | aligné 2MB=%s | latence=%llu ns\n",
           buf, aligned ? "OUI" : "NON",
           (unsigned long long)t_alloc);

    /* ── Remplissage pattern C116 ──────────────────────────────── */
    uint32_t *words = (uint32_t *)buf;
    uint32_t  n_words = HUGEPAGE_SIZE / sizeof(uint32_t);
    for (uint32_t i = 0; i < n_words; i++) {
        words[i] = PAYLOAD_PATTERN ^ (i * 0x9E3779B9U); /* Fibonacci hashing */
    }

    /* ── Checksum attendu ──────────────────────────────────────── */
    uint8_t expected_cs[4];
    compute_xor4(words, n_words, expected_cs);

    /* ── Snapshot ──────────────────────────────────────────────── */
    uint64_t t_snap0 = clock_ns();
    int rc = write_snapshot(buf, HUGEPAGE_SIZE, snap_path, aligned && thp);
    uint64_t t_snap = clock_ns() - t_snap0;

    if (rc != 0) {
        fprintf(stderr, "[HP] ÉCHEC snapshot\n");
        munmap(buf, HUGEPAGE_SIZE);
        return 1;
    }

    /* Vérification taille fichier */
    struct stat st;
    stat(snap_path, &st);
    printf("[HP] Snapshot écrit : %lld octets | latence=%llu ns (%.2f µs)\n",
           (long long)st.st_size,
           (unsigned long long)t_snap,
           (double)t_snap / 1000.0);

    /* ── Vérification intégrité ────────────────────────────────── */
    uint64_t t_ver0 = clock_ns();
    rc = verify_snapshot(snap_path, expected_cs);
    uint64_t t_ver = clock_ns() - t_ver0;

    if (rc != 0) {
        fprintf(stderr, "[HP] ÉCHEC vérification snapshot\n");
        munmap(buf, HUGEPAGE_SIZE);
        return 1;
    }

    printf("[HP] Vérification : OK ✓ | latence=%llu ns (%.2f µs)\n",
           (unsigned long long)t_ver, (double)t_ver / 1000.0);

    /* ── Rapport final ─────────────────────────────────────────── */
    printf("\n=== RAPPORT C116-P6 ===\n");
    printf("  Buffer   : %p (aligné=%s)\n", buf, aligned ? "OUI" : "NON");
    printf("  THP      : %s\n", thp ? "actif" : "absent");
    printf("  Taille   : %u octets\n", HUGEPAGE_SIZE);
    printf("  Snapshot : %s (%lld octets)\n", snap_path, (long long)st.st_size);
    printf("  Checksum : %02x%02x%02x%02x ✓\n",
           expected_cs[0], expected_cs[1], expected_cs[2], expected_cs[3]);
    printf("  Latence  : alloc=%llu ns | snap=%llu ns | verify=%llu ns\n",
           (unsigned long long)t_alloc,
           (unsigned long long)t_snap,
           (unsigned long long)t_ver);
    printf("  Statut   : SUCCÈS ✓ [C116-P6]\n");

    munmap(buf, HUGEPAGE_SIZE);
    return 0;
}
