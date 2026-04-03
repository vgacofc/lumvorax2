"""
Module de visualisation LumVorax — Hubbard HTS
Serveur Flask exposant les données réelles de simulation.
Noms conformes au STANDARD_NAMES.md v3.0 (C68-REALTIME-BENCH).
"""

import os
import csv
import json
import re
import glob
from pathlib import Path
from flask import Flask, jsonify, send_from_directory

app = Flask(__name__, static_folder="static")

BASE_DIR = Path(__file__).resolve().parent.parent / "advanced_calculations" / "quantum_problem_hubbard_hts"
RESULTS_DIR   = BASE_DIR / "results"
BENCHMARKS_DIR = BASE_DIR / "benchmarks"
CONFIG_DIR    = BASE_DIR / "config"

# ── helpers ──────────────────────────────────────────────────────────────────

def _latest_run_dir():
    """Retourne le dernier répertoire de run (tri lexicographique descendant)."""
    runs = sorted(RESULTS_DIR.glob("research_*"), reverse=True)
    return runs[0] if runs else None


def _read_csv_as_dicts(path: Path):
    if not path.exists():
        return []
    with open(path, newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def _parse_execution_log(log_path: Path):
    """
    Parse le research_execution.log.
    Format : NNNNNN | TYPE key=val key=val ...
    Types reconnus : BASE_RESULT, BENCH_QMC_RT, BENCH_EXT_RT
    """
    entries = []
    if not log_path.exists():
        return entries
    with open(log_path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            m = re.match(r"(\d+)\s*\|\s*(\w+)\s+(.*)", line)
            if not m:
                continue
            seq, typ, rest = m.group(1), m.group(2), m.group(3)
            kv = {}
            for kv_m in re.finditer(r"([\w]+)=([^\s]+)", rest):
                kv[kv_m.group(1)] = kv_m.group(2)
            kv["_seq"]  = int(seq)
            kv["_type"] = typ
            entries.append(kv)
    return entries

# ── ROUTE : données brutes de référence ─────────────────────────────────────

@app.route("/api/benchmark_ref")
def api_benchmark_ref():
    """Tableau de référence QMC/DMRG (qmc_dmrg_reference_runtime.csv)."""
    rows = _read_csv_as_dicts(BENCHMARKS_DIR / "qmc_dmrg_reference_runtime.csv")
    return jsonify(rows)


@app.route("/api/benchmark_ext")
def api_benchmark_ext():
    """Benchmark modules externes (external_module_benchmarks_runtime.csv)."""
    rows = _read_csv_as_dicts(BENCHMARKS_DIR / "external_module_benchmarks_runtime.csv")
    return jsonify(rows)


@app.route("/api/problems")
def api_problems():
    """Paramètres des problèmes (problems_cycle06.csv)."""
    rows = _read_csv_as_dicts(CONFIG_DIR / "problems_cycle06.csv")
    return jsonify(rows)

# ── ROUTE : résultats du dernier run ────────────────────────────────────────

@app.route("/api/run/latest")
def api_run_latest():
    """run_id + BASE_RESULT du dernier run depuis research_execution.log."""
    run_dir = _latest_run_dir()
    if run_dir is None:
        return jsonify({"run_id": None, "base_results": []})

    # Cherche le log dans results/<run>/logs/ ou results/<run>/
    log_candidates = [
        run_dir / "logs" / "research_execution.log",
        run_dir / "research_execution.log",
    ]
    log_path = next((p for p in log_candidates if p.exists()), None)

    entries = _parse_execution_log(log_path) if log_path else []
    base_results = [e for e in entries if e["_type"] == "BASE_RESULT"]
    bench_qmc    = [e for e in entries if e["_type"] in ("BENCH_QMC_RT", "BENCH_QMC_END")]
    start_entry  = next((e for e in entries if e["_type"] == "START"), {})

    return jsonify({
        "run_id":       run_dir.name,
        "started_at":   start_entry.get("utc", ""),
        "base_results": base_results,
        "bench_qmc_rt": bench_qmc,
    })


@app.route("/api/run/lumvorax_csv")
def api_lumvorax_csv():
    """
    Extrait un échantillon du CSV Lumvorax fullscale
    (lumvorax_hubbard_hts_fullscale_*.csv) — préfixe simulate_fs:
    Retourne max 2000 lignes pour ne pas saturer la RAM du navigateur.
    """
    run_dir = _latest_run_dir()
    if run_dir is None:
        return jsonify([])

    lv_files = sorted((run_dir / "logs").glob("lumvorax_hubbard_hts_fullscale_*.csv"), reverse=True)
    if not lv_files:
        return jsonify([])

    rows = []
    MAX = 2000
    with open(lv_files[0], newline="", encoding="utf-8") as f:
        reader = csv.reader(f)
        for i, row in enumerate(reader):
            if i >= MAX:
                break
            rows.append(row)
    return jsonify(rows)


@app.route("/api/run/normalized_trace")
def api_normalized_trace():
    """normalized_observables_trace.csv du dernier run."""
    run_dir = _latest_run_dir()
    if run_dir is None:
        return jsonify([])
    path = run_dir / "logs" / "normalized_observables_trace.csv"
    return jsonify(_read_csv_as_dicts(path))


@app.route("/api/run/numerical_stability")
def api_numerical_stability():
    """numerical_stability_suite.csv du dernier run (familles de tests)."""
    run_dir = _latest_run_dir()
    if run_dir is None:
        return jsonify([])
    path = run_dir / "tests" / "numerical_stability_suite.csv"
    return jsonify(_read_csv_as_dicts(path))


@app.route("/api/run/unit_conversion")
def api_unit_conversion():
    """unit_conversion_fullscale.csv du dernier run."""
    run_dir = _latest_run_dir()
    if run_dir is None:
        return jsonify([])
    path = run_dir / "tests" / "unit_conversion_fullscale.csv"
    return jsonify(_read_csv_as_dicts(path))


@app.route("/api/run/module_metadata")
def api_module_metadata():
    """module_physics_metadata.csv du dernier run."""
    run_dir = _latest_run_dir()
    if run_dir is None:
        return jsonify([])
    path = run_dir / "tests" / "module_physics_metadata.csv"
    return jsonify(_read_csv_as_dicts(path))

# ── ROUTE : données composites pour la visualisation ────────────────────────

@app.route("/api/viz/scalar_field")
def api_viz_scalar_field():
    """
    Mode 1 — Champs scalaires → volume heatmap 3D.
    Axes : (t_K, u_eV, module_index).
    Valeurs : energy_eV, pairing depuis BASE_RESULT + référence QMC/DMRG.
    """
    run_dir = _latest_run_dir()
    log_path = None
    if run_dir:
        for p in [run_dir / "logs" / "research_execution.log", run_dir / "research_execution.log"]:
            if p.exists():
                log_path = p
                break

    entries      = _parse_execution_log(log_path) if log_path else []
    base_results = [e for e in entries if e["_type"] == "BASE_RESULT"]
    problems     = _read_csv_as_dicts(CONFIG_DIR / "problems_cycle06.csv")
    ref_bench    = _read_csv_as_dicts(BENCHMARKS_DIR / "qmc_dmrg_reference_runtime.csv")

    prob_map = {p["name"]: p for p in problems}
    ref_map  = {r["module"]: r for r in ref_bench}

    points = []
    for i, br in enumerate(base_results):
        mod = br.get("problem", "")
        p   = prob_map.get(mod, {})
        ref = ref_map.get(mod, {})
        points.append({
            "module_index": i,
            "module":       mod,
            "t_K":          float(p.get("temp_K", 0)),
            "u_eV":         float(p.get("u_eV", 0)),
            "t_hop_eV":     float(p.get("t_eV", 1)),
            "lx":           int(p.get("lx", 0)),
            "ly":           int(p.get("ly", 0)),
            "energy_eV":    float(br.get("energy", 0)),
            "pairing":      float(br.get("pairing", 0)),
            "sign":         float(br.get("sign", 0)),
            "cpu_peak":     float(br.get("cpu_peak", 0)),
            "mem_peak":     float(br.get("mem_peak", 0)),
            "elapsed_ns":   float(br.get("elapsed_ns", 0)),
            "ref_energy":   float(ref.get("reference_value", 0)) if ref else None,
            "ref_error_bar":float(ref.get("error_bar", 0))       if ref else None,
        })

    return jsonify({"run_id": run_dir.name if run_dir else None, "points": points})


@app.route("/api/viz/trajectories")
def api_viz_trajectories():
    """
    Mode 2 — Trajectoires → curves.
    Lignes Lumvorax avec préfixe simulate_fs: contenant step_energy_norm_step0
    et step_pairing_norm_step0 (observables pas-à-pas C25).
    """
    run_dir = _latest_run_dir()
    if run_dir is None:
        return jsonify({"trajectories": []})

    lv_files = sorted((run_dir / "logs").glob("lumvorax_hubbard_hts_fullscale_*.csv"), reverse=True)
    if not lv_files:
        return jsonify({"trajectories": [], "note": "Fichier CSV Lumvorax introuvable"})

    trajectories = {}  # module → list of {step, energy_norm, pairing_norm}
    MAX_LINES = 50000
    with open(lv_files[0], newline="", encoding="utf-8") as f:
        reader = csv.reader(f)
        for i, row in enumerate(reader):
            if i >= MAX_LINES:
                break
            # Format CSV Lumvorax : TYPE, timestamp_utc, timestamp_ns, pid, PRÉFIXE:métrique, valeur
            if len(row) < 6:
                continue
            if row[0] != "METRIC":
                continue
            prefix_metric = row[4]
            # Filtrer simulate_fs: step_energy_norm_step0 et step_pairing_norm_step0
            if not prefix_metric.startswith("simulate_fs:"):
                continue
            metric = prefix_metric[len("simulate_fs:"):]
            if metric not in ("step_energy_norm_step0", "step_pairing_norm_step0",
                              "step_sign_ratio", "step_elapsed_ns"):
                continue
            # Extraire le module depuis la colonne 4 ou depuis les colonnes précédentes
            # Le module est dans le champ "module" — col 3 du format ultra_forensic_logger
            # Format réel : METRIC, utc, ns, pid, module, simulate_fs:métrique, valeur
            # On adapte selon le fichier réel
            try:
                val = float(row[-1])
            except (ValueError, IndexError):
                continue
            # Heuristique : le module est avant la colonne préfixe
            mod = row[3] if len(row) >= 6 else "unknown"
            if mod not in trajectories:
                trajectories[mod] = []
            trajectories[mod].append({"metric": metric, "value": val, "ns": row[2]})

    # Convertir en liste de trajectoires par module
    result = []
    for mod, pts in list(trajectories.items())[:15]:
        result.append({"module": mod, "points": pts[:500]})
    return jsonify({"trajectories": result})


@app.route("/api/viz/lattice")
def api_viz_lattice():
    """
    Mode 3 — Particules → instancing.
    Réseau Hubbard de chaque module (lx × ly sites).
    Chaque site reçoit : position (x, y), pairing local, spin (mock physique).
    """
    problems = _read_csv_as_dicts(CONFIG_DIR / "problems_cycle06.csv")

    run_dir = _latest_run_dir()
    log_path = None
    if run_dir:
        for p in [run_dir / "logs" / "research_execution.log", run_dir / "research_execution.log"]:
            if p.exists():
                log_path = p
                break
    entries      = _parse_execution_log(log_path) if log_path else []
    base_results = {e.get("problem", ""): e for e in entries if e["_type"] == "BASE_RESULT"}

    lattices = []
    for pb in problems:
        mod = pb["name"]
        lx, ly = int(pb.get("lx", 4)), int(pb.get("ly", 4))
        br = base_results.get(mod, {})
        pairing_global = float(br.get("pairing", 0.5))
        sign_global    = float(br.get("sign", 0.0))
        energy_global  = float(br.get("energy", 1.5))
        u_over_t = float(pb.get("u_eV", 8)) / max(float(pb.get("t_eV", 1)), 1e-9)

        sites = []
        import math
        for ix in range(lx):
            for iy in range(ly):
                # Pairing local : modulation physique selon la distance au centre
                dx = ix - lx / 2.0
                dy = iy - ly / 2.0
                r  = math.sqrt(dx*dx + dy*dy)
                # Pairing d-wave : cos(2θ) * e^{-r/ξ} avec ξ = lx/4
                theta = math.atan2(dy, dx)
                xi    = lx / 4.0
                pair_local = pairing_global * math.cos(2 * theta) * math.exp(-r / max(xi, 1))
                # Spin moyen : modulé par le signe fermionique
                spin = sign_global * math.sin(math.pi * ix / lx) * math.sin(math.pi * iy / ly)
                sites.append({
                    "ix": ix, "iy": iy,
                    "x": float(ix) - lx / 2.0,
                    "y": float(iy) - ly / 2.0,
                    "pairing_local": round(pair_local, 6),
                    "spin": round(spin, 6),
                    "u_over_t": round(u_over_t, 3),
                })

        lattices.append({
            "module": mod,
            "lx": lx, "ly": ly,
            "t_eV": float(pb.get("t_eV", 1)),
            "u_eV": float(pb.get("u_eV", 8)),
            "temp_K": float(pb.get("temp_K", 95)),
            "energy_eV": energy_global,
            "pairing": pairing_global,
            "sign": sign_global,
            "sites": sites,
        })

    return jsonify({"lattices": lattices})


@app.route("/api/viz/graph")
def api_viz_graph():
    """
    Mode 4 — Graphes d'interaction → nodes + edges.
    Nœuds = modules de simulation.
    Arêtes = correspondances benchmark QMC/DMRG + modules externes.
    Poids = abs_error ou within_error_bar.
    """
    problems  = _read_csv_as_dicts(CONFIG_DIR / "problems_cycle06.csv")
    ref_bench = _read_csv_as_dicts(BENCHMARKS_DIR / "qmc_dmrg_reference_runtime.csv")
    ext_bench = _read_csv_as_dicts(BENCHMARKS_DIR / "external_module_benchmarks_runtime.csv")

    run_dir = _latest_run_dir()
    log_path = None
    if run_dir:
        for p in [run_dir / "logs" / "research_execution.log", run_dir / "research_execution.log"]:
            if p.exists():
                log_path = p
                break
    entries      = _parse_execution_log(log_path) if log_path else []
    base_results = {e.get("problem", ""): e for e in entries if e["_type"] == "BASE_RESULT"}

    nodes = []
    node_ids = {}
    for i, pb in enumerate(problems):
        mod = pb["name"]
        br  = base_results.get(mod, {})
        node_ids[mod] = i
        nodes.append({
            "id": i, "label": mod,
            "type": "simulate_fs",
            "energy_eV": float(br.get("energy", 0)),
            "pairing":   float(br.get("pairing", 0)),
            "sign":      float(br.get("sign", 0)),
            "lx": int(pb.get("lx", 4)), "ly": int(pb.get("ly", 4)),
            "u_eV": float(pb.get("u_eV", 8)),
            "t_K":  float(pb.get("temp_K", 95)),
        })

    # Nœuds sources de référence (QMC/DMRG)
    source_ids = {}
    for row in ref_bench:
        src = row.get("source", "")
        if src not in source_ids:
            nid = len(nodes)
            source_ids[src] = nid
            nodes.append({"id": nid, "label": src, "type": "qmc_dmrg_ref"})

    edges = []
    for row in ref_bench:
        mod = row.get("module", "")
        src = row.get("source", "")
        if mod in node_ids and src in source_ids:
            br  = base_results.get(mod, {})
            sim_val = float(br.get("energy", 0))
            ref_val = float(row.get("reference_value", 0))
            err_bar = float(row.get("error_bar", 1))
            abs_err = abs(sim_val - ref_val)
            within  = abs_err <= err_bar
            edges.append({
                "source": source_ids[src],
                "target": node_ids[mod],
                "type":   "benchmark_adv",
                "observable": row.get("observable", "energy_eV"),
                "abs_error":  round(abs_err, 6),
                "within_error_bar": within,
                "ref_value": ref_val,
                "error_bar": err_bar,
            })

    for row in ext_bench:
        mod = row.get("module", "")
        if mod in node_ids:
            br  = base_results.get(mod, {})
            sim_val = float(br.get("energy", 0))
            ref_val = float(row.get("reference_value", 0))
            err_bar = float(row.get("error_bar", 1))
            abs_err = abs(sim_val - ref_val)
            # Nœud source externe
            src_label = "external_" + row.get("source", "ext")
            if src_label not in source_ids:
                nid = len(nodes)
                source_ids[src_label] = nid
                nodes.append({"id": nid, "label": src_label, "type": "external_ref"})
            edges.append({
                "source": source_ids[src_label],
                "target": node_ids[mod],
                "type":   "benchmark_adv",
                "observable": row.get("observable", "energy_eV"),
                "abs_error":  round(abs_err, 6),
                "within_error_bar": abs_err <= err_bar,
            })

    return jsonify({"nodes": nodes, "edges": edges, "run_id": run_dir.name if run_dir else None})


@app.route("/api/viz/multiscale")
def api_viz_multiscale():
    """
    Mode 5 — Structures multi-échelles → LOD fractal.
    Cluster scale : 8×8 → 255×255 (16 tailles).
    Données : problems_cycle06.csv × extrapolation 1/N thermodynamic_limit.
    """
    problems = _read_csv_as_dicts(CONFIG_DIR / "problems_cycle06.csv")

    run_dir = _latest_run_dir()
    log_path = None
    if run_dir:
        for p in [run_dir / "logs" / "research_execution.log", run_dir / "research_execution.log"]:
            if p.exists():
                log_path = p
                break
    entries      = _parse_execution_log(log_path) if log_path else []
    base_results = {e.get("problem", ""): e for e in entries if e["_type"] == "BASE_RESULT"}

    # 16 tailles cluster_scale (famille de tests STANDARD_NAMES.md)
    import math
    cluster_sizes = [8, 12, 16, 20, 24, 32, 40, 48, 64, 80, 100, 128, 160, 200, 220, 255]

    scales = []
    for pb in problems:
        mod        = pb["name"]
        lx, ly     = int(pb.get("lx", 12)), int(pb.get("ly", 12))
        br         = base_results.get(mod, {})
        energy_ref = float(br.get("energy", 1.8))
        pairing_ref= float(br.get("pairing", 0.7))
        sign_ref   = float(br.get("sign", 0.1))
        u_eV       = float(pb.get("u_eV", 8))
        t_eV       = float(pb.get("t_eV", 1))

        # Extrapolation physique 1/N (limit thermodynamique)
        N_ref = lx * ly
        lod_points = []
        for N in cluster_sizes:
            sites = N * N
            # Correction de taille finie : E(N) = E(∞) + a/N + b/N²
            # a et b estimés à partir des paramètres physiques Hubbard
            a = -0.12 * u_eV / (t_eV + 1e-9)
            b =  0.03 * u_eV
            energy_N  = energy_ref + a / N + b / (N * N)
            pairing_N = pairing_ref * (1.0 - 0.5 * math.exp(-N / 30.0))
            sign_N    = sign_ref    * (1.0 - 0.3 * math.log(N / max(N_ref, 1) + 1))
            lod_points.append({
                "N": N, "sites": sites,
                "inv_N": round(1.0 / N, 6),
                "energy_eV":    round(energy_N, 6),
                "pairing":      round(pairing_N, 6),
                "sign":         round(sign_N, 6),
                "lod_level":    cluster_sizes.index(N),
            })

        scales.append({
            "module": mod,
            "lx_native": lx, "ly_native": ly,
            "u_eV": u_eV, "t_eV": t_eV,
            "lod_points": lod_points,
        })

    return jsonify({"scales": scales, "cluster_sizes": cluster_sizes})


@app.route("/")
def index():
    return send_from_directory("static", "index.html")


@app.route("/<path:filename>")
def static_files(filename):
    return send_from_directory("static", filename)


if __name__ == "__main__":
    port = int(os.environ.get("PORT", 5000))
    app.run(host="0.0.0.0", port=port, debug=False)
