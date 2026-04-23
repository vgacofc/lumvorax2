#!/usr/bin/env python3
"""LumVorax C90-pre — Recalcul retroactif S(pi) corrige sur tous les .lum
existants (cycles C64 -> C89). Cout IBM = 0.

Pour chaque .lum trouve :
  1. Decompresse + parse le JSON
  2. Cherche un dict de counts (clefs "counts", "merged", "merged_top10",
     "result.counts", ...)
  3. Calcule S(pi)_C88_legacy ET S(pi)_C89_corrige
  4. Empile dans un rapport JSON + Markdown

Sortie :
  src/advanced_calculations/bitcoin_quantum_mining/CHAT/RETRO_S_PI_C90PRE_<stamp>.md
  src/advanced_calculations/bitcoin_quantum_mining/results/retro_s_pi_<stamp>.json
"""
import ctypes as _ct, os as _os
for _p in [
    "/nix/store/bmi5znnqk4kg2grkrhk6py0irc8phf6l-gcc-14.3.0-lib/lib/libstdc++.so.6",
    "/nix/store/bmi5znnqk4kg2grkrhk6py0irc8phf6l-gcc-14.2.1.20250322-lib/lib/libstdc++.so.6",
    "/usr/lib/x86_64-linux-gnu/libstdc++.so.6",
]:
    if _os.path.exists(_p):
        try: _ct.CDLL(_p); break
        except: pass

import os, sys, json, gzip, math, datetime, hashlib
from pathlib import Path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from run_header import (print_header, ok, record, warn, err, info,
                         GREEN, RED, YELLOW, RESET, BOLD)

import numpy as np

REPO = Path(__file__).resolve().parent.parent
LUM_DIRS = [
    REPO / "src/advanced_calculations/bitcoin_quantum_mining/logs/lum_native",
    REPO / "src/advanced_calculations/quantum_problem_hubbard_hts/logs/lum_native",
    REPO / "src/advanced_calculations/bitcoin_quantum_mining/results",
]
OUT_MD  = REPO / "src/advanced_calculations/bitcoin_quantum_mining/CHAT"
OUT_JSON= REPO / "src/advanced_calculations/bitcoin_quantum_mining/results"
OUT_MD.mkdir(parents=True, exist_ok=True)
OUT_JSON.mkdir(parents=True, exist_ok=True)
STAMP = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%SZ")

def s_pi_c88_legacy(counts):
    if not counts: return None
    total = sum(counts.values())
    if total == 0: return None
    n = len(next(iter(counts)))
    z = np.zeros((total, n), dtype=np.int8); idx = 0
    for bs, c in counts.items():
        bits = np.array([1 - 2*int(b) for b in bs[::-1]], dtype=np.int8)
        z[idx:idx+c] = bits; idx += c
    z = z[:idx]
    S = 0.0
    for i in range(n):
        for j in range(n):
            S += ((-1)**((i-j)%2)) * float(np.mean(z[:,i]*z[:,j]))
    return float(S/n)

def s_pi_c89_corrige(counts):
    if not counts: return None
    total = sum(counts.values())
    if total == 0: return None
    n = len(next(iter(counts)))
    z = np.zeros((total, n), dtype=np.int8); idx = 0
    for bs, c in counts.items():
        bits = np.array([1 - 2*int(b) for b in bs[::-1]], dtype=np.int8)
        z[idx:idx+c] = bits; idx += c
    z = z[:idx]
    S = 0.0; cnt = 0
    for i in range(n):
        for j in range(n):
            if i==j: continue
            S += ((-1)**((i-j)%2)) * float(np.mean(z[:,i]*z[:,j]))
            cnt += 1
    return float(S/max(cnt,1))

def shannon_bits(counts):
    if not counts: return 0.0
    total = max(1, sum(counts.values()))
    return float(-sum((c/total)*math.log2(max(c/total,1e-12)) for c in counts.values()))

def find_counts_in_obj(obj, depth=0, path=""):
    """Cherche recursivement un dict bitstring->int."""
    out = []
    if depth > 6: return out
    if isinstance(obj, dict):
        # heuristique : counts = dict where keys are 0/1 strings
        if obj and all(isinstance(k, str) and set(k).issubset(set("01 "))
                       and isinstance(v, (int, float)) for k, v in obj.items()):
            keys = [k.replace(" ", "") for k in obj.keys()]
            if keys and len(set(len(k) for k in keys)) == 1 and len(keys[0]) >= 2:
                out.append((path, {k.replace(" ",""): int(v) for k, v in obj.items()}))
        for k, v in obj.items():
            out.extend(find_counts_in_obj(v, depth+1, path+"."+str(k)))
    elif isinstance(obj, list):
        for i, v in enumerate(obj):
            out.extend(find_counts_in_obj(v, depth+1, path+f"[{i}]"))
    return out

def load_lum(p: Path):
    try:
        data = gzip.open(p, "rb").read()
        j = json.loads(data)
        return j
    except Exception:
        try:
            return json.loads(p.read_text())
        except Exception as e:
            return {"_load_error": str(e)}

def main():
    v = print_header(cycle="C90-pre", backend="LOCAL (retro recompute)",
                     extra={"input": str(LUM_DIRS[0].relative_to(REPO))})
    info("scan", f"recherche .lum dans {len(LUM_DIRS)} dossiers")
    files = []
    for d in LUM_DIRS:
        if d.exists():
            files += sorted(d.glob("*.lum")) + sorted(d.glob("*.json"))
    info("scan", f"{len(files)} fichiers candidats trouves")

    rows = []
    n_with_counts = 0; n_ok = 0; n_skip = 0; n_err = 0
    for i, p in enumerate(files):
        rel = str(p.relative_to(REPO))
        try:
            j = load_lum(p)
            if "_load_error" in j:
                n_err += 1; continue
            found = find_counts_in_obj(j)
            if not found:
                n_skip += 1; continue
            n_with_counts += 1
            for path_in_json, counts in found:
                if len(next(iter(counts))) > 16:
                    continue
                s_legacy = s_pi_c88_legacy(counts)
                s_corr   = s_pi_c89_corrige(counts)
                if s_legacy is None or s_corr is None: continue
                H = shannon_bits(counts)
                row = {
                    "file": rel,
                    "json_path": path_in_json,
                    "n_qubits": len(next(iter(counts))),
                    "total_shots": sum(counts.values()),
                    "n_keys": len(counts),
                    "S_pi_C88_legacy": round(s_legacy, 6),
                    "S_pi_C89_corrige": round(s_corr, 6),
                    "diff_legacy_minus_corrige": round(s_legacy - s_corr, 6),
                    "entropy_bits": round(H, 4),
                }
                rows.append(row)
                n_ok += 1
                # Affichage couleur si signal AFM significatif
                if abs(s_corr) > 0.05:
                    record(f"signal AFM",
                           f"{p.name[:50]}  S_corr={s_corr:+.4f}  H={H:.2f}")
                elif s_corr < -0.10:
                    err("signal anti-AFM fort",
                        f"{p.name[:50]}  S_corr={s_corr:+.4f}")
        except Exception as e:
            n_err += 1
            warn(f"err {p.name[:40]}", str(e)[:60])

    info("done", f"OK={n_ok}  candidates_with_counts={n_with_counts}  skip={n_skip}  err={n_err}")

    # Tri par |S_corr| pour reperer les vrais signaux
    rows_sorted = sorted(rows, key=lambda r: abs(r["S_pi_C89_corrige"]), reverse=True)
    top = rows_sorted[:30]
    bottom_zero = [r for r in rows_sorted if abs(r["S_pi_C89_corrige"]) < 0.01]

    json_path = OUT_JSON / f"retro_s_pi_{STAMP}.json"
    json_path.write_text(json.dumps({
        "cycle": "C90-pre",
        "stamp": STAMP,
        "version": v,
        "n_files_scanned": len(files),
        "n_with_counts": n_with_counts,
        "n_recomputed": n_ok,
        "n_skip": n_skip,
        "n_err": n_err,
        "rows": rows,
    }, indent=2, default=str))
    ok("json saved", str(json_path.relative_to(REPO)))

    md = []
    md.append(f"# RETRO RECOMPUTE S(π) — Cycle C90-pre — {STAMP}")
    md.append("")
    md.append(f"- Fichiers scannes : **{len(files)}**")
    md.append(f"- Avec counts detectes : **{n_with_counts}**")
    md.append(f"- Recalculs reussis : **{n_ok}**")
    md.append(f"- Commit code : `{v['commit']}` ({v['branch']})")
    md.append("")
    md.append("## Top 30 |S(π) corrigé| (potentiels signaux AFM réels)")
    md.append("")
    md.append("| File | n_q | shots | n_keys | S_legacy | S_corr | diff | H_bits |")
    md.append("|---|---:|---:|---:|---:|---:|---:|---:|")
    for r in top:
        f = r["file"].split("/")[-1][:60]
        md.append(f"| `{f}` | {r['n_qubits']} | {r['total_shots']} | {r['n_keys']} | "
                  f"{r['S_pi_C88_legacy']:+.4f} | **{r['S_pi_C89_corrige']:+.4f}** | "
                  f"{r['diff_legacy_minus_corrige']:+.4f} | {r['entropy_bits']:.2f} |")
    md.append("")
    md.append(f"## Bilan — combien de fichiers contiennent un VRAI signal AFM ?")
    md.append("")
    md.append(f"- |S_corr| > 0.10 (signal fort)     : **{sum(1 for r in rows if abs(r['S_pi_C89_corrige'])>0.10)}** / {n_ok}")
    md.append(f"- |S_corr| > 0.05 (signal moyen)    : **{sum(1 for r in rows if abs(r['S_pi_C89_corrige'])>0.05)}** / {n_ok}")
    md.append(f"- |S_corr| > 0.02 (signal faible)   : **{sum(1 for r in rows if abs(r['S_pi_C89_corrige'])>0.02)}** / {n_ok}")
    md.append(f"- |S_corr| < 0.01 (bruit pur)       : **{len(bottom_zero)}** / {n_ok}")
    md.append("")
    md.append("## Validation du fix C89")
    md.append("")
    if rows:
        diffs = [r['diff_legacy_minus_corrige'] for r in rows]
        md.append(f"- diff legacy-corrige : moyenne **{np.mean(diffs):+.4f}**, "
                  f"min={min(diffs):+.4f}, max={max(diffs):+.4f}")
        md.append(f"- Prediction theorique pour signal nul : +1.000")
        md.append(f"- Prediction theorique avec S_corr=0.025 : +1.013")
        md.append(f"- → si moyenne ≈ 1.0 ± 0.2 : **fix C89 valide mathematiquement**")
    md.append("")
    md.append("---")
    md.append("FIN du recalcul retroactif.")
    md_path = OUT_MD / f"RETRO_S_PI_C90PRE_{STAMP}.md"
    md_path.write_text("\n".join(md))
    ok("md saved", str(md_path.relative_to(REPO)))
    return json_path, md_path

if __name__ == "__main__":
    main()
