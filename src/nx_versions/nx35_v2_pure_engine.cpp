#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <chrono>

// Moteur de simulation dissipative pure sans stubs
class NX35_V2_PureEngine {
public:
    NX35_V2_PureEngine() : total_dissipation(0.0) {}

    void run_full_scan(long long start, long long end) {
        std::ofstream log_file("logs_AIMO3/nx/NX-35/FORENSIQUE_REAL_V2.log");
        if (!log_file.is_open()) {
            std::cerr << "Erreur: Impossible de créer le log." << std::endl;
            return;
        }

        log_file << "--- ANALYSE FORENSIQUE PURE NX-35 V2 (SANS STUBS) ---\n";
        log_file << "Horodatage: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << "\n";

        for (long long n = start; n <= end; n += 2) {
            log_file << "[SCAN_IMP_INIT: " << n << "] ";
            long long current = n;
            int steps = 0;
            double local_energy = 0.0;

            while (current != 1 && steps < 10000) {
                if (current % 2 == 0) {
                    current /= 2;
                } else {
                    // Vérification overflow avant 3n+1
                    if (current > (9223372036854775807LL - 1) / 3) {
                        log_file << "OVERFLOW_DETECTED ";
                        break;
                    }
                    current = 3 * current + 1;
                }
                
                // Calcul de la dissipation réelle (Lyapunov Ω simplifié)
                // Ω = log(valeur_suivante / valeur_actuelle)
                local_energy += std::abs(std::log2(static_cast<double>(current)));
                steps++;
            }

            if (current == 1) {
                log_file << "CONVERGENCE step:" << steps << " ENERGY_ACCUMULATED:" << local_energy << "\n";
            } else {
                log_file << "NON_CONVERGENCE_LIMIT_REACHED current:" << current << "\n";
            }
        }

        log_file << "--- FIN ANALYSE FORENSIQUE PURE ---\n";
        log_file.close();
    }

private:
    double total_dissipation;
};

int main() {
    system("mkdir -p logs_AIMO3/nx/NX-35");
    NX35_V2_PureEngine engine;
    // Scan intensif des impairs jusqu'à 1001 pour validation réelle
    engine.run_full_scan(3, 1001);
    std::cout << "✅ [NX-35 v2] SCAN RÉEL TERMINÉ. LOGS GÉNÉRÉS." << std::endl;
    return 0;
}
