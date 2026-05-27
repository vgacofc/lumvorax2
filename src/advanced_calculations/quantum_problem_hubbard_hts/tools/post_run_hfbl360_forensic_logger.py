#!/usr/bin/env python3
"""HFBL-360 forensic logger + memory-tracker traceability report.

Produces:
- logs/hfbl360_forensic_audit.json
- tests/integration_hfbl360_forensic_audit.csv

Purpose: verify (without placeholder data) whether requested forensic naming/
tracking hooks are present in STANDARD_NAMES.md and record runtime persistence
settings (env + filesystem capability) for reproducible auditing.
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import time
from datetime import datetime, timezone
from pathlib import Path

REQUIRED_STANDARD_NAMES = [
    "memory_tracker_enable",
    "memory_tracker_is_enabled",
    "memory_tracker_export_json",
    "memory_tracker_set_release_mode",
]

OPTIONAL_HFBL_KEYS = [
    "HFBL_360",
    "NX-11-HFBL-360",
    "forensic_research_chain_of_custody",
]


def read_text(path: Path) -> str:
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8", errors="replace")


def can_persist(path: Path) -> tuple[bool, str]:
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("a", encoding="utf-8") as fp:
            fp.write("hfbl360_persistence_probe=ok\n")
        return True, "writable"
    except OSError as exc:
        return False, f"not_writable:{exc}"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("run_dir", help="Path to research_* run directory")
    ap.add_argument("--standard-names", default="STANDARD_NAMES.md")
    ap.add_argument("--phase", default="runtime")
    args = ap.parse_args()
    phase = args.phase

    run_dir = Path(args.run_dir).resolve()
    tests_dir = run_dir / "tests"
    logs_dir = run_dir / "logs"
    tests_dir.mkdir(parents=True, exist_ok=True)
    logs_dir.mkdir(parents=True, exist_ok=True)

    std_path = Path(args.standard_names).resolve()
    std_text = read_text(std_path)

    required_rows = []
    for key in REQUIRED_STANDARD_NAMES:
        present = key in std_text
        required_rows.append(("standard_required", key, "present", "1" if present else "0", "PASS" if present else "FAIL"))

    hfbl_rows = []
    for key in OPTIONAL_HFBL_KEYS:
        present = key in std_text
        hfbl_rows.append(("standard_optional", key, "present", "1" if present else "0", "OBSERVED" if present else "MISSING"))

    env_rows = []
    env_keys = [
        "LUMVORAX_FORENSIC_REALTIME",
        "LUMVORAX_LOG_PERSISTENCE",
        "LUMVORAX_HFBL360_ENABLED",
        "LUMVORAX_MEMORY_TRACKER",
    ]
    for key in env_keys:
        value = os.environ.get(key, "")
        normalized = value.strip().lower()
        is_enabled = normalized in {"1", "true", "on", "yes", "enabled"}
        env_rows.append(("runtime_env", key, "value", value if value else "UNSET", "PASS" if is_enabled else "FAIL"))

    persistent_target = logs_dir / "hfbl360_realtime_persistent.log"
    persist_ok, persist_note = can_persist(persistent_target)
    if persist_ok:
        with persistent_target.open("a", encoding="utf-8") as rt:
            rt.write(f"ts_ns={time.time_ns()} event=hfbl360_forensic_logger_{phase} run_dir={run_dir}\n")
            for _, key, _, val, status in env_rows:
                rt.write(f"ts_ns={time.time_ns()} env_key={key} value={val} status={status}\n")
    persist_row = ("filesystem", "persistent_log_target", "writable", "1" if persist_ok else "0", "PASS" if persist_ok else "FAIL")

    csv_path = tests_dir / "integration_hfbl360_forensic_audit.csv"
    with csv_path.open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["family", "check_id", "metric", "value", "status"])
        for row in required_rows + hfbl_rows + env_rows + [persist_row]:
            w.writerow(row)

    summary = {
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "run_dir": str(run_dir),
        "phase": phase,
        "standard_names_path": str(std_path),
        "required": [
            {"name": name, "present": val == "1", "status": status}
            for _, name, _, val, status in required_rows
        ],
        "optional_hfbl": [
            {"name": name, "present": val == "1", "status": status}
            for _, name, _, val, status in hfbl_rows
        ],
        "env": [{"key": name, "value": val, "status": status} for _, name, _, val, status in env_rows],
        "persistence": {"target": str(persistent_target), "ok": persist_ok, "note": persist_note},
    }

    (logs_dir / "hfbl360_forensic_audit.json").write_text(json.dumps(summary, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(csv_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
