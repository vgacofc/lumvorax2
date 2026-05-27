/*
 * LumVorax — LUM DBMS Phase 3 — Index B-Tree natif
 * lum_btree.c — Implementation 100% native, aucune bibliotheque externe
 */
#define _POSIX_C_SOURCE 200809L
/*
 *
 * B-Tree d'ordre M=32 :
 *   - Noeud interne : 31 a 63 cles, 32 a 64 fils
 *   - Noeud feuille : 31 a 63 (cle, valeur) pairs
 *   - Hauteur max pour 4 milliards de cles : log32(4e9) ~= 6 niveaux
 *   - Recherche : O(log32 N) comparaisons = tres rapide
 *
 * Toutes les proprietes B-Tree sont maintenues :
 *   P1 : Chaque noeud non-racine a au moins M-1=31 cles
 *   P2 : Chaque noeud a au plus 2M-1=63 cles
 *   P3 : Toutes les feuilles sont au meme niveau
 *   P4 : Les cles sont triees dans chaque noeud
 *
 * Cycle C69 — LUM DBMS Phase 3 — 2026-04-22
 */

#include "lum_btree.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* ── Allocation / liberation d'un noeud ─────────────────────────── */

static lum_btree_node_t* _node_alloc(bool is_leaf) {
    lum_btree_node_t* n = (lum_btree_node_t*)calloc(1, sizeof(lum_btree_node_t));
    if (!n) return NULL;
    n->is_leaf = is_leaf;
    n->n_keys  = 0;
    return n;
}

static void _node_free_recursive(lum_btree_node_t* n) {
    if (!n) return;
    if (!n->is_leaf) {
        for (int i = 0; i <= n->n_keys; i++)
            _node_free_recursive(n->children[i]);
    }
    free(n);
}

/* ── Creation / destruction de l'arbre ──────────────────────────── */

lum_btree_t* lum_btree_create(void) {
    lum_btree_t* t = (lum_btree_t*)calloc(1, sizeof(lum_btree_t));
    if (!t) return NULL;
    t->root   = _node_alloc(true);
    t->count  = 0;
    t->height = 0;
    if (!t->root) { free(t); return NULL; }
    return t;
}

void lum_btree_destroy(lum_btree_t* tree) {
    if (!tree) return;
    _node_free_recursive(tree->root);
    free(tree);
}

uint64_t lum_btree_count(const lum_btree_t* tree) {
    return tree ? tree->count : 0;
}

/* ── Recherche dans un noeud : indice de la premiere cle >= key ── */
static int _node_lower_bound(const lum_btree_node_t* n, uint64_t key) {
    int lo = 0, hi = n->n_keys;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (n->keys[mid] < key) lo = mid + 1;
        else                    hi = mid;
    }
    return lo;
}

/* ── Recherche dans l'arbre ─────────────────────────────────────── */
uint64_t lum_btree_search(lum_btree_t* tree, uint64_t key) {
    if (!tree || !tree->root) return LUM_BTREE_NOT_FOUND;
    lum_btree_node_t* cur = tree->root;
    while (cur) {
        int i = _node_lower_bound(cur, key);
        if (i < cur->n_keys && cur->keys[i] == key)
            return cur->vals[i];
        if (cur->is_leaf)
            return LUM_BTREE_NOT_FOUND;
        cur = cur->children[i];
    }
    return LUM_BTREE_NOT_FOUND;
}

/* ── Parcours range [lo, hi] ────────────────────────────────────── */
static void _range_recursive(lum_btree_node_t* n,
                              uint64_t lo, uint64_t hi,
                              lum_btree_range_cb cb, void* ud) {
    if (!n) return;
    int i = _node_lower_bound(n, lo);
    while (i <= n->n_keys) {
        /* Descendre dans fils gauche si noeud interne */
        if (!n->is_leaf && i < n->n_keys + 1)
            _range_recursive(n->children[i], lo, hi, cb, ud);
        if (i >= n->n_keys) break;
        if (n->keys[i] > hi) break;
        if (n->keys[i] >= lo)
            cb(n->keys[i], n->vals[i], ud);
        i++;
    }
    /* Dernier fils */
    if (!n->is_leaf && i == n->n_keys + 1 && i > 0 &&
        n->keys[n->n_keys - 1] < hi)
        _range_recursive(n->children[n->n_keys], lo, hi, cb, ud);
}

void lum_btree_range(lum_btree_t* tree, uint64_t lo, uint64_t hi,
                     lum_btree_range_cb cb, void* ud) {
    if (!tree || !tree->root || !cb || lo > hi) return;
    _range_recursive(tree->root, lo, hi, cb, ud);
}

/* ── Split d'un noeud fils plein ───────────────────────────────── */
/* parent->children[child_idx] est plein (2M-1 cles).
 * On coupe en deux et on fait monter la cle mediane dans parent. */
static void _split_child(lum_btree_node_t* parent, int child_idx) {
    lum_btree_node_t* full  = parent->children[child_idx];
    lum_btree_node_t* right = _node_alloc(full->is_leaf);
    if (!right) return; /* OOM */

    int mid = LUM_BTREE_ORDER - 1; /* = M-1 = 31 : index de la cle mediane */

    /* Copier la moitie droite dans right */
    right->n_keys = full->n_keys - mid - 1;
    for (int j = 0; j < right->n_keys; j++) {
        right->keys[j] = full->keys[mid + 1 + j];
        right->vals[j] = full->vals[mid + 1 + j];
    }
    if (!full->is_leaf) {
        for (int j = 0; j <= right->n_keys; j++)
            right->children[j] = full->children[mid + 1 + j];
    }
    full->n_keys = mid;

    /* Inserer right dans parent->children */
    for (int j = parent->n_keys; j >= child_idx + 1; j--)
        parent->children[j + 1] = parent->children[j];
    parent->children[child_idx + 1] = right;

    /* Faire monter la cle mediane dans parent */
    for (int j = parent->n_keys - 1; j >= child_idx; j--) {
        parent->keys[j + 1] = parent->keys[j];
        parent->vals[j + 1] = parent->vals[j];
    }
    parent->keys[child_idx] = full->keys[mid];
    parent->vals[child_idx] = full->vals[mid];
    parent->n_keys++;
}

/* ── Insertion dans un noeud non plein ──────────────────────────── */
/* Retourne false si la cle existait deja (on met juste a jour la valeur). */
static bool _insert_non_full(lum_btree_node_t* n, uint64_t key, uint64_t value) {
    int i = n->n_keys - 1;
    if (n->is_leaf) {
        /* Chercher la position d'insertion et decaler */
        while (i >= 0 && key < n->keys[i]) {
            n->keys[i + 1] = n->keys[i];
            n->vals[i + 1] = n->vals[i];
            i--;
        }
        if (i >= 0 && n->keys[i] == key) {
            /* Cle existe deja : mise a jour */
            n->keys[i + 1] = n->keys[i]; /* annuler le decalage */
            n->vals[i + 1] = n->vals[i];
            /* Retroceder */
            n->vals[i] = value;
            /* Annuler l'insertion */
            n->keys[i + 1] = n->keys[i]; /* rien a faire, deja annule */
            /* En fait : si key == n->keys[i], on ne doit PAS inserer */
            /* Retroceder le decalage effectue */
            for (int j = i + 1; j < n->n_keys; j++) {
                n->keys[j] = n->keys[j + 1];
                n->vals[j] = n->vals[j + 1];
            }
            n->vals[i] = value;
            return false; /* pas de nouvelle cle */
        }
        n->keys[i + 1] = key;
        n->vals[i + 1] = value;
        n->n_keys++;
        return true;
    } else {
        /* Noeud interne : trouver le fils adequat */
        while (i >= 0 && key < n->keys[i]) i--;
        /* Verifier si la cle existe dans ce noeud */
        if (i >= 0 && n->keys[i] == key) {
            n->vals[i] = value;
            return false;
        }
        i++; /* i = indice du fils */
        if (n->children[i]->n_keys == LUM_BTREE_MAX_KEYS) {
            _split_child(n, i);
            /* Apres split, determiner dans quel fils aller */
            if (key > n->keys[i])  i++;
            else if (key == n->keys[i]) {
                n->vals[i] = value;
                return false;
            }
        }
        return _insert_non_full(n->children[i], key, value);
    }
}

/* ── Insertion publique ─────────────────────────────────────────── */
bool lum_btree_insert(lum_btree_t* tree, uint64_t key, uint64_t value) {
    if (!tree) return false;
    lum_btree_node_t* root = tree->root;

    if (root->n_keys == LUM_BTREE_MAX_KEYS) {
        /* Racine pleine : creer une nouvelle racine */
        lum_btree_node_t* new_root = _node_alloc(false);
        if (!new_root) return false;
        new_root->children[0] = root;
        tree->root = new_root;
        tree->height++;
        _split_child(new_root, 0);
        root = new_root;
    }

    bool inserted = _insert_non_full(root, key, value);
    if (inserted) tree->count++;
    return true;
}

/* ── Suppression (delete) ───────────────────────────────────────── */

/* Trouve le successeur en-ordre (plus petite cle dans sous-arbre droit) */
static void _get_inorder_successor(lum_btree_node_t* n, int idx,
                                    uint64_t* succ_key, uint64_t* succ_val) {
    lum_btree_node_t* cur = n->children[idx + 1];
    while (!cur->is_leaf) cur = cur->children[0];
    *succ_key = cur->keys[0];
    *succ_val = cur->vals[0];
}

/* Trouve le predecesseur en-ordre (plus grande cle dans sous-arbre gauche) */
static void _get_inorder_pred(lum_btree_node_t* n, int idx,
                               uint64_t* pred_key, uint64_t* pred_val) {
    lum_btree_node_t* cur = n->children[idx];
    while (!cur->is_leaf) cur = cur->children[cur->n_keys];
    *pred_key = cur->keys[cur->n_keys - 1];
    *pred_val = cur->vals[cur->n_keys - 1];
}

/* Fusion des fils child_idx et child_idx+1 avec la cle mediane du parent */
static void _merge_children(lum_btree_node_t* parent, int idx) {
    lum_btree_node_t* left  = parent->children[idx];
    lum_btree_node_t* right = parent->children[idx + 1];

    /* Copier la cle mediane du parent dans left */
    left->keys[left->n_keys] = parent->keys[idx];
    left->vals[left->n_keys] = parent->vals[idx];
    left->n_keys++;

    /* Copier toutes les cles de right dans left */
    for (int i = 0; i < right->n_keys; i++) {
        left->keys[left->n_keys + i] = right->keys[i];
        left->vals[left->n_keys + i] = right->vals[i];
    }
    if (!left->is_leaf) {
        for (int i = 0; i <= right->n_keys; i++)
            left->children[left->n_keys + i] = right->children[i];
    }
    left->n_keys += right->n_keys;

    /* Supprimer la cle mediane du parent et decaler */
    for (int i = idx; i < parent->n_keys - 1; i++) {
        parent->keys[i] = parent->keys[i + 1];
        parent->vals[i] = parent->vals[i + 1];
    }
    for (int i = idx + 1; i < parent->n_keys; i++)
        parent->children[i] = parent->children[i + 1];
    parent->n_keys--;

    free(right);
}

/* Garantit que children[idx] a au moins M cles avant d'y descendre */
static void _fill_child(lum_btree_node_t* parent, int idx) {
    lum_btree_node_t* child = parent->children[idx];
    if (child->n_keys >= LUM_BTREE_ORDER) return; /* Assez de cles */

    /* Emprunter du frere gauche si possible */
    if (idx > 0 && parent->children[idx - 1]->n_keys >= LUM_BTREE_ORDER) {
        lum_btree_node_t* left_sib = parent->children[idx - 1];
        /* Decaler child a droite */
        for (int i = child->n_keys - 1; i >= 0; i--) {
            child->keys[i + 1] = child->keys[i];
            child->vals[i + 1] = child->vals[i];
        }
        if (!child->is_leaf) {
            for (int i = child->n_keys; i >= 0; i--)
                child->children[i + 1] = child->children[i];
        }
        /* Prendre la cle du parent */
        child->keys[0] = parent->keys[idx - 1];
        child->vals[0] = parent->vals[idx - 1];
        if (!child->is_leaf)
            child->children[0] = left_sib->children[left_sib->n_keys];
        child->n_keys++;
        /* Remplacer la cle du parent par la plus grande du frere gauche */
        parent->keys[idx - 1] = left_sib->keys[left_sib->n_keys - 1];
        parent->vals[idx - 1] = left_sib->vals[left_sib->n_keys - 1];
        left_sib->n_keys--;
        return;
    }

    /* Emprunter du frere droit si possible */
    if (idx < parent->n_keys && parent->children[idx + 1]->n_keys >= LUM_BTREE_ORDER) {
        lum_btree_node_t* right_sib = parent->children[idx + 1];
        /* Prendre la cle du parent */
        child->keys[child->n_keys] = parent->keys[idx];
        child->vals[child->n_keys] = parent->vals[idx];
        if (!child->is_leaf)
            child->children[child->n_keys + 1] = right_sib->children[0];
        child->n_keys++;
        /* Remplacer la cle du parent par la plus petite du frere droit */
        parent->keys[idx] = right_sib->keys[0];
        parent->vals[idx] = right_sib->vals[0];
        /* Decaler right_sib */
        for (int i = 0; i < right_sib->n_keys - 1; i++) {
            right_sib->keys[i] = right_sib->keys[i + 1];
            right_sib->vals[i] = right_sib->vals[i + 1];
        }
        if (!right_sib->is_leaf) {
            for (int i = 0; i < right_sib->n_keys; i++)
                right_sib->children[i] = right_sib->children[i + 1];
        }
        right_sib->n_keys--;
        return;
    }

    /* Fusion : fusionner child avec un frere */
    if (idx < parent->n_keys) {
        _merge_children(parent, idx);
    } else {
        _merge_children(parent, idx - 1);
    }
}

/* Suppression recursive */
static bool _delete_key(lum_btree_node_t* n, uint64_t key) {
    int idx = _node_lower_bound(n, key);

    if (idx < n->n_keys && n->keys[idx] == key) {
        /* Cle trouvee dans ce noeud */
        if (n->is_leaf) {
            /* Cas 1 : noeud feuille — supprimer directement */
            for (int i = idx; i < n->n_keys - 1; i++) {
                n->keys[i] = n->keys[i + 1];
                n->vals[i] = n->vals[i + 1];
            }
            n->n_keys--;
            return true;
        } else {
            /* Cas 2a : fils gauche assez grand → remplacer par predecesseur */
            if (n->children[idx]->n_keys >= LUM_BTREE_ORDER) {
                uint64_t pk, pv;
                _get_inorder_pred(n, idx, &pk, &pv);
                n->keys[idx] = pk;
                n->vals[idx] = pv;
                return _delete_key(n->children[idx], pk);
            }
            /* Cas 2b : fils droit assez grand → remplacer par successeur */
            else if (n->children[idx + 1]->n_keys >= LUM_BTREE_ORDER) {
                uint64_t sk, sv;
                _get_inorder_successor(n, idx, &sk, &sv);
                n->keys[idx] = sk;
                n->vals[idx] = sv;
                return _delete_key(n->children[idx + 1], sk);
            }
            /* Cas 2c : les deux fils ont M-1 cles → fusionner */
            else {
                _merge_children(n, idx);
                return _delete_key(n->children[idx], key);
            }
        }
    } else {
        /* Cle pas dans ce noeud */
        if (n->is_leaf) return false; /* Absent */
        /* Garantir que le fils a assez de cles */
        _fill_child(n, idx);
        /* Relancer sur le bon fils */
        int new_idx = (idx > n->n_keys) ? n->n_keys : idx;
        return _delete_key(n->children[new_idx], key);
    }
}

bool lum_btree_delete(lum_btree_t* tree, uint64_t key) {
    if (!tree || !tree->root) return false;
    bool deleted = _delete_key(tree->root, key);
    if (deleted) {
        tree->count--;
        /* Si la racine est vide apres suppression, descendre */
        if (tree->root->n_keys == 0 && !tree->root->is_leaf) {
            lum_btree_node_t* old_root = tree->root;
            tree->root = old_root->children[0];
            free(old_root);
            tree->height--;
        }
    }
    return deleted;
}

/* ── Validation ─────────────────────────────────────────────────── */
static bool _validate_node(const lum_btree_node_t* n, int depth,
                             uint64_t lo, uint64_t hi,
                             int* leaf_depth, int cur_depth) {
    if (!n) return false;
    /* Verifier le nombre de cles */
    if (n != NULL && n->n_keys > LUM_BTREE_MAX_KEYS) return false;
    if (n->n_keys < 0) return false;
    /* Verifier tri */
    for (int i = 0; i < n->n_keys - 1; i++) {
        if (n->keys[i] >= n->keys[i + 1]) return false;
    }
    /* Verifier bornes */
    if (n->n_keys > 0) {
        if (lo != 0 && n->keys[0] <= lo) return false;
        if (hi != UINT64_MAX && n->keys[n->n_keys - 1] >= hi) return false;
    }
    if (n->is_leaf) {
        if (*leaf_depth == -1) *leaf_depth = cur_depth;
        else if (*leaf_depth != cur_depth) return false;
        return true;
    }
    /* Noeud interne */
    for (int i = 0; i <= n->n_keys; i++) {
        if (!n->children[i]) return false;
        uint64_t child_lo = (i == 0) ? 0 : n->keys[i - 1];
        uint64_t child_hi = (i == n->n_keys) ? UINT64_MAX : n->keys[i];
        if (!_validate_node(n->children[i], depth + 1,
                             child_lo, child_hi, leaf_depth, cur_depth + 1))
            return false;
    }
    return true;
}

bool lum_btree_validate(const lum_btree_t* tree) {
    if (!tree || !tree->root) return false;
    int leaf_depth = -1;
    return _validate_node(tree->root, 0, 0, UINT64_MAX, &leaf_depth, 0);
}

/* ── Dump textuel ───────────────────────────────────────────────── */
static void _dump_node(const lum_btree_node_t* n, int level, int* count, int max) {
    if (!n || *count >= max) return;
    for (int i = 0; i < n->n_keys && *count < max; i++) {
        if (!n->is_leaf && n->children[i])
            _dump_node(n->children[i], level + 1, count, max);
        printf("  [L%d] key=%llu val=%llu\n",
               level, (unsigned long long)n->keys[i],
               (unsigned long long)n->vals[i]);
        (*count)++;
    }
    if (!n->is_leaf && n->children[n->n_keys])
        _dump_node(n->children[n->n_keys], level + 1, count, max);
}

void lum_btree_dump(const lum_btree_t* tree, int max_keys) {
    if (!tree) { printf("[B-Tree] NULL\n"); return; }
    printf("[B-Tree] count=%llu height=%d\n",
           (unsigned long long)tree->count, tree->height);
    int c = 0;
    _dump_node(tree->root, 0, &c, max_keys > 0 ? max_keys : 20);
}
