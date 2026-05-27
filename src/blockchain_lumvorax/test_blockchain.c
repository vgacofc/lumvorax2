/* test_blockchain.c — Tests unitaires du module blockchain_lumvorax.
 *
 * Cycle C95. Couvre :
 *   - SHA-256 vecteurs FIPS 180-4 (chaîne vide, "abc")
 *   - block_header_meets_difficulty (leading_zeros)
 *   - merkle_root sur 1, 2, 3 transactions
 *
 * Compile : voir Makefile à la racine src/.
 */
#include "blockchain_lumvorax.h"
#include <stdio.h>
#include <string.h>

extern void sha256_lumvorax(const uint8_t *data, size_t len, uint8_t out[32]);

static int g_pass = 0, g_fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { g_pass++; printf("[OK]   %s\n", msg); } \
    else      { g_fail++; printf("[FAIL] %s\n", msg); } \
} while(0)

static int hex_eq(const uint8_t *bytes, const char *hex) {
    char buf[3] = {0,0,0};
    for (int i = 0; i < 32; ++i) {
        buf[0] = hex[i*2]; buf[1] = hex[i*2+1];
        unsigned v;
        sscanf(buf, "%x", &v);
        if (bytes[i] != (uint8_t)v) return 0;
    }
    return 1;
}

static void test_sha256_vectors(void) {
    uint8_t h[32];
    /* Vecteur 1 : chaîne vide */
    sha256_lumvorax((const uint8_t*)"", 0, h);
    CHECK(hex_eq(h, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"),
          "SHA256(\"\") = e3b0c4...");
    /* Vecteur 2 : "abc" */
    sha256_lumvorax((const uint8_t*)"abc", 3, h);
    CHECK(hex_eq(h, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"),
          "SHA256(\"abc\") = ba7816...");
}

static void test_difficulty(void) {
    uint8_t h0[32] = {0};   /* tout zéros = 256 leading zeros */
    CHECK(block_header_meets_difficulty(h0, 256) == 1, "256 leading zeros for all-zero hash");
    CHECK(block_header_meets_difficulty(h0, 32)  == 1, "32 LZ ok for all-zero hash");
    uint8_t h1[32] = {0};
    h1[0] = 0x01;   /* 7 leading zeros */
    CHECK(block_header_meets_difficulty(h1, 7) == 1, "7 LZ ok for 0x01...");
    CHECK(block_header_meets_difficulty(h1, 8) == 0, "8 LZ fails for 0x01...");
    uint8_t h2[32] = {0};
    h2[2] = 0x40;   /* 17 leading zeros */
    CHECK(block_header_meets_difficulty(h2, 17) == 1, "17 LZ ok for 0x000040...");
    CHECK(block_header_meets_difficulty(h2, 18) == 0, "18 LZ fails for 0x000040...");
}

static void test_merkle(void) {
    transaction_t tx[3] = {
        { .version = 1, .n_inputs = 1, .n_outputs = 1, .locktime = 0 },
        { .version = 1, .n_inputs = 1, .n_outputs = 2, .locktime = 0 },
        { .version = 1, .n_inputs = 2, .n_outputs = 1, .locktime = 100 },
    };
    uint8_t r1[32], r2[32], r3[32];
    merkle_root(tx, 1, r1);
    merkle_root(tx, 2, r2);
    merkle_root(tx, 3, r3);
    int z1 = 1, z2 = 1, z3 = 1;
    for (int i = 0; i < 32; ++i) {
        if (r1[i]) z1 = 0;
        if (r2[i]) z2 = 0;
        if (r3[i]) z3 = 0;
    }
    CHECK(!z1, "merkle_root(1 tx) non-nul");
    CHECK(!z2, "merkle_root(2 tx) non-nul");
    CHECK(!z3, "merkle_root(3 tx) non-nul (impair → duplique)");
    CHECK(memcmp(r1, r2, 32) != 0, "merkle_root différent pour 1 vs 2 tx");
}

int main(void) {
    printf("=== Tests blockchain_lumvorax (C95) ===\n");
    test_sha256_vectors();
    test_difficulty();
    test_merkle();
    printf("\nRésultat : %d/%d OK, %d FAIL\n", g_pass, g_pass + g_fail, g_fail);
    return g_fail == 0 ? 0 : 1;
}
