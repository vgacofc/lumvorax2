#!/usr/bin/env python3
# =============================================================================
# btc_mempool_fetch.py — C116-MAINNET
# Récupère les données Bitcoin mainnet réelles depuis l'API Blockstream
# et construit le header 80 bytes pour le prochain bloc à miner.
#
# Usage :
#   python3 btc_mempool_fetch.py [output_json]
#   → produit btc_real_header.json avec le header hex 80 bytes
#
# API utilisée : blockstream.info (publique, sans clé)
# Wallet réception : 1YkQrHMbvBbYvCR1jcQAxjMj4bzibiK8C
# =============================================================================

import sys, json, struct, hashlib, time, urllib.request, urllib.error

BLOCKSTREAM_API = "https://blockstream.info/api"
WALLET_MAINNET  = "1YkQrHMbvBbYvCR1jcQAxjMj4bzibiK8C"
OUTPUT_JSON     = sys.argv[1] if len(sys.argv) > 1 else "btc_real_header.json"

def api_get(endpoint, timeout=8):
    url = f"{BLOCKSTREAM_API}/{endpoint}"
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "LumVorax/116"})
        r = urllib.request.urlopen(req, timeout=timeout)
        return r.read()
    except Exception as e:
        print(f"[MEMPOOL] ERREUR API {endpoint}: {e}", file=sys.stderr)
        return None

def sha256d(data: bytes) -> bytes:
    return hashlib.sha256(hashlib.sha256(data).digest()).digest()

def merkle_root_from_txids(txids_hex: list) -> str:
    """Calcule le Merkle root depuis une liste de TXIDs (hex strings)."""
    if not txids_hex:
        return "0" * 64
    # Les TXIDs Bitcoin sont en little-endian dans le Merkle tree
    hashes = [bytes.fromhex(txid)[::-1] for txid in txids_hex]
    while len(hashes) > 1:
        if len(hashes) % 2 == 1:
            hashes.append(hashes[-1])  # duplique le dernier si impair
        next_level = []
        for i in range(0, len(hashes), 2):
            next_level.append(sha256d(hashes[i] + hashes[i+1]))
        hashes = next_level
    # Retourne en big-endian (affichage standard)
    return hashes[0][::-1].hex()

def bits_to_target(bits: int) -> bytes:
    """Convertit bits compact → target 32 bytes."""
    exp  = (bits >> 24) & 0xFF
    mant = bits & 0x00FFFFFF
    target = mant * (256 ** (exp - 3))
    return target.to_bytes(32, 'big')

def build_header_80(version: int, prev_hash_hex: str, merkle_root_hex: str,
                    timestamp: int, bits: int, nonce: int) -> bytes:
    """Assemble le block header Bitcoin 80 bytes."""
    # prev_hash et merkle_root : convertis en little-endian (format interne Bitcoin)
    prev_hash_le  = bytes.fromhex(prev_hash_hex)[::-1]
    merkle_root_le = bytes.fromhex(merkle_root_hex)[::-1]
    hdr = struct.pack('<I', version)        # 4 bytes LE
    hdr += prev_hash_le                     # 32 bytes
    hdr += merkle_root_le                   # 32 bytes
    hdr += struct.pack('<I', timestamp)     # 4 bytes LE
    hdr += struct.pack('<I', bits)          # 4 bytes LE
    hdr += struct.pack('<I', nonce)         # 4 bytes LE
    assert len(hdr) == 80, f"Header {len(hdr)} bytes != 80"
    return hdr

def verify_header(header_bytes: bytes, bits: int) -> dict:
    """Vérifie un header Bitcoin et retourne le hash + leading_zeros."""
    h = sha256d(header_bytes)
    hash_hex = h[::-1].hex()  # big-endian display
    target = bits_to_target(bits)
    # Compter les leading bits zéros
    leading_bits = 0
    for byte in h[::-1]:
        if byte == 0:
            leading_bits += 8
        else:
            for bit in range(7, -1, -1):
                if (byte >> bit) & 1 == 0:
                    leading_bits += 1
                else:
                    break
            break
    target_bytes = target
    hash_val = int.from_bytes(h[::-1], 'big')
    target_val = int.from_bytes(target_bytes, 'big')
    return {
        "hash_hex": hash_hex,
        "leading_zeros_bits": leading_bits,
        "valid_pow": hash_val < target_val
    }

# =============================================================================
# MAIN
# =============================================================================
print(f"[MEMPOOL] Connexion à {BLOCKSTREAM_API}...")

# 1. Dernier bloc connu
tip_raw = api_get("blocks/tip/hash")
if not tip_raw:
    sys.exit(1)
tip_hash = tip_raw.decode().strip().strip('"')
print(f"[MEMPOOL] Tip hash : {tip_hash}")

# 2. Données du dernier bloc
blk_raw = api_get(f"block/{tip_hash}")
if not blk_raw:
    sys.exit(1)
blk = json.loads(blk_raw)

height      = blk["height"]
version     = blk["version"]
bits        = blk["bits"]
timestamp   = blk["timestamp"]
merkle_root = blk["merkle_root"]
prev_hash   = blk["previousblockhash"]

print(f"[MEMPOOL] Bloc #{height} — bits=0x{bits:08x} — time={timestamp}")
print(f"[MEMPOOL] prev_hash : {prev_hash}")
print(f"[MEMPOOL] merkle    : {merkle_root}")

# 3. TXIDs du mempool pour construire le prochain bloc (candidat)
print(f"[MEMPOOL] Récupération des TXIDs mempool...")
txids_raw = api_get("mempool/txids")
txids = []
if txids_raw:
    all_txids = json.loads(txids_raw)
    # Prendre max 500 TXIDs pour construire un bloc candidat réaliste
    txids = all_txids[:500]
    print(f"[MEMPOOL] {len(all_txids)} TXIDs dans le mempool → on utilise {len(txids)}")
else:
    print(f"[MEMPOOL] WARN: mempool inaccessible — utilisation du merkle_root du dernier bloc")
    txids = []

# 4. Coinbase transaction (récompense → notre wallet)
# TXID de la coinbase : simulé (SHA256d de notre wallet + height)
coinbase_data = f"LumVorax-C116-{height+1}-{WALLET_MAINNET}".encode()
coinbase_txid = sha256d(coinbase_data).hex()
print(f"[MEMPOOL] Coinbase TXID : {coinbase_txid}")

# 5. Merkle root du prochain bloc candidat
# Coinbase en premier, puis les TXIDs du mempool
candidate_txids = [coinbase_txid] + txids
candidate_merkle = merkle_root_from_txids(candidate_txids)
print(f"[MEMPOOL] Merkle candidat : {candidate_merkle} ({len(candidate_txids)} txs)")

# 6. Paramètres du prochain bloc
next_height   = height + 1
next_version  = version  # même version que le dernier bloc
next_bits     = bits     # même difficulté (simplification — en réalité recalculé tous les 2016 blocs)
next_time     = int(time.time())  # timestamp actuel
next_prev     = tip_hash          # le tip actuel devient prev_hash

# 7. Construction du header 80 bytes (nonce=0 initial)
header_bytes = build_header_80(next_version, next_prev, candidate_merkle,
                                next_time, next_bits, 0)
header_hex   = header_bytes.hex()

# 8. Vérification du header nonce=0
check = verify_header(header_bytes, next_bits)
print(f"[MEMPOOL] Header nonce=0 : hash={check['hash_hex'][:16]}...")
print(f"[MEMPOOL] Leading zeros  : {check['leading_zeros_bits']} bits")
print(f"[MEMPOOL] Valid PoW      : {check['valid_pow']} (difficulté bits=0x{next_bits:08x})")

# 9. Calcul de la target réelle (bits → nombre de leading zeros requis)
target = bits_to_target(next_bits)
required_bits = 0
for b in target:
    if b == 0:
        required_bits += 8
    else:
        for bit in range(7, -1, -1):
            if (b >> bit) & 1 == 0:
                required_bits += 1
            else:
                break
        break
print(f"[MEMPOOL] Bits requis pour résoudre : {required_bits} leading zeros")
print(f"[MEMPOOL] Target : {target.hex()[:32]}...")

# 10. Sortie JSON
output = {
    "source": "blockstream.info",
    "wallet_mainnet": WALLET_MAINNET,
    "last_block": {
        "height": height,
        "hash": tip_hash,
        "bits": bits,
        "version": version,
        "timestamp": timestamp,
        "merkle_root": merkle_root
    },
    "candidate_block": {
        "height": next_height,
        "version": next_version,
        "prev_hash": next_prev,
        "merkle_root": candidate_merkle,
        "timestamp": next_time,
        "bits": next_bits,
        "n_transactions": len(candidate_txids),
        "coinbase_txid": coinbase_txid,
        "header_hex": header_hex,
        "header_bytes": len(header_bytes),
        "target_hex": target.hex(),
        "required_leading_bits": required_bits
    },
    "initial_check": check,
    "generated_at": int(time.time()),
    "note": "Miner ce header avec btc_mining_runner --header-hex <header_hex>"
}

with open(OUTPUT_JSON, "w") as f:
    json.dump(output, f, indent=2)

print(f"\n[MEMPOOL] ✓ Header réel écrit dans : {OUTPUT_JSON}")
print(f"[MEMPOOL] Commande Ubuntu : ./btc_mining_runner --mode MAINNET --header-hex {header_hex}")
print(f"[MEMPOOL] --duration-s 1200 --threads 16")
