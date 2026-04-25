/* merkle_tree.c — Construction de la racine Merkle (compatible Bitcoin).
 *
 * Cycle C95. Hash double-SHA256 par paires, duplique le dernier si nombre impair.
 */
#include "blockchain_lumvorax.h"
#include <stdlib.h>
#include <string.h>

extern void sha256_lumvorax(const uint8_t *data, size_t len, uint8_t out[32]);

static void hash_pair(const uint8_t a[32], const uint8_t b[32], uint8_t out[32]) {
    uint8_t buf[64];
    memcpy(buf, a, 32);
    memcpy(buf + 32, b, 32);
    uint8_t tmp[32];
    sha256_lumvorax(buf, 64, tmp);
    sha256_lumvorax(tmp, 32, out);   /* double-SHA256 façon Bitcoin */
}

void transaction_hash(const transaction_t *tx, uint8_t out[32]) {
    if (!tx || !out) {
        if (out) memset(out, 0, 32);
        return;
    }
    /* Sérialisation minimale pour hash : version + n_inputs + n_outputs + locktime */
    uint8_t buf[16];
    memcpy(buf,      &tx->version,   4);
    memcpy(buf + 4,  &tx->n_inputs,  4);
    memcpy(buf + 8,  &tx->n_outputs, 4);
    memcpy(buf + 12, &tx->locktime,  4);
    uint8_t tmp[32];
    sha256_lumvorax(buf, 16, tmp);
    sha256_lumvorax(tmp, 32, out);
}

void merkle_root(const transaction_t *txs, size_t n_txs, uint8_t out_root[32]) {
    if (!out_root) return;
    if (!txs || n_txs == 0) {
        memset(out_root, 0, 32);
        return;
    }
    /* Niveau 0 : hash de chaque transaction */
    uint8_t (*level)[32] = malloc(sizeof(uint8_t[32]) * n_txs);
    if (!level) { memset(out_root, 0, 32); return; }
    for (size_t i = 0; i < n_txs; ++i) {
        transaction_hash(&txs[i], level[i]);
    }
    size_t cur = n_txs;
    while (cur > 1) {
        const size_t next = (cur + 1) / 2;
        for (size_t i = 0; i < next; ++i) {
            const size_t l = i * 2;
            const size_t r = (l + 1 < cur) ? l + 1 : l;   /* duplique si impair */
            hash_pair(level[l], level[r], level[i]);
        }
        cur = next;
    }
    memcpy(out_root, level[0], 32);
    free(level);
}
