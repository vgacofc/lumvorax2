#!/usr/bin/env python3
"""
C37-RESUME : generate_resume_config.py
Génère problems_cycle06_resume.csv en excluant les modules déjà convergés
(présence d'un BASE_RESULT dans research_execution.log du dernier run fullscale).
Usage : python3 generate_resume_config.py <problems_cycle06.csv> [<prev_run_dir>]
Sortie : problems_cycle06_resume.csv dans le même répertoire que problems_cycle06.csv
"""
import sys
import os
import re
import csv

def find_converged_modules(prev_run_dir):
    """Retourne l'ensemble des modules ayant un BASE_RESULT dans research_execution.log."""
    converged = set()
    log_path = os.path.join(prev_run_dir, "logs", "research_execution.log")
    if not os.path.isfile(log_path):
        print(f"[C37-RESUME] research_execution.log absent dans {prev_run_dir} — aucun module exclu", flush=True)
        return converged
    pat = re.compile(r"BASE_RESULT problem=(\S+)")
    with open(log_path, "r", errors="replace") as fh:
        for line in fh:
            m = pat.search(line)
            if m:
                converged.add(m.group(1))
    print(f"[C37-RESUME] Modules convergés détectés ({len(converged)}) : {sorted(converged)}", flush=True)
    return converged

def main():
    if len(sys.argv) < 2:
        print("Usage: generate_resume_config.py <problems_cycle06.csv> [<prev_run_dir>]", flush=True)
        sys.exit(1)

    problems_csv = sys.argv[1]
    prev_run_dir = sys.argv[2] if len(sys.argv) >= 3 else None

    converged = set()
    if prev_run_dir and os.path.isdir(prev_run_dir):
        converged = find_converged_modules(prev_run_dir)
    else:
        print("[C37-RESUME] Aucun run précédent fourni — tous les modules seront inclus", flush=True)

    out_dir = os.path.dirname(os.path.abspath(problems_csv))
    out_path = os.path.join(out_dir, "problems_cycle06_resume.csv")

    remaining = []
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
        print("[C37-RESUME] Tous les modules sont déjà convergés — resume = config complète", flush=True)
        remaining = already_done + []

    with open(out_path, "w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=fieldnames)
        writer.writeheader()
        for row in remaining:
            writer.writerow(row)

    print(f"[C37-RESUME] Écrit : {out_path} ({len(remaining)} modules)", flush=True)
    print(out_path, flush=True)

if __name__ == "__main__":
    main()
