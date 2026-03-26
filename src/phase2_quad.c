#include <stdio.h>
#include <quadmath.h>
#include <math.h>

/**
 * PHASE 2: QUAD PRECISION ANALYSIS
 * Utilisation de __float128 pour isoler l'erreur d'arrondi double.
 */

void test_precision_128() {
    __float128 r_quad = 10.0Q;
    __float128 dr_quad = 1e-20Q;
    double r_double = 10.0;
    double dr_double = 1e-20;

    printf("--- PHASE 2: ANALYSE PRÉCISION 128-BIT ---\n");
    printf("Double (10 + 1e-20): %.20f (Inchangé)\n", r_double + dr_double);
    
    char buf[128];
    quadmath_snprintf(buf, sizeof(buf), "%.20Qf", r_quad + dr_quad);
    printf("Quad   (10 + 1e-20): %s (Précision maintenue)\n", buf);
}

int main() {
    test_precision_128();
    return 0;
}
