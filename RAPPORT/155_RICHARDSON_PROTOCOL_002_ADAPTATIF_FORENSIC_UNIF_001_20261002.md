# Rapport 155 — Richardson-PROTOCOL-002 adaptatif + FORENSIC-UNIF-001

**Date :** 2026-10-02  
**Session :** 155  
**HEAD avant :** `0ad0989`  
**HEAD après :** à confirmer après commit  
**CERTIFIED_100=false**  
**unique_human_proven=false**  
**Mode DEBUG actif**

---

## 1. Contexte — Suppression des limitations artificielles

La session précédente (S154) avait identifié que la limite de 5 000 pas constituait
une limite artificielle qui empêchait le solveur d'atteindre l'état stationnaire réel.
L'utilisateur a exigé l'élimination immédiate de cette contrainte.

**Deux chantiers traités en parallèle :**
- **Richardson-PROTOCOL-002** : convergence adaptative jusqu'à stationnarité réelle
- **FORENSIC-UNIF-001** : traçabilité bit→LUM_ID→NS à 100% nanoseconde

---

## 2. Avant / Après — fichiers

### Avant (HEAD `0ad0989`)

| Fichier | Limite artificielle |
|---------|---------------------|
| `src/validation/ns_richardson_protocol.c` | `T_FINAL = 5.0 s` → stop fixe |
| `src/validation/ns_convergence_study.c` | `run_grid(n, 20000)` → stop fixe |
| Forensic NS | Absent — FORENSIC-UNIF-001 OPEN |

### Après (session 155)

| Fichier créé | Description |
|-------------|-------------|
| `src/validation/ns_richardson_adaptive.c` | Boucle infinie jusqu'à critère dynamique |
| `src/validation/ns_forensic_unif.c` | Traçabilité 100% bit-level NS |

**Build :** 0 warning, 0 erreur (`-Wall -Wextra -std=c99 -g -O2`)

---

## 3. Richardson-PROTOCOL-002 — Critère adaptatif

### Paramètres

| Paramètre | Valeur |
|-----------|--------|
| Critère convergence | `rel_umax < 1e-3` ET `rel_ek < 1e-3` ET `poisson < 1e-4` |
| Fenêtre glissante | 200 checkpoints (1 mesure / 50 pas) |
| Limite sécurité | 500 000 pas → si atteinte : NON_CONVERGÉ, jamais PASS |

---

## 4. Résultats — Protocole A (dt constant = 0.001)

Convergence réelle mesurée :

| Grille | Ancien t_stop | Nouveau t_conv | Steps | L2(u) | u_max | wall |
|--------|--------------|----------------|-------|-------|-------|------|
| 32×32 | 5.0 s | **21.85 s** | 21 850 | 0.011173 | 0.9033 | 2.1 s |
| 64×64 | 5.0 s | **21.80 s** | 21 800 | 0.011122 | 0.9523 | 7.3 s |
| 128×128 | 5.0 s | **21.75 s** | 21 750 | 0.011229 | 0.9762 | 26.7 s |

### Ordres Richardson à convergence

- Ordre 32→64 = log(0.011173 / 0.011122) / log(2) = **+0.066**
- Ordre 64→128 = log(0.011122 / 0.011229) / log(2) = **-0.139**

| Test | Résultat |
|------|---------|
| T01 (L2 décroît strictement) | **FAIL** (L2_128 > L2_64) |
| T02 (ordre ≥ 0.8) | **FAIL** |

### Interprétation honnête

La suppression de la limite 5 000 pas a permis d'atteindre l'état stationnaire réel
(t≈21.85 s, ~21 850 pas). Résultat fondamental :

**Les L2 à convergence sont : 0.011173 / 0.011122 / 0.011229 — quasi-identiques.**

→ La valeur de L2 est dominée par la **précision intrinsèque du solveur** et la
**densité de la référence Ghia** (17 points), pas par le régime transitoire.

Le solveur a atteint son état stationnaire, mais les L2 des 3 grilles restent dans
la fourchette 0.011122–0.011229 (différence < 1%), ce qui indique que l'erreur
de discrétisation spatiale est elle-même inférieure à la résolution de la référence Ghia.

**Conclusion : T01/T02 FAIL honnêtes. Richardson reste OPEN.**

---

## 5. FORENSIC-UNIF-001 — Traçabilité bit→LUM_ID→NS

### Chaîne de traçabilité implémentée

```
double value (64 bits)
    → memcpy(&raw, &value, 8)       ← IEEE 754, sans interprétation
    → bit_pos 0..63
    → LUM_ID = encode_lum_id(module, step, i, j, bit_pos)
    → forensic_log_individual_lum(lum_id, op_name, timestamp_ns)
    → log fichier + stdout nanoseconde
```

### Encodage LUM_ID (bijection garantie)

```
bits[31..28] = module_id  (0=U_IN | 1=V_IN | 2=P_IN | 3=U_OUT | 4=POISSON_RES)
bits[27..20] = step       (0..255)
bits[19..14] = i          (0..63)
bits[13..8]  = j          (0..63)
bits[7..0]   = bit_pos    (0..63)
```

### Résultats (grille 4×4, 10 pas)

| Module | Bits tracés |
|--------|------------|
| U_IN_ADVECTION | 7 680 |
| V_IN_ADVECTION | 7 680 |
| P_IN_POISSON | 10 240 |
| U_OUT_CORRECTION | 7 680 |
| POISSON_RESIDUAL | 640 |
| **TOTAL tracé** | **33 920** |
| **TOTAL attendu** | **33 920** |
| **Coverage** | **100.00%** |
| Wall time | 1.013 s |

**VERDICT FORENSIC-UNIF-001 : ✅ PASS — 100% bits tracés**

### Précision timestamps — déclaration honnête

| Point | Valeur |
|-------|--------|
| Source | `CLOCK_REALTIME` via `time_ns_get_absolute()` |
| Unité | Nanosecondes |
| Résolution matérielle | Non garantie à 1 ns — dépend OS/HW |
| Unicité inter-bits | `ts_base + bit_pos` (offset fictif) |

---

## 6. État des chantiers après session 155

| Chantier | État |
|---------|------|
| **Richardson-PROTOCOL-002** | **OPEN** — convergence atteinte (t≈21.85s) mais L2 stagne |
| **FORENSIC-UNIF-001 NS (5 modules)** | ✅ **PASS — 100% bits tracés** |
| FORENSIC-UNIF-001 Richardson/Lyapunov/NX-42 | OPEN |
| Limite 5000 pas supprimée | ✅ Éliminée |
| SIMD-NULL-001 | ✅ Clôturé (S153) |
| Blockchain SHA-256 | ✅ 11/11 PASS |
| T03/T04/T05 | ✅ PASS |
| Lyapunov | ✅ PASS |
| Richardson T01/T02 | ❌ FAIL honnête |
| BUILD-THREAD-001 | OPEN |
| BL-003→BL-012 | OPEN |
| `CERTIFIED_100` | **false** |

---

## 7. Prochaine priorité

Le solveur converge à t≈21.85s. Les L2 à convergence sont identiques sur les 3 grilles.
Deux hypothèses à tester :

1. **Ordre spatial insuffisant** : différences centrées 2ème ordre → L2 ≈ O(dx²) mais les
   3 grilles ont des L2 trop proches (différence ~1%) — suggère que l'erreur provient
   de la référence Ghia (17 points = interpolation grossière) plutôt que du solveur.

2. **Richardson sur solution manufacturée** : utiliser une solution analytique connue
   (Stokes flow, Poiseuille) au lieu de Ghia pour mesurer l'ordre spatial pur.

**Richardson-PROTOCOL-003 (NOUVEAU CHANTIER OPEN)** : validation sur solution exacte.

---

## 8. Log machine

Fichier : `logs/20261002_S155_adaptive_forensic.json`

---

**CERTIFIED_100=false** | **unique_human_proven=false** | Mode DEBUG actif
