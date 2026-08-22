#ifndef FUSION_DT_PROFILES_H_INCLUDED
#define FUSION_DT_PROFILES_H_INCLUDED

// SECTION 8: INTERDICTION D'UTILISER DES EMOJI (conforme lum_core.h)
//
// MODULE FUSION_DT_PROFILES — Profils radiaux parametriques (fondation 1.5-D)
// ===========================================================================
// Les plasmas reels ne sont pas homogenes : densite et temperature sont
// piquees au centre. Ce module calcule par integration numerique radiale
// (Simpson) les facteurs de piquage exacts pour des profils parametriques
// standards (utilises par les codes systemes) :
//   n(rho) = n0 (1 - rho^2)^alpha_n,  T(rho) = T0 (1 - rho^2)^alpha_T
// ou rho est le rayon normalise. Moyennes volumiques (dV ~ 2 rho drho) :
//   <n> = n0/(1+alpha_n),  <T> = T0/(1+alpha_T).
//
// Facteur de piquage FUSION (le seul qui compte pour P_fus) :
//   f_fus = <n^2 sigmav(T)> / (<n>^2 sigmav(<T>))
// et facteur de piquage BREMSSTRAHLUNG :
//   f_brems = <n^2 sqrt(T)> / (<n>^2 sqrt(<T>))
//
// USAGE DANS LE CODE REACTEUR (honnetete de calibration) : la valeur absolue
// pour des paraboliques purs (~1.8-2.1) surestime les machines reelles car
// le piedestal H-mode et la dilution centrale ne sont pas modelises. Le
// reacteur utilise donc la FORME calculee, ANCREE sur la calibration ITER :
//   peaking_effectif(T) = 1.3 * f_fus(T) / f_fus(9 keV)
// (9 keV = temperature du point de calibration ITER du RAPPORT 151).

#include <stdbool.h>

// Parametres de profils standards (litterature codes systemes)
#define FUSION_DT_ALPHA_N_DEFAULT 0.3   // piquage densite H-mode modere
#define FUSION_DT_ALPHA_T_DEFAULT 1.2   // piquage temperature standard

// Facteur de piquage fusion exact par integration de Simpson (200 pas).
// T_avg_keV : temperature MOYENNE volumique. Retourne >= 1.0.
double fusion_dt_profiles_peaking_fusion(double T_avg_keV,
                                         double alpha_n, double alpha_T);

// Facteur de piquage bremsstrahlung exact (meme convention).
double fusion_dt_profiles_peaking_brems(double T_avg_keV,
                                        double alpha_n, double alpha_T);

// Piquage effectif ancre sur la calibration ITER (cf. en-tete) :
//   1.3 * f_fus(T_avg) / f_fus(9 keV)
double fusion_dt_profiles_effective_peaking(double T_avg_keV,
                                            double alpha_n, double alpha_T);

#endif /* FUSION_DT_PROFILES_H_INCLUDED */
