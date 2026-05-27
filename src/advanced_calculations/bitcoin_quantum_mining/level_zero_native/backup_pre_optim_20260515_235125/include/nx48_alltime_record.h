/* nx48_alltime_record.h — Persistance MONOTONE du record absolu (Cycle C100).
 *
 * RÈGLE D'OR : la valeur stockée ne décroît JAMAIS (monotone strictement).
 * Résout le bug C99-LOG : btc_nx48_last.csv contenait best=29 alors que
 *  les runs précédents avaient atteint 31, 35, 37 LZ → records perdus.
 *
 * Fichier dédié : config/btc_nx48_alltime.csv
 *  Format CSV (1 seul enregistrement, header en ligne 1) :
 *   schema_version,best_lz_alltime,best_nonce_alltime,
 *   header_hex_80B,wallet_address,
 *   run_id_first,run_id_last_update,
 *   ts_unix_first,ts_unix_last_update,update_count
 *
 * Atomicité : écriture via tmpfile + rename(2) + fsync.
 * Concurrence : verrou fcntl(F_WRLCK) pendant la lecture-modification-écriture.
 *
 * Hook WebSocket : à chaque mise à jour, l'appelant peut envoyer
 *   le record vers Replit via socketio (canal nx48_record_push).
 *
 * Conformité STANDARD_NAMES.md §M-BTC18-C100, prompt.txt v2.1.
 */
#ifndef NX48_ALLTIME_RECORD_H
#define NX48_ALLTIME_RECORD_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NX48_ALLTIME_SCHEMA_VERSION 1
#define NX48_ALLTIME_HEADER_HEX_LEN 161   /* 80 octets * 2 + nul */
#define NX48_ALLTIME_WALLET_LEN     96
#define NX48_ALLTIME_RUNID_LEN      96
#define NX48_ALLTIME_DEFAULT_PATH \
    "config/btc_nx48_alltime.csv"

typedef struct {
    int      schema_version;       /* = NX48_ALLTIME_SCHEMA_VERSION */
    int      best_lz_alltime;      /* RECORD ABSOLU monotone */
    uint32_t best_nonce_alltime;   /* nonce qui a produit ce record */
    char     header_hex[NX48_ALLTIME_HEADER_HEX_LEN];
    char     wallet_address[NX48_ALLTIME_WALLET_LEN];
    char     run_id_first[NX48_ALLTIME_RUNID_LEN];
    char     run_id_last[NX48_ALLTIME_RUNID_LEN];
    int64_t  ts_unix_first;
    int64_t  ts_unix_last;
    uint64_t update_count;
} nx48_alltime_record_t;

/* Initialise une structure vide neutre. */
void nx48_alltime_init_empty(nx48_alltime_record_t *r);

/* Charge le CSV depuis 'path'. Si le fichier n'existe pas → r vide + retourne 0.
 * Retour : 0 OK (existant ou créé vide), -1 erreur de parsing. */
int  nx48_alltime_load(const char *path, nx48_alltime_record_t *r);

/* Tente une mise à jour MONOTONE :
 *  - si lz_new > r.best_lz_alltime → maj r + persiste sur disque + retourne 1
 *  - sinon → ne touche rien et retourne 0
 *  - en erreur d'I/O → retourne -1
 * Atomique : tmp + rename + fsync, verrou fcntl. */
int  nx48_alltime_try_update(const char *path,
                             int  lz_new,
                             uint32_t nonce_new,
                             const char *header_hex_80B,
                             const char *wallet_address,
                             const char *run_id);

/* Lecture seule du fichier (pour exposition WebSocket). */
int  nx48_alltime_serialize_jsonl(const nx48_alltime_record_t *r,
                                  char *out, size_t out_size);

#ifdef __cplusplus
}
#endif
#endif /* NX48_ALLTIME_RECORD_H */
