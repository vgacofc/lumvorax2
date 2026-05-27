#include <stdio.h>
#include <quadmath.h>
#include "physics/kerr_math.h"

void run_validation_t21() {
    kerr_metric_q_t m;
    kerr_init_q(&m, 1.0q, 0.9999q);
    
    geodesic_state_q_t s = {0, 6.0q, 1.57q, 0, 1.0q, 0, 0, 0.1q};
    float128 c0 = calculate_carter_q(&m, &s);
    
    printf("T21: High-Precision Symplectic Tracking (Quad)\n");
    for(int i=0; i<1000000; i++) {
        kerr_symplectic_step_q(&m, &s, 0.001q);
    }
    
    float128 c1 = calculate_carter_q(&m, &s);
    double delta_c = (double)fabsq(c1 - c0);
    
    char buf[128];
    quadmath_snprintf(buf, sizeof(buf), "%Qe", fabsq(c1 - c0));
    printf("Final Carter Delta: %s\n", buf);
    
    if (delta_c < 1e-15) printf("Result: SUCCESS (Constraint < 1e-15)\n");
    else printf("Result: FAILURE (Constraint too high)\n");
}

int main() {
    run_validation_t21();
    return 0;
}
