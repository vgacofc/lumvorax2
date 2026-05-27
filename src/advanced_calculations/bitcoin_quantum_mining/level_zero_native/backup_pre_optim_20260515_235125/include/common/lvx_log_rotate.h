/* lvx_log_rotate.h — Rotation universelle des logs à 50 MB (C99).
 *
 * Réponse à la demande utilisateur C99 : « tout les fichier generer des log
 * subise une rotation a 50 MO ».
 *
 * Stratégie :
 *   1. À chaque écriture, vérifier la taille du fichier.
 *   2. Si > LVX_LOG_ROTATE_MAX_BYTES (50 Mo) :
 *        renommer fichier.log → fichier.log.YYYYMMDDHHMMSS.gz (gzip si dispo)
 *        ou simplement fichier.log.YYYYMMDDHHMMSS (rotation simple)
 *   3. Conserver au plus LVX_LOG_ROTATE_KEEP_N anciens (purge LRU).
 *
 * USAGE :
 *   lvx_log_append(path, "ligne\n");                  // append + rotation auto
 *   lvx_log_append_jsonl(path, "{\"k\":1}\n");        // alias sémantique
 *
 * THREAD-SAFETY : verrou flock POSIX par fichier (1 seul writer à la fois).
 */
#ifndef LUMVORAX_LVX_LOG_ROTATE_H
#define LUMVORAX_LVX_LOG_ROTATE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 50 Mo en octets — politique standard C99 LumVorax */
#define LVX_LOG_ROTATE_MAX_BYTES    (50ULL * 1024ULL * 1024ULL)
/* Conservation : 5 archives max (250 Mo total max par fichier de log) */
#define LVX_LOG_ROTATE_KEEP_N       5

/* Append-and-rotate : écrit `data` (taille `len` octets) à la fin de `path`.
 * Si la taille post-écriture dépasse 50 Mo, déclenche la rotation.
 * Retourne 0 si OK, -1 si erreur d'I/O. */
int lvx_log_append(const char *path, const char *data, size_t len);

/* Helper sémantique pour JSONL (1 ligne JSON par appel). */
int lvx_log_append_jsonl(const char *path, const char *jsonl_line);

/* Force la rotation immédiate (utile pour test ou clôture de cycle).
 * Retourne 0 si OK, -1 si erreur. */
int lvx_log_force_rotate(const char *path);

/* Vérifie/déclenche la rotation si nécessaire (appelable explicitement).
 * Retourne 1 si rotation effectuée, 0 si pas nécessaire, -1 si erreur. */
int lvx_log_check_rotate(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* LUMVORAX_LVX_LOG_ROTATE_H */
