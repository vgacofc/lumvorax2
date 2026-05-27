/* block_header.c — Header de bloc + comptage des leading zeros.
 *
 * Cycle C95 — squelette compilable. Le hash SHA-256 lui-même est délégué
 * aux fonctions sha256_* déjà présentes dans le minier BTC NX48.
 */
#include "blockchain_lumvorax.h"
#include <string.h>

/* Stub : sera remplacé par appel à sha256_double() du minier BTC. */
static void sha256_stub(const uint8_t *data, size_t len, uint8_t out[32]) {
    (void)data;
    (void)len;
    /* Placeholder pour compilation seule. La vraie impl appelle
     * src/advanced_calculations/bitcoin_quantum_mining/sha256_neural.c
     */
    memset(out, 0, 32);
}

void block_header_hash(const block_header_t *h, uint8_t out_hash[32]) {
    if (!h || !out_hash) return;
    /* Pour compatibilité Bitcoin : hash des 80 premiers octets uniquement. */
    sha256_stub((const uint8_t *)h, 80, out_hash);
}

int block_header_meets_difficulty(const uint8_t hash[32], uint32_t bits) {
    if (!hash) return 0;
    /* leading_zeros bit count */
    uint32_t lz = 0;
    for (int i = 0; i < 32; ++i) {
        uint8_t b = hash[i];
        if (b == 0) {
            lz += 8;
            continue;
        }
        while ((b & 0x80) == 0) {
            lz++;
            b <<= 1;
        }
        break;
    }
    return (lz >= bits) ? 1 : 0;
}
