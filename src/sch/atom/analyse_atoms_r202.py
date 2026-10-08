#!/usr/bin/env python3
"""
analyse_atoms_r202.py — Analyse atomique brute LUM-VORAX
Rapport 202 : graphe, clusters, bimodalité, déterminisme, bugs

CERTIFIED_100=false | Mode DEBUG actif
Auteur : ARTCB Project <contact@artcb.me>
"""

import json
import gzip
import math
import sys
import os
import re
from collections import defaultdict, Counter
from pathlib import Path

# ─── Chemins ──────────────────────────────────────────────────────────────────
SCRIPT_DIR = Path(__file__).parent
# src/sch/atom/ → remonter 3 niveaux jusqu'à la racine LVX&ARTCB
LVX_ROOT   = SCRIPT_DIR.parent.parent.parent
DATA_DIR   = LVX_ROOT / "logs_AIMO3" / "sch" / "atom"

PHYSICS_FILE   = DATA_DIR / "physics_all_atoms.jsonl"
PAIRS_GZ_FILE  = DATA_DIR / "pairs_all_comparisons.jsonl.gz"
PAIRS_FILE     = DATA_DIR / "pairs_all_comparisons.jsonl"   # si décompressé
STEPS_FILE     = DATA_DIR / "step_nanoseconds.jsonl"
TRANSIENT_FILE = DATA_DIR / "transient_events.log"

DEBUG = True

def dbg(msg):
    if DEBUG:
        print(f"[DEBUG] {msg}", flush=True)

# ─── 1. Analyse physics_all_atoms.jsonl ───────────────────────────────────────

def analyse_physics():
    dbg(f"Ouverture {PHYSICS_FILE}")
    if not PHYSICS_FILE.exists():
        return {"error": "fichier absent"}

    atoms_by_step   = defaultdict(list)
    dt_zeros        = 0
    total_lines     = 0
    nan_inf_count   = 0
    auto_interact   = 0
    type_counter    = Counter()
    atom_ids_seen   = set()

    with open(PHYSICS_FILE) as f:
        for lineno, line in enumerate(f):
            line = line.strip()
            if not line:
                continue
            try:
                rec = json.loads(line)
            except json.JSONDecodeError as e:
                dbg(f"JSON erreur ligne {lineno}: {e}")
                continue

            total_lines += 1
            # Step inféré : groupes de 1000
            step_inferred = (lineno) // 1000
            rec["_step"] = step_inferred

            # Vérification NaN/Inf
            for key in ("x_after","y_after","z_after","vx_after","vy_after","vz_after"):
                v = rec.get(key, 0.0)
                if v is not None and (math.isnan(v) or math.isinf(v)):
                    nan_inf_count += 1

            if rec.get("dt_ns", 1) == 0:
                dt_zeros += 1

            type_counter[rec.get("type")] += 1
            atom_ids_seen.add(rec.get("atom_id"))
            atoms_by_step[step_inferred].append(rec)

    # Statistiques par step
    step_stats = {}
    for step, atoms in sorted(atoms_by_step.items()):
        dts = [a.get("dt_ns", 0) for a in atoms]
        types = Counter(a.get("type") for a in atoms)
        step_stats[step] = {
            "n_atoms":      len(atoms),
            "dt_ns_nonzero": sum(1 for d in dts if d > 0),
            "dt_ns_zero":   sum(1 for d in dts if d == 0),
            "dt_ns_min":    min(dts) if dts else 0,
            "dt_ns_max":    max(dts) if dts else 0,
            "dt_ns_mean":   sum(dts) / len(dts) if dts else 0,
            "types":        dict(types),
        }

    # Déplacement moyen (norme)
    displacements = []
    for step_atoms in atoms_by_step.values():
        for a in step_atoms:
            dx = a.get("x_after", 0) - a.get("x_before", 0)
            dy = a.get("y_after", 0) - a.get("y_before", 0)
            dz = a.get("z_after", 0) - a.get("z_before", 0)
            dist = math.sqrt(dx*dx + dy*dy + dz*dz)
            displacements.append(dist)

    disp_nonzero = [d for d in displacements if d > 0]

    return {
        "total_lines":       total_lines,
        "unique_atom_ids":   len(atom_ids_seen),
        "dt_ns_zeros":       dt_zeros,
        "dt_ns_zero_pct":    100.0 * dt_zeros / total_lines if total_lines else 0,
        "nan_inf_count":     nan_inf_count,
        "auto_interact":     auto_interact,
        "type_distribution": dict(type_counter),
        "step_stats":        step_stats,
        "displacement": {
            "n_nonzero":     len(disp_nonzero),
            "mean":          sum(disp_nonzero)/len(disp_nonzero) if disp_nonzero else 0,
            "max":           max(disp_nonzero) if disp_nonzero else 0,
            "min":           min(disp_nonzero) if disp_nonzero else 0,
        },
    }

# ─── 2. Analyse transient_events.log ──────────────────────────────────────────

TRANSIENT_RE = re.compile(
    r'\[TRANSIENT\]\[step=(\d+)\]\[ts_ns=(\d+)\]\s+ATOMS\((\d+),(\d+)\)\s+DIST\(([0-9.eE+\-]+)\)\s+TYPE\((\d+)\)\s+EVENT_DETECTED'
)

def analyse_transient():
    dbg(f"Ouverture {TRANSIENT_FILE}")
    if not TRANSIENT_FILE.exists():
        return {"error": "fichier absent"}

    total = 0
    auto_interact = 0       # Bug : i == j
    double_count  = 0       # Bug : (j,i) vu quand (i,j) déjà vu
    nan_dist      = 0
    steps_seen    = Counter()
    types_seen    = Counter()
    dist_list     = []
    pairs_set     = set()
    hub_counter   = Counter()  # degré par atome (nb d'interactions uniques)

    with open(TRANSIENT_FILE) as f:
        for line in f:
            m = TRANSIENT_RE.search(line)
            if not m:
                continue
            step = int(m.group(1))
            # ts   = int(m.group(2))
            ai   = int(m.group(3))
            aj   = int(m.group(4))
            dist = float(m.group(5))
            typ  = int(m.group(6))

            total += 1
            steps_seen[step] += 1
            types_seen[typ]   += 1

            # Bug auto-interaction
            if ai == aj:
                auto_interact += 1
                dbg(f"AUTO-INTERACTION : atome {ai} ligne {total}")

            # Bug double-comptage
            pair_key = (min(ai,aj), max(ai,aj), step)
            if pair_key in pairs_set:
                double_count += 1
            else:
                pairs_set.add(pair_key)

            # NaN/Inf dist
            if math.isnan(dist) or math.isinf(dist):
                nan_dist += 1
            else:
                dist_list.append(dist)

            hub_counter[ai] += 1
            hub_counter[aj] += 1

    # Top hubs
    top_hubs = hub_counter.most_common(10)

    # Distribution distances
    if dist_list:
        dist_min  = min(dist_list)
        dist_max  = max(dist_list)
        dist_mean = sum(dist_list) / len(dist_list)
        # Médiane approx
        ds = sorted(dist_list)
        dist_med  = ds[len(ds)//2]
    else:
        dist_min = dist_max = dist_mean = dist_med = 0

    return {
        "total_events":   total,
        "auto_interact":  auto_interact,
        "double_count":   double_count,
        "nan_dist":       nan_dist,
        "unique_pairs":   len(pairs_set),
        "steps":          dict(steps_seen),
        "types":          dict(types_seen),
        "dist": {
            "min":   dist_min,
            "max":   dist_max,
            "mean":  dist_mean,
            "median": dist_med,
        },
        "top_hubs_degree": top_hubs,
    }

# ─── 3. Analyse step_nanoseconds.jsonl (bimodalité) ───────────────────────────

def analyse_steps():
    dbg(f"Ouverture {STEPS_FILE}")
    if not STEPS_FILE.exists():
        return {"error": "fichier absent"}

    step_records = []
    phys_records = []

    with open(STEPS_FILE) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                rec = json.loads(line)
            except json.JSONDecodeError:
                continue
            if rec.get("phase") == "C3_physics":
                phys_records.append(rec)
            else:
                step_records.append(rec)

    # Bimodalité sur dt_ns step_records
    dts = [(r.get("step"), r.get("dt_ns", 0)) for r in step_records]
    dt_vals = [d for _, d in dts]
    if dt_vals:
        threshold = 1_000_000_000  # 1 s en ns
        slow_group = [(s, d) for s, d in dts if d > threshold]
        fast_group = [(s, d) for s, d in dts if d <= threshold]
        ratio = (sum(d for _,d in slow_group) / len(slow_group)) / \
                (sum(d for _,d in fast_group) / len(fast_group)) if fast_group and slow_group else None
    else:
        slow_group = fast_group = []
        ratio = None

    # Variabilité clusters_formed
    clusters = [r.get("clusters_formed") for r in step_records if r.get("clusters_formed") is not None]

    return {
        "n_step_records":   len(step_records),
        "n_phys_records":   len(phys_records),
        "bimodal": {
            "slow_group":   [(s, d) for s, d in slow_group],
            "fast_group":   [(s, d) for s, d in fast_group],
            "ratio_slow_fast": ratio,
        },
        "clusters_formed": {
            "values": clusters,
            "min":    min(clusters) if clusters else None,
            "max":    max(clusters) if clusters else None,
            "range":  (max(clusters) - min(clusters)) if clusters else None,
        },
        "falsif_mode_values": list({r.get("falsif_mode") for r in step_records}),
        "pairs_examined":     list({r.get("pairs_examined") for r in step_records}),
    }

# ─── 4. Analyse pairs_all_comparisons.jsonl.gz (streaming) ───────────────────

def analyse_pairs(max_lines=500_000):
    """Analyse en streaming du .gz — lit max_lines lignes."""
    dbg(f"Analyse pairs (streaming, max {max_lines} lignes)")

    # Choisir le fichier disponible
    if PAIRS_GZ_FILE.exists():
        opener = gzip.open(PAIRS_GZ_FILE, "rt")
        src = str(PAIRS_GZ_FILE)
    elif PAIRS_FILE.exists():
        opener = open(PAIRS_FILE, "r")
        src = str(PAIRS_FILE)
    else:
        return {"error": "aucun fichier pairs disponible"}

    total = 0
    auto_interact = 0
    double_count  = 0
    nan_count     = 0
    keys_seen     = None
    step_counter  = Counter()
    type_counter  = Counter()
    dist_sample   = []
    pairs_set     = set()

    with opener as f:
        for lineno, raw in enumerate(f):
            if lineno >= max_lines:
                break
            raw = raw.strip()
            if not raw:
                continue
            try:
                rec = json.loads(raw)
            except json.JSONDecodeError:
                continue

            total += 1
            if keys_seen is None:
                keys_seen = list(rec.keys())
                dbg(f"Clés pairs: {keys_seen}")

            ai  = rec.get("atom_i", rec.get("i", None))
            aj  = rec.get("atom_j", rec.get("j", None))
            dist = rec.get("dist", rec.get("distance", rec.get("d", None)))
            step = rec.get("step", None)
            typ  = rec.get("type", rec.get("interaction_type", None))

            if ai == aj and ai is not None:
                auto_interact += 1
            if ai is not None and aj is not None:
                pk = (min(ai,aj), max(ai,aj))
                if step is not None:
                    pk = (*pk, step)
                if pk in pairs_set:
                    double_count += 1
                else:
                    pairs_set.add(pk)

            if dist is not None:
                try:
                    d = float(dist)
                    if math.isnan(d) or math.isinf(d):
                        nan_count += 1
                    elif len(dist_sample) < 5000:
                        dist_sample.append(d)
                except (ValueError, TypeError):
                    pass

            if step is not None:
                step_counter[step] += 1
            if typ is not None:
                type_counter[typ] += 1

    if dist_sample:
        ds = sorted(dist_sample)
        dist_stats = {
            "n_sample":  len(ds),
            "min":       ds[0],
            "max":       ds[-1],
            "mean":      sum(ds)/len(ds),
            "median":    ds[len(ds)//2],
        }
    else:
        dist_stats = {}

    return {
        "source":         src,
        "lines_analysed": total,
        "keys_detected":  keys_seen,
        "auto_interact":  auto_interact,
        "double_count":   double_count,
        "nan_count":      nan_count,
        "unique_pairs":   len(pairs_set),
        "step_distribution": dict(step_counter),
        "type_distribution": dict(type_counter),
        "dist_stats":     dist_stats,
    }

# ─── MAIN ─────────────────────────────────────────────────────────────────────

def main():
    print("=" * 70)
    print("ANALYSE ATOMIQUE R202 — LUM-VORAX")
    print("CERTIFIED_100=false | Mode DEBUG actif")
    print("=" * 70)

    print("\n[1/4] Analyse physics_all_atoms.jsonl ...")
    phys = analyse_physics()

    print("\n[2/4] Analyse transient_events.log ...")
    trans = analyse_transient()

    print("\n[3/4] Analyse step_nanoseconds.jsonl ...")
    steps = analyse_steps()

    print("\n[4/4] Analyse pairs_all_comparisons.jsonl.gz (streaming 500k lignes) ...")
    pairs = analyse_pairs(max_lines=500_000)

    # ─── Affichage résultats ──────────────────────────────────────────────
    print("\n" + "=" * 70)
    print("RÉSULTATS")
    print("=" * 70)

    print(f"\n── PHYSICS ({phys.get('total_lines',0)} lignes) ──")
    print(f"  Atomes uniques   : {phys.get('unique_atom_ids')}")
    print(f"  dt_ns=0 (%)      : {phys.get('dt_ns_zeros')} ({phys.get('dt_ns_zero_pct',0):.1f}%)")
    print(f"  NaN/Inf          : {phys.get('nan_inf_count')}")
    print(f"  Types            : {phys.get('type_distribution')}")
    print(f"  Déplacements non-zéro : {phys.get('displacement',{}).get('n_nonzero')}")
    print(f"  Déplacement moyen     : {phys.get('displacement',{}).get('mean',0):.6f}")
    for s, ss in sorted(phys.get("step_stats",{}).items()):
        print(f"  Step {s}: {ss['n_atoms']} atomes, dt_zero={ss['dt_ns_zero']}, "
              f"types={ss['types']}")

    print(f"\n── TRANSIENT ({trans.get('total_events',0)} événements) ──")
    print(f"  Auto-interactions (BUG i==j) : {trans.get('auto_interact')}")
    print(f"  Double-comptages (BUG i,j dupliqué) : {trans.get('double_count')}")
    print(f"  NaN distances    : {trans.get('nan_dist')}")
    print(f"  Paires uniques   : {trans.get('unique_pairs')}")
    print(f"  Steps            : {trans.get('steps')}")
    print(f"  Types            : {trans.get('types')}")
    d = trans.get("dist", {})
    print(f"  Distance min={d.get('min',0):.6f} max={d.get('max',0):.6f} "
          f"mean={d.get('mean',0):.6f} median={d.get('median',0):.6f}")
    print(f"  Top hubs (atome, degré) : {trans.get('top_hubs_degree',[][:5])}")

    print(f"\n── STEPS / BIMODALITÉ ──")
    bm = steps.get("bimodal", {})
    print(f"  Groupe LENT  : {bm.get('slow_group')} s")
    print(f"  Groupe RAPIDE: {bm.get('fast_group')} s")
    print(f"  Ratio lent/rapide : {bm.get('ratio_slow_fast')}")
    cl = steps.get("clusters_formed", {})
    print(f"  clusters_formed : min={cl.get('min')} max={cl.get('max')} range={cl.get('range')}")
    print(f"  falsif_mode values : {steps.get('falsif_mode_values')}")
    print(f"  pairs_examined   : {steps.get('pairs_examined')}")

    print(f"\n── PAIRS (streaming {pairs.get('lines_analysed',0)} lignes) ──")
    print(f"  Source           : {pairs.get('source','?')}")
    print(f"  Clés détectées   : {pairs.get('keys_detected')}")
    print(f"  Auto-interactions: {pairs.get('auto_interact')}")
    print(f"  Double-comptages : {pairs.get('double_count')}")
    print(f"  NaN              : {pairs.get('nan_count')}")
    print(f"  Paires uniques   : {pairs.get('unique_pairs')}")
    print(f"  Steps            : {pairs.get('step_distribution')}")
    print(f"  Types            : {pairs.get('type_distribution')}")
    ds = pairs.get("dist_stats", {})
    if ds:
        print(f"  Dist min={ds.get('min',0):.6f} max={ds.get('max',0):.6f} "
              f"mean={ds.get('mean',0):.6f} median={ds.get('median',0):.6f}")

    # ─── Sauvegarder résultats JSON ───────────────────────────────────────
    out = {
        "report": "R202",
        "certified_100": False,
        "unique_human_proven": False,
        "physics":   phys,
        "transient": trans,
        "steps":     steps,
        "pairs":     pairs,
    }
    out_path = DATA_DIR / "r202_analysis_results.json"
    with open(out_path, "w") as f:
        json.dump(out, f, indent=2)
    print(f"\n✅ Résultats sauvegardés : {out_path}")
    print("=" * 70)

    return out

if __name__ == "__main__":
    results = main()
