#!/usr/bin/env python3
"""Validate CSV schema consistency (column count) for one run directory."""
from __future__ import annotations
import argparse
import csv
import json
from pathlib import Path


def check_csv(path: Path):
    with path.open(newline="", encoding="utf-8") as f:
        reader = csv.reader(f)
        try:
            header = next(reader)
        except StopIteration:
            return {
                "file": str(path),
                "status": "OBSERVED",
                "expected_columns": 0,
                "row": 0,
                "actual_columns": 0,
                "details": "empty csv",
            }
        expected = len(header)
        for row_idx, row in enumerate(reader, start=2):
            # Ignorer les lignes commentaires (commençant par '#') :
            # certains CSV scientifiques embarquent des métadonnées en fin de fichier
            # sous la forme "# key,value,key2,value2" — nombre de colonnes variable
            # par construction, donc ignorées du schéma guard.
            if row and row[0].startswith("#"):
                continue
            if len(row) != expected:
                return {
                    "file": str(path),
                    "status": "FAIL",
                    "expected_columns": expected,
                    "row": row_idx,
                    "actual_columns": len(row),
                    "details": "column_count_mismatch",
                }
    return {
        "file": str(path),
        "status": "PASS",
        "expected_columns": expected,
        "row": "",
        "actual_columns": expected,
        "details": "ok",
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("run_dir", help="Path to research_<stamp> run directory")
    args = ap.parse_args()

    run_dir = Path(args.run_dir).resolve()
    tests_dir = run_dir / "tests"
    logs_dir = run_dir / "logs"
    tests_dir.mkdir(parents=True, exist_ok=True)
    logs_dir.mkdir(parents=True, exist_ok=True)

    rows = []
    for p in sorted(run_dir.rglob("*.csv")):
        if p.name == "integration_csv_schema_guard.csv":
            continue
        # C22-BUG04 FIX : exclure les CSV du logger forensique LumVorax.
        # Ces fichiers contiennent des types de lignes mixtes par conception
        # (ANOMALY, METRIC, HW_SAMPLE, PHASE_BRIDGE...) avec des colonnes
        # différentes — un faux positif fatal pour le schema guard.
        if p.name.startswith("lumvorax_"):
            continue
        rows.append(check_csv(p))

    out_csv = tests_dir / "integration_csv_schema_guard.csv"
    with out_csv.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(
            f,
            fieldnames=["file", "status", "expected_columns", "row", "actual_columns", "details"],
        )
        w.writeheader()
        w.writerows(rows)

    summary = {
        "run_dir": str(run_dir),
        "csv_count": len(rows),
        "pass": sum(1 for r in rows if r["status"] == "PASS"),
        "observed": sum(1 for r in rows if r["status"] == "OBSERVED"),
        "fail": sum(1 for r in rows if r["status"] == "FAIL"),
        "failed_files": [r for r in rows if r["status"] == "FAIL"],
    }
    (logs_dir / "csv_schema_guard_summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")

    print(f"[csv-schema-guard] generated: {out_csv}")
    print(f"[csv-schema-guard] generated: {logs_dir / 'csv_schema_guard_summary.json'}")

    if summary["fail"] > 0:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
