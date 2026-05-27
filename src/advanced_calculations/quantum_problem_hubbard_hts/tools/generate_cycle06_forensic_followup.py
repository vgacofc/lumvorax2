#!/usr/bin/env python3
import csv
import hashlib
import json
from collections import Counter, defaultdict
from datetime import datetime, UTC
from pathlib import Path
from statistics import mean

RUN_DIR = Path("src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260308T045332Z_572")
TESTS = RUN_DIR / "tests"
LOGS = RUN_DIR / "logs"
REPORTS = RUN_DIR / "reports"


def read_csv(path: Path):
    with path.open("r", encoding="utf-8", newline="") as f:
        return list(csv.DictReader(f))


def pct(a, b):
    return 0.0 if b == 0 else (100.0 * a / b)


def main():
    new_tests = read_csv(TESTS / "new_tests_results.csv")
    progress = read_csv(TESTS / "integration_problem_solution_progress.csv")
    expert_q = read_csv(TESTS / "expert_questions_matrix.csv")
    num_stability = read_csv(TESTS / "numerical_stability_suite.csv")
    qmc = read_csv(TESTS / "benchmark_comparison_qmc_dmrg.csv")
    ext = read_csv(TESTS / "benchmark_comparison_external_modules.csv")
    open_q = read_csv(TESTS / "integration_open_questions_backlog.csv")
    observables = read_csv(TESTS / "integration_physics_computed_observables.csv")
    drift = read_csv(TESTS / "integration_run_drift_monitor.csv")

    science = json.loads((LOGS / "analysis_scientifique_summary.json").read_text(encoding="utf-8"))
    independent = json.loads((LOGS / "independent_log_review_summary.json").read_text(encoding="utf-8"))

    status_counts = Counter(r["status"] for r in new_tests)
    family_stats = defaultdict(lambda: Counter())
    for row in new_tests:
        fam = row["test_family"]
        family_stats[fam]["total"] += 1
        family_stats[fam][row["status"]] += 1

    for row in progress:
        row["solution_progress_percent"] = float(row["solution_progress_percent"])
        row["remaining_percent"] = 100.0 - row["solution_progress_percent"]

    progress_sorted = sorted(progress, key=lambda r: r["solution_progress_percent"], reverse=True)
    avg_progress = mean(r["solution_progress_percent"] for r in progress_sorted)

    energy_drifts = [float(r["value"]) for r in num_stability if r["metric"] == "energy_density_drift_max"]
    stability_fail = sum(1 for r in num_stability if r["status"] == "FAIL")

    qmc_within = pct(sum(int(r.get("within_error_bar") or "0") for r in qmc), len(qmc))
    ext_within = pct(sum(int(r.get("within_error_bar") or "0") for r in ext), len(ext))
    qmc_rel = mean(float(r["rel_error"]) for r in qmc)
    ext_rel = mean(float(r["rel_error"]) for r in ext)

    sens_t = [r for r in new_tests if r["test_id"].startswith("sens_T_")]
    sens_u = [r for r in new_tests if r["test_id"].startswith("sens_U_")]
    sign_col = next((k for k in observables[0].keys() if "sign" in k.lower()), None)
    sign_vals = [float(r[sign_col]) for r in observables if sign_col and r.get(sign_col)]

    drift_sorted = sorted(drift, key=lambda r: float(r.get("max_abs_diff") or 0.0), reverse=True)

    now = datetime.now(UTC).strftime("%Y-%m-%dT%H:%M:%SZ")
    report_path = REPORTS / "RAPPORT_FORENSIQUE_EXHAUSTIF_CYCLE06_20260308.md"
    targeted_tests_csv = TESTS / "integration_open_questions_targeted_tests_cycle06.csv"

    lines = []
    lines.append("# Rapport forensique exhaustif (post-review) — research_20260308T045332Z_572")
    lines.append("")
    lines.append(f"- Généré UTC: {now}")
    lines.append("- Pipeline: lecture directe CSV/JSON locaux, sans modification des anciens rapports.")
    lines.append("")
    lines.append("## Expertises mobilisées et incidents rencontrés")
    lines.append("- Expertises mobilisées: méthodes numériques, physique de la matière condensée (Hubbard/HTS), validation statistique, audit de traçabilité.")
    lines.append("- Incident observé: manifeste SHA global référence 4 fichiers absents (environment/provenance/runtime). Cela réduit l’auditabilité historique complète, mais n’altère pas les CSV/JSON présents.")
    lines.append("")
    lines.append("## 1) Signification (niveau expert → explication pédagogique)")
    lines.append("- **Pairing**: indicateur de tendance à former des paires corrélées. S’il baisse avec la température, cela indique que l’agitation thermique détruit les paires.")
    lines.append("- **Sign ratio**: indicateur de sévérité du problème de signe; plus proche de zéro (ou négatif instable), plus les estimations peuvent devenir bruitées/biaisées.")
    lines.append("- **Convergence en dt**: vérifie que réduire le pas temporel ne change plus significativement la réponse; sinon la solution dépend trop du schéma numérique.")
    lines.append("- **Von Neumann spectral radius**: si > 1, les erreurs numériques peuvent s’amplifier au cours du temps.")
    lines.append("")
    lines.append("## 2) Ce que les nouveaux logs apportent réellement (nouvelles informations)")
    lines.append(f"- Avancement global confirmé: {science['overall_progress_pct']:.2f}% (vue principale) vs {independent['progress_score_pct']:.2f}% (vue indépendante).")
    lines.append(f"- Questions expertes complètement traitées: {science['expert_complete_pct']:.2f}% ({science['question_status']['complete']}/{science['counts']['expert_questions']}).")
    lines.append(f"- Campagne nouveaux tests: PASS={status_counts['PASS']}, FAIL={status_counts['FAIL']}, OBSERVED={status_counts['OBSERVED']} (total={len(new_tests)}).")
    lines.append(f"- Stabilité numérique: {stability_fail}/{len(num_stability)} FAIL ({pct(stability_fail,len(num_stability)):.2f}%).")
    lines.append("")

    lines.append("## 3) État d’avancement par simulation (exact %) + reste")
    lines.append("| Module | Progression % | Reste % | Lecture rapide |")
    lines.append("|---|---:|---:|---|")
    for r in progress_sorted:
        lines.append(f"| {r['problem']} | {r['solution_progress_percent']:.2f} | {r['remaining_percent']:.2f} | {r['evidence']} |")
    lines.append(f"\n- Moyenne: {avg_progress:.2f}% ; reste moyen: {100.0-avg_progress:.2f}%.")
    lines.append("")

    lines.append("## 4) Analyse scientifique (énergie, corrélations, pairing, signe)")
    if sens_t:
        tvals = [float(x["value"]) for x in sens_t]
        lines.append(f"- Pairing(T): {tvals[0]:.6f} → {tvals[-1]:.6f}, baisse relative={pct(tvals[0]-tvals[-1], tvals[0]):.2f}%.")
    if sens_u:
        uvals = [float(x["value"]) for x in sens_u]
        lines.append(f"- Energy(U): {uvals[0]:.6f} → {uvals[-1]:.6f}, hausse relative={pct(uvals[-1]-uvals[0], uvals[0]):.2f}%.")
    if sign_vals:
        lines.append(f"- Sign ratio ({sign_col}): min={min(sign_vals):.6f}, max={max(sign_vals):.6f}, moyenne={mean(sign_vals):.6f}.")
    lines.append("- Interprétation: tendances thermiques qualitatives cohérentes, mais confiance quantitative limitée par les FAIL benchmark et stabilité.")
    lines.append("")

    lines.append("## 5) Questions expertes par résultat (format Question → Analyse → Réponse → Solution)")
    lines.append("### Résultat A — Pairing décroît avec T")
    lines.append("- Question: la pente est-elle robuste à dt et taille de système ?")
    lines.append("- Analyse: tendance monotone présente, mais dt_sweep contient un FAIL explicite de convergence.")
    lines.append("- Réponse: **partielle**.")
    lines.append("- Solution: exécuter grille dt fine + extrapolation dt→0 + test multi-lattice.")
    lines.append("### Résultat B — Énergie augmente avec U")
    lines.append("- Question: l’échelle énergétique est-elle correctement normalisée par site/unité ?")
    lines.append("- Analyse: benchmark externe très divergent (erreur relative moyenne extrême).")
    lines.append("- Réponse: **partielle**.")
    lines.append("- Solution: recalibrage unités + normalisation par site + cross-check non-heuristic.")
    lines.append("### Résultat C — Reproductibilité inter-run")
    lines.append("- Question: les observables clés varient-elles entre runs ?")
    lines.append("- Analyse: run_drift_monitor montre diff nulle pour energy/pairing/sign_ratio, dérive seulement sur elapsed_ns.")
    lines.append("- Réponse: **complète** pour observables physiques.")
    lines.append("- Solution: conserver ce contrôle en gate bloquant.")
    lines.append("### Résultat D — Stabilité longue durée")
    lines.append("- Question: l’intégrateur est-il stable à long horizon ?")
    lines.append("- Analyse: spectral radius >1 et energy_density_drift_max élevé sur de nombreux modules.")
    lines.append("- Réponse: **incomplète / risque élevé**.")
    lines.append("- Solution: schéma plus dissipatif/implicite et critère de stabilité strict par module.")
    lines.append("")

    lines.append("## 6) Classement par risque numérique (complet)")
    lines.append("| Famille | Pass % | Niveau de risque | Pourquoi |")
    lines.append("|---|---:|---|---|")
    for fam, c in sorted(family_stats.items(), key=lambda kv: pct(kv[1]["PASS"], kv[1]["total"])):
        pp = pct(c["PASS"], c["total"])
        risk = "Élevé" if pp < 40 else ("Moyen" if pp < 80 else "Faible")
        lines.append(f"| {fam} | {pp:.2f} | {risk} | PASS={c['PASS']}, FAIL={c['FAIL']}, OBSERVED={c['OBSERVED']} |")
    lines.append("")

    lines.append("## 7) Nouvelles questions expertes non encore intégrées")
    for q in open_q:
        lines.append(f"- {q['question_id']}: {q['question']} (statut={q['status']}, action={q['recommended_action']})")
    lines.append("")

    lines.append("## 8) Commandes reproductibles")
    lines.append("```bash")
    lines.append("git fetch origin --prune")
    lines.append("git rev-parse HEAD && git rev-parse origin/main")
    lines.append("cd src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260308T045332Z_572")
    lines.append("sha256sum -c logs/checksums.sha256")
    lines.append("python src/advanced_calculations/quantum_problem_hubbard_hts/tools/generate_cycle06_forensic_followup.py")
    lines.append("bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh")
    lines.append("```")

    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    with targeted_tests_csv.open("w", encoding="utf-8", newline="") as f:
        w = csv.writer(f)
        w.writerow(["question_id", "question", "targeted_test", "metric", "acceptance_criteria", "priority"])
        w.writerow(["Q_missing_units", "Unités explicites", "unit_consistency_gate", "unit_mismatch_count", "=0", "P1"])
        w.writerow(["Q_solver_crosscheck", "Cross-check non-heuristic", "independent_solver_large_lattice", "rel_error_vs_reference", "<10%", "P1"])
        w.writerow(["Q_dt_real_sweep", "Stabilité dt réelle", "dt_half_dt_double", "observable_delta", "<1%", "P1"])
        w.writerow(["Q_phase_criteria", "Critères transition phase", "finite_size_scaling_suite", "critical_exponent_fit_rmse", "<0.05", "P2"])
        w.writerow(["Q_production_guardrails", "Rollback V4 NEXT", "degradation_rollback_drill", "rollback_latency_s", "<60", "P2"])

    checksum_lines = []
    for p in [report_path, targeted_tests_csv]:
        digest = hashlib.sha256(p.read_bytes()).hexdigest()
        checksum_lines.append(f"{digest}  ./{p.relative_to(RUN_DIR)}")
    (LOGS / "forensic_followup_checksums.sha256").write_text("\n".join(checksum_lines) + "\n", encoding="utf-8")

    summary = {
        "generated_at_utc": now,
        "report": str(report_path.relative_to(RUN_DIR)),
        "targeted_tests": str(targeted_tests_csv.relative_to(RUN_DIR)),
        "new_insights": {
            "overall_progress_pct": science["overall_progress_pct"],
            "independent_progress_pct": independent["progress_score_pct"],
            "expert_complete_pct": science["expert_complete_pct"],
            "numerical_stability_fail_pct": pct(stability_fail, len(num_stability)),
            "qmc_within_error_bar_pct": qmc_within,
            "external_within_error_bar_pct": ext_within,
        },
        "top_drift_metrics": drift_sorted[:3],
    }
    (LOGS / "forensic_followup_summary.json").write_text(json.dumps(summary, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
