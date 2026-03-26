#include <stdio.h>
#include <quadmath.h>
#include "physics/kerr_math.h"
#include "physics/solver_suite.h"

void compare_solvers() {
    printf("T21.1 : Multi-Solver Comparison (RK4 vs Symplectic vs Implicit)\n");
    kerr_metric_q_t m;
    kerr_init_q(&m, 1.0q, 0.999q);
    
    geodesic_state_q_t s_rk4 = {0, 6.0q, 1.57q, 0, 1.0q, 0, 0, 0.1q};
    geodesic_state_q_t s_sym = s_rk4;
    geodesic_state_q_t s_imp = s_rk4;
    
    float128 h = 0.01q;
    float128 c0 = calculate_carter_q(&m, &s_rk4);
    
    for(int i=0; i<10000; i++) {
        kerr_rk4_step_q(&m, &s_rk4, h);
        kerr_symplectic_step_q(&m, &s_sym, h);
        kerr_implicit_step_q(&m, &s_imp, h);
    }
    
    char b1[128], b2[128], b3[128];
    quadmath_snprintf(b1, 128, "%Qe", fabsq(calculate_carter_q(&m, &s_rk4) - c0));
    quadmath_snprintf(b2, 128, "%Qe", fabsq(calculate_carter_q(&m, &s_sym) - c0));
    quadmath_snprintf(b3, 128, "%Qe", fabsq(calculate_carter_q(&m, &s_imp) - c0));
    
    printf("Error RK4:        %s\n", b1);
    printf("Error Symplectic: %s\n", b2);
    printf("Error Implicit:   %s\n", b3);
}

int main() {
    compare_solvers();
    return 0;
}
