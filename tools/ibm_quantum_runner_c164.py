#!/usr/bin/env python3
"""
ibm_quantum_runner_c164.py — Cycle C164

CORRECTIONS BUG-C163 APPLIQUÉES :
  BUG-C163-B1 FIX : 6 qubits (hash[0..5] → 6 angles RY, au lieu de 2 bytes = 6.25%)
  BUG-C163-B2 FIX : nonce variable (--nonce CLI, non plus fixe à 0)
  BUG-C163-B3 FIX : observables S(π) + C(r=1) + ZZ en batch (3 obs, 1 seul pub IBM)
  BUG-C163-B4 FIX : logging forensique complet (job-info.json + forensic CSV)

Stratégie C164 :
  - 1 seul submit IBM (quota minimal : 1 × 600s = 600 IBM-seconds)
  - Backend : ibm_fez (156Q)
  - Adresses : bc1q... (3.1285 BTC) + 1Ny9... (0.00016 BTC)
  - Nonces : variable via --nonce (défaut 0)
  - Observables batch : S(π), C(r=1), ZZ sur 6 qubits
  - Logging forensique C65-C164 complet

Références forensiques C163 :
  - bc1q : hash[0]=44 → angle=31.06° → EVs_C163=-0.4683 (inversion signe 103σ)
  - 1Ny9 : hash[0]=6  → angle=4.24°  → EVs_C163=+0.9392 (saturation ZZ)
  - Objectif C164 : mesurer S(π) + C(r=1) pour voir corrélations AFM réelles

Usage :
  # Dry-run local (Aer simulator, gratuit)
  python3 tools/ibm_quantum_runner_c164.py --dry-run --nonce 0

  # Soumission IBM réelle (1 pub, 600 IBM-seconds)
  python3 tools/ibm_quantum_runner_c164.py --backend ibm_fez --shots 1024 --nonce 0

  # Soumission avec nonce différent (B2 FIX)
  python3 tools/ibm_quantum_runner_c164.py --backend ibm_fez --shots 1024 --nonce 1
"""

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

# ─── Pre-load libstdc++ pour Nix/Ubuntu ───────────────────────────────────────
import ctypes, glob as _glob
for _p in ([
    "/nix/store/bmi5znnqk4kg2grkrhk6py0irc8phf6l-gcc-14.3.0-lib/lib/libstdc++.so.6",
    "/nix/store/bmi5znnqk4kg2grkrhk6py0irc8phf6l-gcc-14.2.1.20250322-lib/lib/libstdc++.so.6",
    "/usr/lib/x86_64-linux-gnu/libstdc++.so.6",
] + sorted(_glob.glob("/nix/store/*/lib/libstdc++.so.6"), reverse=True)[:3]):
    if os.path.exists(_p):
        try:
            ctypes.CDLL(_p, mode=ctypes.RTLD_GLOBAL)
            break
        except OSError:
            pass

# ─── Imports scientifiques ────────────────────────────────────────────────────
try:
    import numpy as np
    HAVE_NP = True
except ImportError:
    HAVE_NP = False

try:
    from qiskit import QuantumCircuit, transpile
    from qiskit.quantum_info import SparsePauliOp
    HAVE_QISKIT = True
except ImportError as _e:
    HAVE_QISKIT = False
    _qiskit_err = str(_e)

try:
    from qiskit_aer import AerSimulator
    from qiskit_aer.primitives import EstimatorV2 as AerEstimator
    HAVE_AER = True
except ImportError:
    HAVE_AER = False

try:
    from qiskit_ibm_runtime import QiskitRuntimeService, EstimatorV2 as IBMEstimator, EstimatorOptions
    HAVE_IBM = True
except ImportError:
    HAVE_IBM = False

# ─── Constantes cycle ─────────────────────────────────────────────────────────
CYCLE   = "C164"
STAMP   = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")

# Résolution robuste du REPO : supporte /tmp, ~/LVX/lumvorax2/tools/, /home/runner/workspace/tools/
_file_path = Path(__file__).resolve()
if _file_path.parent.name == "tools":
    # Cas normal : script dans repo/tools/
    REPO = _file_path.parent.parent
else:
    # Cas /tmp/ ou autre : fallback sur variables d'env ou paths Ubuntu connus
    _repo_env = os.environ.get("REPO_ROOT", "")
    if _repo_env and Path(_repo_env).exists():
        REPO = Path(_repo_env)
    elif Path(os.path.expanduser("~/LVX/lumvorax2")).exists():
        REPO = Path(os.path.expanduser("~/LVX/lumvorax2"))
    elif Path("/home/runner/workspace").exists():
        REPO = Path("/home/runner/workspace")
    else:
        REPO = _file_path.parent.parent  # fallback

BTC_DIR = REPO / "src/advanced_calculations/bitcoin_quantum_mining"
LOG_DIR = BTC_DIR / "logs/log_job_ibm"
FOR_DIR = BTC_DIR / "logs/forensic_c164"
RES_DIR = BTC_DIR / "results"
CACHE   = BTC_DIR / "logs/forensic_c163/mainnet_keys_cache.json"

N_QUBITS = 6  # B1 FIX : 6 qubits au lieu de 2 (6 bytes SHA256)

# ─── Logging simple ───────────────────────────────────────────────────────────
_t0 = time.time()
_forensic_events = []

def _log(tag, msg, val=0.0):
    ts = round(time.time() - _t0, 4)
    entry = {"ts": ts, "tag": tag, "msg": str(msg), "val": float(val), "cycle": CYCLE}
    _forensic_events.append(entry)
    print(f"[{CYCLE}][{ts:8.3f}s] {tag:20s} {msg}", flush=True)

def _ok(tag, msg):   _log(tag, f"✅ {msg}")
def _warn(tag, msg): _log(tag, f"⚠️  {msg}")
def _err(tag, msg):  _log(tag, f"❌ {msg}")

# ─────────────────────────────────────────────────────────────────────────────
# 1. CHARGEMENT ADRESSES MAINNET (cache C163)
# ─────────────────────────────────────────────────────────────────────────────

def load_addresses():
    """Charge les adresses Bitcoin depuis mainnet_keys_cache.json (C163)."""
    if CACHE.exists():
        with open(CACHE) as f:
            data = json.load(f)
        _ok("cache_load", f"{len(data)} adresses chargées depuis {CACHE.name}")
        return data
    _warn("cache_load", f"Cache absent : {CACHE} — utilisation adresses hardcodées C163")
    return [
        {
            "address": "bc1qwzrryqr3ja8w7hnja2spmkgfdcgvqwp5swz4af4ngsjecfz0w0pqud7k38",
            "value": 312850642,
            "tx_hash": "07c6a01350e92611aa006f1f9ad9c2f18564996ccd9f2d8f0ecbdce115b90d0a"
        },
        {
            "address": "1Ny9toPUCkeidGzHBSjq1NJ9GzwS1UWybB",
            "value": 15792,
            "tx_hash": "fc17b6a7f36a770a606267bdda29a1fcce2ef86f950d832c05266e26ce03a956"
        }
    ]

# ─────────────────────────────────────────────────────────────────────────────
# 2. CIRCUIT C164 — BUG-C163-B1 + B2 FIXES
# ─────────────────────────────────────────────────────────────────────────────

def create_circuit_c164(address: str, nonce: int = 0, n_qubits: int = N_QUBITS) -> "QuantumCircuit":
    """
    B1 FIX : utilise hash[0..n_qubits-1] (6 bytes SHA256 = 18.75%, vs 6.25% en C163)
    B2 FIX : nonce variable (pas fixe à 0)

    Circuit :
      Phase 1 — Rotations initiales (n_qubits angles depuis SHA256)
        RY(θ₀)|q0⟩, RY(θ₁)|q1⟩, ..., RY(θ₅)|q5⟩
        θᵢ = (hash[i] / 255) × π

      Phase 2 — Entanglement en chaîne (Heisenberg 1D)
        CNOT(q0→q1), CNOT(q1→q2), ..., CNOT(q4→q5)

    Valeurs SHA256 de référence (nonce=0) :
      bc1q : hash[0..5] = [44, 81, ?, ?, ?, ?]  → angle[0]=31.06°
      1Ny9 : hash[0..5] = [6, 66, ?, ?, ?, ?]   → angle[0]=4.24°
    """
    if not HAVE_QISKIT:
        raise RuntimeError(f"Qiskit indisponible : {_qiskit_err}")

    # SHA256(adresse + nonce) — B2 FIX : nonce variable
    data = f"{address}{nonce}".encode()
    hash_bytes = hashlib.sha256(data).digest()

    # Log des bytes utilisés (forensique)
    bytes_used = [hash_bytes[i] for i in range(n_qubits)]
    angles_deg = [(b / 255.0) * 180.0 for b in bytes_used]
    _log("sha256_bytes",
         f"addr={address[:20]}... nonce={nonce} bytes={bytes_used}",
         float(bytes_used[0]))
    _log("sha256_angles",
         f"angles_deg={[round(a, 2) for a in angles_deg]}",
         float(angles_deg[0]))

    # Information SHA256 utilisée
    info_pct = (n_qubits / 32.0) * 100.0
    _log("sha256_info",
         f"bytes_utilisés={n_qubits}/32 = {info_pct:.2f}% (C163 utilisait 6.25%)",
         info_pct)

    qc = QuantumCircuit(n_qubits)

    # Phase 1 : Rotations initiales B1 FIX (6 angles au lieu de 2)
    for i in range(n_qubits):
        angle = (hash_bytes[i] / 255.0) * math.pi
        qc.ry(angle, i)

    # Phase 2 : Entanglement en chaîne (Heisenberg 1D)
    for i in range(n_qubits - 1):
        qc.cx(i, i + 1)

    return qc

# ─────────────────────────────────────────────────────────────────────────────
# 3. OBSERVABLES — BUG-C163-B3 FIX
# ─────────────────────────────────────────────────────────────────────────────

def build_zz_observable(n_qubits: int = N_QUBITS) -> "SparsePauliOp":
    """ZZ sur q0-q1 (baseline C163 — pour comparaison directe)."""
    if not HAVE_QISKIT:
        raise RuntimeError("Qiskit indisponible")
    pauli = "I" * (n_qubits - 2) + "ZZ"
    return SparsePauliOp([pauli], coeffs=[1.0])

def build_s_pi_observable(n_qubits: int = N_QUBITS) -> "SparsePauliOp":
    """
    S(π) = (1/N²) Σᵢⱼ (-1)^(i-j) Zᵢ Zⱼ  (facteur de structure AFM)
    Pic antiferromagnétique — détecte l'ordre de Néel.
    """
    if not HAVE_QISKIT:
        raise RuntimeError("Qiskit indisponible")
    N = n_qubits
    terms = []
    for i in range(N):
        for j in range(N):
            coeff = ((-1) ** (i - j)) / (N * N)
            if i == j:
                terms.append(("I" * N, coeff))
            else:
                p = ["I"] * N
                p[i] = "Z"
                p[j] = "Z"
                terms.append(("".join(reversed(p)), coeff))
    return SparsePauliOp.from_list(terms).simplify()

def build_c_r_observable(n_qubits: int = N_QUBITS, r: int = 1) -> "SparsePauliOp":
    """
    C(r) = (1/(N-r)) Σᵢ Zᵢ Z_{i+r}  (corrélation voisins)
    r=1 : corrélations entre plus proches voisins.
    """
    if not HAVE_QISKIT:
        raise RuntimeError("Qiskit indisponible")
    N = n_qubits
    terms = []
    n_pairs = max(1, N - r)
    for i in range(N - r):
        p = ["I"] * N
        p[i] = "Z"
        p[i + r] = "Z"
        terms.append(("".join(reversed(p)), 1.0 / n_pairs))
    return SparsePauliOp.from_list(terms).simplify()

def build_s_k_observable(n_qubits: int = N_QUBITS, k: float = math.pi / 2) -> "SparsePauliOp":
    """
    S(k) = (1/N) Σᵢⱼ cos(k(i-j)) Zᵢ Zⱼ  (structure factor à k=π/2)
    """
    if not HAVE_QISKIT:
        raise RuntimeError("Qiskit indisponible")
    N = n_qubits
    terms = []
    for i in range(N):
        for j in range(N):
            coeff = math.cos(k * (i - j)) / N
            if i == j:
                terms.append(("I" * N, coeff))
            else:
                p = ["I"] * N
                p[i] = "Z"
                p[j] = "Z"
                terms.append(("".join(reversed(p)), coeff))
    return SparsePauliOp.from_list(terms).simplify()

def build_all_observables_c164(n_qubits: int = N_QUBITS) -> list:
    """
    B3 FIX : 4 observables en batch (au lieu de ZZ fixe seulement en C163)
      1. ZZ      : baseline C163 (corrélation q0-q1)
      2. S(π)    : facteur de structure AFM (détecte ordre de Néel)
      3. C(r=1)  : corrélation plus proches voisins
      4. S(k=π/2): facteur de structure à k=π/2 (ferromagnétique vs AFM)

    Retourne liste de (nom, SparsePauliOp)
    """
    obs = [
        ("ZZ_q0q1", build_zz_observable(n_qubits)),
        ("S_pi",    build_s_pi_observable(n_qubits)),
        ("C_r1",    build_c_r_observable(n_qubits, r=1)),
        ("S_k_pi2", build_s_k_observable(n_qubits, k=math.pi / 2)),
    ]
    _ok("observables_built",
        f"B3 FIX : {len(obs)} observables (ZZ+S(π)+C(r=1)+S(k=π/2)) au lieu de ZZ fixe")
    return obs

# ─────────────────────────────────────────────────────────────────────────────
# 4. LOGGING FORENSIQUE — BUG-C163-B4 FIX
# ─────────────────────────────────────────────────────────────────────────────

def save_job_forensic(job_id: str, address: str, nonce: int,
                      circuit_info: dict, observables_info: list,
                      results_ibm: list = None):
    """
    B4 FIX : logging forensique complet (job-info.json + forensic CSV)
    Structure identique au standard C65-C160.
    """
    # Créer répertoire job
    job_dir = LOG_DIR / f"job-{job_id}"
    job_dir.mkdir(parents=True, exist_ok=True)

    # job-info.json
    job_info = {
        "job_id": job_id,
        "cycle": CYCLE,
        "stamp_utc": STAMP,
        "address": address,
        "address_short": address[:20] + "...",
        "nonce": nonce,
        "n_qubits": N_QUBITS,
        "backend": circuit_info.get("backend", "ibm_fez"),
        "shots": circuit_info.get("shots", 1024),
        "circuit_depth": circuit_info.get("depth", 0),
        "n_2q_gates": circuit_info.get("n_2q_gates", 0),
        "sha256_bytes": circuit_info.get("sha256_bytes", []),
        "sha256_angles_deg": circuit_info.get("sha256_angles_deg", []),
        "observables": [n for n, _ in (observables_info if observables_info else [])],
        "fixes_applied": ["BUG-C163-B1", "BUG-C163-B2", "BUG-C163-B3", "BUG-C163-B4"],
        "forensic_standard": "C65-C164",
        "results_ibm": results_ibm or [],
    }
    job_info_path = job_dir / f"job-{job_id}-info.json"
    with open(job_info_path, "w") as f:
        json.dump(job_info, f, indent=2)
    _ok("forensic_job_info", f"Sauvegardé → {job_info_path}")

    # forensic CSV
    FOR_DIR.mkdir(parents=True, exist_ok=True)
    csv_path = FOR_DIR / f"forensic_c164_{STAMP}_{job_id[:8]}.csv"
    fieldnames = ["run_id", "timestamp_ns", "tag", "msg", "val", "cycle"]
    with open(csv_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames, extrasaction="ignore")
        writer.writeheader()
        for evt in _forensic_events:
            writer.writerow({
                "run_id": job_id,
                "timestamp_ns": int(evt["ts"] * 1e9),
                "tag": evt["tag"],
                "msg": evt["msg"],
                "val": evt["val"],
                "cycle": CYCLE,
            })
    _ok("forensic_csv", f"Sauvegardé → {csv_path} ({len(_forensic_events)} événements)")

    # results JSON (si disponible)
    if results_ibm:
        result_path = job_dir / f"job-{job_id}-result-c164.json"
        with open(result_path, "w") as f:
            json.dump({
                "job_id": job_id,
                "cycle": CYCLE,
                "stamp_utc": STAMP,
                "address": address,
                "nonce": nonce,
                "n_qubits": N_QUBITS,
                "results": results_ibm,
                "forensic_standard": "C65-C164",
                "fixes_applied": ["BUG-C163-B1", "BUG-C163-B2", "BUG-C163-B3", "BUG-C163-B4"],
                "reference_c163": {
                    "bc1q_evs": -0.4682646259127003,
                    "bc1q_stds": 0.012670,
                    "1ny9_evs": 0.9391773003598551,
                    "1ny9_stds": 0.005075,
                    "bell_evs": 0.9631722050465082,
                },
            }, f, indent=2)
        _ok("forensic_result", f"Résultats → {result_path}")

    return job_dir

# ─────────────────────────────────────────────────────────────────────────────
# 5. SOUMISSION IBM
# ─────────────────────────────────────────────────────────────────────────────

def submit_ibm_job(qc, observables, backend_name="ibm_fez",
                   shots=1024, resilience=1):
    """Soumet 1 pub IBM avec 4 observables en batch."""
    if not HAVE_IBM:
        raise RuntimeError("qiskit_ibm_runtime indisponible")

    api_key = os.environ.get("IBM_API_KEY") or os.environ.get("IBM_QUANTUM_TOKEN")
    if not api_key:
        raise RuntimeError("IBM_API_KEY absent des secrets (Doppler ou env)")
    _log("ibm_token_len", f"IBM_API_KEY len={len(api_key)}", float(len(api_key)))

    # CRN d'instance IBM Cloud (compte b893e3b64af743f9860d3ae175578d19)
    crn = os.environ.get(
        "IBM_CRN",
        "crn:v1:bluemix:public:quantum-computing:us-east:"
        "a/b893e3b64af743f9860d3ae175578d19:"
        "31cb02b4-5010-422a-a999-d6302e4d7b06::"
    )
    _log("ibm_connect", f"Connexion QiskitRuntimeService → {backend_name} [ibm_cloud]...")
    _log("ibm_crn_len", f"IBM_CRN len={len(crn)}", float(len(crn)))

    # Connexion adaptative : gère 0.28.x et 0.46.x de qiskit-ibm-runtime
    import qiskit_ibm_runtime as _qir
    _qir_ver = tuple(int(x) for x in _qir.__version__.split(".")[:2])
    _log("ibm_runtime_ver", f"qiskit-ibm-runtime={_qir.__version__} parsed={_qir_ver}")

    _channels_to_try = ["ibm_cloud", "ibm_quantum_platform"]
    service = None
    _last_err = None
    for _ch in _channels_to_try:
        for _use_save in (False, True):
            try:
                if _use_save:
                    _log("ibm_connect_try", f"Méthode save_account channel={_ch}")
                    QiskitRuntimeService.save_account(
                        channel=_ch, token=api_key, instance=crn,
                        overwrite=True, set_as_default=True,
                    )
                    service = QiskitRuntimeService(channel=_ch)
                else:
                    _log("ibm_connect_try", f"Méthode directe channel={_ch}")
                    service = QiskitRuntimeService(
                        channel=_ch, token=api_key, instance=crn,
                    )
                _ok("ibm_connected", f"Connecté via channel={_ch} save={_use_save}")
                break
            except Exception as _e:
                _log("ibm_connect_fail", f"channel={_ch} save={_use_save} → {type(_e).__name__}: {_e}")
                _last_err = _e
                service = None
        if service is not None:
            break
    if service is None:
        raise RuntimeError(f"Impossible de se connecter à IBM Cloud : {_last_err}")

    backend = service.backend(backend_name)
    _ok("ibm_backend", f"{backend_name} ({backend.num_qubits}Q)")
    _log("ibm_backend_qubits", backend_name, float(backend.num_qubits))

    # Transpilation
    _log("transpile_start", f"Transpilation {qc.num_qubits}Q → {backend_name}...")
    qc_t = transpile(qc, backend, optimization_level=1)
    depth = qc_t.depth()
    n_2q = sum(1 for instr in qc_t.data if instr.operation.num_qubits == 2)
    _ok("transpile_done", f"depth={depth} 2Q={n_2q}")
    _log("transpile_depth", f"depth={depth}", float(depth))
    _log("transpile_2q", f"n_2q={n_2q}", float(n_2q))

    # apply_layout sur chaque observable (B1 FIX : 6 qubits, pas 2)
    obs_names = [name for name, _ in observables]
    obs_list = []
    for name, obs_raw in observables:
        obs_extended = obs_raw.apply_layout(qc_t.layout)
        obs_list.append(obs_extended)
        _log("apply_layout", f"{name} → {obs_extended.num_qubits}Q physiques")

    # Options EstimatorV2
    options = EstimatorOptions()
    options.default_shots = shots
    options.resilience_level = resilience
    _log("estimator_opts", f"shots={shots} resilience={resilience}")

    estimator = IBMEstimator(mode=backend, options=options)

    # 1 seul pub : (circuit_transpiré, [obs1, obs2, obs3, obs4])
    _log("submit_start", f"Soumission 1 pub, {len(obs_list)} observables...")
    t_submit = time.time()
    job = estimator.run([(qc_t, obs_list)])
    job_id = job.job_id()
    _ok("submit_done", f"Job ID = {job_id}")
    _log("job_id", job_id, 0.0)

    return job, job_id, qc_t, {"depth": depth, "n_2q_gates": n_2q}, obs_names

# ─────────────────────────────────────────────────────────────────────────────
# 6. DRY-RUN AER
# ─────────────────────────────────────────────────────────────────────────────

def dry_run_aer(qc, observables):
    """Simulation locale sur AerSimulator (gratuit, valide l'architecture)."""
    if not HAVE_AER:
        raise RuntimeError("qiskit-aer indisponible")

    _log("aer_start", f"Dry-run AerSimulator {qc.num_qubits}Q...")
    aer_sim = AerSimulator()
    qc_t = transpile(qc, aer_sim, optimization_level=1)
    _ok("aer_transpile", f"depth={qc_t.depth()}")

    obs_list = [obs for _, obs in observables]
    estimator = AerEstimator()
    job = estimator.run([(qc_t, obs_list)])
    result = job.result()

    evs = result[0].data.evs
    if hasattr(evs, "__len__"):
        evs = list(evs)
    else:
        evs = [float(evs)]

    results = []
    for i, (name, _) in enumerate(observables):
        val = float(evs[i]) if i < len(evs) else 0.0
        _ok(f"aer_{name}", f"EVs={val:+.6f}")
        results.append({"name": name, "evs_aer": val, "mode": "dry_run_aer"})

    return results

# ─────────────────────────────────────────────────────────────────────────────
# 7. MAIN
# ─────────────────────────────────────────────────────────────────────────────

def main():
    ap = argparse.ArgumentParser(description="IBM Quantum Runner — Cycle C164")
    ap.add_argument("--backend",    default="ibm_fez",
                    help="Backend IBM (défaut: ibm_fez)")
    ap.add_argument("--shots",      type=int, default=1024,
                    help="Shots par circuit (défaut: 1024)")
    ap.add_argument("--resilience", type=int, default=1,
                    help="Niveau resilience IBM (0-2, défaut: 1)")
    ap.add_argument("--nonce",      type=int, default=0,
                    help="B2 FIX : nonce SHA256 (défaut: 0)")
    ap.add_argument("--addr-idx",   type=int, default=0,
                    help="Index adresse dans le cache (0=bc1q, 1=1Ny9, défaut: 0)")
    ap.add_argument("--n-qubits",   type=int, default=N_QUBITS,
                    help=f"B1 FIX : nombre de qubits (défaut: {N_QUBITS})")
    ap.add_argument("--dry-run",    action="store_true",
                    help="Simulation locale Aer (pas de quota IBM)")
    ap.add_argument("--all-addresses", action="store_true",
                    help="Soumettre 1 job par adresse du cache (dry-run seulement)")
    args = ap.parse_args()

    print(f"\n{'='*72}")
    print(f"IBM QUANTUM RUNNER — Cycle {CYCLE}")
    print(f"{'='*72}")
    print(f"  Backend     : {args.backend}")
    print(f"  Shots       : {args.shots}")
    print(f"  Resilience  : {args.resilience}")
    print(f"  Nonce       : {args.nonce}  [B2 FIX]")
    print(f"  N qubits    : {args.n_qubits}  [B1 FIX : 6 vs 2 en C163]")
    print(f"  Mode        : {'DRY-RUN AER' if args.dry_run else 'SUBMIT IBM RÉEL'}")
    print(f"  Stamp UTC   : {STAMP}")
    print(f"{'='*72}\n")

    _log("script_start", f"ibm_quantum_runner_c164.py nonce={args.nonce}", float(args.nonce))

    if not HAVE_QISKIT:
        _err("qiskit_missing", f"Qiskit indisponible : {_qiskit_err}")
        sys.exit(1)

    # ── Chargement adresses ──────────────────────────────────────────────────
    print("[1/5] Chargement adresses mainnet...")
    addresses = load_addresses()

    if args.all_addresses and args.dry_run:
        # Mode : dry-run toutes les adresses
        for i, addr_data in enumerate(addresses):
            addr = addr_data["address"]
            val_btc = addr_data["value"] / 1e8
            print(f"\n  === Adresse {i+1}/{len(addresses)} : {addr[:30]}... ({val_btc:.8f} BTC) ===")
            qc = create_circuit_c164(addr, args.nonce, args.n_qubits)
            obs = build_all_observables_c164(args.n_qubits)
            results = dry_run_aer(qc, obs)
            # Log forensique partiel
            save_job_forensic(
                job_id=f"dry_{STAMP}_{i}",
                address=addr,
                nonce=args.nonce,
                circuit_info={"backend": "aer_dry", "shots": 0, "depth": 0},
                observables_info=obs,
                results_ibm=results,
            )
        print(f"\n[C164] Dry-run toutes adresses terminé.")
        return 0

    # ── Sélection adresse ────────────────────────────────────────────────────
    if args.addr_idx >= len(addresses):
        _err("addr_idx", f"Index {args.addr_idx} hors limites (max {len(addresses)-1})")
        sys.exit(1)

    addr_data = addresses[args.addr_idx]
    address   = addr_data["address"]
    val_satoshi = addr_data.get("value", 0)
    val_btc   = val_satoshi / 1e8
    tx_hash   = addr_data.get("tx_hash", "unknown")

    print(f"\n  Adresse sélectionnée ({args.addr_idx}) :")
    print(f"    {address}")
    print(f"    Valeur : {val_btc:.8f} BTC ({val_satoshi} satoshis)")
    print(f"    TX hash: {tx_hash[:32]}...")
    _log("address_selected", address[:30], float(args.addr_idx))
    _log("address_value_btc", f"val={val_btc:.8f}", val_btc)

    # ── Construction circuit C164 (B1+B2 FIX) ───────────────────────────────
    print(f"\n[2/5] Construction circuit C164 ({args.n_qubits}Q, nonce={args.nonce})...")
    qc = create_circuit_c164(address, args.nonce, args.n_qubits)
    _ok("circuit_built", f"{qc.num_qubits}Q circuit — {qc.size()} gates")
    _log("circuit_size", f"qubits={qc.num_qubits} gates={qc.size()}", float(qc.size()))

    # Calcul SHA256 pour log forensique
    sha_data = f"{address}{args.nonce}".encode()
    sha_bytes = hashlib.sha256(sha_data).digest()
    bytes_used  = [sha_bytes[i] for i in range(args.n_qubits)]
    angles_deg  = [(b / 255.0) * 180.0 for b in bytes_used]

    circuit_info = {
        "backend":        args.backend,
        "shots":          args.shots,
        "sha256_bytes":   bytes_used,
        "sha256_angles_deg": [round(a, 3) for a in angles_deg],
        "depth":          qc.depth(),
        "n_2q_gates":     sum(1 for i in qc.data if i.operation.num_qubits == 2),
    }

    print(f"    SHA256 bytes[0..{args.n_qubits-1}] : {bytes_used}")
    print(f"    Angles (degrés)              : {[round(a, 2) for a in angles_deg]}")
    print(f"    Info SHA256 utilisée         : {args.n_qubits}/32 bytes = "
          f"{args.n_qubits/32*100:.2f}% [B1 FIX vs 6.25% en C163]")

    # ── Construction observables C164 (B3 FIX) ──────────────────────────────
    print(f"\n[3/5] Construction observables C164 (B3 FIX : 4 obs vs ZZ fixe)...")
    observables = build_all_observables_c164(args.n_qubits)
    for name, obs in observables:
        _log("obs_terms", f"{name} : {len(obs)} termes Pauli", float(len(obs)))

    # ── Dry-run ou IBM réel ───────────────────────────────────────────────────
    if args.dry_run:
        print(f"\n[4/5] Dry-run AerSimulator (gratuit)...")
        if not HAVE_AER:
            _err("aer_missing", "qiskit-aer non disponible pour dry-run")
            sys.exit(1)
        results = dry_run_aer(qc, observables)
        _ok("dry_run_done", f"{len(results)} résultats Aer")

        print(f"\n[5/5] Logging forensique B4 FIX...")
        save_job_forensic(
            job_id=f"dry_{STAMP}",
            address=address,
            nonce=args.nonce,
            circuit_info=circuit_info,
            observables_info=observables,
            results_ibm=results,
        )
        print(f"\n✅ DRY-RUN C164 TERMINÉ")
        print(f"   {len(results)} observables calculés sur AerSimulator")
        for r in results:
            print(f"   {r['name']:12s} EVs={r['evs_aer']:+.6f}")
        return 0

    else:
        # Soumission IBM réelle
        print(f"\n[4/5] Soumission IBM Quantum (backend={args.backend})...")
        if not HAVE_IBM:
            _err("ibm_missing", "qiskit_ibm_runtime non disponible")
            sys.exit(1)

        try:
            job, job_id, qc_t, trans_info, obs_names = submit_ibm_job(
                qc, observables,
                backend_name=args.backend,
                shots=args.shots,
                resilience=args.resilience,
            )
            circuit_info.update(trans_info)
        except Exception as e:
            _err("submit_fail", str(e)[:300])
            # Log forensique même en cas d'erreur
            save_job_forensic(
                job_id=f"err_{STAMP}",
                address=address,
                nonce=args.nonce,
                circuit_info=circuit_info,
                observables_info=observables,
                results_ibm=[{"error": str(e)}],
            )
            sys.exit(1)

        print(f"\n[5/5] Logging forensique B4 FIX...")
        save_job_forensic(
            job_id=job_id,
            address=address,
            nonce=args.nonce,
            circuit_info=circuit_info,
            observables_info=observables,
            results_ibm=[],  # sera rempli à la récupération
        )

        print(f"\n✅ JOB IBM SOUMIS — Cycle {CYCLE}")
        print(f"   Job ID     : {job_id}")
        print(f"   Backend    : {args.backend}")
        print(f"   Adresse    : {address[:30]}...")
        print(f"   Valeur BTC : {val_btc:.8f} BTC ({val_satoshi} satoshis)")
        print(f"   Nonce      : {args.nonce}  [B2 FIX]")
        print(f"   N qubits   : {args.n_qubits}  [B1 FIX]")
        print(f"   Observables: {obs_names}  [B3 FIX]")
        print(f"\n   Pour récupérer les résultats :")
        print(f"   python3 tools/retrieve_ibm_job_results.py {job_id}")

        _log("job_submitted_final", job_id, 0.0)

        # Sauvegarder le job_id dans un fichier de tracking global
        LOG_DIR.mkdir(parents=True, exist_ok=True)
        tracking_path = LOG_DIR / "c164_jobs.json"
        tracking = []
        if tracking_path.exists():
            with open(tracking_path) as f:
                tracking = json.load(f)
        tracking.append({
            "job_id": job_id,
            "cycle": CYCLE,
            "stamp_utc": STAMP,
            "address": address,
            "addr_short": address[:20] + "...",
            "value_btc": val_btc,
            "nonce": args.nonce,
            "n_qubits": args.n_qubits,
            "backend": args.backend,
            "shots": args.shots,
            "observables": obs_names,
            "fixes": ["B1", "B2", "B3", "B4"],
        })
        with open(tracking_path, "w") as f:
            json.dump(tracking, f, indent=2)
        _ok("tracking_saved", f"{tracking_path}")

        return 0


if __name__ == "__main__":
    sys.exit(main())
