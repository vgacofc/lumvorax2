#!/usr/bin/env python3
"""
supabase_realtime_streamer.py — Streaming temps réel des logs vers Supabase
STANDARD_NAMES.md Section H + Section J (table: quantum_realtime_logs)

Usage (background daemon) :
    python3 supabase_realtime_streamer.py --watch <log_file_or_dir> --run-id <run_id> &

Fonctionnement :
  1. Surveille un fichier de log (ou tous les CSV/log d'un répertoire) en continu
  2. Envoie chaque nouvelle ligne vers Supabase (table quantum_realtime_logs) en temps réel
  3. Rotation à 20 MB : ferme le fichier courant, crée une nouvelle partie, supprime l'ancien
  4. Suppression locale après upload réussi (--delete-after)
  5. Batch de 50 lignes max pour éviter les timeouts
  6. Identifie automatiquement : module, metric, value depuis le format CSV Lumvorax

Colonnes Supabase (STANDARD_NAMES.md Section J — table quantum_realtime_logs) :
  run_id, ts_utc, ts_ns, pid, event_type, module, metric, value, detail
"""

import os
import sys
import re
import time
import json
import signal
import argparse
import threading
from datetime import datetime, timezone
from pathlib import Path

# ── Auto-install ────────────────────────────────────────────────────────────
def _ensure(pkg, iname=None):
    try:
        __import__(iname or pkg)
    except ImportError:
        import subprocess
        print(f"[AUTO-INSTALL] {pkg}...", flush=True)
        subprocess.run([sys.executable,"-m","pip","install",pkg,"-q"], check=False)

_ensure("requests")
import requests

# ── Config Supabase ─────────────────────────────────────────────────────────
def _derive_url():
    u = (os.environ.get("SUPABASE8_API_URL") or "").strip().rstrip("/")
    if u.startswith("https://") and "supabase.co" in u:
        return u
    db = os.environ.get("SUPABASE_DB_HOST","").strip()
    if db.startswith("db.") and ".supabase.co" in db:
        pid = db[3:].replace(".supabase.co","")
        return f"https://{pid}.supabase.co"
    return ""

SUPABASE_URL = _derive_url()
SERVICE_KEY  = os.environ.get("SUPABASE_SERVICE_ROLE_KEY","").strip()

MAX_FILE_BYTES = 20 * 1024 * 1024   # 20 MB — STANDARD_NAMES.md C53-ROT
BATCH_SIZE     = 50
POLL_INTERVAL  = 0.5                 # secondes entre polls
TABLE_LOGS     = "quantum_realtime_logs"
TABLE_ROWS     = "quantum_csv_rows"

_stop_event = threading.Event()

def _hdrs():
    return {
        "apikey":        SERVICE_KEY,
        "Authorization": f"Bearer {SERVICE_KEY}",
        "Content-Type":  "application/json",
        "Prefer":        "return=minimal",
    }

def _rest(table):
    return f"{SUPABASE_URL}/rest/v1/{table}"

def _is_supabase_ok():
    return bool(SUPABASE_URL) and bool(SERVICE_KEY)

# ── Parseur de ligne CSV Lumvorax ──────────────────────────────────────────
# Format : event,timestamp_utc,timestamp_ns,pid,detail,value
# Ex : METRIC,2026-03-28T20:00:00Z,1711656000123456789,12345,simulate_fs:energy_eV,1.992
RE_LUMVORAX = re.compile(
    r'^(?P<event>\w+),(?P<ts_utc>[^,]+),(?P<ts_ns>\d+),(?P<pid>\d+),'
    r'(?P<detail>[^,]+),(?P<value>[^\n\r]+)$'
)
# Format research_execution.log : 000001 | BASE_RESULT problem=... energy=... ...
RE_EXEC_LOG = re.compile(r'^\d+\s*\|\s*(?P<event>\w+)\s+(?P<rest>.+)$')

def parse_lumvorax_line(line: str, run_id: str) -> dict | None:
    """Parse une ligne Lumvorax CSV → dict pour Supabase quantum_realtime_logs."""
    line = line.strip()
    if not line or line.startswith("event,"):  # en-tête
        return None
    m = RE_LUMVORAX.match(line)
    if m:
        detail = m.group("detail").strip()
        # detail = "simulate_fs:energy_eV" → module="simulate_fs", metric="energy_eV"
        module, metric = (detail.split(":", 1) if ":" in detail else ("unknown", detail))
        try:
            val = float(m.group("value"))
        except ValueError:
            val = None
        return {
            "run_id":     run_id,
            "ts_utc":     m.group("ts_utc"),
            "ts_ns":      int(m.group("ts_ns")),
            "pid":        int(m.group("pid")),
            "event_type": m.group("event"),
            "module":     module,
            "metric":     metric,
            "value":      val,
            "detail":     detail,
        }
    # Format execution log
    m2 = RE_EXEC_LOG.match(line)
    if m2:
        now = datetime.now(timezone.utc).isoformat()
        return {
            "run_id":     run_id,
            "ts_utc":     now,
            "ts_ns":      time.time_ns(),
            "pid":        os.getpid(),
            "event_type": m2.group("event"),
            "module":     "research_execution",
            "metric":     m2.group("event").lower(),
            "value":      None,
            "detail":     m2.group("rest")[:500],
        }
    return None

# ── Upload batch vers Supabase ─────────────────────────────────────────────
def upload_batch(rows: list[dict], table=TABLE_LOGS) -> bool:
    if not rows or not _is_supabase_ok():
        return False
    try:
        r = requests.post(
            _rest(table), headers=_hdrs(), json=rows, timeout=20
        )
        return r.status_code in (200, 201, 204)
    except Exception as e:
        print(f"[STREAMER][ERR] upload_batch: {e}", flush=True)
        return False

# ── Raw CSV rows (quantum_csv_rows) ────────────────────────────────────────
def upload_csv_rows(run_id: str, file_name: str, lines: list[str]) -> bool:
    rows = []
    for i, line in enumerate(lines):
        rows.append({
            "run_id":     run_id,
            "file_name":  file_name,
            "row_number": i + 1,
            "data":       line.rstrip()[:4000],  # limite TEXT Supabase
        })
    if not rows:
        return True
    ok = True
    for i in range(0, len(rows), BATCH_SIZE):
        ok &= upload_batch(rows[i:i+BATCH_SIZE], TABLE_ROWS)
    return ok

# ── Surveillance d'un fichier ──────────────────────────────────────────────
class FileWatcher:
    """Surveille un fichier en continu, envoie les nouvelles lignes vers Supabase.
    Gère la rotation à 20 MB et la suppression locale après upload.
    """
    def __init__(self, path: Path, run_id: str, delete_after: bool = False):
        self.path         = path
        self.run_id       = run_id
        self.delete_after = delete_after
        self._pos         = 0
        self._buffer      = []
        self._upload_ok   = 0
        self._upload_fail = 0

    def _flush(self):
        if not self._buffer:
            return
        batch = [r for r in self._buffer if r is not None]
        if batch:
            if upload_batch(batch):
                self._upload_ok += len(batch)
                # Aussi uploader en raw (quantum_csv_rows)
                upload_csv_rows(
                    self.run_id,
                    self.path.name,
                    [b.get("detail","") for b in batch]
                )
            else:
                self._upload_fail += len(batch)
                # On conserve localement — pas de suppression si échec
                return
        self._buffer.clear()
        # Suppression locale si demandée et upload réussi
        if self.delete_after and self._upload_fail == 0:
            self._maybe_delete_local()

    def _maybe_delete_local(self):
        """Supprime ou vide le fichier local après upload complet réussi."""
        try:
            size = self.path.stat().st_size
            if size < 1024:  # < 1 KB = presque vide
                return
            # Vider le fichier (truncate) plutôt que supprimer
            # pour préserver le handle si le runner écrit encore dedans
            with open(self.path, "w") as f:
                f.truncate(0)
            print(f"[STREAMER] {self.path.name} vidé localement après upload "
                  f"({size//1024} KB libérés)", flush=True)
            self._pos = 0
        except Exception as e:
            print(f"[STREAMER][WARN] truncate {self.path.name}: {e}", flush=True)

    def poll(self):
        """Lit les nouvelles lignes depuis _pos, parse et bufferise."""
        if not self.path.exists():
            return
        try:
            size = self.path.stat().st_size
        except OSError:
            return

        # Rotation détectée (fichier plus petit que notre position)
        if size < self._pos:
            self._flush()
            self._pos = 0

        if size <= self._pos:
            return

        try:
            with open(self.path, "r", encoding="utf-8", errors="replace") as f:
                f.seek(self._pos)
                new_data = f.read(size - self._pos)
                self._pos = f.tell()
        except Exception as e:
            print(f"[STREAMER][ERR] lecture {self.path.name}: {e}", flush=True)
            return

        for line in new_data.splitlines():
            parsed = parse_lumvorax_line(line, self.run_id)
            if parsed:
                self._buffer.append(parsed)
                if len(self._buffer) >= BATCH_SIZE:
                    self._flush()

    def finalize(self):
        """Flush final + suppression si demandé."""
        self.poll()
        self._flush()
        if self.delete_after and self._upload_fail == 0:
            try:
                self.path.unlink()
                print(f"[STREAMER] {self.path.name} supprimé (upload complet)", flush=True)
            except Exception:
                pass
        print(f"[STREAMER] {self.path.name} finalisé — "
              f"ok={self._upload_ok} fail={self._upload_fail}", flush=True)


# ── Surveiller un répertoire ───────────────────────────────────────────────
class DirectoryWatcher:
    """Surveille tous les fichiers *.csv et *.log d'un répertoire."""
    def __init__(self, directory: Path, run_id: str, delete_after: bool = False):
        self.directory    = directory
        self.run_id       = run_id
        self.delete_after = delete_after
        self._watchers: dict[Path, FileWatcher] = {}

    def _discover(self):
        for ext in ("*.csv", "*.log"):
            for p in self.directory.rglob(ext):
                if p not in self._watchers:
                    print(f"[STREAMER] Nouveau fichier détecté : {p.name}", flush=True)
                    self._watchers[p] = FileWatcher(p, self.run_id, self.delete_after)

    def poll_all(self):
        self._discover()
        for w in list(self._watchers.values()):
            w.poll()

    def finalize_all(self):
        for w in self._watchers.values():
            w.finalize()


# ── Thread de surveillance ─────────────────────────────────────────────────
def watch_loop(target: Path, run_id: str, delete_after: bool):
    print(f"[STREAMER] Démarrage — run_id={run_id} cible={target} "
          f"supabase={'OK' if _is_supabase_ok() else 'ABSENT'}", flush=True)

    if not _is_supabase_ok():
        print("[STREAMER][WARN] Supabase non disponible — streaming local uniquement", flush=True)

    if target.is_dir():
        watcher = DirectoryWatcher(target, run_id, delete_after)
        poll_fn   = watcher.poll_all
        final_fn  = watcher.finalize_all
    else:
        fw = FileWatcher(target, run_id, delete_after)
        poll_fn  = fw.poll
        final_fn = fw.finalize

    while not _stop_event.is_set():
        poll_fn()
        time.sleep(POLL_INTERVAL)

    final_fn()
    print("[STREAMER] Arrêt propre.", flush=True)


def _handle_signal(sig, frame):
    print(f"[STREAMER] Signal {sig} reçu — arrêt propre...", flush=True)
    _stop_event.set()


# ── Point d'entrée ─────────────────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(description="Streamer temps réel → Supabase")
    parser.add_argument("--watch",      required=True,  help="Fichier ou répertoire à surveiller")
    parser.add_argument("--run-id",     required=True,  help="run_id pour Supabase")
    parser.add_argument("--delete-after", action="store_true",
                        help="Supprimer localement après upload réussi")
    parser.add_argument("--duration",   type=int, default=0,
                        help="Durée max en secondes (0 = infini)")
    args = parser.parse_args()

    target = Path(args.watch)
    if not target.exists():
        print(f"[STREAMER][ERR] Cible introuvable : {target}", flush=True)
        sys.exit(1)

    signal.signal(signal.SIGTERM, _handle_signal)
    signal.signal(signal.SIGINT,  _handle_signal)

    if args.duration > 0:
        def _auto_stop():
            time.sleep(args.duration)
            _stop_event.set()
        threading.Thread(target=_auto_stop, daemon=True).start()

    watch_loop(target, args.run_id, args.delete_after)


if __name__ == "__main__":
    main()
