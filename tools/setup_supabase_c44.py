#!/usr/bin/env python3
"""
LumVorax — Setup Supabase C44
Recrée toutes les tables du nouveau compte Supabase avec les noms exacts de STANDARD_NAMES.md.
Cycle C44 — 2026-04-15
"""
import os
import json
import time
import urllib.request
import urllib.parse

SUPABASE_URL = os.environ.get("SUPABASE_URL", "").rstrip("/")
SUPABASE_SERVICE_ROLE_KEY = os.environ.get("SUPABASE_SERVICE_ROLE_KEY", "")
SUPABASE_DB_HOST = os.environ.get("SUPABASE_DB_HOST2") or os.environ.get("SUPABASE_DB_HOST", "")
SUPABASE_DB_PORT = os.environ.get("SUPABASE_DB_PORT2") or os.environ.get("SUPABASE_DB_PORT", "5432")
SUPABASE_DB_USER = os.environ.get("SUPABASE_DB_USER2") or os.environ.get("SUPABASE_DB_USER", "postgres")
SUPABASE_DB_PASSWORD = os.environ.get("SUPABASE_DB_PASSWORD", "")
DATABASE_URL = os.environ.get("DATABASE_URL", "")

DDL_STATEMENTS = [
    """
CREATE TABLE IF NOT EXISTS standard_names_registry (
    id          SERIAL PRIMARY KEY,
    section     TEXT NOT NULL,
    key_name    TEXT NOT NULL,
    value_desc  TEXT,
    file_origin TEXT,
    version     TEXT,
    cycle       TEXT,
    is_canonical BOOLEAN DEFAULT FALSE,
    created_at  TIMESTAMPTZ DEFAULT NOW(),
    UNIQUE (section, key_name)
)
""",
    """
CREATE TABLE IF NOT EXISTS quantum_benchmarks (
    id               SERIAL PRIMARY KEY,
    dataset          TEXT,
    module           TEXT,
    observable       TEXT,
    t_k              DOUBLE PRECISION,
    u_over_t         DOUBLE PRECISION,
    reference_value  DOUBLE PRECISION,
    reference_method TEXT,
    source           TEXT,
    error_bar        DOUBLE PRECISION,
    notes            TEXT,
    created_at       TIMESTAMPTZ DEFAULT NOW()
)
""",
    """
CREATE TABLE IF NOT EXISTS run_scores (
    id           SERIAL PRIMARY KEY,
    run_id       TEXT UNIQUE,
    runner       TEXT,
    score_iso    INTEGER,
    score_trace  INTEGER,
    score_repr   INTEGER,
    score_robust INTEGER,
    score_phys   INTEGER,
    score_expert INTEGER,
    score_total  INTEGER,
    notes        TEXT,
    created_at   TIMESTAMPTZ DEFAULT NOW()
)
""",
    """
CREATE TABLE IF NOT EXISTS quantum_run_files (
    id         SERIAL PRIMARY KEY,
    run_id     TEXT,
    path       TEXT,
    size       BIGINT,
    checksum   TEXT,
    created_at TIMESTAMPTZ DEFAULT NOW()
)
""",
    """
CREATE TABLE IF NOT EXISTS quantum_csv_rows (
    id       BIGSERIAL PRIMARY KEY,
    run_id   TEXT,
    row_json TEXT NOT NULL
)
""",
    """
CREATE TABLE IF NOT EXISTS benchmark_runtime (
    id                SERIAL PRIMARY KEY,
    run_id            TEXT,
    benchmark_type    TEXT,
    u_ev              DOUBLE PRECISION,
    module            TEXT,
    observable        TEXT,
    t_k               DOUBLE PRECISION,
    reference_value   DOUBLE PRECISION,
    error_bar         DOUBLE PRECISION,
    model_value       DOUBLE PRECISION,
    abs_error         DOUBLE PRECISION,
    rel_error         DOUBLE PRECISION,
    within_error_bar  BOOLEAN,
    rmse_global       DOUBLE PRECISION,
    mae_global        DOUBLE PRECISION,
    pct_within_global DOUBLE PRECISION,
    created_at        TIMESTAMPTZ DEFAULT NOW()
)
""",
    """
CREATE TABLE IF NOT EXISTS research_modules_config (
    id         SERIAL PRIMARY KEY,
    module     TEXT UNIQUE,
    enabled    BOOLEAN DEFAULT TRUE,
    lx         INTEGER,
    ly         INTEGER,
    notes      TEXT,
    created_at TIMESTAMPTZ DEFAULT NOW()
)
""",
    """
CREATE TABLE IF NOT EXISTS problems_config (
    id         SERIAL PRIMARY KEY,
    name       TEXT UNIQUE,
    lx         INTEGER,
    ly         INTEGER,
    t_ev       DOUBLE PRECISION,
    u_ev       DOUBLE PRECISION,
    mu_ev      DOUBLE PRECISION,
    temp_k     DOUBLE PRECISION,
    dt         DOUBLE PRECISION,
    steps      INTEGER,
    cycle      INTEGER,
    notes      TEXT,
    created_at TIMESTAMPTZ DEFAULT NOW()
)
""",
    """
CREATE TABLE IF NOT EXISTS btc_run_metrics (
    id                  SERIAL PRIMARY KEY,
    run_id              TEXT UNIQUE,
    schema_version      TEXT DEFAULT 'lumvorax_btc_pow_candidate_v1',
    cycle               TEXT DEFAULT 'C44',
    best_leading_zeros  INTEGER,
    best_nonce          BIGINT,
    hashrate_mhs_final  DOUBLE PRECISION,
    nx48_neuron_count   DOUBLE PRECISION,
    metrics_count       INTEGER,
    anomalies_count     INTEGER,
    duration_s          DOUBLE PRECISION,
    threads             INTEGER,
    nx48_enabled        BOOLEAN,
    header_hex          TEXT,
    notes               TEXT,
    created_at          TIMESTAMPTZ DEFAULT NOW()
)
""",
    """
CREATE TABLE IF NOT EXISTS btc_nx48_ab_benchmarks (
    id                    SERIAL PRIMARY KEY,
    schema_version        TEXT DEFAULT 'lumvorax_btc_nx48_ab_benchmark_v1',
    cycle                 TEXT DEFAULT 'C44',
    created_at_run        TEXT,
    duration_s_per_case   INTEGER,
    threads               INTEGER,
    case_a_nx48_enabled   BOOLEAN,
    case_a_leading_zeros  INTEGER,
    case_a_hashrate_mhs   DOUBLE PRECISION,
    case_a_returncode     INTEGER,
    case_b_nx48_enabled   BOOLEAN,
    case_b_leading_zeros  INTEGER,
    case_b_hashrate_mhs   DOUBLE PRECISION,
    case_b_returncode     INTEGER,
    notes                 TEXT,
    created_at            TIMESTAMPTZ DEFAULT NOW()
)
""",
    """
CREATE TABLE IF NOT EXISTS artifact_sync_log (
    id          SERIAL PRIMARY KEY,
    cycle       TEXT DEFAULT 'C44',
    artifact    TEXT,
    sha256      TEXT,
    destination TEXT,
    status      TEXT,
    details     TEXT,
    created_at  TIMESTAMPTZ DEFAULT NOW()
)
""",
]


def create_tables_via_psycopg2():
    try:
        import psycopg2
    except ImportError:
        return False, "psycopg2 non installé"

    conn_str = DATABASE_URL
    if not conn_str:
        if not SUPABASE_DB_HOST or not SUPABASE_DB_PASSWORD:
            return False, "DATABASE_URL et SUPABASE_DB_HOST/PASSWORD absents"
        conn_str = (
            f"host={SUPABASE_DB_HOST} port={SUPABASE_DB_PORT} "
            f"dbname=postgres user={SUPABASE_DB_USER} password={SUPABASE_DB_PASSWORD} sslmode=require"
        )

    results = []
    try:
        conn = psycopg2.connect(conn_str)
        conn.autocommit = True
        cur = conn.cursor()
        for ddl in DDL_STATEMENTS:
            table_name = "unknown"
            for word in ddl.split():
                if word.strip("(\n") not in ("CREATE", "TABLE", "IF", "NOT", "EXISTS", ""):
                    table_name = word.strip("(\n")
                    break
            try:
                cur.execute(ddl)
                results.append({"table": table_name, "status": "OK"})
                print(f"  ✅ {table_name} créée/vérifiée")
            except Exception as e:
                results.append({"table": table_name, "status": "ERROR", "error": str(e)})
                print(f"  ❌ {table_name} : {e}")
        cur.close()
        conn.close()
        return True, results
    except Exception as e:
        return False, str(e)


def insert_c44_registry_entry():
    if not SUPABASE_URL or not SUPABASE_SERVICE_ROLE_KEY:
        return {"ok": False, "reason": "SUPABASE_URL ou SERVICE_ROLE_KEY absent"}
    entries = [
        {
            "section": "LUMVORAX_C44_SETUP",
            "key_name": "setup_supabase_c44",
            "value_desc": "Recréation tables nouveau compte Supabase C44 — 2026-04-15",
            "file_origin": "tools/setup_supabase_c44.py",
            "version": "4.4",
            "cycle": "C44",
            "is_canonical": True,
        },
        {
            "section": "LUMVORAX_C44_SETUP",
            "key_name": "btc_run_metrics_table",
            "value_desc": "Table btc_run_metrics — résultats forensic BTC par run",
            "file_origin": "tools/setup_supabase_c44.py",
            "version": "4.4",
            "cycle": "C44",
            "is_canonical": True,
        },
        {
            "section": "LUMVORAX_C44_SETUP",
            "key_name": "btc_nx48_ab_benchmarks_table",
            "value_desc": "Table btc_nx48_ab_benchmarks — benchmark A/B NX48",
            "file_origin": "tools/setup_supabase_c44.py",
            "version": "4.4",
            "cycle": "C44",
            "is_canonical": True,
        },
        {
            "section": "LUMVORAX_C44_SETUP",
            "key_name": "artifact_sync_log_table",
            "value_desc": "Table artifact_sync_log — traçabilité syncs artefacts",
            "file_origin": "tools/setup_supabase_c44.py",
            "version": "4.4",
            "cycle": "C44",
            "is_canonical": True,
        },
    ]
    headers = {
        "apikey": SUPABASE_SERVICE_ROLE_KEY,
        "Authorization": f"Bearer {SUPABASE_SERVICE_ROLE_KEY}",
        "Content-Type": "application/json",
        "Prefer": "resolution=merge-duplicates,return=minimal",
    }
    url = f"{SUPABASE_URL}/rest/v1/standard_names_registry?on_conflict=section,key_name"
    data = json.dumps(entries).encode()
    req = urllib.request.Request(url, data=data, headers=headers, method="POST")
    try:
        with urllib.request.urlopen(req, timeout=20) as resp:
            return {"ok": True, "status": resp.status, "count": len(entries)}
    except Exception as e:
        return {"ok": False, "error": str(e)}


def main():
    print("=" * 60)
    print("LumVorax — Setup Supabase C44 — Tables + Registre")
    print("=" * 60)
    print()

    print("[1/2] Création des tables via connexion PostgreSQL directe...")
    ok, result = create_tables_via_psycopg2()
    if ok:
        print(f"  → {len(result)} tables traitées")
    else:
        print(f"  → Échec psycopg2 : {result}")

    print()
    print("[2/2] Enregistrement entrées C44 dans standard_names_registry (REST)...")
    reg = insert_c44_registry_entry()
    print(f"  → {reg}")

    print()
    summary = {
        "cycle": "C44",
        "created_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "tables_ok": ok,
        "tables_result": result if isinstance(result, list) else str(result),
        "registry_sync": reg,
    }
    os.makedirs("logs/forensic", exist_ok=True)
    with open("logs/forensic/supabase_c44_setup.json", "w") as f:
        json.dump(summary, f, indent=2)
    print(f"Rapport écrit : logs/forensic/supabase_c44_setup.json")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
