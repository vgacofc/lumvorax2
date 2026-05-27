# AUTO-PROMPT — FORMAT LEGACY ALIGNÉ ANCIENS RAPPORTS

```text
Reprendre les anciens rapports/prompt de CHAT, respecter le format strict:
- auto-prompt en tête,
- inventaire,
- causes racines,
- tableau AVANT/APRÈS (fichier + ligne exacte),
- roadmap priorisée,
- checklist finale cochée uniquement après validation réelle.
```

---

# RAPPORT FINAL — APPLICATION IMMÉDIATE DES PRIORITÉS (EN PARALLÈLE)

## Expertises mobilisées (auto-critique)
- **Pipeline Reliability**: robustesse orchestration et checkpoints/progression.
- **Forensic Integrity**: génération/validation checksums réellement bloquante.
- **Scientific QA**: cohérence avec analyses antérieures (runs 523/2715/2854/2829/3447).

Auto-critique: jusqu’ici, des corrections étaient présentes mais pas “bloquantes”. Le trou principal était l’absence de **vérification checksum hard-fail** dans le flux normal.

## 1) Priorités appliquées immédiatement
### P0 appliqué: intégrité bloquante
- Ajout d’une vérification explicite `sha256sum -c` intégrée au script de cycle.
- Si checksum invalide, le cycle échoue immédiatement (`set -euo pipefail`).

### P0 appliqué: checkpoints/progression robuste à l’évolution
- Conserve le calcul dynamique de `TOTAL_STEPS` depuis les lignes `print_progress`.
- Ajout de nouveaux checkpoints “verify” absorbés automatiquement (plus de drift manuel).

## 2) Tableau AVANT / APRÈS (fichier + lignes exactes)

| Fichier exact | Lignes AVANT | Code AVANT | Lignes APRÈS | Code APRÈS | Impact |
|---|---:|---|---:|---|---|
| `src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh` | 52-59 | `write_checksums(){...}` uniquement | 52-68 | `write_checksums(){...}` + `verify_checksums(){ sha256sum -c ... }` | Ajoute gate d’intégrité réellement exécutable |
| `src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh` | 82-83 | checksum fullscale généré sans vérification | 91-94 | checksum fullscale + `verify_checksums "$FULLSCALE_RUN_DIR"` | Stoppe immédiatement un manifest fullscale invalide |
| `src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh` | 209-211 | checksum final généré sans vérification | 220-223 | checksum final + `verify_checksums "$RUN_DIR"` | Garantit que le manifest final n’est pas juste “présent”, mais valide |

## 3) Impact attendu sur le problème signalé
- Le bug “checkpoints qui disparaissent/reculent” est traité par le total dynamique (déjà en place) et reste valide même en ajoutant des étapes.
- Le bug “checksums présents mais faux” est traité par la nouvelle vérification hard-fail.

## 4) Ce qui reste ouvert (transparent)
- Les runs historiques déjà produits (ex: 3447) peuvent rester invalides; ce correctif protège les **nouveaux** runs.
- Les FAIL scientifiques persistants (`qmc_dmrg_*`, `external_modules_*`, `cluster_*`, `fft_dominant_amplitude`) restent un chantier P1/P2.

## 5) Commandes exactes reproductibles
```bash
git fetch --prune origin
git rev-list --left-right --count origin/main...HEAD

bash -n src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh
make -C src/advanced_calculations/quantum_problem_hubbard_hts clean all

python3 src/advanced_calculations/quantum_problem_hubbard_hts/tools/analyze_replit_run_evolution.py \
  --results-root src/advanced_calculations/quantum_problem_hubbard_hts/results \
  --runs research_20260312T075921Z_523 research_20260312T181131Z_2715 research_20260312T182101Z_2854 research_20260312T222246Z_2829 research_20260312T222838Z_3447 \
  --out-json src/advanced_calculations/quantum_problem_hubbard_hts/CHAT/reports/replit_evolution_20260312.json \
  --out-md src/advanced_calculations/quantum_problem_hubbard_hts/CHAT/RAPPORT_EVOLUTION_REPLIT_MULTI_RUNS_20260312.md

bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh
```

## 6) Checklist finale (cochée après lecture/validation réelle)
- [x] J’ai relu les anciens rapports/prompt avant de produire ce format.
- [x] J’ai fourni AVANT/APRÈS avec fichiers exacts + lignes exactes.
- [x] J’ai appliqué immédiatement une correction P0 dans le code.
- [x] J’ai validé la syntaxe shell après patch.
- [x] J’ai validé la compilation module après patch.
- [x] J’ai relancé l’analyse comparative multi-runs.
- [x] J’ai documenté ce qui reste ouvert sans le masquer.
