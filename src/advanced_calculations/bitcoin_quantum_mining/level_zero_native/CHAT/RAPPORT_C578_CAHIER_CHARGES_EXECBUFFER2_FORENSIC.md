# RAPPORT C578 - CAHIER DES CHARGES EXECBUFFER2 FORENSIC COMPLET

**CYCLE**: C578 - PRIORITÉ ABSOLUE  
**DATE**: 2026-05-26 00:53:38 UTC+2  
**OBJECTIF**: Implémenter forensic EXECBUFFER2 complet + batch integration  
**ÉTAT**: 0% → 100% (estimation 3-4 cycles)

---

## 🎯 OBJECTIF STRATÉGIQUE

### Transition Critique
```
C577: Forensic statique (batch preparation)
    ↓
C578: Forensic dynamique (GPU execution)
```

### Impact Attendu
- **Déblocage allocation GTT**: Output buffer enfin alloué
- **Premier bit GPU natif**: Validation exécution réelle
- **Timeline unifiée**: Corrélation CPU/GPU
- **Télémétrie runtime**: Observation GPU réelle

---

## 📊 ANALYSE FEEDBACK UTILISATEUR

### Points Critiques Identifiés

#### 1. Niveau Actuel Validé ✅
```
Reverse engineering Gen9:     Très élevé
Batch decoding:                Très élevé
Surface States:                Très élevé
Relocation logic:              Élevé
i915 DRM:                      Élevé
```

#### 2. Faiblesses Critiques ⚠️
```
GPU runtime forensic:          Moyen
GPU execution tracing:         Faible
EU telemetry:                  Très faible
Memory transaction tracing:    Faible
End-to-end execution proof:    Non atteint
```

#### 3. Questions Sans Réponse 🔴
1. Le GPU exécute-t-il réellement vos batches ?
2. Les relocations sont-elles réellement appliquées ?
3. Le kernel accède-t-il réellement aux Surface States ?
4. Le binding table pointer est-il valide ?
5. Les EUs dispatchent-ils réellement des threads ?
6. Les writes GPU arrivent-ils réellement en RAM ?
7. Le scheduler i915 modifie-t-il vos timings ?
8. Les caches GPU perturbent-ils la forensic ?

---

## 🚀 ARCHITECTURE C578

### Composants à Implémenter

#### 1. EXECBUFFER2 Forensic Engine
**Fichier**: `forensic/execbuffer2_forensic.h` (300 lignes)

**Structures**:
```c
typedef struct {
    uint32_t buffer_count;
    uint32_t batch_start_offset;
    uint32_t batch_len;
    uint64_t flags;
    uint64_t rsvd1;  // context ID
    uint32_t rsvd2;  // ring select
    uint32_t num_cliprects;
    uint64_t cliprects_ptr;
    uint64_t DR1;
    uint64_t DR4;
    uint32_t batch_buffer;
    uint32_t exec_queue_id;
    uint64_t timeline_value;
} execbuffer2_params_t;

typedef struct {
    uint32_t handle;
    uint32_t relocation_count;
    uint64_t relocs_ptr;
    uint64_t alignment;
    uint64_t offset;  // GTT offset (OUT)
    uint64_t flags;
    uint64_t rsvd1;
    uint64_t rsvd2;
} exec_object2_t;

typedef struct {
    uint64_t timestamp_submit;
    uint64_t timestamp_start;
    uint64_t timestamp_end;
    uint32_t seqno;
    uint32_t ring_id;
    uint64_t context_id;
    uint32_t buffers_allocated;
    uint32_t relocations_applied;
    bool success;
    char error_msg[256];
} execbuffer2_result_t;
```

**Fonctions**:
```c
bool trace_execbuffer2_submit(int drm_fd, 
                              struct drm_i915_gem_execbuffer2 *args,
                              execbuffer2_result_t *result);

bool decode_exec_objects(struct drm_i915_gem_exec_object2 *objects,
                         uint32_t count,
                         exec_object2_t *decoded);

bool validate_relocations_applied(uint32_t *batch_data,
                                  uint32_t batch_size,
                                  struct drm_i915_gem_relocation_entry *relocs,
                                  uint32_t reloc_count);

bool capture_gtt_allocations(int drm_fd,
                             uint32_t *handles,
                             uint32_t count,
                             uint64_t *gtt_offsets);

bool correlate_cpu_gpu_timeline(execbuffer2_result_t *result,
                                uint64_t cpu_submit_ns,
                                uint64_t cpu_complete_ns);
```

#### 2. Ringbuffer Forensic
**Fichier**: `forensic/ringbuffer_forensic.h` (200 lignes)

**Structures**:
```c
typedef struct {
    uint32_t ring_id;  // RCS=0, BCS=1, VCS=2, VECS=3
    uint64_t head;
    uint64_t tail;
    uint64_t start;
    uint64_t size;
    uint32_t space_available;
    uint32_t queue_depth;
    bool is_idle;
} ringbuffer_state_t;

typedef struct {
    uint64_t timestamp;
    uint32_t seqno;
    uint64_t batch_addr;
    uint32_t batch_len;
    uint32_t context_id;
} ring_submission_t;
```

**Fonctions**:
```c
bool capture_ringbuffer_state(int drm_fd, 
                              uint32_t ring_id,
                              ringbuffer_state_t *state);

bool trace_ring_submission(int drm_fd,
                          uint32_t ring_id,
                          ring_submission_t *submission);

bool calculate_ring_latency(ringbuffer_state_t *before,
                           ringbuffer_state_t *after,
                           uint64_t *latency_ns);
```

#### 3. Batch Builder Automatique
**Fichier**: `forensic/batch_builder_gen9.h` (400 lignes)

**Structures**:
```c
typedef struct {
    uint32_t *batch_data;
    uint32_t batch_size;
    uint32_t batch_capacity;
    uint32_t current_offset;
    
    // Surface States
    uint32_t surface_state_offset;
    uint32_t num_surfaces;
    
    // Binding Table
    uint32_t binding_table_offset;
    uint32_t num_bindings;
    
    // Relocations
    struct drm_i915_gem_relocation_entry *relocations;
    uint32_t num_relocations;
    uint32_t reloc_capacity;
    
    // Buffers
    uint32_t *buffer_handles;
    uint32_t num_buffers;
} batch_builder_t;

typedef struct {
    uint32_t input_handle;
    uint32_t output_handle;
    uint32_t kernel_handle;
    uint32_t input_size;
    uint32_t output_size;
    uint32_t kernel_size;
    uint32_t work_dim;
    uint32_t global_work_size[3];
    uint32_t local_work_size[3];
} mining_config_t;
```

**Fonctions**:
```c
bool batch_builder_init(batch_builder_t *builder, uint32_t capacity);
bool batch_builder_add_surface_state(batch_builder_t *builder,
                                     const surface_config_t *config,
                                     uint32_t buffer_handle);
bool batch_builder_add_binding_table(batch_builder_t *builder);
bool batch_builder_add_state_base_address(batch_builder_t *builder);
bool batch_builder_add_interface_descriptor(batch_builder_t *builder,
                                           uint32_t kernel_offset);
bool batch_builder_add_gpgpu_walker(batch_builder_t *builder,
                                   const mining_config_t *config);
bool batch_builder_finalize(batch_builder_t *builder);
bool batch_builder_generate_relocations(batch_builder_t *builder);
```

#### 4. Memory Forensic Profond
**Fichier**: `forensic/memory_forensic.h` (250 lignes)

**Structures**:
```c
typedef struct {
    uint64_t virtual_addr;
    uint64_t gtt_offset;
    uint64_t physical_addr;
    uint32_t size;
    uint32_t cache_level;  // 0=uncached, 1=LLC, 2=L3, 3=LLC+L3
    bool is_coherent;
    bool is_mapped;
} memory_mapping_t;

typedef struct {
    uint64_t timestamp;
    uint64_t address;
    uint32_t size;
    bool is_read;
    bool is_write;
    uint32_t cache_hits;
    uint32_t cache_misses;
} memory_transaction_t;
```

**Fonctions**:
```c
bool capture_gtt_mapping(int drm_fd,
                        uint32_t handle,
                        memory_mapping_t *mapping);

bool validate_cache_coherency(int drm_fd,
                             uint32_t handle,
                             bool *is_coherent);

bool dump_buffer_content(int drm_fd,
                        uint32_t handle,
                        void *data,
                        uint32_t size);

bool compare_buffer_snapshots(void *before,
                             void *after,
                             uint32_t size,
                             uint32_t *num_changes);
```

---

## 🔬 TESTS VALIDATION C578

### Test 1: EXECBUFFER2 Forensic Basique
**Fichier**: `tests/test_c578_execbuffer2_forensic.c`

**Objectif**: Tracer soumission EXECBUFFER2 complète

**Étapes**:
1. Créer 3 buffers (batch, input, output)
2. Générer batch minimal avec Surface States
3. Créer relocations automatiques
4. Soumettre via EXECBUFFER2
5. Capturer tous les événements
6. Valider GTT allocations
7. Vérifier relocations appliquées

**Métriques**:
- Timestamp submit (ns)
- Timestamp start GPU (ns)
- Timestamp end GPU (ns)
- Latency CPU→GPU (ns)
- Latency GPU execution (ns)
- Buffers alloués GTT (count)
- Relocations appliquées (count)

### Test 2: Ringbuffer Forensic
**Fichier**: `tests/test_c578_ringbuffer_forensic.c`

**Objectif**: Observer état ringbuffer avant/après

**Étapes**:
1. Capturer état ringbuffer initial
2. Soumettre batch
3. Capturer état ringbuffer final
4. Calculer latency ring
5. Vérifier queue depth
6. Valider head/tail pointers

### Test 3: Batch Builder Automatique
**Fichier**: `tests/test_c578_batch_builder.c`

**Objectif**: Générer batch complet automatiquement

**Étapes**:
1. Initialiser builder
2. Ajouter 2 Surface States (input/output)
3. Générer Binding Table
4. Ajouter STATE_BASE_ADDRESS
5. Ajouter Interface Descriptor
6. Ajouter GPGPU_WALKER
7. Finaliser batch
8. Générer relocations automatiques
9. Valider batch généré vs OpenCL

### Test 4: Memory Forensic Complet
**Fichier**: `tests/test_c578_memory_forensic.c`

**Objectif**: Tracer transactions mémoire GPU

**Étapes**:
1. Créer buffer output
2. Capturer snapshot avant
3. Soumettre batch write
4. Capturer snapshot après
5. Comparer différences
6. Valider cache coherency
7. Vérifier GTT mapping

### Test 5: Timeline Unifiée CPU/GPU
**Fichier**: `tests/test_c578_timeline_unified.c`

**Objectif**: Corréler événements CPU et GPU

**Étapes**:
1. Timestamp CPU: submit start
2. Timestamp GPU: execution start (via seqno)
3. Timestamp GPU: execution end (via seqno)
4. Timestamp CPU: readback complete
5. Calculer latencies
6. Générer timeline Wireshark

---

## 📈 MÉTRIQUES SUCCÈS C578

### Critères Validation

#### 1. EXECBUFFER2 Tracé ✅
- [ ] Tous les paramètres capturés
- [ ] Buffers listés avec handles
- [ ] Relocations décodées
- [ ] GTT offsets récupérés
- [ ] Seqno capturé
- [ ] Context ID identifié

#### 2. Allocations GTT Validées ✅
- [ ] Output buffer alloué GTT
- [ ] Input buffer alloué GTT
- [ ] Batch buffer alloué GTT
- [ ] Offsets GTT non-nuls
- [ ] Relocations appliquées

#### 3. Batch Complet Généré ✅
- [ ] Surface States (128 bytes)
- [ ] Binding Table (8 bytes)
- [ ] STATE_BASE_ADDRESS
- [ ] Interface Descriptor
- [ ] GPGPU_WALKER
- [ ] Relocations automatiques

#### 4. Timeline Corrélée ✅
- [ ] Timestamp submit CPU
- [ ] Timestamp start GPU
- [ ] Timestamp end GPU
- [ ] Latency CPU→GPU < 1ms
- [ ] Latency GPU execution mesurée

#### 5. Mémoire Validée ✅
- [ ] Snapshot avant capturé
- [ ] Snapshot après capturé
- [ ] Différences détectées
- [ ] Cache coherency validée
- [ ] GTT mapping correct

---

## 🎯 LIVRABLES C578

### Fichiers à Créer

1. **forensic/execbuffer2_forensic.h** (300 lignes)
2. **forensic/execbuffer2_forensic.c** (800 lignes)
3. **forensic/ringbuffer_forensic.h** (200 lignes)
4. **forensic/ringbuffer_forensic.c** (500 lignes)
5. **forensic/batch_builder_gen9.h** (400 lignes)
6. **forensic/batch_builder_gen9.c** (1200 lignes)
7. **forensic/memory_forensic.h** (250 lignes)
8. **forensic/memory_forensic.c** (600 lignes)
9. **tests/test_c578_execbuffer2_forensic.c** (600 lignes)
10. **tests/test_c578_ringbuffer_forensic.c** (400 lignes)
11. **tests/test_c578_batch_builder.c** (700 lignes)
12. **tests/test_c578_memory_forensic.c** (500 lignes)
13. **tests/test_c578_timeline_unified.c** (450 lignes)

**Total**: ~6,900 lignes code

### Rapports à Produire

1. **RAPPORT_C578_EXECBUFFER2_ANALYSE_COMPLETE.md**
2. **RAPPORT_C578_RINGBUFFER_FORENSIC.md**
3. **RAPPORT_C578_BATCH_BUILDER_VALIDATION.md**
4. **RAPPORT_C578_MEMORY_FORENSIC_PROFOND.md**
5. **RAPPORT_C578_TIMELINE_UNIFIEE.md**
6. **RAPPORT_C578_FINAL_INTEGRATION.md**

---

## 🚨 QUESTIONS CRITIQUES À RÉSOUDRE

### 1. Le GPU exécute-t-il réellement ?
**Méthode**: Capturer seqno avant/après + vérifier completion

### 2. Les relocations sont-elles appliquées ?
**Méthode**: Lire batch après EXECBUFFER2, vérifier adresses patchées

### 3. Le kernel accède-t-il aux Surface States ?
**Méthode**: Instrumenter accès mémoire, vérifier reads GPU

### 4. Le binding table est-il valide ?
**Méthode**: Décoder binding table après relocation

### 5. Les EUs dispatchent-ils des threads ?
**Méthode**: Capturer EU occupancy via perf counters

### 6. Les writes GPU arrivent-ils en RAM ?
**Méthode**: Comparer snapshots mémoire avant/après

### 7. Le scheduler i915 modifie-t-il les timings ?
**Méthode**: Mesurer latencies avec/sans charge système

### 8. Les caches perturbent-ils la forensic ?
**Méthode**: Tester avec différents MOCS, mesurer coherency

---

## 📊 ÉTAT AVANCEMENT GLOBAL

### Avant C578
```
Élimination OpenCL:        100% ✅
Infrastructure Forensic:    30% 🔄
Batch Buffers Auto:         25% 🔄
Allocation GTT:              0% ⏳
Premier Bit GPU:             0% ⏳
```

### Après C578 (Objectif)
```
Élimination OpenCL:        100% ✅
Infrastructure Forensic:    60% 🔄
Batch Buffers Auto:         75% 🔄
Allocation GTT:            100% ✅
Premier Bit GPU:            50% 🔄
```

---

## 🎯 PRIORITÉS ABSOLUES

### Immédiat (C578)
1. **EXECBUFFER2 forensic complet**
2. **Batch builder automatique**
3. **Validation allocations GTT**
4. **Timeline unifiée CPU/GPU**

### Court Terme (C579-C580)
5. **Ringbuffer forensic profond**
6. **Memory forensic complet**
7. **EU telemetry basique**
8. **Premier nonce GPU natif**

### Moyen Terme (C581-C585)
9. **EU execution forensic avancé**
10. **Cache forensic complet**
11. **Scheduler i915 analysis**
12. **Performance optimization**

---

## 🚀 CONCLUSION

**C578 est le pivot critique** entre:
```
Forensic statique (préparation)
    ↓
Forensic dynamique (exécution)
```

**Impact attendu**:
- Déblocage allocation GTT
- Validation exécution GPU réelle
- Réponse aux 8 questions critiques
- Passage niveau "élite absolue"

**Estimation**: 3-4 cycles (C578-C581)  
**Complexité**: Très élevée  
**Priorité**: **ABSOLUE**

---

**LUMVORAX C578: TRANSITION FORENSIC STATIQUE → DYNAMIQUE** 🚀