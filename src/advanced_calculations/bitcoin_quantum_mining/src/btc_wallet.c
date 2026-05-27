/*
 * LumVorax — Module 17 — Bitcoin Quantum Mining Engine
 * btc_wallet.c — Wallet Bitcoin RÉEL avec secp256k1 via OpenSSL
 *
 * STANDARD_NAMES.md v4.2 §M-BTC17-WALLET — Cycle C63 — 2026-04-11
 *
 * IMPLÉMENTATION 100% RÉELLE — Conformité prompt.txt §3.1 (ZÉRO STUB)
 *
 * Algorithme :
 *  1. RAND_bytes(32) → clé privée secp256k1
 *  2. EC_POINT_mul(G, priv) → clé publique (point secp256k1)
 *  3. HASH160 = RIPEMD160(SHA256(pub_key_compressée))
 *  4. Adresse P2PKH = Base58Check(version || HASH160)
 *  5. Adresse Bech32 = bech32_encode("bc"/"tb", 0x00, HASH160)
 *  6. WIF = Base58Check(0x80/0xEF || priv_key || 0x01)
 */

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L

#include "../include/btc_wallet.h"
#include "../include/btc_mining_forensic.h"
#include "debug/ultra_forensic_logger.h"
#include "lumvorax_integration.h"
#include "debug/memory_tracker.h"

/* OpenSSL pour secp256k1 */
#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/obj_mac.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

/* ── Timestamp monotonique ns ───────────────────────────────────────────── */
static uint64_t wallet_ts_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* ── Base58 alphabet Bitcoin ────────────────────────────────────────────── */
static const char BASE58_CHARS[] =
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

/* ── btc_bytes_to_hex ───────────────────────────────────────────────────── */
void btc_bytes_to_hex(const uint8_t* data, size_t len, char* out) {
    static const char HEX[] = "0123456789abcdef";
    for (size_t i = 0; i < len; i++) {
        out[2*i]   = HEX[(data[i] >> 4) & 0x0F];
        out[2*i+1] = HEX[data[i] & 0x0F];
    }
    out[2*len] = '\0';
}

/* ── btc_hash160 ────────────────────────────────────────────────────────── */
void btc_hash160(const uint8_t* data, size_t len, uint8_t out[20]) {
    uint8_t sha256_buf[32];
    SHA256(data, len, sha256_buf);
    RIPEMD160(sha256_buf, 32, out);
}

/* ── Base58 encode (sans version ni checksum) ───────────────────────────── */
static int base58_encode_raw(const uint8_t* data, size_t len,
                              char* out, size_t out_size) {
    BIGNUM* bn  = BN_bin2bn(data, (int)len, NULL);
    BIGNUM* div = BN_new();
    BIGNUM* rem = BN_new();
    BIGNUM* base = BN_new();
    BN_CTX* ctx = BN_CTX_new();

    if (!bn || !div || !rem || !base || !ctx) {
        BN_free(bn); BN_free(div); BN_free(rem); BN_free(base); BN_CTX_free(ctx);
        return -1;
    }

    BN_set_word(base, 58);

    /* Encode les chiffres Base58 en reverse */
    char tmp[256];
    int  tmp_len = 0;

    while (!BN_is_zero(bn)) {
        if (tmp_len >= (int)sizeof(tmp) - 1) break;
        BN_div(div, rem, bn, base, ctx);
        tmp[tmp_len++] = BASE58_CHARS[BN_get_word(rem)];
        BN_copy(bn, div);
    }

    /* Compter les zéros de tête (bytes 0x00 → '1' en Base58) */
    int leading_ones = 0;
    for (size_t i = 0; i < len && data[i] == 0x00; i++) leading_ones++;

    int total = leading_ones + tmp_len;
    if ((size_t)total >= out_size) {
        BN_free(bn); BN_free(div); BN_free(rem); BN_free(base); BN_CTX_free(ctx);
        return -1;
    }

    /* Ajouter '1' pour chaque leading zero */
    int pos = 0;
    for (int i = 0; i < leading_ones; i++) out[pos++] = '1';
    /* Inverser les chiffres */
    for (int i = tmp_len - 1; i >= 0; i--) out[pos++] = tmp[i];
    out[pos] = '\0';

    BN_free(bn); BN_free(div); BN_free(rem); BN_free(base); BN_CTX_free(ctx);
    return pos;
}

/* ── btc_base58check_encode ─────────────────────────────────────────────── */
int btc_base58check_encode(const uint8_t* data, size_t len,
                            uint8_t version, char* out, size_t out_size) {
    /* Payload = version || data */
    uint8_t payload[256];
    if (len + 1 > sizeof(payload) - 4) return -1;
    payload[0] = version;
    memcpy(payload + 1, data, len);
    size_t payload_len = len + 1;

    /* Checksum = SHA256(SHA256(payload))[0:4] */
    uint8_t hash1[32], hash2[32];
    SHA256(payload, payload_len, hash1);
    SHA256(hash1,   32,          hash2);

    memcpy(payload + payload_len, hash2, 4);
    payload_len += 4;

    return base58_encode_raw(payload, payload_len, out, out_size);
}

/* ── Bech32 ─────────────────────────────────────────────────────────────── */
/* Générateur de polynôme Bech32 */
static uint32_t bech32_polymod(const uint8_t* values, size_t len) {
    static const uint32_t GEN[5] = {
        0x3b6a57b2u, 0x26508e6du, 0x1ea119fau, 0x3d4233ddu, 0x2a1462b3u
    };
    uint32_t chk = 1;
    for (size_t i = 0; i < len; i++) {
        uint8_t b = (uint8_t)(chk >> 25);
        chk = ((chk & 0x1ffffffu) << 5) ^ values[i];
        for (int j = 0; j < 5; j++) {
            if ((b >> j) & 1) chk ^= GEN[j];
        }
    }
    return chk;
}

static const char BECH32_CHARSET[] = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

int btc_bech32_encode(const char* hrp, const uint8_t hash160[20],
                      char* out, size_t out_size) {
    /* Convertir hash160 (8-bit) en base32 (5-bit) avec witness version 0x00 */
    size_t hrp_len = strlen(hrp);

    /* data = witness_version(0) || convert8to5(hash160) */
    uint8_t data5[64];
    size_t  data5_len = 0;
    data5[data5_len++] = 0; /* witness version 0 */

    /* Convertir 20 bytes (160 bits) en 5-bit groups = 32 groups */
    uint32_t acc = 0;
    int      bits = 0;
    for (int i = 0; i < 20; i++) {
        acc = (acc << 8) | hash160[i];
        bits += 8;
        while (bits >= 5) {
            bits -= 5;
            data5[data5_len++] = (acc >> bits) & 0x1F;
        }
    }
    if (bits > 0) data5[data5_len++] = (acc << (5 - bits)) & 0x1F;

    /* Checksum : hrp_expand + data + 6 zeros */
    uint8_t enc[256];
    size_t  enc_len = 0;
    /* HRP expand */
    for (size_t i = 0; i < hrp_len; i++) enc[enc_len++] = hrp[i] >> 5;
    enc[enc_len++] = 0;
    for (size_t i = 0; i < hrp_len; i++) enc[enc_len++] = hrp[i] & 0x1F;
    for (size_t i = 0; i < data5_len; i++) enc[enc_len++] = data5[i];
    for (int i = 0; i < 6; i++) enc[enc_len++] = 0;
    uint32_t chk = bech32_polymod(enc, enc_len) ^ 1;

    /* Construire la chaîne finale */
    size_t pos = 0;
    for (size_t i = 0; i < hrp_len && pos < out_size - 1; i++)
        out[pos++] = hrp[i];
    if (pos < out_size - 1) out[pos++] = '1'; /* séparateur */
    for (size_t i = 0; i < data5_len && pos < out_size - 1; i++)
        out[pos++] = BECH32_CHARSET[data5[i]];
    for (int i = 0; i < 6 && pos < out_size - 1; i++)
        out[pos++] = BECH32_CHARSET[(chk >> (5 * (5 - i))) & 0x1F];
    out[pos] = '\0';
    return (int)pos;
}

/* ── Construction interne du wallet depuis une clé EC ───────────────────── */
static int wallet_fill_from_ec_key(lv_btc_wallet_t* w,
                                   EC_KEY* ec_key,
                                   btc_network_e network,
                                   const char* run_id) {
    const EC_GROUP* group = EC_KEY_get0_group(ec_key);
    const EC_POINT* pub   = EC_KEY_get0_public_key(ec_key);
    const BIGNUM*   priv  = EC_KEY_get0_private_key(ec_key);

    if (!group || !pub || !priv) return -1;

    /* ── Clé privée ────────────────────────────────────────── */
    memset(w->priv_key, 0, BTC_PRIV_KEY_BYTES);
    int priv_bytes = BN_num_bytes(priv);
    if (priv_bytes > BTC_PRIV_KEY_BYTES) return -2;
    /* Padder à gauche avec des zéros si moins de 32 bytes */
    BN_bn2bin(priv, w->priv_key + (BTC_PRIV_KEY_BYTES - priv_bytes));

    /* ── Clé publique non-compressée ──────────────────────── */
    size_t pub_unc_len = EC_POINT_point2oct(group, pub,
        POINT_CONVERSION_UNCOMPRESSED,
        w->pub_key_unc, BTC_PUB_KEY_UNCOMP_BYTES, NULL);
    if (pub_unc_len != BTC_PUB_KEY_UNCOMP_BYTES) return -3;

    /* ── Clé publique compressée ──────────────────────────── */
    size_t pub_cmp_len = EC_POINT_point2oct(group, pub,
        POINT_CONVERSION_COMPRESSED,
        w->pub_key_cmp, BTC_PUB_KEY_COMP_BYTES, NULL);
    if (pub_cmp_len != BTC_PUB_KEY_COMP_BYTES) return -4;

    /* ── HASH160 des deux formes ──────────────────────────── */
    btc_hash160(w->pub_key_unc, BTC_PUB_KEY_UNCOMP_BYTES, w->hash160_unc);
    btc_hash160(w->pub_key_cmp, BTC_PUB_KEY_COMP_BYTES,   w->hash160_cmp);

    /* ── Adresse P2PKH (compressée — standard moderne) ────── */
    uint8_t ver = (network == BTC_NETWORK_MAINNET)
                 ? BTC_P2PKH_VERSION_MAINNET : BTC_P2PKH_VERSION_TESTNET;
    if (btc_base58check_encode(w->hash160_cmp, BTC_HASH160_BYTES,
                                ver, w->address_p2pkh,
                                sizeof(w->address_p2pkh)) < 0)
        return -5;

    /* ── Adresse Bech32 (P2WPKH) ─────────────────────────── */
    const char* hrp = (network == BTC_NETWORK_MAINNET) ? "bc" : "tb";
    if (btc_bech32_encode(hrp, w->hash160_cmp,
                          w->address_bech32,
                          sizeof(w->address_bech32)) < 0)
        return -6;

    /* ── WIF compressé ────────────────────────────────────── */
    {
        uint8_t wif_buf[34]; /* priv_key || 0x01 (compressé) */
        memcpy(wif_buf, w->priv_key, BTC_PRIV_KEY_BYTES);
        wif_buf[32] = 0x01;
        uint8_t wif_ver = (network == BTC_NETWORK_MAINNET)
                         ? BTC_WIF_VERSION_MAINNET : BTC_WIF_VERSION_TESTNET;
        if (btc_base58check_encode(wif_buf, 33, wif_ver,
                                   w->wif_compressed,
                                   sizeof(w->wif_compressed)) < 0)
            return -7;
        memset(wif_buf, 0, sizeof(wif_buf)); /* Zeroize */
    }

    /* ── WIF non-compressé ────────────────────────────────── */
    {
        uint8_t wif_ver = (network == BTC_NETWORK_MAINNET)
                         ? BTC_WIF_VERSION_MAINNET : BTC_WIF_VERSION_TESTNET;
        if (btc_base58check_encode(w->priv_key, BTC_PRIV_KEY_BYTES,
                                   wif_ver, w->wif_uncompressed,
                                   sizeof(w->wif_uncompressed)) < 0)
            return -8;
    }

    /* ── Représentations hexadécimales ────────────────────── */
    btc_bytes_to_hex(w->priv_key,    BTC_PRIV_KEY_BYTES,         w->priv_key_hex);
    btc_bytes_to_hex(w->pub_key_unc, BTC_PUB_KEY_UNCOMP_BYTES,   w->pub_key_hex);
    btc_bytes_to_hex(w->hash160_cmp, BTC_HASH160_BYTES,           w->hash160_hex);

    /* ── Métadonnées ──────────────────────────────────────── */
    w->network       = network;
    w->is_compressed = 1;
    w->ts_created_ns = wallet_ts_ns();
    if (run_id) strncpy(w->run_id, run_id, sizeof(w->run_id) - 1);

    return 0;
}

/* ── btc_wallet_create ──────────────────────────────────────────────────── */
lv_btc_wallet_t* btc_wallet_create(btc_network_e network, const char* run_id) {
    uint64_t ts_start = wallet_ts_ns();
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_wallet_create_start", 1.0);

    /* Allouer structure wallet */
    lv_btc_wallet_t* w = LV_CALLOC(1, sizeof(lv_btc_wallet_t));
    if (!w) {
        FORENSIC_LOG_ANOMALY(BTC_MODULE_NAME, "btc_wallet_alloc_failed", 0.0);
        return NULL;
    }
    w->magic = BTC_WALLET_MAGIC;

    /* Créer groupe secp256k1 */
    EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    if (!group) {
        FORENSIC_LOG_ANOMALY(BTC_MODULE_NAME, "btc_wallet_ec_group_failed", 0.0);
        LV_FREE(w);
        return NULL;
    }

    /* Créer une nouvelle clé EC */
    EC_KEY* ec_key = EC_KEY_new();
    if (!ec_key || EC_KEY_set_group(ec_key, group) != 1) {
        FORENSIC_LOG_ANOMALY(BTC_MODULE_NAME, "btc_wallet_ec_key_failed", 0.0);
        EC_GROUP_free(group); LV_FREE(w);
        return NULL;
    }

    /* Générer la paire de clés (CSPRNG OpenSSL) */
    if (EC_KEY_generate_key(ec_key) != 1) {
        FORENSIC_LOG_ANOMALY(BTC_MODULE_NAME, "btc_wallet_ec_generate_failed", 0.0);
        EC_KEY_free(ec_key); EC_GROUP_free(group); LV_FREE(w);
        return NULL;
    }

    /* Remplir la structure wallet */
    int rc = wallet_fill_from_ec_key(w, ec_key, network, run_id);
    EC_KEY_free(ec_key);
    EC_GROUP_free(group);

    if (rc != 0) {
        FORENSIC_LOG_ANOMALY(BTC_MODULE_NAME, "btc_wallet_fill_failed", (double)rc);
        /* Zeroize avant libération */
        memset(w->priv_key, 0, BTC_PRIV_KEY_BYTES);
        LV_FREE(w);
        return NULL;
    }

    uint64_t elapsed_ns = wallet_ts_ns() - ts_start;
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_wallet_created", 1.0);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_wallet_create_ns", (double)elapsed_ns);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_wallet_network", (double)network);

    return w;
}

/* ── btc_wallet_from_privkey ────────────────────────────────────────────── */
lv_btc_wallet_t* btc_wallet_from_privkey(
    const uint8_t priv_key_bytes[32],
    btc_network_e network,
    const char*   run_id)
{
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_wallet_from_privkey_start", 1.0);

    lv_btc_wallet_t* w = LV_CALLOC(1, sizeof(lv_btc_wallet_t));
    if (!w) return NULL;
    w->magic = BTC_WALLET_MAGIC;

    EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    EC_KEY*   ec_key = EC_KEY_new();
    if (!group || !ec_key || EC_KEY_set_group(ec_key, group) != 1) {
        EC_KEY_free(ec_key); EC_GROUP_free(group); LV_FREE(w);
        return NULL;
    }

    /* Convertir les 32 bytes en BIGNUM */
    BIGNUM* priv_bn = BN_bin2bn(priv_key_bytes, 32, NULL);
    if (!priv_bn) {
        EC_KEY_free(ec_key); EC_GROUP_free(group); LV_FREE(w);
        return NULL;
    }

    /* Définir la clé privée */
    if (EC_KEY_set_private_key(ec_key, priv_bn) != 1) {
        BN_free(priv_bn); EC_KEY_free(ec_key); EC_GROUP_free(group); LV_FREE(w);
        return NULL;
    }

    /* Dériver la clé publique : pub_point = priv_bn * G */
    EC_POINT* pub_point = EC_POINT_new(group);
    BN_CTX*   ctx       = BN_CTX_new();
    if (!pub_point || !ctx ||
        EC_POINT_mul(group, pub_point, priv_bn, NULL, NULL, ctx) != 1) {
        EC_POINT_free(pub_point); BN_CTX_free(ctx); BN_free(priv_bn);
        EC_KEY_free(ec_key); EC_GROUP_free(group); LV_FREE(w);
        return NULL;
    }
    EC_KEY_set_public_key(ec_key, pub_point);

    BN_free(priv_bn);
    BN_CTX_free(ctx);
    EC_POINT_free(pub_point);

    int rc = wallet_fill_from_ec_key(w, ec_key, network, run_id);
    EC_KEY_free(ec_key);
    EC_GROUP_free(group);

    if (rc != 0) {
        memset(w->priv_key, 0, BTC_PRIV_KEY_BYTES);
        LV_FREE(w);
        return NULL;
    }

    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_wallet_from_privkey_ok", 1.0);
    return w;
}

/* ── btc_wallet_destroy ─────────────────────────────────────────────────── */
void btc_wallet_destroy(lv_btc_wallet_t* wallet) {
    if (!wallet || wallet->magic != BTC_WALLET_MAGIC) return;

    /* Zeroize les clés privées avant libération (sécurité) */
    memset(wallet->priv_key,      0, BTC_PRIV_KEY_BYTES);
    memset(wallet->wif_compressed,  0, sizeof(wallet->wif_compressed));
    memset(wallet->wif_uncompressed,0, sizeof(wallet->wif_uncompressed));
    memset(wallet->priv_key_hex,  0, sizeof(wallet->priv_key_hex));

    wallet->magic = 0xDEADC0DEu; /* Marqueur destruction */
    LV_FREE(wallet);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_wallet_destroyed", 1.0);
}

/* ── btc_wallet_print ───────────────────────────────────────────────────── */
void btc_wallet_print(const lv_btc_wallet_t* w) {
    if (!w || w->magic != BTC_WALLET_MAGIC) {
        printf("[BTC_WALLET] ERREUR: wallet invalide ou détruit\n");
        return;
    }

    const char* net_str = (w->network == BTC_NETWORK_MAINNET)
                         ? "MAINNET" : "TESTNET3";
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║        LumVorax — Wallet Bitcoin RÉEL                    ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║ Réseau      : %-46s║\n", net_str);
    printf("║ run_id      : %-46s║\n", w->run_id);
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║ Adresse P2PKH   : %-42s║\n", w->address_p2pkh);
    printf("║ Adresse Bech32  : %-42s║\n", w->address_bech32);
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    /* Masquer la clé privée - afficher seulement les 8 premiers chars */
    char priv_masked[70];
    snprintf(priv_masked, sizeof(priv_masked), "%.8s...(masquée)", w->priv_key_hex);
    printf("║ Clé privée (hex): %-42s║\n", priv_masked);
    printf("║ WIF compressé   : %-42s║\n", w->wif_compressed);
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    printf("║ HASH160 (hex)   : %-42s║\n", w->hash160_hex);
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    fflush(stdout);
}

/* ── btc_wallet_verify ──────────────────────────────────────────────────── */
int btc_wallet_verify(const lv_btc_wallet_t* w) {
    if (!w || w->magic != BTC_WALLET_MAGIC) return 0;

    /* Re-dériver le wallet depuis la clé privée et comparer */
    lv_btc_wallet_t* w2 = btc_wallet_from_privkey(w->priv_key, w->network, w->run_id);
    if (!w2) return 0;

    int ok = (memcmp(w->address_p2pkh, w2->address_p2pkh,
                     sizeof(w->address_p2pkh)) == 0) &&
             (memcmp(w->hash160_cmp, w2->hash160_cmp, BTC_HASH160_BYTES) == 0);

    btc_wallet_destroy(w2);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_wallet_verify_result", (double)ok);
    return ok;
}

/* ── btc_wallet_save_json ───────────────────────────────────────────────── */
int btc_wallet_save_json(const lv_btc_wallet_t* w,
                         const char* path, int include_priv) {
    if (!w || w->magic != BTC_WALLET_MAGIC || !path) return -1;

    /* Créer le répertoire parent si nécessaire */
    char dir_copy[512];
    strncpy(dir_copy, path, sizeof(dir_copy) - 1);
    char* slash = strrchr(dir_copy, '/');
    if (slash) { *slash = '\0'; mkdir(dir_copy, 0755); }

    FILE* f = fopen(path, "w");
    if (!f) return -2;

    fprintf(f, "{\n");
    fprintf(f, "  \"lumvorax_module\": 17,\n");
    fprintf(f, "  \"standard_names_version\": \"v4.2\",\n");
    fprintf(f, "  \"run_id\": \"%s\",\n", w->run_id);
    fprintf(f, "  \"network\": \"%s\",\n",
            w->network == BTC_NETWORK_MAINNET ? "MAINNET" : "TESTNET3");
    fprintf(f, "  \"ts_created_ns\": %"PRIu64",\n", w->ts_created_ns);
    fprintf(f, "  \"address_p2pkh\": \"%s\",\n", w->address_p2pkh);
    fprintf(f, "  \"address_bech32\": \"%s\",\n", w->address_bech32);
    fprintf(f, "  \"public_key_compressed_hex\": \"%s\",\n", w->pub_key_hex);
    fprintf(f, "  \"hash160_hex\": \"%s\",\n", w->hash160_hex);
    fprintf(f, "  \"wif_compressed\": \"%s\",\n", w->wif_compressed);
    if (include_priv) {
        fprintf(f, "  \"private_key_hex\": \"%s\",\n", w->priv_key_hex);
        fprintf(f, "  \"WARNING\": \"CLE PRIVEE EN CLAIR — GARDER SECRET\"\n");
    } else {
        fprintf(f, "  \"private_key_hex\": \"*** MASQUEE — include_priv=0 ***\"\n");
    }
    fprintf(f, "}\n");

    fclose(f);
    FORENSIC_LOG_MODULE_METRIC(BTC_MODULE_NAME, "btc_wallet_saved_json", 1.0);
    return 0;
}
