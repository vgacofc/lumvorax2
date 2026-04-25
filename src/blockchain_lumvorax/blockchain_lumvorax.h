/* blockchain_lumvorax.h — Blockchain décentralisée bâtie sur LUM DBMS.
 *
 * Cycle C95 — squelette d'API publique. Voir CAHIER_DES_CHARGES_C95_MAITRE.md §4.
 *
 * Architecture :
 *   Bloc        = .lum enrichi (magic LUMQ v2 + section BLOCKCHAIN)
 *   Chaîne      = WAL hash-chained (transaction_wal_extension.c) + Merkle root
 *   Consensus   = PoW NX48 (réutilise btc_mining_runner) ou PoS neural (axe A)
 *   P2P         = gossip UDP multicast + TCP fallback
 *   Wallet      = Ed25519 (libsodium ou implémentation manuelle)
 *
 * Tous les hashs sont SHA-256 (réutilisés du minier BTC).
 */
#ifndef LUMVORAX_BLOCKCHAIN_H
#define LUMVORAX_BLOCKCHAIN_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LUMVORAX_BLOCK_HASH_LEN     32   /* SHA-256 */
#define LUMVORAX_BLOCK_VERSION       1
#define LUMVORAX_GENESIS_HEIGHT      0
#define LUMVORAX_MAX_TX_PER_BLOCK 1024

/* ============================================================================
 * Header de bloc (80 octets compatibles Bitcoin + extensions LUMVORAX)
 * ============================================================================ */
typedef struct {
    uint32_t version;
    uint8_t  prev_hash[LUMVORAX_BLOCK_HASH_LEN];
    uint8_t  merkle_root[LUMVORAX_BLOCK_HASH_LEN];
    uint64_t timestamp;
    uint32_t bits;        /* difficulté encodée (compact form Bitcoin) */
    uint64_t nonce;       /* 64 bits (vs 32 Bitcoin) — espace NX48 plus grand */
    /* extensions LUMVORAX */
    uint64_t height;
    uint8_t  validator_pubkey[32];   /* pour PoS, sinon zero pour PoW */
    uint8_t  signature[64];          /* Ed25519 du producteur */
    double   neural_energy;          /* PoS axe A : énergie -E réseau */
    uint32_t lum_payload_size;       /* taille section .lum suivante */
} block_header_t;

/* Calcule SHA-256 du header (compatible Bitcoin pour les 80 premiers octets). */
void block_header_hash(const block_header_t *h, uint8_t out_hash[LUMVORAX_BLOCK_HASH_LEN]);

/* Vérifie si le hash satisfait la difficulté (leading_zeros >= bits). */
int block_header_meets_difficulty(const uint8_t hash[LUMVORAX_BLOCK_HASH_LEN], uint32_t bits);

/* ============================================================================
 * Transaction simple (input → outputs)
 * ============================================================================ */
typedef struct {
    uint8_t  prev_tx_hash[LUMVORAX_BLOCK_HASH_LEN];
    uint32_t prev_out_index;
    uint8_t  signature[64];
    uint8_t  pubkey[32];
} tx_input_t;

typedef struct {
    uint64_t value;       /* en LUM-satoshis (1 LUM = 10^8 LUM-sat) */
    uint8_t  pubkey_hash[20];   /* RIPEMD-160(SHA-256(pubkey)) */
} tx_output_t;

typedef struct {
    uint32_t version;
    uint32_t n_inputs;
    tx_input_t *inputs;
    uint32_t n_outputs;
    tx_output_t *outputs;
    uint32_t locktime;
} transaction_t;

void transaction_hash(const transaction_t *tx, uint8_t out_hash[LUMVORAX_BLOCK_HASH_LEN]);

/* ============================================================================
 * Merkle tree
 * ============================================================================ */
void merkle_root(const transaction_t *txs, size_t n_txs,
                 uint8_t out_root[LUMVORAX_BLOCK_HASH_LEN]);

/* ============================================================================
 * Bloc complet
 * ============================================================================ */
typedef struct {
    block_header_t header;
    uint32_t n_txs;
    transaction_t *txs;
} block_t;

/* Sérialisation .lum (intègre LUM DBMS) */
int block_serialize_lum(const block_t *b, const char *path);
block_t *block_deserialize_lum(const char *path);

/* ============================================================================
 * Consensus PoW (réutilise NX48 du minier BTC existant)
 * ============================================================================ */
typedef struct {
    uint32_t target_bits;
    uint64_t max_nonce;
    int      use_nx48;        /* 1 = pilotage NX48, 0 = brute force */
} consensus_pow_config_t;

/* Cherche un nonce qui valide le bloc. Retourne 1 si trouvé, 0 sinon. */
int consensus_pow_mine(block_t *b, const consensus_pow_config_t *cfg);

/* ============================================================================
 * Wallet Ed25519 (signatures)
 * ============================================================================ */
typedef struct {
    uint8_t public_key[32];
    uint8_t secret_key[64];   /* concat seed + pubkey */
} wallet_t;

int wallet_create(wallet_t *w);
int wallet_sign(const wallet_t *w, const uint8_t *msg, size_t msg_len,
                uint8_t signature[64]);
int wallet_verify(const uint8_t public_key[32], const uint8_t *msg, size_t msg_len,
                  const uint8_t signature[64]);

/* ============================================================================
 * Genèse + chaîne
 * ============================================================================ */
typedef struct {
    block_t **blocks;
    size_t   length;
    size_t   capacity;
} chain_t;

chain_t *chain_create(void);
void chain_destroy(chain_t *c);
int chain_append(chain_t *c, block_t *b);   /* valide + ajoute */
const block_t *chain_tip(const chain_t *c);

#ifdef __cplusplus
}
#endif

#endif /* LUMVORAX_BLOCKCHAIN_H */
