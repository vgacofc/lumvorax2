/* block_header.c — Sérialisation canonique + hash du header LUMVORAX.
 *
 * Projet : LUMVORAX / LVX&ARTCB
 * Module : blockchain_lumvorax
 * Auteur : LumVorax Project
 *
 * BL-013 FIX (rapport 149) : block_header_hash() utilisait les 80 premiers
 *   octets de la représentation mémoire C — bits (offset 80) et nonce
 *   (offset 88) étaient hors fenêtre → modifier le nonce ne changeait
 *   pas le digest. PoW basé sur nonce impossible.
 *
 * BL-015 FIX (rapport 149) : block_header_hash() et genesis_compute_hash()
 *   utilisaient deux sérialisations différentes (cast struct vs memcpy
 *   explicite), produisant deux séquences d'octets divergentes pour le
 *   même header.
 *
 * SOLUTION : block_header_serialize_canonical() — sérialisation unique,
 *   déterministe, indépendante du padding/ABI/endianness compilateur.
 *   Format (88 octets, little-endian explicite) :
 *     [  0..  3] version       uint32_t LE
 *     [  4.. 35] prev_hash     32 octets verbatim
 *     [ 36.. 67] merkle_root   32 octets verbatim
 *     [ 68.. 75] timestamp     uint64_t LE
 *     [ 76.. 79] bits          uint32_t LE
 *     [ 80.. 87] nonce         uint64_t LE
 *   = LUMVORAX_HEADER_SERIAL_LEN (88) octets
 *
 * Toutes les fonctions de hash/mining/validation doivent utiliser cette
 * sérialisation. genesis.c a été mis à jour en conséquence (rapport 149).
 *
 * CERTIFIED_100=false | unique_human_proven=false
 * Mode DEBUG actif
 */
#include "blockchain_lumvorax.h"
#include <string.h>

extern void sha256_lumvorax(const uint8_t *data, size_t len, uint8_t out[32]);

/* Macro interne : écriture d'un entier N octets en little-endian dans un buffer. */
#define WRITE_LE32(buf, offset, val) do {           \
    (buf)[(offset)+0] = (uint8_t)((val)       & 0xFF); \
    (buf)[(offset)+1] = (uint8_t)(((val)>> 8) & 0xFF); \
    (buf)[(offset)+2] = (uint8_t)(((val)>>16) & 0xFF); \
    (buf)[(offset)+3] = (uint8_t)(((val)>>24) & 0xFF); \
} while(0)

#define WRITE_LE64(buf, offset, val) do {               \
    (buf)[(offset)+0] = (uint8_t)((val)        & 0xFF); \
    (buf)[(offset)+1] = (uint8_t)(((val)>> 8)  & 0xFF); \
    (buf)[(offset)+2] = (uint8_t)(((val)>>16)  & 0xFF); \
    (buf)[(offset)+3] = (uint8_t)(((val)>>24)  & 0xFF); \
    (buf)[(offset)+4] = (uint8_t)(((val)>>32)  & 0xFF); \
    (buf)[(offset)+5] = (uint8_t)(((val)>>40)  & 0xFF); \
    (buf)[(offset)+6] = (uint8_t)(((val)>>48)  & 0xFF); \
    (buf)[(offset)+7] = (uint8_t)(((val)>>56)  & 0xFF); \
} while(0)

int block_header_serialize_canonical(const block_header_t *h,
                                     uint8_t out[LUMVORAX_HEADER_SERIAL_LEN]) {
    if (!h || !out) return -1;
    WRITE_LE32(out,  0, h->version);
    memcpy(out +  4, h->prev_hash,   32);
    memcpy(out + 36, h->merkle_root, 32);
    WRITE_LE64(out, 68, h->timestamp);
    WRITE_LE32(out, 76, h->bits);
    WRITE_LE64(out, 80, h->nonce);
    return LUMVORAX_HEADER_SERIAL_LEN;
}

void block_header_hash(const block_header_t *h, uint8_t out_hash[32]) {
    if (!h || !out_hash) return;
    /* BL-013 + BL-015 FIX : sérialisation canonique — nonce et bits inclus,
     * indépendant du padding/ABI. Double-SHA256 (protocole LUMVORAX). */
    uint8_t buf[LUMVORAX_HEADER_SERIAL_LEN];
    block_header_serialize_canonical(h, buf);
    uint8_t mid[32];
    sha256_lumvorax(buf, LUMVORAX_HEADER_SERIAL_LEN, mid);
    sha256_lumvorax(mid, 32, out_hash);
}

int block_header_meets_difficulty(const uint8_t hash[32], uint32_t bits) {
    if (!hash) return 0;
    uint32_t lz = 0;
    for (int i = 0; i < 32; ++i) {
        uint8_t b = hash[i];
        if (b == 0) { lz += 8; continue; }
        while ((b & 0x80) == 0) { lz++; b <<= 1; }
        break;
    }
    return (lz >= bits) ? 1 : 0;
}
