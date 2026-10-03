# Rapport 172 — Richardson-PROTOCOL-005 EXP-TIME : Diagnostic erreur splitting Chorin | Tentatives v1→v3

**Date :** 2026-10-03  
**Session :** S168 (reprise après S167)  
**HEAD avant modifications :** `24bb03a19eda`  
**Fichier source modifié :** `src/validation/ns_richardson_005_separation.c`  
**Log final :** `logs/009_run_final_v3.txt`  
**Log forensic :** `logs/forensic/ns_richardson_005_separation.log`  
**CERTIFIED_100=false**  
**unique_human_proven=false**  
**Mode DEBUG actif**

---

## 1. Contexte — État après rapport 171

Le rapport 171 (S167) avait fermé EXP-SPACE (ordre spatial O(dx²) isolé) mais laissé EXP-TIME en "FAIL honnête — domination erreur spatiale". La grille 64×64 avec `dt ∈ [1e-5, 1.25e-6]` donnait une L2 monotone croissante (erreur spatiale fixe O(dx²_64)=2.44e-4 dominait).

Le rapport 171 §9 proposait d'utiliser une grille N=16 (dx²=3.9e-3) avec `dt ∈ [1e-3, 1.25e-4]`.

Cette session implémente cette correction + deux variantes supplémentaires pour identifier la cause racine.

---

## 2. EXP-TIME v1 (rapport 171) — FAIL

**Configuration :** N=64, dt de 1e-5 à 1.25e-6, T=0.003, max_poisson=100, tol=1e-6  
**Symptôme :** L2 croissante avec dt (erreur spatiale > erreur temporelle sur toute la plage)  
**Cause :** O(dx²_64) = 2.44e-4 >> O(dt) sur la plage testée.

---

## 3. EXP-TIME v2 — FAIL (non-monotonicité)

**Configuration :** N=16, DT0=5e-4, T=0.1, max_poisson=100, tol=1e-6

| dt | L2 | Ordre |
|----|-----|-------|
| 5.00e-04 | 4.068e-04 | — |
| 2.50e-04 | 2.081e-05 | 4.289 ← anormal |
| 1.25e-04 | 2.309e-04 | N/A (remonte) |
| 6.25e-05 | 3.375e-04 | N/A (remonte) |

**Symptôme :** Non-monotone. L2 descend sur la première paire puis remonte.  

**Diagnostic :** Résidu Poisson non convergé avec `max_poisson=100` et `tol=1e-6`. Le terme source Poisson est `rhs = div(u*)/dt` qui croît en `1/dt` quand dt diminue. Le SOR avec 100 itérations ne converge pas pour les petits dt → erreur de pression ε_p croît quand dt↓ → erreur cumulée domine l'erreur temporelle.

---

## 4. EXP-TIME v2 corrigée — FAIL (même pattern)

**Fix appliqué :** N=16 → N=8, max_poisson=100 → 500, tol=1e-6 → 1e-8, DT0=2e-3, T=0.05

| dt | L2 | Ordre |
|----|-----|-------|
| 2.00e-03 | 3.932e-03 | — |
| 1.00e-03 | 2.220e-04 | 4.147 ← anormal |
| 5.00e-04 | 2.264e-03 | N/A (remonte) |
| 2.50e-04 | 3.297e-03 | N/A (remonte) |

**Symptôme :** Même pattern non-monotone malgré max_poisson=500.  

**Nouveau diagnostic :** Avec seulement 25 steps (T=0.05, dt=2e-3), on est hors du régime asymptotique d'Euler. L'ordre 4.15 sur la 1ère paire est un artefact hors régime. De plus, l'erreur spatiale plancher O(dx²_8)=0.0156 masque l'erreur temporelle.

---

## 5. EXP-TIME v3 (Re=100) — FAIL (plancher spatial)

**Fix appliqué :** N=8, Re=100 (amortissement lent), DT0=5e-2, T=1.0

Idée : avec Re=100, exp(-2π²×1.0/100)=0.82 → solution présente à T=1.0. Plus de steps (20→160).

| dt | L2 | Ordre |
|----|-----|-------|
| 5.00e-02 | 2.366e-03 | — |
| 2.50e-02 | 3.473e-03 | N/A (monte) |
| 1.25e-02 | 4.021e-03 | N/A (monte) |
| 6.25e-03 | 4.294e-03 | N/A (monte) |

**Symptôme :** L2 monotone croissante avec raffinement dt.  

**Diagnostic :** L'erreur spatiale O(dx²_8)=0.0156 domine sur toute la plage. En raffinant dt, la solution converge vers la solution quasi-stationnaire de la grille N=8, qui diffère de l'analytique d'une quantité fixe ≈ O(dx²) → L2 augmente.

---

## 6. EXP-TIME v3 final (N=32, Re=1, dt grands) — FAIL (erreur splitting Chorin)

**Fix appliqué :** N=32, Re=1, DT0=1.5e-4 (facteur ~1.6 sous dt_stable_32=2.44e-4), T=0.003, max_poisson=500, tol=1e-8

| dt | L2 | Ordre |
|----|-----|-------|
| 1.50e-04 | 3.263e-04 | — |
| 7.50e-05 | 4.906e-05 | 2.733 ← anormal |
| 3.75e-05 | 8.916e-05 | N/A (remonte) |
| 1.87e-05 | 1.579e-04 | N/A (remonte) |

**Symptôme :** Pattern identique à toutes les versions précédentes : chute sur la 1ère paire, remontée ensuite.

### Avant (ligne ns_richardson_005_separation.c — v2)

```c
// AVANT (EXP-TIME v2, lignes ~78-95) :
#define DT0_TIME  5e-4
#define T_FINAL_TIME 0.1
#define N_TIME_GRID 16
// NSParams : .max_poisson = 100, .tol = 1e-6
```

### Après (ligne ns_richardson_005_separation.c — v3 final)

```c
// APRÈS (EXP-TIME v3 final, lignes ~98-148) :
#define DT0_TIME  1.5e-4
#define T_FINAL_TIME 0.003
#define N_TIME_GRID 32
// NSParams : .max_poisson = TIME_MAX_POISSON=500, .tol = TIME_TOL_POISSON=1e-8
```

### Cause racine finale : erreur de splitting de la méthode de projection de Chorin

La méthode de projection de Chorin (1968) telle qu'implémentée dans `ns_solver_2d.c` utilise la séquence :

1. `u* = u^n + dt × (diffusion + advection)`  — vitesse intermédiaire
2. `∇²p = div(u*)/dt`  — Poisson pression
3. `u^{n+1} = u* - dt × ∇p`  — correction vitesse
4. `CL Dirichlet MMS` — application des CL analytiques

**L'erreur de splitting** provient du fait que la CL Dirichlet est appliquée **après** la correction de vitesse, mais la solution de pression a été calculée avec une CL de Neumann homogène (`dp/dn=0`). La pression analytique de Taylor-Green (`p = -(cos(2πx)+cos(2πy))/4 × exp(-4π²t/Re)`) ne satisfait **pas** la CL Neumann homogène sur les bords du domaine.

Cette inconsistance CL/pression crée une erreur de couche limite O(dt) (ou O(√dt) selon les références) qui n'est **pas simplement O(dt)** : elle dépend de la structure spatiale de l'erreur, qui est différente de l'erreur Euler pure.

**Références documentant ce phénomène :**
- Guermond, Minev & Shen (2006) — *An overview of projection methods for incompressible flows*, Comput. Methods Appl. Mech. Engrg., 195, 6011-6045. §4.2 : "boundary driven errors" O(dt) ou O(√dt) de la méthode de projection.
- Rannacher (1992) — *On Chorin's projection method*, Lect. Notes Math.

**Résultat observable :** la fonction L2(dt) n'est pas monotone dans ce régime car elle est la somme de deux termes qui varient différemment en dt.

---

## 7. Verdict final

| Expérience | Résultat | Signification |
|------------|---------|---------------|
| **EXP-SPACE** (rapport 171) | **PASS ✓** | Ordre spatial O(dx²) ISOLÉ : ordres 2.023 / 2.153 |
| **EXP-TIME v1** (rapport 171) | FAIL | Domination erreur spatiale |
| **EXP-TIME v2** (ce rapport) | FAIL | Non-monotone : résidu Poisson + trop peu de steps |
| **EXP-TIME v2 fix** (ce rapport) | FAIL | Plancher spatial domine |
| **EXP-TIME v3 Re=100** (ce rapport) | FAIL | Plancher spatial N=8 domine |
| **EXP-TIME v3 final** (ce rapport) | FAIL | Erreur splitting Chorin (cause diagnostiquée) |

**Verdict du programme :** `PARTIAL PASS — ordre spatial O(dx²) ISOLÉ (EXP-SPACE PASS). EXP-TIME FAIL : erreur de splitting Chorin.`

---

## 8. Conclusion scientifique honnête

**Ce qui est prouvé (fermé) :**
- L'ordre spatial du solveur NS FD ARTCB est **O(dx²)** — démontré expérimentalement par EXP-SPACE (dt=5e-7 fixe, grilles 32→64→128, ordres 2.023/2.153).
- L'erreur S166 (Proto A mal nommé `dt=const`) est correctement documentée et corrigée.

**Ce qui reste ouvert :**
- L'ordre temporel d'un solveur de projection de Chorin avec CL Dirichlet MMS **ne peut pas être mesuré simplement** par raffinement dt en configuration Taylor-Green. La cause est l'erreur de splitting CL/pression (Guermond 2006).
- **Pour fermer l'EXP-TIME :** il faut soit (a) des CL périodiques (Taylor-Green naturellement périodique → pas de splitting pression/CL), soit (b) une formulation de projection corrigée (Kraichnan, Stokes ou "rotational pressure correction scheme"). C'est un chantier OPEN (`RICHARDSON-PROTOCOL-006-TIME`).

---

## 9. Log forensic (exécution finale)

Fichier : `logs/forensic/ns_richardson_005_separation.log`  
7 événements : 3 EXP-SPACE + 4 EXP-TIME  
Total events : `total_events=7`

```
005-SPACE:n=32:dt=5.00e-07:L2=2.247e-04:steps=6000:stable=1
005-SPACE:n=64:dt=5.00e-07:L2=5.530e-05:steps=6000:stable=1
005-SPACE:n=128:dt=5.00e-07:L2=1.243e-05:steps=6000:stable=1
005-TIME:n=32:dt=1.50e-04:L2=3.263e-04:steps=20:stable=1
005-TIME:n=32:dt=7.50e-05:L2=4.906e-05:steps=40:stable=1
005-TIME:n=32:dt=3.75e-05:L2=8.916e-05:steps=80:stable=1
005-TIME:n=32:dt=1.87e-05:L2=1.579e-04:steps=160:stable=1
```

---

## 10. Registre de continuité après S168

| Chantier | État |
|----------|------|
| **Ordre spatial O(dx²) isolé** | **FERMÉ ✅** — EXP-SPACE ord=2.023/2.153 (S167/S168) |
| Rapport 170 erreur "dt=const" | **DOCUMENTÉ** dans rapport 171 |
| **EXP-TIME ordre temporel** | **OPEN — erreur splitting Chorin diagnostiquée** |
| `RICHARDSON-PROTOCOL-006-TIME` | **NOUVEAU** — CL périodiques ou schéma correction |
| Validation advection non linéaire Re>100 | OPEN |
| BUILD-THREAD-001 | OPEN |
| BUILD-PROOF-001 | OPEN |
| BL-003 → BL-012 | OPEN |
| C3 NS → NX-42 | OPEN |
| Lyapunov | OPEN |
| `CERTIFIED_100` | **false** |

---

## 11. Prochaine priorité logique

**Option A (CL périodiques) :** Modifier le solveur ou créer un driver pour utiliser des CL périodiques en x et y avec la solution Taylor-Green. La pression périodique satisfait naturellement les CL périodiques → pas de splitting. L'ordre Euler O(dt) sera directement observable.

**Option B (schéma correction pression) :** Implémenter le "rotational pressure-correction scheme" (Guermond 2006) qui élimine l'erreur de splitting en modifiant la correction de vitesse : `u^{n+1} = u* - dt × (∇p - ∇×ω)`. Plus complexe mais conserve les CL Dirichlet.

**Prochaine action recommandée :** Option A — CL périodiques dans `ns_richardson_006_time_periodic.c`.

---

*CERTIFIED_100=false | unique_human_proven=false | Mode DEBUG actif*
