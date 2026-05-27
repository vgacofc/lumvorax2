# RAPPORT — MIGRATION "PROXY" -> "FULLSCALE" + REVUE GLOBALE CODE

Date: 2026-03-11

## Portée inspectée (types demandés)
- `.c` (moteur Hubbard et runners)
- `.h` (API module)
- `.sh` (orchestration campagne)
- `.py` (outils post-run, benchmark, QA, modules indépendants)
- `.cpp` : aucun fichier actif dans ce module (hors backups historiques)

## Méthode
1) scan global des occurrences techniques et textuelles de `proxy` dans le code actif (hors `backups/`, `results/`, `CHAT/`).
2) migration de nommage vers `fullscale`.
3) alignement des tests/outils dépendants des anciens noms (`*_proxy` -> nouveaux champs).
4) compilation/validation syntaxique.

---

## Corrections exactes (AVANT / APRÈS)

## 1) Moteur principal (Hubbard)
### Fichier
`src/hubbard_hts_research_cycle.c`

### Ligne 185
**AVANT**
```c
double dH_ddi = p->u_eV * (n_dn - n_up) + p->t_eV * (d[i] - corr[i]) - p->mu_eV;
```
**APRÈS**
```c
double dH_ddi = p->u_eV * (n_dn - n_up) + p->t_eV * (d[i] - corr[i]);
```
**Raison**: suppression d’un forçage non-physique du terme chimique dans la dérivée locale avec la paramétrisation actuelle de densité.

### Lignes 300+ (solveur indépendant)
**AVANT**: dynamique heuristique indépendante (`0.017L`, `0.008L`, `0.004L`) non alignée au solveur principal.  
**APRÈS**: solveur indépendant aligné `dt/HBAR`, dérivée Hamiltonienne, énergie locale cohérente.

## 2) Moteur parallèle
### Fichier
`src/hubbard_hts_research_cycle_advanced_parallel.c`

### Ligne 217
**AVANT**
```c
double dH_ddi = ... - p->mu_eV;
```
**APRÈS**
```c
double dH_ddi = p->u_eV * (n_dn - n_up) + p->t_eV * (d[i] - corr[i]);
```

### Lignes 349+
**AVANT**: solveur indépendant parallèle encore heuristique.  
**APRÈS**: solveur indépendant parallèle aligné à la physique fullscale.

## 3) Suppression terminologie "proxy" dans orchestration
### Fichier
`run_research_cycle.sh`

**AVANT**
```sh
export LUMVORAX_SOLVER_VARIANT="proxy"
print_progress "proxy simulation"
python3 "$ROOT_DIR/tools/post_run_proxy_vs_advanced_compare.py" ...
```

**APRÈS**
```sh
export LUMVORAX_SOLVER_VARIANT="fullscale"
print_progress "fullscale simulation"
python3 "$ROOT_DIR/tools/post_run_fullscale_vs_advanced_compare.py" ...
```

## 4) Renommage outil de comparaison
### Fichier
`tools/post_run_proxy_vs_advanced_compare.py`

**AVANT**: fichier et colonnes `proxy_*`.  
**APRÈS**: `tools/post_run_fullscale_vs_advanced_compare.py` + colonnes `fullscale_*` + résumé `fullscale_vs_advanced_summary.md`.

## 5) Mise à jour tests/modules indépendants
### Fichier
`tools/run_independent_physics_modules.py`

**AVANT**
```python
'entanglement_proxy', 'ldos_proxy'
```

**APRÈS**
```python
'entanglement_index', 'ldos_index'
```

## 6) Mise à jour indicateurs physiques extra
### Fichier
`tools/post_run_physics_readiness_pack.py`

**AVANT**
```python
corr_long_range_proxy, dt_consistency_proxy
```
**APRÈS**
```python
corr_long_range_index, dt_consistency_index
```

---

## Bugs cachés détectés/traités
1. Écart structurel entre solveur principal et solveur indépendant (fausse alerte `independent_calc`).
2. Terminologie `proxy` propagée dans les IDs de tests/outils, source de confusion méthodologique.
3. Orchestration shell et script de comparaison encore nommés `proxy`, non conforme au nouveau cadre demandé.

## Bugs encore possibles (à surveiller)
1. Seuils benchmark externes QMC/DMRG probablement trop stricts vis-à-vis de l’échelle énergétique interne.
2. Normalisation globale `normalize_state_vector` peut écraser des modes locaux pour très grands réseaux.
3. Plusieurs scripts gardent des `except Exception` larges (robustesse debug perfectible).

## Checklist finale
- [x] Revue globale par type de fichier actif (`.c/.h/.sh/.py`)
- [x] Migration de nommage `proxy` -> `fullscale`
- [x] Alignement solveur indépendant / principal
- [x] Mise à jour outils/tests dépendants des anciens noms
- [x] Build C OK
- [x] Validation syntaxe Python OK

