#!/usr/bin/env python3
import argparse
import base64
import hashlib
import json
import os
import sys
import urllib.request


def double_sha256(raw):
    return hashlib.sha256(hashlib.sha256(raw).digest()).digest()


def below_target(block_hash, target):
    return int.from_bytes(block_hash, "big") <= int.from_bytes(target, "big")


def varint(n):
    if n < 0xfd:
        return bytes([n]).hex()
    if n <= 0xffff:
        return "fd" + n.to_bytes(2, "little").hex()
    if n <= 0xffffffff:
        return "fe" + n.to_bytes(4, "little").hex()
    return "ff" + n.to_bytes(8, "little").hex()


def derive_full_block_hex(candidate, context):
    full_block_hex = candidate.get("full_block_hex", "")
    if full_block_hex:
        return full_block_hex, "candidate_full_block_hex"
    if not context:
        return "", "missing_context"
    header_hex = candidate.get("header_hex", "")
    tail_hex = context.get("full_block_tail_hex", "")
    if tail_hex:
        return header_hex + tail_hex, "context_full_block_tail_hex"
    coinbase = context.get("coinbase_tx_hex", "")
    txs = context.get("transactions_hex", [])
    if coinbase and isinstance(txs, list):
        return header_hex + varint(1 + len(txs)) + coinbase + "".join(txs), "context_coinbase_transactions"
    return "", "context_incomplete"


def rpc_call(method, params):
    url = os.environ.get("BTC_RPC_URL", "")
    user = os.environ.get("BTC_RPC_USER", "")
    password = os.environ.get("BTC_RPC_PASSWORD", "")
    if not url:
        return None, "BTC_RPC_URL manquant"
    payload = json.dumps({"jsonrpc": "1.0", "id": "lumvorax", "method": method, "params": params}).encode()
    headers = {"Content-Type": "application/json"}
    if user or password:
        token = base64.b64encode(f"{user}:{password}".encode()).decode()
        headers["Authorization"] = f"Basic {token}"
    req = urllib.request.Request(url, data=payload, headers=headers, method="POST")
    with urllib.request.urlopen(req, timeout=30) as response:
        data = json.loads(response.read().decode())
    if data.get("error"):
        return None, data["error"]
    return data.get("result"), None


def main():
    parser = argparse.ArgumentParser(description="Valide localement un candidat POW LumVorax et soumet un bloc complet si disponible")
    parser.add_argument("candidate_json")
    parser.add_argument("--context", help="Job JSON getblocktemplate contenant full_block_tail_hex ou coinbase+transactions")
    parser.add_argument("--write-enriched", action="store_true")
    parser.add_argument("--submit", action="store_true")
    args = parser.parse_args()

    with open(args.candidate_json, "r", encoding="utf-8") as f:
        candidate = json.load(f)

    header_hex = candidate.get("header_hex", "")
    target_hex = candidate.get("target_hex", "")
    context = None
    if args.context:
        with open(args.context, "r", encoding="utf-8") as f:
            context = json.load(f)
    full_block_hex, block_source = derive_full_block_hex(candidate, context)
    if len(header_hex) != 160:
        print("INVALID: header_hex doit contenir 160 hex chars")
        return 2
    if len(target_hex) != 64:
        print("INVALID: target_hex doit contenir 64 hex chars")
        return 2

    header = bytes.fromhex(header_hex)
    target = bytes.fromhex(target_hex)
    digest = double_sha256(header)
    digest_hex = digest.hex()
    ok = below_target(digest, target)
    print(f"local_pow_hash={digest_hex}")
    print(f"local_pow_below_target={ok}")
    print(f"candidate_reported_below_target={candidate.get('below_target')}")

    if not ok:
        print("CHAIN_SUBMIT_READY=false reason=pow_below_target_false")
        return 1

    if not full_block_hex:
        print(f"CHAIN_SUBMIT_READY=false reason=full_block_hex_absent source={block_source}")
        return 0

    print(f"CHAIN_SUBMIT_READY=true full_block_source={block_source} full_block_bytes={len(full_block_hex)//2}")
    if args.write_enriched:
        candidate["full_block_hex"] = full_block_hex
        candidate["network_submission_ready"] = True
        candidate["submission_reason"] = f"full_block_hex_derived_from_{block_source}"
        with open(args.candidate_json, "w", encoding="utf-8") as f:
            json.dump(candidate, f, indent=2, sort_keys=True)
            f.write("\n")

    if not args.submit:
        print("SUBMITBLOCK_SKIPPED=true reason=use --submit with BTC_RPC_URL to call submitblock")
        return 0

    result, error = rpc_call("submitblock", [full_block_hex])
    if error:
        print(f"SUBMITBLOCK_ERROR={error}")
        return 3
    print(f"SUBMITBLOCK_RESULT={result}")
    return 0


if __name__ == "__main__":
    sys.exit(main())