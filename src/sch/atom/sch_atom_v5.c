#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <string.h>

// SCH-ATOM - Phase C-3.1 & D.1
// Simulation atomique intégrale sans abstraction

typedef enum { ATOM_H, ATOM_C, ATOM_N, ATOM_O, ATOM_NA, ATOM_K, ATOM_CA, ATOM_CL } SCH_AtomType;

typedef struct {
    uint64_t id;
    SCH_AtomType type;
    double x, y, z;
    double vx, vy, vz;
    double energy;
} SCH_Atom;

typedef struct {
    uint64_t timestamp;
    uint64_t a1_id;
    uint64_t a2_id;
    double dist;
    int class; // 1: Invariant Fort (Structurel), 2: Unique (Bruit)
    char interaction_type[16]; // vdW, Hydrogen, Ionic
} SCH_Invariant;

#define NUM_ATOMS 1000
#define DT 1.0 // fs
#define THRESHOLD 0.3 // nm
#define STRONG_THRESHOLD 0.15 // nm

int FALSIFICATION_MODE = 0;

void log_event(const char* path, const char* msg) {
    FILE *f = fopen(path, "a");
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
}

void apply_physics(SCH_Atom* a) {
    // Stochastique thermique pure
    a->vx += ((double)rand() / RAND_MAX - 0.5) * 0.02;
    a->vy += ((double)rand() / RAND_MAX - 0.5) * 0.02;
    a->vz += ((double)rand() / RAND_MAX - 0.5) * 0.02;
    a->x += a->vx * DT;
    a->y += a->vy * DT;
    a->z += a->vz * DT;
}

void detect_and_classify(SCH_Atom* pool, uint64_t step) {
    for (int i = 0; i < NUM_ATOMS; i++) {
        for (int j = i + 1; j < NUM_ATOMS; j++) {
            if (FALSIFICATION_MODE && (pool[i].id % 10 == 0)) continue; // Suppression ciblée d'invariants

            double dx = pool[i].x - pool[j].x;
            double dy = pool[i].y - pool[j].y;
            double dz = pool[i].z - pool[j].z;
            double d = sqrt(dx*dx + dy*dy + dz*dz);

            if (d < THRESHOLD) {
                SCH_Invariant inv;
                inv.timestamp = step;
                inv.a1_id = pool[i].id;
                inv.a2_id = pool[j].id;
                inv.dist = d;
                inv.class = (d < STRONG_THRESHOLD) ? 1 : 2;
                
                if (d < 0.12) strcpy(inv.interaction_type, "Ionic");
                else if (d < 0.20) strcpy(inv.interaction_type, "Hydrogen");
                else strcpy(inv.interaction_type, "vdW");

                char buf[256];
                sprintf(buf, "[%s][%llu] ATOMS(%llu,%llu) DIST(%.4f) CLASS(%d) TYPE(%s)", 
                        FALSIFICATION_MODE ? "FALSIF" : "C-3.1", 
                        (unsigned long long)step, (unsigned long long)inv.a1_id, 
                        (unsigned long long)inv.a2_id, d, inv.class, inv.interaction_type);
                
                log_event(FALSIFICATION_MODE ? "logs_AIMO3/sch/atom/D.1_falsification_events.log" : "logs_AIMO3/sch/atom/C-3.1_transient_events.log", buf);
            }
        }
    }
}

int main() {
    srand(time(NULL));
    SCH_Atom* pool = malloc(sizeof(SCH_Atom) * NUM_ATOMS);
    
    for (int i = 0; i < NUM_ATOMS; i++) {
        pool[i].id = i;
        pool[i].type = i % 4;
        pool[i].x = (double)rand() / RAND_MAX * 5.0;
        pool[i].y = (double)rand() / RAND_MAX * 5.0;
        pool[i].z = (double)rand() / RAND_MAX * 5.0;
        pool[i].vx = pool[i].vy = pool[i].vz = 0;
    }

    printf("[SCH-ATOM] Lancement Phase C-3.1 (Cartographie)...\n");
    for (int s = 0; s < 100; s++) {
        for (int i = 0; i < NUM_ATOMS; i++) apply_physics(&pool[i]);
        if (s % 10 == 0) detect_and_classify(pool, s);
    }

    printf("[SCH-ATOM] Lancement Phase D.1 (Falsification)...\n");
    FALSIFICATION_MODE = 1;
    for (int s = 100; s < 200; s++) {
        for (int i = 0; i < NUM_ATOMS; i++) apply_physics(&pool[i]);
        if (s % 10 == 0) detect_and_classify(pool, s);
    }

    free(pool);
    printf("[SCH-ATOM] Simulation terminée. Rapports en cours...\n");
    return 0;
}
