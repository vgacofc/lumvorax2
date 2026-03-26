# RAPPORT DE CORRECTION — Overflow post-run forensic (Replit, 2026-03-12)

## 1) Synchronisation dépôt distant (sans récupérer de binaire)
- Dépôt synchronisé depuis `https://github.com/lumc01/Lumvorax.git` via fetch Git uniquement.
- Vérification: le commit distant pointé par `FETCH_HEAD` est identique au `HEAD` local (`55fcb5af...`), donc le dépôt local était déjà à jour.
- Aucun téléchargement d’artefacts binaires, aucun `git lfs pull`, aucun import ZIP.

## 2) Symptôme observé
Pendant le cycle:
- `post_run_forensic_extension_tests.py` échoue avec:
  - `OverflowError: (34, 'Numerical result out of range')`
  - origine: calcul RMSE sur `(a - b) ** 2`
- Ensuite `post_run_full_scope_integrator.py` échoue par propagation (`CalledProcessError`, `check=True`).

## 3) Cause racine exacte
Le flux fautif est:
1. Le modèle power-law en validation croisée calcule `exp(la + lb * log(x))`.
2. Certaines combinaisons de données produisent des prédictions extrêmes.
3. Le RMSE carré (`(a-b)^2`) déborde en flottant.
4. Le script lève une exception et casse la chaîne post-run.

## 4) Correctifs implémentés (code)
Fichier corrigé:
- `src/advanced_calculations/quantum_problem_hubbard_hts/tools/post_run_forensic_extension_tests.py`

### 4.1 Garde-fous numériques globaux
- `MAX_EXP_ARG = 700.0` (borne stable pour `exp` en float64)
- `MAX_SAFE_MAG = 1e154` (borne avant carré)

### 4.2 Fonctions robustes ajoutées
- `safe_square(x)`
  - renvoie `inf` si `x` non-fini ou trop grand
  - sinon calcule `x*x`
- `safe_exp(x)`
  - borne l’argument dans `[-700, 700]`
  - évite les overflows de `exp`

### 4.3 RMSE robuste
- `rmse()`:
  - ignore les paires non-finies
  - utilise `safe_square` + `math.fsum`
  - renvoie `inf` si aucune paire exploitable (au lieu d’exception)

### 4.4 Validation croisée robuste
- Prédiction power-law migrée de `math.exp(...)` vers `safe_exp(...)`.
- Le statut `PASS` est maintenant strict:
  - uniquement si RMSE power et linéaire sont **finies** et `rmse_pow <= rmse_lin`
  - sinon statut `OBSERVED` (pipeline continue)

## 5) Vérification d’intégrité des données/résultats
- Le script réparé exécute la génération des artefacts:
  - `integration_forensic_extension_tests.csv`
  - `integration_test_coverage_dashboard.csv`
  - `forensic_extension_summary.json`
- Aucun crash sur le run historique incriminé.

## 6) Ce qui manquait à préciser (ajouts)
1. Le bug est **post-traitement Python**, pas compilation C ni solveur principal.
2. Le comportement attendu en valeurs extrêmes doit être **dégradation contrôlée** (`OBSERVED`) et non arrêt brutal.
3. Les non-finies doivent être auditables dans les CSV (`inf`, `nan`) pour analyse scientifique ultérieure.
4. Reproductibilité: commande d’exécution exacte ci-dessous.

## 7) Commandes exactes reproductibles (Replit)
Depuis la racine repo:

```bash
git fetch --no-tags https://github.com/lumc01/Lumvorax.git
bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh
```

Validation isolée du correctif (sur un run existant):

```bash
python3 src/advanced_calculations/quantum_problem_hubbard_hts/tools/post_run_forensic_extension_tests.py \
  src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T173817Z_1335
```

## 8) Recommandations complémentaires (non bloquantes)
- Ajouter un indicateur `numeric_guard_triggered` dans la sortie CSV.
- Ajouter un test CI synthétique “extrêmes numériques” pour forcer des valeurs limites et valider l’absence de crash.
- Optionnel: journaliser le nombre de points exclus (non-finies) par test pour diagnostic rapide.
