#!/usr/bin/env python3
"""
analyse_atoms_r204.py — Tests P0-A / P0-B / P0-C pour Rapport 204
LVXARTCB | Périmètre : LVX&ARTCB/ uniquement
CERTIFIED_100=false | unique_human_proven=false | Mode DEBUG actif

Tests réalisés :
  P0-A : Résolution des 90 031 répétitions dans pairs_all_comparisons.jsonl.gz
         → 3 clés de déduplication : strict, symétrique, contextuel
  P0-B : Continuité atomique after(step N) == before(step N+1)
         → Pour chaque atome, vérifier la cohérence de trajectoire
  P0-C : Comparaison état atomique step rapide vs step lent
         → Répondre à la question centrale : phénomène physique ou artefact CPU ?
  BIMODAL-FIX : Recalcul avec seuil adaptatif (médiane)
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
STEPS_FILE = os.path.join(BASE, "step_nanoseconds.jsonl")
EVENTS     = os.path.join(BASE, "transient_events.log")
OUT_JSON   = os.path.join(BASE, "r204_analysis_results.json")

EPSILON_POS = 1e-9   # seuil continuité position (nm)
EPSILON_VEL = 1e-9   # seuil continuité vitesse

# ─────────────────────────────────────────────────────────────────────────────
# UTILITAIRES
# ─────────────────────────────────────────────────────────────────────────────

def log(msg):
    print(f"[R204] {msg}", flush=True)

def safe_float(v):
    try:
        f = float(v)
        return f if math.isfinite(f) else None
    except (TypeError, ValueError):
        return None

# ─────────────────────────────────────────────────────────────────────────────
# P0-A : RÉSOLUTION DES 90 031 RÉPÉTITIONS (3 clés)
# ─────────────────────────────────────────────────────────────────────────────

def p0a_dedup_analysis():
    """
    Lit pairs_all_comparisons.jsonl.gz en streaming.
    Teste 3 clés de déduplication :
      - Clé STRICT    : (atom_i, atom_j)  — paire orientée
      - Clé SYM       : (min(i,j), max(i,j)) — paire non-orientée
      - Clé CONTEXT   : (step, atom_i, atom_j) — paire dans un step
    Pour chaque clé, compte : total, uniques, doublons, % doublons
    """
    log("P0-A démarré — lecture streaming pairs_all_comparisons.jsonl.gz")
    t0 = time.monotonic()

    seen_strict  = set()
    seen_sym     = set()
    seen_context = set()

    n_total = 0
    n_dup_strict  = 0
    n_dup_sym     = 0
    n_dup_context = 0

    # Exemples de doublons pour audit
    dup_strict_examples  = []
    dup_sym_examples     = []
    dup_context_examples = []

    step_counter = Counter()

    with gzip.open(PAIRS_GZ, "rt", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                rec = json.loads(line)
            except json.JSONDecodeError:
                continue

            n_total += 1
            ai = rec.get("atom_i")
            aj = rec.get("atom_j")
            st = rec.get("step")

            if ai is None or aj is None:
                continue

            step_counter[st] += 1

            # Clé STRICT
            k_strict = (ai, aj)
            if k_strict in seen_strict:
                n_dup_strict += 1
                if len(dup_strict_examples) < 5:
                    dup_strict_examples.append({"step": st, "atom_i": ai, "atom_j": aj})
            else:
                seen_strict.add(k_strict)

            # Clé SYM
            k_sym = (min(ai, aj), max(ai, aj))
            if k_sym in seen_sym:
                n_dup_sym += 1
                if len(dup_sym_examples) < 5:
                    dup_sym_examples.append({"step": st, "atom_i": ai, "atom_j": aj, "sym_key": list(k_sym)})
            else:
                seen_sym.add(k_sym)

            # Clé CONTEXT
            k_ctx = (st, ai, aj)
            if k_ctx in seen_context:
                n_dup_context += 1
                if len(dup_context_examples) < 5:
                    dup_context_examples.append({"step": st, "atom_i": ai, "atom_j": aj})
            else:
                seen_context.add(k_ctx)

    elapsed = time.monotonic() - t0
    log(f"P0-A terminé en {elapsed:.1f}s — {n_total} lignes lues")

    n_uniq_strict  = len(seen_strict)
    n_uniq_sym     = len(seen_sym)
    n_uniq_context = len(seen_context)

    # Interprétation
    # Si dup_sym >> dup_context → les répétitions sont des paires (i,j)+(j,i)
    # Si dup_context >> 0       → mêmes paires dans le même step (multi-run ou bug)
    # Si dup_strict >> dup_sym  → doublons orientés (ordre différent)

    interpretation = []
    if n_dup_sym > 0 and n_dup_context == 0:
        interpretation.append("SYM_ONLY: répétitions = paires (i,j)+(j,i) symétriques — ATTENDU si comparaison bidirectionnelle")
    elif n_dup_context > 0 and n_dup_sym > n_dup_context:
        interpretation.append("SYM_DOMINANT: majorité des répétitions = symétrie (i,j)/(j,i)")
        interpretation.append(f"CONTEXT_DOUBLONS: {n_dup_context} répétitions intra-step — peut indiquer multi-runs ou bug")
    elif n_dup_context == n_dup_sym:
        interpretation.append("SYM_EQ_CONTEXT: répétitions strictement symétriques sans doublons intra-step")
    else:
        interpretation.append(f"MIXTE: dup_strict={n_dup_strict}, dup_sym={n_dup_sym}, dup_context={n_dup_context} — analyse fine requise")

    return {
        "total_lines": n_total,
        "step_distribution": dict(step_counter.most_common()),
        "strict": {
            "unique": n_uniq_strict,
            "duplicates": n_dup_strict,
            "dup_pct": round(100 * n_dup_strict / n_total, 3) if n_total else 0,
            "examples": dup_strict_examples,
        },
        "sym": {
            "unique": n_uniq_sym,
            "duplicates": n_dup_sym,
            "dup_pct": round(100 * n_dup_sym / n_total, 3) if n_total else 0,
            "examples": dup_sym_examples,
        },
        "context": {
            "unique": n_uniq_context,
            "duplicates": n_dup_context,
            "dup_pct": round(100 * n_dup_context / n_total, 3) if n_total else 0,
            "examples": dup_context_examples,
        },
        "interpretation": interpretation,
        "elapsed_s": round(elapsed, 2),
    }


# ─────────────────────────────────────────────────────────────────────────────
# P0-B : CONTINUITÉ ATOMIQUE after(step N) == before(step N+1)
# ─────────────────────────────────────────────────────────────────────────────

def p0b_continuity():
    """
    Lit physics_all_atoms.jsonl.
    Reconstruit la trajectoire de chaque atome par step (trié).
    Pour chaque transition N→N+1 : compare (x_after, y_after, z_after)[N]
    avec (x_before, y_before, z_before)[N+1].
    Compte les discontinuités (delta > EPSILON_POS).
    """
    log("P0-B démarré — lecture physics_all_atoms.jsonl")
    t0 = time.monotonic()

    # Structure : atom_id → step → {"before": (x,y,z,vx,vy,vz), "after": ...}
    atoms = defaultdict(dict)

    with open(PHYSICS, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                rec = json.loads(line)
            except json.JSONDecodeError:
                continue

            atom_id = rec.get("atom_id")
            step    = rec.get("step")

            if atom_id is None or step is None:
                continue

            atoms[atom_id][step] = {
                "xb": safe_float(rec.get("x_before")),
                "yb": safe_float(rec.get("y_before")),
                "zb": safe_float(rec.get("z_before")),
                "xa": safe_float(rec.get("x_after")),
                "ya": safe_float(rec.get("y_after")),
                "za": safe_float(rec.get("z_after")),
                "vxb": safe_float(rec.get("vx_before")),
                "vyb": safe_float(rec.get("vy_before")),
                "vzb": safe_float(rec.get("vz_before")),
                "vxa": safe_float(rec.get("vx_after")),
                "vya": safe_float(rec.get("vy_after")),
                "vza": safe_float(rec.get("vz_after")),
            }

    # Vérifier si le champ "step" existe dans les données
    n_atoms_with_step = len(atoms)
    log(f"P0-B : {n_atoms_with_step} atomes avec champ 'step' explicite")

    if n_atoms_with_step == 0:
        # Le champ step n'existe pas → inférence par groupes de 1000
        log("P0-B : pas de champ 'step' — inférence par groupes de 1000 lignes")
        return _p0b_infer_steps()

    # Analyse continuité
    n_transitions  = 0
    n_discont_pos  = 0
    n_discont_vel  = 0
    n_missing_step = 0
    discont_examples = []

    for atom_id, step_data in atoms.items():
        sorted_steps = sorted(step_data.keys())
        for i in range(len(sorted_steps) - 1):
            sN  = sorted_steps[i]
            sN1 = sorted_steps[i + 1]
            if sN1 != sN + 1:
                n_missing_step += 1
                continue
            n_transitions += 1

            dN  = step_data[sN]
            dN1 = step_data[sN1]

            # Position : after(N) vs before(N+1)
            if dN["xa"] is not None and dN1["xb"] is not None:
                dx = abs(dN["xa"] - dN1["xb"])
                dy = abs(dN["ya"] - dN1["yb"])
                dz = abs(dN["za"] - dN1["zb"])
                dist = math.sqrt(dx*dx + dy*dy + dz*dz)
                if dist > EPSILON_POS:
                    n_discont_pos += 1
                    if len(discont_examples) < 10:
                        discont_examples.append({
                            "atom_id": atom_id,
                            "step_N": sN,
                            "step_N1": sN1,
                            "delta_pos": round(dist, 12),
                            "after_N":  [round(dN["xa"], 8), round(dN["ya"], 8), round(dN["za"], 8)],
                            "before_N1":[round(dN1["xb"], 8), round(dN1["yb"], 8), round(dN1["zb"], 8)],
                        })

            # Vitesse : after(N) vs before(N+1)
            if dN.get("vxa") is not None and dN1.get("vxb") is not None:
                dvx = abs(dN["vxa"] - dN1["vxb"])
                dvy = abs(dN["vya"] - dN1["vyb"])
                dvz = abs(dN["vza"] - dN1["vzb"])
                dv = math.sqrt(dvx*dvx + dvy*dvy + dvz*dvz)
                if dv > EPSILON_VEL:
                    n_discont_vel += 1

    continuity_rate_pos = round(100 * (1 - n_discont_pos / n_transitions), 3) if n_transitions else None
    continuity_rate_vel = round(100 * (1 - n_discont_vel / n_transitions), 3) if n_transitions else None

    elapsed = time.monotonic() - t0
    log(f"P0-B terminé en {elapsed:.1f}s — {n_transitions} transitions, {n_discont_pos} discontinuités pos")

    return {
        "method": "explicit_step_field",
        "n_atoms": n_atoms_with_step,
        "n_transitions": n_transitions,
        "n_missing_step_gap": n_missing_step,
        "n_discont_pos": n_discont_pos,
        "n_discont_vel": n_discont_vel,
        "continuity_rate_pos_pct": continuity_rate_pos,
        "continuity_rate_vel_pct": continuity_rate_vel,
        "epsilon_pos": EPSILON_POS,
        "epsilon_vel": EPSILON_VEL,
        "discont_examples": discont_examples,
        "interpretation": _interpret_continuity(n_discont_pos, n_transitions),
        "elapsed_s": round(elapsed, 2),
    }


def _p0b_infer_steps():
    """Continuité par inférence de steps (groupes de 1000 lignes)."""
    t0 = time.monotonic()
    atoms_by_inferred_step = defaultdict(lambda: defaultdict(dict))
    inferred_step = 0
    line_in_step = 0

    with open(PHYSICS, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                rec = json.loads(line)
            except json.JSONDecodeError:
                continue
            atom_id = rec.get("atom_id")
            if atom_id is None:
                continue
            atoms_by_inferred_step[inferred_step][atom_id] = {
                "xa": safe_float(rec.get("x_after")),
                "ya": safe_float(rec.get("y_after")),
                "za": safe_float(rec.get("z_after")),
                "xb": safe_float(rec.get("x_before")),
                "yb": safe_float(rec.get("y_before")),
                "zb": safe_float(rec.get("z_before")),
            }
            line_in_step += 1
            if line_in_step >= 1000:
                inferred_step += 1
                line_in_step = 0

    n_steps = len(atoms_by_inferred_step)
    n_transitions = 0
    n_discont_pos = 0
    discont_examples = []

    for s in range(n_steps - 1):
        step_n  = atoms_by_inferred_step[s]
        step_n1 = atoms_by_inferred_step[s + 1]
        for atom_id in step_n:
            if atom_id not in step_n1:
                continue
            n_transitions += 1
            dN  = step_n[atom_id]
            dN1 = step_n1[atom_id]
            if dN["xa"] is not None and dN1["xb"] is not None:
                dx = abs(dN["xa"] - dN1["xb"])
                dy = abs(dN["ya"] - dN1["yb"])
                dz = abs(dN["za"] - dN1["zb"])
                dist = math.sqrt(dx*dx + dy*dy + dz*dz)
                if dist > EPSILON_POS:
                    n_discont_pos += 1
                    if len(discont_examples) < 10:
                        discont_examples.append({
                            "atom_id": atom_id,
                            "inferred_step_N": s,
                            "delta_pos": round(dist, 12),
                        })

    elapsed = time.monotonic() - t0
    continuity_rate = round(100 * (1 - n_discont_pos / n_transitions), 3) if n_transitions else None
    log(f"P0-B (inféré) terminé en {elapsed:.1f}s — {n_transitions} transitions, {n_discont_pos} discontinuités")
    return {
        "method": "inferred_groups_of_1000",
        "n_inferred_steps": n_steps,
        "n_transitions": n_transitions,
        "n_discont_pos": n_discont_pos,
        "continuity_rate_pos_pct": continuity_rate,
        "epsilon_pos": EPSILON_POS,
        "discont_examples": discont_examples,
        "interpretation": _interpret_continuity(n_discont_pos, n_transitions),
        "elapsed_s": round(elapsed, 2),
    }


def _interpret_continuity(n_discont, n_trans):
    if n_trans == 0:
        return ["NO_TRANSITIONS: impossible de tester la continuité"]
    rate = n_discont / n_trans
    if rate == 0:
        return ["CONTINUITY_PERFECT: after(N) == before(N+1) pour tous les atomes — trajectoires cohérentes"]
    elif rate < 0.01:
        return [f"CONTINUITY_HIGH: {100*(1-rate):.2f}% cohérent — quelques discontinuités isolées"]
    elif rate < 0.5:
        return [f"CONTINUITY_PARTIAL: {100*(1-rate):.2f}% cohérent — discontinuités significatives"]
    else:
        return [f"CONTINUITY_BROKEN: {100*rate:.1f}% discontinu — les fichiers sont probablement des snapshots indépendants, pas une trajectoire continue"]


# ─────────────────────────────────────────────────────────────────────────────
# P0-C : COMPARAISON ÉTAT ATOMIQUE STEP RAPIDE vs STEP LENT
# ─────────────────────────────────────────────────────────────────────────────

def p0c_fast_vs_slow():
    """
    Compare l'état atomique complet entre :
    - Steps RAPIDES : 4, 23, 30 (~420s CPU)
    - Steps LENTS   : 3, 22, 29 (~3200s CPU)
    Pour chaque groupe : distributions de positions, vitesses, clusters.
    Réponse à la question centrale de l'audit.
    """
    log("P0-C démarré — comparaison états atomiques fast vs slow")
    t0 = time.monotonic()

    FAST_STEPS = {4, 23, 30}
    SLOW_STEPS = {3, 22, 29}

    fast_atoms = defaultdict(list)
    slow_atoms = defaultdict(list)

    # Chargement physics (avec ou sans champ step)
    atoms_raw = []
    with open(PHYSICS, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                rec = json.loads(line)
                atoms_raw.append(rec)
            except json.JSONDecodeError:
                continue

    # Détecter si champ step présent
    has_step = any("step" in r for r in atoms_raw[:10])

    if has_step:
        for rec in atoms_raw:
            st = rec.get("step")
            if st in FAST_STEPS:
                fast_atoms["steps"].append(st)
                fast_atoms["dx"].append(safe_float(rec.get("x_after")) or 0)
                fast_atoms["dy"].append(safe_float(rec.get("y_after")) or 0)
                fast_atoms["dz"].append(safe_float(rec.get("z_after")) or 0)
                fast_atoms["vx"].append(safe_float(rec.get("vx_after")) or 0)
                fast_atoms["vy"].append(safe_float(rec.get("vy_after")) or 0)
                fast_atoms["vz"].append(safe_float(rec.get("vz_after")) or 0)
                fast_atoms["types"].append(rec.get("type"))
                dt = safe_float(rec.get("dt_ns"))
                if dt is not None:
                    fast_atoms["dt_ns"].append(dt)
            elif st in SLOW_STEPS:
                slow_atoms["steps"].append(st)
                slow_atoms["dx"].append(safe_float(rec.get("x_after")) or 0)
                slow_atoms["dy"].append(safe_float(rec.get("y_after")) or 0)
                slow_atoms["dz"].append(safe_float(rec.get("z_after")) or 0)
                slow_atoms["vx"].append(safe_float(rec.get("vx_after")) or 0)
                slow_atoms["vy"].append(safe_float(rec.get("vy_after")) or 0)
                slow_atoms["vz"].append(safe_float(rec.get("vz_after")) or 0)
                slow_atoms["types"].append(rec.get("type"))
                dt = safe_float(rec.get("dt_ns"))
                if dt is not None:
                    slow_atoms["dt_ns"].append(dt)
    else:
        # Inférence : step = ligne // 1000
        for i, rec in enumerate(atoms_raw):
            inferred = i // 1000
            if inferred in FAST_STEPS:
                fast_atoms["dx"].append(safe_float(rec.get("x_after")) or 0)
                fast_atoms["dy"].append(safe_float(rec.get("y_after")) or 0)
                fast_atoms["dz"].append(safe_float(rec.get("z_after")) or 0)
                fast_atoms["vx"].append(safe_float(rec.get("vx_after")) or 0)
                fast_atoms["vy"].append(safe_float(rec.get("vy_after")) or 0)
                fast_atoms["vz"].append(safe_float(rec.get("vz_after")) or 0)
                fast_atoms["types"].append(rec.get("type"))
                dt = safe_float(rec.get("dt_ns"))
                if dt is not None:
                    fast_atoms["dt_ns"].append(dt)
            elif inferred in SLOW_STEPS:
                slow_atoms["dx"].append(safe_float(rec.get("x_after")) or 0)
                slow_atoms["dy"].append(safe_float(rec.get("y_after")) or 0)
                slow_atoms["dz"].append(safe_float(rec.get("z_after")) or 0)
                slow_atoms["vx"].append(safe_float(rec.get("vx_after")) or 0)
                slow_atoms["vy"].append(safe_float(rec.get("vy_after")) or 0)
                slow_atoms["vz"].append(safe_float(rec.get("vz_after")) or 0)
                slow_atoms["types"].append(rec.get("type"))
                dt = safe_float(rec.get("dt_ns"))
                if dt is not None:
                    slow_atoms["dt_ns"].append(dt)

    def stats(lst):
        if not lst:
            return {"n": 0, "mean": None, "std": None, "min": None, "max": None}
        n = len(lst)
        mn = sum(lst) / n
        if n > 1:
            var = sum((x - mn)**2 for x in lst) / (n - 1)
            std = math.sqrt(var)
        else:
            std = 0.0
        return {
            "n": n,
            "mean": round(mn, 8),
            "std": round(std, 8),
            "min": round(min(lst), 8),
            "max": round(max(lst), 8),
        }

    def type_dist(lst):
        c = Counter(lst)
        return dict(c.most_common())

    # Calcul des vitesses scalaires
    def speed_list(vx, vy, vz):
        return [math.sqrt(a*a + b*b + c*c) for a, b, c in zip(vx, vy, vz)]

    fast_speed = speed_list(fast_atoms["vx"], fast_atoms["vy"], fast_atoms["vz"])
    slow_speed = speed_list(slow_atoms["vx"], slow_atoms["vy"], slow_atoms["vz"])

    fast_results = {
        "n_atoms": len(fast_atoms["dx"]),
        "steps": list(set(fast_atoms.get("steps", []))),
        "pos_x": stats(fast_atoms["dx"]),
        "pos_y": stats(fast_atoms["dy"]),
        "pos_z": stats(fast_atoms["dz"]),
        "speed_scalar": stats(fast_speed),
        "dt_ns": stats(fast_atoms["dt_ns"]),
        "type_dist": type_dist(fast_atoms["types"]),
    }
    slow_results = {
        "n_atoms": len(slow_atoms["dx"]),
        "steps": list(set(slow_atoms.get("steps", []))),
        "pos_x": stats(slow_atoms["dx"]),
        "pos_y": stats(slow_atoms["dy"]),
        "pos_z": stats(slow_atoms["dz"]),
        "speed_scalar": stats(slow_speed),
        "dt_ns": stats(slow_atoms["dt_ns"]),
        "type_dist": type_dist(slow_atoms["types"]),
    }

    # Comparaison statistique : différence relative des moyennes
    def compare(f, s, key, sub="mean"):
        fv = f.get(key, {}).get(sub)
        sv = s.get(key, {}).get(sub)
        if fv is None or sv is None or fv == 0 and sv == 0:
            return None
        denom = (abs(fv) + abs(sv)) / 2
        if denom == 0:
            return 0
        return round(abs(fv - sv) / denom, 4)

    comparison = {
        "pos_x_rel_diff":    compare(fast_results, slow_results, "pos_x"),
        "pos_y_rel_diff":    compare(fast_results, slow_results, "pos_y"),
        "pos_z_rel_diff":    compare(fast_results, slow_results, "pos_z"),
        "speed_rel_diff":    compare(fast_results, slow_results, "speed_scalar"),
        "dt_ns_rel_diff":    compare(fast_results, slow_results, "dt_ns"),
    }

    # Interprétation
    interp = []
    pos_diff = max(
        comparison.get("pos_x_rel_diff") or 0,
        comparison.get("pos_y_rel_diff") or 0,
        comparison.get("pos_z_rel_diff") or 0,
    )
    spd_diff = comparison.get("speed_rel_diff") or 0

    if fast_results["n_atoms"] == 0 or slow_results["n_atoms"] == 0:
        interp.append("INSUFFICIENT_DATA: pas assez d'atomes pour la comparaison (champ step probablement absent)")
    elif pos_diff < 0.05 and spd_diff < 0.05:
        interp.append("SAME_STATE: positions et vitesses statistiquement identiques (<5% diff) → bimodalité = ARTEFACT CPU/OS probable")
        interp.append("CONCLUSION: les steps rapides et lents traitent le même état physique dans des conditions informatiques différentes")
    elif pos_diff < 0.20 and spd_diff < 0.20:
        interp.append("SIMILAR_STATE: légères différences (<20%) → bimodalité peut être mixte (CPU + dynamique physique réelle)")
    else:
        interp.append(f"DIFFERENT_STATE: différences significatives (pos={pos_diff:.1%}, speed={spd_diff:.1%}) → phénomène dynamique collectif plausible")
        interp.append("CONCLUSION: les steps rapides et lents correspondent à des états physiques distincts")

    elapsed = time.monotonic() - t0
    log(f"P0-C terminé en {elapsed:.1f}s — fast_n={fast_results['n_atoms']}, slow_n={slow_results['n_atoms']}")

    return {
        "fast_steps": sorted(list(FAST_STEPS)),
        "slow_steps": sorted(list(SLOW_STEPS)),
        "fast": fast_results,
        "slow": slow_results,
        "comparison": comparison,
        "interpretation": interp,
        "elapsed_s": round(elapsed, 2),
    }


# ─────────────────────────────────────────────────────────────────────────────
# BIMODAL-FIX : seuil adaptatif (médiane des dt_ns)
# ─────────────────────────────────────────────────────────────────────────────

def bimodal_fix():
    """Recalcule la bimodalité avec seuil adaptatif = médiane."""
    records = []
    with open(STEPS_FILE, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                rec = json.loads(line)
                if "dt_ns" in rec and "step" in rec and "phase" not in rec:
                    records.append(rec)
            except json.JSONDecodeError:
                continue

    if not records:
        return {"error": "no step records found"}

    dt_values = [r["dt_ns"] for r in records]
    dt_values_sorted = sorted(dt_values)
    n = len(dt_values_sorted)
    median_dt = dt_values_sorted[n // 2]
    threshold = median_dt

    slow_group = [(r["step"], r["dt_ns"]) for r in records if r["dt_ns"] >= threshold]
    fast_group = [(r["step"], r["dt_ns"]) for r in records if r["dt_ns"] < threshold]

    ratio = None
    if fast_group:
        slow_mean = sum(v for _, v in slow_group) / len(slow_group) if slow_group else 0
        fast_mean = sum(v for _, v in fast_group) / len(fast_group) if fast_group else 1
        ratio = round(slow_mean / fast_mean, 2) if fast_mean > 0 else None

    return {
        "n_records": n,
        "median_dt_ns": median_dt,
        "median_dt_s": round(median_dt / 1e9, 2),
        "threshold_used": "median",
        "slow_group": [{"step": s, "dt_ns": v, "dt_s": round(v/1e9, 1)} for s, v in sorted(slow_group)],
        "fast_group": [{"step": s, "dt_ns": v, "dt_s": round(v/1e9, 1)} for s, v in sorted(fast_group)],
        "ratio_slow_fast": ratio,
        "interpretation": (
            f"Bimodalité CONFIRMÉE : {len(slow_group)} steps lents / {len(fast_group)} steps rapides, ratio={ratio}×"
            if ratio and len(fast_group) > 0
            else "DONNÉES INSUFFISANTES pour confirmation bimodalité"
        ),
    }


# ─────────────────────────────────────────────────────────────────────────────
# MAIN
# ─────────────────────────────────────────────────────────────────────────────

def main():
    ts_start = time.time()
    log("=" * 60)
    log("Rapport 204 — Tests P0-A / P0-B / P0-C")
    log(f"Base data : {BASE}")
    log("=" * 60)

    results = {
        "report": "R204",
        "certified_100": False,
        "unique_human_proven": False,
        "mode_debug": True,
        "ts_start": ts_start,
    }

    # Bimodalité fix (rapide, fait en premier)
    log("\n--- BIMODAL-FIX ---")
    results["bimodal_fix"] = bimodal_fix()

    # P0-A
    log("\n--- P0-A : RÉSOLUTION DOUBLONS ---")
    results["p0a"] = p0a_dedup_analysis()

    # P0-B
    log("\n--- P0-B : CONTINUITÉ ATOMIQUE ---")
    results["p0b"] = p0b_continuity()

    # P0-C
    log("\n--- P0-C : FAST vs SLOW STATES ---")
    results["p0c"] = p0c_fast_vs_slow()

    results["ts_end"] = time.time()
    results["elapsed_total_s"] = round(results["ts_end"] - ts_start, 2)

    # Sauvegarde JSON
    with open(OUT_JSON, "w", encoding="utf-8") as f:
        json.dump(results, f, indent=2, ensure_ascii=False)
    log(f"\nLog sauvegardé : {OUT_JSON}")
    log(f"Durée totale : {results['elapsed_total_s']}s")

    return results


if __name__ == "__main__":
    results = main()
    print("\n=== RÉSUMÉ ===")
    bm = results.get("bimodal_fix", {})
    print(f"BIMODAL : {bm.get('interpretation', 'N/A')}")
    p0a = results.get("p0a", {})
    print(f"P0-A strict  : {p0a.get('strict', {}).get('duplicates', '?')} doublons ({p0a.get('strict', {}).get('dup_pct', '?')}%)")
    print(f"P0-A sym     : {p0a.get('sym', {}).get('duplicates', '?')} doublons ({p0a.get('sym', {}).get('dup_pct', '?')}%)")
    print(f"P0-A context : {p0a.get('context', {}).get('duplicates', '?')} doublons ({p0a.get('context', {}).get('dup_pct', '?')}%)")
    print(f"P0-A interprétation : {p0a.get('interpretation', [])}")
    p0b = results.get("p0b", {})
    print(f"P0-B : {p0b.get('interpretation', ['?'])}")
    p0c = results.get("p0c", {})
    print(f"P0-C : {p0c.get('interpretation', ['?'])}")
