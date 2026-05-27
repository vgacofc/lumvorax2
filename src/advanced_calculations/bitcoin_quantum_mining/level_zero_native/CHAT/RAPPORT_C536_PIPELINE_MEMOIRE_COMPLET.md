# RAPPORT C536 - PIPELINE MÉMOIRE i915 COMPLET (BO → GTT → EU → SEND)

**Date**: 2026-05-25 02:26 UTC+2  
**Objectif**: Dessiner le pipeline mémoire complet avec mapping exact des structures  
**ROOT CAUSE #42**: Confusion CPU virtual addresses vs GPU GTT addresses

---

## 1. VUE D'ENSEMBLE DU PIPELINE

```
┌─────────────────────────────────────────────────────────────────┐
│                    PIPELINE MÉMOIRE i915                        │
└─────────────────────────────────────────────────────────────────┘

CPU Side                    Kernel i915              GPU Side
─────────                   ───────────              ─────────

[1] gem_create()
     ↓
[2] BO allocation
     ↓                      
[3] gem_mmap()              GTT mapping
     ↓                           ↓
[4] CPU writes         →   Physical pages    →   [5] GPU reads
     data                   pinned in RAM          via GTT
     ↓                           ↓                      ↓
[6] EXECBUFFER2        →   Batch parsing     →   [7] EU execution
     submission             Command stream          Kernel ISA
                                 ↓                      ↓
                            [8] Address           [9] SEND instruction
                                translation           (dataport write)
                                (GTT → Physical)       ↓
                                     ↓            [10] Write to memory
                                [11] Fence            (via GTT)
                                     ↓                 ↓
[12] wait_rendering()  ←   GPU completion    ←   [13] Write-back
     ↓                      signal                    complete
[14] CPU reads result
     from BO
```

---

## 2. DÉTAIL NIVEAU PAR NIVEAU

### [1-2] Allocation BO (Buffer Object)

**Code CPU**:
```c
drm_intel_bo *output_bo = drm_intel_bo_alloc(bufmgr, "output", 4096, 4096);
```

**Ce qui se passe**:
```
CPU userspace
    ↓ ioctl(DRM_IOCTL_I915_GEM_CREATE)
Kernel i915
    ↓ allocate GEM object
    ↓ assign handle (e.g., handle=5)
    ↓ allocate physical pages (lazy or immediate)
    ↓ return handle to userspace
```

**Résultat**:
- Handle: `5` (opaque identifier)
- Physical memory: Allocated but NOT yet mapped
- CPU address: NONE yet
- GPU address: NONE yet

---

### [3] Mapping CPU (gem_mmap)

**Code CPU**:
```c
drm_intel_bo_map(output_bo, 1);  // 1 = write access
uint32_t *ptr = (uint32_t*)output_bo->virtual;
```

**Ce qui se passe**:
```
CPU userspace
    ↓ ioctl(DRM_IOCTL_I915_GEM_MMAP)
Kernel i915
    ↓ create VMA (Virtual Memory Area)
    ↓ map physical pages into process address space
    ↓ return CPU virtual address
```

**Résultat**:
- CPU virtual address: `0x00007F8A12340000` (example)
- Physical pages: Same as BO
- GPU address: STILL NONE (not yet in GTT)

---

### [4] CPU Writes Data

**Code CPU**:
```c
for (int i = 0; i < 1024; i++) {
    ptr[i] = 0xFFFFFFFF;  // Initialize
}
drm_intel_bo_unmap(output_bo);
```

**Ce qui se passe**:
```
CPU write
    ↓ MMU translation (CPU virtual → physical)
    ↓ write to physical RAM pages
    ↓ cache coherency (flush if needed)
```

**Résultat**:
- Physical memory: Contains `0xFFFFFFFF` pattern
- CPU can read/write via `ptr`
- GPU: CANNOT access yet (no GTT mapping)

---

### [5] GTT Mapping (Graphics Translation Table)

**Code CPU**:
```c
struct drm_i915_gem_exec_object2 exec_obj;
exec_obj.handle = output_bo->handle;
exec_obj.flags = EXEC_OBJECT_SUPPORTS_48B_ADDRESS;
```

**Ce qui se passe lors du PASS 1**:
```
CPU userspace
    ↓ ioctl(DRM_IOCTL_I915_GEM_EXECBUFFER2)
Kernel i915
    ↓ validate BO
    ↓ allocate GPU virtual address (GTT entry)
    ↓ map physical pages into GTT
    ↓ return GPU address in exec_obj.offset
```

**Résultat**:
- GPU GTT address: `0x0000000000004000` (example from our tests)
- Physical pages: SAME as before
- CPU address: UNCHANGED
- **CRITICAL**: GPU now sees BO at `0x4000`, CPU sees it at `0x7F8A12340000`

---

### [6] EXECBUFFER2 Submission

**Code CPU**:
```c
struct drm_i915_gem_execbuffer2 execbuf;
execbuf.buffers_ptr = (uintptr_t)exec_objects;
execbuf.buffer_count = 6;
execbuf.batch_start_offset = 0;
execbuf.batch_len = 320;
execbuf.flags = I915_EXEC_RENDER | I915_EXEC_NO_RELOC;

drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
```

**Ce qui se passe**:
```
Kernel i915
    ↓ validate all BOs
    ↓ pin BOs in memory (prevent swapping)
    ↓ parse batch buffer
    ↓ validate commands
    ↓ submit to GPU ring buffer
    ↓ signal GPU to start execution
```

**Résultat**:
- Batch buffer: Queued in GPU ring
- GPU: Will start execution asynchronously
- CPU: Returns immediately (non-blocking)

---

### [7] GPU Execution - EU (Execution Unit)

**GPU Hardware**:
```
GPU Command Streamer
    ↓ fetch batch commands
    ↓ parse GPGPU_WALKER
    ↓ read IDRT (Interface Descriptor)
    ↓ load kernel ISA from GTT address
    ↓ spawn EU threads (24 EUs on UHD 620)
    ↓ each thread executes kernel
```

**IDRT Structure** (8 DWORDs):
```
DW0: Kernel Start Pointer (GTT address)
     = 0x0000000000003000 (our kernel)
     
DW1: Reserved

DW2: Sampler State Pointer

DW3: Binding Table Pointer (GTT address)
     = 0x00000000000001A0 (offset in Surface State BO)
     
DW4: CURBE Read Offset/Length

DW5: Thread Group Size

DW6: Barrier Enable / SLM Size

DW7: Reserved
```

**Kernel Execution**:
```
EU Thread 0
    ↓ fetch instruction from kernel ISA
    ↓ decode instruction
    ↓ execute (e.g., MOV, ADD, SEND)
    ↓ access registers (GRF - General Register File)
```

---

### [8] Address Translation (GTT → Physical)

**When GPU accesses memory**:
```
GPU instruction: SEND (write to address 0x4000)
    ↓
GTT lookup
    ↓ GTT[0x4000] → Physical page 0x12AB34000
    ↓
Memory Controller
    ↓ write to physical RAM
    ↓ cache coherency (LLC - Last Level Cache)
```

**CRITICAL POINT**:
- GPU uses GTT addresses (e.g., `0x4000`)
- CPU uses virtual addresses (e.g., `0x7F8A12340000`)
- BOTH map to SAME physical pages
- Translation happens in hardware (GTT + MMU)

---

### [9] SEND Instruction (Dataport Write)

**Kernel ISA** (from our kernel):
```
Offset 0x60: e0 0f 00 06 10 00 00 82
             ↑
             SEND opcode (compressed format)
```

**SEND Message Format**:
```
SEND instruction
    ↓ message type: dataport write
    ↓ destination: binding table index (BTI)
    ↓ data: register containing value (0x12345678)
    ↓ address: computed from thread ID + offset
```

**Binding Table**:
```
BTI[0] → Surface State @ offset 0x40
         ↓
         Surface State (16 DWORDs):
         DW0-1: Surface Base Address (GTT)
                = 0x0000000000004000 (output BO)
         DW2: Width, Height, Depth
         DW3: Surface Pitch
         DW4-7: Format, MOCS, etc.
```

**Dataport Write Flow**:
```
EU executes SEND
    ↓ read BTI from IDRT
    ↓ fetch Surface State from BTI
    ↓ extract base address (0x4000)
    ↓ compute final address (base + offset)
    ↓ send write message to dataport
    ↓ dataport translates via GTT
    ↓ write to physical memory
```

---

### [10] Write to Memory (via GTT)

**Physical Write**:
```
Dataport
    ↓ address: 0x4000 (GTT)
    ↓ GTT translation → 0x12AB34000 (physical)
    ↓ Memory Controller
    ↓ write 0x12345678 to physical address
    ↓ update cache (LLC)
    ↓ mark cache line dirty
```

**Result**:
- Physical memory @ `0x12AB34000`: Contains `0x12345678`
- CPU can read via `ptr[0]` (after fence)
- GPU write is complete

---

### [11] Fence / Completion Signal

**GPU Hardware**:
```
All EU threads complete
    ↓ write fence value
    ↓ signal interrupt to CPU
    ↓ update seqno (sequence number)
```

**Kernel i915**:
```
Interrupt handler
    ↓ read fence value
    ↓ mark BO as idle
    ↓ wake up waiting processes
```

---

### [12] CPU Wait (wait_rendering)

**Code CPU**:
```c
drm_intel_bo_wait_rendering(output_bo);
```

**Ce qui se passe**:
```
CPU userspace
    ↓ ioctl(DRM_IOCTL_I915_GEM_WAIT)
Kernel i915
    ↓ check if BO is idle
    ↓ if busy: sleep until fence
    ↓ if idle: return immediately
```

**Result**:
- CPU blocks until GPU completes
- Ensures memory coherency

---

### [13] Write-Back Complete

**Cache Coherency**:
```
GPU write complete
    ↓ LLC (Last Level Cache) updated
    ↓ cache line marked dirty
    ↓ eventual write-back to RAM
    ↓ CPU can read latest value
```

**On Intel integrated GPU**:
- CPU and GPU share LLC
- Coherency is automatic (mostly)
- No explicit flush needed (usually)

---

### [14] CPU Reads Result

**Code CPU**:
```c
drm_intel_bo_map(output_bo, 0);  // 0 = read-only
uint32_t *ptr = (uint32_t*)output_bo->virtual;
uint32_t result = ptr[0];  // Should be 0x12345678
drm_intel_bo_unmap(output_bo);
```

**Ce qui se passe**:
```
CPU read
    ↓ MMU translation (CPU virtual → physical)
    ↓ check cache (LLC)
    ↓ if hit: return cached value
    ↓ if miss: fetch from RAM
    ↓ return value to CPU
```

**Result**:
- CPU reads `0x12345678` (if GPU wrote correctly)
- OR `0xFFFFFFFF` (if GPU write failed) ← OUR CURRENT PROBLEM

---

## 3. MAPPING EXACT DES STRUCTURES

### Surface State (16 DWORDs = 64 bytes)

```
Offset  DWord   Content                         Our Value
------  -----   -------                         ---------
0x00    DW0     Surface Type, Format            0x00000000
0x04    DW1     Surface Base Address [31:0]     0x00004000
0x08    DW2     Surface Base Address [63:32]    0x00000000
0x0C    DW3     Width, Height                   0x00000000
0x10    DW4     Depth, Surface Pitch            0x00000000
0x14    DW5     Reserved                        0x00000000
0x18    DW6     Reserved                        0x00000000
0x1C    DW7     Reserved                        0x00000000
0x20    DW8     MOCS, Shader Channel Select     0x00000000
0x24    DW9     Reserved                        0x00000000
0x28    DW10    Reserved                        0x00000000
0x2C    DW11    Reserved                        0x00000000
0x30    DW12    Reserved                        0x00000000
0x34    DW13    Reserved                        0x00000000
0x38    DW14    Reserved                        0x00000000
0x3C    DW15    Reserved                        0x00000000
```

**CRITICAL**: DW1-DW2 contain the **GPU GTT address** of output buffer

---

### Binding Table (array of offsets)

```
Offset  Content                     Our Value
------  -------                     ---------
0x00    Offset to Surface State 0   0x00000040 (64 bytes)
0x04    Offset to Surface State 1   0x00000000 (unused)
0x08    Offset to Surface State 2   0x00000000 (unused)
...
```

**Location**: Inside Surface State BO at offset 0x1A0

---

### IDRT (Interface Descriptor - 8 DWORDs = 32 bytes)

```
Offset  DWord   Content                         Our Value
------  -----   -------                         ---------
0x00    DW0     Kernel Start Pointer [31:6]     0x00003000 >> 6
0x04    DW1     Kernel Start Pointer [47:32]    0x00000000
0x08    DW2     Reserved / Sampler State        0x00000000
0x0C    DW3     Binding Table Pointer           0x000001A0
0x10    DW4     CURBE Read Offset/Length        0x00000000
0x14    DW5     Thread Group Size               0x00000000
0x18    DW6     Barrier Enable, SLM Size        0x00000000
0x1C    DW7     Reserved                        0x00000000
```

**CRITICAL**: 
- DW0-DW1: Kernel GTT address (must be 64-byte aligned)
- DW3: Binding Table offset (relative to Surface State BO base)

---

### CURBE (Constant URB Entry)

```
Offset  Content                     Our Value
------  -------                     ---------
0x00    Kernel Argument 0           (unused in our simple kernel)
0x04    Kernel Argument 1           (unused)
...
```

**Our kernel has NO arguments** (output is via binding table, not CURBE)

---

## 4. POURQUOI 0xFFFFFFFF ?

### Analyse du Signal

`0xFFFFFFFF` est le pattern d'initialisation que nous écrivons:
```c
for (int i = 0; i < 1024; i++) {
    output[i] = 0xFFFFFFFF;
}
```

**Si output[0] reste 0xFFFFFFFF**, cela signifie:

1. **GPU n'a PAS écrit** (le plus probable)
   - SEND instruction n'a pas été exécutée
   - OU SEND a écrit à une mauvaise adresse
   - OU SEND a échoué silencieusement

2. **Kernel n'a PAS démarré**
   - IDRT invalide
   - Kernel pointer incorrect
   - Thread spawn failed

3. **Binding Table incorrect**
   - BTI pointer invalide
   - Surface State incorrect
   - Address translation failed

---

## 5. ROOT CAUSE #42 - NOTRE ERREUR

### Ce que nous avons fait (INCORRECT):

```c
// C535 - ERREUR!
AddressPatch patches[] = {
    {0x0B0, "idrt", &idrt_offset},      // idrt_offset = 0x2000 (CPU!)
    {0x114, "kernel", &kernel_offset},  // kernel_offset = 0x3000 (CPU!)
};
```

**Problème**: `idrt_offset` et `kernel_offset` sont des **CPU offsets relatifs**, PAS des **GPU GTT addresses** !

### Ce que nous DEVONS faire (CORRECT):

```c
// Après PASS 1, i915 nous donne les VRAIES adresses GTT:
uint64_t kernel_gtt = exec_objects_pass1[3].offset;  // e.g., 0x78FA3804F000
uint64_t idrt_gtt = exec_objects_pass1[2].offset;    // e.g., 0x78FA37D06001

// Utiliser CES adresses dans le batch:
AddressPatch patches[] = {
    {0x0B0, "idrt", &idrt_gtt},      // VRAIE adresse GPU!
    {0x114, "kernel", &kernel_gtt},  // VRAIE adresse GPU!
};
```

---

## 6. SOLUTION POUR C537

### Approche Correcte

1. **PASS 1**: Laisser i915 assigner les adresses GTT
2. **Lire les adresses GTT** depuis `exec_objects[].offset`
3. **Patcher le batch** avec ces adresses GTT (pas des offsets CPU)
4. **PASS 2**: Soumettre avec `EXEC_OBJECT_PINNED` + `I915_EXEC_NO_RELOC`

### Code Correct

```c
// PASS 1 - i915 assigne les adresses
drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_pass1);

// Lire les VRAIES adresses GTT
uint64_t kernel_gtt = exec_objects_pass1[3].offset;
uint64_t idrt_gtt = exec_objects_pass1[2].offset;
uint64_t output_gtt = exec_objects_pass1[4].offset;

// Patcher le batch avec les adresses GTT
drm_intel_bo_map(batch_bo, 1);
uint8_t *batch = (uint8_t*)batch_bo->virtual;
memcpy(batch, opencl_batch_template, 320);

// Patcher avec les VRAIES adresses GPU
*(uint64_t*)(batch + 0x0B0) = idrt_gtt;
*(uint64_t*)(batch + 0x114) = kernel_gtt;
// etc.

drm_intel_bo_unmap(batch_bo);

// PASS 2 - avec adresses pinnées
execbuf_pass2.flags = I915_EXEC_RENDER | I915_EXEC_NO_RELOC;
drmIoctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf_pass2);
```

---

## 7. CONCLUSION

### Pipeline Complet Résumé

```
CPU alloc BO → i915 GEM → Physical pages
                ↓
CPU mmap → CPU virtual address (0x7F8A...)
                ↓
CPU write data → Physical RAM
                ↓
EXECBUFFER2 → i915 GTT mapping → GPU virtual address (0x4000)
                ↓
GPU execution → EU threads → Kernel ISA
                ↓
SEND instruction → Binding Table → Surface State
                ↓
Dataport write → GTT translation → Physical RAM
                ↓
Fence → CPU wait → CPU read result
```

### Erreur Critique Identifiée

**ROOT CAUSE #42**: Confusion entre:
- **CPU virtual addresses**: `0x00007F8A12340000`
- **CPU relative offsets**: `0x0000000000003000`
- **GPU GTT addresses**: `0x000078FA3804F000`

**Solution**: Utiliser les adresses GTT retournées par i915 dans le PASS 1, PAS des offsets inventés.

---

**FIN DU RAPPORT C536**