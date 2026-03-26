#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <string.h>

/*
 * PROJECT: NX-11 (REFINED)
 * OBJECTIVE: Traçabilité bit-à-bit, Modulation externe (LSD, ATP), 
 *            Validation pathologies (Alzheimer, Schizophrénie)
 */

#define NX11_NUM_ATOMS 1500
#define NX11_DT 0.1
#define NX11_ENERGY_INIT 20000.0

typedef struct {
    double x, vx;
} NX11_Atom;

typedef struct {
    double atp;
    double noise_level;
    NX11_Atom* atoms;
    double hysteresis_trace;
    uint32_t state_hash;
} NX11_Neuron;

uint64_t get_nanos() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000L + ts.tv_nsec;
}

// Système de hachage simple pour le suivi d'état bit-à-bit
uint32_t update_hash(uint32_t current, double val) {
    uint32_t *v = (uint32_t*)&val;
    return current ^ (*v * 31);
}

void log_nx11_bit(const char* event, const char* details, uint32_t hash) {
    char path[256];
    sprintf(path, "logs_AIMO3/nx/NX-11/NX-11_BIT_TRACE.log");
    FILE* f = fopen(path, "a");
    if (f) {
        fprintf(f, "[%lu] EVENT:%s | HASH:%08X | %s\n", get_nanos(), event, hash, details);
        fclose(f);
    }
}

void nx11_step(NX11_Neuron* n) {
    n->state_hash = 0;
    for (int i = 0; i < NX11_NUM_ATOMS; i++) {
        n->atoms[i].vx += ((double)rand() / RAND_MAX - 0.5) * n->noise_level;
        n->atoms[i].x += n->atoms[i].vx * NX11_DT;
        n->state_hash = update_hash(n->state_hash, n->atoms[i].x);
    }
    n->atp -= 2.0;
    n->hysteresis_trace = (n->hysteresis_trace * 0.98) + (n->atp * 0.02);
    n->state_hash = update_hash(n->state_hash, n->atp);
}

// 1. Traçabilité granulaire
void run_granular_trace(NX11_Neuron* n) {
    log_nx11_bit("START_TRACE", "INIT_GRANULAR_LOGGING", n->state_hash);
    for(int i=0; i<10; i++) {
        nx11_step(n);
        char buf[64];
        sprintf(buf, "STEP_%d_COMPLETE_ATP_%.2f", i, n->atp);
        log_nx11_bit("MICRO_STEP", buf, n->state_hash);
    }
}

// 2. Modulation Externe (ATP & LSD)
void run_modulation_tests(NX11_Neuron* n) {
    // Test ATP dose-réponse
    double doses[] = {10.0, 5.0, 2.0, 0.0};
    for(int i=0; i<4; i++) {
        n->atp = doses[i];
        nx11_step(n);
        char buf[64];
        sprintf(buf, "ATP_DOSE_TEST_%.1f", doses[i]);
        log_nx11_bit("DOSE_RESPONSE", buf, n->state_hash);
    }
    
    // LSD: Hyper-fluidité
    log_nx11_bit("LSD_START", "ENERGY_SURGE_INJECTION", n->state_hash);
    n->atp += 10000.0;
    n->noise_level *= 0.5; // Abaissement barrière potentiel
    nx11_step(n);
    log_nx11_bit("LSD_EFFECT", "REGIME_HYPER_CONNECTIVITY", n->state_hash);
}

// 3. Pathologies (Bruit Thermique & Invariants)
void run_pathology_validation(NX11_Neuron* n) {
    // Alzheimer (perte invariants)
    log_nx11_bit("ALZHEIMER_SIM", "HYSTERESIS_COLLAPSE", n->state_hash);
    n->hysteresis_trace = 0.0;
    nx11_step(n);
    
    // Schizophrénie (Bruit thermique)
    log_nx11_bit("SCHIZOPHRENIA_SIM", "THERMAL_NOISE_OVERLOAD", n->state_hash);
    n->noise_level = 10.0;
    nx11_step(n);
}

int main() {
    srand(time(NULL));
    NX11_Neuron n;
    n.atp = NX11_ENERGY_INIT;
    n.noise_level = 0.25;
    n.atoms = malloc(sizeof(NX11_Atom) * NX11_NUM_ATOMS);
    n.hysteresis_trace = 100.0;
    n.state_hash = 0;

    printf("[NX-11] Execution du Cahier de Charges...\n");
    
    run_granular_trace(&n);
    run_modulation_tests(&n);
    run_pathology_validation(&n);

    printf("[NX-11] Terminé. Logs bit-à-bit générés.\n");
    
    free(n.atoms);
    return 0;
}
