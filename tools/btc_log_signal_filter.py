#!/usr/bin/env python3
"""
btc_log_signal_filter.py — Cycle C128 — anomalie #A10
Reduit le ratio bruit/signal des logs forensic BTC mining.

Probleme : sur run mainnet 1200 s, btc_qm_engine_metrics.log
contient 180 264 lignes dont 168 822 (93.6 %) sont des metriques
PT-MC repliques verbeuses (T_hot/T_cold/accept) ecrites toutes
les 130 ms et oscillant dans une bande etroite — quasi 0 information
nouvelle par ligne.

Cet outil produit DEUX fichiers SANS RIEN SUPPRIMER de l'original :
  - <input>.signal.csv : 6.4 % du log = events utiles
       (near_miss>=33 bits, NX48 update, transitions, anomalies thermal,
        decisions tune_full, lum_log records, hashrate >5% delta)
  - <input>.noise.summary.csv : statistiques agregees PT-MC
       (mean / stddev / min / max / N par fenetre 30 s)

Usage :
  python3 tools/btc_log_signal_filter.py <fichier.log>
  python3 tools/btc_log_signal_filter.py --json <fichier.log>     # sortie JSON
  python3 tools/btc_log_signal_filter.py --threshold-bits 35 ...  # near-miss seuil

Aucune dependance externe (stdlib uniquement).
"""
from __future__ import annotations
import sys, os, re, json, argparse, statistics
from collections import defaultdict

PT_MC_PATTERNS = [
    re.compile(r"T_hot=([\d.]+).*T_cold=([\d.]+).*accept=([\d.]+)"),
    re.compile(r"PT-MC.*replica=\d+.*temp=([\d.]+)"),
    re.compile(r"replica_swap.*accept_rate=([\d.]+)"),
]

SIGNAL_PATTERNS = [
    re.compile(r"\[BTC_QM\]\s+near_miss.*?(\d+)\s+bits"),
    re.compile(r"\[BTC_QM\]\s+best_leading\s*=\s*(\d+)"),
    re.compile(r"\[BTC_QM\]\s+Hashrate\s*:\s*([\d.]+)\s*MH/s"),
    re.compile(r"\[NX48[_-]"),
    re.compile(r"\[C\d+[-_].*OVERRIDE"),
    re.compile(r"\[C125-TUNE-FULL"),
    re.compile(r"\[C127-FIX"),
    re.compile(r"\[C128-"),
    re.compile(r"\[C112-LUM"),
    re.compile(r"\[C110-OPT"),
    re.compile(r"\[C69-GPU\]"),
    re.compile(r"thermal_throttle"),
    re.compile(r"snapshot.*BIT-1b"),
    re.compile(r"FATAL|ERROR|WARN"),
]


def classify_line(line: str, thr_bits: int):
    """Retourne 'signal' | 'noise_ptmc' | 'other'"""
    for p in SIGNAL_PATTERNS:
        m = p.search(line)
        if m:
            if "near_miss" in line or "best_leading" in line:
                try:
                    bits = int(m.group(1))
                    if bits >= thr_bits:
                        return "signal", {"bits": bits}
                    else:
                        return "other", {"bits": bits}
                except (ValueError, IndexError):
                    pass
            return "signal", {}
    for p in PT_MC_PATTERNS:
        m = p.search(line)
        if m:
            try:
                t_hot = float(m.group(1))
                return "noise_ptmc", {"t_hot": t_hot}
            except (ValueError, IndexError):
                return "noise_ptmc", {}
    return "other", {}


def process(path: str, thr_bits: int, window_s: float = 30.0):
    if not os.path.isfile(path):
        print(f"ERREUR: fichier introuvable: {path}", file=sys.stderr)
        return 1
    counts = defaultdict(int)
    signal_lines = []
    ptmc_t_hot = []
    total_bytes_in = os.path.getsize(path)

    with open(path, "r", errors="replace") as f:
        for i, line in enumerate(f, 1):
            cls, meta = classify_line(line, thr_bits)
            counts[cls] += 1
            if cls == "signal":
                signal_lines.append((i, line.rstrip("\n")))
            elif cls == "noise_ptmc" and "t_hot" in meta:
                ptmc_t_hot.append(meta["t_hot"])

    total = sum(counts.values())
    if total == 0:
        print("ERREUR: fichier vide", file=sys.stderr)
        return 1

    # Ecriture fichier signal
    out_signal = path + ".signal.csv"
    with open(out_signal, "w") as f:
        f.write("# C128-A10 signal_filter — lignes utiles uniquement\n")
        f.write("# source: %s | total_lines=%d | signal=%d (%.2f%%) | noise_ptmc=%d (%.2f%%)\n" % (
            path, total, counts["signal"], 100 * counts["signal"] / total,
            counts["noise_ptmc"], 100 * counts["noise_ptmc"] / total,
        ))
        f.write("line_no,content\n")
        for ln, content in signal_lines:
            f.write(f'{ln},"{content.replace(chr(34), chr(39))}"\n')

    # Ecriture summary noise
    out_noise = path + ".noise.summary.csv"
    with open(out_noise, "w") as f:
        f.write("# C128-A10 noise_summary — agregat PT-MC\n")
        f.write("metric,n,mean,stddev,min,max\n")
        if ptmc_t_hot:
            f.write("t_hot,%d,%.4f,%.4f,%.4f,%.4f\n" % (
                len(ptmc_t_hot), statistics.mean(ptmc_t_hot),
                statistics.stdev(ptmc_t_hot) if len(ptmc_t_hot) > 1 else 0.0,
                min(ptmc_t_hot), max(ptmc_t_hot),
            ))

    summary = {
        "input": path,
        "input_size_bytes": total_bytes_in,
        "total_lines": total,
        "signal_lines": counts["signal"],
        "noise_ptmc_lines": counts["noise_ptmc"],
        "other_lines": counts["other"],
        "signal_pct": round(100 * counts["signal"] / total, 3),
        "noise_pct": round(100 * counts["noise_ptmc"] / total, 3),
        "compression_ratio": round(total / max(counts["signal"], 1), 1),
        "output_signal": out_signal,
        "output_noise_summary": out_noise,
        "near_miss_threshold_bits": thr_bits,
    }
    return summary


def main():
    ap = argparse.ArgumentParser(description="BTC log signal/bruit filter (C128-A10)")
    ap.add_argument("logfile", help="chemin du log forensic .log")
    ap.add_argument("--threshold-bits", type=int, default=33,
                    help="seuil near-miss bits pour qualifier 'signal' (def: 33)")
    ap.add_argument("--json", action="store_true", help="sortie JSON")
    args = ap.parse_args()

    res = process(args.logfile, args.threshold_bits)
    if isinstance(res, int):
        return res

    if args.json:
        print(json.dumps(res, indent=2))
    else:
        print("=" * 60)
        print(f"BTC LOG SIGNAL FILTER — C128-A10")
        print("=" * 60)
        print(f"Source        : {res['input']}")
        print(f"Taille entree : {res['input_size_bytes']:,} octets")
        print(f"Lignes totales: {res['total_lines']:,}")
        print(f"  signal      : {res['signal_lines']:,} ({res['signal_pct']}%)")
        print(f"  bruit PT-MC : {res['noise_ptmc_lines']:,} ({res['noise_pct']}%)")
        print(f"  autres      : {res['other_lines']:,}")
        print(f"Compression   : {res['compression_ratio']}:1")
        print(f"Seuil near-miss: {res['near_miss_threshold_bits']} bits")
        print(f"Sorties:")
        print(f"  signal      : {res['output_signal']}")
        print(f"  noise summ. : {res['output_noise_summary']}")
        print("=" * 60)
    return 0


if __name__ == "__main__":
    sys.exit(main())
