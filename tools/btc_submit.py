#!/usr/bin/env python3
"""
btc_submit.py — Cycle C119 (squelette préparatoire P2)

Soumission d'un bloc Bitcoin candidat à un nœud bitcoind via JSON-RPC.

ETAT C119 : SQUELETTE PRÉPARATOIRE — NE SOUMET PAS RÉELLEMENT.
  - bitcoind Mainnet en cours d'IBD (Initial Block Download) bloc 0
  - testnet4 non encore configuré sur la machine Ubuntu
  - le wallet MAINNET 1YkQrHMbvBbYvCR1jcQAxjMj4bzibiK8C est déjà branché
  - aucun bloc valide n'a été trouvé (38 LZ atteints vs 78 LZ requis)

ROADMAP D'ACTIVATION (cycles C120+) :
  1. Configurer bitcoind testnet4 sur Ubuntu (~/.bitcoin/bitcoin.conf)
  2. Attendre fin d'IBD (peut prendre 2-6h selon bande passante)
  3. Tester soumission avec bloc trivialement faux pour valider le RPC
  4. Activer la branche réelle (variable env BTC_SUBMIT_LIVE=1 obligatoire)

ARCHITECTURE :
  - lit le hex 80 octets du header complet (after nonce found)
  - construit le bloc complet : header + varint(n_tx) + raw_tx_bytes
  - appelle submitblock via RPC
  - log la réponse complète dans logs/c119_submit_<timestamp>.json

Référence : RAPPORT_C117_ANALYSE §12 (P2.1) + STANDARD_NAMES.md M-BTC-SUBMIT-C119
"""
from __future__ import annotations
import argparse
import base64
import json
import os
import sys
import time
import urllib.request
import urllib.error

DEFAULT_RPC_URL_TESTNET = "http://127.0.0.1:48332"   # testnet4 standard
DEFAULT_RPC_URL_MAINNET = "http://127.0.0.1:8332"

WALLET_MAINNET_FIXED = "1YkQrHMbvBbYvCR1jcQAxjMj4bzibiK8C"


def rpc_call(url: str, method: str, params: list,
             user: str, password: str, timeout: float = 30.0) -> dict:
    """Appel JSON-RPC v1 (compatible bitcoind)."""
    payload = json.dumps({
        "jsonrpc": "1.0",
        "id": f"lumvorax_c119_{int(time.time())}",
        "method": method,
        "params": params,
    }).encode("utf-8")
    auth = base64.b64encode(f"{user}:{password}".encode()).decode()
    req = urllib.request.Request(url, data=payload, headers={
        "Content-Type": "application/json",
        "Authorization": f"Basic {auth}",
    })
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            return json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        body = e.read().decode("utf-8", errors="replace")
        return {"error": {"code": e.code, "message": str(e), "body": body}}
    except urllib.error.URLError as e:
        return {"error": {"code": -1, "message": f"Connection refused: {e}"}}


def submit_block(url: str, block_hex: str, user: str, password: str) -> dict:
    """Soumet un bloc complet (header+txs) via submitblock RPC."""
    return rpc_call(url, "submitblock", [block_hex], user, password)


def get_blockchain_info(url: str, user: str, password: str) -> dict:
    """Vérifier que bitcoind répond et n'est pas en IBD."""
    return rpc_call(url, "getblockchaininfo", [], user, password)


def main(argv: list) -> int:
    ap = argparse.ArgumentParser(prog="btc_submit",
                                 description="LumVorax C119 — squelette submitblock")
    ap.add_argument("--network", choices=["mainnet", "testnet4"],
                    default="testnet4",
                    help="Réseau cible (default: testnet4 — sécurité)")
    ap.add_argument("--rpc-url", help="URL RPC personnalisée (sinon défaut réseau)")
    ap.add_argument("--rpc-user", default=os.environ.get("BTC_RPC_USER", ""))
    ap.add_argument("--rpc-password", default=os.environ.get("BTC_RPC_PASSWORD", ""))
    ap.add_argument("--block-hex", help="Hex du bloc complet à soumettre")
    ap.add_argument("--block-file", help="Fichier contenant le hex du bloc")
    ap.add_argument("--check-only", action="store_true",
                    help="Vérifie uniquement la connexion bitcoind (getblockchaininfo)")
    ap.add_argument("--log-dir", default="logs/c119_submit/",
                    help="Répertoire de log JSON pour la réponse RPC")
    args = ap.parse_args(argv)

    # Sécurité C119 : refuser la soumission si BTC_SUBMIT_LIVE != 1
    live = os.environ.get("BTC_SUBMIT_LIVE", "0") == "1"
    if not args.check_only and not live:
        print("[C119-SAFETY] BTC_SUBMIT_LIVE != 1 — soumission BLOQUÉE.", file=sys.stderr)
        print("              Pour activer : export BTC_SUBMIT_LIVE=1", file=sys.stderr)
        print("              Wallet MAINNET attendu : "
              f"{WALLET_MAINNET_FIXED}", file=sys.stderr)
        return 3

    if not args.rpc_url:
        args.rpc_url = (DEFAULT_RPC_URL_MAINNET if args.network == "mainnet"
                        else DEFAULT_RPC_URL_TESTNET)

    if not args.rpc_user or not args.rpc_password:
        print("[C119-ERROR] BTC_RPC_USER et BTC_RPC_PASSWORD requis "
              "(ou --rpc-user/--rpc-password)", file=sys.stderr)
        return 4

    # Vérification de l'état bitcoind
    info = get_blockchain_info(args.rpc_url, args.rpc_user, args.rpc_password)
    if "error" in info and info["error"]:
        print(f"[C119-RPC-ERROR] getblockchaininfo : {info['error']}", file=sys.stderr)
        return 5
    result = info.get("result", {})
    print(f"[C119] bitcoind chain={result.get('chain')} "
          f"blocks={result.get('blocks')} headers={result.get('headers')} "
          f"ibd={result.get('initialblockdownload')}")

    if result.get("initialblockdownload"):
        print("[C119-IBD] bitcoind est en Initial Block Download — "
              "soumission impossible avant fin d'IBD.", file=sys.stderr)
        return 6

    if args.check_only:
        return 0

    # Charger le hex du bloc
    block_hex = args.block_hex
    if args.block_file:
        with open(args.block_file, "r") as f:
            block_hex = f.read().strip()
    if not block_hex:
        print("[C119-ERROR] --block-hex ou --block-file requis", file=sys.stderr)
        return 7
    if len(block_hex) % 2 != 0:
        print("[C119-ERROR] block-hex doit être de longueur paire", file=sys.stderr)
        return 8

    # Soumission réelle
    os.makedirs(args.log_dir, exist_ok=True)
    ts = int(time.time())
    log_path = os.path.join(args.log_dir, f"submit_{args.network}_{ts}.json")

    print(f"[C119-LIVE] Soumission bloc {len(block_hex)//2} octets sur "
          f"{args.network} via {args.rpc_url}...")
    response = submit_block(args.rpc_url, block_hex, args.rpc_user, args.rpc_password)
    with open(log_path, "w") as f:
        json.dump({
            "ts": ts,
            "network": args.network,
            "rpc_url": args.rpc_url,
            "block_size_bytes": len(block_hex) // 2,
            "wallet_mainnet_expected": WALLET_MAINNET_FIXED,
            "response": response,
        }, f, indent=2)

    if response.get("error"):
        print(f"[C119-REJECT] Bloc rejeté : {response['error']}", file=sys.stderr)
        print(f"[C119-LOG] Détails : {log_path}")
        return 9

    rpc_result = response.get("result")
    if rpc_result is None:
        print(f"[C119-ACCEPT] Bloc ACCEPTÉ par {args.network} !")
    else:
        print(f"[C119-INVALID] Bloc invalide : {rpc_result}")
    print(f"[C119-LOG] Réponse complète : {log_path}")
    return 0 if rpc_result is None else 10


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
