#ifndef FLOAT_UTILS_H
#define FLOAT_UTILS_H

#include <float.h>
#include <math.h>
#include <stdbool.h>

// Comparaison floating point avec epsilon tolerance
bool double_equals_epsilon(double a, double b, double epsilon) {
    if (isnan(a) || isnan(b)) return false;
    if (isinf(a) || isinf(b)) return (a == b);

    double abs_diff = fabs(a - b);
    if (abs_diff <= epsilon) return true;

    double abs_a = fabs(a);
    double abs_b = fabs(b);
    double max_abs = (abs_a > abs_b) ? abs_a : abs_b;

    return abs_diff <= epsilon * max_abs;
}

// Validation neural network convergence avec epsilon
bool neural_network_converged(double current_loss, double previous_loss, 
                             double convergence_threshold) {
    if (!isfinite(current_loss) || !isfinite(previous_loss)) {
        return false;
    }

    if (previous_loss <= DBL_EPSILON) {
        return current_loss <= convergence_threshold;
    }

    double relative_improvement = (previous_loss - current_loss) / previous_loss;
    return relative_improvement >= 0 && relative_improvement < convergence_threshold;
}

#endif // FLOAT_UTILS_H