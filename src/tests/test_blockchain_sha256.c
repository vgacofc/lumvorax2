/* **************************************************************************
** test_blockchain_sha256.c — Test SHA-256 + block_header_hash()
**
** Projet : ARTCB (Autonomous Reflexive Temporal Cognitive Blockchain)
** Module : blockchain_lumvorax / tests
** Auteur : ARTCB Project <contact@artcb.me>
**
** Vecteurs de test :
**   T01 — SHA-256("abc") = vecteur NIST FIPS 180-4
**   T02 — SHA-256("") = vecteur NIST FIPS 180-4
**   T03 — double-SHA256("abc") reproduit le comportement Bitcoin
**   T04 — block_header_hash() produit un digest non nul
**   T05 — block_header_hash() est déterministe (deux appels identiques)
**   T06 — block_header_hash() diffère si le header change
**   T07 — Symboles sha256_lumvorax + block_header_hash dans le binaire
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "../blockchain_lumvorax/blockchain_lumvorax.h"

/* Déclaration externe de sha256_lumvorax depuis sha256_mini.c */
extern void sha256_lumvorax(const uint8_t* data, size_t len, uint8_t* out);

/* ---- utilitaires -------------------------------------------------------- */

static void print_hex(const char* label, const uint8_t* buf, size_t len) {
    printf("  %s: ", label);
    for (size_t i = 0; i < len; ++i) printf("%02x", buf[i]);
    printf("\n");
}

static int bytes_all_zero(const uint8_t* buf, size_t len) {
    for (size_t i = 0; i < len; ++i) if (buf[i]) return 0;
    return 1;
}

/* ---- compteur de tests -------------------------------------------------- */

static int g_pass = 0;
static int g_fail = 0;

static void check(int cond, const char* id, const char* desc) {
    if (cond) { printf("  [PASS] %s — %s\n", id, desc); g_pass++; }
    else       { printf("  [FAIL] %s — %s\n", id, desc); g_fail++; }
}

/* ---- vecteurs NIST FIPS 180-4 ------------------------------------------ */

/* SHA-256("abc") */
static const uint8_t FIPS_ABC[32] = {
    0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,
    0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
    0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,
    0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad
};

/* SHA-256("") — message vide */
static const uint8_t FIPS_EMPTY[32] = {
    0xe3,0xb0,0xc4,0x42,0x98,0xfc,0x1c,0x14,
    0x9a,0xfb,0xf4,0xc8,0x99,0x6f,0xb9,0x24,
    0x27,0xae,0x41,0xe4,0x64,0x9b,0x93,0x4c,
    0xa4,0x95,0x99,0x1b,0x78,0x52,0xb8,0x55
};

/* Double-SHA256("abc") = SHA256(SHA256("abc"))
 * Vecteur calculé indépendamment (Python hashlib + C runtime session 145) :
 *   SHA256("abc") = ba7816bf...
 *   SHA256(SHA256("abc")) = 4f8b42c2...
 * Calculé par : python3 -c "import hashlib; r=hashlib.sha256(b'abc').digest(); print(hashlib.sha256(r).digest().hex())" */
static const uint8_t DOUBLE_SHA256_ABC[32] = {
    0x4f,0x8b,0x42,0xc2,0x2d,0xd3,0x72,0x9b,
    0x51,0x9b,0xa6,0xf6,0x8d,0x2d,0xa7,0xcc,
    0x5b,0x2d,0x60,0x6d,0x05,0xda,0xed,0x5a,
    0xd5,0x12,0x8c,0xc0,0x3e,0x6c,0x63,0x58
};

/* block_header_hash(version=1, all-zero fields except timestamp=1727712000, nonce=42, bits=0)
 * Vecteur recalculé (rapport 149, BL-013+BL-015 FIX) — sérialisation canonique 88 octets LE :
 *   version(4LE) + prev_hash(32) + merkle_root(32) + timestamp(8LE) + bits(4LE) + nonce(8LE)
 * python3: struct.pack('<IQQQ'...) → double-SHA256
 * = 27b8b9304208721bed3ba89297dc593d1d9c6840883bc62933042a9b7e921db0
 * Ancien vecteur (80 oct struct cast, BL-013) : c9c8cd4d... — invalide car nonce hors fenêtre */
static const uint8_t CANONICAL_HEADER_HASH[32] = {
    0x27,0xb8,0xb9,0x30,0x42,0x08,0x72,0x1b,
    0xed,0x3b,0xa8,0x92,0x97,0xdc,0x59,0x3d,
    0x1d,0x9c,0x68,0x40,0x88,0x3b,0xc6,0x29,
    0x33,0x04,0x2a,0x9b,0x7e,0x92,0x1d,0xb0
};

/* ---- tests ---------------------------------------------------------------- */

static void test_sha256_abc(void) {
    printf("\n[T01] SHA-256(\"abc\") — vecteur NIST FIPS 180-4\n");
    uint8_t out[32];
    sha256_lumvorax((const uint8_t*)"abc", 3, out);
    print_hex("sha256(abc)", out, 32);
    print_hex("expected   ", FIPS_ABC, 32);
    check(memcmp(out, FIPS_ABC, 32) == 0, "T01", "sha256(\"abc\") == FIPS 180-4 vecteur");
}

static void test_sha256_empty(void) {
    printf("\n[T02] SHA-256(\"\") — vecteur NIST FIPS 180-4\n");
    uint8_t out[32];
    sha256_lumvorax((const uint8_t*)"", 0, out);
    print_hex("sha256(\"\") ", out, 32);
    print_hex("expected   ", FIPS_EMPTY, 32);
    check(memcmp(out, FIPS_EMPTY, 32) == 0, "T02", "sha256(\"\") == FIPS 180-4 vecteur");
}

static void test_double_sha256_abc(void) {
    printf("\n[T03] Double-SHA256(\"abc\") — vecteur de référence calculé indépendamment\n");
    uint8_t round1[32], round2[32];
    sha256_lumvorax((const uint8_t*)"abc", 3, round1);
    sha256_lumvorax(round1, 32, round2);
    print_hex("double_sha256(abc)", round2, 32);
    print_hex("expected           ", DOUBLE_SHA256_ABC, 32);
    /* T03 : comparaison exacte avec vecteur de référence (Python hashlib indépendant) */
    check(memcmp(round2, DOUBLE_SHA256_ABC, 32) == 0, "T03",
          "double-SHA256(abc) == vecteur de reference calcule independamment");
    check(memcmp(round1, round2, 32) != 0, "T03b", "double-SHA256 differe du single-SHA256");
}

static void test_block_header_hash_canonical(void) {
    printf("\n[T04] block_header_hash() — vecteur canonique de reference\n");
    block_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.version = 1;
    hdr.timestamp = 1727712000ULL;
    hdr.nonce = 42;

    uint8_t out[32];
    block_header_hash(&hdr, out);
    print_hex("block_hash ", out, 32);
    print_hex("expected   ", CANONICAL_HEADER_HASH, 32);
    /* T04 : comparaison exacte avec vecteur calculé par Python hashlib (double-SHA256 des 80 oct) */
    check(memcmp(out, CANONICAL_HEADER_HASH, 32) == 0, "T04",
          "block_header_hash(canonical) == vecteur de reference calcule independamment");
    check(!bytes_all_zero(out, 32), "T04b", "block_header_hash() produit un digest non nul");
}

static void test_block_header_hash_deterministic(void) {
    printf("\n[T05] block_header_hash() — déterminisme\n");
    block_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.version = 1;
    hdr.timestamp = 1727712000ULL;
    hdr.nonce = 42;

    uint8_t out1[32], out2[32];
    block_header_hash(&hdr, out1);
    block_header_hash(&hdr, out2);
    check(memcmp(out1, out2, 32) == 0, "T05", "deux appels identiques → même hash");
}

static void test_block_header_hash_sensitivity(void) {
    printf("\n[T06] block_header_hash() — sensibilite au changement\n");
    /* BL-013 FIX (rapport 149) : sérialisation canonique — TOUS les champs
     * (version, prev_hash, merkle_root, timestamp, bits, nonce) entrent dans
     * le digest. Vérification : changer prev_hash, nonce ou bits doit produire
     * un hash différent. */
    block_header_t hdr1, hdr2;
    memset(&hdr1, 0, sizeof(hdr1));
    hdr1.version = 1; hdr1.timestamp = 1727712000ULL;
    hdr1.prev_hash[0] = 0xAA;

    memcpy(&hdr2, &hdr1, sizeof(hdr1));
    hdr2.prev_hash[0] = 0xBB;

    uint8_t out1[32], out2[32];
    block_header_hash(&hdr1, out1);
    block_header_hash(&hdr2, out2);
    print_hex("hash(prev_hash[0]=0xAA)", out1, 32);
    print_hex("hash(prev_hash[0]=0xBB)", out2, 32);
    check(memcmp(out1, out2, 32) != 0, "T06", "prev_hash different → hash different");

    /* T06b FIX (rapport 149, BL-013) : nonce inclus dans la sérialisation canonique —
     * changer le nonce DOIT maintenant produire un hash différent.
     * Ancien comportement (BL-013) : nonce hors fenêtre → hash identique.
     * Nouveau comportement attendu : nonce=42 ≠ nonce=999 → digest différent. */
    block_header_t hdr3;
    memcpy(&hdr3, &hdr1, sizeof(hdr1));
    hdr3.nonce = 999;
    uint8_t out3[32];
    block_header_hash(&hdr3, out3);
    print_hex("hash(nonce=0, base)  ", out1, 32);
    print_hex("hash(nonce=999)      ", out3, 32);
    check(memcmp(out1, out3, 32) != 0, "T06b",
          "BL-013 CORRIGE: nonce inclus dans serialisation canonique → hash different");

    /* T06c : bits inclus dans la sérialisation canonique */
    block_header_t hdr4;
    memcpy(&hdr4, &hdr1, sizeof(hdr1));
    hdr4.bits = 0x1d00ffff;
    uint8_t out4[32];
    block_header_hash(&hdr4, out4);
    check(memcmp(out1, out4, 32) != 0, "T06c",
          "BL-013 CORRIGE: bits inclus dans serialisation canonique → hash different");
}

static void test_sha256_nist_message_448bit(void) {
    printf("\n[T07] SHA-256(message 448 bits) — vecteur NIST additionnel\n");
    /* NIST FIPS 180-4, exemple message 448 bits = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" */
    const char* msg = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    static const uint8_t expected[32] = {
        0x24,0x8d,0x6a,0x61,0xd2,0x06,0x38,0xb8,
        0xe5,0xc0,0x26,0x93,0x0c,0x3e,0x60,0x39,
        0xa3,0x3c,0xe4,0x59,0x64,0xff,0x21,0x67,
        0xf6,0xec,0xed,0xd4,0x19,0xdb,0x06,0xc1
    };
    uint8_t out[32];
    sha256_lumvorax((const uint8_t*)msg, strlen(msg), out);
    print_hex("sha256(msg) ", out, 32);
    print_hex("expected    ", expected, 32);
    check(memcmp(out, expected, 32) == 0, "T07", "sha256(448-bit NIST msg) == vecteur attendu");
}

/* ---- main ----------------------------------------------------------------- */

int main(void) {
    printf("=== TEST BLOCKCHAIN SHA-256 / block_header_hash() ===\n");
    printf("Mode DEBUG actif | CERTIFIED_100=false | unique_human_proven=false\n");

    test_sha256_abc();
    test_sha256_empty();
    test_double_sha256_abc();
    test_block_header_hash_canonical();
    test_block_header_hash_deterministic();
    test_block_header_hash_sensitivity();
    test_sha256_nist_message_448bit();

    printf("\n=== RÉSULTATS ===\n");
    printf("  PASS : %d\n", g_pass);
    printf("  FAIL : %d\n", g_fail);
    printf("  TOTAL: %d\n", g_pass + g_fail);

    if (g_fail == 0) {
        printf("  STATUS : OK — 0 échec\n");
        return 0;
    } else {
        printf("  STATUS : ECHEC — %d test(s) en échec\n", g_fail);
        return 1;
    }
}
