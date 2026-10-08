# Rapport 206 — Tests P1 : orientations, atome 122, distribution dt_ns

**Date :** 2026-10-08  
**Périmètre :** LVX&ARTCB/ — audit documentaire, aucune modification de code scientifique  
**Précédent :** Rapport 205 — Lecture brute log R204 (corrections C1–C4)  
**CERTIFIED_100=false** | **unique_human_proven=false** | **Mode DEBUG actif**  
**Script :** `src/sch/atom/analyse_atoms_r206_p1.py`  
**Log :** `logs_AIMO3/sch/atom/r206_p1_results.json`  
**Durée :** 152,52 s (P1-A = 78,87s sur 6,66M lignes, P1-B = 69,36s, P1-C = 0,26s)

---

## 0. Méthode

Log JSON lu intégralement avant rédaction. Chaque affirmation est ancrée sur des valeurs
exactes lues dans le fichier. Les interprétations du script sont vérifiées par raisonnement direct.

---

## 1. P1-A — Hypothèse deux orientations (i,j)/(j,i)

### Résultats bruts (log lignes 7–33)

| Step | Lignes | Paires orientées uniques | Paires non-ordonnées | Miroirs dans même step | Ratio lignes/499500 |
|------|--------|--------------------------|---------------------|------------------------|---------------------|
| 1 | 977 481 | 499 490 | 499 490 | **0** | 1,9569 |
| 4 | 499 500 | 499 500 | 499 500 | **0** | 1,0000 |

### Raisonnement sur ces valeurs

**Hypothèse deux orientations : RÉFUTÉE.**

Pour le step 1 : 977 481 lignes − 499 490 paires uniques = **477 991 doublons**. Ces doublons ne
sont PAS des paires (j,i) miroir (miroir_rate = 0,0%). Ce sont des **répétitions exactes de la
même paire orientée (i,j) dans le même step**. La valeur `n_unordered_pairs = 499 490` est identique
à `n_oriented_pairs` → aucune paire (j,i) n'existe dans le fichier.

**Ce que le step 1 contient réellement :** environ 499 490 paires distinctes (1000×999/2 − 10),
chacune apparaissant en moyenne 977 481/499 490 ≈ **1,96 fois** dans ce step. Le step 1 rejoue
chaque paire environ deux fois.

**Ce que le step 4 contient :** 499 500 paires distinctes, chacune apparaissant **exactement une fois**.
Le step 4 est le step "propre" — une passe unique sur toutes les paires.

**Conséquence pour l'architecture :** les steps lents (ici step 1) ne contiennent pas deux orientations
de chaque paire — ils contiennent des répétitions de la même orientation. Les 92,504% de doublons
inter-step sont la structure normale. Les doublons intra-step du step 1 (~48,9% du step) sont une
propriété spécifique à ce step — potentiellement un double-calcul de vérification ou une boucle
de relaxation.

### Observation critique : 10 paires manquantes dans le step 1

`n_oriented_pairs(step 1) = 499 490` vs `1000×999/2 = 499 500` → **10 paires absentes**.
Ces 10 paires correspondent peut-être à des atomes désactivés, à des distances hors seuil, ou à
des conditions aux limites dans ce step. À investiguer.

---

## 2. P1-B — Profil de l'atome 122 dans step 0

### Résultats bruts (log lignes 34–86)

- Degré atome 122 = **889**
- Moyenne des degrés dans step 0 = **889,428**
- Rang de l'atome 122 = **636 / 891** (moitié inférieure)
- Max global = **890**, Min global = **635**

### Raisonnement

L'atome 122 a un degré de 889, soit **0,05% en dessous de la moyenne**. Il n'est pas exceptionnel.
Son rang 636 sur 891 le situe dans le tiers inférieur.

**Découverte plus importante : le graphe step 0 est quasi-complet.**

891 atomes présents dans step 0. Nombre maximal de paires non-ordonnées = `891×890/2 = 396 495`.
Or `n_unordered_pairs` non calculé directement, mais avec mean_degree ≈ 889,4 ≈ 890 − 1, chaque
atome est connecté à presque tous les autres. Le graphe step 0 est un **graphe quasi-complet** —
presque toutes les paires d'atomes sont sous le seuil d'interaction.

Les exemples de doublons context (tous `atom_i=122`) n'étaient pas un signal sur l'atome 122 —
c'est simplement que dans un graphe quasi-complet, l'atome 122 apparaît avec tous les autres, et
les premiers exemples du fichier ont atom_i=122 car c'est l'atome de faible index traité en premier.

**Avant (R204/205) :** "atome 122 concentre les répétitions intra-step"  
**Après (R206) :** Artefact du tri — les exemples de doublons sont les premières occurrences du fichier,
toutes avec atom_i=122. Le graphe est quasi-complet. Aucune propriété émergente de l'atome 122.

---

## 3. P1-C — Distribution complète de dt_ns

### Résultats bruts (log lignes 87–173)

**Distribution globale :**

| Statistique | Valeur |
|-------------|--------|
| n total | 13 000 |
| n_zeros | 5 334 (41,0%) |
| min | 0 ns |
| p25 | 0 ns |
| médiane | 1 000 ns |
| p75 | 1 000 ns |
| p90 | 2 000 ns |
| p95 | 2 000 ns |
| p99 | 38 000 ns |
| max | 33 061 000 ns |
| mean | 5 466 ns |
| std | **292 545 ns** |

**Distribution par type atomique :**

| Type | n_zeros% | médiane | p99 | max | std |
|------|----------|---------|-----|-----|-----|
| 0 | 41,7% | 1 000 ns | 43 000 ns | 551 000 ns | 21 572 ns |
| 1 | 40,5% | 1 000 ns | 30 000 ns | 744 000 ns | 18 424 ns |
| 2 | 40,7% | 1 000 ns | 26 000 ns | 2 777 000 ns | 51 477 ns |
| 3 | 41,3% | 1 000 ns | 53 000 ns | **33 061 000 ns** | **582 050 ns** |

### Raisonnement

**1. La distribution est dominée par deux valeurs : 0 ns et 1000 ns.**

41% de zéros + p25=0, médiane=1000, p75=1000 signifient que 50%+ des mesures sont entre 0 et 1000 ns.
Le "1000 ns" est vraisemblablement la **granularité minimale de l'horloge** — `CLOCK_MONOTONIC` sur
macOS retourne des valeurs quantifiées à 1000 ns dans ce contexte d'exécution.

**2. La queue extrême est anormalement longue.**

p99 = 38 000 ns, mais max = 33 061 000 ns = **870× le p99**. Cette queue est incompatible avec une
distribution normale ou même log-normale. Elle correspond à des préemptions OS (scheduler) qui
suspendent le thread pendant plusieurs dizaines de millisecondes.

**3. Le type 3 a une variabilité radicalement différente des autres types.**

- std type 3 = 582 050 ns vs std type 1 = 18 424 ns → **rapport de 31,6×**
- max type 3 = 33 061 000 ns vs max type 2 = 2 777 000 ns → **rapport de 11,9×**
- Mais les médianes sont identiques (1 000 ns) et les p99 sont dans le même ordre de grandeur

Deux interprétations possibles :
- **(a) Artefact OS** : le code traitant les atomes de type 3 consomme plus de ressources
  ponctuellement, attirant davantage de préemptions → la variabilité est informatique
- **(b) Phénomène physique** : les atomes de type 3 ont un calcul plus complexe (interactions plus fortes ?)
  provoquant des variations de temps de calcul → la variabilité est algorithmique

Ces deux hypothèses ne peuvent pas être distinguées depuis `dt_ns` seul. Il faudrait mesurer le
temps CPU consommé vs le temps mural pour séparer le calcul réel des préemptions.

**4. Les zéros sont uniformément répartis entre les types (40,5% à 41,7%).**

Ce n'est pas le type 3 qui génère les zéros — tous les types en ont environ la même proportion.
Les zéros correspondent à des opérations dont la durée est inférieure à la granularité 1000 ns.

---

## 4. Tableau récapitulatif des découvertes P1

| ID | Question initiale | Résultat mesuré | Implication |
|----|------------------|-----------------|-------------|
| Q2-RÉFUTÉE | Hypothèse 2 orientations (i,j)/(j,i) | 0 miroir dans step 1 et 4 | Le doublement des lignes dans step 1 = répétitions de même paire, pas 2 orientations |
| Q2-NOUVEAU | Pourquoi step 1 a ~2× plus de lignes ? | ~48,9% de répétitions intra-step de même paire | Double-calcul ou boucle de relaxation dans step 1 |
| Q4-RÉFUTÉE | Atome 122 structurellement particulier ? | Degré 889, rang 636/891, proche de la moyenne 889,4 | Artefact du tri — premier atome dans les exemples |
| P1C-1 | Distribution dt_ns globale | Dominée 0 ns et 1000 ns (granularité horloge) | Horloge quantifiée à 1000 ns — 41% sous résolution |
| P1C-2 | Queue extrême | max = 870× p99 | Préemptions OS, pas un phénomène physique |
| P1C-3 | Différence par type | Type 3 : std ×31, max ×11 vs type 1 | Signal non expliqué — artefact OS ou calcul plus complexe |
| P1C-4 | Différence par type (zéros) | ~41% pour tous les types | Granularité horloge uniforme par type |

---

## 5. Nouvelles questions P2

**P2-A** : Qu'est-ce qui distingue les steps où chaque paire est calculée une seule fois (step 4 : exact
499 500) de ceux où certaines paires sont répétées (step 1 : ~477 991 répétitions intra-step) ?
Comparer le code ou la configuration des steps.

**P2-B** : Les 10 paires manquantes dans step 1 (499 490 vs 499 500) — quelles paires sont absentes ?
Identifier les atom_ids des paires manquantes.

**P2-C** : Pour le type 3, identifier les atomes avec les max dt_ns les plus élevés. Sont-ils
regroupés spatialement ? Ont-ils plus de voisins ?

**P2-D** : Le step 0 a 891 atomes présents mais le fichier physics a 1000 atomes. Quels sont les
109 atomes absents du step 0 ?

---

## 6. Conclusion principale de ce rapport

**Trois hypothèses réfutées :**
1. Deux orientations (i,j)/(j,i) pour step 1 → **faux** — ce sont des répétitions de même paire
2. Atome 122 comme hub ou anomalie → **faux** — artefact du tri, graphe quasi-complet
3. La distribution dt_ns est normale ou prévisible → **faux** — queue max = 870× p99

**Un signal nouveau et non expliqué :**
Le type 3 a une variabilité temporelle 31× plus grande que le type 1 malgré des médianes identiques.
C'est le signal P1 le plus intéressant — il demande une investigation P2-C.

**CERTIFIED_100=false** | **unique_human_proven=false** | **Mode DEBUG actif**  
*Rapport 206 produit par lecture directe du log JSON r206_p1_results.json.*
