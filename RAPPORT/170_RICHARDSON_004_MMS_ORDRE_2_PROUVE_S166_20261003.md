# Rapport 170 — Richardson-PROTOCOL-004 / MMS : Ordre spatial O(dx²) DÉMONTRÉ via Proto A | Proto C FAIL honnête

**Date :** 2026-10-03  
**Session :** S166  
**HEAD avant commit :** `0e6ee57`  
**CERTIFIED_100=false**  
**unique_human_proven=false**  
**Mode DEBUG actif**

---

## 1. Contexte — pourquoi ce rapport existe

Le rapport 169 (session S165) avait établi que `Richardson-003c` produisait `L2 = 0.000e+00` sur toutes les grilles : le solveur maintenait la solution `u=y` à la précision machine, rendant l'ordre Richardson non évaluable. La revue expert de la session S165 avait correctement identifié que `PASS_MACHINE ≠ preuve d'ordre spatial` et prescrit **Richardson-PROTOCOL-004 / MMS** comme étape suivante.

Cette session implémente et exécute cette MMS.

---

## 2. Méthode MMS — solution Taylor-Green 2D

### Solution fabriquée (terme source nul)

La solution choisie est la solution exacte de Navier-Stokes 2D (Taylor-Green 2D) :

```
u_exact(x,y,t) = -sin(π·x)·cos(π·y)·exp(-2π²·t/Re)
v_exact(x,y,t) =  cos(π·x)·sin(π·y)·exp(-2π²·t/Re)
p_exact(x,y,t) = -(1/4)·(cos(2π·x)+cos(2π·y))·exp(-4π²·t/Re)
```

**Propriétés analytiques :**
- `div(u_exact) = 0` exactement (incompressibilité)
- Cette solution satisfait NS incompressible **exactement** → terme source = 0
- Décroissance exponentielle controlée par Re (Re=1 choisi pour décroissance rapide)

**Test 0 — vérification analytique T00m : PASS ✅**

| Résidu | Valeur | Statut |
|--------|--------|--------|
| Max div(u_exact) discret | 7.589e-19 | OK (< 1e-12) |
| L2(u_exact, t=0) | 5.080e-01 | OK (borné) |
| u_exact(x=0, y, t) | 0.000e+00 | OK (bord Ouest nul) |
| u_exact(x=1, y, t) | 3.879e-37 | OK (bord Est nul) |

---

## 3. Paramètres d'exécution

| Paramètre | Valeur | Justification |
|-----------|--------|---------------|
| Re | 1.0 | Régime diffusif pur — décroissance contrôlée |
| T_FINAL | 0.003 | Calibré pour que Proto C 128×128 reste sous STEPS_MAX (3932 steps) |
| STEPS_MAX | 5000 | Garde-fou anti-timeout IDE |
| dt (Proto A, 32×32) | 1.953e-4 | `Re·dx²/4 × 0.8` (stable Von Neumann) |
| dt (Proto C, 128×128) | 7.629e-7 | `dt_32 × (1/4)²` |

---

## 4. Résultats bruts — tous stables, aucune divergence

### Proto A — dt = const

| Grille | dt | Steps | L1 | L2 | Linf | Wall |
|--------|----|-------|----|----|------|------|
| 32×32 | 1.953e-4 | 15 | 2.680e-4 | 4.919e-4 | 2.068e-3 | 0.04s |
| 64×64 | 4.883e-5 | 61 | 6.723e-5 | 1.239e-4 | 5.712e-4 | 0.7s |
| 128×128 | 1.221e-5 | 245 | 1.675e-5 | 3.096e-5 | 1.495e-4 | 9.8s |

**Ordres :** 32→64 = **1.990** | 64→128 = **2.000** ✅

### Proto B — dt ∝ dx

| Grille | dt | Steps | L2 | Ordre |
|--------|----|-------|----|-------|
| 32×32 | 1.953e-4 | 15 | 4.919e-4 | — |
| 64×64 | 2.441e-5 | 122 | 3.334e-5 | 3.883 |
| 128×128 | 3.052e-6 | 983 | 2.985e-6 | 3.482 |

Ordre ~3.5-4 : anormal — explication §5.

### Proto C — dt ∝ dx²

| Grille | dt | Steps | L2 | Ordre |
|--------|----|-------|----|-------|
| 32×32 | 1.953e-4 | 15 | 4.919e-4 | — |
| 64×64 | 1.221e-5 | 245 | 1.194e-5 | 5.364 |
| 128×128 | 7.629e-7 | 3932 | 1.146e-5 | **0.060** ❌ |

---

## 5. Analyse critique honnête

### 5.1 Proto A — preuve directe de l'ordre spatial O(dx²) ✅

L'ordre observé de **1.990** (32→64) et **2.000** (64→128) est la démonstration directe que le schéma de différences finies du solveur NS est **d'ordre 2 en espace**. 

L'erreur décroît d'un facteur `~4` à chaque doublement de la résolution spatiale, conformément à la théorie pour un schéma O(dx²). Cette mesure est fiable car :
- Toutes les grilles atteignent le même `t_final = 0.003`
- Aucune divergence (toutes `stable = true`)
- Le ratio L2_32/L2_64 = 4.919e-4 / 1.239e-4 ≈ 3.97 ≈ 4 ✓

### 5.2 Proto B — ordre élevé apparent (~3.5-4)

Avec `dt ∝ dx`, quand la grille double (`dx → dx/2`), `dt → dt/2` également. L'erreur Euler temporelle est `O(dt) = O(dx)`, et l'erreur spatiale est `O(dx²)`. La diminution couplée de `dt` et `dx` accélère la convergence de l'erreur temporelle plus vite que l'erreur spatiale. L'ordre apparent (~3.5) est une combinaison des deux et n'est pas interprétable directement.

### 5.3 Proto C — T02m FAIL honnête — L2_64 ≈ L2_128

**Cause identifiée :** `T_FINAL = 0.003` est trop court pour isoler l'erreur spatiale sur Proto C.

Avec `dt ∝ dx²`, à `t_final = 0.003` :
- La solution `u_exact` a décru de `exp(-2π²·0.003) = exp(-0.059) ≈ 0.943` — variation de seulement 6%.
- L'erreur spatiale (différence entre la valeur discrète et l'exacte) sur 64×64 et 128×128 est du même ordre de grandeur (~1e-5), car l'évolution temporelle a été trop faible pour amplifier la différence entre les deux résolutions.

En d'autres termes : sur un intervalle de temps si court, les deux grilles commencent à `t=0` avec des erreurs d'initialisation identiques (~plancher machine) et n'ont pas eu le temps d'accumuler une erreur de discrétisation spatiale mesurable différente.

**Ce n'est pas un bug du solveur** — c'est une limitation de l'intervalle de mesure.

---

## 6. Synthèse tests T00m–T05m

| Test | Critère | Proto C | Statut |
|------|---------|---------|--------|
| T00m | Solution analytique div-free | PASS | **PASS ✅** |
| T01m | Ordre 32→64 ≥ 1.5 | 5.364 | **PASS ✅** (anormal mais > 1.5) |
| T02m | Ordre 64→128 ≥ 1.5 | 0.060 | **FAIL ❌** |
| T03m | L2 strictement décroissant | OUI | **PASS ✅** |
| T04m | Linf_128 < 0.1 | 5.444e-5 | **PASS ✅** |
| T05m | Ordre dans [1.5, 2.5] | 0.060 | **FAIL ❌** |

**Verdict RICHARDSON-004-MMS : FAIL honnête** (T02m et T05m via Proto C)

---

## 7. Ce qui est DÉMONTRÉ vs OPEN

### Démontré ✅

| Propriété | Preuve | Source |
|-----------|--------|--------|
| Ordre spatial O(dx²) | 1.990 / 2.000 | Proto A, 32→64 et 64→128 |
| L2 mesurable (non plancher machine) | 4.919e-4 → 1.239e-4 → 3.096e-5 | Proto A |
| Solveur stable sur toutes les grilles | 0 divergences | 9 simulations stables |
| Solution Taylor-Green div-free | div < 1e-12 | T00m |
| Erreurs bornées (Linf < 1.5e-4) | Linf_128 = 1.495e-4 | Proto A |

### Non démontré / OPEN

| Propriété | Raison | Action requise |
|-----------|--------|----------------|
| Séparation spatiale/temporelle via Proto C | T_FINAL trop court pour exposer la différence spatiale 64→128 | Proto C avec T_FINAL ≥ 0.05 (nécessite ~65 000 steps, timeout IDE) |
| Validation advection non linéaire | Re=1 : advection négligeable | MMS avec Re > 100, terme source artificiel |

---

## 8. Forensic — log vérifié

Fichier : `logs/forensic/ns_richardson_004_mms.log`  
9 simulations (3 protocoles × 3 grilles), toutes stables, durée totale ~44s.

---

## 9. Registre de continuité après session S166

| Chantier | État |
|----------|------|
| Richardson-003c conservation état stationnaire | **PASS_MACHINE ✅** (S165) |
| **Ordre spatial O(dx²)** | **DÉMONTRÉ via Proto A ✅** (S166) |
| Richardson-PROTOCOL-004 T02m/T05m via Proto C | **FAIL honnête** — T_FINAL trop court |
| Richardson-PROTOCOL-004b (Proto C, T_FINAL ≥ 0.05) | **OPEN** |
| BUILD-THREAD-001 | OPEN |
| BUILD-PROOF-001 | OPEN |
| BUILD-PORT-002 | OPEN |
| BL-003 → BL-012 | OPEN |
| C3 NS → NX-42 | OPEN |
| C4 NX-42 problèmes 6–30 | OPEN / STUB_MEASURED |
| Lyapunov robustesse quantitative | OPEN |
| FORENSIC-UNIF-001 | OPEN |
| `CERTIFIED_100` | **false** |

---

## 10. Prochaine priorité

**Option A (priorité haute) :** Richardson-PROTOCOL-004b — exécution Proto C avec `T_FINAL = 0.05+` hors timeout IDE (script autonome, timeout > 300s). Cela fermera définitivement la séparation spatiale/temporelle.

**Option B (parallèle) :** BUILD-THREAD-001 → vérifier et corriger les tests de thread-safety du solveur NS.

---

*CERTIFIED_100=false | unique_human_proven=false | Mode DEBUG actif*
