#!/usr/bin/env python3
"""
dry_run_pqe_pure.py — Dry-run PQE NX48 PURE PYTHON (sans numpy/qiskit)
Simule les 7 étapes du script ibm_quantum_pqe_nx48_integrated.py
avec forensic logging complet et rapport de run.
"""

import csv
import hashlib
import json
import math
import os
import struct
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

RUN_ID = datetime.now(timezone.utc).strftime("pqe_dryrun_%Y%m%dT%H%M%SZ")
RUN_START = time.time()
LOG_ENTRIES = []

def ts_ns():
    return int(time.time_ns())

def forensic_log(key, value):
    LOG_ENTRIES.append({
        "ts_ns": ts_ns(),
        "run_id": RUN_ID,
        "key": key,
        "value": str(value)
    })
    print(f"  [FORENSIC] {key} = {value}")

def load_nx48_csv(csv_path="config/btc_nx48_last.csv"):
    state = {
        "exploration_bias": 0.902,
        "delta_nonce_scale": 40.73,
        "best_leading": 38,
        "update_count": 2336,
        "weights": [0.0] * 8,
        "source": "default"
    }
    if os.path.exists(csv_path):
        try:
            with open(csv_path, "r") as f:
                reader = csv.DictReader(f)
                for row in reader:
                    key = row.get("key", "").strip()
                    val = row.get("value", "").strip()
                    if key == "exploration_bias":
                        state["exploration_bias"] = float(val)
                    elif key == "delta_nonce_scale":
                        state["delta_nonce_scale"] = float(val)
                    elif key == "best_leading":
                        state["best_leading"] = int(float(val))
                    elif key == "update_count":
                        state["update_count"] = int(float(val))
            state["source"] = "csv"
        except Exception as e:
            state["source"] = f"default (erreur: {e})"
    return state

def simulate_pre_measure(bias):
    # Méthode analytique : alpha^2 + beta^2 = 1
    alpha2 = 0.5 * (1.0 + math.cos(math.pi * bias))
    beta2 = 1.0 - alpha2
    return alpha2, beta2

def build_observables_pure(n_qubits=2):
    """
    Construction symbolique des observables S(π), S(k), C(r).
    Sans qiskit — représentation chaîne Pauli.
    """
    obs = []
    # S(π) : opérateur d'espacement spectral
    s_pi = {"label": "S_pi", "pauli": "ZZ" if n_qubits == 2 else "Z" * n_qubits, "coeff": 1.0}
    obs.append(s_pi)
    # S(k) : densité de niveaux k=0..n_qubits-1
    for k in range(n_qubits):
        pauli_str = "I" * k + "Z" + "I" * (n_qubits - k - 1)
        obs.append({"label": f"S_k_{k}", "pauli": pauli_str, "coeff": 0.5})
    # C(r) : corrélateur longue portée
    if n_qubits >= 2:
        obs.append({"label": "C_r_0_1", "pauli": "XX", "coeff": 0.25})
    return obs

def build_pubs(observables, nx48_state, pre_measure, n_qubits=2, n_params=6):
    """
    Construit les Publication Units (pubs) symboliques.
    Chaque pub = (circuit_params, observable, nx48_weight).
    """
    pubs = []
    bias = nx48_state["exploration_bias"]
    delta = nx48_state["delta_nonce_scale"]

    for i, obs in enumerate(observables):
        params = [bias * math.sin(j * delta / (i + 1)) for j in range(n_params)]
        pub = {
            "pub_index": i,
            "observable": obs,
            "params": params,
            "pre_measure_alpha2": pre_measure[0],
            "pre_measure_beta2": pre_measure[1],
            "nx48_bias": bias,
            "nx48_delta": delta,
        }
        pubs.append(pub)
    return pubs

def simulate_estimator_run(pubs):
    """
    Simule le résultat d'un Estimator pour chaque pub.
    Retourne des valeurs d'expectation synthétiques.
    """
    results = []
    for pub in pubs:
        obs = pub["observable"]
        params = pub["params"]
        # Expectation value simulée : combinaison params + pauli label hash
        seed = int(hashlib.md5(obs["label"].encode()).hexdigest()[:8], 16)
        ev = math.cos(sum(params)) * obs["coeff"] * ((seed % 100) / 100.0)
        std = abs(math.sin(sum(params))) * 0.05 + 0.01
        results.append({
            "pub_index": pub["pub_index"],
            "observable": obs["label"],
            "expectation_value": round(ev, 6),
            "std_error": round(std, 6),
            "shots": 5  # dry-run = 5 pubs, pas de shots réels
        })
    return results

def save_forensic_csv(path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["ts_ns", "run_id", "key", "value"])
        writer.writeheader()
        for entry in LOG_ENTRIES:
            writer.writerow(entry)
    print(f"  [FORENSIC] CSV sauvegardé : {path}")

def save_forensic_json(path, extra):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    data = {
        "run_id": RUN_ID,
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "entries": LOG_ENTRIES,
        **extra
    }
    with open(path, "w") as f:
        json.dump(data, f, indent=2)
    print(f"  [FORENSIC] JSON sauvegardé : {path}")

def main():
    print("=" * 70)
    print(f"  PQE NX48 DRY-RUN (PURE PYTHON) — {RUN_ID}")
    print("=" * 70)

    forensic_log("script_start", RUN_ID)

    # ── 1/7 : Chargement NX48 ─────────────────────────────────────────────
    print("\n[1/7] Chargement état NX48...")
    nx48_state = load_nx48_csv()
    forensic_log("nx48_source", nx48_state["source"])
    forensic_log("nx48_exploration_bias", nx48_state["exploration_bias"])
    forensic_log("nx48_delta_nonce_scale", nx48_state["delta_nonce_scale"])
    forensic_log("nx48_best_leading", nx48_state["best_leading"])
    forensic_log("nx48_update_count", nx48_state["update_count"])
    print(f"  Exploration bias : {nx48_state['exploration_bias']}")
    print(f"  Delta nonce scale: {nx48_state['delta_nonce_scale']}")
    print(f"  Source           : {nx48_state['source']}")

    # ── 2/7 : Quantum pre-measure ─────────────────────────────────────────
    print("\n[2/7] Simulation quantum pre-measure state...")
    alpha2, beta2 = simulate_pre_measure(nx48_state["exploration_bias"])
    forensic_log("pre_measure_alpha2", alpha2)
    forensic_log("pre_measure_beta2", beta2)
    print(f"  |α|² (prob |0⟩) : {alpha2:.4f}")
    print(f"  |β|² (prob |1⟩) : {beta2:.4f}")
    print(f"  Méthode         : analytical (pure-python)")

    # ── 3/7 : Construction observables ───────────────────────────────────
    print("\n[3/7] Construction observables complets...")
    n_qubits = 2
    observables = build_observables_pure(n_qubits)
    forensic_log("observables_count", len(observables))
    for obs in observables:
        forensic_log(f"observable_{obs['label']}", obs["pauli"])
        print(f"  {obs['label']} : {obs['pauli']} (coeff={obs['coeff']})")

    # ── 4/7 : Construction circuits (pubs) ───────────────────────────────
    print("\n[4/7] Construction pubs NX48...")
    pre_measure = (alpha2, beta2)
    pubs = build_pubs(observables, nx48_state, pre_measure, n_qubits=n_qubits)
    forensic_log("pubs_total", len(pubs))
    # Dry-run : limiter à 5
    pubs_dryrun = pubs[:5]
    forensic_log("pubs_dryrun", len(pubs_dryrun))
    print(f"  Pubs construits  : {len(pubs)}")
    print(f"  Pubs dry-run     : {len(pubs_dryrun)}")

    # ── 5/7 : SPSA params ────────────────────────────────────────────────
    print("\n[5/7] Paramètres SPSA bi-phasique...")
    spsa_a = 0.1
    spsa_c = 0.05
    spsa_alpha_exp = 0.602
    spsa_gamma_exp = 0.101
    forensic_log("spsa_a", spsa_a)
    forensic_log("spsa_c", spsa_c)
    print(f"  a={spsa_a}  c={spsa_c}  α_exp={spsa_alpha_exp}  γ_exp={spsa_gamma_exp}")

    # ── 6/7 : Run dry-run ────────────────────────────────────────────────
    print("\n[6/7] Dry-run Aer (mode pur Python)...")
    forensic_log("job_type", "pure_python_dry_run")
    t_run_start = time.time()
    results = simulate_estimator_run(pubs_dryrun)
    t_run_end = time.time()
    forensic_log("job_status", "completed")
    forensic_log("job_duration_ms", round((t_run_end - t_run_start) * 1000, 2))

    print(f"  RÉSULTATS : {len(results)} pubs")
    print("")
    print(f"  {'Pub':>4}  {'Observable':>12}  {'EV':>10}  {'Std':>8}")
    print(f"  {'-'*4}  {'-'*12}  {'-'*10}  {'-'*8}")
    for r in results:
        forensic_log(f"result_pub{r['pub_index']}_ev", r["expectation_value"])
        forensic_log(f"result_pub{r['pub_index']}_std", r["std_error"])
        print(f"  {r['pub_index']:>4}  {r['observable']:>12}  {r['expectation_value']:>10.6f}  {r['std_error']:>8.6f}")

    # ── 7/7 : Sauvegarde forensique ──────────────────────────────────────
    print("\n[7/7] Sauvegarde logs forensiques...")
    forensic_log("script_end", "success")
    run_duration = round(time.time() - RUN_START, 3)
    forensic_log("total_duration_s", run_duration)

    base_dir = "src/advanced_calculations/bitcoin_quantum_mining/CHAT"
    csv_path = f"{base_dir}/DONNEES_FORENSIQUES_{RUN_ID}.csv"
    json_path = f"{base_dir}/FORENSIC_{RUN_ID}.json"

    extra = {
        "nx48_state": nx48_state,
        "pre_measure": {"alpha2": alpha2, "beta2": beta2},
        "observables": observables,
        "results": results,
        "run_duration_s": run_duration,
        "dry_run": True,
        "mode": "pure_python"
    }

    save_forensic_csv(csv_path)
    save_forensic_json(json_path, extra)

    print(f"\n{'='*70}")
    print(f"✅ DRY-RUN TERMINÉ — {RUN_ID}")
    print(f"   Durée totale    : {run_duration} s")
    print(f"   Pubs traités    : {len(results)}")
    print(f"   Logs CSV        : {csv_path}")
    print(f"   Logs JSON       : {json_path}")
    print(f"{'='*70}\n")

    return RUN_ID, results, nx48_state, pre_measure, observables, csv_path, json_path

if __name__ == "__main__":
    main()
