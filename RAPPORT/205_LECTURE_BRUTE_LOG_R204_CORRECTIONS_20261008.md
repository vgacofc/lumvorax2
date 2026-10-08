# Rapport 205 — Lecture brute du log R204 : corrections et nouvelles observations

**Date :** 2026-10-08  
**Périmètre :** LVX&ARTCB/ — audit documentaire, aucune modification de code scientifique  
**Précédent :** Rapport 204 — Tests P0-A/B/C + corrections R202  
**CERTIFIED_100=false** | **unique_human_proven=false** | **Mode DEBUG actif**  
**Source primaire :** `logs_AIMO3/sch/atom/r204_analysis_results.json` — lue ligne par ligne par l'agent  
**Principe appliqué (L-060) :** Un script ne raisonne pas. L'agent lit le JSON brut lui-même et raisonne sur ce qu'il voit — pas sur le résumé produit par le script.

---

## 0. Méthode de ce rapport

Ce rapport ne repose sur aucun nouveau script. Il est produit par lecture directe du fichier
`r204_analysis_results.json` (405 lignes). Chaque observation est ancrée sur des numéros de
ligne et des valeurs exactes lues dans le fichier. Les interprétations du script sont distinctement
signalées et évaluées à leur juste valeur.

---

## 1. Correction C1 — Le ratio 2,15 masque trois régimes, pas deux

### Ce que le script a produit (lignes 81–82)

```json
"ratio_slow_fast": 2.15,
"interpretation": "Bimodalité CONFIRMÉE : 7 steps lents / 6 steps rapides, ratio=2.15×"
```

### Ce que le log révèle à la lecture brute

Le groupe "fast" (lignes 49–80) contient 6 éléments :

| Step | dt_s |
|------|------|
| 0 | 2310,1 |
| 0 | 3019,1 |
| 3 | 3434,6 |
| **4** | **382,5** |
| **23** | **444,4** |
| **30** | **424,5** |

Le seuil utilisé est la **médiane** = 3484,8 s (ligne 10). Ce seuil place dans le groupe "fast"
des valeurs à 2310 s, 3019 s et 3434 s — soit des valeurs proches du groupe "slow" (3484–3688 s).

**La structure réelle est à trois niveaux :**

| Régime | Steps | Plage (s) |
|--------|-------|-----------|
| ULTRA-RAPIDE | 4, 23, 30 | 382–444 |
| MODÉRÉ | 0 (×2), 3 | 2310–3434 |
| LENT | 1 (×2), 5, 22, 24, 29, 31 | 3484–3688 |

**Ratio réel ultra-rapide vs ultra-lent :** 3688 / 382 ≈ **9,65×** (3 observations dans chaque extrême).

**Avant (R204) :** "Bimodalité confirmée, ratio 2,15×"  
**Après (R205) :** Structure à trois régimes observée ; ratio 2,15 est un artefact du seuil médiane
qui mélange le régime MODÉRÉ avec les 3 ultra-rapides. Le vrai écart extrême est ~9,65×
sur 3 observations seulement — pas suffisant pour conclure à une bimodalité statistique formelle.

---

## 2. Correction C2 — SYM_DOMINANT est une interprétation fausse

### Ce que le script a produit (lignes 219–221)

```json
"interpretation": [
  "SYM_DOMINANT: majorité des répétitions = symétrie (i,j)/(j,i)",
  "CONTEXT_DOUBLONS: 1312864 répétitions intra-step..."
]
```

### Ce que le log révèle à la lecture brute

Clé strict (lignes 103–133) :

```
unique    = 499 500
duplicates = 6 163 667   dup_pct = 92,504
```

Clé sym / min(i,j)+max(i,j) (lignes 135–186) :

```
unique    = 499 500
duplicates = 6 163 667   dup_pct = 92,504
```

**Les deux clés donnent exactement le même résultat.** La clé symétrique `(min(i,j), max(i,j))`
n'a réduit le nombre d'uniques d'aucune unité par rapport à la clé stricte `(step, i, j)`.

**Raisonnement logique direct :** si `sym.unique == strict.unique`, cela signifie que dans tout
le fichier il n'existe **aucune paire** où `(i,j)` et `(j,i)` coexistent dans le même step.
La symétrie n'est donc PAS la cause des 92,504% de doublons.

**Ce qui explique les 92,504% de doublons :** la clé `strict` utilisée dans le script est en fait
`(atom_i, atom_j)` **sans le step** (les exemples lignes 107–133 montrent `step`, `atom_i`, `atom_j`
mais les steps sont identiques dans les exemples — le script ne l'a pas clarifié).

La clé `context` (lignes 187–217) qui inclut le step donne :
```
unique    = 5 350 303
duplicates = 1 312 864   dup_pct = 19,703
```

**Conclusion correcte :** les 92,504% sont des répétitions **inter-step** — la même paire
`(atom_i, atom_j)` réapparaît dans plusieurs steps différents. C'est la structure normale d'une
simulation multi-step. La symétrie (j,i) n'est probablement pas présente du tout dans le fichier.

**Avant (R204) :** "SYM_DOMINANT"  
**Après (R205) :** Interprétation fausse. Les doublons sont inter-step (même paire, steps différents).
La symétrie (j,i) est absente du fichier — résultat important pour la compréhension de l'architecture.

---

## 3. Observation nouvelle O1 — Incohérence interne entre bimodal_fix et p0c

### Observé à la lecture brute

`bimodal_fix.fast_group` (lignes 49–80) contient 6 steps : 0, 0, 3, 4, 23, 30.

`p0c.fast_steps` (lignes 290–293) contient 3 steps : `[4, 23, 30]`.  
`p0c.slow_steps` (lignes 294–297) contient 3 steps : `[3, 22, 29]`.

Le step 3 est dans `bimodal_fix.fast_group` mais dans `p0c.slow_steps`.  
Les steps 0 (×2) ne sont dans aucun des deux groupes p0c.

**Conséquence :** P0-C a été calculé sur les 3 vrais ultra-rapides (4, 23, 30) contre 3 modérés
(3, 22, 29). C'est une comparaison plus pertinente que les 6 vs 7 de bimodal_fix, mais elle n'a
pas été documentée explicitement dans le rapport 204. Le rapport 204 disait "6 steps fast / 7 slow"
pour la bimodalité, mais le test P0-C réel utilisait une sélection différente.

---

## 4. Observation nouvelle O2 — Dispersion dt_ns non exploitée dans P0-C

### Valeurs lues directement (lignes 331–396)

**Groupe FAST (steps 4, 23, 30) :**
- mean = 1641 ns, std = **11 039 ns**, min = 0, max = **258 000 ns**

**Groupe SLOW (steps 3, 22, 29) :**
- mean = 1609 ns, std = **20 057 ns**, min = 0, max = **616 000 ns**

**comparison.dt_ns_rel_diff = 0,0197** (2%)

### Raisonnement

Le script a conclu "SAME_STATE" en regardant la différence relative des **moyennes** (2%).
Mais les écarts-types diffèrent de **1,82×** et les valeurs maximales de **2,39×**.

Deux distributions avec la même moyenne mais des variances très différentes ne sont pas
statistiquement identiques. La distribution des temps atomiques est beaucoup plus dispersée
dans les steps lents que dans les steps rapides.

**Hypothèse à tester :** les steps lents présentent davantage de préemptions du thread OS
(pics à 616 000 ns) que les steps rapides (max 258 000 ns). C'est cohérent avec l'hypothèse
d'un artefact CPU/OS, mais ce n'est pas la même chose que dire "état physique identique".

**Avant (R204) :** "positions et vitesses statistiquement identiques (<5% diff) → ARTEFACT CPU/OS probable"  
**Après (R205) :** La conclusion sur les positions/vitesses est correcte. Mais la dispersion temporelle
(std ×1,82 et max ×2,39) est un signal supplémentaire non exploité qui suggère que les steps lents
subissent davantage d'interruptions OS — cela renforce l'hypothèse artefact mais ne la prouve pas.

---

## 5. Correction C3 — P0-B n'est pas interprétable physiquement sans run_id

### Ce que le log révèle (lignes 225–287)

```json
"method": "inferred_groups_of_1000",
"n_transitions": 12000,
"n_discont_pos": 12000,
"continuity_rate_pos_pct": 0.0
```

La méthode `inferred_groups_of_1000` découpe `physics_all_atoms.jsonl` en groupes de 1000 lignes
successives et compare la position de l'atome N dans le groupe k à sa position dans le groupe k+1.

**Problème fondamental :** sans `run_id` dans le fichier source, deux groupes successifs de 1000
lignes peuvent appartenir à des runs entièrement distincts. Dans ce cas, chaque `delta_pos` mesure
la distance entre la position finale d'un run et la position initiale d'un autre run — valeur
sans signification physique de continuité.

Les exemples (lignes 233–282) montrent des delta_pos = 2,01 à 5,55 nm pour les atomes 0–9 au
`inferred_step_N=0`. Ces valeurs sont de l'ordre de grandeur de l'espacement initial entre atomes
dans une boîte de simulation typique — cohérent avec deux configurations initiales distinctes.

**Avant (R204) :** "CONTINUITY_BROKEN: 100% discontinu — snapshots indépendants"  
**Après (R205) :** Résultat non interprétable physiquement. Le 100% de discontinuités peut
simplement signifier que des snapshots de runs distincts ont été juxtaposés. Il n'est pas possible
de conclure que les "fichiers sont des snapshots indépendants" depuis ce test seul — cela supposerait
que les 13 groupes correspondent à des steps d'un même run, ce qui n'est pas démontré.

---

## 6. Observation nouvelle O3 — Steps 6, 22, 25, 32 présents dans pairs sans mesure temporelle

### Lu directement dans step_distribution (lignes 86–101)

| Step | Lignes dans pairs.gz |
|------|---------------------|
| 6 | 284 726 |
| 22 | 282 890 |
| 25 | 55 994 |
| 32 | 97 822 |

Ces 4 steps représentent **721 432 lignes** (10,8% du fichier).

Le fichier `step_nanoseconds` contient 13 entrées pour les steps :
0 (×2), 1 (×2), 3, 4, 5, 22, 23, 24, 29, 30, 31.

**Steps 6, 25 et 32 ont des données de paires mais aucune mesure temporelle.**
Le step 22 est dans les deux (présent dans pairs ET dans step_nanoseconds).

Cela signifie soit : (a) ces steps appartiennent à des runs dont la mesure temporelle
n'a pas été journalisée ; (b) l'instrumentation temporelle et l'instrumentation des paires
proviennent de chemins d'exécution distincts.

Non signalé dans le rapport 204.

---

## 7. Observation nouvelle O4 — Structure des doublons inter-step

### Lu dans la clé context (lignes 187–218)

Avec la clé `(step, atom_i, atom_j)` : unique = 5 350 303, duplicates = **1 312 864** (19,7%).

Cela signifie que 1 312 864 tuples `(step, atom_i, atom_j)` apparaissent plus d'une fois dans le
fichier. Ce sont des répétitions **intra-step avec la même paire** — c'est-à-dire que dans un même
step donné, la paire (i,j) a été évaluée plusieurs fois.

Les exemples (lignes 191–217) montrent tous step=0 avec atom_i=122 et atom_j=758→762.
**Tous les exemples de doublons context proviennent du step 0.** Cela suggère que le step 0 est
particulièrement affecté par les répétitions intra-step — peut-être parce qu'il correspond à une
phase d'initialisation exécutée plusieurs fois, ou à plusieurs runs ayant le même step 0.

---

## 8b. Correction C4 — P0-B : physics_all_atoms.jsonl n'a pas de champ `step`

### Lu dans le code source (lignes 230–233 de analyse_atoms_r204.py)

```python
if n_atoms_with_step == 0:
    # Le champ step n'existe pas → inférence par groupes de 1000
    log("P0-B : pas de champ 'step' — inférence par groupes de 1000 lignes")
    return _p0b_infer_steps()
```

Le fait que le log ait utilisé la méthode `inferred_groups_of_1000` **prouve** que `physics_all_atoms.jsonl`
ne contient aucun enregistrement avec un champ `step` valide. Le fichier est une séquence brute de
13 000 lignes atomiques sans identification de step ni de run.

Conséquence directe sur l'interprétation de P0-B :
- Les "13 inferred steps" sont artificiels (groupes de 1000 lignes consécutives)
- Les 12 000 transitions comparent l'atome k dans le groupe G à l'atome k dans le groupe G+1
- Sans garantie que c'est le même atome dans la même simulation
- Le 100% de discontinuités est **inévitable par construction** si les groupes viennent de runs distincts

**Avant (R204) :** méthode décrite en passant
**Après (R205) :** Clarification décisive — le fichier physique n'est pas indexé par step/run.
P0-B ne peut pas répondre à la question "les snapshots sont-ils d'une trajectoire continue".

---

## 9. Tableau récapitulatif — Avant / Après (complet)

| ID | Formulation R204 | Correction R205 | Source |
|----|-----------------|-----------------|--------|
| C1 | "Bimodalité confirmée, ratio 2,15×" | Trois régimes : ultra-rapide (382–444s) / modéré (2310–3434s) / lent (3484–3688s). Ratio 2,15 = artefact du seuil médiane. | log L.81–82 |
| C2 | "SYM_DOMINANT" | Faux. sym.unique == strict.unique → aucune paire (j,i) miroir. Doublons 92,5% = inter-step (clé sans step, confirmé code L.102). | log L.103–221 + code L.102 |
| C3 | "100% discontinu = snapshots indépendants" | Non interprétable sans run_id. Deux runs juxtaposés = 100% inévitable par construction. | log L.225–287 |
| C4 | — | physics_all_atoms.jsonl n'a pas de champ `step` — prouvé par le branchement code L.230–233. P0-B ne mesure pas la continuité. | code L.230–233 |
| O1 | — | Incohérence interne : bimodal_fix.fast = 6 steps, p0c.fast_steps = 3 — sélections différentes non documentées. | log L.49–80 / 290–297 |
| O2 | "SAME_STATE (<5% diff)" | Positions/vitesses OK mais std dt_ns ×1,82 et max ×2,39 non exploités. | log L.331–396 |
| O3 | — | Steps 6, 25, 32 : 721 432 lignes de paires sans mesure temporelle. | log L.86–101 |
| O4 | — | Doublons context (step=0, atom_i=122) — step 0 concentre les répétitions intra-step. | log L.191–217 |

---

## 10. Questions ouvertes P1

**Q1 — RÉPONDUE** : clé `strict` = `(atom_i, atom_j)` sans step (code L.102). Les 92,504% de doublons
sont normaux et attendus — même paire réapparaissant dans des steps différents. Pas un bug.

**Q2** : Steps 4, 23, 30 ont exactement 499 500 lignes = `1000×999/2`. Steps 1 (~977 481) et 2
(~919 811) approchent `999 000 = 2×499 500`. Hypothèse deux orientations (i,j)+(j,i) pour ces steps.

**Q3** : Les 13 enregistrements temporels (step_nanoseconds) correspondent-ils au même run ?
Sans `run_id`, indéterminé.

**Q4** : L'atome 122 (tous les exemples de doublons context) est-il structurellement particulier,
ou simplement le premier atome du fichier avec cet index ?

---

## 11. Prochaine action

Écrire le script P1 avec les trois tests décisifs :
1. Pour step 1 et step 4 : compter explicitement les paires `(i,j)` et `(j,i)` coexistantes
   dans le même step → valider/invalider l'hypothèse deux orientations.
2. Pour step 0 : profil de l'atome 122 (type, position, degré) → O4.
3. Construire la distribution complète de `dt_ns` (histogramme, quantiles, par step, par type).

---

**CERTIFIED_100=false** | **unique_human_proven=false** | **Mode DEBUG actif**  
*Rapport 205 produit par lecture directe du log JSON — aucun nouveau script exécuté.*
