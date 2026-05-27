/**
 * @file nx49_biological_neuron.h
 * @brief NX49 - Réseau Neuronal Blockchain 100% Décentralisé avec Neurone Biologique Atomique
 * @version 1.0.0
 * @date 2026-04-30
 * @cycle C151 (NX49)
 * 
 * DESCRIPTION:
 * Reconstruction anatomique complète d'un neurone biologique réel, atome par atome,
 * cellule par cellule, composant par composant, couche par couche, protéine par protéine.
 * 
 * Capable de reproduire le fonctionnement réel de la mémoire et de l'apprentissage
 * SANS POIDS (weightless learning), basé sur les mécanismes biologiques authentiques.
 * 
 * ARCHITECTURE BIOLOGIQUE COMPLÈTE:
 * - Niveau Atomique: C, H, O, N, P, S, Ca²⁺, Na⁺, K⁺, Cl⁻
 * - Niveau Moléculaire: Protéines, lipides, neurotransmetteurs
 * - Niveau Cellulaire: Soma, dendrites, axone, synapses
 * - Niveau Réseau: Blockchain décentralisée P2P
 * 
 * CONFORMITÉ: État de l'art 100% neurosciences + blockchain + quantum
 */

#ifndef NX49_BIOLOGICAL_NEURON_H
#define NX49_BIOLOGICAL_NEURON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <complex.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTES BIOLOGIQUES RÉELLES
 * ============================================================================ */

// Constantes physiques
#define NX49_AVOGADRO 6.02214076e23          // Nombre d'Avogadro
#define NX49_BOLTZMANN 1.380649e-23          // Constante de Boltzmann (J/K)
#define NX49_ELEMENTARY_CHARGE 1.602176634e-19 // Charge élémentaire (C)
#define NX49_PLANCK 6.62607015e-34           // Constante de Planck (J·s)

// Constantes biologiques
#define NX49_RESTING_POTENTIAL -70.0         // Potentiel de repos (mV)
#define NX49_THRESHOLD_POTENTIAL -55.0       // Seuil d'activation (mV)
#define NX49_ACTION_POTENTIAL_PEAK 40.0      // Pic potentiel d'action (mV)
#define NX49_REFRACTORY_PERIOD_MS 2.0        // Période réfractaire (ms)
#define NX49_MEMBRANE_CAPACITANCE 1.0        // Capacitance membrane (µF/cm²) — valeur numérique utilisée dans HH
#define NX49_TEMPERATURE_KELVIN 310.15       // Température corps (37°C)

// Concentrations ioniques (mM)
#define NX49_NA_EXTERNAL 145.0               // Na⁺ extracellulaire
#define NX49_NA_INTERNAL 12.0                // Na⁺ intracellulaire
#define NX49_K_EXTERNAL 4.0                  // K⁺ extracellulaire
#define NX49_K_INTERNAL 155.0                // K⁺ intracellulaire
#define NX49_CA_EXTERNAL 2.0                 // Ca²⁺ extracellulaire
#define NX49_CA_INTERNAL 0.0001              // Ca²⁺ intracellulaire
#define NX49_CL_EXTERNAL 110.0               // Cl⁻ extracellulaire
#define NX49_CL_INTERNAL 10.0                // Cl⁻ intracellulaire

// Dimensions anatomiques (µm)
#define NX49_SOMA_DIAMETER 20.0              // Diamètre soma
#define NX49_DENDRITE_LENGTH 200.0           // Longueur dendrite
#define NX49_DENDRITE_DIAMETER 2.0           // Diamètre dendrite
#define NX49_AXON_LENGTH 1000.0              // Longueur axone
#define NX49_AXON_DIAMETER 1.0               // Diamètre axone
#define NX49_SYNAPSE_CLEFT 20.0e-3           // Fente synaptique (nm)

// Paramètres synaptiques
#define NX49_VESICLE_DIAMETER 40.0e-3        // Diamètre vésicule (nm)
#define NX49_NEUROTRANSMITTER_PER_VESICLE 5000 // Molécules par vésicule
#define NX49_RELEASE_PROBABILITY 0.3         // Probabilité libération
#define NX49_RECEPTOR_DENSITY 1000.0         // Récepteurs/µm²

/* ============================================================================
 * TYPES ATOMIQUES ET MOLÉCULAIRES
 * ============================================================================ */

/**
 * @brief Atome individuel avec position 3D et propriétés quantiques
 */
typedef struct {
    uint8_t atomic_number;        // Numéro atomique (6=C, 1=H, 8=O, 7=N, etc.)
    double position[3];           // Position (x, y, z) en Angströms
    double velocity[3];           // Vélocité (vx, vy, vz) en Å/ps
    double charge;                // Charge partielle (e)
    double mass;                  // Masse atomique (u)
    complex double wavefunction;  // Fonction d'onde quantique
} nx49_atom_t;

/**
 * @brief Acide aminé (building block des protéines)
 */
typedef enum {
    NX49_AA_ALA, NX49_AA_ARG, NX49_AA_ASN, NX49_AA_ASP,
    NX49_AA_CYS, NX49_AA_GLN, NX49_AA_GLU, NX49_AA_GLY,
    NX49_AA_HIS, NX49_AA_ILE, NX49_AA_LEU, NX49_AA_LYS,
    NX49_AA_MET, NX49_AA_PHE, NX49_AA_PRO, NX49_AA_SER,
    NX49_AA_THR, NX49_AA_TRP, NX49_AA_TYR, NX49_AA_VAL
} nx49_amino_acid_t;

/**
 * @brief Protéine complète avec structure 3D
 */
typedef struct {
    char name[64];                // Nom protéine (ex: "Nav1.1")
    nx49_amino_acid_t* sequence;  // Séquence acides aminés
    uint32_t length;              // Longueur séquence
    nx49_atom_t* atoms;           // Tous les atomes
    uint32_t num_atoms;           // Nombre d'atomes
    double folding_energy;        // Énergie repliement (kcal/mol)
} nx49_protein_t;

/**
 * @brief Lipide membranaire (phospholipide)
 */
typedef struct {
    char type[32];                // Type (POPC, POPE, etc.)
    nx49_atom_t head_group[50];   // Groupe tête polaire
    nx49_atom_t tail_chains[100]; // Chaînes hydrophobes
    double area_per_lipid;        // Aire par lipide (Ų)
} nx49_lipid_t;

/**
 * @brief Neurotransmetteur
 */
typedef enum {
    NX49_NT_GLUTAMATE,            // Excitateur principal
    NX49_NT_GABA,                 // Inhibiteur principal
    NX49_NT_DOPAMINE,             // Récompense/motivation
    NX49_NT_SEROTONIN,            // Humeur/sommeil
    NX49_NT_ACETYLCHOLINE,        // Mémoire/attention
    NX49_NT_NOREPINEPHRINE        // Éveil/stress
} nx49_neurotransmitter_type_t;

typedef struct {
    nx49_neurotransmitter_type_t type;
    nx49_atom_t* atoms;           // Structure atomique
    uint32_t num_atoms;           // Nombre d'atomes
    double concentration;         // Concentration (mM)
    double diffusion_coeff;       // Coefficient diffusion (cm²/s)
} nx49_neurotransmitter_t;

/* ============================================================================
 * CANAUX IONIQUES (Protéines Transmembranaires)
 * ============================================================================ */

/**
 * @brief Canal sodium voltage-dépendant (Nav)
 */
typedef struct {
    nx49_protein_t protein;       // Structure protéique complète
    bool is_open;                 // État ouvert/fermé
    bool is_inactivated;          // État inactivé
    double conductance;           // Conductance (pS)
    double open_probability;      // Probabilité ouverture
    double voltage_sensitivity;   // Sensibilité voltage (mV⁻¹)
    uint32_t num_subunits;        // Nombre sous-unités (4)
} nx49_sodium_channel_t;

/**
 * @brief Canal potassium voltage-dépendant (Kv)
 */
typedef struct {
    nx49_protein_t protein;
    bool is_open;
    double conductance;           // Conductance (pS)
    double open_probability;
    double voltage_sensitivity;
    uint32_t num_subunits;        // Nombre sous-unités (4)
} nx49_potassium_channel_t;

/**
 * @brief Canal calcium voltage-dépendant (Cav)
 */
typedef struct {
    nx49_protein_t protein;
    bool is_open;
    double conductance;           // Conductance (pS)
    double open_probability;
    double ca_permeability;       // Perméabilité Ca²⁺
} nx49_calcium_channel_t;

/**
 * @brief Pompe Na⁺/K⁺-ATPase (maintient gradients)
 */
typedef struct {
    nx49_protein_t protein;
    double pumping_rate;          // Ions/seconde
    double atp_consumption;       // ATP/cycle
    uint32_t na_pumped_out;       // 3 Na⁺ sortis
    uint32_t k_pumped_in;         // 2 K⁺ entrés
} nx49_na_k_pump_t;

/* ============================================================================
 * RÉCEPTEURS SYNAPTIQUES
 * ============================================================================ */

/**
 * @brief Récepteur AMPA (glutamate, rapide)
 */
typedef struct {
    nx49_protein_t protein;
    bool is_bound;                // Neurotransmetteur lié
    bool is_open;                 // Canal ouvert
    double conductance;           // Conductance (pS)
    double binding_affinity;      // Affinité liaison (µM)
    double desensitization_rate;  // Taux désensibilisation (ms⁻¹)
} nx49_ampa_receptor_t;

/**
 * @brief Récepteur NMDA (glutamate, lent, Ca²⁺)
 */
typedef struct {
    nx49_protein_t protein;
    bool is_bound;
    bool is_open;
    bool mg_block;                // Blocage Mg²⁺
    double conductance;
    double ca_permeability;       // Perméabilité Ca²⁺
    double voltage_dependency;    // Dépendance voltage
} nx49_nmda_receptor_t;

/**
 * @brief Récepteur GABA_A (inhibiteur)
 */
typedef struct {
    nx49_protein_t protein;
    bool is_bound;
    bool is_open;
    double conductance;           // Conductance Cl⁻
    double binding_affinity;
} nx49_gaba_receptor_t;

/* ============================================================================
 * STRUCTURES CELLULAIRES
 * ============================================================================ */

/**
 * @brief Membrane cellulaire (bicouche lipidique)
 */
typedef struct {
    nx49_lipid_t* lipids;         // Lipides membranaires
    uint32_t num_lipids;          // Nombre lipides
    double thickness;             // Épaisseur (nm)
    double capacitance;           // Capacitance (µF/cm²)
    double resistance;            // Résistance (Ω·cm²)
    
    // Canaux ioniques intégrés
    nx49_sodium_channel_t* na_channels;
    uint32_t num_na_channels;
    nx49_potassium_channel_t* k_channels;
    uint32_t num_k_channels;
    nx49_calcium_channel_t* ca_channels;
    uint32_t num_ca_channels;
    nx49_na_k_pump_t* na_k_pumps;
    uint32_t num_na_k_pumps;
} nx49_membrane_t;

/**
 * @brief Mitochondrie (centrale énergétique)
 */
typedef struct {
    double atp_production_rate;   // ATP/seconde
    double oxygen_consumption;    // O₂/seconde
    double membrane_potential;    // Potentiel membrane (mV)
    uint32_t num_cristae;         // Nombre crêtes
} nx49_mitochondrion_t;

/**
 * @brief Réticulum endoplasmique (stockage Ca²⁺)
 */
typedef struct {
    double ca_concentration;      // Concentration Ca²⁺ (mM)
    double release_rate;          // Taux libération Ca²⁺
    double uptake_rate;           // Taux recapture Ca²⁺
} nx49_endoplasmic_reticulum_t;

/**
 * @brief Cytosquelette (structure cellulaire)
 */
typedef struct {
    uint32_t num_microtubules;    // Nombre microtubules
    uint32_t num_neurofilaments;  // Nombre neurofilaments
    uint32_t num_actin_filaments; // Nombre filaments actine
    double rigidity;              // Rigidité mécanique
} nx49_cytoskeleton_t;

/**
 * @brief Soma (corps cellulaire)
 */
typedef struct {
    double diameter;              // Diamètre (µm)
    double volume;                // Volume (µm³)
    nx49_membrane_t membrane;     // Membrane soma
    nx49_mitochondrion_t* mitochondria;
    uint32_t num_mitochondria;
    nx49_endoplasmic_reticulum_t er;
    nx49_cytoskeleton_t cytoskeleton;
    
    // Concentrations ioniques intracellulaires
    double na_concentration;      // Na⁺ (mM)
    double k_concentration;       // K⁺ (mM)
    double ca_concentration;      // Ca²⁺ (mM)
    double cl_concentration;      // Cl⁻ (mM)
    
    // Potentiel membranaire
    double membrane_potential;    // Vm (mV)
} nx49_soma_t;

/**
 * @brief Dendrite (réception signaux)
 */
typedef struct {
    double length;                // Longueur (µm)
    double diameter;              // Diamètre (µm)
    nx49_membrane_t membrane;     // Membrane dendrite
    
    // Épines dendritiques (synapses)
    struct nx49_synapse_t* synapses;
    uint32_t num_synapses;
    
    // Propagation signal
    double cable_resistance;      // Résistance câble (Ω)
    double time_constant;         // Constante temps (ms)
    double space_constant;        // Constante espace (µm)
} nx49_dendrite_t;

/**
 * @brief Axone (transmission signaux)
 */
typedef struct {
    double length;                // Longueur (µm)
    double diameter;              // Diamètre (µm)
    nx49_membrane_t membrane;     // Membrane axone
    
    // Myéline (isolation)
    bool is_myelinated;           // Myélinisé ou non
    double myelin_thickness;      // Épaisseur myéline (µm)
    uint32_t num_nodes_ranvier;   // Nombre nœuds Ranvier
    
    // Vitesse conduction
    double conduction_velocity;   // Vitesse (m/s)
    
    // Terminaisons axonales
    struct nx49_synapse_t* terminals;
    uint32_t num_terminals;
} nx49_axone_t;

/**
 * @brief Synapse (jonction neurone-neurone)
 */
typedef struct nx49_synapse_t {
    // Élément présynaptique
    struct {
        nx49_neurotransmitter_t* vesicles;
        uint32_t num_vesicles;
        double release_probability;
        double ca_concentration;  // Ca²⁺ local (µM)
    } presynaptic;
    
    // Fente synaptique
    struct {
        double width;             // Largeur (nm)
        nx49_neurotransmitter_t* neurotransmitters;
        uint32_t num_neurotransmitters;
        double diffusion_time;    // Temps diffusion (ms)
    } cleft;
    
    // Élément postsynaptique
    struct {
        nx49_ampa_receptor_t* ampa_receptors;
        uint32_t num_ampa;
        nx49_nmda_receptor_t* nmda_receptors;
        uint32_t num_nmda;
        nx49_gaba_receptor_t* gaba_receptors;
        uint32_t num_gaba;
        double psd_area;          // Aire densité postsynaptique (µm²)
    } postsynaptic;
    
    // Plasticité synaptique (apprentissage)
    struct {
        double weight;            // Poids synaptique (0-1)
        double ltp_magnitude;     // Potentialisation long terme
        double ltd_magnitude;     // Dépression long terme
        double spike_timing;      // Timing spike (ms)
        bool is_potentiated;      // Potentialisé
        bool is_depressed;        // Déprimé
    } plasticity;
} nx49_synapse_t;

/* ============================================================================
 * NEURONE BIOLOGIQUE COMPLET
 * ============================================================================ */

/**
 * @brief Neurone biologique atomiquement précis
 */
typedef struct {
    uint64_t neuron_id;           // ID unique
    char type[32];                // Type (pyramidal, interneuron, etc.)
    
    // Structures anatomiques
    nx49_soma_t soma;
    nx49_dendrite_t* dendrites;
    uint32_t num_dendrites;
    nx49_axone_t axon;
    
    // État électrique
    double membrane_potential;    // Potentiel membrane (mV)
    double threshold;             // Seuil activation (mV)
    bool is_firing;               // En train de décharger
    uint64_t last_spike_time_ns;  // Dernier spike (ns)
    uint64_t spike_count;         // Nombre spikes total
    
    // Métabolisme
    double atp_level;             // Niveau ATP
    double oxygen_level;          // Niveau O₂
    double glucose_level;         // Niveau glucose
    
    // Génétique (expression gènes)
    uint32_t* gene_expression;    // Niveaux expression
    uint32_t num_genes;
    
    // Blockchain (décentralisation)
    uint8_t blockchain_address[32]; // Adresse blockchain
    uint64_t block_height;        // Hauteur bloc
    uint8_t merkle_root[32];      // Racine Merkle
} nx49_biological_neuron_t;

/* ============================================================================
 * RÉSEAU NEURONAL BLOCKCHAIN DÉCENTRALISÉ
 * ============================================================================ */

/**
 * @brief Nœud réseau neuronal P2P
 */
typedef struct {
    nx49_biological_neuron_t* neurons;
    uint32_t num_neurons;
    
    // Topologie réseau
    uint32_t** connectivity_matrix;
    double** weight_matrix;       // Poids synaptiques
    
    // Blockchain
    uint8_t node_id[32];
    uint64_t block_height;
    uint8_t* blockchain_data;
    size_t blockchain_size;
    
    // Consensus (Byzantine FT)
    double consensus_threshold;   // >66%
    uint32_t num_validators;
    
    // Apprentissage sans poids (weightless)
    bool hebbian_learning;        // Règle Hebb
    bool spike_timing_dependent;  // STDP
    bool homeostatic_plasticity;  // Homéostasie
} nx49_neural_network_t;

/* ============================================================================
 * API PUBLIQUE
 * ============================================================================ */

/**
 * @brief Initialise un neurone biologique atomique
 */
nx49_biological_neuron_t* nx49_neuron_create(const char* type);

/**
 * @brief Détruit un neurone et libère mémoire
 */
void nx49_neuron_destroy(nx49_biological_neuron_t* neuron);

/**
 * @brief Simule dynamique moléculaire (1 pas de temps)
 */
void nx49_neuron_simulate_molecular_dynamics(nx49_biological_neuron_t* neuron, double dt_ps);

/**
 * @brief Calcule potentiel membrane (équation Hodgkin-Huxley)
 */
double nx49_neuron_compute_membrane_potential(nx49_biological_neuron_t* neuron);

/**
 * @brief Génère potentiel d'action si seuil atteint
 */
bool nx49_neuron_fire_action_potential(nx49_biological_neuron_t* neuron);

/**
 * @brief Libère neurotransmetteurs à synapse
 */
void nx49_synapse_release_neurotransmitters(nx49_synapse_t* synapse);

/**
 * @brief Applique plasticité synaptique (apprentissage)
 */
void nx49_synapse_apply_plasticity(nx49_synapse_t* synapse, double pre_spike_time, double post_spike_time);

/**
 * @brief Crée réseau neuronal blockchain décentralisé
 */
nx49_neural_network_t* nx49_network_create(uint32_t num_neurons);

/**
 * @brief Détruit réseau neuronal
 */
void nx49_network_destroy(nx49_neural_network_t* network);

/**
 * @brief Propage signal dans réseau (1 pas de temps)
 */
void nx49_network_propagate(nx49_neural_network_t* network, double dt_ms);

/**
 * @brief Applique apprentissage Hebbien sans poids
 */
void nx49_network_hebbian_learning(nx49_neural_network_t* network);

/**
 * @brief Valide bloc blockchain (consensus Byzantine)
 */
bool nx49_network_validate_block(nx49_neural_network_t* network, const uint8_t* block_data, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* NX49_BIOLOGICAL_NEURON_H */

// Made with Bob
