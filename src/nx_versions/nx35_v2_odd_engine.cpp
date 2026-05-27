#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <cstdlib>

class NX35OddEngine {
public:
    NX35OddEngine(int num_neurons = 5000) : neurons(num_neurons, 1.0) {}

    void explore_odds() {
        std::ofstream forensique("logs_AIMO3/nx/NX-35/FORENSIQUE_ODD_EXPLORATION.log");
        forensique << "--- DEBUT ANALYSE FORENSIQUE : EXPLORATION DES IMPAIRS ---\n";

        for (int n_start = 3; n_start <= 101; n_start += 2) {
            forensique << "[BASE_ODD: " << n_start << "] ";
            double n = static_cast<double>(n_start);
            double energy = 0.0;
            
            for (int step = 0; step < 500; ++step) {
                if (fmod(n, 2.0) == 0) n /= 2.0;
                else n = 3.0 * n + 1.0;

                for (size_t j = 0; j < 5; ++j) {
                    neurons[j] = std::sin(neurons[j] * 0.99) + 0.0001 * n;
                    energy += std::abs(neurons[j]);
                }

                if (n <= 1.0) {
                    forensique << "CONVERGENCE_ATTEINTE step:" << step << " FINAL_ENERGY:" << energy << "\n";
                    break;
                }
                
                if (step == 499) {
                    forensique << "LIMITE_ATTEINTE n_current:" << n << " ENERGY:" << energy << "\n";
                }
            }
        }
        forensique << "--- FIN ANALYSE FORENSIQUE ---\n";
        forensique.close();
        std::cout << "✅ [NX-35 v2] EXPLORATION DES IMPAIRS TERMINÉE." << std::endl;
    }

private:
    std::vector<double> neurons;
};

int main() {
    system("mkdir -p logs_AIMO3/nx/NX-35");
    NX35OddEngine engine;
    engine.explore_odds();
    return 0;
}
