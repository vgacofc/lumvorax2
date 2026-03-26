#!/usr/bin/env python3
# G-C23-06 : Granularité 100% — log forensique Python step-by-step
import csv
import json
import sys
from pathlib import Path
from statistics import mean

def _lv_log(phase: str, step: str, metric: str, value):
    """G-C23-06 : Log forensique Python step-by-step vers stdout."""
    print(f"[FORENSIC_PY] phase={phase} step={step} metric={metric} value={value}", flush=True)

EXPECTED_METADATA_FIELDS = [
    "model_id",
    "hamiltonian_id",
    "lattice_size",
    "boundary_conditions",
    "u_over_t",
    "time_step",
    "seed",
    "solver_version",
]

GLOSSARY_ROWS = [
    ("problem", "Nom du module simulé", "Chaque groupe de lignes correspond à un module", "Comparer modules sans normalisation peut induire une fausse interprétation"),
    ("step", "Indice d'avancement numérique", "+100 = incrément de simulation", "step n'est pas forcément un temps physique réel"),
    ("energy", "Observable énergétique interne", "Peut être négative/positive selon le modèle", "Sans unités/normalisation, pas de conclusion physique forte"),
    ("pairing", "Observable de corrélation", "Ici souvent cumulative", "Ne pas interpréter comme ordre supraconducteur sans définition formelle"),
    ("sign_ratio", "Indicateur statistique de signe", "Proche de 0 peut indiquer compensation des signes", "Ne suffit pas seul pour conclure sur le sign-problem"),
    ("cpu_percent", "Charge CPU système", "Valeur stable = performance stable", "N'apporte pas de preuve de validité physique"),
    ("mem_percent", "Usage RAM système", "Valeur stable = pas de fuite évidente", "N'évalue pas la justesse scientifique"),
    ("elapsed_ns", "Temps écoulé en nanosecondes", "Croissance ~linéaire = coût local stable", "Ne prouve pas la complexité asymptotique globale"),
]


def load_metrics(path: Path):
    rows = []
    malformed = []
    with path.open(newline="") as f:
        reader = csv.DictReader(f)
        for idx, row in enumerate(reader, start=2):
            if None in row.values():
                malformed.append((idx, row))
                continue
            try:
                row["step"] = int(row["step"])
                for k in ["energy", "pairing", "sign_ratio", "cpu_percent", "mem_percent", "elapsed_ns"]:
                    row[k] = float(row[k])
            except (TypeError, ValueError):
                malformed.append((idx, row))
                continue
            rows.append(row)
    return rows, malformed


def load_scalar_metadata(run_dir: Path):
    meta_path = run_dir / "logs" / "model_metadata.json"
    if not meta_path.exists():
        return {}
    try:
        payload = json.loads(meta_path.read_text())
        if isinstance(payload, dict):
            return payload
        return {}
    except Exception:
        return {}


def write_csv(path: Path, header, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(header)
        writer.writerows(rows)


def find_previous_run(results_dir: Path, current_run: str):
    runs = sorted([d.name for d in results_dir.iterdir() if d.is_dir() and d.name.startswith("research_")])
    if current_run not in runs:
        return None
    idx = runs.index(current_run)
    if idx == 0:
        return None
    return runs[idx - 1]


def main():
    if len(sys.argv) != 3:
        print("Usage: post_run_cycle_guard.py <root_dir> <run_dir>", file=sys.stderr)
        return 2

    root_dir = Path(sys.argv[1]).resolve()
    run_dir = Path(sys.argv[2]).resolve()
    results_dir = root_dir / "results"

    baseline = run_dir / "logs" / "baseline_reanalysis_metrics.csv"
    if not baseline.exists():
        print(f"[cycle-guard] baseline missing: {baseline}", file=sys.stderr)
        return 3

    rows, malformed = load_metrics(baseline)
    scalar_meta = load_scalar_metadata(run_dir)

    glossary_path = run_dir / "tests" / "integration_terms_glossary.csv"
    write_csv(glossary_path, ["term", "simple_explanation", "how_to_read_value", "risk_of_misinterpretation"], GLOSSARY_ROWS)

    claims_path = run_dir / "tests" / "integration_claim_confidence_tags.csv"
    claims = [
        ("CSV integrity", "certain" if len(malformed) == 0 else "certain_failure", "Computed from column parse and malformed rows count"),
        ("Observable reproducibility vs previous run", "probable", "Upgraded to certain if point-to-point diffs are all zero"),
        ("Physical interpretation finality", "unknown", "Needs explicit metadata + physics gate criteria"),
        ("Performance drift interpretation", "probable", "elapsed_ns drift often infra-driven; verify with controlled benchmark"),
    ]
    write_csv(claims_path, ["claim", "confidence_tag", "reason"], claims)

    absent_path = run_dir / "tests" / "integration_absent_metadata_fields.csv"
    absent_rows = []
    missing_count = 0
    for field in EXPECTED_METADATA_FIELDS:
        value = scalar_meta.get(field, "")
        is_missing = str(value).strip() in ("", "UNKNOWN", "None")
        if is_missing:
            missing_count += 1
            absent_rows.append((field, "ABSENT", "Add to schema/header/provenance before physics-claim gate"))
        else:
            absent_rows.append((field, "PRESENT", f"value={value}"))
        # G-C23-06 : log forensique de chaque champ de métadonnée
        _lv_log("cycle_guard_meta", field, "is_missing", int(is_missing))
    write_csv(absent_path, ["field", "status", "action"], absent_rows)

    modules = sorted({r["problem"] for r in rows})
    steps_by_module = {m: sorted([r["step"] for r in rows if r["problem"] == m]) for m in modules}
    manifest_rows = []
    for m in modules:
        s = steps_by_module[m]
        monotonic = 1 if all(s[i] >= s[i - 1] for i in range(1, len(s))) else 0
        # G-C23-06 : log forensique de chaque module du manifeste
        _lv_log("cycle_guard_manifest", m, "row_count",  len(s))
        _lv_log("cycle_guard_manifest", m, "step_min",   s[0]  if s else -1)
        _lv_log("cycle_guard_manifest", m, "step_max",   s[-1] if s else -1)
        _lv_log("cycle_guard_manifest", m, "monotonic",  monotonic)
        manifest_rows.append((m, len(s), s[0] if s else "", s[-1] if s else "", monotonic))
    manifest_path = run_dir / "tests" / "integration_manifest_check.csv"
    write_csv(manifest_path, ["module", "row_count", "step_min", "step_max", "step_monotonic_non_decreasing"], manifest_rows)

    drift_path = run_dir / "tests" / "integration_run_drift_monitor.csv"
    prev_run = find_previous_run(results_dir, run_dir.name)
    drift_rows = []
    if prev_run:
        prev_baseline = results_dir / prev_run / "logs" / "baseline_reanalysis_metrics.csv"
        if prev_baseline.exists():
            prev_rows, prev_malformed = load_metrics(prev_baseline)
            prev_map = {(r["problem"], r["step"]): r for r in prev_rows}
            common = [((r["problem"], r["step"]), r, prev_map[(r["problem"], r["step"])]) for r in rows if (r["problem"], r["step"]) in prev_map]
            for metric in ["energy", "pairing", "sign_ratio", "elapsed_ns"]:
                diffs = [abs(cur[metric] - old[metric]) for _, cur, old in common]
                max_d = max(diffs) if diffs else 0.0
                mean_d = mean(diffs) if diffs else 0.0
                # G-C23-06 : log forensique step-by-step de chaque métrique de drift
                _lv_log("cycle_guard_drift", metric, "n_common",  len(common))
                _lv_log("cycle_guard_drift", metric, "max_diff",  max_d)
                _lv_log("cycle_guard_drift", metric, "mean_diff", mean_d)
                drift_rows.append((metric, len(common), max_d if diffs else "", mean_d if diffs else "", prev_run, run_dir.name))
            drift_rows.append(("prev_malformed_rows", len(prev_malformed), "", "", prev_run, run_dir.name))
        else:
            drift_rows.append(("info", 0, "", "previous baseline missing", prev_run, run_dir.name))
    else:
        drift_rows.append(("info", 0, "", "no previous run", "", run_dir.name))

    write_csv(drift_path, ["metric", "common_points", "max_abs_diff", "mean_abs_diff", "reference_run", "current_run"], drift_rows)

    gate_path = run_dir / "tests" / "integration_gate_summary.csv"
    gate_rows = [
        ("CSV_INTEGRITY_GATE", "PASS" if len(malformed) == 0 else "FAIL", f"malformed_rows={len(malformed)}"),
        ("MODULE_COVERAGE_GATE", "PASS" if len(modules) > 0 else "FAIL", f"module_count={len(modules)}"),
        ("GLOSSARY_GATE", "PASS", f"file={glossary_path.name}"),
        ("CONFIDENCE_TAG_GATE", "PASS", f"file={claims_path.name}"),
        ("ABSENT_METADATA_EXTRACTOR_GATE", "PASS" if missing_count == 0 else "FAIL", f"missing_fields={missing_count}"),
    ]
    write_csv(gate_path, ["gate", "status", "evidence"], gate_rows)

    print(f"[cycle-guard] generated: {glossary_path}")
    print(f"[cycle-guard] generated: {claims_path}")
    print(f"[cycle-guard] generated: {absent_path}")
    print(f"[cycle-guard] generated: {manifest_path}")
    print(f"[cycle-guard] generated: {drift_path}")
    print(f"[cycle-guard] generated: {gate_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
