#!/usr/bin/env python3
"""
LumVorax — Module 17 — Bitcoin Quantum Mining Engine
scripts/fetch_btc_real_pow.py — Fetch de vrais block headers Bitcoin

Usage:
    python3 fetch_btc_real_pow.py [<height>]
    python3 fetch_btc_real_pow.py recent          → 5 derniers blocs

Description:
    Récupère un vrai block header Bitcoin depuis l'API Blockstream.info
    (accès public, sans clé API, sans Bitcoin Core requis).
    Sortie : header hex 160 chars (80 bytes = structure Bitcoin header).

    Utilisation avec le moteur de minage :
    HEADER=$(python3 scripts/fetch_btc_real_pow.py | grep '^[0-9a-f]')
    ./btc_mining_runner --header-hex "$HEADER" --duration-s 60

    Structure du header Bitcoin (80 bytes) :
      bytes  0- 3 : version (LE uint32)
      bytes  4-35 : prev_block_hash (32 bytes)
      bytes 36-67 : merkle_root (32 bytes)
      bytes 68-71 : timestamp (LE uint32)
      bytes 72-75 : bits — difficulté compacte (LE uint32)
      bytes 76-79 : nonce (LE uint32 — vrai nonce gagnant)

    Note : le moteur ignorera le nonce du header réel (il explore l'espace complet).
    Le but est d'utiliser les vrais champs version/prev_hash/merkle_root/timestamp/bits
    pour un test SHA-256 sur données Bitcoin authentiques.

Conformité : STANDARD_NAMES.md v4.2 §M-BTC17-C38
Ref : analysechatgpt91.38.md §C38-REAL-POW — 2026-04-12
"""

import sys
import urllib.request
import urllib.error
import json


BASE_API = "https://blockstream.info/api"
TIMEOUT  = 10


def fetch_url(url):
    """Fetch une URL et retourne le contenu en bytes."""
    with urllib.request.urlopen(url, timeout=TIMEOUT) as r:
        return r.read()


def fetch_block_header_hex(block_hash):
    """Retourne le header hex (160 chars) pour un block hash donné."""
    url = f"{BASE_API}/block/{block_hash}/header"
    return fetch_url(url).decode().strip()


def fetch_block_info(block_hash):
    """Retourne les infos JSON d'un bloc."""
    url = f"{BASE_API}/block/{block_hash}"
    return json.loads(fetch_url(url).decode())


def fetch_tip_height():
    """Retourne la hauteur du dernier bloc."""
    url = f"{BASE_API}/blocks/tip/height"
    return int(fetch_url(url).decode().strip())


def fetch_hash_at_height(height):
    """Retourne le block hash à une hauteur donnée."""
    url = f"{BASE_API}/block-height/{height}"
    return fetch_url(url).decode().strip()


def decode_header(header_hex):
    """Décode un header hex Bitcoin en champs lisibles."""
    if len(header_hex) < 160:
        return None
    raw = bytes.fromhex(header_hex[:160])
    import struct
    version    = struct.unpack_from('<I', raw,  0)[0]
    prev_hash  = raw[4:36][::-1].hex()   # big-endian pour affichage
    merkle     = raw[36:68][::-1].hex()  # big-endian pour affichage
    timestamp  = struct.unpack_from('<I', raw, 68)[0]
    bits       = struct.unpack_from('<I', raw, 72)[0]
    nonce      = struct.unpack_from('<I', raw, 76)[0]
    return {
        'version':   version,
        'prev_hash': prev_hash,
        'merkle':    merkle,
        'timestamp': timestamp,
        'bits':      bits,
        'nonce':     nonce,
    }


def count_leading_zeros(header_hex, nonce=None):
    """Calcule les leading zeros du double-SHA256 d'un header.
    Si nonce fourni, remplace le nonce dans le header.
    Note: nécessite hashlib (stdlib Python)."""
    import hashlib, struct
    raw = bytearray.fromhex(header_hex[:160])
    if nonce is not None:
        struct.pack_into('<I', raw, 76, nonce)
    h1 = hashlib.sha256(raw).digest()
    h2 = hashlib.sha256(h1).digest()
    lz = 0
    for byte in h2:
        if byte == 0:
            lz += 8
        else:
            for bit in range(7, -1, -1):
                if (byte >> bit) & 1:
                    break
                lz += 1
            break
    return lz, h2.hex()


def main():
    args = sys.argv[1:]
    mode = args[0] if args else None

    try:
        if mode == "recent" or mode == "5":
            # Afficher les 5 derniers blocs
            tip = fetch_tip_height()
            print(f"# LumVorax — Vrais blocs Bitcoin récents (tip={tip})")
            print(f"# Timestamp         Height  Bits         Leading  Header (premiers 20 bytes...)")
            for h in range(tip, tip - 5, -1):
                bh  = fetch_hash_at_height(h)
                hdr = fetch_block_header_hex(bh)
                info = decode_header(hdr)
                lz, hash_hex = count_leading_zeros(hdr)
                print(f"# Block #{h:<7d}  bits=0x{info['bits']:08x}  lz={lz:2d} bits  {hdr[:40]}...")
            print()
            print("# Pour utiliser le dernier bloc avec le moteur :")
            tip_hash = fetch_hash_at_height(tip)
            tip_hdr  = fetch_block_header_hex(tip_hash)
            print(f"# ./btc_mining_runner --header-hex {tip_hdr} --duration-s 60")
            print()
            print(f"# Header complet du bloc tip #{tip} :")
            print(tip_hdr)

        elif mode and mode.isdigit():
            height = int(mode)
            block_hash = fetch_hash_at_height(height)
            header_hex = fetch_block_header_hex(block_hash)
            info = decode_header(header_hex)
            lz, hash_hex = count_leading_zeros(header_hex)
            print(f"# Bloc Bitcoin #{height}")
            print(f"# Hash  : {block_hash}")
            print(f"# version=0x{info['version']:08x}  bits=0x{info['bits']:08x}  ts={info['timestamp']}")
            print(f"# nonce original: {info['nonce']} → leading_zeros avec nonce réel : {lz} bits")
            print(f"# double-SHA256(header) = {hash_hex}")
            print(f"# Utilisation : ./btc_mining_runner --header-hex <header> --duration-s 60")
            print()
            print(header_hex)

        else:
            # Mode par défaut : dernier bloc
            tip    = fetch_tip_height()
            bh     = fetch_hash_at_height(tip)
            hdr    = fetch_block_header_hex(bh)
            info   = decode_header(hdr)
            lz, hash_hex = count_leading_zeros(hdr)
            print(f"# Bloc Bitcoin tip #{tip} — {bh}")
            print(f"# version=0x{info['version']:08x}  bits=0x{info['bits']:08x}  ts={info['timestamp']}")
            print(f"# nonce original: {info['nonce']} → leading_zeros nonce réel : {lz} bits")
            print(f"# SHA256d(header_with_real_nonce) = {hash_hex}")
            print(f"# API : {BASE_API}/block/{bh}/header")
            print()
            print(f"# Commande d'utilisation LumVorax Module 17 :")
            print(f"# ./btc_mining_runner --header-hex {hdr} --duration-s 60")
            print()
            print(hdr)

    except urllib.error.URLError as e:
        print(f"ERREUR réseau : {e}", file=sys.stderr)
        print(f"L'API Blockstream.info n'est pas accessible depuis cet environnement.", file=sys.stderr)
        print(f"Alternative : utiliser un vrai nœud Bitcoin (bitcoind --getblocktemplate)", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"ERREUR : {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
