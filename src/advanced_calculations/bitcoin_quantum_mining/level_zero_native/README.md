# LumVorax C198 — Level Zero Native Bitcoin Mining

**Version native Level Zero 100% indépendante d'OpenCL avec traçabilité bit-level totale**

---

## 🎯 Objectif

Développer une implémentation **native Level Zero** pour le Bitcoin mining qui :

1. **Élimine la dépendance OpenCL** (NEO runtime, IGC compiler)
2. **Réduit l'overhead** de 0.27% → 0.10% (-63%)
3. **Élimine le JIT** (1,183 ms → 0 ms avec pré-compilation)
4. **Augmente le hashrate** de 21.86 MH/s → 22.00+ MH/s (+0.64% minimum)
5. **Traçabilité bit-level totale** (RÈGLE #1 du prompt.txt)

## 📊 État Actuel

**Phase 1/5 COMPLÈTE** : Architecture de base ✅

- ✅ Initialisation Level Zero (drivers, devices, context, queue, list)
- ✅ Logging forensique bit-level (format .lum avec CRC32C)
- ✅ Timestamps nanoseconde (CLOCK_MONOTONIC)
- ✅ Magic numbers validation
- ✅ Test initialisation complet
- ⏳ Gestion mémoire (Phase 2)
- ⏳ Compilation kernel (Phase 3)
- ⏳ Exécution (Phase 4)
- ⏳ Mining Bitcoin (Phase 5)

**Métriques** :
- Fichiers créés : 6 (2,400 lignes)
- Fonctions implémentées : 8/21 (38%)
- Tests : 1/6 (17%)

## 🏗️ Architecture

```
level_zero_native/
├── include/
│   └── btc_levelzero_runner.h      # API complète (485 lignes)
├── src/
│   └── btc_levelzero_runner.c      # Implémentation (685 lignes)
├── tests/
│   ├── test_init.c                 # Test initialisation (145 lignes)
│   ├── test_memory.c               # À créer (Phase 2)
│   ├── test_kernel.c               # À créer (Phase 3)
│   ├── test_exec.c                 # À créer (Phase 4)
│   ├── test_mining.c               # À créer (Phase 5)
│   └── benchmark_vs_opencl.c       # À créer (Phase 6)
├── logs/
│   └── forensic/                   # Logs .lum
├── reports/
│   └── RAPPORT_C198_*.md           # Rapports forensiques
├── Makefile                        # Build system (235 lignes)
└── README.md                       # Ce fichier
```

## 🚀 Quick Start

### Prérequis

```bash
# Installation Level Zero SDK
sudo apt install level-zero-dev libz-dev

# Vérification
ls /usr/include/level_zero/ze_api.h
ldconfig -p | grep libze_loader
lspci | grep -i vga  # GPU Intel requis
```

### Compilation

```bash
# Build test initialisation
make all

# Build avec debug (AddressSanitizer)
make debug

# Build avec logging forensique complet
make forensic
```

### Exécution

```bash
# Test initialisation Level Zero
make test

# Vérifier dépendances
make check-deps

# Aide complète
make help
```

### Résultat Attendu

```
═══════════════════════════════════════════════════════════
  LumVorax C198 — Test Initialisation Level Zero
═══════════════════════════════════════════════════════════

[TEST 1/3] Initialisation Level Zero...
[C198-BIT-LEVEL] ========================================
[C198-BIT-LEVEL] btc_l0_init() START
[C198-BIT-LEVEL] Timestamp: 1715011234.123456789
[C198-BIT-LEVEL] Étape 1/6 : zeInit()...
[C198-BIT-LEVEL] Étape 2/6 : Énumération drivers...
[C198-BIT-LEVEL] 1 driver(s) trouvé(s)
[C198-BIT-LEVEL] Étape 3/6 : Énumération devices...
[C198-BIT-LEVEL] 1 device(s) trouvé(s)
[C198-BIT-LEVEL] Device: Intel(R) UHD Graphics 620 (ID=0x5917, CUs=24)
[C198-BIT-LEVEL] Mémoire: 7680 MB (max_alloc=1920 MB)
[C198-BIT-LEVEL] Étape 4/6 : Création contexte...
[C198-BIT-LEVEL] Étape 5/6 : Création command queue...
[C198-BIT-LEVEL] Étape 6/6 : Création command list...
[C198-BIT-LEVEL] Initialisation OK : 2345678 ns (2.346 ms)
✅ Initialisation réussie

[TEST 2/3] Validation contexte...
  ✅ Magic number valide: 0x4C5A4552
  ✅ Contexte initialisé
  ✅ Driver Level Zero: 0x7f1234567890
  ✅ Device Level Zero: 0x7f1234567898
  ✅ Contexte Level Zero: 0x7f12345678a0
  ✅ Command queue: 0x7f12345678a8
  ✅ Command list: 0x7f12345678b0

[TEST 3/3] Informations device...
  - Nom: Intel(R) UHD Graphics 620
  - Device ID: 0x00005917
  - Compute Units: 24
  - Mémoire globale: 7680 MB
  - Max allocation: 1920 MB

[FORENSIQUE] Métriques logging:
  - Événements totaux: 12
  - Événements perdus: 0
  - Fichier log: logs/forensic/test_init_c198.lum

[CLEANUP] Libération ressources...
✅ Cleanup terminé

═══════════════════════════════════════════════════════════
  RÉSULTAT : ✅ TOUS LES TESTS RÉUSSIS
═══════════════════════════════════════════════════════════
```

## 📋 API Principale

### Initialisation

```c
#include "btc_levelzero_runner.h"

// Configuration
btc_l0_config_t config = {
    .batch_size = 262144,
    .work_group_size = 256,
    .enable_profiling = true,
    .enable_forensic_log = true,
    .forensic_log_path = "logs/forensic/mining.lum",
    .timeout_ms = 30000
};

// Initialisation
btc_l0_context_t* ctx = btc_l0_init(&config);
if (!ctx) {
    fprintf(stderr, "Erreur initialisation\n");
    return 1;
}
```

### Gestion Mémoire (Phase 2 - À implémenter)

```c
// Allocation buffer GPU
btc_l0_buffer_t* buf = btc_l0_alloc_buffer(ctx, 1024, "test_buffer");

// Transfert Host → Device
uint8_t data[1024] = {0};
btc_l0_write_buffer(ctx, buf, data, 1024);

// Transfert Device → Host
btc_l0_read_buffer(ctx, buf, data, 1024);

// Libération
btc_l0_free_buffer(ctx, buf);
```

### Compilation Kernel (Phase 3 - À implémenter)

```c
// Compilation depuis SPIR-V ou OpenCL C
btc_l0_kernel_t* kernel = btc_l0_compile_kernel(
    ctx,
    kernel_source,
    source_size,
    "btc_sha256_double",
    "-cl-fast-relaxed-math"
);
```

### Exécution (Phase 4 - À implémenter)

```c
// Configuration arguments
btc_l0_set_kernel_arg(kernel, 0, sizeof(cl_mem), &buf_midstate);
btc_l0_set_kernel_arg(kernel, 1, sizeof(cl_mem), &buf_tail);
btc_l0_set_kernel_arg(kernel, 2, sizeof(uint32_t), &nonce_start);

// Exécution
size_t global_size = 262144;
size_t local_size = 256;
btc_l0_execute_kernel(ctx, kernel, global_size, &local_size);

// Synchronisation
btc_l0_sync(ctx);
```

### Mining Bitcoin (Phase 5 - À implémenter)

```c
// Mining haut niveau
btc_l0_result_t result;
int ret = btc_l0_mine_batch(
    ctx,
    midstate,      // SHA-256 midstate (32 bytes)
    tail,          // Tail du header (12 bytes)
    nonce_start,   // Nonce de départ
    262144,        // Batch size
    &result
);

if (ret == BTC_L0_OK) {
    printf("Hashrate: %.2f MH/s\n", result.hashrate_mhs);
    if (result.solution_found) {
        printf("Solution trouvée! Nonce: 0x%08x\n", result.nonce_found);
    }
}
```

### Cleanup

```c
// Libération ressources
btc_l0_cleanup(ctx);
```

## 🔬 Traçabilité Bit-Level

### Format .lum

Tous les événements sont loggés dans un fichier `.lum` avec :

- **Header 64 bytes** : `LUM-LEVELZERO-C198-FORENSIC-LOG`
- **Événements** : Structures `btc_l0_forensic_event_t` avec :
  - Timestamp nanoseconde (CLOCK_MONOTONIC)
  - Event ID unique
  - Thread ID
  - Type événement
  - Description
  - Données contextuelles (3× uint64_t)
  - CRC32C pour intégrité
- **Footer 32 bytes** : SHA-256 checksum

### Événements Loggés

**Initialisation** :
- `INIT_START`, `INIT_STEP`, `DEVICE_INFO`, `MEMORY_INFO`, `INIT_COMPLETE`

**Mémoire** (Phase 2) :
- `ALLOC_START`, `ALLOC_COMPLETE`, `TRANSFER_H2D_*`, `TRANSFER_D2H_*`, `FREE_BUFFER`

**Kernel** (Phase 3) :
- `COMPILE_START`, `COMPILE_COMPLETE`, `KERNEL_ARG_SET`

**Exécution** (Phase 4) :
- `EXEC_START`, `EXEC_COMPLETE`, `SYNC_START`, `SYNC_COMPLETE`

**Mining** (Phase 5) :
- `MINE_BATCH_START`, `MINE_BATCH_COMPLETE`

**Cleanup** :
- `CLEANUP_START`, `CLEANUP_STEP`, `CLEANUP_COMPLETE`

## 📈 Performance Attendue

### Baseline OpenCL (C197.50)

```
Hashrate:           21.86 MH/s
Overhead:           0.27%
JIT compile:        1,183 ms (une fois)
Kernel execution:   99.73%
```

### Cible Level Zero (C198)

```
Hashrate:           22.00 MH/s (+0.64%)
Overhead:           0.10% (-63%)
JIT compile:        0 ms (pré-compilé)
Kernel execution:   99.90%
```

**Gain minimum** : +0.17% (40 kH/s)  
**Gain cible** : +10% (2.4 MH/s)

## 📚 Documentation

### Rapports

- [`RAPPORT_C198_ARCHITECTURE_LEVELZERO_NATIVE.md`](reports/RAPPORT_C198_ARCHITECTURE_LEVELZERO_NATIVE.md:1) — Architecture complète

### Références

- [Level Zero Specification](https://spec.oneapi.io/level-zero/latest/)
- [Intel Compute Runtime](https://github.com/intel/compute-runtime)
- [RAPPORT_SYNTHESE_FINALE_C197_17_51.md](../CHAT/RAPPORT_SYNTHESE_FINALE_C197_17_51.md:1) — OpenCL optimisé

## 🛠️ Développement

### Phases

1. ✅ **Phase 1** : Architecture de base (COMPLÈTE)
2. ⏳ **Phase 2** : Gestion mémoire
3. ⏳ **Phase 3** : Compilation kernel
4. ⏳ **Phase 4** : Exécution
5. ⏳ **Phase 5** : Mining Bitcoin
6. ⏳ **Phase 6** : Optimisations

### Prochaines Étapes

1. Implémenter `btc_l0_alloc_buffer()` avec `zeMemAllocDevice()`
2. Implémenter `btc_l0_write_buffer()` / `btc_l0_read_buffer()`
3. Créer kernel SHA-256 SPIR-V
4. Implémenter `btc_l0_compile_kernel()` avec `zeModuleCreate()`
5. Implémenter `btc_l0_execute_kernel()` avec `zeCommandListAppendLaunchKernel()`
6. Implémenter `btc_l0_mine_batch()` complet
7. Benchmark vs OpenCL

### Contribution

Ce module fait partie du projet LumVorax et suit les règles strictes du [`prompt.txt`](../../../../prompt.txt:1) :

- **RÈGLE #1** : Traçabilité bit-level totale (priorité absolue)
- **RÈGLE #2** : Lecture complète de STANDARD_NAMES.md avant modification
- **RÈGLE #3** : Validation compilation 0 erreur 0 warning
- **RÈGLE #4** : Tests progressifs 10-100K éléments
- **RÈGLE #5** : Mise à jour STANDARD_NAMES.md après chaque modification

## 📞 Support

Pour toute question ou problème :

1. Consulter [`RAPPORT_C198_ARCHITECTURE_LEVELZERO_NATIVE.md`](reports/RAPPORT_C198_ARCHITECTURE_LEVELZERO_NATIVE.md:1)
2. Vérifier les logs forensiques dans `logs/forensic/`
3. Exécuter `make check-deps` pour valider l'environnement
4. Consulter la documentation Level Zero officielle

---

**LumVorax C198 — Level Zero Native Bitcoin Mining**  
*Made with Bob - Version native avec traçabilité bit-level totale*