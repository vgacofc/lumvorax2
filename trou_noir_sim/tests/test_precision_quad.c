#include <stdio.h>
#include <stdlib.h>
#include <quadmath.h>
#include "../physics/kerr_math.h"
#include "../logging/log_writer.h"

void run_quad_simulation(float128 mass, float128 spin, float128 r0, const char* id) {
    kerr_metric_q_t m;
    geodesic_state_q_t s = {0, r0, 1.5707963267948966192313216916397514Q, 0, 1.0Q, -0.1Q, 0, 2.0Q};
    
    kerr_init_q(&m, mass, spin);
    log_init_session();
    
    char buf[128];
    quadmath_snprintf(buf, sizeof(buf), "%Qe", spin);
    printf("Exécuting Quad Test: %s (Spin: %s)\n", id, buf);
    
    for(int i=0; i<1000; i++) {
        kerr_symplectic_step_q(&m, &s, 0.001Q);
        if (s.r <= m.h_plus) break;
    }
    
    float128 carter = calculate_carter_q(&m, &s);
    quadmath_snprintf(buf, sizeof(buf), "%Qe", carter);
    printf("Carter Invariant: %s\n", buf);
}

int main() {
    run_quad_simulation(1.0Q, 0.99Q, 5.0Q, "QUAD_PRECISION_V28");
    return 0;
}
