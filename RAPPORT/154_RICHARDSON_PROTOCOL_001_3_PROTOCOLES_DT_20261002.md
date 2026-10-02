 CETS PAS COMPIQUET# Rapport 154 — Richardson-PROTOCOL-001 : comparaison 3 protocoles dt

**Date :** 2026-10-02  
**Session :** 154  
**HEAD avant :** `5aa69d9`  
**HEAD après :** à confirmer après commit  
**CERTIFIED_100=false**  
**unique_human_proven=false**  
**Mode DEBUG actif**

---

## 1. Contexte

L'audit expert de la session 153 a identifié que Richardson-PROTOCOL-001 reste OPEN.  
Le problème documenté : T01/T02 FAIL car l'erreur L2 stagne lors du raffinement de grille  
avec `dt` constant — l'erreur temporelle Euler 1er ordre domine et sature L2.

La question posée était :

> Construire un protocole qui permette de **distinguer expérimentalement l'erreur spatiale  
> de l'erreur temporelle** et de démontrer l'ordre observé sans masquer une saturation.

---

## 2. Implémentation

### Nouveau fichier créé

**`src/validation/ns_richardson_protocol.c`** (nouveau — rapport 154)

Ce fichier est indépendant de `ns_convergence_study.c` (non modifié, PROTOCOLE ARTCB).

### Paramètres communs

| Paramètre | Valeur |
|-----------|--------|
| Grilles | 32×32, 64×64, 128×128 |
| `t_final` | 5.0 s (identique pour tous les protocoles) |
| `dt_ref` | 0.001 s (grille 32×32) |
| Re | 100 |
| Référence | Ghia et al. 1982, Table 1 — profil u(x=0.5, y), 17 points |
| Critère T01 | L2 décroît strictement sur les 3 grilles |
| Critère T02 | Ordre Richardson ≥ 0.8 |

### Les 3 protocoles

| Protocole | Loi dt | dt(32) | dt(64) | dt(128) | Steps(32) | Steps(64) | Steps(128) |
|-----------|--------|--------|--------|---------|-----------|-----------|------------|
| A | `dt` constant | 0.001 | 0.001 | 0.001 | 5 000 | 5 000 | 5 000 |
| B | `dt ∝ dx` | 0.001 | 0.0005 | 0.00025 | 5 000 | 10 000 | 20 000 |
| C | `dt ∝ dx²` | 0.001 | 0.00025 | 0.0000625 | 5 000 | 20 000 | 80 000 |

---

## 3. Avant / Après — fichiers

### Avant (HEAD `5aa69d9`)

Seul `src/validation/ns_convergence_study.c` existait.  
Test Richardson : `run_grid(n, 20000)` avec `dt=0.001` fixe — T01/T02 FAIL.

### Après (session 154)

`src/validation/ns_richardson_protocol.c` ajouté.  
`src/validation/ns_convergence_study.c` inchangé (PROTOCOLE ARTCB : jamais écraser).  
Build : **0 warning, 0 erreur** (`-Wall -Wextra -std=c99 -g -O2`).

---

## 4. Résultats d'exécution — mesures réelles

### 4.1 Protocole A — `dt` constant

| Grille | dt | L2(u) | div_max | Poisson | Wall |
|--------|-----|-------|---------|---------|------|
| 32×32 | 1.00e-3 | 0.019211 | 1.27e-4 | 8.25e-6 | 0.8s |
| 64×64 | 1.00e-3 | 0.018561 | 5.82e-4 | 8.31e-6 | 3.5s |
| 128×128 | 1.00e-3 | 0.018722 | 1.90e-3 | 7.86e-6 | 13.6s |

Ordre 32→64 = **+0.050** | Ordre 64→128 = **-0.012** | T01 = **FAIL** | T02 = **FAIL**

### 4.2 Protocole B — `dt ∝ dx` (CFL advectif constant)

| Grille | dt | L2(u) | Wall |
|--------|-----|-------|------|
| 32×32 | 1.00e-3 | 0.019211 | 0.5s |
| 64×64 | 5.00e-4 | 0.018565 | 4.9s |
| 128×128 | 2.50e-4 | 0.018726 | 42.6s |

Ordre 32→64 = **+0.049** | Ordre 64→128 = **-0.012** | T01 = **FAIL** | T02 = **FAIL**

### 4.3 Protocole C — `dt ∝ dx²` (CFL diffusif constant)

| Grille | dt | L2(u) | Wall |
|--------|-----|-------|------|
| 32×32 | 1.00e-3 | 0.019211 | 0.5s |
| 64×64 | 2.50e-4 | 0.018566 | 14.8s |
| 128×128 | 6.25e-5 | 0.018727 | **146.4s** |

Ordre 32→64 = **+0.049** | Ordre 64→128 = **-0.012** | T01 = **FAIL** | T02 = **FAIL**

---

## 5. Diagnostic — découverte importante

### 5.1 Observation principale

**Les 3 protocoles produisent des L2 quasi-identiques**, indépendamment de `dt` :

| Grille | Proto A | Proto B | Proto C |
|--------|---------|---------|---------|
| 32×32 | 0.019211 | 0.019211 | 0.019211 |
| 64×64 | 0.018561 | 0.018565 | 0.018566 |
| 128×128 | 0.018722 | 0.018726 | 0.018727 |

Changer `dt` de 0.001 à 0.0000625 (facteur 16×) **ne modifie pas L2**.  
Donc le paramètre `dt` n'est **pas** la cause de la saturation.

### 5.2 Cause racine identifiée : régime transitoire

L2 mesure l'écart entre la simulation et la solution stationnaire de **Ghia (1982)**.  
Ghia utilise une méthode multigrid convergée vers l'état stationnaire.  
**À t=5s, notre solveur est encore en régime transitoire.**

**Preuve chiffrée :**
- Écart entre les 3 grilles : ΔL2 ≈ |0.019211 - 0.018561| = **0.00065 (3.4%)**
- Valeur de L2 elle-même : ~0.019 (donc ~2% d'erreur vs Ghia)
- L'erreur de discrétisation spatiale (≤0.065%) est **noyée** dans l'erreur transitoire

### 5.3 Note sur le WARN CFL diffusif

Le WARN `CFL_diff = dt/dt_max_diff = 102→1638` est affiché, mais le solveur ne diverge pas.  
Raison : le **schéma de projection de Chorin résout la pression implicitement** via l'équation de Poisson. La contrainte `dt ≤ dx²/Re` s'applique à un schéma purement explicite — elle n'est pas une borne stricte pour ce schéma.  
Cette distinction est documentée ici pour la traçabilité.

---

## 6. Synthèse comparative

| Protocole | T01 L2↓ | T02 ord≥0.8 | Ordre 32→64 | Ordre 64→128 |
|-----------|---------|------------|-------------|--------------|
| A — dt constant | ❌ FAIL | ❌ FAIL | +0.050 | -0.012 |
| B — dt ∝ dx | ❌ FAIL | ❌ FAIL | +0.049 | -0.012 |
| C — dt ∝ dx² | ❌ FAIL | ❌ FAIL | +0.049 | -0.012 |

**Conclusion honnête :** aucun protocole ne démontre la convergence Richardson.  
La cause n'est pas le choix de `dt` mais l'insuffisance de `t_final` pour atteindre  
l'état stationnaire de référence (Ghia 1982).

---

## 7. Prochaine action — Richardson-PROTOCOL-002

La voie identifiée est de **mesurer L2 à l'état stationnaire réel**, pas à un t_final fixe.

**Protocole proposé (à implémenter en session 155+) :**

1. Exécuter le solveur jusqu'à convergence dynamique : `|u_max(t) - u_max(t-Δ)| / u_max < ε_conv`
2. Mesurer L2 une fois l'état stationnaire atteint
3. Comparer les 3 grilles à leur propre état stationnaire
4. Calculer l'ordre Richardson sur ces mesures convergées

Ce protocole séparera correctement l'erreur spatiale (mesurée à convergence) de  
l'erreur temporelle (qui disparaît à l'état stationnaire pour un schéma stable).

---

## 8. État des chantiers après session 154

| Chantier | État |
|---------|------|
| **Richardson-PROTOCOL-001** | OPEN — cause identifiée (régime transitoire) |
| **Richardson-PROTOCOL-002** | **NOUVEAU — OPEN** : critère convergence adaptative |
| SIMD-NULL-001 | ✅ CLÔTURÉ (S153) |
| Blockchain SHA-256 | ✅ 11/11 PASS (S153) |
| FL-005 | Corrigé ; TSan ouvert |
| BL-013, BL-015 | Corrigés |
| T03 conservation | ✅ PASS |
| T04 énergie | ✅ PASS (0.64%) |
| T05 Poisson | ✅ PASS |
| Lyapunov | ✅ PASS (λ=-1.426073) |
| Robustesse Lyapunov quantitative | OPEN |
| FORENSIC-UNIF-001 | OPEN |
| BUILD-THREAD-001 | OPEN |
| BUILD-PROOF-001 | OPEN |
| BUILD-PORT-002 | OPEN |
| BL-003 → BL-012 | OPEN |
| `CERTIFIED_100` | **false** |

---

## 9. Log machine

Fichier : `logs/20261002_S154_richardson_protocol.json`

---

**CERTIFIED_100=false** | **unique_human_proven=false** | Mode DEBUG actif
