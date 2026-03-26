#!/usr/bin/env python3
"""
shadow_c_monitor.py — Cycle 18 — Shadow C Monitoring Framework
============================================================
Compare les résultats du binaire C (advanced_parallel_work) avec une
implémentation Python de référence légère, pour détecter tout comportement
non programmé ou régression avant de basculer totalement vers du C pur.

Conforme STANDARD_NAMES.md — événements : SHADOW_MATCH, SHADOW_DRIFT, ANOMALY

Usage :
    python3 shadow_c_monitor.py <run_dir> [--lumvorax-csv <path>]

Arguments :
    run_dir         : Répertoire du run C (research_YYYYMMDDTHHMMSSZ_PPPP/)
    --lumvorax-csv  : Chemin CSV LumVorax pour écriture des événements shadow
"""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
import math
import os
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from statistics import mean, stdev


# ── Implémentation Python de référence (shadow) ─────────────────────────────

def py_local_energy(state: list[float], t_eV: float, u_eV: float, mu_eV: float) -> float:
    """Énergie Hubbard locale (même formule que le C)."""
    n = len(state)
    if n < 2:
        return 0.0
    e_hop = e_hubb = e_mu = 0.0
    for i in range(n):
        j = (i + 1) % n
        e_hop  += -t_eV  * state[i] * state[j]
        e_hubb += u_eV   * state[i]**2 * state[(i + n//2) % n]**2
        e_mu   += -mu_eV * state[i]**2
    return (e_hop + e_hubb + e_mu) / n


def py_mc_simulate(sites: int, steps: int, t_eV: float, u_eV: float,
                    mu_eV: float, seed: int) -> dict:
    """Simulation Monte Carlo Python (référence shadow)."""
    import random
    rng = random.Random(seed)
    state = [rng.gauss(0, 1) for _ in range(sites)]
    norm = math.sqrt(sum(x**2 for x in state)) or 1e-12
    state = [x / norm for x in state]

    energy = py_local_energy(state, t_eV, u_eV, mu_eV)
    pairing_acc = 0.0
    energy_acc  = 0.0
    n_acc       = 0

    for step in range(steps):
        i = rng.randrange(sites)
        old = state[i]
        state[i] += rng.gauss(0, 0.3)
        new_e = py_local_energy(state, t_eV, u_eV, mu_eV)
        dE = new_e - energy
        if dE <= 0 or rng.random() < math.exp(-dE / max(t_eV * 8.617e-5, 1e-12)):
            energy = new_e
        else:
            state[i] = old
        if step > steps // 4:  # burn-in
            energy_acc  += energy
            pairing_s = sum(state[k] * state[(k+1)%sites] for k in range(sites-1))
            pairing_acc += abs(pairing_s / max(sites - 1, 1))
            n_acc += 1

    n_acc = max(n_acc, 1)
    return {
        "energy_eV": energy_acc / n_acc,
        "pairing":   pairing_acc / n_acc,
        "sites":     sites,
        "steps":     steps,
    }


# ── Lecture des résultats C depuis les CSV du run ───────────────────────────

def load_c_results(run_dir: Path) -> list[dict]:
    """Charge les résultats C depuis baseline_reanalysis_metrics.csv."""
    csv_path = run_dir / "logs" / "baseline_reanalysis_metrics.csv"
    if not csv_path.exists():
        return []
    results = {}
    with open(csv_path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            name = row.get("problem", "")
            try:
                step = int(row.get("step", 0))
                energy = float(row.get("energy", float("nan")))
                pairing = float(row.get("pairing", float("nan")))
            except (ValueError, TypeError):
                continue
            # Garde le dernier step (résultat final)
            results[name] = {"energy_eV": energy, "pairing": pairing,
                             "step": step, "name": name}
    return list(results.values())


def load_c_metadata(run_dir: Path) -> list[dict]:
    """Charge les métadonnées physiques depuis module_physics_metadata.csv."""
    csv_path = run_dir / "tests" / "module_physics_metadata.csv"
    if not csv_path.exists():
        return []
    meta = []
    with open(csv_path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                t_eV = float(row.get("t", 1.0))
                u_eV = float(row.get("U", 8.0))
                meta.append({
                    "name": row.get("module", ""),
                    "lattice_size": row.get("lattice_size", "10x10"),
                    "t_eV": t_eV,
                    "u_eV": u_eV,
                    "geometry": row.get("geometry", "square_2d"),
                })
            except (ValueError, TypeError):
                continue
    return meta


# ── Écriture événements dans le CSV LumVorax (BC-LV04 bridge) ───────────────

def write_lumvorax_event(csv_path: str | None, event: str,
                          detail: str, value: str) -> None:
    """Écrit un événement shadow dans le CSV LumVorax."""
    if not csv_path or not os.path.isfile(csv_path):
        return
    iso = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    ts  = time.time_ns()
    pid = os.getpid()
    try:
        with open(csv_path, "a") as f:
            f.write(f"{event},{iso},{ts},{pid},{detail},{value}\n")
    except OSError:
        pass


# ── Détection d'anomalies comportementales ───────────────────────────────────

def detect_anomalies(c_res: dict, py_res: dict, threshold: float = 0.30) -> list[str]:
    """
    Compare C vs Python et retourne une liste d'anomalies.
    Seuil : 30% de divergence relative = anomalie comportementale.
    """
    anomalies = []
    # Énergie
    c_e, py_e = c_res.get("energy_eV", float("nan")), py_res.get("energy_eV", float("nan"))
    if not math.isfinite(c_e):
        anomalies.append(f"C_energy_nan_or_inf:{c_e}")
    elif not math.isfinite(py_e):
        anomalies.append(f"PY_energy_nan_or_inf:{py_e}")
    else:
        rel_delta = abs(c_e - py_e) / (abs(py_e) + 1e-12)
        if rel_delta > threshold:
            anomalies.append(f"ENERGY_DIVERGE_C={c_e:.6f}_PY={py_e:.6f}_delta={rel_delta:.3f}")

    # Pairing
    c_p, py_p = c_res.get("pairing", float("nan")), py_res.get("pairing", float("nan"))
    if math.isfinite(c_p) and math.isfinite(py_p):
        rel_pair = abs(c_p - py_p) / (abs(py_p) + 1e-12)
        if rel_pair > threshold * 2:
            anomalies.append(f"PAIRING_DIVERGE_C={c_p:.6f}_PY={py_p:.6f}_delta={rel_pair:.3f}")

    return anomalies


# ── Rapport shadow ────────────────────────────────────────────────────────────

def run_shadow_monitor(run_dir: Path, lumvorax_csv: str | None) -> dict:
    """Exécute le monitoring shadow complet et retourne le rapport."""
    t_start = time.time_ns()
    report = {
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "run_dir": str(run_dir),
        "shadow_comparisons": [],
        "anomalies_total": 0,
        "shadow_match_rate": 0.0,
        "status": "UNKNOWN",
    }

    c_results = load_c_results(run_dir)
    c_metadata = load_c_metadata(run_dir)

    if not c_results:
        report["status"] = "NO_C_RESULTS"
        write_lumvorax_event(lumvorax_csv, "ANOMALY", "shadow:no_c_results", "1")
        return report

    # Map métadonnées par nom de problème
    meta_map = {m["name"]: m for m in c_metadata}

    total_comparisons = 0
    total_matches = 0
    total_anomalies = 0

    for c_res in c_results:
        name = c_res["name"]
        meta = meta_map.get(name, {})
        t_eV = meta.get("t_eV", 1.0)
        u_eV = meta.get("u_eV", 8.0)
        lat  = meta.get("lattice_size", "10x10")

        # Déduction des sites depuis la taille du réseau
        try:
            lx, ly = (int(x) for x in lat.split("x"))
            sites = lx * ly
        except (ValueError, TypeError):
            sites = 100
        sites = max(sites, 4)
        # Limite shadow à 100 sites max pour la rapidité
        shadow_sites = min(sites, 100)

        # Simulation Python shadow (rapide : 200 steps)
        t_py_start = time.time_ns()
        py_res = py_mc_simulate(
            sites=shadow_sites,
            steps=200,
            t_eV=t_eV,
            u_eV=u_eV,
            mu_eV=0.0,
            seed=hash(name) & 0xFFFFFFFF,
        )
        py_elapsed_ns = time.time_ns() - t_py_start

        # Détection d'anomalies
        anomalies = detect_anomalies(c_res, py_res)
        is_match = len(anomalies) == 0
        total_comparisons += 1
        total_anomalies   += len(anomalies)
        if is_match:
            total_matches += 1

        comparison = {
            "problem":        name,
            "c_energy_eV":    c_res.get("energy_eV"),
            "py_energy_eV":   py_res["energy_eV"],
            "c_pairing":      c_res.get("pairing"),
            "py_pairing":     py_res["pairing"],
            "shadow_match":   is_match,
            "anomalies":      anomalies,
            "py_elapsed_ns":  py_elapsed_ns,
        }
        report["shadow_comparisons"].append(comparison)

        # Écriture dans CSV LumVorax
        event_type = "SHADOW_MATCH" if is_match else "SHADOW_DRIFT"
        c_e_str = f"{c_res.get('energy_eV', float('nan')):.6f}"
        write_lumvorax_event(lumvorax_csv, event_type,
                              f"shadow:{name}:c_energy", c_e_str)
        if anomalies:
            for anom in anomalies:
                write_lumvorax_event(lumvorax_csv, "ANOMALY",
                                      f"shadow:{name}:{anom}", "1")

    total_elapsed_ns = time.time_ns() - t_start
    report["anomalies_total"]    = total_anomalies
    report["shadow_match_rate"]  = total_matches / max(total_comparisons, 1)
    report["total_comparisons"]  = total_comparisons
    report["total_elapsed_ns"]   = total_elapsed_ns
    report["status"]             = "OK" if total_anomalies == 0 else "ANOMALIES_DETECTED"

    write_lumvorax_event(lumvorax_csv, "METRIC",
                          "shadow:match_rate",
                          f"{report['shadow_match_rate']:.4f}")
    write_lumvorax_event(lumvorax_csv, "METRIC",
                          "shadow:total_elapsed_ns",
                          str(total_elapsed_ns))

    return report


# ── Main ─────────────────────────────────────────────────────────────────────

def main() -> int:
    parser = argparse.ArgumentParser(description="Shadow C Monitor — Cycle 18")
    parser.add_argument("run_dir", type=Path, help="Répertoire du run C")
    parser.add_argument("--lumvorax-csv", default=None,
                        help="Chemin CSV LumVorax pour écriture des événements")
    args = parser.parse_args()

    if not args.run_dir.is_dir():
        print(f"[ERREUR] Répertoire non trouvé : {args.run_dir}", file=sys.stderr)
        return 1

    lumvorax_csv = args.lumvorax_csv or os.environ.get("LUMVORAX_CSV_PATH")

    print(f"[SHADOW_C_MONITOR] Démarrage — run_dir={args.run_dir}", flush=True)
    print(f"[SHADOW_C_MONITOR] LumVorax CSV = {lumvorax_csv or 'non configuré'}", flush=True)

    report = run_shadow_monitor(args.run_dir, lumvorax_csv)

    # Sauvegarde du rapport JSON dans le run_dir
    report_path = args.run_dir / "tests" / "shadow_c_monitor_report.json"
    report_path.parent.mkdir(parents=True, exist_ok=True)
    with open(report_path, "w") as f:
        json.dump(report, f, indent=2, default=str)

    # Rapport console
    print(f"[SHADOW_C_MONITOR] Comparaisons : {report.get('total_comparisons', 0)}", flush=True)
    print(f"[SHADOW_C_MONITOR] Match rate   : {report['shadow_match_rate']:.1%}", flush=True)
    print(f"[SHADOW_C_MONITOR] Anomalies    : {report['anomalies_total']}", flush=True)
    print(f"[SHADOW_C_MONITOR] Statut       : {report['status']}", flush=True)
    print(f"[SHADOW_C_MONITOR] Rapport JSON : {report_path}", flush=True)

    if report["anomalies_total"] > 0:
        print("\n[SHADOW_C_MONITOR] === ANOMALIES DÉTECTÉES ===", flush=True)
        for comp in report["shadow_comparisons"]:
            if comp["anomalies"]:
                print(f"  {comp['problem']}: {comp['anomalies']}", flush=True)

    return 0 if report["status"] == "OK" else 1


if __name__ == "__main__":
    sys.exit(main())
