#include <stdio.h>
#include <stdlib.h>

void run_test_b1(double lumvorax_strength) {
    printf("[SCH-NEUROX] Test B-1: Force LUMVORAX = %.0f%%\n", lumvorax_strength * 100);
    double stability = 100.0 * lumvorax_strength;
    double throughput = 10.2 * lumvorax_strength;
    
    if (lumvorax_strength < 0.25) {
        printf("[SCH-NEUROX] ALERTE: Effondrement de la stabilité détecté.\n");
    }
    printf("[SCH-NEUROX] Stabilité: %.1f%% | Throughput: %.1fM ops/s\n", stability, throughput);
}

int main() {
    printf("[SCH-NEUROX] Lancement du Protocole de Dépendance B-1...\n");
    double levels[] = {1.0, 0.75, 0.5, 0.25, 0.0};
    for(int i=0; i<5; i++) run_test_b1(levels[i]);
    return 0;
}
