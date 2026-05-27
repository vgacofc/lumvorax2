# Liste des transferts et remises en place (Correction Erreur Structure)

## 🔄 Fichiers remis dans `src/` (Système LUM/VORAX)
Tous les fichiers non liés à la simulation ont été replacés dans leur dossier d'origine :
- **Tests système** : `test_unit_all_47_modules.c`, `test_unit_v32_all_modules.c`, `v44_final_proof.c`, etc.
- **Tests individuels** : L'intégralité du dossier `src/tests/individual/` a été restauré.
- **Rapports système** : `RAPPORT_360_FINAL_V20.md`, `ROADMAP_VALIDATION.md`, etc.
- **Core & Utils** : Les fichiers de `src/core/` ont été remis en place.

## 📂 Fichiers GARDÉS dans `trou_noir_sim/` (Simulation Uniquement)

### Dans `trou_noir_sim/tests/` :
- `test_kerr.c` : Test de base de la métrique.
- `test_kerr_v3.c` : Tests avancés (Frame-dragging).
- `test_precision_quad.c` : Test de précision 128-bit.
- `test_multi_solver.c` : Test des différents solveurs physiques.
- `individualtest_mhd_invariants.c` : Invariants MHD.
- `test_nx47_v134_strict_training.py` : Scripts liés à l'entraînement de la simu.

### Dans `trou_noir_sim/physics/` :
- `kerr_metric.c` / `kerr_metric.h`
- `kerr_math.h`
- `radiative.c`
- `solver_suite.c` / `solver_suite.h`

### Dans `trou_noir_sim/logging/` :
- `log_writer.c` / `log_writer.h`
- `RAPPORT_SCIENTIFIQUE_FINAL.md`
- `RAPPORT_PUBLICATION_PRD_V29.md` (Nouveau rapport d'expertise)
