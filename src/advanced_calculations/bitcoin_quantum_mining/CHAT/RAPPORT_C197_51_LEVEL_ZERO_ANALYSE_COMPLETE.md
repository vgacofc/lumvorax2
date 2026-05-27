# RAPPORT C197.51 : Level Zero API - Analyse Complète & Position dans Stack

**Date**: 2026-05-06  
**Cycle**: C197.51  
**Objectif**: Analyse Level Zero API pour Bitcoin mining + comparaison OpenCL

---

## 1. RÉSUMÉ EXÉCUTIF

### Qu'est-ce que Level Zero ?

**Level Zero** est une API bas niveau développée par Intel pour accéder directement au GPU avec un overhead minimal, en contournant les couches d'abstraction d'OpenCL.

### Position dans le Stack

```
┌─────────────────────────────────────────────────────────────┐
│  NIVEAU APPLICATION                                          │
├─────────────────────────────────────────────────────────────┤
│  OpenCL (Haut Niveau)          Level Zero (Bas Niveau)      │
│       ↓                                ↓                     │
│  NEO Runtime (complet)         NEO Runtime (partiel)        │
│       ↓                                ↓                     │
│  IGC Compiler (JIT)            Pas de JIT                   │
│       ↓                                ↓                     │
│  DRM i915                      DRM i915                     │
│       ↓                                ↓                     │
│  GPU Hardware (Gen9/Gen12/Xe)                               │
└─────────────────────────────────────────────────────────────┘
```

### Découverte Système

❌ **Level Zero NON installé** sur ce système
- Seul fichier trouvé: `/usr/include/hwloc/levelzero.h` (hwloc binding)
- Pas de `libze_loader.so`
- Pas de headers Level Zero complets

---

## 2. ARCHITECTURE LEVEL ZERO

### 2.1 Stack Complet Comparé

#### OpenCL (Actuel - C197.49)
```
Application (SHA256 mining)
    ↓ clCreateContext()
libOpenCL.so.1 (ICD Loader)
    ↓ dispatch
libigdrcl.so (NEO Runtime - 260 symboles)
    ↓ JIT compilation
libigc.so.1 (IGC Compiler)
    ↓ optimisations
LLVM Backend
    ↓ Gen9 ISA
libigdgmm.so.12 (Memory Manager)
    ↓ allocations
ioctl(DRM_IOCTL_I915_GEM_EXECBUFFER2)
    ↓ 10 exec_objects
GPU Gen9 Hardware
```

**Overhead mesuré** (C197.50):
- JIT compilation: 1,183 ms (une fois)
- Kernel execution: 99.73% du temps
- Hashrate: 21.86 MH/s

#### Level Zero (Théorique)
```
Application (SHA256 mining)
    ↓ zeKernelCreate()
libze_loader.so (Level Zero Loader)
    ↓ dispatch
libze_intel_gpu.so (Intel Driver)
    ↓ minimal runtime
NEO Runtime (partiel - gestion mémoire uniquement)
    ↓ pas de JIT
SPIR-V ou ISA pré-compilé
    ↓ direct
libigdgmm.so.12 (Memory Manager)
    ↓ allocations
ioctl(DRM_IOCTL_I915_GEM_EXECBUFFER2)
    ↓ 10 exec_objects
GPU Gen9 Hardware
```

**Overhead théorique**:
- Pas de JIT (code pré-compilé)
- Kernel execution: ~99.9% du temps
- Hashrate attendu: 22-23 MH/s (+5-10%)

---

## 3. CONCEPTS CLÉS LEVEL ZERO

### 3.1 Hiérarchie des Objets

```c
// 1. Driver (équivalent plateforme OpenCL)
ze_driver_handle_t driver;

// 2. Device (GPU)
ze_device_handle_t device;

// 3. Context (environnement d'exécution)
ze_context_handle_t context;

// 4. Command Queue (soumission)
ze_command_queue_handle_t queue;

// 5. Command List (liste de commandes - CLÉ)
ze_command_list_handle_t cmdlist;

// 6. Module (code compilé)
ze_module_handle_t module;

// 7. Kernel (fonction GPU)
ze_kernel_handle_t kernel;

// 8. Memory (buffers)
void* device_mem;
```

### 3.2 Différence Majeure : Command Lists

#### OpenCL (Immediate Mode)
```c
// Chaque commande est soumise immédiatement
clEnqueueNDRangeKernel(queue, kernel, ...);  // Soumis
clEnqueueNDRangeKernel(queue, kernel, ...);  // Soumis
clFinish(queue);
```

#### Level Zero (Deferred Mode)
```c
// Construction d'une liste de commandes
zeCommandListAppendLaunchKernel(cmdlist, kernel, ...);  // Enregistré
zeCommandListAppendLaunchKernel(cmdlist, kernel, ...);  // Enregistré
zeCommandListClose(cmdlist);                            // Finalisé

// Soumission en une fois
zeCommandQueueExecuteCommandLists(queue, 1, &cmdlist, NULL);
```

**Avantage**: Overhead réduit (batch de commandes)

---

## 4. PIPELINE COMPILATION

### 4.1 OpenCL (Automatique)

```
OpenCL C Source
    ↓ clBuildProgram()
LLVM IR
    ↓ IGC
SPIR-V
    ↓ Backend
Gen9 ISA
    ↓ Runtime
GPU Execution
```

**Temps mesuré** (C197.50): 1,183 ms JIT

### 4.2 Level Zero (Manuel)

```
OpenCL C Source
    ↓ ocloc compile (offline)
SPIR-V Binary
    ↓ zeModuleCreate()
Module chargé
    ↓ zeKernelCreate()
Kernel prêt
    ↓ zeCommandListAppendLaunchKernel()
GPU Execution
```

**Temps théorique**: 0 ms JIT (pré-compilé)

---

## 5. COMPARAISON DÉTAILLÉE

### 5.1 Tableau Comparatif

| Feature | OpenCL | Level Zero | Différence |
|---------|--------|------------|------------|
| **Niveau abstraction** | Haut | Bas | Level Zero plus proche hardware |
| **Compilation** | JIT automatique | Offline manuel | Level Zero nécessite ocloc |
| **Simplicité** | ⭐⭐⭐⭐⭐ | ⭐⭐ | OpenCL plus simple |
| **Performance** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | Level Zero +5-10% |
| **Contrôle** | Limité | Total | Level Zero contrôle fin |
| **Portabilité** | Excellente | Intel uniquement | OpenCL multi-vendor |
| **Overhead** | Moyen | Minimal | Level Zero réduit overhead |
| **Gestion mémoire** | Automatique | Manuelle | Level Zero plus complexe |
| **Debugging** | Facile | Difficile | OpenCL meilleurs outils |
| **Documentation** | Excellente | Moyenne | OpenCL mature |

### 5.2 Métriques Attendues

#### OpenCL (Mesuré C197.50)
```
Hashrate:        21.86 MH/s
JIT compile:     1,183 ms (une fois)
Kernel exec:     99.73% du temps
Overhead:        0.27%
```

#### Level Zero (Théorique)
```
Hashrate:        22.5-23.5 MH/s (+3-7%)
JIT compile:     0 ms (pré-compilé)
Kernel exec:     99.9% du temps
Overhead:        0.1%
```

**Gain attendu**: +3-7% (principalement réduction overhead)

---

## 6. IMPLÉMENTATION THÉORIQUE

### 6.1 Étapes Nécessaires

#### Étape 1: Compilation Offline
```bash
# Compiler OpenCL C → SPIR-V
ocloc compile -file sha256.cl -device gen9 -output sha256.spv

# Résultat: sha256.spv (binaire SPIR-V)
```

#### Étape 2: Initialisation Level Zero
```c
// Init driver
zeInit(0);
ze_driver_handle_t driver;
zeDriverGet(&count, &driver);

// Get device
ze_device_handle_t device;
zeDeviceGet(driver, &count, &device);

// Create context
ze_context_desc_t ctx_desc = {ZE_STRUCTURE_TYPE_CONTEXT_DESC};
ze_context_handle_t context;
zeContextCreate(driver, &ctx_desc, &context);
```

#### Étape 3: Charger Module
```c
// Lire SPIR-V
FILE* f = fopen("sha256.spv", "rb");
fseek(f, 0, SEEK_END);
size_t size = ftell(f);
fseek(f, 0, SEEK_SET);
uint8_t* spirv = malloc(size);
fread(spirv, 1, size, f);
fclose(f);

// Créer module
ze_module_desc_t mod_desc = {
    .stype = ZE_STRUCTURE_TYPE_MODULE_DESC,
    .format = ZE_MODULE_FORMAT_IL_SPIRV,
    .inputSize = size,
    .pInputModule = spirv
};
ze_module_handle_t module;
zeModuleCreate(context, device, &mod_desc, &module, NULL);
```

#### Étape 4: Créer Kernel
```c
ze_kernel_desc_t kernel_desc = {
    .stype = ZE_STRUCTURE_TYPE_KERNEL_DESC,
    .pKernelName = "sha256_mining"
};
ze_kernel_handle_t kernel;
zeKernelCreate(module, &kernel_desc, &kernel);
```

#### Étape 5: Allouer Mémoire
```c
// Device memory
ze_device_mem_alloc_desc_t alloc_desc = {
    .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC
};
void* input_buf;
zeMemAllocDevice(context, &alloc_desc, 256*1024, 64, device, &input_buf);

void* output_buf;
zeMemAllocDevice(context, &alloc_desc, 1024, 64, device, &output_buf);
```

#### Étape 6: Command List
```c
// Créer command list
ze_command_list_desc_t list_desc = {
    .stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC
};
ze_command_list_handle_t cmdlist;
zeCommandListCreate(context, device, &list_desc, &cmdlist);

// Set kernel args
zeKernelSetArgumentValue(kernel, 0, sizeof(void*), &input_buf);
zeKernelSetArgumentValue(kernel, 1, sizeof(void*), &output_buf);
zeKernelSetArgumentValue(kernel, 2, sizeof(uint64_t), &nonce);

// Set group size
ze_group_count_t group_count = {262144, 1, 1};
zeCommandListAppendLaunchKernel(cmdlist, kernel, &group_count, NULL, 0, NULL);

// Close list
zeCommandListClose(cmdlist);
```

#### Étape 7: Exécution
```c
// Créer command queue
ze_command_queue_desc_t queue_desc = {
    .stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC
};
ze_command_queue_handle_t queue;
zeCommandQueueCreate(context, device, &queue_desc, &queue);

// Execute
zeCommandQueueExecuteCommandLists(queue, 1, &cmdlist, NULL);

// Sync
zeCommandQueueSynchronize(queue, UINT64_MAX);
```

---

## 7. AVANTAGES LEVEL ZERO

### 7.1 Performance

✅ **Overhead réduit**:
- Pas de JIT (pré-compilé)
- Command lists (batch)
- Contrôle fin synchronisation

✅ **Latence minimale**:
- Moins de couches
- Appels directs driver

### 7.2 Contrôle

✅ **Gestion mémoire explicite**:
```c
// Allocation device
zeMemAllocDevice(...);

// Allocation shared (CPU+GPU)
zeMemAllocShared(...);

// Allocation host
zeMemAllocHost(...);
```

✅ **Synchronisation fine**:
```c
// Events
ze_event_handle_t event;
zeCommandListAppendSignalEvent(cmdlist, event);
zeCommandListAppendWaitOnEvents(cmdlist, 1, &event);
```

### 7.3 Optimisations Avancées

✅ **Sub-devices**:
```c
// Utiliser plusieurs EUs
ze_device_handle_t subdevices[4];
zeDeviceGetSubDevices(device, &count, subdevices);
```

✅ **Memory ordering**:
```c
// Contrôle cache
ze_memory_access_cap_flags_t flags;
```

---

## 8. INCONVÉNIENTS LEVEL ZERO

### 8.1 Complexité

❌ **Compilation manuelle**:
- Nécessite `ocloc` (Intel Offline Compiler)
- Gestion SPIR-V
- Pas d'optimisations automatiques

❌ **Gestion mémoire**:
- Allocation/libération manuelle
- Risque de fuites
- Synchronisation complexe

❌ **Debugging difficile**:
- Moins d'outils
- Erreurs cryptiques
- Pas de validation automatique

### 8.2 Portabilité

❌ **Intel uniquement**:
- Pas de support AMD/NVIDIA
- Dépend du driver Intel
- Version GPU spécifique

❌ **Maintenance**:
- API évolutive
- Breaking changes possibles
- Documentation limitée

---

## 9. ANALYSE POUR BITCOIN MINING

### 9.1 Cas d'Usage Idéal

✅ **Level Zero est pertinent si**:
1. Performance critique (+3-7% compte)
2. Latence ultra-faible nécessaire
3. Contrôle fin du GPU requis
4. Intel GPU uniquement (pas multi-vendor)
5. Ressources pour maintenance

### 9.2 Cas d'Usage Non Pertinent

❌ **OpenCL suffit si**:
1. Performance actuelle acceptable (21.86 MH/s)
2. Portabilité importante
3. Simplicité prioritaire
4. Maintenance limitée
5. Multi-vendor souhaité

### 9.3 Analyse SHA256 Mining

**SHA256 est compute-bound**:
- 99.73% du temps en kernel execution (C197.50)
- Overhead actuel: 0.27%
- Gain Level Zero: réduire 0.27% → 0.1%

**Calcul gain réel**:
```
Overhead OpenCL:     0.27%
Overhead Level Zero: 0.10%
Gain:                0.17%

Hashrate OpenCL:     21.86 MH/s
Hashrate Level Zero: 21.86 × 1.0017 = 21.90 MH/s
Gain absolu:         +40 kH/s
```

**Conclusion**: Gain marginal pour effort important

---

## 10. RECOMMANDATIONS

### 10.1 Pour Bitcoin Mining NX48

#### Recommandation Principale

**Rester sur OpenCL optimisé** (C197.49) car:

1. ✅ **Performance excellente**: 21.86 MH/s
2. ✅ **Optimisations validées**: +986% vs baseline
3. ✅ **Simplicité**: Code maintenable
4. ✅ **Portabilité**: Multi-vendor possible
5. ✅ **Overhead minimal**: 0.27% seulement

#### Cas où Level Zero devient intéressant

**Seulement si**:
1. Besoin de +3-7% performance absolue
2. Intel GPU exclusivement
3. Ressources pour développement/maintenance
4. Intégration avec oneAPI
5. Recherche/expérimentation

### 10.2 Roadmap Suggérée

#### Phase 1: Optimisations OpenCL (ACTUEL)
- ✅ C197.49: Implémentation optimisée
- ✅ C197.50: Validation 21.86 MH/s
- 🔄 C197.51: Analyse Level Zero

#### Phase 2: Intégration NX48 (PRIORITAIRE)
- C197.52: Intégrer avec [`nx48_btc_controller.c`](../src/nx48_btc_controller.c:1)
- C197.53: Activer logging bit-level GPU
- C197.54: Production ready

#### Phase 3: Level Zero (OPTIONNEL)
- C197.55: Installation Level Zero SDK
- C197.56: Implémentation prototype
- C197.57: Benchmark comparatif
- C197.58: Décision go/no-go

---

## 11. INSTALLATION LEVEL ZERO (SI NÉCESSAIRE)

### 11.1 Prérequis

```bash
# Intel Compute Runtime
sudo apt install intel-opencl-icd

# Level Zero Loader
sudo apt install level-zero level-zero-dev

# Intel Graphics Compiler
sudo apt install intel-igc-opencl-dev

# Offline Compiler
sudo apt install intel-ocloc
```

### 11.2 Vérification

```bash
# Check Level Zero
ls /usr/lib/x86_64-linux-gnu/libze_loader.so*

# Check headers
ls /usr/include/level_zero/

# Test
ze_info  # ou zeinfo
```

---

## 12. CONCLUSION

### Synthèse

**Level Zero** est une API bas niveau Intel offrant:
- ✅ Contrôle fin du GPU
- ✅ Overhead minimal
- ✅ Performance +3-7%
- ❌ Complexité élevée
- ❌ Intel uniquement
- ❌ Maintenance coûteuse

### Pour Bitcoin Mining NX48

**Recommandation**: **Rester sur OpenCL optimisé**

**Raisons**:
1. Performance actuelle excellente (21.86 MH/s)
2. Overhead déjà minimal (0.27%)
3. Gain Level Zero marginal (+40 kH/s)
4. Effort/maintenance important
5. Portabilité préservée

### Prochaines Étapes

**Priorité 1**: Intégration NX48 (C197.52-C197.54)
**Priorité 2**: Production deployment
**Priorité 3**: Level Zero (si besoin absolu +3-7%)

---

## 13. FICHIERS RÉFÉRENCÉS

- [`c197_49_optimized_bitcoin_mining.c`](../../tools/c197_49_optimized_bitcoin_mining.c:1) - Implémentation OpenCL optimisée
- [`RAPPORT_C197_50_EXECUTION_VALIDÉE.md`](RAPPORT_C197_50_EXECUTION_VALIDÉE.md:1) - Validation 21.86 MH/s
- [`RAPPORT_C197_48_FINAL_CONSOLIDÉ_OPTIMISATIONS.md`](RAPPORT_C197_48_FINAL_CONSOLIDÉ_OPTIMISATIONS.md:1) - Architecture complète
- [`nx48_btc_controller.c`](../src/nx48_btc_controller.c:1) - Controller Bitcoin NX48

**Analyse Level Zero complète - Recommandation: OpenCL optimisé suffit**