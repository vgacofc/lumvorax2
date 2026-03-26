#ifndef KERR_MATH_H
#define KERR_MATH_H

#include <quadmath.h>

typedef __float128 float128;

typedef struct {
    float128 t, r, theta, phi;
    float128 ut, ur, utheta, uphi;
} geodesic_state_q_t;

typedef struct {
    float128 mass;
    float128 spin;
    float128 h_plus;
} kerr_metric_q_t;

void kerr_init_q(kerr_metric_q_t* m, float128 mass, float128 spin);
void kerr_symplectic_step_q(const kerr_metric_q_t* m, geodesic_state_q_t* s, float128 ds);
float128 calculate_carter_q(const kerr_metric_q_t* m, const geodesic_state_q_t* s);

#endif
