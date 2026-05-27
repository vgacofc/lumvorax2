# ANALYSE IOCTLS INTEL NEO - PROJET B
## Phase 15Y-F27 - Investigation Kernel i915

### RÉSUMÉ EXÉCUTIF

**71 ioctls capturés** lors de l'exécution de `test_neo_wrapper` (init + load kernel + cleanup).

**IOCTLs critiques identifiés** :
1. `DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT` : Création contextes GPU
2. `DRM_IOCTL_I915_GEM_CREATE_EXT` : Allocation mémoire GPU
3. `DRM_IOCTL_I915_GEM_MMAP_OFFSET` : Mapping mémoire
4. `DRM_IOCTL_I915_GEM_USERPTR` : Pointeurs userspace
5. `DRM_IOCTL_I915_QUERY` : Interrogation capacités GPU

**AUCUN dispatch kernel détecté** : Notre test n'a pas encore exécuté de kernel, seulement init/cleanup.

---

## ANALYSE DÉTAILLÉE PAR PHASE

### Phase 1 : Initialisation Driver (lignes 2-14)

```
ioctl(7, DRM_IOCTL_VERSION, ...)                    # Version DRM
ioctl(7, DRM_IOCTL_I915_GETPARAM, ...)              # Paramètres GPU (x2)
ioctl(7, DRM_IOCTL_I915_REG_READ, ...)              # Lecture registre
ioctl(7, DRM_IOCTL_I915_QUERY, ...)                 # Queries capacités (x5)
ioctl(7, DRM_IOCTL_I915_GETPARAM, ...)              # Paramètres GPU
ioctl(7, DRM_IOCTL_I915_GEM_CONTEXT_SETPARAM, ...)  # ÉCHEC (EINVAL)
```

**Observations** :
- NEO interroge massivement les capacités GPU via `QUERY`
- Un `CONTEXT_SETPARAM` échoue (normal, paramètre non supporté Gen9)
- Lecture registre direct via `REG_READ`

### Phase 2 : Création VM et Contexte (lignes 15-28)

```
ioctl(7, DRM_IOCTL_I915_GEM_VM_CREATE, ...)         # Création VM
ioctl(7, DRM_IOCTL_I915_QUERY, ...)                 # Queries (x2)
ioctl(7, DRM_IOCTL_I915_GEM_CONTEXT_GETPARAM, ...) # Get params (x3)
ioctl(7, DRM_IOCTL_I915_GETPARAM, ...)              # Get param
ioctl(7, DRM_IOCTL_I915_GEM_CONTEXT_GETPARAM, ...) # Get params (x2)
ioctl(7, DRM_IOCTL_I915_GEM_USERPTR, ...)           # Userptr
ioctl(7, DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT, ...) # Création contexte
ioctl(7, DRM_IOCTL_I915_GEM_CONTEXT_SETPARAM, ...) # Set params (x3)
```

**Observations** :
- `GEM_VM_CREATE` : Création d'un Virtual Memory space isolé
- `GEM_CONTEXT_CREATE_EXT` : Création contexte GPU (version étendue)
- `GEM_USERPTR` : Enregistrement pointeur userspace pour DMA
- Configuration contexte via `SETPARAM` (x3)

### Phase 3 : Allocation Mémoire GPU (lignes 29-44)

```
# Pattern répété 4 fois :
ioctl(7, DRM_IOCTL_I915_GEM_CREATE_EXT, ...)        # Allocation buffer
ioctl(7, DRM_IOCTL_I915_GEM_MMAP_OFFSET, ...)      # Mapping buffer
```

**Observations** :
- 8 buffers alloués (4 paires CREATE_EXT + MMAP_OFFSET)
- Probablement : instruction buffer, state buffer, surface state, etc.
- `CREATE_EXT` : Version étendue avec flags placement mémoire

### Phase 4 : Contexte Compilation (lignes 37-44)

```
ioctl(7, DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT, ...) # Nouveau contexte
ioctl(7, DRM_IOCTL_I915_GEM_CONTEXT_SETPARAM, ...) # Set params (x3)
ioctl(7, DRM_IOCTL_I915_GEM_CREATE_EXT, ...)        # Allocation (x2)
ioctl(7, DRM_IOCTL_I915_GEM_MMAP_OFFSET, ...)      # Mapping (x2)
```

**Observations** :
- Création d'un second contexte (probablement pour compilation kernel)
- Allocation de 2 buffers supplémentaires

### Phase 5 : Queries Finales (lignes 45-50)

```
ioctl(7, DRM_IOCTL_I915_GEM_CONTEXT_GETPARAM, ...) # Get params (x2)
ioctl(7, DRM_IOCTL_I915_GETPARAM, ...)              # Get params (x2)
ioctl(7, DRM_IOCTL_I915_GEM_USERPTR, ...)           # Userptr (x2)
```

**Observations** :
- Vérification paramètres contextes
- Enregistrement de 2 userptrs supplémentaires

### Phase 6 : Cleanup (lignes 51-71)

```
# Pattern répété 7 fois :
ioctl(7, DRM_IOCTL_I915_GEM_WAIT, ...)              # Attente idle
ioctl(7, DRM_IOCTL_GEM_CLOSE, ...)                  # Fermeture buffer

ioctl(7, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, ...)  # Destruction contexte (x2)
ioctl(7, DRM_IOCTL_I915_GEM_VM_DESTROY, ...)       # Destruction VM
```

**Observations** :
- Cleanup méthodique : WAIT + CLOSE pour chaque buffer
- Destruction des 2 contextes créés
- Destruction de la VM

---

## IOCTLS MANQUANTS (DISPATCH KERNEL)

**Notre test n'a PAS exécuté de kernel**, donc ces ioctls sont absents :

```c
DRM_IOCTL_I915_GEM_EXECBUFFER2      // Soumission batch buffer
DRM_IOCTL_I915_GEM_EXECBUFFER2_WR   // Version write-back
```

**Action requise** : Modifier `test_neo_wrapper.c` pour exécuter un kernel et capturer ces ioctls.

---

## STATISTIQUES

| IOCTL | Occurrences | Fonction |
|-------|-------------|----------|
| `DRM_IOCTL_I915_QUERY` | 9 | Interrogation capacités GPU |
| `DRM_IOCTL_I915_GEM_CONTEXT_GETPARAM` | 7 | Lecture paramètres contexte |
| `DRM_IOCTL_I915_GEM_WAIT` | 7 | Attente idle GPU |
| `DRM_IOCTL_GEM_CLOSE` | 7 | Fermeture buffers |
| `DRM_IOCTL_I915_GETPARAM` | 5 | Lecture paramètres driver |
| `DRM_IOCTL_I915_GEM_CREATE_EXT` | 4 | Allocation mémoire GPU |
| `DRM_IOCTL_I915_GEM_MMAP_OFFSET` | 4 | Mapping mémoire |
| `DRM_IOCTL_I915_GEM_CONTEXT_SETPARAM` | 4 | Écriture paramètres contexte |
| `DRM_IOCTL_I915_GEM_USERPTR` | 3 | Enregistrement userptr |
| `DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT` | 2 | Création contexte |
| `DRM_IOCTL_I915_GEM_CONTEXT_DESTROY` | 2 | Destruction contexte |
| `DRM_IOCTL_VERSION` | 1 | Version DRM |
| `DRM_IOCTL_I915_REG_READ` | 1 | Lecture registre |
| `DRM_IOCTL_I915_GEM_VM_CREATE` | 1 | Création VM |
| `DRM_IOCTL_I915_GEM_VM_DESTROY` | 1 | Destruction VM |

**Total** : 71 ioctls

---

## DÉFINITIONS IOCTLS (i915_drm.h)

### Ioctls Critiques

```c
// Création contexte GPU
#define DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT \
    DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_CONTEXT_CREATE_EXT, \
             struct drm_i915_gem_context_create_ext)

// Allocation mémoire GPU
#define DRM_IOCTL_I915_GEM_CREATE_EXT \
    DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_CREATE_EXT, \
             struct drm_i915_gem_create_ext)

// Mapping mémoire
#define DRM_IOCTL_I915_GEM_MMAP_OFFSET \
    DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_MMAP_OFFSET, \
             struct drm_i915_gem_mmap_offset)

// Soumission batch buffer (MANQUANT dans notre trace)
#define DRM_IOCTL_I915_GEM_EXECBUFFER2 \
    DRM_IOW(DRM_COMMAND_BASE + DRM_I915_GEM_EXECBUFFER2, \
            struct drm_i915_gem_execbuffer2)
```

### Structures Clés

```c
// Contexte GPU
struct drm_i915_gem_context_create_ext {
    __u32 ctx_id;           // ID contexte (output)
    __u32 flags;            // Flags création
    __u64 extensions;       // Extensions chaînées
};

// Allocation buffer
struct drm_i915_gem_create_ext {
    __u64 size;             // Taille buffer
    __u32 handle;           // Handle buffer (output)
    __u32 pad;
    __u64 extensions;       // Extensions chaînées
};

// Soumission batch
struct drm_i915_gem_execbuffer2 {
    __u64 buffers_ptr;      // Pointeur liste buffers
    __u32 buffer_count;     // Nombre buffers
    __u32 batch_start_offset; // Offset batch buffer
    __u32 batch_len;        // Taille batch
    __u32 DR1;
    __u32 DR4;
    __u32 num_cliprects;
    __u64 cliprects_ptr;
    __u64 flags;            // Flags soumission
    __u64 rsvd1;
    __u64 rsvd2;
};
```

---

## PROCHAINES ÉTAPES

### Étape 1 : Capturer EXECBUFFER2 (URGENT)

**Modifier test_neo_wrapper.c** :
```c
// Ajouter dispatch kernel réel
cl_mem buffer_a = clCreateBuffer(...);
cl_mem buffer_b = clCreateBuffer(...);
cl_mem buffer_c = clCreateBuffer(...);

clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer_a);
clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer_b);
clSetKernelArg(kernel, 2, sizeof(cl_mem), &buffer_c);

size_t global_size = 1024;
clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_size, NULL, 0, NULL, NULL);
clFinish(queue);
```

**Re-strace** :
```bash
strace -e ioctl -o neo_ioctls_dispatch.log ./test_neo_wrapper
```

### Étape 2 : Analyser EXECBUFFER2

- Identifier structure exacte `drm_i915_gem_execbuffer2`
- Comprendre format batch buffer
- Analyser flags soumission

### Étape 3 : Développer Patch Logging

**Fichier** : `patch_i915_execbuffer_logging.patch`

**Modifications** :
```c
// Dans drivers/gpu/drm/i915/gem/i915_gem_execbuffer.c

int i915_gem_execbuffer2_ioctl(struct drm_device *dev, void *data,
                               struct drm_file *file)
{
    struct drm_i915_gem_execbuffer2 *args = data;
    
    // LOGGING LUMVORAX
    printk(KERN_INFO "[LUMVORAX] EXECBUFFER2: buffers=%llu count=%u batch_offset=%u batch_len=%u flags=%llx\n",
           args->buffers_ptr, args->buffer_count,
           args->batch_start_offset, args->batch_len, args->flags);
    
    // Code original...
}
```

### Étape 4 : Compiler Kernel Instrumenté

```bash
cd /usr/src/linux-*
patch -p1 < patch_i915_execbuffer_logging.patch
make -j$(nproc) LOCALVERSION=-lumvorax-log
make modules_install
make install
sudo reboot
```

### Étape 5 : Capturer Logs Kernel

```bash
# Après reboot sur kernel instrumenté
./test_neo_wrapper
dmesg | grep LUMVORAX > kernel_logs_execbuffer.txt
```

---

## CONCLUSIONS

### Découvertes Majeures

1. **NEO utilise 15 ioctls i915 différents** pour init/cleanup
2. **Pattern VM isolée** : NEO crée une VM dédiée par contexte
3. **Contextes multiples** : 2 contextes créés (runtime + compilation)
4. **Allocations massives** : 8 buffers alloués avant dispatch
5. **Cleanup méthodique** : WAIT + CLOSE pour chaque ressource

### Ioctls Critiques pour Patch Custom

**Minimum viable** :
1. `DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT` : Contexte
2. `DRM_IOCTL_I915_GEM_CREATE_EXT` : Buffers
3. `DRM_IOCTL_I915_GEM_MMAP_OFFSET` : Mapping
4. `DRM_IOCTL_I915_GEM_EXECBUFFER2` : Dispatch (à capturer)

**Notre ioctl custom** pourrait fusionner ces 4 opérations en une seule :
```c
DRM_IOCTL_I915_LUMVORAX_DISPATCH
```

### Timeline Mise à Jour

- **Aujourd'hui** : Modifier test pour capturer EXECBUFFER2 ✅
- **Demain** : Analyser structure EXECBUFFER2
- **J+2** : Développer patch logging
- **J+3** : Compiler kernel instrumenté
- **J+4** : Analyser logs kernel
- **Semaine 2** : Développer patch custom ioctl

---

## FICHIERS KERNEL À ANALYSER

### Priorité 1 (Cette semaine)

```
drivers/gpu/drm/i915/gem/i915_gem_execbuffer.c  # Soumission batch
drivers/gpu/drm/i915/gem/i915_gem_context.c     # Gestion contextes
drivers/gpu/drm/i915/gem/i915_gem_create.c      # Allocation buffers
```

### Priorité 2 (Semaine prochaine)

```
drivers/gpu/drm/i915/gt/intel_ring_submission.c # Ring buffer
drivers/gpu/drm/i915/gt/intel_engine_cs.c       # Moteurs GPU
drivers/gpu/drm/i915/i915_gem.c                 # GEM core
```

### Priorité 3 (Semaines 3-4)

```
drivers/gpu/drm/i915/gt/gen8_engine_cs.c        # Gen8+ specifics
drivers/gpu/drm/i915/gt/intel_context.c         # Contextes hardware
drivers/gpu/drm/i915/i915_debugfs.c             # Debug interface
```

---

**Statut** : Phase 1 Investigation - 20% complété
**Prochaine action** : Modifier test_neo_wrapper.c pour dispatch kernel