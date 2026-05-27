/**
 * @file nx49_biological_neuron.c
 * @brief Implémentation NX49 - Neurone Biologique Atomique + Blockchain
 * @version 1.0.0
 * @date 2026-04-30
 * @cycle C151 (NX49)
 */

#include "nx49_biological_neuron.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ============================================================================
 * UTILITAIRES MATHÉMATIQUES
 * ============================================================================ */

static inline double nernst_potential(double z, double c_out, double c_in) {
    // Équation de Nernst: E = (RT/zF) * ln(c_out/c_in)
    // F = constante de Faraday = 96485 C/mol  (NX49_ELEMENTARY_CHARGE est la charge
    // d'un seul proton 1.6e-19 C — mauvaise unité pour la formule de Nernst)
    const double FARADAY = 96485.0;  // C/mol
    const double RT_F = (8.314 * NX49_TEMPERATURE_KELVIN) / FARADAY; // V (~26.7 mV)
    return (RT_F / z) * log(c_out / c_in) * 1000.0; // mV
}

static inline double boltzmann_distribution(double energy_kj_mol) {
    // P = exp(-E/kT)
    double energy_j = energy_kj_mol * 1000.0 / NX49_AVOGADRO;
    return exp(-energy_j / (NX49_BOLTZMANN * NX49_TEMPERATURE_KELVIN));
}

/* ============================================================================
 * CRÉATION/DESTRUCTION NEURONE
 * ============================================================================ */

nx49_biological_neuron_t* nx49_neuron_create(const char* type) {
    nx49_biological_neuron_t* neuron = calloc(1, sizeof(nx49_biological_neuron_t));
    if (!neuron) return NULL;
    
    // ID unique basé sur timestamp + random
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    neuron->neuron_id = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    
    strncpy(neuron->type, type, sizeof(neuron->type) - 1);
    
    // === SOMA (Corps cellulaire) ===
    neuron->soma.diameter = NX49_SOMA_DIAMETER;
    neuron->soma.volume = (4.0/3.0) * M_PI * pow(NX49_SOMA_DIAMETER/2.0, 3);
    neuron->soma.membrane_potential = NX49_RESTING_POTENTIAL;
    
    // Concentrations ioniques initiales
    neuron->soma.na_concentration = NX49_NA_INTERNAL;
    neuron->soma.k_concentration = NX49_K_INTERNAL;
    neuron->soma.ca_concentration = NX49_CA_INTERNAL;
    neuron->soma.cl_concentration = NX49_CL_INTERNAL;
    
    // Membrane soma
    neuron->soma.membrane.thickness = 5.0; // nm
    neuron->soma.membrane.capacitance = NX49_MEMBRANE_CAPACITANCE;
    neuron->soma.membrane.resistance = 1000.0; // Ω·cm²
    
    // Canaux ioniques Na+ (densité: 100-500/µm²)
    double soma_area = M_PI * pow(NX49_SOMA_DIAMETER, 2); // µm²
    neuron->soma.membrane.num_na_channels = (uint32_t)(soma_area * 300);
    neuron->soma.membrane.na_channels = calloc(neuron->soma.membrane.num_na_channels, 
                                               sizeof(nx49_sodium_channel_t));
    
    for (uint32_t i = 0; i < neuron->soma.membrane.num_na_channels; i++) {
        neuron->soma.membrane.na_channels[i].conductance = 20.0; // pS
        neuron->soma.membrane.na_channels[i].voltage_sensitivity = 0.1; // mV⁻¹
        neuron->soma.membrane.na_channels[i].num_subunits = 4;
        neuron->soma.membrane.na_channels[i].is_open = false;
    }
    
    // Canaux ioniques K+ (densité: 50-200/µm²)
    neuron->soma.membrane.num_k_channels = (uint32_t)(soma_area * 100);
    neuron->soma.membrane.k_channels = calloc(neuron->soma.membrane.num_k_channels,
                                              sizeof(nx49_potassium_channel_t));
    
    for (uint32_t i = 0; i < neuron->soma.membrane.num_k_channels; i++) {
        neuron->soma.membrane.k_channels[i].conductance = 10.0; // pS
        neuron->soma.membrane.k_channels[i].voltage_sensitivity = 0.05; // mV⁻¹
        neuron->soma.membrane.k_channels[i].num_subunits = 4;
        neuron->soma.membrane.k_channels[i].is_open = false;
    }
    
    // Mitochondries (1-2% volume cellulaire)
    neuron->soma.num_mitochondria = (uint32_t)(neuron->soma.volume * 0.015 / 0.5); // ~0.5 µm³ par mito
    neuron->soma.mitochondria = calloc(neuron->soma.num_mitochondria, sizeof(nx49_mitochondrion_t));
    
    for (uint32_t i = 0; i < neuron->soma.num_mitochondria; i++) {
        neuron->soma.mitochondria[i].atp_production_rate = 1000.0; // ATP/s
        neuron->soma.mitochondria[i].membrane_potential = -180.0; // mV
        neuron->soma.mitochondria[i].num_cristae = 50;
    }
    
    // === DENDRITES ===
    neuron->num_dendrites = 5; // Neurone pyramidal typique
    neuron->dendrites = calloc(neuron->num_dendrites, sizeof(nx49_dendrite_t));
    
    for (uint32_t d = 0; d < neuron->num_dendrites; d++) {
        neuron->dendrites[d].length = NX49_DENDRITE_LENGTH;
        neuron->dendrites[d].diameter = NX49_DENDRITE_DIAMETER;
        neuron->dendrites[d].cable_resistance = 100.0; // Ω
        neuron->dendrites[d].time_constant = 20.0; // ms
        neuron->dendrites[d].space_constant = 200.0; // µm
        
        // Synapses sur dendrite (1-2 par µm)
        neuron->dendrites[d].num_synapses = (uint32_t)(NX49_DENDRITE_LENGTH * 1.5);
        neuron->dendrites[d].synapses = calloc(neuron->dendrites[d].num_synapses, 
                                               sizeof(nx49_synapse_t));
        
        // Initialiser synapses
        for (uint32_t s = 0; s < neuron->dendrites[d].num_synapses; s++) {
            nx49_synapse_t* syn = &neuron->dendrites[d].synapses[s];
            
            // Vésicules présynaptiques
            syn->presynaptic.num_vesicles = 200; // Pool readily releasable
            syn->presynaptic.vesicles = calloc(syn->presynaptic.num_vesicles,
                                               sizeof(nx49_neurotransmitter_t));
            syn->presynaptic.release_probability = NX49_RELEASE_PROBABILITY;
            
            // Fente synaptique
            syn->cleft.width = NX49_SYNAPSE_CLEFT;
            syn->cleft.diffusion_time = 0.1; // ms
            
            // Récepteurs postsynaptiques
            syn->postsynaptic.num_ampa = 50;
            syn->postsynaptic.ampa_receptors = calloc(syn->postsynaptic.num_ampa,
                                                      sizeof(nx49_ampa_receptor_t));
            syn->postsynaptic.num_nmda = 20;
            syn->postsynaptic.nmda_receptors = calloc(syn->postsynaptic.num_nmda,
                                                      sizeof(nx49_nmda_receptor_t));
            
            // Plasticité synaptique
            syn->plasticity.weight = 0.5; // Poids initial
            syn->plasticity.ltp_magnitude = 1.5;
            syn->plasticity.ltd_magnitude = 0.7;
        }
    }
    
    // === AXONE ===
    neuron->axon.length = NX49_AXON_LENGTH;
    neuron->axon.diameter = NX49_AXON_DIAMETER;
    neuron->axon.is_myelinated = true;
    neuron->axon.myelin_thickness = 0.2; // µm
    neuron->axon.num_nodes_ranvier = (uint32_t)(NX49_AXON_LENGTH / 100.0); // Nœud tous les 100µm
    neuron->axon.conduction_velocity = 50.0; // m/s (myélinisé)
    
    // Terminaisons axonales
    neuron->axon.num_terminals = 1000; // Boutons synaptiques
    neuron->axon.terminals = calloc(neuron->axon.num_terminals, sizeof(nx49_synapse_t));
    
    // === ÉTAT ÉLECTRIQUE ===
    neuron->membrane_potential = NX49_RESTING_POTENTIAL;
    neuron->threshold = NX49_THRESHOLD_POTENTIAL;
    neuron->is_firing = false;
    neuron->last_spike_time_ns = 0;
    neuron->spike_count = 0;
    
    // === MÉTABOLISME ===
    neuron->atp_level = 5.0; // mM
    neuron->oxygen_level = 0.2; // mM
    neuron->glucose_level = 5.0; // mM
    
    // === GÉNÉTIQUE ===
    neuron->num_genes = 20000; // Génome humain ~20K gènes
    neuron->gene_expression = calloc(neuron->num_genes, sizeof(uint32_t));
    
    // Expression basale
    for (uint32_t g = 0; g < neuron->num_genes; g++) {
        neuron->gene_expression[g] = rand() % 1000; // Niveau expression
    }
    
    // === BLOCKCHAIN ===
    // Adresse blockchain (hash du neuron_id)
    uint64_t id = neuron->neuron_id;
    for (int i = 0; i < 32; i++) {
        neuron->blockchain_address[i] = (uint8_t)(id >> (i * 8));
    }
    neuron->block_height = 0;
    
    return neuron;
}

void nx49_neuron_destroy(nx49_biological_neuron_t* neuron) {
    if (!neuron) return;
    
    // Libérer canaux ioniques
    free(neuron->soma.membrane.na_channels);
    free(neuron->soma.membrane.k_channels);
    free(neuron->soma.membrane.ca_channels);
    free(neuron->soma.membrane.na_k_pumps);
    
    // Libérer mitochondries
    free(neuron->soma.mitochondria);
    
    // Libérer dendrites et synapses
    for (uint32_t d = 0; d < neuron->num_dendrites; d++) {
        for (uint32_t s = 0; s < neuron->dendrites[d].num_synapses; s++) {
            nx49_synapse_t* syn = &neuron->dendrites[d].synapses[s];
            free(syn->presynaptic.vesicles);
            free(syn->postsynaptic.ampa_receptors);
            free(syn->postsynaptic.nmda_receptors);
            free(syn->postsynaptic.gaba_receptors);
        }
        free(neuron->dendrites[d].synapses);
    }
    free(neuron->dendrites);
    
    // Libérer terminaisons axonales
    free(neuron->axon.terminals);
    
    // Libérer expression génétique
    free(neuron->gene_expression);
    
    free(neuron);
}

/* ============================================================================
 * DYNAMIQUE MOLÉCULAIRE
 * ============================================================================ */

void nx49_neuron_simulate_molecular_dynamics(nx49_biological_neuron_t* neuron, double dt_ps) {
    if (!neuron) return;
    
    // Simulation dynamique moléculaire simplifiée
    // En réalité nécessiterait GROMACS/NAMD pour précision atomique
    
    // 1. Diffusion ionique (loi de Fick)
    double D_na = 1.33e-5; // cm²/s diffusion Na+
    double D_k = 1.96e-5;  // cm²/s diffusion K+
    double D_ca = 0.79e-5; // cm²/s diffusion Ca2+
    
    // Gradient concentration → flux
    double na_gradient = NX49_NA_EXTERNAL - neuron->soma.na_concentration;
    double k_gradient = NX49_K_EXTERNAL - neuron->soma.k_concentration;
    double ca_gradient = NX49_CA_EXTERNAL - neuron->soma.ca_concentration;
    
    // Flux = -D * ∇C * dt
    double dt_s = dt_ps * 1e-12;
    neuron->soma.na_concentration += D_na * na_gradient * dt_s * 0.001;
    neuron->soma.k_concentration += D_k * k_gradient * dt_s * 0.001;
    neuron->soma.ca_concentration += D_ca * ca_gradient * dt_s * 0.001;
    
    // 2. Mouvement Brownien des protéines
    for (uint32_t i = 0; i < neuron->soma.membrane.num_na_channels; i++) {
        // Fluctuations thermiques (mouvement Brownien)
        double thermal_energy = NX49_BOLTZMANN * NX49_TEMPERATURE_KELVIN;
        (void)thermal_energy; // Utilisé pour calculs futurs
        
        // Probabilité ouverture canal (fonction voltage)
        double vm = neuron->membrane_potential;
        double p_open = 1.0 / (1.0 + exp(-(vm + 50.0) / 10.0)); // Fonction sigmoïde
        neuron->soma.membrane.na_channels[i].open_probability = p_open;
        neuron->soma.membrane.na_channels[i].is_open = (rand() / (double)RAND_MAX) < p_open;
    }
    
    // 3. Métabolisme mitochondrial
    for (uint32_t m = 0; m < neuron->soma.num_mitochondria; m++) {
        nx49_mitochondrion_t* mito = &neuron->soma.mitochondria[m];
        
        // Production ATP (glycolyse + cycle Krebs + phosphorylation oxydative)
        double glucose_consumed = 0.1 * dt_s; // mol/s
        double atp_produced = glucose_consumed * 38; // 38 ATP par glucose
        
        mito->atp_production_rate = atp_produced / dt_s;
        neuron->atp_level += atp_produced;
        neuron->glucose_level -= glucose_consumed;
        
        // Consommation O2
        mito->oxygen_consumption = glucose_consumed * 6; // 6 O2 par glucose
        neuron->oxygen_level -= mito->oxygen_consumption * dt_s;
    }
    
    // 4. Expression génétique (transcription/traduction)
    for (uint32_t g = 0; g < neuron->num_genes; g++) {
        // Régulation épigénétique simplifiée
        double activity_level = neuron->spike_count / 1000.0; // Activité neuronale
        double expression_change = activity_level * dt_s * 0.01;
        
        neuron->gene_expression[g] += (uint32_t)(expression_change * 100);
        if (neuron->gene_expression[g] > 10000) neuron->gene_expression[g] = 10000;
    }
}

/* ============================================================================
 * ÉLECTROPHYSIOLOGIE (HODGKIN-HUXLEY)
 * ============================================================================ */

double nx49_neuron_compute_membrane_potential(nx49_biological_neuron_t* neuron) {
    if (!neuron) return 0.0;
    
    // Modèle Hodgkin-Huxley complet
    
    // 1. Potentiels d'équilibre (Nernst)
    double E_na = nernst_potential(1.0, NX49_NA_EXTERNAL, neuron->soma.na_concentration);
    double E_k = nernst_potential(1.0, NX49_K_EXTERNAL, neuron->soma.k_concentration);
    // Potentiels Ca2+ et Cl- pour calculs futurs
    (void)nernst_potential(2.0, NX49_CA_EXTERNAL, neuron->soma.ca_concentration);
    (void)nernst_potential(-1.0, NX49_CL_EXTERNAL, neuron->soma.cl_concentration);
    
    // 2. Conductances (dépendantes voltage et temps)
    double vm = neuron->membrane_potential;
    
    // Conductance Na+ (activation rapide, inactivation)
    double alpha_m = 0.1 * (vm + 40.0) / (1.0 - exp(-(vm + 40.0) / 10.0));
    double beta_m = 4.0 * exp(-(vm + 65.0) / 18.0);
    double m_inf = alpha_m / (alpha_m + beta_m);
    
    double alpha_h = 0.07 * exp(-(vm + 65.0) / 20.0);
    double beta_h = 1.0 / (1.0 + exp(-(vm + 35.0) / 10.0));
    double h_inf = alpha_h / (alpha_h + beta_h);
    
    double g_na_max = 120.0; // mS/cm²
    double g_na = g_na_max * pow(m_inf, 3) * h_inf;
    
    // Conductance K+ (activation lente)
    double alpha_n = 0.01 * (vm + 55.0) / (1.0 - exp(-(vm + 55.0) / 10.0));
    double beta_n = 0.125 * exp(-(vm + 65.0) / 80.0);
    double n_inf = alpha_n / (alpha_n + beta_n);
    
    double g_k_max = 36.0; // mS/cm²
    double g_k = g_k_max * pow(n_inf, 4);
    
    // Conductance fuite
    double g_leak = 0.3; // mS/cm²
    double E_leak = -54.4; // mV
    
    // 3. Courants ioniques
    double I_na = g_na * (vm - E_na);
    double I_k = g_k * (vm - E_k);
    double I_leak = g_leak * (vm - E_leak);
    
    // 4. Équation différentielle membrane
    double C_m = NX49_MEMBRANE_CAPACITANCE; // µF/cm²
    double I_total = I_na + I_k + I_leak;
    
    // dV/dt = -I_total / C_m
    double dt = 0.01; // ms
    double dv_dt = -I_total / C_m;
    
    neuron->membrane_potential += dv_dt * dt;
    
    return neuron->membrane_potential;
}

bool nx49_neuron_fire_action_potential(nx49_biological_neuron_t* neuron) {
    if (!neuron) return false;
    
    // Vérifier seuil d'activation
    if (neuron->membrane_potential >= neuron->threshold && !neuron->is_firing) {
        
        // Période réfractaire
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint64_t current_time_ns = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
        
        if (current_time_ns - neuron->last_spike_time_ns < NX49_REFRACTORY_PERIOD_MS * 1000000) {
            return false; // Encore en période réfractaire
        }
        
        // Déclencher potentiel d'action
        neuron->is_firing = true;
        neuron->membrane_potential = NX49_ACTION_POTENTIAL_PEAK;
        neuron->last_spike_time_ns = current_time_ns;
        neuron->spike_count++;
        
        // Propagation le long de l'axone (temps calculé pour référence)
        // double propagation_time_ms = (neuron->axon.length * 1e-6) / neuron->axon.conduction_velocity * 1000.0;
        
        // Libération neurotransmetteurs aux terminaisons
        for (uint32_t t = 0; t < neuron->axon.num_terminals; t++) {
            nx49_synapse_release_neurotransmitters(&neuron->axon.terminals[t]);
        }
        
        return true;
    }
    
    // Repolarisation
    if (neuron->is_firing && neuron->membrane_potential > NX49_RESTING_POTENTIAL) {
        neuron->membrane_potential -= 5.0; // Repolarisation rapide
        if (neuron->membrane_potential <= NX49_RESTING_POTENTIAL) {
            neuron->is_firing = false;
            neuron->membrane_potential = NX49_RESTING_POTENTIAL;
        }
    }
    
    return false;
}

/* ============================================================================
 * TRANSMISSION SYNAPTIQUE
 * ============================================================================ */

void nx49_synapse_release_neurotransmitters(nx49_synapse_t* synapse) {
    if (!synapse) return;
    
    // Probabilité de libération vésiculaire
    double p_release = synapse->presynaptic.release_probability;
    
    // Dépendance Ca²⁺ (coopérativité n=4)
    double ca_factor = pow(synapse->presynaptic.ca_concentration / 1.0, 4);
    p_release *= ca_factor;
    
    // Libération stochastique des vésicules
    uint32_t vesicles_released = 0;
    for (uint32_t v = 0; v < synapse->presynaptic.num_vesicles; v++) {
        if ((rand() / (double)RAND_MAX) < p_release) {
            vesicles_released++;
            
            // Chaque vésicule contient ~5000 molécules neurotransmetteur
            synapse->cleft.num_neurotransmitters += NX49_NEUROTRANSMITTER_PER_VESICLE;
        }
    }
    
    // Diffusion dans fente synaptique (loi de Fick)
    double diffusion_coeff = 0.76e-6; // cm²/s pour glutamate
    double diffusion_time = pow(synapse->cleft.width * 1e-7, 2) / (2 * diffusion_coeff);
    synapse->cleft.diffusion_time = diffusion_time * 1000; // ms
    
    // Liaison aux récepteurs postsynaptiques
    double binding_prob = 0.1; // 10% des molécules se lient
    uint32_t molecules_bound = (uint32_t)(synapse->cleft.num_neurotransmitters * binding_prob);
    
    // Activation récepteurs AMPA
    for (uint32_t r = 0; r < synapse->postsynaptic.num_ampa && molecules_bound > 0; r++) {
        if (!synapse->postsynaptic.ampa_receptors[r].is_bound) {
            synapse->postsynaptic.ampa_receptors[r].is_bound = true;
            synapse->postsynaptic.ampa_receptors[r].is_open = true;
            molecules_bound--;
        }
    }
    
    // Activation récepteurs NMDA (si dépolarisation suffisante)
    for (uint32_t r = 0; r < synapse->postsynaptic.num_nmda && molecules_bound > 0; r++) {
        if (!synapse->postsynaptic.nmda_receptors[r].is_bound) {
            synapse->postsynaptic.nmda_receptors[r].is_bound = true;
            // NMDA nécessite dépolarisation pour lever blocage Mg²⁺
            if (!synapse->postsynaptic.nmda_receptors[r].mg_block) {
                synapse->postsynaptic.nmda_receptors[r].is_open = true;
            }
            molecules_bound--;
        }
    }
}

/* ============================================================================
 * PLASTICITÉ SYNAPTIQUE (APPRENTISSAGE SANS POIDS)
 * ============================================================================ */

void nx49_synapse_apply_plasticity(nx49_synapse_t* synapse, double pre_spike_time, double post_spike_time) {
    if (!synapse) return;
    
    // Spike-Timing Dependent Plasticity (STDP)
    double delta_t = post_spike_time - pre_spike_time; // ms
    
    if (fabs(delta_t) < 100.0) { // Fenêtre temporelle 100ms
        
        if (delta_t > 0) {
            // Post après pré → LTP (potentialisation)
            double ltp_magnitude = synapse->plasticity.ltp_magnitude * exp(-delta_t / 20.0);
            synapse->plasticity.weight += ltp_magnitude * 0.01;
            synapse->plasticity.is_potentiated = true;
            
            // Augmentation nombre récepteurs AMPA
            if (synapse->postsynaptic.num_ampa < 200) {
                synapse->postsynaptic.num_ampa += 1;
            }
            
        } else {
            // Pré après post → LTD (dépression)
            double ltd_magnitude = synapse->plasticity.ltd_magnitude * exp(delta_t / 20.0);
            synapse->plasticity.weight -= ltd_magnitude * 0.01;
            synapse->plasticity.is_depressed = true;
            
            // Diminution nombre récepteurs AMPA
            if (synapse->postsynaptic.num_ampa > 10) {
                synapse->postsynaptic.num_ampa -= 1;
            }
        }
        
        // Borner poids synaptique
        if (synapse->plasticity.weight > 1.0) synapse->plasticity.weight = 1.0;
        if (synapse->plasticity.weight < 0.0) synapse->plasticity.weight = 0.0;
    }
    
    // Homéostasie synaptique (scaling)
    static double target_activity = 1.0; // Hz
    double current_activity = 1.0 / (post_spike_time / 1000.0); // Approximation
    
    if (current_activity > target_activity * 1.2) {
        // Trop d'activité → scaling down
        synapse->plasticity.weight *= 0.99;
    } else if (current_activity < target_activity * 0.8) {
        // Pas assez d'activité → scaling up
        synapse->plasticity.weight *= 1.01;
    }
}

/* ============================================================================
 * RÉSEAU NEURONAL BLOCKCHAIN
 * ============================================================================ */

nx49_neural_network_t* nx49_network_create(uint32_t num_neurons) {
    nx49_neural_network_t* network = calloc(1, sizeof(nx49_neural_network_t));
    if (!network) return NULL;
    
    network->num_neurons = num_neurons;
    network->neurons = calloc(num_neurons, sizeof(nx49_biological_neuron_t));
    
    // Créer neurones individuels
    for (uint32_t i = 0; i < num_neurons; i++) {
        nx49_biological_neuron_t* neuron = nx49_neuron_create("pyramidal");
        if (neuron) {
            memcpy(&network->neurons[i], neuron, sizeof(nx49_biological_neuron_t));
            free(neuron);
        }
    }
    
    // Matrice de connectivité (sparse)
    network->connectivity_matrix = calloc(num_neurons, sizeof(uint32_t*));
    network->weight_matrix = calloc(num_neurons, sizeof(double*));
    
    for (uint32_t i = 0; i < num_neurons; i++) {
        network->connectivity_matrix[i] = calloc(num_neurons, sizeof(uint32_t));
        network->weight_matrix[i] = calloc(num_neurons, sizeof(double));
        
        // Connectivité aléatoire (10% connexions)
        for (uint32_t j = 0; j < num_neurons; j++) {
            if (i != j && (rand() / (double)RAND_MAX) < 0.1) {
                network->connectivity_matrix[i][j] = 1;
                network->weight_matrix[i][j] = 0.5; // Poids initial
            }
        }
    }
    
    // Blockchain
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    for (int i = 0; i < 32; i++) {
        network->node_id[i] = (uint8_t)(ts.tv_nsec >> (i % 8));
    }
    
    network->block_height = 0;
    network->consensus_threshold = 0.67; // >66% Byzantine FT
    network->num_validators = num_neurons;
    
    // Apprentissage
    network->hebbian_learning = true;
    network->spike_timing_dependent = true;
    network->homeostatic_plasticity = true;
    
    return network;
}

void nx49_network_destroy(nx49_neural_network_t* network) {
    if (!network) return;
    
    // Détruire neurones
    for (uint32_t i = 0; i < network->num_neurons; i++) {
        // Note: neurones sont des copies, pas des pointeurs
        // Libération mémoire interne nécessaire
        free(network->neurons[i].dendrites);
        free(network->neurons[i].gene_expression);
    }
    free(network->neurons);
    
    // Libérer matrices
    for (uint32_t i = 0; i < network->num_neurons; i++) {
        free(network->connectivity_matrix[i]);
        free(network->weight_matrix[i]);
    }
    free(network->connectivity_matrix);
    free(network->weight_matrix);
    
    free(network->blockchain_data);
    free(network);
}

void nx49_network_propagate(nx49_neural_network_t* network, double dt_ms) {
    if (!network) return;
    
    // Propagation synchrone dans réseau
    for (uint32_t i = 0; i < network->num_neurons; i++) {
        nx49_biological_neuron_t* neuron = &network->neurons[i];
        
        // Calculer potentiel membrane
        nx49_neuron_compute_membrane_potential(neuron);
        
        // Vérifier décharge
        bool fired = nx49_neuron_fire_action_potential(neuron);
        
        if (fired) {
            // Propager signal aux neurones connectés
            for (uint32_t j = 0; j < network->num_neurons; j++) {
                if (network->connectivity_matrix[i][j]) {
                    // Signal arrive avec délai synaptique
                    double weight = network->weight_matrix[i][j];
                    double epsp_amplitude = 2.0 * weight; // mV
                    
                    network->neurons[j].membrane_potential += epsp_amplitude;
                }
            }
        }
        
        // Simulation moléculaire
        nx49_neuron_simulate_molecular_dynamics(neuron, dt_ms * 1e9); // ps
    }
}

void nx49_network_hebbian_learning(nx49_neural_network_t* network) {
    if (!network || !network->hebbian_learning) return;
    
    // Règle de Hebb: "Cells that fire together, wire together"
    for (uint32_t i = 0; i < network->num_neurons; i++) {
        for (uint32_t j = 0; j < network->num_neurons; j++) {
            if (network->connectivity_matrix[i][j]) {
                
                // Activité corrélée
                bool i_active = network->neurons[i].is_firing;
                bool j_active = network->neurons[j].is_firing;
                
                if (i_active && j_active) {
                    // Renforcement connexion
                    network->weight_matrix[i][j] += 0.01;
                    if (network->weight_matrix[i][j] > 1.0) {
                        network->weight_matrix[i][j] = 1.0;
                    }
                } else if (i_active && !j_active) {
                    // Affaiblissement connexion
                    network->weight_matrix[i][j] -= 0.005;
                    if (network->weight_matrix[i][j] < 0.0) {
                        network->weight_matrix[i][j] = 0.0;
                    }
                }
            }
        }
    }
}

bool nx49_network_validate_block(nx49_neural_network_t* network, const uint8_t* block_data, size_t size) {
    if (!network || !block_data) return false;
    
    // Consensus Byzantine Fault Tolerant
    uint32_t votes_for = 0;
    uint32_t total_validators = network->num_validators;
    
    // Chaque neurone vote basé sur son état
    for (uint32_t i = 0; i < network->num_neurons && i < total_validators; i++) {
        nx49_biological_neuron_t* neuron = &network->neurons[i];
        
        // Critères validation (simplifiés)
        bool valid_size = (size > 0 && size < 1000000); // 1MB max
        bool valid_neuron_state = (neuron->membrane_potential > -100.0 && 
                                   neuron->membrane_potential < 100.0);
        bool valid_activity = (neuron->spike_count > 0);
        
        if (valid_size && valid_neuron_state && valid_activity) {
            votes_for++;
        }
    }
    
    // Règle Byzantine: >66% votes requis
    double vote_ratio = (double)votes_for / total_validators;
    bool consensus_reached = (vote_ratio > network->consensus_threshold);
    
    if (consensus_reached) {
        // Ajouter bloc à blockchain
        network->block_height++;
        
        // Calculer Merkle root (simplifié)
        uint32_t hash = 0;
        for (size_t i = 0; i < size; i++) {
            hash = hash * 31 + block_data[i];
        }
        
        for (int i = 0; i < 32; i++) {
            network->neurons[0].merkle_root[i] = (uint8_t)(hash >> (i % 8));
        }
    }
    
    return consensus_reached;
}

// Made with Bob
