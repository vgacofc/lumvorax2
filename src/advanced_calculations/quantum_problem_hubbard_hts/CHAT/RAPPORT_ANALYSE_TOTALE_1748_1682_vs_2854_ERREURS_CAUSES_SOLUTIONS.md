# AUTO-PROMPT — MODÈLE DE REQUÊTE POUR CE TYPE D'ANALYSE
*(Inclus en tête de rapport conformément aux instructions. À réutiliser tel quel pour les cycles suivants.)*

```
Lis toi-même ligne par ligne, fichier par fichier, chaque CSV, JSON, log et code source C
des nouveaux runs ainsi que les anciens runs de référence et toutes les analyses antérieures :

NOUVEAUX RUNS (à analyser) :
  src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T203411Z_1748/
  src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T202816Z_1682/

ANCIENS RUNS (référence) :
  src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T182101Z_2854/
  src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T181131Z_2715/
  src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T123420Z_1312/

ANALYSES PRÉCÉDENTES (ne jamais modifier) :
  src/advanced_calculations/quantum_problem_hubbard_hts/CHAT/

CODE SOURCE C (à auditer ligne par ligne) :
  src/advanced_calculations/quantum_problem_hubbard_hts/src/hubbard_hts_research_cycle_advanced_parallel.c
  src/advanced_calculations/quantum_problem_hubbard_hts/src/hubbard_hts_research_cycle.c

Sauvegarder le rapport dans CHAT/ sans modifier aucun fichier existant.
```

---

# RAPPORT D'ANALYSE TOTALE — RUNS 1748 ET 1682
## `research_20260312T203411Z_1748` (fullscale V2) et `research_20260312T202816Z_1682` (fullscale gelé)
## vs référence `research_20260312T182101Z_2854` (advanced_parallel — meilleur run connu)
**Date :** 2026-03-12 20:34Z | **Fichiers lus :** tous CSV, JSON, logs, MD, code C audité

---

## CONTEXTE — DEUX MOTEURS, DEUX ARCHITECTURES RADICALEMENT DIFFÉRENTES

La dernière exécution de campagne a produit **deux runs simultanés** utilisant des variantes
différentes du moteur `hubbard_hts_research_cycle.c` (fullscale) — **ni l'un ni l'autre n'est
le moteur `advanced_parallel`** qui avait atteint les meilleures performances au run 2854.

**C'est-à-dire ?** Le meilleur moteur (advanced_parallel, run 2854) n'a PAS été réexécuté.
Les deux nouveaux runs utilisent l'ancienne architecture fullscale, dans deux états différents.

| Run | Moteur | Heure | Énergies | Pairing | Vitesse |
|---|---|---|---|---|---|
| `research_20260312T182101Z_2854` | advanced_parallel ← **RÉFÉRENCE** | 18:21Z | **Négatives ✓** | 0.84–0.92 | Rapide |
| `research_20260312T203411Z_1748` | fullscale V2 (sans centering) | 20:34Z | **Positives ✗** | 0.67–0.92 | Rapide |
| `research_20260312T202816Z_1682` | fullscale V1 (valeurs gelées) | 20:28Z | **Gelées/mixtes ✗** | 0.07–0.50 | **×50 plus lent** |

---

## GLOSSAIRE RAPIDE

- **Centering correction :** Terme `n_up*n_dn − 0.25` dans l'énergie locale. Présent dans `advanced_parallel.c`, ABSENT dans `fullscale.c`. Sans lui, l'énergie est toujours positive.
- **Valeur gelée :** Énergie ou pairing identique à 10 décimales de step 100 à 2700. Signature d'un calcul effectué une seule fois puis copié à chaque step.
- **Drift d'énergie :** Écart max de l'énergie entre les checkpoints 2200/4400/6600/8800 steps. Seuil physique : < 1×10⁻⁶.
- **Von Neumann :** Test de stabilité spectrale. Rayon spectral ≤ 1 = stable. Les deux runs : 0.9984608348 (PASS).
- **Robust score :** Score de robustesse aux perturbations (0–100). Run 1748 = 58, Run 1682 = 92.
- **Valeur rationnelle :** Fraction exacte produite par un calcul fermé (ex. 0.1600000000 = 4/25). Signature de calcul hardcodé ou convergé instantanément.

---

## PARTIE I — INVENTAIRE DES FICHIERS

### 1.1 Run 1748 — Fichiers présents

| Dossier | Fichiers clés |
|---|---|
| `logs/` | baseline_reanalysis_metrics.csv, normalized_observables_trace.csv, research_execution.log, provenance.log |
| `tests/` | benchmark_comparison_qmc_dmrg.csv, numerical_stability_suite.csv, toy_model_validation.csv, new_tests_results.csv, temporal_derivatives_variance.csv |
| `reports/` | RAPPORT_COMPARAISON_AVANT_APRES_CYCLE06.md, RAPPORT_RECHERCHE_CYCLE_06_ADVANCED.md |

### 1.2 Run 1682 — Fichiers présents

| Dossier | Fichiers clés |
|---|---|
| `logs/` | baseline_reanalysis_metrics.csv, normalized_observables_trace.csv, research_execution.log, provenance.log |
| `tests/` | benchmark_comparison_qmc_dmrg.csv, numerical_stability_suite.csv, toy_model_validation.csv |
| `reports/` | RAPPORT_COMPARAISON_AVANT_APRES_CYCLE06.md, RAPPORT_RECHERCHE_CYCLE_06_ADVANCED.md |

### 1.3 Fichiers présents dans 2854 mais ABSENTS dans 1748 et 1682

| Fichier absent | Conséquence |
|---|---|
| `tests/integration_v4next_rollout_status.csv` | V4-Next non évalué dans les nouveaux runs |
| `tests/integration_problem_solution_progress.csv` | Progression par problème non tracée |
| `tests/integration_physics_gate_summary.csv` | Portes physiques non générées |
| `logs/analysis_scientifique_summary.json` | Résumé scientifique absent |
| `logs/parallel_calibration_bridge_summary.json` | Calibration parallèle non générée |
| `audit/` | Dossier d'audit cryptographique entier absent des deux nouveaux runs |

**Donc :** Les deux nouveaux runs ont une couverture d'artefacts significativement réduite
par rapport au run 2854 (environ 15 fichiers de tests avancés manquants).

---

## PARTIE II — TABLEAU COMPARATIF LIGNE PAR LIGNE — 6 RUNS

### 2.1 Observables énergétiques — module `hubbard_hts_core`

| Observable | Run 523 | Run 1312 | Run 2715 | Run 2854 | **Run 1748** | **Run 1682** |
|---|---|---|---|---|---|---|
| Moteur | fullscale gelé | fullscale V1 | fullscale gelé | advanced_parallel | **fullscale V2** | **fullscale gelé V2** |
| Énergie finale | −1.040 | +1.761 | −1.040 | **−0.039 ✓** | **+1.985 ✗** | **+0.160 ✗** |
| Signe énergie | Négatif ✓* | Positif ✗ | Négatif ✓* | **Négatif ✓** | **Positif ✗** | **Positif ✗** |
| Valeur gelée ? | OUI ✗ | NON | OUI ✗ | NON ✓ | NON ✓ | **OUI ✗ CRITIQUE** |
| Pairing final | 0.241 | 0.847 | 0.241 | **0.847 ✓** | **0.847 ✓** | **0.241 ✗** |
| Drift énergie | ~0 (gelé) | 4.01×10⁻⁶ ✗ | ~0 (gelé) | **8.3×10⁻⁸ ✓** | **4.5×10⁻⁶ ✗** | **3.6×10⁻⁷ ✓*** |
| Von Neumann | 0.9985 ✓ | 0.9985 ✓ | 0.9985 ✓ | **0.9985 ✓** | **0.9985 ✓** | **0.9985 ✓** |
| Benchmark pairing 1/8 | 0/8 ✗ | 0/8 ✗ | 0/8 ✗ | **0/8 ✗** | **1/8 ✓ partiel** | **0/8 ✗** |
| conv_monotonic | PASS | FAIL | PASS | **FAIL** | **FAIL** | **PASS (artificiel)** |

*Négatif artificiel par valeur gelée hardcodée, non physique.
**Drift PASS car valeur gelée = dérive réelle zéro (faux positif).

### 2.2 Observables — tous les 13 modules — Run 1748 vs Run 2854

| Module | Énergie 2854 (✓) | Énergie 1748 | Pairing 2854 | Pairing 1748 | Drift 2854 | Drift 1748 |
|---|---|---|---|---|---|---|
| `hubbard_hts_core` | −0.039 | **+1.985** ✗ | 0.847 | **0.847** ✓ | 8.3×10⁻⁸ ✓ | **4.5×10⁻⁶** ✗ |
| `qcd_lattice_fullscale` | −0.052 | **+2.221** ✗ | 0.762 | **0.762** ✓ | 1.3×10⁻⁷ ✓ | **6.2×10⁻⁶** ✗ |
| `quantum_field_noneq` | −0.054 | **+1.738** ✗ | 0.675 | **0.676** ✓ | 2.0×10⁻⁷ ✓ | **6.2×10⁻⁶** ✗ |
| `dense_nuclear_fullscale` | −0.070 | **+2.710** ✗ | 0.849 | **0.849** ✓ | 2.0×10⁻⁷ ✓ | **8.6×10⁻⁶** ✗ |
| `quantum_chemistry_fullscale` | −0.064 | **+1.621** ✗ | 0.876 | **0.875** ✓ | 2.5×10⁻⁷ ✓ | **6.6×10⁻⁶** ✗ |
| `spin_liquid_exotic` | −0.042 | **+2.603** ✗ | 0.916 | **0.915** ✓ | 7.7×10⁻⁸ ✓ | **4.9×10⁻⁶** ✗ |
| `topological_correlated_materials` | −0.034 | **+1.939** ✗ | 0.897 | **0.896** ✓ | 7.0×10⁻⁸ ✓ | **3.6×10⁻⁶** ✗ |
| `correlated_fermions_non_hubbard` | −0.048 | **+2.134** ✗ | 0.856 | **0.856** ✓ | 9.8×10⁻⁸ ✓ | **5.4×10⁻⁶** ✗ |
| `multi_state_excited_chemistry` | −0.046 | **+1.694** ✗ | 0.914 | **0.914** ✓ | 1.6×10⁻⁷ ✓ | **4.8×10⁻⁶** ✗ |
| `bosonic_multimode_systems` | −0.031 | **+1.287** ✗ | 0.808 | **0.808** ✓ | 9.8×10⁻⁸ ✓ | **3.7×10⁻⁶** ✗ |
| `multiscale_nonlinear_field_models` | −0.054 | **+2.285** ✗ | 0.807 | **0.807** ✓ | 9.7×10⁻⁸ ✓ | **5.4×10⁻⁶** ✗ |
| `far_from_equilibrium_kinetic_lattices` | −0.040 | **+1.984** ✗ | 0.768 | **0.768** ✓ | 9.2×10⁻⁸ ✓ | **4.6×10⁻⁶** ✗ |
| `multi_correlated_fermion_boson_networks` | −0.038 | **+1.838** ✗ | 0.840 | **0.840** ✓ | 9.6×10⁻⁸ ✓ | **4.2×10⁻⁶** ✗ |

**Observation centrale :** Le pairing du run 1748 est **identique à 3 décimales** au run 2854
pour tous les 13 modules. **C'est-à-dire ?** Le pairing est calculé correctement par les deux
moteurs. L'unique différence est le signe de l'énergie, dû à l'absence du centering (−0.25).

### 2.3 Run 1682 — Révélation sur les valeurs gelées

**Diagnostic complet module par module (valeur finale step 2700) :**

| Module | Énergie gelée | Valeur rationnelle équivalente | Nature |
|---|---|---|---|
| `hubbard_hts_core` | **+0.1600000000** | 4/25 | Fraction exacte ✗ |
| `qcd_lattice_fullscale` | **−0.0604938272** | −49/810 | Fraction exacte ✗ |
| `quantum_field_noneq` | **+0.0812500000** | 13/160 | Fraction exacte ✗ |
| `dense_nuclear_fullscale` | **−0.0000000000** | **ZÉRO EXACT** | Annulation totale ✗✗ |
| `quantum_chemistry_fullscale` | **+0.2285714286** | 8/35 | Fraction exacte ✗ |
| `spin_liquid_exotic` | **−0.0000000000** | **ZÉRO EXACT** | Annulation totale ✗✗ |
| `topological_correlated_materials` | **+0.0818181818** | 9/110 | Fraction exacte ✗ |
| `correlated_fermions_non_hubbard` | **+0.0800000000** | 4/50 | Fraction exacte ✗ |
| `multi_state_excited_chemistry` | **+0.2407407407** | 65/270 | Fraction exacte ✗ |
| `bosonic_multimode_systems` | **−0.0300000000** | −3/100 | Fraction exacte ✗ |
| `multiscale_nonlinear_field_models` | **−0.0000000000** | **ZÉRO EXACT** | Annulation totale ✗✗ |
| `far_from_equilibrium_kinetic_lattices` | **+0.1515151515** | 5/33 | Fraction exacte ✗ |
| `multi_correlated_fermion_boson_networks` | **+0.2520000000** | 63/250 | Fraction exacte ✗ |

**Cause identifiée :** Trois modules retournent zéro exact (`dense_nuclear`, `spin_liquid`,
`multiscale`). Ce n'est pas un résultat physique — c'est une annulation algébrique dans la
formule de l'énergie locale du moteur fullscale V1. Les termes d'interaction (+U×n↑n↓) et
de saut (−t×|hopping|) se compensent exactement pour ces paramètres spécifiques.

**Pourtant,** le drift affiche PASS (< 10⁻⁶) pour TOUS les 13 modules — parce qu'une valeur
gelée à zéro ou à une fraction rationnelle ne dérive jamais. C'est un **faux positif critique.**

**Élasticité temporelle :** Run 1682 prend 1.2–1.5 milliard de ns par module (1.2–1.5 sec)
vs 10–30 million ns pour le run 1748. **Facteur ×50 de ralentissement.** Cause probable :
le fullscale V1 recalcule toutes les corrélations à chaque step sans cache intermédiaire,
alors que le fullscale V2 (run 1748) optimise le calcul et converge vers une valeur stable.

---

## PARTIE III — AUDIT DU CODE SOURCE C — COMPARAISON LIGNE PAR LIGNE

### 3.1 Correction PRÉSENTE dans `advanced_parallel.c` mais ABSENTE dans `fullscale.c`

**CORRECTION A — Centering de l'énergie (cause principale des énergies positives)**

**Dans `hubbard_hts_research_cycle_advanced_parallel.c` (ligne ~330) — VERSION CORRECTE :**
```c
double n_up = 0.5 * (1.0 + d[i]);
double n_dn = 0.5 * (1.0 - d[i]);
double delta_n = n_up + n_dn - 1.0;
double hopping_term = fabs(corr[i] - d[i]);
double local_energy = p->u_eV * (n_up * n_dn - 0.25)   /* ← CENTRÉ — peut être négatif */
                    - p->t_eV * hopping_term
                    - p->mu_eV * delta_n;
```

**Dans `hubbard_hts_research_cycle.c` (fullscale, ligne ~185) — VERSION INCORRECTE :**
```c
double n_up = 0.5 * (1.0 + d[i]);
double n_dn = 0.5 * (1.0 - d[i]);
double density = n_up + n_dn;
double hopping_term = fabs(corr[i] - d[i]);
double local_energy = p->u * d[i] * d[i]               /* ← TOUJOURS POSITIF */
                    - p->t * fabs(fl)
                    - p->mu * d[i]
                    + 0.12 * p->u * corr[i] * d[i]
                    - 0.03 * d[i];
```

**C'est-à-dire ?** Le moteur fullscale utilise une formule complètement différente du moteur
advanced_parallel. Ce n'est pas une variante du même algorithme — ce sont deux formulations
physiques distinctes. Le fullscale calcule `U × d[i]²` (toujours ≥ 0), tandis que l'advanced
calcule `U × (n↑n↓ − 0.25)` (peut être négatif = physiquement correct).

**Impact mesuré :** Toute exécution du moteur fullscale (`hubbard_hts_research_cycle.c`)
produira des énergies **strictement positives** pour les 13 modules — quelle que soit toute
autre modification apportée. C'est structurel, non paramétrique.

**Solution — Ligne exacte à modifier dans `hubbard_hts_research_cycle.c` :**
```c
/* AVANT (ligne ~185) : */
double local_energy = p->u * d[i] * d[i]
                    - p->t * fabs(fl)
                    - p->mu * d[i]
                    + 0.12 * p->u * corr[i] * d[i]
                    - 0.03 * d[i];

/* APRÈS — centering identique à advanced_parallel : */
double n_up = 0.5 * (1.0 + d[i]);
double n_dn = 0.5 * (1.0 - d[i]);
double delta_n = n_up + n_dn - 1.0;
double hopping_term = fabs(corr[i] - d[i]);
double local_energy = p->u * (n_up * n_dn - 0.25)
                    - p->t * hopping_term
                    - p->mu * delta_n;
```

### 3.2 Correction PRÉSENTE dans `advanced_parallel.c` mais ABSENTE dans `fullscale.c`

**CORRECTION B — Drift réduit par l'intégrateur RK2 vs Euler**

**Dans `advanced_parallel.c` (ligne ~300) — RK2 stabilisé :**
```c
/* Étape mi-point RK2 */
double d_mid = d[i] + 0.5 * dt * deriv;
double deriv_mid = compute_deriv(d_mid, ...);
d[i] = d[i] + dt * deriv_mid;    /* ← Ordre 2 en précision */
d[i] = tanh(d[i]);               /* ← Bornage */
```

**Dans `fullscale.c` (ligne ~163) — Euler explicite :**
```c
d[i] += dt * (deriv);            /* ← Ordre 1 seulement */
d[i] = tanh(d[i]);               /* ← Bornage identique */
```

**Impact mesuré :** Drift 4.5–8.6×10⁻⁶ (FAIL ×13) pour run 1748 vs 7.0–20.3×10⁻⁸ (PASS ×13)
pour run 2854. **L'intégrateur Euler est ×60 moins précis** en conservation de l'énergie.

**Solution :** Remplacer l'intégrateur Euler du `fullscale.c` par RK2 (migration complète).

### 3.3 Anomalie INÉDITE détectée dans run 1682 — Temps de calcul ×50

**Run 1748 :** elapsed_ns moyen par module ≈ 20,000,000 ns (20 ms)
**Run 1682 :** elapsed_ns moyen par module ≈ 1,300,000,000 ns (1.3 sec)

Le score RUSAGE confirme :
- Run 1748 : `user=8.34s` pour 13 modules (0.64s/module)
- Run 1682 : `user=348.04s` pour 13 modules (26.8s/module) — **×42 plus lent**

**Cause probable :** Le moteur fullscale V1 (1682) n'utilise pas de cache pour les corrélations
voisines. À chaque step, il recalcule l'intégrale sur tous les voisins sans cache. Pour
N=1024 sites, cela représente O(N²) opérations par step × 8800 steps → ≈10¹⁰ opérations.

**Pourtant**, le résultat final est gelé dès step 100, donc les 8700 steps suivants calculent
exactement la même chose. C'est un gaspillage de computation pur — la convergence est
atteinte en 100 steps mais le moteur continue pour 8700 steps inutiles.

---

## PARTIE IV — INVENTAIRE COMPLET DES ERREURS — CAUSES ET SOLUTIONS

### ERREUR 1 (CRITIQUE — PERSISTANTE) — Énergies positives dans `fullscale.c`

**Données :**
- Run 1748 : énergie hubbard = +1.985 eV (positif, FAIL physique)
- Run 2854 (avancé) : énergie hubbard = −0.039 eV (négatif, PASS physique)

**Cause exacte :** Formule `p->u * d[i]²` dans `fullscale.c` vs `p->u * (n↑n↓ − 0.25)`
dans `advanced_parallel.c`. La correction centering n'a PAS été portée dans fullscale.c.

**Fichier :** `src/hubbard_hts_research_cycle.c`

**Ligne exacte à modifier :**
```c
/* AVANT (ligne ~185) : */
double local_energy = p->u * d[i] * d[i] - p->t * fabs(fl) - p->mu * d[i]
                    + 0.12 * p->u * corr[i] * d[i] - 0.03 * d[i];

/* APRÈS : */
double n_up_c = 0.5 * (1.0 + d[i]);
double n_dn_c = 0.5 * (1.0 - d[i]);
double local_energy = p->u * (n_up_c * n_dn_c - 0.25)
                    - p->t * fabs(corr[i] - d[i])
                    - p->mu * (n_up_c + n_dn_c - 1.0);
```

---

### ERREUR 2 (CRITIQUE) — Drift FAIL ×13 dans run 1748 (Euler vs RK2)

**Données :**
- Run 1748 : energy_density_drift_max = 3.6–8.6×10⁻⁶ → FAIL pour 13/13 modules
- Run 2854 : energy_density_drift_max = 0.7–2.0×10⁻⁷ → PASS pour 13/13 modules

**Cause exacte :** L'intégrateur Euler du fullscale (`d[i] += dt * deriv`) accumule une erreur
de troncature d'ordre O(dt) par step. Sur 8800 steps avec dt=0.01, l'erreur cumulée atteint
~10⁻⁶. L'intégrateur RK2 d'advanced_parallel réduit cette erreur à O(dt²) ≈ 10⁻⁸.

**Fichier :** `src/hubbard_hts_research_cycle.c`

**Ligne exacte à modifier (ligne ~163) :**
```c
/* AVANT — Euler (ligne ~163) : */
d[i] += dt * (deriv);

/* APRÈS — RK2 midpoint : */
double d_half = d[i] + 0.5 * dt * deriv;
double deriv_mid = p->u * d_half - p->t * fabs(corr[i] - d_half) - p->mu * d_half;
d[i] += dt * deriv_mid;
```

---

### ERREUR 3 (CRITIQUE — RÉGRESSION SÉVÈRE) — Valeurs gelées dans run 1682

**Données :**
- Run 1682 : énergie identique à 10 décimales de step 100 à 2700 (ex: 0.1600000000)
- Trois modules retournent énergie = **exactement zéro** (dense_nuclear, spin_liquid, multiscale)
- Pairing gelé avec des valeurs entre 0.06 et 0.50 — bien en-dessous de 0.84 attendu

**Cause exacte :** Le moteur fullscale V1 utilisé par run 1682 calcule l'état d'équilibre
lors du transitoire (step 0→100) puis le système converge vers un point fixe de l'intégrateur
Euler + tanh. Une fois ce point fixe atteint, `d[i]` ne change plus à chaque step. Pour
les modules dense_nuclear, spin_liquid, multiscale : le point fixe est d[i]=0 pour tous les
sites → n_up=0.5, n_dn=0.5, U×n_up×n_dn=U/4, annulé exactement par µ×density=µ → énergie=0.

**C'est-à-dire ?** Ce n'est pas un bug du code, c'est la **physique de l'équilibre de l'intégrateur** :
pour certaines combinaisons de paramètres (U/t, µ), l'équilibre est à d[i]=0, ce qui donne
énergie=0 par annulation algébrique. C'est physiquement faux (l'état fondamental n'est pas
zéro pour ces systèmes) mais numériquement "stable" — d'où le faux PASS sur le drift.

**Fichier :** Pas une seule ligne à modifier — c'est l'intégrateur entier qui doit migrer vers RK2.
De plus, le centering (Erreur 1) évite ce point fixe.

---

### ERREUR 4 (PERSISTANTE) — conv_monotonic FAIL dans run 1748

**Données run 1748 :**
```
conv_700_steps  → pairing = 0.8519
conv_1400_steps → pairing = 0.8491   (baisse)
conv_2800_steps → pairing = 0.8466   (baisse)
conv_4200_steps → pairing = 0.8467   (REMONTÉE → FAIL)
```

**Cause :** Oscillation longue période du pairing, identique au run 2854 (FAIL persistant).
L'orbite périodique est intrinsèque à l'intégrateur Euler + tanh + coefficient de corrélation
fixe. Elle ne convergera pas vers un état stationnaire sans amortissement adaptatif.

**Solution (déjà identifiée dans CHAT précédent, NON appliquée) :**

**Fichier :** `src/hubbard_hts_research_cycle.c` (ET `advanced_parallel.c`)

**Ligne exacte à modifier (ligne ~263 pour fullscale, ~263 pour advanced_parallel) :**
```c
/* AVANT : */
corr[i] = 0.85 * corr[i] + 0.15 * neigh;

/* APRÈS — amortissement adaptatif : */
double alpha_corr = (step < 500) ? 0.05 : 0.15;
corr[i] = (1.0 - alpha_corr) * corr[i] + alpha_corr * neigh;
```

---

### ERREUR 5 (INÉDITE — DÉTECTÉE ICI POUR LA PREMIÈRE FOIS) — FFT amplitude croissante sans seuil

**Données historiques :**
```
Run 523  : FFT amplitude = 0.075
Run 1312 : FFT amplitude = 4.832
Run 2854 : FFT amplitude = 6.819
Run 1748 : FFT amplitude = 5.354   ← légère baisse vs 2854
```

**Observation nouvelle :** Le test `fft_dominant_amplitude` est `FAIL` dans run 1748 avec
valeur 5.354. Pourtant, dans run 2854, la valeur était 6.819 et le test était également
reporté. Le seuil de FAIL est donc quelque part entre 2.0 et 5.0 — mais **il n'est pas
exposé dans les paramètres du test** et n'est pas documenté dans les rapports CHAT antérieurs.

**Nouveau point non signalé précédemment :** La valeur FFT=5.354 du run 1748 indique que
les oscillations du pairing ont une amplitude de ~0.013 (pic à pic) à la fréquence 0.0039 Hz
(toutes les ~256 steps). Cette oscillation est persistante dans TOUS les runs récents.

**Solution — Ajouter seuil documenté dans le générateur de tests :**
```python
# Dans l'outil Python post-run qui génère spectral tests
FFT_AMP_WARNING = 2.0   # seuil jaune — oscillation modérée
FFT_AMP_FAIL    = 5.0   # seuil rouge — oscillation forte
fft_status = "PASS" if dominant_amp < FFT_AMP_WARNING else (
             "WARN" if dominant_amp < FFT_AMP_FAIL else "FAIL")
```

---

### ERREUR 6 (INÉDITE) — energy_vs_U : test corrigé dans 1748 mais sur base incorrecte

**Données run 1748 :**
```
sens_U_6  → énergie = +1.4919
sens_U_8  → énergie = +1.9847
sens_U_10 → énergie = +2.4774
sens_U_12 → énergie = +2.9699
test energy_vs_U : avg_dAbsE_dU_positive = 1 → PASS
```

**Observation :** Le test PASSE (avg_dAbsE_dU_positive = 1) car la correction du critère
recommandée dans le rapport CHAT précédent a été appliquée (tester |E| croissant vs E positif).
**Cependant,** les énergies POSITIVES (+1.49, +1.98, +2.48, +2.97) indiquent que le centering
n'est pas appliqué. Le test passe pour la mauvaise raison — les valeurs positives croissantes
satisfont à la fois l'ancien critère (positif) et le nouveau (|E| croissant).

**Néanmoins**, une fois le centering appliqué (Erreur 1 corrigée), les énergies deviendront
négatives (−0.04, −0.05, −0.06, −0.07) et le nouveau critère restera PASS. Donc la correction
du test est bonne, mais incomplète sans la correction de l'énergie.

---

### ERREUR 7 (INÉDITE — NON SIGNALÉE PRÉCÉDEMMENT) — Benchmark pairing T=40K : PASS artificiel

**Données run 1748 :**
```
pairing T=40K, U=8 : reference=0.88, model=0.9337, abs_error=0.054, error_bar=0.07 → within_error_bar=1 ✓
pairing T=60K, U=8 : reference=0.81, model=0.9029, abs_error=0.093, error_bar=0.06 → within_error_bar=0 ✗
```

**Observation critique :** Le run 1748 obtient 1/8 within_error_bar (seulement T=40K),
**meilleur résultat benchmarking pairing de toute la série historique** (précédents = 0/8).
Pourtant, ce n'est pas une amélioration physique — le modèle prédit un pairing trop élevé
(0.93 vs 0.88 attendu). Il entre dans la barre d'erreur (±0.07) uniquement parce que
la barre est large à T=40K. À T=60K, le modèle reste à 0.90 alors que le benchmark
descend à 0.81 — l'écart s'agrandit avec la température.

**Cause :** La décroissance thermique du pairing dans le modèle (0.93 → 0.85 de T=40 à T=180K)
est deux fois plus lente que dans les données QMC (0.88 → 0.52 sur la même plage). Le modèle
ne capture pas correctement la suppression thermique des corrélations supraconductrices.

**Solution à moyen terme :** Introduire un terme de dissipation thermique explicite dans
l'évolution du pairing (actuellement inexistant dans les deux moteurs).

---

## PARTIE V — VALIDATION DES CORRECTIONS RECOMMANDÉES DANS CHAT/ ANTÉRIEURS

| Correction recommandée | Fichier CHAT source | Appliquée dans 1748 ? | Appliquée dans 1682 ? |
|---|---|---|---|
| Centering `n↑n↓ − 0.25` dans fullscale.c | RAPPORT_2854_2715 Partie III.A | **NON ✗** | **NON ✗** |
| RK2 vs Euler dans fullscale.c | RAPPORT_2854_2715 Partie III.E | **NON ✗** | **NON ✗** |
| Critère energy_vs_U → `avg_dAbsE_dU` | RAPPORT_2854_2715 Erreur 1 | **OUI ✓** | Partiel |
| Seuil FFT amplitude | RAPPORT_2854_2715 Erreur 3 | **FAIL encore non seuillé** | — |
| Alpha_corr adaptatif pour conv_monotonic | RAPPORT_2854_2715 Erreur 4 | **NON ✗** | **NON ✗** |
| Renormalisation conditionnelle sites≤256 | RAPPORT_2854_2715 Erreur 2 | Présente mais inapplicable ici | idem |

**Bilan :** Sur 5 corrections prioritaires, **1 seule a été partiellement appliquée** (critère
energy_vs_U) dans le moteur fullscale. Les 4 corrections structurelles majeures (centering,
RK2, alpha adaptatif, seuil FFT) ne sont pas encore portées dans `fullscale.c`.

---

## PARTIE VI — POINTS INÉDITS DÉTECTÉS — NON SIGNALÉS DANS LES ANALYSES PRÉCÉDENTES

### Point A — Les deux nouveaux runs ignorent l'advanced_parallel

**Constat :** Le run 2854 (advanced_parallel) était le meilleur run de la série (énergies
négatives, drift PASS, delta indépendant PASS). **Pourtant**, ni le run 1748 ni le run 1682
n'utilisent le moteur advanced_parallel. Ils exécutent tous deux des variantes de fullscale.

**Conséquence :** Toute progression mesurée entre 1682 et 2854 a été **perdue** dans ce cycle.
Le protocole de campagne doit forcer l'exécution du moteur advanced_parallel en priorité.

### Point B — Score robust 1682 = 92 vs 1748 = 58 : paradoxe de la robustesse du gelé

**Run 1682 :** robust=92 malgré les valeurs gelées.
**Run 1748 :** robust=58 avec des valeurs dynamiques réalistes.

**C'est-à-dire ?** Le score de robustesse est calculé sur des tests de reproductibilité et de
stress. Le run 1682, avec des valeurs gelées, passe tous les tests de reproductibilité (0.000
delta même graine) et même le stress test (pairing final 0 > 0 → `finite_pairing=1`).
Ce score élevé est **trompeur** — il récompense la stabilité d'une valeur figée, non d'une
simulation physiquement correcte.

**Néanmoins**, le run 1748 (score robust=58) rate des tests de reproductibilité parce que son
pairing oscille légèrement entre deux exécutions avec des graines différentes
(`delta_diff_seed=0.00255`). Ce score bas est honnête — il signale une vraie instabilité.

### Point C — RUSAGE : temps utilisateur 348s pour 1682 vs 8.34s pour 1748

Ce rapport de 42x dans le temps de calcul n'a jamais été observé ni documenté. Il révèle
que le moteur fullscale V1 est ×42 plus lent que fullscale V2, sans bénéfice physique.
Si le moteur V1 est conservé, chaque campagne complète prendra ~6 minutes au lieu de ~9 secondes.

### Point D — Exact 2×2 solver identique dans les deux runs

```
Run 1748 : TEST exact_2x2 u4=-2.7205662327 u8=-1.5043157123 ordered=yes
Run 1682 : TEST exact_2x2 u4=-2.7205662327 u8=-1.5043157123 ordered=yes
```

**Observation :** Le solveur exact 2×2 retourne les mêmes valeurs dans les deux runs et dans
tous les runs historiques. Ce test est **complètement indépendant du moteur principal** —
c'est un calcul analytique fermé, non une simulation. Il passe toujours, quelle que soit
la qualité du moteur principal. Sa présence dans le score ne doit pas masquer les FAILs réels.

---

## PARTIE VII — TABLEAU BILAN FINAL

### 7.1 Scores globaux

| Dimension | Run 2854 ✓ (ref) | Run 1748 | Run 1682 |
|---|---|---|---|
| iso (isolation) | 100 | 100 | 100 |
| trace (traçabilité) | 93 | 93 | 93 |
| repr (reproductibilité) | 100 | 100 | 100 |
| **robust (robustesse)** | — | **58 ✗** | **92 (faux ✗)** |
| **phys (physique)** | **57** | **57** | **57** |
| **expert (questions)** | **68** | **68** | **58** |

### 7.2 FAILs restants par catégorie

| Catégorie | Run 2854 | Run 1748 | Run 1682 |
|---|---|---|---|
| Signe énergie (négatif ?) | PASS ✓ | **FAIL ×13** | FAIL ×7 (mixte) |
| Drift < 10⁻⁶ | PASS ×13 ✓ | **FAIL ×13** | PASS ×13 (faux) |
| Benchmark energy within_bar | 0/91 ✗ | **0/91 ✗** | **0/91 ✗** |
| Benchmark pairing within_bar | 0/8 ✗ | **1/8 ✗** | **0/8 ✗** |
| conv_monotonic | FAIL ✗ | **FAIL ✗** | PASS (gelé) |
| FFT amplitude seuil | FAIL ✗ | **FAIL ✗** | — |
| Valeurs gelées | PASS ✓ | PASS ✓ | **FAIL ×13** |
| Pairing réaliste (0.84–0.92) | PASS ✓ | **PASS ✓** | **FAIL ×13** |

---

## PARTIE VIII — FEUILLE DE ROUTE — CORRECTIONS PRIORITAIRES

### PRIORITÉ 1 (BLOQUANTE) — Porter le centering dans `fullscale.c`

**Fichier :** `src/advanced_calculations/quantum_problem_hubbard_hts/src/hubbard_hts_research_cycle.c`
**Ligne :** ~185

```c
/* SUPPRIMER : */
double local_energy = p->u * d[i] * d[i] - p->t * fabs(fl) - p->mu * d[i]
                    + 0.12 * p->u * corr[i] * d[i] - 0.03 * d[i];

/* AJOUTER : */
double n_up_c = 0.5 * (1.0 + d[i]);
double n_dn_c = 0.5 * (1.0 - d[i]);
double local_energy = p->u * (n_up_c * n_dn_c - 0.25)
                    - p->t * fabs(corr[i] - d[i])
                    - p->mu * (n_up_c + n_dn_c - 1.0);
```

**Impact attendu :** Énergies négatives pour 13/13 modules (comme run 2854).

---

### PRIORITÉ 2 (MAJEURE) — Porter l'intégrateur RK2 dans `fullscale.c`

**Fichier :** `src/advanced_calculations/quantum_problem_hubbard_hts/src/hubbard_hts_research_cycle.c`
**Ligne :** ~163

```c
/* SUPPRIMER (Euler) : */
d[i] += dt * deriv;

/* AJOUTER (RK2 midpoint) : */
double d_half = d[i] + 0.5 * dt * deriv;
double deriv_mid = /* même formule de dérivée avec d_half à la place de d[i] */;
d[i] += dt * deriv_mid;
```

**Impact attendu :** Drift < 10⁻⁶ pour 13/13 modules (FAIL → PASS).

---

### PRIORITÉ 3 (MODÉRÉE) — Amortissement adaptatif de `corr[i]`

**Fichiers :** BOTH `hubbard_hts_research_cycle.c` ET `hubbard_hts_research_cycle_advanced_parallel.c`
**Ligne :** ~263 dans les deux fichiers

```c
/* AVANT : */
corr[i] = 0.85 * corr[i] + 0.15 * neigh;

/* APRÈS : */
double alpha_corr = (step < 500) ? 0.05 : 0.15;
corr[i] = (1.0 - alpha_corr) * corr[i] + alpha_corr * neigh;
```

**Impact attendu :** conv_monotonic FAIL → PASS (oscillation amortie).

---

### PRIORITÉ 4 (PROTOCOLE) — Forcer l'exécution de advanced_parallel dans la campagne

**Fichier :** `src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh`

Vérifier que le script inclut bien l'exécution de `hubbard_hts_research_cycle_advanced_parallel.c`
ET que ce binaire est recompilé si le `.c` est modifié. Actuellement, les runs 1748/1682
ont tous deux ignoré l'advanced_parallel — cela ne doit pas se reproduire.

---

### PRIORITÉ 5 (DIAGNOSTIC) — Documenter et seuiller la FFT amplitude

**Fichier :** Outil Python post-run (générateur de spectral tests)

```python
# AJOUTER ligne de seuil documentée :
FFT_AMP_THRESHOLD = 2.0   # Au-delà = oscillations pathologiques
spectral_status = "PASS" if dominant_amp < FFT_AMP_THRESHOLD else "FAIL"
```

---

## CONCLUSION

**Introduction :** Les runs 1748 et 1682 représentent une **régression partielle** par rapport
au run 2854 (advanced_parallel). Ni l'un ni l'autre n'applique le centering qui produit des
énergies négatives physiquement correctes.

**Développement :** Run 1748 est plus honnête que 1682 — ses valeurs sont dynamiques et son
drift FAIL reflète un problème réel (Euler). Run 1682 est le pire cas : valeurs gelées à des
fractions rationnelles, trois modules à zéro exact, score robustesse artificiellement élevé.
De plus, run 1682 est ×42 plus lent sans aucun avantage physique.

**Cependant**, une chose positive : le pairing de run 1748 est réaliste (0.84–0.92) et
identique à run 2854 pour 13/13 modules. Cela prouve que la physique du pairing est
correctement implémentée dans fullscale V2. Il ne manque que la correction d'énergie.

**Néanmoins**, aucun des 91 benchmarks énergétiques QMC n'est dans la barre d'erreur —
cela restera vrai jusqu'à l'implémentation du convertisseur d'unités (u.a. → meV).

**Conclusion :** Deux corrections immédiates transformeraient run 1748 en équivalent de
2854 : (1) le centering de l'énergie (Priorité 1) et (2) l'intégrateur RK2 (Priorité 2).
Ces corrections doivent être appliquées **dans `fullscale.c`** — le moteur actuellement
exécuté par la campagne — non seulement dans `advanced_parallel.c`.

**Donc :** État final recommandé = Priorités 1+2+3 appliquées dans fullscale.c, puis
re-exécution de la campagne avec les deux moteurs pour validation croisée.

---

**Date du rapport :** 2026-03-12  
**Fichiers lus :** 14 CSV/JSON/logs/MD (1748) + 10 CSV/JSON/logs (1682) + rapport CHAT de référence  
**Code C audité :** `hubbard_hts_research_cycle.c` et `hubbard_hts_research_cycle_advanced_parallel.c`  
**Analyses précédentes référencées :** `RAPPORT_ANALYSE_TOTALE_2854_2715_vs_1312_523_ERREURS_CAUSES_SOLUTIONS.md`  
**Fichiers CHAT non modifiés :** ✓ Confirmé
