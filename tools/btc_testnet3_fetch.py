#!/usr/bin/env python3
"""btc_testnet3_fetch.py — Fetch real testnet3 block headers via blockstream.info.

Cycle C99 — Réponse à la critique experte « le projet reste une simulation
avancée tant que le header testnet3 réel n'est pas branché ».

Usage :
    python3 tools/btc_testnet3_fetch.py [--height N | --tip] [--out FILE.json]

Sortie : JSON contenant
    {
      "height": int,
      "hash": str (hex 64),
      "raw_header_hex": str (hex 160 = 80 bytes Bitcoin block header),
      "version": int,
      "previousblockhash": str,
      "merkle_root": str,
      "timestamp": int,
      "bits": int,
      "nonce": int,
      "difficulty": float,
      "verified": bool   # double-SHA256(raw_header) == hash ?
    }

Le `raw_header_hex` est le header binaire Bitcoin standard (80 bytes) :
   4   version (LE)
   32  previousblockhash (LE)
   32  merkle_root (LE)
   4   timestamp (LE)
   4   bits (LE)
   4   nonce (LE)

→ utilisé directement par btc_qm_engine_real_target_search() pour le hashing.
"""
import argparse, hashlib, json, sys, urllib.request

API_BASE = "https://blockstream.info/testnet/api"


def http_get(url: str, timeout: float = 10.0) -> str:
    req = urllib.request.Request(url, headers={"User-Agent": "lumvorax-c99/1.0"})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return r.read().decode().strip()


def fetch_header(height: int | None) -> dict:
    if height is None:
        h = int(http_get(f"{API_BASE}/blocks/tip/height"))
    else:
        h = int(height)
    block_hash = http_get(f"{API_BASE}/block-height/{h}")
    meta = json.loads(http_get(f"{API_BASE}/block/{block_hash}"))
    raw_hex = http_get(f"{API_BASE}/block/{block_hash}/header")
    if len(raw_hex) != 160:
        raise ValueError(f"raw header len {len(raw_hex)} != 160 (80 bytes)")
    raw = bytes.fromhex(raw_hex)
    computed = hashlib.sha256(hashlib.sha256(raw).digest()).digest()[::-1].hex()
    verified = computed == block_hash
    return {
        "height": h,
        "hash": block_hash,
        "raw_header_hex": raw_hex,
        "version": meta.get("version"),
        "previousblockhash": meta.get("previousblockhash"),
        "merkle_root": meta.get("merkle_root"),
        "timestamp": meta.get("timestamp"),
        "bits": meta.get("bits"),
        "nonce": meta.get("nonce"),
        "difficulty": meta.get("difficulty"),
        "verified": verified,
        "computed_hash": computed,
    }


def main() -> int:
    p = argparse.ArgumentParser()
    g = p.add_mutually_exclusive_group()
    g.add_argument("--height", type=int, help="hauteur du bloc à récupérer")
    g.add_argument("--tip", action="store_true", help="récupérer le tip courant")
    p.add_argument("--out", type=str, default=None, help="fichier JSON sortie")
    args = p.parse_args()
    h = None if args.tip or args.height is None else args.height
    data = fetch_header(h)
    out = json.dumps(data, indent=2)
    if args.out:
        with open(args.out, "w") as fp:
            fp.write(out + "\n")
        print(f"[testnet3] {args.out} → height={data['height']} hash={data['hash'][:16]}... "
              f"verified={data['verified']} bits={data['bits']:08x} nonce={data['nonce']}")
    else:
        print(out)
    return 0 if data.get("verified") else 1


if __name__ == "__main__":
    sys.exit(main())
