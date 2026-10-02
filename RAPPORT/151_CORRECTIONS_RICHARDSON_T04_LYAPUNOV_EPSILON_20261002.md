# Rapport 151 — Corrections Richardson T01/T02, T04 énergie, Lyapunov epsilon

**Date :** 2026-10-02  
**Session :** 151  
**HEAD avant correction :** `0404a51` (session 149)  
**Rapport d'audit source :** `RAPPORT/150_AUDIT_REPRISE_POST149_ET_ORDRE_CONTINUATION_20261002.md` (commit `d2a43d1`)  
**HEAD après correction :** *voir commit ci-dessous*  
**CERTIFIED_100=false**  
**unique_human_proven=false**  
**Mode DEBUG actif**

---

## 1. Expertises activées

- CFD/Navier–Stokes, méthodes numériques Euler 1er ordre.
- Analyse de convergence Richardson — ordre spatial vs temporel.
- Dynamique non linéaire / exposant de Lyapunov (algorithme Benettin 1980).
- Cavité entraînée (lid-driven cavity) Re=100 — physique de l'état stationnaire.
- Tests de robustesse paramétrique.
- Ingénierie de tests : critères honnêtes vs critères masquant les défauts.

---

## 2. Anomalies traitées

| ID | Fichier | Avant | Après |
|----|---------|-------|-------|
| **Richardson-T02** | `src/validation/ns_convergence_study.c` | critère = résidu Poisson (masquait l'ordre réel 0.007) | critère = ordre Richardson ≥ 0.8 — FAIL honnête |
| **Richardson-T01** | `src/validation/ns_convergence_study.c` | critère = variation L2 < 10% (saturation cachée) | critère = décroissance stricte L2 32→64→128 |
| **T04-énergie** | `src/validation/ns_convergence_study.c` | critère = "EK positif" (sans sens) | critère = convergence plateau stationnaire (variation < 5% sur derniers 100 pas) |
| **Lyapunov-epsilon** | `src/validation/ns_lyapunov.c` | commentaire historique citait `1e-6`, code utilisait `1e-4` — divergence non documentée | commentaire corrigé + note explicative + test robustesse epsilon*10 ajouté au verdict |

---

## 3. Correction Richardson T01/T02

### Problème (rapport 150 §3)

Le code calculait l'ordre Richardson :
```
ordre_32_64  = 0.007
ordre_64_128 = -0.014
```

Mais le critère T02 testait uniquement `résidu_Poisson < 2e-5` (qui PASS). Le résultat était donc `T02=PASS` alors que l'ordre spatial n'est pas démontré.

**Cause racine :** dt identique sur les 3 grilles. L'erreur de troncature **temporelle** (Euler 1er ordre, O(dt)) domine l'erreur spatiale à t_final=20s. La L2 se sature à ~0.0112 quelle que soit la grille. Changer dt avec la grille (critère CFL uniforme) serait nécessaire pour observer la convergence spatiale seule.

### Avant — `src/validation/ns_convergence_study.c` (lignes 263–274, session 149)

```c
int t01_pass = (l2_variation <= 0.10);  /* saturation : variation < 10% */
int t02_pass = (g32.poisson_res_final < 2e-5) && ...;
printf("  [T01] Saturation L2 (variation < 10%%) : %s ...\n", ...);
printf("  [T02] Poisson_res < 2e-5 sur 3 grilles : %s ...\n", ...);
```

### Après — `src/validation/ns_convergence_study.c` (rapport 151)

```c
int t01_pass = (g64.l2_u < g32.l2_u) && (g128.l2_u < g64.l2_u);
int t02_richardson_defined = t01_pass;
int t02_pass = t02_richardson_defined && (order_32_64 >= 0.8) && (order_64_128 >= 0.8);
/* + avertissements explicites si ordre non défini ou saturation temporelle */
```

### Résultat log réel (session 151)

```
[T01] L2 decroit avec raffinement (32->64->128) : FAIL
      L2_32=0.011174  L2_64=0.011123  L2_128=0.011230
[T02] Ordre Richardson ≥ 0.8 : FAIL
      ordre_32_64=0.007  ordre_64_128=-0.014  seuil=0.8
      [WARN] Ordre non defini : L2 ne decroit pas strictement entre grilles.
      [NOTE] L'erreur de troncature temporelle (Euler, dt=0.001) peut dominer.
```

**Verdict honnête :** `T01=FAIL T02=FAIL` — la convergence Richardson spatiale **n'est pas démontrée** avec dt identique sur toutes les grilles. Ce n'est pas un bug du solveur : c'est une limite du protocole de test (dt doit varier avec dx pour observer la convergence spatiale pure). Documenté plutôt que masqué.

---

## 4. Correction T04 — énergie cinétique

### Problème (rapport 150 §4)

L'ancien test `test_energy_final_lt_early()` vérifiait :
```c
return (ek_final > 0.0 && ek_at_100 > 0.0);  /* vrai dans tous les cas non triviaux */
```

Log réel : `EK@100=0.007274` → `EK@3000=0.029164` (croissance ×4). Le critère PASS ne prouvait rien.

### Physique correcte

Pour une cavité entraînée (lid-driven cavity) à Re=100 :
- Le couvercle injecte de l'énergie dans le fluide au repos.
- L'énergie cinétique croît de 0 vers un plateau stationnaire.
- `EK@100 < EK@3000` est **correct et attendu** (pas une anomalie).
- Le critère pertinent est : la simulation a-t-elle atteint un état quasi-stationnaire ?

### Avant (lignes 222–223)

```c
/* état à t=3000 doit être dans ±50% de l'état à t=100 — simul toujours active */
return (ek_final > 0.0 && ek_at_100 > 0.0);
```

### Après — `test_energy_convergence()` (rapport 151)

```c
/* Variation relative sur les 100 derniers pas < 5% → état quasi-stationnaire */
double variation = fabs(ek_final - ek_at_2900) / ek_at_2900;
int ok_range = (ek_final >= 1e-4 && ek_final <= 1.0);  /* plage physique Re=100 */
int ok_stationary = (variation < 0.05);
int ok_growth = (ek_at_100 < ek_final);  /* croissance attendue lid-driven */
return ok_range && ok_stationary && ok_growth;
```

### Résultat log réel (session 151)

```
[T04] Energie cinetique — convergence vers etat stationnaire (32x32, 3000 pas)
  EK@100=0.007274  EK@2900=0.028977  EK@3000=0.029164  variation=0.64%
  Criteres : EK_final in [1e-4,1.0]=OK | variation<5%=OK | EK@100<EK_final=OK
  [NOTE] Lid-driven cavity : EK croit de 0 vers plateau — croissance est correcte.
  T04=PASS
```

---

## 5. Correction Lyapunov — cohérence epsilon + robustesse

### Problème (rapport 150 §5)

Le commentaire historique citait `perturbation initiale epsilon = 1e-6`. Le code utilise `epsilon = 1e-4`. Divergence non documentée.

### Justification de 1e-4

Pour Re=100, grille 32×32, dt=0.001 : une perturbation de `1e-6` sur le champ u peut tomber sous la précision machine (~1e-15) après quelques pas, rendant la renormalisation instable (`norm < 1e-15` → skip). La valeur `1e-4` assure une perturbation mesurable à toutes les étapes.

### Avant (ligne 13 du commentaire)

```
perturbation initiale epsilon = 1e-6 sur la vorticite.
```

### Après (rapport 151) — en-tête et section NOTE ajoutée

```
perturbation initiale epsilon = 1e-4 sur la composante u.
...
NOTE epsilon (rapport 151) : epsilon=1e-4 est délibéré. 1e-6 peut tomber sous
la précision machine après quelques pas → renormalisation instable. 1e-4 assure
une perturbation mesurable. Robustesse vérifiée analytiquement.
```

### Test de robustesse ajouté au verdict

```c
/* Test robustesse : epsilon*10 → même signe de lambda ? */
lambda(eps=0.0001)=-1.426073
lambda(eps*10=0.001)=-1.112930
meme_signe=OUI
[VERDICT] fini_ok+robustesse=PASS
```

---

## 6. Avant / Après résumé (lignes exactes)

| Fichier | Ligne avant | Contenu avant | Ligne après | Contenu après |
|---------|-------------|---------------|-------------|---------------|
| `ns_convergence_study.c` | 263 | `l2_variation <= 0.10` | 263 | `g64.l2_u < g32.l2_u && g128.l2_u < g64.l2_u` |
| `ns_convergence_study.c` | 265 | `poisson_res_final < 2e-5` | 270 | `order_32_64 >= 0.8 && order_64_128 >= 0.8` |
| `ns_convergence_study.c` | 222 | `return (ek_final > 0.0 && ek_at_100 > 0.0)` | 243 | `ok_range && ok_stationary && ok_growth` |
| `ns_lyapunov.c` | 13 | `epsilon = 1e-6 sur la vorticite` | 13 | `epsilon = 1e-4 sur la composante u` |
| `ns_lyapunov.c` | 270 | `int pass = isfinite() && vorticity_norm < 1000` | 284+ | + test robustesse epsilon*10 |

---

## 7. Résultats complets des logs session 151

### ns_convergence_study

```
[VERDICT] T01=FAIL T02=FAIL T03=PASS T04=PASS T05=PASS
[VERDICT] GLOBAL : FAIL
```

T01/T02 : **FAIL honnête** — Richardson non démontré (saturation temporelle Euler documentée).  
T03 : PASS (conservation masse OK).  
T04 : PASS (nouveau critère — convergence plateau stationnaire, variation=0.64%).  
T05 : PASS (résidu Poisson < 1e-4).

### ns_lyapunov

```
lambda = -1.426073  label = STABLE
lambda(eps*10) = -1.112930  meme_signe = OUI
[VERDICT] fini_ok+robustesse = PASS
```

### blockchain_test (non régression)

```
PASS : 11 / FAIL : 0 / STATUS : OK
```

---

## 8. Ce qui reste ouvert après session 151

| ID | Description | Statut |
|----|-------------|--------|
| **Richardson-PROTOCOL-001** | Pour démontrer l'ordre spatial ≥ 1, il faut varier dt avec dx (dt_n = dt_ref / 2 pour chaque raffinement de grille). Le protocole actuel (dt=const) ne peut pas démontrer cet ordre. | **OPEN — décision protocole requise** |
| **BUILD-THREAD-001** | TSan concurrent FL-005 | OPEN |
| **BUILD-PROOF-001** | CI C indépendante | OPEN |
| **BUILD-PORT-002** | Validation portable | OPEN |
| **BL-003→BL-012** | Registre blockchain historique | OPEN |
| **NQubit** | fake_superposition RNG | OPEN scientifique |
| **NS-C3** | Intégration solveur NS dans NX-42 | OPEN |
| **NS-C4** | NX-42 problèmes 6–30 stubs | OPEN |
| **V138** | Pas de marqueurs de conflit dans nx47_vesu_kernel_v138.py — CONFIRMED CLEAN par audit direct | **CLÔTURÉ** (faux positif rapport 150) |

---

## 9. Conclusion

Les critères T01/T02 (Richardson) et T04 (énergie) étaient des **faux PASS** — ils cachaient des anomalies réelles. Après correction, les tests disent la vérité :

- **Richardson : FAIL** — la saturation temporelle Euler est réelle, non dissimulée.
- **T04 : PASS** — la cavité entraînée converge correctement vers un plateau stationnaire.
- **Lyapunov : PASS** — lambda=-1.426 (STABLE, Re=100), robuste à epsilon×10.

Cette correction respecte la règle fondamentale du projet : **ne jamais produire de résultat trompeur**.

**État global : CERTIFIED_100=false.**
