/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * sha256_lumvorax.c — SHA-256 avec traçabilité forensic totale A–Z
 *
 * STANDARD_NAMES.md v4.1 §M-BTC17 — Cycle C62 — 2026-04-11
 *
 * Implémentation :
 *  - SHA-256 standard RFC 6234 / FIPS 180-4
 *  - Traçage forensic nanoseconde via ultra_forensic_logger
 *  - Midstate pré-calculé (optim classique ×2)
 *  - Vecteurs de test NIST (gate BTC_SHA256_INTEGRITY_GATE)
 *  - Mémoire tracée LV_MALLOC / LV_FREE / LV_CALLOC
 */

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L

#include "sha256_lumvorax.h"
#include "../include/btc_mining_forensic.h"
#include "debug/ultra_forensic_logger.h"
#include "lumvorax_integration.h"
#include "debug/memory_tracker.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <stdatomic.h>

/* ── Timestamp nanoseconde monotonique ──────────────────────────── */
static uint64_t btc_ts_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ── Constantes SHA-256 K[0..63] ────────────────────────────────── */
static const uint32_t K[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};

/* ── Valeurs initiales SHA-256 (H0..H7) ─────────────────────────── */
static const uint32_t SHA256_H0[8] = {
    0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
    0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
};

/* ── Opérations SHA-256 ─────────────────────────────────────────── */
#define ROTR32(x, n)  (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x,y,z)     (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z)    (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define SIGMA0(x)     (ROTR32(x, 2)  ^ ROTR32(x, 13) ^ ROTR32(x, 22))
#define SIGMA1(x)     (ROTR32(x, 6)  ^ ROTR32(x, 11) ^ ROTR32(x, 25))
#define sigma0(x)     (ROTR32(x, 7)  ^ ROTR32(x, 18) ^ ((x) >> 3))
#define sigma1(x)     (ROTR32(x, 17) ^ ROTR32(x, 19) ^ ((x) >> 10))

/* Big-endian helpers */
static inline uint32_t be32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
         | ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
}
static inline void put_be32(uint8_t* p, uint32_t v) {
    p[0] = (v >> 24) & 0xFF; p[1] = (v >> 16) & 0xFF;
    p[2] = (v >>  8) & 0xFF; p[3] =  v        & 0xFF;
}

/* ── Transformation SHA-256 (un bloc de 64 octets) ──────────────── */
static void sha256_transform(uint32_t state[8], const uint8_t block[64]) {
    uint32_t W[64], a, b, c, d, e, f, g, h, T1, T2;
    int i;
    for (i = 0; i < 16; i++) W[i] = be32(block + 4*i);
    for (i = 16; i < 64; i++)
        W[i] = sigma1(W[i-2]) + W[i-7] + sigma0(W[i-15]) + W[i-16];
    a=state[0]; b=state[1]; c=state[2]; d=state[3];
    e=state[4]; f=state[5]; g=state[6]; h=state[7];
    for (i = 0; i < 64; i++) {
        T1 = h + SIGMA1(e) + CH(e,f,g) + K[i] + W[i];
        T2 = SIGMA0(a) + MAJ(a,b,c);
        h=g; g=f; f=e; e=d+T1;
        d=c; c=b; b=a; a=T1+T2;
    }
    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
    state[4]+=e; state[5]+=f; state[6]+=g; state[7]+=h;
}

/* ── Initialise un contexte SHA-256 forensic ────────────────────── */
void lv_sha256_init(lv_sha256_ctx_t* ctx, uint32_t nonce, const char* run_id) {
    memcpy(ctx->state, SHA256_H0, sizeof(SHA256_H0));
    memset(ctx->buf, 0, sizeof(ctx->buf));
    ctx->bit_count      = 0;
    ctx->nonce          = nonce;
    ctx->leading_zeros  = 0;
    ctx->difficulty_ratio = 0.0;
    ctx->midstate_valid = false;
    ctx->ts_start_ns    = btc_ts_ns();
    ctx->ts_end_ns      = 0;
    strncpy(ctx->run_id, run_id ? run_id : "unknown", sizeof(ctx->run_id)-1);
    ctx->run_id[sizeof(ctx->run_id)-1] = '\0';
    pthread_mutex_init(&ctx->mutex, NULL);
}

/* ── Ajoute des données ─────────────────────────────────────────── */
void lv_sha256_update(lv_sha256_ctx_t* ctx, const uint8_t* data, size_t len) {
    size_t used = (size_t)((ctx->bit_count / 8) % 64);
    ctx->bit_count += (uint64_t)len * 8;
    while (len > 0) {
        size_t copy = 64 - used;
        if (copy > len) copy = len;
        memcpy(ctx->buf + used, data, copy);
        data += copy; len -= copy; used += copy;
        if (used == 64) {
            sha256_transform(ctx->state, ctx->buf);
            used = 0;
        }
    }
}

/* ── Finalise et retourne le digest ─────────────────────────────── */
void lv_sha256_final(lv_sha256_ctx_t* ctx, uint8_t digest[LV_SHA256_DIGEST_SIZE]) {
    size_t used = (size_t)((ctx->bit_count / 8) % 64);
    ctx->buf[used++] = 0x80;
    if (used > 56) {
        memset(ctx->buf + used, 0, 64 - used);
        sha256_transform(ctx->state, ctx->buf);
        used = 0;
    }
    memset(ctx->buf + used, 0, 56 - used);
    /* Longueur en big-endian */
    put_be32(ctx->buf + 56, (uint32_t)(ctx->bit_count >> 32));
    put_be32(ctx->buf + 60, (uint32_t)(ctx->bit_count & 0xFFFFFFFFu));
    sha256_transform(ctx->state, ctx->buf);
    for (int i = 0; i < 8; i++) put_be32(digest + 4*i, ctx->state[i]);
    ctx->ts_end_ns = btc_ts_ns();
}

/* ── Compte les zéros de tête ───────────────────────────────────── */
int lv_sha256_count_leading_zeros(const uint8_t digest[LV_SHA256_DIGEST_SIZE]) {
    int count = 0;
    for (int i = 0; i < LV_SHA256_DIGEST_SIZE; i++) {
        if (digest[i] == 0x00) { count += 8; continue; }
        uint8_t b = digest[i];
        if      (b < 0x10) count += 4;
        else if (b < 0x20) count += 3;
        else if (b < 0x40) count += 2;
        else if (b < 0x80) count += 1;
        break;
    }
    return count;
}

/* ── Vérifie si digest < target ─────────────────────────────────── */
bool lv_sha256_below_target(
    const uint8_t digest[LV_SHA256_DIGEST_SIZE],
    const uint8_t target[32])
{
    for (int i = 0; i < 32; i++) {
        if (digest[i] < target[i]) return true;
        if (digest[i] > target[i]) return false;
    }
    return false;
}

/* ── Bits compacts → target 256 bits ───────────────────────────── */
void lv_sha256_bits_to_target(uint32_t bits, uint8_t target_out[32]) {
    memset(target_out, 0, 32);
    int exp  = (int)((bits >> 24) & 0xFF);
    uint32_t coeff = bits & 0x007FFFFFu;
    int byte_idx = exp - 3;
    if (byte_idx >= 0 && byte_idx < 29) {
        target_out[byte_idx]   = (coeff >> 16) & 0xFF;
        target_out[byte_idx+1] = (coeff >>  8) & 0xFF;
        target_out[byte_idx+2] =  coeff        & 0xFF;
    }
}

/* ── Pré-calcule le midstate du premier bloc (64 octets fixes) ─── */
void lv_sha256_compute_midstate(
    const lv_btc_block_header_t* header,
    uint32_t midstate_out[LV_SHA256_MIDSTATE_WORDS])
{
    memcpy(midstate_out, SHA256_H0, 32);
    /* Les 64 premiers octets du header sont fixes (version + prev_hash + merkle[0..31]) */
    sha256_transform(midstate_out, (const uint8_t*)header);
}

/* ── Double-SHA256 avec midstate ────────────────────────────────── */
/*
 * C65-FIX-MIDSTATE : Correction de l'erreur cryptographique dans la construction
 * du bloc "tail" (octets 64..79 du header Bitcoin 80 octets).
 *
 * Structure du header Bitcoin (80 octets, little-endian) :
 *   [0.. 3] version         (4 octets)   ─┐ couverts par le midstate
 *   [4..35] prev_block_hash (32 octets)   │ (sha256_transform sur les
 *   [36..67] merkle_root    (32 octets)  ─┘  64 premiers octets)
 *
 *   [64..67] merkle_root[28..31] (4 derniers octets de merkle_root)  ─┐
 *   [68..71] timestamp           (4 octets LE)                        │ ces 16 octets
 *   [72..75] bits                (4 octets LE)                        │ forment la "tail"
 *   [76..79] nonce               (4 octets LE)  ← VARIE              ─┘
 *
 * ERREUR ANTÉRIEURE : tail[0..3] = timestamp (incorrect — merkle[28..31] était absent)
 *                     tail[4..11] = zeros     (bits absent)
 *                     tail[12..15] = nonce    (correct en position)
 *
 * CORRECTION C65 :
 *   tail[0..3]  = merkle_root[28..31] (mémoire cast depuis le struct packed)
 *   tail[4..7]  = timestamp (LE)
 *   tail[8..11] = bits (LE)
 *   tail[12..15]= nonce (LE) — seul champ variable par itération
 *   tail[16]    = 0x80 (padding)
 *   tail[62..63]= 0x02, 0x80 (640 bits = 80×8 en big-endian sur 16 bits)
 *
 * Ref : analysechatgpt91.38.md §BUG-MIDSTATE — 2026-04-12
 */
lv_sha256_result_t lv_sha256d_midstate(
    const uint32_t midstate[LV_SHA256_MIDSTATE_WORDS],
    const lv_btc_block_header_t* header,
    uint32_t nonce,
    const uint8_t target[32],
    const char* run_id,
    uint64_t hash_id)
{
    lv_sha256_result_t res;
    memset(&res, 0, sizeof(res));
    res.nonce = nonce;

    uint64_t t0 = btc_ts_ns();

    /* Construction du tail : octets [64..79] du header Bitcoin.
     * La structure est __attribute__((packed)) → cast direct en uint8_t* valide. */
    const uint8_t* hdr_bytes = (const uint8_t*)header;

    uint8_t tail[64];
    memset(tail, 0, sizeof(tail));

    /* tail[0..3] = merkle_root[28..31] = header_bytes[64..67] */
    tail[0] = hdr_bytes[64];
    tail[1] = hdr_bytes[65];
    tail[2] = hdr_bytes[66];
    tail[3] = hdr_bytes[67];

    /* tail[4..7] = timestamp (little-endian) */
    tail[4] = (header->timestamp)       & 0xFF;
    tail[5] = (header->timestamp >> 8)  & 0xFF;
    tail[6] = (header->timestamp >> 16) & 0xFF;
    tail[7] = (header->timestamp >> 24) & 0xFF;

    /* tail[8..11] = bits (little-endian) */
    tail[8]  = (header->bits)       & 0xFF;
    tail[9]  = (header->bits >> 8)  & 0xFF;
    tail[10] = (header->bits >> 16) & 0xFF;
    tail[11] = (header->bits >> 24) & 0xFF;

    /* tail[12..15] = nonce (little-endian) — seule valeur variable */
    tail[12] = (nonce)       & 0xFF;
    tail[13] = (nonce >> 8)  & 0xFF;
    tail[14] = (nonce >> 16) & 0xFF;
    tail[15] = (nonce >> 24) & 0xFF;

    /* Padding SHA-256 du bloc 80 octets (640 bits) */
    tail[16] = 0x80;
    /* Longueur : 640 bits = 0x0280 en big-endian sur 64 bits → octets [56..63] */
    tail[62] = 0x02;
    tail[63] = 0x80;

    /* Première passe avec midstate (état après les 64 premiers octets du header) */
    uint32_t state1[8];
    memcpy(state1, midstate, 32);
    sha256_transform(state1, tail);

    /* Finalisation première passe */
    uint8_t mid_digest[32];
    for (int i = 0; i < 8; i++) put_be32(mid_digest + 4*i, state1[i]);

    /* Deuxième passe SHA-256 */
    uint32_t state2[8];
    memcpy(state2, SHA256_H0, 32);
    uint8_t block2[64];
    memcpy(block2, mid_digest, 32);
    block2[32] = 0x80;
    memset(block2 + 33, 0, 23);
    block2[62] = 0x01; block2[63] = 0x00; /* 256 bits */
    sha256_transform(state2, block2);
    for (int i = 0; i < 8; i++) put_be32(res.digest + 4*i, state2[i]);

    uint64_t t1 = btc_ts_ns();
    res.elapsed_ns    = t1 - t0;
    res.leading_zeros = lv_sha256_count_leading_zeros(res.digest);
    res.below_target  = lv_sha256_below_target(res.digest, target);

    /* Log forensic nano (ring buffer, sans I/O bloquante) */
    BTC_NANO("btc_sha256_nonce",      (double)nonce);
    BTC_NANO("btc_leading_zeros",     (double)res.leading_zeros);
    BTC_NANO("btc_sha256_elapsed_ns", (double)res.elapsed_ns);

    (void)run_id; (void)hash_id;
    return res;
}

/* ── Double-SHA256 complet sans midstate ────────────────────────── */
lv_sha256_result_t lv_sha256d_block(
    const lv_btc_block_header_t* header,
    const uint8_t target[32],
    const char* run_id,
    uint64_t hash_id)
{
    lv_sha256_result_t res;
    memset(&res, 0, sizeof(res));
    res.nonce = header->nonce;

    uint64_t t0 = btc_ts_ns();

    /* Première SHA-256 sur le bloc header 80 octets */
    uint8_t buf1[64];
    uint32_t state1[8];
    memcpy(state1, SHA256_H0, 32);
    /* Bloc 1 : octets [0..63] */
    memcpy(buf1, header, 64);
    sha256_transform(state1, buf1);
    /* Bloc 2 : octets [64..79] + padding */
    uint8_t buf2[64];
    memset(buf2, 0, 64);
    memcpy(buf2, (const uint8_t*)header + 64, 16);
    buf2[16] = 0x80;
    buf2[62] = 0x02; buf2[63] = 0x80;
    sha256_transform(state1, buf2);
    uint8_t mid_digest[32];
    for (int i = 0; i < 8; i++) put_be32(mid_digest + 4*i, state1[i]);

    /* Deuxième SHA-256 */
    uint32_t state2[8];
    memcpy(state2, SHA256_H0, 32);
    uint8_t buf3[64];
    memcpy(buf3, mid_digest, 32);
    buf3[32] = 0x80;
    memset(buf3 + 33, 0, 23);
    buf3[62] = 0x01; buf3[63] = 0x00;
    sha256_transform(state2, buf3);
    for (int i = 0; i < 8; i++) put_be32(res.digest + 4*i, state2[i]);

    uint64_t t1 = btc_ts_ns();
    res.elapsed_ns    = t1 - t0;
    res.leading_zeros = lv_sha256_count_leading_zeros(res.digest);
    res.below_target  = lv_sha256_below_target(res.digest, target);

    BTC_NANO("btc_sha256d_block_ns",  (double)res.elapsed_ns);
    BTC_NANO("btc_leading_zeros",     (double)res.leading_zeros);

    (void)run_id; (void)hash_id;
    return res;
}

/* ── Gate : test intégrité SHA-256 ─────────────────────────────── */
/* SHA-256("abc") = ba7816bf 8f01cfea 414140de 5dae2223 b00361a3 ... */
bool lv_sha256_self_test(void) {
    /* SHA-256("abc") — vecteur vérifié : openssl, python hashlib, sha256sum */
    static const uint8_t expected[32] = {
        0xba,0x78,0x16,0xbf, 0x8f,0x01,0xcf,0xea,
        0x41,0x41,0x40,0xde, 0x5d,0xae,0x22,0x23,
        0xb0,0x03,0x61,0xa3, 0x96,0x17,0x7a,0x9c,
        0xb4,0x10,0xff,0x61, 0xf2,0x00,0x15,0xad
    };
    lv_sha256_ctx_t ctx;
    lv_sha256_init(&ctx, 0, "selftest");
    lv_sha256_update(&ctx, (const uint8_t*)"abc", 3);
    uint8_t digest[32];
    lv_sha256_final(&ctx, digest);
    pthread_mutex_destroy(&ctx.mutex);

    bool ok = (memcmp(digest, expected, 32) == 0);
    if (!ok) {
        fprintf(stderr, "[SHA256_DIAG] NIST gate FAILED\n  got     : ");
        for (int i = 0; i < 32; i++) fprintf(stderr, "%02x", digest[i]);
        fprintf(stderr, "\n  expected: ");
        for (int i = 0; i < 32; i++) fprintf(stderr, "%02x", expected[i]);
        fprintf(stderr, "\n");
    }
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_sha256_self_test_pass", (double)ok);
    if (!ok) {
        FORENSIC_LOG_ANOMALY(BTC_MODULE_NAME, "BTC_SHA256_INTEGRITY_GATE_FAILED", 0.0);
    }
    return ok;
}

/* ── Pool de contextes ──────────────────────────────────────────── */
lv_sha256_pool_t* lv_sha256_pool_create(int n_threads, const char* run_id) {
    lv_sha256_pool_t* pool = LV_MALLOC(sizeof(lv_sha256_pool_t));
    if (!pool) return NULL;
    pool->pool = LV_CALLOC((size_t)n_threads, sizeof(lv_sha256_ctx_t));
    if (!pool->pool) { LV_FREE(pool); return NULL; }
    pool->n_ctx         = n_threads;
    pool->hash_counter  = 0;
    pthread_mutex_init(&pool->counter_mutex, NULL);
    for (int i = 0; i < n_threads; i++) {
        lv_sha256_init(&pool->pool[i], 0, run_id);
    }
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_sha256_pool_n_ctx", (double)n_threads);
    return pool;
}

void lv_sha256_pool_destroy(lv_sha256_pool_t* pool) {
    if (!pool) return;
    for (int i = 0; i < pool->n_ctx; i++)
        pthread_mutex_destroy(&pool->pool[i].mutex);
    pthread_mutex_destroy(&pool->counter_mutex);
    LV_FREE(pool->pool);
    LV_FREE(pool);
}
