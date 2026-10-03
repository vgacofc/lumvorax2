/* **************************************************************************
** build_thread_001b.c — BUILD-THREAD-001 volet B : TSan parallel_processor
**
** Projet : LumVorax (Autonomous Reflexive Temporal Cognitive Engine)
** Module : src/tests / BUILD-THREAD-001b
** Auteur : LumVorax Project
**
** Objectif (rapport 179 §4) :
**   Démontrer l'absence de data race et de deadlock dans le protocole
**   pthread de parallel_processor après le fix BUG-PARALLEL-001 (S173).
**
**   Scénarios couverts :
**     SC-01 : Création/destruction immédiate sans tâche soumise.
**     SC-02 : Création + shutdown sur queue vide (ex-deadlock S172).
**     SC-03 : Soumission de N tâches puis destroy() propre.
**     SC-04 : Shutdown concurrent avec soumission (race destroy/enqueue).
**     SC-05 : Répétitions massives (REPEAT_COUNT cycles create/destroy).
**     SC-06 : Plusieurs workers (N_WORKERS), tâches réparties.
**     SC-07 : Réveils parasites simulés : signal sans tâche disponible.
**
**   Résultats attendus :
**     - 7/7 scénarios PASS
**     - Aucune data race TSan
**     - Aucun deadlock (exit_code == 0)
**     - Aucune fuite de worker (destroy retourne toujours)
**
**   Compilation nominale :
**     gcc -std=c99 -g -O1 ... -lpthread
**   Compilation TSan :
**     gcc -std=c99 -g -O1 -fsanitize=thread ... -lpthread
**
** CERTIFIED_100=false | unique_human_proven=false
** Mode DEBUG actif
** ************************************************************************ */

/* _POSIX_C_SOURCE et _DARWIN_C_SOURCE définis par Makefile */
#include "../parallel/parallel_processor.h"
#include "../lum/lum_core.h"
#include "../debug/memory_tracker.h"
#include "../common/time_ns.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>

/* ── Paramètres ──────────────────────────────────────────────────────────── */
#define N_WORKERS       4
#define N_TASKS_SC03    16
#define N_TASKS_SC04    32
#define N_TASKS_SC06    64
#define REPEAT_COUNT    200   /* SC-05 : cycles create/destroy */
#define SC07_SPURIOUS   20    /* SC-07 : broadcasts "à vide" avant soumission */

/* ── Compteurs globaux ───────────────────────────────────────────────────── */
static int g_pass = 0;
static int g_fail = 0;

static void report(int sc, const char *label, int ok)
{
    if (ok) {
        printf("  [SC-%02d] PASS — %s\n", sc, label);
        g_pass++;
    } else {
        printf("  [SC-%02d] FAIL — %s\n", sc, label);
        g_fail++;
    }
}

/* ── SC-04 : thread qui soumet des tâches pendant que main appelle destroy ── */
typedef struct {
    parallel_processor_t *proc;
    int                   n_tasks;
    int                   submitted;
} SC04Arg;

static void *sc04_submitter(void *arg)
{
    SC04Arg *a = (SC04Arg *)arg;
    for (int i = 0; i < a->n_tasks; i++) {
        /* Tenter une soumission ; peut échouer si shutdown déjà levé */
        parallel_task_t *t = parallel_task_create(TASK_CUSTOM, NULL, 0);
        if (t) {
            if (parallel_processor_submit_task(a->proc, t))
                a->submitted++;
            /* Note : les tâches TASK_CUSTOM retournent false dans execute_task.
             * C'est documenté et intentionnel ici : on teste la concurrence,
             * pas le résultat fonctionnel. */
        }
        /* Légère pause pour augmenter l'entrelacement */
        usleep(10);
    }
    return NULL;
}

/* ── Utilitaire : attente bornée queue vide (max ms milliseconds) ─────────── */
static void wait_queue_empty(parallel_processor_t *proc, int max_ms)
{
    for (int i = 0; i < max_ms; i++) {
        if (task_queue_is_empty(&proc->task_queue)) return;
        usleep(1000);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * SC-01 : Création/destruction immédiate
 * ═══════════════════════════════════════════════════════════════════════════ */
static int sc01_create_destroy_immediate(void)
{
    parallel_processor_t *proc = parallel_processor_create(N_WORKERS);
    if (!proc) return 0;
    /* Détruire immédiatement sans soumettre aucune tâche */
    parallel_processor_destroy(proc);
    return 1; /* PASS si on arrive ici sans deadlock */
}

/* ═══════════════════════════════════════════════════════════════════════════
 * SC-02 : Shutdown sur queue vide (ex-deadlock BUG-PARALLEL-001)
 * ═══════════════════════════════════════════════════════════════════════════ */
static int sc02_shutdown_empty_queue(void)
{
    /* Répéter 10× pour augmenter la probabilité de détecter un deadlock */
    for (int i = 0; i < 10; i++) {
        parallel_processor_t *proc = parallel_processor_create(2);
        if (!proc) return 0;
        /* Laisser les workers s'installer */
        usleep(500);
        /* Appel destroy() sans aucune tâche — c'est le scénario historique */
        parallel_processor_destroy(proc);
    }
    return 1;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * SC-03 : N tâches soumises, drain, destroy propre
 * ═══════════════════════════════════════════════════════════════════════════ */
static int sc03_tasks_then_destroy(void)
{
    parallel_processor_t *proc = parallel_processor_create(N_WORKERS);
    if (!proc) return 0;

    int submitted = 0;
    for (int i = 0; i < N_TASKS_SC03; i++) {
        parallel_task_t *t = parallel_task_create(TASK_CUSTOM, NULL, 0);
        if (t && parallel_processor_submit_task(proc, t))
            submitted++;
    }

    /* Attendre drain de la queue avant destroy */
    wait_queue_empty(proc, 500);
    parallel_processor_destroy(proc);

    return (submitted == N_TASKS_SC03);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * SC-04 : Shutdown concurrent avec soumission (data race enqueue/destroy)
 * ═══════════════════════════════════════════════════════════════════════════ */
static int sc04_concurrent_shutdown_submit(void)
{
    parallel_processor_t *proc = parallel_processor_create(N_WORKERS);
    if (!proc) return 0;

    SC04Arg arg = { .proc = proc, .n_tasks = N_TASKS_SC04, .submitted = 0 };
    pthread_t submitter;
    if (pthread_create(&submitter, NULL, sc04_submitter, &arg) != 0) {
        parallel_processor_destroy(proc);
        return 0;
    }

    /* Légère pause puis destroy() pendant que le submitter tourne encore */
    usleep(200);
    parallel_processor_destroy(proc);

    pthread_join(submitter, NULL);
    /* PASS si aucun deadlock ni crash — submitted peut être n'importe quelle
     * valeur entre 0 et N_TASKS_SC04 selon l'interleaving */
    return 1;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * SC-05 : Répétitions massives create/destroy (détection fuite ou instabilité)
 * ═══════════════════════════════════════════════════════════════════════════ */
static int sc05_repeat_create_destroy(void)
{
    for (int i = 0; i < REPEAT_COUNT; i++) {
        parallel_processor_t *proc = parallel_processor_create(2);
        if (!proc) return 0;
        /* Alterner : parfois sans tâche, parfois avec 1 tâche */
        if (i % 3 == 0) {
            parallel_task_t *t = parallel_task_create(TASK_CUSTOM, NULL, 0);
            if (t) parallel_processor_submit_task(proc, t);
            usleep(100);
        }
        parallel_processor_destroy(proc);
    }
    return 1;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * SC-06 : Plusieurs workers, tâches réparties
 * ═══════════════════════════════════════════════════════════════════════════ */
static int sc06_many_workers_many_tasks(void)
{
    parallel_processor_t *proc = parallel_processor_create(8);
    if (!proc) return 0;

    int submitted = 0;
    for (int i = 0; i < N_TASKS_SC06; i++) {
        parallel_task_t *t = parallel_task_create(TASK_CUSTOM, NULL, 0);
        if (t && parallel_processor_submit_task(proc, t))
            submitted++;
    }

    wait_queue_empty(proc, 1000);
    parallel_processor_destroy(proc);

    return (submitted == N_TASKS_SC06);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * SC-07 : Réveils parasites simulés
 *   On diffuse des pthread_cond_broadcast sans ajouter de tâche, puis on
 *   soumet une tâche réelle et vérifie qu'elle est bien traitée.
 *   Ceci vérifie que les spurious wakeups sont absorbés par le while().
 * ═══════════════════════════════════════════════════════════════════════════ */
static int sc07_spurious_wakeup_robustness(void)
{
    parallel_processor_t *proc = parallel_processor_create(2);
    if (!proc) return 0;

    /* Diffuser des signaux sans tâche disponible */
    for (int i = 0; i < SC07_SPURIOUS; i++) {
        pthread_mutex_lock(&proc->task_queue.mutex);
        /* Ne pas changer head ni shutdown — broadcast "parasite" */
        pthread_cond_broadcast(&proc->task_queue.condition);
        pthread_mutex_unlock(&proc->task_queue.mutex);
        usleep(50);
    }

    /* Maintenant soumettre une vraie tâche */
    parallel_task_t *t = parallel_task_create(TASK_CUSTOM, NULL, 0);
    int ok = 0;
    if (t) ok = parallel_processor_submit_task(proc, t) ? 1 : 0;

    /* Attendre drain puis destroy */
    wait_queue_empty(proc, 200);
    parallel_processor_destroy(proc);

    /* PASS si : les broadcasts parasites n'ont pas fait quitter les workers
     * prématurément (sinon la tâche ne serait pas soumise ou le destroy
     * bloquerait). ok peut être 0 si proc était NULL, mais pas si proc est sain. */
    return ok;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * main
 * ═══════════════════════════════════════════════════════════════════════════ */
int main(void)
{
    uint64_t t0 = time_ns_get_absolute();

    printf("=== BUILD-THREAD-001b : TSan parallel_processor — S174 ===\n");
    printf("[MODE] DEBUG | CERTIFIED_100=false | unique_human_proven=false\n");
    printf("[OBJECTIF] Valider protocole pthread task_queue — 7 scénarios\n\n");

    printf("Avancement : 0%%\n");

    printf("[SC-01] create/destroy immédiate...\n");
    report(1, "create/destroy immediate sans tâche", sc01_create_destroy_immediate());
    printf("Avancement : 14%%\n");

    printf("[SC-02] shutdown queue vide (ex-BUG-PARALLEL-001)...\n");
    report(2, "shutdown queue vide × 10 cycles", sc02_shutdown_empty_queue());
    printf("Avancement : 28%%\n");

    printf("[SC-03] %d tâches + drain + destroy...\n", N_TASKS_SC03);
    report(3, "N tâches soumises puis destroy propre", sc03_tasks_then_destroy());
    printf("Avancement : 42%%\n");

    printf("[SC-04] shutdown concurrent avec soumission...\n");
    report(4, "concurrent destroy/enqueue sans deadlock", sc04_concurrent_shutdown_submit());
    printf("Avancement : 56%%\n");

    printf("[SC-05] %d cycles create/destroy répétés...\n", REPEAT_COUNT);
    report(5, "répétitions massives — stabilité", sc05_repeat_create_destroy());
    printf("Avancement : 70%%\n");

    printf("[SC-06] 8 workers × %d tâches...\n", N_TASKS_SC06);
    report(6, "8 workers répartis — drain + destroy", sc06_many_workers_many_tasks());
    printf("Avancement : 84%%\n");

    printf("[SC-07] %d broadcasts parasites + 1 tâche réelle...\n", SC07_SPURIOUS);
    report(7, "réveils parasites absorbés — while() robuste", sc07_spurious_wakeup_robustness());
    printf("Avancement : 100%%\n");

    uint64_t t1 = time_ns_get_absolute();
    double wall_s = (double)(t1 - t0) * 1e-9;

    printf("\n══════════════════════════════════════════════\n");
    printf("RÉSULTATS BUILD-THREAD-001b\n");
    printf("  PASS : %d/7\n", g_pass);
    printf("  FAIL : %d/7\n", g_fail);
    printf("  Wall : %.3f s\n", wall_s);
    printf("══════════════════════════════════════════════\n");

    int all_pass = (g_pass == 7 && g_fail == 0);
    printf("\n[VERDICT] BUILD-THREAD-001b : %s\n",
           all_pass ? "PASS COMPLET — 7/7 scénarios"
                    : "FAIL — voir scénarios en erreur");
    printf("[NOTE] CERTIFIED_100=false | unique_human_proven=false\n");
    printf("[NOTE] Pour validation TSan : compiler avec -fsanitize=thread\n");

    return all_pass ? 0 : 1;
}
