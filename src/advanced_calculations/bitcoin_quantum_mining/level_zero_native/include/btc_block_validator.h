/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_block_validator.h — Validateur de blocs Bitcoin + forensic
 *
 * STANDARD_NAMES.md v4.2 §M-BTC17 — Cycle C63 — 2026-04-11
 *
 * Rôle :
 *  - Valider qu'un hash de bloc est < target (preuve de travail)
 *  - Vérifier la cohérence du header Bitcoin (version, bits, timestamp)
 *  - Construire un coinbase transaction pointant vers l'adresse du wallet
 *  - Sérialiser le bloc pour soumission réseau
 *  - Logging forensic complet de chaque validation
 *
 * Conformité STANDARD_NAMES.md §A : macros FORENSIC_LOG_* jamais renommées.
 */

#ifndef BTC_BLOCK_VALIDATOR_H
#define BTC_BLOCK_VALIDATOR_H

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "btc_wallet.h"
#include "sha256_lumvorax.h"

/* ── Constantes validateur ──────────────────────────────────────────────── */
#define BTC_BLOCK_VALIDATOR_MAGIC   0xB10CB10Cu  /* Protection double-free */
#define BTC_BLOCK_RESULT_MAGIC      0xB10C4E50u
#define BTC_MAX_BLOCK_SIZE          1000000      /* 1 MB max bloc Bitcoin */
#define BTC_COINBASE_MAX_BYTES      100          /* Taille max script coinbase */
#define BTC_BLOCK_REWARD_SATOSHI    625000000LL  /* 6.25 BTC en satoshis (post-halving 2020) */
#define BTC_BLOCK_SUBSIDY_2024      312500000LL  /* 3.125 BTC (halving 2024) */

/* ── Structure bloc Bitcoin complet (header + coinbase) ─────────────────── */
typedef struct {
    uint32_t magic;                  /* Protection double-free */

    /* Header Bitcoin (80 bytes standardisés) */
    lv_btc_block_header_t header;    /* version, prev_hash, merkle, ts, bits, nonce */

    /* Hash du bloc (double-SHA256 du header) */
    uint8_t  block_hash[32];
    char     block_hash_hex[65];

    /* Résultat validation */
    int      leading_zeros;          /* Nombre de bits zéro en tête */
    bool     below_target;           /* hash < target (bloc valide) */
    bool     timestamp_valid;        /* Timestamp dans fenêtre ±2h */
    bool     bits_valid;             /* Bits de difficulté cohérents */
    bool     fully_valid;            /* Toutes les validations passées */

    /* Target */
    uint8_t  target[32];
    char     target_hex[65];

    /* Coinbase transaction */
    char     coinbase_addr[64];      /* Adresse destinataire récompense */
    int64_t  reward_satoshi;         /* Récompense en satoshis */
    uint8_t  coinbase_tx[256];       /* Transaction coinbase sérialisée */
    size_t   coinbase_tx_len;        /* Longueur transaction coinbase */
    uint8_t  coinbase_txid[32];      /* TXID coinbase */
    char     coinbase_txid_hex[65];

    /* Merkle root recalculée (avec coinbase) */
    uint8_t  merkle_root[32];
    char     merkle_root_hex[65];

    /* Timestamps forensic */
    uint64_t ts_validated_ns;        /* Timestamp validation */
    uint64_t validation_elapsed_ns;  /* Durée validation */

    /* run_id LumVorax */
    char     run_id[64];
} lv_btc_validated_block_t;

/* ── Prototypes API ─────────────────────────────────────────────────────── */

/*
 * btc_block_validate — Valide un bloc Bitcoin complet
 *  @header  : header Bitcoin avec nonce trouvé
 *  @target  : target de difficulté (32 bytes, big-endian)
 *  @wallet  : wallet pour la transaction coinbase (peut être NULL)
 *  @run_id  : run_id LumVorax
 *  Retourne : lv_btc_validated_block_t* alloué, ou NULL si erreur système
 *             Libérer avec btc_validated_block_destroy()
 */
lv_btc_validated_block_t* btc_block_validate(
    const lv_btc_block_header_t* header,
    const uint8_t                target[32],
    const lv_btc_wallet_t*       wallet,
    const char*                  run_id);

/*
 * btc_block_validate_from_hash — Valide depuis un hash déjà calculé
 */
lv_btc_validated_block_t* btc_block_validate_from_hash(
    const lv_btc_block_header_t* header,
    const uint8_t                block_hash[32],
    const uint8_t                target[32],
    const lv_btc_wallet_t*       wallet,
    const char*                  run_id);

/*
 * btc_validated_block_destroy — Libère un bloc validé
 */
void btc_validated_block_destroy(lv_btc_validated_block_t* block);

/*
 * btc_block_print_report — Affiche le rapport complet du bloc
 */
void btc_block_print_report(const lv_btc_validated_block_t* block);

/*
 * btc_block_save_report — Sauvegarde le rapport en JSON
 */
int btc_block_save_report(const lv_btc_validated_block_t* block,
                          const char* dir, const char* run_id);

/*
 * btc_count_leading_zero_bits — Compte les bits zéro en tête d'un hash
 */
int btc_count_leading_zero_bits(const uint8_t hash[32]);

/*
 * btc_hash_below_target — Vérifie si hash < target (comparaison big-endian)
 */
bool btc_hash_below_target(const uint8_t hash[32], const uint8_t target[32]);

/*
 * btc_build_coinbase_tx — Construit la transaction coinbase
 *  @height         : hauteur du bloc (pour script coinbase BIP34)
 *  @address        : adresse destinataire (P2PKH ou Bech32)
 *  @reward_satoshi : montant récompense
 *  @extra_nonce    : nonce supplémentaire (identifiant mineur)
 *  @tx_out         : buffer sortie
 *  @tx_out_size    : taille buffer
 *  Retourne : longueur transaction ou -1 si erreur
 */
int btc_build_coinbase_tx(uint32_t height, const char* address,
                          int64_t reward_satoshi, uint32_t extra_nonce,
                          uint8_t* tx_out, size_t tx_out_size);

#endif /* BTC_BLOCK_VALIDATOR_H */
