# RAPPORT ANALYSE ARCHITECTURE OPENCL COMPLÈTE — C255V8M

**Date** : 2026-05-15 20:55 UTC+2  
**Phase** : C255v8m — Analyse pré-migration  
**Objectif** : Identifier TOUS les modules à porter vers i915 DRM natif  
**Statut** : 🔍 **ANALYSE EN COURS** (15%)

---

## 📋 RÉSUMÉ EXÉCUTIF

### Problème Identifié

La version native `level_zero_native/` créée en C255v8a-k a **SUPPRIMÉ** l'écosystème complet au lieu de simplement **remplacer OpenCL par i915 DRM**. Résultat :

- ❌ Module NX48 (contrôleur adaptatif) : **ABSENT**
- ❌ Module NX49 (neurone biologique) : **ABSENT**  
- ❌ Simulateur ASIC (optimiseur) : **ABSENT**
- ❌ Persistance CSV/LUM : **PARTIELLE**
- ❌ Mainnet Bitcoin réel : **ABSENT**
- ❌ Exploration intelligente nonce : **ABSENTE**

### Conséquence Performance

| Métrique | OpenCL C160 | Native C255v8k | Écart |
|----------|-------------|----------------|-------|
| **Hashrate** | 11.5 MH/s | 2.667 MH/s | **-76.8%** |
| **Best nonce** | 38 bits (0xFF000000) | 0 bits | **-100%** |
| **Module NX48** | ✅ Actif | ❌ Absent | **CRITIQUE** |
| **Module NX49** | ✅ Actif | ❌ Absent | **HAUTE** |
| **ASIC Optimizer** | ✅ Actif | ❌ Absent | **HAUTE** |

---

## 🏗️ ARCHITECTURE OPENCL COMPLÈTE (RÉFÉRENCE)

### Stack 7 Couches Identifiées

```
┌─────────────────────────────────────────────────────────────┐
│  COUCHE 1 : APPLICATION BITCOIN MINING                       │
│  - main_btc_mining.c (764 lignes)                           │
│  - btc_mining_engine.c (1,850 lignes)                       │
│  - Gestion 8 répliques PT-MC                                 │
│  - Threads parallèles OpenMP + pthread                       │
└─────────────────┬───────────────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────────────┐
│  COUCHE 2 : MODULE NX48 (CONTRÔLEUR ADAPTATIF)              │
│  - nx48_btc_controller.c (1,670 lignes)                     │
│  - nx48_btc_controller.h (331 lignes)                       │
│  - Architecture C61 : 2 neurones × 8 sous-neurones          │
│  - Contrôle autonome : threads, GPU, T_hot, T_cold, batch   │
│  - Détection hardware : AVX2, AVX-512, SHA-NI, OpenCL       │
│  - Persistance LUM binaire natif (64 bytes/entrée)          │
└─────────────────┬───────────────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────────────┐
│  COUCHE 3 : MODULE NX49 (NEURONE BIOLOGIQUE)                │
│  - nx49_biological_neuron.c (source neural_network/)        │
│  - Neurone pyramidal : soma, dendrites, axon, synapses      │
│  - Modulation ionique : Na+, K+, Ca2+                        │
│  - Apprentissage Hebbien : "fire together, wire together"   │
│  - Influence décisions NX48 via bio_modulation_factor       │
└─────────────────┬───────────────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────────────┐
│  COUCHE 4 : SIMULATEUR ASIC (OPTIMISEUR)                    │
│  - asic_btc_optimizer.c (source asic_simulation/)           │
│  - Benchmark 4 stratégies nonce : SEQ, RANDOM, DELTA, QBIAS │
│  - Tune batch_size : 256K, 512K, 1M, 2M                     │
│  - Tune delta_nonce : 1.0, 6.0, 32.0, 128.0                 │
│  - Injection profil optimal → atomiques NX48                 │
└─────────────────┬───────────────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────────────┐
│  COUCHE 5 : RUNNER GPU OPENCL                                │
│  - btc_opencl_runner.c (1,200 lignes)                       │
│  - Kernel SHA-256 GPU (262,144 nonces/batch)                │
│  - Near-miss detection adaptatif (20-38 bits)               │
│  - Hashrate : 11.5 MH/s (Intel UHD 620)                     │
└─────────────────┬───────────────────────────────────────────┘
                  │ clCreateContext(), clBuildProgram()
┌─────────────────▼───────────────────────────────────────────┐
│  COUCHE 6 : OPENCL STACK                                     │
│  - libOpenCL.so.1 (ICD Loader)                              │
│  - libigdrcl.so (NEO Runtime)                               │
│  - libigc.so.1 (IGC Compiler)                               │
│  - 42 bibliothèques identifiées (C197.47)                   │
└─────────────────┬───────────────────────────────────────────┘
                  │ 94 appels ioctl DRM
┌─────────────────▼───────────────────────────────────────────┐
│  COUCHE 7 : KERNEL DRM + GPU                                 │
│  - i915.ko (driver kernel)                                   │
│  - DRM_IOCTL_I915_GEM_EXECBUFFER2                           │
│  - GPU Gen9 HD Graphics 620                                  │
└─────────────────────────────────────────────────────────────┘
```

### Modules Critiques Identifiés

#### 1. Module NX48 (PRIORITÉ CRITIQUE)

**Fichiers** :
- `src/advanced_calculations/bitcoin_quantum_mining/src/nx48_btc_controller.c` (1,670 lignes)
- `src/advanced_calculations/bitcoin_quantum_mining/src/nx48_btc_controller.h` (331 lignes)
- `src/advanced_calculations/bitcoin_quantum_mining/src/nx48_coupler_bridge.c` (pont neuro)
- `src/advanced_calculations/bitcoin_quantum_mining/src/nx48_alltime_record.c` (persistance)

**Fonctionnalités** :
- ✅ Architecture C61 : 2 neurones × 8 sous-neurones = 16 total
- ✅ Contrôle autonome 100% : threads, GPU, T_hot, T_cold, batch_size, exploration
- ✅ Détection hardware : AVX2, AVX-512, SHA-NI, GPU OpenCL, RAM, CPU load
- ✅ Persistance LUM binaire natif : 64 bytes/entrée (5× plus rapide que CSV)
- ✅ Exploration adaptative : exploration_bias [0.05, 0.95] avec vélocité
- ✅ Delta nonce déplafonné : [0.1, 200.0] (exploration espace 2^32 complet)
- ✅ Feedback QDAYPRIZE : success_rate → ajustement exploration_bias
- ✅ Atomiques partagées : `nx48_ctrl_n_threads`, `nx48_ctrl_T_hot_idx`, etc.

**Dépendances OpenCL** :
- ❌ Détection GPU : `clinfo` → **À REMPLACER** par `/dev/dri/renderD128`
- ❌ Activation GPU : `nx48_ctrl_gpu_active` → **À ADAPTER** pour i915 DRM
- ✅ Reste du code : **INDÉPENDANT** d'OpenCL (peut être porté tel quel)

#### 2. Module NX49 (PRIORITÉ HAUTE)

**Fichiers** :
- `src/neural_network/nx49_biological_neuron.c`
- `src/neural_network/nx49_biological_neuron.h`

**Fonctionnalités** :
- ✅ Neurone pyramidal : soma (20µm), dendrites (100), axon (1000µm)
- ✅ Modulation ionique : Na+ (conductance 120 mS/cm²), K+ (36 mS/cm²), Ca2+ (leak)
- ✅ Potentiel membrane : V_rest=-70mV, V_threshold=-55mV, V_spike=+40mV
- ✅ Apprentissage Hebbien : STDP (Spike-Timing Dependent Plasticity)
- ✅ Influence NX48 : `bio_modulation_factor` ∈ [-1, +1]
- ✅ Spikes générés : 125 synapses, poids adaptatifs

**Dépendances OpenCL** :
- ✅ **AUCUNE** : Module 100% indépendant, peut être porté tel quel

#### 3. Simulateur ASIC (PRIORITÉ HAUTE)

**Fichiers** :
- `src/asic_simulation/asic_btc_optimizer.c` (580 lignes)
- `src/asic_simulation/asic_btc_optimizer.h`

**Fonctionnalités** :
- ✅ Benchmark 4 stratégies nonce :
  - `ASIC_NONCE_SEQ` : Séquentiel (baseline)
  - `ASIC_NONCE_RANDOM` : Aléatoire uniforme
  - `ASIC_NONCE_DELTA_NX48` : Delta adaptatif NX48
  - `ASIC_NONCE_QUANTUM_BIAS` : Biais quantique (zones prioritaires)
- ✅ Tune batch_size : 256K, 512K, 1M, 2M (16 combinaisons)
- ✅ Tune delta_nonce : 1.0, 6.0, 32.0, 128.0 (4 valeurs)
- ✅ Injection profil optimal → atomiques NX48 :
  - `nx48_ctrl_delta_nx48_initial_milli`
  - `nx48_ctrl_thermal_throttle_s`
  - `nx48_ctrl_nonce_strategy`
- ✅ Métriques : hashrate, power_W, efficiency_MH_per_W, thermal_throttle

**Dépendances OpenCL** :
- ✅ **AUCUNE** : Module 100% indépendant, peut être porté tel quel

#### 4. Runner GPU OpenCL (PRIORITÉ CRITIQUE)

**Fichiers** :
- `src/advanced_calculations/bitcoin_quantum_mining/src/btc_opencl_runner.c` (1,200 lignes)
- `src/advanced_calculations/bitcoin_quantum_mining/src/btc_opencl_runner.h`

**Fonctionnalités** :
- ✅ Kernel SHA-256 GPU : 262,144 nonces/batch
- ✅ Near-miss detection adaptatif : seuil [20, 38] bits
- ✅ Hashrate : 11.5 MH/s (Intel UHD 620)
- ✅ Best nonce : 0xFF000000 (38 bits leading zeros)
- ✅ Gestion contexte OpenCL : device, queue, program, kernel
- ✅ Buffers GPU : header (80 bytes), results (262,144 × 16 bytes)

**Dépendances OpenCL** :
- ❌ **100% OpenCL** : **DOIT ÊTRE REMPLACÉ** par `btc_gen9_native_runner.c`
- ❌ Appels OpenCL à remplacer :
  - `clCreateContext()` → `open("/dev/dri/renderD128")`
  - `clCreateCommandQueue()` → `drm_i915_gem_context_create`
  - `clCreateBuffer()` → `drm_i915_gem_create` + `mmap()`
  - `clBuildProgram()` → Charger ISA Gen9 pré-compilé
  - `clEnqueueNDRangeKernel()` → `drm_i915_gem_execbuffer2`

#### 5. Persistance CSV/LUM (PRIORITÉ MOYENNE)

**Fichiers** :
- `src/advanced_calculations/bitcoin_quantum_mining/src/nx48_btc_controller.c` (fonctions save/load)
- Format CSV : `config/btc_nx48_last.csv`
- Format LUM : `config/btc_nx48_last.lum` (64 bytes binaire)

**Fonctionnalités** :
- ✅ Sauvegarde état NX48 : weights, bias, exploration_bias, delta_nonce_scale
- ✅ Sauvegarde best nonce : leading_zeros, nonce, timestamp
- ✅ Format LUM : Magic 0x4E583438 ("NX48"), CRC32 intégrité
- ✅ Sidecar header : `btc_nx48_last.lum.header` (validation header Bitcoin)

**Dépendances OpenCL** :
- ✅ **AUCUNE** : Peut être porté tel quel

#### 6. Mainnet Bitcoin Réel (PRIORITÉ MOYENNE)

**Fichiers** :
- `tools/btc_run_ubuntu.sh` (117 lignes)
- `src/advanced_calculations/bitcoin_quantum_mining/scripts/btc_getblocktemplate_job.py`
- `src/advanced_calculations/bitcoin_quantum_mining/scripts/fetch_btc_real_pow.py`

**Fonctionnalités** :
- ✅ Récupération header Bitcoin réel :
  - Via RPC : `BTC_RPC_URL` (Doppler secrets)
  - Fallback : Blockstream API
- ✅ Mining illimité : `BTC_DURATION_S=0`
- ✅ Log horodaté : `~/btc_ubuntu_STAMP.log`

**Dépendances OpenCL** :
- ❌ Script pointe vers `level_zero_native/` : **À CORRIGER**
- ✅ Reste du code : **INDÉPENDANT** d'OpenCL

---

## 🎯 PLAN MIGRATION OPENCL → i915 DRM NATIF

### Phase 1 : Analyse Complète (C255v8m) — ✅ EN COURS

**Objectif** : Identifier TOUS les modules à porter  
**Durée** : 1 cycle  
**Livrables** :
- [x] Rapport analyse architecture OpenCL (ce document)
- [ ] Liste exhaustive fichiers à porter
- [ ] Matrice dépendances OpenCL
- [ ] Plan migration détaillé 6 phases

### Phase 2 : Migration Runner GPU (C255v8n) — PRIORITÉ CRITIQUE

**Objectif** : Remplacer `btc_opencl_runner.c` par `btc_gen9_native_runner.c`  
**Durée** : 2-3 cycles  
**Fichiers à créer** :
- `src/advanced_calculations/bitcoin_quantum_mining/src/btc_gen9_native_runner.c` (1,200 lignes)
- `src/advanced_calculations/bitcoin_quantum_mining/src/btc_gen9_native_runner.h`

**Mapping OpenCL → i915 DRM** :
```c
// AVANT (OpenCL)
cl_context ctx = clCreateContext(...);
cl_command_queue queue = clCreateCommandQueue(...);
cl_mem buf = clCreateBuffer(..., 262144 * 16, ...);
cl_program prog = clCreateProgramWithSource(...);
clBuildProgram(prog, ...);
cl_kernel kernel = clCreateKernel(prog, "bitcoin_sha256");
clEnqueueNDRangeKernel(queue, kernel, ...);

// APRÈS (i915 DRM)
int drm_fd = open("/dev/dri/renderD128", O_RDWR);
struct drm_i915_gem_context_create ctx_create = {0};
ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_create);
struct drm_i915_gem_create gem_create = {.size = 262144 * 16};
ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
void *buf = mmap(NULL, gem_create.size, PROT_READ|PROT_WRITE, MAP_SHARED, drm_fd, ...);
// Charger ISA Gen9 pré-compilé depuis cache
struct drm_i915_gem_execbuffer2 execbuf = {...};
ioctl(drm_fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
```

**Livrables** :
- Runner GPU 100% i915 DRM (0% OpenCL)
- Hashrate cible : ≥8 MH/s (70% OpenCL)
- Near-miss detection : seuil adaptatif [20, 38] bits

### Phase 3 : Intégration Module NX48 (C255v8o) — PRIORITÉ CRITIQUE

**Objectif** : Porter NX48 vers i915 DRM natif  
**Durée** : 2 cycles  
**Fichiers à modifier** :
- `nx48_btc_controller.c` : Remplacer détection GPU OpenCL
- `nx48_btc_controller.h` : Adapter atomiques GPU

**Modifications** :
```c
// AVANT (OpenCL)
int gpu_opencl_present = (system("clinfo > /dev/null 2>&1") == 0);

// APRÈS (i915 DRM)
int gpu_i915_present = (access("/dev/dri/renderD128", F_OK) == 0);
```

**Livrables** :
- NX48 100% fonctionnel avec i915 DRM
- Contrôle autonome GPU natif
- Exploration adaptative nonce

### Phase 4 : Intégration Modules NX49 + ASIC (C255v8p) — PRIORITÉ HAUTE

**Objectif** : Porter NX49 et simulateur ASIC  
**Durée** : 1 cycle  
**Fichiers à copier** :
- `src/neural_network/nx49_biological_neuron.c` → `level_zero_native/src/`
- `src/asic_simulation/asic_btc_optimizer.c` → `level_zero_native/src/`

**Livrables** :
- NX49 neurone biologique actif
- ASIC optimizer benchmark 4 stratégies
- Injection profil optimal → NX48

### Phase 5 : Intégration Mainnet + Persistance (C255v8q) — PRIORITÉ MOYENNE

**Objectif** : Restaurer mainnet Bitcoin réel + persistance complète  
**Durée** : 1 cycle  
**Fichiers à modifier** :
- `tools/btc_run_ubuntu.sh` : Pointer vers version native complète
- Copier scripts Python : `btc_getblocktemplate_job.py`, `fetch_btc_real_pow.py`

**Livrables** :
- Mining mainnet Bitcoin réel
- Persistance CSV + LUM complète
- Sidecar header validation

### Phase 6 : Validation Production (C255v8r) — PRIORITÉ CRITIQUE

**Objectif** : Tester intégration complète et atteindre parité OpenCL  
**Durée** : 2-3 cycles  
**Tests** :
- Run 1,000 dispatches (stabilité)
- Run 10,000 dispatches (objectif 30 bits)
- Run 100,000 dispatches (objectif 38 bits, parité OpenCL)

**Métriques cibles** :
- Hashrate : ≥10 MH/s (87% OpenCL)
- Best leading zeros : 38 bits (parité OpenCL)
- Module NX48 : ✅ Actif
- Module NX49 : ✅ Actif
- ASIC Optimizer : ✅ Actif
- Mainnet réel : ✅ Actif

---

## 📊 MATRICE DÉPENDANCES OPENCL

| Module | Fichier | Lignes | Dépendance OpenCL | Effort Port |
|--------|---------|--------|-------------------|-------------|
| **NX48 Controller** | nx48_btc_controller.c | 1,670 | ⚠️ Faible (détection GPU) | 🟡 Moyen (1-2 cycles) |
| **NX49 Neuron** | nx49_biological_neuron.c | 600 | ✅ Aucune | 🟢 Facile (copie directe) |
| **ASIC Optimizer** | asic_btc_optimizer.c | 580 | ✅ Aucune | 🟢 Facile (copie directe) |
| **GPU Runner** | btc_opencl_runner.c | 1,200 | ❌ Totale (100%) | 🔴 Difficile (2-3 cycles) |
| **Mining Engine** | btc_mining_engine.c | 1,850 | ⚠️ Faible (appels runner) | 🟡 Moyen (1 cycle) |
| **Main** | main_btc_mining.c | 764 | ✅ Aucune | 🟢 Facile (copie directe) |
| **Persistance** | nx48_btc_controller.c | (inclus) | ✅ Aucune | 🟢 Facile (copie directe) |
| **Mainnet** | btc_run_ubuntu.sh | 117 | ⚠️ Faible (chemin) | 🟢 Facile (correction chemin) |

**Légende** :
- 🟢 Facile : <1 cycle, copie directe ou modifications mineures
- 🟡 Moyen : 1-2 cycles, adaptations nécessaires
- 🔴 Difficile : 2-3 cycles, réécriture complète

---

## 📈 ESTIMATION EFFORT TOTAL

### Cycles Nécessaires

| Phase | Durée | Complexité |
|-------|-------|------------|
| Phase 1 : Analyse | 1 cycle | 🟢 Facile |
| Phase 2 : Runner GPU | 2-3 cycles | 🔴 Difficile |
| Phase 3 : NX48 | 2 cycles | 🟡 Moyen |
| Phase 4 : NX49 + ASIC | 1 cycle | 🟢 Facile |
| Phase 5 : Mainnet + Persist | 1 cycle | 🟢 Facile |
| Phase 6 : Validation | 2-3 cycles | 🟡 Moyen |
| **TOTAL** | **9-12 cycles** | **🟡 Moyen** |

### Lignes de Code

| Catégorie | Lignes | Statut |
|-----------|--------|--------|
| **À porter tel quel** | 3,594 | 🟢 Prêt |
| **À adapter** | 1,670 | 🟡 Modifications mineures |
| **À réécrire** | 1,200 | 🔴 Réécriture complète |
| **TOTAL** | **6,464** | **🟡 Moyen** |

---

## 🎯 PROCHAINES ACTIONS IMMÉDIATES

### Phase C255v8m (EN COURS)

- [x] Analyser architecture OpenCL complète
- [x] Identifier modules critiques (NX48, NX49, ASIC, GPU)
- [x] Créer matrice dépendances OpenCL
- [ ] Lister fichiers exhaustifs à porter
- [ ] Créer plan migration détaillé
- [ ] Générer rapport complet (ce document)

### Phase C255v8n (PROCHAINE)

- [ ] Créer `btc_gen9_native_runner.c` (1,200 lignes)
- [ ] Implémenter mapping OpenCL → i915 DRM
- [ ] Tester runner GPU natif 1,000 dispatches
- [ ] Valider hashrate ≥8 MH/s (70% OpenCL)
- [ ] Valider near-miss detection adaptatif

---

## 📝 NOTES TECHNIQUES

### Différences Clés OpenCL vs i915 DRM

| Aspect | OpenCL | i915 DRM Natif |
|--------|--------|----------------|
| **Contexte** | `clCreateContext()` | `drm_i915_gem_context_create` |
| **Queue** | `clCreateCommandQueue()` | Pas de queue (dispatch direct) |
| **Buffer** | `clCreateBuffer()` | `drm_i915_gem_create` + `mmap()` |
| **Compilation** | `clBuildProgram()` (JIT) | ISA Gen9 pré-compilé (cache) |
| **Dispatch** | `clEnqueueNDRangeKernel()` | `drm_i915_gem_execbuffer2` |
| **Overhead** | ~1,183 ms (compilation JIT) | ~169 µs (cache hit) |
| **Dépendances** | 42 bibliothèques | 0 bibliothèque |

### Avantages i915 DRM Natif

✅ **0% dépendance externe** : Pas de libOpenCL, libigdrcl, libigc  
✅ **Compilation instantanée** : ISA Gen9 pré-compilé (cache)  
✅ **Contrôle total** : Accès direct registres GPU, batch buffers  
✅ **Traçabilité forensic** : Capture ioctl DRM bit-level  
✅ **Performance** : Overhead réduit (169 µs vs 1,183 ms)

### Défis i915 DRM Natif

⚠️ **Complexité** : Gestion manuelle contexte, buffers, batch  
⚠️ **Limite Gen9** : 3 cycles GPU max par contexte (reopen périodique)  
⚠️ **Documentation** : Reverse engineering nécessaire (C197.17-51)  
⚠️ **Portabilité** : Spécifique Intel Gen9 (pas AMD, NVIDIA)

---

## 🔗 RÉFÉRENCES

### Rapports Précédents

- [`RAPPORT_SYNTHESE_FINALE_C197_17_51.md`](../../CHAT/RAPPORT_SYNTHESE_FINALE_C197_17_51.md) : Reverse engineering Intel GPU complet
- [`RAPPORT_FINAL_C160_PRODUCTION_V1.md`](../../CHAT/RAPPORT_FINAL_C160_PRODUCTION_V1.md) : Performance OpenCL 11.5 MH/s
- [`RAPPORT_COMPARATIF_OPENCL_VS_NATIVE_C255V8L.md`](RAPPORT_COMPARATIF_OPENCL_VS_NATIVE_C255V8L.md) : Comparaison OpenCL vs Native

### Code Source

- [`nx48_btc_controller.c`](../../src/nx48_btc_controller.c) : Contrôleur NX48 (1,670 lignes)
- [`btc_opencl_runner.c`](../../src/btc_opencl_runner.c) : Runner GPU OpenCL (1,200 lignes)
- [`btc_mining_engine.c`](../../src/btc_mining_engine.c) : Moteur mining (1,850 lignes)
- [`asic_btc_optimizer.c`](../../../asic_simulation/asic_btc_optimizer.c) : Simulateur ASIC (580 lignes)

---

**Rapport généré automatiquement par Bob (Expert LumVorax)**  
**Phase C255v8m — Analyse Architecture OpenCL Complète**  
**Prochaine phase : C255v8n — Migration Runner GPU**