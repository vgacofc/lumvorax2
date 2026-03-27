#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define NUM_NEURONS 10
#define NUM_ASTROCYTES 2

typedef struct {
    double v_membrane;
    double atp;
    double plasticity;
} Neuron;

typedef struct {
    double atp_reservoir;
} Astrocyte;

typedef struct {
    double weight;
    int pre;
    int post;
    double failure_rate;
} Synapse;

void log_bio_v4(const char* msg) {
    FILE *f = fopen("logs_AIMO3/sch/bio/forensic_bio_v4.log", "a");
    if (f) { fprintf(f, "[BIO-V4][%ld] %s\n", time(NULL), msg); fclose(f); }
}

int main() {
    srand(time(NULL));
    Neuron neurons[NUM_NEURONS];
    Astrocyte astros[NUM_ASTROCYTES];
    Synapse synapses[NUM_NEURONS * 2];

    for(int i=0; i<NUM_NEURONS; i++) {
        neurons[i].v_membrane = -70.0;
        neurons[i].atp = 100.0;
        neurons[i].plasticity = 0.01;
    }
    for(int i=0; i<NUM_ASTROCYTES; i++) astros[i].atp_reservoir = 200.0;
    for(int i=0; i<NUM_NEURONS * 2; i++) {
        synapses[i].pre = rand() % NUM_NEURONS;
        synapses[i].post = rand() % NUM_NEURONS;
        synapses[i].weight = (double)rand()/RAND_MAX * 0.5;
        synapses[i].failure_rate = 0.6;
    }

    printf("[SCH-BIO] Lancement Phase 3A/3B (Micro-réseau & Mémoire fragile)...\n");
    for(int t=0; t<50; t++) {
        int n_idx = rand() % NUM_NEURONS;
        int a_idx = n_idx / (NUM_NEURONS/NUM_ASTROCYTES);
        
        // Conflit énergétique local
        if(neurons[n_idx].atp < 40.0 && astros[a_idx].atp_reservoir > 0) {
            neurons[n_idx].atp += 5.0;
            astros[a_idx].atp_reservoir -= 5.0;
        }
        
        // Activité et plasticité fragile
        neurons[n_idx].v_membrane += (double)rand()/RAND_MAX * 10.0 - 4.0;
        if(neurons[n_idx].v_membrane > -50.0) {
            for(int s=0; s<NUM_NEURONS*2; s++) {
                if(synapses[s].pre == n_idx && (double)rand()/RAND_MAX > synapses[s].failure_rate) {
                    neurons[synapses[s].post].v_membrane += synapses[s].weight;
                    synapses[s].weight += neurons[n_idx].plasticity; // Plasticité locale
                    log_bio_v4("NETWORK_SPIKE_PROPAGATED");
                }
            }
        }
        neurons[n_idx].atp -= 0.2;
    }

    printf("[SCH-BIO] Simulation micro-réseau terminée.\n");
    return 0;
}
