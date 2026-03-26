#ifndef KERR_METRIC_H
#define KERR_METRIC_H

#include <stdint.h>

typedef struct {
    double mass;          // M
    double spin;          // a = J/M
    double horizon_plus;  // r+
    double horizon_minus; // r-
} kerr_metric_t;

typedef struct {
    double t, r, theta, phi;
    double ut, ur, utheta, uphi;
} geodesic_state_t;

// Kerr metric solver prototypes
void kerr_metric_init(kerr_metric_t* metric, double mass, double spin);
void kerr_geodesic_step(const kerr_metric_t* metric, geodesic_state_t* state, double ds);
void kerr_schild_geodesic_step(const kerr_metric_t* metric, geodesic_state_t* state, double ds);

// T21 - Symplectic Solver
void kerr_symplectic_step(const kerr_metric_t* metric, geodesic_state_t* state, double ds);

// T24 - Lyapunov & Poincaré Tools
double calculate_lyapunov_exponent(double d0, double d1, double dt);

// T27 - MHD Interaction
typedef struct {
    double b_field;
    double rho;
} mhd_state_t;
void update_mhd_plasma(const kerr_metric_t* m, const geodesic_state_t* s, mhd_state_t* p, double ds);

#endif
