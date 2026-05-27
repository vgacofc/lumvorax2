-- LumVorax — Setup Supabase C46 — SQL à exécuter dans SQL Editor Supabase (nouveau compte)
-- Cycle C46 — 2026-04-15
-- Noms de tables/colonnes exacts conformes STANDARD_NAMES.md v4.3 §M-BTC17-C46
--
-- INSTRUCTIONS :
--   1. Ouvrir le SQL Editor du NOUVEAU compte Supabase
--   2. Copier-coller ce fichier en entier
--   3. Cliquer Run
--   4. Vérifier : Table Editor → 11 tables créées
--   5. Ajouter dans secrets Replit :
--        SUPABASE_URL          = https://<project>.supabase.co
--        SUPABASE_ANON_KEY     = eyJ...
--        SUPABASE_SERVICE_ROLE_KEY = eyJ...

-- TABLE 1 : standard_names_registry
CREATE TABLE IF NOT EXISTS standard_names_registry (
    id           SERIAL PRIMARY KEY,
    section      TEXT NOT NULL,
    key_name     TEXT NOT NULL,
    value_desc   TEXT,
    file_origin  TEXT,
    version      TEXT,
    cycle        TEXT DEFAULT 'C46',
    is_canonical BOOLEAN DEFAULT FALSE,
    created_at   TIMESTAMPTZ DEFAULT NOW(),
    UNIQUE (section, key_name)
);

-- TABLE 2 : quantum_benchmarks
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
);

-- TABLE 3 : run_scores
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
);

-- TABLE 4 : quantum_run_files
CREATE TABLE IF NOT EXISTS quantum_run_files (
    id         SERIAL PRIMARY KEY,
    run_id     TEXT,
    path       TEXT,
    size       BIGINT,
    checksum   TEXT,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- TABLE 5 : quantum_csv_rows (schéma réel C53 vérifié)
CREATE TABLE IF NOT EXISTS quantum_csv_rows (
    id       BIGSERIAL PRIMARY KEY,
    run_id   TEXT,
    row_json TEXT NOT NULL
);

-- TABLE 6 : benchmark_runtime (vérifié Supabase 2026-03-30)
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
);

-- TABLE 7 : research_modules_config
CREATE TABLE IF NOT EXISTS research_modules_config (
    id         SERIAL PRIMARY KEY,
    module     TEXT UNIQUE,
    enabled    BOOLEAN DEFAULT TRUE,
    lx         INTEGER,
    ly         INTEGER,
    notes      TEXT,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- TABLE 8 : problems_config (vérifié Supabase 2026-03-30)
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
);

-- TABLE 9 : btc_run_metrics (Module 17 BTC C46)
CREATE TABLE IF NOT EXISTS btc_run_metrics (
    id                 SERIAL PRIMARY KEY,
    run_id             TEXT UNIQUE,
    schema_version     TEXT DEFAULT 'lumvorax_btc_pow_candidate_v1',
    cycle              TEXT DEFAULT 'C46',
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
);

-- TABLE 10 : btc_nx48_ab_benchmarks (Benchmark A/B NX48 C46)
CREATE TABLE IF NOT EXISTS btc_nx48_ab_benchmarks (
    id                    SERIAL PRIMARY KEY,
    schema_version        TEXT DEFAULT 'lumvorax_btc_nx48_ab_benchmark_v1',
    cycle                 TEXT DEFAULT 'C46',
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
);

-- TABLE 11 : artifact_sync_log (Traçabilité sync artefacts)
CREATE TABLE IF NOT EXISTS artifact_sync_log (
    id          SERIAL PRIMARY KEY,
    cycle       TEXT DEFAULT 'C46',
    artifact    TEXT,
    sha256      TEXT,
    destination TEXT,
    status      TEXT,
    details     TEXT,
    created_at  TIMESTAMPTZ DEFAULT NOW()
);

-- Activation Row Level Security
ALTER TABLE standard_names_registry ENABLE ROW LEVEL SECURITY;
ALTER TABLE quantum_benchmarks ENABLE ROW LEVEL SECURITY;
ALTER TABLE run_scores ENABLE ROW LEVEL SECURITY;
ALTER TABLE quantum_run_files ENABLE ROW LEVEL SECURITY;
ALTER TABLE quantum_csv_rows ENABLE ROW LEVEL SECURITY;
ALTER TABLE benchmark_runtime ENABLE ROW LEVEL SECURITY;
ALTER TABLE research_modules_config ENABLE ROW LEVEL SECURITY;
ALTER TABLE problems_config ENABLE ROW LEVEL SECURITY;
ALTER TABLE btc_run_metrics ENABLE ROW LEVEL SECURITY;
ALTER TABLE btc_nx48_ab_benchmarks ENABLE ROW LEVEL SECURITY;
ALTER TABLE artifact_sync_log ENABLE ROW LEVEL SECURITY;

-- Policies (service_role peut tout faire)
CREATE POLICY "service_role_all" ON standard_names_registry FOR ALL TO service_role USING (true) WITH CHECK (true);
CREATE POLICY "service_role_all" ON quantum_benchmarks FOR ALL TO service_role USING (true) WITH CHECK (true);
CREATE POLICY "service_role_all" ON run_scores FOR ALL TO service_role USING (true) WITH CHECK (true);
CREATE POLICY "service_role_all" ON quantum_run_files FOR ALL TO service_role USING (true) WITH CHECK (true);
CREATE POLICY "service_role_all" ON quantum_csv_rows FOR ALL TO service_role USING (true) WITH CHECK (true);
CREATE POLICY "service_role_all" ON benchmark_runtime FOR ALL TO service_role USING (true) WITH CHECK (true);
CREATE POLICY "service_role_all" ON research_modules_config FOR ALL TO service_role USING (true) WITH CHECK (true);
CREATE POLICY "service_role_all" ON problems_config FOR ALL TO service_role USING (true) WITH CHECK (true);
CREATE POLICY "service_role_all" ON btc_run_metrics FOR ALL TO service_role USING (true) WITH CHECK (true);
CREATE POLICY "service_role_all" ON btc_nx48_ab_benchmarks FOR ALL TO service_role USING (true) WITH CHECK (true);
CREATE POLICY "service_role_all" ON artifact_sync_log FOR ALL TO service_role USING (true) WITH CHECK (true);

-- Données initiales C46 — standard_names_registry
INSERT INTO standard_names_registry (section, key_name, value_desc, file_origin, version, cycle, is_canonical) VALUES
('§M-BTC17-C46', 'btc_c46_batch_size_1024',    'Batch SHA-256 1024 hashes/lot — pipeline AVX2 8-way saturé 2× vs C44',  'src/btc_mining_engine.c', 'v4.3', 'C46', TRUE),
('§M-BTC17-C46', 'btc_c46_nx48_update_256k',   'NX48 update toutes 256k hashes — puissance de 2, overhead -22%',          'src/btc_mining_engine.c', 'v4.3', 'C46', TRUE),
('§M-BTC17-C46', 'btc_c46_ts_cache_2ms',        'Cache timestamp 2ms — 2× moins de syscalls clock_gettime Replit NFS',     'src/btc_mining_engine.c', 'v4.3', 'C46', TRUE),
('§M-BTC17-C46', 'btc_c46_supabase_new_account','Nouveau compte Supabase — tables recréées via supabase_c46_setup.sql',    'tools/supabase_c46_setup.sql', 'v4.3', 'C46', TRUE),
('§M-BTC17-C46', 'btc_c46_benchmark_run_id_a',  'Cas A benchmark C46 : btc_20260415T210100Z_1169 — NX48 OFF — 0.43 MH/s', 'logs/forensic/btc_nx48_ab_c46.json', 'v4.3', 'C46', TRUE),
('§M-BTC17-C46', 'btc_c46_benchmark_run_id_b',  'Cas B benchmark C46 : btc_20260415T210105Z_1173 — NX48 ON — 0.43 MH/s, LZ=19', 'logs/forensic/btc_nx48_ab_c46.json', 'v4.3', 'C46', TRUE),
('§M-BTC17-C46', 'btc_c46_binary_sha256',       'SHA-256 binaire C46 : d33be168df62114a56cb070e26938e3e92cd5af56f73c8964fb996eb91e22979 (117K)', 'btc_mining_runner', 'v4.3', 'C46', TRUE),
('§M-BTC17-C46', 'btc_c46_block_height_945230', 'Header Bitcoin bloc 945230 utilisé pour benchmark POW C46',              'scripts/fetch_btc_real_pow.py', 'v4.3', 'C46', TRUE)
ON CONFLICT (section, key_name) DO UPDATE SET value_desc=EXCLUDED.value_desc, cycle=EXCLUDED.cycle;

-- Données initiales C46 — btc_nx48_ab_benchmarks
INSERT INTO btc_nx48_ab_benchmarks (
    schema_version, cycle, created_at_run, duration_s_per_case, threads,
    case_a_nx48_enabled, case_a_leading_zeros, case_a_hashrate_mhs, case_a_returncode,
    case_b_nx48_enabled, case_b_leading_zeros, case_b_hashrate_mhs, case_b_returncode,
    notes
) VALUES (
    'lumvorax_btc_nx48_ab_benchmark_v1', 'C46', '2026-04-15T21:01:00Z', 3, 2,
    FALSE, 0, 0.43, 0,
    TRUE, 19, 0.43, 0,
    'Bloc Bitcoin 945230 — batch=1024 C46 — header hex: 04401b26...64b3'
);

-- Données initiales C46 — btc_run_metrics (Cas B)
INSERT INTO btc_run_metrics (
    run_id, schema_version, cycle, best_leading_zeros, best_nonce,
    hashrate_mhs_final, nx48_neuron_count, metrics_count, anomalies_count,
    duration_s, threads, nx48_enabled, header_hex, notes
) VALUES (
    'btc_20260415T210105Z_1173',
    'lumvorax_btc_pow_candidate_v1',
    'C46',
    19, 0,
    0.43, 2, 1734, 0,
    5.04, 2, TRUE,
    '04401b266e223cec2d387f38c7966beabffcf379fba9a943d06d000000000000000000002304b39d10b242733d8b7d3aa16bc94ef8e4136cce964c86286e32618b9fdcf353f9df6984060217902264b3',
    'Benchmark C46 Cas B — NX48 ON — bloc 945230 mainnet — batch=1024 C46-1'
);

-- Données initiales C46 — artifact_sync_log
INSERT INTO artifact_sync_log (cycle, artifact, sha256, destination, status, details) VALUES
('C46', 'btc_mining_runner',             'd33be168df62114a56cb070e26938e3e92cd5af56f73c8964fb996eb91e22979', 'supabase:artifact_sync_log', 'SYNCED', '117K — compilé 2026-04-15 21:00 UTC — C46 batch=1024'),
('C46', 'btc_nx48_ab_c46.json',          'benchmark_forensic_c46',                                          'supabase:btc_nx48_ab_benchmarks', 'SYNCED', 'cas A=0.43 MH/s LZ=0, cas B=0.43 MH/s LZ=19'),
('C46', 'supabase_c46_setup.sql',        'schema_c46',                                                      'supabase:DDL',                   'DONE',   '11 tables + RLS + 8 entrées standard_names_registry'),
('C46', 'analysechatgpt91.46.md',        'rapport_c46',                                                     'CHAT/',                          'DONE',   'Rapport forensic C46 — format auto-prompt');
