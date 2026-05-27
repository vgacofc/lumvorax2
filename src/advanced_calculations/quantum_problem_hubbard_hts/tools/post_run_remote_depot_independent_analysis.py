#!/usr/bin/env python3
"""Generate an independent remote-sync scientific analysis report.

The report is written in ROOT_DIR/RAPPORT and never overwrites prior reports.
It consolidates the latest run coverage, unresolved modules that are still not
integrated, and traceability checks from raw artifacts.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path

REMOTE_URL_DEFAULT = "https://github.com/lumc01/Lumvorax.git"

POSSIBLE_UNTREATED_MODULES = [
    {
        "module": "spin_liquid_exotic",
        "domain": "Matière condensée",
        "observables": "énergie, pairing, densité locale, corrélations courte/moyenne portée",
        "constraints": "réseaux 1D/2D petits-moyens; coût RAM élevé en 2D dense",
    },
    {
        "module": "topological_correlated_materials",
        "domain": "Matière condensée",
        "observables": "énergie, pairing, corrélateurs topologiques, estimateurs Chern approchés",
        "constraints": "invariants sur petites grilles; pas de grande taille réaliste",
    },
    {
        "module": "correlated_fermions_non_hubbard",
        "domain": "Condensée / chimie",
        "observables": "énergie, pairing, densité locale, corrélateurs simples",
        "constraints": "Hamiltoniens complexes limités à espaces réduits",
    },
    {
        "module": "multi_state_excited_chemistry",
        "domain": "Chimie quantique",
        "observables": "énergie par état, densité électronique locale, gap excité, pairing fullscale",
        "constraints": "petites molécules et bases réduites",
    },
    {
        "module": "bosonic_multimode_systems",
        "domain": "Physique quantique",
        "observables": "occupation de modes, énergie, corrélations mode-à-mode",
        "constraints": "troncature de Fock nécessaire sur CPU",
    },
    {
        "module": "multiscale_nonlinear_field_models",
        "domain": "Théorie des champs",
        "observables": "énergie, pseudo-spectres locaux, extrema spatiaux/temps",
        "constraints": "pas de QCD full-scale; schémas explicites bornés par stabilité",
    },
    {
        "module": "far_from_equilibrium_kinetic_lattices",
        "domain": "Théorie des champs / noneq",
        "observables": "énergie, pairing, spectre k, charge locale",
        "constraints": "forte sensibilité au pas de temps",
    },
    {
        "module": "multi_correlated_fermion_boson_networks",
        "domain": "Statistique / condensée",
        "observables": "énergie, pairing, densité locale, corrélations moyenne portée",
        "constraints": "montée combinatoire des interactions",
    },
]


def run(cmd: list[str], cwd: Path) -> str:
    out = subprocess.check_output(cmd, cwd=str(cwd), text=True)
    return out.strip()


def run_optional(cmd: list[str], cwd: Path) -> tuple[bool, str]:
    proc = subprocess.run(cmd, cwd=str(cwd), text=True, capture_output=True)
    if proc.returncode == 0:
        return True, proc.stdout.strip()
    details = (proc.stderr or proc.stdout).strip()
    return False, details


def read_csv(path: Path) -> list[dict[str, str]]:
    if not path.exists() or path.stat().st_size == 0:
        return []
    with path.open("r", newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def latest_run(results_dir: Path) -> Path:
    runs = sorted([p for p in results_dir.iterdir() if p.is_dir() and p.name.startswith("research_")])
    if not runs:
        raise RuntimeError(f"No research_* run found in {results_dir}")
    return runs[-1]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("root_dir", help="Path to quantum_problem_hubbard_hts root")
    ap.add_argument("--run-dir", default="", help="Specific run dir; defaults to latest research_*")
    ap.add_argument("--remote-url", default=REMOTE_URL_DEFAULT)
    args = ap.parse_args()

    root = Path(args.root_dir).resolve()
    results_dir = root / "results"
    run_dir = Path(args.run_dir).resolve() if args.run_dir else latest_run(results_dir)
    tests_dir = run_dir / "tests"
    logs_dir = run_dir / "logs"

    now = datetime.now(timezone.utc)
    stamp = now.strftime("%Y%m%dT%H%M%SZ")
    report_dir = root / "RAPPORT"
    report_dir.mkdir(parents=True, exist_ok=True)
    report_path = report_dir / f"RAPPORT_ANALYSE_INDEPENDANTE_DEPOT_DISTANT_{stamp}.md"
    json_path = report_dir / f"RAPPORT_ANALYSE_INDEPENDANTE_DEPOT_DISTANT_{stamp}.json"

    repo_root = root.parent.parent.parent
    head = run(["git", "rev-parse", "HEAD"], cwd=repo_root)

    fetch_ok, fetch_details = run_optional(
        ["git", "fetch", args.remote_url, "--prune"],
        cwd=repo_root,
    )
    fetch_head = "UNAVAILABLE"
    if fetch_ok:
        fetch_head = run(["git", "rev-parse", "FETCH_HEAD"], cwd=repo_root)
    else:
        origin_main_ok, origin_main = run_optional(["git", "rev-parse", "origin/main"], cwd=repo_root)
        if origin_main_ok:
            fetch_head = origin_main

    module_meta = read_csv(tests_dir / "module_physics_metadata.csv")
    new_tests = read_csv(tests_dir / "new_tests_results.csv")
    questions = read_csv(tests_dir / "expert_questions_matrix.csv")
    toy = read_csv(tests_dir / "toy_model_validation.csv")

    integrated_modules = sorted({(r.get("module") or r.get("problem") or "") for r in module_meta if (r.get("module") or r.get("problem"))})
    control_rows = [r for r in new_tests if r.get("test_family") == "control"]
    control_by_id = sorted({r.get("test_id", "") for r in control_rows})

    unresolved = []
    integrated_set = set(integrated_modules)
    for row in POSSIBLE_UNTREATED_MODULES:
        unresolved.append({
            **row,
            "status": "NON_INTÉGRÉ" if row["module"] not in integrated_set else "INTÉGRÉ",
        })

    toy_empty_runs: list[str] = []
    for candidate in sorted([p for p in results_dir.iterdir() if p.is_dir() and p.name.startswith("research_")]):
        toy_path = candidate / "tests" / "toy_model_validation.csv"
        if toy_path.exists() and toy_path.stat().st_size == 0:
            toy_empty_runs.append(candidate.name)

    checksummed_files = [
        tests_dir / "new_tests_results.csv",
        tests_dir / "expert_questions_matrix.csv",
        tests_dir / "module_physics_metadata.csv",
        tests_dir / "toy_model_validation.csv",
        logs_dir / "baseline_reanalysis_metrics.csv",
    ]
    checksums = {str(p.relative_to(root)): (sha256(p) if p.exists() else "MISSING") for p in checksummed_files}

    q_complete = sum(1 for r in questions if r.get("response_status") == "complete")
    q_total = len(questions)
    q_pct = (100.0 * q_complete / q_total) if q_total else 0.0

    lines = [
        "# RAPPORT ANALYSE INDEPENDANTE DEPOT DISTANT",
        "",
        f"- Horodatage UTC: `{now.isoformat()}`",
        f"- Dépôt distant synchronisé: `{args.remote_url}`",
        f"- Commit local courant: `{head}`",
        f"- Commit FETCH_HEAD: `{fetch_head}`",
        f"- Résultat `git fetch`: `{'OK' if fetch_ok else 'FAIL (fallback origin/main ou indisponible)'}`",
        f"- Run analysé: `{run_dir.name}`",
        "",
        "## 1) Synchronisation / intégrité",
        "- Analyse de synchronisation effectuée avant exploitation du run local.",
        f"- Artefacts analysés: tests={tests_dir}, logs={logs_dir}.",
        "- Les checksums listés plus bas permettent un audit externe bit-à-bit.",
        "",
        "## 2) Couverture réelle des modules actuellement intégrés",
        f"- Modules détectés dans `module_physics_metadata.csv`: **{', '.join(integrated_modules) if integrated_modules else 'aucun'}**.",
        f"- Questions expertes complètes: **{q_complete}/{q_total} ({q_pct:.2f}%)**.",
        f"- Simulations de contrôle détectées (`new_tests_results.csv`, family=control): **{', '.join(control_by_id) if control_by_id else 'aucune'}**.",
        "",
        "## 3) Modules avancés simulables CPU/RAM mais non encore traités (écart maximal)",
        "| Module candidat | Domaine | Observables possibles immédiatement | Contraintes CPU/RAM | Statut |",
        "|---|---|---|---|---|",
    ]

    for row in unresolved:
        lines.append(
            f"| {row['module']} | {row['domain']} | {row['observables']} | {row['constraints']} | {row['status']} |"
        )

    lines += [
        "",
        "## 4) Pourquoi `toy_model_validation.csv` peut sembler vide sur certains runs",
        f"- Run courant `{run_dir.name}`: lignes utiles détectées = **{len(toy)}**.",
        f"- Runs historiques avec fichier vide (taille 0): **{', '.join(toy_empty_runs) if toy_empty_runs else 'aucun'}**.",
        "- Diagnostic: un fichier vide signifie une interruption avant l’écriture des en-têtes/mesures (crash, kill, arrêt prématuré, ou artefact d’un ancien binaire), pas une validation scientifique réussie.",
        "- Correctif opérationnel immédiat: considérer tout `toy_model_validation.csv` de taille 0 comme **FAIL bloquant** lors des post-runs.",
        "",
        "## 5) Intégration métrique avancée (`METRIQUE/METRIQUE.md`)",
        "- Le référentiel métrique doit contenir les métriques exploitables immédiatement (énergie, pairing, sign ratio, corrélations, convergence dt, FFT dominante, entropie fullscale, stabilité numérique, runtime matériel).",
        "- Ce rapport renvoie vers cette base pour planifier les nouveaux modules sans simplifications non contrôlées.",
        "",
        "## 6) Modélisation 3D + serveur",
        "- Génération d’un artefact 3D (HTML + JSON) recommandée à partir des observables normalisées pour inspection interactive.",
        "- Exposition locale reproductible: `python3 -m http.server --directory <run>/reports/3d 8080`.",
        "",
        "## 7) Checksums de preuve (A→Z)",
    ]

    for rel, digest in checksums.items():
        lines.append(f"- `{rel}`: `{digest}`")

    if not fetch_ok and fetch_details:
        lines += [
            "",
            "## 8) Diagnostic fetch (non bloquant)",
            "```text",
            fetch_details,
            "```",
        ]

    lines += [
        "",
        "## 9) Commandes exactes reproductibles",
        "```bash",
        f"git fetch {args.remote_url} --prune",
        "bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh",
        f"python3 src/advanced_calculations/quantum_problem_hubbard_hts/tools/post_run_remote_depot_independent_analysis.py src/advanced_calculations/quantum_problem_hubbard_hts --run-dir {run_dir}",
        "```",
    ]

    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    summary = {
        "generated_at_utc": now.isoformat(),
        "remote_url": args.remote_url,
        "head": head,
        "fetch_head": fetch_head,
        "fetch_status": "ok" if fetch_ok else "failed",
        "fetch_details": fetch_details,
        "run": run_dir.name,
        "integrated_modules": integrated_modules,
        "control_tests": control_by_id,
        "questions_complete": {"complete": q_complete, "total": q_total, "pct": q_pct},
        "unresolved_cpu_ram_modules": unresolved,
        "toy_rows_current_run": len(toy),
        "toy_empty_runs": toy_empty_runs,
        "checksums": checksums,
        "report": str(report_path.relative_to(root)),
    }
    json_path.write_text(json.dumps(summary, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    print(report_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
