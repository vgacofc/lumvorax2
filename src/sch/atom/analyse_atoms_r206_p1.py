#!/usr/bin/env python3
"""
analyse_atoms_r206_p1.py — Tests P1 : orientations, profil atome 122, distribution dt_ns
LVXARTCB | Périmètre : LVX&ARTCB/ uniquement
CERTIFIED_100=false | unique_human_proven=false | Mode DEBUG actif

Tests :
  P1-A : Hypothèse deux orientations (i,j)/(j,i) pour steps 1 et 4
         → Compter les paires (i,j) et (j,i) coexistant DANS LE MÊME STEP
  P1-B : Profil de l'atome 122 dans step 0
         → Est-il particulier ou simplement le premier exemple du fichier ?
  P1-C : Distribution complète de dt_ns
         → Histogramme, quantiles, par step, par type atomique
"""

import gzip
import json
import math
import os
import sys
import time
from collections import defaultdict, Counter

BASE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "logs_AIMO3", "sch", "atom")
PAIRS_GZ   = os.path.join(BASE, "pairs_all_comparisons.jsonl.gz")
PHYSICS    = os.path.join(BASE, "physics_all_atoms.jsonl")
OUT_JSON   = os.path.join(BASE, "r206_p1_results.json")

DEBUG = True


def log(msg):
    print(f"[R206-P1] {msg}", flush=True)


def safe_float(v):
    try:
        f = float(v)
        return f if math.isfinite(f) else None
    except (TypeError, ValueError):
        return None


# ─────────────────────────────────────────────────────────────────────────────
# P1-A : ORIENTATIONS — paires (i,j) ET (j,i) coexistant dans le même step
# ─────────────────────────────────────────────────────────────────────────────

def p1a_orientation_analysis():
    """
    Pour chaque step, construit l'ensemble des paires observées.
    Teste si pour une paire (i,j) dans le step S, la paire (j,i) est aussi dans le step S.
    Focus sur steps 1 (attendu ~977481 lignes ≈ 2×499500) et step 4 (499500 lignes exactement).
    """
    log("P1-A démarré — analyse des orientations par step")
    t0 = time.monotonic()

    # Pour chaque step : ensemble des paires orientées (i,j)
    step_pairs = defaultdict(set)
    step_line_count = Counter()

    FOCUS_STEPS = {1, 4}   # steps d'intérêt principal

    with gzip.open(PAIRS_GZ, "rt", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                rec = json.loads(line)
            except json.JSONDecodeError:
                continue

            ai = rec.get("atom_i")
            aj = rec.get("atom_j")
            st = rec.get("step")

            if ai is None or aj is None or st is None:
                continue

            step_line_count[st] += 1

            # Stocker uniquement pour les steps d'intérêt pour limiter la mémoire
            if st in FOCUS_STEPS:
                step_pairs[st].add((ai, aj))

    elapsed = time.monotonic() - t0
    log(f"P1-A lecture terminée en {elapsed:.1f}s")

    results = {}
    for st in sorted(FOCUS_STEPS):
        pairs = step_pairs[st]
        n_total_pairs = len(pairs)
        n_lines = step_line_count[st]

        # Compter les paires (i,j) pour lesquelles (j,i) est aussi présent
        n_with_mirror = 0
        mirror_examples = []
        for (i, j) in pairs:
            if (j, i) in pairs:
                n_with_mirror += 1
                if len(mirror_examples) < 5:
                    mirror_examples.append({"i": i, "j": j})

        # Compter les paires non ordonnées uniques
        unordered = {(min(i, j), max(i, j)) for (i, j) in pairs}
        n_unordered = len(unordered)

        results[st] = {
            "n_lines_in_step": n_lines,
            "n_oriented_pairs": n_total_pairs,
            "n_unordered_pairs": n_unordered,
            "n_pairs_with_mirror_in_same_step": n_with_mirror,
            "mirror_rate_pct": round(100 * n_with_mirror / n_total_pairs, 3) if n_total_pairs else 0,
            "mirror_examples": mirror_examples,
            "hypothesis_two_orientations": n_with_mirror > (n_total_pairs * 0.5),
            "reference_499500": 499500,
            "ratio_lines_vs_499500": round(n_lines / 499500, 4),
        }
        log(f"  step {st}: {n_lines} lignes, {n_total_pairs} paires orientées, "
            f"{n_with_mirror} avec miroir ({results[st]['mirror_rate_pct']}%)")

    return {"results_by_step": results, "elapsed_s": round(elapsed, 2)}


# ─────────────────────────────────────────────────────────────────────────────
# P1-B : PROFIL DE L'ATOME 122 dans step 0
# ─────────────────────────────────────────────────────────────────────────────

def p1b_atom122_profile():
    """
    Dans pairs_all_comparisons.jsonl.gz, step=0 :
    - Compte combien de fois l'atome 122 apparaît comme atom_i ou atom_j
    - Compte le degré de l'atome 122 (partenaires uniques)
    - Compare avec la distribution globale des degrés pour step=0
    - Répond : l'atome 122 est-il spécial ou est-ce juste le premier dans les exemples ?
    """
    log("P1-B démarré — profil atome 122 dans step 0")
    t0 = time.monotonic()

    # Degré pour step 0 uniquement
    degree_step0 = Counter()   # atom_id → nombre de partenaires uniques
    partners_step0 = defaultdict(set)

    with gzip.open(PAIRS_GZ, "rt", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                rec = json.loads(line)
            except json.JSONDecodeError:
                continue

            st = rec.get("step")
            if st != 0:
                continue

            ai = rec.get("atom_i")
            aj = rec.get("atom_j")
            if ai is None or aj is None:
                continue

            partners_step0[ai].add(aj)
            partners_step0[aj].add(ai)

    # Calculer degrés
    degrees = {atom: len(partners) for atom, partners in partners_step0.items()}

    deg_values = list(degrees.values())
    n_atoms = len(deg_values)
    mean_deg = sum(deg_values) / n_atoms if n_atoms else 0
    sorted_deg = sorted(deg_values, reverse=True)

    deg_122 = degrees.get(122, 0)
    rank_122 = sorted(degrees.keys(), key=lambda a: -degrees[a]).index(122) + 1 if 122 in degrees else None

    # Top 10 atomes par degré
    top10 = sorted(degrees.items(), key=lambda x: -x[1])[:10]

    elapsed = time.monotonic() - t0
    log(f"P1-B terminé en {elapsed:.1f}s — degré atome 122 = {deg_122}, rang = {rank_122}/{n_atoms}")

    return {
        "step": 0,
        "n_atoms_in_step": n_atoms,
        "atom_122_degree": deg_122,
        "atom_122_rank": rank_122,
        "mean_degree": round(mean_deg, 3),
        "max_degree": sorted_deg[0] if sorted_deg else 0,
        "min_degree": sorted_deg[-1] if sorted_deg else 0,
        "top10_atoms_by_degree": [{"atom": a, "degree": d} for a, d in top10],
        "interpretation": (
            "ATOM_122_EXCEPTIONAL"
            if deg_122 > mean_deg * 2
            else "ATOM_122_FIRST_EXAMPLE_ONLY"
            if deg_122 <= mean_deg * 1.1
            else "ATOM_122_ABOVE_AVERAGE"
        ),
        "elapsed_s": round(elapsed, 2),
    }


# ─────────────────────────────────────────────────────────────────────────────
# P1-C : DISTRIBUTION COMPLÈTE DE dt_ns dans physics_all_atoms.jsonl
# ─────────────────────────────────────────────────────────────────────────────

def p1c_dt_distribution():
    """
    Lit physics_all_atoms.jsonl.
    Construit la distribution complète de dt_ns :
    - Global : min, max, mean, std, médiane, p90, p95, p99, n_zeros
    - Par type atomique (0, 1, 2, 3)
    """
    log("P1-C démarré — distribution dt_ns dans physics_all_atoms.jsonl")
    t0 = time.monotonic()

    all_dt = []
    by_type = defaultdict(list)
    n_lines = 0
    n_no_dt = 0

    with open(PHYSICS, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                rec = json.loads(line)
            except json.JSONDecodeError:
                continue

            n_lines += 1
            dt = safe_float(rec.get("dt_ns"))
            atom_type = rec.get("type")

            if dt is None:
                n_no_dt += 1
                continue

            all_dt.append(dt)
            if atom_type is not None:
                by_type[atom_type].append(dt)

    elapsed = time.monotonic() - t0
    log(f"P1-C lecture terminée en {elapsed:.1f}s — {n_lines} lignes, {len(all_dt)} dt valides")

    def stats(values):
        if not values:
            return {}
        n = len(values)
        s = sorted(values)
        mean = sum(values) / n
        variance = sum((v - mean) ** 2 for v in values) / n
        std = math.sqrt(variance)
        n_zero = sum(1 for v in values if v == 0)
        return {
            "n": n,
            "n_zeros": n_zero,
            "zero_pct": round(100 * n_zero / n, 3),
            "min": s[0],
            "p10": s[int(n * 0.10)],
            "p25": s[int(n * 0.25)],
            "median": s[n // 2],
            "p75": s[int(n * 0.75)],
            "p90": s[int(n * 0.90)],
            "p95": s[int(n * 0.95)],
            "p99": s[int(n * 0.99)],
            "max": s[-1],
            "mean": round(mean, 3),
            "std": round(std, 3),
        }

    global_stats = stats(all_dt)
    type_stats = {str(t): stats(vals) for t, vals in sorted(by_type.items())}

    return {
        "n_lines_total": n_lines,
        "n_no_dt": n_no_dt,
        "global": global_stats,
        "by_type": type_stats,
        "elapsed_s": round(elapsed, 2),
    }


# ─────────────────────────────────────────────────────────────────────────────
# MAIN
# ─────────────────────────────────────────────────────────────────────────────

def main():
    log(f"=== R206-P1 démarré | certified_100=false | unique_human_proven=false | debug={DEBUG} ===")
    ts_start = time.monotonic()

    result = {
        "report": "R206_P1",
        "certified_100": False,
        "unique_human_proven": False,
        "mode_debug": DEBUG,
        "ts_start": time.time(),
    }

    log("--- P1-A : orientations ---")
    result["p1a"] = p1a_orientation_analysis()

    log("--- P1-B : profil atome 122 ---")
    result["p1b"] = p1b_atom122_profile()

    log("--- P1-C : distribution dt_ns ---")
    result["p1c"] = p1c_dt_distribution()

    result["elapsed_total_s"] = round(time.monotonic() - ts_start, 2)

    with open(OUT_JSON, "w", encoding="utf-8") as f:
        json.dump(result, f, indent=2, ensure_ascii=False)

    log(f"=== R206-P1 terminé en {result['elapsed_total_s']}s → {OUT_JSON} ===")
    return result


if __name__ == "__main__":
    main()
