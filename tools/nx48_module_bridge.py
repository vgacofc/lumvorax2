#!/usr/bin/env python3
import argparse
import hashlib
import json
from pathlib import Path


EXTENSIONS = {".c", ".h", ".py", ".lean", ".cpp", ".hpp", ".rs", ".js", ".ts"}
EXCLUDED_PARTS = {"__pycache__", ".git", "CHAT", "logs", "RAPPORTS", "results"}


def classify(path):
    text = str(path).lower()
    if "bitcoin_quantum_mining" in text:
        return "btc_native"
    if "nx_versions" in text or "/nx/" in text:
        return "nx_reasoning"
    if "quantum_problem_hubbard_hts" in text:
        return "hubbard_hts"
    if "optimization" in text:
        return "optimization"
    if "debug" in text or "forensic" in text:
        return "forensic"
    if "visualization" in text:
        return "visualization"
    return "lumvorax_src"


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def build_manifest(src_root):
    src_root = Path(src_root).resolve()
    files = []
    for path in src_root.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in EXTENSIONS:
            continue
        rel = path.relative_to(src_root)
        parts = set(rel.parts)
        if parts & EXCLUDED_PARTS:
            continue
        files.append(path)
    modules = []
    for index, path in enumerate(sorted(files), start=1):
        rel = path.relative_to(src_root)
        modules.append({
            "id": f"btc_src_module_{index:03d}",
            "path": str(rel),
            "kind": classify(rel),
            "sha256": digest(path),
            "bytes": path.stat().st_size,
        })
    kind_counts = {}
    total_bytes = 0
    for module in modules:
        kind_counts[module["kind"]] = kind_counts.get(module["kind"], 0) + 1
        total_bytes += module["bytes"]
    return {
        "schema": "lumvorax_btc_module_bridge_v1",
        "source_root": str(src_root),
        "module_count": len(modules),
        "kind_counts": kind_counts,
        "total_bytes": total_bytes,
        "btc_nx48_usage": {
            "mode": "advisory_manifest_for_forensic_and_parameter_context",
            "hot_path": False,
            "reason": "les sources sont cartographiées et hashées; elles ne sont pas toutes exécutées dans la boucle SHA-256 afin de préserver le débit"
        },
        "modules": modules,
    }


def main():
    parser = argparse.ArgumentParser(description="LumVorax BTC module bridge manifest")
    parser.add_argument("--src-root", default="src")
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    manifest = build_manifest(args.src_root)
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(manifest, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"manifest={output} modules={manifest['module_count']}")


if __name__ == "__main__":
    main()