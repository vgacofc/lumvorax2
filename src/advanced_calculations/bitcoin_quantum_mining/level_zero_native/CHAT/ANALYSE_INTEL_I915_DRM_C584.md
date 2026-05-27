# ANALYSE INTEL i915 DRM - C584

**Date**: 2026-05-26 17:39 CET  
**Source**: database/linux/include/uapi/drm/i915_drm.h  
**Objectif**: Spécifications officielles relocations et EXEC_OBJECT_PINNED

---

## 📋 STRUCTURE RELOCATION OFFICIELLE

### drm_i915_gem_relocation_entry (Lignes 1118-1161)

```c
struct drm_i915_gem_relocation_entry {
    /**
     * Handle of the buffer being pointed to by this relocation entry.
     * It's appealing to make this be an index into the mm_validate_entry
     * list to refer to the buffer, but this allows the driver to create
     * a relocation list for state buffers and not re-write it per
     * exec using the buffer.
     */
    __u32 target_handle;

    /**
     * Value to be added to the offset of the target buffer to make up
     * the relocation entry.
     */
    __u32 delta;

    /** Offset in the buffer the relocation entry will be written into */
    __u64 offset;

    /**
     * Offset value of the target buffer that the relocation entry was last
     * written as.
     * If the buffer has the same offset as last time, we can skip syncing
     * and writing the relocation. This value is written back out by
     * the execbuffer ioctl when the relocation is written.
     */
    __u64 presumed_offset;

    /**
     * Target memory domains read by this operation.
     */
    __u32 read_domains;

    /**
     * Target memory domains written by this operation.
     * Note that only one domain may be written by the whole
     * execbuffer operation, so that where there are conflicts,
     * the application will get -EINVAL back.
     */
    __u32 write_domain;
};
```

### Découvertes Critiques

1. **target_handle**: Handle du buffer cible (heap dans notre cas)
2. **delta**: Offset DANS le buffer cible (0 pour heap base)
3. **offset**: Offset DANS le batch où écrire l'adresse patchée
4. **presumed_offset**: Optimisation (skip si GTT inchangée)
5. **read_domains / write_domain**: Memory domains pour cache coherency

---

## 🎯 MEMORY DOMAINS OFFICIELS (Lignes 1163-1180)

### Définitions

```c
/** CPU cache */
#define I915_GEM_DOMAIN_CPU         0x00000001

/** Render cache, used by 2D and 3D drawing */
#define I915_GEM_DOMAIN_RENDER      0x00000002

/** Sampler cache, used by texture engine */
#define I915_GEM_DOMAIN_SAMPLER     0x00000004

/** Command queue, used to load batch buffers */
#define I915_GEM_DOMAIN_COMMAND     0x00000008

/** Instruction cache, used by shader programs */
#define I915_GEM_DOMAIN_INSTRUCTION 0x00000010

/** Vertex address cache */
#define I915_GEM_DOMAIN_VERTEX      0x00000020
```

### Usage Pour Compute Workloads Gen9

**Pour STATE_BASE_ADDRESS (heap base)**:
```c
.read_domains = I915_GEM_DOMAIN_RENDER,
.write_domain = I915_GEM_DOMAIN_RENDER
```

**Justification**:
- Render cache utilisé pour compute workloads Gen9
- Heap contient kernel ISA (INSTRUCTION) + structures GPU (RENDER)
- RENDER domain couvre les deux cas

---

## 🔒 EXEC_OBJECT_PINNED (Lignes 1257-1278)

### Définition Officielle

```c
#define EXEC_OBJECT_PINNED (1<<4)
```

### Documentation (Lignes 1257-1270)

```
/**
 * When the EXEC_OBJECT_PINNED flag is specified this is populated by
 * the user with the GTT offset at which this object will be pinned.
 *
 * When the I915_EXEC_NO_RELOC flag is specified this must contain the
 * presumed_offset of the object.
 *
 * During execbuffer2 the kernel populates it with the value of the
 * current GTT offset of the object, for future presumed_offset writes.
 *
 * See struct drm_i915_gem_create_ext for the rules when dealing with
 * alignment restrictions with I915_MEMORY_CLASS_DEVICE, on devices with
 * minimum page sizes, like DG2.
 */
__u64 offset;
```

### Autres Flags Pertinents

```c
#define EXEC_OBJECT_NEEDS_FENCE          (1<<0)
#define EXEC_OBJECT_NEEDS_GTT            (1<<1)
#define EXEC_OBJECT_WRITE                (1<<2)  // Buffer modifié par GPU
#define EXEC_OBJECT_SUPPORTS_48B_ADDRESS (1<<3)
#define EXEC_OBJECT_PAD_TO_SIZE          (1<<5)
```

### Usage Recommandé C584

**Option 1: SANS EXEC_OBJECT_PINNED (Relocations)**:
```c
struct drm_i915_gem_exec_object2 heap_obj = {
    .handle = heap_handle,
    .relocation_count = 1,
    .relocs_ptr = (uint64_t)&reloc,
    .flags = EXEC_OBJECT_WRITE,  // Heap modifié par GPU
    .offset = 0  // Kernel alloue GTT
};
```

**Option 2: AVEC EXEC_OBJECT_PINNED (Soft Pinning)**:
```c
struct drm_i915_gem_exec_object2 heap_obj = {
    .handle = heap_handle,
    .relocation_count = 0,  // Pas de relocations
    .relocs_ptr = 0,
    .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_WRITE,
    .offset = 0x0000000100000000  // GTT souhaitée (4GB)
};
```

---

## 🔧 ALIGNMENT REQUIREMENTS

### Documentation Officielle (Ligne 1254-1255)

```c
/** Required alignment in graphics aperture */
__u64 alignment;
```

### Alignments Gen9 (Inférés depuis architecture)

- **STATE_BASE_ADDRESS**: 4KB (0x1000) aligned
- **Batch buffer**: 4-byte aligned minimum
- **Kernel ISA**: 64-byte aligned
- **Surface State**: 64-byte aligned
- **Binding Table**: 32-byte aligned
- **IDRT**: 64-byte aligned

---

## 💡 DÉCISION ARCHITECTURE C584

### Analyse Comparative

#### Option 1: Relocations (RECOMMANDÉE)
**Avantages**:
- ✅ Pas de dépendance soft pinning
- ✅ GTT address garantie par kernel
- ✅ Modèle Gen9 correct (1 relocation + offsets relatifs)
- ✅ Feedback expert validé

**Inconvénients**:
- ⚠️ Nécessite validation relocation constraints
- ⚠️ Alignment requirements à respecter

**Implémentation**:
```c
// 1 SEULE relocation: STATE_BASE_ADDRESS DWord 1-2 (heap base)
struct drm_i915_gem_relocation_entry reloc = {
    .target_handle = heap_handle,
    .delta = 0,  // Heap base (pas d'offset)
    .offset = (sba_offset + 1) * 4,  // DWord 1 STATE_BASE_ADDRESS
    .read_domains = I915_GEM_DOMAIN_RENDER,
    .write_domain = I915_GEM_DOMAIN_RENDER,
    .presumed_offset = 0  // Kernel remplit
};

// Tous les pointeurs internes: offsets relatifs depuis heap base
idrt[0] = OFFSET_KERNEL_ISA;  // Relatif heap base
idrt[3] = OFFSET_BINDING_TABLE;  // Relatif heap base
binding_table[0] = OFFSET_SURFACE_STATE;  // Relatif heap base
surface_state[0] = OFFSET_OUTPUT;  // Relatif heap base
```

#### Option 2: Soft Pinning
**Avantages**:
- ✅ GTT address connue à l'avance
- ✅ Batch construit en 1 PASS
- ✅ Architecture OpenCL validée

**Inconvénients**:
- ❌ Peut échouer si address occupée
- ❌ Nécessite gestion erreurs
- ❌ Moins portable

**Implémentation**:
```c
struct drm_i915_gem_exec_object2 heap_obj = {
    .handle = heap_handle,
    .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_WRITE,
    .offset = 0x0000000100000000  // 4GB
};

// Batch construit avec GTT connue
batch[sba_offset + 1] = 0x00000000;  // Low (4GB)
batch[sba_offset + 2] = 0x00000001;  // High
```

### Décision Finale

**OPTION 1 (Relocations)** sera implémentée en priorité car:
1. Plus robuste (pas de dépendance soft pinning)
2. Modèle Gen9 correct validé par feedback expert
3. Permet de valider mécanisme relocations i915
4. Fallback vers Option 2 si échec

---

## 📊 PLAN AJUSTÉ C584

### Phase 1: Implémentation Relocations (Priorité)

**test_c584_relocation.c**:
1. Créer 2 buffers (heap 64KB + batch 4KB)
2. Setup heap layout (kernel ISA, Surface State, IDRT, Binding Table)
3. Build batch avec STATE_BASE_ADDRESS (DWords 1-2 à 0)
4. Créer 1 relocation (STATE_BASE_ADDRESS DWord 1 → heap base)
5. Submit EXECBUFFER2 avec relocation
6. Valider output[0] = 0x12345678

**Durée estimée**: 2-3 heures

### Phase 2: Fallback Soft Pinning (Si Échec Phase 1)

**test_c584_pinned.c**:
1. Créer buffer unifié 64KB
2. Setup heap layout
3. Build batch avec GTT fixe (4GB)
4. Submit EXECBUFFER2 avec EXEC_OBJECT_PINNED
5. Valider output[0] = 0x12345678

**Durée estimée**: 1-2 heures

---

## ✅ VALIDATION SPÉCIFICATIONS

### Relocations i915
- ✅ Structure drm_i915_gem_relocation_entry documentée
- ✅ Memory domains définis (RENDER pour compute)
- ✅ presumed_offset optimisation comprise
- ✅ target_handle + delta + offset mécanisme clair

### EXEC_OBJECT_PINNED
- ✅ Flag défini (1<<4)
- ✅ Documentation offset field claire
- ✅ Usage avec/sans relocations compris
- ✅ Alignment requirements identifiés

### Memory Domains
- ✅ I915_GEM_DOMAIN_RENDER pour compute workloads
- ✅ I915_GEM_DOMAIN_INSTRUCTION pour kernel ISA
- ✅ Cache coherency mécanisme compris

---

**Made with Bob - Cycle C584 PLAN MODE**  
**Statut**: Analyse Intel complète, plan ajusté, prêt pour mode advanced  
**Prochaine étape**: Switch to advanced mode pour implémentation test_c584_relocation.c