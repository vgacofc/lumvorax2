#!/usr/bin/env python3
"""nx48_ab_wilcoxon.py — C99 P1.2 / Q2 protocole A/B rigoureux.

Lance 5 paires (A=NX48 seul, B=NX48+coupler) en ordre randomisé,
réinitialise btc_nx48_last.csv avant chaque run, durée 30 min chacun.

Métrique principale : best_leading_zeros + hashrate moyen.
Test statistique : Wilcoxon signed-rank (paires) — non-paramétrique, robuste à petit N.

Sortie : logs/c105/nx48_ab_results.json + verdict.

Usage Ubuntu :
    python3 tools/nx48_ab_wilcoxon.py --pairs 5 --duration 1800 \
        --binary src/advanced_calculations/bitcoin_quantum_mining/btc_mining_runner
"""
import argparse, json, os, random, re, shutil, subprocess, sys, time
from pathlib import Path
from datetime import datetime

LZ_RE      = re.compile(r"best_leading_zeros\s*[:=]\s*(\d+)", re.I)
HASH_RE    = re.compile(r"([0-9.]+)\s*MH/?s", re.I)
LOSS_RE    = re.compile(r"loss\s*[:=]\s*([0-9.eE+-]+)")


def reset_nx48_state(state_dir: Path):
    """Sauvegarde puis réinitialise btc_nx48_last.csv."""
    f = state_dir / "btc_nx48_last.csv"
    bak = state_dir / f"btc_nx48_last.bak.{int(time.time())}.csv"
    if f.exists():
        shutil.copy(f, bak)
        f.unlink()


def run_miner(binary: Path, duration: int, use_coupler: bool, run_id: str,
              env_extra: dict | None = None):
    """Lance le mineur pendant `duration` secondes, parse la sortie."""
    env = os.environ.copy()
    env["BTC_USE_NEURAL_COUPLER"] = "1" if use_coupler else "0"
    env["BTC_RUN_ID"] = run_id
    if env_extra: env.update(env_extra)

    log_path = Path(f"logs/c105/ab_{run_id}.log")
    log_path.parent.mkdir(parents=True, exist_ok=True)
    t0 = time.time()
    with log_path.open("w") as lf:
        proc = subprocess.Popen([str(binary), "--duration", str(duration)],
                                stdout=lf, stderr=subprocess.STDOUT, env=env)
        try:
            proc.wait(timeout=duration + 60)
        except subprocess.TimeoutExpired:
            proc.kill(); proc.wait()
    elapsed = time.time() - t0

    text = log_path.read_text()
    lzs   = [int(m) for m in LZ_RE.findall(text)]
    hrs   = [float(m) for m in HASH_RE.findall(text)]
    losses = [float(m) for m in LOSS_RE.findall(text)]

    return {
        "run_id": run_id,
        "use_coupler": use_coupler,
        "duration_actual_s": elapsed,
        "best_lz": max(lzs) if lzs else 0,
        "mean_hashrate_mhs": sum(hrs)/len(hrs) if hrs else 0.0,
        "n_lz_samples": len(lzs),
        "loss_first": losses[0] if losses else None,
        "loss_last":  losses[-1] if losses else None,
        "log_file": str(log_path),
    }


def wilcoxon_signed_rank(diffs):
    """Wilcoxon signed-rank pure-python (suffit pour N=5)."""
    abs_d = [(abs(d), 1 if d > 0 else -1) for d in diffs if d != 0]
    if len(abs_d) < 2:
        return {"W": 0, "p": 1.0, "n": len(abs_d), "method": "trivial"}
    abs_d.sort()
    ranks = {}
    i = 0
    while i < len(abs_d):
        j = i
        while j + 1 < len(abs_d) and abs_d[j+1][0] == abs_d[i][0]:
            j += 1
        avg_rank = (i + j) / 2 + 1
        for k in range(i, j+1):
            ranks[k] = avg_rank
        i = j + 1
    W_plus  = sum(ranks[k] for k,(_,s) in enumerate(abs_d) if s > 0)
    W_minus = sum(ranks[k] for k,(_,s) in enumerate(abs_d) if s < 0)
    W = min(W_plus, W_minus)
    n = len(abs_d)
    # Approximation normale (n>=5 OK pour signal)
    mu = n * (n + 1) / 4
    sigma = (n * (n + 1) * (2 * n + 1) / 24) ** 0.5
    z = (W - mu) / sigma if sigma > 0 else 0.0
    # p-value bilatéral approx normal
    import math
    p = 2 * (1 - 0.5 * (1 + math.erf(abs(z) / 2**0.5)))
    return {"W": W, "z": z, "p": p, "n": n, "W_plus": W_plus, "W_minus": W_minus}


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--binary", required=True, help="path to btc_mining_runner")
    p.add_argument("--pairs", type=int, default=5)
    p.add_argument("--duration", type=int, default=1800, help="secondes par run")
    p.add_argument("--state-dir", default=".",
                   help="dossier btc_nx48_last.csv")
    p.add_argument("--out", default="logs/c105/nx48_ab_results.json")
    args = p.parse_args()

    binary = Path(args.binary).resolve()
    if not binary.exists():
        print(f"FATAL: binary {binary} introuvable"); sys.exit(2)
    state_dir = Path(args.state_dir)
    Path("logs/c105").mkdir(parents=True, exist_ok=True)

    results = {"started_iso": datetime.utcnow().isoformat()+"Z",
               "binary": str(binary),
               "duration_per_run_s": args.duration, "pairs": []}

    rng = random.Random(20260426)
    for i in range(args.pairs):
        order = ["A", "B"]
        rng.shuffle(order)            # randomisation ordre dans la paire
        pair = {"pair_id": i+1, "order": order, "A": None, "B": None}
        for cond in order:
            reset_nx48_state(state_dir)        # état neutre obligatoire
            run_id = f"p{i+1}{cond}_{int(time.time())}"
            r = run_miner(binary, args.duration,
                          use_coupler=(cond == "B"), run_id=run_id)
            pair[cond] = r
            print(f"[PAIR {i+1}] {cond} (coupler={r['use_coupler']}) "
                  f"best_lz={r['best_lz']} mean_mhs={r['mean_hashrate_mhs']:.2f}")
        results["pairs"].append(pair)
        Path(args.out).write_text(json.dumps(results, indent=2))

    # Analyse statistique
    diffs_lz   = [p["B"]["best_lz"]      - p["A"]["best_lz"]      for p in results["pairs"]]
    diffs_mhs  = [p["B"]["mean_hashrate_mhs"] - p["A"]["mean_hashrate_mhs"]
                  for p in results["pairs"]]
    results["wilcoxon_lz"]  = wilcoxon_signed_rank(diffs_lz)
    results["wilcoxon_mhs"] = wilcoxon_signed_rank(diffs_mhs)
    results["finished_iso"] = datetime.utcnow().isoformat()+"Z"

    Path(args.out).write_text(json.dumps(results, indent=2))
    print(f"\n=== RÉSULTAT {args.out} ===")
    print(f"Wilcoxon best_lz       : {results['wilcoxon_lz']}")
    print(f"Wilcoxon mean_hashrate : {results['wilcoxon_mhs']}")
    p_lz = results["wilcoxon_lz"]["p"]
    if p_lz < 0.05:
        print(f"VERDICT : coupler {'AMÉLIORE' if sum(diffs_lz) > 0 else 'DÉGRADE'} significativement (p={p_lz:.4f})")
    else:
        print(f"VERDICT : pas de différence significative (p={p_lz:.4f})")


if __name__ == "__main__":
    main()
