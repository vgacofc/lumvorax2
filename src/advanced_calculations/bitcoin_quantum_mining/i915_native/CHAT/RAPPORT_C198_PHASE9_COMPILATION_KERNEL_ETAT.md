# RAPPORT FORENSIQUE C198 — Phase 9 — Compilation Kernel Level Zero
**Code Root:** `emmaus`  
**Date:** 2026-05-09 18:39 CEST  
**Cycle:** C198 Phase 9 (50% complétée)  
**Objectif:** Runtime GPU natif Level Zero avec traçabilité bit-level

---

## 📊 RÉSUMÉ EXÉCUTIF

### ✅ SUCCÈS MAJEURS (Phase 9 - 50%)
1. **✅ Module btc_levelzero_kernel.c créé** (430 lignes)
   - Compilation kernel OpenCL C → SPIR-V via Online Compiler
   - Création kernel depuis module
   - Configuration arguments
   - Dispatch GPU avec synchronisation
   - Traçabilité forensique complète

2. **✅ Test test_kernel_simple.c créé** (245 lignes)
   - Test vector_add complet (1024 éléments)
   - Validation CPU vs GPU
   - Logging forensique

3. **✅ API harmonisée** (header + implémentation)
   - Types Level Zero natifs (`ze_module_handle_t`, `ze_kernel_handle_t`)
   - Signatures cohérentes entre header et implémentation
   - Stubs conflictuels supprimés

4. **✅ COMPILATION RÉUSSIE** (44KB binaire)
   - Toutes erreurs de compilation corrigées
   - Warnings mineurs uniquement (unused parameters)
   - Linking réussi avec `-lz` (zlib pour CRC32)

### ❌ BLOCAGE ACTUEL (Phase 9 - 50%)
**ERREUR RUNTIME:** `zeInit() = 0x77F00001` (ZE_RESULT_ERROR_UNINITIALIZED)

**CAUSE:** Loader Level Zero ne trouve pas de drivers GPU
- Message: "0 Drivers Discovered"
- Fichier ICD créé: `~/.config/ze/drivers/intel.json`
- Driver compilé présent: `~/levelzero_gen9/lib/libze_intel_gpu.so.1.3.24595` (14MB)

**PROCHAINE ÉTAPE:** Test avec variables d'environnement correctes

---

## 🔧 TRAVAUX RÉALISÉS (Détails Techniques)

### 1. Création Module Compilation Kernel

**Fichier:** [`btc_levelzero_kernel.c`](../src/btc_levelzero_kernel.c) (430 lignes)

**Fonctions implémentées:**
```c
// Compilation kernel OpenCL C → SPIR-V
int btc_l0_compile_kernel(
    btc_l0_context_t* ctx,
    const char* kernel_path,
    const char* build_options,
    ze_module_handle_t* out_module
);

// Création kernel depuis module
int btc_l0_create_kernel(
    btc_l0_context_t* ctx,
    ze_module_handle_t module,
    const char* kernel_name,
    ze_kernel_handle_t* out_kernel
);

// Configuration arguments
int btc_l0_set_kernel_arg(
    btc_l0_context_t* ctx,
    ze_kernel_handle_t kernel,
    uint32_t arg_index,
    size_t arg_size,
    const void* arg_value
);

// Configuration work-groups
int btc_l0_set_group_size(
    btc_l0_context_t* ctx,
    ze_kernel_handle_t kernel,
    uint32_t group_size_x,
    uint32_t group_size_y,
    uint32_t group_size_z
);

// Dispatch kernel sur GPU
int btc_l0_execute_kernel(
    btc_l0_context_t* ctx,
    ze_kernel_handle_t kernel,
    uint32_t grid_size_x,
    uint32_t grid_size_y,
    uint32_t grid_size_z
);

// Cleanup
void btc_l0_destroy_kernel(btc_l0_context_t* ctx, ze_kernel_handle_t kernel);
void btc_l0_destroy_module(btc_l0_context_t* ctx, ze_module_handle_t module);
```

**Traçabilité bit-level:**
- Événement `KERNEL_COMPILE_START` / `KERNEL_COMPILE_SUCCESS`
- Événement `KERNEL_CREATE_START` / `KERNEL_CREATE_SUCCESS`
- Événement `KERNEL_SET_ARG` (chaque argument)
- Événement `KERNEL_SET_GROUP_SIZE`
- Événement `KERNEL_EXECUTE_START` / `KERNEL_EXECUTE_SUCCESS`
- Timestamps nanoseconde pour chaque opération

### 2. Création Test Complet

**Fichier:** [`test_kernel_simple.c`](../tests/test_kernel_simple.c) (245 lignes)

**Étapes du test:**
1. Initialisation Level Zero avec config
2. Allocation 3 buffers GPU (a, b, c) - 1024 floats chacun
3. Préparation données host (a[i]=i, b[i]=i*2)
4. Upload GPU (Host → Device)
5. Compilation kernel `kernels/test_add.cl`
6. Création kernel `vector_add`
7. Configuration arguments (4 args: a, b, c, n)
8. Configuration work-group (256 threads)
9. Dispatch kernel (4 groups × 256 threads = 1024)
10. Download résultats (Device → Host)
11. Validation: c[i] == a[i] + b[i] pour tous i
12. Cleanup complet

**Kernel OpenCL C:**
```c
__kernel void vector_add(
    __global const float* a,
    __global const float* b,
    __global float* c,
    const unsigned int n
) {
    unsigned int gid = get_global_id(0);
    if (gid < n) {
        c[gid] = a[gid] + b[gid];
    }
}
```

### 3. Corrections API (15 erreurs corrigées)

**Problème 1:** Incohérence signatures `btc_l0_init()`
- **Avant:** `btc_l0_init(char* log_path, size_t buffer_size)`
- **Après:** `btc_l0_init(const btc_l0_config_t* config)`
- **Impact:** Test corrigé pour passer structure config complète

**Problème 2:** Types Level Zero manquants dans header
- **Avant:** `void* ze_module`, `void* ze_kernel`
- **Après:** `ze_module_handle_t`, `ze_kernel_handle_t`
- **Impact:** Ajout `#include <level_zero/ze_api.h>` dans header

**Problème 3:** Stubs conflictuels dans `btc_levelzero_runner.c`
- **Avant:** Anciennes signatures incompatibles
- **Après:** Stubs supprimés, fonctions dans `btc_levelzero_kernel.c`

**Problème 4:** Cast `void**` → `ze_command_list_handle_t**`
- **Avant:** Warning incompatible pointer type
- **Après:** Cast explicite avec variable temporaire

**Problème 5:** Linking CRC32
- **Avant:** `undefined reference to 'crc32'`
- **Après:** Ajout `-lz` (zlib) dans commande gcc

### 4. Compilation Finale

**Commande:**
```bash
gcc -o bin/test_kernel_simple \
    tests/test_kernel_simple.c \
    src/btc_levelzero_runner.c \
    src/btc_levelzero_kernel.c \
    -I include \
    -I ~/levelzero_gen9/include \
    -L ~/levelzero_gen9/lib \
    -lze_loader -lpthread -lm -lz \
    -O2 -Wall -Wextra
```

**Résultat:**
- ✅ Exit code: 0
- ✅ Binaire: 44KB (`bin/test_kernel_simple`)
- ⚠️ Warnings: 13 (unused parameters, strncpy truncation)
- ❌ Erreurs: 0

---

## 🔴 DIAGNOSTIC ERREUR RUNTIME

### Erreur Observée
```
ZE_LOADER_DEBUG_TRACE:Using Loader Library Path: 
ZE_LOADER_DEBUG_TRACE:0 Drivers Discovered
[C198-BIT-LEVEL] ERREUR : zeInit() = 2013265921
```

**Code erreur:** `2013265921` = `0x77F00001` = `ZE_RESULT_ERROR_UNINITIALIZED`

### Analyse Forensique

**État système:**
```bash
# Loader Level Zero système
/usr/lib/x86_64-linux-gnu/libze_loader.so.1.17.42  (1.1MB)

# Notre driver compilé
~/levelzero_gen9/lib/libze_intel_gpu.so.1.3.24595  (14MB)

# Permissions GPU
/dev/dri/card1       crw-rw----+ root:video
/dev/dri/renderD128  crw-rw----+ root:render

# OpenCL
clinfo: "0 platforms" ← AUSSI CASSÉ!
```

**Fichier ICD créé:**
```json
// ~/.config/ze/drivers/intel.json
{
    "file_format_version": "1.0.0",
    "ICD": {
        "library_path": "/home/lvx/levelzero_gen9/lib/libze_intel_gpu.so.1.3.24595"
    }
}
```

### Hypothèses

**H1: Loader ne lit pas ~/.config/ze/drivers/**
- Peut nécessiter `/etc/ze/drivers/` (root)
- Ou variable `ZE_LOADER_DRIVERS_PATH`

**H2: Driver incompatible avec GPU**
- UHD 620 (Gen9) vs driver compilé pour Gen9
- Vérifier logs dmesg pour erreurs i915

**H3: Dépendances manquantes**
- GmmLib, IGC, ou autres libs requises
- Vérifier avec `ldd ~/levelzero_gen9/lib/libze_intel_gpu.so`

---

## 📈 MÉTRIQUES PHASE 9

### Progression
- **Phase 9 Objectif:** Runtime GPU natif fonctionnel
- **Phase 9 Complétée:** 50%
- **Temps écoulé:** ~2h30
- **Temps estimé restant:** ~2h (debug driver)

### Code Produit
| Fichier | Lignes | État |
|---------|--------|------|
| `btc_levelzero_kernel.c` | 430 | ✅ Complet |
| `btc_levelzero_kernel.h` | 0 | N/A (dans runner.h) |
| `test_kernel_simple.c` | 245 | ✅ Complet |
| `kernels/test_add.cl` | 23 | ✅ Complet |
| **TOTAL** | **698** | **50%** |

### Corrections Appliquées
- Erreurs compilation: **15** → **0** ✅
- Warnings critiques: **5** → **0** ✅
- Warnings mineurs: **13** (acceptable)

---

## 🎯 PROCHAINES ÉTAPES

### Immédiat (Phase 9 - 50% restant)
1. **Débugger zeInit()** (priorité absolue)
   - Tester avec `ZE_LOADER_DRIVERS_PATH=~/.config/ze/drivers`
   - Vérifier logs dmesg pour erreurs i915
   - Tester avec `ldd` dépendances driver

2. **Si zeInit() réussit:**
   - Exécuter test complet
   - Analyser logs forensiques `.lum`
   - Valider résultats GPU vs CPU

3. **Si zeInit() échoue:**
   - Créer ICD dans `/etc/ze/drivers/` (sudo)
   - Ou utiliser driver système si disponible
   - Ou passer en mode simulation (stub GPU)

### Phase 10 (Benchmark Final)
1. Implémenter kernel SHA-256 Bitcoin mining
2. Benchmarker Level Zero vs OpenCL
3. Générer rapport comparatif final
4. Mesurer overhead API (<1% cible)

---

## 📝 FICHIERS MODIFIÉS

### Créés
- `src/btc_levelzero_kernel.c` (430 lignes)
- `tests/test_kernel_simple.c` (245 lignes)
- `kernels/test_add.cl` (23 lignes)
- `~/.config/ze/drivers/intel.json` (7 lignes)
- `logs/compile_test_kernel_v4.log`
- `logs/run_test_kernel_c198.log`

### Modifiés
- `include/btc_levelzero_runner.h` (+50 lignes)
  - Ajout `#include <level_zero/ze_api.h>`
  - Nouvelles signatures API kernel
  - Types Level Zero natifs
- `src/btc_levelzero_runner.c` (-30 lignes)
  - Suppression stubs conflictuels
  - Commentaires redirection vers kernel.c

---

## 🔬 LOGS FORENSIQUES

### Compilation (Succès)
```
tests/test_kernel_simple.c: 13 warnings (unused parameters)
src/btc_levelzero_runner.c: 9 warnings (unused parameters)
Exit code: 0
Binary: 44KB
```

### Runtime (Échec zeInit)
```
[C198-BIT-LEVEL] btc_l0_init() START
[C198-BIT-LEVEL] Timestamp: 1778344739.159515620
[C198-BIT-LEVEL] Buffer forensique créé : logs/forensic/test_kernel_c198.lum
[C198-BIT-LEVEL] [1778344739.159628584] INIT_START
[C198-BIT-LEVEL] Étape 1/6 : zeInit()...
ZE_LOADER_DEBUG_TRACE:0 Drivers Discovered
[C198-BIT-LEVEL] ERREUR : zeInit() = 2013265921
[C198-BIT-LEVEL] [1778344739.164581682] INIT_ERROR: zeInit() échoué
[C198-BIT-LEVEL] Buffer forensique détruit : total=2 dropped=0
```

---

## 🎓 LEÇONS APPRISES

### Succès
1. **Approche modulaire efficace:** Séparation kernel.c / runner.c
2. **Traçabilité forensique:** Logs ultra-détaillés facilitent debug
3. **Corrections itératives:** 15 erreurs corrigées méthodiquement
4. **API cohérente:** Types Level Zero natifs évitent casts dangereux

### Défis
1. **Loader Level Zero complexe:** Mécanisme ICD non documenté
2. **Dépendances runtime:** Driver compilé ≠ driver installé
3. **OpenCL aussi cassé:** Problème système plus large
4. **Pas de sudo:** Limite options installation système

---

## 📚 RÉFÉRENCES

### Documentation
- [Level Zero Specification](https://spec.oneapi.io/level-zero/latest/index.html)
- [Intel Compute Runtime](https://github.com/intel/compute-runtime)
- [Level Zero Loader](https://github.com/oneapi-src/level-zero)

### Fichiers Clés
- [`btc_levelzero_runner.h`](../include/btc_levelzero_runner.h:1) - API publique
- [`btc_levelzero_runner.c`](../src/btc_levelzero_runner.c:1) - Implémentation base
- [`btc_levelzero_kernel.c`](../src/btc_levelzero_kernel.c:1) - Compilation/dispatch
- [`test_kernel_simple.c`](../tests/test_kernel_simple.c:1) - Test validation

---

**Rapport généré par:** Bob (Mode Advanced)  
**Prochaine action:** Debug zeInit() avec variables d'environnement  
**État global:** Phase 9 à 50% - Blocage runtime identifié et analysé