#!/usr/bin/env python3
import argparse
import json
import os
import re
import subprocess
import tempfile
import time


def run_case(binary, header_hex, duration_s, threads, nx48_enabled):
    env = os.environ.copy()
    if not nx48_enabled:
        env["BTC_NX48_DISABLED"] = "1"
    csv_path = tempfile.NamedTemporaryFile(prefix="lumvorax_nx48_ab_", suffix=".csv", delete=False).name
    cmd = [
        binary,
        "--mode", "BENCHMARK",
        "--threads", str(threads),
        "--duration-s", str(duration_s),
        "--nx48-csv", csv_path,
    ]
    if header_hex:
        cmd += ["--header-hex", header_hex]
    t0 = time.time()
    proc = subprocess.run(cmd, cwd=os.path.dirname(binary) or ".", env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=duration_s + 45)
    elapsed = time.time() - t0
    output = proc.stdout
    best = 0
    hashrate = 0.0
    for pattern in [r"best[_ ]leading[_ ]zeros[^0-9]*(\d+)", r"leading_zeros[^0-9]*(\d+)"]:
        for match in re.finditer(pattern, output, re.IGNORECASE):
            best = max(best, int(match.group(1)))
    for pattern in [r"hashrate[=_: ]+(\d+(?:\.\d+)?)\s*MH/s", r"btc_hashrate_mhs_final[^0-9]*(\d+(?:\.\d+)?)"]:
        for match in re.finditer(pattern, output, re.IGNORECASE):
            try:
                hashrate = max(hashrate, float(match.group(1)))
            except ValueError:
                pass
    return {
        "nx48_enabled": nx48_enabled,
        "returncode": proc.returncode,
        "elapsed_s": elapsed,
        "best_leading_zeros": best,
        "hashrate_mhs_observed": hashrate,
        "csv_path": csv_path,
        "output_tail": "\n".join(output.splitlines()[-40:]),
    }


def main():
    parser = argparse.ArgumentParser(description="Benchmark A/B LumVorax BTC avec et sans NX48")
    parser.add_argument("--binary", default="./btc_mining_runner")
    parser.add_argument("--header-hex", default="")
    parser.add_argument("--duration-s", type=int, default=10)
    parser.add_argument("--threads", type=int, default=2)
    parser.add_argument("--output", default="logs/forensic/btc_nx48_ab_benchmark.json")
    args = parser.parse_args()
    binary = os.path.abspath(args.binary)
    results = {
        "schema": "lumvorax_btc_nx48_ab_benchmark_v1",
        "created_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "duration_s_per_case": args.duration_s,
        "threads": args.threads,
        "cases": [
            run_case(binary, args.header_hex, args.duration_s, args.threads, False),
            run_case(binary, args.header_hex, args.duration_s, args.threads, True),
        ],
    }
    os.makedirs(os.path.dirname(args.output) or ".", exist_ok=True)
    with open(args.output, "w", encoding="utf-8") as f:
        json.dump(results, f, indent=2, sort_keys=True)
        f.write("\n")
    print(json.dumps(results, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())