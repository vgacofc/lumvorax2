# Rapport avant/après — fusion V2 en staging (sans toucher l'officiel)

## 1) Suppression binaire demandée
- Fichier supprimé: `src/advanced_calculations/quantum_simulator_v2_staging/quantum_simulator.o`.
- Raison: éviter les binaires versionnés et l'erreur de revue Replit/GitHub (fichier binaire non supporté en diff texte).

## 2) Réintégration de l'existant (sans casser)
Comparaison stricte avec les fichiers officiels:
- `src/advanced_calculations/quantum_simulator.c` vs `.../staging/quantum_simulator.c`: **0 ligne de différence**.
- `src/advanced_calculations/quantum_simulator.h` vs `.../staging/quantum_simulator.h`: **0 ligne de différence**.

Conclusion: tout l'existant a été réintégré à l'identique dans la staging.

## 3) Ce qui a été ajouté pour la fusion V2

### 3.1 Nouveau module fusion (sans modifier l'ancien code)
- `quantum_simulator_fusion_v2.h`
  - ajoute `quantum_rng_mode_e`:
    - `QUANTUM_RNG_HARDWARE_ONLY`
    - `QUANTUM_RNG_HARDWARE_PREFERRED`
    - `QUANTUM_RNG_DETERMINISTIC_SEEDED`
  - ajoute `quantum_fusion_v2_summary_t` (wins/losses/win_rate/kpi débit).
  - expose `quantum_fusion_v2_run_forensic_benchmark(...)`.

- `quantum_simulator_fusion_v2.c`
  - benchmark forensic multi-scénarios complet (log JSONL **chaque scénario**, pas échantillonné 1/64).
  - calcule `nqubit_wins` et `baseline_wins` pour expliciter le 65.3% / 34.7%.
  - supporte RNG hardware-only (échec explicite si aucune source hardware lisible).
  - écrit une ligne `run_config` + lignes `scenario_margin` + ligne `summary`.

### 3.2 Runner Replit exact
- `run_on_replit.sh`
  - trouve les fichiers staging automatiquement via `find`.
  - compile le test unitaire existant avec la staging (sans toucher l'officiel).
  - exécute le benchmark fusion V2 et produit:
    - `results/fusion_forensic_full.jsonl`

## 4) Commande exacte Replit (trouve le fichier puis exécute)
Depuis n'importe où:

```bash
bash /workspace/Lumvorax/src/advanced_calculations/quantum_simulator_v2_staging/run_on_replit.sh /workspace/Lumvorax
```

Cette commande:
1. localise automatiquement les fichiers staging,
2. compile et exécute les tests staging,
3. génère le log forensic JSONL complet de fusion V2.

## 5) Pourquoi cette approche respecte la contrainte
- aucun changement sur `src/advanced_calculations/quantum_simulator.c` officiel,
- aucun changement sur `src/advanced_calculations/quantum_simulator.h` officiel,
- fusion V2 isolée et exécutable,
- suppression du binaire versionné.
