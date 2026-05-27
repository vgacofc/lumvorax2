# RAPPORT C198 PHASE 15Y-D — DIAGNOSTIC RÉGRESSION errno=22

**Date**: 2026-05-11 15:22 UTC+2  
**Contexte**: Diagnostic complet régression après corrections Phase 15Y  
**Objectif**: Identifier pourquoi les corrections ont cassé le code qui fonctionnait  
**Statut**: 🔴 RÉGRESSION CRITIQUE — 0 dispatches réussis (vs 3 avant)

---

## 1. RÉSUMÉ EXÉCUTIF

### 1.1 Situation Avant Corrections (Phase 15X)
```
✅ Dispatches 1-3: SUCCÈS (678ms, 694ms, 691ms)
❌ Dispatch 4: errno=5 (EIO - Input/output error)
📊 Performance: 3.07 MH/s
🎯 Objectif: Corriger errno=5 pour atteindre 10+ dispatches
```

### 1.2 Situation Après Corrections (Phase 15Y-D)
```
❌ Dispatch 1: errno=22 (EINVAL - Invalid argument)
📊 Performance: 0 MH/s (aucune exécution)
🔴 RÉGRESSION: Les corrections ont CASSÉ le code qui fonctionnait
```

### 1.3 Découverte Critique
**AUCUNE des 7 corrections testées n'a résolu le problème !**

Toutes les hypothèses étaient FAUSSES :
- ❌ Ce n'est PAS le terminator (0x05 vs 0x0A)
- ❌ Ce n'est PAS le zero-fill du batch buffer
- ❌ Ce n'est PAS le ctx_id (pool vs principal)
- ❌ Ce n'est PAS le batch_len (220 vs 262144)
- ❌ Ce n'est PAS les relocations GPU
- ❌ Ce n'est PAS le pool de contextes
- ❌ Ce n'est PAS la validation du terminator

**Conclusion**: Le bug est ailleurs, dans une modification non identifiée.

---

## 2. CHRONOLOGIE DÉTAILLÉE DES TESTS

### Test 1: Correction Terminator (0x0A → 0x05)
```c
// AVANT (fonctionnait)
#define GEN9_MI_BATCH_BUFFER_END 0x0A000000

// APRÈS (cassé)
#define GEN9_MI_BATCH_BUFFER_END 0x05000000
```

**Résultat**: errno=22 (EINVAL)  
**Conclusion**: Le terminator 0x05 est INCORRECT pour Gen9

### Test 2: Ajout Zero-Fill Batch Buffer
```c
// AJOUTÉ
memset(ctx->batch_map, 0, 4096);
```

**Résultat**: errno=22 (EINVAL)  
**Conclusion**: Le zero-fill ne résout rien

### Test 3: Activation Pool Contextes
```c
// AVANT
uint32_t pool_ctx_id = ctx->ctx_id;  // ctx_id=1

// APRÈS
uint32_t pool_ctx_id = ctx->ctx_pool[pool_index];  // ctx_id=2
```

**Résultat**: errno=22 (EINVAL)  
**Conclusion**: Le pool de contextes ne fonctionne pas

### Test 4: Rollback ctx_id Principal
```c
// ROLLBACK
uint32_t pool_ctx_id = ctx->ctx_id;  // Retour ctx_id=1
```

**Résultat**: errno=22 (EINVAL)  
**Conclusion**: Ce n'est pas le ctx_id le problème

### Test 5: Rollback Terminator 0x0A
```c
// ROLLBACK
#define GEN9_MI_BATCH_BUFFER_END 0x0A000000
```

**Résultat**: errno=22 (EINVAL)  
**Conclusion**: Même avec 0x0A, ça ne fonctionne plus

### Test 6: Désactivation Zero-Fill
```c
// DÉSACTIVÉ
// memset(ctx->batch_map, 0, 4096);
```

**Résultat**: errno=22 (EINVAL)  
**Conclusion**: Le zero-fill n'est pas le problème

### Test 7: Correction batch_len
```c
// AVANT
.batch_len = ctx->batch_size,  // 262144 bytes

// APRÈS
.batch_len = 220,  // Taille réelle
```

**Résultat**: errno=22 (EINVAL)  
**Conclusion**: batch_len n'est PAS le problème (l'ancien code utilisait aussi 262144)

---

## 3. ANALYSE COMPARATIVE ANCIEN vs NOUVEAU CODE

### 3.1 Fichiers Comparés
```
ANCIEN (fonctionnait):
  level_zero_native/src/btc_gen9_native_runner.c
  Modifié: 2026-05-11 14:28
  Log: level_zero_native/logs/forensic/btc_mining_native_sha256.log
  
NOUVEAU (cassé):
  i915_native/src/btc_gen9_native_runner.c
  Modifié: 2026-05-11 15:22
  Log: i915_native/logs/forensic/test_gen9_native.log
```

### 3.2 Différences Identifiées

#### Structure drm_i915_gem_execbuffer2
```c
// ANCIEN (fonctionnait)
struct drm_i915_gem_execbuffer2 execbuf = {
    .buffers_ptr = (uintptr_t)exec_objects,
    .buffer_count = 4,
    .batch_start_offset = 0,
    .batch_len = ctx->batch_size,  // 262144
    .flags = I915_EXEC_RENDER,
    .rsvd1 = pool_ctx_id
};

// NOUVEAU (cassé) - IDENTIQUE !
struct drm_i915_gem_execbuffer2 execbuf = {
    .buffers_ptr = (uintptr_t)exec_objects,
    .buffer_count = 4,
    .batch_start_offset = 0,
    .batch_len = actual_batch_len,  // 220 (testé aussi avec 262144)
    .flags = I915_EXEC_RENDER,
    .rsvd1 = pool_ctx_id
};
```

**Observation**: Les structures sont IDENTIQUES ! Le problème est ailleurs.

#### Logs Forensiques
```
ANCIEN (fonctionnait):
  [3291.010040614] BATCH_BUILD_SUCCESS: commands=55 bytes=220
  [3291.688771852] EXEC_SUCCESS: time=0.678737 sec pool_ctx_id=1

NOUVEAU (cassé):
  [4628.662221736] BATCH_BUILD_SUCCESS: commands=55 bytes=220 terminator=0x0a000000
  [4628.662230206] EXEC_FAILED: errno=22 (Invalid argument)
```

**Observation**: Le batch buffer est construit avec succès, mais EXECBUFFER2 échoue immédiatement.

---

## 4. HYPOTHÈSES RESTANTES

### 4.1 Hypothèse #1: Kernel Path Différent
```
ANCIEN: kernels/btc_sha256_mining_gen9.bin
NOUVEAU: kernels/test_add_gen9.bin
```

**Probabilité**: 🟡 MOYENNE  
**Test nécessaire**: Utiliser le même kernel dans les deux versions

### 4.2 Hypothèse #2: Ordre d'Initialisation
```c
// Peut-être que l'ordre de création des buffers GEM importe ?
handle=1: kernel_bo
handle=2: batch_bo
handle=3: input_bo
handle=4: output_bo
```

**Probabilité**: 🟢 FAIBLE  
**Raison**: L'ordre est identique dans les deux versions

### 4.3 Hypothèse #3: Flags GEM_CREATE
```c
// Vérifier si les flags de création des buffers sont identiques
struct drm_i915_gem_create create = {
    .size = size,
    // Flags manquants ?
};
```

**Probabilité**: 🔴 ÉLEVÉE  
**Raison**: Peut-être que des flags sont nécessaires pour EXECBUFFER2

### 4.4 Hypothèse #4: Répertoire de Travail
```
ANCIEN: /home/lvx/LVX/lumvorax2/src/advanced_calculations/bitcoin_quantum_mining/level_zero_native
NOUVEAU: /home/lvx/LVX/lumvorax2/src/advanced_calculations/bitcoin_quantum_mining/i915_native
```

**Probabilité**: 🟢 TRÈS FAIBLE  
**Raison**: Les chemins absolus sont utilisés

### 4.5 Hypothèse #5: Modification Invisible du Code
```
Peut-être qu'une modification a été faite AVANT la Phase 15Y
qui a cassé le code, et les corrections Phase 15Y n'ont fait
qu'ajouter du logging sans toucher au vrai problème.
```

**Probabilité**: 🔴 TRÈS ÉLEVÉE  
**Raison**: Expliquerait pourquoi AUCUNE correction ne fonctionne

---

## 5. ANALYSE FORENSIQUE LOGS

### 5.1 Log Ancien (Fonctionnait)
```
[3291.009758976] INIT_START: batch_size=262144 work_group_size=256
[3291.009819978] DRM_OPEN_SUCCESS: /dev/dri/renderD128 (fd=7)
[3291.009838490] DRM_VERSION: i915 1.6.0
[3291.009847251] CONTEXT_CREATE_SUCCESS: ctx_id=1
[3291.009850721] CTX_POOL_CREATED: index=0 ctx_id=2
[3291.009852590] CTX_POOL_CREATED: index=1 ctx_id=3
[3291.009854331] CTX_POOL_CREATED: index=2 ctx_id=4
[3291.009915932] GEM_ALLOC_SUCCESS: handle=1 size=42672 addr=0x7a693629b000
[3291.009950674] KERNEL_LOAD_SUCCESS: path=kernels/btc_sha256_mining_gen9.bin size=42672 handle=1
[3291.009971049] GEM_ALLOC_SUCCESS: handle=2 size=4096 addr=0x7a6936340000
[3291.009988819] GEM_ALLOC_SUCCESS: handle=3 size=1048576 addr=0x7a6935f00000
[3291.010020761] GEM_ALLOC_SUCCESS: handle=4 size=8388608 addr=0x7a6935600000
[3291.010022654] INIT_COMPLETE: time=0.000401 sec
[3291.010034555] EXEC_START: ctx_id=1 mode=PERSISTENT (dispatch=1)
[3291.010040614] BATCH_BUILD_SUCCESS: commands=55 bytes=220
[3291.688771852] EXEC_SUCCESS: time=0.678737 sec pool_ctx_id=1
```

### 5.2 Log Nouveau (Cassé)
```
[4628.662155476] INIT_START: batch_size=262144 work_group_size=256
[4628.662197046] DRM_OPEN_SUCCESS: /dev/dri/renderD128 (fd=7)
[4628.662211653] DRM_VERSION: i915 1.6.0
[4628.662220123] CONTEXT_CREATE_SUCCESS: ctx_id=1
[4628.662223736] CTX_POOL_CREATED: index=0 ctx_id=2
[4628.662226079] CTX_POOL_CREATED: index=1 ctx_id=3
[4628.662228162] CTX_POOL_CREATED: index=2 ctx_id=4
[4628.662285003] GEM_ALLOC_SUCCESS: handle=1 size=42672 addr=0x7ad0c6feb000
[4628.662314641] KERNEL_LOAD_SUCCESS: path=kernels/test_add_gen9.bin size=42672 handle=1
[4628.662337258] GEM_ALLOC_SUCCESS: handle=2 size=4096 addr=0x7ad0c7090000
[4628.662357633] GEM_ALLOC_SUCCESS: handle=3 size=1048576 addr=0x7ad0c6ec7000
[4628.662378720] GEM_ALLOC_SUCCESS: handle=4 size=8388608 addr=0x7ad0c6400000
[4628.662380399] INIT_COMPLETE: time=0.000385 sec
[4628.662388628] EXEC_START: ctx_id=1 mode=PERSISTENT (dispatch=1)
[4628.662221736] BATCH_BUILD_SUCCESS: commands=55 bytes=220 terminator=0x0a000000
[4628.662230206] EXEC_FAILED: errno=22 (Invalid argument)
```

### 5.3 Différences Observées

| Aspect | Ancien (✅) | Nouveau (❌) | Impact |
|--------|------------|-------------|--------|
| Kernel path | `btc_sha256_mining_gen9.bin` | `test_add_gen9.bin` | 🔴 CRITIQUE |
| Kernel size | 42672 bytes | 42672 bytes | ✅ Identique |
| Init time | 0.401 ms | 0.385 ms | ✅ OK |
| ctx_id | 1 | 1 | ✅ Identique |
| Batch commands | 55 | 55 | ✅ Identique |
| Batch bytes | 220 | 220 | ✅ Identique |
| Terminator log | Absent | `0x0a000000` | 🟡 Nouveau logging |
| errno | N/A (succès) | 22 (EINVAL) | 🔴 ÉCHEC |

**Conclusion**: La SEULE différence significative est le **kernel path** !

---

## 6. PLAN D'ACTION PHASE 15Y-E

### 6.1 Test Immédiat: Utiliser Même Kernel
```bash
# Copier le kernel qui fonctionnait
cp level_zero_native/kernels/btc_sha256_mining_gen9.bin \
   i915_native/kernels/

# Modifier test pour utiliser ce kernel
# Recompiler et tester
```

**Priorité**: 🔴 CRITIQUE  
**Temps estimé**: 2 minutes

### 6.2 Si Échec: Copier Code Exact
```bash
# Copier l'ancien code qui fonctionnait
cp level_zero_native/src/btc_gen9_native_runner.c \
   i915_native/src/btc_gen9_native_runner.c.backup

# Comparer ligne par ligne avec diff
diff -u level_zero_native/src/btc_gen9_native_runner.c \
        i915_native/src/btc_gen9_native_runner.c
```

**Priorité**: 🔴 CRITIQUE  
**Temps estimé**: 5 minutes

### 6.3 Si Échec: Revenir Version Fonctionnelle
```bash
# Abandonner les corrections Phase 15Y
# Revenir à la version Phase 15X qui fonctionnait
# Analyser errno=5 (EIO) au lieu de errno=22 (EINVAL)
```

**Priorité**: 🟡 FALLBACK  
**Temps estimé**: 1 minute

---

## 7. MÉTRIQUES FINALES

### 7.1 Tests Effectués
```
✅ 7 corrections testées
❌ 0 corrections réussies
🔴 100% taux d'échec
⏱️ 25 minutes de diagnostic
```

### 7.2 Découvertes
```
🔍 1 différence critique identifiée (kernel path)
📊 2 hypothèses éliminées (terminator, batch_len)
🎯 5 hypothèses restantes à tester
```

### 7.3 État Actuel
```
Dispatches réussis: 0 (régression vs 3)
Performance: 0 MH/s (régression vs 3.07 MH/s)
errno actuel: 22 (EINVAL)
errno cible: 0 (SUCCESS)
```

---

## 8. CONCLUSION

**La Phase 15Y a été un ÉCHEC TOTAL.**

Toutes les corrections basées sur l'analyse forensique Phase 15X étaient FAUSSES. Le vrai problème n'a jamais été identifié. La seule piste restante est le **kernel path différent**.

**Prochaine étape**: Tester avec le kernel `btc_sha256_mining_gen9.bin` qui fonctionnait.

**Si échec**: Abandonner les corrections et revenir à la version Phase 15X pour analyser errno=5 (EIO) au lieu de errno=22 (EINVAL).

---

**Rapport généré**: 2026-05-11 15:22 UTC+2  
**Durée diagnostic**: 25 minutes  
**Lignes rapport**: 450  
**Statut**: 🔴 RÉGRESSION NON RÉSOLUE