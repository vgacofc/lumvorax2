# Rapport de corrections techniques appliquées (phases restantes ciblées)

## Portée exécutée immédiatement
- Corrections appliquées sur les fichiers source actifs et backups explicitement signalés.
- Objectif: réduire les risques d'injection d'énergie, normaliser par site, et isoler le RNG par module.

## Tableau des corrections (avant/après)

| Fichier | Ligne originale | Ligne corrigée | Description |
|---|---|---|---|
| `backups/.../hubbard_hts_module.c` | `out.energy += pb->interaction_u * density[i] * density[i] - pb->hopping_t * fabs(fluct);` | `out.energy += (pb->interaction_u * density[i] * density[i] - pb->hopping_t * fabs(fluct)) / (double)sites;` | Normalisation énergie par site. |
| `backups/.../hubbard_hts_module.c` | `out.pairing += exp(-fabs(density[i]) * pb->temperature_k / 120.0);` | `out.pairing += exp(-fabs(density[i]) * pb->temperature_k / 120.0) / (double)sites;` | Normalisation pairing par site. |
| `backups/.../hubbard_hts_module.c` | `out.energy += burn * 1e-8;` | `/* burn metric intentionally excluded from physical energy ... */` | Suppression injection artificielle d'énergie physique. |
| `backups/.../hubbard_hts_research_cycle.c` | `r.energy += local_energy;` | `r.energy += local_energy / (double)sites;` | Normalisation énergie par site. |
| `backups/.../hubbard_hts_research_cycle.c` | `r.pairing += local_pair;` | `r.pairing += local_pair / (double)sites;` | Normalisation pairing par site. |
| `backups/.../hubbard_hts_research_cycle.c` | `r.energy += burn * 1e-8;` | `/* burn_metric kept for diagnostics only ... */` | Suppression injection d'énergie. |
| `backups/.../hubbard_hts_research_cycle.c` | `r.energy += (double)local_energy;` | `r.energy += (double)local_energy / (double)sites;` | Normalisation énergie par site (solveur indépendant). |
| `backups/.../hubbard_hts_research_cycle.c` | `r.pairing += (double)local_pair;` | `r.pairing += (double)local_pair / (double)sites;` | Normalisation pairing par site (solveur indépendant). |
| `backups/.../hubbard_hts_research_cycle.c` | `r.energy += (double)(burn * 1e-8L) / (double)sites;` | `/* burn_metric kept for diagnostics only ... */` | Suppression injection énergie (solveur indépendant). |
| `src/hubbard_hts_research_cycle.c` | seed non dérivé du module | `seed ^= seed_from_module_name(p->name);` | RNG indépendant par module (décorrélation inter-modules). |
| `src/hubbard_hts_research_cycle_advanced_parallel.c` | seed non dérivé du module | `seed ^= seed_from_module_name(p->name);` | RNG indépendant par module (runner parallèle). |
| `src/hubbard_hts_research_cycle.c` | `r.energy = (double)(step_energy + burn * 1e-8L);` | `r.energy = (double)step_energy;` | Suppression injection artificielle d'énergie dans le runner principal. |

## Checklist finale (état après ce patch)
- [x] Normalisation énergie par site (zones signalées du backup)
- [x] Normalisation pairing/observables par site (zones signalées du backup)
- [x] Réduction du risque d'injection énergétique artificielle (burn exclu de l'énergie physique dans les zones corrigées)
- [x] RNG indépendant par module (hash du nom module injecté dans la seed)
- [x] Horodatage/checksum/progression/signal de fin déjà présents dans les pipelines de run (inchangés par ce patch)
- [ ] Harmonisation complète de tout `src/` pour suffixes unités (`_eV`, `_ns`, `_m`, `_K`) non terminée sur chaque fichier historique
- [ ] Remplacement intégral de tous schémas proxy par un intégrateur strictement unitaire/symplectique domaine-par-domaine à finaliser

## Note importante
Ce patch traite immédiatement les lignes explicitement demandées et les points critiques restants les plus risqués (normalisation, burn, RNG). Une passe exhaustive de tous les fichiers historiques de `src/` reste un chantier volumineux séparé, à dérouler par lots module-par-module avec re-génération complète des artefacts.
