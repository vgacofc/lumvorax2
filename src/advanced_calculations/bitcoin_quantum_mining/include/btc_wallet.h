/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_wallet.h — Wallet Bitcoin RÉEL avec secp256k1 via OpenSSL
 *
 * STANDARD_NAMES.md v4.2 §M-BTC17-WALLET — Cycle C63 — 2026-04-11
 *
 * Fonctionnalités :
 *  - Génération clé privée secp256k1 (256 bits CSPRNG OpenSSL)
 *  - Dérivation clé publique via multiplication point EC
 *  - Adresse Bitcoin P2PKH (HASH160 + Base58Check)
 *  - Adresse Bitcoin P2WPKH / Bech32 (SegWit native)
 *  - WIF (Wallet Import Format) pour export/import portefeuille
 *  - Traçabilité forensic 100% A–Z LumVorax
 *
 * Conformité STANDARD_NAMES.md §A : macros FORENSIC_LOG_* jamais renommées.
 * Conformité prompt.txt §3.1 : 0 stub, implémentation 100% réelle.
 */

#ifndef BTC_WALLET_H
#define BTC_WALLET_H

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ── Constantes wallet ──────────────────────────────────────────────────── */
#define BTC_WALLET_MAGIC        0xBE57C011u  /* "BTC-COIN" protection double-free */
#define BTC_PRIV_KEY_BYTES      32           /* Clé privée : 256 bits */
#define BTC_PUB_KEY_UNCOMP_BYTES 65          /* Non-compressée : 0x04 + x(32) + y(32) */
#define BTC_PUB_KEY_COMP_BYTES  33           /* Compressée : 0x02/0x03 + x(32) */
#define BTC_HASH160_BYTES       20           /* RIPEMD160(SHA256(pubkey)) */
#define BTC_ADDRESS_MAX_LEN     64           /* Adresse P2PKH / Bech32 */
#define BTC_WIF_MAX_LEN         64           /* Wallet Import Format */
#define BTC_BECH32_MAX_LEN      90           /* Adresse Bech32 (SegWit) */
#define BTC_TXID_HEX_LEN        65           /* Hash transaction hex (64 + \0) */

/* ── Réseau Bitcoin ─────────────────────────────────────────────────────── */
typedef enum {
    BTC_NETWORK_MAINNET  = 0,   /* 0x00 version byte pour adresses P2PKH */
    BTC_NETWORK_TESTNET3 = 1,   /* 0x6f version byte pour adresses testnet */
    BTC_NETWORK_REGTEST  = 2    /* 0x6f même que testnet3 */
} btc_network_e;

/* Préfixes réseau */
#define BTC_P2PKH_VERSION_MAINNET   0x00u
#define BTC_P2PKH_VERSION_TESTNET   0x6Fu
#define BTC_WIF_VERSION_MAINNET     0x80u
#define BTC_WIF_VERSION_TESTNET     0xEFu

/* ── Structure wallet Bitcoin complet ───────────────────────────────────── */
typedef struct {
    uint32_t magic;                          /* MAGIC — protection double-free */
    /* Clés cryptographiques */
    uint8_t  priv_key[BTC_PRIV_KEY_BYTES];   /* Clé privée secp256k1 */
    uint8_t  pub_key_unc[BTC_PUB_KEY_UNCOMP_BYTES]; /* Clé pub non-compressée */
    uint8_t  pub_key_cmp[BTC_PUB_KEY_COMP_BYTES];   /* Clé pub compressée */
    /* Hashes */
    uint8_t  hash160_unc[BTC_HASH160_BYTES]; /* HASH160 clé non-compressée */
    uint8_t  hash160_cmp[BTC_HASH160_BYTES]; /* HASH160 clé compressée */
    /* Adresses encodées */
    char     address_p2pkh[BTC_ADDRESS_MAX_LEN];  /* Adresse P2PKH principale */
    char     address_bech32[BTC_BECH32_MAX_LEN];  /* Adresse SegWit (P2WPKH) */
    char     wif_compressed[BTC_WIF_MAX_LEN];     /* WIF compressé (import wallet) */
    char     wif_uncompressed[BTC_WIF_MAX_LEN];   /* WIF non-compressé */
    /* Métadonnées LumVorax */
    char     run_id[64];        /* run_id du run de minage associé */
    uint64_t ts_created_ns;     /* Timestamp création (ns monotonique) */
    btc_network_e network;      /* Réseau : MAINNET / TESTNET3 / REGTEST */
    int      is_compressed;     /* 1 = utiliser clé pub compressée (standard) */
    /* Forensic */
    char     pub_key_hex[131];  /* Clé pub non-compressée en hex (130 + \0) */
    char     priv_key_hex[65];  /* Clé privée en hex (64 + \0) */
    char     hash160_hex[41];   /* HASH160 en hex (40 + \0) */
} lv_btc_wallet_t;

/* ── Résultat de création de wallet ─────────────────────────────────────── */
#define BTC_WALLET_RESULT_MAGIC 0xB17CA1C0u

typedef struct {
    uint32_t       magic;
    lv_btc_wallet_t* wallet;     /* Wallet créé (ou NULL si erreur) */
    int            error_code;   /* 0 = succès, <0 = erreur */
    char           error_msg[128];
    uint64_t       ts_ns;        /* Timestamp résultat */
} lv_btc_wallet_result_t;

/* ── Prototypes API ─────────────────────────────────────────────────────── */

/*
 * btc_wallet_create — Génère un nouveau wallet Bitcoin complet
 *  @network   : BTC_NETWORK_MAINNET ou BTC_NETWORK_TESTNET3
 *  @run_id    : run_id LumVorax (pour traçabilité)
 *  Retourne : lv_btc_wallet_t* alloué via LV_MALLOC, ou NULL si erreur
 *             Libérer avec btc_wallet_destroy()
 */
lv_btc_wallet_t* btc_wallet_create(btc_network_e network, const char* run_id);

/*
 * btc_wallet_from_privkey — Crée un wallet depuis une clé privée existante
 *  @priv_key_bytes : 32 bytes de clé privée secp256k1
 *  @network        : réseau cible
 *  @run_id         : run_id LumVorax
 */
lv_btc_wallet_t* btc_wallet_from_privkey(
    const uint8_t priv_key_bytes[32],
    btc_network_e network,
    const char*   run_id);

/*
 * btc_wallet_destroy — Libère un wallet de manière sécurisée (zeroize les clés)
 */
void btc_wallet_destroy(lv_btc_wallet_t* wallet);

/*
 * btc_wallet_print — Affiche le wallet (sans la clé privée complète)
 */
void btc_wallet_print(const lv_btc_wallet_t* wallet);

/*
 * btc_wallet_verify — Vérifie la cohérence cryptographique du wallet
 *  Retourne : 1 si cohérent, 0 si incohérent
 */
int btc_wallet_verify(const lv_btc_wallet_t* wallet);

/*
 * btc_wallet_save_json — Sauvegarde le wallet en JSON (clé privée masquée par défaut)
 *  @path         : chemin fichier
 *  @include_priv : 1 = inclure clé privée en clair (DANGEREUX), 0 = masquer
 */
int btc_wallet_save_json(const lv_btc_wallet_t* wallet,
                         const char* path, int include_priv);

/* ── Fonctions utilitaires cryptographiques ─────────────────────────────── */

/*
 * btc_hash160 — Calcule HASH160 = RIPEMD160(SHA256(data))
 *  Conforme Bitcoin Core hash160
 */
void btc_hash160(const uint8_t* data, size_t len, uint8_t out[20]);

/*
 * btc_base58check_encode — Encode en Base58Check (pour adresses et WIF)
 *  @data    : données à encoder
 *  @len     : longueur données
 *  @version : byte de version (préfixé)
 *  @out     : buffer résultat (min 64 bytes)
 *  Retourne : longueur chaîne ou -1 si erreur
 */
int btc_base58check_encode(const uint8_t* data, size_t len,
                           uint8_t version, char* out, size_t out_size);

/*
 * btc_bech32_encode — Encode une adresse Bech32 (P2WPKH)
 *  @hrp     : "bc" (mainnet) ou "tb" (testnet)
 *  @hash160 : 20 bytes HASH160
 *  @out     : buffer résultat (min 90 bytes)
 *  Retourne : longueur chaîne ou -1 si erreur
 */
int btc_bech32_encode(const char* hrp, const uint8_t hash160[20],
                      char* out, size_t out_size);

/*
 * btc_bytes_to_hex — Convertit des bytes en chaîne hexadécimale
 *  Conforme STANDARD_NAMES.md : bytes_to_hex_string (2025-01-07)
 */
void btc_bytes_to_hex(const uint8_t* data, size_t len, char* out);

#endif /* BTC_WALLET_H */
