# RAPPORT D'EXÉCUTION DES PRIORITÉS — 2854/2715 (2026-03-12T20:30Z)

## Portée
Ce rapport exécute explicitement les priorités listées dans:
`CHAT/RAPPORT_ANALYSE_TOTALE_2854_2715_vs_1312_523_ERREURS_CAUSES_SOLUTIONS.md`
sans modifier les anciens rapports.

## Priorités appliquées

### P1 — Hopping quantique corrigé (advanced_parallel)
- Fichier: `src/hubbard_hts_research_cycle_advanced_parallel.c`
- Remplacement de `fabs(corr[i]-d[i])` par un terme cinétique local symétrique:
  - `hopping_lr = -0.5 * d[i] * (d_left + d_right)`
- Énergie locale alignée sur la formule demandée:
  - `U*n_up*n_dn - t*hopping_lr - mu*(n_up+n_dn-1)`

### P2 — Même correction dans le module indépendant (advanced_parallel)
- Même formule implémentée en `long double` dans `simulate_problem_independent`.

### P3 — Critère `energy_vs_U` corrigé
- Test modifié pour suivre la pente moyenne de `|E|` vs `U`:
  - paramètre exporté: `avg_dAbsE_dU_positive`

### P4 — Renormalisation appliquée à toutes tailles
- Suppression de la condition `if (sites <= 256)`.
- Renormalisation systématique dans les deux solveurs (double et long double) du moteur avancé.

### P5 — Seuil FAIL amplitude FFT
- Ajout d’un seuil explicite:
  - `fft_amp_ok = fft_valid && (fft_amp < 2.0)`
- Le statut de `fft_dominant_amplitude` est maintenant FAIL au-delà du seuil.

### P6 — Propagation dans moteur fullscale (2715)
- Même correction de formule énergétique + module indépendant.
- Même critère `energy_vs_U` sur `|E|`.
- Même seuil FFT amplitude.

## Validation technique
- Compilation stricte OK:
  - `make -C src/advanced_calculations/quantum_problem_hubbard_hts clean all`
- Aucune erreur de compilation.
- Avertissements éliminés (notamment variable locale inutilisée dans advanced).

## Commandes de reproduction
```bash
git fetch --no-tags https://github.com/lumc01/Lumvorax.git
make -C src/advanced_calculations/quantum_problem_hubbard_hts clean all
bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh
```
