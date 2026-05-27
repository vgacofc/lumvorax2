# PROJET B - KERNEL i915 PATCHES
## Phase 15Y-F27 - Investigation Kernel Linux i915

### OBJECTIF
Développer un patch kernel i915 custom pour accès direct GPU Gen9 sans dépendre d'Intel NEO.

---

## PHASE 1 : INVESTIGATION (Semaine 1)

### 1.1 Architecture i915 Driver

**Fichiers clés à analyser** :
```
drivers/gpu/drm/i915/
├── i915_drv.c              # Point d'entrée driver
├── i915_gem.c              # GEM (Graphics Execution Manager)
├── gem/
│   ├── i915_gem_execbuffer.c   # Soumission commandes GPU
│   ├── i915_gem_context.c      # Contextes GPU
│   └── i915_gem_object.c       # Objets mémoire GPU
├── gt/
│   ├── intel_engine_cs.c       # Moteurs GPU (RCS, CCS, etc.)
│   ├── intel_context.c         # Contextes hardware
│   └── intel_ring_submission.c # Soumission via ring buffer
└── i915_debugfs.c          # Interface debug
```

### 1.2 IOCTLs Privés Intel NEO

**IOCTLs à identifier** :
- `DRM_IOCTL_I915_GEM_EXECBUFFER2` : Soumission batch buffer
- `DRM_IOCTL_I915_GEM_CREATE` : Allocation mémoire GPU
- `DRM_IOCTL_I915_GEM_MMAP` : Mapping mémoire
- `DRM_IOCTL_I915_GEM_CONTEXT_CREATE` : Création contexte
- IOCTLs privés NEO (à découvrir via strace)

### 1.3 Méthodologie Investigation

**Étape 1 : Strace Intel NEO**
```bash
# Capturer tous les ioctls utilisés par NEO
strace -e ioctl -o neo_ioctls.log ./test_neo_wrapper
```

**Étape 2 : Analyse source kernel**
```bash
# Télécharger source kernel Linux
cd /usr/src
sudo apt-get source linux-image-$(uname -r)
cd linux-*/drivers/gpu/drm/i915
```

**Étape 3 : Identifier ioctls privés**
```bash
# Chercher définitions ioctls
grep -r "DRM_IOCTL_I915" include/uapi/drm/i915_drm.h
```

---

## PHASE 2 : DÉVELOPPEMENT PATCH (Semaines 2-8)

### 2.1 Patch Instrumenté (Logging)

**Objectif** : Ajouter logging kernel pour tracer toutes les opérations GPU.

**Fichier** : `patch_i915_logging.patch`

**Modifications** :
- Ajouter `printk()` dans `i915_gem_execbuffer.c`
- Logger tous les paramètres batch buffer
- Tracer allocations mémoire GPU
- Capturer états hardware

### 2.2 Patch Custom IOCTL

**Objectif** : Créer ioctl custom pour accès direct GPU.

**Fichier** : `patch_i915_custom_ioctl.patch`

**Nouveau ioctl** : `DRM_IOCTL_I915_LUMVORAX_DISPATCH`

**Fonctionnalités** :
- Soumission batch buffer simplifié
- Pas de validation complexe NEO
- Accès direct ring buffer
- Logging bit-level intégré

### 2.3 Compilation Kernel Custom

```bash
# Configuration kernel
cd /usr/src/linux-*
cp /boot/config-$(uname -r) .config
make oldconfig

# Appliquer patches
patch -p1 < patch_i915_logging.patch
patch -p1 < patch_i915_custom_ioctl.patch

# Compiler
make -j$(nproc) LOCALVERSION=-lumvorax
make modules_install
make install

# Reboot sur nouveau kernel
sudo reboot
```

---

## PHASE 3 : VALIDATION (Semaines 9-12)

### 3.1 Tests Unitaires

**Programme test** : `test_custom_ioctl.c`
- Utiliser nouveau ioctl `DRM_IOCTL_I915_LUMVORAX_DISPATCH`
- Valider 10000 dispatches stables
- Comparer performances vs NEO

### 3.2 Intégration LumVorax

**Fichier** : `btc_i915_native.c`
- Remplacer OpenCL par ioctl custom
- Logging bit-level complet
- Métriques forensiques

### 3.3 Benchmarks

**Comparaison** :
- NEO Wrapper (Projet A) : baseline
- Kernel custom (Projet B) : objectif +20% perf
- Stabilité : 0 crash sur 1M dispatches

---

## TIMELINE DÉTAILLÉE

### Semaine 1 (EN COURS)
- [x] Créer structure Projet B
- [ ] Strace Intel NEO
- [ ] Télécharger source kernel
- [ ] Analyser `i915_gem_execbuffer.c`
- [ ] Identifier ioctls privés

### Semaines 2-3
- [ ] Développer patch logging
- [ ] Compiler kernel instrumenté
- [ ] Capturer logs kernel
- [ ] Analyser comportement GPU

### Semaines 4-6
- [ ] Développer patch custom ioctl
- [ ] Implémenter `DRM_IOCTL_I915_LUMVORAX_DISPATCH`
- [ ] Compiler kernel custom
- [ ] Tests unitaires ioctl

### Semaines 7-8
- [ ] Développer `btc_i915_native.c`
- [ ] Intégrer logging bit-level
- [ ] Tests stabilité

### Semaines 9-12
- [ ] Benchmarks complets
- [ ] Optimisations
- [ ] Documentation
- [ ] **PRODUCTION READY**

---

## RISQUES ET MITIGATIONS

### Risque 1 : Complexité kernel
**Mitigation** : Commencer par patch logging simple, itérer progressivement.

### Risque 2 : Instabilité kernel
**Mitigation** : Tests exhaustifs en VM avant déploiement production.

### Risque 3 : Maintenance long terme
**Mitigation** : Documenter tous les changements, suivre évolutions kernel upstream.

---

## RESSOURCES

### Documentation
- [Intel i915 Driver Documentation](https://www.kernel.org/doc/html/latest/gpu/i915.html)
- [DRM Subsystem](https://www.kernel.org/doc/html/latest/gpu/drm-internals.html)
- [Linux Kernel Development](https://www.kernel.org/doc/html/latest/process/development-process.html)

### Source Code
- [Linux Kernel Git](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git)
- [Intel Graphics Driver](https://cgit.freedesktop.org/drm-intel/)

### Outils
- `strace` : Tracer syscalls/ioctls
- `perf` : Profiling kernel
- `ftrace` : Tracer fonctions kernel
- `dmesg` : Logs kernel

---

## NOTES TECHNIQUES

### Gen9 Architecture (UHD 620)
- **EU (Execution Units)** : 24 EUs
- **Threads par EU** : 7 threads
- **Total threads** : 168 threads simultanés
- **Fréquence** : 300-1000 MHz
- **Cache L3** : 768 KB

### Ring Buffer Submission
```c
// Pseudo-code soumission batch buffer
struct i915_execbuffer2 {
    uint64_t buffers_ptr;      // Pointeur liste buffers
    uint32_t buffer_count;     // Nombre buffers
    uint32_t batch_start_offset; // Offset batch buffer
    uint32_t batch_len;        // Taille batch
    uint32_t flags;            // Flags soumission
    uint64_t rsvd1;
    uint64_t rsvd2;
};

// Ioctl soumission
ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
```

---

## PROCHAINES ÉTAPES IMMÉDIATES

1. **Strace NEO** : Capturer ioctls utilisés
2. **Télécharger kernel** : Source Linux 6.17
3. **Analyser execbuffer** : Comprendre soumission batch
4. **Identifier ioctls privés** : Reverse engineering NEO

**Début investigation** : MAINTENANT (parallèle Projet A)