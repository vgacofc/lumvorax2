# Rapport ultra détaillé A→Z : validation réelle des benchmarks, anomalies, lignes de code à corriger

## 0) Ce qui a été fait maintenant (et ce qui n’a pas été inventé)
- Synchronisation distante tentée et réussie après ajout du remote `origin` vers `https://github.com/lumc01/Lumvorax.git` (fetch exécuté).
- Vérification ciblée des notifications demandées :
  - `CHAT/PROMPTCORRECTION1.md` (attentes R1..R11 et méthode).
  - `CHAT/analysechatgpt4.md` (règles unités, intégrateur, stabilité).
  - Le chemin `src/advanced_calculations/CHAT/analysechatgpt4.md` demandé dans le message n’existe pas dans le dépôt local ; le fichier présent est dans `quantum_problem_hubbard_hts/CHAT/`.
- Lecture des CSV/logs de la run “avant” et “après” pour comparer simulation par simulation.

## 1) Résultat global avant/après (la photo globale)
- Tests total après: 80
- PASS après: 20
- FAIL après: 11
- OBSERVED après: 49
- Comparaison avant/après: les fichiers benchmark QMC/DMRG et external modules sont identiques (aucune amélioration numérique mesurée), donc les écarts de benchmark ne sont pas corrigés dans cette correction.

### Traduction enfant (10 ans)
- Imagine un bulletin scolaire: avant la correction et après la correction, les mauvaises notes principales des contrôles de comparaison n’ont pas changé. Donc ce qui était faux reste faux pour ces contrôles précis.

## 2) Benchmarks QMC/DMRG — valeurs attendues, valeurs obtenues, écart en %
> Règle simple: `reference` = valeur attendue; `model` = valeur calculée; `% écart` = `rel_error*100`; `within_error_bar=1` veut dire “accepté”, `0` veut dire “refusé”.

| Module | Observable | Référence attendue | Valeur modèle | Erreur absolue | Écart % | Barre erreur | Validé ? |
|---|---:|---:|---:|---:|---:|---:|---|
| hubbard_hts_core | pairing | 0.8800000000 | 0.9775524556 | 0.0975524556 | 11.085506% | 0.0700000000 | NON |
| hubbard_hts_core | pairing | 0.8100000000 | 0.9689502296 | 0.1589502296 | 19.623485% | 0.0600000000 | NON |
| hubbard_hts_core | pairing | 0.7500000000 | 0.9558710159 | 0.2058710159 | 27.449469% | 0.0600000000 | NON |
| hubbard_hts_core | pairing | 0.7000000000 | 0.9486564447 | 0.2486564447 | 35.522349% | 0.0600000000 | NON |
| hubbard_hts_core | pairing | 0.6500000000 | 0.9403784225 | 0.2903784225 | 44.673603% | 0.0600000000 | NON |
| hubbard_hts_core | pairing | 0.6100000000 | 0.9271726215 | 0.3171726215 | 51.995512% | 0.0600000000 | NON |
| hubbard_hts_core | pairing | 0.5600000000 | 0.9184385583 | 0.3584385583 | 64.006885% | 0.0600000000 | NON |
| hubbard_hts_core | pairing | 0.5200000000 | 0.9151667136 | 0.3951667136 | 75.993599% | 0.0600000000 | NON |
| hubbard_hts_core | energy | 652800.0000000000 | -0.2307495231 | 652800.2307495230 | 100.000035% | 12000.0000000000 | NON |
| hubbard_hts_core | energy | 1013875.0000000000 | -0.1784050990 | 1013875.1784050990 | 100.000018% | 9000.0000000000 | NON |
| hubbard_hts_core | energy | 1374940.0000000000 | -0.1637852448 | 1374940.1637852448 | 100.000012% | 9000.0000000000 | NON |
| hubbard_hts_core | energy | 1736003.0000000000 | -0.1531290586 | 1736003.1531290587 | 100.000009% | 9000.0000000000 | NON |
| hubbard_hts_core | energy | 2097068.0000000000 | -0.0978825438 | 2097068.0978825437 | 100.000005% | 9000.0000000000 | NON |
| hubbard_hts_core | energy | 2458130.0000000000 | -0.1005756834 | 2458130.1005756836 | 100.000004% | 12000.0000000000 | NON |
| hubbard_hts_core | energy | 2819200.0000000000 | -0.0813641499 | 2819200.0813641497 | 100.000003% | 14000.0000000000 | NON |

### Lecture simple
- Dans ce tableau, **toutes les lignes sont NON validées** (`within_error_bar=0`).
- Donc les références scientifiques attendues ne sont pas encore atteintes pour ces cas de comparaison.

## 3) Benchmarks modules externes — valeurs attendues, valeurs obtenues, écart en %
| Module | Observable | Référence attendue | Valeur modèle | Erreur absolue | Écart % | Barre erreur | Validé ? |
|---|---:|---:|---:|---:|---:|---:|---|
| spin_liquid_exotic | pairing | 0.6840000000 | 0.9735648894 | 0.2895648894 | 42.334048% | 0.0900000000 | NON |
| spin_liquid_exotic | energy | 56120.0000000000 | -0.1092616367 | 56120.1092616367 | 100.000195% | 9500.0000000000 | NON |
| topological_correlated_materials | pairing | 0.6360000000 | 0.9658992859 | 0.3298992859 | 51.870957% | 0.0900000000 | NON |
| topological_correlated_materials | energy | 56760.0000000000 | -0.2312037365 | 56760.2312037365 | 100.000407% | 9500.0000000000 | NON |
| correlated_fermions_non_hubbard | pairing | 0.5650000000 | 0.9497437471 | 0.3847437471 | 68.096238% | 0.0900000000 | NON |
| correlated_fermions_non_hubbard | energy | 42900.0000000000 | -0.1813758610 | 42900.1813758610 | 100.000423% | 9500.0000000000 | NON |
| multi_state_excited_chemistry | pairing | 0.7490000000 | 0.9696946161 | 0.2206946161 | 29.465236% | 0.0900000000 | NON |
| multi_state_excited_chemistry | energy | 36920.0000000000 | -0.2921883209 | 36920.2921883209 | 100.000791% | 9500.0000000000 | NON |
| bosonic_multimode_systems | pairing | 0.5210000000 | 0.9350640060 | 0.4140640060 | 79.474857% | 0.1000000000 | NON |
| bosonic_multimode_systems | energy | 11200.0000000000 | -0.0963865324 | 11200.0963865324 | 100.000861% | 9500.0000000000 | NON |
| multiscale_nonlinear_field_models | pairing | 0.4220000000 | 0.9307619961 | 0.5087619961 | 120.559715% | 0.1000000000 | NON |
| multiscale_nonlinear_field_models | energy | 30240.0000000000 | -0.2274892640 | 30240.2274892640 | 100.000752% | 9500.0000000000 | NON |
| far_from_equilibrium_kinetic_lattices | pairing | 0.2860000000 | 0.9196206410 | 0.6336206410 | 221.545679% | 0.1100000000 | NON |
| far_from_equilibrium_kinetic_lattices | energy | 35250.0000000000 | -0.1477859043 | 35250.1477859043 | 100.000419% | 9500.0000000000 | NON |
| multi_correlated_fermion_boson_networks | pairing | 0.4870000000 | 0.9425357688 | 0.4555357688 | 93.539172% | 0.1000000000 | NON |
| multi_correlated_fermion_boson_networks | energy | 56130.0000000000 | -0.1759782042 | 56130.1759782042 | 100.000314% | 9500.0000000000 | NON |

### Lecture simple
- Dans ce tableau aussi, **toutes les lignes sont NON validées**.
- Le pire écart en pourcentage dépasse 200%, ce qui est trop loin des références.

## 4) Stabilité numérique (ce qui est validé / non validé)
- Conservation d’énergie: plusieurs lignes PASS dans la suite `energy_conservation` (bon signal sur cette métrique locale).
- Von Neumann: lignes FAIL présentes avec `spectral_radius` autour de 1.0002246148 (>1), donc instabilité non corrigée complètement.

| Module | Test | Valeur | Statut |
|---|---|---:|---|
| hubbard_hts_core | spectral_radius | 1.0002246148 | FAIL |
| qcd_lattice_proxy | spectral_radius | 1.0002246148 | FAIL |
| quantum_field_noneq | spectral_radius | 1.0002246148 | FAIL |
| dense_nuclear_proxy | spectral_radius | 1.0002246148 | FAIL |
| quantum_chemistry_proxy | spectral_radius | 1.0002246148 | FAIL |
| spin_liquid_exotic | spectral_radius | 1.0002246148 | FAIL |
| topological_correlated_materials | spectral_radius | 1.0002246148 | FAIL |
| correlated_fermions_non_hubbard | spectral_radius | 1.0002246148 | FAIL |
| multi_state_excited_chemistry | spectral_radius | 1.0002246148 | FAIL |
| bosonic_multimode_systems | spectral_radius | 1.0002246148 | FAIL |
| multiscale_nonlinear_field_models | spectral_radius | 1.0002246148 | FAIL |
| far_from_equilibrium_kinetic_lattices | spectral_radius | 1.0002246148 | FAIL |
| multi_correlated_fermion_boson_networks | spectral_radius | 1.0002246148 | FAIL |

### Traduction enfant (10 ans)
- C’est comme un vélo: il tient debout si la roue reste dans la limite. Ici la roue dépasse un tout petit peu la limite (1.000224 > 1), donc le test dit “attention, ça peut dériver”.

## 5) Vérification “mélange de formats / indépendance des simulations” dans le code source
### 5.1 Ce qui est bien (déjà en place)
- Un seed est passé par valeur dans chaque appel de simulation, ce qui aide à garder des campagnes indépendantes.
- Les modules ont une conversion d’unité d’énergie dédiée pour les exports (`meV`, `GeV`, `MeV`, sinon `eV`).
- Des tests de garde existent pour unités, norme psi et ratio sans dimension (`integration_units_end_to_end.csv`, `integration_norm_psi_guard.csv`, `integration_dimensionless_ht_over_hbar.csv`).

### 5.2 Ce qui pose encore problème (lignes exactes à corriger)
1. **Proxy Von Neumann trop permissif/instable**: `src/hubbard_hts_research_cycle_advanced_parallel.c` lignes 299-309.
   - `amp = fabs(base) + dt * forcing` additionne un terme de forçage qui pousse facilement le rayon spectral au-dessus de 1.
   - Correction à faire: utiliser une formulation de stabilité cohérente avec l’intégrateur réellement utilisé (ou une borne analytique stricte par module), puis recalibrer les coefficients pour obtenir `spectral_radius <= 1` dans les cas cibles.
2. **Chemin “advanced” vs “non advanced” non totalement harmonisé**: `src/hubbard_hts_research_cycle.c` contient une autre implémentation avec structures et noms plus anciens (`energy`, `pairing` sans suffixes d’unité), lignes 43-50.
   - Correction à faire: aligner cette voie avec le fichier `...advanced_parallel.c` (noms unités explicites, mêmes garde-fous, mêmes exports de tests).
3. **Risque de confusion d’unité dans nommage de variable**: dans `...advanced_parallel.c`, `r.energy_meV` reçoit `step_energy` sans conversion dans la boucle de simulation (ligne 234), alors que la conversion de module est appliquée plus tard pour des exports spécifiques.
   - Correction à faire: soit renommer en `energy_eV_internal`, soit convertir explicitement avant d’écrire une variable suffixée `meV` pour éviter toute ambiguïté.

## 6) Question demandée: “les tests bout-en-bout de format benchmark existent-ils ?”
- Oui, il existe des fichiers d’intégration ciblés format/unité/norme/dimensionless dans la run après:
  - `integration_units_end_to_end.csv`
  - `integration_norm_psi_guard.csv`
  - `integration_dimensionless_ht_over_hbar.csv`
- Mais ces tests n’ont **pas** suffi à faire passer les benchmarks de comparaison (`within_error_bar` reste à 0).

## 7) Question demandée: “document MD standardisé des fonctions/variables/chemins/dépendances ?”
- Je n’ai pas trouvé de fichier unique dédié de type `STANDARD_NAMES_...md` ou `DEPENDENCIES_...md` dans le dossier `quantum_problem_hubbard_hts` au format demandé “spécifique projet, exhaustif fonctions + variables + chemins + dépendances”.
- Il existe un `README.md`, mais il décrit surtout l’exécution et la structure de run, pas une nomenclature exhaustive ligne par ligne des fonctions/variables/sous-fonctions/dépendances.
- Action restante recommandée: créer un fichier dédié, par exemple `NOMENCLATURE_ET_DEPENDANCES_QUANTUM_PROBLEM_HUBBARD_HTS.md`, avec sections obligatoires: fonctions publiques, fonctions statiques, variables globales, unités attendues, dépendances internes/externes, chemins d’I/O standards.

## 8) Ce qui a été validé / non validé (réponse brute)
### Validé
- Les 3 nouveaux garde-fous de format (units, norm psi, dimensionless) existent dans les artefacts après correction.
- Le pipeline produit bien logs + checksums + rapports dans la run après correction.

### Non validé
- Les benchmarks de comparaison restent non validés (aucune ligne dans la barre d’erreur).
- Le test Von Neumann reste en échec sur plusieurs modules.
- L’unification de style de nommage unités n’est pas homogène sur tous les runners (`advanced_parallel.c` vs `research_cycle.c`).

### Reste à faire exactement
- Corriger le calcul/critère de stabilité dans `von_neumann_proxy` et revalider la suite `numerical_stability_suite.csv`.
- Harmoniser le runner `hubbard_hts_research_cycle.c` avec les protections du runner `hubbard_hts_research_cycle_advanced_parallel.c`.
- Clarifier les noms de variables d’énergie pour éviter ambiguïté unité (`energy_meV` vs valeur interne eV).
- Ajouter un MD de nomenclature/dépendances spécifique projet (manquant).

## 9) Petit dictionnaire “mot technique → explication enfant 10 ans”
- **benchmark**: Test de comparaison avec une copie modèle qu’on considère comme la référence.
- **reference**: La valeur cible attendue (la bonne réponse de comparaison).
- **model**: La valeur sortie par ton programme.
- **abs_error**: La distance simple entre la valeur attendue et la valeur calculée.
- **rel_error**: La même distance, mais ramenée en pourcentage pour voir si c’est petit ou grand.
- **error_bar**: La marge acceptable; si tu restes dedans, c’est validé.
- **within_error_bar**: 1 = dans la marge, 0 = hors marge.
- **von_neumann**: Un contrôle mathématique pour savoir si les calculs explosent avec le temps.
- **spectral_radius**: Le chiffre clé de stabilité; au-dessus de 1, ça devient risqué.
- **norm psi**: La “taille” de l’état quantique; on veut la garder à 1 pour rester cohérent.
- **dimensionless**: Nombre sans unité, comme un score pur.
- **seed RNG**: Le numéro de départ du hasard; même seed = même tirage aléatoire.
- **drift**: Glissement progressif (ex: énergie qui bouge alors qu’elle devrait rester stable).