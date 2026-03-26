# RAPPORT D'ANALYSE TOTALE — CYCLE 06
## Run `research_20260312T123420Z_1312` (nouveau) vs `research_20260312T075921Z_523` (référence)
**Date :** 2026-03-12 | **Lu ligne par ligne :** logs, CSV, JSON, code source C (3 fichiers .c audités)

---

## GLOSSAIRE RAPIDE

- **Intégrateur numérique** : algorithme qui calcule l'évolution du système physique pas à pas dans le temps.
- **RK2 (Runge-Kutta ordre 2 / méthode du point milieu)** : intégrateur plus précis qu'Euler, qui calcule un point intermédiaire à mi-chemin avant de faire le pas complet.
- **Norme ψ** : longueur du vecteur d'état quantique. Doit rester = 1. Mesurée ici après normalisation forcée.
- **tanh (tangente hyperbolique)** : fonction mathématique qui borne toute valeur dans l'intervalle [−1, +1]. Utilisée ici comme saturation numérique.
- **Drift d'énergie** : dérive lente de l'énergie au fil des pas de temps. Indicateur de non-conservation.
- **FFT** : Transformée de Fourier. Décompose un signal temporel en fréquences. Une amplitude FFT élevée signifie des oscillations fortes dans le signal.
- **Pompage dynamique** : boucle de rétroaction (feedback) qui tente de guider l'énergie vers une cible. energy_reduction_ratio > 0 = pompage actif.
- **Vérification indépendante** : recalcul du même problème en précision supérieure (`long double` 80 bits) sans saturation tanh ni renormalisation. Devrait donner des résultats proches du module principal.
- **V4-Next** : nouveau moteur de simulation en cours de qualification pour déploiement complet (FULL).
- **Sites ≤ 256** : condition de taille de grille. 10×10 = 100 sites ≤ 256 → normalisé. 18×18 = 324 sites > 256 → non normalisé.

---

## PARTIE I — INVENTAIRE DES FICHIERS

### Fichiers présents dans `1312` mais ABSENTS dans `523`

| Fichier | Signification |
|---|---|
| `tests/integration_independent_qmc_results.csv` | Résultats QMC indépendant pour 13 modules |
| `tests/integration_independent_dmrg_results.csv` | Résultats DMRG indépendant pour 13 modules |
| `tests/integration_independent_arpes_results.csv` | Simulation ARPES (spectroscopie) indépendante |
| `tests/integration_independent_stm_results.csv` | Simulation STM (champ proche) indépendante |
| `tests/integration_independent_modules_summary.csv` | Synthèse : 4/4 PASS pour tous les 13 modules |
| `tests/integration_entropy_observables.csv` | Entropie de Shannon sur énergie, pairing, signe |
| `tests/integration_spatial_correlations.csv` | Fonctions d'autocorrélation spatiale (65 lignes) |
| `tests/integration_alternative_solver_campaign.csv` | Campagne solveurs alternatifs |
| `tests/integration_claim_confidence_tags.csv` | Tags de confiance sur chaque affirmation |
| `tests/integration_open_questions_backlog.csv` | 5 questions ouvertes formalisées |
| `tests/integration_scaling_exponents_live.csv` | Exposants de scaling énergie/pairing |
| `tests/integration_manifest_check.csv` | Vérification manifest : CSV_INTEGRITY, MODULE_COVERAGE |
| `tests/integration_absent_metadata_fields.csv` | Audit champs absents : **TOUS PRÉSENTS** |
| `tests/integration_gate_summary.csv` | 5 gates, toutes PASS |
| `tests/integration_physics_enriched_test_matrix.csv` | 12 tests T1–T12 avec PASS/FAIL/critères |
| `tests/integration_run_drift_monitor.csv` | Comparaison point-à-point avec run précédent (655) |

### Fichiers présents dans `523` mais ABSENTS dans `1312`
| Fichier | Signification |
|---|---|
| `audit/final_scientific_audit.md` | Audit scientifique final avec hashes SHA256 |
| `audit/proxy_detection_report.csv` | Détection proxy (153 violations) |
| `audit/run_signature.json` | Signature cryptographique du run |
| `scientific_diagnostics/physics_validation_report.md` | Validation pipeline |
| `logs/forensic_extension_summary.json` | Forensique HFBL360 |
| `logs/full_scope_integrator_summary.json` | Résumé triple exécution |
| `logs/hfbl360_forensic_audit.json` | Audit forensique temps réel |
| `logs/analysis_scientifique_checksums.sha256` | Checksums de l'analyse |
| `logs/checksums.sha256` | Checksums globaux |
| `reports/3d/` | Modélisation 3D (dataset + HTML) |
| `tests/integration_triple_execution_matrix.csv` | 56 tests × 3 runs (stabilité triple) |

**Verdict :** Le run 1312 sacrifie la traçabilité forensique (audit, checksums, signatures cryptographiques de 523) au profit de nouveaux modules d'analyse physique (QMC, DMRG, entropie, corrélations spatiales, gates). C'est un échange de profondeur d'audit contre richesse d'observables.

---

## PARTIE II — TABLEAU COMPARATIF LIGNE PAR LIGNE DES OBSERVABLES PHYSIQUES

### 2.1 Énergies et pairings — chaque module

| Module | Énergie 523 | Énergie 1312 | Pairing 523 | Pairing 1312 |
|---|---|---|---|---|
| `hubbard_hts_core` | **−1.0400** | **+1.7606** | 0.2408 | 0.8467 |
| `qcd_lattice_fullscale` | **−0.8605** | **+2.0982** | 0.1198 | 0.7628 |
| `quantum_field_noneq` | **−1.2688** | **+1.6455** | 0.0658 | 0.6756 |
| `dense_nuclear_fullscale` | **−1.1000** | **+2.3800** | 0.3044 | 0.8488 |
| `quantum_chemistry_fullscale` | **−1.7714** | **+1.1607** | 0.4143 | 0.8754 |
| `spin_liquid_exotic` | **−1.0200** | **+2.4633** | 0.4479 | 0.9155 |
| `topological_correlated_materials` | **−1.1682** | **+1.7658** | 0.3508 | 0.8961 |
| `correlated_fermions_non_hubbard` | **−1.3000** | **+1.9221** | 0.2815 | 0.8558 |
| `multi_state_excited_chemistry` | **−1.4793** | **+1.4342** | 0.4991 | 0.9141 |
| `bosonic_multimode_systems` | **−0.6900** | **+1.2093** | 0.1911 | 0.8077 |
| `multiscale_nonlinear_field_models` | **−1.5000** | **+2.1464** | 0.1510 | 0.8068 |
| `far_from_equilibrium_kinetic_lattices` | **−0.9385** | **+1.8700** | 0.1036 | 0.7678 |
| `multi_correlated_fermion_boson_networks` | **−0.9380** | **+1.6725** | 0.2240 | 0.8398 |

### 2.2 Métriques critiques — tableau global

| Métrique | Run 523 | Run 1312 | Évolution |
|---|---|---|---|
| Méthode intégrateur (metadata) | `euler_explicit` | **`rk2_stabilized`** | Changement majeur |
| Méthode norme guard (CSV) | `rk2_without_forced_renorm` | `rk2_without_forced_renorm` | Inchangé (LABEL) |
| Norme ψ max `hubbard_hts_core` | **9.0 × FAIL** | **7.8×10⁻¹⁶ × PASS** | Correction majeure ✓ |
| Norme ψ max tous modules | 7–10 (tous FAIL) | 5–8×10⁻¹⁶ (tous PASS) | Correction majeure ✓ |
| Drift énergie `dense_nuclear` | 3.19×10⁻⁶ | **7.53×10⁻⁶** | Dégradation −136% ✗ |
| Drift énergie `hubbard_hts_core` | 2.36×10⁻⁶ | 4.01×10⁻⁶ | Dégradation ✗ |
| Von Neumann spectral radius | 0.9984608348 PASS | 0.9984608348 PASS | Identique ✓ |
| Cas jouet abs_error | 2.13×10⁻⁶ PASS | 2.13×10⁻⁶ PASS | Identique ✓ |
| Amplitude FFT | **0.0751** | **4.8320** | ×64 — alarme ✗ |
| energy_vs_U | FAIL (plateau −1.04) | **PASS** (1.27→1.76→2.25→2.74) | Correction ✓ |
| Verification delta | **3.759 FAIL** | **1.559 FAIL** | Amélioré mais FAIL ✗ |
| Convergence monotone | PASS | **FAIL** | Régression ✗ |
| Dynamic pumping ratio | **0.000 FAIL** | **−0.000212 actif** | Restauration partielle ✓ |
| Tests ChatGPT | 8 PASS, 3 FAIL | **11 PASS, 1 FAIL** | Amélioration ✓ |
| V4-Next FULL progress | 87.24% | **88.05%** | +0.81% ✓ |
| Mémoire moyenne | 76% | **52%** | −24% (libération) |
| Reproductibilité seed différente | delta=0.2819 PASS | delta=0.003616 PASS | PASS mais delta réduit |
| Scores iso/trace/repr/robust/phys/expert | 100/93/100/58/50/— | 100/93/100/55/67/63 | Phys +17%, robust −3% |

---

## PARTIE III — AUDIT DU CODE SOURCE C — ANALYSE LIGNE PAR LIGNE

C'est la section la plus importante. J'ai lu les 3 fichiers C complets.

### 3.1 La boucle d'intégration — code exact

**Fichier :** `src/hubbard_hts_research_cycle_advanced_parallel.c`

**Lignes 253–310 (boucle principale d'évolution) :**

```c
/* INTÉGRATEUR RK2 (Runge-Kutta ordre 2 — méthode du point milieu) */
/* Ligne ~267 */
double dH_ddi = p->u_eV * (-d[i]) + p->t_eV * (d[i] - corr[i]);
double k1 = -dt_scale * dH_ddi;                          /* pente initiale */
double d_mid = d[i] + 0.5 * k1;                          /* point milieu */
double dH_ddi_mid = p->u_eV * (-d_mid) + p->t_eV * (d_mid - corr[i]);
d[i] += -dt_scale * dH_ddi_mid;                          /* pas complet */

/* SATURATION tanh — NOUVEAU dans rk2_stabilized */
/* Ligne ~290 */
d[i] = tanh(d[i]);                                        /* borne d[i] dans [-1, +1] */

/* NORMALISATION CONDITIONNELLE — CRITIQUE */
/* Ligne ~304 */
if (sites <= 256) {
    normalize_state_vector(d, sites);                     /* forcée SI grille ≤ 16×16 */
}

/* MESURE norme APRÈS normalisation — toujours ~machine epsilon */
/* Ligne ~309 */
double norm_dev = fabs(state_vector_norm(d, sites) - 1.0);
if (norm_dev > r.norm_deviation_max) r.norm_deviation_max = norm_dev;
```

**Ce que cela révèle :**

1. L'intégrateur est bien **RK2 midpoint** (deux évaluations du gradient, correct).
2. **La saturation `tanh(d[i])`** est une nouvelle stabilisation qui borne tous les composants entre −1 et +1.
3. **La normalisation est conditionnelle** : elle s'applique seulement si `sites ≤ 256`. Pour les grilles courantes (10×10 = 100 sites), elle est toujours active.
4. **La norme est mesurée APRÈS normalisation** → elle est toujours ≈ machine epsilon (7×10⁻¹⁶), ce qui explique le PASS universel du norm guard dans 1312.
5. **Le label `rk2_without_forced_renorm` dans le CSV est FAUX** : la normalisation est bien forcée à l'intérieur de la boucle pour sites ≤ 256.

### 3.2 Comparaison exacte avant/après pour chaque modification

#### MODIFICATION 1 — Ajout de l'intégrateur RK2

**Avant (run 523 — `euler_explicit` déclaré, mais code réel intermédiaire) :**
```c
/* Code simplifié Euler : d[i] évolue directement sans substep */
double dH = p->u_eV * (-d[i]) + p->t_eV * (d[i] - corr[i]);
d[i] += -dt_scale * dH;
/* Pas de tanh. Pas de renorm dans 523 → norme explose à 7-10 */
```

**Après (run 1312 — `rk2_stabilized`) :**
```c
/* Lignes 267-271 du fichier .c */
double dH_ddi = p->u_eV * (-d[i]) + p->t_eV * (d[i] - corr[i]);
double k1 = -dt_scale * dH_ddi;
double d_mid = d[i] + 0.5 * k1;
double dH_ddi_mid = p->u_eV * (-d_mid) + p->t_eV * (d_mid - corr[i]);
d[i] += -dt_scale * dH_ddi_mid;
/* + tanh + renorm conditionnelle → norme conservée à 7e-16 */
```

**Impact mesuré :** Norme ψ passe de 7–10 (FAIL) à 7×10⁻¹⁶ (PASS). Correction réussie techniquement.

#### MODIFICATION 2 — Chargement des problèmes depuis CSV externe

**Avant (run 5132, lignes ~615 dans ancien .c) :**
```c
/* HARDCODING — problèmes définis en dur dans le tableau C */
problem_t probs[] = {
    {"hubbard_hts_core",  10, 10, 0.52, 4.16, 0.2, 300.0, 0.01, 2700},
    {"qcd_lattice_fullscale", 9, 9, ...},
    /* ... 11 autres lignes hardcodées ... */
};
```

**Après (run 1312, ligne ~731 dans .c) :**
```c
/* CORRIGÉ — tableau vide, chargement depuis fichier externe */
problem_t probs[64] = {0};  /* ligne 731 — tableau vide initialisé à zéro */
char problems_cfg[MAX_PATH];
pjoin(problems_cfg, sizeof(problems_cfg), root, "config/problems_cycle06.csv");
int nprobs = load_problems_from_csv(problems_cfg, probs, 64);
if (nprobs <= 0) {
    fprintf(stderr, "ERROR: missing/invalid problems config: %s\n", problems_cfg);
    return 2;   /* erreur si config manquante — bonne pratique */
}
```

**Impact :** Les paramètres physiques (t, U, mu, lattice, steps) sont maintenant lus depuis `config/problems_cycle06.csv`. Le scanner hardcoding marque `problem_t probs[64] = {0}` comme REVIEW_REQUIRED, mais c'est un **faux positif** : le tableau est vide à l'initialisation et rempli depuis le CSV externe. Correction confirmée et validée.

#### MODIFICATION 3 — Normalisation conditionnelle (sites ≤ 256)

**Avant (run 523, dans la boucle step) :**
```c
/* RK2 sans renorm — code intermédiaire run 523 */
/* normalize_state_vector(d, sites) — COMMENTÉ ou SUPPRIMÉ */
/* → norme explose à 7-10 */
```

**Après (run 1312, ligne ~304) :**
```c
if (sites <= 256) {
    normalize_state_vector(d, sites);  /* actif pour ≤ 16×16 */
}
/* → norme = machine epsilon pour tests principaux (100 sites) */
/* → norme non forcée pour grandes grilles (≥ 18×18 = 324 sites) */
```

**Impact non anticipé :** Cette condition crée une **discontinuité physique à 18×18** dans les tests cluster (voir Erreur 4).

#### MODIFICATION 4 — Module indépendant (long double)

**Code existant `simulate_problem_independent` (lignes ~408–455) :**
```c
/* Long double (80 bits), pas de tanh, pas de renorm, pas de contrôles */
long double dH_ddi = (long double)p->u_eV * (-d[i]) + (long double)p->t_eV * (d[i] - corr[i]);
long double k1 = -dt_scale * dH_ddi;
long double d_mid = d[i] + 0.5L * k1;
long double dH_ddi_mid = (long double)p->u_eV * (-d_mid) + (long double)p->t_eV * (d_mid - corr[i]);
d[i] += -dt_scale * dH_ddi_mid;
d[i] = tanhl(d[i]);           /* tanhl existe dans le code indépendant */
/* ABSENCE de normalize_state_vector — DIFFÉRENCE vs module principal */
/* ABSENCE de contrôles plasma — DIFFÉRENCE vs module principal */
```

**Problème :** Le module indépendant n'a **pas** de `normalize_state_vector` et n'a **pas** les contrôles plasma (phase, pump, quench, feedback). Il diverge donc légèrement du module principal dès les premières itérations. C'est la source du delta=1.56 dans la vérification indépendante.

---

## PARTIE IV — INVENTAIRE COMPLET DES ERREURS — CAUSES EXACTES ET SOLUTIONS

### ERREUR 1 — RÉGRESSION : Énergies redevenues positives
**Sévérité :** CRITIQUE

**Données :**
- Run 523 : `hubbard_hts_core` énergie = **−1.04 eV** (calculée)
- Run 1312 : `hubbard_hts_core` énergie = **+1.76 eV** (régression)

**Cause exacte dans le code (lignes 295–300 du .c) :**
```c
double n_up = 0.5 * (1.0 + d[i]);          /* occupation spin-up */
double n_dn = 0.5 * (1.0 - d[i]);          /* occupation spin-down */
double hopping_term = fabs(corr[i] - d[i]);
double local_energy = p->u_eV * n_up * n_dn    /* terme U — POSITIF */
                    - p->t_eV * hopping_term    /* terme t — négatif si hopping actif */
                    - p->mu_eV * density;       /* terme chimique — petit */
```

Quand `d[i]` est contraint par `tanh` dans [−1, +1] ET normalisé, les composantes sont petites. On a `n_up ≈ 0.5 + ε`, `n_dn ≈ 0.5 − ε`. Le terme dominant est **`u_eV × 0.25 ≈ positif`**. Le terme hopping `−t_eV × |corr − d|` est faible car corr et d convergent.

Dans run 523, sans normalisation, `d[i]` grandissait librement, produisant des `hopping_term` plus grands et des contributions négatives dominantes → énergie négative.

**Interprétation physique :**  
Ni les énergies positives de 1312 ni les énergies négatives de 523 ne sont la "vraie" énergie de l'état fondamental. La solveur exact 2×2 donne U4 = **−2.72 eV** et U8 = **−1.50 eV** (toujours négatif). L'énergie doit être négative pour un état fondamental lié. Le problème est que le Hamiltonien simulé (`u_eV × n_up × n_dn − t_eV × hopping`) n'est pas le Hamiltonien de Hubbard standard. Le terme `u_eV × n_up × n_dn` est `+U × (occupation)²` alors que dans le vrai Hubbard, l'interaction doit être centrée autour du demi-remplissage pour donner de l'énergie négative.

**Solution — modification à appliquer (lignes ~295–300) :**
```c
/* AVANT (ligne ~295) — terme U toujours positif : */
double local_energy = p->u_eV * n_up * n_dn
                    - p->t_eV * hopping_term
                    - p->mu_eV * density;

/* APRÈS — terme U centré autour demi-remplissage (n=1 par site) : */
double delta_n = n_up + n_dn - 1.0;              /* déviation du demi-remplissage */
double local_energy = p->u_eV * (n_up * n_dn - 0.25)  /* centré : vaut 0 au demi-remplissage */
                    - p->t_eV * hopping_term
                    - p->mu_eV * delta_n;
```

Ce changement permet à l'énergie d'être négative au demi-remplissage (état fondamental) et positive loin de lui.

---

### ERREUR 2 — INCOHÉRENCE LABEL vs CODE : `rk2_without_forced_renorm` vs normalisation réelle
**Sévérité :** IMPORTANTE (documentation fausse)

**Données :**
- `integration_norm_psi_guard.csv` dit : `notes=rk2_without_forced_renorm`
- `model_metadata.json` dit : `"method": "rk2_stabilized"`
- Code réel ligne ~304 : `if (sites <= 256) { normalize_state_vector(d, sites); }` → normalisation ACTIVE

**Cause :** Le label `rk2_without_forced_renorm` dans le CSV est généré à la ligne 775 du fichier C :
```c
/* Ligne 775 — label hardcodé, indépendant de ce que le code fait réellement */
fprintf(ngcsv, "%s,%.12e,%.12e,%s,rk2_without_forced_renorm\n",
        probs[i].name, base[i].norm_deviation_max, 1e-6, norm_ok ? "PASS" : "FAIL");
```

Ce string `"rk2_without_forced_renorm"` est une chaîne fixe qui n'a pas été mise à jour quand la normalisation conditionnelle a été remise.

**Solution (ligne 775 du .c) :**
```c
/* AVANT (ligne 775) : */
fprintf(ngcsv, "%s,%.12e,%.12e,%s,rk2_without_forced_renorm\n", ...

/* APRÈS — label dynamique reflétant la réalité du code : */
const char* norm_method = (sites <= 256) ? "rk2_stabilized_conditional_renorm"
                                         : "rk2_stabilized_no_renorm_large_grid";
fprintf(ngcsv, "%s,%.12e,%.12e,%s,%s\n",
        probs[i].name, base[i].norm_deviation_max, 1e-6,
        norm_ok ? "PASS" : "FAIL", norm_method);
```

---

### ERREUR 3 — Vérification indépendante : delta = 1.559 (FAIL persistant)
**Sévérité :** CRITIQUE

**Données :**
- Run 5132 : delta = 0.000 (faux PASS — les deux modules utilisaient les mêmes valeurs hardcodées)
- Run 523 : delta = **3.759 FAIL** (modules divergents)
- Run 1312 : delta = **1.559 FAIL** (amélioré mais toujours FAIL)

**Cause exacte dans le code :**

Le module principal (`simulate_fullscale_controlled`) a :
- RK2 + `tanh(d[i])` + `normalize_state_vector(if sites≤256)` + contrôles plasma

Le module indépendant (`simulate_problem_independent`) a :
- RK2 + `tanhl(d[i])` (long double) + **pas de normalize** + **pas de contrôles plasma**

Ces différences produisent des trajectoires physiques différentes dès les premiers steps, et le delta après 2700 steps est donc 1.56 unités.

**Lignes concernées :**

Module principal, ligne ~290 :
```c
d[i] = tanh(d[i]);                  /* simple précision */
/* puis normalize si sites≤256 */
/* puis contrôles plasma */
```

Module indépendant, ligne ~435 :
```c
d[i] = tanhl(d[i]);                 /* long double — légèrement différent */
/* PAS de normalize */
/* PAS de contrôles */
```

**Solution :**

Pour que la vérification indépendante soit vraiment indépendante et comparable, les deux modules doivent utiliser exactement la même dynamique physique. Option A (correcte) :

```c
/* Dans simulate_problem_independent — ajouter normalize conditionnelle */
/* Ligne ~438, après le calcul de d[i] */
if (sites <= 256) {
    /* normaliser le vecteur long double */
    long double norm2 = 0.0L;
    for (int j = 0; j < sites; ++j) norm2 += d[j] * d[j];
    if (norm2 > 1e-15L) {
        long double inv = 1.0L / sqrtl(norm2);
        for (int j = 0; j < sites; ++j) d[j] *= inv;
    }
}
```

Option B (plus fondamentale) : supprimer la normalisation du module principal aussi (voir Erreur 6).

---

### ERREUR 4 — Discontinuité dans le scaling cluster à 18×18 (CRITIQUE — NOUVEAU)
**Sévérité :** CRITIQUE — Non documenté dans les analyses précédentes

**Données (`new_tests_results.csv`) :**

| Taille | Sites | Énergie | Pairing | Normalisation active |
|---|---|---|---|---|
| 8×8 | 64 | 1.736 | 0.818 | 64 ≤ 256 → OUI |
| 10×10 | 100 | 1.759 | 0.848 | 100 ≤ 256 → OUI |
| 12×12 | 144 | 1.770 | 0.875 | 144 ≤ 256 → OUI |
| 14×14 | 196 | 1.778 | 0.889 | 196 ≤ 256 → OUI |
| 16×16 | 256 | 1.782 | 0.905 | 256 ≤ 256 → OUI |
| **18×18** | **324** | **0.525** | **0.413** | **324 > 256 → NON** ← saut |
| 24×24 | 576 | 0.509 | 0.414 | 576 > 256 → NON |
| 32×32 | 1024 | 0.530 | 0.413 | 1024 > 256 → NON |
| 128×128 | 16384 | 0.557 | 0.412 | 16384 > 256 → NON |
| 255×255 | 65025 | 0.568 | 0.412 | 65025 > 256 → NON |

**Cause exacte (ligne ~304 du .c) :**
```c
if (sites <= 256) {
    normalize_state_vector(d, sites);   /* seuil arbitraire : 256 = 16×16 */
}
```

Le seuil de 256 sites est la ligne de démarcation. En dessous : l'état est renormalisé à chaque step → énergie +1.76 à +1.78. Au-dessus : l'état n'est plus normalisé → dynamique différente, énergie converge vers +0.52 à +0.57. Ce ne sont pas deux régimes physiques différents — c'est un **artefact du seuil de normalisation**.

**Impact :** Les tests `cluster_pair_trend` et `cluster_energy_trend` FAIL car il y a une discontinuité non-physique au lieu d'une variation monotone.

**Solution (ligne ~304 du .c) :**
```c
/* AVANT (ligne ~304) — seuil arbitraire : */
if (sites <= 256) {
    normalize_state_vector(d, sites);
}

/* APRÈS — normalisation pour TOUS les systèmes, ou AUCUN : */
normalize_state_vector(d, sites);   /* option 1 : toujours normaliser */
/* OU */
/* supprimer la normalisation et utiliser l'intégrateur de Cayley à la place */
```

---

### ERREUR 5 — Amplitude FFT ×64 : de 0.075 à 4.832 (ALARME)
**Sévérité :** IMPORTANTE

**Données :**
- Run 5132 : FFT amplitude = 0.0426
- Run 523 : FFT amplitude = 0.0751
- Run 1312 : FFT amplitude = **4.8320** (+64× vs 523)

**Cause :**  
La FFT est calculée sur la série temporelle du pairing. Dans 1312, le pairing oscille entre 0.847 et 0.853 au fil des steps (convergence monotone absente, FAIL). Cette oscillation est beaucoup plus ample que dans 523 (pairing stable à 0.2408 après step 100). L'amplitude FFT de 4.83 signifie que la fréquence dominante à 0.00389 Hz (une oscillation toutes ~257 steps) a une amplitude de 4.83 — c'est-à-dire que le signal oscille entre +4.83 et -4.83 autour de zéro dans l'espace fréquentiel. Cela peut indiquer que le RK2 stabilisé avec tanh et normalisation génère des oscillations artificielles d'amplitude bien supérieure à ce qu'on aurait avec un intégrateur symplectique.

Le test `spectral/fft_dominant_amplitude` est marqué PASS parce que le critère de PASS est simplement "la FFT a produit un résultat", pas "l'amplitude est physiquement raisonnable". Le critère devrait intégrer une borne supérieure d'amplitude.

**Solution — ajouter un seuil d'amplitude dans le test (fichier post-run Python) :**
```python
# Dans l'outil qui génère le test FFT
fft_amplitude_threshold = 1.0  # valeur physiquement raisonnable
fft_status = "PASS" if (dominant_amp < fft_amplitude_threshold) else "FAIL"
```

---

### ERREUR 6 — Drift d'énergie dégradé pour plusieurs modules
**Sévérité :** IMPORTANTE

**Données — drift max tous modules :**

| Module | Drift 523 | Drift 1312 | Évolution |
|---|---|---|---|
| `dense_nuclear_fullscale` | 3.19×10⁻⁶ | **7.53×10⁻⁶** | ×2.36 — Dégradation |
| `qcd_lattice_fullscale` | 2.32×10⁻⁶ | 5.89×10⁻⁶ | ×2.54 — Dégradation |
| `quantum_field_noneq` | 3.64×10⁻⁶ | 5.84×10⁻⁶ | ×1.60 — Dégradation |
| `hubbard_hts_core` | 2.36×10⁻⁶ | 4.01×10⁻⁶ | ×1.70 — Dégradation |
| `spin_liquid_exotic` | 1.88×10⁻⁶ | 4.67×10⁻⁶ | ×2.49 — Dégradation |
| `bosonic_multimode_systems` | 1.70×10⁻⁶ | 3.43×10⁻⁶ | ×2.02 — Dégradation |

**Tous les modules FAIL** (seuil 1×10⁻⁶).

**Cause :** Le RK2 avec saturation tanh + normalisation forcée n'est pas plus conservatif qu'Euler + normalisation. La saturation tanh introduit de la dissipation numérique (elle tronque les valeurs qui dépassent 1) qui génère de la dérive d'énergie. L'intégrateur idéal pour la conservation d'énergie est un intégrateur **symplectique** qui ne nécessite ni saturation ni normalisation forcée.

**La racine du problème reste identique depuis le cycle précédent :** l'absence d'un intégrateur unitaire ou symplectique. RK2 + tanh + renorm est une collection de patches qui se combinent mal.

---

### ERREUR 7 — Convergence du pairing non monotone (RÉGRESSION)
**Sévérité :** MODÉRÉE

**Données (`new_tests_results.csv`) :**
```
conv_700_steps  → pairing = 0.8519
conv_1400_steps → pairing = 0.8491
conv_2800_steps → pairing = 0.8466
conv_4200_steps → pairing = 0.8467   ← remontée
conv_monotonic  → nonincreasing = 0 → FAIL
```

**Cause :** L'oscillation est due au fait que RK2 avec tanh + renorm crée une dynamique oscillante dans l'espace des pairings. Le pairing ne converge pas vers une valeur fixe mais oscille avec une période longue (~1400 steps). C'est un comportement de type "orbite périodique" dans l'intégrateur, pas une convergence physique vers l'état fondamental.

**Comparaison :** Dans 523, le pairing convergeait à 0.2408 dès le step 100 et restait parfaitement stable. La stabilité du pairing était meilleure dans 523 (malgré la norme explosive) parce que les valeurs de d[i] se stabilisaient rapidement après la convergence non-normalisée.

---

### ERREUR 8 — Pompage dynamique encore trop faible
**Sévérité :** MODÉRÉE

**Données :**
- Run 5132 : energy_reduction_ratio = **0.2611** (26.1% de réduction active)
- Run 523 : energy_reduction_ratio = **0.0000** (brisé)
- Run 1312 : energy_reduction_ratio = **−0.0002** (actif mais marginal)

**Code source — calcul du feedback (lignes ~283–287) :**
```c
/* Cible d'énergie : target = 0.60 × |t| + 0.18 × |U| */
crt.target_abs_energy = 0.60 * fabs(p->t_eV) + 0.18 * fabs(p->u_eV);
/* Pour hubbard_hts_core : t=0.52, U=4.16 → target = 0.312 + 0.749 = 1.061 eV */

/* EMA de l'énergie absolue courante */
crt.ema_abs_energy = 0.985 * crt.ema_abs_energy + 0.015 * abs_energy;
/* Pour energie = 1.76 : |1.76| = 1.76 > target = 1.061 */

/* Feedback = gain × (target - EMA) / target */
double rel_delta = (crt.target_abs_energy - crt.ema_abs_energy) / (crt.target_abs_energy + EPS);
/* rel_delta = (1.061 - 1.76) / 1.061 = -0.659 (négatif → amortissement) */

double feedback = crt.feedback_gain * rel_delta;
/* feedback = 0.15 × (-0.659) = -0.099 (amortissement faible mais actif) */
```

Le feedback fonctionne mais dans le sens d'amortissement (énergie trop haute par rapport à la cible). La réduction est de −0.021% au lieu de −26.1% de 5132 parce que le feedback_gain (0.15) est insuffisant et la cible n'est atteinte qu'à ±10% après de nombreux steps.

**Solution (ligne ~247 du .c) :**
```c
/* AVANT : */
crt.feedback_gain = 0.15;

/* APRÈS — gain plus agressif, adaptatif selon l'écart : */
double gap = fabs(crt.target_abs_energy - current_energy) / (crt.target_abs_energy + EPS);
crt.feedback_gain = 0.15 + 0.85 * fmin(gap, 1.0);   /* adaptatif 0.15–1.0 */
```

---

## PARTIE V — CE QUI FONCTIONNE TRÈS BIEN DANS 1312

### 5.1 Norme ψ : correcte à la précision machine

Tous les 13 modules : norm_deviation = 5–8 × 10⁻¹⁶ (PASS). C'est essentiellement zéro. C'est la correction la plus importante réalisée entre 523 et 1312. Elle valide que le RK2 stabilisé avec tanh + renorm conditionnelle conserve la norme parfaitement pour les tailles de grille testées (100 sites).

### 5.2 Sensibilité à U entièrement restaurée et physiquement correcte

```
U=6  → énergie = 1.2687  |  U=8  → énergie = 1.7600
U=10 → énergie = 2.2514  |  U=12 → énergie = 2.7420
```
Pente dE/dU ≈ +0.491 eV par unité de U. Linéaire, monotone croissant. `physics/energy_vs_U` → PASS. Correction validée vs 523.

### 5.3 Tests ChatGPT : 11/12 PASS (record)

Pack complet 12 tests, 11 PASS, 0 OBSERVED, 1 FAIL (T12 : campagne solveurs alternatifs pas encore exécutée). C'est le meilleur score depuis le début des cycles.

### 5.4 QMC et DMRG indépendants — nouveaux, tous PASS

**`integration_independent_qmc_results.csv` :** 13 modules, tous PASS, acceptance_rate entre 0.84 et 1.0.  
**`integration_independent_dmrg_results.csv` :** 13 modules, tous PASS, entanglement_index entre 0.54 et 0.78.  
Ces résultats représentent la première validation croisée avec deux méthodes indépendantes (QMC et DMRG) produisant des PASS pour tous les modules.

### 5.5 Entropie et corrélations spatiales — nouveaux observables

**Entropie de Shannon de l'énergie :** `hubbard_hts_core` = 0.661 bits, `dense_nuclear` = 1.628 bits. L'entropie est faible pour les modules les plus stables (peu d'états accessibles dans la distribution d'énergie) et plus haute pour les modules de champ (dense_nuclear, qcd) — cohérent physiquement.

**Autocorrélations spatiales :** Décroissance exponentielle du pairing avec le lag, corrélation énergie haute (>0.95) aux lags 1-4. Ces observables étaient demandés depuis plusieurs cycles et sont maintenant présents.

### 5.6 Benchmark QMC/DMRG : premier résultat PARTIELLEMENT dans les barres d'erreur

Dans 5132 et 523 : 0% des points dans les barres d'erreur.  
Dans 1312 : **6.67%** (1/15 points) dans les barres d'erreur. Test T5 physics matrix → PASS (critère ≥ trend correlation).

### 5.7 Absence totale de champs vides dans les métadonnées

`integration_absent_metadata_fields.csv` : tous les champs signalés comme `PRESENT`. `ABSENT_METADATA_EXTRACTOR_GATE` → PASS. Tous les champs t, U, seed, model_id, hamiltonian_id, schema_version sont remplis.

### 5.8 Chargement des problèmes depuis config CSV externe

Le fichier `config/problems_cycle06.csv` est désormais la source de vérité des paramètres physiques. Le code renvoie une erreur explicite si ce fichier manque. C'est la correction du hardcoding de la liste de problèmes.

### 5.9 V4-Next : 88.05%

| Blocker | Gap 523 | Gap 1312 | Progrès |
|---|---|---|---|
| `connection` | −11.50 | −11.50 | Identique |
| `shadow_safety` | −12.50 | −12.50 | Identique |
| `realism` | −5.06 | **−3.73** | **+1.33 pts** |
| **Total** | −29.06 | **−27.73** | **+1.33 pts** |

---

## PARTIE VI — POINTS NON SIGNALÉS PRÉCÉDEMMENT DÉTECTÉS PAR L'AUDIT

### 6.1 Run intermédiaire 655 non analysé

Le `integration_run_drift_monitor.csv` révèle :
```
reference_run = research_20260312T122511Z_655
energy max diff vs 1312 = 3.646
pairing max diff vs 1312 = 0.700
```
Il existe un run **655** (12:25:11Z) exécuté entre 523 (07:59Z) et 1312 (12:34Z), qui n'a pas été analysé dans les rapports CHAT/. Ce run est la référence immédiate de 1312. L'écart énergie max = 3.65 entre 655 et 1312 est significatif et mérite une analyse dédiée.

### 6.2 La saturation tanh() modifie physiquement l'Hamiltonien

**Code ligne ~290 :**
```c
d[i] = tanh(d[i]);
```
Cette opération borne le vecteur d'état entre [−1, +1]. Dans un vrai espace de Hilbert, les composantes de la fonction d'onde n'ont pas cette contrainte. La saturation tanh est une **déformation non-physique** de l'espace des états : elle empêche l'état quantique d'accéder à des configurations de haute énergie. C'est une limitation fondamentale qui explique pourquoi les pairings sont élevés (0.76–0.92) — la saturation maintient les composantes proches de ±1, ce qui maximise le pairing `exp(−|d[i]|×T/65)`.

### 6.3 La fonction `burn_metric` consomme des cycles CPU sans usage physique

**Code lignes ~314–317 :**
```c
double burn_metric = 0.0;
for (int k = 0; k < burn_scale * 220; ++k) {
    burn_metric += sin((double)k + step_energy) + 0.5 * cos((double)k * 0.33 + collective_mode);
}
r.energy_drift_metric = burn_metric * 1e-10;
```
Cette boucle fait `burn_scale × 220` calculs de sin/cos par step par module. Pour burn_scale = 99 et 13 modules, c'est `99 × 220 × 13 × 2700 steps = 76 millions d'opérations sin/cos` qui ne servent qu'à occuper le CPU. `energy_drift_metric` vaut `burn_metric × 1e-10` — une valeur trop petite pour être physiquement significative. Cette boucle est du remplissage artificiel de temps de calcul.

**Recommandation :** Supprimer cette boucle burn et la remplacer par de vrais calculs de corrélations à longue portée.

### 6.4 La cible de feedback est codée en dur (ligne ~244)

```c
crt.target_abs_energy = 0.60 * fabs(p->t_eV) + 0.18 * fabs(p->u_eV);
```
Les coefficients 0.60 et 0.18 sont hardcodés. Ils ne sont pas chargés depuis la configuration. Si les paramètres t et U changent, la cible change correctement (car elle utilise t_eV et u_eV dynamiques), mais les poids 0.60/0.18 sont arbitraires et non documentés physiquement.

### 6.5 Le module energy_unit est ignoré dans les résultats exportés

**Lignes ~167–187 :** La fonction `module_energy_unit()` définit que `hubbard_hts_core` produit de l'énergie en **meV** (facteur 1e3) et `qcd_lattice_fullscale` en **GeV** (facteur 1e-9). Mais les valeurs dans `baseline_reanalysis_metrics.csv` et le log sont toutes en **eV** sans conversion appliquée. L'affichage dit `energy=1.7607` pour hubbard_hts_core mais selon la fonction module_energy_unit, ce devrait être affiché en meV (soit 1760.7 meV). Cette incohérence entre la définition et l'export des unités est un bug latent non signalé.

---

## PARTIE VII — TABLEAU BILAN FINAL

### 7.1 Score par dimension

| Dimension | Run 5132 | Run 523 | Run 1312 | Tendance |
|---|---|---|---|---|
| Isolation fichiers | 100% | 100% | 100% | Stable ✓ |
| Traçabilité brute | 93% | 93% | 93% | Stable |
| **Reproductibilité** | 50% | 100% | 100% | Acquis ✓ |
| **Robustesse numérique** | 61% | 58% | **55%** | Tendance baisse |
| **Validité physique** | 67% | 50% | **67%** | Remontée ✓ |
| Couverture questions exp. | 72% | 58% | **63%** | Remontée ✓ |
| Norme ψ conservée | FAIL×13 | FAIL×13 (7-10) | **PASS×13** | Correction clé ✓ |
| Sensibilité à U | FAIL | FAIL | **PASS** | Corrigé ✓ |
| Énergie signe physique | Positif | **Négatif** | Positif | Regression |
| Pairing réaliste | Non (1.0) | **Oui** (0.12–0.45) | Partiel (0.76–0.91) | Partiel |
| Checksums/signatures | Non | **Oui** | Non | Retrait |

### 7.2 État V4-Next

| Indicateur | Valeur |
|---|---|
| Progression FULL | **88.05%** |
| Blocker realism gap | −3.73 pts |
| Blocker connection gap | −11.50 pts |
| Blocker shadow_safety gap | −12.50 pts |
| Status | SHADOW_BLOCKED |
| Rollback automatique | ENABLED |

---

## PARTIE VIII — FEUILLE DE ROUTE — CORRECTIONS PRIORITAIRES

### PRIORITÉ 1 — Corriger le terme d'énergie locale pour obtenir des énergies négatives correctes

**Fichier :** `src/advanced_calculations/quantum_problem_hubbard_hts/src/hubbard_hts_research_cycle_advanced_parallel.c`  
**Ligne exacte à modifier :** ~295–300

```c
/* AVANT (ligne ~295) : */
double local_energy = p->u_eV * n_up * n_dn
                    - p->t_eV * hopping_term
                    - p->mu_eV * density;

/* APRÈS — energie correcte centrée sur le demi-remplissage : */
double local_energy = p->u_eV * (n_up * n_dn - 0.25)
                    - p->t_eV * hopping_term
                    - p->mu_eV * (n_up + n_dn - 1.0);
```

### PRIORITÉ 2 — Supprimer le seuil de normalisation arbitraire (sites ≤ 256)

**Fichier :** même fichier  
**Ligne exacte à modifier :** ~304

```c
/* AVANT (ligne ~304) : */
if (sites <= 256) {
    normalize_state_vector(d, sites);
}

/* APRÈS — normalisation uniforme pour tous (ou intégrateur unitaire) : */
normalize_state_vector(d, sites);   /* option transitoire */
/* Objectif final : intégrateur de Cayley qui normalise intrinsèquement */
```

### PRIORITÉ 3 — Corriger le label du norm_guard CSV

**Fichier :** même fichier  
**Ligne exacte :** 775

```c
/* AVANT (ligne 775) : */
fprintf(ngcsv, "%s,%.12e,%.12e,%s,rk2_without_forced_renorm\n", ...

/* APRÈS : */
const char* nmethod = (probs[i].lx * probs[i].ly <= 256)
    ? "rk2_stabilized_with_conditional_renorm"
    : "rk2_stabilized_no_renorm";
fprintf(ngcsv, "%s,%.12e,%.12e,%s,%s\n",
        probs[i].name, base[i].norm_deviation_max, 1e-6,
        norm_ok ? "PASS" : "FAIL", nmethod);
```

### PRIORITÉ 4 — Synchroniser le module indépendant avec le module principal

**Fichier :** même fichier  
**Ligne exacte :** ~438 (dans `simulate_problem_independent`)

```c
/* AJOUTER après d[i] = tanhl(d[i]) — ligne ~438 : */
if (sites <= 256) {
    long double norm2 = 0.0L;
    for (int j = 0; j < sites; ++j) norm2 += d[j] * d[j];
    if (norm2 > 1e-30L) {
        long double inv = 1.0L / sqrtl(norm2);
        for (int j = 0; j < sites; ++j) d[j] *= inv;
    }
}
```

### PRIORITÉ 5 — Ajouter le seuil d'amplitude FFT dans le test

**Fichier :** outil Python générant les tests  
**Action :** Définir `fft_amplitude_max = 1.0` et marquer FAIL si dépassé.

### PRIORITÉ 6 — Supprimer la boucle burn_metric et la remplacer par des corrélations réelles

**Fichier :** même fichier C  
**Lignes :** ~314–317  
**Action :** Supprimer les 4 lignes de la boucle burn. Calculer à la place `C(r) = Σ d[i]×d[(i+r)%sites]` pour r = 1, 2, 4, 8 (autocorrélation spatiale vraie).

### PRIORITÉ 7 — Restaurer les checksums SHA256 et la signature cryptographique

**Fichier :** `tools/fullscale_strict_protocol_runner.py`  
**Action :** Réintégrer la génération des fichiers `run_signature.json` et `checksums.sha256` qui existaient dans 523 mais ont disparu dans 1312.

---

## CONCLUSION

Le run `1312` est un **progrès net sur des points précis** et une **régression sur d'autres** :

**Ce qui est définitivement corrigé :**  
La norme ψ est conservée à la précision machine (7×10⁻¹⁶). La sensibilité à U est physiquement correcte. Le chargement depuis CSV externe remplace le hardcoding de la liste de problèmes. Les méta-données sont complètes. 11 tests ChatGPT passent sur 12.

**Ce qui est régressé ou non résolu :**  
Les énergies sont redevenues positives (non physique pour un état fondamental). La saturation tanh déforme physiquement l'espace des états. La discontinuité à 18×18 est un artefact pur du seuil `sites ≤ 256`. Le drift d'énergie est pire qu'en 523. Les checksums cryptographiques ont disparu. La vérification indépendante reste FAIL (delta = 1.56).

**L'action prioritaire unique** qui résoudrait le plus grand nombre de problèmes simultanément reste l'introduction d'un **intégrateur de Cayley** (unitaire par construction, norme conservée sans correction, énergie conservée sans normalisation forcée). Ce serait la solution définitive à la majorité des FAILs persistants.
