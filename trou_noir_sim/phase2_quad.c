#include <stdio.h>
#include <quadmath.h>

int main() {
    printf("--- PHASE 2: ANALYSE PRÉCISION 128-BIT ---\n");
    
    double d_val = 10.0;
    double d_eps = 1e-20;
    double d_res = d_val + d_eps;
    
    __float128 q_val = 10.0q;
    __float128 q_eps = 1.0e-20q;
    __float128 q_res = q_val + q_eps;
    
    char buf[128];
    quadmath_snprintf(buf, sizeof(buf), "%.30Qg", q_res);
    
    printf("Double (10 + 1e-20): %.20f (Inchangé)\n", d_res);
    printf("Quad   (10 + 1e-20): %s (Précision maintenue)\n", buf);
    
    return 0;
}
