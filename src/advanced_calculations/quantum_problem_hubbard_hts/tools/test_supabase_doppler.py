#!/usr/bin/env python3
"""
test_supabase_doppler.py — Validation complète Supabase + Doppler
Conforme STANDARD_NAMES.md Section H + Section J (tables + colonnes exactes)

Usage:
    python3 test_supabase_doppler.py
    python3 test_supabase_doppler.py --create-tables
    python3 test_supabase_doppler.py --full

Retourne 0 si tous les tests passent, 1 sinon.
"""

import os
import sys
import json
import time
from datetime import datetime, timezone

# ── Auto-install dépendances manquantes ────────────────────────────────────────
def _ensure(pkg, import_name=None):
    name = import_name or pkg
    try:
        __import__(name)
    except ImportError:
        import subprocess
        print(f"[AUTO-INSTALL] {pkg} manquant — installation...")
        subprocess.run([sys.executable, "-m", "pip", "install", pkg, "-q"], check=False)

_ensure("requests")
_ensure("psycopg2-binary", "psycopg2")

import requests

SUPABASE_URL    = ""
SERVICE_KEY     = os.environ.get("SUPABASE_SERVICE_ROLE_KEY", "").strip()
DOPPLER_TOKEN   = os.environ.get("DOPPLER_TOKEN", "").strip()
DOPPLER_API_KEY = os.environ.get("DOPPLER_API_KEY", "").strip()

def _derive_url():
    u = (os.environ.get("SUPABASE8_API_URL") or
         os.environ.get("SUPABASE_URL_HTTP") or "").strip().rstrip("/")
    if u.startswith("https://") and "supabase.co" in u:
        return u
    db = os.environ.get("SUPABASE_DB_HOST","").strip()
    if db.startswith("db.") and ".supabase.co" in db:
        pid = db[3:].replace(".supabase.co","")
        return f"https://{pid}.supabase.co"
    return ""

SUPABASE_URL = _derive_url()

PASS = "\033[32m✔ PASS\033[0m"
FAIL = "\033[31m✘ FAIL\033[0m"
WARN = "\033[33m⚠ WARN\033[0m"

results = []

def record(name, ok, detail=""):
    tag = PASS if ok else FAIL
    print(f"  [{tag}] {name}" + (f" — {detail}" if detail else ""))
    results.append((name, ok))
    return ok

# ══════════════════════════════════════════════════════════════════════════════
# TEST 1 — Variables d'environnement (STANDARD_NAMES.md Section H)
# ══════════════════════════════════════════════════════════════════════════════
print("\n═══ TEST 1 : Variables d'environnement (Section H) ═══")
record("SUPABASE8_API_URL",          bool(SUPABASE_URL),    SUPABASE_URL or "ABSENT")
record("SUPABASE_SERVICE_ROLE_KEY",  bool(SERVICE_KEY),     "présent" if SERVICE_KEY else "ABSENT")
record("SUPABASE_DB_HOST",           bool(os.environ.get("SUPABASE_DB_HOST","")), os.environ.get("SUPABASE_DB_HOST","ABSENT"))
record("SUPABASE_DB_USER",           bool(os.environ.get("SUPABASE_DB_USER","")), "présent" if os.environ.get("SUPABASE_DB_USER") else "ABSENT")
record("SUPABASE_DB_PASSWORD",       bool(os.environ.get("SUPABASE_DB_PASSWORD","")), "présent" if os.environ.get("SUPABASE_DB_PASSWORD") else "ABSENT")
record("DOPPLER_TOKEN",              bool(DOPPLER_TOKEN),   "présent" if DOPPLER_TOKEN else "ABSENT")
record("DOPPLER_API_KEY",            bool(DOPPLER_API_KEY), "présent" if DOPPLER_API_KEY else "ABSENT")

# ══════════════════════════════════════════════════════════════════════════════
# TEST 2 — Connexion Doppler API
# ══════════════════════════════════════════════════════════════════════════════
print("\n═══ TEST 2 : Connexion Doppler API ═══")
def test_doppler():
    for token_val, token_name in [(DOPPLER_TOKEN, "DOPPLER_TOKEN"), (DOPPLER_API_KEY, "DOPPLER_API_KEY")]:
        if not token_val:
            continue
        try:
            r = requests.get(
                "https://api.doppler.com/v3/me",
                headers={"Authorization": f"Bearer {token_val}"},
                timeout=10
            )
            if r.status_code == 200:
                data = r.json()
                workplace = data.get("workplace", {}).get("name","—")
                record(f"Doppler API ({token_name})", True, f"workplace={workplace}")
                return True
            else:
                record(f"Doppler API ({token_name})", False, f"HTTP {r.status_code} — {r.text[:200]}")
        except Exception as e:
            record(f"Doppler API ({token_name})", False, str(e))
    if not DOPPLER_TOKEN and not DOPPLER_API_KEY:
        record("Doppler API", False, "aucun token disponible")
    return False

doppler_ok = test_doppler()

# Test secrets Doppler (liste les secrets disponibles)
if doppler_ok and DOPPLER_TOKEN:
    try:
        r2 = requests.get(
            "https://api.doppler.com/v3/configs/config/secrets",
            headers={"Authorization": f"Bearer {DOPPLER_TOKEN}"},
            params={"project": "lumvorax", "config": "dev_lumvorax"},
            timeout=10
        )
        if r2.status_code == 200:
            secrets = r2.json().get("secrets", {})
            has_supa = any("SUPABASE" in k for k in secrets)
            record("Doppler secrets SUPABASE présents", has_supa,
                   f"{sum(1 for k in secrets if 'SUPABASE' in k)} clés SUPABASE trouvées")
        else:
            print(f"  [{WARN}] Doppler secrets : HTTP {r2.status_code} (projet/config peut différer)")
    except Exception as e:
        print(f"  [{WARN}] Doppler secrets : {e}")

# ══════════════════════════════════════════════════════════════════════════════
# TEST 3 — Connexion Supabase REST (PostgREST)
# ══════════════════════════════════════════════════════════════════════════════
print("\n═══ TEST 3 : Connexion Supabase REST (PostgREST) ═══")

def _hdrs():
    return {
        "apikey":        SERVICE_KEY,
        "Authorization": f"Bearer {SERVICE_KEY}",
        "Content-Type":  "application/json",
        "Prefer":        "return=minimal",
    }

def _rest(table):
    return f"{SUPABASE_URL}/rest/v1/{table}"

rest_ok = False
if not SUPABASE_URL:
    record("URL Supabase dérivée", False, "URL manquante — vérifier SUPABASE8_API_URL")
elif not SERVICE_KEY:
    record("SERVICE_ROLE_KEY présent", False, "clé JWT absente")
else:
    try:
        r = requests.get(f"{SUPABASE_URL}/rest/v1/", headers=_hdrs(), timeout=10)
        rest_ok = r.status_code in (200, 404)  # 404 = pas de table listée mais connexion ok
        record("Supabase REST endpoint", rest_ok, f"HTTP {r.status_code}")
    except Exception as e:
        record("Supabase REST endpoint", False, str(e))

# ══════════════════════════════════════════════════════════════════════════════
# TEST 4 — Connexion PostgreSQL (pooler via SUPABASE_URL2 ou direct)
# ══════════════════════════════════════════════════════════════════════════════
print("\n═══ TEST 4 : Connexion PostgreSQL directe ═══")
pg_ok = False
try:
    import re as _re
    import psycopg2

    def _try_pg_connect(host, port, user, passwd, dbname):
        conn = psycopg2.connect(
            host=host, port=port, user=user, password=passwd,
            dbname=dbname, connect_timeout=10, sslmode="require"
        )
        cur = conn.cursor()
        cur.execute("SELECT version();")
        ver = cur.fetchone()[0]
        conn.close()
        return ver

    db_pass = os.environ.get("SUPABASE_DB_PASSWORD","").strip()
    pg_ver  = None

    # — Tentative 1 : pooler via SUPABASE_URL2 (aws-1-eu-west-1, port 6543) —
    url2 = os.environ.get("SUPABASE_URL2","").strip()
    if url2:
        h = _re.search(r'-h\s+(\S+)', url2)
        p = _re.search(r'-p\s+(\d+)', url2)
        u = _re.search(r'-U\s+(\S+)', url2)
        d = _re.search(r'-d\s+(\S+)', url2)
        if h and u and db_pass:
            try:
                pg_ver = _try_pg_connect(
                    h.group(1), int(p.group(1)) if p else 6543,
                    u.group(1), db_pass, d.group(1) if d else "postgres"
                )
                pg_ok = True
                record("PostgreSQL pooler (SUPABASE_URL2)", True, pg_ver[:80])
            except Exception as e1:
                record("PostgreSQL pooler (SUPABASE_URL2)", False, str(e1)[:100])

    # — Tentative 2 : connexion directe SUPABASE_DB_HOST (port 5432) —
    if not pg_ok:
        db_host = os.environ.get("SUPABASE_DB_HOST","").strip()
        db_user = os.environ.get("SUPABASE_DB_USER","postgres").strip()
        db_port = int(os.environ.get("SUPABASE_DB_PORT","5432"))
        db_name = os.environ.get("SUPABASE_DB_NAME","postgres").strip()
        if not db_host or not db_pass:
            record("PostgreSQL direct", False, "SUPABASE_DB_HOST ou DB_PASSWORD absent")
        else:
            try:
                pg_ver = _try_pg_connect(db_host, db_port, db_user, db_pass, db_name)
                pg_ok = True
                record("PostgreSQL direct", True, pg_ver[:80])
            except Exception as e2:
                record("PostgreSQL direct", False, str(e2)[:120])
except Exception as e:
    record("PostgreSQL direct", False, str(e)[:120])

# ══════════════════════════════════════════════════════════════════════════════
# TEST 5 — Tables Supabase (STANDARD_NAMES.md Section J — noms exacts)
# ══════════════════════════════════════════════════════════════════════════════
print("\n═══ TEST 5 : Tables Supabase (Section J) ═══")

# Tables officiales STANDARD_NAMES.md Section J
TABLES_REQUIRED = {
    "quantum_run_files": {
        "cols": ["run_id","module","lx","ly","t_ev","u_ev","mu_ev",
                 "temp_k","dt","steps","energy","pairing","sign_ratio",
                 "cpu_percent","ram_percent","created_at"]
    },
    "quantum_csv_rows": {
        "cols": ["run_id","file_name","row_number","data","created_at"]
    },
    "quantum_benchmarks": {
        "cols": ["id","dataset","module","observable","t_k","u_over_t",
                 "reference_value","reference_method","source","error_bar",
                 "notes","created_at"]
    },
    "run_scores": {
        "cols": ["id","run_id","runner","score_iso","score_trace","score_repr",
                 "score_robust","score_phys","score_expert","score_total",
                 "notes","created_at"]
    },
    "benchmark_runtime": {
        "cols": ["run_id","dataset","module","observable","t_k","u_over_t",
                 "reference_value","error_bar","model_value","abs_error",
                 "rel_error","within_error_bar","created_at"]
    },
    "research_modules_config": {
        "cols": ["id","module","lx","ly","t_ev","u_ev","mu_ev","temp_k",
                 "dt","steps","created_at"]
    },
    "problems_config": {
        "cols": ["id","name","lx","ly","t_ev","u_ev","mu_ev","temp_k",
                 "dt","steps","enabled","created_at"]
    },
    "quantum_realtime_logs": {
        "cols": ["id","run_id","ts_utc","ts_ns","pid","event_type",
                 "module","metric","value","detail","created_at"]
    },
}

tables_status = {}
for table, meta in TABLES_REQUIRED.items():
    if not rest_ok:
        print(f"  [SKIP] {table} — REST non disponible")
        continue
    try:
        r = requests.get(f"{_rest(table)}?limit=1", headers=_hdrs(), timeout=10)
        if r.status_code == 200:
            tables_status[table] = "EXISTS"
            record(f"Table '{table}'", True)
        elif r.status_code == 404:
            tables_status[table] = "MISSING"
            record(f"Table '{table}'", False, "TABLE MANQUANTE — lancer --create-tables")
        else:
            tables_status[table] = f"ERR({r.status_code})"
            record(f"Table '{table}'", False, f"HTTP {r.status_code}")
    except Exception as e:
        tables_status[table] = f"ERR({e})"
        record(f"Table '{table}'", False, str(e)[:80])

# ══════════════════════════════════════════════════════════════════════════════
# TEST 6 — Vérification colonnes exactes (STANDARD_NAMES.md)
# ══════════════════════════════════════════════════════════════════════════════
print("\n═══ TEST 6 : Colonnes STANDARD_NAMES.md (noms exacts) ═══")
if pg_ok:
    try:
        import psycopg2
        conn = psycopg2.connect(
            host=os.environ.get("SUPABASE_DB_HOST",""),
            user=os.environ.get("SUPABASE_DB_USER","postgres"),
            password=os.environ.get("SUPABASE_DB_PASSWORD",""),
            port=int(os.environ.get("SUPABASE_DB_PORT","5432")),
            dbname=os.environ.get("SUPABASE_DB_NAME","postgres"),
            connect_timeout=10, sslmode="require"
        )
        cur = conn.cursor()
        for table, meta in TABLES_REQUIRED.items():
            if tables_status.get(table) != "EXISTS":
                continue
            cur.execute("""
                SELECT column_name FROM information_schema.columns
                WHERE table_schema='public' AND table_name=%s
                ORDER BY ordinal_position
            """, (table,))
            actual_cols = [row[0] for row in cur.fetchall()]
            expected = meta["cols"]
            missing  = [c for c in expected if c not in actual_cols]
            extra    = [c for c in actual_cols if c not in expected]
            ok = (len(missing) == 0)
            detail = ""
            if missing: detail += f"MANQUANTES={missing} "
            if extra:   detail += f"EXTRA={extra} "
            if not detail: detail = f"{len(actual_cols)} colonnes OK"
            record(f"Colonnes '{table}'", ok, detail)
        conn.close()
    except Exception as e:
        print(f"  [{WARN}] Vérification colonnes : {e}")
else:
    print(f"  [{WARN}] Vérification colonnes ignorée (PostgreSQL non disponible)")

# ══════════════════════════════════════════════════════════════════════════════
# TEST 7 — Écriture + lecture test dans quantum_realtime_logs
# ══════════════════════════════════════════════════════════════════════════════
print("\n═══ TEST 7 : Écriture temps réel (quantum_realtime_logs) ═══")
if rest_ok and tables_status.get("quantum_realtime_logs") == "EXISTS":
    try:
        now = datetime.now(timezone.utc).isoformat()
        payload = {
            "run_id":     "TEST_CONNEXION_DOPPLER",
            "ts_utc":     now,
            "ts_ns":      str(time.time_ns()),
            "pid":        os.getpid(),
            "event_type": "TEST",
            "module":     "test_supabase_doppler",
            "metric":     "doppler_connexion_ok",
            "value":      1.0,
            "detail":     f"test automatique {now}",
        }
        r = requests.post(_rest("quantum_realtime_logs"), headers=_hdrs(), json=payload, timeout=10)
        record("Écriture quantum_realtime_logs", r.status_code in (200,201,204),
               f"HTTP {r.status_code}")
    except Exception as e:
        record("Écriture quantum_realtime_logs", False, str(e)[:80])
else:
    print(f"  [{WARN}] Test écriture ignoré (table absente ou REST non dispo)")

# ══════════════════════════════════════════════════════════════════════════════
# TEST 8 — Création tables manquantes (--create-tables)
# ══════════════════════════════════════════════════════════════════════════════
if "--create-tables" in sys.argv and pg_ok:
    print("\n═══ CRÉATION des tables manquantes ═══")
    try:
        import psycopg2
        conn = psycopg2.connect(
            host=os.environ.get("SUPABASE_DB_HOST",""),
            user=os.environ.get("SUPABASE_DB_USER","postgres"),
            password=os.environ.get("SUPABASE_DB_PASSWORD",""),
            port=int(os.environ.get("SUPABASE_DB_PORT","5432")),
            dbname=os.environ.get("SUPABASE_DB_NAME","postgres"),
            connect_timeout=10, sslmode="require"
        )
        cur = conn.cursor()

        ddls = {
            "quantum_realtime_logs": """
                CREATE TABLE IF NOT EXISTS quantum_realtime_logs (
                    id          BIGSERIAL PRIMARY KEY,
                    run_id      TEXT NOT NULL,
                    ts_utc      TIMESTAMPTZ,
                    ts_ns       BIGINT,
                    pid         INT,
                    event_type  TEXT,
                    module      TEXT,
                    metric      TEXT,
                    value       DOUBLE PRECISION,
                    detail      TEXT,
                    created_at  TIMESTAMPTZ DEFAULT now()
                );
                CREATE INDEX IF NOT EXISTS idx_rtlogs_run_id ON quantum_realtime_logs(run_id);
                CREATE INDEX IF NOT EXISTS idx_rtlogs_module  ON quantum_realtime_logs(module);
                CREATE INDEX IF NOT EXISTS idx_rtlogs_metric  ON quantum_realtime_logs(metric);
            """,
            "research_modules_config": """
                CREATE TABLE IF NOT EXISTS research_modules_config (
                    id         SERIAL PRIMARY KEY,
                    module     TEXT NOT NULL,
                    lx         INT,
                    ly         INT,
                    t_ev       DOUBLE PRECISION,
                    u_ev       DOUBLE PRECISION,
                    mu_ev      DOUBLE PRECISION,
                    temp_k     DOUBLE PRECISION,
                    dt         DOUBLE PRECISION,
                    steps      BIGINT,
                    created_at TIMESTAMPTZ DEFAULT now()
                );
            """,
            "problems_config": """
                CREATE TABLE IF NOT EXISTS problems_config (
                    id         SERIAL PRIMARY KEY,
                    name       TEXT NOT NULL UNIQUE,
                    lx         INT,
                    ly         INT,
                    t_ev       DOUBLE PRECISION,
                    u_ev       DOUBLE PRECISION,
                    mu_ev      DOUBLE PRECISION,
                    temp_k     DOUBLE PRECISION,
                    dt         DOUBLE PRECISION,
                    steps      BIGINT,
                    enabled    BOOLEAN DEFAULT TRUE,
                    created_at TIMESTAMPTZ DEFAULT now()
                );
            """,
        }

        for table, ddl in ddls.items():
            try:
                cur.execute(ddl)
                conn.commit()
                record(f"CREATE TABLE {table}", True)
            except Exception as e:
                conn.rollback()
                record(f"CREATE TABLE {table}", False, str(e)[:80])

        conn.close()
    except Exception as e:
        print(f"  [{FAIL}] Création tables : {e}")

# ══════════════════════════════════════════════════════════════════════════════
# RÉSUMÉ FINAL
# ══════════════════════════════════════════════════════════════════════════════
print("\n" + "═"*60)
total = len(results)
passed = sum(1 for _, ok in results if ok)
failed = total - passed
print(f"  RÉSULTAT : {passed}/{total} tests passés — {failed} échecs")
if failed == 0:
    print(f"  [{PASS}] Toutes les connexions Supabase + Doppler sont opérationnelles")
else:
    print(f"  [{FAIL}] {failed} problème(s) détecté(s) — corriger avant lancement simulation")
print("═"*60)
sys.exit(0 if failed == 0 else 1)
