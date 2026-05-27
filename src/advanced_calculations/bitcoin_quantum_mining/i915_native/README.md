# i915_native — Bitcoin Mining via i915 DRM Direct

**Version** : 1.0  
**Date** : 2026-05-11  
**Architecture** : Intel Gen9 (UHD 620)  
**Indépendance** : 0% OpenCL, 0% Level Zero, 100% i915 DRM natif

---

## 🎯 OBJECTIF

Développer une implémentation **100% native i915 DRM** pour Bitcoin mining SHA256, totalement indépendante d'OpenCL et Level Zero, avec **traçabilité bit-level complète** sur toutes les couches (RAM, CPU, GPU).

---

## 📁 STRUCTURE RÉPERTOIRE

```
i915_native/
├── README.md                   # Ce fichier
├── STANDARD_NAMES_i915.md      # Conventions de nommage
├── Makefile                    # Compilation
├── src/                        # Code source C
│   ├── btc_gen9_native_runner.c       # Runner Gen9 natif
│   ├── btc_gen9_native_compiler.h     # Compiler batch buffer
│   ├── btc_bit_level_tracker.c        # Tracker bit-level (à créer)
│   └── btc_memory_tracker_gpu.c       # Tracker mémoire GPU (à créer)
├── include/                    # Headers publics
│   ├── btc_gen9_native_runner.h
│   ├── btc_bit_level_tracker.h        # (à créer)
│   └── btc_memory_tracker_gpu.h       # (à créer)
├── tests/                      # Tests unitaires
│   ├── test_gen9_native.c
│   ├── test_btc_mining_native.c
│   └── test_bit_level_tracker.c       # (à créer)
├── kernels/                    # Binaires ISA Gen9
│   └── btc_sha256_mining_gen9.bin     # Kernel SHA256 (42KB)
├── logs/forensic/              # Logs forensiques
│   ├── btc_mining_native_sha256.log   # Logs texte
│   └── *.lum                          # Logs bit-level (à générer)
├── CHAT/                       # Rapports conversation
│   └── RAPPORT_C198_*.md
├── bin/                        # Exécutables compilés
└── build/                      # Objets compilation
```

---

## 🚀 COMPILATION

```bash
# Compiler tout
make all

# Compiler et tester
make test

# Test Bitcoin mining (1000 dispatches)
make test_btc

# Nettoyer
make clean
```

---

## 📊 MÉTRIQUES ACTUELLES

### Performance
- **Init time** : 1.35 ms (37× plus rapide qu'OpenCL)
- **Dispatch time** : 685.9 ms (dispatches 2-3)
- **Dispatch 1** : 7206 ms (anormal, 10× plus lent)
- **Throughput** : 3.07 MH/s (vs 21.86 MH/s OpenCL)
- **Stabilité** : ±1.5% variance temporelle

### Limites Actuelles
- ❌ **Échec dispatch 4** : errno=5 (EIO) sur EXECBUFFER2
- ❌ **Dispatches max** : 3 (puis échec systématique)
- ❌ **Traçabilité bit-level** : NON implémentée (format .lum manquant)
- ❌ **Memory tracker** : NON actif (pas de tracking RAM/GPU)

---

## 🔬 PROBLÈME CRITIQUE : errno=5 au Dispatch 4

### Pattern Observé
```
Dispatch 1: 7206 ms ✅ (anormal, initialisation GPU lourde)
Dispatch 2: 757 ms  ✅ (normal)
Dispatch 3: 693 ms  ✅ (normal)
Dispatch 4: ÉCHEC  ❌ errno=5 (Input/output error)
```

### Hypothèses
1. **Watchdog i915** : Protection anti-abuse après 3 dispatches
2. **Limite ressources GPU** : Épuisement après 3 exécutions
3. **État GPU corrompu** : Dispatch 1 initialise, 2-3 OK, 4 état invalide
4. **Hangcheck kernel** : Détection pattern répétitif

### Tests Effectués
- ✅ Test recyclage contexte : INVALIDÉ (échec même sans recycle)
- ✅ Test contexte persistant : Échec identique au dispatch 4
- ⏳ Test kernel simple : À faire (éliminer hypothèse SHA256 trop lourd)
- ⏳ Monitoring GEM objects : À faire (vérifier fuites)
- ⏳ Analyse dmesg : À faire (logs kernel i915)

---

## 🛠️ DIFFÉRENCES AVEC level_zero_native

| Aspect | level_zero_native | i915_native |
|--------|-------------------|-------------|
| **API** | Level Zero (ze_*) | i915 DRM (ioctl) |
| **Dépendances** | libze_loader.so | libdrm.so uniquement |
| **Contextes** | ze_context_handle_t | uint32_t ctx_id |
| **Buffers** | ze_device_mem_alloc | GEM objects |
| **Exécution** | zeCommandListAppendLaunchKernel | DRM_IOCTL_I915_GEM_EXECBUFFER2 |
| **Préfixes** | btc_ze_* | btc_gen9_*, btc_i915_* |
| **Répertoire** | level_zero_native/ | i915_native/ |
| **Indépendance** | Dépend Level Zero | 100% natif |

---

## 📋 PROCHAINES ÉTAPES

### Priorité 1 : Traçabilité Bit-Level
- [ ] Implémenter `btc_bit_level_tracker.c`
- [ ] Format fichier `.lum` (header + événements)
- [ ] Tracking GEM_CREATE, GPU_MMAP, BATCH_WRITE, EXECBUFFER2, GPU_WAIT
- [ ] Tests unitaires tracker

### Priorité 2 : Memory Tracker
- [ ] Implémenter `btc_memory_tracker_gpu.c`
- [ ] Tracking allocations RAM (malloc/free)
- [ ] Tracking allocations GPU (GEM create/close)
- [ ] Détection fuites mémoire

### Priorité 3 : Reverse Engineering errno=5
- [ ] Tester kernel simple (addition 1+1)
- [ ] Monitoring `/sys/kernel/debug/dri/0/i915_gem_objects`
- [ ] Analyser `dmesg` après échec
- [ ] Tester différentes tailles batch buffer
- [ ] Tester différents timeouts GPU_WAIT

### Priorité 4 : Optimisations
- [ ] Réduire temps dispatch 1 (7.2 sec → <1 sec)
- [ ] Augmenter dispatches max (3 → illimité)
- [ ] Augmenter throughput (3.07 → 21.86 MH/s)
- [ ] Batch processing (grouper dispatches)

---

## 📖 DOCUMENTATION

- [`STANDARD_NAMES_i915.md`](STANDARD_NAMES_i915.md) : Conventions de nommage
- [`CHAT/RAPPORT_C198_*.md`](CHAT/) : Rapports détaillés phases 15N-15U
- [`logs/forensic/`](logs/forensic/) : Logs forensiques exécution

---

## 🔗 DÉPENDANCES

```bash
# Ubuntu/Debian
sudo apt install libdrm-dev

# Vérifier permissions GPU
ls -l /dev/dri/renderD128
# Doit être accessible en lecture/écriture
```

---

## ⚠️ RÈGLES STRICTES

### ❌ INTERDICTIONS ABSOLUES
1. **Aucune dépendance OpenCL** : Pas de `#include <CL/cl.h>`
2. **Aucune dépendance Level Zero** : Pas de `#include <level_zero/ze_api.h>`
3. **Pas de variables globales** : Sauf constantes `static const`
4. **Pas de malloc() direct** : Utiliser macros tracking
5. **Pas de printf() direct** : Utiliser macros LOG_*

### ✅ OBLIGATIONS ABSOLUES
1. **Vérifier errno** : Après chaque ioctl()
2. **Cleanup garanti** : Même en cas d'échec
3. **Logs forensiques** : Chaque allocation/libération
4. **Format .lum** : Traçabilité bit-level
5. **Tests unitaires** : Chaque fonction publique

---

## 📞 CONTACT

**Projet** : LumVorax  
**Module** : i915_native  
**Phase** : C198 (15N-15U)  
**Objectif** : 100% indépendance OpenCL/Level Zero

---

**FIN README.md**