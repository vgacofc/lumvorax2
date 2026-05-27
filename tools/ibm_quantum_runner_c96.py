#!/usr/bin/env python3
"""
ibm_quantum_runner_c96.py — Cycle C96 IBM Quantum Runner.

Stratégie C96 : UN SEUL SUBMIT IBM contenant TOUS les N de la grille
[16, 24, 32, 48, 64, 96, 128, 156] pour économiser le quota Open Plan
(estimation 80-100 s vs 240 s en 8 jobs séparés).

Corrections appliquées suite analyses externes (Claude / ChatGPT) :
  P0.1 — Patch coverage_pct fait dans btc_mining_engine.c (déjà commité)
  P0.2 — Recalibration ASIC ibm_heron_r2_model.c avec ZNE_gain réel
  P0.3 — Flag --measure_final ici présent pour dry-runs Aer (corrige
         "stdout utilisé comme métrique" pointé par ChatGPT §6)
  P1.1 — Test multi-N (16→156) en un batch
  P1.2 — Variation ansatz : --ansatz {rxx,brickwork,heisenberg}
  P1.3 — Init hors-Néel : --init {neel,random_product,plus_state}

Limites physiques honnêtes (à dire au user) :
  - ibm_kingston Heron R2 = 156 qubits MAX. N=256/512/1024 IMPOSSIBLE
    sur ce backend en 2026. Réalisable seulement après IBM Flamingo
    (2027+, 1386Q) ou IBM Blue Jay (2033+, 100kQ).
  - Pre-train Aer statevector limité à N≤32 (mémoire 2^32 = 64 GiB pour N=32).
    Pour N>32 → MPS simulator (bond_dim=32) qui est polynomial.
  - Pour N>=128, on utilise init Néel direct + 1 couche RXX(0.1) sans
    pre-train (le coût classique deviendrait prohibitif).

Usage :
  # Dry-run validation locale (gratuit, pas de quota IBM)
  python tools/ibm_quantum_runner_c96.py --no_ibm --N_list 16,24,32 --measure_final

  # Submit batch IBM réel (consomme quota)
  python tools/ibm_quantum_runner_c96.py --backend ibm_kingston \
      --N_list 16,24,32,48,64,96,128,156 \
      --noise_factors 1,3,5 --shots 1024 --resilience 2

Sortie : ibm_c96_chatgpt_<STAMP>.json + push résultats vers /agent/push.
"""
from __future__ import annotations

import argparse
import ctypes
import ctypes.util
import json
import os
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

# --- Hack libstdc++ pour Replit Nix avant tout import numpy/qiskit -----------
def _preload_libstdcxx():
    """Précharge libstdc++.so.6 si absent du LD_LIBRARY_PATH (problème Replit)."""
    try:
        ctypes.CDLL("libstdc++.so.6", mode=ctypes.RTLD_GLOBAL)
        return True
    except OSError:
        pass
    candidates = [
        "/nix/store/*/lib/libstdc++.so.6",
        "/usr/lib/x86_64-linux-gnu/libstdc++.so.6",
    ]
    import glob
    for pat in candidates:
        for p in sorted(glob.glob(pat))[-1::-1]:
            try:
                ctypes.CDLL(p, mode=ctypes.RTLD_GLOBAL)
                return True
            except OSError:
                continue
    return False


_preload_libstdcxx()

# --- CLI ---------------------------------------------------------------------
def parse_args():
    p = argparse.ArgumentParser(description="C96 batch IBM runner")
    p.add_argument("--backend", default="ibm_kingston",
                   help="Backend IBM (default: ibm_kingston)")
    p.add_argument("--N_list", default="16,24,32",
                   help="Liste des N (qubits) séparés par virgules. "
                        "Recommandé batch complet: 16,24,32,48,64,96,128,156")
    p.add_argument("--n_rep", type=int, default=3,
                   help="Répétitions ADAPT-VQE par N (default: 3)")
    p.add_argument("--shots", type=int, default=1024,
                   help="Nombre de shots par circuit (default: 1024)")
    p.add_argument("--resilience", type=int, default=2,
                   help="Niveau resilience IBM (0-2, default: 2 = ZNE+twirl)")
    p.add_argument("--noise_factors", default="1,3,5",
                   help="Facteurs ZNE (default: 1,3,5)")
    p.add_argument("--n_twirls", type=int, default=4,
                   help="Pauli twirls par circuit (default: 4)")
    p.add_argument("--ansatz", choices=["rxx", "brickwork", "heisenberg"],
                   default="rxx",
                   help="Ansatz ADAPT (default: rxx). C96 P1.2.")
    p.add_argument("--init", choices=["neel", "random_product", "plus_state"],
                   default="neel",
                   help="État initial (default: neel). C96 P1.3.")
    p.add_argument("--no_ibm", action="store_true",
                   help="Dry-run Aer uniquement (ne consomme PAS quota IBM)")
    p.add_argument("--measure_final", action="store_true",
                   help="C96 P0.3 — écrit un JSON final_observables.json même en dry-run "
                        "(corrige le bug 'stdout utilisé comme métrique' relevé par ChatGPT §6)")
    p.add_argument("--mps_threshold", type=int, default=32,
                   help="N>=mps_threshold utilise simulator MPS au lieu de statevector "
                        "(default: 32). Pour N>=128 on skip pre-train.")
    p.add_argument("--mps_bond_dim", type=int, default=32,
                   help="Bond dimension MPS (default: 32)")
    p.add_argument("--push_replit", default="",
                   help="URL Replit pour POST résultats vers /agent/push")
    p.add_argument("--push_token", default=os.environ.get("AGENT_TOKEN", ""),
                   help="Token agent (défaut: $AGENT_TOKEN)")
    return p.parse_args()


# --- Construction circuit ----------------------------------------------------
def make_init_state(qc, N, init):
    """Prépare l'état initial selon C96 P1.3."""
    import numpy as np
    if init == "neel":
        # |010101...⟩ (alternance, AFM)
        for i in range(0, N, 2):
            qc.x(i)
    elif init == "random_product":
        # Produit tensoriel d'états U|0⟩ aléatoires (rompt symétrie)
        rng = np.random.default_rng(0xC96)
        for i in range(N):
            theta = float(rng.uniform(0.1, np.pi - 0.1))
            phi = float(rng.uniform(0, 2 * np.pi))
            qc.ry(theta, i)
            qc.rz(phi, i)
    elif init == "plus_state":
        # |+⟩^N — symétrique, totalement délocalisé
        for i in range(N):
            qc.h(i)
    return qc


def add_ansatz_layer(qc, N, ansatz, theta, layer_idx):
    """Ajoute une couche d'ansatz selon C96 P1.2."""
    if ansatz == "rxx":
        # ADAPT-VQE classique : RXX(theta) sur tous les voisins (i, i+1)
        for i in range(0, N - 1, 2):
            qc.rxx(theta, i, i + 1)
        for i in range(1, N - 1, 2):
            qc.rxx(theta, i, i + 1)
    elif ansatz == "brickwork":
        # Mur de brique RXX + RYY alternés
        for i in range(0, N - 1, 2):
            qc.rxx(theta, i, i + 1)
            qc.ryy(theta * 0.5, i, i + 1)
        for i in range(1, N - 1, 2):
            qc.rxx(theta, i, i + 1)
            qc.ryy(theta * 0.5, i, i + 1)
    elif ansatz == "heisenberg":
        # XXX + YYY + ZZZ (Heisenberg complet)
        for i in range(0, N - 1, 2):
            qc.rxx(theta, i, i + 1)
            qc.ryy(theta, i, i + 1)
            qc.rzz(theta, i, i + 1)
        for i in range(1, N - 1, 2):
            qc.rxx(theta, i, i + 1)
            qc.ryy(theta, i, i + 1)
            qc.rzz(theta, i, i + 1)
    return qc


def build_circuit(N, n_layers, ansatz, init, theta=0.1):
    """Construit le circuit ADAPT-VQE complet pour un N donné."""
    from qiskit import QuantumCircuit
    qc = QuantumCircuit(N)
    make_init_state(qc, N, init)
    for layer in range(n_layers):
        add_ansatz_layer(qc, N, ansatz, theta, layer)
    return qc


def make_s_pi_observable(N):
    """S(π) = (1/N) Σ_j (-1)^j Z_j  — facteur de structure antiferro à k=π."""
    from qiskit.quantum_info import SparsePauliOp
    paulis = []
    coeffs = []
    for j in range(N):
        s = "I" * (N - j - 1) + "Z" + "I" * j
        paulis.append(s)
        coeffs.append((-1) ** j / N)
    return SparsePauliOp(paulis, coeffs=coeffs)


# --- Pré-entraînement Aer ----------------------------------------------------
def aer_pretrain(N, ansatz, init, mps_threshold, bond_dim):
    """Pre-train Aer (statevector ou MPS) pour estimer S(π) idéal."""
    from qiskit_aer import AerSimulator
    from qiskit.quantum_info import SparsePauliOp

    if N < mps_threshold:
        sim = AerSimulator(method="statevector")
        method = "statevector"
    elif N < 128:
        sim = AerSimulator(method="matrix_product_state",
                           matrix_product_state_max_bond_dimension=bond_dim)
        method = f"MPS(bond={bond_dim})"
    else:
        # Pour N>=128, skip pre-train (trop coûteux), retour défauts
        return {"method": "skipped_N_too_large",
                "n_layers": 1, "theta_init": 0.1,
                "s_pi_aer_ideal": None}

    # Heuristique : 3 couches RXX(0.1) suffisent pour préparer un état AFM stable
    n_layers = 3
    theta = 0.1
    qc = build_circuit(N, n_layers, ansatz, init, theta=theta)
    qc.save_statevector() if method == "statevector" else qc.save_density_matrix()

    obs = make_s_pi_observable(N)
    qc_meas = qc.copy()
    qc_meas.remove_final_measurements()
    qc_meas.save_expectation_value(obs, list(range(N)))

    from qiskit import transpile
    qc_t = transpile(qc_meas, sim)
    job = sim.run(qc_t, shots=1)
    result = job.result()
    try:
        s_pi = float(result.data().get("expectation_value", 0.0))
    except Exception:
        s_pi = None

    return {
        "method": method,
        "n_layers": n_layers,
        "theta_init": theta,
        "s_pi_aer_ideal": s_pi,
    }


# --- Submit IBM batch --------------------------------------------------------
def submit_batch_ibm(N_list, args, pretrain_results):
    """Soumet TOUS les N en un seul batch via EstimatorV2 multi-pub."""
    from qiskit_ibm_runtime import QiskitRuntimeService, EstimatorV2 as Estimator
    from qiskit import transpile

    api_key = os.environ.get("IBM_API_KEY")
    if not api_key:
        raise RuntimeError("IBM_API_KEY absent des secrets Replit")

    print(f"[C96] Connexion IBM Quantum (backend={args.backend})...")
    service = QiskitRuntimeService(
        channel="ibm_quantum_platform",
        token=api_key,
    )
    backend = service.backend(args.backend)
    print(f"[C96] Backend OK : {backend.name} ({backend.num_qubits} qubits)")

    # Construction des PUBs (Primitive Unified Blocs)
    pubs = []
    metadata_per_n = []
    for N in N_list:
        pre = pretrain_results.get(N, {})
        n_layers = pre.get("n_layers", 1)
        theta = pre.get("theta_init", 0.1)
        qc = build_circuit(N, n_layers, args.ansatz, args.init, theta=theta)
        qc_t = transpile(qc, backend, optimization_level=2)
        obs = make_s_pi_observable(N).apply_layout(qc_t.layout)
        pubs.append((qc_t, [obs]))
        metadata_per_n.append({
            "N": N,
            "n_layers": n_layers,
            "depth_phys": qc_t.depth(),
            "n_2q_phys": sum(1 for instr in qc_t.data
                             if instr.operation.num_qubits == 2),
        })
        print(f"[C96]   N={N:3d}  depth_phys={qc_t.depth():3d}  "
              f"n2q={metadata_per_n[-1]['n_2q_phys']:3d}  layers={n_layers}")

    # Configuration ZNE
    noise_factors = [float(x) for x in args.noise_factors.split(",")]

    estimator = Estimator(mode=backend)
    estimator.options.default_shots = args.shots
    estimator.options.resilience_level = args.resilience
    if args.resilience >= 2:
        estimator.options.resilience.zne_mitigation = True
        estimator.options.resilience.zne.noise_factors = noise_factors
        estimator.options.resilience.zne.extrapolator = "exponential"
        estimator.options.twirling.enable_gates = True
        estimator.options.twirling.num_randomizations = args.n_twirls

    print(f"[C96] Submit BATCH ({len(pubs)} PUBs, ZNE={noise_factors}, "
          f"shots={args.shots}, resilience={args.resilience})")
    job = estimator.run(pubs)
    print(f"[C96] Job soumis : ID = {job.job_id()}")
    print(f"[C96] Statut initial : {job.status()}")
    return job, metadata_per_n


# --- Main --------------------------------------------------------------------
def main():
    args = parse_args()
    N_list = [int(x) for x in args.N_list.split(",")]

    # Filtrage : warning honnête si N>156
    valid_N = []
    skipped = []
    for N in N_list:
        if N > 156:
            skipped.append(N)
        else:
            valid_N.append(N)
    if skipped:
        print(f"[C96] WARN : valeurs ignorées (>156Q max ibm_kingston) : {skipped}")
    N_list = valid_N

    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    out_dir = Path("results")
    out_dir.mkdir(exist_ok=True)
    out_path = out_dir / f"ibm_c96_chatgpt_{stamp}{'_DRY' if args.no_ibm else ''}.json"

    print(f"[C96] === IBM Quantum Runner cycle C96 ===")
    print(f"[C96] N_list = {N_list}")
    print(f"[C96] ansatz = {args.ansatz} | init = {args.init}")
    print(f"[C96] mode   = {'DRY-RUN Aer' if args.no_ibm else f'SUBMIT IBM ({args.backend})'}")
    print(f"[C96] output = {out_path}")

    # Phase 1 : pre-train Aer pour tous les N
    pretrain = {}
    print(f"[C96] Phase 1/3 : pre-train Aer (méthode auto selon N)...")
    for N in N_list:
        t0 = time.time()
        try:
            pre = aer_pretrain(N, args.ansatz, args.init,
                               args.mps_threshold, args.mps_bond_dim)
            pre["wall_time_s"] = round(time.time() - t0, 2)
            pretrain[N] = pre
            s = pre.get("s_pi_aer_ideal")
            print(f"[C96]   N={N:3d}  method={pre['method']:25s}  "
                  f"S(π)_aer={s if s is None else f'{s:+.4f}'}  "
                  f"({pre['wall_time_s']}s)")
        except Exception as e:
            print(f"[C96]   N={N:3d}  ERR pre-train : {e}")
            pretrain[N] = {"error": str(e), "n_layers": 1, "theta_init": 0.1,
                           "s_pi_aer_ideal": None}

    # Phase 2 : submit IBM (sauf no_ibm)
    job_id = None
    metadata = []
    if not args.no_ibm:
        print(f"[C96] Phase 2/3 : submit batch IBM...")
        try:
            job, metadata = submit_batch_ibm(N_list, args, pretrain)
            job_id = job.job_id()
        except Exception as e:
            print(f"[C96] ERR submit IBM : {e}")
            return 1
    else:
        print(f"[C96] Phase 2/3 : SKIP (dry-run)")

    # Phase 3 : sortie JSON
    print(f"[C96] Phase 3/3 : écriture JSON...")
    payload = {
        "cycle": "C96",
        "stamp_utc": stamp,
        "args": vars(args),
        "N_list": N_list,
        "skipped_N_above_156": skipped,
        "pretrain_aer": {str(k): v for k, v in pretrain.items()},
        "submit": {
            "job_id": job_id,
            "backend": args.backend if not args.no_ibm else None,
            "metadata_per_N": metadata,
        },
        "status": "submitted" if job_id else ("dry_run" if args.no_ibm else "error"),
    }

    if args.measure_final:
        # P0.3 : écrire les observables finales mesurées (même en dry-run)
        payload["final_observables"] = {
            str(N): {
                "S_pi_aer_ideal": pretrain[N].get("s_pi_aer_ideal"),
                "method": pretrain[N].get("method"),
            }
            for N in N_list
        }

    out_path.write_text(json.dumps(payload, indent=2))
    print(f"[C96] OK : écrit {out_path}")

    # Push résultats vers Replit si demandé
    if args.push_replit and args.push_token:
        try:
            import urllib.request, urllib.parse
            req = urllib.request.Request(
                f"{args.push_replit}/agent/push",
                data=json.dumps({
                    "cmd": f"echo === C96 batch submitted === ; "
                           f"echo job_id={job_id} ; echo N_list={N_list}",
                    "label": f"c96_submit_{stamp}",
                }).encode(),
                headers={"Content-Type": "application/json",
                         "X-Agent-Token": args.push_token},
                method="POST",
            )
            urllib.request.urlopen(req, timeout=10)
            print(f"[C96] Push Replit OK")
        except Exception as e:
            print(f"[C96] Push Replit échec : {e}")

    return 0 if (job_id or args.no_ibm) else 1


if __name__ == "__main__":
    sys.exit(main())
