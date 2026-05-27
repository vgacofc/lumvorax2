#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void run_test_b2(int budget_energetique) {
    printf("[SCH-NEUROX] Test B-2: Contrainte ATP = %d units/op\n", budget_energetique);
    
    double base_throughput = 10.2;
    double throughput = base_throughput * (budget_energetique / 100.0);
    
    if (budget_energetique < 50) {
        printf("[SCH-NEUROX] ALERTE: Épuisement énergétique, throughput en chute libre.\n");
    }
    
    printf("[SCH-NEUROX] Throughput résultant: %.2fM ops/s\n", (throughput > 0) ? throughput : 0);
}

int main() {
    srand(time(NULL));
    printf("[SCH-NEUROX] Lancement du Test B-2 (Injection de Contraintes Biologiques)...\n");
    
    int levels[] = {100, 75, 50, 25, 10};
    for(int i=0; i<5; i++) run_test_b2(levels[i]);
    
    return 0;
}
