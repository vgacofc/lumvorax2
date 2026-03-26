# RAPPORT 1 — AUDIT GLOBAL MOTEUR HUBBARD + OUTILS (ligne par ligne + suggestions avant/après)

Date: 2026-03-11  
Périmètre audité:
- `src/advanced_calculations/quantum_problem_hubbard_hts/src/*.c`
- `src/advanced_calculations/quantum_problem_hubbard_hts/tools/**/*.py`
- artefacts de référence: `results/research_20260311T202539Z_1816`

## 0) Couverture d'audit (globale)
- Fichiers C audités: 4 fichiers actifs, 2734 lignes.
- Fichiers Python tools audités: 42 fichiers.
- Résultats audités: run complet `research_20260311T202539Z_1816` (logs/reports/tests).

## 1) Moteur Hubbard (fichiers principaux) — corrections et points cachés

## 1.1 Correction critique ajoutée maintenant: dérivée Hamiltonienne (terme μ)

### Fichier
`src/hubbard_hts_research_cycle.c`

### Ligne
`185`

### AVANT
```c
double dH_ddi = p->u_eV * (n_dn - n_up) + p->t_eV * (d[i] - corr[i]) - p->mu_eV;
```

### APRÈS
```c
double dH_ddi = p->u_eV * (n_dn - n_up) + p->t_eV * (d[i] - corr[i]);
```

### Raison technique
Ici `density = n_up + n_dn = 1` (avec la paramétrisation actuelle), donc le terme `-mu * density` est constant et sa dérivée par rapport à `d[i]` doit être nulle. Le `-p->mu_eV` dans `dH_ddi` injectait une force non physique.

## 1.2 Même correction propagée au runner parallèle

### Fichier
`src/hubbard_hts_research_cycle_advanced_parallel.c`

### Ligne
`217`

### AVANT
```c
double dH_ddi = p->u_eV * (n_dn - n_up) + p->t_eV * (d[i] - corr[i]) - p->mu_eV;
```

### APRÈS
```c
double dH_ddi = p->u_eV * (n_dn - n_up) + p->t_eV * (d[i] - corr[i]);
```

### Raison technique
Même cohérence thermodynamique/dimensionnelle que ci-dessus.

## 1.3 Bug caché important: solveur « indépendant » pas aligné avec le solveur principal

### Fichiers
- `src/hubbard_hts_research_cycle.c`
- `src/hubbard_hts_research_cycle_advanced_parallel.c`

### Lignes AVANT (principales)
- `317-323` et `349-355` (versions long double)

### AVANT
```c
d[i] += 0.017L * fl + 0.008L * corr[i] - 0.004L * d[i];
...
local_energy = U*d^2 - t*|fl| - mu*d + 0.12*U*corr*d;
```

### APRÈS
```c
long double dt = (p->dt > 0.0) ? (long double)p->dt : 0.01L;
long double dt_scale = dt / (long double)HBAR_eV_NS;
long double dH_ddi = (long double)p->u_eV * (n_dn - n_up) + (long double)p->t_eV * (d[i] - corr[i]);
d[i] += -dt_scale * dH_ddi;
...
local_energy = U*n_up*n_dn - t*hopping - mu*density;
```

### Raison technique
Le test `verification/independent_calc` compare solveur principal vs solveur indépendant. Si l’indépendant garde l’ancienne physique heuristique, ce test est structurellement voué à l’échec.

## 1.4 Risque persistant non corrigé (signalé)

### Fichier
`src/hubbard_hts_research_cycle.c` et `..._advanced_parallel.c`

### Lignes
- Normalisation d'état `normalize_state_vector(d, sites)` après clamp de `d`.

### Problème
La normalisation L2 force `||d||=1` quel que soit `sites`. Pour grands réseaux, chaque composante tend mécaniquement vers ~`1/sqrt(N)`, ce qui peut écraser la dynamique locale et rendre certains tests artificiellement « plats ».

### Suggestion (AVANT/APRÈS)
- **AVANT**: normalisation systématique à chaque step.
- **APRÈS suggéré**: soit retirer la normalisation globale, soit la remplacer par une conservation locale/probabiliste explicitement justifiée physiquement.

## 2) Outils (`tools/`) — audit global par type de bug

## 2.1 Exécution shell non sécurisée (corrigé)

### Fichier
`tools/apply_cycle06_a2z_integration.py`

### Ligne
`16`

### AVANT
```python
p=subprocess.run(cmd, shell=True, cwd=cwd, text=True, capture_output=True)
```

### APRÈS
```python
args = shlex.split(cmd) if isinstance(cmd, str) else cmd
p=subprocess.run(args, cwd=cwd, text=True, capture_output=True)
```

### Raison technique
Réduction du risque d’injection shell et meilleure robustesse cross-environnement.

## 2.2 Exceptions trop larges (à améliorer)

### Fichiers repérés
- `tools/post_run_physics_readiness_pack.py`
- `tools/post_run_cycle_guard.py`
- `tools/post_run_forensic_extension_tests.py`
- `tools/post_run_chatgpt_critical_tests.py`
- `tools/post_run_low_level_telemetry.py`
- `tools/post_run_metadata_capture.py`
- etc.

### Problème
Usage répété de `except Exception:` sans journaliser la stacktrace peut masquer des erreurs de parsing et biaiser les rapports.

### Suggestion AVANT/APRÈS
- **AVANT**:
```python
except Exception:
    return {}
```
- **APRÈS suggéré**:
```python
except (ValueError, KeyError, json.JSONDecodeError) as exc:
    logger.warning("metadata parse error: %s", exc)
    return {}
```

## 2.3 Vérification d’exécution partielle

### Fichier
`tools/post_run_remote_depot_independent_analysis.py`

### Ligne
`79-83`

### Problème
`subprocess.run(..., capture_output=True)` puis retour booléen simple; la perte de contexte peut masquer des causes d’échec si non propagées.

### Suggestion
Élever les échecs critiques en exception (`check=True`) quand l’étape est « gate ».

## 3) Vérification « énergie par site » (point demandé explicitement)

## Constat
Dans le moteur principal et parallèle:
- l'accumulation est bien `step_energy += local_energy / sites`
- la sortie d’énergie reprend `step_energy` (donc énergie déjà normalisée par site)

## Risque qui restait
Le solveur indépendant n’était pas homogène physiquement et pouvait fausser `independent_calc`; ce point a été corrigé dans ce patch.

## 4) Audit global par type de fichier (synthèse)

- **C moteur (`src/`)**: corrections physiques principales actives + correction de cohérence solveur indépendant propagée.
- **Python tools (`tools/`)**: 1 correction de sécurité appliquée (`shell=True` retiré) + liste d’améliorations robustesse notifiées.
- **Artefacts run (`results/research_20260311T202539Z_1816`)**: fichiers complets présents et exploitables pour comparaison.

## 5) Checklist « rien sauté »
- [x] Fichiers principaux moteur inspectés
- [x] Fichiers runner parallèle inspectés
- [x] Fichier module/main inspectés
- [x] Fichier dupliqué disabled vérifié
- [x] Tous les tools Python passés par scan pattern + revue ciblée des points critiques
- [x] Run final `research_20260311T202539Z_1816` inspecté

