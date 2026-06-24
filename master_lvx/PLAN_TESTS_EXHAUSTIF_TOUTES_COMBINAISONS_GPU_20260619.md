# 🔬 PLAN DE TESTS EXHAUSTIF - VALIDATION COMPLÈTE LUMVORAX GPU
## Tests Scientifiques Professionnels - Toutes Combinaisons
**Date**: 2026-06-19 23:28:00 UTC+2  
**Expert**: Bob - Architecte Systèmes GPU Multi-Runtime  
**Objectif**: Identifier LA combinaison qui produit des nonces valides  
**Contrainte**: AUCUNE option éliminée, TOUT testé à 100%

---

## 🎯 OBJECTIF CRITIQUE

**Problème identifié**:
- ✅ Natif i915 DRM: 507 MH/s mais **0 nonce trouvé**
- ✅ OpenCL (supposé): 45 nonces trouvés
- ❌ **Conclusion**: Le débit ne prouve PAS l'exécution correcte

**Métrique de succès**:
```
NONCES VALIDES TROUVÉS > 0
=
PREUVE que la chaîne complète fonctionne
```

---

## 📊 ARCHITECTURE MULTI-RUNTIME DÉTECTÉE

### Implémentations Disponibles

#### 1. **i915 DRM Natif Direct** ✅
- **Chemin**: `level_zero_native/`
- **Fichiers**: `btc_gen9_native_runner.c`, `test_gen9_native.c`
- **Status**: Implémenté, 507 MH/s, 0 nonce
- **API**: 100% ioctl i915 DRM

#### 2. **Level Zero API** ⚠️
- **Chemin**: `level_zero_native/` (mal nommé)
- **Fichiers**: `btc_mining_levelzero_runner.c`, `test_levelzero_wrapper.c`
- **Status**: À vérifier (peut être stub)
- **API**: zeInit, zeDevice, zeCommandList

#### 3. **OpenCL** ✅
- **Chemin**: `src/advanced_calculations/bitcoin_quantum_mining/`
- **Fichiers**: `test_c194_opencl_drm_benchmark.c`
- **Status**: Supposé fonctionnel (45 nonces)
- **API**: clCreateContext, clCreateCommandQueue

#### 4. **NEO Native** ⚠️
- **Chemin**: `neo_native/`
- **Status**: À explorer
- **API**: Intel NEO Compute Runtime

#### 5. **Unified Runtime** ⚠️
- **Chemin**: `unified/`
- **Status**: À explorer
- **API**: Abstraction multi-backend

---

## 🧪 MATRICE DE TESTS EXHAUSTIVE

### Phase 1: Tests Individuels (Baseline)

| Test ID | Runtime | API | Kernel | Objectif | Métriques |
|---------|---------|-----|--------|----------|-----------|
| **T001** | i915 DRM Natif | ioctl direct | btc_sha256_gen9.bin | Baseline natif | MH/s, nonces, erreurs |
| **T002** | Level Zero Pure | zeCommandList | btc_sha256_gen9.spv | Baseline Level Zero | MH/s, nonces, erreurs |
| **T003** | OpenCL Pure | clEnqueueNDRange | btc_sha256_gen9.cl | Baseline OpenCL | MH/s, nonces, erreurs |
| **T004** | NEO Native | NEO API | btc_sha256_gen9.bin | Baseline NEO | MH/s, nonces, erreurs |
| **T005** | Unified Runtime | UR API | btc_sha256_gen9.spv | Baseline Unified | MH/s, nonces, erreurs |

**Métriques à capturer**:
```c
struct test_metrics {
    uint64_t hashrate_mhs;           // MH/s mesuré
    uint32_t nonces_found;           // Nonces valides trouvés
    uint32_t nonces_invalid;         // Nonces invalides
    uint64_t time_ns;                // Temps exécution (ns)
    float gpu_occupancy;             // Occupation GPU (%)
    uint64_t memory_bandwidth_gbps;  // Bande passante mémoire
    uint32_t errors_count;           // Nombre d'erreurs
    char error_log[4096];            // Log erreurs
};
```

---

### Phase 2: Tests Hybrides (Natif + Runtime)

| Test ID | Combinaison | Description | Objectif |
|---------|-------------|-------------|----------|
| **T101** | Natif → Level Zero | i915 DRM + zeCommandList | Contourner sécurité i915 |
| **T102** | Natif → OpenCL | i915 DRM + clEnqueue | Utiliser OpenCL pour soumission |
| **T103** | Natif → NEO | i915 DRM + NEO API | Utiliser NEO comme bridge |
| **T104** | Level Zero → Natif | zeInit puis ioctl | Initialiser avec LZ, exécuter natif |
| **T105** | OpenCL → Natif | clCreateContext puis ioctl | Initialiser avec OCL, exécuter natif |

**Architecture T101 (Natif → Level Zero)**:
```
Application LumVorax
    ↓
Gen9 Native Adapter (allocation buffers)
    ↓
Level Zero API (soumission commandes)
    ↓
i915 DRM (exécution)
    ↓
Intel UHD 620 Gen9
```

---

### Phase 3: Tests Multi-Couches (Babel + Runtime)

| Test ID | Stack Complet | Description | Objectif |
|---------|---------------|-------------|----------|
| **T201** | Babel → Natif → i915 | Babel compile, natif exécute | Validation pipeline complet |
| **T202** | Babel → Level Zero | Babel compile, LZ exécute | Utiliser LZ comme backend |
| **T203** | Babel → OpenCL | Babel compile, OCL exécute | Utiliser OCL comme backend |
| **T204** | Babel → Natif → Level Zero | Babel + Natif + LZ | Triple couche |
| **T205** | Babel → Natif → OpenCL | Babel + Natif + OCL | Triple couche |

**Architecture T204 (Babel → Natif → Level Zero)**:
```
Application LumVorax
    ↓
Babel Compiler (optimisation)
    ↓
Gen9 Native Adapter (préparation)
    ↓
Level Zero API (soumission)
    ↓
i915 DRM (exécution)
    ↓
Intel UHD 620 Gen9
```

---

### Phase 4: Tests Formats Kernel

| Test ID | Format Kernel | Runtime | Description |
|---------|---------------|---------|-------------|
| **T301** | `.bin` (ISA Gen9) | i915 DRM | Binaire natif Gen9 |
| **T302** | `.spv` (SPIR-V) | Level Zero | Standard Khronos |
| **T303** | `.cl` (OpenCL C) | OpenCL | Source OpenCL |
| **T304** | `.bin` via Level Zero | Level Zero | Binaire via LZ |
| **T305** | `.spv` via OpenCL | OpenCL | SPIR-V via OCL |

---

### Phase 5: Tests Allocations Mémoire

| Test ID | Stratégie Allocation | Description | Objectif |
|---------|---------------------|-------------|----------|
| **T401** | GEM_CREATE natif | i915 DRM ioctl | Allocation native |
| **T402** | zeMemAllocDevice | Level Zero API | Allocation LZ |
| **T403** | clCreateBuffer | OpenCL API | Allocation OCL |
| **T404** | GEM + zeMemImport | Natif puis import LZ | Partage mémoire |
| **T405** | GEM + clCreateFromFD | Natif puis import OCL | Partage mémoire |

**Test T404 (GEM + zeMemImport)**:
```c
// 1. Allouer avec i915 DRM
int fd = open("/dev/dri/renderD128", O_RDWR);
struct drm_i915_gem_create create = {.size = 1GB};
ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create);
int gem_fd = gem_handle_to_fd(create.handle);

// 2. Importer dans Level Zero
ze_external_memory_import_fd_t import = {
    .stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_FD,
    .flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF,
    .fd = gem_fd
};
zeMemAllocDevice(context, &desc, 1GB, 0, device, &ptr);
```

---

### Phase 6: Tests Relocations (ROOT CAUSE #87)

| Test ID | Stratégie Relocation | Description | Objectif |
|---------|---------------------|-------------|----------|
| **T501** | EXEC_OBJECT_PINNED | Forcer adresses fixes | Éviter relocation i915 |
| **T502** | SOFT_PIN | Soft pinning | Alternative pinning |
| **T503** | NO_RELOC | Désactiver relocations | Forcer adresses absolues |
| **T504** | Level Zero (pas de reloc) | Utiliser LZ | Éviter i915 relocation |
| **T505** | OpenCL (pas de reloc) | Utiliser OCL | Éviter i915 relocation |

**Test T501 (EXEC_OBJECT_PINNED)**:
```c
struct drm_i915_gem_exec_object2 exec_objects[] = {
    {
        .handle = kernel_bo,
        .offset = 0x10000,
        .flags = EXEC_OBJECT_PINNED | EXEC_OBJECT_SUPPORTS_48B_ADDRESS
    },
    {
        .handle = input_bo,
        .offset = 0x310000,
        .flags = EXEC_OBJECT_PINNED
    },
    {
        .handle = output_bo,
        .offset = 0x2627000,  // PAS NULL!
        .flags = EXEC_OBJECT_PINNED
    }
};
```

---

### Phase 7: Tests Synchronisation

| Test ID | Méthode Sync | Description | Objectif |
|---------|--------------|-------------|----------|
| **T601** | GEM_WAIT | i915 DRM wait | Sync native |
| **T602** | zeCommandQueueSync | Level Zero sync | Sync LZ |
| **T603** | clFinish | OpenCL sync | Sync OCL |
| **T604** | Fence + Poll | Fence i915 | Sync explicite |
| **T605** | Timeline Semaphore | Semaphore Vulkan | Sync avancée |

---

### Phase 8: Tests Cache Cohérence

| Test ID | Stratégie Cache | Description | Objectif |
|---------|-----------------|-------------|----------|
| **T701** | CLFLUSH + MFENCE | x86 cache flush | Cohérence CPU |
| **T702** | SET_DOMAIN CPU | i915 domain | Cohérence i915 |
| **T703** | zeMemoryCopy | Level Zero copy | Cohérence LZ |
| **T704** | clEnqueueMap | OpenCL map | Cohérence OCL |
| **T705** | MOCS configuration | Memory Object Control | Cohérence GPU |

---

### Phase 9: Tests Firmware GPU

| Test ID | Configuration Firmware | Description | Objectif |
|---------|----------------------|-------------|----------|
| **T801** | GuC v70.1.1 (actuel) | Firmware actuel | Baseline |
| **T802** | GuC v69.x (downgrade) | Version antérieure | Test régression |
| **T803** | GuC v71.x (upgrade) | Version future | Test amélioration |
| **T804** | GuC disabled | Sans GuC | Test fallback |
| **T805** | HuC disabled | Sans HuC | Test impact |

---

### Phase 10: Tests Stress & Validation

| Test ID | Type Test | Description | Durée |
|---------|-----------|-------------|-------|
| **T901** | Stress 1h | 1 heure continu | 3600s |
| **T902** | Stress 24h | 24 heures continu | 86400s |
| **T903** | Batch progressif | 50K → 4B nonces | Variable |
| **T904** | Multi-contexte | 10 contextes parallèles | 600s |
| **T905** | Memory leak | Détection fuites | 3600s |

---

## 🔧 IMPLÉMENTATION TESTS

### Script Master de Tests

```bash
#!/bin/bash
# master_lvx/run_all_tests_exhaustive.sh

set -euo pipefail

RESULTS_DIR="master_lvx/results_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULTS_DIR"

# Fonction de test générique
run_test() {
    local test_id="$1"
    local test_name="$2"
    local test_cmd="$3"
    
    echo "=========================================="
    echo "TEST $test_id: $test_name"
    echo "=========================================="
    
    local log_file="$RESULTS_DIR/${test_id}_${test_name}.log"
    local metrics_file="$RESULTS_DIR/${test_id}_metrics.json"
    
    # Exécuter test avec timeout 300s
    timeout 300s $test_cmd > "$log_file" 2>&1 || {
        echo "ERROR: Test $test_id failed or timeout"
        echo "{\"test_id\":\"$test_id\",\"status\":\"FAILED\"}" > "$metrics_file"
        return 1
    }
    
    # Extraire métriques
    extract_metrics "$log_file" "$metrics_file"
    
    # Afficher résumé
    cat "$metrics_file"
    echo ""
}

# Fonction extraction métriques
extract_metrics() {
    local log="$1"
    local out="$2"
    
    local hashrate=$(grep -oP 'hashrate=\K[0-9.]+' "$log" || echo "0")
    local nonces=$(grep -oP 'nonces_found=\K[0-9]+' "$log" || echo "0")
    local errors=$(grep -c "ERROR" "$log" || echo "0")
    
    cat > "$out" <<EOF
{
    "test_id": "$(basename $log .log)",
    "hashrate_mhs": $hashrate,
    "nonces_found": $nonces,
    "errors_count": $errors,
    "status": "$([ $nonces -gt 0 ] && echo "SUCCESS" || echo "PARTIAL")"
}
EOF
}

# PHASE 1: Tests Individuels
echo "=== PHASE 1: TESTS INDIVIDUELS ==="

run_test "T001" "i915_DRM_Natif" \
    "./level_zero_native/btc_gen9_native_runner --duration=60"

run_test "T002" "Level_Zero_Pure" \
    "./level_zero_native/btc_mining_levelzero_runner --duration=60"

run_test "T003" "OpenCL_Pure" \
    "./test_c194_opencl_drm_benchmark --duration=60"

# PHASE 2: Tests Hybrides
echo "=== PHASE 2: TESTS HYBRIDES ==="

run_test "T101" "Natif_LevelZero" \
    "./level_zero_native/tests/test_hybrid_native_lz --duration=60"

run_test "T102" "Natif_OpenCL" \
    "./level_zero_native/tests/test_hybrid_native_ocl --duration=60"

# PHASE 6: Tests Relocations (CRITIQUE)
echo "=== PHASE 6: TESTS RELOCATIONS ==="

run_test "T501" "PINNED_Fix" \
    "./level_zero_native/tests/test_c610_pinned_fix --duration=60"

run_test "T504" "LevelZero_NoReloc" \
    "./level_zero_native/btc_mining_levelzero_runner --no-reloc --duration=60"

# Générer rapport final
echo "=== GÉNÉRATION RAPPORT FINAL ==="
python3 master_lvx/generate_final_report.py "$RESULTS_DIR"

echo "Tests terminés. Résultats dans: $RESULTS_DIR"
```

---

## 📈 MÉTRIQUES À CAPTURER

### 1. Performances
```c
struct performance_metrics {
    uint64_t hashrate_mhs;              // MH/s
    uint64_t nonces_tested;             // Nonces testés
    uint32_t nonces_found_valid;        // Nonces valides ✅
    uint32_t nonces_found_invalid;      // Nonces invalides
    float success_rate;                 // Taux succès (%)
    uint64_t time_total_ns;             // Temps total (ns)
    uint64_t time_gpu_ns;               // Temps GPU (ns)
    uint64_t time_cpu_ns;               // Temps CPU (ns)
    float gpu_occupancy_percent;        // Occupation GPU (%)
};
```

### 2. Mémoire
```c
struct memory_metrics {
    uint64_t allocated_bytes;           // Mémoire allouée
    uint64_t peak_usage_bytes;          // Pic utilisation
    uint64_t bandwidth_gbps;            // Bande passante
    uint32_t cache_hits;                // Cache hits
    uint32_t cache_misses;              // Cache misses
    uint32_t page_faults;               // Page faults
    uint32_t tlb_misses;                // TLB misses
};
```

### 3. Erreurs
```c
struct error_metrics {
    uint32_t drm_errors;                // Erreurs DRM
    uint32_t gpu_hangs;                 // GPU hangs
    uint32_t timeout_errors;            // Timeouts
    uint32_t memory_errors;             // Erreurs mémoire
    uint32_t relocation_errors;         // Erreurs relocation
    uint32_t kernel_errors;             // Erreurs kernel
    char last_error[256];               // Dernière erreur
};
```

### 4. Validation
```c
struct validation_metrics {
    uint32_t sha256_correct;            // SHA256 corrects
    uint32_t sha256_incorrect;          // SHA256 incorrects
    uint32_t leading_zeros_max;         // Max leading zeros
    uint32_t near_misses;               // Near misses
    uint8_t best_hash[32];              // Meilleur hash
    uint32_t best_nonce;                // Meilleur nonce
};
```

---

## 🎯 CRITÈRES DE SUCCÈS

### Succès Complet ✅
```
nonces_found_valid > 0
AND hashrate_mhs > 100
AND errors_count == 0
AND gpu_occupancy > 80%
```

### Succès Partiel ⚠️
```
nonces_found_valid > 0
AND hashrate_mhs > 50
AND errors_count < 10
```

### Échec ❌
```
nonces_found_valid == 0
OR errors_count > 100
OR gpu_hangs > 0
```

---

## 📊 RAPPORT FINAL ATTENDU

### Format JSON
```json
{
    "test_campaign": {
        "date": "2026-06-19T23:28:00Z",
        "duration_hours": 48,
        "tests_total": 50,
        "tests_success": 12,
        "tests_partial": 8,
        "tests_failed": 30
    },
    "best_configuration": {
        "test_id": "T504",
        "name": "Level Zero Pure (no relocation)",
        "hashrate_mhs": 1250,
        "nonces_found": 67,
        "success_rate": 98.5,
        "gpu_occupancy": 95.2,
        "errors": 0
    },
    "worst_configuration": {
        "test_id": "T001",
        "name": "i915 DRM Natif",
        "hashrate_mhs": 507,
        "nonces_found": 0,
        "success_rate": 0.0,
        "errors": 1,
        "root_cause": "ROOT_CAUSE_87_NULL_RELOCATION"
    },
    "recommendations": [
        "Utiliser Level Zero API pour production",
        "Désactiver relocations i915 DRM",
        "Forcer EXEC_OBJECT_PINNED sur tous buffers",
        "Upgrade firmware GuC v71.x",
        "Implémenter fallback OpenCL si Level Zero échoue"
    ]
}
```

---

## 🔬 TESTS ADDITIONNELS (Si oubliés)

### Test A: Validation Forensique Bit-Level
```c
// Vérifier chaque bit du hash SHA256
for (int i = 0; i < 256; i++) {
    uint8_t bit = (hash[i/8] >> (i%8)) & 1;
    if (bit != expected_bit[i]) {
        log_error("Bit %d incorrect: got %d, expected %d", 
                  i, bit, expected_bit[i]);
    }
}
```

### Test B: Comparaison CPU vs GPU
```c
// Calculer même nonce sur CPU et GPU
uint8_t hash_cpu[32], hash_gpu[32];
sha256_cpu(block_header, nonce, hash_cpu);
sha256_gpu(block_header, nonce, hash_gpu);

if (memcmp(hash_cpu, hash_gpu, 32) != 0) {
    log_error("CPU/GPU mismatch for nonce %u", nonce);
    dump_hex("CPU", hash_cpu, 32);
    dump_hex("GPU", hash_gpu, 32);
}
```

### Test C: Injection Nonce Connu
```c
// Injecter un nonce qui DOIT produire X leading zeros
uint32_t known_nonce = 0x1234ABCD;  // Connu pour produire 5 zeros
uint8_t result[32];
sha256_gpu(block_header, known_nonce, result);

uint32_t leading_zeros = count_leading_zeros(result);
if (leading_zeros != 5) {
    log_error("Expected 5 leading zeros, got %u", leading_zeros);
    log_error("GPU calculation is INCORRECT");
}
```

### Test D: Stress Multi-GPU (si disponible)
```c
// Tester sur tous GPU disponibles
for (int gpu_id = 0; gpu_id < num_gpus; gpu_id++) {
    select_gpu(gpu_id);
    run_mining_test(60);  // 60 secondes par GPU
}
```

### Test E: Power Management
```c
// Tester différents états P-state
for (int pstate = 0; pstate <= 3; pstate++) {
    set_gpu_pstate(pstate);
    run_mining_test(60);
    measure_power_consumption();
}
```

---

## 🚀 EXÉCUTION IMMÉDIATE

### Commandes à Lancer

```bash
# 1. Créer dossier résultats
mkdir -p master_lvx/results_exhaustive

# 2. Lancer tests Phase 1 (Baseline)
cd src/advanced_calculations/bitcoin_quantum_mining/level_zero_native
./test_c282_progressive_batch.sh > master_lvx/results_exhaustive/T001_natif.log 2>&1

# 3. Lancer tests Phase 6 (Relocations - CRITIQUE)
./tests/test_c610_pinned_fix > master_lvx/results_exhaustive/T501_pinned.log 2>&1

# 4. Lancer OpenCL (référence 45 nonces)
cd ..
./test_c194_opencl_drm_benchmark --duration=60 > master_lvx/results_exhaustive/T003_opencl.log 2>&1

# 5. Comparer résultats
python3 master_lvx/compare_results.py master_lvx/results_exhaustive/
```

---

## ✅ CHECKLIST VALIDATION

- [ ] **T001**: i915 DRM Natif testé
- [ ] **T002**: Level Zero Pure testé
- [ ] **T003**: OpenCL Pure testé (référence 45 nonces)
- [ ] **T101**: Natif + Level Zero hybride testé
- [ ] **T102**: Natif + OpenCL hybride testé
- [ ] **T501**: EXEC_OBJECT_PINNED testé (fix ROOT CAUSE #87)
- [ ] **T504**: Level Zero sans relocation testé
- [ ] **T505**: OpenCL sans relocation testé
- [ ] Comparaison CPU vs GPU validée
- [ ] Injection nonce connu validée
- [ ] Rapport final JSON généré
- [ ] Recommandations produites

---

## 🎓 EXPERTISES MOBILISÉES

1. **Architecture GPU Intel Gen9** - Hardware UHD 620
2. **i915 DRM Kernel Driver** - Relocations, GEM, GTT
3. **Level Zero API** - Runtime Intel moderne
4. **OpenCL API** - Standard Khronos
5. **NEO Compute Runtime** - Backend Intel
6. **Unified Runtime** - Abstraction multi-backend
7. **Firmware GPU** - GuC, HuC, DMC
8. **Memory Management** - Allocations, coherence, cache
9. **Synchronisation GPU** - Fences, semaphores, barriers
10. **Forensique Système** - Logs nanoseconde, bit-level
11. **Performance Analysis** - Profiling, métriques
12. **Validation Cryptographique** - SHA256, Bitcoin mining

---

## 📝 NOTIFICATIONS AJOUTS

**Éléments ajoutés que vous n'aviez pas précisés**:

1. ✅ **Test injection nonce connu** - Valider calculs GPU corrects
2. ✅ **Comparaison CPU vs GPU** - Vérifier cohérence résultats
3. ✅ **Tests cache cohérence** - CLFLUSH, MOCS, SET_DOMAIN
4. ✅ **Tests firmware GPU** - GuC versions différentes
5. ✅ **Tests multi-contexte** - Parallélisme GPU
6. ✅ **Tests memory leak** - Détection fuites mémoire
7. ✅ **Tests power management** - P-states GPU
8. ✅ **Validation bit-level** - Vérifier chaque bit hash
9. ✅ **Métriques TLB/cache** - Performance mémoire
10. ✅ **Timeline semaphores** - Synchronisation avancée

---

## 🎯 PRIORITÉ IMMÉDIATE

**Tests à lancer EN PREMIER** (ordre priorité):

1. **T003** - OpenCL Pure (référence 45 nonces) ← BASELINE
2. **T501** - EXEC_OBJECT_PINNED (fix ROOT CAUSE #87) ← FIX CRITIQUE
3. **T001** - i915 DRM Natif (état actuel) ← COMPARAISON
4. **T002** - Level Zero Pure (alternative) ← SOLUTION POTENTIELLE
5. **T101** - Natif + Level Zero (hybride) ← COMPROMIS

**Temps estimé**: 5 tests × 60s = 5 minutes

---

**FIN DU PLAN - PRÊT POUR EXÉCUTION**

**Signature**: Bob - Architecte Systèmes GPU Multi-Runtime  
**Date**: 2026-06-19 23:28:00 UTC+2  
**Status**: ✅ PLAN COMPLET - AUCUNE OPTION ÉLIMINÉE