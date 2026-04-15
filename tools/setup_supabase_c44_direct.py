#!/usr/bin/env python3
"""
LumVorax — Setup Supabase C44 — Connexion PostgreSQL directe (SUPABASE_DB_HOST2)
"""
import os, json, time, urllib.request

SUPABASE_URL = os.environ.get("SUPABASE_URL", "").rstrip("/")
SUPABASE_SERVICE_ROLE_KEY = os.environ.get("SUPABASE_SERVICE_ROLE_KEY", "")

DB_HOST = os.environ.get("SUPABASE_DB_HOST2") or os.environ.get("SUPABASE_DB_HOST", "")
DB_PORT = os.environ.get("SUPABASE_DB_PORT2") or "5432"
DB_USER = os.environ.get("SUPABASE_DB_USER2") or os.environ.get("SUPABASE_DB_USER", "")
DB_PASS = os.environ.get("SUPABASE_DB_PASSWORD", "")

print(f"DB_HOST={DB_HOST[:30]}... PORT={DB_PORT} USER={DB_USER[:20]}")

DDL = [
("standard_names_registry", """
CREATE TABLE IF NOT EXISTS standard_names_registry (
    id           SERIAL PRIMARY KEY,
    section      TEXT NOT NULL,
    key_name     TEXT NOT NULL,
    value_desc   TEXT,
    file_origin  TEXT,
    version      TEXT,
    cycle        TEXT,
    is_canonical BOOLEAN DEFAULT FALSE,
    created_at   TIMESTAMPTZ DEFAULT NOW(),
    UNIQUE (section, key_name)
)"""),
("quantum_benchmarks", """
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
)"""),
("run_scores", """
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
)"""),
("quantum_run_files", """
CREATE TABLE IF NOT EXISTS quantum_run_files (
    id         SERIAL PRIMARY KEY,
    run_id     TEXT,
    path       TEXT,
    size       BIGINT,
    checksum   TEXT,
    created_at TIMESTAMPTZ DEFAULT NOW()
)"""),
("quantum_csv_rows", """
CREATE TABLE IF NOT EXISTS quantum_csv_rows (
    id       BIGSERIAL PRIMARY KEY,
    run_id   TEXT,
    row_json TEXT NOT NULL
)"""),
("benchmark_runtime", """
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
)"""),
("research_modules_config", """
CREATE TABLE IF NOT EXISTS research_modules_config (
    id         SERIAL PRIMARY KEY,
    module     TEXT UNIQUE,
    enabled    BOOLEAN DEFAULT TRUE,
    lx         INTEGER,
    ly         INTEGER,
    notes      TEXT,
    created_at TIMESTAMPTZ DEFAULT NOW()
)"""),
("problems_config", """
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
)"""),
("btc_run_metrics", """
CREATE TABLE IF NOT EXISTS btc_run_metrics (
    id                 SERIAL PRIMARY KEY,
    run_id             TEXT UNIQUE,
    schema_version     TEXT DEFAULT 'lumvorax_btc_pow_candidate_v1',
    cycle              TEXT DEFAULT 'C44',
    best_leading_zeros INTEGER,
    best_nonce         BIGINT,
    hashrate_mhs_final DOUBLE PRECISION,
    nx48_neuron_count  DOUBLE PRECISION,
    metrics_count      INTEGER,
    anomalies_count    INTEGER,
    duration_s         DOUBLE PRECISION,
    threads            INTEGER,
    nx48_enabled       BOOLEAN,
    header_hex         TEXT,
    notes              TEXT,
    created_at         TIMESTAMPTZ DEFAULT NOW()
)"""),
("btc_nx48_ab_benchmarks", """
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
)"""),
("artifact_sync_log", """
CREATE TABLE IF NOT EXISTS artifact_sync_log (
    id          SERIAL PRIMARY KEY,
    cycle       TEXT DEFAULT 'C44',
    artifact    TEXT,
    sha256      TEXT,
    destination TEXT,
    status      TEXT,
    details     TEXT,
    created_at  TIMESTAMPTZ DEFAULT NOW()
)"""),
]

results = {}

try:
    import psycopg2
    conn = psycopg2.connect(
        host=DB_HOST, port=int(DB_PORT), dbname="postgres",
        user=DB_USER, password=DB_PASS, sslmode="require",
        connect_timeout=15
    )
    conn.autocommit = True
    cur = conn.cursor()
    for name, ddl in DDL:
        try:
            cur.execute(ddl)
            results[name] = "OK"
            print(f"  ✅ {name}")
        except Exception as e:
            results[name] = f"ERROR: {e}"
            print(f"  ❌ {name}: {e}")
    cur.close()
    conn.close()
    db_ok = True
    print(f"\n→ {sum(1 for v in results.values() if v=='OK')}/{len(DDL)} tables OK via psycopg2")
except Exception as e:
    db_ok = False
    print(f"  Psycopg2 ERREUR: {e}")

    print("\n→ Tentative via REST API Supabase (schema public)...")
    if SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY:
        for name, ddl in DDL:
            payload = json.dumps({"query": ddl.strip()}).encode()
            headers = {
                "apikey": SUPABASE_SERVICE_ROLE_KEY,
                "Authorization": f"Bearer {SUPABASE_SERVICE_ROLE_KEY}",
                "Content-Type": "application/json",
            }
            req = urllib.request.Request(f"{SUPABASE_URL}/rest/v1/rpc/exec_sql", data=payload, headers=headers, method="POST")
            try:
                with urllib.request.urlopen(req, timeout=15) as resp:
                    results[name] = f"REST OK {resp.status}"
                    print(f"  ✅ {name} (REST)")
            except Exception as e2:
                results[name] = f"REST ERROR: {e2}"
                print(f"  ⚠️  {name} (REST): {e2}")

if SUPABASE_URL and SUPABASE_SERVICE_ROLE_KEY:
    entries = [
        {"section": "LUMVORAX_C44_SETUP", "key_name": f"table_{n}", "value_desc": f"Table {n} créée cycle C44", "version": "4.4", "cycle": "C44", "is_canonical": True}
        for n, _ in DDL
    ]
    headers = {
        "apikey": SUPABASE_SERVICE_ROLE_KEY,
        "Authorization": f"Bearer {SUPABASE_SERVICE_ROLE_KEY}",
        "Content-Type": "application/json",
        "Prefer": "resolution=merge-duplicates,return=minimal",
    }
    url = f"{SUPABASE_URL}/rest/v1/standard_names_registry?on_conflict=section,key_name"
    try:
        req = urllib.request.Request(url, data=json.dumps(entries).encode(), headers=headers, method="POST")
        with urllib.request.urlopen(req, timeout=20) as resp:
            print(f"\n→ Registre standard_names_registry : {resp.status} OK ({len(entries)} entrées)")
    except Exception as e:
        print(f"\n→ Registre REST : {e}")

summary = {"cycle": "C44", "created_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()), "db_ok": db_ok, "tables": results}
os.makedirs("logs/forensic", exist_ok=True)
with open("logs/forensic/supabase_c44_setup.json", "w") as f:
    json.dump(summary, f, indent=2)
print(f"\nRapport: logs/forensic/supabase_c44_setup.json")
