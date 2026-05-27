#include "physics/kerr_metric.h"
#include <math.h>

/**
 * accretion_disk_emission
 * Simplified model of local emission in the relativistic disk.
 */
double accretion_disk_emission(double r, double mass) {
    // Temperature profile T ~ r^(-3/4)
    return 1000.0 * pow(r / (3.0 * mass), -0.75);
}

/**
 * radiative_transport_apply
 * Standard Name: radiative_transport_apply
 * Adjusts intensity based on Doppler and gravitational redshift.
 */
double radiative_transport_apply(double intensity, double z) {
    // I_obs = I_emit / (1 + z)^4
    return intensity / pow(1.0 + z, 4.0);
}
