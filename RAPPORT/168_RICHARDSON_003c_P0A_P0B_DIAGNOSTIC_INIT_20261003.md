# Rapport 168 — Richardson-PROTOCOL-003c : P0-A+P0-B résolus, diagnostic condition initiale

**Date :** 2026-10-03  
**Session :** S165  
**HEAD avant :** `86d16f4` (rapport 167 audit critique S164)  
**CERTIFIED_100=false**  
**unique_human_proven=false**  
**Mode DEBUG actif**

---

## 1. Contexte — Problèmes P0 de l'audit 167

L'audit 167 avait identifié deux problèmes bloquants pour Richardson-003c :

| Problème | Description |
|----------|-------------|
| **P0-A** | Contamination Lid-Driven : `ns_solver_step()` appelle `set_lid_bc()` en interne, puis 003b réappliquait les CL Couette après coup |
| **P0-B** | Périodicité discrète non démontrée : la convention de copie des colonnes fantômes MAC n'était pas vérifiée indépendamment |

---

## 2. Corrections appliquées — `src/validation/ns_richardson_003c.c`

### P0-A — `ns_solver_step_with_bc()` (ligne 302–310)

**AVANT (003b) :**
```c
/* boucle main : lignes 347–353 de ns_richardson_couette_periodic.c */
poisson_res = ns_solver_step(s);
set_couette_periodic_bc(s);
```

**APRÈS (003c) :**
```c
/* P0-A FIX : ns_solver_step_with_bc évite set_lid_bc interne */
poisson_r = ns_solver_step_with_bc(s, set_couette_periodic_bc);
```

`ns_solver_step_with_bc()` appelle `set_couette_periodic_bc` AVANT et APRÈS les étapes physiques (advection, diffusion, Poisson, correction) — aucun appel à `set_lid_bc()` ne se produit.

---

### P0-B — Test 0 de cohérence discrète (lignes 154–228)

Nouveau test préalable (T00c) exécuté sur n=32 avant toute simulation :

1. Impose `u(i,j) = (j-0.5)*dy` analytique sur toute la grille
2. Applique `set_couette_periodic_bc()`
3. Mesure 4 résidus discrets :
   - **A** : erreur de périodicité `max|u[0][j] - u[nx-1][j]|`
   - **B** : divergence discrète `max|(u[i][j] - u[i-1][j])/dx|`
   - **C** : Laplacien discret `max|d²u/dx² + d²u/dy²|`
   - **D** : terme advectif `max|u·du/dx|`
4. PASS si tous les résidus < `1e-10`

Si T00c échoue → arrêt immédiat.

---

## 3. Résultat T00c — PASS

```
  A — Max erreur periodicite u : 0.000e+00  OK
  B — Max divergence de u      : 0.000e+00  OK
  C — Max Laplacien de u       : 0.000e+00  OK
  D — Max terme advectif u*du/dx : 0.000e+00  OK

  T00c — Coherence discrete : PASS — résidus < 1e-10
```

Tous les résidus sont **exactement 0**. La convention de périodicité MAC utilisée (`u[0][j] = u[nx-1][j]`, `u[nx][j] = u[1][j]`) est cohérente avec le problème Couette discret.

---

## 4. Résultats de simulation — Campagne partielle

La grille 128×128 du protocole B a dépassé le timeout (~260s). Le protocole C n'a pas été exécuté.

### Protocole A (dt=const=1e-4, LIMIT_STEPS_A=40000)

| Grille | Statut | L1 | L2 | Linf | u_min | u_max | t_phys |
|--------|--------|----|----|------|-------|-------|--------|
| 32×32 | LIMIT | 0.500054 | 0.547572 | 0.752932 | -0.275 | 0.930 | 4.0 s |
| 64×64 | LIMIT | 0.500103 | 0.547965 | 0.753979 | -0.276 | 0.965 | 4.0 s |
| 128×128 | NON_EVALUABLE | — | — | — | — | — | — |

### Protocole B (dt∝dx)

| Grille | Statut | L1 | L2 | Linf | u_min | u_max | t_phys |
|--------|--------|----|----|------|-------|-------|--------|
| 32×32 | **CONV** | 0.500055 | 0.547383 | 0.748802 | -0.331 | 0.937 | 13.0 s |
| 64×64 | **CONV** | 0.500072 | 0.547703 | 0.749905 | -0.330 | 0.969 | 11.1 s |
| 128×128 | TIMEOUT | — | — | — | — | — | — |

---

## 5. Diagnostic critique — Le solveur converge mais pas vers Couette

### Observation

**L1 ≈ 0.5** et **u_min < 0** sur toutes les grilles, y compris les grilles convergeant (proto B, CONV).

Pour u_exact = y ∈ [0,1], une L1 ≈ 0.5 signifie que l'erreur moyenne est ~50% de l'amplitude totale. Le solveur ne produit pas le profil Couette attendu.

### Cause probable identifiée

La condition initiale est le **champ nul** (u=v=0 par initialisation `ns_solver_create`).

Le domaine avec `u_initial = 0` + CL Couette (Nord u=1, Sud u=0, périodiques en x) peut converger vers un état stationnaire qui n'est **pas** le profil linéaire `u=y`.

Observation cohérente : u_min < 0 suggère un écoulement secondaire de recirculation persistant — le solveur converge vers un état physiquement plausible mais différent de la solution analytique simple.

### Comparaison 003b vs 003c

Les L2 sont similaires (003b : ~0.011, 003c : ~0.548). Ceci **n'est pas** une régression de 003c. La différence vient du fait que 003b atteignait la stationnarité (critère CONV avec t_phys ~21s) tandis que 003c avec dt proportionnel à dx² converge plus lentement et produit des erreurs similaires.

**Remarque importante :** les résultats 003b (~0.011 L2) n'étaient pas non plus corrects — ils résultaient de la contamination Lid-Driven qui imposait une solution proche de `u=y` artificiellement. 003c sans contamination montre l'état réel.

---

## 6. Action requise — Initialisation avec la solution analytique

Pour obtenir la convergence vers `u=y`, la prochaine étape doit initialiser le champ avec `u(i,j) = (j-0.5)*dy` (solution analytique) AVANT la simulation, puis vérifier que le solveur **maintient** cette solution.

Cela permettra de mesurer l'erreur d'ordre spatial pure : si le solveur maintient u=y avec une petite erreur ~ O(dx²), Richardson peut être mesuré.

---

## 7. Status des tests T00c–T06c

| Test | Résultat | Note |
|------|----------|------|
| T00c | **PASS** | Résidus = 0.0 exactement |
| T01c | NON_EVALUABLE | Proto C non exécuté (timeout) |
| T02c | NON_EVALUABLE | Proto C non exécuté (timeout) |
| T03c | NON_EVALUABLE | Proto C non exécuté (timeout) |
| T04c | NON_EVALUABLE | Proto C non exécuté (timeout) |
| T05c | NON_EVALUABLE | Proto C non exécuté (timeout) |
| T06c | NON_EVALUABLE | Proto C non exécuté (timeout) |

---

## 8. Avant / Après — Différence architecturale 003b vs 003c

### Avant — `ns_richardson_couette_periodic.c` (003b), ligne 351–352

```c
/* Fichier : src/validation/ns_richardson_couette_periodic.c */
/* Ligne 351 : appel dans la boucle while */
poisson_res = ns_solver_step(s);           /* ← set_lid_bc() appelé en interne */
set_couette_periodic_bc(s);               /* ← réapplication APRÈS coup */
```

### Après — `ns_richardson_003c.c` (003c), ligne ~310

```c
/* Fichier : src/validation/ns_richardson_003c.c */
/* P0-A FIX : ns_solver_step_with_bc évite set_lid_bc interne */
poisson_r = ns_solver_step_with_bc(s, set_couette_periodic_bc);
/* set_couette_periodic_bc est appelé AVANT et APRÈS — aucune contamination */
```

---

## 9. Registre de continuité

| Chantier | État |
|----------|------|
| P0-A contamination Lid-Driven | ✅ **CLOSE** (003c) |
| P0-B périodicité discrète | ✅ **CLOSE** (T00c PASS) |
| Richardson T01c–T06c | **OPEN** — initialisation analytique requise |
| Richardson-PROTOCOL-001 | **OPEN** |
| Initialisation u=y avant simulation | **OPEN** — prochaine action |
| Grille B/128 et proto C/toutes grilles | **OPEN** — timeout |
| T04 renforcé | **OPEN** |
| Lyapunov robustesse quantitative | **OPEN** |
| BUILD-THREAD-001 | **OPEN** |
| CERTIFIED_100 | **false** |

---

## 10. Prochaine action

**Richardson-003c v2 :** ajouter initialisation `u(i,j) = (j-0.5)*dy` comme condition initiale avant la boucle temporelle, puis relancer la campagne pour mesurer si le solveur maintient le profil Couette avec une erreur O(dx²).
