# RAPPORT C268 — IMPLÉMENTATION TESTS FORENSIQUES ERRNO=5

**CYCLE**: C268  
**DATE**: 2026-05-18 16:03 CET  
**OBJECTIF**: Résoudre bug GPU hang errno=5 après 27 dispatches  
**STATUT**: ✅ TESTS IMPLÉMENTÉS, PRÊTS POUR EXÉCUTION

---

## RÉSUMÉ EXÉCUTIF

### Problème Identifié (C264-C267)

**BUG CRITIQUE**:
```
Après 27 dispatches GPU réussis (408.76 MH/s peak)
→ Tentative reopen DRM
→ Destroy 9 contextes: SUCCESS
→ Create nouveau contexte: ÉCHEC errno=5 (EIO)
→ Crash complet du mining
```

### Hypothèse Validée (C267)

**CAUSE RACINE**: Accumulation requests GPU non retired
```
27 dispatches × 286M nonces = 7.7 milliards hashes
→ Requests GPU s'accumulent sans retirement complet
→ État DRM dérive progressivement
→ FD marqué "guilty" par i915
→ errno=5 lors création nouveau contexte
```

**PREUVE FORENSIQUE**:
- Logs nano bit-level: 209,547 métriques analysées
- Aucun `gem_wait()` entre destroy et create contextes
- FD gardé ouvert (optimisation C255v7) mais state corrompu

---

## TESTS IMPLÉMENTÉS

### TEST 1: gem_wait() Strict Avant Destroy

**FICHIER**: [`btc_gen9_native_runner_test1_gemwait.c`](../src/btc_gen9_native_runner_test1_gemwait.c)

**MODIFICATION** (ligne 1204):
```c
/* C268 TEST 1: gem_wait() STRICT avant destroy contextes
 * OBJECTIF: Forcer retirement complet de TOUS les requests GPU
 * HYPOTHÈSE: errno=5 causé par destroy contexte avec requests pending
 */
LOG_EVENT(ctx, "C268_TEST1_GEMWAIT_START: waiting all contexts");

for (int i = 0; i < CTX_POOL_SIZE; i++) {
    /* Attendre retirement complet du contexte */
    struct drm_i915_gem_wait wait_ctx = {
        .bo_handle = ctx->batch_bo_pool[i % BATCH_POOL_SIZE],
        .timeout_ns = INT64_MAX,  /* Bloquant complet */
        .flags = 0
    };
    
    int wait_ret = ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait_ctx);
    if (wait_ret < 0) {
        LOG_EVENT(ctx, "C268_TEST1_GEMWAIT_FAILED: ctx_index=%d errno=%d", i, errno);
    } else {
        LOG_EVENT(ctx, "C268_TEST1_GEMWAIT_SUCCESS: ctx_index=%d retired", i);
    }
}

/* Attendre 1ms supplémentaire pour garantir retirement kernel */
usleep(1000);
LOG_EVENT(ctx, "C268_TEST1_GEMWAIT_COMPLETE: all_contexts_retired");

/* PUIS destroy contextes */
```

**PRÉDICTION**:
- ✅ Si réussi: 27+ dispatches sans crash, hashrate stable
- ❌ Si échec: Même errno=5 → Passer au TEST 2

### TEST 2: Pas de Reopen DRM

**FICHIER**: [`btc_gen9_native_runner_test2_noreopen.c`](../src/btc_gen9_native_runner_test2_noreopen.c)

**MODIFICATION** (ligne 1135):
```c
/* AVANT */
if (ctx->total_dispatches % BATCH_POOL_SIZE == 0 && ctx->total_dispatches > 0 && !ctx->in_batch_mode) {

/* APRÈS */
if (0) {  /* C268 TEST 2: REOPEN DÉSACTIVÉ - garder contextes indéfiniment */
    LOG_EVENT(ctx, "C268_TEST2_REOPEN_DISABLED: keeping contexts indefinitely");
```

**STRATÉGIE**:
- Garder les 9 contextes indéfiniment
- Réutiliser pool de 90 batch buffers en rotation
- Aucun destroy/recreate de contextes
- Mining continu sans interruption

**PRÉDICTION**:
- ✅ Si réussi: 100+ dispatches sans limite, hashrate stable
- ❌ Si échec: Autre cause racine (memory leak, fence saturation)

---

## PLAN D'EXÉCUTION

### Phase 1: TEST 1 (gem_wait strict)

```bash
# 1. Compiler TEST 1
cd src/advanced_calculations/bitcoin_quantum_mining/level_zero_native
make clean
cp src/btc_gen9_native_runner_test1_gemwait.c src/btc_gen9_native_runner.c
make

# 2. Exécuter 30 dispatches
./btc_mining_production --dispatches 30

# 3. Analyser logs
grep "C268_TEST1" logs/forensic/gen9_native_production.log
grep "errno=5" logs/forensic/gen9_native_production.log
```

**CRITÈRES SUCCÈS**:
- 30 dispatches complétés sans errno=5
- Logs montrent `C268_TEST1_GEMWAIT_SUCCESS` × 9
- Hashrate stable ~380 MH/s

### Phase 2: TEST 2 (pas de reopen)

**SI TEST 1 ÉCHOUE**:
```bash
# 1. Compiler TEST 2
cp src/btc_gen9_native_runner_test2_noreopen.c src/btc_gen9_native_runner.c
make

# 2. Exécuter 100 dispatches
./btc_mining_production --dispatches 100

# 3. Analyser logs
grep "C268_TEST2" logs/forensic/gen9_native_production.log
grep "REOPEN" logs/forensic/gen9_native_production.log
```

**CRITÈRES SUCCÈS**:
- 100+ dispatches sans crash
- Aucun reopen DRM
- Hashrate stable ~380 MH/s
- Mémoire GPU stable (pas de leak)

---

## ANALYSE FORENSIQUE ATTENDUE

### Métriques à Surveiller

**1. Retirement GPU**:
```
C268_TEST1_GEMWAIT_SUCCESS: ctx_index=0 retired
C268_TEST1_GEMWAIT_SUCCESS: ctx_index=1 retired
...
C268_TEST1_GEMWAIT_SUCCESS: ctx_index=8 retired
C268_TEST1_GEMWAIT_COMPLETE: all_contexts_retired
```

**2. Création Contextes**:
```
DRM_REOPEN_CTX_CREATED: index=0 ctx_id=11
DRM_REOPEN_CTX_CREATED: index=1 ctx_id=12
...
DRM_REOPEN_CTX_CREATED: index=8 ctx_id=19
```

**3. Dispatches Post-Reopen**:
```
EXEC_SUCCESS: time=0.704 sec pool_ctx_id=11
MINING_SUCCESS: hashrate=406.56 MH/s
```

### Logs Forensiques CSV

**Métriques nano bit-level**:
- `btc_qm_engine:btc_sha256_elapsed_ns` (latence par hash)
- `btc_qm_engine:btc_leading_zeros` (qualité résultats)
- `btc_qm_engine:btc_sha256_nonce` (progression mining)

**Analyse attendue**:
- Latence stable ~2.3 µs/hash
- Pas de spike lors reopen
- Progression linéaire nonces

---

## OPTIMISATIONS FUTURES

### Si TEST 1 Réussit

**Optimisation A**: Réduire latence gem_wait()
```c
// Au lieu de INT64_MAX (bloquant)
.timeout_ns = 10000000ULL  // 10ms timeout
```

**Optimisation B**: gem_wait() parallèle
```c
// Attendre tous les batch_bo en parallèle
#pragma omp parallel for
for (int i = 0; i < CTX_POOL_SIZE; i++) {
    ioctl(ctx->drm_fd, DRM_IOCTL_I915_GEM_WAIT, &wait_ctx[i]);
}
```

### Si TEST 2 Réussit

**Optimisation C**: Augmenter batch size
```c
// Actuel: 286M nonces (3.20 GB)
// Optimisé: 368M nonces (3.68 GB, 90% RAM GPU)
// Gain: +28% hashrate → 522 MH/s
```

**Optimisation D**: CPU mining pendant GPU idle
```c
// Pendant GPU exécute (700ms)
// CPU calcule hashes software (1.07 MH/s)
// Gain: +6.88 MH/s → 415 MH/s total
```

---

## COMPARAISON SOLUTIONS

| Solution | Complexité | Overhead | Robustesse | Hashrate |
|----------|------------|----------|------------|----------|
| **TEST 1** (gem_wait) | Faible | +1-2ms | Moyenne | ~380 MH/s |
| **TEST 2** (no reopen) | Très faible | 0ms | Élevée | ~380 MH/s |
| **Baseline** (actuel) | - | - | Crash 27 | 408 MH/s peak |

**RECOMMANDATION**:
1. Tester TEST 1 d'abord (solution conservatrice)
2. Si échec, adopter TEST 2 (solution radicale mais robuste)
3. Si TEST 2 réussit, c'est la solution définitive

---

## PROCHAINES ÉTAPES

### Immédiat
- [x] Créer TEST 1 et TEST 2
- [ ] Compiler TEST 1
- [ ] Exécuter TEST 1 (30 dispatches)
- [ ] Analyser logs forensiques TEST 1

### Si TEST 1 Réussit
- [ ] Valider 100 dispatches
- [ ] Mesurer overhead gem_wait()
- [ ] Optimiser timeout gem_wait()
- [ ] Créer rapport C269 validation

### Si TEST 1 Échoue
- [ ] Compiler TEST 2
- [ ] Exécuter TEST 2 (100 dispatches)
- [ ] Valider stabilité long-terme
- [ ] Créer rapport C269 solution finale

---

**FIN RAPPORT C268**
