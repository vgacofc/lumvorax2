#ifndef BTC_TESTNET3_RUNTIME_H
#define BTC_TESTNET3_RUNTIME_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* C99 P0.3 — Header testnet3 RÉEL au runtime
 * Utilise tools/btc_testnet3_fetch.py via popen(3) pour récupérer le bloc tip
 * (ou hauteur fixée) depuis blockstream.info. Aucun fallback silencieux.
 */

typedef struct {
    uint32_t height;
    uint8_t  hash[32];           /* big-endian (display order) */
    uint8_t  raw_header[80];     /* binaire LE prêt pour SHA256d */
    uint32_t version;
    uint32_t timestamp;
    uint32_t bits;
    uint32_t nonce;
    double   difficulty;
    int      verified;           /* 1 = SHA256d(raw)==hash */
} btc_testnet3_header_t;

/* Récupère le tip courant. height=-1 pour tip, sinon hauteur fixe.
 * Retourne 0 OK / négatif erreur (script Python introuvable, JSON invalide,
 * non-vérifié). Logge stderr en cas d'échec — pas de fallback silencieux.
 */
int btc_testnet3_fetch_header(int height, btc_testnet3_header_t *out);

/* Calcule la cible 256-bit depuis le champ bits compact Bitcoin */
void btc_testnet3_compact_to_target(uint32_t bits, uint8_t target_be[32]);

/* Compare hash (BE) <= target (BE) (tous deux 32 bytes display-order) */
int  btc_testnet3_hash_le_target(const uint8_t hash_be[32],
                                 const uint8_t target_be[32]);

#ifdef __cplusplus
}
#endif

#endif
