#include <math.h>
#include <stdio.h>

#ifndef KERR_METRIC_H
#include "physics/kerr_metric.h"
#endif

/**
 * Q1 - Métrique de Boyer-Lindquist Implémentée
 * g_tt = -(1 - 2Mr/Sigma)
 * g_tphi = -2Mar sin^2(theta) / Sigma
 * g_rr = Sigma / Delta
 * g_thetatheta = Sigma
 * g_phiphi = (r^2 + a^2 + 2Ma^2r sin^2(theta) / Sigma) sin^2(theta)
 * 
 * Sigma = r^2 + a^2 cos^2(theta)
 * Delta = r^2 - 2Mr + a^2
 */

typedef struct {
    double gtt, gtphi, grr, gthth, gphph;
} kerr_tensor_t;

void compute_kerr_tensor(const kerr_metric_t* m, double r, double theta, kerr_tensor_t* g) {
    double a = m->spin;
    double M = m->mass;
    double sin2th = sin(theta) * sin(theta);
    double cos2th = cos(theta) * cos(theta);
    double sigma = r*r + a*a*cos2th;
    double delta = r*r - 2.0*M*r + a*a;

    g->gtt = -(1.0 - 2.0*M*r/sigma);
    g->gtphi = -(2.0*M*a*r*sin2th)/sigma;
    g->grr = sigma/delta;
    g->gthth = sigma;
    g->gphph = (r*r + a*a + (2.0*M*a*a*r*sin2th)/sigma) * sin2th;
}

/**
 * Q2 - Satisfaction des contraintes (Hamiltonian & Momentum)
 * Dans le vide (Tmunu=0), les contraintes Gmunu = 0 doivent être vérifiées.
 * Ici, nous mesurons l'erreur résiduelle R du solveur ODE.
 */
double measure_hamiltonian_constraint(const kerr_metric_t* m, const geodesic_state_t* s) {
    kerr_tensor_t g;
    compute_kerr_tensor(m, s->r, s->theta, &g);
    // g_munu u^mu u^nu = -kappa (0 pour photons, 1 pour matière)
    return g.gtt*s->ut*s->ut + 2.0*g.gtphi*s->ut*s->uphi + g.grr*s->ur*s->ur + g.gthth*s->utheta*s->utheta + g.gphph*s->uphi*s->uphi;
}
