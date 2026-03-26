#include "nqbit_nx.h"
#include "nx_forensic.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static double get_total_ram_gb(void) {
    const long pages = sysconf(_SC_PHYS_PAGES);
    const long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages <= 0 || page_size <= 0) {
        return 0.0;
    }
    return ((double)pages * (double)page_size) / (1024.0 * 1024.0 * 1024.0);
}

int main(void) {
    nqbit_config_t base;
    nqbit_default_config(&base);

    nx_forensic_logger_t logger;
    if (!nx_forensic_open(&logger, "results/nqbit_forensic_ns.log")) {
        fprintf(stderr, "Erreur: impossible d'ouvrir le log forensique\n");
        return 1;
    }

    const long cpus = sysconf(_SC_NPROCESSORS_ONLN);
    const double ram_gb = get_total_ram_gb();
    nx_forensic_log(&logger, "hardware", "init", "ressources", "cpu_detected", (double)cpus);
    nx_forensic_log(&logger, "hardware", "init", "ressources", "ram_gb_detected", ram_gb);

    nx_forensic_log_expert_question(
        &logger,
        "phase_0",
        "Le bruit est-il piloté et non destructeur ?",
        "Oui, via lyapunov_gain qui ramène l'etat vers la cible.");

    size_t wins = 0U;
    const size_t scenarios = 360U;
    FILE *csv = fopen("results/nqbit_benchmark.csv", "w");
    if (csv == NULL) {
        nx_forensic_close(&logger);
        fprintf(stderr, "Erreur: impossible d'ouvrir le CSV\n");
        return 1;
    }
    fprintf(csv, "test_id,nx_score,classical_score,nx_energy,classical_energy,nx_better\n");

    for (size_t i = 0; i < scenarios; ++i) {
        nqbit_test_case_t tc;
        tc.test_id = (uint64_t)i;
        tc.initial = -1.5 + 3.0 * ((double)i / (double)scenarios);
        tc.target = 0.7 + 0.4 * (double)(i % 11U) / 10.0;
        tc.config = base;
        tc.config.seed = 0xABC000ULL + (uint64_t)i * 7919ULL;
        tc.config.junction_noise_sigma = 0.02 + 0.001 * (double)(i % 20U);
        tc.config.thermal_factor = 1.0 + 0.02 * (double)(i % 15U);

        nqbit_result_t out;
        (void)nqbit_run_test_case(&tc, &out);
        wins += out.nx_better ? 1U : 0U;

        nx_forensic_log(&logger, "simulation", "run", "score", "nx_score", out.nx_score);
        nx_forensic_log(&logger, "simulation", "run", "score", "classical_score", out.classical_score);

        fprintf(csv, "%zu,%.12f,%.12f,%.12f,%.12f,%d\n",
                i,
                out.nx_score,
                out.classical_score,
                out.nx_energy,
                out.classical_energy,
                out.nx_better ? 1 : 0);
    }

    fclose(csv);

    const double win_rate = (double)wins / (double)scenarios;
    nx_forensic_log(&logger, "summary", "final", "kpi", "win_rate", win_rate);
    nx_forensic_log_expert_question(
        &logger,
        "phase_finale",
        "Le chaos NX depasse-t-il la baseline de superposition simulee ?",
        win_rate > 0.5 ? "Oui sur ce prototype." : "Non, ajuster lyapunov_gain et bruit.");

    FILE *report = fopen("results/rapport_final.md", "w");
    if (report != NULL) {
        fprintf(report, "# Rapport final prototype NQubit NX\n\n");
        fprintf(report, "- Version: %s\n", NQUBIT_NX_VERSION);
        fprintf(report, "- CPU detectes: %ld\n", cpus);
        fprintf(report, "- RAM detectee (GB): %.2f\n", ram_gb);
        fprintf(report, "- Scenarios executes: %zu\n", scenarios);
        fprintf(report, "- Victoires NX: %zu\n", wins);
        fprintf(report, "- Taux de victoire NX: %.4f\n\n", win_rate);
        fprintf(report, "## Questions d'experts a chaque phase\n");
        fprintf(report, "1. Le bruit reste-t-il sous controle lyapunov ?\n");
        fprintf(report, "2. Quelle sensibilite au sigma de bruit de jonction ?\n");
        fprintf(report, "3. Quel impact thermique sur convergence ?\n");
        fprintf(report, "4. Quelle reproductibilite via seed ?\n");
        fclose(report);
    }

    nx_forensic_close(&logger);

    printf("Prototype NQubit NX termine. Win rate NX=%.4f\n", win_rate);
    return 0;
}
