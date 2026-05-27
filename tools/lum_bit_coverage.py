#!/usr/bin/env python3
"""
tools/lum_bit_coverage.py — Cycle C124
Calcule le taux de couverture bit-level effectif des fichiers .lum produits par
le runner BTC (logs/forensic/modules/, logs/lum_native/) en comparant les bits
realmente traces (granularite PAGE/BYTE/BIT) au theorique total RSS du process.

Repond a la question utilisateur :
  "quel est le pourcentage reel de tracabilite bit-level ?"

Sans dependance externe (Python 3 stdlib uniquement).
Compatible C111 (header 32 octets, magic LUMT) et C117 (header 64 octets v2).

Usage :
  python3 tools/lum_bit_coverage.py [--logs-dir logs] [--json|--text]
"""
from __future__ import annotations
import argparse
import json
import os
import struct
import sys
from pathlib import Path

LUMT_MAGIC = 0x4C554D54   # 'LUMT' little-endian
LMUL_MAGIC = 0x4C554D4C   # 'LMUL' little-endian
LUMN_MAGIC = 0x4C554D4E   # 'LUMN' little-endian
LUMQ_MAGIC = 0x4C554D51   # 'LUMQ' little-endian

LUM_FORMAT_VERSION_C117 = 2

GRAN_PAGE = 0
GRAN_BYTE = 1
GRAN_BIT  = 2
GRAN_HUGE = 3

PAGE_SIZE = 4096


def read_lum_header(fp) -> dict:
    """Lit le header lum_file_header_t (v1=32o ou v2=64o C117)."""
    head32 = fp.read(32)
    if len(head32) < 32:
        return {"valid": False, "reason": "header trop court"}
    magic, granularity, _flags, _reserved, total_lums, total_pages_resident, snapshot_ns = \
        struct.unpack("<IBBHQQQ", head32[:32])
    if magic != LUMT_MAGIC:
        return {"valid": False, "magic_hex": f"0x{magic:08x}", "reason": "pas LUMT"}
    info = {
        "valid": True,
        "format": "LUMT",
        "version": 1,
        "granularity": int(granularity),
        "total_lums": int(total_lums),
        "total_pages_resident": int(total_pages_resident),
        "snapshot_ns": int(snapshot_ns),
        "header_size": 32,
    }
    extra = fp.read(32)
    if len(extra) == 32:
        version_major = struct.unpack("<I", extra[:4])[0]
        if version_major == LUM_FORMAT_VERSION_C117:
            info["version"] = 2
            info["header_size"] = 64
            checksum_algo, ts_realtime_ns, ts_monotonic_ns = \
                struct.unpack("<IQQ", extra[4:28])
            info["checksum_algo"] = int(checksum_algo)
            info["timestamp_realtime_ns"] = int(ts_realtime_ns)
            info["timestamp_monotonic_ns"] = int(ts_monotonic_ns)
        else:
            fp.seek(-32, 1)
    return info


def estimate_traced_bits(info: dict, file_size: int) -> int:
    """Combien de bits effectifs sont traces dans ce fichier .lum ?"""
    if not info.get("valid"):
        return 0
    total_lums = info["total_lums"]
    gran = info["granularity"]
    if gran == GRAN_PAGE:
        return total_lums * PAGE_SIZE * 8
    if gran == GRAN_BYTE:
        return total_lums * 8
    if gran == GRAN_BIT:
        return total_lums * 1
    if gran == GRAN_HUGE:
        return total_lums * 512 * PAGE_SIZE * 8
    return 0


def scan_dir(root: Path) -> list:
    out = []
    if not root.exists():
        return out
    for path in root.rglob("*.lum"):
        try:
            file_size = path.stat().st_size
            with open(path, "rb") as fp:
                info = read_lum_header(fp)
            traced_bits = estimate_traced_bits(info, file_size)
            out.append({
                "path": str(path),
                "size_bytes": file_size,
                "info": info,
                "traced_bits": traced_bits,
            })
        except Exception as e:
            out.append({"path": str(path), "error": str(e)})
    return out


def aggregate(records: list) -> dict:
    total_traced_bits = 0
    total_pages_resident = 0
    by_granularity = {0: 0, 1: 0, 2: 0, 3: 0}
    valid_count = 0
    for r in records:
        if "error" in r:
            continue
        info = r.get("info", {})
        if not info.get("valid"):
            continue
        valid_count += 1
        total_traced_bits += r["traced_bits"]
        total_pages_resident = max(total_pages_resident, info.get("total_pages_resident", 0))
        by_granularity[info["granularity"]] = by_granularity.get(info["granularity"], 0) + 1
    theoretical_max_bits = total_pages_resident * PAGE_SIZE * 8 if total_pages_resident else 0
    coverage_pct = (100.0 * total_traced_bits / theoretical_max_bits) if theoretical_max_bits else None
    return {
        "files_scanned": len(records),
        "files_valid": valid_count,
        "by_granularity": {
            "page":     by_granularity.get(0, 0),
            "byte":     by_granularity.get(1, 0),
            "bit":      by_granularity.get(2, 0),
            "hugepage": by_granularity.get(3, 0),
        },
        "total_traced_bits":           total_traced_bits,
        "total_traced_bytes":          total_traced_bits // 8,
        "total_pages_resident":        total_pages_resident,
        "theoretical_max_bits_RSS":    theoretical_max_bits,
        "coverage_vs_RSS_percent":     coverage_pct,
        "interpretation":              _interpret(coverage_pct, by_granularity),
    }


def _interpret(coverage_pct, by_gran):
    if coverage_pct is None:
        return "Aucun snapshot tracer trouve : impossible de calculer le ratio."
    if by_gran.get(2, 0) == 0:
        return ("0 fichier en granularite BIT detecte : la tracabilite reelle "
                "est en realite par PAGE (4 KiB) ou par BYTE. Le label "
                "'bit-level' decrit l'ABI lum_t (alignement 64 octets) mais "
                "non la granularite des snapshots actuels.")
    return f"Couverture effective ~{coverage_pct:.2f}% du RSS resident."


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--logs-dir", default="logs",
                    help="Repertoire racine de logs (defaut: logs)")
    ap.add_argument("--json", action="store_true", help="Sortie JSON")
    args = ap.parse_args(argv)

    root = Path(args.logs_dir)
    records = []
    for sub in ("forensic/modules", "lum_native", "forensic/sessions"):
        records.extend(scan_dir(root / sub))
    summary = aggregate(records)

    if args.json:
        print(json.dumps({"summary": summary, "files": records},
                         indent=2, default=str))
        return 0

    print("=" * 72)
    print("LUM bit-level coverage report (C124)")
    print("=" * 72)
    print(f"Logs root      : {root.resolve()}")
    print(f"Files scanned  : {summary['files_scanned']}")
    print(f"Files valid    : {summary['files_valid']}")
    print(f"Granularity    : page={summary['by_granularity']['page']} "
          f"byte={summary['by_granularity']['byte']} "
          f"bit={summary['by_granularity']['bit']} "
          f"hugepage={summary['by_granularity']['hugepage']}")
    print(f"Traced bits    : {summary['total_traced_bits']:,}")
    print(f"Traced bytes   : {summary['total_traced_bytes']:,}")
    print(f"Pages resident : {summary['total_pages_resident']:,} "
          f"({summary['total_pages_resident']*PAGE_SIZE//1024} KiB max RSS)")
    if summary["theoretical_max_bits_RSS"]:
        print(f"Coverage / RSS : {summary['coverage_vs_RSS_percent']:.4f}%")
    else:
        print("Coverage / RSS : n/a (pas de snapshot baseline trouve)")
    print()
    print("Interpretation :")
    print(f"  {summary['interpretation']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
