"""Build an inventory of C/C++ LUMVORAX modules and public functions."""

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCAN_DIRS = [
    ROOT / "src/lum",
    ROOT / "src/file_formats",
    ROOT / "src/vorax",
    ROOT / "src/logger",
    ROOT / "src/debug",
]

FUNC_RE = re.compile(r"^\s*([a-zA-Z_][\w\s\*]+?)\s+([a-zA-Z_]\w*)\s*\(([^;]*?)\)\s*;\s*$")


def module_role(path: Path) -> str:
    p = str(path)
    if "/src/lum/" in p:
        return "LUM core structures/security/memory"
    if "/src/file_formats/" in p:
        return "Universal .lum format encoding/decoding and file manager"
    if "/src/vorax/" in p:
        return "VORAX 3D operators/processing"
    if "/src/logger/" in p or "/src/debug/" in p:
        return "Forensic/logging/monitoring"
    return "Other"


def collect() -> dict:
    data = {"modules": [], "summary": {"headers": 0, "functions": 0}}
    for d in SCAN_DIRS:
        if not d.exists():
            continue
        for h in sorted(d.glob("*.h")):
            txt = h.read_text(encoding="utf-8", errors="replace").splitlines()
            funcs = []
            for ln in txt:
                m = FUNC_RE.match(ln)
                if m:
                    funcs.append({"return": m.group(1).strip(), "name": m.group(2), "args": m.group(3).strip()})
            data["modules"].append(
                {
                    "header": str(h.relative_to(ROOT)),
                    "role": module_role(h),
                    "function_count": len(funcs),
                    "functions": funcs,
                }
            )
            data["summary"]["headers"] += 1
            data["summary"]["functions"] += len(funcs)
    return data


def render_markdown(inv: dict, out_md: Path) -> None:
    lines = []
    lines.append("# Inventaire modules/fonctions LUMVORAX (local)\n\n")
    lines.append(f"- Headers scannés: **{inv['summary']['headers']}**\n")
    lines.append(f"- Fonctions publiques détectées: **{inv['summary']['functions']}**\n\n")
    for mod in inv["modules"]:
        lines.append(f"## {mod['header']}\n")
        lines.append(f"- Rôle: {mod['role']}\n")
        lines.append(f"- Fonctions: {mod['function_count']}\n")
        for f in mod["functions"]:
            lines.append(f"  - `{f['name']}`: `{f['return']}` ({f['args']})\n")
        lines.append("\n")
    out_md.write_text("".join(lines), encoding="utf-8")


def main() -> None:
    inv = collect()
    out_json = ROOT / "RAPPORT-VESUVIUS/validation_lumvorax/lumvorax_module_inventory_20260220.json"
    out_md = ROOT / "RAPPORT-VESUVIUS/validation_lumvorax/RAPPORT_INVENTAIRE_MODULES_FONCTIONS_LUMVORAX_20260220.md"
    out_json.write_text(json.dumps(inv, indent=2), encoding="utf-8")
    render_markdown(inv, out_md)
    print(out_json)
    print(out_md)


if __name__ == "__main__":
    main()
