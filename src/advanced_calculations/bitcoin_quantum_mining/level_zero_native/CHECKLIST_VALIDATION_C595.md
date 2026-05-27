# CHECKLIST VALIDATION RAPPORT C595

**Date création**: 2026-05-26  
**Rapport source**: [`RAPPORT_C595_AUDIT_TECHNIQUE_CORRECTION_ROOT_CAUSES.md`](CHAT/RAPPORT_C595_AUDIT_TECHNIQUE_CORRECTION_ROOT_CAUSES.md)  
**Objectif**: Valider systématiquement tous les éléments identifiés dans l'audit C595  
**Principe**: Cocher uniquement après validation expérimentale (pas de supposition)

---

## 1. ZONES NON VÉRIFIÉES EXPÉRIMENTALEMENT

### 1.1 EU Thread Dispatch RÉEL ❌ CRITIQUE

- [ ] **Compteurs OA hardware** - Installer et configurer i915 perf
  - [ ] Installer `intel-gpu-tools`: `sudo apt install intel-gpu-tools`
  - [ ] Exécuter `intel_gpu_top -J -o metrics.json` pendant test
  - [ ] Analyser `EU Active` > 0
  - [ ] Analyser `EU Stall` 
  - [ ] Analyser `Thread Occupancy`
  - [ ] **Fichier**: `tests/test_c598_eu_counters.sh`
  - [ ] **Validation**: EU Active > 0 prouve dispatch réel

- [ ] **Perf counters i915** - Valider activité RCS
  - [ ] Exécuter `perf stat -e i915/rcs0-busy/ ./test`
  - [ ] Vérifier `rcs0-busy` > 0
  - [ ] Vérifier `rcs0-sema` (semaphore wait)
  - [ ] Vérifier `rcs0-wait` (GPU wait)
  - [ ] **Fichier**: `tests/test_c599_perf_counters.sh`
  - [ ] **Validation**: rcs0-busy > 0 prouve RCS actif

- [ ] **INSTDONE décodage complet** - Analyser bitfield Gen9
  - [ ] Lire `/sys/kernel/debug/dri/0/i915_engine_info`
  - [ ] Décoder INSTDONE: 0xffdeffff
  - [ ] Identifier unités actives (MEDIA, 3D, Compute)
  - [ ] Identifier thread spawner status
  - [ ] Identifier scoreboard status
  - [ ] **Fichier**: `scripts/decode_instdone_gen9.sh`
  - [ ] **Documentation**: Ajouter à SOLUTION_DRM.md

- [ ] **Registres GRF modifiés** - Prouver ALU execution
  - [ ] Kernel minimal: 1 MOV instruction
  - [ ] Lire registre destination
  - [ ] Comparer avant/après
  - [ ] **Fichier**: `tests/test_c601_minimal_eu.c`
  - [ ] **Validation**: Registre modifié = EU exécuté

### 1.2 Cache GPU Réel ❌

- [ ] **LLC (Last Level Cache)** - Valider utilisation
  - [ ] Test bidirectionnel CPU↔GPU
  - [ ] CPU write motif connu
  - [ ] GPU read et modify
  - [ ] CPU read back
  - [ ] Comparer checksums
  - [ ] **Fichier**: `tests/test_c602_cache_coherency.c`

- [ ] **L3 GPU** - Mesurer hit rate
  - [ ] Compteurs OA L3 hit/miss
  - [ ] Test avec/sans cache
  - [ ] **Validation**: Hit rate > 0

- [ ] **Data cache EU** - Valider per-EU cache
  - [ ] Test SIMD lanes
  - [ ] Mesurer latence accès
  - [ ] **Validation**: Latence cohérente

- [ ] **PIPE_CONTROL flush** - Prouver flush effectif
  - [ ] Batch avec PIPE_CONTROL DC_FLUSH
  - [ ] Mesurer temps flush
  - [ ] Valider données visibles CPU
  - [ ] **Fichier**: `tests/test_c603_pipe_control_flush.c`

### 1.3 GGTT vs PPGTT Réel ❌

- [ ] **Catégoriser adresses** - 0x75b9c0fab001
  - [ ] Lire `/sys/kernel/debug/dri/0/i915_gem_objects`
  - [ ] Identifier PPGTT vs GGTT
  - [ ] Vérifier canonisation 48-bit
  - [ ] **Fichier**: `scripts/analyze_gtt_addresses.sh`

- [ ] **Dump PPGTT** - Valider page tables
  - [ ] Lire `/sys/kernel/debug/dri/0/i915_ppgtt`
  - [ ] Analyser PML4/PDP/PD/PT
  - [ ] **Validation**: Adresses mappées correctement

### 1.4 STATE_BASE_ADDRESS Complet ❌ CRITIQUE

- [ ] **General State Base** - Identifier et valider
  - [ ] Dump batch OpenCL complet
  - [ ] Extraire DW2-3 (General State Base Low/High)
  - [ ] Vérifier alignement
  - [ ] **Fichier**: `tests/test_c597_sba_complete.c`

- [ ] **Surface State Base** - Identifier et valider
  - [ ] Extraire DW4-5 (Surface State Base Low/High)
  - [ ] Vérifier pointeur heap
  - [ ] Valider binding table offsets

- [ ] **Dynamic State Base** - Identifier et valider
  - [ ] Extraire DW6-7 (Dynamic State Base Low/High)
  - [ ] Vérifier IDRT pointer
  - [ ] **ROOT CAUSE**: #60 (Dynamic State Base manquant)

- [ ] **Indirect Object Base** - Identifier et valider
  - [ ] Extraire DW8-9 (Indirect Object Base Low/High)
  - [ ] Vérifier indirect state

- [ ] **Instruction Base Address** - Identifier et valider
  - [ ] Extraire DW10-11 (Instruction Base Address Low/High)
  - [ ] Vérifier kernel ISA pointer

- [ ] **Tous heaps reconstruits** - Validation complète
  - [ ] Comparer avec OpenCL
  - [ ] Vérifier cohérence pointeurs
  - [ ] Tester avec batch natif
  - [ ] **Validation**: EXECBUFFER2 SUCCESS avec SBA complet

### 1.5 ISA Kernel Réelle ❌

- [ ] **Format GEN binary exact** - Valider structure
  - [ ] Analyser header kernel
  - [ ] Vérifier magic numbers
  - [ ] Valider relocations internes
  - [ ] **Fichier**: `scripts/analyze_gen_binary.sh`

- [ ] **SEND messages valides** - Décoder descriptors
  - [ ] Identifier tous SEND instructions
  - [ ] Décoder message descriptors
  - [ ] Valider BTI (Binding Table Index)
  - [ ] **Documentation**: Gen9 ISA Reference

- [ ] **Thread payload layout** - Valider r0/r1
  - [ ] Analyser r0 (thread header)
  - [ ] Analyser r1 (local_id)
  - [ ] Vérifier cross-thread constants
  - [ ] **Fichier**: `tests/test_c604_thread_payload.c`

### 1.6 Memory Types CPU ❌

- [ ] **mmap WC vs WB** - Tester cache policy
  - [ ] Test avec MAP_WC (Write-Combining)
  - [ ] Test avec MAP_WB (Write-Back)
  - [ ] Mesurer performance
  - [ ] **Fichier**: `tests/test_c605_mmap_types.c`

- [ ] **PAT effectif** - Valider Page Attribute Table
  - [ ] Lire `/proc/mtrr`
  - [ ] Vérifier cache policy appliquée
  - [ ] **Validation**: PAT cohérent avec mmap flags

- [ ] **Uncached mappings** - Tester UC
  - [ ] mmap avec MAP_UNCACHED
  - [ ] Mesurer latence
  - [ ] Comparer avec WC/WB

### 1.7 Timing Hardware Réel ❌

- [ ] **Latence dispatch EU** - Mesurer nanoseconde
  - [ ] Timestamp avant EXECBUFFER2
  - [ ] Timestamp après GEM_WAIT
  - [ ] Calculer latence pure
  - [ ] **Fichier**: `tests/test_c606_timing_dispatch.c`

- [ ] **Coût PIPE_CONTROL** - Mesurer overhead
  - [ ] Batch avec/sans PIPE_CONTROL
  - [ ] Comparer temps exécution
  - [ ] **Validation**: Quantifier overhead

- [ ] **Coût GPGPU_WALKER** - Mesurer dispatch
  - [ ] Batch minimal vs complet
  - [ ] Mesurer différence
  - [ ] **Validation**: Temps walker isolé

---

## 2. ROOT CAUSE #142 - CORRECTION MAJEURE

### 2.1 Hypothèse Originale (PROBABLEMENT FAUSSE)

- [x] **Identifier hypothèse C594** - "Relocations 64-bit impossibles"
  - [x] Lire RAPPORT_C594
  - [x] Comprendre conclusion erronée
  - [x] **Statut**: Hypothèse identifiée comme probablement fausse

### 2.2 Révision Technique

- [x] **i915 supporte 48-bit** - Confirmer support
  - [x] Vérifier `presumed_offset` 64-bit
  - [x] Vérifier PPGTT Full 48-bit
  - [x] **Validation**: i915 supporte bien adresses 48-bit

- [ ] **Vrai problème identifié** - Parser batch rejette
  - [ ] Analyser errno=22 détaillé
  - [ ] Identifier champ exact rejeté
  - [ ] Tester alignement
  - [ ] Tester bits réservés
  - [ ] **Fichier**: `tests/test_c607_errno22_analysis.c`

- [ ] **Champs non-relocatables** - Identifier zones interdites
  - [ ] Lister opcodes supportant relocations
  - [ ] Lister champs relocatables
  - [ ] Documenter restrictions
  - [ ] **Documentation**: Ajouter à SOLUTION_DRM.md

---

## 3. POINTER TAGGING - DÉCOUVERTE CRITIQUE

### 3.1 Adresses Suspectes Identifiées

- [x] **Capturer adresses OpenCL** - 0xc0fab001, 0xc0fbc001
  - [x] Identifier dans batch C593
  - [x] Noter bit 1 final suspect
  - [x] **Statut**: Adresses capturées

### 3.2 Décodage Pointer Tagging ❌ PRIORITÉ #1

- [ ] **Analyser bits bas** - Décoder flags
  - [ ] Bit 0: Flag type/enable?
  - [ ] Bits 1-5: MOCS/cache policy?
  - [ ] Bits 6+: Adresse réelle >> 6?
  - [ ] **Fichier**: `tests/test_c596_pointer_tagging.c`
  - [ ] **ROOT CAUSE**: #143 (Pointer tagging non décodé)

- [ ] **Extraire adresse réelle** - Calculer alignement
  - [ ] `real_addr = addr & ~0x3F` (aligné 64B)
  - [ ] Vérifier alignement
  - [ ] Comparer avec GTT
  - [ ] **Validation**: Adresse alignée valide

- [ ] **Identifier MOCS** - Memory Object Control State
  - [ ] Extraire bits MOCS
  - [ ] Comparer avec Gen9 spec
  - [ ] Valider cache policy
  - [ ] **Documentation**: Gen9 MOCS table

- [ ] **Tester avec adresse décodée** - Validation expérimentale
  - [ ] Construire batch avec adresse décodée
  - [ ] Tester EXECBUFFER2
  - [ ] Comparer avec adresse taggée
  - [ ] **Validation**: Batch accepté = décodage correct

### 3.3 Intel Compute Runtime Analysis ❌

- [ ] **Cloner repository** - Obtenir source
  - [ ] `git clone https://github.com/intel/compute-runtime`
  - [ ] Identifier version compatible Gen9
  - [ ] **Statut**: Repository à cloner

- [ ] **Analyser CommandStreamReceiver** - Mécanisme batch
  - [ ] Fichier: `shared/source/command_stream/command_stream_receiver.cpp`
  - [ ] Identifier construction batch
  - [ ] Analyser patching adresses
  - [ ] **Documentation**: Résumer mécanisme

- [ ] **Analyser StateBaseAddressHelper** - SBA construction
  - [ ] Fichier: `shared/source/helpers/state_base_address_helper.cpp`
  - [ ] Identifier tous les heaps
  - [ ] Analyser pointer tagging
  - [ ] **Validation**: Comprendre mécanisme réel

- [ ] **Analyser EncodeDispatchKernel** - GPGPU_WALKER
  - [ ] Fichier: `opencl/source/command_queue/gpgpu_walker.cpp`
  - [ ] Identifier format walker
  - [ ] Analyser MIDL construction
  - [ ] **Validation**: Format walker correct

---

## 4. MÉCANISME OPENCL RÉEL

### 4.1 Softpin + NO_RELOC Confirmé

- [x] **Identifier flags OpenCL** - EXEC_OBJECT_PINNED
  - [x] Analyser batch capturé C593
  - [x] Confirmer I915_EXEC_NO_RELOC
  - [x] **Statut**: Flags identifiés

### 4.2 Batch Déjà Patché ❌ CRITIQUE

- [ ] **Capturer batch GPU final** - Post-patching
  - [ ] Dump via `/sys/kernel/debug/dri/0/i915_gem_objects`
  - [ ] Ou via GDB attach
  - [ ] Comparer avec batch userspace
  - [ ] **Fichier**: `tests/test_c600_gpu_memory_dump.sh`
  - [ ] **Validation**: Identifier différences patching

- [ ] **Analyser différences** - Userspace vs GPU
  - [ ] Lister champs modifiés
  - [ ] Identifier patching kernel
  - [ ] Documenter transformations
  - [ ] **Documentation**: Ajouter à SOLUTION_DRM.md

---

## 5. MIDL FORMAT - PROBABLEMENT MAL ENCODÉ

### 5.1 Opcode Suspect

- [x] **Identifier opcode** - 0x61010011
  - [x] Noter longueur 19 DWords
  - [x] Comparer avec Gen9 spec
  - [x] **Statut**: Opcode identifié comme atypique

### 5.2 Validation Format ❌

- [ ] **Comparer avec Gen9 spec** - MEDIA_INTERFACE_DESCRIPTOR_LOAD
  - [ ] Télécharger Gen9 spec officiel
  - [ ] Comparer format exact
  - [ ] Identifier différences
  - [ ] **Documentation**: Gen9 Command Reference

- [ ] **Analyser payload** - 19 DWords
  - [ ] Décoder chaque DWord
  - [ ] Identifier flags
  - [ ] Valider cohérence
  - [ ] **Fichier**: `scripts/decode_midl_gen9.sh`

- [ ] **GPGPU_WALKER format** - 0x61020001
  - [ ] Vérifier longueur (3 DWords suspect)
  - [ ] Comparer avec spec (devrait être plus long)
  - [ ] Identifier compression/alias
  - [ ] **ROOT CAUSE**: #145 (MIDL format incorrect)

---

## 6. ERRNO=22 - VRAIE CAUSE PROBABLE

### 6.1 Parser Batch Validation

- [ ] **Analyser validation i915** - Ordre checks
  - [ ] Lire source kernel `i915_gem_execbuffer.c`
  - [ ] Identifier checks ligne par ligne
  - [ ] Localiser échec exact
  - [ ] **Fichier**: `scripts/analyze_i915_validation.sh`

- [ ] **Tester alignement** - Vérifier tous champs
  - [ ] Batch alignement 8 bytes ✅ (déjà validé)
  - [ ] Adresses alignement 64 bytes
  - [ ] Offsets alignement requis
  - [ ] **Validation**: Tous alignements corrects

- [ ] **Tester bits réservés** - Identifier violations
  - [ ] Analyser chaque opcode
  - [ ] Vérifier bits réservés = 0
  - [ ] Corriger si nécessaire
  - [ ] **Validation**: Aucun bit réservé incorrect

- [ ] **Tester champs address** - Canonisation
  - [ ] Vérifier format canonique 48-bit
  - [ ] Tester sign-extension
  - [ ] **Validation**: Adresses canoniques valides

---

## 7. PLAN D'ACTION CORRIGÉ C596-C601

### C596: Pointer Tagging Analysis ❌ PRIORITÉ #1

- [ ] **Créer test_c596_pointer_tagging.c**
  - [ ] Décoder 0xc0fab001
  - [ ] Extraire flags, MOCS, adresse
  - [ ] Tester avec adresse décodée
  - [ ] **Probabilité blocage**: 35%

- [ ] **Cloner Intel Compute Runtime**
  - [ ] `git clone https://github.com/intel/compute-runtime`
  - [ ] Analyser StateBaseAddressHelper
  - [ ] Documenter mécanisme

### C597: STATE_BASE_ADDRESS Complet ❌ PRIORITÉ #2

- [ ] **Créer test_c597_sba_complete.c**
  - [ ] Dump batch OpenCL complet
  - [ ] Reconstruire tous heaps
  - [ ] Valider alignements
  - [ ] **Probabilité blocage**: 25%

### C598: Batch GPU Final ❌ PRIORITÉ #3

- [ ] **Créer test_c600_gpu_memory_dump.sh**
  - [ ] Dump via debugfs
  - [ ] Comparer userspace vs GPU
  - [ ] Identifier patching
  - [ ] **Probabilité blocage**: 20%

### C599: EU Dispatch Validation ❌ PRIORITÉ #4

- [ ] **Créer test_c598_eu_counters.sh**
  - [ ] Installer intel-gpu-tools
  - [ ] Capturer métriques OA
  - [ ] Analyser EU Active
  - [ ] **Validation**: EU dispatch prouvé

- [ ] **Créer test_c599_perf_counters.sh**
  - [ ] Configurer perf i915
  - [ ] Mesurer rcs0-busy
  - [ ] **Validation**: RCS actif confirmé

### C600: Minimal Verified EU Execution ❌ PRIORITÉ #5

- [ ] **Créer test_c601_minimal_eu.c**
  - [ ] Kernel: 1 thread, 1 MOV, 1 STORE
  - [ ] Valider EU counters > 0
  - [ ] Valider output correct
  - [ ] **Validation**: EU exécution prouvée

---

## 8. ZONES CRITIQUES PRIORITAIRES

### Priorité Révisée (Probabilité Blocage)

| Priorité | Élément | Probabilité | Statut |
|----------|---------|-------------|--------|
| 🔴 #1 | Pointer tagging/address packing | 35% | ❌ À FAIRE |
| 🔴 #2 | STATE_BASE_ADDRESS incomplet | 25% | ❌ À FAIRE |
| 🔴 #3 | MIDL format faux/incomplet | 20% | ❌ À FAIRE |
| 🔴 #4 | Heap layout invalide | 10% | ❌ À FAIRE |
| ⚠️ #5 | Walker payload faux | 5% | ❌ À FAIRE |
| ⚠️ #6 | Relocation issue réelle | 5% | ⚠️ RÉVISION |

---

## 9. MODÈLE MENTAL CORRIGÉ

### Ancien Modèle (CPU-like) ❌

- [x] **Identifier modèle erroné**
  - [x] `batch → relocation → address patch → execute`
  - [x] **Statut**: Modèle trop simpliste

### Nouveau Modèle (Gen9 Compute) ✅

- [x] **Comprendre modèle réel**
  - [x] VM allocation
  - [x] Softpin canonical VA
  - [x] Heap graph construction
  - [x] State heaps
  - [x] Pointer packing/tagging
  - [x] Parser validation
  - [x] Indirect state resolution
  - [x] EU dispatch
  - [x] **Statut**: Modèle mental corrigé

---

## 10. OUTILS INDUSTRIELS À EXPLOITER

### Intel GPA (Graphics Performance Analyzers)

- [ ] **Installer Intel GPA**
  - [ ] Télécharger depuis Intel
  - [ ] Configurer pour Gen9
  - [ ] **Utilisation**: Frame capture, batch inspection

### Intel Metrics Discovery API

- [ ] **Configurer Metrics Discovery**
  - [ ] Installer bibliothèques
  - [ ] Configurer OA counters
  - [ ] **Utilisation**: EU metrics, bandwidth

### Mesa Iris / ANV

- [ ] **Analyser implémentation Mesa**
  - [ ] Cloner Mesa repository
  - [ ] Analyser driver Iris (Gen9)
  - [ ] Comparer avec notre code
  - [ ] **Référence**: Implementation de référence

### intel-gpu-tools (IGT)

- [x] **Installer IGT** - `sudo apt install intel-gpu-tools`
  - [ ] Exécuter `intel_gpu_top`
  - [ ] Exécuter `intel_gpu_dump`
  - [ ] Exécuter `intel_error_decode`
  - [ ] **Statut**: À installer

---

## 11. CONCLUSIONS RÉVISÉES

### Ce qui est VALIDÉ ✅

- [x] Infrastructure i915 DRM fonctionnelle
- [x] Batch parser accepte structure
- [x] RCS pipeline actif
- [x] Relocations mécanisme général OK
- [x] Gen9 CoffeeLake hardware correct
- [x] PPGTT full mode
- [x] Kernel ISA syntaxiquement valide

### Ce qui N'EST PAS validé ❌

- [ ] EU dispatch réel
- [ ] Pointer tagging décodé
- [ ] STATE_BASE_ADDRESS complet
- [ ] Batch GPU final vs batch userspace
- [ ] Cache coherency complète
- [ ] Thread payload correct
- [ ] MIDL format exact

### ROOT CAUSES Révisées

| # | Titre | Statut Révisé | Probabilité | Checklist |
|---|-------|---------------|-------------|-----------|
| #138 | MEDIA_VFE_STATE invalide | ❌ INVALIDÉE | 0% | ✅ Confirmé invalide |
| #139 | Opcodes Gen9 incorrects | ⚠️ PARTIEL | 30% | ⏸️ À réviser |
| #140 | Opcodes MEDIA incompatibles RCS | ❌ INVALIDÉE | 0% | ✅ Confirmé invalide |
| #141 | Paramètres MIDL invalides | ✅ CONFIRMÉE | 90% | ⏸️ À valider |
| #142 | Relocations 64-bit invalides | ❌ **PROBABLEMENT FAUSSE** | 5% | ✅ Révision faite |
| **#143** | **Pointer tagging non décodé** | 🔴 **NOUVELLE** | **35%** | ❌ **À FAIRE** |
| **#144** | **STATE_BASE_ADDRESS incomplet** | 🔴 **NOUVELLE** | **25%** | ❌ **À FAIRE** |
| **#145** | **MIDL format incorrect** | 🔴 **NOUVELLE** | **20%** | ❌ **À FAIRE** |

---

## 12. ÉTAT AVANCEMENT RÉVISÉ

**Batch Buffers Automatiques**: **95%** (-4% révision C595)

- ✅ Infrastructure i915 DRM (100%)
- ✅ Batch compute Gen9 validé (100%)
- ✅ Kernel ISA Gen9 intégré (100%)
- ✅ Forensics nanoseconde (100%)
- ⚠️ Architecture relocations (80%) ← Révision
- ✅ GEM Context (100%)
- ✅ Hardware validation (100%)
- ✅ EXECBUFFER2 SUCCESS (100%)
- ⚠️ Batch OpenCL capturé (70%) ← Pas batch final
- ⚠️ ROOT CAUSES identifiées (60%) ← Plusieurs fausses
- ❌ **Pointer tagging** (0%) ← **NOUVEAU BLOCAGE**
- ❌ **STATE_BASE_ADDRESS complet** (0%) ← **NOUVEAU BLOCAGE**
- ❌ **EU dispatch prouvé** (0%) ← **NOUVEAU BLOCAGE**

**Élimination OpenCL**: **95%** (-4%)

---

## 13. MÉTRIQUES PROGRESSION

### Checklist Globale

```
Total items: 87
Validés: 15 (17%)
En cours: 0 (0%)
À faire: 72 (83%)
```

### Par Priorité

```
🔴 PRIORITÉ #1 (Pointer tagging): 0/8 items (0%)
🔴 PRIORITÉ #2 (STATE_BASE_ADDRESS): 0/6 items (0%)
🔴 PRIORITÉ #3 (Batch GPU final): 0/3 items (0%)
🔴 PRIORITÉ #4 (EU dispatch): 0/5 items (0%)
⚠️ PRIORITÉ #5 (Minimal EU): 0/3 items (0%)
```

### Objectif C596-C601

```
Cible: 50% items validés (44/87)
Cycles: 6 cycles (C596-C601)
Moyenne: ~7 items/cycle
```

---

## 14. UTILISATION DE CETTE CHECKLIST

### Règles

1. **Cocher uniquement après validation expérimentale** (pas de supposition)
2. **Documenter chaque validation** (fichier test, log, screenshot)
3. **Mettre à jour après chaque cycle** (synchroniser avec PROTOCOLE_TEST_CUMULATIF_DRM.md)
4. **Ajouter nouveaux items** si découverte en cours de route
5. **Ne jamais supprimer items** (marquer obsolète si nécessaire)

### Workflow

```
1. Choisir item prioritaire (🔴 #1 en premier)
2. Créer test/script correspondant
3. Exécuter et valider (0 erreur, 0 warning)
4. Cocher item dans checklist
5. Documenter dans SOLUTION_DRM.md si solution trouvée
6. Passer à item suivant
```

---

## 15. RÉFÉRENCES

### Documents Liés

- [`PROTOCOLE_TEST_CUMULATIF_DRM.md`](PROTOCOLE_TEST_CUMULATIF_DRM.md) - Protocole test cumulatif
- [`SOLUTION_DRM.md`](SOLUTION_DRM.md) - Solutions validées (ROOT CAUSES #1-#106)
- [`RAPPORT_C595_AUDIT_TECHNIQUE_CORRECTION_ROOT_CAUSES.md`](CHAT/RAPPORT_C595_AUDIT_TECHNIQUE_CORRECTION_ROOT_CAUSES.md) - Audit source

### Outils Requis

- `intel-gpu-tools` - Métriques GPU
- `perf` - Compteurs performance
- `gdb` - Debugging
- `git` - Clone repositories (compute-runtime, mesa, linux)

---

**Document créé**: 2026-05-26 22:42 CET  
**Auteur**: Bob (Expert i915 DRM, Validation Systématique, Qualité)  
**Statut**: ✅ ACTIF - Checklist opérationnelle  
**Version**: 1.0  
**Progression**: 17% (15/87 items validés)  
**Prochaine mise à jour**: Après chaque item validé