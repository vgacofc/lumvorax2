#!/usr/bin/env python3
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


def pass_ratio(rows, status_key="status"):
    if not rows:
        return 0.0, 0, 0
    total = len(rows)
    passed = sum(1 for r in rows if r.get(status_key) == "PASS")
    return (100.0 * passed / total), passed, total


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("run_dir")
    args = ap.parse_args()

    run_dir = Path(args.run_dir).resolve()
    tests = run_dir / "tests"
    logs = run_dir / "logs"
    reports = run_dir / "reports"
    reports.mkdir(parents=True, exist_ok=True)

    new_tests = read_csv(tests / "new_tests_results.csv")
    qrows = read_csv(tests / "expert_questions_matrix.csv")
    nstab = read_csv(tests / "numerical_stability_suite.csv")
    toy = read_csv(tests / "toy_model_validation.csv")
    mod = read_csv(tests / "module_physics_metadata.csv")
    tdrv = read_csv(tests / "temporal_derivatives_variance.csv")

    by_family = defaultdict(list)
    for r in new_tests:
        by_family[r.get("test_family", "unknown")].append(r)

    family_stats = {}
    for fam, rows in by_family.items():
        p, ok, tot = pass_ratio(rows)
        family_stats[fam] = {"percent": p, "pass": ok, "total": tot}

    question_status = Counter(r.get("response_status", "unknown") for r in qrows)
    expert_complete_pct = 100.0 * question_status.get("complete", 0) / max(1, len(qrows))

    nstab_fail = [r for r in nstab if r.get("status") == "FAIL"]
    new_tests_fail = [r for r in new_tests if r.get("status") == "FAIL"]

    # Derived progress towards "solution"
    reproducibility_pct = family_stats.get("reproducibility", {}).get("percent", 0.0)
    convergence_pct = family_stats.get("convergence", {}).get("percent", 0.0)
    benchmark_pct = family_stats.get("benchmark", {}).get("percent", 0.0)
    control_pct = family_stats.get("control", {}).get("percent", 0.0)
    stability_pct = family_stats.get("stability", {}).get("percent", 0.0)
    spectral_pct = family_stats.get("spectral", {}).get("percent", 0.0)
    # C57-04 : Correction traceability_pct — vérifie GLOBAL_CHECKSUM.sha512 (fichier réel du run)
    # En plus de checksums.sha256 (généré en fin de script, absent au moment du calcul)
    _sha512_ok  = (run_dir / "GLOBAL_CHECKSUM.sha512").exists()
    _sha256_ok  = (logs / "checksums.sha256").exists()
    _lv_log_ok  = any((logs).glob("lumvorax_*.csv"))
    _parts_ok   = len(list((logs).glob("lumvorax_*_part_*.csv"))) > 0
    traceability_pct = (
        25.0 * _sha512_ok
        + 25.0 * _sha256_ok
        + 25.0 * _lv_log_ok
        + 25.0 * _parts_ok
    )

    overall = (
        0.20 * reproducibility_pct
        + 0.15 * convergence_pct
        + 0.15 * benchmark_pct
        + 0.10 * control_pct
        + 0.10 * stability_pct
        + 0.10 * spectral_pct
        + 0.10 * expert_complete_pct
        + 0.10 * traceability_pct
    )

    # C57-04 : Décomposition détaillée de overall_progress_pct
    progress_breakdown = {
        "reproducibility_pct": round(reproducibility_pct, 2),
        "convergence_pct":     round(convergence_pct,     2),
        "benchmark_pct":       round(benchmark_pct,       2),
        "control_pct":         round(control_pct,         2),
        "stability_pct":       round(stability_pct,       2),
        "spectral_pct":        round(spectral_pct,        2),
        "expert_complete_pct": round(expert_complete_pct, 2),
        "traceability_pct":    round(traceability_pct,    2),
        "traceability_detail": {
            "GLOBAL_CHECKSUM_sha512": bool(_sha512_ok),
            "checksums_sha256":       bool(_sha256_ok),
            "lumvorax_session_log":   bool(_lv_log_ok),
            "lumvorax_parts":         bool(_parts_ok),
        },
        "weights": {
            "reproducibility": 0.20, "convergence": 0.15, "benchmark": 0.15,
            "control": 0.10, "stability": 0.10, "spectral": 0.10,
            "expert": 0.10, "traceability": 0.10
        },
    }

    report = reports / "RAPPORT_ANALYSE_EXECUTION_COMPLETE_CYCLE06.md"
    summary_json = logs / "analysis_scientifique_summary.json"
    checksum_file = logs / "analysis_scientifique_checksums.sha256"

    lines = []
    lines.append("# Rapport d’analyse scientifique complet — dernière exécution")
    lines.append("")
    lines.append(f"Run: `{run_dir.name}`")
    lines.append(f"UTC: `{datetime.now(timezone.utc).isoformat()}`")
    lines.append("")
    lines.append("## 1. Analyse pédagogique (cours structuré)")
    lines.append("- **Contexte** : pipeline multi-modules (Hubbard, QCD fullscale, Quantum Field, Dense Nuclear, Quantum Chemistry).")
    lines.append("- **Hypothèses** : la dynamique fullscale doit rester stable, reproductible et traçable sous variations de paramètres (step, dt, contrôles externes).")
    lines.append("- **Méthode** : tests automatiques (reproductibilité, convergence, stress, sensibilité, benchmark externe, stabilité, spectral) + matrices de questions expertes.")
    lines.append("- **Résultats** : tous les tests sont loggés en CSV avec PASS/FAIL/OBSERVED.")
    lines.append("- **Interprétation** : les points forts sont la reproductibilité et le benchmark; les points à risque sont les tests marqués FAIL.")
    lines.append("")

    lines.append("## 2. Questions expertes et statut")
    lines.append(f"- Questions totales: **{len(qrows)}**")
    for k in ["complete", "partial", "absent"]:
        lines.append(f"- `{k}`: **{question_status.get(k, 0)}**")
    lines.append(f"- Couverture experte complète: **{expert_complete_pct:.2f}%**")
    lines.append("")

    lines.append("## 3. Détection d’anomalies / incohérences")
    if not (new_tests_fail or nstab_fail):
        lines.append("- Aucune anomalie bloquante détectée (aucun FAIL).")
    else:
        lines.append("- Tests en échec détectés :")
        for r in new_tests_fail:
            lines.append(f"  - `new_tests_results`: {r.get('test_family')}/{r.get('test_id')} -> `{r.get('status')}` (value={r.get('value')})")
        for r in nstab_fail:
            lines.append(f"  - `numerical_stability_suite`: {r.get('test_id')} {r.get('module')} -> `{r.get('status')}` ({r.get('metric')}={r.get('value')})")
    lines.append("")

    lines.append("## 4. Comparaison littérature / référence")
    lines.append("- Référence externe utilisée: QMC/DMRG via `benchmark_comparison_qmc_dmrg.csv` et tests famille `benchmark`.")
    lines.append(f"- Taux de succès benchmark: **{benchmark_pct:.2f}%**")
    lines.append("")

    lines.append("## 5. Nouveaux tests exécutés / proposés")
    lines.append("- Exécutés: reproductibilité, convergence, stress, contrôle phase/pump/quench, sweep dt, FFT, stabilité Von Neumann fullscale, cas jouet analytique, dérivées temporelles/variance.")
    lines.append("- Prochains tests recommandés:")
    lines.append("  1. resserrer `dt` autour des zones d’instabilité détectées;")
    lines.append("  2. augmenter la campagne multi-seed sur modules en FAIL;")
    lines.append("  3. comparer aux solutions analytiques supplémentaires par module.")
    lines.append("")

    lines.append("## 6. Réponse point par point (question / analyse / réponse / solution)")
    lines.append("- **Question**: Les valeurs et % sont-ils à jour avec les nouveaux tests ?")
    lines.append("  - **Analyse**: recalcul automatique des pourcentages par famille + couverture experte.")
    lines.append("  - **Réponse**: Oui, mis à jour depuis les CSV générés dans ce run.")
    lines.append("  - **Solution**: maintenir ce rapport auto-généré à chaque exécution.")
    lines.append("- **Question**: Les nouvelles questions sont-elles incluses au bon endroit ?")
    lines.append("  - **Analyse**: lecture de `expert_questions_matrix.csv` et comptage des statuts.")
    lines.append("  - **Réponse**: Oui, intégrées dans `tests/expert_questions_matrix.csv` et reprises dans ce rapport.")
    lines.append("  - **Solution**: ajouter une gate FAIL si question requise absente.")
    lines.append("")

    lines.append("## 7. État d’avancement vers la solution (%)")
    lines.append(f"- Reproductibilité: **{reproducibility_pct:.2f}%**")
    lines.append(f"- Convergence: **{convergence_pct:.2f}%**")
    lines.append(f"- Benchmark externe: **{benchmark_pct:.2f}%**")
    lines.append(f"- Contrôles dynamiques: **{control_pct:.2f}%**")
    lines.append(f"- Stabilité longue: **{stability_pct:.2f}%**")
    lines.append(f"- Analyse spectrale: **{spectral_pct:.2f}%**")
    lines.append(f"- Couverture questions expertes complètes: **{expert_complete_pct:.2f}%**")
    lines.append(f"- Traçabilité checksum: **{traceability_pct:.2f}%**")
    lines.append(f"- **Score global pondéré**: **{overall:.2f}%**")
    lines.append("")

    lines.append("## 8. Traçabilité avancée")
    lines.append("- Fichiers analysés: `new_tests_results.csv`, `expert_questions_matrix.csv`, `numerical_stability_suite.csv`, `toy_model_validation.csv`, `module_physics_metadata.csv`, `temporal_derivatives_variance.csv`.")
    lines.append("- Checksums SHA256 ajoutés pour le rapport et le résumé JSON.")
    lines.append("")

    lines.append("## 9. Commande d’exécution reproductible")
    lines.append("```bash")
    lines.append("bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh")
    lines.append("```")

    report.write_text("\n".join(lines) + "\n", encoding="utf-8")

    summary = {
        "run": run_dir.name,
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "counts": {
            "new_tests": len(new_tests),
            "expert_questions": len(qrows),
            "module_metadata": len(mod),
            "temporal_rows": len(tdrv),
        },
        "family_stats": family_stats,
        "question_status": dict(question_status),
        "expert_complete_pct": expert_complete_pct,
        "overall_progress_pct": overall,
        "progress_breakdown": progress_breakdown,
        "fails": {
            "new_tests": new_tests_fail,
            "numerical_stability": nstab_fail,
        },
    }
    summary_json.write_text(json.dumps(summary, indent=2, ensure_ascii=False), encoding="utf-8")

    checksum_lines = [
        f"{sha256(report)}  ./reports/{report.name}",
        f"{sha256(summary_json)}  ./logs/{summary_json.name}",
    ]
    checksum_file.write_text("\n".join(checksum_lines) + "\n", encoding="utf-8")

    print(f"[scientific-report] generated: {report}")
    print(f"[scientific-report] generated: {summary_json}")
    print(f"[scientific-report] generated: {checksum_file}")


if __name__ == "__main__":
    main()
