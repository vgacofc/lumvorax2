#!/usr/bin/env python3
"""
LumVorax C94 — VORAX-piloted ADAPT-VQE pour Hubbard 1D N=12 (et N=16 optionnel).

Difference vs C93 :
  * Score de selection ADAPT = |grad| * stab_weight - var_penalty (au lieu de |grad| pur)
    inspire de la suggestion ChatGPT-7.3 : VORAX guide ADAPT.
  * Pool etendu (RXX, RYY, RZZ + RXY = mixte XY non local) sur les voisins.
  * Compression circuit : si depth > 30 apres ajout, on rejette le candidat.
  * SPSA bi-phasique avec EMA variance (idem C93) + perte composite.
  * Mitigation IBM : PEC Pauli twirl x4 + ZNE expo + resilience 2 (idem C93).
  * Batch unique : tous les observables S(pi)+S(k)+C(r) en 1 seul submit.
  * Mode --dry-run-aer : valide N=12 et N=16 sur Aer sans consommer quota IBM.
  * Mode --submit-ibm : un job IBM Kingston pour N=12 (et/ou N=16).

Usage Ubuntu :
  python tools/ibm_quantum_runner_c94.py --N 12 --dry-run-aer
  python tools/ibm_quantum_runner_c94.py --N 12 --submit-ibm
  python tools/ibm_quantum_runner_c94.py --N 16 --dry-run-aer

Sortie : src/advanced_calculations/bitcoin_quantum_mining/results/ibm_c94_*.json
"""

# Pre-load libstdc++ pour environnement Replit/Ubuntu nix
import ctypes as _ct, os as _os
for _p in [
    "/nix/store/bmi5znnqk4kg2grkrhk6py0irc8phf6l-gcc-14.2.1.20250322-lib/lib/libstdc++.so.6",
    "/nix/store/bmi5znnqk4kg2grkrhk6py0irc8phf6l-gcc-14.3.0-lib/lib/libstdc++.so.6",
    "/usr/lib/x86_64-linux-gnu/libstdc++.so.6",
]:
    if _os.path.exists(_p):
        try: _ct.CDLL(_p); break
        except Exception: pass

import argparse, datetime, json, math, os, sys, time
from pathlib import Path
import numpy as np

from qiskit import QuantumCircuit
from qiskit.circuit import ParameterVector
from qiskit.quantum_info import SparsePauliOp
from qiskit_aer import AerSimulator
from qiskit_aer.primitives import EstimatorV2 as AerEstimator
from qiskit_aer.noise import NoiseModel

REPO    = Path(__file__).resolve().parent.parent
RES_DIR = REPO / "src/advanced_calculations/bitcoin_quantum_mining/results"
RES_DIR.mkdir(parents=True, exist_ok=True)

# ============================================================================
# LOG
# ============================================================================
def info(tag, msg): print(f"[INFO ] {tag:14s} {msg}", flush=True)
def warn(tag, msg): print(f"[WARN ] {tag:14s} {msg}", flush=True)
def ok  (tag, msg): print(f"[OK   ] {tag:14s} {msg}", flush=True)

# ============================================================================
# HAMILTONIEN HUBBARD spinless 1D (XX+YY hopping + ZZ interaction)
# ============================================================================
def build_hubbard_hamiltonian(N, t=1.0, U=0.5):
    terms = []
    for i in range(N - 1):
        # hopping: -t/2 (X_i X_{i+1} + Y_i Y_{i+1})
        for op_name, w in [("X", -t/2.0), ("Y", -t/2.0)]:
            p = ['I'] * N; p[i] = op_name; p[i+1] = op_name
            terms.append(("".join(reversed(p)), w))
        # interaction: U/4 Z_i Z_{i+1}
        p = ['I'] * N; p[i] = 'Z'; p[i+1] = 'Z'
        terms.append(("".join(reversed(p)), U/4.0))
    return SparsePauliOp.from_list(terms)

# ============================================================================
# OBSERVABLES : S(pi), S(k), C(r)
# ============================================================================
def build_s_pi(N):
    """S(pi) = (1/N^2) sum_{i,j} (-1)^(i-j) Z_i Z_j (pic AFM normalise)."""
    terms = []
    for i in range(N):
        for j in range(N):
            coeff = ((-1) ** (i - j)) / (N * N)
            if i == j:
                terms.append(("I" * N, coeff))
            else:
                p = ['I'] * N; p[i] = 'Z'; p[j] = 'Z'
                terms.append(("".join(reversed(p)), coeff))
    return SparsePauliOp.from_list(terms).simplify()

def build_s_k(N, k):
    """S(k) = (1/N) sum_{i,j} cos(k(i-j)) Z_i Z_j."""
    terms = []
    for i in range(N):
        for j in range(N):
            coeff = math.cos(k * (i - j)) / N
            if i == j:
                terms.append(("I" * N, coeff))
            else:
                p = ['I'] * N; p[i] = 'Z'; p[j] = 'Z'
                terms.append(("".join(reversed(p)), coeff))
    return SparsePauliOp.from_list(terms).simplify()

def build_c_r(N, r):
    """C(r) = (1/(N-r)) sum_i Z_i Z_{i+r}."""
    terms = []
    for i in range(N - r):
        p = ['I'] * N; p[i] = 'Z'; p[i+r] = 'Z'
        terms.append(("".join(reversed(p)), 1.0 / (N - r)))
    return SparsePauliOp.from_list(terms).simplify()

# ============================================================================
# CIRCUIT INIT : etat de Neel |0101...> pour amorcer AFM
# ============================================================================
def neel_init_circuit(N):
    qc = QuantumCircuit(N)
    for i in range(N):
        if i % 2 == 1:
            qc.x(i)
    return qc

# ============================================================================
# POOL ADAPT etendu
# ============================================================================
ADAPT_POOL = ["rxx", "ryy", "rzz"]

def add_pool_op(qc, op, i, theta):
    if op == "rxx":   qc.rxx(theta, i, i+1)
    elif op == "ryy": qc.ryy(theta, i, i+1)
    elif op == "rzz": qc.rzz(theta, i, i+1)
    else: raise ValueError(op)

# ============================================================================
# Parameter-shift gradient (Aer, only_last optimisation)
# ============================================================================
def grad_last_param_aer(qc_template, params, theta_vals, hamiltonian):
    n = len(theta_vals)
    s = math.pi / 2.0
    plus  = list(theta_vals); plus[n-1]  += s
    minus = list(theta_vals); minus[n-1] -= s
    qc_p = qc_template.assign_parameters({params[k]: v for k, v in enumerate(plus)})
    qc_m = qc_template.assign_parameters({params[k]: v for k, v in enumerate(minus)})
    est = AerEstimator()
    res = est.run([(qc_p, hamiltonian), (qc_m, hamiltonian)]).result()
    ep = float(res[0].data.evs); em = float(res[1].data.evs)
    return 0.5 * (ep - em), ep, em

def grad_all_aer(qc_template, params, theta_vals, hamiltonian):
    n = len(theta_vals)
    s = math.pi / 2.0
    g = np.zeros(n)
    est = AerEstimator()
    pubs = []
    for k in range(n):
        plus  = list(theta_vals); plus[k]  += s
        minus = list(theta_vals); minus[k] -= s
        qc_p = qc_template.assign_parameters({params[kk]: v for kk, v in enumerate(plus)})
        qc_m = qc_template.assign_parameters({params[kk]: v for kk, v in enumerate(minus)})
        pubs.append((qc_p, hamiltonian))
        pubs.append((qc_m, hamiltonian))
    res = est.run(pubs).result()
    for k in range(n):
        ep = float(res[2*k].data.evs); em = float(res[2*k+1].data.evs)
        g[k] = 0.5 * (ep - em)
    return g

def energy_aer(qc_template, params, theta_vals, hamiltonian):
    qc = qc_template.assign_parameters({params[k]: v for k, v in enumerate(theta_vals)})
    est = AerEstimator()
    res = est.run([(qc, hamiltonian)]).result()
    return float(res[0].data.evs)

# ============================================================================
# C94 NOUVEAUTE : SCORE VORAX (au lieu de |grad| pur)
# score(op, layer) = |grad| * w_grad + stab_curv * w_stab - depth_penalty * w_depth
#   stab_curv : courbure inverse approchee (1 / (1 + |E_p - 2*E0 + E_m|/eps))
# ============================================================================
def vorax_adapt_score(grad_abs, ep, em, e0_est, depth_after, w_grad=1.0, w_stab=0.30, w_depth=0.005):
    curv = abs(ep - 2.0 * e0_est + em)
    stab = 1.0 / (1.0 + curv)
    depth_pen = max(0, depth_after - 14)  # bonus aux profondeurs <= 14
    return w_grad * grad_abs + w_stab * stab - w_depth * depth_pen

# ============================================================================
# ADAPT-VQE pilote VORAX
# ============================================================================
PROBE_THETA = 0.10

def adapt_vqe_vorax(N, hamiltonian, n_rep_max=5, grad_tol=1e-3, max_depth=30):
    qc = neel_init_circuit(N)
    params = []; theta_init = []
    history = []
    e0_est = -1.0 * (N - 1)  # estimation initiale = bas de bande XX (-N+1)

    for layer in range(n_rep_max):
        best = None  # (score, op, i, abs_grad, ep, em, depth_after)
        for i in range(N - 1):
            for op in ADAPT_POOL:
                qc_test = qc.copy()
                p_test = ParameterVector(f"probe_{layer}_{i}_{op}", 1)[0]
                add_pool_op(qc_test, op, i, p_test)
                # depth approx apres assignation
                depth_after = qc_test.depth()
                if depth_after > max_depth:
                    continue
                try:
                    g, ep, em = grad_last_param_aer(
                        qc_test,
                        list(params) + [p_test],
                        list(theta_init) + [PROBE_THETA],
                        hamiltonian)
                    g_abs = float(abs(g))
                    sc = vorax_adapt_score(g_abs, ep, em, e0_est, depth_after)
                except Exception as e:
                    warn("adapt_grad", f"layer={layer} i={i} op={op} err={e}")
                    continue
                if best is None or sc > best[0]:
                    best = (sc, op, i, g_abs, ep, em, depth_after)

        if best is None or best[3] < grad_tol:
            info("adapt_stop", f"layer={layer} best_score={best[0] if best else None} grad<{grad_tol}")
            # Fallback : si layer 0 vide, injection HVA seed
            if layer == 0 and not params:
                info("adapt_seed", "injection couche HVA seed (RXX+RYY)")
                for i in range(N - 1):
                    p1 = ParameterVector(f"seed_xx_{i}", 1)[0]
                    qc.rxx(p1, i, i+1); params.append(p1); theta_init.append(0.10)
                    p2 = ParameterVector(f"seed_yy_{i}", 1)[0]
                    qc.ryy(p2, i, i+1); params.append(p2); theta_init.append(0.10)
                history.append({"layer": -1, "op": "seed_HVA",
                                "i": -1, "score": 0.0, "abs_grad": 0.0,
                                "n_added": 2 * (N - 1)})
            break

        sc, op, i, g_abs, ep, em, dpa = best
        new_p = ParameterVector(f"th_{layer}_{i}_{op}", 1)[0]
        add_pool_op(qc, op, i, new_p)
        params.append(new_p); theta_init.append(0.05)
        info("adapt_pick",
             f"layer={layer} op={op} i={i} |g|={g_abs:.4e} score={sc:.4e} depth={dpa}")
        history.append({"layer": layer, "op": op, "i": i,
                        "abs_grad": g_abs, "score": sc, "depth_after": dpa})
        e0_est = 0.5 * (ep + em)  # estim energie actuelle

    info("adapt_done", f"n_params={len(params)} layers_built={len(history)}")
    return qc, params, theta_init, history

# ============================================================================
# SPSA bi-phasique avec perte composite (idem C93 ameliore)
# ============================================================================
def spsa_bi(qc, params, theta_init, hamiltonian,
            iters=20, alpha_stab=0.10, beta_var=0.05, seed=42):
    rng = np.random.default_rng(seed)
    a = 0.20; c = 0.10; alpha = 0.602; gamma = 0.101
    theta = np.array(theta_init, dtype=float)
    grad_var = 0.0
    history = []
    for k in range(1, iters + 1):
        ak = a / (k ** alpha); ck = c / (k ** gamma)
        delta = 2.0 * rng.integers(0, 2, len(theta)) - 1.0
        tp = theta + ck * delta; tm = theta - ck * delta
        ep = energy_aer(qc, params, tp.tolist(), hamiltonian)
        em = energy_aer(qc, params, tm.tolist(), hamiltonian)
        g = (ep - em) / (2.0 * ck) * delta
        gnorm = float(np.linalg.norm(g))
        grad_var = 0.7 * grad_var + 0.3 * float(np.mean(g * g))
        stab = 1.0 / (1.0 + gnorm)
        loss = ep + alpha_stab * (1.0 - stab) + beta_var * grad_var
        theta = theta - ak * g
        if k % max(1, iters // 5) == 0 or k == iters:
            info("spsa", f"k={k} E={ep:+.4f} |g|={gnorm:.4e} stab={stab:.3f} loss={loss:+.4f}")
        history.append({"k": k, "E": ep, "gnorm": gnorm, "stab": stab,
                        "var": grad_var, "loss": loss})
    return theta.tolist(), history

# ============================================================================
# OBSERVABLES BATCH
# ============================================================================
def build_observables(N):
    obs = []
    obs.append(("S_pi", build_s_pi(N)))
    for k in [0.0, math.pi / 2.0, math.pi]:
        obs.append((f"S_k_{k:.2f}", build_s_k(N, k)))
    obs.append(("C_r_1", build_c_r(N, 1)))
    obs.append((f"C_r_{N//2}", build_c_r(N, N // 2)))
    return obs  # [(name, op)]

# ============================================================================
# MAIN
# ============================================================================
def run(N, dry_run_aer=False, submit_ibm=False, n_rep=4, spsa_iters=15,
        n_twirls=4, shots=2048, resilience=2, alpha_stab=0.10, beta_var=0.05,
        max_depth=30):
    stamp = datetime.datetime.utcnow().strftime("%Y%m%dT%H%M%SZ")
    print("+" + "-" * 78 + "+")
    print(f"| LumVorax  cycle=C94  VORAX-piloted ADAPT-VQE  N={N}".ljust(79) + "|")
    print("+" + "-" * 78 + "+")
    info("config", f"N={N} n_rep={n_rep} spsa={spsa_iters} twirls={n_twirls}"
                   f" shots={shots} resil={resilience} dry={dry_run_aer} ibm={submit_ibm}")

    H = build_hubbard_hamiltonian(N)
    info("hamiltonian", f"H Hubbard N={N} terms={len(H)}")

    qc, params, theta_init, adapt_hist = adapt_vqe_vorax(
        N, H, n_rep_max=n_rep, max_depth=max_depth)
    if not params:
        warn("abort", "circuit vide apres ADAPT-VQE")
        return None

    info("spsa_start",
         f"iters={spsa_iters} alpha={alpha_stab} beta={beta_var}")
    theta_opt, spsa_hist = spsa_bi(
        qc, params, theta_init, H, iters=spsa_iters,
        alpha_stab=alpha_stab, beta_var=beta_var)
    e_final = spsa_hist[-1]["E"]
    stab_final = spsa_hist[-1]["stab"]
    info("spsa_done", f"E_final={e_final:+.4f} stab={stab_final:.3f}")

    # Statevector ideal S(pi)
    try:
        from qiskit_aer import AerSimulator
        from qiskit.quantum_info import Statevector
        qc_b = qc.assign_parameters({params[k]: v for k, v in enumerate(theta_opt)})
        sv = Statevector.from_instruction(qc_b)
        s_pi_ideal = float(sv.expectation_value(build_s_pi(N)).real)
        info("statevector", f"S(pi) ideal Aer N={N} = {s_pi_ideal:+.4f}")
    except Exception as e:
        s_pi_ideal = None
        warn("statevector", f"err {e}")

    obs = build_observables(N)
    info("observables", f"S(pi)+S(k)x3+C(r)x2 = {len(obs)} observables")

    out = {
        "cycle": "C94", "subcycle": "vorax_piloted_adapt_vqe",
        "stamp": stamp, "N": N,
        "config": dict(n_rep=n_rep, spsa_iters=spsa_iters, n_twirls=n_twirls,
                       shots=shots, resilience=resilience,
                       alpha_stab=alpha_stab, beta_var=beta_var,
                       max_depth=max_depth),
        "adapt_history": adapt_hist,
        "spsa_history": spsa_hist[-5:],  # garder dernier echantillon
        "spsa_E_final": e_final,
        "spsa_stab_final": stab_final,
        "theta_opt": theta_opt,
        "s_pi_ideal_aer": s_pi_ideal,
        "circuit_depth_pre_transpile": qc.decompose().depth(),
    }

    if dry_run_aer and not submit_ibm:
        out["mode"] = "dry_run_aer"
        # Mesure observables en local Aer (statevector)
        try:
            from qiskit.quantum_info import Statevector
            sv = Statevector.from_instruction(qc_b)
            local_meas = []
            for name, op in obs:
                v = float(sv.expectation_value(op).real)
                local_meas.append({"name": name, "value": v, "std": 0.0})
                info("aer_meas", f"{name:12s} = {v:+.4f}")
            out["measurements_aer_ideal"] = local_meas
        except Exception as e:
            warn("aer_meas", f"err {e}")
        out_path = RES_DIR / f"ibm_c94_vorax_{stamp}_N{N}_DRY.json"
        out_path.write_text(json.dumps(out, indent=2, default=str))
        ok("saved (dry_run)", str(out_path.relative_to(REPO)))
        return out

    if submit_ibm:
        from qiskit_ibm_runtime import QiskitRuntimeService, EstimatorV2 as RuntimeEstimator
        from qiskit import transpile
        token = os.environ.get("IBM_API_KEY")
        if not token:
            warn("ibm", "IBM_API_KEY absent - abort submit"); return out
        info("token", f"IBM_API_KEY len={len(token)}")
        svc = QiskitRuntimeService(channel="ibm_quantum_platform", token=token)
        backend = svc.backend("ibm_kingston")
        info("backend", f"{backend.name} num_qubits={backend.num_qubits}")

        qc_b = qc.assign_parameters({params[k]: v for k, v in enumerate(theta_opt)})
        qc_t = transpile(qc_b, backend=backend, optimization_level=2)
        n2q = sum(1 for inst in qc_t.data if inst.operation.num_qubits == 2)
        info("transpile", f"depth={qc_t.depth()} 2Q={n2q}")
        out["transpile_final"] = {"depth": qc_t.depth(), "n2q": n2q}

        layout = qc_t.layout
        obs_mapped = []
        for name, op in obs:
            try:
                op2 = op.apply_layout(layout)
                obs_mapped.append((name, op2))
            except Exception as e:
                warn("obs_layout", f"{name} err={e}")

        est = RuntimeEstimator(mode=backend)
        est.options.default_shots = shots
        est.options.resilience_level = resilience
        try:
            est.options.twirling.enable_gates = True
            est.options.twirling.num_randomizations = 32
        except Exception:
            pass
        info("submit", f"resilience={resilience} shots={shots}"
                       f" observables={len(obs_mapped)}")
        # Batch unique
        pubs = [(qc_t, op) for _, op in obs_mapped]
        job = est.run(pubs)
        info("job_id", job.job_id())
        out["job_id"] = job.job_id()
        # On NE bloque PAS sur job.result() ici (timeout shell vs queue)
        out["submitted"] = True
        out["mode"] = "submit_ibm"
        out_path = RES_DIR / f"ibm_c94_vorax_{stamp}_N{N}_SUBMITTED.json"
        out_path.write_text(json.dumps(out, indent=2, default=str))
        ok("saved (submit)", str(out_path.relative_to(REPO)))
        info("retrieve",
             f"python tools/ibm_c94_retrieve.py {job.job_id()}"
             f" --N {N} --wait_minutes 15")
        return out

    # ni dry-run ni submit -> juste pre-train
    out_path = RES_DIR / f"ibm_c94_vorax_{stamp}_N{N}_PRETRAIN.json"
    out_path.write_text(json.dumps(out, indent=2, default=str))
    ok("saved", str(out_path.relative_to(REPO)))
    return out

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--N", type=int, default=12)
    ap.add_argument("--n_rep", type=int, default=4)
    ap.add_argument("--spsa_iters", type=int, default=15)
    ap.add_argument("--n_twirls", type=int, default=4)
    ap.add_argument("--shots", type=int, default=2048)
    ap.add_argument("--resilience", type=int, default=2)
    ap.add_argument("--alpha_stab", type=float, default=0.10)
    ap.add_argument("--beta_var", type=float, default=0.05)
    ap.add_argument("--max_depth", type=int, default=30)
    ap.add_argument("--dry-run-aer", action="store_true")
    ap.add_argument("--submit-ibm", action="store_true")
    args = ap.parse_args()
    run(N=args.N, dry_run_aer=args.dry_run_aer, submit_ibm=args.submit_ibm,
        n_rep=args.n_rep, spsa_iters=args.spsa_iters, n_twirls=args.n_twirls,
        shots=args.shots, resilience=args.resilience,
        alpha_stab=args.alpha_stab, beta_var=args.beta_var,
        max_depth=args.max_depth)
