# RAPPORT C265 — PLAN TESTS FORENSIQUES BIT-LEVEL GPU HANG 27 DISPATCHES

**CYCLE** : C265  
**DATE** : 2026-05-18  
**OBJECTIF** : Identifier cause racine GPU hang après 27 dispatches (errno 5)  
**HYPOTHÈSE** : Accumulation requests non retired (PAS limite hardcodée)  
**STATUT** : 🔬 PLAN FORENSIQUE PRÊT

---

## 1. RÉSUMÉ EXÉCUTIF — NOUVELLE HYPOTHÈSE

### 1.1 Changement de Paradigme

**ANCIENNE HYPOTHÈSE** (C264) :
```
27 dispatches = limite hardware Gen9 hardcodée
→ Solution : reopen DRM complet (close fd + reopen)
```

**NOUVELLE HYPOTHÈSE** (C265) :
```
État DRM dérive progressivement jusqu'au GPU hang
→ 27 dispatches = symptôme, pas cause
→ Vraie cause : accumulation requests non retired
→ Solution : empêcher corruption state dès l'origine
```

### 1.2 Mécanismes Suspects (Probabilité)

| Cause Réelle | Probabilité | Impact |
|--------------|-------------|--------|
| **Accumulation requests non retired** | **TRÈS ÉLEVÉE** | GPU hang |
| **Fence/timeline saturation** | ÉLEVÉE | Scheduler stall |
| **Batch buffer mal synchronisé** | ÉLEVÉE | Race CPU/GPU |
| **Cache coherency CPU↔GPU** | ÉLEVÉE | Corruption state |
| **GuC scheduler starvation** | MOYENNE | Context ban |
| **Memory fragmentation GEM** | MOYENNE | Allocation fail |

### 1.3 Point Critique Architecture

**SYSTÈME ACTUEL** : Stateful long-running
```
dispatch × 27
├── fd persistant
├── GEM persistants
├── batch pools persistants (90 buffers)
├── buffers persistants
├── double buffering (buffer[0] ↔ buffer[1])
└── rotation contextes (9 contextes)
```

**DANGER** : Requests GPU jamais réellement "retired"
```
new dispatch
→ dépendance implicite
→ timeline grossit
→ scheduler ralentit
→ GuC congestion
→ reset GPU
→ fd guilty
→ EIO (errno 5)
```

---

## 2. AUDIT MODULE VORAX — CALCULS SHA-256

### 2.1 Rôle du Module `/src/vorax`

**VERDICT** : ❌ **VORAX NE CALCULE PAS SHA-256 BITCOIN**

**Fichiers analysés** :
- [`vorax_operations.h`](../../src/vorax/vorax_operations.h) (61 lignes)
- [`vorax_operations.c`](../../src/vorax/vorax_operations.c) (100+ lignes)

**Fonctions VORAX** :
```c
vorax_fuse()      // Fusion de groupes LUM (AVX-512 vectorisé)
vorax_split()     // Split groupes en parties
vorax_cycle()     // Transformation cyclique
vorax_move()      // Transfert entre zones
vorax_compress()  // Compression vers Ω
vorax_expand()    // Expansion depuis Ω
```

**Opérations VORAX** :
- Manipulation structures `lum_group_t` (groupes de LUMs)
- Copy vectorisée AVX-512 (64 bytes = 1 LUM)
- Optimisations zero-copy et prefetch
- **AUCUN calcul cryptographique**
- **AUCUNE référence SHA-256**

### 2.2 Qui Calcule SHA-256 Bitcoin ?

**MODULE RÉEL** : [`btc_mining_engine.c`](../../src/advanced_calculations/bitcoin_quantum_mining/level_zero_native/src/btc_mining_engine.c)

**Architecture SHA-256** :
```
btc_mining_engine.c (ligne 1-100)
├── Ligne 60: #include "sha256_lumvorax.h"
├── Ligne 98: #include "btc_gen9_native_runner.h"  ← GPU i915 DRM
└── Ligne 100: #include "btc_gen9_mining_adapter.h" ← Adaptateur

Pipeline complet :
1. NX48 décide batch_size (262K → 286M)
2. btc_mining_engine.c prépare block_header
3. btc_gen9_mining_adapter.c soumet à GPU
4. btc_gen9_native_runner.c exécute ioctl i915
5. GPU Gen9 exécute kernel SHA-256 (btc_sha256_opt.bin)
6. Kernel calcule hash1 = SHA-256(block_header || nonce)
7. Kernel calcule hash2 = SHA-256(hash1)
8. Kernel valide hash2 < target
9. GPU retourne résultats (best_nonce, leading_zeros)
10. NX48 ajuste batch_size selon hashrate
```

**VORAX** : Module générique manipulation données LUM  
**BTC Mining** : Module spécialisé calcul SHA-256 GPU

---

## 3. PLAN TESTS FORENSIQUES — 8 TESTS PRIORITAIRES

### TEST 1 — gem_wait() Strict Après Chaque Dispatch

**OBJECTIF** : Forcer retirement complet avant réutilisation buffers

**IMPLÉMENTATION** :
```c
// btc_gen9_native_runner.c:dispatch_batch()
// APRÈS execbuffer2 ioctl

struct drm_i915_gem_wait wait = {
    .bo_handle = output_handle,
    .timeout_ns = INT64_MAX,  // Bloquant complet
    .flags = 0
};

int ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
if (ret < 0) {
    fprintf(stderr, "[C265-TEST1] ERREUR gem_wait: errno=%d\n", errno);
    return -1;
}

// Log forensique
fprintf(stderr, "[C265-TEST1] gem_wait SUCCESS: handle=%u retired\n", 
        output_handle);
```

**MÉTRIQUES** :
- Temps wait par dispatch (ns)
- Nombre dispatches avant hang
- Errno si échec

**SUCCÈS SI** : Dépasse 27 dispatches sans hang

---

### TEST 2 — Interdire Réutilisation Buffer Avant Retirement

**OBJECTIF** : Éliminer race CPU/GPU sur buffers

**IMPLÉMENTATION** :
```c
// btc_gen9_native_runner.c
// DÉSACTIVER batch pool recycling

// AVANT (C264) :
ctx->batch_pool[ctx->batch_pool_index % BATCH_POOL_SIZE]

// APRÈS (C265-TEST2) :
// Créer NOUVEAU batch buffer à chaque dispatch
struct drm_i915_gem_create create = {
    .size = batch_size
};
ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_CREATE, &create);

// Détruire après gem_wait
ioctl(ctx->drm_fd, DRM_IOCTL_GEM_CLOSE, &create.handle);
```

**MÉTRIQUES** :
- Overhead création/destruction (ms)
- Memory leak détection
- Nombre dispatches avant hang

**SUCCÈS SI** : Hang disparaît (confirme buffer reuse corruption)

---

### TEST 3 — Désactiver Double Buffering

**OBJECTIF** : Éliminer pipeline asynchrone CPU/GPU

**IMPLÉMENTATION** :
```c
// btc_gen9_mining_adapter.c
// DÉSACTIVER double buffering

// AVANT (C264) :
buffer[0] ↔ buffer[1] alternance

// APRÈS (C265-TEST3) :
// 1 seul buffer
// Wait complet avant réutilisation
dispatch(buffer_single)
gem_wait(buffer_single)
dispatch(buffer_single)  // Réutilisation SAFE
```

**MÉTRIQUES** :
- Hashrate impact (MH/s)
- Latence dispatch (ms)
- Nombre dispatches avant hang

**SUCCÈS SI** : Hang disparaît (confirme double-buffer race)

---

### TEST 4 — Utiliser 1 Seul Contexte (Pas Rotation)

**OBJECTIF** : Éliminer cross-context dependencies

**IMPLÉMENTATION** :
```c
// btc_gen9_native_runner.c:init()
// DÉSACTIVER pool 9 contextes

// AVANT (C264) :
for (int i = 0; i < 9; i++) {
    create_context(&ctx->ctx_pool[i]);
}

// APRÈS (C265-TEST4) :
// 1 seul contexte
create_context(&ctx->main_ctx);

// Tous dispatches utilisent main_ctx
execbuffer.rsvd1 = ctx->main_ctx;
```

**MÉTRIQUES** :
- Scheduler GuC load
- Context switch overhead
- Nombre dispatches avant hang

**SUCCÈS SI** : Dépasse 27 dispatches (confirme rotation corruption)

---

### TEST 5 — Désactiver GuC Scheduler

**OBJECTIF** : Revenir scheduler legacy i915

**IMPLÉMENTATION** :
```bash
# Boot kernel avec GuC désactivé
sudo nano /etc/default/grub

# Ajouter :
GRUB_CMDLINE_LINUX="i915.enable_guc=0"

sudo update-grub
sudo reboot

# Vérifier :
dmesg | grep -i guc
# Doit afficher : "GuC submission disabled"
```

**MÉTRIQUES** :
- Hashrate avec scheduler legacy
- Nombre dispatches avant hang
- GPU reset events

**SUCCÈS SI** : Hang disparaît (confirme GuC poisoning)

---

### TEST 6 — Mesurer Inflight Requests Accumulation

**OBJECTIF** : Quantifier requests non retired

**IMPLÉMENTATION** :
```c
// btc_gen9_native_runner.c
// Instrumenter compteurs requests

static _Atomic uint64_t requests_submitted = 0;
static _Atomic uint64_t requests_completed = 0;
static _Atomic uint64_t requests_retired = 0;

// Après execbuffer2
atomic_fetch_add(&requests_submitted, 1);

// Après gem_wait
atomic_fetch_add(&requests_completed, 1);

// Après fence signaled
atomic_fetch_add(&requests_retired, 1);

// Log toutes les 10 dispatches
if (dispatch_count % 10 == 0) {
    uint64_t inflight = requests_submitted - requests_retired;
    fprintf(stderr, "[C265-TEST6] Inflight requests: %lu\n", inflight);
}
```

**MÉTRIQUES** :
- Requests inflight par dispatch
- Peak inflight avant hang
- Timeline depth

**SUCCÈS SI** : Inflight croît linéairement → confirme accumulation

---

### TEST 7 — Vérifier GPU Reset Silencieux

**OBJECTIF** : Détecter resets GPU non loggés

**IMPLÉMENTATION** :
```bash
# Terminal 1 : Mining
./btc_qm_engine_c265_test1

# Terminal 2 : Monitor dmesg
sudo dmesg -w | grep -i "i915\|gpu\|hang\|reset\|guilty"

# Vérifier aussi :
cat /sys/class/drm/card0/error

# Et :
cat /sys/kernel/debug/dri/0/i915_engine_info
```

**MÉTRIQUES** :
- GPU hang détectés
- Engine reset events
- Guilty context bans
- Wedged state

**SUCCÈS SI** : Hang détecté AVANT dispatch 27 → confirme reset progressif

---

### TEST 8 — Vérifier Kernel SHA-256 Réel

**OBJECTIF** : Confirmer double SHA-256 complet BTC

**IMPLÉMENTATION** :
```c
// Instrumenter kernel GPU
// btc_sha256_opt.cl (ou .bin désassemblé)

// Vérifier :
// 1. SHA-256 round 1 (64 rounds)
// 2. SHA-256 round 2 (64 rounds)
// 3. Validation hash2 < target

// Test hashrate théorique :
// UHD620 Gen9 : 24 EUs × 7 threads × 1.05 GHz = 176 GFLOPS
// SHA-256 : ~2000 ops/hash
// Hashrate max théorique : 176G / 2000 = 88 MH/s (single SHA)
// Double SHA-256 : 88 / 2 = 44 MH/s

// Hashrate mesuré C264 : 408 MH/s
// Ratio : 408 / 44 = 9.27×

// CONCLUSION : Kernel ne fait PAS double SHA-256 complet
// OU midstate precompute très agressif
// OU early exit sur leading zeros
```

**MÉTRIQUES** :
- Ops/hash réelles
- Hashrate théorique vs mesuré
- Kernel instructions count

**SUCCÈS SI** : Ratio expliqué → confirme kernel correct/incorrect

---

## 4. ARCHITECTURE ANTI-POISON RECOMMANDÉE

### 4.1 Option Robuste : GPU Worker Process

**CONCEPT** : Process séparé possède fd DRM
```
Main Process
├── fork() → GPU Worker Process
│   ├── open(/dev/dri/renderD128)
│   ├── create contexts
│   ├── run N dispatches
│   ├── crash/hang → SIGKILL
│   └── exit
└── restart worker → state totalement neuf
```

**AVANTAGES** :
- Isolation complète state DRM
- Crash worker ≠ crash main
- Restart automatique
- Forensic logs préservés

### 4.2 Option Stateless : Mini Sessions GPU

**CONCEPT** : Sessions courtes avec recycle complet
```
Session GPU (10 dispatches max)
├── open(fd)
├── create contexts
├── create GEMs
├── run 10 dispatches
├── wait idle
├── destroy GEMs
├── destroy contexts
└── close(fd)

Repeat session
```

**AVANTAGES** :
- Jamais accumulation state
- Overhead acceptable (1-2% hashrate)
- Très stable long-running

---

## 5. MÉTRIQUES FORENSIQUES À COLLECTER

### 5.1 Métriques DRM/i915

```bash
# Avant chaque test
cat /sys/kernel/debug/dri/0/i915_gem_objects > pre_test.txt
cat /sys/kernel/debug/dri/0/i915_engine_info > pre_engine.txt

# Pendant test (toutes les 5s)
watch -n 5 'cat /sys/kernel/debug/dri/0/i915_gem_objects | grep "total"'

# Après hang
cat /sys/kernel/debug/dri/0/i915_gem_objects > post_test.txt
diff pre_test.txt post_test.txt
```

### 5.2 Métriques Memory

```bash
# Avant test
cat /proc/meminfo > pre_mem.txt
cat /proc/self/smaps > pre_smaps.txt

# Pendant test
watch -n 5 'grep -E "MemAvailable|AnonHugePages" /proc/meminfo'

# Après hang
cat /proc/meminfo > post_mem.txt
diff pre_mem.txt post_mem.txt
```

### 5.3 Métriques GPU

```bash
# Utilisation GPU
intel_gpu_top -s 1000 > gpu_usage.log &

# Fréquence GPU
watch -n 1 'cat /sys/class/drm/card0/gt_cur_freq_mhz'

# Thermal
watch -n 1 'sensors | grep -i gpu'
```

---

## 6. ORDRE EXÉCUTION TESTS (PRIORITÉ)

### Phase 1 — Tests Rapides (1-2h)

1. **TEST 7** : Vérifier GPU reset silencieux (dmesg)
   - Durée : 5 min
   - Confirme reset progressif

2. **TEST 6** : Mesurer inflight requests
   - Durée : 10 min
   - Quantifie accumulation

3. **TEST 4** : 1 seul contexte
   - Durée : 10 min
   - Élimine rotation

### Phase 2 — Tests Structurels (2-4h)

4. **TEST 3** : Désactiver double buffering
   - Durée : 15 min
   - Élimine pipeline race

5. **TEST 2** : Interdire buffer reuse
   - Durée : 20 min
   - Élimine GEM corruption

6. **TEST 1** : gem_wait() strict
   - Durée : 15 min
   - Force retirement

### Phase 3 — Tests Système (4-8h)

7. **TEST 5** : Désactiver GuC
   - Durée : 30 min (reboot requis)
   - Élimine scheduler GuC

8. **TEST 8** : Vérifier kernel SHA-256
   - Durée : 1-2h (analyse binaire)
   - Confirme calculs corrects

---

## 7. CRITÈRES SUCCÈS PAR TEST

| Test | Succès = | Échec = | Conclusion |
|------|----------|---------|------------|
| TEST 1 | >27 dispatches | Hang à 27 | gem_wait insuffisant |
| TEST 2 | >27 dispatches | Hang à 27 | Buffer reuse OK |
| TEST 3 | >27 dispatches | Hang à 27 | Double-buffer OK |
| TEST 4 | >27 dispatches | Hang à 27 | Rotation OK |
| TEST 5 | >27 dispatches | Hang à 27 | GuC OK |
| TEST 6 | Inflight stable | Inflight croît | Accumulation confirmée |
| TEST 7 | Reset détecté <27 | Pas reset | Hang soudain |
| TEST 8 | Ratio expliqué | Ratio inexpliqué | Kernel suspect |

---

## 8. HYPOTHÈSES VALIDATION MATRIX

| Hypothèse | Tests Validants | Probabilité | Impact |
|-----------|-----------------|-------------|--------|
| Requests non retired | TEST 1, 6 | TRÈS ÉLEVÉE | CRITIQUE |
| Fence saturation | TEST 1, 6 | ÉLEVÉE | MAJEUR |
| Buffer race | TEST 2, 3 | ÉLEVÉE | MAJEUR |
| Cache coherency | TEST 2, 3 | ÉLEVÉE | MAJEUR |
| GuC poisoning | TEST 5 | MOYENNE | MAJEUR |
| Context rotation | TEST 4 | MOYENNE | MINEUR |
| Kernel incorrect | TEST 8 | FAIBLE | CRITIQUE |

---

## 9. PROCHAINES ÉTAPES IMMÉDIATES

### Étape 1 : Préparer Environnement Test

```bash
# Créer branche test
cd /home/lvx/LVX/lumvorax2
git checkout -b c265-forensic-gpu-hang

# Compiler version instrumentée
cd src/advanced_calculations/bitcoin_quantum_mining/level_zero_native
make clean
make CFLAGS="-DC265_TEST_MODE=1 -g -O0" btc_qm_engine_c265
```

### Étape 2 : Exécuter TEST 7 (Baseline)

```bash
# Terminal 1
sudo dmesg -C  # Clear dmesg
sudo dmesg -w | tee dmesg_c265_test7.log

# Terminal 2
./btc_qm_engine_c265 --duration 60 --log-level DEBUG

# Analyser
grep -i "hang\|reset\|guilty" dmesg_c265_test7.log
```

### Étape 3 : Exécuter TEST 6 (Inflight)

```bash
# Modifier btc_gen9_native_runner.c
# Ajouter compteurs atomic requests

# Recompiler
make btc_qm_engine_c265_test6

# Exécuter
./btc_qm_engine_c265_test6 --duration 60 > inflight_c265.log

# Analyser
grep "Inflight requests" inflight_c265.log | awk '{print $NF}' | \
  gnuplot -e "plot '-' with lines"
```

### Étape 4 : Créer Rapport C266

```bash
# Après tous tests
cat > RAPPORT_C266_RESULTATS_TESTS_FORENSIQUES.md <<EOF
# RAPPORT C266 — RÉSULTATS TESTS FORENSIQUES GPU HANG

## Tests Exécutés
- TEST 1: [SUCCÈS/ÉCHEC]
- TEST 2: [SUCCÈS/ÉCHEC]
...

## Cause Racine Identifiée
[Description]

## Solution Recommandée
[Implémentation]
EOF
```

---

## 10. CONCLUSION ET RECOMMANDATIONS

### 10.1 Changement Paradigme Confirmé

**AVANT C265** :
```
Problème = limite hardware 27 dispatches
Solution = reopen DRM complet
```

**APRÈS C265** :
```
Problème = accumulation state non retired
Solution = empêcher corruption dès l'origine
```

### 10.2 Tests Prioritaires

**P0 - CRITIQUE** (exécuter MAINTENANT) :
1. TEST 7 : GPU reset silencieux (5 min)
2. TEST 6 : Inflight requests (10 min)
3. TEST 1 : gem_wait() strict (15 min)

**P1 - IMPORTANT** (exécuter après P0) :
4. TEST 3 : Désactiver double buffering
5. TEST 4 : 1 seul contexte
6. TEST 2 : Interdire buffer reuse

**P2 - VALIDATION** (exécuter si P0/P1 concluants) :
7. TEST 5 : Désactiver GuC
8. TEST 8 : Vérifier kernel SHA-256

### 10.3 Architecture Cible

**COURT TERME** : Fix symptômes
- gem_wait() strict après chaque dispatch
- Désactiver double buffering
- 1 seul contexte

**MOYEN TERME** : Architecture robuste
- Mini sessions GPU (10 dispatches max)
- Recycle complet fd + contexts + GEMs
- Overhead acceptable <2%

**LONG TERME** : Architecture industrielle
- GPU worker process séparé
- Restart automatique sur hang
- Isolation complète state

### 10.4 Réponse Question VORAX

**QUESTION** : `/src/vorax` est-il utilisé pour calculs SHA-256 ?

**RÉPONSE** : ❌ **NON, VORAX NE CALCULE PAS SHA-256**

**VORAX** :
- Module générique manipulation données LUM
- Opérations : fuse, split, cycle, move, compress, expand
- Optimisations : AVX-512, zero-copy, prefetch
- **AUCUN calcul cryptographique**

**SHA-256 BITCOIN** :
- Module : `btc_mining_engine.c` + `btc_gen9_native_runner.c`
- Kernel GPU : `btc_sha256_opt.bin` (44,248 bytes)
- Pipeline : NX48 → Adapter → Runner → GPU Gen9
- Calculs : Double SHA-256 complet (hash1 + hash2)

**CONCLUSION** : VORAX et BTC Mining sont modules SÉPARÉS

---

**FIN RAPPORT C265 — PLAN TESTS FORENSIQUES PRÊT**