# PLAN OPTIMISATIONS C257 — Post-Validation GPU HANG

**Date**: 2026-05-17  
**Contexte**: Validation complète effectuée, timeout n'est PAS le problème

---

## 🎯 CONCLUSIONS VALIDATION C256

### ✅ Ce qui a été prouvé
1. **Pas de GPU HANG au premier dispatch** (27 dispatches OK avec temps ~700ms)
2. **Timeout 650ms n'est PAS la cause** (driver tolère le dépassement)
3. **Limite 27 dispatches confirmée** (context reset accumulation)
4. **Solution C255V13 fonctionne** (réouverture périodique, overhead 0.089%)

### ❌ Ce qui est INUTILE
1. ❌ Patch timeout driver (650ms → 5000ms)
2. ❌ Découpage kernel en sous-kernels
3. ❌ Limitation batch size à 20M pour "éviter timeout"

---

## 🚀 OPTIMISATIONS À IMPLÉMENTER (C257)

### PRIORITÉ 1: Supprimer Limitations Artificielles

#### Fichier: `src/btc_gen9_mining_adapter.c`

**Problème actuel** (lignes 37-43):
```c
#define BATCH_SIZE_MIN  262144       /* 256K nonces */
#define BATCH_SIZE_MAX  20971520     /* 20M nonces - LIMITE ARTIFICIELLE */
#define BATCH_SIZE_STEP 262144       /* Incrément 256K */
```

**Solution C257**:
```c
#define BATCH_SIZE_MIN  262144       /* 256K nonces (démarrage sécurisé) */
#define BATCH_SIZE_MAX  UINT32_MAX   /* Pas de limite - dynamique total */
#define BATCH_SIZE_STEP 262144       /* Incrément 256K */

// Nouvelle limite basée sur MÉMOIRE GPU disponible
#define BATCH_SIZE_GPU_MEM_LIMIT(mem_gb) ((uint32_t)((mem_gb * 1024 * 1024 * 1024) / 80))
// Calcul: 1.5 GB GPU → 20M nonces max (80 bytes/nonce)
```

**Justification**:
- La limite 20M était basée sur l'hypothèse timeout (FAUSSE)
- La vraie limite est la **mémoire GPU disponible**
- Avec 1.5 GB → Max théorique = **19.6M nonces**
- Mais on peut aller plus loin avec streaming/double-buffering

---

### PRIORITÉ 2: Optimiser Kernel SHA-256d (Saturation Bande Passante)

#### Objectif: Atteindre 25.6 GB/s (limite hardware Gen9)

**Analyse actuelle**:
- Bandwidth utilisé: ~9.1 GB/s (6.4 GB / 0.7s)
- Bandwidth disponible: 25.6 GB/s
- **Utilisation: 35.5%** ← ÉNORME MARGE!

**Optimisations kernel**:

#### A. Message Schedule Optimization (Réduction Accès Mémoire)

**Fichier kernel**: `kernels/btc_sha256_gen9.cl` (à créer/modifier)

**Actuel** (64 rounds, 64 accès mémoire):
```c
__kernel void sha256d_mining(
    __global uint32_t* nonces,
    __global uint32_t* results,
    __constant uint32_t* block_header
) {
    uint32_t W[64];  // Message schedule array
    
    // Expansion message (16 → 64 words)
    for (int i = 16; i < 64; i++) {
        W[i] = sigma1(W[i-2]) + W[i-7] + sigma0(W[i-15]) + W[i-16];
    }
    
    // 64 rounds SHA-256
    for (int i = 0; i < 64; i++) {
        // ... utilise W[i] ...
    }
}
```

**Optimisé C257** (circular buffer, -75% accès mémoire):
```c
__kernel void sha256d_mining_optimized(
    __global uint32_t* nonces,
    __global uint32_t* results,
    __constant uint32_t* block_header
) {
    uint32_t W[16];  // Circular buffer (au lieu de 64)
    
    // Calcul on-the-fly sans stockage
    for (int i = 0; i < 64; i++) {
        uint32_t W_i;
        if (i < 16) {
            W_i = block_header[i];  // Direct load
        } else {
            // Calcul on-the-fly depuis circular buffer
            W_i = sigma1(W[(i-2) & 15]) + W[(i-7) & 15] + 
                  sigma0(W[(i-15) & 15]) + W[(i-16) & 15];
        }
        W[i & 15] = W_i;  // Store dans circular buffer
        
        // SHA-256 round avec W_i
        // ...
    }
}
```

**Gain attendu**:
- Accès mémoire: 64 → 16 words (-75%)
- Temps exec: 700ms → ~550ms (-21%)
- Hashrate: 57 MH/s → **73 MH/s** (+28%)

#### B. Vectorisation SIMD (Gen9 supporte uint4/uint8)

**Optimisation**: Traiter 4 nonces en parallèle par thread

```c
__kernel void sha256d_mining_simd4(
    __global uint32_t* nonces,
    __global uint32_t* results,
    __constant uint32_t* block_header
) {
    uint4 nonce_vec = (uint4)(
        nonces[gid * 4 + 0],
        nonces[gid * 4 + 1],
        nonces[gid * 4 + 2],
        nonces[gid * 4 + 3]
    );
    
    // SHA-256d vectorisé sur 4 nonces simultanément
    uint4 hash_vec = sha256d_simd4(block_header, nonce_vec);
    
    // Store résultats
    vstore4(hash_vec, gid, results);
}
```

**Gain attendu**:
- Throughput: 4× par thread
- Utilisation EUs: 24 EUs × 4 = 96 threads effectifs
- Hashrate: 73 MH/s → **150+ MH/s** (+105%)

---

### PRIORITÉ 3: Vérifier Versions LUM dans Contexte

#### Fichiers à vérifier:

1. **`src/btc_gen9_native_runner.c`**
   - Utilise-t-il `lum_core.h` ?
   - Utilise-t-il `lum_logger.h` ?
   - Utilise-t-il `memory_tracker.h` ?

2. **`src/btc_i915_drm_forensic_tracker.c`**
   - Logging LUM activé ?
   - Métriques temps réel ?

3. **`include/btc_gen9_mining_adapter.h`**
   - Dépendances LUM correctes ?

**Action**: Audit complet des `#include` et remplacement par versions LUM

---

### PRIORITÉ 4: Analyse Nano-Logs Bit-Level

#### Objectif: Identifier micro-optimisations

**Fichiers logs à analyser**:
- `logs/forensic/test_c255v11_nx48.log`
- `logs/forensic/c255v12_bit_level.log`

**Métriques à extraire**:
1. **Temps par phase**:
   - Allocation buffers: X ms
   - Upload données: Y ms
   - Exec kernel: Z ms
   - Download résultats: W ms

2. **Bandwidth réel**:
   - Upload: X GB/s
   - Download: Y GB/s
   - Ratio upload/download

3. **Goulots d'étranglement**:
   - Quelle phase prend le plus de temps?
   - Où optimiser en priorité?

**Outil**: Script Python pour parser logs et générer rapport

---

## 📋 CHECKLIST IMPLÉMENTATION C257

### Phase 1: Suppression Limitations ✅
- [ ] Modifier `BATCH_SIZE_MAX` → `UINT32_MAX`
- [ ] Ajouter limite dynamique basée mémoire GPU
- [ ] Tester avec batch 40M, 80M, 100M
- [ ] Mesurer hashrate et stabilité

### Phase 2: Optimisation Kernel 🔄
- [ ] Implémenter circular buffer W[16]
- [ ] Tester gain performance
- [ ] Implémenter vectorisation SIMD4
- [ ] Benchmark hashrate final

### Phase 3: Audit Versions LUM 🔄
- [ ] Lister tous les `#include` dans src/
- [ ] Remplacer par versions LUM
- [ ] Vérifier logging forensique actif
- [ ] Tester intégration complète

### Phase 4: Analyse Nano-Logs 🔄
- [ ] Parser logs bit-level
- [ ] Identifier goulots
- [ ] Optimiser phases critiques
- [ ] Valider saturation 25.6 GB/s

---

## 🎯 OBJECTIFS FINAUX C257

### Performance
- **Hashrate cible**: 150+ MH/s (vs 57 MH/s actuel)
- **Bandwidth**: 25.6 GB/s (saturation complète)
- **Stabilité**: 100% (0 crash avec réouverture)

### Code Quality
- **Versions LUM**: 100% du code
- **Limitations**: 0 (tout dynamique)
- **Logging**: Forensique complet

### Production
- **Multi-GPU**: 2 GPUs → 300+ MH/s
- **Cache persistant**: Reprise après crash
- **Monitoring**: Temps réel avec métriques

---

## 📊 GAINS ATTENDUS

| Optimisation | Hashrate Avant | Hashrate Après | Gain |
|--------------|----------------|----------------|------|
| Baseline C255V14 | 52.88 MH/s | - | - |
| Suppression limite 20M | 52.88 MH/s | 57 MH/s | +8% |
| Circular buffer W[16] | 57 MH/s | 73 MH/s | +28% |
| Vectorisation SIMD4 | 73 MH/s | 150 MH/s | +105% |
| **TOTAL C257** | **52.88 MH/s** | **150+ MH/s** | **+184%** |

### Multi-GPU (2× Gen9)
- **Hashrate**: 300+ MH/s
- **Scaling**: Linéaire (2× 150 MH/s)

---

## 🚀 PROCHAINES ÉTAPES

### Immédiat (Aujourd'hui)
1. Modifier `btc_gen9_mining_adapter.c` (supprimer limite 20M)
2. Compiler et tester avec batch 40M, 80M
3. Mesurer hashrate et stabilité

### Court terme (Cette semaine)
1. Implémenter circular buffer dans kernel
2. Tester vectorisation SIMD4
3. Audit complet versions LUM

### Moyen terme (Ce mois)
1. Déploiement multi-GPU
2. Cache persistant production
3. Monitoring temps réel

---

**Signature**: Bob - Expert GPU Mining  
**Validation**: Post-analyse C256, plan optimisations C257  
**Status**: 🚀 PRÊT POUR IMPLÉMENTATION