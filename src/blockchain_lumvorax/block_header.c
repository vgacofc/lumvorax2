/* block_header.c — Header de bloc + comptage des leading zeros.
 *
 * Cycle C143 — stub sha256_stub() remplacé par sha256_lumvorax() réelle
 * (FIPS 180-4, définie dans sha256_mini.c — même module blockchain).
 * L'anomalie "32 octets nuls" du rapport 135 est corrigée ici.
 */
#include "blockchain_lumvorax.h"
#include <string.h>

/* SHA-256 FIX (rapport 135 / rapport 143) : suppression du stub memset(0).
 * sha256_lumvorax() est déclarée dans sha256_mini.c, même répertoire.
 * Double-SHA256 conforme au protocole Bitcoin (hash(hash(header))). */
extern void sha256_lumvorax(const uint8_t *data, size_t len, uint8_t out[32]);

void block_header_hash(const block_header_t *h, uint8_t out_hash[32]) {
    if (!h || !out_hash) return;
    /* Double-SHA256 sur les 80 premiers octets du header (protocole Bitcoin). */
    uint8_t mid[32];
    sha256_lumvorax((const uint8_t *)h, 80, mid);
    sha256_lumvorax(mid, 32, out_hash);
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
