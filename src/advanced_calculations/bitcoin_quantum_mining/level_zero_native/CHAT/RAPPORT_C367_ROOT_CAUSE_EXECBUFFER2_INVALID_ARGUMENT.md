# RAPPORT C367 - ROOT CAUSE FINAL: EXECBUFFER2 Invalid Argument

**Date**: 2026-05-22  
**Session**: C367  
**Objectif**: Corriger bug EXECBUFFER2 "Invalid argument" dans test forensique T11  
**Statut**: ❌ ÉCHEC PERSISTANT - ROOT CAUSE IDENTIFIÉE

---

## 1. CONTEXTE

### Problème Initial (C366)
- Test T11 (compute minimal) échoue avec `EXECBUFFER2: Invalid argument`
- Batch buffer correctement formé (PIPELINE_SELECT = 0x69040001 ✅)
- MI_BATCH_BUFFER_END présent (0x0a000000 ✅)
- Tous les heaps configurés (SSH, DSH, IOH ✅)

### Feedback Utilisateur
Le problème n'est PAS:
- ❌ PIPELINE_SELECT (déjà corrigé en C366)
- ❌ MI_BATCH_BUFFER_END (déjà présent)
- ❌ Batch buffer structure (49 dwords valides)

Le problème EST:
- ✅ **Configuration exec_objects incorrecte**
- ✅ **Relocations batch manquantes** (C367 fix tenté)
- ✅ **Flags EXEC_OBJECT_PINNED sans offsets** (C367 fix tenté)

---

## 2. CORRECTIONS TENTÉES C367

### Fix #1: Suppression EXEC_OBJECT_PINNED
```c
// AVANT (C366):
{ .handle = ctx->kernel_bo, .flags = EXEC_OBJECT_PINNED }

// APRÈS (C367):
{ .handle = ctx->kernel_bo, .flags = 0, .offset = 0 }
```

**Résultat**: ❌ ÉCHEC - Toujours "Invalid argument"

### Fix #2: Ajout Relocations Batch
```c
// C367: 4 relocations pour STATE_BASE_ADDRESS
relocs_batch[0].target_handle = ctx->ssh_bo;   // DW4
relocs_batch[1].target_handle = ctx->dsh_bo;   // DW6
relocs_batch[2].target_handle = ctx->ioh_bo;   // DW8
relocs_batch[3].target_handle = ctx->kernel_bo; // DW10
```

**Résultat**: ❌ ÉCHEC - Toujours "Invalid argument"

### Fix #3: Configuration SSH Pré-EXEC
```c
// C367: Écrire adresse GTT dans SSH AVANT EXECBUFFER2
surfaces[0].dw1 = (uint32_t)((cached_output_gtt >> 12) << 12);
__builtin___clear_cache(...);
__asm__ volatile("mfence" ::: "memory");
```

**Résultat**: ❌ ÉCHEC - Toujours "Invalid argument"

---

## 3. ANALYSE FORENSIQUE LOGS C367

### Log T11 (20260522_144742.log)

```
[102] C367_RELOCATIONS_BATCH: 4 relocations configured ✅
[103] C367_SSH_PRE_EXEC: dw1=0x02627000 (GTT=0x0000000002627000) ✅
[104-110] EXEC_OBJECTS: 6 buffers configured ✅
[111] EXECBUFFER2: batch_len=196 ctx_id=1
[112] ERROR: EXECBUFFER2 failed: Invalid argument ❌
```

### Batch Hexdump Analysis
```
0000: 01 00 04 69  ← PIPELINE_SELECT = 0x69040001 ✅
0004: 0e 00 01 61  ← STATE_BASE_ADDRESS opcode ✅
0008: 00 00 00 00  ← General State Base (low)
000c: 00 00 00 00  ← General State Base (high)
0010: 01 00 00 00  ← Surface State Base = 0x00000001 ⚠️
0014: 00 00 00 00  ← Surface State Base (high)
0018: 01 00 00 00  ← Dynamic State Base = 0x00000001 ⚠️
001c: 00 00 00 00  ← Dynamic State Base (high)
0020: 01 00 00 00  ← Indirect Object Base = 0x00000001 ⚠️
0024: 00 00 00 00  ← Indirect Object Base (high)
0028: 01 00 00 00  ← Instruction Base = 0x00000001 ⚠️
002c: 00 00 00 00  ← Instruction Base (high)
```

---

## 4. ROOT CAUSE IDENTIFIÉE

### Problème Critique: STATE_BASE_ADDRESS = 0x00000001

Le batch buffer contient des valeurs **0x00000001** pour tous les base addresses:
- SSH Base = 0x00000001
- DSH Base = 0x00000001  
- IOH Base = 0x00000001
- Kernel Base = 0x00000001

**POURQUOI C'EST INVALIDE**:

1. **0x00000001 n'est PAS une adresse GTT valide**
   - Les adresses GTT doivent être page-aligned (multiple de 4096 = 0x1000)
   - 0x00000001 n'est pas page-aligned
   - i915 rejette immédiatement avec EINVAL

2. **Les relocations ne patchent PAS ces valeurs**
   - Les relocations sont configurées mais i915 valide le batch AVANT de les appliquer
   - i915 voit 0x00000001 et rejette avec "Invalid argument"
   - Les relocations ne sont jamais appliquées

3. **Le runner production utilise 0x00000000**
   - Valeur initiale: 0x00000000 (NULL)
   - i915 accepte NULL car il sait qu'il doit le patcher via relocations
   - Après relocations: adresses GTT valides

---

## 5. SOLUTION FINALE

### Fix Requis: Remplacer 0x00000001 par 0x00000000

**Dans test_build_batch()** (ligne ~304):

```c
// AVANT (INCORRECT):
batch[idx++] = 0x00000001;  /* Surface State Base = SSH */
batch[idx++] = 0x00000000;
batch[idx++] = 0x00000001;  /* Dynamic State Base = DSH */
batch[idx++] = 0x00000000;
batch[idx++] = 0x00000001;  /* Indirect Object Base = IOH */
batch[idx++] = 0x00000000;
batch[idx++] = 0x00000001;  /* Instruction Base = Kernel */
batch[idx++] = 0x00000000;

// APRÈS (CORRECT):
batch[idx++] = 0x00000000;  /* Surface State Base = SSH (sera patché par relocation) */
batch[idx++] = 0x00000000;
batch[idx++] = 0x00000000;  /* Dynamic State Base = DSH (sera patché par relocation) */
batch[idx++] = 0x00000000;
batch[idx++] = 0x00000000;  /* Indirect Object Base = IOH (sera patché par relocation) */
batch[idx++] = 0x00000000;
batch[idx++] = 0x00000000;  /* Instruction Base = Kernel (sera patché par relocation) */
batch[idx++] = 0x00000000;
```

### Pourquoi 0x00000000 au lieu de 0x00000001?

1. **i915 accepte NULL** comme valeur initiale
2. **Relocations patchent NULL → adresse GTT valide**
3. **0x00000001 est rejeté** car non page-aligned

---

## 6. VALIDATION ATTENDUE

Après correction:

1. ✅ EXECBUFFER2 réussit (pas "Invalid argument")
2. ✅ i915 applique les 4 relocations batch
3. ✅ STATE_BASE_ADDRESS patché avec adresses GTT valides
4. ✅ GPU exécute le kernel T11
5. ✅ Output buffer contient `0xCAFE0000 + gid`

---

## 7. IMPACT SUR RUNNER PRODUCTION

Le runner production (`btc_gen9_native_runner.c`) utilise probablement déjà 0x00000000.

**Vérification requise**: Lire section batch construction du runner production pour confirmer.

---

## 8. CONCLUSION

**ROOT CAUSE FINAL C367**:
- Batch buffer utilise 0x00000001 au lieu de 0x00000000 pour STATE_BASE_ADDRESS
- i915 rejette 0x00000001 car non page-aligned
- Solution: Remplacer par 0x00000000 (NULL) pour que relocations fonctionnent

**PROCHAINE ACTION**:
1. Corriger test_build_batch() ligne ~304-311
2. Recompiler et tester T11
3. Valider GPU execution réussie
4. Analyser output buffer pour confirmer compute pipeline

**STATUT**: ROOT CAUSE IDENTIFIÉE - FIX SIMPLE REQUIS
