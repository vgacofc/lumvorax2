#!/usr/bin/env python3
"""Independent post-run scientific review.

This script intentionally reads raw run artifacts and produces an analysis report
independent from pipeline-generated narrative reports.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
from collections import Counter, defaultdict
from datetime import datetime, timezone
from pathlib import Path


def read_csv(path: Path):
    if not path.exists():
        return []
    with path.open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def safe_float(v: str, default: float = 0.0) -> float:
    try:
        return float(v)
    except Exception:
        return default


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("run_dir")
    args = ap.parse_args()

    run_dir = Path(args.run_dir).resolve()
    logs = run_dir / "logs"
    tests = run_dir / "tests"
    reports = run_dir / "reports"
    reports.mkdir(parents=True, exist_ok=True)

    new_tests = read_csv(tests / "new_tests_results.csv")
    qrows = read_csv(tests / "expert_questions_matrix.csv")
    base_metrics = read_csv(logs / "baseline_reanalysis_metrics.csv")
    norm_trace = read_csv(logs / "normalized_observables_trace.csv")
    nstab = read_csv(tests / "numerical_stability_suite.csv")
    toy = read_csv(tests / "toy_model_validation.csv")
    module_meta = read_csv(tests / "module_physics_metadata.csv")

    by_family = defaultdict(list)
    for row in new_tests:
        by_family[row.get("test_family", "unknown")].append(row)

    family_ratio = {}
    for fam, rows in sorted(by_family.items()):
        total = len(rows)
        passed = sum(1 for r in rows if r.get("status") == "PASS")
        failed = sum(1 for r in rows if r.get("status") == "FAIL")
        observed = total - passed - failed
        family_ratio[fam] = {
            "total": total,
            "passed": passed,
            "failed": failed,
            "observed": observed,
            "pass_pct": (100.0 * passed / total) if total else 0.0,
        }

    question_counter = Counter(r.get("response_status", "unknown") for r in qrows)
    q_total = len(qrows)
    q_complete = question_counter.get("complete", 0)
    q_complete_pct = 100.0 * q_complete / q_total if q_total else 0.0

    # Core observables summary (independent computations from raw tables)
    energy_vals = [safe_float(r.get("energy", "0")) for r in base_metrics]
    pairing_vals = [safe_float(r.get("pairing", "0")) for r in base_metrics]
    sign_vals = [safe_float(r.get("sign_ratio", "0")) for r in base_metrics]

    mean_energy = sum(energy_vals) / len(energy_vals) if energy_vals else 0.0
    mean_pairing = sum(pairing_vals) / len(pairing_vals) if pairing_vals else 0.0
    mean_sign = sum(sign_vals) / len(sign_vals) if sign_vals else 0.0

    # Progress to solution (independent weighted score)
    w = {
        "reproducibility": 0.18,
        "convergence": 0.14,
        "benchmark": 0.14,
        "control": 0.10,
        "stability": 0.10,
        "spectral": 0.10,
        "cluster_scale": 0.08,
        "questions_complete": 0.08,
        "numerical_stability": 0.08,
    }

    numerical_stability_pass_pct = 0.0
    if nstab:
        npass = sum(1 for r in nstab if r.get("status") == "PASS")
        numerical_stability_pass_pct = 100.0 * npass / len(nstab)

    score = 0.0
    score += w["reproducibility"] * family_ratio.get("reproducibility", {}).get("pass_pct", 0.0)
    score += w["convergence"] * family_ratio.get("convergence", {}).get("pass_pct", 0.0)
    score += w["benchmark"] * family_ratio.get("benchmark", {}).get("pass_pct", 0.0)
    score += w["control"] * family_ratio.get("control", {}).get("pass_pct", 0.0)
    score += w["stability"] * family_ratio.get("stability", {}).get("pass_pct", 0.0)
    score += w["spectral"] * family_ratio.get("spectral", {}).get("pass_pct", 0.0)
    score += w["cluster_scale"] * family_ratio.get("cluster_scale", {}).get("pass_pct", 0.0)
    score += w["questions_complete"] * q_complete_pct
    score += w["numerical_stability"] * numerical_stability_pass_pct

    fails_new_tests = [r for r in new_tests if r.get("status") == "FAIL"]
    fails_nstab = [r for r in nstab if r.get("status") == "FAIL"]

    report_path = reports / "RAPPORT_ANALYSE_INDEPENDANTE_LOGS_CYCLE06.md"
    summary_path = logs / "independent_log_review_summary.json"
    checksum_path = logs / "independent_log_review_checksums.sha256"

    lines = []
    lines.append("# Rapport d’analyse indépendante des logs (hors rapport pipeline)")
    lines.append("")
    lines.append(f"Run analysé: `{run_dir.name}`")
    lines.append(f"Horodatage UTC: `{datetime.now(timezone.utc).isoformat()}`")
    lines.append("")
    lines.append("## Phase 1 — Synchronisation et intégrité")
    lines.append("- Revue indépendante effectuée à partir des artefacts bruts du run (`logs/*.csv`, `tests/*.csv`).")
    lines.append(f"- Fichier checksum global présent: **{'oui' if (logs / 'checksums.sha256').exists() else 'non'}**.")
    lines.append("")
    lines.append("## Phase 2 — Analyse des données")
    lines.append(f"- Lignes métriques brutes: **{len(base_metrics)}**")
    lines.append(f"- Trace normalisée: **{len(norm_trace)}**")
    lines.append(f"- Questions expertes: **{q_total}**")
    lines.append(f"- Modules metadata: **{len(module_meta)}**")
    lines.append(f"- Moyenne énergie brute: **{mean_energy:.6f}**")
    lines.append(f"- Moyenne pairing brut: **{mean_pairing:.6f}**")
    lines.append(f"- Moyenne sign ratio brut: **{mean_sign:.6f}**")
    lines.append("")
    lines.append("## Phase 3 — Vérification exhaustive")
    lines.append("- Vérification famille par famille des tests PASS/FAIL/OBSERVED:")
    for fam, s in family_ratio.items():
        lines.append(
            f"  - `{fam}`: PASS={s['passed']} FAIL={s['failed']} OBSERVED={s['observed']} (pass={s['pass_pct']:.2f}%)"
        )
    lines.append("")
    lines.append("## Phase 4 — Analyse scientifique")
    lines.append(f"- Couverture questions expertes complètes: **{q_complete}/{q_total} ({q_complete_pct:.2f}%)**")
    lines.append(f"- Stabilité numérique (suite dédiée): **{numerical_stability_pass_pct:.2f}% PASS**")
    lines.append("- Points en échec détectés et conservés (pas masqués):")
    if not fails_new_tests and not fails_nstab:
        lines.append("  - Aucun FAIL détecté.")
    else:
        for r in fails_new_tests:
            lines.append(f"  - `new_tests_results`: {r.get('test_family')}/{r.get('test_id')} -> FAIL (value={r.get('value')})")
        for r in fails_nstab:
            lines.append(f"  - `numerical_stability_suite`: {r.get('test_id')}::{r.get('module')} -> FAIL ({r.get('metric')}={r.get('value')})")
    lines.append("")
    lines.append("## Phase 5 — Interprétation pédagogique")
    lines.append("- Un test `PASS` signifie que le critère numérique codé est satisfait.")
    lines.append("- Un test `FAIL` signifie qu’une hypothèse n’est pas validée avec les paramètres actuels.")
    lines.append("- Un test `OBSERVED` signifie valeur mesurée sans gate binaire stricte.")
    lines.append("")
    lines.append("## Phase 6 — Réponse point par point")
    lines.append("- **Question**: Les valeurs et % sont-elles à jour ?")
    lines.append("  - **Analyse**: recalcul direct depuis fichiers du run.")
    lines.append("  - **Réponse**: Oui, ce rapport recalcule tous les ratios au moment de l’exécution.")
    lines.append("  - **Solution**: conserver ce rapport indépendant dans chaque nouveau run.")
    lines.append("- **Question**: Les nouvelles questions sont-elles bien incluses ?")
    lines.append("  - **Analyse**: lecture de `expert_questions_matrix.csv` et comptage des statuts.")
    lines.append("  - **Réponse**: Oui, incluses et quantifiées ici.")
    lines.append("  - **Solution**: ajouter un test bloquant si question obligatoire absente.")
    lines.append("")
    lines.append("## Phase 7 — Correctifs proposés")
    if fails_new_tests or fails_nstab:
        lines.append("1. Réduire dt et augmenter steps uniquement sur modules/tests explicitement en FAIL.")
        lines.append("2. Étendre la campagne multi-seed ciblée sur les familles en échec (pas sur les PASS).")
        lines.append("3. Ajouter/resserrer benchmarks externes individuels pour les modules encore partiels.")
    else:
        lines.append("1. Aucun FAIL: conserver les paramètres et passer en campagne de reproductibilité élargie.")
        lines.append("2. Vérifier périodiquement les checksums et la stabilité inter-run.")
        lines.append("3. Étendre uniquement les observables physiques (pas de changement de base numérique).")
    lines.append("")
    lines.append("## Phase 8/9 — Intégration technique et traçabilité")
    lines.append("- Rapport indépendant écrit dans `reports/`.")
    lines.append("- Résumé machine en JSON + checksums SHA256 dédiés écrits dans `logs/`.")
    lines.append("")
    lines.append("## Phase 10 — Commande reproductible")
    lines.append("```bash")
    lines.append("bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh")
    lines.append("```")
    lines.append("")
    lines.append("## État d’avancement vers la solution (%)")
    lines.append(f"- Score pondéré indépendant: **{score:.2f}%**")

    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    summary = {
        "run": run_dir.name,
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "counts": {
            "base_metrics_rows": len(base_metrics),
            "normalized_rows": len(norm_trace),
            "new_tests": len(new_tests),
            "questions": q_total,
            "modules": len(module_meta),
            "toy_rows": len(toy),
        },
        "means": {
            "energy": mean_energy,
            "pairing": mean_pairing,
            "sign_ratio": mean_sign,
        },
        "family_ratio": family_ratio,
        "question_status": dict(question_counter),
        "question_complete_pct": q_complete_pct,
        "numerical_stability_pass_pct": numerical_stability_pass_pct,
        "progress_score_pct": score,
        "fails": {
            "new_tests": fails_new_tests,
            "numerical_stability": fails_nstab,
        },
    }
    summary_path.write_text(json.dumps(summary, indent=2, ensure_ascii=False), encoding="utf-8")

    checksum_lines = [
        f"{sha256(report_path)}  {report_path.name}",
        f"{sha256(summary_path)}  {summary_path.name}",
    ]
    checksum_path.write_text("\n".join(checksum_lines) + "\n", encoding="utf-8")

    print(f"[independent-review] generated: {report_path}")
    print(f"[independent-review] generated: {summary_path}")
    print(f"[independent-review] generated: {checksum_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
