#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import re
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

DATE_PATTERNS = [
    re.compile(r"(20\d{2}-\d{2}-\d{2})"),
    re.compile(r"(20\d{6})"),
]


@dataclass
class ArtifactInfo:
    path: Path
    size: int
    sha256: str
    inferred_date: str


def sha256_of(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def infer_date(path: Path, text: str) -> str:
    candidates = [path.name, text[:2000]]
    for candidate in candidates:
        for pattern in DATE_PATTERNS:
            match = pattern.search(candidate)
            if match:
                value = match.group(1)
                if len(value) == 8:
                    return f"{value[0:4]}-{value[4:6]}-{value[6:8]}"
                return value
    return "unknown"


def collect_artifacts(base: Path) -> list[ArtifactInfo]:
    files = sorted(base.glob("**/*"))
    artifacts: list[ArtifactInfo] = []
    for file in files:
        if not file.is_file():
            continue
        text = ""
        if file.suffix.lower() in {".md", ".json", ".log", ".txt"}:
            text = file.read_text(encoding="utf-8", errors="ignore")
        artifacts.append(
            ArtifactInfo(
                path=file,
                size=file.stat().st_size,
                sha256=sha256_of(file),
                inferred_date=infer_date(file, text),
            )
        )
    return artifacts


def load_json(path: Path) -> Any:
    if not path.exists() or path.stat().st_size == 0:
        return None
    return json.loads(path.read_text(encoding="utf-8"))


def build_report(artifacts: list[ArtifactInfo], dataset_json: Any) -> str:
    now = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")
    latest_inferred = sorted({a.inferred_date for a in artifacts if a.inferred_date != "unknown"})
    latest_date = latest_inferred[-1] if latest_inferred else "unknown"

    empty_files = [a for a in artifacts if a.size == 0]

    lines = []
    lines.append("# Rapport d'audit — Dernière exécution Replit (forensic)")
    lines.append("")
    lines.append(f"- Horodatage de génération: **{now}**")
    lines.append(f"- Dernière date d'exécution inférée depuis les artefacts: **{latest_date}**")
    lines.append(f"- Nombre d'artefacts audités: **{len(artifacts)}**")
    lines.append("")

    lines.append("## 1) Synchronisation dépôt distant")
    lines.append("- Référence de synchronisation attendue: `origin/main`.")
    lines.append("- Vérification à exécuter: `git fetch origin --prune && git rev-parse HEAD origin/main`.")
    lines.append("")

    lines.append("## 2) Intégrité des données")
    if empty_files:
        lines.append("- Fichiers vides détectés (anomalie de complétude):")
        for f in empty_files:
            lines.append(f"  - `{f.path}` (0 octet)")
    else:
        lines.append("- Aucun fichier vide détecté.")

    lines.append("")
    lines.append("## 3) Résultats clés extraits")
    if dataset_json:
        status = dataset_json.get("status", "unknown")
        runtime_state = dataset_json.get("runtime_confirmation", {}).get("state", "unknown")
        blocking = dataset_json.get("runtime_confirmation", {}).get("blocking_errors", [])
        lines.append(f"- Statut dataset de dépendances: **{status}**")
        lines.append(f"- Statut confirmation runtime: **{runtime_state}**")
        lines.append(f"- Nombre d'erreurs bloquantes déclarées: **{len(blocking)}**")
        for item in blocking:
            lines.append(f"  - `{item}`")
    else:
        lines.append("- JSON de résultats dataset indisponible ou invalide.")

    lines.append("")
    lines.append("## 4) Analyse scientifique et hypothèses")
    lines.append("- Les observables physiques demandées (énergie, corrélations, pairing, sign ratio) ne sont pas présentes dans les artefacts Replit audités.")
    lines.append("- Hypothèse dominante: blocage infrastructure/dépendances (imagecodecs/LZW) avant l'étape d'évaluation scientifique complète.")
    lines.append("- Risque principal: confusion entre validation runtime notebook et validation native stricte C/H (stubs mentionnés dans le rapport 2026-02-21).")

    lines.append("")
    lines.append("## 5) Questions d'expert (format question/analyse/réponse/solution)")
    lines.append("1. **Question**: Les résultats Replit prouvent-ils la validité scientifique de la simulation ?")
    lines.append("   - **Analyse**: Les artefacts montrent une validation d'infrastructure, pas de séries d'observables physiques publiées.")
    lines.append("   - **Réponse**: **Partielle**.")
    lines.append("   - **Solution**: exécuter une matrice de runs instrumentés et exporter les métriques scientifiques brutes en JSON append-only.")
    lines.append("2. **Question**: La reproductibilité est-elle assurée ?")
    lines.append("   - **Analyse**: checksums possibles, mais un fichier de validation principal est vide.")
    lines.append("   - **Réponse**: **Incomplète**.")
    lines.append("   - **Solution**: imposer un manifeste signé (SHA256) + non-vacuité obligatoire des fichiers clés.")

    lines.append("")
    lines.append("## 6) Recommandations immédiates")
    lines.append("- Séparer strictement pipelines `runtime` vs `native_strict` avec indicateurs GO/NO-GO indépendants.")
    lines.append("- Ajouter un test de non-régression qui échoue si `validation_results.json` est vide.")
    lines.append("- Publier un lot de tests convergence/stabilité (température, taille, seed) avant conclusion physique.")

    lines.append("")
    lines.append("## 7) Commande reproductible")
    lines.append("```bash")
    lines.append("python3 scripts/replit_audit_latest_run.py --base RAPPORT-VESUVIUS/validation_lumvorax --output reports/replit_audit")
    lines.append("```")

    lines.append("")
    lines.append("## 8) Checksums (provenance)")
    for artifact in artifacts:
        rel = artifact.path.as_posix()
        lines.append(f"- `{artifact.sha256}`  `{rel}`")

    return "\n".join(lines) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(description="Audit forensic des derniers résultats Replit")
    parser.add_argument("--base", type=Path, default=Path("RAPPORT-VESUVIUS/validation_lumvorax"))
    parser.add_argument("--output", type=Path, default=Path("reports/replit_audit"))
    args = parser.parse_args()

    artifacts = collect_artifacts(args.base)
    dataset_json = load_json(args.base / "validation_results_dataset_v3_pending_20260219.json")

    args.output.mkdir(parents=True, exist_ok=True)
    ts = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")
    report_path = args.output / f"RAPPORT_AUDIT_REPLIT_DERNIERE_EXECUTION_{ts}.md"
    manifest_path = args.output / f"manifest_replit_audit_{ts}.json"

    report_text = build_report(artifacts, dataset_json)
    report_path.write_text(report_text, encoding="utf-8")

    manifest = {
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "base": str(args.base),
        "count": len(artifacts),
        "artifacts": [
            {
                "path": a.path.as_posix(),
                "size": a.size,
                "sha256": a.sha256,
                "inferred_date": a.inferred_date,
            }
            for a in artifacts
        ],
    }
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    print(report_path)
    print(manifest_path)


if __name__ == "__main__":
    main()
