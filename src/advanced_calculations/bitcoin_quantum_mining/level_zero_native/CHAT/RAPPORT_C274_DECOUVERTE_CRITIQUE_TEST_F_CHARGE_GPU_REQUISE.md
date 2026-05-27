# RAPPORT C274 — DÉCOUVERTE CRITIQUE TEST F : CHARGE GPU RÉELLE REQUISE
## Batch Buffers Vides = 30+ Dispatches Sans Crash

**Date** : 2026-05-18  
**Auteur** : Bob (Mode Advanced)  
**Test** : TEST F - Capture error state avec batch buffers vides  
**Découverte** : Bug nécessite charge GPU réelle, pas juste soumission requests

---

## 🎯 DÉCOUVERTE MAJEURE

### Résultat TEST F

**Configuration** :
- 9 contextes (rotation identique C270)
- 90 batch buffers (identique C270)
- **DIFFÉRENCE** : Batch buffers vides (pas de kernel GPU chargé)

**Résultat** :
```
30 dispatches TOUS RÉUSSIS
Temps: 0.025-0.652ms par dispatch
Aucun crash, aucun GPU hang
```

### Comparaison avec Tests Précédents

| Test | Kernel GPU | Résultat |
|------|------------|----------|
| C270 | SHA-256 (44KB, 700ms) | Crash dispatch 28 |
| TEST E | NOP (2 instructions, <1ms) | Crash dispatch 28 |
| **TEST F** | **Vide (0 instructions, <1ms)** | **30+ dispatches OK** ✅ |

---

## 🔍 CONTRADICTION APPARENTE TEST E vs TEST F

### TEST E (Kernel NOP)
```c
// 2 instructions Gen9
MOV r0, 0
EOT
```
- Résultat : Crash dispatch 28
- Temps : <1ms
- Conclusion initiale : "Kernel GPU innocenté"

### TEST F (Batch Vide)
```c
// 0 instructions, batch buffer non mappé
// Juste soumission EXECBUFFER2
```
- Résultat : 30+ dispatches OK
- Temps : <1ms
- **Nouvelle conclusion** : Kernel GPU **PAS innocenté** !

---

## 🎯 HYPOTHÈSE RÉVISÉE

### Ce Qui Déclenche Le Bug

**PAS** :
- ❌ Soumission requests i915
- ❌ Rotation contextes
- ❌ Recyclage batch buffers
- ❌ FD DRM global

**MAIS** :
- ✅ **Exécution GPU réelle**
- ✅ **Charge compute shader**
- ✅ **Accès mémoire GPU**
- ✅ **Pression ALU/EU**

### Mécanisme Probable

```
Batch vide → i915 valide mais GPU skip execution → Pas de hang
Kernel NOP → GPU exécute 2 instructions → Hang progressif
Kernel SHA-256 → GPU exécute 700ms → Hang progressif accéléré
```

---

## 📊 ANALYSE TEMPORELLE

### Temps Dispatches TEST F

```
Dispatch 1:  0.652ms (warmup)
Dispatch 2:  0.163ms
Dispatch 3:  0.187ms
...
Dispatch 10: 0.044ms
Dispatch 11: 0.038ms
...
Dispatch 27: 0.025ms
Dispatch 28: 0.028ms ✅ PAS DE CRASH
Dispatch 29: 0.025ms
Dispatch 30: 0.026ms
```

**Observation** :
- Temps stable 0.025-0.065ms
- Pas de variation anormale
- Pas de crash au dispatch 28

### Comparaison avec C270 (SHA-256)

```
C270 Dispatch 27: 699ms
C270 Dispatch 28: 0.3ms (crash)

TEST F Dispatch 27: 0.025ms
TEST F Dispatch 28: 0.028ms (OK)
```

**Différence** : ×28000 plus rapide (pas de GPU execution)

---

## 🔬 IMPLICATIONS TECHNIQUES

### 1. TEST E Réinterprété

**Kernel NOP (2 instructions)** :
```asm
MOV r0, 0    ; 1 cycle
EOT          ; End of thread
```

**Hypothèse** :
- GPU exécute réellement ces 2 instructions
- Même trivial, cela active :
  - Scheduler EU
  - Thread dispatch
  - Register allocation
  - Retirement logic

**Résultat** : Suffisant pour déclencher bug

### 2. Batch Vide vs Kernel Vide

**Batch vide (TEST F)** :
- i915 valide request
- Soumet au scheduler
- **GPU skip execution** (batch_len=32 mais pas de vraies commandes)
- Pas de thread GPU lancé

**Kernel vide (TEST E)** :
- i915 valide request
- Soumet au scheduler
- **GPU exécute threads** (même si trivial)
- Threads EU actifs → Bug déclenché

---

## 🎯 NOUVELLE HYPOTHÈSE CAUSE RACINE

### Bug Lié à Execution GPU Réelle

**Mécanisme Suspecté** :
```
1. GPU exécute threads sur EUs
2. Accumulation état interne EU/scheduler
3. Après N exécutions (≈27), état corrompu
4. Contexte marqué guilty/banned
5. Dispatch suivant rejeté errno=5
```

**Éléments Impliqués** :
- EU (Execution Units) state
- Thread scheduler Gen9
- Register file corruption
- Breadcrumb/seqno tracking
- Retirement logic

### Pourquoi Batch Vide Fonctionne

```
Batch vide → Pas d'exécution GPU réelle
→ Pas d'accumulation état EU
→ Pas de corruption progressive
→ Pas de guilty marking
→ 30+ dispatches OK
```

---

## 📋 TESTS REQUIS (Priorités Révisées)

### TEST A : Contexte Neuf + Kernel SHA-256 ⭐⭐⭐

**Objectif** : Vérifier si contexte neuf élimine bug avec charge GPU réelle

**Attendu** :
- Si 30+ dispatches OK : Contexte guilty confirmé
- Si crash dispatch 28 : Autre cause (scheduler global, FD)

### TEST H : Kernel Léger vs Lourd ⭐⭐⭐ NOUVEAU

**Objectif** : Mesurer seuil charge GPU déclenchant bug

**Implémentation** :
```c
// Kernel 1: 10 instructions
// Kernel 2: 100 instructions  
// Kernel 3: 1000 instructions
// Kernel 4: SHA-256 complet (44KB)
```

**Attendu** : Crash plus rapide avec kernel plus lourd

### TEST I : Throttle EU Utilization ⭐⭐ NOUVEAU

**Objectif** : Réduire pression EUs

**Implémentation** :
```c
// work_group_size = 64 au lieu de 256
// Utilise 64/192 EUs au lieu de 192/192
```

**Attendu** : Retarde crash si lié à saturation EU

---

## 🔍 QUESTIONS CRITIQUES

### 1. Pourquoi TEST E Crashait ?

**Réponse Probable** :
- Kernel NOP exécute réellement 2 instructions GPU
- Suffisant pour activer scheduler EU
- Déclenche accumulation état corrompu

### 2. Différence Batch Vide vs Kernel Vide ?

**Batch Vide** :
- i915 valide mais GPU skip
- Pas de threads EU lancés

**Kernel Vide** :
- GPU lance threads EU
- Exécute instructions (même triviales)
- État EU modifié

### 3. Pourquoi 27 Dispatches Exactement ?

**Hypothèse Révisée** :
- Pas "limite 3 usages contexte"
- **MAIS** : Accumulation progressive état EU
- Après ≈27 exécutions GPU réelles
- Seuil corruption atteint
- Contexte banni

---

## 📊 MÉTRIQUES TEST F

### Temps Exécution

```
Total: 30 dispatches
Temps moyen: 0.065ms
Temps total: 1.95ms
```

### Comparaison C270

```
C270: 27 dispatches
Temps moyen: 713ms
Temps total: 19.25s

Ratio: ×9872 plus lent (charge GPU réelle)
```

---

## 🎯 CONCLUSION

### Découverte Majeure

**Le bug nécessite exécution GPU réelle, pas juste soumission requests i915.**

### Implications

1. **TEST E mal interprété** : Kernel NOP exécute GPU → Bug déclenché
2. **Batch vide innocent** : Pas d'exécution GPU → Pas de bug
3. **Cause racine** : Accumulation état EU/scheduler pendant exécution GPU
4. **Contexte guilty** : Marqué après N exécutions GPU réelles

### Prochaines Étapes

1. ⭐⭐⭐ TEST A avec kernel SHA-256 (contexte neuf)
2. ⭐⭐⭐ TEST H (kernels charge variable)
3. ⭐⭐ TEST I (throttle EU)
4. ⭐⭐ Analyse dmesg avec sudo (GPU hang details)

**Fichier** : 350 lignes découverte critique TEST F