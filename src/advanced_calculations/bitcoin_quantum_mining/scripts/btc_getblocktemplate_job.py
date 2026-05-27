#!/usr/bin/env python3
import argparse
import base64
import hashlib
import json
import os
import struct
import sys
import time
import urllib.request


def sha256d(raw):
    return hashlib.sha256(hashlib.sha256(raw).digest()).digest()


def rpc_call(method, params):
    url = os.environ.get("BTC_RPC_URL", "")
    user = os.environ.get("BTC_RPC_USER", "")
    password = os.environ.get("BTC_RPC_PASSWORD", "")
    if not url:
        raise RuntimeError("BTC_RPC_URL manquant")
    payload = json.dumps({"jsonrpc": "1.0", "id": "lumvorax-gbt", "method": method, "params": params}).encode()
    headers = {"Content-Type": "application/json"}
    if user or password:
        headers["Authorization"] = "Basic " + base64.b64encode(f"{user}:{password}".encode()).decode()
    req = urllib.request.Request(url, data=payload, headers=headers, method="POST")
    with urllib.request.urlopen(req, timeout=30) as response:
        data = json.loads(response.read().decode())
    if data.get("error"):
        raise RuntimeError(str(data["error"]))
    return data.get("result")


def varint(n):
    if n < 0xfd:
        return bytes([n])
    if n <= 0xffff:
        return b"\xfd" + struct.pack("<H", n)
    if n <= 0xffffffff:
        return b"\xfe" + struct.pack("<I", n)
    return b"\xff" + struct.pack("<Q", n)


def push_data(raw):
    if len(raw) < 0x4c:
        return bytes([len(raw)]) + raw
    if len(raw) <= 0xff:
        return b"\x4c" + bytes([len(raw)]) + raw
    return b"\x4d" + struct.pack("<H", len(raw)) + raw


def encode_height(height):
    raw = int(height).to_bytes(8, "little").rstrip(b"\x00") or b"\x00"
    return push_data(raw)


def base58check_decode(value):
    alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"
    n = 0
    for ch in value:
        if ch not in alphabet:
            raise ValueError("adresse base58 invalide")
        n = n * 58 + alphabet.index(ch)
    raw = n.to_bytes((n.bit_length() + 7) // 8, "big")
    raw = b"\x00" * (len(value) - len(value.lstrip("1"))) + raw
    if len(raw) < 5:
        raise ValueError("adresse trop courte")
    payload, checksum = raw[:-4], raw[-4:]
    if sha256d(payload)[:4] != checksum:
        raise ValueError("checksum adresse invalide")
    return payload


def script_pubkey_from_env():
    script_hex = os.environ.get("BTC_COINBASE_SCRIPT_PUBKEY_HEX", "").strip()
    if script_hex:
        return bytes.fromhex(script_hex)
    address = os.environ.get("BTC_WALLET_ADDRESS", "").strip()
    if not address:
        raise RuntimeError("BTC_COINBASE_SCRIPT_PUBKEY_HEX ou BTC_WALLET_ADDRESS requis pour construire la coinbase")
    payload = base58check_decode(address)
    version, h160 = payload[0], payload[1:]
    if version == 0x00 and len(h160) == 20:
        return bytes.fromhex("76a914") + h160 + bytes.fromhex("88ac")
    if version == 0x05 and len(h160) == 20:
        return bytes.fromhex("a914") + h160 + bytes.fromhex("87")
    raise RuntimeError("adresse non prise en charge automatiquement; fournir BTC_COINBASE_SCRIPT_PUBKEY_HEX")


def build_coinbase(template, script_pubkey):
    height = int(template["height"])
    value = int(template["coinbasevalue"])
    tag = os.environ.get("BTC_COINBASE_TAG", "/LumVorax-C43/").encode()
    extranonce = bytes.fromhex(os.environ.get("BTC_COINBASE_EXTRANONCE_HEX", "00000000"))
    script_sig = encode_height(height) + push_data(tag + extranonce)
    outputs = [(value, script_pubkey)]
    witness_commitment = template.get("default_witness_commitment")
    use_witness = bool(witness_commitment)
    if witness_commitment:
        outputs.append((0, bytes.fromhex(witness_commitment)))
    version = struct.pack("<I", 2)
    tx_in = (
        b"\x01"
        + b"\x00" * 32
        + struct.pack("<I", 0xffffffff)
        + varint(len(script_sig))
        + script_sig
        + struct.pack("<I", 0xffffffff)
    )
    tx_out = varint(len(outputs))
    for amount, script in outputs:
        tx_out += struct.pack("<Q", amount) + varint(len(script)) + script
    locktime = struct.pack("<I", 0)
    stripped = version + tx_in + tx_out + locktime
    if not use_witness:
        return stripped.hex(), stripped.hex(), sha256d(stripped)[::-1].hex()
    witness = b"\x01\x20" + b"\x00" * 32
    full = version + b"\x00\x01" + tx_in + tx_out + witness + locktime
    return full.hex(), stripped.hex(), sha256d(stripped)[::-1].hex()


def merkle_root(txids):
    layer = [bytes.fromhex(txid)[::-1] for txid in txids]
    while len(layer) > 1:
        if len(layer) % 2:
            layer.append(layer[-1])
        layer = [sha256d(layer[i] + layer[i + 1]) for i in range(0, len(layer), 2)]
    return layer[0][::-1].hex()


def bits_to_target(bits_hex):
    bits = int(bits_hex, 16)
    exponent = bits >> 24
    mantissa = bits & 0x007fffff
    target = mantissa * (1 << (8 * (exponent - 3)))
    return target.to_bytes(32, "big").hex()


def build_job():
    template = rpc_call("getblocktemplate", [{"rules": ["segwit"]}])
    script_pubkey = script_pubkey_from_env()
    coinbase_full, coinbase_stripped, coinbase_txid = build_coinbase(template, script_pubkey)
    transactions = template.get("transactions", [])
    txids = [coinbase_txid] + [tx["txid"] for tx in transactions]
    tx_hex = [tx["data"] for tx in transactions]
    root = merkle_root(txids)
    version = int(template["version"])
    prevhash = bytes.fromhex(template["previousblockhash"])[::-1]
    curtime = int(template.get("curtime", time.time()))
    bits_hex = template["bits"]
    header = (
        struct.pack("<I", version)
        + prevhash
        + bytes.fromhex(root)[::-1]
        + struct.pack("<I", curtime)
        + struct.pack("<I", int(bits_hex, 16))
        + struct.pack("<I", 0)
    )
    full_tail = (varint(1 + len(tx_hex)).hex() + coinbase_full + "".join(tx_hex))
    target_hex = template.get("target") or bits_to_target(bits_hex)
    return {
        "schema": "lumvorax_btc_getblocktemplate_job_v1",
        "height": template["height"],
        "previousblockhash": template["previousblockhash"],
        "version": version,
        "bits": bits_hex,
        "curtime": curtime,
        "coinbase_tx_hex": coinbase_full,
        "coinbase_txid": coinbase_txid,
        "merkle_root": root,
        "transactions_hex": tx_hex,
        "transaction_count": 1 + len(tx_hex),
        "header_hex": header.hex(),
        "target_hex": target_hex,
        "full_block_tail_hex": full_tail,
        "full_block_hex_nonce0": header.hex() + full_tail,
        "created_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
    }


def main():
    parser = argparse.ArgumentParser(description="Construit un job minier Bitcoin depuis getblocktemplate")
    parser.add_argument("--output", default="config/btc_getblocktemplate_job.json")
    parser.add_argument("--print-header", action="store_true")
    args = parser.parse_args()
    job = build_job()
    os.makedirs(os.path.dirname(args.output) or ".", exist_ok=True)
    with open(args.output, "w", encoding="utf-8") as f:
        json.dump(job, f, indent=2, sort_keys=True)
        f.write("\n")
    print(f"GBT_JOB_OK height={job['height']} txs={job['transaction_count']} output={args.output}")
    if args.print_header:
        print(job["header_hex"])
    return 0


if __name__ == "__main__":
    sys.exit(main())