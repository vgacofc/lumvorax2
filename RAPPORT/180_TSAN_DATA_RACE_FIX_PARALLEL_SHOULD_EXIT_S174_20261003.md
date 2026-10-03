# Rapport 180 — TSan data race corrigée : should_exit sans mutex dans parallel_processor

**Session :** S174  
**Date :** 2026-10-03  
**Référence :** RAPPORT/179_AUDIT_FERMETURE_INTEGRALE_POST_S173_20261003.md (§4 BUILD-THREAD-001)  
**Commit précédent :** 05623cf (S173 BUG-PARALLEL-001)  
**CERTIFIED_100=false | unique_human_proven=false | Mode DEBUG actif**

---

## 1. Contexte

Le rapport 179 (S174, auditeur externe) exigeait une campagne TSan pour valider le protocole
de condition variable après le fix S173. BUILD-THREAD-001b a été créé à cet effet.

Premier run TSan → **2 data races détectées** sur `workers[i].should_exit`.

---

## 2. Data race identifiée par TSan

```
WARNING: ThreadSanitizer: data race (pid=80561)
  Write of size 1 at 0x7b540000028d by main thread:
    #0 parallel_processor_destroy parallel_processor.c:74

  Previous read of size 1 at 0x7b540000028d by thread T1:
    #0 worker_thread_main parallel_processor.c:268
```

**Root cause :**  
- `parallel_processor_destroy()` **écrivait** `workers[i].should_exit = true` **sans mutex** (L74).  
- `worker_thread_main()` **lisait** `processor->workers[worker_id].should_exit` **sans mutex** (L268, L272).  
- Ces deux accès concurrent sans synchronisation = data race POSIX.

---

## 3. Modifications apportées (avant → après)

### 3.1 `src/parallel/parallel_processor.c` — `parallel_processor_destroy()`

**Fichier :** `src/parallel/parallel_processor.c`

**AVANT (L72-L82) :**
```c
    // Signal all workers to exit
    for (int i = 0; i < processor->worker_count; i++) {
        if (processor->workers[i].is_active) {
            processor->workers[i].should_exit = true;   // SANS MUTEX
        }
    }

    /* BUG-PARALLEL-001 FIX */
    pthread_mutex_lock(&processor->task_queue.mutex);
    processor->task_queue.shutdown = true;
    pthread_mutex_unlock(&processor->task_queue.mutex);
```

**APRÈS (L68-L80) :**
```c
    /* TSan FIX (S174) : should_exit ET shutdown positionnés dans la MÊME
     * section critique, sous queue->mutex, avant le broadcast.
     * Les workers lisent shutdown dans task_queue_dequeue() sous mutex → race éliminée. */
    pthread_mutex_lock(&processor->task_queue.mutex);
    for (int i = 0; i < processor->worker_count; i++) {
        processor->workers[i].should_exit = true;   // SOUS MUTEX
    }
    processor->task_queue.shutdown = true;
    pthread_mutex_unlock(&processor->task_queue.mutex);
```

---

### 3.2 `src/parallel/parallel_processor.c` — `worker_thread_main()`

**AVANT (L268-L273) :**
```c
    while (worker_id >= 0 && !processor->workers[worker_id].should_exit) {  // READ hors mutex
        parallel_task_t* task = task_queue_dequeue(&processor->task_queue);
        if (!task) {
            if (processor->workers[worker_id].should_exit) break;  // READ hors mutex
            continue;
        }
```

**APRÈS (L265-L275) :**
```c
    while (1) {
        parallel_task_t* task = task_queue_dequeue(&processor->task_queue);
        if (!task) {
            /* task_queue_dequeue retourne NULL uniquement si shutdown=true ET
             * queue vide — lu sous queue->mutex → sans race. */
            break;
        }
```

La résolution de `worker_id` (6 lignes) est également supprimée — inutile puisque les stats
utilisent `pthread_self()` directement en boucle (L295).

---

## 4. Invariant de synchronisation après correction

| Accès | Variable | Mutex | Résultat |
|-------|----------|-------|---------|
| `destroy()` écrit `should_exit` | `queue->mutex` | ✅ protégé | |
| `destroy()` écrit `shutdown` | `queue->mutex` | ✅ protégé | |
| `destroy()` broadcast | après unlock | ✅ séquentiel | |
| `dequeue()` lit `shutdown` | `queue->mutex` | ✅ protégé | |
| `worker` sort sur NULL de `dequeue` | — | ✅ pas de lecture directe | |

`should_exit` n'est plus lu hors mutex. Le seul canal de sortie des workers est désormais
le retour `NULL` de `task_queue_dequeue()`, ce qui est atomiquement corrélé à `shutdown=true`.

---

## 5. Résultats d'exécution

### Run TSan v1 (avant correction) — `logs/022_build_thread_001b_tsan.txt`
```
2 data races détectées
ThreadSanitizer: reported 2 warnings
EXIT_CODE=134
```

### Run TSan v2 (après correction) — `logs/023_build_thread_001b_tsan_v2.txt`
```
ThreadSanitizer warnings : 0
7/7 PASS
EXIT_CODE=0
```

### Non-régression integration_lum_opt_001
```
[VERDICT] INTEGRATION-LUM-OPT-001-v2 : PASS COMPLET — 7/7 modules D-executes
EXIT_CODE=0
```

---

## 6. Verdict

```
[VERDICT] BUILD-THREAD-001 TSan : PASS — zéro data race, 7/7 scénarios
Fix : should_exit positionné sous queue->mutex + worker_thread_main sans lecture hors mutex
Log TSan : logs/023_build_thread_001b_tsan_v2.txt
CERTIFIED_100=false | unique_human_proven=false
```

---

## 7. Chantiers fermés cette session (S174)

| Chantier | État avant S174 | État après S174 |
|----------|----------------|----------------|
| Robustesse condition variable (spurious wakeup) | OPEN — preuve manquante | ✅ PROUVÉ — invariant explicite dans code + SC-07 PASS |
| BUILD-THREAD-001 TSan parallel_processor | OPEN | ✅ FERMÉ — 0 data race TSan |

---

## 8. Chantiers restants ouverts

| Chantier | État |
|----------|------|
| BUILD-PROOF-001 (CI C reproductible) | OPEN |
| BUILD-PORT-002 (cible sans ISA extensions) | OPEN |
| FORENSIC-UNIF-002 (BIT_ID/LUM_ID uniques) | OPEN |
| Richardson temporel Chorin complet | OPEN |
| T04 stationnarité forte | OPEN |
| Lyapunov robuste | OPEN |
| BL-003 → BL-012 (blockchain) | OPEN |
| C3 NS → NX-42 | OPEN |
| C4 NX-42 6–30 | OPEN / STUB_MEASURED |

---

## 9. Logs produits

| Fichier | Description |
|---------|-------------|
| `logs/021_build_thread_001b_nominal.txt` | Run nominal S174 — 7/7 PASS, exit 0 |
| `logs/022_build_thread_001b_tsan.txt` | Run TSan v1 — 2 races détectées, exit 134 |
| `logs/023_build_thread_001b_tsan_v2.txt` | Run TSan v2 post-fix — 0 race, 7/7 PASS, exit 0 |
