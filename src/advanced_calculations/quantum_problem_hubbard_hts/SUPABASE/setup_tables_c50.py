#!/usr/bin/env python3
"""
setup_tables_c50.py — Mise à jour tables Supabase pour LumVorax Cycle C50
Nouvelles métriques identifiées dans analysechatgpt91.8.md

Corrections et optimisations C50 :
  C50-FIX-GRID   : lx=56, ly=110 (6160 qubits) — doublement de la grille RCS
  C50-FIX-CONV   : XEB_CONVERGENCE_TOL = 0.05 (était 0.01) → converged=1 attendu
  C50-FIX-ANOM-01: op_init_inv_sqrt_n ÷ n_qubits (pas n_phys_qubits)
  C51-LABEL-FIX  : distinction n_qubits vs n_phys_qubits dans les colonnes

Tables ALTER-ées :
  - module_results_rcs     : colonnes C50 (n_qubits_c50, xeb_converged_c50, etc.)
  - run_scores             : cycle, n_qubits_rcs
  - simulation_runs_extended : willow_ratio_c50, d_eff_log_c50
  - standard_names_registry (CREATE) : centralisation STANDARD_NAMES.md

Auteur : Agent Replit — Cycle C50 — 2026-04-08
Conforme STANDARD_NAMES.md v3.4 — Section J (tables Supabase).
"""

import os
import sys
import traceback

try:
    import pg8000
except ImportError:
    print("pg8000 non installé. Lancer : pip install pg8000")
    sys.exit(1)

SUPABASE_HOST     = os.getenv("SUPABASE_DB_HOST", "").strip()
SUPABASE_USER     = os.getenv("SUPABASE_DB_USER", "postgres").strip()
SUPABASE_PASSWORD = os.getenv("SUPABASE_DB_PASSWORD", "").strip()
SUPABASE_DB       = os.getenv("SUPABASE_DB_NAME", "postgres").strip()
SUPABASE_PORT     = int(os.getenv("SUPABASE_DB_PORT", "5432"))

if not SUPABASE_HOST:
    print("SUPABASE_DB_HOST non défini — arrêt.")
    sys.exit(1)

print(f"Connexion Supabase : host={SUPABASE_HOST}:{SUPABASE_PORT}, db={SUPABASE_DB}, user={SUPABASE_USER}")

try:
    conn = pg8000.connect(
        host=SUPABASE_HOST,
        port=SUPABASE_PORT,
        database=SUPABASE_DB,
        user=SUPABASE_USER,
        password=SUPABASE_PASSWORD,
        ssl_context=True,
    )
    conn.autocommit = True
    cur = conn.cursor()
    print("Connexion Supabase OK")
except Exception as e:
    print(f"Connexion échouée : {e}")
    traceback.print_exc()
    sys.exit(1)


def exec_sql(label, sql):
    try:
        cur.execute(sql)
        print(f"  OK : {label}")
    except Exception as e:
        print(f"  WARN : {label} : {e}")


# ── 1. ALTER TABLE module_results_rcs — colonnes C50 ─────────────────────────
print("\n--- ALTER TABLE module_results_rcs — colonnes C50...")
rcs_c50_cols = [
    ("n_qubits_c50",              "BIGINT DEFAULT 6160"),
    ("n_phys_qubits_c50",         "BIGINT DEFAULT 12320"),
    ("lx_c50",                    "INTEGER DEFAULT 56"),
    ("ly_c50",                    "INTEGER DEFAULT 110"),
    ("f_xeb_mean_c50",            "DOUBLE PRECISION DEFAULT 0.0"),
    ("xeb_std_c50",               "DOUBLE PRECISION DEFAULT 0.0"),
    ("xeb_rel_var_c50",           "DOUBLE PRECISION DEFAULT 0.0"),
    ("xeb_converged_c50",         "BOOLEAN DEFAULT FALSE"),
    ("willow_ratio_c50",          "DOUBLE PRECISION DEFAULT 0.0"),
    ("willow_ratio_real_c50",     "DOUBLE PRECISION DEFAULT 0.0"),
    ("beats_willow_c50",          "BOOLEAN DEFAULT FALSE"),
    ("d_eff_log_c50",             "DOUBLE PRECISION DEFAULT 0.0"),
    ("op_init_inv_sqrt_n_c50",    "DOUBLE PRECISION DEFAULT 0.0"),
    ("p_meas_global_c50",         "DOUBLE PRECISION DEFAULT 0.0"),
    ("n_circuits_c50",            "BIGINT DEFAULT 30000"),
    ("fix_grid_applied",          "BOOLEAN DEFAULT TRUE"),
    ("fix_conv_applied",          "BOOLEAN DEFAULT TRUE"),
    ("fix_anom01_applied",        "BOOLEAN DEFAULT TRUE"),
    ("cycle",                     "TEXT DEFAULT 'C50'"),
]
for col, typedef in rcs_c50_cols:
    exec_sql(
        f"module_results_rcs.{col}",
        f"ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS {col} {typedef};"
    )

# ── 2. ALTER TABLE run_scores — colonnes C50 ──────────────────────────────────
print("\n--- ALTER TABLE run_scores — colonnes C50...")
score_c50_cols = [
    ("cycle",                     "TEXT DEFAULT 'C50'"),
    ("n_qubits_rcs",              "BIGINT DEFAULT 6160"),
    ("d_eff_log",                 "DOUBLE PRECISION DEFAULT 0.0"),
    ("willow_ratio_real",         "DOUBLE PRECISION DEFAULT 0.0"),
    ("beats_willow",              "BOOLEAN DEFAULT FALSE"),
    ("xeb_mean_final",            "DOUBLE PRECISION DEFAULT 0.0"),
    ("xeb_converged_c50",         "BOOLEAN DEFAULT FALSE"),
]
for col, typedef in score_c50_cols:
    exec_sql(
        f"run_scores.{col}",
        f"ALTER TABLE run_scores ADD COLUMN IF NOT EXISTS {col} {typedef};"
    )

# ── 3. ALTER TABLE simulation_runs_extended — colonnes C50 ───────────────────
print("\n--- ALTER TABLE simulation_runs_extended — colonnes C50...")
sim_c50_cols = [
    ("willow_ratio_c50",          "DOUBLE PRECISION DEFAULT 0.0"),
    ("d_eff_log_c50",             "DOUBLE PRECISION DEFAULT 0.0"),
    ("n_qubits_rcs_c50",          "BIGINT DEFAULT 6160"),
    ("c50_fixes_applied",         "TEXT DEFAULT 'GRID,CONV,ANOM01'"),
    ("run_interrupted",           "BOOLEAN DEFAULT FALSE"),
    ("reconnect_count",           "INTEGER DEFAULT 0"),
]
for col, typedef in sim_c50_cols:
    exec_sql(
        f"simulation_runs_extended.{col}",
        f"ALTER TABLE simulation_runs_extended ADD COLUMN IF NOT EXISTS {col} {typedef};"
    )

# ── 4. CREATE TABLE standard_names_registry ───────────────────────────────────
print("\n--- CREATE TABLE standard_names_registry (centralisation STANDARD_NAMES.md)...")
exec_sql(
    "CREATE standard_names_registry",
    """
    CREATE TABLE IF NOT EXISTS standard_names_registry (
        id           BIGSERIAL PRIMARY KEY,
        section      TEXT NOT NULL,
        key_name     TEXT NOT NULL,
        value_desc   TEXT NOT NULL,
        file_origin  TEXT,
        version      TEXT DEFAULT '3.4',
        cycle        TEXT DEFAULT 'C50',
        is_canonical BOOLEAN DEFAULT TRUE,
        created_at   TIMESTAMPTZ DEFAULT NOW(),
        updated_at   TIMESTAMPTZ DEFAULT NOW(),
        UNIQUE(section, key_name)
    );
    """
)
exec_sql(
    "INDEX standard_names_registry(section)",
    "CREATE INDEX IF NOT EXISTS idx_snr_section ON standard_names_registry(section);"
)
exec_sql(
    "INDEX standard_names_registry(key_name)",
    "CREATE INDEX IF NOT EXISTS idx_snr_key ON standard_names_registry(key_name);"
)

# ── 5. CREATE TABLE vercel_log_events (pour double base) ─────────────────────
print("\n--- CREATE TABLE vercel_log_events (bridge Vercel → Supabase)...")
exec_sql(
    "CREATE vercel_log_events",
    """
    CREATE TABLE IF NOT EXISTS vercel_log_events (
        id           BIGSERIAL PRIMARY KEY,
        run_id       TEXT NOT NULL,
        cycle        TEXT DEFAULT 'C50',
        timestamp_ns BIGINT NOT NULL,
        module       TEXT NOT NULL,
        metric       TEXT NOT NULL,
        value        DOUBLE PRECISION,
        value_text   TEXT,
        event_type   TEXT DEFAULT 'METRIC',
        uploaded_at  TIMESTAMPTZ DEFAULT NOW()
    );
    """
)
exec_sql(
    "INDEX vercel_log_events(run_id)",
    "CREATE INDEX IF NOT EXISTS idx_vle_run ON vercel_log_events(run_id);"
)
exec_sql(
    "INDEX vercel_log_events(module, metric)",
    "CREATE INDEX IF NOT EXISTS idx_vle_module_metric ON vercel_log_events(module, metric);"
)

# ── 6. INSERT données de référence STANDARD_NAMES dans la table ───────────────
print("\n--- INSERT entrées clés dans standard_names_registry...")
key_entries = [
    ("MACROS", "FORENSIC_LOG_MODULE_METRIC", "Log principal CSV Lumvorax", "src/debug/ultra_forensic_logger.h"),
    ("MACROS", "FORENSIC_LOG_ALGO", "Log métrique algorithme", "src/debug/ultra_forensic_logger.h"),
    ("METRICS_RCS", "rcs:F_xeb_mean", "Fidélité XEB moyenne (cible: +1/3)", "random_circuit_sampling.c"),
    ("METRICS_RCS", "rcs:n_qubits", "Nombre de qubits grille (C50: 6160)", "random_circuit_sampling.c"),
    ("METRICS_RCS", "rcs:converged", "Convergence XEB (C50-FIX-CONV: seuil 5%)", "random_circuit_sampling.c"),
    ("METRICS_RCS", "rcs:beats_willow", "LumVorax > Google Willow (1.0=vrai)", "random_circuit_sampling.c"),
    ("METRICS_RCS", "rcs:D_eff_log", "log2 dimension Hilbert effective", "random_circuit_sampling.c"),
    ("CORRECTIONS", "C50-FIX-GRID", "lx=56, ly=110 (6160 qubits)", "config/problems_cycle06.csv"),
    ("CORRECTIONS", "C50-FIX-CONV", "XEB_CONVERGENCE_TOL=0.05", "random_circuit_sampling.c"),
    ("CORRECTIONS", "C50-FIX-ANOM-01", "inv_sqrt_n = 1/sqrt(n_qubits)", "random_circuit_sampling.c"),
    ("TABLES_SUPABASE", "module_results_rcs", "Résultats RCS par run", "SUPABASE/setup_tables_c38.py"),
    ("TABLES_SUPABASE", "benchmark_rt_results", "Benchmarks QMC/DMRG runtime", "SUPABASE/setup_tables_c38.py"),
    ("TABLES_SUPABASE", "standard_names_registry", "Registre centralisé des noms canoniques", "SUPABASE/setup_tables_c50.py"),
]
for section, key, desc, origin in key_entries:
    exec_sql(
        f"INSERT standard_names_registry: {key}",
        f"""
        INSERT INTO standard_names_registry (section, key_name, value_desc, file_origin, cycle)
        VALUES ('{section}', '{key}', '{desc}', '{origin}', 'C50')
        ON CONFLICT (section, key_name) DO UPDATE
          SET value_desc = EXCLUDED.value_desc,
              cycle = EXCLUDED.cycle,
              updated_at = NOW();
        """
    )

print("\nSetup Supabase C50 terminé.")
cur.close()
conn.close()
