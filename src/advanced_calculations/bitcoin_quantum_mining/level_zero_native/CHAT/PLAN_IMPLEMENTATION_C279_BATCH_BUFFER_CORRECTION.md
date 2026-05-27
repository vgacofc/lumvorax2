# PLAN D'IMPLÉMENTATION C279 — CORRECTION BATCH BUFFER
## Guide Étape par Étape pour Atteindre 100+ Dispatches

**Date**: 2026-05-18  
**Objectif**: Corriger batch buffer invalide → 100+ dispatches sans GPU hang  
**Complexité**: Élevée (structures GPU Gen9)  
**Temps estimé**: 4-6 heures de développement

---

## ⚠️ AVERTISSEMENT IMPORTANT

Cette implémentation nécessite une connaissance approfondie de l'architecture GPU Gen9 Intel. Les structures doivent être **EXACTEMENT** conformes à la spécification Intel, sinon le GPU hang persistera.

**Recommandation** : Utiliser OpenCL pour le mining Bitcoin réel (100% fonctionnel) pendant que cette implémentation est développée et testée.

---

## 📋 ÉTAPE 1 : CRÉER STRUCTURES GEN9 (Nouveau fichier)

### Fichier : `include/btc_gen9_structures.h`

```c
#ifndef BTC_GEN9_STRUCTURES_H
#define BTC_GEN9_STRUCTURES_H

#include <stdint.h>

/* Gen9 Surface State (64 bytes) */
struct gen9_surface_state {
    uint32_t dw0;  // Surface Type, Format, etc.
    uint32_t dw1;  // Base Address Low
    uint32_t dw2;  // Width, Height
    uint32_t dw3;  // Depth, Pitch
    uint32_t dw4;  // Min LOD, Mip Count
    uint32_t dw5;  // X Offset, Y Offset
    uint32_t dw6;  // Reserved
    uint32_t dw7;  // Resource Min LOD
    uint32_t dw8;  // Base Address High
    uint32_t dw9;  // Reserved
    uint32_t dw10; // Reserved
    uint32_t dw11; // Reserved
    uint32_t dw12; // Reserved
    uint32_t dw13; // Reserved
    uint32_t dw14; // Reserved
    uint32_t dw15; // Reserved
} __attribute__((packed));

/* Gen9 Surface Type */
#define GEN9_SURFACE_1D    0
#define GEN9_SURFACE_2D    1
#define GEN9_SURFACE_3D    2
#define GEN9_SURFACE_CUBE  3
#define GEN9_SURFACE_BUFFER 4

/* Gen9 Surface Format */
#define GEN9_SURFACEFORMAT_RAW  0x1FF

/* Gen9 Interface Descriptor (32 bytes) */
struct gen9_interface_descriptor {
    uint32_t dw0;  // Kernel Start Pointer Low
    uint32_t dw1;  // Kernel Start Pointer High
    uint32_t dw2;  // Reserved
    uint32_t dw3;  // Sampler State Pointer
    uint32_t dw4;  // Binding Table Pointer
    uint32_t dw5;  // Constant URB Entry Read Length
    uint32_t dw6;  // Number of Threads, Barrier Enable
    uint32_t dw7;  // Shared Local Memory Size
} __attribute__((packed));

#endif /* BTC_GEN9_STRUCTURES_H */
```

---

## 📋 ÉTAPE 2 : AJOUTER BUFFERS DANS CONTEXTE

### Fichier : `src/btc_gen9_native_runner.c` (lignes 52-143)

**Ajouter dans `btc_gen9_context_t`** :

```c
typedef struct {
    // ... (code existant)
    
    /* C279: Buffers pour batch buffer valide */
    uint32_t surface_state_bo;     /* Surface states (input/output) */
    void* surface_state_map;
    size_t surface_state_size;
    
    uint32_t binding_table_bo;     /* Binding table */
    void* binding_table_map;
    size_t binding_table_size;
    
    uint32_t interface_desc_bo;    /* Interface descriptor */
    void* interface_desc_map;
    size_t interface_desc_size;
    
    // ... (code existant)
} btc_gen9_context_t;
```

---

## 📋 ÉTAPE 3 : ALLOUER NOUVEAUX BUFFERS (Initialisation)

### Fichier : `src/btc_gen9_native_runner.c` (après ligne 957)

**Ajouter après allocation output_bo** :

```c
/* C279 CORRECTION #1: Allouer surface states (2 surfaces × 64 bytes) */
ctx->surface_state_size = 4096;  /* 1 page, suffisant pour plusieurs surfaces */
if (alloc_gpu_buffer(ctx, ctx->surface_state_size, 
                     &ctx->surface_state_bo, 
                     &ctx->surface_state_map) < 0) {
    LOG_EVENT(ctx, "SURFACE_STATE_ALLOC_FAILED");
    goto error;
}
LOG_EVENT(ctx, "SURFACE_STATE_CREATED: size=%zu handle=%u", 
         ctx->surface_state_size, ctx->surface_state_bo);

/* C279 CORRECTION #2: Allouer binding table (2 entries × 4 bytes) */
ctx->binding_table_size = 4096;  /* 1 page */
if (alloc_gpu_buffer(ctx, ctx->binding_table_size,
                     &ctx->binding_table_bo,
                     &ctx->binding_table_map) < 0) {
    LOG_EVENT(ctx, "BINDING_TABLE_ALLOC_FAILED");
    goto error;
}
LOG_EVENT(ctx, "BINDING_TABLE_CREATED: size=%zu handle=%u",
         ctx->binding_table_size, ctx->binding_table_bo);

/* C279 CORRECTION #3: Allouer interface descriptor (32 bytes) */
ctx->interface_desc_size = 4096;  /* 1 page */
if (alloc_gpu_buffer(ctx, ctx->interface_desc_size,
                     &ctx->interface_desc_bo,
                     &ctx->interface_desc_map) < 0) {
    LOG_EVENT(ctx, "INTERFACE_DESC_ALLOC_FAILED");
    goto error;
}
LOG_EVENT(ctx, "INTERFACE_DESC_CREATED: size=%zu handle=%u",
         ctx->interface_desc_size, ctx->interface_desc_bo);
```

---

## 📋 ÉTAPE 4 : CONFIGURER SURFACE STATES

### Fichier : `src/btc_gen9_native_runner.c` (nouvelle fonction)

**Ajouter avant `btc_gen9_execute()`** :

```c
/**
 * C279: Configure surface states pour input/output buffers
 */
static int configure_surface_states(btc_gen9_context_t* ctx) {
    struct gen9_surface_state* surfaces = 
        (struct gen9_surface_state*)ctx->surface_state_map;
    
    /* Surface 0: Input buffer */
    memset(&surfaces[0], 0, sizeof(struct gen9_surface_state));
    surfaces[0].dw0 = (GEN9_SURFACE_BUFFER << 29) |  /* Surface Type */
                      (GEN9_SURFACEFORMAT_RAW << 18); /* Format */
    surfaces[0].dw1 = ctx->input_bo;  /* Base Address Low */
    surfaces[0].dw2 = ctx->input_size - 1;  /* Width (size-1) */
    surfaces[0].dw3 = 0;  /* Pitch = 0 for buffer */
    
    /* Surface 1: Output buffer */
    memset(&surfaces[1], 0, sizeof(struct gen9_surface_state));
    surfaces[1].dw0 = (GEN9_SURFACE_BUFFER << 29) |
                      (GEN9_SURFACEFORMAT_RAW << 18);
    surfaces[1].dw1 = ctx->output_bo;  /* Base Address Low */
    surfaces[1].dw2 = ctx->output_size - 1;  /* Width (size-1) */
    surfaces[1].dw3 = 0;
    
    LOG_EVENT(ctx, "SURFACE_STATES_CONFIGURED: input_bo=%u output_bo=%u",
             ctx->input_bo, ctx->output_bo);
    
    return 0;
}
```

---

## 📋 ÉTAPE 5 : CONFIGURER BINDING TABLE

### Fichier : `src/btc_gen9_native_runner.c` (nouvelle fonction)

```c
/**
 * C279: Configure binding table (pointeurs vers surface states)
 */
static int configure_binding_table(btc_gen9_context_t* ctx) {
    uint32_t* binding_table = (uint32_t*)ctx->binding_table_map;
    
    /* Entry 0: Offset vers surface state 0 (input) */
    binding_table[0] = 0;  /* Offset 0 dans surface_state_bo */
    
    /* Entry 1: Offset vers surface state 1 (output) */
    binding_table[1] = 64;  /* Offset 64 bytes (1 surface state) */
    
    LOG_EVENT(ctx, "BINDING_TABLE_CONFIGURED: entries=2");
    
    return 0;
}
```

---

## 📋 ÉTAPE 6 : CONFIGURER INTERFACE DESCRIPTOR

### Fichier : `src/btc_gen9_native_runner.c` (nouvelle fonction)

```c
/**
 * C279: Configure interface descriptor (kernel + binding table)
 */
static int configure_interface_descriptor(btc_gen9_context_t* ctx) {
    struct gen9_interface_descriptor* desc = 
        (struct gen9_interface_descriptor*)ctx->interface_desc_map;
    
    memset(desc, 0, sizeof(struct gen9_interface_descriptor));
    
    /* DW0-1: Kernel Start Pointer */
    desc->dw0 = ctx->kernel_bo;  /* Kernel base address */
    desc->dw1 = 0;  /* High 32 bits (0 pour <4GB) */
    
    /* DW3: Sampler State Pointer (0 si pas de sampler) */
    desc->dw3 = 0;
    
    /* DW4: Binding Table Pointer */
    desc->dw4 = ctx->binding_table_bo;
    
    /* DW5: Constant URB Entry Read Length */
    desc->dw5 = 0;  /* Pas de constants pour SHA256 */
    
    /* DW6: Number of Threads */
    desc->dw6 = 256;  /* 256 threads par work-group */
    
    /* DW7: Shared Local Memory Size */
    desc->dw7 = 0;  /* Pas de SLM pour SHA256 */
    
    LOG_EVENT(ctx, "INTERFACE_DESC_CONFIGURED: kernel_bo=%u binding_table_bo=%u",
             ctx->kernel_bo, ctx->binding_table_bo);
    
    return 0;
}
```

---

## 📋 ÉTAPE 7 : APPELER CONFIGURATION (Avant Premier Dispatch)

### Fichier : `src/btc_gen9_native_runner.c` (dans `btc_gen9_init`)

**Ajouter après création async_save_thread** :

```c
/* C279 CORRECTION #4: Configurer structures GPU */
if (configure_surface_states(ctx) < 0) {
    LOG_EVENT(ctx, "SURFACE_STATES_CONFIG_FAILED");
    goto error;
}

if (configure_binding_table(ctx) < 0) {
    LOG_EVENT(ctx, "BINDING_TABLE_CONFIG_FAILED");
    goto error;
}

if (configure_interface_descriptor(ctx) < 0) {
    LOG_EVENT(ctx, "INTERFACE_DESC_CONFIG_FAILED");
    goto error;
}

LOG_EVENT(ctx, "GPU_STRUCTURES_CONFIGURED: ready_for_dispatch");
```

---

## 📋 ÉTAPE 8 : CORRIGER BATCH BUFFER (CRITIQUE)

### Fichier : `src/btc_gen9_native_runner.c` (lignes 1202-1237)

**REMPLACER** :

```c
/* STATE_BASE_ADDRESS - AVANT (invalide) */
batch[offset++] = GEN9_STATE_BASE_ADDRESS | (19 - 2);
for (int i = 0; i < 18; i++) {
    batch[offset++] = 0;  // ❌ INVALIDE
}
```

**PAR** :

```c
/* STATE_BASE_ADDRESS - APRÈS (valide) */
batch[offset++] = GEN9_STATE_BASE_ADDRESS | (19 - 2);
batch[offset++] = 0;  /* General State Base Address (optionnel) */
batch[offset++] = 0;  /* Modify Enable */
batch[offset++] = ctx->surface_state_bo;  /* ✅ Surface State Base Address */
batch[offset++] = 0;  /* Modify Enable */
batch[offset++] = ctx->interface_desc_bo;  /* ✅ Dynamic State Base Address */
batch[offset++] = 0;  /* Modify Enable */
batch[offset++] = 0;  /* Indirect Object Base Address */
batch[offset++] = 0;  /* Modify Enable */
batch[offset++] = ctx->kernel_bo;  /* ✅ Instruction Base Address */
batch[offset++] = 0;  /* Modify Enable */
batch[offset++] = 0xFFFFF000;  /* General State Buffer Size */
batch[offset++] = 0xFFFFF000;  /* Dynamic State Buffer Size */
batch[offset++] = 0;  /* Indirect Object Buffer Size */
batch[offset++] = 0xFFFFF000;  /* Instruction Buffer Size */
batch[offset++] = 0;  /* General State Base Address Upper */
batch[offset++] = 0;  /* Dynamic State Base Address Upper */
batch[offset++] = 0;  /* Indirect Object Base Address Upper */
batch[offset++] = 0;  /* Instruction Base Address Upper */
```

**REMPLACER** :

```c
/* INTERFACE_DESCRIPTOR_DATA - AVANT (invalide) */
batch[offset++] = 0;  // ❌ Kernel Start Pointer = NULL
batch[offset++] = 0;
// ... (tous à 0)
```

**PAR** :

```c
/* INTERFACE_DESCRIPTOR_DATA - APRÈS (valide) */
batch[offset++] = 0;  /* ✅ Offset dans interface_desc_bo */
batch[offset++] = 0;
batch[offset++] = 0;
batch[offset++] = 0;
batch[offset++] = 0;
batch[offset++] = 0;
batch[offset++] = 0x00000200;  /* ✅ SIMD16 configuration */
batch[offset++] = 0;
batch[offset++] = 0;  /* ✅ Binding table offset (dans binding_table_bo) */
batch[offset++] = 0;
batch[offset++] = 0;
batch[offset++] = 0;
```

---

## 📋 ÉTAPE 9 : AJOUTER BUFFERS DANS EXECBUFFER2

### Fichier : `src/btc_gen9_native_runner.c` (ligne 1271)

**REMPLACER** :

```c
struct drm_i915_gem_exec_object2 exec_objects[4] = {
    { .handle = ctx->kernel_bo },
    { .handle = ctx->input_bo },
    { .handle = ctx->output_bo },
    { .handle = current_batch_bo }
};
```

**PAR** :

```c
/* C279: Ajouter nouveaux buffers GPU */
struct drm_i915_gem_exec_object2 exec_objects[7] = {
    { .handle = ctx->kernel_bo },
    { .handle = ctx->input_bo },
    { .handle = ctx->output_bo },
    { .handle = ctx->surface_state_bo },      /* ✅ Surface states */
    { .handle = ctx->binding_table_bo },      /* ✅ Binding table */
    { .handle = ctx->interface_desc_bo },     /* ✅ Interface descriptor */
    { .handle = current_batch_bo }
};
```

**ET** :

```c
struct drm_i915_gem_execbuffer2 execbuf = {
    .buffers_ptr = (uintptr_t)exec_objects,
    .buffer_count = 7,  /* ✅ 7 buffers au lieu de 4 */
    .batch_start_offset = 0,
    .batch_len = ctx->batch_size,
    .flags = I915_EXEC_RENDER,
    .rsvd1 = pool_ctx_id
};
```

---

## 📋 ÉTAPE 10 : CLEANUP (Libérer Nouveaux Buffers)

### Fichier : `src/btc_gen9_native_runner.c` (dans cleanup)

**Ajouter dans section cleanup** :

```c
/* C279: Libérer nouveaux buffers */
if (ctx->surface_state_bo) {
    struct drm_gem_close close_arg = { .handle = ctx->surface_state_bo };
    ioctl(ctx->drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
}
if (ctx->binding_table_bo) {
    struct drm_gem_close close_arg = { .handle = ctx->binding_table_bo };
    ioctl(ctx->drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
}
if (ctx->interface_desc_bo) {
    struct drm_gem_close close_arg = { .handle = ctx->interface_desc_bo };
    ioctl(ctx->drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg);
}
```

---

## 📋 ÉTAPE 11 : COMPILATION ET TEST

### Commandes :

```bash
# Compiler
cd src/advanced_calculations/bitcoin_quantum_mining/level_zero_native
make -f Makefile.c278 clean
make -f Makefile.c278

# Tester 100 dispatches
./bin/test_btc_mining_gen9 100

# Vérifier logs
tail -f logs/gen9_native_production.log

# Vérifier GPU hangs
sudo dmesg | grep -i "i915\|GPU hang" | tail -20
```

### Critères de Succès :

```
✅ Compilation sans erreurs
✅ 100 dispatches terminés
✅ 0 GPU hang dans dmesg
✅ Temps exécution < 640ms par dispatch
✅ Hashrate > 0 (calculs SHA256 réels)
✅ Pas d'errno=5
```

---

## ⚠️ PROBLÈMES POTENTIELS ET SOLUTIONS

### Problème 1 : Compilation échoue

**Cause** : Structures Gen9 mal définies  
**Solution** : Vérifier `btc_gen9_structures.h` avec documentation Intel

### Problème 2 : GPU hang persiste

**Cause** : Pointeurs incorrects dans batch buffer  
**Solution** : Vérifier que tous les `_bo` sont des handles GEM valides

### Problème 3 : errno=22 (EINVAL)

**Cause** : Batch buffer toujours invalide  
**Solution** : Vérifier STATE_BASE_ADDRESS avec `intel_gpu_top`

### Problème 4 : Segfault

**Cause** : Buffers non alloués correctement  
**Solution** : Vérifier `alloc_gpu_buffer()` retourne 0

---

## 📊 VALIDATION FINALE

### Test de Validation :

```bash
# Test 1: 10 dispatches (baseline)
./bin/test_btc_mining_gen9 10

# Test 2: 50 dispatches (intermédiaire)
./bin/test_btc_mining_gen9 50

# Test 3: 100 dispatches (objectif)
./bin/test_btc_mining_gen9 100

# Test 4: 1000 dispatches (stress test)
./bin/test_btc_mining_gen9 1000
```

### Métriques Attendues :

```
Dispatches réussis : 100/100 (100%)
GPU hangs          : 0
Temps moyen        : ~700ms/dispatch
Hashrate           : ~1.5 GH/s
Contextes bannis   : 0
```

---

## 🎯 CONCLUSION

Cette implémentation est **complexe** mais **nécessaire** pour atteindre 100% indépendance OpenCL.

**Alternatives** :
1. **Utiliser OpenCL** (recommandé pour production)
2. **Utiliser Level Zero** (si supporté sur Gen9)
3. **Implémenter cette correction** (pour recherche/apprentissage)

**Temps estimé** : 4-6 heures de développement + 2-4 heures de débogage

---

**FIN DU PLAN D'IMPLÉMENTATION C279**