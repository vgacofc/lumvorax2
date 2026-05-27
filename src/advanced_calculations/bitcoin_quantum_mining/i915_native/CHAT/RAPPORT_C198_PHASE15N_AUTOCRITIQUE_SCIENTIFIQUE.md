# RAPPORT C198 PHASE 15N — AUTOCRITIQUE SCIENTIFIQUE RIGOUREUSE

**LumVorax Bitcoin Mining — Analyse Critique Méthodologique**  
**Date** : 2026-05-11 01:04 UTC+2  
**Cycle** : C198 Phase 15N  
**Auteur** : Bob (Autocritique Scientifique)  
**Méthode** : Révision critique des conclusions Phase 15L

---

## 📋 RÉSUMÉ EXÉCUTIF

### Objectif
Identifier les conclusions **scientifiquement défendables** vs **spéculatives** du rapport Phase 15L, corriger les erreurs méthodologiques, et établir un plan d'investigation rigoureux.

### Verdict Global
Le rapport Phase 15L contient :
- ✅ **3 découvertes solides** (reproductibles, quantifiées)
- ⚠️ **5 conclusions spéculatives** (hypothèses non vérifiées)
- ❌ **2 erreurs méthodologiques** (calculs invalides)
- 🔬 **4 investigations prioritaires** (pour valider/invalider)

---

## ✅ SECTION 1 : CE QUI EST SCIENTIFIQUEMENT SOLIDE

### 1.1 Découverte #1 : Pattern Échec Stable (VALIDÉ)

**Observation reproductible** :
```
Dispatches 1-9 : SUCCESS (100% reproductible)
Dispatch 10   : ÉCHEC errno=5 (100% reproductible)
```

**Preuves** :
- 5 exécutions indépendantes
- Même kernel (42KB SHA256)
- Même batch buffer (55 commandes)
- Même contexte pool (3 contextes)
- Timing stable (692 ms ±0.53%)

**Conclusion** : **VALIDÉE**

Ce pattern est une **signature d'échec déterministe**, pas du bruit.

**Hypothèses plausibles** :
1. Corruption state compute GPU
2. Scoreboard incohérent
3. Thread payload invalide
4. Barrier/scratch mismatch
5. Register spill non géré

**Prochaine étape** : Désassembler ISA pour identifier séquence problématique

---

### 1.2 Découverte #2 : Kernel 42KB Plus Rapide que 5.9KB (VALIDÉ)

**Observation reproductible** :

| Kernel | Taille | Temps | Δ |
|--------|--------|-------|---|
| Corrompu | 5.9 KB | 742 ms | Baseline |
| SHA256 | 42 KB | 677 ms | **-8.8%** |

**Preuves** :
- 9 dispatches chacun
- Même batch size (262144 nonces)
- Même hardware (UHD 620 Gen9)
- Timing stable (±0.53%)

**Conclusion** : **VALIDÉE**

**Hypothèse CORRIGÉE** :
- ❌ ~~"Kernel plus petit → moins de cache L2"~~ (FAUX)
- ✅ **"Kernel plus petit → plus de register pressure → moins d'ILP → plus de stalls"** (PLAUSIBLE)

**Mécanismes possibles** :
- Register pressure élevée
- Occupancy réduite
- Thread dispatch packing inefficace
- EU scheduling sous-optimal
- Dépendances registres accrues

**Prochaine étape** : Comparer ISA pour mesurer register usage

---

### 1.3 Découverte #3 : Stabilité ±0.53% (VALIDÉ)

**Observation reproductible** :

```
Dispatches 4-9 : 692.638 ms ± 3.679 ms
Écart-type : 0.53%
```

**Preuves** :
- 6 mesures consécutives
- Timestamps nanoseconde
- Variance minimale (3.7 ms sur 692 ms)

**Conclusion** : **VALIDÉE**

**Signification** :
- ✅ Workload GPU stable
- ✅ Scheduler GPU stable
- ✅ Fréquence relativement stable
- ✅ Protocole mesure rigoureux
- ✅ Reproductibilité excellente

**Impact** : Valide la **qualité méthodologique** du projet

---

## ⚠️ SECTION 2 : CE QUI EST SPÉCULATIF

### 2.1 Hypothèse Spéculative #1 : "Utilisation GPU 0.23%"

**Affirmation Phase 15L** :
```
Temps théorique : 1.56 ms
Temps réel : 692 ms
Ratio : 443× plus lent
Utilisation GPU = 0.23%
```

**Problème méthodologique** : **CALCUL INVALIDE**

**Erreurs** :
1. Suppose `1000 cycles/nonce` (non mesuré)
2. Ignore coût mémoire SHA256
3. Ignore synchronisation threads
4. Ignore register pressure
5. Ignore divergence
6. Ignore occupancy réelle
7. Ignore EU stalls

**Conclusion** : ❌ **REJETÉE**

Le chiffre "0.23%" n'est **pas scientifiquement défendable** sans :
- Compteurs matériels (EU Active, EU Stall, EU Idle)
- Occupancy réelle mesurée
- Thread residency
- Memory busy

**Correction** :
- ✅ "Énorme marge de progression existe" (PLAUSIBLE)
- ❌ "Utilisation GPU = 0.23%" (NON MESURÉE)

---

### 2.2 Hypothèse Spéculative #2 : "Bande Passante 0.16%"

**Affirmation Phase 15L** :
```
Bande passante mesurée : 42.4 MB/s
Bande passante théorique : 25.6 GB/s
Utilisation : 0.16%
```

**Problème méthodologique** : **CALCUL SIMPLISTE**

**Erreurs** :
1. Suppose accès mémoire linéaires (faux pour SHA256)
2. Ignore cache L2 GPU
3. Ignore cache L3 CPU
4. Ignore prefetch hardware
5. Ignore coalescing
6. Ignore bank conflicts

**Conclusion** : ⚠️ **PARTIELLEMENT VALIDÉE**

- ✅ Bande passante mesurée = 42.4 MB/s (CORRECTE)
- ❌ "Utilisation 0.16%" (TROP SIMPLISTE)

**Correction** :
- SHA256 est **memory-bound** (VALIDÉ)
- Mais ratio 0.16% ignore complexité accès mémoire GPU

---

### 2.3 Hypothèse Spéculative #3 : "98.5% Overhead = Scheduler"

**Affirmation Phase 15L** :
```
Overhead inter-dispatch : 10 ms
GPU work : 692 ms
Overhead = 10 / 692 = 1.4%
Mais 98.5% de l'overhead = scheduler Linux
```

**Problème méthodologique** : **CONFUSION MÉTRIQUES**

**Erreurs** :
1. Mélange overhead absolu (10 ms) et relatif (1.4%)
2. Suppose scheduler = seule cause
3. Ignore autres sources :
   - Batch buffer rebuild (60 μs)
   - Logging forensique (50 μs)
   - GEM revalidation
   - Fence polling
   - Context switch

**Conclusion** : ⚠️ **PARTIELLEMENT VALIDÉE**

- ✅ Overhead 10 ms existe (MESURÉ)
- ⚠️ "98.5% = scheduler" (NON PROUVÉ)

**Correction** :
- Overhead 10 ms est **significatif** (1.4% du temps total)
- Mais décomposition précise nécessite `strace` + `perf`

---

### 2.4 Hypothèse Spéculative #4 : "errno=5 Immédiat = Pas de Tentative GPU"

**Affirmation Phase 15L** :
```
Échec recyclage détecté en 148 μs
→ Driver refuse immédiatement sans consulter GPU
```

**Problème méthodologique** : **INTERPRÉTATION HÂTIVE**

**Erreurs** :
1. 148 μs est **largement suffisant** pour :
   - Détecter GPU hang existant
   - Observer engine wedged
   - Refuser submit
2. Ne prouve PAS absence tentative GPU

**Conclusion** : ❌ **REJETÉE**

**Correction** :
- 148 μs prouve **détection rapide** d'un état invalide
- Ne prouve PAS que GPU n'a pas été consulté

---

### 2.5 Hypothèse Spéculative #5 : "Gain 18.5× vs OpenCL"

**Affirmation Phase 15L** :
```
i915 DRM = 18.5× plus rapide qu'OpenCL
```

**Mais aussi** :
```
3.07 MH/s = 14% OpenCL (21.86 MH/s)
```

**Problème méthodologique** : **CONTRADICTION INTERNE**

**Erreurs** :
1. Les deux affirmations sont **incompatibles**
2. Mélange probablement :
   - Init time (1.35 ms vs 50 ms = 37×)
   - Hashrate (3.07 vs 21.86 = 0.14×)

**Conclusion** : ❌ **REJETÉE**

**Correction** :
- ✅ Init i915 DRM = **37× plus rapide** qu'OpenCL (1.35 ms vs 50 ms)
- ❌ Hashrate i915 DRM = **7× plus lent** qu'OpenCL (3.07 vs 21.86 MH/s)

---

## ❌ SECTION 3 : ERREURS MÉTHODOLOGIQUES

### 3.1 Erreur #1 : Calcul "Utilisation GPU 0.23%"

**Problème** : Calcul théorique sans mesure hardware

**Impact** : Conclusion non défendable scientifiquement

**Correction** : Utiliser `intel_gpu_top` + compteurs OA

---

### 3.2 Erreur #2 : Confusion Métriques Performance

**Problème** : Mélange init time, hashrate, overhead

**Impact** : Contradictions internes rapport

**Correction** : Normaliser toutes comparaisons :
- Même kernel
- Même batch size
- Même métrique (MH/s)
- Même fenêtre temporelle

---

## 🔬 SECTION 4 : INVESTIGATIONS PRIORITAIRES

### 4.1 Investigation #1 : Désassemblage ISA Comparatif (CRITIQUE)

**Objectif** : Identifier séquence ISA problématique

**Méthode** :
```bash
# Désassembler kernel stable (42KB)
iga64 -d -p 9 kernels/btc_sha256_mining_gen9.bin > sha256_stable.asm

# Désassembler kernel instable (5.9KB)
iga64 -d -p 9 kernels/btc_sha256_mining_gen9.bin.CORRUPT_BACKUP > sha256_instable.asm

# Comparer
diff -u sha256_stable.asm sha256_instable.asm > isa_diff.txt
```

**Rechercher** :
- `send` instructions (mémoire)
- `barrier` (synchronisation)
- `scratch` (spill)
- `mov` avec registres élevés (r120+)
- Control flow (`jmpi`, `call`)

**Priorité** : **CRITIQUE** (bloque compréhension échec)

---

### 4.2 Investigation #2 : Capture GPU Hangs Réels (HAUTE)

**Objectif** : Obtenir error state i915 complet

**Méthode** :
```bash
# Terminal 1 : Monitoring dmesg
dmesg -w | tee gpu_hang_log.txt

# Terminal 2 : Exécution test
cd level_zero_native
make test_btc

# Après échec, capturer error state
sudo cat /sys/kernel/debug/dri/0/i915_error_state > error_state_dispatch10.txt
```

**Rechercher** :
- Engine (RCS0, VCS0, VECS0)
- ECODE (error code)
- Guilty batch offset
- Reset reason
- Active/pending requests

**Priorité** : **HAUTE** (diagnostic précis échec)

---

### 4.3 Investigation #3 : Mesure Occupation GPU Réelle (HAUTE)

**Objectif** : Remplacer estimations par mesures hardware

**Méthode** :
```bash
# Terminal 1 : Monitoring GPU
intel_gpu_top -o gpu_metrics.log

# Terminal 2 : Exécution test
cd level_zero_native
make test_btc

# Analyser métriques
grep "Render/3D" gpu_metrics.log
```

**Métriques clés** :
- EU Active (%)
- EU Stall (%)
- EU Idle (%)
- Memory Busy (%)
- Frequency (MHz)

**Priorité** : **HAUTE** (valider/invalider "0.23%")

---

### 4.4 Investigation #4 : Analyse Submits Bloquants (MOYENNE)

**Objectif** : Identifier source overhead 10ms

**Méthode** :
```bash
# Tracer syscalls
strace -tt -T -o strace_dispatch.log ./bin/test_btc_mining_native

# Analyser timing
grep "ioctl.*EXECBUFFER" strace_dispatch.log
grep "ioctl.*GEM_WAIT" strace_dispatch.log
```

**Rechercher** :
- Temps `ioctl(DRM_IOCTL_I915_GEM_EXECBUFFER2)`
- Temps `ioctl(DRM_IOCTL_I915_GEM_WAIT)`
- Appels bloquants imprévus

**Priorité** : **MOYENNE** (optimisation secondaire)

---

## 📊 SECTION 5 : CONCLUSIONS CORRIGÉES

### 5.1 Ce Qui Est Prouvé

1. ✅ **Pattern échec stable** : 9 dispatches OK, 10ème FAIL (100% reproductible)
2. ✅ **Kernel 42KB plus rapide** : -8.8% vs 5.9KB (probablement register pressure)
3. ✅ **Stabilité ±0.53%** : Protocole mesure rigoureux validé
4. ✅ **Init 37× plus rapide** : 1.35 ms vs 50 ms OpenCL
5. ✅ **Overhead 10ms existe** : Entre dispatches (1.4% temps total)

### 5.2 Ce Qui Est Spéculatif

1. ⚠️ "Utilisation GPU 0.23%" → **NON MESURÉE** (nécessite compteurs hardware)
2. ⚠️ "Bande passante 0.16%" → **TROP SIMPLISTE** (ignore complexité accès)
3. ⚠️ "98.5% overhead = scheduler" → **NON PROUVÉ** (nécessite strace)
4. ⚠️ "errno=5 immédiat = pas de GPU" → **FAUX** (148μs suffisant pour détection)
5. ⚠️ "Gain 18.5× vs OpenCL" → **CONTRADICTION** (mélange métriques)

### 5.3 Ce Qui Est Faux

1. ❌ "Kernel petit → moins cache L2" → **FAUX** (probablement register pressure)
2. ❌ "Hashrate i915 > OpenCL" → **FAUX** (3.07 vs 21.86 MH/s = 7× plus lent)

---

## 🎯 SECTION 6 : PLAN D'ACTION CORRIGÉ

### Phase 15O : Désassemblage ISA (IMMÉDIAT)
- Comparer ISA stable vs instable
- Identifier séquence problématique
- Mesurer register usage
- Analyser control flow

### Phase 15P : Capture GPU Hangs (IMMÉDIAT)
- Monitorer dmesg en temps réel
- Capturer error state i915
- Analyser engine/ecode/batch

### Phase 15Q : Mesure Occupation GPU (IMMÉDIAT)
- Utiliser intel_gpu_top
- Mesurer EU Active/Stall/Idle
- Valider/invalider "0.23%"

### Phase 15R : Analyse Submits (SECONDAIRE)
- Tracer avec strace
- Identifier appels bloquants
- Décomposer overhead 10ms

---

## 📈 CONCLUSION FINALE

### Progrès Méthodologique

Le projet a franchi un **seuil critique** :
- ✅ Patterns reproductibles
- ✅ Signatures d'échec stables
- ✅ Métriques quantitatives
- ✅ Protocole rigoureux

### Découverte Principale

**Le vrai problème n'est probablement PAS OpenCL**, mais :
- Qualité ISA générée
- State management GPU
- Occupancy/residency
- Pipeline control

### Prochaine Étape

**Désassembler ISA** est maintenant **CRITIQUE** pour :
- Comprendre échec dispatch 10
- Identifier corruption state
- Optimiser register usage
- Valider hypothèses

---

**FIN RAPPORT C198 PHASE 15N — AUTOCRITIQUE SCIENTIFIQUE**

*Made with Bob - LumVorax Bitcoin Mining Engine*  
*Rigueur scientifique > Conclusions hâtives*  
*Mesures hardware > Estimations théoriques*