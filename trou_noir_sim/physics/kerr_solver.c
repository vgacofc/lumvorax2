#include "kerr_math.h"
#include <math.h>

void kerr_init_q(kerr_metric_q_t* m, float128 mass, float128 spin) {
    m->mass = mass;
    m->spin = spin;
    float128 delta = sqrtq(mass * mass - spin * spin);
    m->h_plus = mass + delta;
}

// Solveur symplectique d'ordre 2 (Störmer-Verlet) en Quad Precision
void kerr_symplectic_step_q(const kerr_metric_q_t* m, geodesic_state_q_t* s, float128 ds) {
    // Demi-pas pour les vitesses (approximation d'accélération simplifiée pour démo)
    float128 r2 = s->r * s->r;
    float128 acc = -m->mass / (r2 + 1e-20q);
    
    s->ur += 0.5q * acc * ds;
    
    // Pas complet pour les positions
    s->r += s->ur * ds;
    s->t += s->ut * ds;
    s->theta += s->utheta * ds;
    s->phi += s->uphi * ds;
    
    // Second demi-pas pour les vitesses
    r2 = s->r * s->r;
    acc = -m->mass / (r2 + 1e-20q);
    s->ur += 0.5q * acc * ds;
}

float128 calculate_carter_q(const kerr_metric_q_t* m, const geodesic_state_q_t* s) {
    // Simplifié: C = p_theta^2 + cos^2(theta)*(a^2(1-E^2) + p_phi^2/sin^2(theta))
    float128 a = m->spin;
    float128 cos_th = cosq(s->theta);
    float128 sin_th = sinq(s->theta);
    float128 p_th = s->utheta; // Simplifié
    return p_th * p_th + cos_th * cos_th * (a * a); 
}
