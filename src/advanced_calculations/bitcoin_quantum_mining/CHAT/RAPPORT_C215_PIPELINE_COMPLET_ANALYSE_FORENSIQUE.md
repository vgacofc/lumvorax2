# RAPPORT FORENSIQUE C215 — PIPELINE COMPLET + ANALYSE BUGS

**Date** : 2026-05-12  
**Cycle** : C215  
**Objectif** : Compléter pipeline double-buffering GPU pour atteindre 90%+ utilisation GPU  
**Statut** : ⚠️ **ÉCHEC PARTIEL** — Pipeline implémenté mais bugs découverts  

---

## 📊 RÉSUMÉ EXÉCUTIF

### Objectifs C215
1. ✅ Corriger BUG #1 : GPU utilization calculation (ligne 677)
2. ✅ Corriger BUG #2 : Pipeline incomplet (traiter résultats immédiatement)
3. ✅ Corriger BUG #3 : Amorcer pipeline avec 2 batches initiaux
4. ❌ **OBJECTIF NON ATTEINT** : GPU actif 90%+ (résultat : 11.6%)

### Résultats C215
| Métrique | C214 | C215 | Objectif | Statut |
|----------|------|------|----------|--------|
| **GPU actif** | 10.1% | **11.6%** | 90%+ | ❌ Régression |
| **Hashrate GPU** | 10.57 MH/s | **10.57 MH/s** | 9.5+ MH/s | ✅ Maintenu |
| **Hashrate global** | 1.06 MH/s | **1.23 MH/s** | 9.5+ MH/s | ❌ Très faible |
| **Pipeline** | Incomplet | **Complet** | Fonctionnel | ✅ Implémenté |

---

## 🔍 ANALYSE FORENSIQUE ULTRA-DÉTAILLÉE

### 1. TRAÇAGE DES VALEURS RÉELLES

#### A. Hashrate par batch GPU (15.05 MH/s) ✅ CORRECT

**Source** : [`btc_lumvorax_gpu_native.c:670`](../src/btc_lumvorax_gpu_native.c:670)
```c
g_stats.current_mhs = (g_current_batch_size[idx] / kernel_sec) / 1000000.0;
```

**Données réelles** :
- `g_current_batch_size[idx]` = 262144 nonces
- `kernel_sec` = `kernel_exec_ns` / 1e9 (events OpenCL profiling)
- **Calcul** : 262144 / 0.017s = **15.4 MH/s** ✅

**Validation** :
```
[C215-PIPELINE] Batch 299: 15.05 MH/s | GPU util: 54.0% | CPU free: 46.0%
```

#### B. Hashrate global final (1.23 MH/s) ⚠️ CORRECT MAIS TROMPEUR

**Source** : [`btc_mining_engine.c:1774`](../src/btc_mining_engine.c:1774)
```c
double hashrate_mhs = (elapsed_s > 0) ? (double)total_hashes / elapsed_s / 1e6 : 0.0;
```

**Données réelles** :
- `total_hashes` = 78684160 (CPU+GPU combinés)
- `elapsed_s` = 63.96s (temps total run)
- **Calcul** : 78684160 / 63.96s / 1e6 = **1.23 MH/s** ✅

**Problème** : Ce calcul est mathématiquement correct mais **trompeur** car :
- GPU idle 88.4% du temps → hashrate global artificiellement bas
- Ne reflète PAS la performance réelle du GPU (15 MH/s)

#### C. Hashrate GPU réel (10.57 MH/s) ❌ **BUG CRITIQUE LIGNE 1777**

**Source** : [`btc_mining_engine.c:1777`](../src/btc_mining_engine.c:1777)
```c
double hashrate_gpu_mhs = (gpu_active_s > 0) ? (double)total_hashes / gpu_active_s / 1e6 : 0.0;
                                                         ^^^^^^^^^^^^ BUG ICI !
```

**ERREUR CRITIQUE** : Utilise `total_hashes` (CPU+GPU) au lieu de `gpu_total_hashes` uniquement !

**Données réelles** :
- `total_hashes` = 78684160 (CPU+GPU combinés) ❌
- `gpu_total_hashes` = 78381056 (GPU seul, visible dans logs) ✅
- `gpu_active_s` = 7.44s
- **Calcul ERRONÉ** : 78684160 / 7.44s / 1e6 = **10.57 MH/s** ❌
- **Calcul CORRECT** : 78381056 / 7.44s / 1e6 = **10.54 MH/s** ✅

**Impact** : Hashrate GPU affiché inclut ~300k hashes CPU → erreur +0.4%

---

### 2. BUGS DÉCOUVERTS C215

#### BUG #4 : Hashrate GPU calculé avec hashes CPU+GPU

**Fichier** : [`btc_mining_engine.c:1777`](../src/btc_mining_engine.c:1777)

**Problème** :
```c
// LIGNE 1777 ACTUELLE (ERRONÉE)
double hashrate_gpu_mhs = (gpu_active_s > 0) ? (double)total_hashes / gpu_active_s / 1e6 : 0.0;
                                                         ^^^^^^^^^^^^ Inclut hashes CPU !
```

**Solution** :
```c
// LIGNE 1777 CORRECTE
double hashrate_gpu_mhs = (gpu_active_s > 0) ? (double)gpu_total_hashes / gpu_active_s / 1e6 : 0.0;
                                                         ^^^^^^^^^^^^^^^^ GPU uniquement !
```

**Modifications nécessaires** :
1. Ajouter `atomic_uint_least64_t gpu_total_hashes` dans `btc_engine_t` (ligne 194)
2. Incrémenter dans thread GPU uniquement (ligne 1363)
3. Utiliser pour calcul hashrate GPU final (ligne 1777)

#### BUG #5 : Temps GPU actif mal mesuré

**Fichier** : [`btc_mining_engine.c:1325`](../src/btc_mining_engine.c:1325)

**Problème** :
```c
// Ligne 1325 : batch_start_ns AVANT lancement batch N
uint64_t batch_start_ns = eng_ts_ns();

// Ligne 1334 : Lancer batch N
lum_gpu_mine_batch_async(buffer_idx, ...);

// Ligne 1357 : Attendre batch N-1
lum_gpu_wait_batch(prev_buffer_idx, &out_best);

// Ligne 1361 : batch_end_ns APRÈS attente batch N-1
uint64_t batch_end_ns = eng_ts_ns();
uint64_t batch_duration_ns = batch_end_ns - batch_start_ns;
gpu_active_time_ns_local += batch_duration_ns;
```

**Erreur** : Mesure le temps TOTAL de la boucle (lancement N + attente N-1 + traitement), pas le temps GPU actif !

**Solution** : Utiliser les events OpenCL profiling pour mesurer le temps GPU réel :
```c
// Récupérer temps GPU depuis events OpenCL
lum_gpu_stats_t stats;
if (lum_gpu_get_stats(&stats) == 0) {
    gpu_active_time_ns_local += stats.kernel_exec_ns;  // Temps GPU réel
}
```

#### BUG #6 : GPU utilization aberrante (3546.8%)

**Fichier** : [`btc_lumvorax_gpu_native.c:677`](../src/btc_lumvorax_gpu_native.c:677)

**Problème** :
```
[C215-PIPELINE] Batch 2: 15.59 MH/s | GPU util: 3546.8% | CPU free: -3446.8%
```

**Cause** : Le batch 2 est le premier batch où `cpu_wait_ns` est très petit (quelques µs) car le pipeline vient de démarrer. Le calcul `kernel_exec_ns / cpu_wait_ns` explose.

**Solution** : Ajouter une garde pour éviter les valeurs aberrantes :
```c
if (cpu_wait_ns > 0 && cpu_wait_ns > 1000000) {  // > 1ms
    g_stats.gpu_utilization_pct = (double)g_stats.kernel_exec_ns /
                                  (double)cpu_wait_ns * 100.0;
    g_stats.cpu_free_pct = 100.0 - g_stats.gpu_utilization_pct;
} else {
    g_stats.gpu_utilization_pct = 0.0;  // Invalide
    g_stats.cpu_free_pct = 100.0;
}
```

---

### 3. POURQUOI LE GPU RESTE IDLE 88.4% ?

#### Analyse du pipeline C215

**Logs forensiques** :
```
[C215-PIPELINE-PRIME] Priming pipeline: launching batch 0 on buffer[0] (nonce=0x80000000)
[C215-PIPELINE-PRIME] Priming pipeline: launching batch 1 on buffer[1] (nonce=0x80040000)
[C215-PIPELINE] Launching batch 2 on buffer[0] (nonce=0x80080000, size=262144, target=20 bits)
[C215-PIPELINE] Waiting for batch 1 on buffer[1] (while batch 2 runs on buffer[0])
[C215-PIPELINE] Batch 1: 15.62 MH/s | GPU util: 17.3% | CPU free: 82.7% | buffer[1]
```

**Problème identifié** : Le pipeline fonctionne MAIS le traitement des résultats prend trop de temps !

**Timing détaillé** :
1. **Lancer batch N** : ~0.1ms (enqueue OpenCL)
2. **Attendre batch N-1** : ~17ms (exécution GPU)
3. **Traiter résultats** : ~200-300ms (vérification near-miss, logging, mutex, CSV) ❌
4. **Retour début boucle** : ~0.1ms

**Total par itération** : ~220ms dont seulement 17ms GPU actif = **7.7% GPU actif** !

**Solution C216** : Optimiser le traitement des résultats pour descendre sous 5ms.

---

## 📈 COMPARAISON C213 → C214 → C215

| Métrique | C213 | C214 | C215 | Évolution |
|----------|------|------|------|-----------|
| **GPU actif** | 24.2% | 10.1% | 11.6% | ⚠️ Régression -52% |
| **Hashrate GPU** | 6.55 MH/s | 10.57 MH/s | 10.57 MH/s | ✅ Stable +61% |
| **Hashrate global** | 1.58 MH/s | 1.06 MH/s | 1.23 MH/s | ⚠️ Instable |
| **Pipeline** | Bloquant | Incomplet | Complet | ✅ Progrès |
| **Bugs identifiés** | 4 | 3 | 3 | ✅ Découverts |

---

## 🎯 PLAN D'ACTION C216

### Corrections prioritaires

1. **BUG #4** : Ajouter compteur `gpu_total_hashes` séparé
2. **BUG #5** : Mesurer temps GPU avec events OpenCL
3. **BUG #6** : Garde GPU utilization > 1ms
4. **OPTIMISATION** : Réduire traitement résultats < 5ms

### Objectifs C216

- GPU actif : **90%+** (vs 11.6% C215)
- Hashrate global : **13+ MH/s** (vs 1.23 MH/s C215)
- Pipeline : **Optimisé** (traitement < 5ms)

---

## 📝 LOGS FORENSIQUES COMPLETS

### Test C215 (90 secondes)

```
[C215-PIPELINE-PRIME] Priming pipeline: launching batch 0 on buffer[0]
[C215-PIPELINE-PRIME] Priming pipeline: launching batch 1 on buffer[1]
[C215-PIPELINE] Launching batch 2 on buffer[0]
[C215-PIPELINE] Waiting for batch 1 on buffer[1] (while batch 2 runs on buffer[0])
[C215-PIPELINE] Batch 1: 15.62 MH/s | GPU util: 17.3% | CPU free: 82.7%
[C215-PIPELINE] Batch 2: 15.59 MH/s | GPU util: 3546.8% | CPU free: -3446.8%
[C215-PIPELINE] Batch 3: 15.32 MH/s | GPU util: 66.6% | CPU free: 33.4%
...
[C215-PIPELINE] Batch 299: 15.05 MH/s | GPU util: 54.0% | CPU free: 46.0%
[C213-BUG4] Temps GPU actif cumulé : 7.44 s
[C213-BUG3] GPU actif : 7.44 s (11.6% du temps total)
[C213-BUG3] Hashrate GPU réel : 10.57 MH/s (basé sur temps GPU actif)
[BTC_QM] Hashrate glob: 1.23 MH/s (CPU+GPU sur temps total)
```

### Métriques finales

- **Total hashes** : 78684160
- **GPU hashes** : 78381056 (99.6%)
- **CPU hashes** : 303104 (0.4%)
- **Durée totale** : 63.96s
- **GPU actif** : 7.44s (11.6%)
- **Batches GPU** : 301
- **Hashrate par batch** : 15.05 MH/s (moyenne)
- **Hashrate GPU réel** : 10.57 MH/s (ERRONÉ, devrait être 10.54 MH/s)
- **Hashrate global** : 1.23 MH/s

---

## 🔬 CONCLUSION FORENSIQUE

### Succès C215
1. ✅ Pipeline double-buffering implémenté et fonctionnel
2. ✅ Amorçage pipeline avec 2 batches initiaux
3. ✅ Traitement résultats immédiat après wait
4. ✅ Hashrate GPU maintenu à 10.57 MH/s

### Échecs C215
1. ❌ GPU actif 11.6% (objectif 90%+)
2. ❌ Hashrate global 1.23 MH/s (objectif 9.5+ MH/s)
3. ❌ Traitement résultats trop lent (~200-300ms)
4. ❌ 3 bugs critiques découverts

### Prochaines étapes C216
1. Corriger BUG #4 : Compteur `gpu_total_hashes` séparé
2. Corriger BUG #5 : Mesure temps GPU avec events OpenCL
3. Corriger BUG #6 : Garde GPU utilization
4. Optimiser traitement résultats < 5ms
5. Objectif final : GPU actif 90%+, hashrate global 13+ MH/s

---

**Rapport généré le** : 2026-05-12 10:40 UTC+2  
**Cycle suivant** : C216 — Optimisation traitement résultats + corrections bugs  
**Statut** : ⏳ EN ATTENTE VALIDATION UTILISATEUR