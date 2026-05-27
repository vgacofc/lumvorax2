#!/usr/bin/env python3
"""qdayprize_escalation.py — Escalade QDAYPRIZE 24 → 256 bits (C99 Goal #16)

Mesure le temps réel de cassage classique (CPU SHA256d) pour chaque difficulté
en bits, de 24 (cible QDAYPRIZE officielle) jusqu'à 256 (limite Bitcoin
mainnet hypothétique). À 60 s par cible, on extrapole les difficultés
inatteignables et on documente la limite réelle de notre technologie classique.

Usage :
    python3 tools/qdayprize_escalation.py [--out FILE.json] [--timeout_s 60]
                                          [--start 24] [--end 256] [--step 4]

Sortie : JSON avec, pour chaque bit-target :
    bits, found, nonce_found, hash_hex, lz_obtained, elapsed_s, hashrate_mhs
"""
import argparse, hashlib, json, os, struct, sys, time

def double_sha256(b: bytes) -> bytes:
    return hashlib.sha256(hashlib.sha256(b).digest()).digest()

def count_lz_bits(h: bytes) -> int:
    """Leading-zero bits du hash en représentation big-endian."""
    lz = 0
    for byte in h[::-1]:  # Bitcoin little-endian → MSB côté queue
        if byte == 0:
            lz += 8
        else:
            for bit in range(7, -1, -1):
                if (byte >> bit) & 1:
                    return lz
                lz += 1
            break
    return lz

def mine_until_target(target_bits: int, timeout_s: float, header_template: bytes):
    """Mine jusqu'à trouver hash avec ≥ target_bits LZ ou expirer le timeout.

    Retourne (found, nonce, hash_hex, lz_obtained, elapsed_s, hashes_done)."""
    t0 = time.time()
    nonce = 0
    best_lz = 0
    best_nonce = 0
    best_hash = b'\x00' * 32
    while True:
        # Construit header (80 octets) avec nonce variable (4 derniers octets)
        hdr = header_template[:76] + struct.pack('<I', nonce & 0xFFFFFFFF)
        h = double_sha256(hdr)
        lz = count_lz_bits(h)
        if lz > best_lz:
            best_lz = lz
            best_nonce = nonce
            best_hash = h
            if lz >= target_bits:
                elapsed = time.time() - t0
                return True, nonce, h.hex(), lz, elapsed, nonce + 1
        if (nonce & 0xFFFFF) == 0:  # check timeout toutes les ~1M
            if time.time() - t0 > timeout_s:
                elapsed = time.time() - t0
                return False, best_nonce, best_hash.hex(), best_lz, elapsed, nonce + 1
        nonce += 1
        if nonce > 0xFFFFFFFFFF:  # 40 bits max pour éviter boucle infinie
            elapsed = time.time() - t0
            return False, best_nonce, best_hash.hex(), best_lz, elapsed, nonce

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', default='logs/qdayprize_escalation.json')
    ap.add_argument('--timeout_s', type=float, default=60.0)
    ap.add_argument('--start', type=int, default=24)
    ap.add_argument('--end', type=int, default=256)
    ap.add_argument('--step', type=int, default=4)
    args = ap.parse_args()

    # Header template : merkle_root = SHA256("LUMVORAX-QDAYPRIZE-2026")
    magic_root = hashlib.sha256(b'LUMVORAX-QDAYPRIZE-2026').digest()
    header_template = (
        struct.pack('<I', 1) +              # version
        b'\x00' * 32 +                       # prev_hash zéro
        magic_root +                         # merkle_root
        struct.pack('<I', int(time.time())) +# timestamp
        struct.pack('<I', 0x1d00ffff) +      # bits
        struct.pack('<I', 0)                 # nonce (placeholder)
    )
    assert len(header_template) == 80, len(header_template)

    print(f'[QDPR-ESC] Escalade {args.start}→{args.end} (step {args.step}), '
          f'timeout {args.timeout_s}s par cible', flush=True)

    results = []
    last_hashrate = None
    for bits in range(args.start, args.end + 1, args.step):
        # Optimisation : si la cible précédente a explosé timeout, on extrapole
        if last_hashrate and last_hashrate > 0:
            expected_hashes = 2 ** bits
            expected_seconds = expected_hashes / last_hashrate
            if expected_seconds > args.timeout_s * 100:
                # Extrapolation : déjà inatteignable
                results.append({
                    'bits': bits, 'found': False,
                    'extrapolated': True,
                    'expected_seconds': expected_seconds,
                    'expected_years': expected_seconds / (365.25 * 86400),
                    'extrapolated_from_hashrate_hps': last_hashrate,
                })
                print(f'  bits={bits:3d} EXTRAPOLE inatteignable '
                      f'({expected_seconds:.2e}s = {expected_seconds/3.156e7:.2e} ans)',
                      flush=True)
                continue
        found, nonce, hash_hex, lz, elapsed, hashes_done = mine_until_target(
            bits, args.timeout_s, header_template)
        rate = hashes_done / max(elapsed, 1e-6)
        last_hashrate = rate
        entry = {
            'bits': bits, 'found': found, 'nonce_found': nonce,
            'hash_hex': hash_hex, 'lz_obtained': lz,
            'elapsed_s': round(elapsed, 3),
            'hashes_done': hashes_done,
            'hashrate_hps': round(rate, 1),
            'hashrate_mhs': round(rate / 1e6, 4),
        }
        results.append(entry)
        flag = '✅' if found else '⏱'
        print(f'  bits={bits:3d} {flag} lz={lz:3d} t={elapsed:6.2f}s '
              f'rate={rate/1e6:6.2f} MH/s', flush=True)

    out = {
        'cycle': 'C99 Goal #16',
        'date': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
        'timeout_s': args.timeout_s,
        'header_template_hex': header_template.hex(),
        'results': results,
    }
    os.makedirs(os.path.dirname(args.out) or '.', exist_ok=True)
    with open(args.out, 'w') as f:
        json.dump(out, f, indent=2)
    print(f'[QDPR-ESC] Sortie sauvée: {args.out}', flush=True)

    # Résumé
    print('\n=== RÉSUMÉ ===')
    print(f'  Cibles testées : {len(results)}')
    found_count = sum(1 for r in results if r.get('found'))
    print(f'  Cibles atteintes : {found_count}')
    if found_count > 0:
        max_found = max((r['bits'] for r in results if r.get('found')), default=0)
        print(f'  Meilleure difficulté atteinte : {max_found} bits')
    return 0

if __name__ == '__main__':
    sys.exit(main())
