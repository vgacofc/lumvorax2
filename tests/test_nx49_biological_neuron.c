/**
 * @file test_nx49_biological_neuron.c
 * @brief Tests unitaires NX49 Neurone Biologique
 * @version 1.1.0
 * @date 2026-05-02
 * @cycle C151 — corrigé pour unit_test_framework v1.0
 */

#include "../tests/unit_test_framework.h"
#include "../src/neural_network/nx49_biological_neuron.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ============================================================================
 * SUITE 1 : CRÉATION / DESTRUCTION NEURONE
 * ============================================================================ */

static int test_nx49_neuron_creation(void* ctx) {
    (void)ctx;
    nx49_biological_neuron_t* neuron = nx49_neuron_create("pyramidal");
    ASSERT_NOT_NULL(neuron);
    ASSERT_TRUE(neuron->neuron_id > 0);
    ASSERT_STR_EQ(neuron->type, "pyramidal");
    ASSERT_EQ(neuron->soma.diameter,        NX49_SOMA_DIAMETER);
    ASSERT_TRUE(neuron->soma.volume > 0.0);
    ASSERT_EQ(neuron->membrane_potential,   NX49_RESTING_POTENTIAL);
    ASSERT_EQ(neuron->num_dendrites,        5);
    ASSERT_NOT_NULL(neuron->dendrites);
    ASSERT_TRUE(neuron->axon.length > 0.0);
    ASSERT_TRUE(neuron->axon.is_myelinated);
    nx49_neuron_destroy(neuron);
    return TEST_SUCCESS;
}

static int test_nx49_neuron_soma_structure(void* ctx) {
    (void)ctx;
    nx49_biological_neuron_t* neuron = nx49_neuron_create("pyramidal");
    ASSERT_NOT_NULL(neuron);
    ASSERT_EQ(neuron->soma.na_concentration, NX49_NA_INTERNAL);
    ASSERT_EQ(neuron->soma.k_concentration,  NX49_K_INTERNAL);
    ASSERT_EQ(neuron->soma.ca_concentration, NX49_CA_INTERNAL);
    ASSERT_EQ(neuron->soma.cl_concentration, NX49_CL_INTERNAL);
    ASSERT_EQ(neuron->soma.membrane.capacitance, NX49_MEMBRANE_CAPACITANCE);
    ASSERT_TRUE(neuron->soma.membrane.num_na_channels > 0);
    ASSERT_TRUE(neuron->soma.membrane.num_k_channels  > 0);
    ASSERT_NOT_NULL(neuron->soma.membrane.na_channels);
    ASSERT_NOT_NULL(neuron->soma.membrane.k_channels);
    ASSERT_TRUE(neuron->soma.num_mitochondria > 0);
    ASSERT_NOT_NULL(neuron->soma.mitochondria);
    nx49_neuron_destroy(neuron);
    return TEST_SUCCESS;
}

static int test_nx49_neuron_dendrites_structure(void* ctx) {
    (void)ctx;
    nx49_biological_neuron_t* neuron = nx49_neuron_create("pyramidal");
    ASSERT_NOT_NULL(neuron);
    ASSERT_EQ(neuron->num_dendrites, 5);
    for (uint32_t d = 0; d < neuron->num_dendrites; d++) {
        ASSERT_EQ(neuron->dendrites[d].length,   NX49_DENDRITE_LENGTH);
        ASSERT_EQ(neuron->dendrites[d].diameter, NX49_DENDRITE_DIAMETER);
        ASSERT_TRUE(neuron->dendrites[d].num_synapses > 0);
        ASSERT_NOT_NULL(neuron->dendrites[d].synapses);
        for (uint32_t s = 0; s < neuron->dendrites[d].num_synapses; s++) {
            nx49_synapse_t* syn = &neuron->dendrites[d].synapses[s];
            ASSERT_TRUE(syn->presynaptic.num_vesicles > 0);
            ASSERT_NOT_NULL(syn->presynaptic.vesicles);
            ASSERT_TRUE(syn->postsynaptic.num_ampa > 0);
            ASSERT_TRUE(syn->postsynaptic.num_nmda > 0);
        }
    }
    nx49_neuron_destroy(neuron);
    return TEST_SUCCESS;
}

static int test_nx49_neuron_axon_structure(void* ctx) {
    (void)ctx;
    nx49_biological_neuron_t* neuron = nx49_neuron_create("pyramidal");
    ASSERT_NOT_NULL(neuron);
    ASSERT_EQ(neuron->axon.length,   NX49_AXON_LENGTH);
    ASSERT_EQ(neuron->axon.diameter, NX49_AXON_DIAMETER);
    ASSERT_TRUE(neuron->axon.is_myelinated);
    ASSERT_TRUE(neuron->axon.myelin_thickness > 0.0);
    ASSERT_TRUE(neuron->axon.num_nodes_ranvier > 0);
    ASSERT_TRUE(neuron->axon.conduction_velocity > 0.0);
    ASSERT_TRUE(neuron->axon.num_terminals > 0);
    ASSERT_NOT_NULL(neuron->axon.terminals);
    nx49_neuron_destroy(neuron);
    return TEST_SUCCESS;
}

/* ============================================================================
 * SUITE 2 : ÉLECTROPHYSIOLOGIE
 * ============================================================================ */

static int test_nx49_membrane_potential_resting(void* ctx) {
    (void)ctx;
    nx49_biological_neuron_t* neuron = nx49_neuron_create("pyramidal");
    ASSERT_NOT_NULL(neuron);
    ASSERT_EQ(neuron->membrane_potential, NX49_RESTING_POTENTIAL);
    double vm = nx49_neuron_compute_membrane_potential(neuron);
    ASSERT_TRUE(vm >= -80.0 && vm <= -60.0);
    nx49_neuron_destroy(neuron);
    return TEST_SUCCESS;
}

static int test_nx49_action_potential_threshold(void* ctx) {
    (void)ctx;
    nx49_biological_neuron_t* neuron = nx49_neuron_create("pyramidal");
    ASSERT_NOT_NULL(neuron);
    neuron->membrane_potential = -60.0;
    bool fired = nx49_neuron_fire_action_potential(neuron);
    ASSERT_FALSE(fired);
    ASSERT_FALSE(neuron->is_firing);
    neuron->membrane_potential = -50.0;
    fired = nx49_neuron_fire_action_potential(neuron);
    ASSERT_TRUE(fired);
    ASSERT_TRUE(neuron->is_firing);
    ASSERT_EQ(neuron->membrane_potential, NX49_ACTION_POTENTIAL_PEAK);
    ASSERT_EQ(neuron->spike_count, 1);
    nx49_neuron_destroy(neuron);
    return TEST_SUCCESS;
}

static int test_nx49_refractory_period(void* ctx) {
    (void)ctx;
    nx49_biological_neuron_t* neuron = nx49_neuron_create("pyramidal");
    ASSERT_NOT_NULL(neuron);
    neuron->membrane_potential = -50.0;
    bool fired1 = nx49_neuron_fire_action_potential(neuron);
    ASSERT_TRUE(fired1);
    neuron->membrane_potential = -50.0;
    bool fired2 = nx49_neuron_fire_action_potential(neuron);
    ASSERT_FALSE(fired2);
    nx49_neuron_destroy(neuron);
    return TEST_SUCCESS;
}

static int test_nx49_molecular_dynamics_simulation(void* ctx) {
    (void)ctx;
    nx49_biological_neuron_t* neuron = nx49_neuron_create("pyramidal");
    ASSERT_NOT_NULL(neuron);
    double initial_atp = neuron->atp_level;
    nx49_neuron_simulate_molecular_dynamics(neuron, 1.0);
    ASSERT_TRUE(neuron->atp_level >= initial_atp);
    nx49_neuron_destroy(neuron);
    return TEST_SUCCESS;
}

/* ============================================================================
 * SUITE 3 : TRANSMISSION SYNAPTIQUE
 * ============================================================================ */

static int test_nx49_synapse_neurotransmitter_release(void* ctx) {
    (void)ctx;
    nx49_biological_neuron_t* neuron = nx49_neuron_create("pyramidal");
    ASSERT_NOT_NULL(neuron);
    nx49_synapse_t* synapse = &neuron->dendrites[0].synapses[0];
    uint32_t initial_nt = synapse->cleft.num_neurotransmitters;
    nx49_synapse_release_neurotransmitters(synapse);
    ASSERT_TRUE(synapse->cleft.num_neurotransmitters >= initial_nt);
    nx49_neuron_destroy(neuron);
    return TEST_SUCCESS;
}

static int test_nx49_synapse_stdp_ltp(void* ctx) {
    (void)ctx;
    nx49_biological_neuron_t* neuron = nx49_neuron_create("pyramidal");
    ASSERT_NOT_NULL(neuron);
    nx49_synapse_t* synapse = &neuron->dendrites[0].synapses[0];
    double initial_weight = synapse->plasticity.weight;
    nx49_synapse_apply_plasticity(synapse, 10.0, 15.0);
    ASSERT_TRUE(synapse->plasticity.weight > initial_weight);
    ASSERT_TRUE(synapse->plasticity.is_potentiated);
    nx49_neuron_destroy(neuron);
    return TEST_SUCCESS;
}

static int test_nx49_synapse_stdp_ltd(void* ctx) {
    (void)ctx;
    nx49_biological_neuron_t* neuron = nx49_neuron_create("pyramidal");
    ASSERT_NOT_NULL(neuron);
    nx49_synapse_t* synapse = &neuron->dendrites[0].synapses[0];
    double initial_weight = synapse->plasticity.weight;
    nx49_synapse_apply_plasticity(synapse, 15.0, 10.0);
    ASSERT_TRUE(synapse->plasticity.weight < initial_weight);
    ASSERT_TRUE(synapse->plasticity.is_depressed);
    nx49_neuron_destroy(neuron);
    return TEST_SUCCESS;
}

/* ============================================================================
 * SUITE 4 : RÉSEAU NEURONAL
 * ============================================================================ */

static int test_nx49_network_creation(void* ctx) {
    (void)ctx;
    nx49_neural_network_t* network = nx49_network_create(10);
    ASSERT_NOT_NULL(network);
    ASSERT_EQ(network->num_neurons, 10);
    ASSERT_NOT_NULL(network->neurons);
    ASSERT_NOT_NULL(network->connectivity_matrix);
    ASSERT_NOT_NULL(network->weight_matrix);
    ASSERT_TRUE(network->hebbian_learning);
    ASSERT_TRUE(network->spike_timing_dependent);
    ASSERT_EQ(network->consensus_threshold, 0.67);
    nx49_network_destroy(network);
    return TEST_SUCCESS;
}

static int test_nx49_network_propagation(void* ctx) {
    (void)ctx;
    nx49_neural_network_t* network = nx49_network_create(5);
    ASSERT_NOT_NULL(network);
    network->neurons[0].membrane_potential = -50.0;
    nx49_network_propagate(network, 1.0);
    ASSERT_TRUE(network->neurons[0].spike_count > 0);
    nx49_network_destroy(network);
    return TEST_SUCCESS;
}

static int test_nx49_network_hebbian_learning(void* ctx) {
    (void)ctx;
    nx49_neural_network_t* network = nx49_network_create(5);
    ASSERT_NOT_NULL(network);
    network->neurons[0].is_firing = true;
    network->neurons[1].is_firing = true;
    network->connectivity_matrix[0][1] = 1;
    double initial_weight = network->weight_matrix[0][1];
    nx49_network_hebbian_learning(network);
    ASSERT_TRUE(network->weight_matrix[0][1] > initial_weight);
    nx49_network_destroy(network);
    return TEST_SUCCESS;
}

static int test_nx49_network_blockchain_consensus(void* ctx) {
    (void)ctx;
    nx49_neural_network_t* network = nx49_network_create(10);
    ASSERT_NOT_NULL(network);
    uint8_t block_data[100];
    for (int i = 0; i < 100; i++) block_data[i] = (uint8_t)i;
    for (uint32_t i = 0; i < network->num_neurons; i++) {
        network->neurons[i].membrane_potential = -65.0;
        network->neurons[i].spike_count = 1;
    }
    bool valid = nx49_network_validate_block(network, block_data, 100);
    ASSERT_TRUE(valid);
    ASSERT_EQ(network->block_height, 1);
    nx49_network_destroy(network);
    return TEST_SUCCESS;
}

/* ============================================================================
 * SUITE 5 : BLOCKCHAIN NEURONAL
 * ============================================================================ */

static int test_nx49_blockchain_address(void* ctx) {
    (void)ctx;
    nx49_biological_neuron_t* neuron = nx49_neuron_create("pyramidal");
    ASSERT_NOT_NULL(neuron);
    bool has_address = false;
    for (int i = 0; i < 32; i++) {
        if (neuron->blockchain_address[i] != 0) { has_address = true; break; }
    }
    ASSERT_TRUE(has_address);
    ASSERT_EQ(neuron->block_height, 0);
    nx49_neuron_destroy(neuron);
    return TEST_SUCCESS;
}

static int test_nx49_byzantine_fault_tolerance(void* ctx) {
    (void)ctx;
    nx49_neural_network_t* network = nx49_network_create(10);
    ASSERT_NOT_NULL(network);
    uint8_t block_data[50];
    memset(block_data, 0xAA, 50);
    for (uint32_t i = 0; i < 7; i++) {
        network->neurons[i].membrane_potential = -65.0;
        network->neurons[i].spike_count = 1;
    }
    for (uint32_t i = 7; i < 10; i++) {
        network->neurons[i].membrane_potential = -200.0;
        network->neurons[i].spike_count = 0;
    }
    bool valid = nx49_network_validate_block(network, block_data, 50);
    ASSERT_TRUE(valid);
    nx49_network_destroy(network);
    return TEST_SUCCESS;
}

/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║         TESTS UNITAIRES NX49 NEURONE BIOLOGIQUE               ║\n");
    printf("║              Cycle C151 — Exécution 2026-05-02                 ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");

    test_framework_t* fw = test_framework_init(true, true);
    if (!fw) { fprintf(stderr, "ERREUR: impossible d'initialiser le framework\n"); return 1; }

    /* Suite 1 */
    test_suite_t* s1 = test_suite_create(fw, "S1 — Creation/Destruction Neurone");
    test_suite_add_test(s1, "nx49_neuron_creation",          test_nx49_neuron_creation,          NULL);
    test_suite_add_test(s1, "nx49_neuron_soma_structure",    test_nx49_neuron_soma_structure,    NULL);
    test_suite_add_test(s1, "nx49_neuron_dendrites_struct",  test_nx49_neuron_dendrites_structure, NULL);
    test_suite_add_test(s1, "nx49_neuron_axon_structure",    test_nx49_neuron_axon_structure,    NULL);

    /* Suite 2 */
    test_suite_t* s2 = test_suite_create(fw, "S2 — Electrophysiologie");
    test_suite_add_test(s2, "nx49_membrane_potential_resting",     test_nx49_membrane_potential_resting,     NULL);
    test_suite_add_test(s2, "nx49_action_potential_threshold",     test_nx49_action_potential_threshold,     NULL);
    test_suite_add_test(s2, "nx49_refractory_period",              test_nx49_refractory_period,              NULL);
    test_suite_add_test(s2, "nx49_molecular_dynamics_simulation",  test_nx49_molecular_dynamics_simulation,  NULL);

    /* Suite 3 */
    test_suite_t* s3 = test_suite_create(fw, "S3 — Transmission Synaptique");
    test_suite_add_test(s3, "nx49_synapse_neurotransmitter_release", test_nx49_synapse_neurotransmitter_release, NULL);
    test_suite_add_test(s3, "nx49_synapse_stdp_ltp",                 test_nx49_synapse_stdp_ltp,                NULL);
    test_suite_add_test(s3, "nx49_synapse_stdp_ltd",                 test_nx49_synapse_stdp_ltd,                NULL);

    /* Suite 4 */
    test_suite_t* s4 = test_suite_create(fw, "S4 — Reseau Neuronal");
    test_suite_add_test(s4, "nx49_network_creation",            test_nx49_network_creation,            NULL);
    test_suite_add_test(s4, "nx49_network_propagation",         test_nx49_network_propagation,         NULL);
    test_suite_add_test(s4, "nx49_network_hebbian_learning",    test_nx49_network_hebbian_learning,    NULL);
    test_suite_add_test(s4, "nx49_network_blockchain_consensus",test_nx49_network_blockchain_consensus,NULL);

    /* Suite 5 */
    test_suite_t* s5 = test_suite_create(fw, "S5 — Blockchain Neuronal");
    test_suite_add_test(s5, "nx49_blockchain_address",           test_nx49_blockchain_address,           NULL);
    test_suite_add_test(s5, "nx49_byzantine_fault_tolerance",    test_nx49_byzantine_fault_tolerance,    NULL);

    test_stats_t stats = test_framework_run_all(fw);
    test_stats_print(&stats);

    /* Rapport JSON */
    test_framework_generate_json_report(fw, "nx49_test_results.json");
    printf("Rapport JSON: tests/nx49_test_results.json\n\n");

    int exit_code = (stats.failed_tests == 0 && stats.error_tests == 0) ? 0 : 1;
    test_framework_destroy(fw);
    return exit_code;
}
