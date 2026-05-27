/*
 * LumVorax — LUM DBMS Phase 3 — Index B-Tree
 * lum_btree.h — Arbre B d'ordre 32 (64 fils max par noeud)
 *
 * IMPLEMENTATION 100% NATIVE — pas de bibliotheque externe.
 * Structure : B-Tree d'ordre M=32 (capacite max 2M-1=63 cles par noeud).
 * Cles    : uint64_t (ex: hash record, timestamp, nonce)
 * Valeurs : uint64_t (offset fichier .lum ou ID interne)
 *
 * Operations :
 *   lum_btree_insert(tree, key, value)  -> O(log N)
 *   lum_btree_search(tree, key)         -> O(log N)
 *   lum_btree_range(tree, lo, hi, cb)   -> O(log N + K)
 *   lum_btree_delete(tree, key)         -> O(log N)
 *   lum_btree_count(tree)               -> O(1)
 *
 * Cycle C69 — LUM DBMS Phase 3 — 2026-04-22
 * STANDARD_NAMES.md v4.4 par M-LUM-DBMS-C69
 */
#ifndef LUM_BTREE_H
#define LUM_BTREE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Ordre du B-Tree : chaque noeud contient au plus 2*M-1 cles */
#define LUM_BTREE_ORDER     32
#define LUM_BTREE_MAX_KEYS  (2 * LUM_BTREE_ORDER - 1)   /* 63 */
#define LUM_BTREE_MIN_KEYS  (LUM_BTREE_ORDER - 1)        /* 31 */
#define LUM_BTREE_MAX_CHILD (2 * LUM_BTREE_ORDER)        /* 64 */

/* Valeur sentinelle "cle non trouvee" */
#define LUM_BTREE_NOT_FOUND UINT64_MAX

/* Noeud B-Tree */
typedef struct lum_btree_node {
    uint64_t                keys[LUM_BTREE_MAX_KEYS];     /* Cles triees */
    uint64_t                vals[LUM_BTREE_MAX_KEYS];     /* Valeurs associees */
    struct lum_btree_node*  children[LUM_BTREE_MAX_CHILD];/* Fils (NULL si feuille) */
    int                     n_keys;                       /* Nombre de cles actuelles */
    bool                    is_leaf;                      /* Noeud feuille ? */
} lum_btree_node_t;

/* Arbre B complet */
typedef struct {
    lum_btree_node_t* root;    /* Racine de l'arbre */
    uint64_t          count;   /* Nombre total de cles */
    int               height;  /* Hauteur (racine=0) */
} lum_btree_t;

/* Callback pour lum_btree_range : appele pour chaque (key, value) dans [lo, hi] */
typedef void (*lum_btree_range_cb)(uint64_t key, uint64_t value, void* user_data);

/* ── API publique ─────────────────────────────────────────────────── */

/* Cree un arbre B vide */
lum_btree_t* lum_btree_create(void);

/* Detruit l'arbre et libere toute la memoire */
void lum_btree_destroy(lum_btree_t* tree);

/* Insere (key, value). Si key existe deja, met a jour value. */
bool lum_btree_insert(lum_btree_t* tree, uint64_t key, uint64_t value);

/* Cherche la valeur pour key. Retourne LUM_BTREE_NOT_FOUND si absent. */
uint64_t lum_btree_search(lum_btree_t* tree, uint64_t key);

/* Supprime key de l'arbre. Retourne true si supprime, false si absent. */
bool lum_btree_delete(lum_btree_t* tree, uint64_t key);

/* Parcourt tous les (key, value) avec lo <= key <= hi. */
void lum_btree_range(lum_btree_t* tree,
                     uint64_t lo, uint64_t hi,
                     lum_btree_range_cb cb, void* user_data);

/* Nombre de cles dans l'arbre */
uint64_t lum_btree_count(const lum_btree_t* tree);

/* Dump textuel pour debug */
void lum_btree_dump(const lum_btree_t* tree, int max_keys);

/* Validation interne de la structure B-Tree (toutes proprietes) */
bool lum_btree_validate(const lum_btree_t* tree);

#endif /* LUM_BTREE_H */
