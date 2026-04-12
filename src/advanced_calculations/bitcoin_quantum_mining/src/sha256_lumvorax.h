#ifndef SHA256_LUMVORAX_H
#define SHA256_LUMVORAX_H

/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * sha256_lumvorax.h — SHA-256 avec traçabilité forensic totale A–Z
 *
 * Conformité : STANDARD_NAMES.md v4.1 §M-BTC17
 * Cycle C62 — 2026-04-11
 *
 * Caractéristiques :
 *  - SHA-256 standard (RFC 6234) — vecteurs de test NIST
 *  - Traçage forensic bit-par-bit via ultra_forensic_logger
 *  - Backend AVX2 (8-way SIMD) si disponible
 *  - Midstate pré-calculé (optimisation classique ×2)
 *  - Thread-safe (mutex par contexte)
 *  - Mémoire tracée (LV_MALLOC / LV_FREE)
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>

/* ── Constantes SHA-256 ─────────────────────────────────────────── */
#define LV_SHA256_DIGEST_SIZE   32
#define LV_SHA256_BLOCK_SIZE    64
#define LV_SHA256_MIDSTATE_WORDS 8

/* ── Contexte SHA-256 LumVorax forensic complet ─────────────────── */
typedef struct {
    uint32_t state[8];          /* État SHA-256 courant (h0..h7) */
    uint8_t  buf[64];           /* Buffer bloc en cours */
    uint64_t bit_count;         /* Compteur bits traités */

    /* Forensic fields */
    uint32_t nonce;             /* Nonce testé dans ce hash */
    uint64_t hash_id;           /* ID unique atomique de ce hash */
    uint64_t ts_start_ns;       /* Timestamp nanoseconde début hash */
    uint64_t ts_end_ns;         /* Timestamp nanoseconde fin hash */
    int      leading_zeros;     /* Zéros de tête dans le résultat */
    double   difficulty_ratio;  /* hash_value / target */
    char     run_id[64];        /* run_id LumVorax */

    /* Midstate */
    uint32_t midstate[8];       /* État après premier bloc (header fixe) */
    bool     midstate_valid;    /* Midstate pré-calculé valide */

    pthread_mutex_t mutex;      /* Thread-safety par contexte */
} lv_sha256_ctx_t;

/* ── Résultat d'un double-SHA256 ────────────────────────────────── */
typedef struct {
    uint8_t  digest[LV_SHA256_DIGEST_SIZE];  /* Hash résultat */
    uint32_t nonce;                          /* Nonce utilisé */
    int      leading_zeros;                  /* Zéros de tête */
    uint64_t elapsed_ns;                     /* Durée double-SHA256 ns */
    bool     below_target;                   /* hash < target ? */
} lv_sha256_result_t;

/* ── Block header Bitcoin (80 octets) ───────────────────────────── */
typedef struct __attribute__((packed)) {
    uint32_t version;               /* Version (LE) */
    uint8_t  prev_block_hash[32];   /* Hash bloc précédent */
    uint8_t  merkle_root[32];       /* Racine Merkle */
    uint32_t timestamp;             /* Timestamp UNIX (LE) */
    uint32_t bits;                  /* Difficulté compacte (LE) */
    uint32_t nonce;                 /* Nonce (LE) — à faire varier */
} lv_btc_block_header_t;

/* ── Pool de contextes SHA-256 (thread-safe) ────────────────────── */
typedef struct {
    lv_sha256_ctx_t*  pool;         /* Tableau de contextes (LV_CALLOC) */
    int               n_ctx;        /* Nombre de contextes */
    volatile uint64_t hash_counter; /* Compteur atomique global */
    pthread_mutex_t   counter_mutex;
} lv_sha256_pool_t;

/* ── API publique SHA-256 LumVorax ──────────────────────────────── */

/* Initialise un contexte SHA-256 forensic */
void lv_sha256_init(lv_sha256_ctx_t* ctx, uint32_t nonce, const char* run_id);

/* Ajoute des données au contexte */
void lv_sha256_update(lv_sha256_ctx_t* ctx, const uint8_t* data, size_t len);

/* Finalise et retourne le digest */
void lv_sha256_final(lv_sha256_ctx_t* ctx, uint8_t digest[LV_SHA256_DIGEST_SIZE]);

/* Double-SHA256 complet avec forensic intégré (hash Bitcoin) */
lv_sha256_result_t lv_sha256d_block(
    const lv_btc_block_header_t* header,
    const uint8_t target[32],
    const char* run_id,
    uint64_t hash_id
);

/* Pré-calcule le midstate du premier bloc (header[0..63]) */
void lv_sha256_compute_midstate(
    const lv_btc_block_header_t* header,
    uint32_t midstate_out[LV_SHA256_MIDSTATE_WORDS]
);

/* Double-SHA256 avec midstate pré-calculé (×2 plus rapide).
 * C65-FIX-MIDSTATE : La queue du header (octets [64..79]) doit inclure
 * merkle_root[28..31] + timestamp + bits + nonce dans le bon ordre.
 * Le header complet est passé pour accéder à TOUS les champs nécessaires.
 * Ref : analysechatgpt91.38.md §BUG-MIDSTATE — 2026-04-12 */
lv_sha256_result_t lv_sha256d_midstate(
    const uint32_t midstate[LV_SHA256_MIDSTATE_WORDS],
    const lv_btc_block_header_t* header,   /* Header complet pour tail correct */
    uint32_t nonce,                        /* Nonce à tester (remplace header.nonce) */
    const uint8_t target[32],
    const char* run_id,
    uint64_t hash_id
);

/* Compte les zéros de tête dans un digest SHA-256 */
int lv_sha256_count_leading_zeros(const uint8_t digest[LV_SHA256_DIGEST_SIZE]);

/* Vérifie si digest < target (hash valide Bitcoin) */
bool lv_sha256_below_target(
    const uint8_t digest[LV_SHA256_DIGEST_SIZE],
    const uint8_t target[32]
);

/* Décode bits compacts → target 256 bits */
void lv_sha256_bits_to_target(uint32_t bits, uint8_t target_out[32]);

/* Vecteur de test NIST — gate BTC_SHA256_INTEGRITY_GATE */
bool lv_sha256_self_test(void);

/* Pool de contextes */
lv_sha256_pool_t* lv_sha256_pool_create(int n_threads, const char* run_id);
void              lv_sha256_pool_destroy(lv_sha256_pool_t* pool);

#endif /* SHA256_LUMVORAX_H */
