#!/usr/bin/env python3
"""
BTC Runs Aggregator — Cycle C114

Scanne les répertoires `logs/forensic/sessions/` et `logs/forensic/modules/`
de tous les runs BTC (Replit + Ubuntu) et produit un CSV agrégé avec :

  run_id, ts_unix, hashrate_mhps, best_lz, best_nonce, duration_s,
  threads, mode, has_lum_log, has_mem_baseline, has_mem_final,
  reasoning_nodes, async_log_entries

Usage :
    python3 tools/btc_runs_aggregator.py
    python3 tools/btc_runs_aggregator.py --root logs/forensic --out runs.csv
"""
from __future__ import annotations
import argparse
import csv
import glob
import json
import os
import re
import sys
from typing import Dict, List, Optional


HASHRATE_RE = re.compile(r"Hashrate\s*:\s*([\d.]+)\s*MH/s", re.I)
BEST_LZ_RE = re.compile(r"Best leading\s*:\s*(\d+)\s*bits?", re.I)
BEST_NONCE_RE = re.compile(r"Best nonce\s*:\s*(\d+)", re.I)
DURATION_RE = re.compile(r"Dur(?:é|e)e\s*:\s*([\d.]+)\s*s", re.I)
THREADS_RE = re.compile(r"threads=(\d+)", re.I)
MODE_RE = re.compile(r"mode[:= ]+(\w+)", re.I)


MAX_PARSE_BYTES = 4 * 1024 * 1024  # 4 MiB max par fichier scanné


def parse_summary(path: str) -> Dict[str, Optional[str]]:
    out: Dict[str, Optional[str]] = {
        "hashrate_mhps": None, "best_lz": None, "best_nonce": None,
        "duration_s": None, "threads": None, "mode": None,
    }
    try:
        size = os.path.getsize(path)
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            if size > MAX_PARSE_BYTES:
                # On lit le début + la fin du fichier (logs structurés)
                head = f.read(MAX_PARSE_BYTES // 2)
                f.seek(max(0, size - MAX_PARSE_BYTES // 2))
                tail = f.read(MAX_PARSE_BYTES // 2)
                txt = head + "\n" + tail
            else:
                txt = f.read()
    except OSError:
        return out
    for key, regex in [
        ("hashrate_mhps", HASHRATE_RE),
        ("best_lz", BEST_LZ_RE),
        ("best_nonce", BEST_NONCE_RE),
        ("duration_s", DURATION_RE),
        ("threads", THREADS_RE),
        ("mode", MODE_RE),
    ]:
        m = regex.search(txt)
        if m:
            out[key] = m.group(1)
    return out


KNOWN_PREFIXES = (
    "btc_lum_log_",
    "btc_mem_baseline_",
    "btc_mem_final_",
    "btc_async_log_",
    "btc_qm_engine_forensic_",
    "btc_reasoning_trace_",
    "btc_engine_forensic_",
)


def list_run_ids(root: str) -> List[str]:
    """Détecte les run_id depuis les noms `btc_<kind>_<run_id>.<ext>` connus."""
    ids = set()
    for p in glob.glob(os.path.join(root, "modules", "btc_*")):
        name = os.path.basename(p)
        for prefix in KNOWN_PREFIXES:
            if name.startswith(prefix):
                rest = name[len(prefix):]
                rest = os.path.splitext(rest)[0]
                if rest:
                    ids.add(rest)
                break
    return sorted(ids)


def gather_run(root: str, run_id: str) -> Dict[str, object]:
    modules_dir = os.path.join(root, "modules")
    sessions_dir = os.path.join(root, "sessions")

    has_lum_log = bool(glob.glob(os.path.join(modules_dir, f"btc_lum_log_{run_id}.lum")))
    has_mem_baseline = bool(glob.glob(os.path.join(modules_dir, f"btc_mem_baseline_{run_id}.lum")))
    has_mem_final = bool(glob.glob(os.path.join(modules_dir, f"btc_mem_final_{run_id}.lum")))

    reasoning_nodes = 0
    rt = os.path.join(modules_dir, f"btc_reasoning_trace_{run_id}.json")
    if os.path.isfile(rt):
        try:
            with open(rt, "r", encoding="utf-8") as f:
                data = json.load(f)
            if isinstance(data, dict):
                reasoning_nodes = int(data.get("node_count", len(data.get("nodes", []))))
            elif isinstance(data, list):
                reasoning_nodes = len(data)
        except Exception:
            pass

    async_log_entries = 0
    al = os.path.join(modules_dir, f"btc_async_log_{run_id}.log")
    if os.path.isfile(al):
        try:
            with open(al, "rb") as f:
                async_log_entries = sum(1 for _ in f)
        except OSError:
            pass

    summary = {}
    candidates = (
        glob.glob(os.path.join(sessions_dir, f"console_{run_id}.log")) +
        glob.glob(os.path.join(sessions_dir, f"summary_*{run_id}*"))
    )
    for c in candidates:
        s = parse_summary(c)
        for k, v in s.items():
            if v is not None and not summary.get(k):
                summary[k] = v
    if not summary:
        engine = os.path.join(modules_dir, f"btc_qm_engine_forensic_{run_id}.log")
        if os.path.isfile(engine):
            summary = parse_summary(engine)

    ts_unix = None
    m = re.search(r"_(\d{10})$", run_id)
    if m:
        ts_unix = int(m.group(1))

    row = {
        "run_id": run_id,
        "ts_unix": ts_unix,
        "hashrate_mhps": summary.get("hashrate_mhps"),
        "best_lz": summary.get("best_lz"),
        "best_nonce": summary.get("best_nonce"),
        "duration_s": summary.get("duration_s"),
        "threads": summary.get("threads"),
        "mode": summary.get("mode"),
        "has_lum_log": int(has_lum_log),
        "has_mem_baseline": int(has_mem_baseline),
        "has_mem_final": int(has_mem_final),
        "reasoning_nodes": reasoning_nodes,
        "async_log_entries": async_log_entries,
    }
    return row


def main(argv: List[str]) -> int:
    ap = argparse.ArgumentParser()
    default_root = "src/advanced_calculations/bitcoin_quantum_mining/logs/forensic"
    ap.add_argument("--root", default=default_root,
                    help=f"racine logs/forensic (def: {default_root})")
    ap.add_argument("--out", default="btc_runs_aggregated.csv",
                    help="CSV de sortie")
    args = ap.parse_args(argv)

    if not os.path.isdir(args.root):
        print(f"[AGG] répertoire introuvable : {args.root}", file=sys.stderr)
        return 1

    run_ids = list_run_ids(args.root)
    print(f"[AGG] {len(run_ids)} run_id détectés sous {args.root}/modules/")

    rows = [gather_run(args.root, rid) for rid in run_ids]
    rows.sort(key=lambda r: (r.get("ts_unix") or 0, r["run_id"]))

    fieldnames = [
        "run_id", "ts_unix", "mode", "threads", "duration_s",
        "hashrate_mhps", "best_lz", "best_nonce",
        "reasoning_nodes", "async_log_entries",
        "has_lum_log", "has_mem_baseline", "has_mem_final",
    ]
    with open(args.out, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fieldnames)
        w.writeheader()
        for r in rows:
            w.writerow({k: r.get(k, "") for k in fieldnames})
    print(f"[AGG] CSV agrégé écrit → {args.out} ({len(rows)} lignes)")

    print()
    print(f"{'run_id':<40} {'mode':<10} {'MH/s':>7} {'lz':>4} {'nodes':>6}")
    print("-" * 80)
    for r in rows[-20:]:
        print(f"{r['run_id']:<40} {str(r.get('mode') or '?'):<10} "
              f"{str(r.get('hashrate_mhps') or '?'):>7} "
              f"{str(r.get('best_lz') or '?'):>4} "
              f"{str(r.get('reasoning_nodes') or 0):>6}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
