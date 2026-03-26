#!/usr/bin/env python3
"""Analyse comparative de notebooks/scripts Vesuvius et rapport des écarts clés."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from statistics import mean
from typing import Dict, List

PATTERNS = {
    "T_low": re.compile(r"\bT_low\s*=\s*([0-9]*\.?[0-9]+)"),
    "T_high": re.compile(r"\bT_high\s*=\s*([0-9]*\.?[0-9]+)"),
    "overlap_public": re.compile(r"\boverlap_public\s*=\s*([0-9]*\.?[0-9]+)"),
    "overlap_base": re.compile(r"\boverlap_base\s*=\s*([0-9]*\.?[0-9]+)"),
    "overlap_hi": re.compile(r"\boverlap_hi\s*=\s*([0-9]*\.?[0-9]+)"),
    "z_radius": re.compile(r"\bz_radius\s*=\s*([0-9]+)"),
    "xy_radius": re.compile(r"\bxy_radius\s*=\s*([0-9]+)"),
    "dust_min_size": re.compile(r"\bdust_min_size\s*=\s*([0-9]+)"),
}


def notebook_source(path: Path) -> str:
    if path.suffix == ".ipynb":
        data = json.loads(path.read_text(encoding="utf-8"))
        chunks: List[str] = []
        for cell in data.get("cells", []):
            if cell.get("cell_type") == "code":
                src = cell.get("source", [])
                chunks.append("".join(src) if isinstance(src, list) else str(src))
        return "\n".join(chunks)
    return path.read_text(encoding="utf-8", errors="ignore")


def extract_features(text: str) -> Dict[str, float]:
    out: Dict[str, float] = {}
    for name, pattern in PATTERNS.items():
        m = pattern.search(text)
        if m:
            out[name] = float(m.group(1))

    out["uses_tta"] = 1.0 if re.search(r"\bUSE_TTA\b\s*=\s*True", text) else 0.0
    out["uses_hysteresis"] = 1.0 if "binary_propagation" in text else 0.0
    out["uses_public_anchor"] = 1.0 if "pub_fg" in text or ("public" in text and "weak" in text) else 0.0
    out["uses_multiclass_logit"] = 1.0 if "logsumexp2" in text or "binary_logit_from_multiclass_logits" in text else 0.0
    return out


def summarize(values: List[float]) -> Dict[str, float]:
    return {"count": len(values), "min": min(values), "max": max(values), "mean": mean(values)}


def build_key_findings(aggregate: Dict[str, Dict[str, float]], baseline: Dict[str, float] | None) -> List[str]:
    findings: List[str] = []

    def mv(key: str, default: float = 0.0) -> float:
        return float(aggregate.get(key, {}).get("mean", default))

    findings.append(
        f"Seuils centraux concurrents: T_low≈{mv('T_low'):.3f}, T_high≈{mv('T_high'):.3f}; "
        "les runs forts utilisent souvent une hysteresis à deux seuils."
    )
    findings.append(
        f"Overlaps moyens: public≈{mv('overlap_public'):.3f}, base≈{mv('overlap_base'):.3f}, hi≈{mv('overlap_hi'):.3f}; "
        "les écarts d’overlap modifient fortement la stabilité slice-wise."
    )
    findings.append(
        f"Usage TTA={mv('uses_tta'):.2%}, hysteresis={mv('uses_hysteresis'):.2%}, public-anchor={mv('uses_public_anchor'):.2%}."
    )

    if baseline:
        delta_tlow = mv("T_low") - float(baseline.get("T_low", mv("T_low")))
        delta_thigh = mv("T_high") - float(baseline.get("T_high", mv("T_high")))
        findings.append(
            f"Delta vs baseline: ΔT_low={delta_tlow:+.4f}, ΔT_high={delta_thigh:+.4f}; "
            "cet écart peut expliquer une partie du gap 35-38 vs 58+ via precision/recall imbalance."
        )

    findings.append(
        "Hypothèse principale du gap score: combinaison calibration de seuils + ancrage public + stratégie overlap/TTA, "
        "plus impact de post-traitement topologique (closing/dust)."
    )
    return findings


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("folder", type=Path, help="Dossier contenant notebooks/scripts concurrents")
    parser.add_argument("--baseline", type=Path, default=None, help="Script baseline à comparer")
    parser.add_argument("--output", type=Path, default=Path("notebook_diff_report.json"))
    parser.add_argument("--top-n", type=int, default=100, help="Nombre max de fichiers à analyser (0=all)")
    args = parser.parse_args()

    files = sorted([p for p in args.folder.rglob("*") if p.suffix in {".ipynb", ".py"}])
    if args.top_n > 0:
        files = files[: args.top_n]

    report: Dict[str, object] = {
        "folder": str(args.folder),
        "file_count": len(files),
        "files": {},
        "aggregate": {},
    }

    aggregate: Dict[str, List[float]] = {}
    for path in files:
        feats = extract_features(notebook_source(path))
        report["files"][str(path)] = feats
        for k, v in feats.items():
            aggregate.setdefault(k, []).append(v)

    agg = {k: summarize(v) for k, v in aggregate.items() if v}
    report["aggregate"] = agg

    baseline_feats = None
    if args.baseline and args.baseline.exists():
        baseline_feats = extract_features(notebook_source(args.baseline))
        report["baseline"] = baseline_feats
        report["baseline_mean_delta"] = {
            k: round(agg[k]["mean"] - baseline_feats[k], 6)
            for k in agg
            if k in baseline_feats
        }

    report["key_findings"] = build_key_findings(agg, baseline_feats)

    args.output.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(f"Report written: {args.output} | files={len(files)}")


if __name__ == "__main__":
    main()
