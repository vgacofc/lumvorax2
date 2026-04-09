#!/usr/bin/env python3
"""
vercel_log_streamer.py — Streaming logs LumVorax vers Vercel en temps réel
Conforme STANDARD_NAMES.md v3.4 — Cycle C54+ (label dynamique via LUMVORAX_CYCLE_ID)

Architecture :
  - Surveille les nouveaux fichiers CSV Lumvorax (rotation toutes les 20 MB)
  - Parse les lignes METRIC et les envoie vers Vercel Blob Storage
  - Synchronise également vers Supabase (double base de données)
  - Respecte le rate limit Replit et Vercel

Variables requises :
  VERCEL_API_KEY         ← clé API Vercel (intégrée par l'utilisateur)
  SUPABASE_URL           ← URL Supabase REST
  SUPABASE_SERVICE_ROLE_KEY ← clé service Supabase

Usage :
  python3 vercel_log_streamer.py [--run-dir PATH] [--batch 100]
  
  Ou lancement automatique dans run_research_cycle.sh :
  python3 tools/vercel_log_streamer.py &
"""

import os
import sys
import time
import json
import csv
import glob
import logging
import argparse
import traceback
from pathlib import Path
from datetime import datetime, timezone

try:
    import requests
except ImportError:
    print("requests non installé. pip install requests")
    sys.exit(1)

logging.basicConfig(
    level=logging.INFO,
    format="[VERCEL-STREAMER] %(asctime)s %(levelname)s %(message)s",
    datefmt="%Y-%m-%dT%H:%M:%SZ",
)
log = logging.getLogger("vercel_streamer")

# ── Configuration ─────────────────────────────────────────────────────────────

VERCEL_API_KEY    = os.getenv("VERCEL_TOKEN", os.getenv("VERCEL_API_KEY", "")).strip()
# C53-FIX-VERCEL-URL : URL de production déployée (Node.js 20.x, @vercel/node, ID dpl_AJ2LEiNdtsgoc2huS9ogsXTN3FBA)
# Endpoint de production LumVorax — seule URL valide pour l'API /api/lumvorax-logs
_VERCEL_URL_DEFAULT = "https://lumvorax-hts-ks02ngkt3-vgac4237-8522s-projects.vercel.app"
# C55-FIX-VERCEL-URL : ignorer VERCEL_URL si ce n'est pas l'hostname de l'API LumVorax.
# Bug C54 : VERCEL_URL="vercel.com/vgac4237-8522s-projects" overridait le défaut correct
#   → après ajout https:// : "https://vercel.com/vgac4237-8522s-projects/api/lumvorax-logs"
#   → c'est une page projet Vercel, PAS l'endpoint API → 100% échecs
# Solution C55 : utiliser VERCEL_URL seulement si il contient "lumvorax-hts" (l'hostname prod)
_env_url = os.getenv("VERCEL_URL", "").strip()
if _env_url:
    if not _env_url.startswith("http"):
        _env_url = "https://" + _env_url
    if "lumvorax-hts" in _env_url:
        VERCEL_URL_BASE = _env_url      # URL env valide → on l'utilise
    else:
        VERCEL_URL_BASE = _VERCEL_URL_DEFAULT   # URL env invalide → on ignore, fallback prod
else:
    VERCEL_URL_BASE = _VERCEL_URL_DEFAULT       # VERCEL_URL absent → fallback prod
SUPABASE_URL      = os.getenv("SUPABASE_URL", "").rstrip("/")
SUPABASE_URL2     = os.getenv("SUPABASE_URL2", "").rstrip("/")
SUPABASE_KEY      = os.getenv("SUPABASE_SERVICE_ROLE_KEY", os.getenv("SUPABASE_ANON_KEY", ""))

# C54-FIX-CYCLE-LABEL : Label cycle dynamique — lit LUMVORAX_CYCLE_ID depuis l'env.
# Si absent, dérive depuis le répertoire de run courant ou fallback "C54".
# NE PLUS HARDCODER "C50", "C52" etc. dans les payloads.
def _resolve_cycle_label() -> str:
    env_label = os.getenv("LUMVORAX_CYCLE_ID", "").strip()
    if env_label:
        return env_label
    runs = sorted(Path(__file__).resolve().parent.parent.glob("results/research_*"), reverse=True)
    if runs:
        import re
        m = re.search(r'research_(\d{8}T\d{6}Z)_(\d+)', runs[0].name)
        if m:
            return f"C54-{m.group(1)}"
    return "C54"

CYCLE_LABEL = _resolve_cycle_label()

BASE_DIR = Path(__file__).resolve().parent.parent
RESULTS_DIR = BASE_DIR / "results"

BATCH_SIZE     = 100        # Lignes par batch envoyé
POLL_INTERVAL  = 5          # Secondes entre chaque scan
MAX_RETRIES    = 3          # Tentatives en cas d'erreur réseau
TIMEOUT_S      = 15         # Timeout requête HTTP


def _latest_run_dir() -> Path | None:
    runs = sorted(RESULTS_DIR.glob("research_*"), reverse=True)
    return runs[0] if runs else None


def _get_headers_supabase() -> dict:
    return {
        "apikey": SUPABASE_KEY,
        "Authorization": f"Bearer {SUPABASE_KEY}",
        "Content-Type": "application/json",
        "Prefer": "resolution=merge-duplicates",
    }


def _get_headers_vercel() -> dict:
    return {
        "Authorization": f"Bearer {VERCEL_API_KEY}",
        "Content-Type": "application/json",
    }


def parse_csv_lines(csv_path: Path, start_line: int = 0) -> list[dict]:
    """Parse les lignes CSV Lumvorax depuis start_line."""
    events = []
    try:
        with open(csv_path, newline="", encoding="utf-8", errors="replace") as f:
            reader = csv.reader(f)
            for i, row in enumerate(reader):
                if i < start_line:
                    continue
                if len(row) < 6:
                    continue
                event_type = row[0].strip() if row[0] else ""
                if event_type not in ("METRIC", "ANOMALY", "START", "END", "OP"):
                    continue
                try:
                    ts_utc = row[1].strip() if len(row) > 1 else ""
                    ts_ns  = int(row[2].strip()) if len(row) > 2 else 0
                    pid    = row[3].strip() if len(row) > 3 else ""
                    metric = row[4].strip() if len(row) > 4 else ""
                    value_raw = row[5].strip() if len(row) > 5 else ""
                    module = metric.split(":")[0] if ":" in metric else pid
                    try:
                        value = float(value_raw)
                        value_text = None
                    except ValueError:
                        value = None
                        value_text = value_raw
                    events.append({
                        "event_type": event_type,
                        "ts_utc": ts_utc,
                        "ts_ns": ts_ns,
                        "module": module,
                        "metric": metric,
                        "value": value,
                        "value_text": value_text,
                        "line_index": i,
                    })
                except Exception:
                    continue
    except Exception as e:
        log.warning(f"Erreur lecture {csv_path}: {e}")
    return events


def send_to_vercel(run_id: str, events: list[dict]) -> bool:
    """Envoie un batch d'événements vers Vercel via l'API."""
    if not VERCEL_API_KEY:
        log.warning("VERCEL_API_KEY non défini — skip Vercel")
        return False
    if not events:
        return True

    payload = {
        "run_id": run_id,
        "cycle": CYCLE_LABEL,
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "n_events": len(events),
        "events": events,
    }

    # C52-FIX-VERCEL-ENDPOINT : utilise VERCEL_URL_BASE/api/logs si disponible,
    # sinon l'API Vercel Blob (https://blob.vercel-storage.com) comme fallback.
    # L'ancien endpoint (https://api.vercel.com/v1/data-cache/config) était incorrect —
    # c'est une API de configuration du cache, pas d'ingestion de logs.
    if VERCEL_URL_BASE:
        vercel_endpoint = f"{VERCEL_URL_BASE.rstrip('/')}/api/lumvorax-logs"
    else:
        log.warning("VERCEL_URL non défini — skip Vercel")
        return False

    for attempt in range(MAX_RETRIES):
        try:
            r = requests.post(
                vercel_endpoint,
                headers=_get_headers_vercel(),
                json=payload,
                timeout=TIMEOUT_S,
            )
            if r.status_code in (200, 201, 204):
                log.info(f"Vercel OK : {len(events)} events envoyés (run={run_id})")
                return True
            else:
                log.warning(f"Vercel HTTP {r.status_code} [{vercel_endpoint}]: {r.text[:200]}")
        except Exception as e:
            log.warning(f"Vercel tentative {attempt+1}/{MAX_RETRIES}: {e}")
            time.sleep(2 ** attempt)
    return False


def send_to_supabase(run_id: str, events: list[dict]) -> bool:
    """Envoie un batch d'événements vers Supabase (table vercel_log_events)."""
    if not SUPABASE_URL or not SUPABASE_KEY:
        log.warning("SUPABASE_URL ou SUPABASE_SERVICE_ROLE_KEY non défini — skip Supabase")
        return False
    if not events:
        return True

    rows = []
    for ev in events:
        rows.append({
            "run_id": run_id,
            "cycle": CYCLE_LABEL,
            "timestamp_ns": ev["ts_ns"],
            "module": ev["module"],
            "metric": ev["metric"],
            "value": ev["value"],
            "value_text": ev["value_text"],
            "event_type": ev["event_type"],
        })

    url = f"{SUPABASE_URL}/rest/v1/vercel_log_events"
    for attempt in range(MAX_RETRIES):
        try:
            r = requests.post(
                url,
                headers=_get_headers_supabase(),
                json=rows,
                timeout=TIMEOUT_S,
            )
            if r.status_code in (200, 201, 204):
                log.info(f"Supabase OK : {len(rows)} événements insérés")
                return True
            elif r.status_code == 409:
                log.warning(f"Supabase conflit (doublons) : ignoré")
                return True
            else:
                log.warning(f"Supabase HTTP {r.status_code}: {r.text[:200]}")
        except Exception as e:
            log.warning(f"Supabase tentative {attempt+1}/{MAX_RETRIES}: {e}")
            time.sleep(2 ** attempt)
    return False


def stream_run(run_dir: Path):
    """Surveille un répertoire de run et streame les nouveaux CSV."""
    logs_dir = run_dir / "logs"
    run_id = run_dir.name
    log.info(f"Streaming run : {run_id}")

    file_cursors: dict[str, int] = {}
    processed_files: set[str] = set()

    while True:
        csv_files = sorted(logs_dir.glob("lumvorax_*.csv"))
        for csv_path in csv_files:
            fname = csv_path.name
            if fname not in file_cursors:
                file_cursors[fname] = 0
                log.info(f"Nouveau fichier détecté : {fname}")

            start_line = file_cursors[fname]
            events = parse_csv_lines(csv_path, start_line)
            if not events:
                continue

            new_cursor = start_line + len(events)
            file_cursors[fname] = new_cursor

            batch = []
            for ev in events:
                batch.append(ev)
                if len(batch) >= BATCH_SIZE:
                    send_to_vercel(run_id, batch)
                    send_to_supabase(run_id, batch)
                    batch = []
            if batch:
                send_to_vercel(run_id, batch)
                send_to_supabase(run_id, batch)

        exec_log = run_dir / "logs" / "research_execution.log"
        if exec_log.exists():
            with open(exec_log, encoding="utf-8") as f:
                content = f.read()
            if "END" in content or "DONE" in content or "FINISH" in content:
                log.info(f"Run {run_id} terminé — fin du streaming")
                break

        time.sleep(POLL_INTERVAL)


def main():
    global BATCH_SIZE  # C52-FIX-SYNTAX : déclaration avant tout usage (SyntaxError corrigé)
    parser = argparse.ArgumentParser(description="Vercel Log Streamer — LumVorax C52")
    parser.add_argument("--run-dir", type=str, default=None, help="Répertoire du run (défaut: dernier run)")
    parser.add_argument("--batch", type=int, default=BATCH_SIZE, help="Taille batch")
    args = parser.parse_args()

    BATCH_SIZE = args.batch

    if not VERCEL_API_KEY and not SUPABASE_URL:
        log.error("Ni VERCEL_API_KEY ni SUPABASE_URL définis — aucun streaming possible.")
        log.error("Définir au moins une variable dans les secrets Replit.")
        sys.exit(1)

    if args.run_dir:
        run_dir = Path(args.run_dir)
    else:
        run_dir = _latest_run_dir()

    if run_dir is None or not run_dir.exists():
        log.error("Aucun répertoire de run trouvé. Lancer d'abord run_research_cycle.sh.")
        sys.exit(1)

    log.info(f"Démarrage streamer — run_dir={run_dir}")
    log.info(f"Vercel API  : {'✅' if VERCEL_API_KEY else '❌ NON DÉFINI (VERCEL_TOKEN)'}")
    log.info(f"Supabase DB1: {'✅' if SUPABASE_URL else '❌ NON DÉFINI (SUPABASE_URL)'}")
    log.info(f"Supabase DB2: {'✅' if SUPABASE_URL2 else '❌ NON DÉFINI (SUPABASE_URL2)'}")

    try:
        stream_run(run_dir)
    except KeyboardInterrupt:
        log.info("Arrêt du streamer (Ctrl+C)")
    except Exception as e:
        log.error(f"Erreur fatale : {e}")
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
