-- ============================================================
-- SETUP TABLES C50 — LumVorax Quantum Research
-- Conforme STANDARD_NAMES.md v3.4
-- À exécuter dans : Supabase → SQL Editor
-- Auteur : Agent Replit — 2026-04-08
-- ============================================================

-- ── 1. ALTER TABLE module_results_rcs — colonnes C50 ─────────────────────────
ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS n_qubits_c50          BIGINT DEFAULT 6160;
ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS n_phys_qubits_c50     BIGINT DEFAULT 12320;
ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS lx_c50                INTEGER DEFAULT 56;
ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS ly_c50                INTEGER DEFAULT 110;
ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS f_xeb_mean_c50        DOUBLE PRECISION DEFAULT 0.0;
ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS xeb_std_c50           DOUBLE PRECISION DEFAULT 0.0;
ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS xeb_rel_var_c50       DOUBLE PRECISION DEFAULT 0.0;
ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS xeb_converged_c50     BOOLEAN DEFAULT FALSE;
ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS willow_ratio_c50      DOUBLE PRECISION DEFAULT 0.0;
ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS willow_ratio_real_c50 DOUBLE PRECISION DEFAULT 0.0;
ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS beats_willow_c50      BOOLEAN DEFAULT FALSE;
ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS d_eff_log_c50         DOUBLE PRECISION DEFAULT 0.0;
ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS op_init_inv_sqrt_n_c50 DOUBLE PRECISION DEFAULT 0.0;
ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS p_meas_global_c50     DOUBLE PRECISION DEFAULT 0.0;
ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS n_circuits_c50        BIGINT DEFAULT 30000;
ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS fix_grid_applied      BOOLEAN DEFAULT TRUE;
ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS fix_conv_applied      BOOLEAN DEFAULT TRUE;
ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS fix_anom01_applied    BOOLEAN DEFAULT TRUE;
ALTER TABLE module_results_rcs ADD COLUMN IF NOT EXISTS cycle                 TEXT DEFAULT 'C50';

-- ── 2. ALTER TABLE run_scores — colonnes C50 ─────────────────────────────────
ALTER TABLE run_scores ADD COLUMN IF NOT EXISTS cycle             TEXT DEFAULT 'C50';
ALTER TABLE run_scores ADD COLUMN IF NOT EXISTS n_qubits_rcs      BIGINT DEFAULT 6160;
ALTER TABLE run_scores ADD COLUMN IF NOT EXISTS d_eff_log         DOUBLE PRECISION DEFAULT 0.0;
ALTER TABLE run_scores ADD COLUMN IF NOT EXISTS willow_ratio_real DOUBLE PRECISION DEFAULT 0.0;
ALTER TABLE run_scores ADD COLUMN IF NOT EXISTS beats_willow      BOOLEAN DEFAULT FALSE;
ALTER TABLE run_scores ADD COLUMN IF NOT EXISTS xeb_mean_final    DOUBLE PRECISION DEFAULT 0.0;
ALTER TABLE run_scores ADD COLUMN IF NOT EXISTS xeb_converged_c50 BOOLEAN DEFAULT FALSE;

-- ── 3. ALTER TABLE simulation_runs_extended — colonnes C50 ───────────────────
ALTER TABLE simulation_runs_extended ADD COLUMN IF NOT EXISTS willow_ratio_c50   DOUBLE PRECISION DEFAULT 0.0;
ALTER TABLE simulation_runs_extended ADD COLUMN IF NOT EXISTS d_eff_log_c50      DOUBLE PRECISION DEFAULT 0.0;
ALTER TABLE simulation_runs_extended ADD COLUMN IF NOT EXISTS n_qubits_rcs_c50   BIGINT DEFAULT 6160;
ALTER TABLE simulation_runs_extended ADD COLUMN IF NOT EXISTS c50_fixes_applied  TEXT DEFAULT 'GRID,CONV,ANOM01';
ALTER TABLE simulation_runs_extended ADD COLUMN IF NOT EXISTS run_interrupted     BOOLEAN DEFAULT FALSE;
ALTER TABLE simulation_runs_extended ADD COLUMN IF NOT EXISTS reconnect_count     INTEGER DEFAULT 0;

-- ── 4. CREATE TABLE standard_names_registry ───────────────────────────────────
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
CREATE INDEX IF NOT EXISTS idx_snr_section ON standard_names_registry(section);
CREATE INDEX IF NOT EXISTS idx_snr_key     ON standard_names_registry(key_name);

-- ── 5. CREATE TABLE vercel_log_events ─────────────────────────────────────────
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
CREATE INDEX IF NOT EXISTS idx_vle_run           ON vercel_log_events(run_id);
CREATE INDEX IF NOT EXISTS idx_vle_module_metric ON vercel_log_events(module, metric);

-- ── 6. INSERT données de référence STANDARD_NAMES ────────────────────────────
INSERT INTO standard_names_registry (section, key_name, value_desc, file_origin, cycle)
VALUES
    ('MACROS',       'FORENSIC_LOG_MODULE_METRIC', 'Log principal CSV Lumvorax',                    'src/debug/ultra_forensic_logger.h', 'C50'),
    ('MACROS',       'FORENSIC_LOG_ALGO',          'Log métrique algorithme',                       'src/debug/ultra_forensic_logger.h', 'C50'),
    ('METRICS_RCS',  'rcs:F_xeb_mean',             'Fidélité XEB moyenne (cible: +1/3)',            'random_circuit_sampling.c', 'C50'),
    ('METRICS_RCS',  'rcs:n_qubits',               'Nombre de qubits grille (C50: 6160)',           'random_circuit_sampling.c', 'C50'),
    ('METRICS_RCS',  'rcs:converged',              'Convergence XEB (C50-FIX-CONV: seuil 5%)',     'random_circuit_sampling.c', 'C50'),
    ('METRICS_RCS',  'rcs:beats_willow',           'LumVorax > Google Willow (1.0=vrai)',           'random_circuit_sampling.c', 'C50'),
    ('METRICS_RCS',  'rcs:D_eff_log',              'log2 dimension Hilbert effective',               'random_circuit_sampling.c', 'C50'),
    ('CORRECTIONS',  'C50-FIX-GRID',              'lx=56, ly=110 (6160 qubits)',                   'config/problems_cycle06.csv', 'C50'),
    ('CORRECTIONS',  'C50-FIX-CONV',              'XEB_CONVERGENCE_TOL=0.05',                       'random_circuit_sampling.c', 'C50'),
    ('CORRECTIONS',  'C50-FIX-ANOM-01',           'inv_sqrt_n = 1/sqrt(n_qubits)',                 'random_circuit_sampling.c', 'C50'),
    ('TABLES',       'module_results_rcs',         'Résultats RCS par run',                         'SUPABASE/setup_tables_c38.py', 'C50'),
    ('TABLES',       'benchmark_rt_results',       'Benchmarks QMC/DMRG runtime',                   'SUPABASE/setup_tables_c38.py', 'C50'),
    ('TABLES',       'standard_names_registry',    'Registre centralisé des noms canoniques',       'SUPABASE/setup_tables_c50.sql', 'C50'),
    ('TABLES',       'vercel_log_events',          'Bridge Vercel → Supabase pour événements CSV', 'SUPABASE/setup_tables_c50.sql', 'C50')
ON CONFLICT (section, key_name) DO UPDATE
    SET value_desc = EXCLUDED.value_desc,
        cycle = EXCLUDED.cycle,
        updated_at = NOW();

-- ── 7. VÉRIFICATION FINALE ────────────────────────────────────────────────────
SELECT
    'standard_names_registry' AS table_name,
    COUNT(*) AS n_rows
FROM standard_names_registry
UNION ALL
SELECT
    'vercel_log_events',
    COUNT(*)
FROM vercel_log_events;
