#include "solver_suite.h"
#include <math.h>

// Dérivées pour RK4 (simplifiées pour r et ur)
static void get_derivs(const kerr_metric_q_t* m, const geodesic_state_q_t* s, geodesic_state_q_t* d) {
    float128 r2 = s->r * s->r;
    d->t = s->ut;
    d->r = s->ur;
    d->theta = s->utheta;
    d->phi = s->uphi;
    d->ur = -m->mass / (r2 + 1e-20q);
    d->ut = 0; d->utheta = 0; d->uphi = 0; // Constantes de mouvement
}

void kerr_rk4_step_q(const kerr_metric_q_t* m, geodesic_state_q_t* s, float128 h) {
    geodesic_state_q_t k1, k2, k3, k4, tmp;
    
    get_derivs(m, s, &k1);
    
    tmp = *s;
    tmp.r += k1.r * 0.5q * h; tmp.ur += k1.ur * 0.5q * h;
    get_derivs(m, &tmp, &k2);
    
    tmp = *s;
    tmp.r += k2.r * 0.5q * h; tmp.ur += k2.ur * 0.5q * h;
    get_derivs(m, &tmp, &k3);
    
    tmp = *s;
    tmp.r += k3.r * h; tmp.ur += k3.ur * h;
    get_derivs(m, &tmp, &k4);
    
    s->r += (h/6.0q) * (k1.r + 2.0q*k2.r + 2.0q*k3.r + k4.r);
    s->ur += (h/6.0q) * (k1.ur + 2.0q*k2.ur + 2.0q*k3.ur + k4.ur);
    s->t += s->ut * h;
    s->theta += s->utheta * h;
    s->phi += s->uphi * h;
}

void kerr_implicit_step_q(const kerr_metric_q_t* m, geodesic_state_q_t* s, float128 h) {
    // Schéma d'Euler Implicite simplifié (1 itération de point fixe)
    float128 r_old = s->r;
    float128 ur_old = s->ur;
    
    // Prediction
    float128 r_next = r_old + ur_old * h;
    float128 acc_next = -m->mass / (r_next * r_next + 1e-20q);
    float128 ur_next = ur_old + acc_next * h;
    
    // Correction
    s->r = r_old + 0.5q * (ur_old + ur_next) * h;
    s->ur = ur_old + 0.5q * ((-m->mass/(r_old*r_old + 1e-20q)) + acc_next) * h;
    s->t += s->ut * h;
    s->theta += s->utheta * h;
    s->phi += s->uphi * h;
}
