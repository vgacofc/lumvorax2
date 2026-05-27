# RAPPORT C277 — PLAN IMPLÉMENTATION FORMAT INTEL
## Modification btc_gen9_native_runner.c pour Support VM + CREATE_EXT

**Date**: 2026-05-18 20:41  
**Fichier cible**: [`btc_gen9_native_runner.c`](../src/btc_gen9_native_runner.c)  
**Objectif**: Passer de 27 dispatches max à ∞ dispatches

---

## 1. ANALYSE CODE ACTUEL

### 1.1 Structure Contexte (Lignes 52-100)

**Actuel**:
```c
typedef struct {
    int drm_fd;
    uint32_t ctx_id;               // Contexte principal (non utilisé)
    uint32_t ctx_pool[9];          // Pool de 9 contextes
    int ctx_pool_index;
    int ctx_usage_count[9];
    // ... autres champs
} btc_gen9_context_t;
```

**Manque**:
- ❌ Pas de `vm_id` (Virtual Memory)
- ❌ Pas de tracking VM par contexte
- ❌ Pas de support CREATE_EXT

### 1.2 Fonction create_gpu_context (Lignes 475-487)

**Actuel**:
```c
static int create_gpu_context(btc_gen9_context_t* ctx) {
    struct drm_i915_gem_context_create create = {0};
    
    int ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &create);
    if (ret < 0) {
        LOG_EVENT(ctx, "CONTEXT_CREATE_FAILED: errno=%d (%s)", errno, strerror(errno));
        return -1;
    }
    
    ctx->ctx_id = create.ctx_id;
    LOG_EVENT(ctx, "CONTEXT_CREATE_SUCCESS: ctx_id=%u", ctx->ctx_id);
    return 0;
}
```

**Problème**: Utilise CREATE simple, pas CREATE_EXT avec VM.

### 1.3 Création Pool Contextes (Lignes 684-694)

**Actuel**:
```c
for (int i = 0; i < CTX_POOL_SIZE; i++) {
    struct drm_i915_gem_context_create ctx_create = {0};
    int ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create);
    if (ret < 0) {
        LOG_EVENT(ctx, "CTX_POOL_CREATE_FAILED: index=%d errno=%d", i, errno);
        goto error;
    }
    ctx->ctx_pool[i] = ctx_create.ctx_id;
    ctx->ctx_usage_count[i] = 0;
    LOG_EVENT(ctx, "CTX_POOL_CREATED: index=%d ctx_id=%u", i, ctx->ctx_pool[i]);
}
```

**Problème**: Chaque contexte créé sans VM → Limite 3 utilisations.

### 1.4 Recyclage Contextes (Lignes 930-939)

**Actuel**:
```c
struct drm_i915_gem_context_create create = {0};
ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &create);
if (ret < 0) {
    LOG_EVENT(ctx, "CTX_RECYCLE_CREATE_FAILED: errno=%d (%s)", errno, strerror(errno));
    return -1;
}
ctx->ctx_pool[pool_index] = create.ctx_id;
ctx->ctx_usage_count[pool_index] = 0;
```

**Problème**: Recrée contexte sans VM → Échec après 27 dispatches.

### 1.5 DRM Reopen (Lignes 1258-1267)

**Actuel**:
```c
for (int i = 0; i < CTX_POOL_SIZE; i++) {
    struct drm_i915_gem_context_create create = {0};
    ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &create);
    if (ret < 0) {
        LOG_EVENT(ctx, "DRM_REOPEN_CTX_CREATE_FAILED: index=%d errno=%d", i, errno);
        return -1;
    }
    ctx->ctx_pool[i] = create.ctx_id;
    ctx->ctx_usage_count[i] = 0;
    LOG_EVENT(ctx, "DRM_REOPEN_CTX_CREATED: index=%d ctx_id=%u", i, ctx->ctx_pool[i]);
}
```

**Problème**: Recrée contextes sans VM → errno=5 au dispatch 28.

---

## 2. MODIFICATIONS REQUISES

### 2.1 Structure Contexte (Ligne 52)

**AVANT**:
```c
typedef struct {
    int drm_fd;
    uint32_t ctx_id;
    uint32_t ctx_pool[9];
    // ...
} btc_gen9_context_t;
```

**APRÈS**:
```c
typedef struct {
    int drm_fd;
    uint32_t ctx_id;               // Contexte principal (deprecated)
    
    /* C277: Support VM Intel pour isolation contextes */
    uint32_t vm_pool[CTX_POOL_SIZE];      // Pool de 9 VMs (1 VM par contexte)
    uint32_t ctx_pool[CTX_POOL_SIZE];     // Pool de 9 contextes
    int ctx_pool_index;
    int ctx_usage_count[CTX_POOL_SIZE];
    // ...
} btc_gen9_context_t;
```

### 2.2 Nouvelle Fonction: create_gpu_context_with_vm

**Ajouter après ligne 487**:

```c
/**
 * C277: Crée contexte GPU avec VM (format Intel correct)
 * 
 * DÉCOUVERTE REVERSE ENGINEERING OpenCL:
 * - OpenCL utilise DRM_IOCTL_I915_GEM_VM_CREATE
 * - OpenCL utilise DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT
 * - OpenCL fait 3× CONTEXT_SETPARAM (VM, RECOVERABLE, PERSISTENCE)
 * 
 * OBJECTIF: Éliminer limite 27 dispatches (9 ctx × 3 utilisations)
 * SOLUTION: VM isole chaque contexte → ∞ utilisations possibles
 */
static int create_gpu_context_with_vm(btc_gen9_context_t* ctx, 
                                      uint32_t* vm_id_out, 
                                      uint32_t* ctx_id_out) {
    int ret;
    
    /* ÉTAPE 1: Créer VM (Virtual Memory) */
    struct drm_i915_gem_vm_create vm_create = {0};
    ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_VM_CREATE, &vm_create);
    if (ret < 0) {
        LOG_EVENT(ctx, "VM_CREATE_FAILED: errno=%d (%s)", errno, strerror(errno));
        return -1;
    }
    *vm_id_out = vm_create.vm_id;
    LOG_EVENT(ctx, "VM_CREATE_SUCCESS: vm_id=%u", *vm_id_out);
    
    /* ÉTAPE 2: Créer contexte avec CREATE_EXT */
    struct drm_i915_gem_context_create_ext ctx_create = {
        .flags = 0,
        .extensions = 0  // Pas d'extensions pour l'instant
    };
    ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT, &ctx_create);
    if (ret < 0) {
        LOG_EVENT(ctx, "CONTEXT_CREATE_EXT_FAILED: errno=%d (%s)", errno, strerror(errno));
        /* Cleanup VM */
        struct drm_i915_gem_vm_destroy vm_destroy = { .vm_id = *vm_id_out };
        ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_VM_DESTROY, &vm_destroy);
        return -1;
    }
    *ctx_id_out = ctx_create.ctx_id;
    LOG_EVENT(ctx, "CONTEXT_CREATE_EXT_SUCCESS: ctx_id=%u", *ctx_id_out);
    
    /* ÉTAPE 3: Associer VM au contexte */
    struct drm_i915_gem_context_param param = {
        .ctx_id = *ctx_id_out,
        .param = I915_CONTEXT_PARAM_VM,
        .value = *vm_id_out
    };
    ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_SETPARAM, &param);
    if (ret < 0) {
        LOG_EVENT(ctx, "CONTEXT_SETPARAM_VM_FAILED: errno=%d (%s)", errno, strerror(errno));
        /* Cleanup contexte et VM */
        struct drm_i915_gem_context_destroy ctx_destroy = { .ctx_id = *ctx_id_out };
        ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_destroy);
        struct drm_i915_gem_vm_destroy vm_destroy = { .vm_id = *vm_id_out };
        ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_VM_DESTROY, &vm_destroy);
        return -1;
    }
    LOG_EVENT(ctx, "CONTEXT_SETPARAM_VM_SUCCESS: ctx_id=%u vm_id=%u", *ctx_id_out, *vm_id_out);
    
    /* ÉTAPE 4: Activer recovery (optionnel mais recommandé) */
    param.param = I915_CONTEXT_PARAM_RECOVERABLE;
    param.value = 1;
    ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_SETPARAM, &param);
    if (ret < 0) {
        LOG_EVENT(ctx, "CONTEXT_SETPARAM_RECOVERABLE_FAILED: errno=%d (non-fatal)", errno);
        /* Non-fatal, continue */
    } else {
        LOG_EVENT(ctx, "CONTEXT_SETPARAM_RECOVERABLE_SUCCESS");
    }
    
    /* ÉTAPE 5: Désactiver persistence (meilleur pour compute) */
    param.param = I915_CONTEXT_PARAM_PERSISTENCE;
    param.value = 0;
    ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_SETPARAM, &param);
    if (ret < 0) {
        LOG_EVENT(ctx, "CONTEXT_SETPARAM_PERSISTENCE_FAILED: errno=%d (non-fatal)", errno);
        /* Non-fatal, continue */
    } else {
        LOG_EVENT(ctx, "CONTEXT_SETPARAM_PERSISTENCE_SUCCESS");
    }
    
    LOG_EVENT(ctx, "CREATE_GPU_CONTEXT_WITH_VM_COMPLETE: vm_id=%u ctx_id=%u", 
             *vm_id_out, *ctx_id_out);
    return 0;
}
```

### 2.3 Modifier create_gpu_context (Ligne 475)

**AVANT**:
```c
static int create_gpu_context(btc_gen9_context_t* ctx) {
    struct drm_i915_gem_context_create create = {0};
    
    int ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &create);
    if (ret < 0) {
        LOG_EVENT(ctx, "CONTEXT_CREATE_FAILED: errno=%d (%s)", errno, strerror(errno));
        return -1;
    }
    
    ctx->ctx_id = create.ctx_id;
    LOG_EVENT(ctx, "CONTEXT_CREATE_SUCCESS: ctx_id=%u", ctx->ctx_id);
    return 0;
}
```

**APRÈS**:
```c
static int create_gpu_context(btc_gen9_context_t* ctx) {
    /* C277: Utiliser format Intel avec VM (deprecated: contexte principal non utilisé) */
    uint32_t vm_id, ctx_id;
    int ret = create_gpu_context_with_vm(ctx, &vm_id, &ctx_id);
    if (ret < 0) {
        return -1;
    }
    
    ctx->ctx_id = ctx_id;
    LOG_EVENT(ctx, "CONTEXT_CREATE_SUCCESS: ctx_id=%u (avec VM)", ctx->ctx_id);
    return 0;
}
```

### 2.4 Modifier Création Pool (Ligne 684)

**AVANT**:
```c
for (int i = 0; i < CTX_POOL_SIZE; i++) {
    struct drm_i915_gem_context_create ctx_create = {0};
    int ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create);
    if (ret < 0) {
        LOG_EVENT(ctx, "CTX_POOL_CREATE_FAILED: index=%d errno=%d", i, errno);
        goto error;
    }
    ctx->ctx_pool[i] = ctx_create.ctx_id;
    ctx->ctx_usage_count[i] = 0;
    LOG_EVENT(ctx, "CTX_POOL_CREATED: index=%d ctx_id=%u", i, ctx->ctx_pool[i]);
}
```

**APRÈS**:
```c
/* C277: Créer pool avec VM (format Intel correct) */
for (int i = 0; i < CTX_POOL_SIZE; i++) {
    uint32_t vm_id, ctx_id;
    int ret = create_gpu_context_with_vm(ctx, &vm_id, &ctx_id);
    if (ret < 0) {
        LOG_EVENT(ctx, "CTX_POOL_CREATE_FAILED: index=%d", i);
        goto error;
    }
    ctx->vm_pool[i] = vm_id;
    ctx->ctx_pool[i] = ctx_id;
    ctx->ctx_usage_count[i] = 0;
    LOG_EVENT(ctx, "CTX_POOL_CREATED: index=%d vm_id=%u ctx_id=%u", 
             i, vm_id, ctx_id);
}
```

### 2.5 Modifier Recyclage Contextes (Ligne 930)

**AVANT**:
```c
struct drm_i915_gem_context_create create = {0};
ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &create);
if (ret < 0) {
    LOG_EVENT(ctx, "CTX_RECYCLE_CREATE_FAILED: errno=%d (%s)", errno, strerror(errno));
    return -1;
}
ctx->ctx_pool[pool_index] = create.ctx_id;
ctx->ctx_usage_count[pool_index] = 0;
```

**APRÈS**:
```c
/* C277: Détruire VM avant de recréer contexte */
struct drm_i915_gem_vm_destroy vm_destroy = {
    .vm_id = ctx->vm_pool[pool_index]
};
ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_VM_DESTROY, &vm_destroy);
if (ret < 0) {
    LOG_EVENT(ctx, "CTX_RECYCLE_VM_DESTROY_FAILED: errno=%d", errno);
    /* Non-fatal, continue */
}
LOG_EVENT(ctx, "CTX_RECYCLE_VM_DESTROYED: vm_id=%u", ctx->vm_pool[pool_index]);

/* Créer nouveau contexte avec nouvelle VM */
uint32_t vm_id, ctx_id;
ret = create_gpu_context_with_vm(ctx, &vm_id, &ctx_id);
if (ret < 0) {
    LOG_EVENT(ctx, "CTX_RECYCLE_CREATE_FAILED");
    return -1;
}
ctx->vm_pool[pool_index] = vm_id;
ctx->ctx_pool[pool_index] = ctx_id;
ctx->ctx_usage_count[pool_index] = 0;
LOG_EVENT(ctx, "CTX_RECYCLE_SUCCESS: index=%d vm_id=%u ctx_id=%u",
         pool_index, vm_id, ctx_id);
```

### 2.6 Modifier DRM Reopen (Ligne 1258)

**AVANT**:
```c
for (int i = 0; i < CTX_POOL_SIZE; i++) {
    struct drm_i915_gem_context_create create = {0};
    ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &create);
    if (ret < 0) {
        LOG_EVENT(ctx, "DRM_REOPEN_CTX_CREATE_FAILED: index=%d errno=%d", i, errno);
        return -1;
    }
    ctx->ctx_pool[i] = create.ctx_id;
    ctx->ctx_usage_count[i] = 0;
    LOG_EVENT(ctx, "DRM_REOPEN_CTX_CREATED: index=%d ctx_id=%u", i, ctx->ctx_pool[i]);
}
```

**APRÈS**:
```c
/* C277: Recréer pool avec VM (format Intel correct) */
for (int i = 0; i < CTX_POOL_SIZE; i++) {
    uint32_t vm_id, ctx_id;
    ret = create_gpu_context_with_vm(ctx, &vm_id, &ctx_id);
    if (ret < 0) {
        LOG_EVENT(ctx, "DRM_REOPEN_CTX_CREATE_FAILED: index=%d", i);
        return -1;
    }
    ctx->vm_pool[i] = vm_id;
    ctx->ctx_pool[i] = ctx_id;
    ctx->ctx_usage_count[i] = 0;
    LOG_EVENT(ctx, "DRM_REOPEN_CTX_CREATED: index=%d vm_id=%u ctx_id=%u", 
             i, vm_id, ctx_id);
}
```

### 2.7 Cleanup Contextes (Ajouter destruction VM)

**Chercher fonction cleanup** et ajouter destruction VMs:

```c
/* C277: Détruire VMs avant contextes */
for (int i = 0; i < CTX_POOL_SIZE; i++) {
    if (ctx->vm_pool[i] != 0) {
        struct drm_i915_gem_vm_destroy vm_destroy = {
            .vm_id = ctx->vm_pool[i]
        };
        ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_VM_DESTROY, &vm_destroy);
        LOG_EVENT(ctx, "CLEANUP_VM_DESTROYED: index=%d vm_id=%u", i, ctx->vm_pool[i]);
    }
}
```

---

## 3. HEADERS REQUIS

**Vérifier présence dans i915_drm.h**:

```c
/* Devrait être présent dans /usr/include/drm/i915_drm.h */
#define DRM_IOCTL_I915_GEM_VM_CREATE        DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_VM_CREATE, struct drm_i915_gem_vm_create)
#define DRM_IOCTL_I915_GEM_VM_DESTROY       DRM_IOW(DRM_COMMAND_BASE + DRM_I915_GEM_VM_DESTROY, struct drm_i915_gem_vm_destroy)
#define DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_CONTEXT_CREATE, struct drm_i915_gem_context_create_ext)

struct drm_i915_gem_vm_create {
    __u64 extensions;
    __u32 flags;
    __u32 vm_id;
};

struct drm_i915_gem_vm_destroy {
    __u32 vm_id;
};

struct drm_i915_gem_context_create_ext {
    __u32 ctx_id;
    __u32 flags;
    __u64 extensions;
};

#define I915_CONTEXT_PARAM_VM           0x9
#define I915_CONTEXT_PARAM_RECOVERABLE  0x8
#define I915_CONTEXT_PARAM_PERSISTENCE  0xb
```

**Si manquant**, ajouter après ligne 43:

```c
/* C277: Définitions VM si absentes de i915_drm.h */
#ifndef DRM_IOCTL_I915_GEM_VM_CREATE
#define DRM_I915_GEM_VM_CREATE 0x3a
#define DRM_I915_GEM_VM_DESTROY 0x3b
#define DRM_IOCTL_I915_GEM_VM_CREATE DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_VM_CREATE, struct drm_i915_gem_vm_create)
#define DRM_IOCTL_I915_GEM_VM_DESTROY DRM_IOW(DRM_COMMAND_BASE + DRM_I915_GEM_VM_DESTROY, struct drm_i915_gem_vm_destroy)

struct drm_i915_gem_vm_create {
    __u64 extensions;
    __u32 flags;
    __u32 vm_id;
};

struct drm_i915_gem_vm_destroy {
    __u32 vm_id;
};

struct drm_i915_gem_context_create_ext {
    __u32 ctx_id;
    __u32 flags;
    __u64 extensions;
};

#define I915_CONTEXT_PARAM_VM 0x9
#define I915_CONTEXT_PARAM_RECOVERABLE 0x8
#define I915_CONTEXT_PARAM_PERSISTENCE 0xb
#endif
```

---

## 4. RÉSUMÉ MODIFICATIONS

### Fichiers à Modifier

1. **btc_gen9_native_runner.c** (1 fichier, 7 modifications)

### Lignes à Modifier

| Ligne | Fonction | Modification |
|-------|----------|--------------|
| 52-100 | Structure | Ajouter `uint32_t vm_pool[9]` |
| 475-487 | create_gpu_context | Utiliser create_gpu_context_with_vm |
| 487+ | NOUVEAU | Ajouter create_gpu_context_with_vm (100 lignes) |
| 684-694 | Init pool | Utiliser create_gpu_context_with_vm |
| 930-939 | Recyclage | Détruire VM + recréer avec VM |
| 1258-1267 | DRM reopen | Recréer pool avec VM |
| Cleanup | Destruction | Ajouter destruction VMs |

### Lignes de Code

- **Ajoutées**: ~150 lignes (fonction create_gpu_context_with_vm + modifications)
- **Modifiées**: ~50 lignes (7 emplacements)
- **Total**: ~200 lignes de modifications

---

## 5. PRÉDICTIONS APRÈS IMPLÉMENTATION

### 5.1 Avant (Code Actuel)

```
Max dispatches: 27 (9 contextes × 3 utilisations)
Cause: Pas de VM, contextes corrompus après 3 utilisations
Symptôme: errno=5 au dispatch 28
```

### 5.2 Après (Format Intel avec VM)

```
Max dispatches: ∞
Raison: VM isole chaque contexte, nettoyage complet
Symptôme: Aucun errno, exécution continue
```

### 5.3 Test Validation

```bash
# Compiler
cd src/advanced_calculations/bitcoin_quantum_mining/level_zero_native
make clean && make

# Tester 1000 dispatches
./btc_gen9_native_runner --dispatches 1000

# Attendu:
# - 1000 dispatches réussis
# - 0 errno=5
# - 0 "preemption timeout"
# - Hashrate stable ~408 MH/s
```

---

## 6. CHECKLIST IMPLÉMENTATION

- [ ] Modifier structure btc_gen9_context_t (ajouter vm_pool)
- [ ] Ajouter fonction create_gpu_context_with_vm
- [ ] Modifier create_gpu_context (utiliser nouvelle fonction)
- [ ] Modifier création pool contextes (ligne 684)
- [ ] Modifier recyclage contextes (ligne 930)
- [ ] Modifier DRM reopen (ligne 1258)
- [ ] Ajouter destruction VMs dans cleanup
- [ ] Vérifier headers i915_drm.h (ajouter si manquant)
- [ ] Compiler et tester
- [ ] Valider 1000 dispatches sans erreur

---

## 7. PROCHAINE ACTION

**IMMÉDIAT**: Implémenter les 7 modifications dans btc_gen9_native_runner.c

**COMMANDE**:
```bash
# Backup avant modification
cp src/btc_gen9_native_runner.c src/btc_gen9_native_runner.c.backup_c277

# Appliquer modifications (via apply_diff)
# ... (7 apply_diff successifs)

# Compiler
make clean && make

# Tester
./btc_gen9_native_runner --dispatches 100
```

---

**FIN RAPPORT C277 — PLAN IMPLÉMENTATION FORMAT INTEL**

**Découverte**: Format Intel nécessite VM + CREATE_EXT + SETPARAM  
**Solution**: 7 modifications, ~200 lignes code  
**Résultat attendu**: ∞ dispatches, 408 MH/s stable