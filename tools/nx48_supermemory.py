#!/usr/bin/env python3
"""
LumVorax — Module 17 — Supermemory Integration
tools/nx48_supermemory.py — Mémoire persistante NX48 inter-sessions

URL correcte (validée C41): https://api.supermemory.ai/v3/documents
                             (redirect 308 depuis /v3/memories)

Usage:
  python3 tools/nx48_supermemory.py --init <stamp>
  python3 tools/nx48_supermemory.py --store "découverte" --cycle C41 --run-id <run_id>
  python3 tools/nx48_supermemory.py --recall --query "btc record"

STANDARD_NAMES.md v4.2 — Cycle C41 — 2026-04-13
"""

import os, sys, json, argparse, urllib.request, urllib.error, time

SUPERMEMORY_URL = "https://api.supermemory.ai/v3/documents"
SUPERMEMORY_SEARCH_URL = "https://api.supermemory.ai/v3/search"
FALLBACK_CACHE = "/tmp/lumvorax_supermemory_cache.jsonl"
# Container tag validé depuis console.supermemory.ai — 662 docs / 237 mémoires
SUPERMEMORY_CONTAINER = "lumvorax_nx48"

def get_key():
    # Priorité 1: Doppler via env DOPPLER_TOKEN + projet lumvorax
    key = os.environ.get("SUPERMEMORY_API_KEY", "")
    if not key:
        print("[NX48-MEM] WARN: SUPERMEMORY_API_KEY absent")
    return key

def _post_document(key, content, metadata=None):
    """POST un document vers Supermemory /v3/documents avec container lumvorax_nx48"""
    payload = {
        "content": content,
        "metadata": metadata or {},
        "containerTags": [SUPERMEMORY_CONTAINER]
    }
    data = json.dumps(payload).encode()
    req = urllib.request.Request(
        SUPERMEMORY_URL,
        data=data,
        headers={
            "Authorization": f"Bearer {key}",
            "Content-Type": "application/json",
        },
        method="POST"
    )
    try:
        with urllib.request.urlopen(req, timeout=10) as r:
            resp = json.loads(r.read())
            return True, resp
    except urllib.error.HTTPError as e:
        body = e.read().decode()[:300]
        return False, {"error": e.code, "body": body}
    except Exception as e:
        return False, {"error": str(e)}

def _search(key, query):
    """GET recherche dans Supermemory"""
    url = f"{SUPERMEMORY_SEARCH_URL}?q={urllib.parse.quote(query)}&limit=5"
    req = urllib.request.Request(
        url,
        headers={"Authorization": f"Bearer {key}", "Content-Type": "application/json"}
    )
    try:
        with urllib.request.urlopen(req, timeout=10) as r:
            return True, json.loads(r.read())
    except Exception as e:
        return False, {"error": str(e)}

def _fallback_save(content, metadata):
    """Sauvegarde locale si Supermemory injoignable"""
    entry = {"timestamp": time.time(), "content": content, "metadata": metadata}
    with open(FALLBACK_CACHE, "a") as f:
        f.write(json.dumps(entry) + "\n")
    print(f"[NX48-MEM] Fallback cache local: {FALLBACK_CACHE}")

def store_discovery(cycle, run_id, content, extra_meta=None):
    """Envoie une découverte vers Supermemory + fallback local"""
    key = get_key()
    metadata = {
        "source": "LumVorax",
        "module": "btc_quantum_mining",
        "cycle": cycle,
        "run_id": run_id,
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "standard": "STANDARD_NAMES_v4.2",
        **(extra_meta or {})
    }
    print(f"[NX48-MEM] Envoi Supermemory: cycle={cycle} run={run_id}")
    ok, resp = _post_document(key, content, metadata)
    if ok:
        print(f"[NX48-MEM] ✅ Supermemory OK: {json.dumps(resp)[:200]}")
        return resp
    else:
        print(f"[NX48-MEM] ⚠️ Supermemory FAIL: {resp}")
        _fallback_save(content, metadata)
        return None

def init_session(stamp):
    """Initialise la session: charge mémoires précédentes + envoie état init"""
    key = get_key()
    print(f"[NX48-MEM] Init session stamp={stamp}")
    # Rappel des mémoires BTC précédentes
    ok, data = _search(key, "LumVorax BTC record leading zeros")
    if ok:
        print(f"[NX48-MEM] Mémoires récupérées: {json.dumps(data)[:300]}")
    else:
        print(f"[NX48-MEM] Recherche FAIL: {data}")

    # Envoie l'état init de la session
    content = f"LumVorax Session Init {stamp} - Module BTC Quantum Mining - Cycle C41"
    store_discovery("C41", f"init_{stamp}", content, {"event": "session_start"})

def main():
    import urllib.parse
    parser = argparse.ArgumentParser()
    parser.add_argument("--init", metavar="STAMP")
    parser.add_argument("--store", metavar="CONTENT")
    parser.add_argument("--cycle", default="C41")
    parser.add_argument("--run-id", default="unknown")
    parser.add_argument("--recall", action="store_true")
    parser.add_argument("--query", default="LumVorax BTC")
    args = parser.parse_args()

    if args.init:
        init_session(args.init)
    elif args.store:
        store_discovery(args.cycle, args.run_id, args.store)
    elif args.recall:
        key = get_key()
        ok, data = _search(key, args.query)
        print("RECALL:", json.dumps(data, indent=2)[:1000])

if __name__ == "__main__":
    main()
