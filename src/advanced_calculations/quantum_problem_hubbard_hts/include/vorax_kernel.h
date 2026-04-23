/* VORAX kernel — Variational Orthogonal Refinement of Adaptive eXpectations.
 *
 * Module SOLVEUR REEL branche dans la boucle main du runner advanced_parallel.
 * Pas un wrapper de logging : effectue une descente coordonnee Givens sur les
 * angles variationnels HVA pour minimiser l'energie locale du probleme courant.
 *
 * API minimale, sans dependances externes (libm seulement).
 */
#ifndef VORAX_KERNEL_H
#define VORAX_KERNEL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* Parametres physiques du probleme (lus depuis problems_cycle06.csv) */
    const char *problem_name;
    double      t_eV;
    double      U_eV;
    double      mu_eV;
    double      temp_K;
    int         n_sites;       /* taille systeme N */
    /* Angles variationnels HVA (theta_h, theta_u) — entree/sortie */
    double      theta_h;       /* hopping rotation XX+YY */
    double      theta_u;       /* onsite rotation ZZ      */
    /* Sortie : energie locale estimee + nb d'iterations effectuees */
    double      energy_in;     /* energie avant raffinement */
    double      energy_out;    /* energie apres raffinement */
    int         n_iters;
    int         n_evals;
} vorax_problem_t;

/* Initialisation (alloue traceurs internes). Retourne 0 si OK. */
int vorax_kernel_init(const char *run_dir);

/* Cycle de raffinement variationnel sur 1 probleme.
 * Effectue une descente coordonnee Givens, max_iters iterations.
 * Met a jour p->theta_h, p->theta_u, p->energy_out, p->n_iters, p->n_evals.
 * Loggue chaque iteration dans run_dir/vorax_<problem>.log.
 * Retourne 0 si convergence, 1 si max_iters atteint, <0 si erreur.
 */
int vorax_kernel_refine_problem(vorax_problem_t *p,
                                 int max_iters,
                                 double tol_energy);

/* Liberation traceurs. */
void vorax_kernel_destroy(void);

/* Statistiques globales (a appeler en fin de run). */
void vorax_kernel_stats(uint64_t *out_total_evals,
                        uint64_t *out_total_iters,
                        double   *out_total_dE);

#ifdef __cplusplus
}
#endif
#endif
