/* test_btc_testnet3_runtime.c — Smoke test C99 P0.3
 * Vérifie : fetch tip → header 80 bytes → SHA256d == hash → cible compact
 */
#include "btc_testnet3_runtime.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    btc_testnet3_header_t hdr;
    int rc = btc_testnet3_fetch_header(-1, &hdr);
    if (rc != 0) {
        fprintf(stderr, "FAIL fetch rc=%d\n", rc);
        return 1;
    }
    printf("[testnet3] height=%u verified=%d bits=0x%08x diff=%.2f\n",
           hdr.height, hdr.verified, hdr.bits, hdr.difficulty);
    printf("[testnet3] hash = ");
    for (int i = 0; i < 32; i++) printf("%02x", hdr.hash[i]);
    printf("\n[testnet3] header[0..15] = ");
    for (int i = 0; i < 16; i++) printf("%02x", hdr.raw_header[i]);
    printf("...\n");

    uint8_t target[32];
    btc_testnet3_compact_to_target(hdr.bits, target);
    printf("[testnet3] target = ");
    for (int i = 0; i < 32; i++) printf("%02x", target[i]);
    printf("\n");

    int valid = btc_testnet3_hash_le_target(hdr.hash, target);
    printf("[testnet3] hash <= target ? %s\n", valid ? "YES (bloc valide)" : "NO");

    if (!hdr.verified) return 2;
    if (!valid) {
        fprintf(stderr, "WARN: hash > target (anomalie blockstream)\n");
        return 3;
    }
    printf("[testnet3] ✅ P0.3 RUNTIME OK\n");
    return 0;
}
