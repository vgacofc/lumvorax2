#!/usr/bin/env python3
"""
LUM Parser — Cycle C112-finition

Lit les fichiers `.lum` produits par :
  - lum_log_encoder.c    (magic 'LMUL' = 0x4C554D4C, événements de log)
  - lum_memory_tracer.c  (magic 'LUMT' = 0x4C554D54, snapshot mémoire process)

Produit un rapport HTML lisible (sans dépendance externe) avec :
  - Liste des événements (texte / metric / record / décision / observable quantique)
  - Statistiques snapshots mémoire baseline → final (delta pages, delta octets)
  - Diagramme SVG inline (barres baseline vs final)

Usage :
    python3 tools/lum_parser.py logs/forensic/modules/btc_lum_log_<run>.lum
    python3 tools/lum_parser.py --html out.html  logs/forensic/modules/btc_lum_log_<run>.lum
    python3 tools/lum_parser.py --html out.html  --baseline B.lum --final F.lum
"""
from __future__ import annotations
import argparse
import struct
import sys
import os
import html
from typing import List, Optional, Tuple

LUM_LOG_MAGIC = 0x4C554D4C   # 'LMUL'
LUM_TRACER_MAGIC = 0x4C554D54  # 'LUMT'

LUM_LOG_KIND = {
    10: "INFO",
    11: "WARN",
    12: "ERROR",
    20: "METRIC",
    30: "RECORD",
    40: "DECISION",
    50: "QUANTUM_OBS",
    255: "CONTINUATION",
}

LUM_T_SIZE = 64

# Conforme src/lum/lum_core.h :
# uint32 id, uint8×4 (presence/structure_type/is_destroyed/reserved_flags),
# int32×2 (position_x,position_y), uint64 timestamp, uint64 memory_address,
# uint32 checksum, uint32 magic, uint8[20] padding, +4 octets de padding final
# imposés par l'alignement 8 octets de la struct (sizeof(lum_t)==64).
LUM_T_FMT = "<IBBBBiiQQII20s4x"


def parse_lum_t(buf: bytes) -> dict:
    if len(buf) < LUM_T_SIZE:
        return {}
    (lid, presence, structure_type, is_destroyed, reserved_flags,
     pos_x, pos_y, ts_ns, mem_addr, checksum, magic, padding) = struct.unpack(
        LUM_T_FMT, buf[:LUM_T_SIZE]
    )
    return {
        "id": lid,
        "presence": presence,
        "structure_type": structure_type,
        "is_destroyed": is_destroyed,
        "reserved_flags": reserved_flags,
        "position_x": pos_x,
        "position_y": pos_y,
        "timestamp_ns": ts_ns,
        "memory_address": mem_addr,
        "checksum": checksum,
        "magic": magic,
        "padding": padding,
    }


def parse_log_file(path: str) -> List[dict]:
    events = []
    with open(path, "rb") as f:
        data = f.read()
    n_lums = len(data) // LUM_T_SIZE
    pending_text: Optional[bytes] = None
    pending_kind: Optional[int] = None
    pending_payload_len = 0
    for i in range(n_lums):
        chunk = data[i * LUM_T_SIZE:(i + 1) * LUM_T_SIZE]
        lum = parse_lum_t(chunk)
        if not lum:
            continue
        if lum["magic"] != LUM_LOG_MAGIC:
            continue
        kind = lum["structure_type"]
        if kind == 0xFF:
            if pending_text is not None:
                pos_bytes = struct.pack("<ii", lum["position_x"], lum["position_y"])
                pending_text += pos_bytes + lum["padding"]
            continue
        if pending_text is not None and pending_kind is not None:
            events.append({
                "kind": LUM_LOG_KIND.get(pending_kind, f"K{pending_kind}"),
                "text": pending_text[:pending_payload_len].decode("utf-8", errors="replace"),
                "id": lum["id"] - 1,
            })
            pending_text = None
        pending_kind = kind
        pending_payload_len = lum["reserved_flags"]
        if pending_payload_len == 0:
            pending_payload_len = 20
        pending_text = bytes(lum["padding"])
        if pending_payload_len <= 20:
            events.append({
                "kind": LUM_LOG_KIND.get(kind, f"K{kind}"),
                "text": pending_text[:pending_payload_len].decode("utf-8", errors="replace"),
                "id": lum["id"],
                "ts_ns": lum["timestamp_ns"],
            })
            pending_text = None
            pending_kind = None
    if pending_text is not None and pending_kind is not None:
        events.append({
            "kind": LUM_LOG_KIND.get(pending_kind, f"K{pending_kind}"),
            "text": pending_text[:pending_payload_len].decode("utf-8", errors="replace"),
            "id": -1,
        })
    return events


# Conforme src/lum/lum_memory_tracer.c (struct __attribute__((packed))) :
# uint32 magic, uint32 granularity, uint64 total_lums,
# uint64 total_bytes, uint64 timestamp_ns  → 32 octets fixes.
TRACER_HEADER_FMT = "<IIQQQ"


def parse_tracer_header(path: str) -> Optional[dict]:
    with open(path, "rb") as f:
        head = f.read(struct.calcsize(TRACER_HEADER_FMT))
    if len(head) < struct.calcsize(TRACER_HEADER_FMT):
        return None
    (magic, granularity, total_lums, total_bytes, timestamp_ns) = struct.unpack(
        TRACER_HEADER_FMT, head
    )
    if magic != LUM_TRACER_MAGIC:
        return None
    return {
        "path": path,
        "size_on_disk": os.path.getsize(path),
        "magic": magic,
        "granularity": granularity,
        "total_lums": total_lums,
        "total_bytes": total_bytes,
        "timestamp_ns": timestamp_ns,
    }


def render_html(log_events: List[dict],
                baseline: Optional[dict],
                final: Optional[dict],
                source_log: Optional[str]) -> str:
    rows = []
    for e in log_events[:5000]:
        rows.append(
            f"<tr><td>{e.get('id', '?')}</td>"
            f"<td>{html.escape(e['kind'])}</td>"
            f"<td><pre>{html.escape(e['text'])}</pre></td></tr>"
        )

    delta_section = ""
    svg = ""
    if baseline and final:
        d_lums = int(final["total_lums"]) - int(baseline["total_lums"])
        d_bytes = int(final["total_bytes"]) - int(baseline["total_bytes"])
        delta_section = f"""
<h2>Delta mémoire (final - baseline)</h2>
<table>
  <tr><th>métrique</th><th>baseline</th><th>final</th><th>delta</th></tr>
  <tr><td>lums émis</td><td>{baseline['total_lums']:,}</td><td>{final['total_lums']:,}</td><td>{d_lums:+,}</td></tr>
  <tr><td>octets dumpés</td><td>{baseline['total_bytes']:,}</td><td>{final['total_bytes']:,}</td><td>{d_bytes:+,}</td></tr>
  <tr><td>taille .lum on-disk</td><td>{baseline['size_on_disk']:,}</td><td>{final['size_on_disk']:,}</td><td>{final['size_on_disk']-baseline['size_on_disk']:+,}</td></tr>
</table>
"""
        max_b = max(baseline["total_bytes"], final["total_bytes"], 1)
        bw = int(700 * baseline["total_bytes"] / max_b)
        fw = int(700 * final["total_bytes"] / max_b)
        svg = f"""
<svg width="800" height="160" xmlns="http://www.w3.org/2000/svg">
  <text x="10" y="20" font-family="monospace" font-size="14">Baseline (octets dumpés)</text>
  <rect x="10" y="30" width="{bw}" height="30" fill="#3a86ff"/>
  <text x="{bw + 20}" y="50" font-family="monospace" font-size="12">{baseline['total_bytes']:,}</text>
  <text x="10" y="100" font-family="monospace" font-size="14">Final (octets dumpés)</text>
  <rect x="10" y="110" width="{fw}" height="30" fill="#fb5607"/>
  <text x="{fw + 20}" y="130" font-family="monospace" font-size="12">{final['total_bytes']:,}</text>
</svg>
"""

    src_label = html.escape(source_log) if source_log else "(aucun .lum log fourni)"
    return f"""<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="utf-8">
<title>LUM Parser — rapport</title>
<style>
  body{{font-family:Arial,sans-serif;margin:24px;background:#fafafa}}
  h1,h2{{color:#1d3557}}
  table{{border-collapse:collapse;margin-bottom:24px;background:#fff}}
  td,th{{border:1px solid #ccc;padding:6px 10px;font-size:13px}}
  th{{background:#1d3557;color:#fff}}
  pre{{margin:0;font-size:12px;white-space:pre-wrap;max-width:760px}}
  .sub{{color:#777;font-size:12px}}
</style>
</head>
<body>
<h1>LUM Parser — rapport (C112)</h1>
<p class="sub">Source log : {src_label} — {len(log_events)} événements parsés</p>
{delta_section}
{svg}
<h2>Événements log ({min(len(log_events), 5000)} affichés)</h2>
<table>
  <thead><tr><th>id</th><th>kind</th><th>texte</th></tr></thead>
  <tbody>
    {''.join(rows)}
  </tbody>
</table>
</body>
</html>
"""


def main(argv: List[str]) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("log", nargs="?", help="fichier .lum (lum_log_encoder)")
    ap.add_argument("--baseline", help="fichier .lum (mem baseline)")
    ap.add_argument("--final", help="fichier .lum (mem final)")
    ap.add_argument("--html", help="chemin du rapport HTML à produire")
    args = ap.parse_args(argv)

    log_events: List[dict] = []
    baseline = None
    final = None

    if args.log:
        log_events = parse_log_file(args.log)
        print(f"[LUM-PARSER] {len(log_events)} événements lus depuis {args.log}")
        for e in log_events[:20]:
            print(f"  #{e.get('id', '?'):>4}  {e['kind']:<14}  {e['text']!r}")

    if args.baseline:
        baseline = parse_tracer_header(args.baseline)
        if baseline:
            print(f"[LUM-PARSER] baseline : {baseline['total_lums']:,} lums, "
                  f"{baseline['total_bytes']:,} octets dumpés, "
                  f"taille fichier {baseline['size_on_disk']:,} o")
    if args.final:
        final = parse_tracer_header(args.final)
        if final:
            print(f"[LUM-PARSER] final    : {final['total_lums']:,} lums, "
                  f"{final['total_bytes']:,} octets dumpés, "
                  f"taille fichier {final['size_on_disk']:,} o")
    if baseline and final:
        d_lums = int(final['total_lums']) - int(baseline['total_lums'])
        d_bytes = int(final['total_bytes']) - int(baseline['total_bytes'])
        print(f"[LUM-PARSER] delta    : {d_lums:+,} lums, {d_bytes:+,} octets")

    if args.html:
        out = render_html(log_events, baseline, final, args.log)
        with open(args.html, "w", encoding="utf-8") as f:
            f.write(out)
        print(f"[LUM-PARSER] rapport HTML écrit → {args.html}")

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
