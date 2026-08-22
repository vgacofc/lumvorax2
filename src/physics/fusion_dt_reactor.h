#ifndef FUSION_DT_REACTOR_H_INCLUDED
#define FUSION_DT_REACTOR_H_INCLUDED

// SECTION 8: INTERDICTION D'UTILISER DES EMOJI (conforme lum_core.h)
//
// MODULE FUSION_DT_REACTOR — Conception contrainte de reacteurs tokamak D-T
// ==========================================================================
// Etage "code systeme" au-dessus du moteur plasma fusion_dt_plasma :
// geometrie tokamak, loi de confinement IPB98(y,2) complete (predite, plus
// un parametre libre), limites physiques et d'ingenierie publiees, et
// catalogues de materiaux. C'est la classe d'outils (PROCESS/UKAEA,
// SYCOMORE/CEA) utilisee pour concevoir les reacteurs reels.
//
// CONTRAINTES IMPLEMENTEES (toutes issues de la litterature) :
//   C1. Limite de densite de Greenwald : n <= f_GW * I_p/(pi a^2) 1e20
//       (Greenwald, Plasma Phys. Control. Fusion 44 (2002) R27).
//   C2. Limite beta de Troyon (stabilite MHD globale) :
//       beta_N = beta(%) a B / I_p <= beta_N_max (Troyon 1984, ~2.8-3.5).
//   C3. Facteur de securite q95 >= q95_min (disruptions) — formule ITER
//       Physics Design Guidelines (Uckan 1990), calibree : ITER => q95 = 3.00.
//   C4. Charge neutronique murale : 0.8 P_fus / S <= limite materiaux
//       (EUROFER ~1 MW/m2 ; W/avance ~2.5 ; hypothetique 4).
//   C5. Seuil de transition mode H : P_chauffage >= P_LH
//       (Martin et al., J. Phys. Conf. Ser. 123 (2008) 012033) —
//       IPB98(y,2) ne vaut qu'en mode H.
//   C6. Champ au conducteur : B_coil = B0 R/(R - a - d_gap) <= B_coil_max
//       (Nb3Sn ~12-13 T ; REBCO HTS ~23 T demontre SPARC/ARC).
//   C7. Regime opperationnel valide : T <= 25 keV. Au-dela, les pertes
//       synchrotron (proportionnelles a B^2 T^2, non modelisees ici) et le
//       courant de bootstrap deviennent dominants : le modele sort de son
//       domaine de confiance. Les reacteurs reels operent a 8-25 keV.
//   C8 (V4). Evacuation de puissance au divertor : P_separatrice/R <=
//       limite technologique (~15-20 MW/m, classe ITER tungstene). La
//       fraction rayonnee au bord est controlable par semis d'impuretes
//       (Ne/Ar) jusqu'a f_rad_max ; au-dela le design est invalide.
//   C9 (V5). Auto-suffisance en tritium : TBR = TBR_local * couverture
//       geometrique >= 1.05 (marge demarrage/pertes/decroissance). Le
//       tritium n'existe pas a l'etat naturel en quantite (stock mondial
//       civil ~25 kg) : une centrale consomme ~56 kg/an/GW_fus et DOIT les
//       regenerer dans sa couverture lithiee (6Li + n -> T + He). TBR_local
//       par concept publie : HCPB EU-DEMO ~1.30, FLiBe+Be ARC ~1.42.
//   V4 STATIONNARITE : un reacteur STATIONNAIRE doit entretenir son courant.
//       Fraction bootstrap f_bs = 0.7 sqrt(eps) beta_p (approximation
//       standard, Wesson "Tokamaks"), le reste par generation de courant
//       (efficacite gamma_CD = n20 R I_CD / P_CD, litterature 0.2-0.45).
//       La puissance de recirculation reelle est max(P_chauffage, P_CD) :
//       c'est le poste qui penalise les designs a fort courant et basse
//       pression (physique des "advanced tokamaks").
//
// CATALOGUES MATERIAUX : la reponse honnete a "developper de nouveaux
// materiaux" — un logiciel ne peut pas inventer un alliage reel (cela exige
// des annees d'irradiation experimentale), mais il peut QUANTIFIER ce que
// chaque generation de materiaux permet, et chiffrer les EXIGENCES que la
// prochaine generation devrait satisfaire. Les trois catalogues :
//   - LTS_ITER   : supraconducteurs Nb3Sn + acier EUROFER (etat ITER).
//   - HTS_REBCO  : rubans REBCO haute temperature critique + tungstene
//                  (etat SPARC/ARC, demontre en 2021-2026).
//   - FUTUR_HYPOTHETIQUE : cibles materiaux NON DEMONTREES, clairement
//                  etiquetees, pour chiffrer le gain si la R&D materiaux
//                  aboutit (B 32 T, mur 4 MW/m2, beta_N 3.5).

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "fusion_dt_plasma.h"

#define FUSION_DT_REACTOR_MAGIC 0x46525254  // "FRRT"

// Catalogue de materiaux / limites d'ingenierie
typedef struct {
    char name[48];
    double b_coil_max_T;         // Champ max au conducteur (T)
    double d_gap_m;              // Epaisseur couverture+ecran+structure (m)
    double wall_load_max_MW_m2;  // Charge neutronique murale max (MW/m^2)
    double beta_n_max;           // Limite de Troyon
    double q95_min;              // Facteur de securite minimal
    double greenwald_frac_max;   // Fraction de Greenwald max exploitable
    double eta_th;               // Rendement thermique -> electrique
    double eta_aux;              // Rendement des systemes de chauffage
    // V4 — divertor et stationnarite
    double div_limit_MW_m;       // Limite P_sep/R du divertor (MW/m)
    double f_rad_max;            // Fraction rayonnee au bord max (semis d'impuretes)
    double gamma_cd;             // Efficacite generation de courant n20*R*I/P (A/W*m^2/1e20)
    // V5 — couverture tritigene
    double tbr_local;            // TBR local du concept de couverture (publie)
    double blanket_coverage;     // Couverture geometrique (1 - ports - divertor)
    bool hypothetical;           // true = cibles materiaux NON demontrees
} fusion_dt_material_catalog_t;

// Machine tokamak derivee (geometrie + grandeurs deduites)
typedef struct {
    double R_m;          // Grand rayon
    double a_m;          // Petit rayon
    double kappa;        // Elongation
    double delta;        // Triangularite
    double B0_T;         // Champ toroidal sur l'axe
    double I_p_MA;       // Courant plasma
    double volume_m3;    // V = 2 pi^2 R a^2 kappa
    double surface_m2;   // S = 4 pi^2 R a sqrt((1+kappa^2)/2)
    double n_gw_m3;      // Densite de Greenwald (m^-3)
    double q95;          // Facteur de securite (Uckan)
    double b_coil_T;     // Champ au conducteur
} fusion_dt_machine_t;

// Resultat d'evaluation d'un design (equilibre de combustion + contraintes)
typedef struct {
    fusion_dt_machine_t machine;
    double n_e_m3;
    double p_aux_W;
    // Point de fonctionnement a l'equilibre (calcule par burn reel)
    double T_final_keV;
    double q_factor;
    double p_fusion_MW;
    double p_net_MW;
    double tau_E_s;              // Confinement PREDIT par IPB98(y,2)
    double he_fraction;
    // Contraintes evaluees
    double f_greenwald;          // n / n_GW
    double beta_n;               // Troyon
    double wall_load_MW_m2;      // Charge neutronique murale
    double p_lh_MW;              // Seuil L-H (Martin 2008)
    double p_heat_MW;            // Chauffage total a l'equilibre
    // V4 — stationnarite et divertor
    double beta_p;               // Beta poloidal a l'equilibre
    double f_bootstrap;          // Fraction de courant auto-genere
    double i_cd_MA;              // Courant a generer exterieurement
    double p_cd_MW;              // Puissance de generation de courant requise
    double p_recirc_MW;          // Recirculation reelle max(P_aux, P_CD)
    double p_sep_MW;             // Puissance a la separatrice (avant semis)
    double f_rad_required;       // Fraction rayonnee necessaire pour le divertor
    // V5 — tritium et profils
    double tbr;                  // Tritium Breeding Ratio du design
    double tritium_burn_kg_year; // Consommation tritium (kg/an)
    double tritium_margin_kg_year; // Production nette (kg/an), >0 si TBR>1
    double peaking_used;         // Piquage fusion calcule (profils radiaux)
    // V6 — indice economique physique
    // Energie magnetique stockee E_mag = B0^2/(2 mu0) x volume enveloppe par
    // les bobines (2 pi^2 R (a+d_gap)^2 kappa) : le premier poste de cout
    // d'un tokamak (validation ITER : ~27 GJ calcule, ~41 GJ publie systeme
    // TF complet — proxy conservateur du meme ordre).
    double e_mag_GJ;             // Energie magnetique stockee (GJ)
    double cost_index_GJ_MW;     // E_mag / P_net : cout capital par MW (relatif)
    bool c_greenwald, c_beta, c_q95, c_wall, c_lh, c_bcoil, c_regime,
         c_divertor, c_tbr;
    bool viable;                 // Toutes contraintes satisfaites + burn stable
} fusion_dt_reactor_point_t;

// Resultat d'optimisation contrainte pour un catalogue
typedef struct {
    bool success;
    fusion_dt_material_catalog_t catalog;
    fusion_dt_reactor_point_t best;    // Design viable maximisant P_net
    size_t points_evaluated;
    size_t points_viable;
    fusion_dt_reactor_point_t* points; // Grille complete (TRACKED_MALLOC)
    uint64_t total_compute_time_ns;
    char message[512];
} fusion_dt_reactor_result_t;

// ---- Catalogues materiaux (valeurs publiees, sauf le 3e : hypothetique) ----
fusion_dt_material_catalog_t fusion_dt_catalog_lts_iter(void);
fusion_dt_material_catalog_t fusion_dt_catalog_hts_rebco(void);
fusion_dt_material_catalog_t fusion_dt_catalog_future_hypothetical(void);

// ---- Geometrie et grandeurs derivees ----
// Derive la machine complete depuis (catalogue, R, rapport d'aspect A=R/a,
// kappa, delta, q95 cible) : B0 est pris au maximum permis par le catalogue
// (B0 = B_coil_max (R-a-d)/R), I_p fixe par la cible q95 (>= q95_min).
// q95 est un LEVIER DE CONCEPTION reel : ITER opere a 3.0, ARC a 7.2 —
// les machines a tres haut champ reduisent volontairement leur courant.
void fusion_dt_machine_derive(fusion_dt_machine_t* machine,
                              const fusion_dt_material_catalog_t* catalog,
                              double R_m, double aspect_ratio,
                              double kappa, double delta, double q95_target);

// Facteur de securite q95 — ITER Physics Design Guidelines (Uckan) :
// q95 = (5 a^2 B / (R I)) * (1+kappa^2(1+2 delta^2-1.2 delta^3))/2
//       * (1.17-0.65 eps)/(1-eps^2)^2      [calibre : ITER -> 3.00]
double fusion_dt_q95_uckan(double R_m, double a_m, double kappa, double delta,
                           double B0_T, double I_p_MA);

// Seuil de puissance L-H (Martin 2008) : P_LH [MW] =
//   0.0488 n20^0.717 B^0.803 S^0.941
double fusion_dt_p_lh_martin(double n_e_m3, double B0_T, double surface_m2);

// ---- Evaluation d'un design (burn reel + contraintes) ----
// steady_state=true  : centrale stationnaire — si P_CD > P_aux, le burn est
//                      re-simule avec p_aux = P_CD (coherence), et la
//                      recirculation reelle est max(P_aux, P_CD).
// steady_state=false : machine pulsee (ITER : courant par transformateur) —
//                      P_CD est calcule a titre informatif, recirc = P_aux.
bool fusion_dt_reactor_evaluate(const fusion_dt_material_catalog_t* catalog,
                                const fusion_dt_machine_t* machine,
                                double greenwald_fraction, double p_aux_W,
                                bool steady_state,
                                fusion_dt_reactor_point_t* out);

// ---- Optimisation contrainte (grille R x f_GW) ----
fusion_dt_reactor_result_t* fusion_dt_reactor_optimize(
    const fusion_dt_material_catalog_t* catalog,
    double R_min_m, double R_max_m, size_t R_points,
    double fgw_min, double fgw_max, size_t fgw_points,
    double aspect_ratio, double kappa, double delta, double p_aux_W);
void fusion_dt_reactor_result_destroy(fusion_dt_reactor_result_t** result_ptr);

#endif /* FUSION_DT_REACTOR_H_INCLUDED */
