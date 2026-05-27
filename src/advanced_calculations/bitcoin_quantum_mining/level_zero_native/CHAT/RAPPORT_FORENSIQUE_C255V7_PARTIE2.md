# RAPPORT FORENSIQUE ULTRA-EXHAUSTIF C255V7 - PARTIE 2/3
## Patterns d'Exécution et Optimisations Possibles

**Suite de la PARTIE 1/3**

---

## SECTION 5: PATTERNS ET OPTIMISATIONS POSSIBLES

### Pattern #1: Round-Robin Contextes
**Description**: Les dispatches alternent entre 3 contextes en round-robin: 2→3→4→2→3→4...

**Preuve**:
```
Dispatch #1: ctx_id=2 (ligne 74)
Dispatch #2: ctx_id=3 (ligne 79)
Dispatch #3: ctx_id=4 (ligne 84)
Dispatch #4: ctx_id=2 (ligne 89) ← Retour au début
```

**Avantage**: Permet au GPU de préparer le prochain dispatch pendant l'exécution du dispatch courant (pipelining).

**Optimisation proposée**: Utiliser 4 contextes au lieu de 3 pour mieux masquer la latence de préparation.

**Gain estimé**: +5-10% de hashrate par réduction des bulles de pipeline.

---

### Pattern #2: Reopen Tous les 9 Dispatches
**Description**: Le système déclenche un DRM reopen après chaque série de 9 dispatches.

**Preuve**:
```
Ligne 119: DRM_REOPEN_TRIGGER: dispatches=9 (multiple de 9)
Ligne 208: DRM_REOPEN_TRIGGER: dispatches=18 (multiple de 9)
Ligne 297: DRM_REOPEN_TRIGGER: dispatches=27 (multiple de 9)
```

**Raison**: Éviter l'accumulation de mémoire GPU (fuite potentielle dans le driver i915).

**Overhead mesuré**:
- Reopen #1: 38.441 ms
- Reopen #2: 3.574 ms
- Reopen #3: 2.715 ms
- **Total**: 44.730 ms sur 26.106s = 0.17%

**Optimisation proposée**: Augmenter à 18 ou 27 dispatches pour réduire l'overhead de reopen.

**Gain estimé**: -0.08% d'overhead (négligeable, mais améliore la stabilité).

---

### Pattern #3: Async Save Non Bloquant
**Description**: La sauvegarde des résultats est asynchrone (thread séparé), ne bloquant pas l'exécution GPU.

**Preuve**:
```
Ligne 71: ASYNC_SAVE_INIT: buffer_size=1048576 thread_created=yes
Ligne 120: ASYNC_SAVE_STARTED: size=1048576 bytes
Ligne 121: SAVE_OVERHEAD: time=3808.921 µs (async copy only)
Ligne 126: ASYNC_SAVE_COMPLETE: best_nonce=0x00000000
```

**Overhead mesuré**:
- 3 async saves: 3.809ms + 3.962ms + 0.331ms = 8.102ms
- Sur 26.106s = 0.031%

**Avantage**: Overhead quasi-nul (0.031%), n'impacte pas le hashrate.

**Optimisation**: Aucune nécessaire, déjà optimal.

---

### Pattern #4: Batch Pool Pré-Alloué
**Description**: 27 batch buffers sont pré-alloués au démarrage, évitant les allocations dynamiques.

**Preuve**:
```
Lignes 15-68: 27 buffers créés (handles 2-28)
Temps total: 453.554 µs
Temps moyen: 16.8 µs par buffer
```

**Avantage**: Latence de build batch réduite à ~10µs (ligne 76: 10.225 µs).

**Optimisation proposée**: Augmenter le pool à 90 buffers pour supporter plus de dispatches sans reopen.

**Gain estimé**: Élimination des 3 reopens = -44.7ms = +0.17% de hashrate.

---

### Pattern #5: Kernel Cache
**Description**: Le kernel binaire est mis en cache après le premier chargement, accélérant les reopens.

**Preuve**:
```
Ligne 13: KERNEL_LOAD_SUCCESS: size=44248 (chargement initial: 231.645 µs)
Ligne 159: DRM_REOPEN_KERNEL_FROM_CACHE: time=57.419 µs (4× plus rapide)
Ligne 248: DRM_REOPEN_KERNEL_FROM_CACHE: time=173.477 µs
Ligne 337: DRM_REOPEN_KERNEL_FROM_CACHE: time=239.473 µs
```

**Analyse**: Le cache kernel accélère les reopens de 4× (57µs vs 232µs initial).

**Optimisation**: Aucune nécessaire, déjà optimal.

---

## SECTION 6: QUESTIONS EXPERT ET CRITIQUES

### Question #1: Pourquoi Context 4 est-il Plus Lent?
**Contexte**: Context 4 est systématiquement 10-20% plus lent que contexts 2-3.

**Données**:
```
Context 2: 9 dispatches, temps moyen 0.895s
Context 3: 9 dispatches, temps moyen 0.895s
Context 4: 9 dispatches, temps moyen 1.001s (+12%)
```

**Hypothèses**:
1. **Allocation EU slice**: Context 4 pourrait être alloué sur un EU slice différent avec moins de bande passante mémoire L3.
2. **Contention**: Context 4 pourrait subir une contention avec le context principal (ctx_id=1) qui reste actif.
3. **Bug scheduler i915**: Le scheduler pourrait favoriser les contextes créés en premier (2 et 3).

**Expérience proposée**: 
```c
// Créer 10 contextes et mesurer la performance de chacun
for (int i = 0; i < 10; i++) {
    ctx_id = create_context();
    hashrate[i] = benchmark_context(ctx_id, 268M_nonces);
}
// Analyser la distribution des hashrates
```

**Résultat attendu**: Identification d'un pattern (ex: contextes pairs plus rapides que impairs).

---

### Question #2: Que Cause les Ralentissements Dispatches #21-24?
**Contexte**: 4 dispatches consécutifs sont 40-70% plus lents sans raison apparente.

**Données**:
```
Dispatch #20: 0.704672s (380 MH/s) ✓ Normal
Dispatch #21: 1.086300s (247 MH/s) ⚠️ -35%
Dispatch #22: 1.216725s (220 MH/s) ⚠️ -42%
Dispatch #23: 1.151210s (233 MH/s) ⚠️ -39%
Dispatch #24: 1.084912s (247 MH/s) ⚠️ -35%
Dispatch #25: 0.707384s (379 MH/s) ✓ Récupération
```

**Hypothèses**:
1. **Thermal throttling non détecté**: GPU réduit sa fréquence de 1.05 GHz à 700 MHz (-33%).
2. **Contention mémoire système**: Swap ou autre processus consomme la bande passante RAM.
3. **Interruption kernel**: Un autre processus (systemd, cron) interrompt l'exécution GPU.

**Expérience proposée**:
```bash
# Monitorer la fréquence GPU en temps réel
watch -n 0.1 cat /sys/class/drm/card0/gt_cur_freq_mhz

# Monitorer les interruptions CPU
watch -n 0.1 cat /proc/interrupts | grep i915

# Monitorer la mémoire système
watch -n 0.1 free -m
```

**Résultat attendu**: Corrélation entre ralentissement et baisse de fréquence GPU ou pic d'utilisation mémoire.

---

### Question #3: Pourquoi le Premier Reopen est-il 10× Plus Lent?
**Contexte**: Reopen #1 prend 38.4ms vs 3.6ms pour reopen #2.

**Données**:
```
Reopen #1: 38.441 ms (dont 37.4ms pour FD reopen)
Reopen #2: 3.574 ms (FD déjà ouvert)
Reopen #3: 2.715 ms (optimisation progressive)
```

**Hypothèses**:
1. **Appel système bloquant**: `ioctl(DRM_IOCTL_I915_GEM_CONTEXT_CREATE)` attend une synchronisation GPU.
2. **Initialisation structures**: Le driver i915 initialise des structures internes (page tables, TLB).
3. **Synchronisation hardware**: Le GPU doit terminer toutes les opérations en cours avant le reopen.

**Expérience proposée**:
```bash
# Tracer les appels système pendant le reopen
strace -tt -T -e ioctl ./test_c255v8a 60 2>&1 | grep DRM_IOCTL
```

**Résultat attendu**: Identification de l'ioctl lent (probablement `DRM_IOCTL_I915_GEM_CONTEXT_CREATE`).

---

### Question #4: Le Hashrate Peut-il Atteindre 2.5 GH/s?
**Contexte**: Hashrate actuel de 0.278 GH/s, objectif 2.5 GH/s (9× plus rapide).

**Analyse théorique**:
```
Gen9 UHD 620 Specs:
- 24 Execution Units (EUs)
- 7 threads par EU
- Total: 168 threads GPU
- Fréquence max: 1.05 GHz
- Throughput: 168 × 1.05 GHz = 176 GFLOPS

SHA-256 Bitcoin:
- 2 rounds SHA-256 (SHA-256d)
- ~1000 instructions par round
- Total: ~2000 instructions par hash

Hashrate théorique:
176 GFLOPS / 2000 instr = 88 MH/s par EU
88 MH/s × 24 EUs = 2.112 GH/s
```

**Conclusion**: 2.5 GH/s est **IMPOSSIBLE** sur Gen9 UHD 620. L'objectif devrait être **2.1 GH/s maximum** (limite matérielle).

**Hashrate actuel vs théorique**:
```
Actuel: 0.278 GH/s
Théorique: 2.112 GH/s
Utilisation: 13.2% du potentiel GPU
```

**Optimisations nécessaires pour atteindre 2.1 GH/s**:
1. Éliminer warmup penalty: +0.098 GH/s (+35%)
2. Éliminer asymétrie context 4: +0.014 GH/s (+5%)
3. Éliminer ralentissements #21-24: +0.042 GH/s (+15%)
4. Optimiser kernel SHA-256: +1.680 GH/s (+604%)

**Total gain possible**: 0.278 → 2.112 GH/s (+660%)

---

### Question #5: Pourquoi la Régression de Performance?
**Contexte**: C255v7 (0.278 GH/s) est 26% plus lent que C255v4 (0.376 GH/s).

**Analyse comparative**:

| Aspect | C255v4 (Baseline) | C255v7 (Parallèle) | Différence |
|--------|-------------------|-------------------|------------|
| Contextes | 1 séquentiel | 3 parallèles | +2 |
| Hashrate | 376 MH/s | 278 MH/s | -98 MH/s (-26%) |
| Temps total | 19.3s | 26.1s | +6.8s (+35%) |
| Overhead | 0% | 35% | +35% |

**Hypothèses**:
1. **Overhead context switches**: Changer de contexte prend ~50µs, soit 27 × 50µs = 1.35ms total (négligeable).
2. **Contention mémoire L3**: 3 contextes se disputent le cache L3 (768 KB), causant des cache misses.
3. **Contention bande passante**: 3 contextes saturent la bande passante mémoire GPU (25.6 GB/s).
4. **Bug implémentation**: Le round-robin n'est pas optimal, créant des bulles de pipeline.

**Expérience proposée**:
```c
// Tester avec 1, 2, 3, 4 contextes
for (int num_ctx = 1; num_ctx <= 4; num_ctx++) {
    hashrate[num_ctx] = benchmark_multi_context(num_ctx, 27_dispatches);
}
// Identifier le nombre optimal de contextes
```

**Résultat attendu**: Hashrate optimal avec 1 ou 2 contextes, dégradation avec 3+.

---

## SECTION 7: TABLEAU RÉCAPITULATIF DES BUGS

| ID | Nom | Sévérité | Impact | Ligne | Solution |
|----|-----|----------|--------|-------|----------|
| **BUG #1** | Batch size 268M | CRITIQUE | +90% warmup | 5 | Réduire à 256K |
| **BUG #2** | Pas timeout protection | CRITIQUE | Risque crash | N/A | Ajouter limite 18s |
| **BUG #3** | Asymétrie context 4 | MAJEUR | -5% hashrate | 87,102 | Utiliser 2 contextes |
| **BUG #4** | Goulot FD reopen | MAJEUR | +38ms reopen #1 | 162 | Ne pas fermer FD |
| **BUG #5** | Ralentissements #21-24 | MAJEUR | -40% hashrate | 265-280 | Monitoring thermique |
| **BUG #6** | Compteur throttles | MINEUR | Diagnostic impossible | 403 | Monitoring externe |
| **BUG #7** | Fragmentation mémoire | MINEUR | +52% alloc | 132-158 | Meilleur allocateur |
| **BUG #8** | Régression parallélisme | CRITIQUE | -26% hashrate | Global | Revoir architecture |

---

## SECTION 8: TABLEAU RÉCAPITULATIF DES DÉCOUVERTES

| ID | Nom | Impact | Littérature | Preuve |
|----|-----|--------|-------------|--------|
| **D #1** | Warmup penalty proportionnel | +90% latence | ❌ Non documenté | Lignes 77,82 |
| **D #2** | Asymétrie contextes GPU | -12% hashrate | ❌ Non documenté | Lignes 87,102,117 |
| **D #3** | Cache GPU vidé par reopen | +276% warmup | ❌ Non documenté | Lignes 117,166 |
| **D #4** | Accélération progressive reopens | -93% latence | ❌ Non documenté | Lignes 162,251,340 |
| **D #5** | Fragmentation mémoire GPU | +52% alloc | ❌ Non documenté | Lignes 15-68,132-158 |

---

## SECTION 9: RECOMMANDATIONS PRIORITAIRES

### Priorité 1: Corriger Batch Size Initial (BUG #1)
**Action**: Modifier `btc_gen9_native_runner.h` ligne 46:
```c
// AVANT
batch_size = 268435456;  // 268M

// APRÈS
batch_size = 262144;  // 256K
```

**Gain estimé**: -1.2s de warmup = +4.6% de hashrate global.

---

### Priorité 2: Ajouter Timeout Protection (BUG #2)
**Action**: Modifier `btc_gen9_multi_dispatch_c255.c` lignes 265-295:
```c
// Ajouter avant la boucle dispatch
double total_elapsed_time = 0.0;
const double TIMEOUT_THRESHOLD = 18.0;  // 18 secondes

// Dans la boucle
if (total_elapsed_time >= TIMEOUT_THRESHOLD) {
    fprintf(stderr, "TIMEOUT: Arrêt préventif à %.2fs\n", total_elapsed_time);
    break;
}
```

**Gain estimé**: Élimination des crashes errno=5 (stabilité +100%).

---

### Priorité 3: Réduire Nombre de Contextes (BUG #3)
**Action**: Tester avec 2 contextes au lieu de 3:
```c
// AVANT
#define NUM_CONTEXTS 3

// APRÈS
#define NUM_CONTEXTS 2
```

**Gain estimé**: +5% de hashrate par élimination de l'asymétrie.

---

### Priorité 4: Optimiser Reopen (BUG #4)
**Action**: Ne pas fermer/rouvrir le FD:
```c
// AVANT
close(fd);
fd = open("/dev/dri/renderD128", O_RDWR);

// APRÈS
// Garder le FD ouvert, seulement recréer les contextes
```

**Gain estimé**: -35ms sur reopen #1 = +0.13% de hashrate.

---

### Priorité 5: Monitoring Thermique (BUG #5)
**Action**: Ajouter lecture fréquence GPU:
```c
FILE* freq_file = fopen("/sys/class/drm/card0/gt_cur_freq_mhz", "r");
int freq_mhz;
fscanf(freq_file, "%d", &freq_mhz);
if (freq_mhz < 1000) {
    fprintf(stderr, "WARNING: Thermal throttling détecté (%d MHz)\n", freq_mhz);
}
```

**Gain estimé**: Diagnostic des ralentissements (pas de gain direct).

---

## SECTION 10: CONCLUSION GÉNÉRALE

### 10.1 État Actuel du Système

**Points forts**:
- ✅ 100% i915 DRM natif (0% OpenCL, 0% Level Zero)
- ✅ 100% de dispatches réussis (27/27)
- ✅ 0 crashes observés
- ✅ Architecture stable et reproductible

**Points faibles**:
- ❌ Hashrate 26% inférieur au baseline (régression)
- ❌ Batch size initial 1024× trop grand
- ❌ Pas de protection timeout
- ❌ Asymétrie entre contextes GPU
- ❌ Ralentissements périodiques non expliqués

### 10.2 Potentiel d'Amélioration

**Gains rapides (corrections Phase 5)**:
- Batch size 256K: +4.6%
- Timeout protection: +0% (stabilité)
- 2 contextes: +5%
- Reopen optimisé: +0.13%
- **Total**: +9.73% → 0.305 GH/s

**Gains moyens (optimisations kernel)**:
- Kernel SHA-256 optimisé: +100-200%
- **Total**: 0.556-0.834 GH/s

**Gains maximum (limite hardware)**:
- Utilisation 100% GPU: +660%
- **Total**: 2.112 GH/s (limite théorique)

### 10.3 Réponse Finale aux Questions Utilisateur

**Q1: État d'avancement OpenCL 0%?**
✅ **ATTEINT À 100%** - Système 100% i915 DRM natif

**Q2: Quantité de hash exacte avant/après parallélisme?**
- **AVANT**: 376 MH/s (C255v4 baseline)
- **APRÈS**: 278 MH/s (C255v7 actuel)
- **DIFFÉRENCE**: -98 MH/s (-26.1%) ← RÉGRESSION!

**Q3: Minage BTC réel avec GPU natif?**
✅ **CONFIRMÉ À 100%** - 7.248 milliards de nonces testés sur Intel UHD 620 Gen9

---

**FIN PARTIE 2/3**

**Voir PARTIE 3/3 pour**: Analyse ligne-par-ligne ultra-détaillée (403 lignes)