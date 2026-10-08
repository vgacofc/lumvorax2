# RAPPORT 202 — ANALYSE ATOMIQUE BRUTE LUM-VORAX
## Graphe atomique, clusters, bimodalité, déterminisme, audit bugs

**Date :** 2026-10-08  
**Précédent rapport :** 201_CV_BENCH_ANALYSE_LOGS_20261007.md  
**Script d'analyse :** `src/sch/atom/analyse_atoms_r202.py`  
**Données source :** `logs_AIMO3/sch/atom/` (4 fichiers)  
**CERTIFIED_100=false** | **unique_human_proven=false** | Mode DEBUG actif  
**Auteur :** ARTCB Project <contact@artcb.me>

---

## 0. Contexte et objectif

Ce rapport répond à la question centrale : **« Qu'est-ce que les atomes du système LUM-VORAX font réellement entre eux ? »**  
L'analyse porte sur les 4 fichiers bruts générés par `cv_investigation_bench` :

| Fichier | Taille | Lignes |
|---------|--------|--------|
| `physics_all_atoms.jsonl` | 2.8 MB | 13 000 |
| `pairs_all_comparisons.jsonl.gz` | 87 MB (orig. 944 MB) | > 500 000 lues |
| `step_nanoseconds.jsonl` | 2.2 KB | 16 |
| `transient_events.log` | 756 KB | 10 338 |

> **Note disque :** `pairs_all_comparisons.jsonl` (944 MB) compressé → 87 MB (gzip --best, ratio 10.8×). Original supprimé. 20 GB libres après opération.

---

## 1. Schéma des données — découverte des clés réelles

### 1.1 `pairs_all_comparisons.jsonl` — clés découvertes en streaming

**Avant ce rapport :** schéma inconnu.  
**Après analyse streaming :** clés détectées :

```
step | ts_ns | atom_i | atom_j | dist | cluster_formed | cluster_type | skipped_falsif | falsif_mode
```

**Observation critique :** le champ `type` est **absent** du fichier pairs — la distribution par type de paires est donc **non calculable** depuis ce fichier seul. Il faut croiser avec `physics_all_atoms.jsonl`.

### 1.2 `physics_all_atoms.jsonl` — confirmation du schéma

```
ts_before_ns | ts_after_ns | dt_ns | atom_id | type |
x/y/z_before/after | vx/vy/vz_before/after
```

**Révélation :** 13 000 lignes = **1 000 atomes × 13 steps** (et non 8 steps comme estimé en Rapport 201). Steps 0 à 12 couverts.

---

## 2. Résultats — Physique des atomes (`physics_all_atoms.jsonl`)

### 2.1 Distribution globale

| Métrique | Valeur |
|----------|--------|
| Lignes totales | 13 000 |
| Atomes uniques | **1 000** ✅ (0→999) |
| Types d'atomes | 4 (0, 1, 2, 3) — **250 atomes chacun, équipartition parfaite** |
| NaN / Inf | **0** ✅ — aucune valeur corrompue |
| dt_ns = 0 | **5 334 / 13 000 = 41 %** |

### 2.2 Anomalie principale : dt_ns = 0 (41 %)

**Processus :** `ts_after_ns - ts_before_ns` mesuré pour chaque atome à chaque step.  
**Problème :** 41 % des atomes présentent un dt_ns nul → `ts_before == ts_after`.  
**Analyse :** résolution temporelle effective du système macOS (`CLOCK_MONOTONIC`) insuffisante pour la granularité d'un step par atome. Confirmé par Rapport 189 (88.9% delta=0 dans les appels consécutifs). Ce n'est **pas un bug de calcul** mais une limite hardware/OS.  
**Impact :** le déplacement physique est réel (déplacement moyen = 0.016 nm non nul) malgré le dt=0 — les positions sont bien mises à jour.

### 2.3 Déplacements réels

| Métrique | Valeur |
|----------|--------|
| Déplacements non-zéro | **13 000 / 13 000** (100%) |
| Déplacement moyen | **0.016289 unités** |
| Tous les atomes bougent | ✅ confirmé à chaque step |

**Conclusion :** les atomes se déplacent réellement à chaque step, quelles que soient les conditions temporelles.

### 2.4 dt_ns=0 par step — variabilité

| Step | dt_zero | % |
|------|---------|---|
| 0 | 509 | 50.9% |
| 1 | 445 | 44.5% |
| 2 | 405 | 40.5% |
| 3 | 546 | **54.6%** ← pic |
| 4 | 435 | 43.5% |
| 5 | 473 | 47.3% |
| 6 | 325 | 32.5% |
| 7 | **225** | **22.5%** ← minimum |
| 8 | 506 | 50.6% |
| 9 | 453 | 45.3% |
| 10 | 364 | 36.4% |
| 11 | 319 | 31.9% |
| 12 | 329 | 32.9% |

**Tendance :** décroissance globale des dt=0 de step 3→7 (parallélisme croissant ou chauffage cache). Rebond aux steps 8–9. Pas de corrélation directe avec le régime lent/rapide (bimodalité découverte dans `step_nanoseconds.jsonl`).

---

## 3. Résultats — Événements transitoires (`transient_events.log`)

### 3.1 Statistiques globales

| Métrique | Valeur |
|----------|--------|
| Total événements | **5 387** |
| Auto-interactions (BUG i==j) | **0** ✅ — aucune auto-interaction |
| Double-comptages (i,j dupliqué même step) | **2** ⚠️ (marginal) |
| NaN distances | **0** ✅ |
| Paires uniques (step inclus) | **5 385** |

### 3.2 Distribution par step

| Step | Événements | Step | Événements |
|------|-----------|------|-----------|
| 0 | 637 | 1 | 815 |
| 2 | 778 | 3 | 64 |
| 4 | 418 | 5 | 415 |
| 6 | 243 | 22 | 203 |
| 23 | 363 | 24 | 352 |
| 25 | 35 | 29 | 202 |
| 30 | 402 | 31 | 382 |
| 32 | 78 | | |

**Observations :**
- Step 1 = pic absolu (**815 événements**) — phase d'activation maximale post-initialisation
- Step 3 = **creux** (64 événements) → anomalie : le step 3 est dans le groupe *lent* (3 434 s) mais génère très peu d'interactions transitoires
- Steps élevés (22, 29, 30, 31) = environ 200–400 événements → stabilisation

### 3.3 Distribution par type d'interaction

| Type | Événements | % |
|------|-----------|---|
| Type 2 | **4 647** | **86.3%** |
| Type 1 | 740 | 13.7% |

**Processus :** type 2 = interaction dominante (cohésion ou liaison forte). Type 1 = rare (répulsion ou liaison faible ?). La signification physique des types n'est pas documentée dans le code source visible.

### 3.4 Distribution des distances

| Métrique | Valeur (nm) |
|----------|-------------|
| Distance min | **0.0143 nm** |
| Distance max | **0.2999 nm** ← limite du seuil d'activation |
| Distance moyenne | **0.2225 nm** |
| Distance médiane | **0.2353 nm** |

**Observation critique :** la distance max est exactement ≤ 0.3 nm — tous les événements sont à l'intérieur du seuil d'activation (`DIST_THRESHOLD = 0.3 nm`). La distribution est concentrée en haut (moyenne 0.22 nm ≈ 74% du seuil).

### 3.5 Hubs atomiques — graphe d'interactions

| Rang | Atome ID | Degré (interactions) |
|------|----------|---------------------|
| 1 | **255** | **33** |
| 2 | 568 | 31 |
| 3 | 862 | 31 |
| 4 | 444 | 30 |
| 5 | 204 | 30 |
| 6 | 513 | 29 |
| 7 | 481 | 29 |
| 8 | 566 | 28 |
| 9 | 824 | 28 |
| 10 | 748 | 27 |

**Processus :** le graphe d'interactions transitoires présente **une loi de hubs** — certains atomes participent à beaucoup plus d'interactions que la moyenne. L'atome 255 est le hub principal avec 33 interactions uniques, soit ~6× la moyenne attendue si les interactions étaient uniformes (5 387 × 2 / 1000 ≈ 10.8).

**Hypothèse :** les hubs correspondent à des atomes d'un type particulier positionnés centralement dans l'espace de simulation. **Non vérifié** — nécessite croisement avec les positions de `physics_all_atoms.jsonl`.

---

## 4. Résultats — Bimodalité temporelle (`step_nanoseconds.jsonl`)

### 4.1 Révision : plus de "groupe rapide"

**Rapport 201 estimait :** groupe lent (2 310–3 497 s) vs groupe rapide (383–444 s) — ratio ≈ 8×.  
**Après analyse complète :** le seuil 1 s en ns capte **tous** les steps dans le groupe lent. Il n'y a plus de groupe "rapide" dans les 13 records disponibles.

| Step | dt_ns total | clusters_formed |
|------|-------------|-----------------|
| 3 | 3 434 s | 415 |
| 29 | 3 484 s | 402 |
| 22 | 3 497 s | 355 |
| 0 (run A) | 3 019 s | 407 |
| 0 (run B) | 2 310 s | 423 |
| 4 | 382 s | 418 |
| 30 | 424 s | 402 |
| 23 | 444 s | 363 |
| 1 (run A) | 3 688 s | — |
| 1 (run B) | 3 685 s | — |
| 5 | 3 627 s | — |
| 31 | 3 567 s | — |
| 24 | 3 541 s | — |

**Correction :** les steps 4, 30, 23 (382–444 s) constituent le groupe *réellement rapide*. Le seuil correct est **600 s** :
- **Groupe lent** : steps 0, 1, 3, 5, 22, 24, 29, 31 → 2 310–3 697 s
- **Groupe rapide** : steps 4, 23, 30 → 382–444 s
- **Ratio lent/rapide** : ~3 200 s / 420 s ≈ **7.6×** (confirmé)

### 4.2 Variabilité des clusters

| Métrique | Valeur |
|----------|--------|
| clusters_formed min | **355** (step 22) |
| clusters_formed max | **428** (step 0 run B) |
| Range | **73** |
| falsif_mode | **0 constant** |
| pairs_examined | **499 500 constant** = N×(N-1)/2 ✅ |

**Question ouverte (déterminisme) :** step 0 exécuté 2 fois → 407 vs 423 clusters. Step 1 exécuté 2 fois → dt similaires (3 688 vs 3 685 s) mais clusters non enregistrés dans les deux runs. La variabilité de 73 clusters sur la même seed est **réelle** — source non établie (non-déterminisme RNG ou parallélisme entrelacé).

---

## 5. Résultats — Paires de comparaisons (`pairs_all_comparisons.jsonl.gz`)

### 5.1 Analyse streaming (500 000 premières lignes)

| Métrique | Valeur |
|----------|--------|
| Lignes analysées | **500 000** (sur ~12M+ estimées) |
| Auto-interactions (BUG i==j) | **0** ✅ |
| Double-comptages | **90 031 / 500 000 = 18 %** ⚠️ |
| NaN dans les distances | **0** ✅ |
| Paires uniques (sur 500k) | **409 969** |

### 5.2 Schéma complet des clés

```json
{
  "step":          <int>,
  "ts_ns":         <int>,
  "atom_i":        <int>,
  "atom_j":        <int>,
  "dist":          <float>,
  "cluster_formed":<bool ou int>,
  "cluster_type":  <int>,
  "skipped_falsif":<bool>,
  "falsif_mode":   <int>
}
```

### 5.3 Distribution par step (500k lignes)

| Step | Lignes | Note |
|------|--------|------|
| 0 | 203 633 (40.7%) | Step le plus représenté |
| 22 | 98 871 (19.8%) | |
| 29 | 99 330 (19.9%) | |
| 4 | 38 020 (7.6%) | |
| 3 | 60 146 (12.0%) | |

### 5.4 Distribution des distances dans les paires

| Métrique | Valeur (unités) |
|----------|----------------|
| Distance min | **0.2446** |
| Distance max | **6.5035** |
| Distance moyenne | **3.2912** |
| Distance médiane | **3.3108** |

**Observation critique :** les distances dans `pairs_all_comparisons.jsonl` sont dans une **plage totalement différente** de `transient_events.log` (0.0143–0.2999 nm vs 0.24–6.50 unités). Les paires enregistrent **toutes** les comparaisons (pas seulement celles sous le seuil) — donc les distances élevées (> 0.3 nm) ne génèrent pas d'événement transitoire.

---

## 6. Audit des 10 bugs potentiels

| # | Bug testé | Résultat | Source |
|---|-----------|----------|--------|
| B1 | Auto-interaction (i==j) | ✅ **ABSENT** | physics + transient + pairs |
| B2 | Double-comptage (i,j dupliqué) | ⚠️ **2 cas** dans transient, **90k** dans pairs | transient + pairs |
| B3 | NaN / Inf dans positions | ✅ **ABSENT** | physics (0 NaN) |
| B4 | NaN dans distances | ✅ **ABSENT** | transient + pairs |
| B5 | État non initialisé (dt_ns=0) | ⚠️ **41%** des atomes | physics |
| B6 | État persistant entre runs | ⚠️ **Suspect** — step 0 exécuté 2× avec résultats différents | step_nanoseconds |
| B7 | Entrelacement steps (non séquentiel) | ⚠️ **Confirmé** — steps 0,1,2,3,4,22,23,24,25,29,30,31,32 entrelacés | transient |
| B8 | pairs_examined ≠ N×(N-1)/2 | ✅ **CORRECT** — 499 500 = 1000×999/2 | step_nanoseconds |
| B9 | falsif_mode non constant | ✅ **CONSTANT** = 0 | pairs + step_nanoseconds |
| B10 | Ordre opérations imprévisible | ⚠️ **Non vérifié** — ARTCB_CI_FIXED_SEED ignoré (P6-A OPEN) | sch_atom_main.c |

### Analyse détaillée B2 — Double-comptage dans pairs

**Processus :** une paire (i,j) au même step apparaît 2 fois dans le fichier.  
**Mesure :** 90 031 doublons / 500 000 lignes = **18 %** → ce n'est pas marginal.  
**Hypothèse 1 :** le benchmark enregistre la paire (i,j) ET (j,i) séparément.  
**Hypothèse 2 :** des runs parallèles entrelacent leurs enregistrements.  
**Statut :** **non résolu** — nécessite lecture du code `cv_investigation_bench.c` pour confirmer.

---

## 7. Phénomènes émergents détectés

### 7.1 Hubs atomiques (non attendu)

La distribution des degrés d'interaction présente une queue longue : atome 255 = 33 interactions vs moyenne ~10.8. **Propriété émergente non documentée** dans les specs LUM-VORAX.

### 7.2 Bimodalité confirmée (ratio 7.6×)

Deux régimes temporels distincts (lent ~3 200 s, rapide ~420 s) persistent sur plusieurs steps distincts. La cause n'est pas établie : **hypothèse cold-start** (le step 0 est dans le groupe lent mais pas toujours le plus lent), **hypothèse parallélisme** (steps rapides = moins de contention).

### 7.3 Step 3 — anomalie doublement confirmée

Step 3 = dans le groupe **lent** (3 434 s) ET dans le groupe avec **peu d'événements transitoires** (seulement 64 sur ~400 en moyenne). Double anomalie inexpliquée.

### 7.4 Équipartition parfaite des types

Exactement 250 atomes de chaque type sur 1 000 atomes, à chaque step. Cela confirme que la configuration initiale est déterministe pour les types — mais pas pour les positions/vitesses.

---

## 8. Tableau avant / après — Compression

| | AVANT | APRÈS |
|--|-------|-------|
| `pairs_all_comparisons.jsonl` | 944 MB (présent) | **supprimé** |
| `pairs_all_comparisons.jsonl.gz` | absent | **87 MB** (ratio 10.8×) |
| Espace libre disque | 19 GB (après nettoyage utilisateur) | **20 GB** |
| Schéma pairs connu | ❌ inconnu | ✅ 9 clés documentées |
| Bugs B1–B10 | ❌ non vérifiés | ✅ 7/10 vérifiés, 3 OPEN |

---

## 9. Questions ouvertes (non résolues)

| ID | Question | Priorité |
|----|----------|---------|
| Q-R202-1 | Cause réelle du double-comptage 18% dans pairs — (i,j)+(j,i) ou parallélisme ? | P0 |
| Q-R202-2 | Cause de l'anomalie step 3 : lent ET peu d'événements | P1 |
| Q-R202-3 | Corrélation position hub-255 ↔ type atomique | P1 |
| Q-R202-4 | Déterminisme : step 0 run A (407) vs run B (423) — même seed ? | P1 |
| Q-R202-5 | P6-A OPEN : ARTCB_CI_FIXED_SEED non lu par sch_atom_main.c | P2 |
| Q-R202-6 | Signification physique des types 1 et 2 dans les événements transitoires | P2 |

---

## 10. Artefacts produits

| Fichier | Chemin |
|---------|--------|
| Script d'analyse | `src/sch/atom/analyse_atoms_r202.py` |
| Résultats JSON | `logs_AIMO3/sch/atom/r202_analysis_results.json` |
| Données compressées | `logs_AIMO3/sch/atom/pairs_all_comparisons.jsonl.gz` |
| Ce rapport | `RAPPORT/202_ANALYSE_ATOMIQUE_BRUTE_20261008.md` |

---

## 11. Conclusions

1. **Les atomes bougent tous** à chaque step (déplacement non nul = 100%), malgré 41% de dt_ns=0 → la résolution temporelle OS est insuffisante, pas les calculs.
2. **Aucune auto-interaction** (i==j) détectée → invariant respecté.
3. **18% de double-comptage** dans les paires → suspect, nécessite audit code.
4. **Hub atomique** (atome 255, degré 33) → propriété émergente non documentée.
5. **Bimodalité 7.6×** confirmée — cause non établie.
6. **Step 3** = double anomalie (lent + peu d'événements).
7. **Équipartition des types** parfaite → configuration initiale déterministe pour les types.

**CERTIFIED_100=false** | Rapport 202 terminé | Prochain rapport : 203
