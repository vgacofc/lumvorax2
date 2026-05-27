# RAPPORT ANALYSE C255 — ÉCHEC PARALLÉLISME

**CYCLE**: C255  
**DATE**: 2026-05-15  
**STATUT**: ❌ ÉCHEC - Dispatches séquentiels au lieu de parallèles  
**HASHRATE MESURÉ**: 256 MH/s (vs 423 MH/s baseline C251)

---

## 📊 RÉSULTATS TEST

### Métriques Mesurées
| Métrique | Valeur | Attendu | Écart |
|----------|--------|---------|-------|
| **Hashrate** | 256.14 MH/s | 5-6 GH/s | **-95%** ❌ |
| **vs C251** | 0.6× | 12-15× | **-96%** ❌ |
| **Dispatches réussis** | 8/9 | 9/9 | -11% ⚠️ |
| **Temps total** | 8.384 sec | ~1 sec | +738% ❌ |
| **Temps par dispatch** | 1.048 sec | 0.111 sec | +844% ❌ |

### Verdict
🚨 **RÉGRESSION MAJEURE**: Performance **PIRE** que C251 baseline!

---

## 🔍 ANALYSE FORENSIQUE LOGS

### Problème #1: Dispatches Séquentiels (CRITIQUE)

**Preuve dans les logs**:
```
[2041.931622844] EXEC_START: ctx_id=2 dispatch=3
[2045.989154907] EXEC_SUCCESS: time=4.057453 sec  ← Thread 1 attend 4s
[2050.209568023] EXEC_SUCCESS: time=8.277945 sec  ← Thread 2 attend 8s
[2050.212875151] EXEC_SUCCESS: time=8.280893 sec  ← Thread 3 attend 8s
```

**Analyse**:
- Dispatch 3 démarre à `2041.931s`
- Dispatch 3 termine à `2045.989s` → **4 secondes d'attente**
- Dispatch 4 termine à `2050.209s` → **8 secondes d'attente**
- **Total: 12+ secondes pour 3 dispatches** (devrait être ~1 seconde en parallèle)

**Cause**: `btc_gen9_execute()` est **SYNCHRONE** (contient `GEM_WAIT`)

### Problème #2: Race Condition DRM Reopen (CRITIQUE)

**Preuve dans les logs**:
```
[2050.215117860] DRM_REOPEN_TRIGGER: dispatches=9  ← Thread 1
[2050.215155267] DRM_REOPEN_TRIGGER: dispatches=9  ← Thread 2
[2050.215168043] DRM_REOPEN_TRIGGER: dispatches=9  ← Thread 3
[2050.217124534] DRM_REOPEN_TRIGGER: dispatches=9  ← Thread 4
```

**Analyse**:
- **4 threads** tentent de reopen DRM **simultanément**
- Chaque thread détruit les contextes → **race condition**
- Résultat: `errno=9 (EBADF)` → contextes détruits pendant utilisation

**Logs erreur**:
```
[2050.218271246] DRM_REOPEN_CTX_DESTROY_FAILED: index=0 errno=9
[2050.230347922] DRM_REOPEN_BATCH_MMAP_FAILED: errno=9
[2050.240202941] DRM_REOPEN_FAILED: errno=9
[2050.315369690] C255_DISPATCH_FAILED: dispatch=2 ctx_idx=0 result=-1
```

### Problème #3: Métriques Exec Time Incorrectes

**Logs**:
```
Min Exec Time: 0.000000 sec  ← FAUX!
Max Exec Time: 0.000000 sec  ← FAUX!
Avg Exec Time: 1.048004 sec  ← Calculé sur temps total, pas exec GPU
```

**Cause**: Le code C255 ne mesure PAS le temps d'exécution GPU réel, seulement le temps de soumission.

### Problème #4: Ordre Dispatches Incohérent

**Logs**:
```
dispatch=3 → dispatch=4 → dispatch=2 → dispatch=5 → dispatch=6 → ...
```

**Attendu**:
```
dispatch=1 → dispatch=2 → dispatch=3 → dispatch=4 → ...
```

**Cause**: Threads démarrent dans un ordre non déterministe.

---

## 🐛 BUGS IDENTIFIÉS

### Bug #1: btc_gen9_execute() est Synchrone
**Fichier**: `btc_gen9_native_runner.c:886-896`  
**Code**:
```c
/* Attendre fin exécution */
struct drm_i915_gem_wait wait = {
    .bo_handle = ctx->batch_bo,
    .timeout_ns = 30000000000ULL,  /* 30 secondes */
    .flags = 0
};

ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait);  ← BLOQUE ICI!
```

**Impact**: Chaque thread attend la fin de son dispatch → **exécution séquentielle**

**Solution**: Séparer soumission et attente:
```c
// Phase 1: Soumission (asynchrone)
int btc_gen9_submit_async(ctx, &fence_out);

// Phase 2: Attente (synchrone, mais après toutes les soumissions)
int btc_gen9_wait_fence(ctx, fence);
```

### Bug #2: DRM Reopen Non Thread-Safe
**Fichier**: `btc_gen9_native_runner.c:1000-1100`  
**Code**:
```c
int btc_gen9_reopen_drm(btc_gen9_context_t* ctx) {
    // Détruit contextes sans mutex → RACE CONDITION!
    for (int i = 0; i < CTX_POOL_SIZE; i++) {
        ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, ...);
    }
}
```

**Impact**: Multiples threads détruisent les mêmes contextes simultanément

**Solution**: Ajouter mutex global:
```c
static pthread_mutex_t g_drm_reopen_mutex = PTHREAD_MUTEX_INITIALIZER;

int btc_gen9_reopen_drm(btc_gen9_context_t* ctx) {
    pthread_mutex_lock(&g_drm_reopen_mutex);
    // ... reopen logic ...
    pthread_mutex_unlock(&g_drm_reopen_mutex);
}
```

### Bug #3: Compteur total_dispatches Non Atomique
**Fichier**: `btc_gen9_native_runner.c:901`  
**Code**:
```c
ctx->total_dispatches++;  ← NON ATOMIQUE!
```

**Impact**: Race condition sur compteur → reopen déclenché plusieurs fois

**Solution**: Utiliser atomic:
```c
__atomic_fetch_add(&ctx->total_dispatches, 1, __ATOMIC_SEQ_CST);
```

---

## 🔧 CORRECTIONS NÉCESSAIRES

### Correction #1: Implémentation Vraie Soumission Asynchrone

**Nouveau fichier**: `btc_gen9_async_submit.c`

```c
/**
 * Structure fence pour tracking dispatch asynchrone
 */
typedef struct {
    uint32_t batch_bo;
    int drm_fd;
    uint64_t submit_ts;
} btc_gen9_fence_t;

/**
 * Soumettre dispatch asynchrone (retour immédiat)
 */
int btc_gen9_submit_async(btc_gen9_context_t* ctx, btc_gen9_fence_t* fence_out) {
    // Construire batch buffer
    build_batch_buffer(ctx);
    
    // Soumettre via EXECBUFFER2 (SANS GEM_WAIT)
    struct drm_i915_gem_execbuffer2 execbuf = {
        .buffers_ptr = (uintptr_t)exec_objects,
        .buffer_count = 4,
        .batch_start_offset = 0,
        .batch_len = ctx->batch_size,
        .flags = I915_EXEC_RENDER,
        .rsvd1 = ctx->ctx_id
    };
    
    int ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
    
    // Sauvegarder fence pour attente ultérieure
    fence_out->batch_bo = ctx->batch_bo;
    fence_out->drm_fd = ctx->drm_fd;
    fence_out->submit_ts = get_timestamp_ns();
    
    return ret;  // RETOUR IMMÉDIAT (pas d'attente)
}

/**
 * Attendre fin dispatch via fence
 */
int btc_gen9_wait_fence(const btc_gen9_fence_t* fence) {
    struct drm_i915_gem_wait wait = {
        .bo_handle = fence->batch_bo,
        .timeout_ns = 30000000000ULL,
        .flags = 0
    };
    
    return ioctl(fence->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
}
```

### Correction #2: Architecture C255v2 Corrigée

```c
int c255v2_execute_multi_dispatch_batch(btc_gen9_context_t* ctx, int num_batches) {
    btc_gen9_fence_t fences[C255_TOTAL_DISPATCHES];
    
    for (int batch = 0; batch < num_batches; batch++) {
        // PHASE 1: Soumission asynchrone (9 dispatches)
        for (int i = 0; i < C255_TOTAL_DISPATCHES; i++) {
            int ctx_idx = i / 3;
            btc_gen9_set_ctx_id(ctx, btc_gen9_get_ctx_pool_id(ctx, ctx_idx));
            
            // Soumettre SANS attendre
            btc_gen9_submit_async(ctx, &fences[i]);
        }
        
        // PHASE 2: Attente parallèle (tous les dispatches en même temps)
        for (int i = 0; i < C255_TOTAL_DISPATCHES; i++) {
            btc_gen9_wait_fence(&fences[i]);
        }
        
        // PHASE 3: Reopen DRM (thread-safe)
        pthread_mutex_lock(&g_drm_reopen_mutex);
        btc_gen9_reopen_drm(ctx);
        pthread_mutex_unlock(&g_drm_reopen_mutex);
    }
}
```

### Correction #3: Mutex DRM Reopen

```c
static pthread_mutex_t g_drm_reopen_mutex = PTHREAD_MUTEX_INITIALIZER;

int btc_gen9_reopen_drm(btc_gen9_context_t* ctx) {
    pthread_mutex_lock(&g_drm_reopen_mutex);
    
    // ... logique reopen existante ...
    
    pthread_mutex_unlock(&g_drm_reopen_mutex);
    return 0;
}
```

---

## 📈 GAIN ATTENDU APRÈS CORRECTIONS

### Scénario Optimiste
| Métrique | Avant (C255) | Après (C255v2) | Gain |
|----------|--------------|----------------|------|
| Temps dispatch | 8.384 sec | 0.933 sec | ×9 |
| Hashrate | 256 MH/s | 2.3 GH/s | ×9 |
| vs C251 | 0.6× | 5.4× | ×9 |

### Calcul
- **9 dispatches séquentiels**: 9 × 1.048s = 9.432s
- **9 dispatches parallèles**: max(1.048s) = 1.048s
- **Gain théorique**: ×9

**Hashrate attendu**: 256 MH/s × 9 = **2.3 GH/s**

---

## 🎯 PLAN D'ACTION

### Immédiat (Aujourd'hui)
1. ✅ Analyser logs C255 (fait)
2. ✅ Identifier bugs critiques (fait)
3. ⏳ Implémenter `btc_gen9_submit_async()` + `btc_gen9_wait_fence()`
4. ⏳ Ajouter mutex DRM reopen
5. ⏳ Créer C255v2 avec architecture corrigée

### Court Terme (Demain)
1. Compiler et tester C255v2
2. Valider hashrate 2-3 GH/s
3. Analyser logs forensiques C255v2
4. Documenter résultats

### Moyen Terme (Cette Semaine)
1. Optimiser davantage si <2 GH/s
2. Implémenter pipeline multi-batch
3. Viser 5-6 GH/s

---

## 📝 LEÇONS APPRISES

### Erreur Conceptuelle
❌ **Hypothèse fausse**: `btc_gen9_execute()` était asynchrone  
✅ **Réalité**: `btc_gen9_execute()` contient `GEM_WAIT` → synchrone

### Erreur Implémentation
❌ **Hypothèse fausse**: Threads = parallélisme automatique  
✅ **Réalité**: Threads qui attendent = séquentiel

### Erreur Thread-Safety
❌ **Hypothèse fausse**: DRM reopen thread-safe  
✅ **Réalité**: Race conditions multiples

---

**FIN DU RAPPORT C255 ANALYSE ÉCHEC**

*Prochaine étape: Implémentation C255v2 avec corrections*