#!/usr/bin/env python3
"""
setup_tables_c48.py — Mise à jour tables Supabase pour LumVorax Cycle C48
Nouvelles métriques identifiées dans analysechatgpt91.1.md + analyses ChatGPT attached.

Corrections et optimisations C48 :
  Bug #5         : ed_benchmark_energy_within (convention énergie par site vs totale)
  C48-OPT-MOTT   : détection précoce Mott (Worm-MC)
  C48-OPT-DMFT   : facteur de correction local post-MF (local_corr_factor)
  C48-OPT-NOISE  : bruit physique réaliste (décoherence T2)
  C48-OPT-CIRC   : n_circuits=10000 pour convergence XEB
  C48-TC-ULTRA   : scan Tc à 0.1K entre 64-70K

Tables ALTER-ées :
  - module_results_rcs         : local_corr_factor, realisme_score_est, noise_physical_eV,
                                  n_circuits_c48, f_xeb_post_mf, dmft_correction_applied
  - benchmark_rt_results       : ed_benchmark_energy_within_c48, ed_ref_site_eV, ed_mod_site_eV
  - tc_scan_results (CREATE)   : résultats scan 0.1K Tc (C48-TC-ULTRA)
  - run_scores                 : realisme_score, dmft_local_corr, mott_early_exit_count,
                                  noise_decoher_eV, n_circuits_rcs, f_xeb_converged

Auteur : Agent Replit — Cycle C48 — 2026-04-07
Conforme STANDARD_NAMES.md v3.3 — Section J (tables Supabase).
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

if not SUPABASE_HOST:
    print("SUPABASE_DB_HOST non défini — arrêt.")
    sys.exit(1)

print(f"Connexion Supabase : host={SUPABASE_HOST}, db={SUPABASE_DB}, user={SUPABASE_USER}")

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


# ── 1. ALTER TABLE module_results_rcs — nouvelles colonnes C48 ───────────────
print("\n--- ALTER TABLE module_results_rcs — colonnes C48-OPT-DMFT...")
rcs_cols = [
    ("local_corr_factor_c48",     "DOUBLE PRECISION DEFAULT 1.0"),
    ("realisme_score_est",        "DOUBLE PRECISION DEFAULT 35.0"),
    ("noise_physical_eV",         "DOUBLE PRECISION DEFAULT 0.0"),
    ("noise_thermal_eV",          "DOUBLE PRECISION DEFAULT 0.0"),
    ("noise_decoher_eV",          "DOUBLE PRECISION DEFAULT 0.0"),
    ("T2_rate_eV_per_layer",      "DOUBLE PRECISION DEFAULT 0.0005"),
    ("n_circuits_c48",            "BIGINT DEFAULT 10000"),
    ("f_xeb_post_mf",             "DOUBLE PRECISION DEFAULT 0.0"),
    ("dmft_correction_applied",   "BOOLEAN DEFAULT FALSE"),
    ("u_over_t",                  "DOUBLE PRECISION DEFAULT 8.0"),
    ("alpha_dmft",                "DOUBLE PRECISION DEFAULT 0.12"),
    ("f_xeb_plateau_broken",      "BOOLEAN DEFAULT FALSE"),
    ("xeb_converged_c48",         "BOOLEAN DEFAULT FALSE"),
]
for col, typedef in rcs_cols:
    exec_sql(
        f"module_results_rcs.{col}",
        f"ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS {col} {typedef};"
    )

# ── 2. ALTER TABLE benchmark_rt_results — fix ed_benchmark C48 ───────────────
print("\n--- ALTER TABLE benchmark_rt_results — colonnes C48-FIX-ED-BENCH...")
bench_cols = [
    ("ed_benchmark_energy_within_c48", "INTEGER DEFAULT 0"),
    ("ed_ref_site_eV",                 "DOUBLE PRECISION DEFAULT 0.0"),
    ("ed_mod_site_eV",                 "DOUBLE PRECISION DEFAULT 0.0"),
    ("ed_ebar_site_eV",                "DOUBLE PRECISION DEFAULT 0.0"),
    ("ed_abs_error_site",              "DOUBLE PRECISION DEFAULT 0.0"),
    ("ed_c48_fix_applied",             "BOOLEAN DEFAULT FALSE"),
]
for col, typedef in bench_cols:
    exec_sql(
        f"benchmark_rt_results.{col}",
        f"ALTER TABLE benchmark_rt_results ADD COLUMN IF NOT EXISTS {col} {typedef};"
    )

# ── 3. CREATE TABLE tc_scan_results (C48-TC-ULTRA) ───────────────────────────
print("\n--- CREATE TABLE tc_scan_results (C48-TC-ULTRA 0.1K)...")
exec_sql(
    "CREATE tc_scan_results",
    """
    CREATE TABLE IF NOT EXISTS tc_scan_results (
        id              BIGSERIAL PRIMARY KEY,
        run_id          TEXT NOT NULL,
        cycle           TEXT NOT NULL DEFAULT 'C48',
        temp_K          DOUBLE PRECISION NOT NULL,
        E_cold_eV       DOUBLE PRECISION,
        pairing_cold    DOUBLE PRECISION,
        chi_sc          DOUBLE PRECISION,
        dpairing_dT     DOUBLE PRECISION,
        is_tc_candidate BOOLEAN DEFAULT FALSE,
        resolution_K    DOUBLE PRECISION DEFAULT 0.1,
        scan_zone       TEXT,
        created_at      TIMESTAMPTZ DEFAULT NOW()
    );
    """
)
exec_sql(
    "INDEX tc_scan_results(run_id)",
    "CREATE INDEX IF NOT EXISTS idx_tc_scan_run ON tc_scan_results(run_id);"
)
exec_sql(
    "INDEX tc_scan_results(temp_K)",
    "CREATE INDEX IF NOT EXISTS idx_tc_scan_temp ON tc_scan_results(temp_K);"
)

# ── 4. ALTER TABLE run_scores — nouvelles métriques C48 ──────────────────────
print("\n--- ALTER TABLE run_scores — colonnes C48...")
score_cols = [
    ("realisme_score",         "DOUBLE PRECISION DEFAULT 35.0"),
    ("dmft_local_corr",        "DOUBLE PRECISION DEFAULT 1.0"),
    ("mott_early_exit_count",  "INTEGER DEFAULT 0"),
    ("noise_decoher_eV",       "DOUBLE PRECISION DEFAULT 0.0"),
    ("n_circuits_rcs",         "BIGINT DEFAULT 10000"),
    ("f_xeb_converged",        "BOOLEAN DEFAULT FALSE"),
    ("tc_resolution_K",        "DOUBLE PRECISION DEFAULT 0.1"),
    ("tc_scan_points",         "INTEGER DEFAULT 84"),
    ("ed_bench_c48_fixed",     "BOOLEAN DEFAULT FALSE"),
]
for col, typedef in score_cols:
    exec_sql(
        f"run_scores.{col}",
        f"ALTER TABLE run_scores ADD COLUMN IF NOT EXISTS {col} {typedef};"
    )

# ── 5. ALTER TABLE quantum_run_files — colonnes C48 ──────────────────────────
print("\n--- ALTER TABLE quantum_run_files — colonnes C48...")
qrf_cols = [
    ("c48_corrections_applied",  "TEXT DEFAULT ''"),
    ("realisme_score_c48",       "DOUBLE PRECISION DEFAULT 35.0"),
    ("local_corr_factor_c48",    "DOUBLE PRECISION DEFAULT 1.0"),
    ("n_circuits_rcs",           "BIGINT DEFAULT 10000"),
    ("tc_scan_n_points",         "INTEGER DEFAULT 84"),
    ("ed_bench_fix_c48",         "BOOLEAN DEFAULT FALSE"),
]
for col, typedef in qrf_cols:
    exec_sql(
        f"quantum_run_files.{col}",
        f"ALTER TABLE quantum_run_files ADD COLUMN IF NOT EXISTS {col} {typedef};"
    )

# ── 6. ALTER TABLE module_results_worm_mc — C48-OPT-MOTT ────────────────────
print("\n--- ALTER TABLE module_results_worm_mc — colonnes C48-OPT-MOTT...")
worm_cols = [
    ("mott_early_exit",          "BOOLEAN DEFAULT FALSE"),
    ("mott_proposals_saved",     "BIGINT DEFAULT 0"),
    ("mott_detect_window",       "INTEGER DEFAULT 500"),
    ("mott_accept_threshold",    "DOUBLE PRECISION DEFAULT 0.001"),
]
for col, typedef in worm_cols:
    exec_sql(
        f"module_results_worm_mc.{col}",
        f"ALTER TABLE module_results_worm_mc ADD COLUMN IF NOT EXISTS {col} {typedef};"
    )

print("\nSetup Supabase C48 terminé.")
cur.close()
conn.close()
