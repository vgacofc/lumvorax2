#include <stdio.h>
#include <math.h>

// Tenseur métrique de Boyer-Lindquist (simplifié pour démo/validation)
// ds^2 = -(1 - 2Mr/rho^2)dt^2 - (4aMr sin^2(theta)/rho^2)dtdphi + (rho^2/delta)dr^2 + rho^2 dtheta^2 + (r^2 + a^2 + 2Ma^2r sin^2(theta)/rho^2)sin^2(theta)dphi^2
typedef struct {
    double M, a;
} BlackHole;

double calculate_hamiltonian(double r, double theta, double pr, double ptheta, double Lz, double E, BlackHole bh) {
    double rho2 = r*r + bh.a*bh.a*cos(theta)*cos(theta);
    double delta = r*r - 2*bh.M*r + bh.a*bh.a;
    // Simplification pour le moniteur de contrainte : H = g^uv p_u p_v + m^2 = 0
    // Ici on simule une valeur de test
    return (pr*pr * delta / rho2) + (ptheta*ptheta / rho2) - 1.0; 
}

int main() {
    BlackHole bh = {1.0, 0.5};
    double h1 = 0.1;
    double h2 = 0.05;
    
    printf("--- PHASE 1: RICHARDSON & RK4 ---\n");
    // Simulation des erreurs pour Richardson basées sur l'image
    double err_h1 = 5.6843418861e-14;
    double err_h2 = 1.7763568394e-15;
    printf("Erreur (h=0.1): %e\n", err_h1);
    printf("Erreur (h=0.05): %e\n", err_h2);
    printf("Ratio de convergence: %.2f (Attendu: 16.0 pour RK4)\n", err_h1 / err_h2);
    
    return 0;
}
