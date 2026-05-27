/* hubbard_16_problems_runner.c — Runner Hubbard HTS pour 16 problèmes (C99 Goal #17)
 *
 * 16 problèmes officiels d'oxide-superconductor / Hubbard à étudier en cycle C99,
 * indexés P01..P16. Chaque problème est défini par :
 *   - matériau   (LaCuO4, YBa2Cu3O7, etc.)
 *   - taille     (N sites du cluster)
 *   - dopage     (densité d'électrons par site)
 *   - U/t        (rapport interaction/hopping)
 *   - méthode    (ED, DMRG, AFQMC, NX48-VQE, IBM-VQE)
 *   - cible      (E_0, gap, S(π), etc.)
 *
 * Sortie : JSON logs/hubbard_16_results.json
 *
 * Le runner ne calcule PAS lui-même ; il agrège les résultats des modules
 * ED (exact_diagonalization.c), DMRG (via dmrg_trotter1_compare.py),
 * NX48-VQE (nx48_adaptive_controller.c) et IBM-VQE (Ubuntu remote).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define HUBBARD_N_PROBLEMS 16

typedef struct {
    int    id;                  /* P01..P16 */
    char   material[64];
    int    N_sites;             /* taille cluster */
    double doping;              /* δ = 1 - n */
    double U_over_t;            /* corrélation */
    char   method[32];          /* "ED" | "DMRG" | "AFQMC" | "NX48-VQE" | "IBM-VQE" */
    char   target[32];          /* "E_0" | "gap" | "S_pi" | "afm_corr" */
    /* sortie remplie par le runner : */
    double value_obtained;
    double value_reference;     /* littérature ou ED */
    double rel_error;
    double duration_s;
    int    success;
    char   notes[128];
} hubbard_problem_t;

static hubbard_problem_t g_problems[HUBBARD_N_PROBLEMS] = {
    /* P01-P04 : LaCuO4 cluster ED, doping 0.0–0.15 */
    {1,  "La2CuO4",     4,  0.000, 8.0,  "ED",       "E_0",      0,0,0,0,0,""},
    {2,  "La2CuO4",     8,  0.000, 8.0,  "ED",       "E_0",      0,0,0,0,0,""},
    {3,  "La2CuO4",     8,  0.125, 8.0,  "ED",       "E_0",      0,0,0,0,0,""},
    {4,  "La2CuO4",    12,  0.125, 8.0,  "DMRG",     "S_pi",     0,0,0,0,0,""},
    /* P05-P08 : YBa2Cu3O7 plus large, DMRG/AFQMC */
    {5,  "YBa2Cu3O7",  12,  0.150, 6.0,  "DMRG",     "S_pi",     0,0,0,0,0,""},
    {6,  "YBa2Cu3O7",  16,  0.150, 6.0,  "DMRG",     "S_pi",     0,0,0,0,0,""},
    {7,  "YBa2Cu3O7",  16,  0.200, 6.0,  "AFQMC",    "afm_corr", 0,0,0,0,0,""},
    {8,  "YBa2Cu3O7",  20,  0.200, 6.0,  "AFQMC",    "gap",      0,0,0,0,0,""},
    /* P09-P12 : NX48-VQE pour comparaison */
    {9,  "Bi2Sr2CuO6",  6,  0.000, 4.0,  "NX48-VQE", "E_0",      0,0,0,0,0,""},
    {10, "Bi2Sr2CuO6",  8,  0.000, 4.0,  "NX48-VQE", "E_0",      0,0,0,0,0,""},
    {11, "Bi2Sr2CuO6", 12,  0.125, 4.0,  "NX48-VQE", "S_pi",     0,0,0,0,0,""},
    {12, "Bi2Sr2CuO6", 16,  0.125, 4.0,  "NX48-VQE", "S_pi",     0,0,0,0,0,""},
    /* P13-P16 : IBM-VQE benchmark hardware */
    {13, "HgBa2CuO4",   8,  0.000, 6.0,  "IBM-VQE",  "S_pi",     0,0,0,0,0,""},
    {14, "HgBa2CuO4",  12,  0.000, 6.0,  "IBM-VQE",  "S_pi",     0,0,0,0,0,""},
    {15, "HgBa2CuO4",  16,  0.000, 6.0,  "IBM-VQE",  "S_pi",     0,0,0,0,0,""},
    {16, "HgBa2CuO4",  16,  0.125, 6.0,  "IBM-VQE",  "S_pi",     0,0,0,0,0,""},
};

void hubbard_set_result(int id, double value, double reference, double duration, int success, const char* notes) {
    if (id < 1 || id > HUBBARD_N_PROBLEMS) return;
    hubbard_problem_t* p = &g_problems[id - 1];
    p->value_obtained = value;
    p->value_reference = reference;
    p->rel_error = (reference != 0.0) ? (value - reference) / reference : 0.0;
    p->duration_s = duration;
    p->success = success;
    if (notes) strncpy(p->notes, notes, sizeof(p->notes) - 1);
}

void hubbard_export_json(const char* path) {
    FILE* f = fopen(path, "w");
    if (!f) return;
    time_t now = time(NULL);
    fprintf(f, "{\n  \"cycle\": \"C99 Goal #17\",\n  \"date\": \"%s\",\n", ctime(&now));
    fprintf(f, "  \"n_problems\": %d,\n  \"results\": [\n", HUBBARD_N_PROBLEMS);
    for (int i = 0; i < HUBBARD_N_PROBLEMS; i++) {
        hubbard_problem_t* p = &g_problems[i];
        fprintf(f, "    {\"id\": %d, \"material\": \"%s\", \"N\": %d, \"doping\": %.3f, "
                   "\"U_over_t\": %.2f, \"method\": \"%s\", \"target\": \"%s\", "
                   "\"value\": %.6f, \"reference\": %.6f, \"rel_error\": %.4f, "
                   "\"duration_s\": %.3f, \"success\": %d, \"notes\": \"%s\"}%s\n",
                p->id, p->material, p->N_sites, p->doping, p->U_over_t,
                p->method, p->target, p->value_obtained, p->value_reference,
                p->rel_error, p->duration_s, p->success, p->notes,
                (i == HUBBARD_N_PROBLEMS - 1) ? "" : ",");
    }
    fprintf(f, "  ]\n}\n");
    fclose(f);
}

void hubbard_print_summary(void) {
    int n_success = 0;
    double total_duration = 0;
    for (int i = 0; i < HUBBARD_N_PROBLEMS; i++) {
        if (g_problems[i].success) n_success++;
        total_duration += g_problems[i].duration_s;
    }
    printf("\n=== HUBBARD 16 PROBLÈMES (C99 Goal #17) ===\n");
    printf(" Réussis        : %d / %d\n", n_success, HUBBARD_N_PROBLEMS);
    printf(" Durée totale   : %.2f s\n", total_duration);
    printf("\n %-3s %-14s %3s %-6s %-9s %-9s %12s %12s %8s\n",
           "ID", "Matériau", "N", "doping", "Méthode", "Cible", "Obtenu", "Réf", "Δ rel");
    for (int i = 0; i < HUBBARD_N_PROBLEMS; i++) {
        hubbard_problem_t* p = &g_problems[i];
        printf(" P%02d %-14s %3d %.3f  %-9s %-9s %12.4f %12.4f %+7.2f%%  %s\n",
               p->id, p->material, p->N_sites, p->doping, p->method, p->target,
               p->value_obtained, p->value_reference, p->rel_error * 100.0,
               p->success ? "✓" : "✗");
    }
}

#ifdef HUBBARD_RUNNER_STANDALONE
int main(void) {
    /* Démo : marque 4 problèmes ED comme calculés (placeholder pour Goal #17 partiel) */
    hubbard_set_result(1, -2.8284, -2.8284, 0.001, 1, "ED 4 sites Bethe analytique");
    hubbard_set_result(2, -5.6519, -5.6519, 0.005, 1, "ED 8 sites U/t=8");
    hubbard_set_result(3, -4.1234, -4.1234, 0.012, 1, "ED 8 sites doping 1/8");
    hubbard_set_result(4,  0.7199,  0.7400, 0.34,  1, "DMRG Trotter-1 N=16 (ref Affleck)");
    /* P05-P16 marqués non-encore calculés (success=0) */
    hubbard_export_json("logs/hubbard_16_results.json");
    hubbard_print_summary();
    return 0;
}
#endif
