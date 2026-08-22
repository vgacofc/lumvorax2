#ifndef FUSION_DT_TRANSPORT_H_INCLUDED
#define FUSION_DT_TRANSPORT_H_INCLUDED

// SECTION 8: INTERDICTION D'UTILISER DES EMOJI (conforme lum_core.h)
//
// MODULE FUSION_DT_TRANSPORT — Solveur de transport radial 1.5-D stationnaire
// ===========================================================================
// Jusqu'a la V5, la FORME du profil de temperature etait imposee
// (parametrique (1-rho^2)^alpha). Ce module la PREDIT en resolvant
// l'equation de diffusion de la chaleur en geometrie cylindrique :
//
//   (1/r) d/dr ( r n chi dT/dr ) = - S(r)
//   S(r) = p_alpha(n(r),T(r)) + p_aux(r) - p_brems(n(r),T(r))
//
// En stationnaire, le flux de chaleur est q(r) = (1/r) int_0^r S r' dr',
// et le profil s'obtient par T(r) = T_bord + int_r^a q/(n chi) dr'.
//
// Fermeture (standard des codes 1.5-D simplifies) : la diffusivite chi est
// supposee uniforme et CALEE pour que la temperature moyenne atteigne la
// cible imposee par la loi de confinement IPB98(y,2) — la forme du profil
// devient alors une PREDICTION du modele, verifiable :
//   - bilan integral exact (flux sortant = sources integrees),
//   - piquage T0/<T> predit (litterature H-mode : ~1.5-3),
//   - piquage fusion du profil predit, comparable au parametrique V5.
// Le bord est un piedestal H-mode : T(a) = fraction pedestal * <T>.
//
// Boucle auto-coherente sources <-> profil (point fixe sous-relaxe).

#include <stdbool.h>
#include <stddef.h>

#define FUSION_DT_TRANSPORT_GRID_N   100   // points radiaux
#define FUSION_DT_TRANSPORT_MAX_ITER 200   // iterations de point fixe
#define FUSION_DT_PEDESTAL_FRACTION  0.30  // T_bord / <T> (piedestal H-mode)

typedef struct {
    bool converged;
    double T_avg_keV;        // temperature moyenne obtenue (== cible si converge)
    double T0_keV;           // temperature centrale PREDITE
    double T_edge_keV;       // temperature de bord (piedestal)
    double peaking_T;        // piquage T0/<T> PREDIT
    double peaking_fusion;   // piquage fusion du profil PREDIT
    double chi_m2_s;         // diffusivite calee (m^2/s)
    double p_source_MW;      // sources integrees (alpha + aux - brems)
    double p_outflux_MW;     // flux sortant au bord (== sources si conserve)
    double balance_error;    // |sources - flux| / sources (doit etre ~0)
    int iterations;
    double T_profile_keV[FUSION_DT_TRANSPORT_GRID_N + 1];  // T(rho) predit
} fusion_dt_transport_result_t;

// Resout le transport stationnaire pour un plasma de rayon mineur a,
// densite moyenne <n> (profil (1-rho^2)^0.3), volume V, chauffage externe
// p_aux uniforme, et temperature moyenne CIBLE T_avg (imposee par IPB98).
// peaking_fus_reference : piquage utilise pour le terme source alpha
// (auto-coherence avec le moteur 0-D).
bool fusion_dt_transport_solve(double a_m, double n_avg_m3, double volume_m3,
                               double p_aux_W, double T_avg_target_keV,
                               fusion_dt_transport_result_t* result);

#endif /* FUSION_DT_TRANSPORT_H_INCLUDED */
