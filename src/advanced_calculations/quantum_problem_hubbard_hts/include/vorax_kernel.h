/* VORAX kernel — Variational Orthogonal Refinement of Adaptive eXpectations.
 *
 * Module SOLVEUR REEL branche dans la boucle main du runner advanced_parallel.
 * Pas un wrapper de logging : effectue une descente coordonnee Givens sur les
 * angles variationnels HVA pour minimiser l'energie locale du probleme courant.
 *
 * C92-PLUS : Vecteur de correlation complet + extraction metrics + logs JSON
 *            forensiques par iteration + checksum FNV1a (auditable).
 *
 * API minimale, sans dependances externes (libm + pthread seulement).
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

/* C92-PLUS : Vecteur de correlation complet (extractable par NX/IBM/BTC). */
typedef struct {
    /* Identite */
    char        problem_name[64];
    int         N;                 /* n_sites */
    double      temperature;       /* temp_K */
    /* Parametres variationnels finaux */
    double      theta_h;
    double      theta_u;
    /* Observables physiques */
    double      energy;            /* E_out (apres optim) */
    double      delta_energy;      /* E_in - E_out (toujours >= 0) */
    double      delta_energy_norm; /* delta_energy / N (Q4 — normalisation par site) */
    /* Gradients numeriques */
    double      grad_theta_h;
    double      grad_theta_u;
    double      grad_norm;         /* sqrt(grad_h^2 + grad_u^2) */
    /* Courbure (Hessien diagonal approche) */
    double      curv_theta_h;
    double      curv_theta_u;
    /* Signature multi-echelle */
    double      chi_local;         /* |delta_energy_norm| / (1 + grad_norm) */
    /* Metriques d'extraction (score / stability / signal_strength) */
    double      score;             /* |delta_energy_norm| * log(1+|delta_energy|) */
    double      stability;         /* 1 / (1 + grad_norm) — 1 = stable */
    double      signal_strength;   /* delta_energy_norm / max(grad_norm, eps) */
    /* Trace d'optimisation */
    int         iter;              /* derniere iteration */
    int         n_evals;
    /* C92-FINAL : courbure 5-points (plus precise que 3-points) */
    double      curv_h_5pt;        /* d²E/dth² 5-point central */
    double      curv_u_5pt;        /* d²E/dtu² 5-point central */
    /* C92-FINAL : flag de classification physique */
    int         is_unstable;       /* 1 si stab < 0.30 (cible ZNE level 3) */
    int         is_pure_physics;   /* 1 si stab > 0.90 (reference propre) */
    int         feedback_rounds;   /* nb de rounds boucle fermee NX48-VORAX */
    /* Forensique */
    uint64_t    timestamp_ns;
    uint64_t    checksum_state;    /* FNV1a sur tous les champs precedents */
} correlation_vector_t;

/* Initialisation (alloue traceurs internes). Retourne 0 si OK. */
int vorax_kernel_init(const char *run_dir);

/* Cycle de raffinement variationnel sur 1 probleme.
 * Effectue une descente coordonnee Givens, max_iters iterations.
 * Met a jour p->theta_h, p->theta_u, p->energy_out, p->n_iters, p->n_evals.
 * Loggue chaque iteration dans run_dir/vorax_<problem>.log + .jsonl.
 * Retourne 0 si convergence, 1 si max_iters atteint, <0 si erreur.
 */
int vorax_kernel_refine_problem(vorax_problem_t *p,
                                 int max_iters,
                                 double tol_energy);

/* C92-PLUS : Construit le vecteur de correlation complet apres refine.
 * out doit etre alloue par l'appelant. Calcule grad/curv numeriquement
 * (eps=1e-6) et toutes les metriques d'extraction.
 * Loggue 1 ligne JSON dans run_dir/vorax_correlation.jsonl. */
int vorax_kernel_extract_correlation(const vorax_problem_t *p,
                                      correlation_vector_t *out);

/* C92-FINAL [A1] : Boucle fermee NX48 <-> VORAX.
 * Effectue jusqu'a max_rounds cycles de refine + extract + re-randomisation
 * theta si stability < target_stability. Retourne nb de rounds executes.
 * La meilleure correlation vector (max stability) est ecrite dans best_cv.
 * Loggue chaque round dans run_dir/vorax_feedback_<problem>.jsonl. */
int vorax_kernel_refine_with_feedback(vorax_problem_t *p,
                                       int max_rounds,
                                       double target_stability,
                                       int max_iters_per_round,
                                       double tol_energy,
                                       correlation_vector_t *best_cv);

/* C93 [A1+] : Boucle fermee gradient-aware (remplace Box-Muller random).
 * Utilise une etape SPSA-like : delta_theta = -lr * grad / (1 + |grad|)
 * + perturbation gaussienne sigma adaptative scale ~ (1 - stability).
 * Plus de re-randomisation aveugle : convergence guidee par la geometrie
 * locale du paysage d'energie.
 *   - lr  : taux d'apprentissage initial (0.05 par defaut)
 *   - sigma_explore : amplitude bruit exploratoire (0.05 par defaut)
 *   - alpha_stab : penalisation (1-stab) dans la perte (0.10 par defaut)
 *   - beta_var   : penalisation variance gradient (0.05 par defaut)
 * Loggue chaque round dans run_dir/vorax_feedback_c93_<problem>.jsonl. */
int vorax_kernel_refine_gradient_aware(vorax_problem_t *p,
                                        int max_rounds,
                                        double target_stability,
                                        int max_iters_per_round,
                                        double tol_energy,
                                        double lr,
                                        double sigma_explore,
                                        double alpha_stab,
                                        double beta_var,
                                        correlation_vector_t *best_cv);

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
