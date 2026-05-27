#!/usr/bin/env python3
import csv
import json
import sys
from pathlib import Path


def parse_lattice_sites(lattice_size: str) -> int:
    try:
        w, h = str(lattice_size).lower().split("x")
        return int(w) * int(h)
    except Exception:
        return 0


def read_problem_names(baseline: Path):
    with baseline.open(newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    return sorted({r.get("problem", "") for r in rows if r.get("problem")})


def read_module_metadata(run_dir: Path):
    """Load metadata from generated run artifacts (no hardcoded physics table)."""
    candidates = [
        run_dir / "tests" / "module_physics_metadata.csv",
        run_dir / "logs" / "model_metadata.csv",
    ]
    for p in candidates:
        if p.exists() and p.stat().st_size > 0:
            with p.open(newline="", encoding="utf-8") as f:
                return list(csv.DictReader(f)), p
    return [], None


def normalize_row(problem: str, src: dict):
    lattice_size = src.get("lattice_size", "")
    t_value = src.get("t", src.get("hopping_t", ""))
    u_value = src.get("U", src.get("interaction_u", ""))
    temp_value = src.get("T", src.get("beta", src.get("temperature", "")))
    dt_value = src.get("dt", src.get("time_step", ""))

    u_over_t = src.get("u_over_t", src.get("U_over_t", ""))
    try:
        tf = float(t_value)
        uf = float(u_value)
        if tf != 0.0:
            u_over_t = uf / tf
    except Exception:
        # keep existing source value if already present
        if not u_over_t:
            u_over_t = ""

    # Historical runs may only contain U_over_t without explicit t/U columns.
    # Keep reconstruction explicit and deterministic for auditability.
    try:
        if (not t_value or str(t_value).strip().upper() == "NA") and u_over_t not in ("", None):
            t_value = "1.0"
        if (not u_value or str(u_value).strip().upper() == "NA") and u_over_t not in ("", None):
            u_value = str(float(u_over_t) * float(t_value))
    except Exception:
        pass

    model_id = src.get("model_id", "") or f"hubbard::{problem}"
    hamiltonian_id = src.get("hamiltonian_id", "") or "single_band_hubbard_2d"
    schema_version = src.get("schema_version", "") or "1.1"

    return {
        "problem": problem,
        "model_id": model_id,
        "hamiltonian_id": hamiltonian_id,
        "schema_version": schema_version,
        "lattice_size": lattice_size,
        "lattice_sites": parse_lattice_sites(lattice_size),
        "geometry": src.get("geometry", ""),
        "boundary_conditions": src.get("boundary_conditions", ""),
        "t": t_value,
        "U": u_value,
        "u_over_t": u_over_t,
        "mu": src.get("mu", src.get("doping", "")),
        "T": temp_value,
        "dt": dt_value,
        "time_step": dt_value,
        "seed": src.get("seed", "") or "module_seed_base:0xABC000+index",
        "solver_version": src.get("solver_version", "") or "hubbard_hts_research_cycle_v8_metadata",
        "method": src.get("method", src.get("integration_scheme", "")),
    }


def main():
    if len(sys.argv) != 2:
        print("Usage: post_run_metadata_capture.py <run_dir>", file=sys.stderr)
        return 2

    run_dir = Path(sys.argv[1]).resolve()
    baseline = run_dir / "logs" / "baseline_reanalysis_metrics.csv"
    if not baseline.exists():
        print(f"[metadata-capture] baseline missing: {baseline}", file=sys.stderr)
        return 3

    problems = read_problem_names(baseline)
    src_rows, src_path = read_module_metadata(run_dir)

    by_problem = {}
    for r in src_rows:
        key = (r.get("problem") or r.get("module") or "").strip()
        if key:
            by_problem[key] = r

    rows = []
    for p in problems:
        rows.append(normalize_row(p, by_problem.get(p, {})))

    csv_path = run_dir / "logs" / "model_metadata.csv"
    json_path = run_dir / "logs" / "model_metadata.json"

    csv_path.parent.mkdir(parents=True, exist_ok=True)
    with csv_path.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(
            f,
            fieldnames=[
                "problem", "model_id", "hamiltonian_id", "schema_version", "lattice_size", "lattice_sites",
                "geometry", "boundary_conditions", "t", "U", "u_over_t", "mu", "T", "dt", "time_step",
                "seed", "solver_version", "method"
            ],
        )
        w.writeheader()
        w.writerows(rows)

    first = rows[0] if rows else {}
    payload = {
        "lattice_size": first.get("lattice_size", ""),
        "geometry": first.get("geometry", ""),
        "boundary_conditions": first.get("boundary_conditions", ""),
        "t": first.get("t", ""),
        "U": first.get("U", ""),
        "mu": first.get("mu", ""),
        "T": first.get("T", ""),
        "dt": first.get("dt", ""),
        "method": first.get("method", ""),
        "model_id": first.get("model_id", ""),
        "hamiltonian_id": first.get("hamiltonian_id", ""),
        "schema_version": first.get("schema_version", ""),
        "u_over_t": first.get("u_over_t", ""),
        "time_step": first.get("time_step", ""),
        "seed": first.get("seed", ""),
        "solver_version": first.get("solver_version", ""),
        "per_problem": rows,
        "source": "post_run_metadata_capture_no_hardcoded_table",
        "source_metadata_file": str(src_path) if src_path else "",
    }
    json_path.write_text(json.dumps(payload, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    print(f"[metadata-capture] generated: {csv_path}")
    print(f"[metadata-capture] generated: {json_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
