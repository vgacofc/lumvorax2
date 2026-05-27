# RAPPORT C457 - REVERSE ENGINEERING: Pourquoi EXECBUFFER2 échoue

**Date**: 2026-05-23  
**Cycle**: C457  
**Objectif**: Identifier pourquoi test_c433 RÉUSSIT EXECBUFFER2 mais c455/c456 ÉCHOUENT

## 1. ÉTAT DES LIEUX

### Tests qui RÉUSSISSENT EXECBUFFER2
- ✅ **test_c433**: EXECBUFFER2 réussit, GPU produit 0x00000000
- ✅ **test_c452_opencl**: EXECBUFFER2 réussit, GPU produit 0xAABBCCDD

### Tests qui ÉCHOUENT EXECBUFFER2  
- ❌ **test_c455**: CREATE_EXT → "Invalid argument"
- ❌ **test_c456**: Batch minimal → "Invalid argument"

## 2. DÉCOUVERTES CRITIQUES

### C456: Même MI_BATCH_BUFFER_END seul échoue!
```
Batch: [MI_BATCH_BUFFER_END]
EXECBUFFER2: ❌ Invalid argument (errno=22)
```

**Conclusion**: Le problème n'est PAS:
- ❌ Le contenu du batch buffer
- ❌ Les commandes GPU
- ❌ Les relocations
- ❌ CREATE vs CREATE_EXT

**Le problème EST dans la configuration de `drm_i915_gem_execbuffer2` ou `drm_i915_gem_exec_object2`!**

## 3. COMPARAISON test_c433 vs test_c456

### test_c433 (FONCTIONNE)
```c
struct drm_i915_gem_exec_object2 exec_objects[6];
memset(exec_objects, 0, sizeof(exec_objects));

exec_objects[0].handle = ctx->kernel_bo;
exec_objects[1].handle = ctx->output_bo;
exec_objects[2].handle = ctx->ssh_bo;
exec_objects[2].relocation_count = 1;
exec_objects[2].relocs_ptr = (uint64_t)&relocs[5];
exec_objects[3].handle = ctx->dsh_bo;
exec_objects[3].relocation_count = 1;
exec_objects[3].relocs_ptr = (uint64_t)&relocs[4];
exec_objects[4].handle = ctx->ioh_bo;
exec_objects[5].handle = ctx->batch_bo;
exec_objects[5].relocation_count = 4;
exec_objects[5].relocs_ptr = (uint64_t)&relocs[0];

struct drm_i915_gem_execbuffer2 execbuf = {
    .buffers_ptr = (uint64_t)exec_objects,
    .buffer_count = 6,
    .batch_len = offset * 4,
    .flags = I915_EXEC_RENDER,
    .rsvd1 = ctx->ctx_id
};
```

### test_c456 (ÉCHOUE)
```c
struct drm_i915_gem_exec_object2 exec_obj;
memset(&exec_obj, 0, sizeof(exec_obj));
exec_obj.handle = batch_handle;

struct drm_i915_gem_execbuffer2 execbuf;
memset(&execbuf, 0, sizeof(execbuf));
execbuf.buffers_ptr = (uint64_t)&exec_obj;
execbuf.buffer_count = 1;
execbuf.batch_len = 20;
execbuf.flags = I915_EXEC_RENDER;
execbuf.rsvd1 = ctx_id;
```

### DIFFÉRENCE CLÉS

1. **Nombre de buffers**: c433 utilise 6 buffers, c456 utilise 1 seul
2. **Batch buffer position**: c433 met batch en DERNIER (index 5), c456 en PREMIER (index 0)

## 4. HYPOTHÈSE ROOT CAUSE

**Le batch buffer DOIT être le DERNIER buffer dans exec_objects!**

C'est une règle i915 DRM: le dernier buffer de la liste est considéré comme le batch buffer.

## 5. PLAN D'ACTION C457

### Test 1: Batch buffer en dernier
```c
struct drm_i915_gem_exec_object2 exec_objects[2];
memset(exec_objects, 0, sizeof(exec_objects));

exec_objects[0].handle = dummy_bo;  // Buffer dummy
exec_objects[1].handle = batch_handle;  // Batch EN DERNIER

execbuf.buffer_count = 2;
```

### Test 2: Si échec, copier EXACTEMENT c433
Copier ligne par ligne la configuration de c433 qui fonctionne.

## 6. PROCHAINES ÉTAPES

1. ⏳ Créer test_c457 avec batch en dernier
2. ⏳ Si réussite → Appliquer à test_c455
3. ⏳ Résoudre problème 0x00000000 (GPU n'écrit pas)
4. ⏳ Propager corrections dans btc_gen9_native_runner.c

## 7. ÉTAT D'AVANCEMENT

**Infrastructure i915**: 100% ✅  
**EXECBUFFER2**: 50% ⏳ (fonctionne dans c433, échoue dans c455/c456)  
**GPU Write**: 0% ❌ (produit toujours 0x00000000)  
**Projet Global**: 95% ⏳

---

**Prochaine action**: Créer test_c457 avec batch buffer en DERNIER position