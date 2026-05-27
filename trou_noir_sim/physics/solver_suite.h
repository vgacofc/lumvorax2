#ifndef SOLVER_SUITE_H
#define SOLVER_SUITE_H

#include "physics/kerr_math.h"

// RK4 Classique en Quad
void kerr_rk4_step_q(const kerr_metric_q_t* m, geodesic_state_q_t* s, float128 ds);

// Implicite (Point Fixe / Newton simplifié)
void kerr_implicit_step_q(const kerr_metric_q_t* m, geodesic_state_q_t* s, float128 ds);

#endif
