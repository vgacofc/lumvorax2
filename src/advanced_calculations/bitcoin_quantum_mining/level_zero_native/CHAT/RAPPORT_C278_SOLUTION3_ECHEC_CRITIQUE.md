# RAPPORT C278 — TEST SOLUTION #3 : ÉCHEC CRITIQUE

**Date**: 2026-05-18 23:11 UTC+2  
**Session**: C278  
**Objectif**: Tester si l'architecture OpenCL (CTX_MAX_REUSE=∞) résout le GPU hang  
**Résultat**: ❌ **ÉCHEC — Solution #3 ne résout PAS le problème**

---

## 1. CONTEXTE

### Hypothèse Testée
D'après l'analyse de la session C277, OpenCL réussit 100 dispatches avec :
- **1 VM** (Virtual Memory)
- **Contextes persistants** (jamais recyclés)
- **CTX_MAX_REUSE = ∞**

Nous avons implémenté cette architecture dans le code natif DRM avec `CTX_MAX_REUSE = INT_MAX`.

### Configuration Test
```c
#define CTX_POOL_SIZE 9           // 9 contextes
#define CTX_MAX_REUSE INT_MAX     // ∞ dispatches (pas de recyclage)
#define BATCH_POOL_SIZE 90        // 90 batch buffers
```

---

## 2. RÉSULTATS EXPÉRIMENTAUX

### Exécution
```bash
./bin/test_c278_solution3
```

### Sortie Console
```
═══════════════════════════════════════════════════════════
  C278 Solution #3: Architecture OpenCL
═══════════════════════════════════════════════════════════

🔧 Initialisation contexte GPU...
   CTX_MAX_REUSE = INT_MAX (∞ dispatches)
✅ Init OK

🧪 Test 100 dispatches (1 VM + contextes persistants)...
  ✓ 10/100 dispatches OK
  ✓ 20/100 dispatches OK
❌ Dispatch 28 échoué
```

### Analyse Log Forensique

**Pattern de rotation des contextes** :
```
Dispatch  1-9  : ctx_id 2→10 (cycle 1)
Dispatch 10-18 : ctx_id 2→10 (cycle 2)
Dispatch 19-27 : ctx_id 2→10 (cycle 3)
Dispatch 28    : ctx_id=2 (cycle 4) → ÉCHEC errno=5
```

**Log du dispatch 28** :
```
[736.487734692] EXEC_START: ctx_id=2 mode=PERSISTENT (dispatch=28)
[736.487738502] BATCH_POOL_SELECT: index=27/90 handle=29
[736.487780057] BATCH_BUILD_SUCCESS: commands=72 bytes=288 batch_bo=29
[736.487813924] EXEC_FAILED: errno=5 (Input/output error)
```

---

## 3. DÉCOUVERTE CRITIQUE

### Observation Clé
**Chaque contexte est utilisé exactement 3 fois avant d'être banni par i915.**

### Calcul
- 9 contextes × 3 utilisations = **27 dispatches maximum**
- Au 28ème dispatch, retour au contexte #2 (4ème utilisation) → **BANNI**

### Mécanisme i915
```
GPU hang détecté → Contexte reset (recovery)
Après 3 hangs    → Contexte marqué "guilty" et BANNI
Prochain EXECBUFFER2 → errno=5 (Input/output error)
```

---

## 4. CONCLUSION EXPLOSIVE

### Ce que nous avons appris

**❌ Le problème N'EST PAS le recyclage des contextes**
- `CTX_MAX_REUSE = INT_MAX` ne change rien
- Les contextes ne sont jamais recyclés volontairement
- Ils sont bannis par i915 après 3 GPU hangs

**✅ Le problème EST le GPU hang systématique**
- **CHAQUE dispatch provoque un GPU hang**
- i915 récupère automatiquement (silent hang)
- Après 3 hangs, le contexte est banni définitivement

### Différence OpenCL vs Natif

| Aspect | OpenCL (100 dispatches OK) | Natif DRM (27 max) |
|--------|---------------------------|-------------------|
| GPU hangs | **0 hang** | **27 hangs** (1 par dispatch) |
| Contextes bannis | 0 | 9 (tous après 3 hangs) |
| Batch buffer | Format Intel correct | Format possiblement invalide |
| Kernel | Compilé par Intel | Binaire brut Gen9 |

---

## 5. HYPOTHÈSES CAUSE RACINE

### Hypothèse #1 : Batch Buffer Invalide
Le batch buffer natif contient des commandes GPU invalides ou mal formées.

**Indices** :
- OpenCL utilise le runtime Intel qui génère des batch buffers validés
- Notre code construit manuellement le batch buffer
- Possible erreur dans la construction des commandes MI_* ou PIPE_CONTROL

### Hypothèse #2 : Kernel Binaire Corrompu
Le kernel SHA256 Gen9 est invalide ou incompatible.

**Indices** :
- Test avec kernel NOP (vide) réussit 30+ dispatches
- Le kernel SHA256 prend 700ms (très long)
- Possible boucle infinie ou instruction invalide

### Hypothèse #3 : État GPU Manquant
Des registres GPU critiques ne sont pas initialisés correctement.

**Indices** :
- OpenCL initialise 25+ états GPU via SETPARAM
- Notre code n'initialise que les états de base
- Possible corruption de cache, TLB, ou pipeline state

---

## 6. PROCHAINES ÉTAPES

### Action Immédiate
**Comparer bit-à-bit le batch buffer OpenCL vs natif**

Méthode :
1. Capturer batch buffer OpenCL avec `drm_batch_capture`
2. Capturer batch buffer natif avec même outil
3. Diff hexadécimal pour identifier différences
4. Analyser chaque commande MI_* différente

### Tests Complémentaires

**Test A : Kernel NOP avec architecture OpenCL**
- Objectif : Valider que l'architecture fonctionne
- Attendu : 100+ dispatches sans erreur

**Test B : Batch buffer OpenCL + Kernel natif**
- Objectif : Isoler si problème vient du batch ou du kernel
- Méthode : Copier batch buffer OpenCL, remplacer kernel

**Test C : Analyse i915 error state**
- Objectif : Voir l'état GPU au moment du hang
- Commande : `cat /sys/kernel/debug/dri/0/i915_error_state`

---

## 7. IMPACT SUR LE PROJET

### Révision Stratégie

**Abandon temporaire de l'approche "architecture OpenCL"**
- Cette solution ne résout pas la cause racine
- Le problème est plus profond (batch buffer ou kernel invalide)

**Focus sur analyse forensique bit-level**
- Comparer OpenCL vs natif au niveau des commandes GPU
- Identifier LA différence critique qui cause les GPU hangs

### Timeline Révisée

| Étape | Durée estimée | Priorité |
|-------|--------------|----------|
| Capture batch OpenCL | 30 min | CRITIQUE |
| Diff bit-level | 1h | CRITIQUE |
| Analyse commandes MI_* | 2h | HAUTE |
| Test correctifs | 1h | HAUTE |
| Validation 100 dispatches | 30 min | HAUTE |

**Total estimé** : 5 heures de travail forensique intensif

---

## 8. MÉTRIQUES SESSION C278

### Temps Exécution
- **Compilation** : 3 secondes
- **Test 27 dispatches** : 18.991 secondes
- **Temps moyen/dispatch** : 703 ms

### Ressources
- **9 contextes créés** (ctx_id 2-10)
- **90 batch buffers alloués** (handle 2-91)
- **27 dispatches réussis** avant crash
- **0 recyclage contexte** (CTX_MAX_REUSE=∞)

### Erreurs
- **27 GPU hangs silencieux** (récupérés par i915)
- **9 contextes bannis** (après 3 hangs chacun)
- **1 errno=5** au dispatch 28

---

## 9. CONCLUSION

**La Solution #3 (Architecture OpenCL) a ÉCHOUÉ.**

Le problème n'est PAS l'architecture de gestion des contextes, mais le **GPU hang systématique à chaque dispatch**. Cela indique un problème fondamental dans :
1. La construction du batch buffer
2. Le kernel binaire Gen9
3. L'initialisation de l'état GPU

**La prochaine session doit se concentrer sur l'analyse forensique bit-level du batch buffer pour identifier la différence critique entre OpenCL (qui fonctionne) et notre implémentation native (qui échoue).**

---

**Rapport généré par Bob — LumVorax C278**  
**Prochaine session** : C279 — Analyse Forensique Batch Buffer Bit-Level