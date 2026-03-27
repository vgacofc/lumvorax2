#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// SCH-BIO : Phase 2A & 2B
// Synapse Tripartite et Pathologies

typedef struct {
    double v_membrane;
    double atp;
} SCH_Bio_Neuron_V3;

typedef struct {
    double weight;
    double neurotransmitter_level;
    double probability_release;
    double delay_ms;
} SCH_Tripartite_Synapse;

typedef struct {
    double atp_reservoir;
    double glial_support_efficiency; // Pathologie si < 1.0
} SCH_Astrocyte_V3;

void log_bio_v3(const char* msg) {
    FILE *f = fopen("logs_AIMO3/sch/bio/forensic_bio_v3.log", "a");
    if (f) { fprintf(f, "[BIO-V3][%ld] %s\n", time(NULL), msg); fclose(f); }
}

void simulate_synapse(SCH_Bio_Neuron_V3* pre, SCH_Bio_Neuron_V3* post, SCH_Tripartite_Synapse* syn, SCH_Astrocyte_V3* astro) {
    // 2A: Libération probabiliste
    double r = (double)rand() / RAND_MAX;
    if (pre->v_membrane > -50.0 && r < syn->probability_release) {
        // Transmission réussie
        syn->neurotransmitter_level += 1.0;
        post->v_membrane += syn->weight * syn->neurotransmitter_level;
        log_bio_v3("SYNAPTIC_TRANSMISSION_SUCCESS");
    } else if (pre->v_membrane > -50.0) {
        log_bio_v3("SYNAPTIC_TRANSMISSION_FAILURE");
    }

    // Influence Astrocytaire (Synapse Tripartite)
    if (syn->neurotransmitter_level > 0.5) {
        syn->neurotransmitter_level *= 0.8; // Recapture
        astro->atp_reservoir -= 0.05 * (2.0 - astro->glial_support_efficiency);
    }

    // 2B: Dégradation (Pathologie)
    if (astro->glial_support_efficiency < 0.5) {
        pre->atp -= 0.5; // Épuisement accéléré
        log_bio_v3("PATHOLOGICAL_ENERGY_DRAIN");
    }
}

int main() {
    srand(time(NULL));
    SCH_Bio_Neuron_V3 pre = {-70.0, 100.0};
    SCH_Bio_Neuron_V3 post = {-70.0, 100.0};
    SCH_Tripartite_Synapse synapse = {0.5, 0.0, 0.4, 2.0}; // 40% probabilité
    SCH_Astrocyte_V3 astro = {100.0, 1.0};

    printf("[SCH-BIO] Lancement Phase 2A (Synapse Tripartite)...\n");
    for(int i=0; i<50; i++) {
        pre.v_membrane += (rand() % 30) - 10; // Stimulus bruité
        simulate_synapse(&pre, &post, &synapse, &astro);
    }

    printf("[SCH-BIO] Lancement Phase 2B (Pathologie : Dégradation Gliale)...\n");
    astro.glial_support_efficiency = 0.2; // Chute de l'efficacité
    for(int i=0; i<50; i++) simulate_synapse(&pre, &post, &synapse, &astro);

    printf("[SCH-BIO] Simulation terminée. Post V: %.2f | Astro ATP: %.2f\n", post.v_membrane, astro.atp_reservoir);
    return 0;
}
