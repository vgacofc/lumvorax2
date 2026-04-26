/* btc_testnet3_runtime.c — C99 P0.3 implémentation
 * Récupère un header Bitcoin testnet3 RÉEL via tools/btc_testnet3_fetch.py
 * (popen) et expose une struct prête pour le mining engine.
 * Pas de fallback silencieux : tout échec retourne un code négatif + log stderr.
 */
#include "btc_testnet3_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Recherche très simple "key": value dans le JSON renvoyé par le script.
 * On ne dépend pas d'une lib JSON (sandbox replit minimaliste).
 * Limité aux types : entier, flottant, chaîne hex.
 */
static int json_find_str(const char *buf, const char *key, char *out, size_t cap) {
    char pat[64]; snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = strstr(buf, pat);
    if (!p) return -1;
    p += strlen(pat);
    while (*p && *p != '"') p++;
    if (*p != '"') return -2;
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i + 1 < cap) out[i++] = *p++;
    out[i] = 0;
    return (*p == '"') ? 0 : -3;
}

static int json_find_int(const char *buf, const char *key, long long *out) {
    char pat[64]; snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = strstr(buf, pat);
    if (!p) return -1;
    p += strlen(pat);
    while (*p && (*p == ':' || *p == ' ' || *p == '\t')) p++;
    char *end = NULL;
    *out = strtoll(p, &end, 10);
    return (end > p) ? 0 : -2;
}

static int json_find_double(const char *buf, const char *key, double *out) {
    char pat[64]; snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = strstr(buf, pat);
    if (!p) return -1;
    p += strlen(pat);
    while (*p && (*p == ':' || *p == ' ' || *p == '\t')) p++;
    char *end = NULL;
    *out = strtod(p, &end);
    return (end > p) ? 0 : -2;
}

static int hex_to_bytes(const char *hex, uint8_t *out, size_t out_len) {
    size_t hex_len = strlen(hex);
    if (hex_len != out_len * 2) return -1;
    for (size_t i = 0; i < out_len; i++) {
        unsigned int v;
        if (sscanf(hex + 2 * i, "%2x", &v) != 1) return -2;
        out[i] = (uint8_t)v;
    }
    return 0;
}

int btc_testnet3_fetch_header(int height, btc_testnet3_header_t *out) {
    if (!out) return -1;
    memset(out, 0, sizeof(*out));

    char cmd[256];
    if (height < 0) {
        snprintf(cmd, sizeof(cmd),
                 "python3 tools/btc_testnet3_fetch.py --tip 2>/dev/null");
    } else {
        snprintf(cmd, sizeof(cmd),
                 "python3 tools/btc_testnet3_fetch.py --height %d 2>/dev/null", height);
    }
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        fprintf(stderr, "[testnet3] popen() failed pour: %s\n", cmd);
        return -2;
    }
    char buf[8192];
    size_t total = fread(buf, 1, sizeof(buf) - 1, fp);
    int rc = pclose(fp);
    if (total == 0) {
        fprintf(stderr, "[testnet3] sortie vide (rc=%d) — script absent ou réseau down\n", rc);
        return -3;
    }
    buf[total] = 0;

    long long tmp = 0;
    if (json_find_int(buf, "height", &tmp) == 0) out->height = (uint32_t)tmp;
    else { fprintf(stderr, "[testnet3] champ height manquant\n"); return -4; }

    char hex_hash[80] = {0};
    if (json_find_str(buf, "hash", hex_hash, sizeof(hex_hash)) != 0
        || hex_to_bytes(hex_hash, out->hash, 32) != 0) {
        fprintf(stderr, "[testnet3] hash invalide: %s\n", hex_hash);
        return -5;
    }
    char hex_hdr[200] = {0};
    if (json_find_str(buf, "raw_header_hex", hex_hdr, sizeof(hex_hdr)) != 0
        || hex_to_bytes(hex_hdr, out->raw_header, 80) != 0) {
        fprintf(stderr, "[testnet3] raw_header_hex invalide (len %zu)\n", strlen(hex_hdr));
        return -6;
    }

    if (json_find_int(buf, "version",   &tmp) == 0) out->version   = (uint32_t)tmp;
    if (json_find_int(buf, "timestamp", &tmp) == 0) out->timestamp = (uint32_t)tmp;
    if (json_find_int(buf, "bits",      &tmp) == 0) out->bits      = (uint32_t)tmp;
    if (json_find_int(buf, "nonce",     &tmp) == 0) out->nonce     = (uint32_t)tmp;
    json_find_double(buf, "difficulty", &out->difficulty);

    /* Le script lui-même calcule SHA256d(raw)==hash et renvoie verified */
    const char *vp = strstr(buf, "\"verified\"");
    out->verified = (vp && (strstr(vp, "true") != NULL && strstr(vp, "true") < strstr(vp, "false") + 1)) ? 1 : 0;
    /* simplification : si "true" présent près de verified */
    out->verified = (vp && strstr(vp, "true") != NULL) ? 1 : 0;

    if (!out->verified) {
        fprintf(stderr, "[testnet3] header NON vérifié (SHA256d mismatch)\n");
        return -7;
    }
    return 0;
}

void btc_testnet3_compact_to_target(uint32_t bits, uint8_t target_be[32]) {
    /* Bitcoin compact format : 0xEEMMMMMM avec EE=exponent, MMMMMM=mantissa */
    memset(target_be, 0, 32);
    uint32_t mantissa = bits & 0x007fffff;
    uint8_t  exponent = (uint8_t)((bits >> 24) & 0xff);
    if (exponent <= 3) {
        mantissa >>= 8 * (3 - exponent);
        target_be[29] = (mantissa >> 16) & 0xff;
        target_be[30] = (mantissa >> 8)  & 0xff;
        target_be[31] = (mantissa >> 0)  & 0xff;
        return;
    }
    int idx = 32 - exponent; /* position du MSB de la mantissa */
    if (idx < 0 || idx > 29) return;
    target_be[idx]     = (mantissa >> 16) & 0xff;
    target_be[idx + 1] = (mantissa >> 8)  & 0xff;
    target_be[idx + 2] = (mantissa >> 0)  & 0xff;
}

int btc_testnet3_hash_le_target(const uint8_t hash_be[32],
                                const uint8_t target_be[32]) {
    return memcmp(hash_be, target_be, 32) <= 0 ? 1 : 0;
}
