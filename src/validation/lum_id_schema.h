/* **************************************************************************
** lum_id_schema.h — Schéma LUM_ID 64 bits v3 partagé (UNICITE-003)
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/validation / schéma d'identifiants forensic
** Auteur : LumVorax Project
**
** Historique des versions :
**   v1 (UNIF-001/002/003) : (run_id_xor, protocol, module, step32, bit_pos6)
**      → PB : pas de cell_idx → 3840 collisions sur grille 4×4/10steps
**      → PB : run_id = XOR timestamp → non injectif
**      → PB : HASH_EMPTY = UINT64_MAX ∈ espace LUM_ID possible
**
**   v2 (UNIF-004 S162) : (run_id_xor, protocol, module, step16, cell_idx16, bit_pos8)
**      → FIX : cell_idx distingue les cellules → 0 collision S162
**      → PB restant : run_id XOR non injectif (PC3 rapport 163)
**      → PB restant : HASH_EMPTY = UINT64_MAX toujours risqué (PC4 rapport 163)
**      → PB documenté : step limité à 16 bits = max 65535 (PC2 rapport 163)
**
**   v3 (UNICITE-002 S163) : corrections de tous les points critiques rapport 163
**      → FIX PC2 (partiel S163) : step 16 bits documenté + assertion à la compilation
**      → FIX PC3 (partiel S163) : run_seq = compteur séquentiel (injective dans la session)
**      → FIX PC4 : HASH_EMPTY = 0x0 — impossible car run_counter commence à 1
**                  → bit[47:32] = run_counter_low ≥ 1 → LUM_ID ≥ 0x0001_0000_0000_0000
**      → MAINTENU : cell_idx 16 bits, bit_pos 8 bits
**
**   v3.1 (UNICITE-003 S164) : corrections PC2 + PC3 identifiées audit 165
**      → FIX PC2 : renommage LUM_ID_V3_MAX_STEPS → LUM_ID_V3_MAX_STEP_VALUE
**                  (valeur max autorisée, pas un nombre de steps)
**                  + ajout LUM_ID_V3_MAX_STEP_COUNT = 65536U (nombre de valeurs
**                  disponibles dans le champ 16 bits, y compris 0)
**      → FIX PC3 : wrap-around détecté → FAIL-HARD (abort) au lieu de réutiliser
**                  silencieusement run_seq=1. Réutilisation = collision garantie.
**                  Portée locale documentée honnêtement (une copie par .c).
**
** Schéma v3 — Layout 64 bits :
**   [63:48] run_seq      (16 bits) — compteur de session (1..65535, jamais 0)
**   [47:44] protocol     (4 bits)
**   [43:40] module       (4 bits)
**   [39:24] step         (16 bits) — valeur max = LUM_ID_V3_MAX_STEP_VALUE = 65535
**   [23:8]  cell_idx     (16 bits) — max 65535 cellules
**   [7:0]   bit_pos      (8 bits)  — position bit dans double (0..63)
**
** Garantie PC4 :
**   run_seq ∈ [1..65535] → LUM_ID ≥ 0x0001_0000_0000_0000 > 0
**   → HASH_EMPTY = 0x0000_0000_0000_0000 n'appartient JAMAIS à l'espace des LUM_ID
**
** Garantie PC3 (S164 UNICITE-003) :
**   run_seq = compteur séquentiel incrémenté à chaque appel de lum_id_v3_new_run_seq().
**   Injective dans la session courante tant que le nombre d'appels ≤ 65535.
**   PORTÉE : variable statique locale à l'unité de compilation — chaque fichier .c
**            qui inclut ce header a sa propre copie indépendante (OK pour UNIF-002/003/004
**            car les runs sont indépendants).
**   WRAP-AROUND : FAIL-HARD (abort()) — pas de réutilisation silencieuse d'un
**                 run_seq déjà utilisé (collision garantie → interdit).
**
** Garantie PC2 (S164 UNICITE-003) :
**   LUM_ID_V3_MAX_STEP_VALUE = 65535 — valeur maximale autorisée dans le champ step.
**   LUM_ID_V3_MAX_STEP_COUNT = 65536 — nombre de valeurs disponibles (0..65535).
**   Appelant DOIT vérifier step <= LUM_ID_V3_MAX_STEP_VALUE avant d'appeler.
**   UNIF-003 a oublié cette garde en S163 → corrigée en S164.
**
** Garantie contre PC1 (vocabulaire) :
**   Les commentaires distinguent explicitement :
**     - total_events = nombre total d'insertions
**     - distinct_ids  = nombre de LUM_ID distincts dans le hash set
**     - duplicate_ids = nombre de LUM_ID ayant count > 1
**     - extra_events  = total_events - distinct_ids (occurrences supplémentaires)
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

#ifndef LUM_ID_SCHEMA_H
#define LUM_ID_SCHEMA_H

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

/* ── Constantes du schéma v3.1 (UNICITE-003 S164) ────────────────────────── */

/*
 * PC2 FIX S164 : renommage LUM_ID_V3_MAX_STEPS → LUM_ID_V3_MAX_STEP_VALUE
 *   LUM_ID_V3_MAX_STEP_VALUE = valeur maximale autorisée dans le champ step (16 bits).
 *   LUM_ID_V3_MAX_STEP_COUNT = nombre total de valeurs disponibles (0..65535 inclus).
 *   Distinction sémantique :
 *     "step <= LUM_ID_V3_MAX_STEP_VALUE"  → valeur valide
 *     "nb_steps < LUM_ID_V3_MAX_STEP_COUNT" → nombre de pas possible
 * Ancienne macro LUM_ID_V3_MAX_STEPS conservée comme alias déprécié pour
 * compatibilité ascendante avec tout code existant.
 */
#define LUM_ID_V3_MAX_STEP_VALUE 65535U   /* valeur max du champ step (16 bits) */
#define LUM_ID_V3_MAX_STEP_COUNT 65536U   /* nb valeurs possibles dans [0..65535] */
#define LUM_ID_V3_MAX_STEPS      LUM_ID_V3_MAX_STEP_VALUE  /* DÉPRÉCIÉ — alias */

#define LUM_ID_V3_MAX_CELLS      65535U   /* cell_idx ∈ [0..65535] */
#define LUM_ID_V3_MAX_BIT_POS    63U      /* bit_pos ∈ [0..63] pour double IEEE 754 */
#define LUM_ID_V3_MIN_RUN_SEQ    1U       /* run_seq commence à 1, jamais 0 */
#define LUM_ID_V3_MAX_RUN_SEQ    65535U   /* 16 bits */

/* ── Sentinelle hash set (PC4 FIX) ───────────────────────────────────────────
 * run_seq ∈ [1..65535] → bits [63:48] ∈ [0x0001..0xFFFF]
 * → LUM_ID_V3 ≥ 0x0001_0000_0000_0000 pour tout LUM_ID valide
 * → 0x0000_0000_0000_0000 ne peut jamais être un LUM_ID valide
 */
#define LUM_ID_V3_HASH_EMPTY     UINT64_C(0x0000000000000000)

/* ── Compteur de run session (PC3 FIX UNICITE-003 S164) ─────────────────────
 *
 * Compteur séquentiel incrémenté par lum_id_v3_new_run_seq().
 * Chaque appel retourne une valeur unique dans [1..65535] dans la session.
 *
 * PORTÉE HONNÊTE (audit 165, PC3) :
 *   La variable est STATIQUE — une copie par unité de compilation (.c).
 *   UNIF-002, UNIF-003 et UNIF-004 ont chacun leur propre compteur local.
 *   C'est intentionnel : les runs sont indépendants entre les trois programmes.
 *   Ne PAS supposer que cette valeur est partagée entre fichiers sans extern.
 *
 * WRAP-AROUND FAIL-HARD (PC3 FIX S164) :
 *   Si 65535 appels ont déjà été faits dans la même session, un 65536e appel
 *   signifie que run_seq=1 serait réutilisé → collision garantie → INTERDIT.
 *   Comportement : fprintf(stderr, ...) + abort().
 *   Rationale : un WARNING silencieux (ancienne version S163) permettait de
 *               continuer avec des collisions sans le savoir.
 */
static uint16_t g_lum_run_seq_counter = 0;  /* 0 = non initialisé */

static inline uint16_t lum_id_v3_new_run_seq(void)
{
    if (g_lum_run_seq_counter == LUM_ID_V3_MAX_RUN_SEQ) {
        /* PC3 FIX S164 : FAIL-HARD — pas de wrap silencieux */
        fprintf(stderr,
            "[LUM_ID_V3][FATAL] run_seq wrap-around atteint (65535 appels).\n"
            "  Réutiliser run_seq=1 produirait des collisions LUM_ID garanties.\n"
            "  Correction requise : relancer dans un nouveau processus ou\n"
            "  limiter le nombre d'appels à lum_id_v3_new_run_seq() à 65535.\n"
            "  CERTIFIED_100=false — abort().\n");
        abort();  /* FAIL-HARD : pas de continuation possible */
    }
    g_lum_run_seq_counter++;
    /* Invariant post-incrémentation : counter ∈ [1..65535] */
    return g_lum_run_seq_counter;
}

/* ── Encodage LUM_ID 64 bits v3 ───────────────────────────────────────────────
 *
 * Paramètres :
 *   run_seq  : uint16_t, valeur retournée par lum_id_v3_new_run_seq(), ≥ 1
 *   protocol : int, ∈ [0..15] (4 bits)
 *   module   : int, ∈ [0..15] (4 bits)
 *   step     : uint16_t, ∈ [0..LUM_ID_V3_MAX_STEP_VALUE]
 *   cell_idx : uint16_t, ∈ [0..LUM_ID_V3_MAX_CELLS]
 *   bit_pos  : int, ∈ [0..LUM_ID_V3_MAX_BIT_POS]
 *
 * Retourne : LUM_ID 64 bits unique pour le tuple (run_seq, protocol, module,
 *            step, cell_idx, bit_pos) dans les limites ci-dessus.
 *
 * Garantie : run_seq ≥ 1 → LUM_ID ≥ 0x0001_0000_0000_0000 > LUM_ID_V3_HASH_EMPTY
 */
static inline uint64_t lum_id_v3_encode(uint16_t run_seq, int protocol,
                                         int module, uint16_t step,
                                         uint16_t cell_idx, int bit_pos)
{
    /* Assertion runtime DEBUG — vérifie les préconditions */
    if (run_seq < LUM_ID_V3_MIN_RUN_SEQ) {
        fprintf(stderr,
            "[LUM_ID_V3][DEBUG] ERREUR : run_seq=0 invalide — utiliser"
            " lum_id_v3_new_run_seq()\n");
        /* On continue avec run_seq forcé à 1 pour ne pas émettre HASH_EMPTY */
        run_seq = LUM_ID_V3_MIN_RUN_SEQ;
    }
    return ((uint64_t)(run_seq  & 0xFFFFU) << 48)
         | ((uint64_t)(protocol & 0xFU)    << 44)
         | ((uint64_t)(module   & 0xFU)    << 40)
         | ((uint64_t)(step     & 0xFFFFU) << 24)
         | ((uint64_t)(cell_idx & 0xFFFFU) << 8)
         | ((uint64_t)(bit_pos  & 0xFFU));
}

/* ── Hash set LUM_ID v3 (sondage linéaire, sentinelle = 0x0) ──────────────────
 *
 * Structure réutilisable — UNICITE-002 FIX PC4 :
 *   Sentinelle = LUM_ID_V3_HASH_EMPTY = 0x0 (impossible pour un vrai LUM_ID v3).
 *   count == 0 → case vide
 *   count == 1 → unique
 *   count  > 1 → doublon
 *
 * Métriques exposées (PC1 FIX — vocabulaire précis) :
 *   total_insertions : nombre total d'insertions (= total_events attendus)
 *   distinct_ids     : nombre de LUM_ID distincts
 *   duplicate_ids    : nombre de LUM_ID ayant count > 1 (≠ total collisions)
 *   extra_events     : total_insertions - distinct_ids (occurrences supplémentaires)
 */

typedef struct {
    uint64_t *keys;
    uint32_t *counts;
    uint64_t  capacity;         /* puissance de 2 */
    uint64_t  distinct_ids;     /* nb LUM_ID distincts (PC1 : anciennement "size") */
    uint64_t  duplicate_ids;    /* nb LUM_ID avec count > 1 (PC1 : anciennement "duplicates") */
    uint64_t  total_insertions; /* nb total d'insertions */
} LumIDHashSetV3;

static inline LumIDHashSetV3 *lum_hashset_v3_create(uint64_t expected_count)
{
    LumIDHashSetV3 *hs = (LumIDHashSetV3 *)malloc(sizeof(LumIDHashSetV3));
    if (!hs) return NULL;

    uint64_t cap = 1;
    while (cap < 2 * expected_count) cap <<= 1;

    hs->keys   = (uint64_t *)malloc(cap * sizeof(uint64_t));
    hs->counts = (uint32_t *)malloc(cap * sizeof(uint32_t));
    if (!hs->keys || !hs->counts) {
        free(hs->keys);
        free(hs->counts);
        free(hs);
        return NULL;
    }

    /* PC4 FIX : initialiser toutes les cases avec LUM_ID_V3_HASH_EMPTY = 0x0 */
    for (uint64_t i = 0; i < cap; i++) {
        hs->keys[i]   = LUM_ID_V3_HASH_EMPTY;
        hs->counts[i] = 0;
    }
    hs->capacity         = cap;
    hs->distinct_ids     = 0;
    hs->duplicate_ids    = 0;
    hs->total_insertions = 0;
    return hs;
}

static inline void lum_hashset_v3_insert(LumIDHashSetV3 *hs, uint64_t lum_id)
{
    if (!hs) return;
    /* PC4 FIX : refuser LUM_ID_V3_HASH_EMPTY comme clé */
    if (lum_id == LUM_ID_V3_HASH_EMPTY) {
        fprintf(stderr,
            "[LUM_ID_V3][DEBUG] ERREUR : tentative d'insertion de HASH_EMPTY"
            " (lum_id=0) — LUM_ID invalide, ignoré.\n");
        return;
    }

    hs->total_insertions++;

    uint64_t mask = hs->capacity - 1;
    uint64_t idx  = (lum_id ^ (lum_id >> 32)) & mask;

    for (uint64_t probe = 0; probe < hs->capacity; probe++) {
        uint64_t i = (idx + probe) & mask;
        if (hs->keys[i] == LUM_ID_V3_HASH_EMPTY) {
            /* Première insertion */
            hs->keys[i]   = lum_id;
            hs->counts[i] = 1;
            hs->distinct_ids++;
            return;
        }
        if (hs->keys[i] == lum_id) {
            /* Doublon */
            hs->counts[i]++;
            if (hs->counts[i] == 2) hs->duplicate_ids++;
            return;
        }
    }
    fprintf(stderr,
        "[LUM_ID_V3][DEBUG] OVERFLOW hash set — lum_id=0x%016" PRIx64 " non inséré\n",
        lum_id);
}

static inline void lum_hashset_v3_print_summary(const LumIDHashSetV3 *hs)
{
    if (!hs) return;
    uint64_t extra = (hs->total_insertions >= hs->distinct_ids)
                     ? hs->total_insertions - hs->distinct_ids : 0;
    printf("  total_insertions  : %" PRIu64 "\n", hs->total_insertions);
    printf("  distinct_ids      : %" PRIu64 "\n", hs->distinct_ids);
    printf("  duplicate_ids     : %" PRIu64 " %s\n",
           hs->duplicate_ids,
           (hs->duplicate_ids == 0) ? "✓ AUCUN" : "⚠ DOUBLONS!");
    printf("  extra_events      : %" PRIu64 " %s\n",
           extra, (extra == 0) ? "✓ 0" : "⚠");
}

static inline int lum_hashset_v3_is_unique(const LumIDHashSetV3 *hs,
                                            uint64_t expected_count)
{
    if (!hs) return 0;
    return (hs->distinct_ids     == expected_count)
        && (hs->duplicate_ids    == 0)
        && (hs->total_insertions == expected_count);
}

static inline void lum_hashset_v3_destroy(LumIDHashSetV3 *hs)
{
    if (!hs) return;
    free(hs->keys);
    free(hs->counts);
    free(hs);
}

#endif /* LUM_ID_SCHEMA_H */
