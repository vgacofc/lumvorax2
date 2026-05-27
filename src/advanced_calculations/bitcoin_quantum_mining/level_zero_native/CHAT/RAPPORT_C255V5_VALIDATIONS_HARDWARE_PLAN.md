# RAPPORT C255V5 — PLAN VALIDATIONS HARDWARE
## LumVorax Bitcoin Quantum Mining — Validations HARDWARE Complètes

**Date**: 2026-05-15  
**Cycle**: C255v5 (PLANIFICATION)  
**Objectif**: Ajouter validations HARDWARE manquantes pour prouver parallélisme GPU réel  
**État**: 📋 PLANIFICATION DÉTAILLÉE

---

## 🎯 OBJECTIFS C255V5

### Validations HARDWARE à Implémenter

#### 1. Compteurs GPU i915 Perf (EU Busy Time)
**Objectif**: Mesurer utilisation réelle Execution Units GPU  
**API**: i915 Perf (Performance Monitoring Unit)  
**Métriques**:
- EU busy time (% utilisation)
- EU stall time (% attente mémoire)
- EU active threads count
- Validation: 9 batch_bo busy simultanément

**Implémentation**:
```c
// Structure compteurs GPU
typedef struct {
    int perf_fd;                    // File descriptor i915 perf
    uint64_t eu_busy_ns;            // Temps EU busy (nanosec)
    uint64_t eu_stall_ns;           // Temps EU stall (nanosec)
    uint64_t eu_active_threads;     // Threads actifs
    double eu_utilization_percent;  // % utilisation
} btc_gpu_perf_t;

// Initialisation i915 perf
int btc_gen9_init_perf(btc_gen9_context_t* ctx);

// Lecture compteurs avant dispatch
int btc_gen9_perf_start(btc_gen9_context_t* ctx);

// Lecture compteurs après dispatch
int btc_gen9_perf_stop(btc_gen9_context_t* ctx, btc_gpu_perf_t* perf);
```

**Validation Parallélisme**:
- Mesurer EU busy pendant 9 dispatches
- Si parallélisme réel: EU busy ~90% constant
- Si sérialisation: EU busy pics isolés

---

#### 2. Checksums CPU vs GPU
**Objectif**: Valider intégrité calculs GPU  
**Méthode**: Comparer output GPU avec calcul CPU référence  
**Algorithme**: CRC32C (hardware accelerated x86)

**Implémentation**:
```c
// Calcul checksum CPU référence
uint32_t btc_compute_cpu_checksum(const uint32_t* data, size_t count);

// Calcul checksum GPU output
uint32_t btc_compute_gpu_checksum(const uint32_t* gpu_output, size_t count);

// Validation
bool btc_validate_gpu_output(btc_gen9_context_t* ctx) {
    uint32_t cpu_crc = btc_compute_cpu_checksum(ctx->input_map, ctx->input_size/4);
    uint32_t gpu_crc = btc_compute_gpu_checksum(ctx->output_map, ctx->output_size/4);
    
    if (cpu_crc != gpu_crc) {
        LOG_EVENT(ctx, "CHECKSUM_MISMATCH: cpu=0x%08x gpu=0x%08x", cpu_crc, gpu_crc);
        return false;
    }
    
    LOG_EVENT(ctx, "CHECKSUM_OK: crc=0x%08x", cpu_crc);
    return true;
}
```

**Tests**:
- Validation après chaque dispatch
- Détection corruption mémoire GPU
- Détection erreurs calcul GPU

---

#### 3. Timestamps GPU Hardware Fences
**Objectif**: Mesurer temps GPU réel (pas CPU)  
**Méthode**: Utiliser timestamps GPU via fences i915  
**Précision**: Nanoseconde hardware

**Implémentation**:
```c
// Structure fence avec timestamp GPU
typedef struct {
    uint32_t fence_bo;              // Buffer object fence
    void* fence_map;                // Mapping CPU
    uint64_t gpu_timestamp_start;   // Timestamp GPU début
    uint64_t gpu_timestamp_end;     // Timestamp GPU fin
} btc_gpu_fence_t;

// Créer fence avec timestamp GPU
int btc_gen9_create_fence(btc_gen9_context_t* ctx, btc_gpu_fence_t* fence);

// Insérer commande timestamp dans batch buffer
void btc_gen9_insert_timestamp(uint32_t* batch, int* offset, uint32_t fence_bo);

// Lire timestamp GPU après exécution
uint64_t btc_gen9_read_gpu_timestamp(btc_gpu_fence_t* fence);

// Calculer temps GPU réel
double btc_gen9_compute_gpu_time(btc_gpu_fence_t* fence) {
    uint64_t delta_ns = fence->gpu_timestamp_end - fence->gpu_timestamp_start;
    return (double)delta_ns / 1000000000.0;  // Secondes
}
```

**Avantages**:
- Temps GPU précis (pas overhead CPU)
- Détection vraie latence GPU
- Validation parallélisme temporel

---

#### 4. Test Kernel SHA-256 Bitcoin Réel
**Objectif**: Mesurer hashrate réel avec pool batch_bo  
**Kernel**: `btc_sha256_gen9.bin` (44 KB ELF)  
**Baseline**: 1.28 GH/s (C255v3)  
**Objectif**: >1.5 GH/s avec pool

**Implémentation**:
```c
// Configuration test production
btc_gen9_config_t config = {
    .batch_size = 256 * 1024 * 1024,  // 256M nonces
    .work_group_size = 256,
    .kernel_path = "kernels/btc_sha256_gen9.bin",  // Kernel réel 44 KB
    .log_path = "logs/forensic/test_c255v5_production.log",
    .enable_profiling = true
};

// Test 100 dispatches avec pool
#define NUM_DISPATCHES 100
for (int i = 0; i < NUM_DISPATCHES; i++) {
    btc_gen9_perf_start(ctx);
    btc_gen9_execute(ctx);
    btc_gen9_perf_stop(ctx, &perf);
    btc_validate_gpu_output(ctx);
    
    // Métriques
    double hashrate_gh = (double)config.batch_size / perf.gpu_time_sec / 1e9;
    LOG_EVENT(ctx, "DISPATCH_%d: hashrate=%.2f GH/s eu_busy=%.1f%%", 
             i, hashrate_gh, perf.eu_utilization_percent);
}
```

**Métriques Attendues**:
- Hashrate: >1.5 GH/s (amélioration ×1.17 vs baseline)
- EU busy: >85% (parallélisme réel)
- Checksums: 100% validés
- Temps GPU: <700 ms/dispatch

---

## 📋 PLAN IMPLÉMENTATION

### Phase 1: Compteurs GPU i915 Perf (2h)
1. Rechercher API i915 perf documentation
2. Implémenter `btc_gen9_init_perf()`
3. Implémenter `btc_gen9_perf_start/stop()`
4. Tester lecture compteurs EU busy
5. Valider parallélisme 9 batch_bo

### Phase 2: Checksums CPU vs GPU (1h)
1. Implémenter `btc_compute_cpu_checksum()` (CRC32C)
2. Implémenter `btc_compute_gpu_checksum()`
3. Ajouter validation après chaque dispatch
4. Tester détection corruption

### Phase 3: Timestamps GPU Fences (1.5h)
1. Implémenter `btc_gen9_create_fence()`
2. Ajouter commandes timestamp dans batch buffer
3. Implémenter `btc_gen9_read_gpu_timestamp()`
4. Remplacer timestamps CPU par GPU
5. Valider précision nanoseconde

### Phase 4: Test Production Kernel Réel (1h)
1. Configurer test 100 dispatches
2. Utiliser kernel SHA-256 44 KB
3. Mesurer hashrate avec pool batch_bo
4. Comparer vs baseline C255v3
5. Générer rapport performance

### Phase 5: Documentation (0.5h)
1. Générer rapport forensique C255v5
2. Mettre à jour STANDARD_NAMES.md
3. Créer guide validations HARDWARE

**Temps Total Estimé**: 6 heures

---

## 🔍 CRITÈRES SUCCÈS

### Validations Techniques
- ✅ Compteurs GPU i915 perf fonctionnels
- ✅ EU busy >85% pendant dispatches
- ✅ Checksums 100% validés (0 corruption)
- ✅ Timestamps GPU précis (±10 ns)
- ✅ Hashrate >1.5 GH/s avec pool

### Preuves Parallélisme
- ✅ 9 batch_bo busy simultanément (compteurs GPU)
- ✅ EU utilization constant >85%
- ✅ Temps GPU <700 ms/dispatch
- ✅ Pas de pics isolés (sérialisation)

### Documentation
- ✅ Rapport forensique C255v5 complet
- ✅ STANDARD_NAMES.md mis à jour
- ✅ Logs analysés ligne par ligne
- ✅ Guide validations HARDWARE

---

## 📊 MÉTRIQUES ATTENDUES C255V5

### Performance
| Métrique | C255v4 | C255v5 Objectif | Amélioration |
|----------|--------|-----------------|--------------|
| Hashrate | 1.28 GH/s | >1.5 GH/s | +17% |
| Temps dispatch | 844 ms | <700 ms | -17% |
| EU busy | Non mesuré | >85% | Nouveau |
| Checksums validés | 0% | 100% | Nouveau |
| Timestamps GPU | CPU | GPU hardware | Nouveau |

### Validations
- Compteurs GPU: ✅ Implémentés
- Checksums: ✅ Validés
- Timestamps GPU: ✅ Précis
- Parallélisme: ✅ Prouvé

---

## 🚀 PROCHAINE ACTION

**Commencer Phase 1**: Implémenter compteurs GPU i915 perf

**Fichier à créer**: `src/btc_gen9_perf_counters.c` (300 lignes estimées)

**Commande compilation**:
```bash
cd src/advanced_calculations/bitcoin_quantum_mining/level_zero_native
make clean
make test_c255v5_hardware_validation
```

**Test exécution**:
```bash
./tests/test_c255v5_hardware_validation
```

---

**Rapport généré**: 2026-05-15  
**Cycle**: C255v5 (PLANIFICATION)  
**Temps estimé**: 6 heures  
**État**: 📋 Prêt pour implémentation