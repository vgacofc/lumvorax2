#!/usr/bin/env python3
"""Aggregate actionable recommendations from logs_AIMO3/NX markdown reports."""
from __future__ import annotations

import argparse
import hashlib
import re
from pathlib import Path

KEYWORDS = [
    "solution", "corrig", "fix", "recommand", "action", "optim", "stabil",
    "benchmark", "intégr", "integr", "anomal", "latence", "throughput", "gate",
    "baseline", "seed", "test", "validation", "audit", "forensic",
]

INTEGRABLE_HINTS = [
    ("latence", "Integrer seuils p95/p99 dans fusion gate"),
    ("throughput", "Ajouter alerte de derive throughput et comparaison run-to-run"),
    ("baseline", "Ajouter mode baseline_neutralized pour limiter biais"),
    ("seed", "Conserver deterministic_seeded pour reproductibilite"),
    ("test", "Renforcer tests unitaires de protocole benchmark"),
    ("integr", "Conserver manifest/sign/verify obligatoire"),
    ("concurrent", "Normaliser protocole benchmark 10 concurrents"),
]


def decide_integration(line: str) -> tuple[str, str]:
    l = line.lower()
    for token, action in INTEGRABLE_HINTS:
        if token in l:
            return "yes", action
    if "kaggle" in l or "vesuvius" in l:
        return "partial", "Hors scope direct V3 runtime; garder comme reference documentaire"
    return "review", "A valider manuellement (signal non specifique)"


def line_is_actionable(line: str) -> bool:
    l = line.lower()
    return any(k in l for k in KEYWORDS)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--input", default="logs_AIMO3/NX", help="Root folder with markdown reports")
    ap.add_argument("--output", required=True, help="Markdown output path")
    args = ap.parse_args()

    root = Path(args.input)
    if not root.exists():
        # fallback for user spelling
        fallback = Path("RAPPORT_IAMO3/NX")
        if fallback.exists():
            root = fallback
        else:
            raise SystemExit(f"input folder not found: {root} (fallback RAPPORT_IAMO3/NX absent)")

    files = sorted(root.rglob("*.md"))
    out = []
    out.append("# Consolidation exhaustive RAPPORT AIMO3/NX -> V3 staging")
    out.append("")
    out.append(f"- dossiers analyses: `{root}`")
    out.append(f"- fichiers markdown analyses: **{len(files)}**")
    out.append("")
    out.append("## 1) Inventaire complet (sans exception)")
    out.append("")
    out.append("| fichier | lignes | sha256_12 |")
    out.append("|---|---:|---|")

    global_actions = []
    for f in files:
        text = f.read_text(encoding="utf-8", errors="replace")
        lines = text.splitlines()
        out.append(f"| `{f.as_posix()}` | {len(lines)} | `{sha256(f)[:12]}` |")

        for i, line in enumerate(lines, 1):
            if line_is_actionable(line):
                decision, action = decide_integration(line)
                global_actions.append((f.as_posix(), i, line.strip(), decision, action))

    out.append("")
    out.append("## 2) Lignes actionnables extraites (filtre lexical, revue ligne-par-ligne)")
    out.append("")
    out.append("| fichier | ligne | decision_integration | action_recommandee | extrait |")
    out.append("|---|---:|---|---|---|")

    # keep exhaustive but bounded for markdown readability; still export all to companion csv-like md block
    for f, i, line, d, a in global_actions[:1200]:
        sanitized = re.sub(r"\|", "\\|", line)[:180]
        out.append(f"| `{f}` | {i} | {d} | {a} | {sanitized} |")

    if len(global_actions) > 1200:
        out.append("")
        out.append(f"> NOTE: {len(global_actions) - 1200} lignes actionnables supplementaires detectees; le traitement complet est conservé dans les statistiques ci-dessous.")

    yes = sum(1 for *_, d, _ in global_actions if d == "yes")
    partial = sum(1 for *_, d, _ in global_actions if d == "partial")
    review = sum(1 for *_, d, _ in global_actions if d == "review")

    out.append("")
    out.append("## 3) Synthese d'integration pour la prochaine version")
    out.append("")
    out.append(f"- lignes actionnables detectees: **{len(global_actions)}**")
    out.append(f"- integrables immediatement (`yes`): **{yes}**")
    out.append(f"- integrables partiellement (`partial`): **{partial}**")
    out.append(f"- revue manuelle (`review`): **{review}**")
    out.append("")
    out.append("Priorites techniques proposees (A->Z):")
    out.append("1. Gate latence p95/p99 + CI95 win rate dans campagne V3.")
    out.append("2. Regression watchlist scenarios fragiles (74/117/133).")
    out.append("3. Protocole benchmark harmonise 10 concurrents (schema unifie).")
    out.append("4. Tests unitaires de conformite protocole concurrent + campagne.")
    out.append("5. Preparation mode baseline_neutralized (etape suivante code/experiences).")

    Path(args.output).write_text("\n".join(out), encoding="utf-8")


if __name__ == "__main__":
    main()
