#!/usr/bin/env python3
"""
sync_standard_names.py — Centralisation du STANDARD_NAMES.md sur Supabase et Vercel
Conforme STANDARD_NAMES.md v4.3 — Cycle C43

Objectif :
  Rendre STANDARD_NAMES.md accessible à TOUS les agents qui touchent au code,
  via Supabase (table standard_names_registry) et Vercel Blob Storage.

Actions :
  1. Lire src/advanced_calculations/quantum_problem_hubbard_hts/STANDARD_NAMES.md
  2. Parser les sections et les entrées clés
  3. Uploader vers Supabase (table standard_names_registry)
  4. Uploader le fichier complet vers Vercel Blob Storage
  5. Afficher l'URL Vercel publique pour partage

Usage :
  python3 tools/sync_standard_names.py

Variables requises :
  SUPABASE_URL, SUPABASE_SERVICE_ROLE_KEY
  VERCEL_API_KEY
"""

import os
import sys
import re
import json
import time
import logging
import urllib.request
import urllib.error
from pathlib import Path

try:
    import requests
except ImportError:
    requests = None

logging.basicConfig(
    level=logging.INFO,
    format="[SYNC-STD-NAMES] %(asctime)s %(message)s",
    datefmt="%Y-%m-%dT%H:%M:%SZ",
)
log = logging.getLogger("sync_standard_names")

BASE_DIR = Path(__file__).resolve().parent.parent
STANDARD_NAMES_PATH = BASE_DIR / "STANDARD_NAMES.md"
SUPABASE_URL  = os.getenv("SUPABASE_URL", "").rstrip("/")
SUPABASE_URL2 = os.getenv("SUPABASE_URL2", "").rstrip("/")
SUPABASE_KEY  = os.getenv("SUPABASE_SERVICE_ROLE_KEY", os.getenv("SUPABASE_ANON_KEY", ""))
VERCEL_API_KEY = os.getenv("VERCEL_TOKEN", os.getenv("VERCEL_API_KEY", "")).strip()
VERCEL_URL_BASE = os.getenv("VERCEL_URL", "").strip()
TIMEOUT_S = 20


class _Response:
    def __init__(self, status_code, text):
        self.status_code = status_code
        self.text = text

    def json(self):
        return json.loads(self.text) if self.text else {}


def http_request(method: str, url: str, headers: dict, payload=None) -> _Response:
    if requests is not None:
        if method == "POST":
            return requests.post(url, headers=headers, json=payload, timeout=TIMEOUT_S)
        if method == "PUT":
            data = payload if isinstance(payload, (bytes, bytearray)) else str(payload).encode("utf-8")
            return requests.put(url, headers=headers, data=data, timeout=TIMEOUT_S)
    data = None
    if payload is not None:
        data = payload if isinstance(payload, (bytes, bytearray)) else json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(url, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req, timeout=TIMEOUT_S) as r:
            return _Response(r.status, r.read().decode())
    except urllib.error.HTTPError as e:
        return _Response(e.code, e.read().decode())


def _headers_supabase() -> dict:
    return {
        "apikey": SUPABASE_KEY,
        "Authorization": f"Bearer {SUPABASE_KEY}",
        "Content-Type": "application/json",
        "Prefer": "resolution=merge-duplicates",
    }


def _headers_vercel() -> dict:
    return {
        "Authorization": f"Bearer {VERCEL_API_KEY}",
        "Content-Type": "text/plain; charset=utf-8",
        "x-vercel-filename": "STANDARD_NAMES.md",
    }


def parse_standard_names(content: str) -> list[dict]:
    """Parse basique du STANDARD_NAMES.md pour en extraire les entrées clés."""
    entries = []
    current_section = "GENERAL"
    for line in content.splitlines():
        section_match = re.match(r"^## (SECTION .+|[A-Z ]+\b.*)", line)
        if section_match:
            current_section = section_match.group(1).strip()
            continue
        table_match = re.match(r"^\| `(.+?)` \| (.+?) \|", line)
        if table_match:
            key_name = table_match.group(1).strip()
            value_desc = table_match.group(2).strip()
            if key_name and key_name != "NOM D'ORIGINE (officiel)":
                entries.append({
                    "section": current_section[:80],
                    "key_name": key_name[:120],
                    "value_desc": value_desc[:500],
                    "file_origin": "STANDARD_NAMES.md",
                    "version": "4.3",
                    "cycle": "C43",
                    "is_canonical": True,
                })
    return entries


def upload_to_supabase(entries: list[dict]) -> bool:
    """Upload les entrées parsées vers la table standard_names_registry."""
    if not SUPABASE_URL or not SUPABASE_KEY:
        log.warning("Supabase non configuré — skip")
        return False
    if not entries:
        log.info("Aucune entrée à uploader vers Supabase")
        return True

    url = f"{SUPABASE_URL}/rest/v1/standard_names_registry"
    batch_size = 50
    ok = 0
    for i in range(0, len(entries), batch_size):
        batch = entries[i:i + batch_size]
        try:
            r = http_request("POST", url, _headers_supabase(), batch)
            if r.status_code in (200, 201, 204):
                ok += len(batch)
                log.info(f"Supabase batch {i//batch_size + 1}: {len(batch)} entrées ✅")
            else:
                log.warning(f"Supabase HTTP {r.status_code}: {r.text[:300]}")
        except Exception as e:
            log.warning(f"Supabase erreur batch {i//batch_size + 1}: {e}")
        time.sleep(0.5)

    log.info(f"Supabase total: {ok}/{len(entries)} entrées synchronisées")
    return ok > 0


def upload_to_vercel(content: str) -> str | None:
    """Upload le fichier complet STANDARD_NAMES.md vers Vercel Blob Storage."""
    if not VERCEL_API_KEY:
        log.warning("VERCEL_API_KEY non défini — skip Vercel")
        return None

    try:
        r = http_request(
            "PUT",
            "https://api.vercel.com/v2/blob/upload",
            _headers_vercel(),
            content.encode("utf-8"),
        )
        if r.status_code in (200, 201):
            result = r.json()
            url = result.get("url", "")
            log.info(f"Vercel Blob upload OK : {url}")
            return url
        else:
            log.warning(f"Vercel HTTP {r.status_code}: {r.text[:300]}")
    except Exception as e:
        log.warning(f"Erreur upload Vercel : {e}")

    return None


def save_vercel_url(url: str):
    """Sauvegarde l'URL Vercel dans un fichier de référence."""
    ref_file = BASE_DIR / "SUPABASE" / "vercel_standard_names_url.txt"
    with open(ref_file, "w") as f:
        f.write(f"STANDARD_NAMES.md Vercel URL:\n{url}\n")
        f.write(f"Version: 4.3\nCycle: C43\nDate: 2026-04-15\n")
    log.info(f"URL Vercel sauvegardée dans {ref_file}")


def main():
    if not STANDARD_NAMES_PATH.exists():
        log.error(f"STANDARD_NAMES.md introuvable à {STANDARD_NAMES_PATH}")
        sys.exit(1)

    content = STANDARD_NAMES_PATH.read_text(encoding="utf-8")
    log.info(f"STANDARD_NAMES.md lu : {len(content)} caractères, {content.count(chr(10))} lignes")

    entries = parse_standard_names(content)
    log.info(f"Entrées parsées : {len(entries)}")

    if not SUPABASE_URL and not VERCEL_API_KEY:
        log.error("Aucune destination configurée (SUPABASE_URL ou VERCEL_API_KEY requis)")
        sys.exit(1)

    supabase_ok = upload_to_supabase(entries)
    vercel_url  = upload_to_vercel(content)

    print("\n" + "="*60)
    print("SYNC STANDARD_NAMES.md — RÉSUMÉ")
    print("="*60)
    print(f"Entrées parsées     : {len(entries)}")
    print(f"Supabase            : {'✅ OK' if supabase_ok else '❌ ÉCHEC'}")
    print(f"Vercel URL          : {vercel_url or '❌ Non uploadé'}")
    print("="*60)

    if vercel_url:
        save_vercel_url(vercel_url)
        print(f"\n📋 URL publique STANDARD_NAMES.md :")
        print(f"   {vercel_url}")
        print("\nPartager cette URL avec tous les agents qui travaillent sur le code.")


if __name__ == "__main__":
    main()
