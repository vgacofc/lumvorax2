#ifndef FUSION_DT_PLASMA_H_INCLUDED
#define FUSION_DT_PLASMA_H_INCLUDED

// SECTION 8: INTERDICTION D'UTILISER DES EMOJI (conforme lum_core.h)
//
// MODULE FUSION_DT_PLASMA — Moteur de simulation plasma deuterium-tritium
// =======================================================================
// Premier module de fusion nucleaire du systeme LUM/VORAX.
//
// PORTEE SCIENTIFIQUE HONNETE (a lire avant toute utilisation) :
// Ce module NE resout PAS "la fusion nucleaire". Il implemente un modele
// 0-D de bilan de puissance d'un plasma D-T thermonucleaire, base sur des
// donnees physiques publiees et verifiables :
//   - Reactivite <sigma-v> D-T : parametrisation de Bosch & Hale,
//     Nuclear Fusion 32 (1992) 611, validite 0.2 - 100 keV.
//   - Pertes bremsstrahlung : P_br = 5.35e-37 * Zeff * ne^2 * sqrt(T_keV) W/m^3
//     (formulaire NRL Plasma Formulary).
//   - Transport : temps de confinement de l'energie tau_E, soit fixe, soit
//     degrade avec la puissance de chauffage selon la loi d'echelle
//     IPB98(y,2) (tau_E proportionnel a P^-0.69), ce qui stabilise les
//     points de fonctionnement pilotes (physique POPCON des tokamaks).
//   - Critere de Lawson / triple produit n*T*tau_E pour l'ignition.
// Ce qui est HORS de portee d'un modele 0-D : turbulence 3D, profils
// radiaux, stabilite MHD (disruptions, ELMs), materiaux face au flux
// neutronique 14.07 MeV, systemes tritium. Voir le rapport associe.
//
// TRACABILITE (exigence projet : bit-level, nanoseconde par nanoseconde) :
//   - Chaque pas d'integration RK4 est horodate CLOCK_MONOTONIC ns et
//     journalise via ultra_forensic_logger (ring buffer NANO + CSV).
//   - L'etat plasma complet est hashe bit a bit (FNV-1a 64 bits sur la
//     representation memoire brute) a chaque pas : toute alteration d'un
//     seul bit de l'etat change le hash journalise.
//   - Le bilan de particules est projete sur des groupes LUM et verifie
//     par vorax_check_conservation() (conservation des unites de presence).

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../lum/lum_core.h"
#include "../common/magic_numbers.h"

// Magic numbers conformes STANDARD_NAMES (protection double-free)
#define FUSION_DT_PLASMA_MAGIC     0x46445450  // "FDTP"
#define FUSION_DT_PLASMA_DESTROYED MAGIC_DESTROYED_PATTERN

// Constantes physiques (CODATA / donnees nucleaires publiees)
#define FUSION_DT_KEV_TO_JOULE     1.602176634e-16   // 1 keV en joules (exact, SI 2019)
#define FUSION_DT_E_TOTAL_MEV      17.571             // Energie D+T -> alpha+n (MeV)
#define FUSION_DT_E_ALPHA_MEV      3.518              // Energie particule alpha (MeV)
#define FUSION_DT_E_NEUTRON_MEV    14.053             // Energie neutron (MeV)
#define FUSION_DT_E_TOTAL_J        (FUSION_DT_E_TOTAL_MEV * 1.602176634e-13)
#define FUSION_DT_E_ALPHA_J        (FUSION_DT_E_ALPHA_MEV * 1.602176634e-13)
#define FUSION_DT_E_NEUTRON_J      (FUSION_DT_E_NEUTRON_MEV * 1.602176634e-13)

// Constante bremsstrahlung (W m^3 keV^-1/2) — NRL Plasma Formulary
#define FUSION_DT_BREMS_COEFF      5.35e-37

// Domaine de validite de la parametrisation Bosch-Hale D-T
#define FUSION_DT_T_MIN_KEV        0.2
#define FUSION_DT_T_MAX_KEV        100.0

// Configuration d'un scenario plasma 0-D
typedef struct {
    double n_e_m3;            // Densite electronique (m^-3), plasma 50/50 D-T
    double T_keV;             // Temperature initiale (keV), Te = Ti
    double tau_E_s;           // Temps de confinement de l'energie (s)
    double volume_m3;         // Volume du plasma (m^3)
    double p_aux_W;           // Puissance de chauffage externe (W)
    double z_eff;             // Charge effective (pertes bremsstrahlung)
    double dt_s;              // Pas de temps d'integration (s)
    uint64_t max_steps;       // Nombre maximal de pas
    uint64_t log_every;       // Journaliser 1 pas sur N (1 = tous les pas)
    // Degradation du confinement avec la puissance (IPB98(y,2)) :
    // tau_eff = tau_E_s * (P_chauffage / p_ref_W)^(-tau_scaling_exponent)
    // 0.0 = tau_E fixe ; 0.69 = exposant IPB98(y,2) publie.
    double tau_scaling_exponent;
    double p_ref_W;           // Puissance de reference ou tau_E_s est defini
    // Dynamique des cendres d'helium et epuisement du combustible :
    // false = plasma D-T pur a composition fixe (modele simple).
    // true  = quasi-neutralite n_e = n_fuel + 2*n_He (n_e maintenue constante
    //         par injection de combustible — hypothese explicite), avec :
    //           dn_He/dt = R - n_He/tau_He,  R = (n_fuel/2)^2 <sigma-v>
    //           tau_He = tau_he_ratio * tau_E_eff (litterature : ~5)
    //           Z_eff dynamique = (n_fuel + 4*n_He) / n_e
    //         L'accumulation d'helium dilue le combustible et borne
    //         physiquement les excursions thermiques.
    bool enable_ash_dynamics;
    double tau_he_ratio;      // tau_He / tau_E (typiquement 5.0)
    // Loi d'echelle IPB98(y,2) COMPLETE (ITER Physics Basis, Nucl. Fusion 39
    // (1999) 2175) : le confinement n'est plus un parametre libre mais est
    // PREDIT a partir de la machine :
    //   tau_E = 0.0562 * H98 * I^0.93 * B^0.15 * n19^0.41 * M^0.19
    //           * R^1.97 * eps^0.58 * kappa^0.78 * P_MW^-0.69
    // (I en MA, B en T, n19 en 10^19 m^-3, R en m, P en MW). Quand ce mode
    // est actif, tau_E_s / tau_scaling_exponent / p_ref_W sont ignores.
    bool use_ipb98_full;
    double ipb98_I_MA;        // Courant plasma (MA)
    double ipb98_B_T;         // Champ toroidal sur l'axe (T)
    double ipb98_R_m;         // Grand rayon (m)
    double ipb98_epsilon;     // Rapport d'aspect inverse a/R
    double ipb98_kappa;       // Elongation
    double ipb98_M_amu;       // Masse isotopique moyenne (2.5 pour D-T)
    double ipb98_h98;         // Facteur H98 (1.0 = conforme base de donnees)
    // Facteur de piquage de profils : les plasmas reels ont des profils n(r),
    // T(r) piques qui augmentent <sigma-v n^2> par rapport au 0-D plat.
    // Les codes systemes (PROCESS) utilisent des profils paraboliques
    // analytiques equivalents. Ici : facteur multiplicatif sur le taux de
    // reactions, calibre sur le point de conception ITER (1.3).
    // 0.0 ou 1.0 = profils plats (comportement V1/V2 inchange).
    double profile_peaking;
} fusion_dt_config_t;

// Etat instantane du plasma (structure hashee bit a bit a chaque pas)
typedef struct {
    double time_s;            // Temps simule (s)
    double T_keV;             // Temperature courante (keV)
    double W_J_m3;            // Densite d'energie thermique 3*n*T (J/m^3)
    double p_fusion_W_m3;     // Puissance fusion totale volumique (W/m^3)
    double p_alpha_W_m3;      // Chauffage alpha volumique (W/m^3)
    double p_brems_W_m3;      // Pertes bremsstrahlung volumiques (W/m^3)
    double p_transport_W_m3;  // Pertes transport W/tau_E (W/m^3)
    double reactivity_m3_s;   // <sigma-v> courant (m^3/s)
    double q_factor;          // Q = P_fusion / P_aux (INFINITY si P_aux = 0)
    double tau_E_eff_s;       // tau_E effectif courant (apres degradation IPB98)
    double n_fuel_m3;         // Densite combustible D+T (= n_e si cendres off)
    double n_helium_m3;       // Densite cendres helium (0 si cendres off)
    double z_eff_dynamic;     // Z_eff courant (dynamique si cendres on)
    uint64_t step_index;      // Indice du pas courant
} fusion_dt_state_t;

// Resultat agrege d'une simulation de combustion (burn)
typedef struct {
    bool success;
    bool ignited;                   // Vrai si le plasma s'auto-entretient (P_alpha >= pertes)
    bool thermal_runaway_detected;  // Excursion thermique detectee (anomalie journalisee)
    double T_final_keV;
    double q_factor_final;          // Q au dernier pas
    double p_fusion_final_MW;       // Puissance fusion totale finale (MW)
    double p_net_electric_est_MW;   // Estimation nette electrique (voir .c, hypotheses explicites)
    double energy_produced_MJ;      // Energie fusion integree sur la simulation (MJ)
    double triple_product_keV_s_m3; // n * T_final * tau_E
    uint64_t steps_executed;
    uint64_t total_compute_time_ns; // Temps CPU reel de la boucle (CLOCK_MONOTONIC)
    uint64_t state_hash_final;      // Hash FNV-1a 64 bits de l'etat final (bit-level)
    char message[256];
} fusion_dt_burn_result_t;

// Point du balayage de Lawson
typedef struct {
    double T_keV;
    double reactivity_m3_s;
    double n_tau_required_s_m3;      // n*tau_E minimal pour ignition (s/m^3)
    double triple_product_keV_s_m3;  // n*T*tau_E minimal pour ignition
} fusion_dt_lawson_point_t;

// Resultat du balayage de Lawson (recherche du point de fonctionnement optimal)
typedef struct {
    bool success;
    double T_optimal_keV;            // Temperature minimisant le triple produit
    double triple_product_min;       // Triple produit minimal (keV s m^-3)
    double n_tau_min_s_m3;           // n*tau minimal (a T_ntau_optimal)
    double T_ntau_optimal_keV;       // Temperature minimisant n*tau
    size_t point_count;
    fusion_dt_lawson_point_t* points; // Tableau TRACKED_MALLOC (detruit avec le resultat)
    char message[256];
} fusion_dt_lawson_result_t;

// Moteur plasma (objet principal du module)
typedef struct {
    uint32_t magic_number;           // FUSION_DT_PLASMA_MAGIC
    fusion_dt_config_t config;
    fusion_dt_state_t state;
    lum_group_t* lum_group_deuterium; // Population D projetee en LUMs
    lum_group_t* lum_group_tritium;   // Population T projetee en LUMs
    lum_group_t* lum_group_alpha;     // Produits alpha (issus de vorax_fuse)
    double ions_per_lum;              // Facteur d'echelle ions reels / LUM
    uint64_t state_hash;              // Hash bit-level de l'etat courant
    uint64_t creation_timestamp_ns;
    void* memory_address;             // Tracabilite memoire (conforme lum_t)
} fusion_dt_plasma_t;

// ---- API physique pure (sans etat, testable unitairement) ----

// Reactivite D-T <sigma-v> (m^3/s) — Bosch & Hale 1992, T en keV.
// Retourne 0.0 hors du domaine [0.2, 100] keV (journalise en WARNING).
double fusion_dt_reactivity_bosch_hale(double T_keV);

// Puissance fusion volumique (W/m^3) pour un plasma 50/50 : (n/2)(n/2)<sv>E
double fusion_dt_power_density(double n_e_m3, double T_keV);

// Pertes bremsstrahlung volumiques (W/m^3)
double fusion_dt_bremsstrahlung(double n_e_m3, double T_keV, double z_eff);

// n*tau_E minimal pour ignition a T donne (s/m^3), pertes brems incluses.
// Retourne INFINITY si l'ignition est impossible a cette temperature.
double fusion_dt_lawson_n_tau(double T_keV, double z_eff);

// ---- Balayage de Lawson : recherche des points de fonctionnement ----
fusion_dt_lawson_result_t* fusion_dt_lawson_scan(double T_start_keV, double T_end_keV,
                                                 double T_step_keV, double z_eff);
void fusion_dt_lawson_result_destroy(fusion_dt_lawson_result_t** result_ptr);

// ---- Cycle de vie du moteur plasma ----
fusion_dt_plasma_t* fusion_dt_plasma_create(const fusion_dt_config_t* config);
void fusion_dt_plasma_destroy(fusion_dt_plasma_t** plasma_ptr);

// Un pas RK4 sur dW/dt = p_alpha + p_aux - p_brems - W/tau_E (densite fixe).
// Journalise metriques + hash bit-level au rythme config.log_every.
bool fusion_dt_plasma_step(fusion_dt_plasma_t* plasma);

// Simulation de combustion complete (max_steps ou regime stationnaire).
fusion_dt_burn_result_t fusion_dt_plasma_run_burn(fusion_dt_plasma_t* plasma);

// Projection du bilan de particules sur LUMs + verification de conservation
// VORAX (fusion D+T -> alpha : 2 reactifs consommes, 1 produit + 1 neutron).
bool fusion_dt_plasma_sync_lums(fusion_dt_plasma_t* plasma, size_t lum_population);

// Hash FNV-1a 64 bits de la representation memoire brute de l'etat
// (tracabilite bit-level : un bit modifie => hash different).
uint64_t fusion_dt_state_hash(const fusion_dt_state_t* state);

// ---- Optimiseur de point de fonctionnement (recherche de solution) ----

// Point de conception evalue par l'optimiseur
typedef struct {
    double n_e_m3;                  // Densite electronique du design
    double tau_E_s;                 // Confinement de reference du design
    double T_final_keV;             // Temperature stationnaire atteinte
    double q_factor;                // Q = P_fus / P_aux
    double p_fusion_MW;             // Puissance fusion totale
    double p_net_electric_MW;       // Production electrique nette estimee
    double helium_fraction;         // n_He / n_e a l'equilibre (dilution)
    bool viable;                    // Convergence sans excursion, T en domaine
} fusion_dt_design_point_t;

// Resultat de l'optimisation (balayage 2D densite x confinement)
typedef struct {
    bool success;
    fusion_dt_design_point_t best;  // Point maximisant P_net electrique
    size_t points_evaluated;
    size_t points_viable;
    fusion_dt_design_point_t* points; // Grille complete (TRACKED_MALLOC)
    uint64_t total_compute_time_ns;
    char message[256];
} fusion_dt_design_result_t;

// Balayage 2D (n_e, tau_E) : pour chaque point, simulation de combustion
// complete avec cendres d'helium + IPB98, evaluation de la production
// electrique nette. Retourne le meilleur design viable. Chaque point est
// journalise dans le ring buffer nanoseconde.
fusion_dt_design_result_t* fusion_dt_optimize_operating_point(
    double n_min_m3, double n_max_m3, size_t n_points,
    double tau_min_s, double tau_max_s, size_t tau_points,
    double volume_m3, double p_aux_W);
void fusion_dt_design_result_destroy(fusion_dt_design_result_t** result_ptr);

// ---- Integration lum_memory_tracer (bit-level natif du projet) ----

// Snapshot bit a bit de l'etat plasma au format .lum (1 LUM = 1 bit,
// magic LUMT), puis reconstruction et validation diff=0 par les fonctions
// natives lum_memory_snapshot_buffer / lum_memory_reconstruct /
// lum_memory_validate_diff_zero. Retourne true si diff=0 est prouve.
bool fusion_dt_snapshot_bit_level(const fusion_dt_plasma_t* plasma,
                                  const char* out_path);

#endif /* FUSION_DT_PLASMA_H_INCLUDED */
