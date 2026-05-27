# RAPPORT C221-C224 — PLAN MIGRATION LEVEL ZERO NATIF
## Bitcoin Quantum Mining LumVorax — 0% Dépendance OpenCL

**Date** : 2026-05-12  
**Cycles** : C221 → C224  
**Objectif** : Migration complète vers Level Zero natif (0% dépendance OpenCL)

---

## 📋 CONTEXTE

### Situation Actuelle (C220)

**Architecture** :
- ✅ Interface abstraite [`btc_gpu_backend.h`](../src/btc_gpu_backend.h) créée (349 lignes)
- ✅ Support multi-backend (OpenCL + Level Zero)
- ⏳ Implémentations concrètes manquantes

**Dépendance OpenCL** : 100% (runtime obligatoire)

### Analyse C197.51 (Historique)

**Recommandation précédente** : Rester sur OpenCL optimisé
- Performance : 21.86 MH/s
- Overhead : 0.27%
- Gain Level Zero : +0.17% (marginal)

**MAIS** : Nouvelle directive utilisateur = **0% dépendance OpenCL**

### Objectif C221-C224

**Mission** : Migration complète Level Zero natif
- ✅ Implémenter wrapper OpenCL (transition)
- ✅ Implémenter wrapper Level Zero (cible)
- ✅ Tests comparatifs
- ✅ Migration 100% Level Zero
- 🎯 **Résultat** : 0% dépendance OpenCL

---

## 🎯 PLAN D'EXÉCUTION

### C221 : Implémentation Backends Concrets

#### Phase 1 : Wrapper OpenCL (btc_gpu_opencl.c)

**Objectif** : Adapter code actuel vers interface abstraite

**Fichiers à créer** :
- `src/btc_gpu_opencl.c` (~800 lignes)
- `src/btc_gpu_opencl_internal.h` (~100 lignes)

**Mapping API** :
```c
// Interface abstraite → OpenCL
btc_gpu_backend_init()           → clCreateContext()
btc_gpu_buffer_alloc()           → clCreateBuffer()
btc_gpu_kernel_create()          → clCreateKernel()
btc_gpu_kernel_launch()          → clEnqueueNDRangeKernel()
btc_gpu_event_get_profiling()   → clGetEventProfilingInfo()
```

**Code source** : Adapter [`btc_lumvorax_gpu_native.c`](../src/btc_lumvorax_gpu_native.c)

**Estimation** : 2-3 heures

#### Phase 2 : Wrapper Level Zero (btc_gpu_levelzero.c)

**Objectif** : Adapter code C198 vers interface abstraite

**Fichiers à créer** :
- `src/btc_gpu_levelzero.c` (~1200 lignes)
- `src/btc_gpu_levelzero_internal.h` (~150 lignes)

**Mapping API** :
```c
// Interface abstraite → Level Zero
btc_gpu_backend_init()           → zeInit() + zeContextCreate()
btc_gpu_buffer_alloc()           → zeMemAllocDevice()
btc_gpu_kernel_create()          → zeKernelCreate()
btc_gpu_kernel_launch()          → zeCommandListAppendLaunchKernel()
btc_gpu_event_get_profiling()   → zeEventQueryKernelTimestamp()
```

**Code source** : Adapter [`btc_levelzero_runner.c`](../level_zero_native/src/btc_levelzero_runner.c)

**Estimation** : 3-4 heures

#### Phase 3 : Dispatcher Unifié (btc_gpu_backend.c)

**Objectif** : Router appels vers backend actif

**Fichier à créer** :
- `src/btc_gpu_backend.c` (~400 lignes)

**Logique** :
```c
int btc_gpu_backend_init(const btc_gpu_config_t *config, 
                         btc_gpu_backend_context_t **ctx_out) {
    // Détection backend
    btc_gpu_backend_type_t type = config->backend_type;
    if (type == BTC_GPU_BACKEND_AUTO) {
        type = btc_gpu_backend_detect();  // Level Zero prioritaire
    }
    
    // Dispatch
    if (type == BTC_GPU_BACKEND_OPENCL) {
        return btc_gpu_opencl_init(config, ctx_out);
    } else if (type == BTC_GPU_BACKEND_LEVELZERO) {
        return btc_gpu_levelzero_init(config, ctx_out);
    }
    
    return -1;
}
```

**Estimation** : 1 heure

### C222 : Tests Comparatifs

#### Test 1 : Benchmark Hashrate

**Objectif** : Comparer performance OpenCL vs Level Zero

**Fichier** : `tests/test_c222_benchmark_backends.c`

**Métriques** :
- Hashrate (MH/s)
- Latence kernel (ns)
- Overhead CPU (%)
- Bandwidth GPU (GB/s)

**Durée test** : 60 secondes par backend

**Estimation** : 1 heure

#### Test 2 : Validation Métriques Pipeline

**Objectif** : Vérifier métriques C219 sur Level Zero

**Fichier** : `tests/test_c222_pipeline_metrics.c`

**Métriques** :
- `gpu_queue_wait_ns`
- `gpu_transfer_ns`
- `gpu_total_pipeline_ns`
- `gpu_utilization_pct`

**Estimation** : 1 heure

#### Test 3 : Stress Test Longue Durée

**Objectif** : Stabilité Level Zero (180s)

**Fichier** : `tests/test_c222_stress_levelzero.c`

**Métriques** :
- Memory leaks
- Crashes
- Performance drift
- Error rate

**Estimation** : 1 heure

### C223 : Migration Complète Level Zero

#### Phase 1 : Intégration Production

**Objectif** : Remplacer OpenCL par Level Zero dans code production

**Fichiers à modifier** :
- `src/btc_mining_engine.c` : Utiliser `btc_gpu_backend.h`
- `src/nx48_btc_controller.c` : Variable `BTC_GPU_BACKEND=levelzero`
- `Makefile` : Linker Level Zero au lieu d'OpenCL

**Estimation** : 2 heures

#### Phase 2 : Suppression Code OpenCL

**Objectif** : Nettoyer code legacy

**Fichiers à supprimer** :
- `src/btc_opencl_runner.c` (legacy)
- `src/btc_opencl_runner.h` (legacy)
- `src/btc_gpu_opencl.c` (wrapper transition)

**Fichiers à garder** :
- `src/btc_lumvorax_gpu_native.c` (peut être adapté Level Zero)

**Estimation** : 1 heure

#### Phase 3 : Bytecode Embarqué

**Objectif** : 0% dépendance runtime OpenCL

**Actions** :
1. Compiler kernel SHA256 avec `ocloc` :
   ```bash
   ocloc compile -file kernels/btc_sha256_mining.cl \
                 -device gen9 \
                 -output kernels/btc_sha256_gen9.bin
   ```

2. Embarquer bytecode dans header :
   ```bash
   xxd -i kernels/btc_sha256_gen9.bin > include/btc_sha256_gen9_bytecode.h
   ```

3. Charger bytecode au runtime :
   ```c
   extern const unsigned char btc_sha256_gen9_bytecode[];
   extern const unsigned int btc_sha256_gen9_bytecode_len;
   
   config.kernel_bytecode = btc_sha256_gen9_bytecode;
   config.kernel_bytecode_size = btc_sha256_gen9_bytecode_len;
   ```

**Estimation** : 1 heure

### C224 : Validation Finale (Si Nécessaire)

#### Test 1 : Vérification 0% Dépendance

**Objectif** : Confirmer aucune lib OpenCL chargée

**Commande** :
```bash
ldd bin/btc_mining_runner | grep -i opencl
# Résultat attendu : (vide)
```

**Estimation** : 15 minutes

#### Test 2 : Performance Finale

**Objectif** : Valider hashrate ≥ OpenCL

**Métriques** :
- Hashrate Level Zero ≥ 21.86 MH/s
- GPU utilization ≥ 66%
- Stabilité 180s

**Estimation** : 30 minutes

#### Test 3 : Rapport Final

**Objectif** : Documenter migration complète

**Fichier** : `RAPPORT_C224_MIGRATION_LEVEL_ZERO_COMPLETE.md`

**Contenu** :
- Métriques avant/après
- Dépendances supprimées
- Performance validée
- Leçons apprises

**Estimation** : 1 heure

---

## 📊 ESTIMATION TOTALE

| Cycle | Phase | Durée | Cumul |
|-------|-------|-------|-------|
| **C221** | Wrapper OpenCL | 2-3h | 3h |
| **C221** | Wrapper Level Zero | 3-4h | 7h |
| **C221** | Dispatcher | 1h | 8h |
| **C222** | Tests comparatifs | 3h | 11h |
| **C223** | Migration production | 2h | 13h |
| **C223** | Suppression OpenCL | 1h | 14h |
| **C223** | Bytecode embarqué | 1h | 15h |
| **C224** | Validation finale | 2h | 17h |

**Total** : 15-17 heures

---

## 🎯 OBJECTIFS MESURABLES

### Performance

- ✅ Hashrate Level Zero ≥ 21.86 MH/s (baseline OpenCL)
- ✅ GPU utilization ≥ 66% (C219)
- ✅ Overhead ≤ 0.27% (C197)
- ✅ Stabilité 180s sans crash

### Dépendances

- ✅ 0% dépendance OpenCL runtime
- ✅ 0% dépendance `libOpenCL.so`
- ✅ 0% dépendance `libigdrcl.so`
- ✅ Bytecode Gen9 embarqué

### Code

- ✅ Interface abstraite unifiée
- ✅ Support multi-backend
- ✅ Tests automatisés
- ✅ Documentation complète

---

## 🚨 RISQUES IDENTIFIÉS

### Risque 1 : Performance Level Zero < OpenCL

**Probabilité** : Faible (C197.51 : +0.17%)  
**Impact** : Moyen  
**Mitigation** : Optimisations Gen9 spécifiques

### Risque 2 : Bugs Level Zero

**Probabilité** : Moyenne (code C198 complexe)  
**Impact** : Élevé  
**Mitigation** : Tests exhaustifs + logs forensiques

### Risque 3 : Compatibilité Gen9

**Probabilité** : Faible (C198 Phase 12H validé)  
**Impact** : Critique  
**Mitigation** : Tests hardware réels

### Risque 4 : Maintenance Long Terme

**Probabilité** : Moyenne (API Level Zero évolutive)  
**Impact** : Moyen  
**Mitigation** : Documentation + tests régression

---

## 📚 RÉFÉRENCES

### Code Existant

- [`btc_lumvorax_gpu_native.c`](../src/btc_lumvorax_gpu_native.c) : Implémentation OpenCL actuelle
- [`btc_levelzero_runner.c`](../level_zero_native/src/btc_levelzero_runner.c) : Implémentation Level Zero C198
- [`btc_gpu_backend.h`](../src/btc_gpu_backend.h) : Interface abstraite C220

### Rapports

- [`RAPPORT_C219_INSTRUMENTATION_OPENCL_PIPELINE_COMPLET.md`](RAPPORT_C219_INSTRUMENTATION_OPENCL_PIPELINE_COMPLET.md) : Métriques pipeline
- [`RAPPORT_SYNTHESE_FINALE_C197_17_51.md`](RAPPORT_SYNTHESE_FINALE_C197_17_51.md) : Architecture Intel GPU
- [`STANDARD_NAMES_LEVEL_ZERO.md`](../level_zero_native/STANDARD_NAMES_LEVEL_ZERO.md) : Conventions Level Zero

### Documentation Intel

- Level Zero Specification : https://spec.oneapi.io/level-zero/latest/
- Intel Compute Runtime : https://github.com/intel/compute-runtime
- Gen9 Architecture : Intel HD Graphics 620

---

## ✅ CHECKLIST C221-C224

### C221 : Implémentation Backends

- [ ] Créer `btc_gpu_opencl.c` (wrapper OpenCL)
- [ ] Créer `btc_gpu_levelzero.c` (wrapper Level Zero)
- [ ] Créer `btc_gpu_backend.c` (dispatcher)
- [ ] Compiler sans erreurs ni warnings
- [ ] Tests unitaires basiques

### C222 : Tests Comparatifs

- [ ] Benchmark hashrate OpenCL vs Level Zero
- [ ] Validation métriques pipeline Level Zero
- [ ] Stress test 180s Level Zero
- [ ] Rapport comparatif MD

### C223 : Migration Production

- [ ] Intégrer `btc_gpu_backend.h` dans production
- [ ] Variable `BTC_GPU_BACKEND=levelzero`
- [ ] Supprimer code OpenCL legacy
- [ ] Compiler kernel SHA256 avec `ocloc`
- [ ] Embarquer bytecode Gen9
- [ ] Tests production

### C224 : Validation Finale

- [ ] Vérifier 0% dépendance OpenCL (`ldd`)
- [ ] Valider hashrate ≥ 21.86 MH/s
- [ ] Test stabilité 180s
- [ ] Rapport final MD

---

## 🎓 LEÇONS C197 À APPLIQUER

### 1. Optimisations Validées

**À conserver** :
- Réutilisation contexte (pas de recompilation JIT)
- Async execution (overlap CPU/GPU)
- Batch optimization (10 launches/sync)

### 2. Métriques Forensiques

**À implémenter** :
- Timestamps nanoseconde
- Profiling hardware
- Logs bit-level
- Traçabilité totale

### 3. Architecture Modulaire

**À respecter** :
- Interface abstraite
- Backends interchangeables
- Tests automatisés
- Documentation complète

---

## 🚀 DÉMARRAGE C221

**Prochaine action** : Créer `btc_gpu_opencl.c`

**Commande** :
```bash
cd src/advanced_calculations/bitcoin_quantum_mining
# Créer wrapper OpenCL
```

---

**FIN RAPPORT PLANIFICATION C221-C224**