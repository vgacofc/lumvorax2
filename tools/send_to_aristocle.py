#!/usr/bin/env python3
"""
LumVorax — Module 17 — Aristotle IA Discovery Validator
tools/send_to_aristocle.py — Envoi des découvertes BTC à Aristotle (harmonic.fun)

API Aristotle (aristotlelib v1.0.1) :
  URL Base : https://aristotle.harmonic.fun/api/v1
  Auth     : en-tête X-API-Key
  Env var  : ARISTOTLE_API_KEY (mappé depuis ARISTOCLE_IA_API_KEY)
  Mode     : project_type=3 (INFORMAL) pour découvertes textuelles BTC

Système secrets : Doppler (priorité) → Replit (fallback)
STANDARD_NAMES.md v4.2 §M-BTC17-C41 — 2026-04-13
"""

import os, sys, json, time, tempfile, argparse
import httpx

ARISTOTLE_BASE = "https://aristotle.harmonic.fun/api/v2"
FALLBACK_FILE  = "logs/aristocle_pending_discoveries.jsonl"
PROJECT_TYPE_INFORMAL = 3


def get_key() -> str:
    """Récupère la clé — ARISTOCLE_IA_API_KEY (alias ARISTOTLE_API_KEY)."""
    key = (
        os.environ.get("ARISTOCLE_IA_API_KEY") or
        os.environ.get("ARISTOTLE_API_KEY") or
        ""
    )
    if not key:
        print("[ARISTOTLE] WARN: ARISTOCLE_IA_API_KEY absent des secrets")
    return key


def _headers(key: str) -> dict:
    return {"X-API-Key": key, "Content-Type": "application/json"}


def _create_project(key: str, content: str, meta: dict) -> dict | None:
    """Soumet une découverte en mode INFORMAL (project_type=3) — API v2."""
    try:
        import json as _json, io, tarfile

        prompt = (
            f"LumVorax BTC Record — {meta.get('cycle','?')} — "
            f"{meta.get('bits','?')} bits leading zeros — "
            f"run {meta.get('run_id','?')}\n\n{content}"
        )

        # Créer un tar.gz en mémoire contenant le fichier de découverte
        txt_name = f"lumvorax_{meta.get('run_id','btc')}.txt"
        txt_bytes = prompt.encode("utf-8")
        buf = io.BytesIO()
        with tarfile.open(fileobj=buf, mode="w:gz") as tar:
            info = tarfile.TarInfo(name=txt_name)
            info.size = len(txt_bytes)
            tar.addfile(info, io.BytesIO(txt_bytes))
        tar_bytes = buf.getvalue()

        body_json = _json.dumps({"prompt": prompt})

        r = httpx.post(
            f"{ARISTOTLE_BASE}/project",
            headers={"X-API-Key": key},
            data={"body": body_json},
            files={"file": (txt_name + ".tar.gz", tar_bytes, "application/gzip")},
            timeout=20,
        )
        if r.status_code in (200, 201, 202):
            return r.json()
        else:
            print(f"[ARISTOTLE] HTTP {r.status_code}: {r.text[:200]}")
            return None
    except Exception as e:
        print(f"[ARISTOTLE] Erreur création projet: {e}")
        return None


def _get_project_status(key: str, project_id: str) -> dict | None:
    """Récupère le statut d'un projet Aristotle."""
    try:
        r = httpx.get(
            f"{ARISTOTLE_BASE}/project/{project_id}",
            headers={"X-API-Key": key},
            timeout=10,
        )
        if r.status_code == 200:
            return r.json()
    except Exception as e:
        print(f"[ARISTOTLE] Statut projet FAIL: {e}")
    return None


def _save_fallback(payload: dict):
    """Sauvegarde locale si Aristotle inaccessible."""
    os.makedirs("logs", exist_ok=True)
    with open(FALLBACK_FILE, "a") as f:
        entry = {**payload, "pending": True, "saved_at": time.time()}
        f.write(json.dumps(entry) + "\n")
    print(f"[ARISTOTLE] Sauvegarde locale: {FALLBACK_FILE}")


def _save_supabase(payload: dict, project_resp: dict | None):
    """Enregistre dans btc_supermemory_entries via Supabase."""
    try:
        import psycopg2
        conn = psycopg2.connect(os.environ["DATABASE_URL"])
        cur = conn.cursor()
        cur.execute("""
            INSERT INTO btc_supermemory_entries
                (run_id, cycle, content, metadata, aristocle_validated, aristocle_response)
            VALUES (%s, %s, %s, %s, %s, %s)
        """, (
            payload.get("run_id"), payload.get("cycle"),
            payload.get("content"),
            json.dumps(payload.get("metadata", {})),
            project_resp is not None,
            json.dumps(project_resp) if project_resp else None
        ))
        conn.commit()
        conn.close()
        print("[ARISTOTLE] Supabase btc_supermemory_entries OK")
    except Exception as e:
        print(f"[ARISTOTLE] Supabase WARN: {e}")


def send_discovery(
    discovery_type: str,
    content: str,
    metadata: dict | None = None,
    cycle: str = "C41",
    run_id: str = "unknown",
) -> dict | None:
    """Envoie une découverte BTC à Aristotle en mode INFORMAL."""
    key = get_key()
    if not key:
        _save_fallback({
            "discovery_type": discovery_type, "content": content,
            "cycle": cycle, "run_id": run_id,
            "metadata": metadata or {},
            "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        })
        return None

    meta = {
        "discovery_type": discovery_type,
        "cycle": cycle,
        "run_id": run_id,
        "module": "LumVorax_BTC_Quantum_Mining",
        "standard": "STANDARD_NAMES_v4.2",
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        **(metadata or {}),
    }

    print(f"[ARISTOTLE] Soumission projet INFORMAL — type={discovery_type} cycle={cycle}")
    project_resp = _create_project(key, content, meta)

    if project_resp:
        project_id = project_resp.get("project_id", "?")
        status     = project_resp.get("status", "?")
        print(f"[ARISTOTLE] ✅ Projet créé: {project_id} — statut: {status}")
        print(f"[ARISTOTLE] Dashboard: https://aristotle.harmonic.fun/project/{project_id}")
        _save_supabase({"discovery_type": discovery_type, "content": content,
                        "cycle": cycle, "run_id": run_id, "metadata": meta},
                       project_resp)
        return project_resp
    else:
        print("[ARISTOTLE] ⚠️  Projet non créé — sauvegarde locale activée")
        _save_fallback({"discovery_type": discovery_type, "content": content,
                        "cycle": cycle, "run_id": run_id, "metadata": meta,
                        "timestamp": meta["timestamp"]})
        return None


def status_project(project_id: str):
    """Affiche le statut d'un projet Aristotle existant."""
    key = get_key()
    if not key:
        print("[ARISTOTLE] Clé absente — impossible de vérifier le statut")
        return
    data = _get_project_status(key, project_id)
    if data:
        print(f"[ARISTOTLE] Projet {project_id}:")
        print(f"  Statut    : {data.get('status')}")
        print(f"  Créé      : {data.get('created_at')}")
        print(f"  Mis à jour: {data.get('last_updated_at')}")
        pct = data.get("percent_complete")
        if pct is not None:
            print(f"  Avancement: {pct}%")
    else:
        print(f"[ARISTOTLE] Projet {project_id} introuvable ou erreur")


def retry_pending():
    """Retente l'envoi des découvertes locales en attente."""
    if not os.path.exists(FALLBACK_FILE):
        print("[ARISTOTLE] Aucune découverte en attente")
        return
    key = get_key()
    if not key:
        print("[ARISTOTLE] Clé absente — retry impossible")
        return

    remaining = []
    count_ok = 0
    with open(FALLBACK_FILE) as f:
        for line in f:
            try:
                entry = json.loads(line.strip())
            except Exception:
                continue
            if not entry.get("pending"):
                continue
            meta = entry.get("metadata", {})
            meta["cycle"]  = entry.get("cycle", "?")
            meta["run_id"] = entry.get("run_id", "?")
            resp = _create_project(key, entry.get("content", ""), meta)
            if resp:
                count_ok += 1
                print(f"[ARISTOTLE] ✅ Retry OK: {resp.get('project_id')}")
            else:
                entry["pending"] = True
                remaining.append(entry)

    with open(FALLBACK_FILE, "w") as f:
        for e in remaining:
            f.write(json.dumps(e) + "\n")
    print(f"[ARISTOTLE] Retry: {count_ok} envoyés, {len(remaining)} encore en attente")


def main():
    parser = argparse.ArgumentParser(description="LumVorax — Aristotle IA Discovery Sender")
    parser.add_argument("--type",       default="btc_record",    help="Type de découverte")
    parser.add_argument("--content",    default="",              help="Contenu texte de la découverte")
    parser.add_argument("--cycle",      default="C41",           help="Cycle LumVorax")
    parser.add_argument("--run-id",     default="unknown",       help="run_id BTC")
    parser.add_argument("--bits",       default="",              help="Bits leading zeros")
    parser.add_argument("--retry",      action="store_true",     help="Retente les envois en attente")
    parser.add_argument("--status",     default="",              help="project_id Aristotle à vérifier")
    args = parser.parse_args()

    if args.retry:
        retry_pending()
    elif args.status:
        status_project(args.status)
    else:
        if not args.content:
            print("Usage: --content 'ma découverte' [--type btc_record] [--cycle C41] [--run-id ...]")
            sys.exit(1)
        meta = {}
        if args.bits:
            meta["bits"] = args.bits
        send_discovery(args.type, args.content,
                       metadata=meta, cycle=args.cycle, run_id=args.run_id)


if __name__ == "__main__":
    main()
