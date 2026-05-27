#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// SCH-BIO : Phase 1A & 1B
// Compartimentation et Astrocytes Acteurs

typedef enum { SOMA, DENDRITE, AXONE } CompartmentType;

typedef struct {
    CompartmentType type;
    double v_membrane;
    double calcium;
    double area;
} SCH_Compartment;

typedef struct {
    SCH_Compartment soma;
    SCH_Compartment dendrite;
    SCH_Compartment axone;
    double atp;
} SCH_Bio_Neuron_V2;

typedef struct {
    double atp_reservoir;
} SCH_Astrocyte;

void log_bio(const char* msg) {
    FILE *f = fopen("logs_AIMO3/sch/bio/forensic_bio_v2.log", "a");
    if (f) { fprintf(f, "[BIO-V2][%ld] %s\n", time(NULL), msg); fclose(f); }
}

void simulate_step(SCH_Bio_Neuron_V2* n, SCH_Astrocyte* a) {
    // 1B: Astrocyte acteur - Régulation ATP
    if (n->atp < 50.0 && a->atp_reservoir > 10.0) {
        n->atp += 5.0;
        a->atp_reservoir -= 5.0;
        log_bio("ASTROCYTE_ATP_DELIVERY");
    }

    // 1A: Propagation de signal simple avec délais/bruit
    double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0;
    n->dendrite.v_membrane += noise;
    n->soma.v_membrane += (n->dendrite.v_membrane - n->soma.v_membrane) * 0.1;
    n->axone.v_membrane += (n->soma.v_membrane - n->axone.v_membrane) * 0.05;

    n->atp -= 0.1; // Consommation de base
}

int main() {
    srand(time(NULL));
    SCH_Bio_Neuron_V2 neuron = {
        {SOMA, -70.0, 0.0, 1.0},
        {DENDRITE, -70.0, 0.0, 2.0},
        {AXONE, -70.0, 0.0, 0.5},
        80.0
    };
    SCH_Astrocyte astro = {100.0};

    printf("[SCH-BIO] Lancement Phase 1A/1B (Compartiments + Glies actives)...\n");
    for(int i=0; i<100; i++) simulate_step(&neuron, &astro);
    
    printf("[SCH-BIO] Simulation terminée. Soma V: %.2f | ATP: %.2f\n", neuron.soma.v_membrane, neuron.atp);
    return 0;
}
