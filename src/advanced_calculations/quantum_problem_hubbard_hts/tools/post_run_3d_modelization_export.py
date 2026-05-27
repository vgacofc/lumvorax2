#!/usr/bin/env python3
"""Export interactive 3D modelization artifacts for a run.

Inputs are raw CSV observables and outputs are:
- reports/3d/modelization_3d_dataset.json
- reports/3d/modelization_3d_view.html
"""

from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path


def read_csv(path: Path) -> list[dict[str, str]]:
    if not path.exists() or path.stat().st_size == 0:
        return []
    with path.open("r", newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def safe_float(v: str, default: float = 0.0) -> float:
    try:
        return float(v)
    except Exception:
        return default


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("run_dir")
    args = ap.parse_args()

    run_dir = Path(args.run_dir).resolve()
    logs = run_dir / "logs"
    out_dir = run_dir / "reports" / "3d"
    out_dir.mkdir(parents=True, exist_ok=True)

    base = read_csv(logs / "baseline_reanalysis_metrics.csv")
    if not base:
        raise RuntimeError(f"Missing or empty baseline_reanalysis_metrics.csv in {logs}")

    points = []
    for idx, row in enumerate(base):
        points.append(
            {
                "x_step": idx,
                "y_module": row.get("problem", "unknown"),
                "z_energy": safe_float(row.get("energy")),
                "pairing": safe_float(row.get("pairing")),
                "sign_ratio": safe_float(row.get("sign_ratio")),
                "elapsed_ns": safe_float(row.get("elapsed_ns")),
            }
        )

    dataset = {
        "run": run_dir.name,
        "points": points,
        "description": "3D scatter: x=sample index, y=module, z=energy, color=pairing",
    }
    json_path = out_dir / "modelization_3d_dataset.json"
    json_path.write_text(json.dumps(dataset, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    html = f"""<!doctype html>
<html lang=\"fr\">
<head>
  <meta charset=\"utf-8\" />
  <title>Modelisation 3D - {run_dir.name}</title>
  <script src=\"https://cdn.plot.ly/plotly-2.35.2.min.js\"></script>
  <style>body{{font-family:Arial,sans-serif;margin:0}}#plot{{width:100vw;height:92vh}}</style>
</head>
<body>
<h3 style=\"padding:10px 12px;margin:0\">Modelisation 3D (run {run_dir.name})</h3>
<div id=\"plot\"></div>
<script>
fetch('modelization_3d_dataset.json').then(r => r.json()).then(data => {{
  const modules = [...new Set(data.points.map(p => p.y_module))];
  const yMap = new Map(modules.map((m, i) => [m, i]));
  const trace = {{
    type: 'scatter3d',
    mode: 'markers',
    x: data.points.map(p => p.x_step),
    y: data.points.map(p => yMap.get(p.y_module)),
    z: data.points.map(p => p.z_energy),
    marker: {{
      size: 4,
      color: data.points.map(p => p.pairing),
      colorscale: 'Turbo',
      colorbar: {{title: 'pairing'}},
      opacity: 0.85
    }},
    text: data.points.map(p => `${{p.y_module}}<br>energy=${{p.z_energy.toFixed(6)}}<br>pairing=${{p.pairing.toFixed(6)}}<br>sign=${{p.sign_ratio.toFixed(6)}}`),
    hoverinfo: 'text'
  }};
  const layout = {{
    scene: {{
      xaxis: {{title: 'sample index'}},
      yaxis: {{title: 'module index', tickvals: modules.map((_,i)=>i), ticktext: modules}},
      zaxis: {{title: 'energy'}}
    }},
    margin: {{l: 0, r: 0, b: 0, t: 30}}
  }};
  Plotly.newPlot('plot', [trace], layout, {{responsive: true}});
}});
</script>
</body>
</html>
"""
    (out_dir / "modelization_3d_view.html").write_text(html, encoding="utf-8")
    print(out_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
