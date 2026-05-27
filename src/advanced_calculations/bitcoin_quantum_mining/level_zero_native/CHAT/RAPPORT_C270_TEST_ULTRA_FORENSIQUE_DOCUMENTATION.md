# RAPPORT C270 — TEST ULTRA-FORENSIQUE NANO BIT-LEVEL
## Documentation Complète du Test d'Identification Bug errno=5

**Date**: 2026-05-18  
**Cycle**: C270  
**Objectif**: Identifier la cause racine exacte du bug errno=5 après 27 dispatches  
**Auteur**: Bob (LumVorax AI Assistant)  
**Device**: Intel UHD Graphics 620 (Gen9)  
**Architecture**: UMA (Unified Memory Architecture)

---

## 1. CONTEXTE ET PROBLÉMATIQUE

### 1.1 Historique du Bug

**Symptôme observé** (Sessions C264-C269):
- Mining Bitcoin GPU natif i915 DRM fonctionne correctement
- Hashrate peak mesuré: **408.76 MH/s**
- **Bug critique**: GPU hang après **27 dispatches** avec `errno=5` (EIO)
- 8.67 milliards de hashes calculés avant crash

**Workaround actuel** (Session C269):
- Reopen DRM périodique tous les 9 dispatches
- Permet d'atteindre 62 dispatches sans crash
- Hashrate: 373.4 MH/s
- **PROBLÈME**: Contourne le bug mais ne l'identifie PAS

### 1.2 Demande Utilisateur Explicite

> "le probleme est toujour la, n'est toujour pas identifier !! ce que je veux decouvrir, c'est quesqu'il produit reellement ce blocage Le bug errno=5 ! je veux savoir ce quil ce produite a linterreur meme du dispach avant, durant et apres qui cause le reel probleme"

**Exigences**:
1. Identifier la **cause racine exacte** du bug errno=5
2. Décomposition **chirurgicale seconde par seconde**
3. Analyser **toutes les couches système** (hardware → application)
4. **Memory tracker ultra-granulaire maximal** (GPU utilise RAM système UMA)
5. Capturer le **moment exact** où le bug se produit

---

## 2. STRATÉGIE TEST C270

### 2.1 Approche Ultra-Forensique

**Principe**: Forcer la reproduction du bug en désactivant le workaround, puis capturer l'état complet du système à chaque étape.

**Modifications par rapport au code baseline**:

1. **DÉSACTIVER reopen DRM** (ligne 1135 de [`btc_gen9_native_runner.c`](../src/btc_gen9_native_runner.c:1135))
   ```c
   /* C270 TEST: DÉSACTIVER reopen pour forcer reproduction bug errno=5 */
   if (0 && ctx->total_dispatches % BATCH_POOL_SIZE == 0 && ...) {
   ```

2. **ACTIVER tracking ultra-granulaire maximal**
   - Memory tracker RAM système (UMA)
   - État DRM complet
   - Métriques hardware GPU
   - Timestamps nanoseconde

3. **CAPTURER snapshots système**
   - AVANT chaque dispatch
   - APRÈS chaque dispatch
   - Analyser différences (delta)

### 2.2 Architecture du Test

```
┌─────────────────────────────────────────────────────────────┐
│  TEST C270 ULTRA-FORENSIQUE                                 │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  1. Initialisation Contexte Forensique             │   │
│  │     - Ouvrir fichiers logs (4 fichiers)            │   │
│  │     - Activer tous les trackings                   │   │
│  └─────────────────────────────────────────────────────┘   │
│                          │                                  │
│                          ▼                                  │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  2. Initialisation GPU (btc_gen9_init)             │   │
│  │     - Ouvrir DRM /dev/dri/renderD128               │   │
│  │     - Créer 9 contextes i915                       │   │
│  │     - Allouer 90 batch buffers                     │   │
│  │     - Charger kernel SHA-256 Gen9                  │   │
│  └─────────────────────────────────────────────────────┘   │
│                          │                                  │
│                          ▼                                  │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  3. Boucle Dispatches (jusqu'au crash)             │   │
│  │                                                     │   │
│  │     Pour chaque dispatch (i = 1..30):              │   │
│  │                                                     │   │
│  │     ┌───────────────────────────────────────────┐  │   │
│  │     │  3.1 Capturer Snapshot AVANT              │  │   │
│  │     │      - Timestamp nanoseconde              │  │   │
│  │     │      - État RAM (total, free, available) │  │   │
│  │     │      - Fréquence GPU (MHz)                │  │   │
│  │     │      - Température GPU (°C)               │  │   │
│  │     │      - État DRM (fd, ctx_id, handles)    │  │   │
│  │     │      - Compteurs (dispatches, usage)     │  │   │
│  │     └───────────────────────────────────────────┘  │   │
│  │                          │                          │   │
│  │                          ▼                          │   │
│  │     ┌───────────────────────────────────────────┐  │   │
│  │     │  3.2 Exécuter Dispatch GPU                │  │   │
│  │     │      - Construire batch buffer            │  │   │
│  │     │      - Soumettre via EXECBUFFER2          │  │   │
│  │     │      - Attendre completion (GEM_WAIT)     │  │   │
│  │     └───────────────────────────────────────────┘  │   │
│  │                          │                          │   │
│  │                          ▼                          │   │
│  │     ┌───────────────────────────────────────────┐  │   │
│  │     │  3.3 Capturer Snapshot APRÈS              │  │   │
│  │     │      (mêmes métriques que AVANT)          │  │   │
│  │     └───────────────────────────────────────────┘  │   │
│  │                          │                          │   │
│  │                          ▼                          │   │
│  │     ┌───────────────────────────────────────────┐  │   │
│  │     │  3.4 Analyser Delta                       │  │   │
│  │     │      - RAM consommée                      │  │   │
│  │     │      - Changement fréquence GPU           │  │   │
│  │     │      - Détection anomalies                │  │   │
│  │     └───────────────────────────────────────────┘  │   │
│  │                          │                          │   │
│  │                          ▼                          │   │
│  │     ┌───────────────────────────────────────────┐  │   │
│  │     │  3.5 Vérifier Résultat                    │  │   │
│  │     │      - Si errno=5 → CRASH DÉTECTÉ         │  │   │
│  │     │      - Logger état complet bug            │  │   │
│  │     │      - Arrêter test                       │  │   │
│  │     └───────────────────────────────────────────┘  │   │
│  │                                                     │   │
│  └─────────────────────────────────────────────────────┘   │
│                          │                                  │
│                          ▼                                  │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  4. Cleanup et Analyse                              │   │
│  │     - Fermer contexte GPU                           │   │
│  │     - Fermer fichiers logs                          │   │
│  │     - Générer rapport analyse                       │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 3. IMPLÉMENTATION TECHNIQUE

### 3.1 Fichiers Créés

#### 3.1.1 [`btc_gen9_native_runner_test_c270_ultra_forensic.c`](../src/btc_gen9_native_runner_test_c270_ultra_forensic.c) (598 lignes)

**Structures de données**:

```c
typedef struct {
    /* Timestamp nanoseconde */
    uint64_t timestamp_ns;
    
    /* Hardware GPU */
    int gpu_freq_mhz;
    int gpu_temp_celsius;
    
    /* Mémoire RAM système (UMA) */
    uint64_t ram_total_bytes;
    uint64_t ram_free_bytes;
    uint64_t ram_available_bytes;
    uint64_t ram_buffers_bytes;
    uint64_t ram_cached_bytes;
    
    /* Mémoire GPU (via i915 DRM) */
    uint64_t gpu_mem_total_bytes;
    uint64_t gpu_mem_used_bytes;
    uint64_t gpu_mem_free_bytes;
    
    /* État DRM */
    int drm_fd;
    uint32_t ctx_id_active;
    int ctx_pool_index;
    int batch_pool_index;
    
    /* Handles GEM */
    uint32_t kernel_bo;
    uint32_t input_bo;
    uint32_t output_bo;
    uint32_t batch_bo_current;
    
    /* Compteurs dispatches */
    uint64_t total_dispatches;
    int ctx_usage_count[9];
    
    /* Métriques performance */
    double exec_time_sec;
    double hashrate_mhs;
    
    /* Flags état */
    int in_batch_mode;
    int thermal_throttle_detected;
    
} c270_system_snapshot_t;
```

**Fonctions clés**:

1. **`c270_capture_ram_state()`**: Lit `/proc/meminfo` pour état RAM UMA
2. **`c270_read_gpu_frequency()`**: Lit `/sys/class/drm/card0/gt_cur_freq_mhz`
3. **`c270_read_gpu_temperature()`**: Lit hwmon pour température GPU
4. **`c270_capture_system_snapshot()`**: Capture snapshot complet système
5. **`c270_log_snapshot()`**: Logger snapshot dans fichiers forensiques
6. **`c270_btc_gen9_execute_wrapper()`**: Wrapper dispatch avec tracking

#### 3.1.2 [`run_test_c270_ultra_forensic.sh`](../run_test_c270_ultra_forensic.sh) (95 lignes)

**Script automatisé**:
- Compilation du test
- Exécution avec capture logs
- Analyse automatique résultats
- Génération rapport résumé

### 3.2 Modifications Code Baseline

#### 3.2.1 [`btc_gen9_native_runner.c`](../src/btc_gen9_native_runner.c:1135) (ligne 1135)

**AVANT** (avec workaround):
```c
if (ctx->total_dispatches % BATCH_POOL_SIZE == 0 && ctx->total_dispatches > 0 && !ctx->in_batch_mode) {
    LOG_EVENT(ctx, "DRM_REOPEN_TRIGGER: dispatches=%lu (multiple de %d)", ctx->total_dispatches, BATCH_POOL_SIZE);
    // ... code reopen DRM ...
}
```

**APRÈS** (C270 test):
```c
/* C270 TEST: DÉSACTIVER reopen pour forcer reproduction bug errno=5 */
if (0 && ctx->total_dispatches % BATCH_POOL_SIZE == 0 && ctx->total_dispatches > 0 && !ctx->in_batch_mode) {
    LOG_EVENT(ctx, "DRM_REOPEN_TRIGGER_DISABLED_C270: dispatches=%lu (reopen disabled for bug reproduction)", ctx->total_dispatches);
    // ... code reopen DRM (jamais exécuté) ...
}
```

**Impact**: Le reopen DRM ne sera JAMAIS déclenché → Bug errno=5 se reproduira après 27 dispatches

---

## 4. LOGS FORENSIQUES GÉNÉRÉS

### 4.1 Structure Répertoire Logs

```
logs/forensic/c270_ultra/
├── ultra_forensic.log          # Log principal (snapshots + analyse)
├── memory_tracking.log         # Tracking mémoire RAM/GPU détaillé
├── drm_state.log              # État DRM (contextes, handles, etc.)
└── hardware_state.log         # État hardware (freq, temp, voltage)
```

### 4.2 Format Snapshot

**Exemple snapshot AVANT dispatch**:
```
[C270_SNAPSHOT_BEFORE_DISPATCH] timestamp=1234567890.123456789
  HARDWARE: gpu_freq=1150 MHz gpu_temp=65°C
  RAM_UMA: total=15872 MB free=8234 MB available=10456 MB buffers=512 MB cached=2048 MB
  DRM: fd=3 ctx_active=5 ctx_pool_idx=4 batch_pool_idx=26
  GEM_HANDLES: kernel_bo=2 input_bo=3 output_bo=4 batch_bo=28
  COUNTERS: total_dispatches=26
  CTX_USAGE: [3,3,3,3,3,3,3,3,2]
  FLAGS: in_batch_mode=0 thermal_throttle=0
```

**Exemple snapshot APRÈS dispatch**:
```
[C270_SNAPSHOT_AFTER_DISPATCH] timestamp=1234567890.823456789
  HARDWARE: gpu_freq=1150 MHz gpu_temp=66°C
  RAM_UMA: total=15872 MB free=8230 MB available=10452 MB buffers=512 MB cached=2048 MB
  DRM: fd=3 ctx_active=6 ctx_pool_idx=5 batch_pool_idx=27
  GEM_HANDLES: kernel_bo=2 input_bo=3 output_bo=4 batch_bo=29
  COUNTERS: total_dispatches=27
  CTX_USAGE: [3,3,3,3,3,3,3,3,3]
  FLAGS: in_batch_mode=0 thermal_throttle=0
```

**Analyse delta**:
```
[C270_DELTA] ram_consumed=4194304 bytes freq_change=0 MHz result=0
```

### 4.3 Détection Bug errno=5

**Quand le bug se produit** (dispatch 28 attendu):
```
[C270_SNAPSHOT_BEFORE_DISPATCH] timestamp=1234567891.523456789
  HARDWARE: gpu_freq=1150 MHz gpu_temp=66°C
  RAM_UMA: total=15872 MB free=8226 MB available=10448 MB buffers=512 MB cached=2048 MB
  DRM: fd=3 ctx_active=7 ctx_pool_idx=6 batch_pool_idx=28
  GEM_HANDLES: kernel_bo=2 input_bo=3 output_bo=4 batch_bo=30
  COUNTERS: total_dispatches=27
  CTX_USAGE: [3,3,3,3,3,3,3,3,3]
  FLAGS: in_batch_mode=0 thermal_throttle=0

[C270_ANOMALY_DETECTED] dispatch_failed errno=5
[C270_CRITICAL] This is the bug! Analyzing state...

[C270_BUG_STATE_DUMP]
  dispatch_number=27
  ctx_pool_index=6
  batch_pool_index=28
  ram_free=8226 MB
  gpu_freq=1150 MHz
```

---

## 5. MÉTRIQUES CAPTURÉES

### 5.1 Couche Hardware

| Métrique | Source | Fréquence | Unité |
|----------|--------|-----------|-------|
| Fréquence GPU | `/sys/class/drm/card0/gt_cur_freq_mhz` | Chaque dispatch | MHz |
| Température GPU | `/sys/class/drm/card0/device/hwmon/hwmon*/temp1_input` | Chaque dispatch | °C |
| Voltage GPU | `/sys/class/drm/card0/gt_RP*_freq_mhz` | Chaque dispatch | mV |

### 5.2 Couche Mémoire (UMA)

| Métrique | Source | Fréquence | Unité |
|----------|--------|-----------|-------|
| RAM totale | `/proc/meminfo` (MemTotal) | Chaque dispatch | bytes |
| RAM libre | `/proc/meminfo` (MemFree) | Chaque dispatch | bytes |
| RAM disponible | `/proc/meminfo` (MemAvailable) | Chaque dispatch | bytes |
| Buffers | `/proc/meminfo` (Buffers) | Chaque dispatch | bytes |
| Cache | `/proc/meminfo` (Cached) | Chaque dispatch | bytes |

### 5.3 Couche DRM

| Métrique | Source | Fréquence | Unité |
|----------|--------|-----------|-------|
| File descriptor | `ctx->drm_fd` | Chaque dispatch | int |
| Context ID actif | `ctx->ctx_pool[ctx->ctx_pool_index]` | Chaque dispatch | uint32_t |
| Index pool contextes | `ctx->ctx_pool_index` | Chaque dispatch | int |
| Index pool batch | `ctx->batch_pool_index` | Chaque dispatch | int |
| Handle kernel BO | `ctx->kernel_bo` | Chaque dispatch | uint32_t |
| Handle input BO | `ctx->input_bo` | Chaque dispatch | uint32_t |
| Handle output BO | `ctx->output_bo` | Chaque dispatch | uint32_t |
| Handle batch BO | `ctx->batch_bo_pool[batch_pool_index]` | Chaque dispatch | uint32_t |

### 5.4 Couche Application

| Métrique | Source | Fréquence | Unité |
|----------|--------|-----------|-------|
| Total dispatches | `ctx->total_dispatches` | Chaque dispatch | uint64_t |
| Usage contextes | `ctx->ctx_usage_count[0..8]` | Chaque dispatch | int[9] |
| Mode batch | `ctx->in_batch_mode` | Chaque dispatch | bool |
| Temps exécution | `exec_time_sec` | Chaque dispatch | seconds |
| Hashrate | `hashrate_mhs` | Chaque dispatch | MH/s |

---

## 6. ANALYSE ATTENDUE

### 6.1 Hypothèses à Vérifier

**Hypothèse 1**: Accumulation requests GPU non retired
- **Test**: Comparer `ctx_usage_count` avant/après crash
- **Attendu**: Tous contextes à 3 cycles (limite Gen9)

**Hypothèse 2**: Memory leak RAM système (UMA)
- **Test**: Analyser delta `ram_free` sur 27 dispatches
- **Attendu**: Consommation linéaire ou accumulation

**Hypothèse 3**: État DRM corrompu
- **Test**: Vérifier handles GEM valides avant crash
- **Attendu**: Handles cohérents ou corruption détectée

**Hypothèse 4**: Thermal throttling
- **Test**: Vérifier `gpu_freq` et `gpu_temp` avant crash
- **Attendu**: Fréquence stable ou throttling détecté

### 6.2 Patterns à Identifier

1. **Pattern temporel**: Crash toujours après exactement 27 dispatches ?
2. **Pattern mémoire**: Fuite mémoire progressive ou soudaine ?
3. **Pattern contextes**: Rotation contextes correcte ou blocage ?
4. **Pattern hardware**: Dégradation performance ou stable ?

### 6.3 Couche Système Responsable

**Identification par élimination**:

```
SI ram_free stable ET gpu_freq stable ET ctx_usage_count correct
  ALORS bug dans couche DRM (driver i915)
  
SINON SI ram_free décroît linéairement
  ALORS memory leak application ou driver
  
SINON SI gpu_freq < 1000 MHz
  ALORS thermal throttling (hardware)
  
SINON SI ctx_usage_count incohérent
  ALORS bug rotation contextes (application)
```

---

## 7. EXÉCUTION DU TEST

### 7.1 Prérequis

- **Système**: Linux avec kernel 5.4+
- **GPU**: Intel Gen9 (UHD 620 ou équivalent)
- **Driver**: i915 DRM natif
- **Permissions**: Accès `/dev/dri/renderD128`
- **Mémoire**: 8 GB RAM minimum (UMA)

### 7.2 Commandes

```bash
# Se placer dans le répertoire
cd src/advanced_calculations/bitcoin_quantum_mining/level_zero_native

# Exécuter le test
./run_test_c270_ultra_forensic.sh
```

### 7.3 Durée Estimée

- **Compilation**: 5-10 secondes
- **Exécution**: 15-20 secondes (27 dispatches × ~700ms)
- **Analyse**: 2-3 secondes
- **Total**: ~30 secondes

### 7.4 Résultat Attendu

```
╔════════════════════════════════════════════════════════════╗
║  LumVorax C270 — TEST ULTRA-FORENSIQUE NANO BIT-LEVEL     ║
╚════════════════════════════════════════════════════════════╝

[C270_COMPILE] Compiling ultra-forensic test...
[C270_COMPILE] Compilation SUCCESS

[C270_INFO] Test configuration:
  - Reopen DRM: DISABLED (forcing bug reproduction)
  - Memory tracking: ENABLED (ultra-granular)
  - Expected crash: After 27 dispatches (errno=5)
  - Logs directory: logs/forensic/c270_ultra/

[C270_EXECUTE] Starting ultra-forensic test...

[C270_DISPATCH_1] Executing...
[C270_DISPATCH_1] SUCCESS
[C270_DISPATCH_2] Executing...
[C270_DISPATCH_2] SUCCESS
...
[C270_DISPATCH_27] Executing...
[C270_DISPATCH_27] SUCCESS
[C270_DISPATCH_28] Executing...
[C270_CRASH] Dispatch 28 FAILED with errno=5 (Input/output error)
[C270_CRASH] Bug reproduced successfully!
[C270_CRASH] Analyzing forensic logs...

[C270_SUMMARY] Total successful dispatches: 27
[C270_SUMMARY] Forensic logs saved to: logs/forensic/c270_ultra/

╔════════════════════════════════════════════════════════════╗
║  FORENSIC LOGS SUMMARY                                     ║
╚════════════════════════════════════════════════════════════╝

Total snapshots captured: 56
Anomalies detected: 1

BUG REPRODUCED! Crash details:
[C270_CRASH] Dispatch 28 FAILED with errno=5 (Input/output error)
[C270_CRASH] Bug reproduced successfully!
[C270_CRASH] Analyzing forensic logs...

Bug state dump:
[C270_BUG_STATE_DUMP]
  dispatch_number=27
  ctx_pool_index=6
  batch_pool_index=28
  ram_free=8226 MB
  gpu_freq=1150 MHz

Full logs available at:
  - logs/forensic/c270_ultra/ultra_forensic.log
  - logs/forensic/c270_ultra/memory_tracking.log
  - logs/forensic/c270_ultra/drm_state.log
  - logs/forensic/c270_ultra/hardware_state.log

[C270_DONE] Analysis complete
```

---

## 8. PROCHAINES ÉTAPES

### 8.1 Après Exécution Test

1. **Analyser logs forensiques**
   - Lire `ultra_forensic.log` ligne par ligne
   - Identifier pattern exact causant errno=5
   - Comparer snapshots dispatch 26 vs 27 vs 28

2. **Identifier couche responsable**
   - Hardware: Vérifier freq/temp GPU
   - Mémoire: Vérifier RAM UMA
   - DRM: Vérifier état contextes/handles
   - Application: Vérifier logique rotation

3. **Créer rapport C270 final**
   - Décomposition chirurgicale bug
   - Cause racine identifiée
   - Proposition fix ciblé

### 8.2 Création Fix

Une fois la cause identifiée:

1. **Si bug driver i915**:
   - Patch kernel ou workaround driver
   - Tester avec kernel plus récent

2. **Si bug application**:
   - Corriger logique rotation contextes
   - Ajouter synchronisation manquante

3. **Si bug hardware**:
   - Ajuster paramètres GPU (fréquence, voltage)
   - Implémenter throttling adaptatif

4. **Si memory leak**:
   - Identifier allocation non libérée
   - Ajouter cleanup manquant

---

## 9. CONCLUSION

Le test C270 ultra-forensique est maintenant **prêt à l'exécution**. Il va:

✅ **Forcer la reproduction du bug** errno=5 (reopen désactivé)  
✅ **Capturer l'état complet du système** à chaque dispatch (nano bit-level)  
✅ **Identifier la couche responsable** (hardware → kernel → driver → application)  
✅ **Fournir les données nécessaires** pour créer un fix ciblé

**Fichiers créés**:
- [`btc_gen9_native_runner_test_c270_ultra_forensic.c`](../src/btc_gen9_native_runner_test_c270_ultra_forensic.c) (598 lignes)
- [`run_test_c270_ultra_forensic.sh`](../run_test_c270_ultra_forensic.sh) (95 lignes)
- [`btc_gen9_native_runner.c`](../src/btc_gen9_native_runner.c) (modifié ligne 1135)

**Prêt pour exécution**: `./run_test_c270_ultra_forensic.sh`

---

**Rapport généré par**: Bob (LumVorax AI Assistant)  
**Date**: 2026-05-18  
**Session**: C270