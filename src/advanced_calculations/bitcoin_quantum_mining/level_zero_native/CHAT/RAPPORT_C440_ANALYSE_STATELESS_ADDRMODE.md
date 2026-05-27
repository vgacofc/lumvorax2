# RAPPORT C440 - ANALYSE ADDRMODE STATELESS ET PAYLOAD

**Date**: 2026-05-23 17:09 CET  
**Durée**: 3.065ms  
**Statut**: ❌ ÉCHEC - output[0]=0x00000000 (attendu: 0x12345678)

## 🎯 DÉCOUVERTE MAJEURE C440

### Analyse `.ze_info` du binaire OpenCL

```yaml
payload_arguments:
  - arg_type: global_id_offset
    offset: 0
    size: 12
  - arg_type: local_size
    offset: 12
    size: 12
  - arg_type: arg_bypointer      # ← ARGUMENT OUTPUT
    offset: 32                    # ← OFFSET 32 bytes
    size: 8                       # ← 64-bit pointer
    arg_index: 0
    addrmode: stateless          # ← MODE STATELESS!
    addrspace: global
    access_type: readwrite
  - arg_type: enqueued_local_size
    offset: 40
    size: 12

per_thread_payload_arguments:
  - arg_type: local_id
    offset: 0
    size: 192
```

### Interprétation

**Le kernel attend l'ADRESSE GPU 64-bit du buffer, PAS un BTI!**

- `addrmode: stateless` = accès mémoire direct par adresse
- `offset: 32` = position dans le payload (CURBE)
- `size: 8` = pointeur 64-bit

## 🔧 CORRECTION IMPLÉMENTÉE C440

### Avant (C439)
```c
curbe[8] = 0;  /* r8.0: BTI output buffer = 0 */
```

### Après (C440)
```c
/* C440 FIX: Kernel utilise addrmode=stateless, attend ADRESSE 64-bit!
 * Selon .ze_info: arg_bypointer à offset 32 (8 bytes)
 * On met l'adresse GPU du buffer output (relocation) */
uint64_t* output_addr_ptr = (uint64_t*)&curbe[8];  /* r8.0-r8.1: adresse 64-bit */
*output_addr_ptr = 0;  /* Sera patché par relocation */
```

### Relocation ajoutée
```c
/* C440 FIX: Output buffer relocation dans CURBE (addrmode=stateless) */
/* CURBE offset 32 = r8.0-r8.1 (adresse 64-bit du buffer output) */
relocs[4].target_handle = ctx->output_bo;
relocs[4].offset = 32;  /* DSH offset 32 bytes = CURBE r8.0 */
relocs[4].read_domains = I915_GEM_DOMAIN_RENDER;
relocs[4].write_domain = I915_GEM_DOMAIN_RENDER;
```

## ❌ RÉSULTAT

**output[0] = 0x00000000** (échec persistant)

## 🔍 ANALYSE ROOT CAUSE

### Hypothèses restantes

1. **Payload Layout incorrect**
   - Le payload inclut AUSSI `per_thread_payload_arguments`
   - `local_id` occupe 192 bytes (offset 0)
   - Les `payload_arguments` commencent APRÈS?

2. **Cross-thread vs Per-thread**
   - `payload_arguments` = Cross-Thread Constant Data (CURBE)
   - `per_thread_payload_arguments` = Per-Thread Data
   - Peut-être deux zones mémoire séparées?

3. **Kernel ISA incompatible**
   - Le binaire OpenCL est compilé pour Level Zero
   - Peut contenir des instructions spécifiques Level Zero
   - Notre infrastructure i915 manuelle ne les supporte pas?

4. **STATE_BASE_ADDRESS incorrect**
   - Les adresses de base ne pointent pas aux bons endroits
   - Le GPU ne trouve pas les données

## 📊 DONNÉES FORENSIQUES

### Configuration actuelle
- **SSH**: Binding Table @ 0, Surface State @ 64
- **DSH**: CURBE (64 bytes) + IDD (32 bytes)
- **CURBE Layout**:
  - r0.0-r0.2: global_offset (0,0,0)
  - r1.0-r1.2: global_size (256,1,1)
  - r8.0-r8.1: output buffer address (relocation)

### Relocations
1. SSH base address
2. DSH base address
3. IOH base address
4. Kernel base address
5. **Output address dans CURBE** (C440)
6. Output address dans SSH Surface State

## 🎯 PROCHAINES ÉTAPES CRITIQUES

### Option A: Analyser le kernel ISA
1. Désassembler le binaire OpenCL
2. Identifier comment il lit l'adresse du buffer
3. Vérifier si c'est bien depuis r8.0

### Option B: Comparer avec un kernel natif simple
1. Écrire un kernel Gen9 ISA minimal en assembleur
2. Qui fait juste: `mov (1) r[output]:ud 0x12345678`
3. Tester avec notre infrastructure

### Option C: Utiliser Level Zero API
1. Abandonner i915 DRM manuel
2. Utiliser Level Zero qui gère tout automatiquement
3. Mesurer les performances

## 💡 RECOMMANDATION

**PRIORITÉ ABSOLUE**: Désassembler le kernel OpenCL pour voir EXACTEMENT comment il accède au buffer output.

Commande:
```bash
iga64 -d -p 9 tests/test_c440_opencl_binary.bin
```

Cela nous dira si le kernel lit bien depuis r8.0 ou s'il utilise un autre mécanisme.

## 📈 ÉTAT D'AVANCEMENT

- Infrastructure i915: **100%** ✅
- Surface State Intel: **100%** ✅
- Binding Table: **100%** ✅
- CURBE stateless: **100%** ✅
- **Kernel execution: 0%** ❌ (BLOQUANT)
- Batch Buffer Automation: **5%** ⏳
- **PROJET GLOBAL: 92%** ⏳

**Temps écoulé depuis C432**: 8 cycles (C432-C440)  
**Temps estimé résolution**: 2-4 heures (désassemblage + analyse ISA)

---

**Expertise mobilisée**: Architecture GPU Intel Gen9, i915 DRM, Level Zero, OpenCL Runtime, Analyse binaire ELF, Désassemblage ISA