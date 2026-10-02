/* genesis.c — Bloc genesis LUMVORAX (C99 Q6)
 *
 * Projet : LUMVORAX / LVX&ARTCB
 * Module : blockchain_lumvorax/genesis
 * Auteur : LumVorax Project
 *
 * BL-015 FIX (rapport 149) : lumvorax_genesis_compute_hash() utilisait une
 *   sérialisation divergente de block_header_hash() — tronquait timestamp
 *   et nonce à 32 bits, ignorait height. Les deux fonctions produisaient des
 *   digests différents pour le même header.
 *   CORRECTION : lumvorax_genesis_compute_hash() délègue maintenant à
 *   block_header_serialize_canonical() — sérialisation unique, 88 octets,
 *   little-endian explicite, tous champs PoW inclus.
 *
 * Le bloc genesis est le bloc 0 de la chaîne LUMVORAX. Il a :
 *  - prev_hash = 0x00...0 (32 octets nuls)
 *  - merkle_root = SHA256("LUMVORAX-GENESIS-2026-CYCLE-C99")
 *  - timestamp = 1714000000 (2024-04-24 17:46:40 UTC — fixe pour reproductibilité)
 *  - bits = 0x1d00ffff (difficulté minimale)
 *  - nonce = solution PoW (cherchée par GENESIS_STANDALONE)
 *
 * CERTIFIED_100=false | unique_human_proven=false
 * Mode DEBUG actif
 */
#include "blockchain_lumvorax.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

extern void sha256_lumvorax(const uint8_t* data, size_t len, uint8_t out[32]);

static const char GENESIS_MAGIC[] = "LUMVORAX-GENESIS-2026-CYCLE-C99";
static const uint32_t GENESIS_VERSION   = 1;
static const uint64_t GENESIS_TIMESTAMP = 1714000000ULL;  /* 2024-04-24 17:46:40 UTC */
static const uint32_t GENESIS_BITS      = 0x1d00ffffU;    /* difficulté minimale */

int lumvorax_genesis_create(block_header_t* hdr) {
    if (!hdr) return -1;
    memset(hdr, 0, sizeof(*hdr));
    hdr->version   = GENESIS_VERSION;
    /* prev_hash = tout zéro (déjà fait par memset) */
    sha256_lumvorax((const uint8_t*)GENESIS_MAGIC, sizeof(GENESIS_MAGIC) - 1, hdr->merkle_root);
    hdr->timestamp = GENESIS_TIMESTAMP;
    hdr->bits      = GENESIS_BITS;
    hdr->nonce     = 0;  /* sera mis à jour par mining initial */
    hdr->height    = 0;
    return 0;
}

int lumvorax_genesis_compute_hash(const block_header_t* hdr, uint8_t out[32]) {
    if (!hdr || !out) return -1;
    /* BL-015 FIX : délègue à block_header_serialize_canonical() — même sérialisation
     * que block_header_hash(). Tous les champs PoW (nonce 64 bits, bits, timestamp
     * 64 bits) sont correctement inclus. Plus de divergence inter-chemins. */
    uint8_t buf[LUMVORAX_HEADER_SERIAL_LEN];
    block_header_serialize_canonical(hdr, buf);
    uint8_t mid[32];
    sha256_lumvorax(buf, LUMVORAX_HEADER_SERIAL_LEN, mid);
    sha256_lumvorax(mid, 32, out);
    return 0;
}

int lumvorax_genesis_count_lz(const uint8_t hash[32]) {
    int lz = 0;
    /* Hash en LITTLE-endian Bitcoin → on inverse pour compter MSB */
    for (int i = 31; i >= 0; i--) {
        uint8_t b = hash[i];
        if (b == 0) { lz += 8; continue; }
        for (int bit = 7; bit >= 0; bit--) {
            if ((b >> bit) & 1) return lz;
            lz++;
        }
        break;
    }
    return lz;
}

#ifdef GENESIS_STANDALONE
int main(void) {
    block_header_t hdr;
    if (lumvorax_genesis_create(&hdr) != 0) {
        fprintf(stderr, "ERR: genesis create\n");
        return 1;
    }
    /* Mine un peu pour obtenir LZ ≥ 8 (rapide, ~256 essais) */
    int best_lz = 0;
    uint32_t best_nonce = 0;
    uint8_t best_hash[32] = {0};
    printf("[GENESIS] Mining bloc 0 jusqu'à LZ ≥ 8 ...\n");
    time_t t0 = time(NULL);
    for (uint32_t n = 0; n < 1000000U; n++) {
        hdr.nonce = n;
        uint8_t h[32];
        lumvorax_genesis_compute_hash(&hdr, h);
        int lz = lumvorax_genesis_count_lz(h);
        if (lz > best_lz) {
            best_lz = lz;
            best_nonce = n;
            memcpy(best_hash, h, 32);
            if (lz >= 16) break;
        }
    }
    hdr.nonce = best_nonce;
    printf("[GENESIS] LZ = %d, nonce = %u (%.0fs)\n", best_lz, best_nonce, (double)(time(NULL) - t0));
    printf("[GENESIS] hash = ");
    for (int i = 31; i >= 0; i--) printf("%02x", best_hash[i]);
    printf("\n");
    printf("[GENESIS] merkle_root = ");
    for (int i = 0; i < 32; i++) printf("%02x", hdr.merkle_root[i]);
    printf("\n");
    return best_lz >= 8 ? 0 : 2;
}
#endif
