# Rapport 166 — UNICITE-003 : Corrections PC2 + PC3 + Migration UNIF-004 v3

**Date :** 2026-10-03  
**Session :** S164  
**HEAD avant :** `5fdf8d2` (rapport 165 audit critique S163)  
**CERTIFIED_100=false**  
**unique_human_proven=false**  
**Mode DEBUG actif**

---

## 1. Contexte — Points critiques de l'audit 165

L'audit 165 avait identifié 4 problèmes sur le travail de S163 (UNICITE-002) :

| Code | Fichier | Problème identifié |
|------|---------|-------------------|
| PC2 | `lum_id_schema.h` | `LUM_ID_V3_MAX_STEPS` : nom ambigu (valeur max vs nombre de steps) |
| PC2 | `ns_forensic_unif3.c` | Garde `step >= MAX_STEPS` manquante dans la boucle (oubli S163) |
| PC3 | `lum_id_schema.h` | Wrap-around run_seq → réutilisation silencieuse = collision garantie |
| UNIF-004 | `ns_forensic_unif4.c` | Header inclus mais fonctions locales v2 encore actives (migration sans effet) |

---

## 2. Corrections appliquées

### 2.1 `src/validation/lum_id_schema.h` — PC2 + PC3

#### PC2 — Renommage et clarification sémantique

**AVANT (ligne 76) :**
```c
#define LUM_ID_V3_MAX_STEPS      65535U   /* step ∈ [0..65535] */
```

**APRÈS :**
```c
#define LUM_ID_V3_MAX_STEP_VALUE 65535U   /* valeur max du champ step (16 bits) */
#define LUM_ID_V3_MAX_STEP_COUNT 65536U   /* nb valeurs possibles dans [0..65535] */
#define LUM_ID_V3_MAX_STEPS      LUM_ID_V3_MAX_STEP_VALUE  /* DÉPRÉCIÉ — alias */
```

**Distinction sémantique ajoutée :**
- `step <= LUM_ID_V3_MAX_STEP_VALUE` → vérification de valeur
- `nb_steps < LUM_ID_V3_MAX_STEP_COUNT` → vérification de nombre

L'ancien nom `LUM_ID_V3_MAX_STEPS` est conservé comme alias déprécié pour compatibilité ascendante.

---

#### PC3 — Wrap-around : WARNING silencieux → FAIL-HARD

**AVANT (lignes 101–113) :**
```c
if (g_lum_run_seq_counter == LUM_ID_V3_MAX_RUN_SEQ) {
    /* Wrap-around — signal DEBUG (ne devrait pas arriver en pratique) */
    fprintf(stderr, "[LUM_ID_V3][DEBUG] AVERTISSEMENT : run_seq wrap-around...\n");
    g_lum_run_seq_counter = LUM_ID_V3_MIN_RUN_SEQ;  /* RÉUTILISATION = COLLISION */
} else {
    g_lum_run_seq_counter++;
    if (g_lum_run_seq_counter < LUM_ID_V3_MIN_RUN_SEQ)
        g_lum_run_seq_counter = LUM_ID_V3_MIN_RUN_SEQ;
}
```

**APRÈS :**
```c
if (g_lum_run_seq_counter == LUM_ID_V3_MAX_RUN_SEQ) {
    /* PC3 FIX S164 : FAIL-HARD — pas de wrap silencieux */
    fprintf(stderr,
        "[LUM_ID_V3][FATAL] run_seq wrap-around atteint (65535 appels).\n"
        "  Réutiliser run_seq=1 produirait des collisions LUM_ID garanties.\n"
        "  ...\n"
        "  CERTIFIED_100=false — abort().\n");
    abort();  /* FAIL-HARD : pas de continuation possible */
}
g_lum_run_seq_counter++;
/* Invariant post-incrémentation : counter ∈ [1..65535] */
return g_lum_run_seq_counter;
```

**Portée documentée honnêtement :** variable `static` = une copie par unité de compilation. UNIF-002/003/004 ont chacun leur propre compteur local (intentionnel — runs indépendants).

---

### 2.2 `src/validation/ns_forensic_unif3.c` — PC2 garde manquante

**AVANT (ligne 375) :** boucle sans garde
```c
for (int step = 0; step < UNIF3_STEPS; step++) {
    printf("[TRACE] Pas %d/%d ...\n", step + 1, UNIF3_STEPS);
    trace_ns_step_unif3(s, (uint32_t)step, &ctx);
}
```

**APRÈS :**
```c
for (int step = 0; step < UNIF3_STEPS; step++) {
    /* PC2 FIX S164 : garde manquante identifiée audit 165 */
    if ((uint32_t)step > (uint32_t)LUM_ID_V3_MAX_STEP_VALUE) {
        fprintf(stderr,
            "[UNIF3][FATAL] step=%d dépasse LUM_ID_V3_MAX_STEP_VALUE=%u"
            " — encodage LUM_ID impossible. Arrêt.\n",
            step, LUM_ID_V3_MAX_STEP_VALUE);
        ns_solver_destroy(s);
        lum_hashset_v3_destroy(ctx.hs);
        forensic_logger_destroy();
        return 1;
    }
    printf("[TRACE] Pas %d/%d ...\n", step + 1, UNIF3_STEPS);
    trace_ns_step_unif3(s, (uint32_t)step, &ctx);
}
```

Note : `UNIF3_STEPS=10` → la garde ne se déclenche jamais en pratique. Elle est néanmoins obligatoire pour respecter le contrat de `lum_id_v3_encode()`.

---

### 2.3 `src/validation/ns_forensic_unif4.c` — Migration complète v2 → v3

C'est la correction principale. L'audit 165 avait constaté que l'inclusion de `lum_id_schema.h` en S163 était **sans effet opérationnel** : les fonctions locales v2 masquaient les fonctions v3 du header.

#### Suppressions (schéma v2 local)

| Élément supprimé | Problème |
|-----------------|----------|
| `encode_lum_id_64()` (local) | Duplique `lum_id_v3_encode()` du header |
| `typedef LumIDHashSet` | Remplacé par `LumIDHashSetV3` |
| `#define HASH_EMPTY UINT64_MAX` | UINT64_MAX est un LUM_ID v2 possible → sentinelle risquée |
| `hashset_create/insert/destroy()` | Remplacés par fonctions v3 du header |
| `make_run_id()` XOR | Non-injective : deux timestamps → même run_id possible |

#### Remplacements (schéma v3 partagé)

| Remplacement | Source |
|-------------|--------|
| `lum_id_v3_encode()` | `lum_id_schema.h` |
| `LumIDHashSetV3` | `lum_id_schema.h` |
| `lum_hashset_v3_create/insert/destroy()` | `lum_id_schema.h` |
| `lum_hashset_v3_print_summary/is_unique()` | `lum_id_schema.h` |
| `lum_id_v3_new_run_seq()` | `lum_id_schema.h` |
| `ctx.run_id` → `ctx.run_seq` | Vocabulaire cohérent v3 |

Garde `step <= LUM_ID_V3_MAX_STEP_VALUE` ajoutée dans la boucle main() (même contrat que UNIF3).

---

## 3. Résultats d'exécution

Les 3 binaires ont été recompilés et exécutés sur `main` (macOS, gcc, grille 4×4, 10 steps, 7 modules, 64 bits/double).

| Binaire | Verdict | Events | distinct_ids | duplicate_ids | Remarques |
|---------|---------|--------|-------------|--------------|-----------|
| `ns_forensic_unif2` | **PASS** ✅ | 49280 | 49280 | 0 | Non modifié en S164 — non-régression |
| `ns_forensic_unif3` | **PASS** ✅ | 49280 | 49280 | 0 | Garde step ajoutée, 0 gap event_seq |
| `ns_forensic_unif4` | **PASS** ✅ | 49280 | 49280 | 0 | Migration v3 complète, 0 régression monotonie |

### UNIF4 — Résultats complets

```
=== UNICITE-003 — HASH SET LUM_ID v3 ===

  total_insertions  : 49280
  distinct_ids      : 49280
  duplicate_ids     : 0 ✓ AUCUN
  extra_events      : 0 ✓ 0
  Unicité exacte    : PASS — tous les LUM_ID sont distincts (v3, 0 doublon)

=== VÉRIFICATION LOG (event_seq + monotonie timestamps) ===

  Événements lus        : 49280
  Gaps séquence (pertes): 0 ✓
  Régressions timestamp : 0 ✓ MONOTONE

  BUG-3 FINAL CLOSE : timestamps = CLOCK_MONOTONIC : PROUVÉ

[VERDICT] FORENSIC-UNIF-004 : PASS — schéma v3, timestamps MONOTONIC, unicité exacte, 0 gap, 0 régression
```

---

## 4. État du registre de continuité

| Chantier | État |
|----------|------|
| UNIF-002 / UNIF-003 | ✅ PASS (schéma v3, 0 doublon, S163 + non-régression S164) |
| UNIF-004 | ✅ PASS (migration v3 effective S164) |
| PC1 vocabulaire | ✅ CLOSE (S163) |
| PC2 sémantique MAX_STEPS | ✅ CLOSE (S164) |
| PC2 garde step UNIF3 | ✅ CLOSE (S164) |
| PC2 garde step UNIF4 | ✅ CLOSE (S164) |
| PC3 wrap fail-hard | ✅ CLOSE (S164) |
| PC3 portée documentée | ✅ CLOSE (S164) |
| PC4 sentinelle HASH_EMPTY | ✅ CLOSE (S163) |
| UNICITE-003 globalement | ✅ **CLOSE** |
| Richardson-PROTOCOL-001 | **OPEN** — distinguer erreur spatiale/temporelle |
| T04 renforcé | **OPEN** — quasi-stationnarité sur 100 derniers pas |
| Lyapunov robustesse quantitative | **OPEN** — balayage epsilon 5 valeurs |
| BUILD-THREAD-001 | **OPEN** |
| BUILD-PROOF-001 | **OPEN** |
| BUILD-PORT-002 | **OPEN** |
| BL-003→BL-012 | **OPEN** |
| C3 NS→NX-42 | **OPEN** |
| C4 NX-42 problèmes 6-30 | **OPEN** |
| FORENSIC-UNIF-001 (branchement NS/Rich/Lyap) | **OPEN** |
| CERTIFIED_100 | **false** |

---

## 5. Priorité logique suivante

Selon l'ordre établi après session 151 et confirmé dans l'état de reprise :

**Richardson-PROTOCOL-001** — Distinguer expérimentalement erreur spatiale et erreur temporelle. Le protocole doit comparer explicitement :
- Protocole A : `dt constant`
- Protocole B : `dt ∝ dx`
- Protocole C : `dt ∝ dx²` (contrainte CFL diffusive)

Ensuite : **T04 renforcé** → **Lyapunov robustesse** → **BUILD-THREAD-001** → ...

---

## 6. CERTIFIED_100=false

La session S164 ferme tous les points identifiés par l'audit 165 (PC2/PC3/UNIF-004). Les chantiers OPEN listés ci-dessus restent actifs.
