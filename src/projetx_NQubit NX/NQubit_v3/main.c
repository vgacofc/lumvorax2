#include "nqbit_nx.h"
#include "nx_forensic.h"

#include <float.h>
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
        "Oui, via lyapunov_gain qui ramene l'etat vers la cible.");

    size_t wins = 0U;
    const size_t scenarios = 360U;
    FILE *csv = fopen("results/nqbit_benchmark.csv", "w");
    if (csv == NULL) {
        nx_forensic_close(&logger);
        fprintf(stderr, "Erreur: impossible d'ouvrir le CSV\n");
        return 1;
    }
    fprintf(csv, "test_id,nx_score,classical_score,margin,nx_energy,classical_energy,energy_delta,nx_better\n");

    double sum_nx_score = 0.0;
    double sum_classical_score = 0.0;
    double sum_margin = 0.0;
    double sum_nx_energy = 0.0;
    double sum_classical_energy = 0.0;
    double sum_energy_delta = 0.0;

    double min_margin = DBL_MAX;
    double max_margin = -DBL_MAX;
    size_t min_margin_id = 0U;
    size_t max_margin_id = 0U;

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

        const double margin = out.nx_score - out.classical_score;
        const double energy_delta = out.nx_energy - out.classical_energy;

        wins += out.nx_better ? 1U : 0U;
        sum_nx_score += out.nx_score;
        sum_classical_score += out.classical_score;
        sum_margin += margin;
        sum_nx_energy += out.nx_energy;
        sum_classical_energy += out.classical_energy;
        sum_energy_delta += energy_delta;

        if (margin < min_margin) {
            min_margin = margin;
            min_margin_id = i;
        }
        if (margin > max_margin) {
            max_margin = margin;
            max_margin_id = i;
        }

        nx_forensic_log(&logger, "simulation", "run", "score", "nx_score", out.nx_score);
        nx_forensic_log(&logger, "simulation", "run", "score", "classical_score", out.classical_score);
        nx_forensic_log(&logger, "simulation", "run", "delta", "score_margin", margin);
        nx_forensic_log(&logger, "simulation", "run", "delta", "energy_delta", energy_delta);

        fprintf(csv, "%zu,%.12f,%.12f,%.12f,%.12f,%.12f,%.12f,%d\n",
                i,
                out.nx_score,
                out.classical_score,
                margin,
                out.nx_energy,
                out.classical_energy,
                energy_delta,
                out.nx_better ? 1 : 0);
    }

    fclose(csv);

    const double win_rate = (double)wins / (double)scenarios;
    const double avg_nx_score = sum_nx_score / (double)scenarios;
    const double avg_classical_score = sum_classical_score / (double)scenarios;
    const double avg_margin = sum_margin / (double)scenarios;
    const double avg_nx_energy = sum_nx_energy / (double)scenarios;
    const double avg_classical_energy = sum_classical_energy / (double)scenarios;
    const double avg_energy_delta = sum_energy_delta / (double)scenarios;

    nx_forensic_log(&logger, "summary", "final", "kpi", "win_rate", win_rate);
    nx_forensic_log(&logger, "summary", "final", "kpi", "avg_nx_score", avg_nx_score);
    nx_forensic_log(&logger, "summary", "final", "kpi", "avg_classical_score", avg_classical_score);
    nx_forensic_log(&logger, "summary", "final", "kpi", "avg_margin", avg_margin);
    nx_forensic_log(&logger, "summary", "final", "kpi", "avg_nx_energy", avg_nx_energy);
    nx_forensic_log(&logger, "summary", "final", "kpi", "avg_classical_energy", avg_classical_energy);
    nx_forensic_log(&logger, "summary", "final", "kpi", "avg_energy_delta", avg_energy_delta);

    nx_forensic_log_expert_question(
        &logger,
        "phase_finale",
        "Le chaos NX depasse-t-il la baseline de superposition simulee ?",
        win_rate > 0.5 ? "Oui sur ce prototype." : "Non, ajuster lyapunov_gain et bruit.");

    FILE *stats = fopen("results/nqbit_stats_v2.csv", "w");
    if (stats != NULL) {
        fprintf(stats, "metric,value\n");
        fprintf(stats, "scenarios,%zu\n", scenarios);
        fprintf(stats, "wins,%zu\n", wins);
        fprintf(stats, "win_rate,%.12f\n", win_rate);
        fprintf(stats, "avg_nx_score,%.12f\n", avg_nx_score);
        fprintf(stats, "avg_classical_score,%.12f\n", avg_classical_score);
        fprintf(stats, "avg_margin,%.12f\n", avg_margin);
        fprintf(stats, "min_margin,%.12f\n", min_margin);
        fprintf(stats, "min_margin_scenario,%zu\n", min_margin_id);
        fprintf(stats, "max_margin,%.12f\n", max_margin);
        fprintf(stats, "max_margin_scenario,%zu\n", max_margin_id);
        fprintf(stats, "avg_nx_energy,%.12f\n", avg_nx_energy);
        fprintf(stats, "avg_classical_energy,%.12f\n", avg_classical_energy);
        fprintf(stats, "avg_energy_delta,%.12f\n", avg_energy_delta);
        fclose(stats);
    }

    FILE *report = fopen("results/rapport_final.md", "w");
    if (report != NULL) {
        fprintf(report, "# Rapport final prototype NQubit NX - Version 2\n\n");
        fprintf(report, "- Version: %s\n", NQUBIT_NX_VERSION);
        fprintf(report, "- CPU detectes: %ld\n", cpus);
        fprintf(report, "- RAM detectee (GB): %.2f\n", ram_gb);
        fprintf(report, "- Scenarios executes: %zu\n", scenarios);
        fprintf(report, "- Victoires NX: %zu\n", wins);
        fprintf(report, "- Taux de victoire NX: %.4f\n", win_rate);
        fprintf(report, "- Score NX moyen: %.6f\n", avg_nx_score);
        fprintf(report, "- Score classique moyen: %.6f\n", avg_classical_score);
        fprintf(report, "- Marge moyenne: %.6f\n", avg_margin);
        fprintf(report, "- Marge min (scenario): %.6f (%zu)\n", min_margin, min_margin_id);
        fprintf(report, "- Marge max (scenario): %.6f (%zu)\n", max_margin, max_margin_id);
        fprintf(report, "- Energie NX moyenne: %.6f\n", avg_nx_energy);
        fprintf(report, "- Energie classique moyenne: %.6f\n", avg_classical_energy);
        fprintf(report, "- Delta energie moyen: %.6f\n\n", avg_energy_delta);
        fprintf(report, "## Questions d'experts a chaque phase\n");
        fprintf(report, "1. Le bruit reste-t-il sous controle lyapunov ?\n");
        fprintf(report, "2. Quelle sensibilite au sigma de bruit de jonction ?\n");
        fprintf(report, "3. Quel impact thermique sur convergence ?\n");
        fprintf(report, "4. Quelle reproductibilite via seed ?\n");
        fprintf(report, "5. La marge reste-t-elle stable sur tous les scenarios ?\n");
        fprintf(report, "6. Le cout energetique reste-t-il acceptable ?\n");
        fclose(report);
    }

    nx_forensic_close(&logger);

    printf("Prototype NQubit NX V2 termine. Win rate NX=%.4f\n", win_rate);
    return 0;
}
