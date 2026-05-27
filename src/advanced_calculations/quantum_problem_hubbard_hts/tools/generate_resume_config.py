#!/usr/bin/env python3
"""
C37-RESUME : generate_resume_config.py
Génère problems_cycle06_resume.csv en excluant les modules déjà convergés
sur l'ENSEMBLE des runs précédents (pas seulement le dernier run).
Scanne TOUS les répertoires results/research_* et agrège les BASE_RESULT.
Usage : python3 generate_resume_config.py <problems_cycle06.csv> <results_dir>
Sortie : problems_cycle06_resume.csv dans le même répertoire que problems_cycle06.csv
"""
import sys
import os
import re
import csv

def find_converged_modules_all_runs(results_dir):
    """
    Aggrège les BASE_RESULT de TOUS les runs dans results_dir.
    Retourne l'ensemble des modules ayant convergé dans au moins un run.
    """
    converged = set()
    if not os.path.isdir(results_dir):
        print(f"[C37-RESUME] Répertoire results absent : {results_dir} — aucun module exclu", flush=True)
        return converged

    pat = re.compile(r"BASE_RESULT problem=(\S+)")
    run_dirs = sorted([
        os.path.join(results_dir, d)
        for d in os.listdir(results_dir)
        if d.startswith("research_") and os.path.isdir(os.path.join(results_dir, d))
    ])

    if not run_dirs:
        print(f"[C37-RESUME] Aucun run trouvé dans {results_dir}", flush=True)
        return converged

    print(f"[C37-RESUME] Scan de {len(run_dirs)} run(s) dans {results_dir}", flush=True)
    for run_dir in run_dirs:
        log_path = os.path.join(run_dir, "logs", "research_execution.log")
        if not os.path.isfile(log_path):
            continue
        run_name = os.path.basename(run_dir)
        run_conv = set()
        with open(log_path, "r", errors="replace") as fh:
            for line in fh:
                m = pat.search(line)
                if m:
                    run_conv.add(m.group(1))
        if run_conv:
            print(f"[C37-RESUME]   {run_name} : {len(run_conv)} convergé(s) — {sorted(run_conv)}", flush=True)
            converged |= run_conv

    print(f"[C37-RESUME] Total convergés sur tous les runs ({len(converged)}) : {sorted(converged)}", flush=True)
    return converged

def main():
    if len(sys.argv) < 3:
        print("Usage: generate_resume_config.py <problems_cycle06.csv> <results_dir>", flush=True)
        sys.exit(1)

    problems_csv = sys.argv[1]
    results_dir  = sys.argv[2]

    converged = find_converged_modules_all_runs(results_dir)

    out_dir  = os.path.dirname(os.path.abspath(problems_csv))
    out_path = os.path.join(out_dir, "problems_cycle06_resume.csv")

    remaining   = []
    already_done = []
    with open(problems_csv, "r") as fh:
        reader = csv.DictReader(fh)
        fieldnames = reader.fieldnames
        for row in reader:
            if row["name"] in converged:
                already_done.append(row)
            else:
                remaining.append(row)

    print(f"[C37-RESUME] {len(remaining)} modules restants / {len(already_done)} déjà convergés", flush=True)

    if not remaining:
        # Tous les modules sont convergés → générer un CSV VIDE (header seulement).
        # Le runner C verra nprobs=0 et devra passer aux phases avancées (stabilité/PTMC).
        # NE PAS utiliser la config complète comme fallback — cela causerait une re-simulation.
        print("[C37-RESUME] TOUS LES 15 MODULES CONVERGÉS — Resume vide généré → phases avancées", flush=True)
        with open(out_path, "w", newline="") as fh:
            writer = csv.DictWriter(fh, fieldnames=fieldnames)
            writer.writeheader()
        print(f"[C37-RESUME] Écrit : {out_path} (0 modules — convergence totale)", flush=True)
        print(out_path, flush=True)
        return

    with open(out_path, "w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=fieldnames)
        writer.writeheader()
        for row in remaining:
            writer.writerow(row)

    print(f"[C37-RESUME] Écrit : {out_path} ({len(remaining)} modules)", flush=True)
    print(out_path, flush=True)

if __name__ == "__main__":
    main()
