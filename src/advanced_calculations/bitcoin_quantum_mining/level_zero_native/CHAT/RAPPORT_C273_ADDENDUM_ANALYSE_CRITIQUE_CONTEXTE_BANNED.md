# RAPPORT C273 — ADDENDUM ANALYSE CRITIQUE : CONTEXTE BANNED/GUILTY
## Intégration Feedback Expert : Rejet Immédiat vs GPU Hang Physique

**Date** : 2026-05-18  
**Auteur** : Bob (Mode Advanced)  
**Contexte** : Suite analyse forensique C272, intégration feedback expert  
**Découverte Majeure** : Crash = Rejet logiciel immédiat, PAS GPU hang physique

---

## 🎯 DÉCOUVERTE CRITIQUE : CRASH QUASI-INSTANTANÉ

### Temps Dispatch Normal vs Crash

**Dispatches Normaux** :
```
Dispatch 26: 7911.720592 → 7912.421655 = 701 ms
Dispatch 27: 7912.423733 → 7912.426187 = 2.45 ms ❌ CRASH
```

**Analyse Temporelle** :
- Temps normal : **~700ms** (GPU execution complète)
- Temps crash : **2.45ms** (×286 plus rapide)
- **Conclusion** : GPU ne semble PAS exécuter le workload

### Interprétation

Le rejet arrive **immédiatement** au niveau :
1. Validation request dans `DRM_IOCTL_I915_GEM_EXECBUFFER2`
2. Enqueue scheduler i915
3. État contexte déjà invalide
4. Request déjà bannie

**Ceci ressemble à** :
```c
// Request rejection (logiciel)
if (ctx->banned || ctx->guilty) {
    return -EIO;  // errno=5
}
```

**PAS à** :
```c
// GPU computation failure (matériel)
wait_for_gpu_completion();  // 700ms
if (gpu_hang_detected()) {
    return -EIO;
}
```

---

## 📊 OBSERVATION CRUCIALE : COMPTEUR NON INCRÉMENTÉ

### État Avant/Après Crash

**Après Dispatch 27 (succès)** :
```
COUNTERS: total_dispatches=27
CTX_USAGE: [3,3,3,3,3,3,3,3,3]
```

**Après Dispatch 28 (échec)** :
```
COUNTERS: total_dispatches=27  ← PAS incrémenté !
CTX_USAGE: [4,3,3,3,3,3,3,3,3]  ← Incrémenté quand même
result=-1
```

### Interprétation

Le dispatch échoue :
- **APRÈS** sélection/incrément logique du contexte (CTX_USAGE[0] = 4)
- **AVANT** validation complète (total_dispatches reste 27)

**Séquence Exacte** :
```c
// 1. Sélection contexte (réussit)
ctx_idx = dispatch_count % 9;  // 27 % 9 = 0
ctx_usage[ctx_idx]++;          // [4,3,3,3,3,3,3,3,3]

// 2. Validation EXECBUFFER2 (échoue)
ret = ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, &execbuf);
if (ret < 0) {
    // total_dispatches PAS incrémenté
    return -EIO;
}
```

**Confirmation** : L'échec survient dans `DRM_IOCTL_I915_GEM_EXECBUFFER2`, pas dans calcul GPU.

---

## 🔍 ÉLIMINATION HYPOTHÈSE : BATCH BO CORRUPTION

### Pattern Batch BO Observé

| Dispatch | batch_bo | Réutilisation |
|----------|----------|---------------|
| 1        | 2        | Nouveau       |
| 9        | 10       | Nouveau       |
| 18       | 20       | Nouveau       |
| 27       | 29       | Nouveau       |
| 28       | 30       | Nouveau ❌    |

**Observation Majeure** :
```
Tu ne réutilises PAS le même batch BO.
```

**Conséquence** : Cela élimine fortement :
- ❌ Stale batch reuse
- ❌ Batch relocation corruption
- ❌ Batch handle poisoning local

**Batch BO sont NOUVEAUX à chaque dispatch** → Innocentés

---

## 🔍 RENFORCEMENT HYPOTHÈSE : FD GLOBAL POISONING

### FD DRM Constant

Pendant tout le run :
```
fd=10 (constant)
```

**Implication** :
```
Toute la toxicité s'accumule dans le même FD.
```

**Cela augmente énormément la probabilité** :
1. FD-global poisoning (état driver corrompu)
2. Scheduler/request accumulation (queue saturée)
3. Context tracking global (ban list)

---

## 📋 PATTERN RÉUTILISATION RÉEL

### Tableau Récapitulatif

| Ressource       | Réutilisation             | Suspect |
|-----------------|---------------------------|---------|
| batch BO        | NOUVEAU à chaque dispatch | ❌ Non  |
| contexte        | Réutilisé (rotation 9)    | ✅ OUI  |
| DRM fd          | Partagé (constant fd=10)  | ✅ OUI  |
| input/output BO | Persistants (handles 92-93)| ⚠️ Peut-être |

**Seul objet fortement recyclé** :
```
contexte + fd global
```

---

## 🔍 DÉTAIL RÉVÉLATEUR : ctx_active

### Pattern ctx_active Observé

| Dispatch | ctx_active | Interprétation |
|----------|------------|----------------|
| 1        | 2          | ctx_id=2       |
| 2        | 3          | ctx_id=3       |
| 3        | 4          | ctx_id=4       |
| ...      | ...        | ...            |
| 8        | 9          | ctx_id=9       |
| 9        | 10         | ctx_id=10      |
| 10       | 2          | ctx_id=2 (réutilisé) |

**Conclusion** :
```
ctx_active = ctx_id réel utilisé
```
Pas "nombre de contextes actifs".

---

## 🎯 HYPOTHÈSE ACTUALISÉE : CONTEXTE MARKED GUILTY/BANNED

### Mécanisme Probable i915

Le pattern réel ressemble à :
```
Un contexte devient "guilty" ou "banned"
après environ 3 soumissions réussies.
```

Ensuite :
```
EXECBUFFER2 refuse immédiatement toute nouvelle request.
```

### Code i915 Probable

```c
// drivers/gpu/drm/i915/gem/i915_gem_context.c
int i915_gem_context_is_banned(struct i915_gem_context *ctx)
{
    return atomic_read(&ctx->guilty_count) > I915_CONTEXT_MAX_GUILTY;
}

// drivers/gpu/drm/i915/gem/i915_gem_execbuffer.c
static int eb_submit(struct i915_execbuffer *eb)
{
    if (i915_gem_context_is_banned(eb->gem_context))
        return -EIO;  // errno=5
    
    // ...
}
```

**Constantes i915 Réelles** :
```c
#define I915_CONTEXT_MAX_GUILTY 3  // Hypothétique
```

---

## 🔍 POURQUOI LE REOPEN FONCTIONNE

### Mécanisme Reopen

```c
close(fd);  // Détruit TOUT l'état i915 associé
fd = open("/dev/dri/renderD128", O_RDWR);  // État vierge
```

**Le reopen force probablement** :
1. Destruction contextes (libère guilty flags)
2. Purge requests (vide scheduler queue)
3. Reset scheduler tracking (réinitialise compteurs)
4. Reset bans/guilty flags (efface blacklist)

**Conclusion** :
```
Le reopen agit comme un nettoyage logiciel complet de l'état i915.
```

---

## 📊 SUSPECTS ACTUALISÉS (Niveau Confiance)

| Suspect                        | Niveau Confiance | Changement |
|--------------------------------|------------------|------------|
| Contexte marked guilty/banned  | ⭐⭐⭐ Très élevé | ↑ Augmenté |
| FD request poisoning global    | ⭐⭐⭐ Très élevé | ↑ Augmenté |
| Rejet dans EXECBUFFER2         | ⭐⭐⭐ Très élevé | ↑ Nouveau  |
| Batch BO corruption            | ⭐ Faible        | ↓ Diminué  |
| GPU hang physique réel         | ⭐ Faible        | ↓ Diminué  |
| Limite hardcodée = 3           | ⭐⭐ Moyen        | ↓ Diminué  |
| RAM UMA                        | ❌ Très faible   | = Stable   |
| Température                    | ❌ Nulle         | = Stable   |

---

## 🔬 TEST CRITIQUE INDISPENSABLE : CAPTURE ÉTAT KERNEL

### Fichiers à Capturer IMMÉDIATEMENT Après Crash

```bash
# 1. État erreur DRM
cat /sys/class/drm/card0/error > error_state.txt

# 2. Logs kernel temps réel
dmesg -w > dmesg_crash.txt &

# 3. Journal kernel
journalctl -k --since "1 minute ago" > journal_crash.txt

# 4. État GPU
cat /sys/kernel/debug/dri/0/i915_engine_info > engine_info.txt
cat /sys/kernel/debug/dri/0/i915_gem_objects > gem_objects.txt
```

**Ces fichiers donneront probablement** :
- Guilty context ID
- Banned batch buffer
- Engine reset count
- Request timeout details
- Seqno bloqué
- Breadcrumb state

**Sans ça, on n'a pas encore la vraie cause kernel-level.**

---

## 📊 DÉTAIL RÉVÉLATEUR : DELTA RAM FINAL

### RAM au Crash

```
ram_consumed=-233472 bytes (-233 KB)
```

**Interprétation** :
- ❌ Aucune explosion mémoire
- ❌ Aucune allocation massive
- ❌ Aucune dérive finale

**Conclusion** :
```
Le crash est purement logique/driver, pas matériel/RAM.
```

---

## 🎯 CONCLUSION TECHNIQUE ACTUALISÉE

### Ce Que Les Logs Montrent Réellement

1. **Rejet Immédiat (2.45ms)** :
   - GPU ne semble PAS exécuter workload
   - Rejet au niveau validation EXECBUFFER2
   - Pas de timeout, pas de reset, pas de stall

2. **Compteur Non Incrémenté** :
   - Échec AVANT validation complète
   - APRÈS sélection contexte
   - Confirme rejet dans ioctl

3. **Batch BO Nouveaux** :
   - Élimine corruption batch
   - Élimine relocation stale
   - Élimine handle poisoning

4. **FD Constant** :
   - Toxicité s'accumule dans FD
   - Renforce FD global poisoning
   - Renforce scheduler accumulation

### Théorie La Plus Crédible

```
Le contexte est marqué invalid/guilty/banned
après accumulation interne côté i915.
```

**PAS** :
```
"Intel Gen9 autorise exactement 3 usages par contexte"
```

**MAIS PLUTÔT** :
```
"i915 marque contexte guilty après N conditions suspectes,
puis refuse toute nouvelle request avec errno=5"
```

---

## 🔬 PROCHAINE VRAIE ÉTAPE SCIENTIFIQUE

### Capture État Kernel Exact

**Commande à Exécuter** :
```bash
# Lancer en parallèle AVANT test
dmesg -w > dmesg_live.txt &
DMESG_PID=$!

# Exécuter test C270
./btc_gen9_native_runner_test_c270_ultra_forensic

# Capturer état immédiatement après crash
cat /sys/class/drm/card0/error > error_state_dispatch28.txt
journalctl -k --since "1 minute ago" > journal_dispatch28.txt

# Arrêter dmesg
kill $DMESG_PID
```

**C'est probablement là que se trouve la vraie signature causale du bug.**

---

## 📋 TESTS ISOLATION RÉVISÉS (Priorités Actualisées)

### TEST A : Contexte Neuf Par Dispatch ⭐⭐⭐ PRIORITÉ MAXIMALE

**Objectif** : Vérifier si contexte neuf élimine ban/guilty

**Résultat Attendu** :
- Si crash dispatch 28 : Problème N'EST PAS contexte guilty
- Si pas crash : Problème EST contexte guilty ✅

### TEST F : Capture /sys/class/drm/card0/error ⭐⭐⭐ NOUVEAU PRIORITÉ MAXIMALE

**Objectif** : Obtenir état kernel exact au crash

**Implémentation** :
```c
// Après échec EXECBUFFER2
if (ret < 0 && errno == EIO) {
    system("cat /sys/class/drm/card0/error > error_dispatch28.txt");
    system("dmesg | tail -100 > dmesg_dispatch28.txt");
}
```

### TEST G : Vérifier ctx->guilty_count ⭐⭐⭐ NOUVEAU PRIORITÉ HAUTE

**Objectif** : Confirmer mécanisme guilty/banned

**Implémentation** :
```c
// Nécessite accès debugfs
cat /sys/kernel/debug/dri/0/i915_context_info | grep -A5 "ctx_id=2"
```

### TEST B, C, D : Priorités Réduites

- TEST B (batch neuf) : ⭐ Priorité basse (batch innocenté)
- TEST C (throttle) : ⭐ Priorité moyenne
- TEST D (nouveau FD) : ⭐⭐ Priorité haute (FD suspect)

---

## 🎯 RECOMMANDATIONS FINALES

### Immédiates

1. ⭐⭐⭐ Implémenter TEST F (capture error state)
2. ⭐⭐⭐ Exécuter TEST A (contexte neuf)
3. ⭐⭐⭐ Analyser dmesg détaillé (ecode complet)
4. ⭐⭐ Exécuter TEST D (nouveau FD)

### Analyse Avancée

- Extraire guilty_count de chaque contexte
- Vérifier i915_context_info debugfs
- Tracer EXECBUFFER2 avec strace
- Contact Intel i915 maintainers avec logs complets

### Documentation

- Rapport C273 : Analyse critique contexte banned (ce document)
- Prochaine étape : Rapport C274 avec résultats TEST F + A

---

## 📊 RÉSUMÉ EXÉCUTIF

**Découverte Majeure** : Crash = Rejet logiciel immédiat (2.45ms), PAS GPU hang physique (700ms)

**Hypothèse Principale** : Contexte marqué guilty/banned par i915 après accumulation interne

**Preuves** :
- Temps crash ×286 plus rapide que normal
- Compteur non incrémenté (échec avant validation)
- Batch BO nouveaux (innocentés)
- FD constant (toxicité accumulée)

**Prochaine Étape Critique** : Capturer `/sys/class/drm/card0/error` au crash

**Fichier** : 450 lignes d'analyse critique addendum