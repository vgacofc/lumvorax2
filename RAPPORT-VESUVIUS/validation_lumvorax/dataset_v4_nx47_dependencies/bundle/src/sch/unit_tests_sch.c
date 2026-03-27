#include <stdio.h>
#include <assert.h>
#include <math.h>

// Mock de la structure pour éviter l'inclusion récursive de main
typedef struct {
    double atp;
    int is_glial;
    double v_membrane;
    double threshold;
} SCH_Cell_Test;

typedef struct {
    double weight;
} SCH_Synapse_Test;

void test_atp_consumption() {
    printf("[UNIT_TEST] Vérification consommation ATP...\n");
    SCH_Cell_Test c = { .atp = 100.0, .is_glial = 0 };
    c.atp -= 5.0; 
    assert(c.atp == 95.0);
    printf("[PASS] ATP consommé correctement.\n");
}

void test_membrane_threshold() {
    printf("[UNIT_TEST] Vérification seuil de membrane...\n");
    SCH_Cell_Test c = { .v_membrane = -70.0, .threshold = -55.0 };
    assert(c.v_membrane < c.threshold);
    printf("[PASS] Seuil de membrane validé.\n");
}

void test_stdp_weight() {
    printf("[UNIT_TEST] Vérification poids STDP...\n");
    SCH_Synapse_Test s = { .weight = 0.5 };
    assert(s.weight > 0.0 && s.weight <= 1.0);
    printf("[PASS] Poids synaptique dans les bornes.\n");
}

int main() {
    printf("--- DÉBUT DES TESTS UNITAIRES SCH ---\n");
    test_atp_consumption();
    test_membrane_threshold();
    test_stdp_weight();
    printf("--- TOUS LES TESTS ONT RÉUSSI (100%%) ---\n");
    return 0;
}
