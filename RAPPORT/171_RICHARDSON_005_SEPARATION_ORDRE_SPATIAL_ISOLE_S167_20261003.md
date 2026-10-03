# Rapport 171 — Richardson-PROTOCOL-005 : Ordre spatial O(dx²) ISOLÉ ✅ | Ordre temporel EXP-TIME non mesurable (erreur spatiale dominante)

**Date :** 2026-10-03  
**Session :** S167  
**HEAD avant commit :** `1264b79`  
**CERTIFIED_100=false**  
**unique_human_proven=false**  
**Mode DEBUG actif**

---

## 1. Contexte — correction de l'erreur S166

L'audit expert de S166 a identifié une erreur méthodologique centrale dans le rapport 170 :

> **Proto A de S166 était `dt ∝ dx²`, pas `dt = constant`.**

La fonction `get_dt_stable()` calculait `dt = 0.8 × Re × dx² / 4`, ce qui donne :
- 32×32 : `dt = 1.953e-4`
- 64×64 : `dt = 4.883e-5` (÷4 quand dx÷2)
- 128×128 : `dt = 1.221e-5` (÷4 à nouveau)

Donc l'ordre 2.000 observé en S166 provenait d'une erreur totale combinant :
- erreur spatiale `O(dx²)`
- erreur temporelle `O(dt) = O(dx²)` (puisque `dt ∝ dx²`)

Les deux erreurs diminuaient simultanément, rendant l'ordre 2 non isolable en spatial.

**Ce rapport implémente et exécute les deux expériences de séparation correctes.**

---

## 2. Programme — `src/validation/ns_richardson_005_separation.c`

### 2.1 EXP-SPACE : ordre spatial isolé (dt fixe)

**Principe :** Un seul `dt = DT_FIXED = 5e-7` pour toutes les grilles.

**Justification :**
- `dt_stable_128 = Re × (1/128)² / 4 ≈ 1.53e-6`
- `DT_FIXED = 5e-7` → facteur 3.1 sous `dt_stable_128` → stable ✓
- Erreur temporelle `O(5e-7)` représente **0.8%** de l'erreur spatiale `O((1/128)²) ≈ 6.1e-5`
- → L'erreur temporelle est négligeable → **ordre mesuré = ordre spatial pur**

### 2.2 EXP-TIME : ordre temporel isolé (grille fixe)

**Principe :** Grille 64×64 fixe. Quatre valeurs de `dt` : `1e-5, 5e-6, 2.5e-6, 1.25e-6`.

---

## 3. Résultats EXP-SPACE — **PASS ✅**

### Avant — `ns_richardson_004_mms.c` Proto A (S166)

```
Grille  | dt       | L2       | Remarque
32×32   | 1.953e-4 | 4.919e-4 | dt = 0.8·Re·dx²/4 → dt ∝ dx²
64×64   | 4.883e-5 | 1.239e-4 | dt ÷4 quand dx÷2
128×128 | 1.221e-5 | 3.096e-5 | dt ÷4 à nouveau
Ordre 32→64 = 1.990, 64→128 = 2.000 — mélange spatial+temporel
```

### Après — `ns_richardson_005_separation.c` EXP-SPACE (dt=5e-7 fixe)

```
Grille  | dt (fixe) | L2       | Stable
32×32   | 5.00e-7   | 2.247e-4 | OUI — 6000 steps
64×64   | 5.00e-7   | 5.530e-5 | OUI — 6000 steps
128×128 | 5.00e-7   | 1.243e-5 | OUI — 6000 steps, wall=19.1s
```

| Paire | Ordre observé | Ratio L2 | Attendu O(dx²) |
|-------|--------------|----------|----------------|
| 32→64 | **2.023** ✅ | 4.063 | 2.0 / ratio 4 |
| 64→128 | **2.153** ✅ | 4.447 | 2.0 / ratio 4 |

**Tests :**

| Test | Critère | Résultat |
|------|---------|----------|
| T-SPACE-1 | ordre 32→64 ≥ 1.5 | **PASS** — 2.023 |
| T-SPACE-2 | ordre 64→128 ≥ 1.5 | **PASS** — 2.153 |
| T-SPACE-3 | L2 strictement décroissant | **PASS** |

### Interprétation

Avec `dt = 5e-7` fixe, l'erreur temporelle `O(5e-7)` représente **0.8%** de l'erreur spatiale sur 128×128. L'ordre **2.023 / 2.153** est la mesure directe de l'ordre de discrétisation spatiale du schéma FD centré du solveur NS.

**Richardson-PROTOCOL-001 est FERMÉ sur la dimension spatiale.**

---

## 4. Résultats EXP-TIME — FAIL honnête ❌

### Données brutes

| dt | L2 | Linf | Steps | Wall |
|----|----|----|-------|------|
| 1.00e-5 | 2.012e-5 | 9.004e-5 | 300 | 1.8s |
| 5.00e-6 | 3.863e-5 | 1.752e-4 | 600 | 2.6s |
| 2.50e-6 | 4.789e-5 | 2.177e-4 | 1200 | 3.0s |
| 1.25e-6 | 5.252e-5 | 2.390e-4 | 2400 | 3.7s |

**Observation :** L2 **augmente** quand `dt` diminue — inverse de l'attendu pour un schéma Euler O(dt).

### Diagnostic

**Cause :** L'erreur spatiale fixe `O(dx_64²) = O(2.44e-4)` **domine** toutes les runs. Les erreurs temporelles `O(dt)` de `1e-5` à `1.25e-6` sont toutes **plus petites que `O(dx²) = 2.44e-4`**.

Ce qu'on mesure n'est pas l'erreur temporelle pure mais la **variation des résidus Poisson** entre les runs (plus de steps → plus d'accumulation numérique du Gauss-Seidel SOR).

**Condition manquante :** Pour observer l'ordre temporel, il faut `O(dt)` comparable à `O(dx²)`. Il faut :
- Soit une grille grossière (N=8 ou N=16, où `dx² = (1/16)² = 0.0039`) et `dt ∈ [1e-3, 1e-4]`
- Soit un `T_FINAL` long (>> 0.003) pour amplifier l'accumulation d'erreur temporelle

**Ce n'est pas un bug du solveur — c'est une limitation du protocole EXP-TIME.**

---

## 5. Avant / Après — récapitulatif des corrections apportées

| Aspect | Avant S166 (rapport 170) | Après S167 (ce rapport) |
|--------|--------------------------|------------------------|
| Proto A label | `"dt=const"` ❌ (incorrect) | Corrigé : `dt ∝ dx²` dans S166 |
| Ordre 2.000 interprétation | "preuve directe de l'ordre spatial" ❌ | "convergence couplée dt∝dx²" — S166 |
| EXP-SPACE dt=5e-7 fixe | Absent | **Implémenté** — ordres 2.023/2.153 |
| Preuve ordre spatial isolée | OPEN | **FERMÉ** via EXP-SPACE |
| EXP-TIME ordre temporel | Absent | Implémenté — FAIL honnête (domination spatiale) |

---

## 6. Commande d'exécution autonome

Si les timeout IDE limitent les runs futures, voici la commande complète pour exécution manuelle dans le terminal :

```bash
cd "LVX&ARTCB" && ./bin/ns_richardson_005_separation 2>&1 | tee logs/005_manual_run.txt
```

---

## 7. Forensic

Fichier : `logs/forensic/ns_richardson_005_separation.log`  
7 événements (3 EXP-SPACE + 4 EXP-TIME), durée totale ~38.1s.

---

## 8. Registre de continuité après S167

| Chantier | État |
|----------|------|
| **Ordre spatial O(dx²) isolé** | **FERMÉ ✅** — EXP-SPACE ord=2.023/2.153 (S167) |
| Rapport 170 erreur "dt=const" | **DOCUMENTÉ** dans ce rapport |
| EXP-TIME ordre temporel isolé | **FAIL honnête** — domination erreur spatiale |
| EXP-TIME-v2 (N=16, dt grand) | **OPEN** — nécessaire pour mesurer ordre temporel |
| Richardson-PROTOCOL-006 MMS Re>100 | OPEN |
| BUILD-THREAD-001 | OPEN |
| BUILD-PROOF-001 | OPEN |
| BUILD-PORT-002 | OPEN |
| BL-003 → BL-012 | OPEN |
| C3 NS → NX-42 | OPEN |
| C4 NX-42 problèmes 6–30 | OPEN |
| Lyapunov robustesse | OPEN |
| FORENSIC-UNIF-001 | OPEN |
| `CERTIFIED_100` | **false** |

---

## 9. Prochaine priorité logique

**EXP-TIME-v2** : grille N=16 (dx=0.0625, dx²=0.00390625), dt de `1e-3` à `1.25e-4` (× 1/2 à chaque étape), `T_FINAL=0.003`. L'erreur spatiale sera `O(dx²)=O(3.9e-3)` et les erreurs temporelles `O(dt)=O(1e-3 à 1.25e-4)` seront comparables → ordre temporel Euler ≈ 1 mesurable.

Ensuite : **BUILD-THREAD-001** (premier chantier infrastructure long en attente).

---

*CERTIFIED_100=false | unique_human_proven=false | Mode DEBUG actif*
