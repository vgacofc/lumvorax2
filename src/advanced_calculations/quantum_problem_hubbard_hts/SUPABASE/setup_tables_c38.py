#!/usr/bin/env python3
"""
setup_tables_c38.py — Création/mise à jour tables Supabase pour LumVorax Cycle C38
Corrections identifiées dans analysechatgpt85.3.md Section 12.

Tables créées ou ALTER-ées :
  - module_results_rcs       : résultats XEB, H_norm, KL_PT, xeb_ratio
  - module_results_worm_mc   : résultats Worm MC bosonique (C36-P3)
  - hw_samples_realtime      : échantillons HW temps réel
  - lumvorax_rotation_index  : index rotation archives LumVorax
  - simulation_runs_extended : runs complets avec métadonnées étendues
  - module_algo_conversions  : traçabilité conversions ALGO (RENORM-04)
  - benchmark_rt_results     : résultats benchmark runtime
  ALTER TABLE module_results  : colonnes supplémentaires si manquantes

Auteur : Agent Replit — Cycle C38 — 2026-04-04
"""

import os
import sys
import traceback

try:
    import pg8000
except ImportError:
    print("❌ pg8000 non installé. Lancer : pip install pg8000")
    sys.exit(1)

SUPABASE_URL = os.getenv("SUPABASE_URL", "")
SUPABASE_HOST = os.getenv("SUPABASE_DB_HOST", "")
SUPABASE_USER = os.getenv("SUPABASE_DB_USER", "postgres").strip()
SUPABASE_PASSWORD = os.getenv("SUPABASE_DB_PASSWORD", "").strip()
SUPABASE_DB = os.getenv("SUPABASE_DB_NAME", "postgres").strip()

if not SUPABASE_HOST:
    print("❌ SUPABASE_DB_HOST non défini")
    sys.exit(1)

print(f"🔗 Connexion Supabase : host={SUPABASE_HOST}, db={SUPABASE_DB}, user={SUPABASE_USER}")

try:
    conn = pg8000.connect(
        host=SUPABASE_HOST,
        port=5432,
        database=SUPABASE_DB,
        user=SUPABASE_USER,
        password=SUPABASE_PASSWORD,
        ssl_context=True,
    )
    conn.autocommit = True
    cur = conn.cursor()
    print("✅ Connexion Supabase OK")
except Exception as e:
    print(f"❌ Connexion échouée : {e}")
    traceback.print_exc()
    sys.exit(1)


def exec_sql(label, sql):
    try:
        cur.execute(sql)
        print(f"  ✅ {label}")
    except Exception as e:
        print(f"  ⚠️  {label} : {e}")


# ─── 1. Colonnes supplémentaires sur module_results ────────────────────────
print("\n📌 ALTER TABLE module_results — colonnes C38...")
alter_cols = [
    ("sign_ratio",         "DOUBLE PRECISION DEFAULT 0"),
    ("energy_drift_metric","DOUBLE PRECISION DEFAULT 0"),
    ("cpu_peak_pct",       "DOUBLE PRECISION DEFAULT 0"),
    ("elapsed_ns",         "BIGINT DEFAULT 0"),
    ("norm_deviation_max", "DOUBLE PRECISION DEFAULT 0"),
    ("beta_eV_inv",        "DOUBLE PRECISION DEFAULT 0"),
    ("U_over_t",           "DOUBLE PRECISION DEFAULT 0"),
    ("T_K",                "DOUBLE PRECISION DEFAULT 0"),
]
for col, typedef in alter_cols:
    exec_sql(
        f"ALTER module_results ADD {col}",
        f"ALTER TABLE module_results ADD COLUMN IF NOT EXISTS {col} {typedef};"
    )

# ─── 2. Table module_results_rcs ──────────────────────────────────────────
print("\n📌 CREATE TABLE module_results_rcs...")
exec_sql("CREATE module_results_rcs", """
CREATE TABLE IF NOT EXISTS module_results_rcs (
    id                 BIGSERIAL PRIMARY KEY,
    run_id             TEXT NOT NULL,
    run_ts             TIMESTAMPTZ DEFAULT now(),
    problem_name       TEXT NOT NULL,
    n_qubits           INTEGER,
    circuit_depth      INTEGER,
    n_circuits         INTEGER,
    F_xeb_mean         DOUBLE PRECISION,
    xeb_std            DOUBLE PRECISION,
    xeb_rel_var        DOUBLE PRECISION,
    H_norm             DOUBLE PRECISION,
    porter_thomas_kl   DOUBLE PRECISION,
    xeb_ratio_willow   DOUBLE PRECISION,
    norm_dev_max       DOUBLE PRECISION,
    converged          BOOLEAN,
    elapsed_ns         BIGINT,
    cpu_peak_pct       DOUBLE PRECISION,
    mem_peak_pct       DOUBLE PRECISION,
    willow_fidelity_ref DOUBLE PRECISION DEFAULT 2e-4,
    log_D_hilbert      DOUBLE PRECISION,
    bug_c_rcs_a3_fixed BOOLEAN DEFAULT TRUE,
    notes              TEXT
);
""")

# ─── 3. Table module_results_worm_mc ──────────────────────────────────────
print("\n📌 CREATE TABLE module_results_worm_mc...")
exec_sql("CREATE module_results_worm_mc", """
CREATE TABLE IF NOT EXISTS module_results_worm_mc (
    id                BIGSERIAL PRIMARY KEY,
    run_id            TEXT NOT NULL,
    run_ts            TIMESTAMPTZ DEFAULT now(),
    problem_name      TEXT NOT NULL,
    beta_eV_inv       DOUBLE PRECISION,
    T_K               DOUBLE PRECISION,
    n_sites           INTEGER,
    energy_eV         DOUBLE PRECISION,
    pairing_norm      DOUBLE PRECISION,
    acceptance_rate   DOUBLE PRECISION,
    phase_label       TEXT,
    n_proposed        BIGINT,
    n_accepted        BIGINT,
    converged         BOOLEAN,
    elapsed_ns        BIGINT,
    c38_mott_phase    BOOLEAN DEFAULT FALSE,
    c39_accept0_ok    BOOLEAN DEFAULT TRUE,
    notes             TEXT
);
""")

# ─── 4. Table hw_samples_realtime ─────────────────────────────────────────
print("\n📌 CREATE TABLE hw_samples_realtime...")
exec_sql("CREATE hw_samples_realtime", """
CREATE TABLE IF NOT EXISTS hw_samples_realtime (
    id           BIGSERIAL PRIMARY KEY,
    sampled_at   TIMESTAMPTZ DEFAULT now(),
    run_id       TEXT,
    module_name  TEXT NOT NULL,
    cpu_pct      DOUBLE PRECISION,
    mem_pct      DOUBLE PRECISION,
    elapsed_ns   BIGINT,
    step_index   BIGINT,
    notes        TEXT
);
""")

# ─── 5. Table lumvorax_rotation_index ─────────────────────────────────────
print("\n📌 CREATE TABLE lumvorax_rotation_index...")
exec_sql("CREATE lumvorax_rotation_index", """
CREATE TABLE IF NOT EXISTS lumvorax_rotation_index (
    id              BIGSERIAL PRIMARY KEY,
    run_id          TEXT NOT NULL UNIQUE,
    run_ts          TIMESTAMPTZ DEFAULT now(),
    runner_name     TEXT,
    csv_path        TEXT,
    n_rows          INTEGER,
    n_problems      INTEGER,
    phases_complete INTEGER,
    phases_total    INTEGER,
    git_sha         TEXT,
    notes           TEXT
);
""")

# ─── 6. Table simulation_runs_extended ────────────────────────────────────
print("\n📌 CREATE TABLE simulation_runs_extended...")
exec_sql("CREATE simulation_runs_extended", """
CREATE TABLE IF NOT EXISTS simulation_runs_extended (
    id                    BIGSERIAL PRIMARY KEY,
    run_id                TEXT NOT NULL UNIQUE,
    run_ts                TIMESTAMPTZ DEFAULT now(),
    runner_name           TEXT,
    n_problems            INTEGER,
    rmse_eV               DOUBLE PRECISION,
    tests_pass            INTEGER,
    tests_total           INTEGER,
    xeb_F_mean            DOUBLE PRECISION,
    worm_acceptance_rate  DOUBLE PRECISION,
    sign_ratio_median     DOUBLE PRECISION,
    elapsed_total_s       DOUBLE PRECISION,
    cpu_peak_pct          DOUBLE PRECISION,
    mem_peak_pct          DOUBLE PRECISION,
    git_sha               TEXT,
    correction_c95_applied BOOLEAN DEFAULT FALSE,
    correction_xeb_applied BOOLEAN DEFAULT FALSE,
    notes                 TEXT
);
""")

# ─── 7. Table module_algo_conversions ─────────────────────────────────────
print("\n📌 CREATE TABLE module_algo_conversions...")
exec_sql("CREATE module_algo_conversions", """
CREATE TABLE IF NOT EXISTS module_algo_conversions (
    id              BIGSERIAL PRIMARY KEY,
    run_id          TEXT NOT NULL,
    conv_ts         TIMESTAMPTZ DEFAULT now(),
    module_name     TEXT NOT NULL,
    problem_name    TEXT,
    conv_name       TEXT NOT NULL,
    conv_in         DOUBLE PRECISION,
    conv_out        DOUBLE PRECISION,
    T_star_K        DOUBLE PRECISION DEFAULT 27.0,
    renorm_tag      TEXT,
    notes           TEXT
);
""")

# ─── 8. Table benchmark_rt_results ────────────────────────────────────────
print("\n📌 CREATE TABLE benchmark_rt_results...")
exec_sql("CREATE benchmark_rt_results", """
CREATE TABLE IF NOT EXISTS benchmark_rt_results (
    id              BIGSERIAL PRIMARY KEY,
    bench_ts        TIMESTAMPTZ DEFAULT now(),
    run_id          TEXT,
    runner_name     TEXT,
    rmse_eV         DOUBLE PRECISION,
    rmse_threshold  DOUBLE PRECISION DEFAULT 0.05,
    rmse_pass       BOOLEAN,
    within_count    INTEGER,
    within_total    INTEGER,
    within_pass     BOOLEAN,
    ref_updated     BOOLEAN DEFAULT FALSE,
    notes           TEXT
);
""")

# ─── Vérification finale ───────────────────────────────────────────────────
print("\n📊 Tables existantes après setup :")
cur.execute("""
    SELECT table_name, pg_size_pretty(pg_total_relation_size(quote_ident(table_name)))
    FROM information_schema.tables
    WHERE table_schema = 'public'
    ORDER BY table_name;
""")
tables = cur.fetchall()
for name, size in tables:
    print(f"  📋 {name:50s} {size}")

cur.close()
conn.close()
print("\n✅ Setup Supabase terminé — toutes les tables C38 créées/vérifiées.")
