# Rapport 178 — BUG-PARALLEL-001 FERMÉ : fix pthread_join deadlock dans task_queue_dequeue()

**Session :** S173  
**Date :** 2026-10-03  
**Référence registre :** RAPPORT/176_REGISTRE_FERMETURE_INTEGRALE_S172_20261003.md  
**Référence précédente :** RAPPORT/177_INTEGRATION_LUM_OPT_001_v2_PASS_7_7_S172_20261003.md  
**Commit de référence :** à créer après ce rapport  
**CERTIFIED_100=false | unique_human_proven=false | Mode DEBUG actif**

---

## 1. Contexte

Le rapport 177 (S172) documentait honnêtement **BUG-PARALLEL-001** comme OPEN :

> `parallel_processor_destroy()` bloque indéfiniment sur `pthread_join` car
> `task_queue_dequeue()` (`parallel_processor.c` L197-L199) fait un
> `pthread_cond_wait` sans vérifier `should_exit` après réveil — les workers
> retournent immédiatement en attente quand la queue est vide.

Le workaround S172 omettait d'appeler `parallel_processor_destroy()`, laissant les workers fuir en mémoire.

La session S173 ferme ce chantier par correction réelle et preuve d'exécution.

---

## 2. Analyse de la cause racine

```
destroy()
  → workers[i].should_exit = true
  → pthread_cond_broadcast()        ← réveille tous les workers
                                      qui sont bloqués dans dequeue()
  → pthread_join()  ← bloque ici indéfiniment

worker_thread_main()
  while (!should_exit)
    task = task_queue_dequeue()      ← revient ici
      while (queue->head == NULL)   ← voit queue vide → re-attend
        pthread_cond_wait()         ← deadlock : broadcast déjà passé
```

**Root cause :** la condition `while (queue->head == NULL)` dans `task_queue_dequeue()` ne testait pas le flag `shutdown`. Quand `broadcast` réveillait les workers, ils testaient `queue->head == NULL` → `true` (queue vide) et retournaient bloquer sur `pthread_cond_wait`. Le `pthread_join` dans `destroy()` ne revenait jamais.

---

## 3. Modifications apportées (avant → après)

### 3.1 `src/parallel/parallel_processor.h` — ajout champ `shutdown`

**Fichier :** `src/parallel/parallel_processor.h`

**AVANT (L39-L46) :**
```c
// Task queue
typedef struct {
    parallel_task_t* head;
    parallel_task_t* tail;
    size_t count;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} task_queue_t;
```

**APRÈS (L39-L47) :**
```c
// Task queue
typedef struct {
    parallel_task_t* head;
    parallel_task_t* tail;
    size_t count;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    bool shutdown;  /* BUG-PARALLEL-001 FIX: signale aux workers que destroy() est en cours */
} task_queue_t;
```

---

### 3.2 `src/parallel/parallel_processor.c` — 3 points de modification

**Point A — `task_queue_init()` L140 :**

**AVANT :**
```c
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
```

**APRÈS :**
```c
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    queue->shutdown = false;  /* BUG-PARALLEL-001 FIX: initialiser à false */
```

---

**Point B — `parallel_processor_destroy()` L74-L83 :**

**AVANT :**
```c
    // Signal all workers to exit
    for (int i = 0; i < processor->worker_count; i++) {
        if (processor->workers[i].is_active) {
            processor->workers[i].should_exit = true;
        }
    }

    // Wake up all workers
    pthread_cond_broadcast(&processor->task_queue.condition);
```

**APRÈS :**
```c
    // Signal all workers to exit
    for (int i = 0; i < processor->worker_count; i++) {
        if (processor->workers[i].is_active) {
            processor->workers[i].should_exit = true;
        }
    }

    /* BUG-PARALLEL-001 FIX: positionner shutdown=true SOUS mutex AVANT le broadcast
     * pour que task_queue_dequeue() sorte de son while() quand la queue est vide. */
    pthread_mutex_lock(&processor->task_queue.mutex);
    processor->task_queue.shutdown = true;
    pthread_mutex_unlock(&processor->task_queue.mutex);

    // Wake up all workers
    pthread_cond_broadcast(&processor->task_queue.condition);
```

---

**Point C — `task_queue_dequeue()` L194-L215 :**

**AVANT :**
```c
parallel_task_t* task_queue_dequeue(task_queue_t* queue) {
    if (!queue) return NULL;

    pthread_mutex_lock(&queue->mutex);

    while (queue->head == NULL) {
        pthread_cond_wait(&queue->condition, &queue->mutex);
    }

    parallel_task_t* task = queue->head;
    queue->head = task->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    queue->count--;

    pthread_mutex_unlock(&queue->mutex);

    return task;
}
```

**APRÈS :**
```c
parallel_task_t* task_queue_dequeue(task_queue_t* queue) {
    if (!queue) return NULL;

    pthread_mutex_lock(&queue->mutex);

    /* BUG-PARALLEL-001 FIX: vérifier shutdown DANS la condition du while
     * pour éviter le deadlock lors de parallel_processor_destroy().
     * Si shutdown==true ET queue vide → retourner NULL → le worker sort. */
    while (queue->head == NULL && !queue->shutdown) {
        pthread_cond_wait(&queue->condition, &queue->mutex);
    }

    /* shutdown signalé ET queue vide : sortie propre du worker */
    if (queue->head == NULL) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }

    parallel_task_t* task = queue->head;
    queue->head = task->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    queue->count--;

    pthread_mutex_unlock(&queue->mutex);

    return task;
}
```

---

### 3.3 `src/tests/integration_lum_opt_001.c` — LOOP-005 mise à jour

**AVANT :** LOOP-005 omettait `parallel_processor_destroy()` (workaround deadlock, workers fuyaient).

**APRÈS :** LOOP-005 appelle `parallel_processor_destroy(proc)` après l'attente bornée. Le destroy retourne proprement.

Note de l'opération mise à jour :
```
"BUG-PARALLEL-001 FERME: destroy() propre — pthread_join sans deadlock."
```

---

## 4. Mécanisme de correction (séquence exacte)

```
destroy()
  → workers[i].should_exit = true          (1)
  → lock(mutex)
  → queue->shutdown = true                  (2) ← NOUVEAU
  → unlock(mutex)
  → pthread_cond_broadcast()               (3)
  → pthread_join()                         (4)

worker_thread_main()
  while (!should_exit)                      ← (1) true → sort
    task = task_queue_dequeue()
      lock(mutex)
      while (head==NULL && !shutdown)       ← (2) shutdown=true → sort du while
      if (head==NULL) → return NULL         ← retourne NULL
    if (!task)
      if (should_exit) break               ← break → worker_thread_main retourne
→ pthread_join() (4) retourne ✅
```

---

## 5. Résultats d'exécution

**Build :** `make bin/integration_lum_opt_001` — zéro warning, zéro erreur.

**Commande :** `./bin/integration_lum_opt_001 > logs/020_integration_lum_opt_001_v3_bug_parallel_fix.txt 2>&1`

**Exit code :** `0`

**Log LOOP-005 (ligne 204 du log) :**
```
LOOP-005   |      8 |      8 |    1322000 |       1 | SCAL | parallel_processor_create(workers=2) ok=1.
           Taches soumises=8/8. BUG-PARALLEL-001 FERME: destroy() propre — pthread_join sans deadlock.
```

**Preuve de libération mémoire (ligne 101 du log) :**
```
[MEMORY_TRACKER] FREE: 0x7f8a90a04520 (624 bytes) at src/parallel/parallel_processor.c:96
    in parallel_processor_destroy() - originally allocated at src/parallel/parallel_processor.c:21
```

La structure `parallel_processor_t` est bien libérée via `parallel_processor_destroy()` → `TRACKED_FREE(processor)`.

---

## 6. Verdict

```
[VERDICT] BUG-PARALLEL-001 : FERMÉ — S173
Fix : task_queue_t.shutdown + destroy() positionne shutdown avant broadcast
Preuve : exit 0, destroy() retourne, TRACKED_FREE confirmé
Log : logs/020_integration_lum_opt_001_v3_bug_parallel_fix.txt
CERTIFIED_100=false | unique_human_proven=false
```

---

## 7. Chantiers fermés cette session (S173)

| Chantier | État avant S173 | État après S173 |
|----------|----------------|----------------|
| BUG-PARALLEL-001 | OPEN — deadlock pthread_join | ✅ FERMÉ — sortie propre workers |

---

## 8. Chantiers restants ouverts (registre 176)

| Chantier | État |
|----------|------|
| Lyapunov robuste (multi-epsilon/warmup/intervalle) | OPEN |
| FORENSIC-UNIF-002 (BIT_ID/LUM_ID globalement uniques) | OPEN |
| BUILD-THREAD-001 (ThreadSanitizer) | OPEN |
| BUILD-PROOF-001 (CI C reproductible) | OPEN |
| BUILD-PORT-002 (cible sans ISA extensions) | OPEN |
| Richardson temporel complet (Chorin splitting) | OPEN |
| BL-003 → BL-012 (blockchain) | OPEN |
| C3 NS → NX-42 (intégration solveur NS réel) | OPEN |
| C4 NX-42 6-30 (problèmes 6–30 stub) | OPEN / STUB_MEASURED |

---

## 9. Logs produits

| Fichier | Description |
|---------|-------------|
| `logs/020_integration_lum_opt_001_v3_bug_parallel_fix.txt` | Run S173 — exit 0, 245 lignes, BUG-PARALLEL-001 fermé |
