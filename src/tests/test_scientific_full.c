#include "physics/kerr_math.h"
#include <stdio.h>
#include <quadmath.h>

void run_scaling_t22() {
    printf("\nT22: Scaling Analysis (1-a)^n\n");
    float128 spins[] = {0.9q, 0.99q, 0.999q, 0.9999q};
    for(int i=0; i<4; i++) {
        kerr_metric_q_t m;
        kerr_init_q(&m, 1.0q, spins[i]);
        char buf[128];
        quadmath_snprintf(buf, sizeof(buf), "%.4Qf", spins[i]);
        printf("Spin a = %s | Horizon r+ = ", buf);
        quadmath_snprintf(buf, sizeof(buf), "%.10Qf", m.h_plus);
        printf("%s\n", buf);
    }
}

void run_lyapunov_t24() {
    printf("\nT24: Lyapunov Exponent Convergence\n");
    kerr_metric_q_t m;
    kerr_init_q(&m, 1.0q, 0.9999q);
    
    geodesic_state_q_t s1 = {0, 2.1q, 1.57q, 0, 1.0q, -0.01q, 0, 0.01q};
    geodesic_state_q_t s2 = s1;
    float128 d0 = 1e-12q;
    s2.r += d0;
    
    float128 dt = 0.01q;
    for(int i=0; i<1000; i++) {
        kerr_symplectic_step_q(&m, &s1, dt);
        kerr_symplectic_step_q(&m, &s2, dt);
    }
    
    float128 dr = fabsq(s1.r - s2.r);
    float128 lambda = logq(dr / d0) / (1000.0q * dt);
    
    char buf[128];
    quadmath_snprintf(buf, sizeof(buf), "%.6Qf", lambda);
    printf("Lyapunov Lambda: %s\n", buf);
}

int main() {
    printf("--- ADVANCED SCIENTIFIC VALIDATION (QUAD PRECISION) ---\n");
    // T21 est déjà validé dans test_precision, on l'inclut ici aussi pour la suite complète
    kerr_metric_q_t m;
    kerr_init_q(&m, 1.0q, 0.9999q);
    geodesic_state_q_t s = {0, 6.0q, 1.57q, 0, 1.0q, 0, 0, 0.1q};
    float128 c0 = calculate_carter_q(&m, &s);
    for(int i=0; i<10000; i++) kerr_symplectic_step_q(&m, &s, 0.01q);
    float128 c1 = calculate_carter_q(&m, &s);
    char buf[128];
    quadmath_snprintf(buf, sizeof(buf), "%Qe", fabsq(c1 - c0));
    printf("T21 Carter Stability: %s\n", buf);

    run_scaling_t22();
    run_lyapunov_t24();
    return 0;
}
