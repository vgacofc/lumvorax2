-- C65/C37 — DDL Supabase conforme STANDARD_NAMES.md Section J
-- Colonnes en MINUSCULES (standard PostgreSQL sans guillemets).
-- Le code Python (upload_to_supabase.py, supabase_client.py) utilise ces noms exacts.
-- À exécuter dans Supabase → SQL Editor (coller et exécuter).
-- IMPORTANT : supprimer les DROP si vous voulez conserver les données existantes.

-- ════════════════════════════════════════════════════════════════════════════
-- TABLE : quantum_realtime_logs (STANDARD_NAMES.md Section J — NOM D'ORIGINE)
-- Logs temps réel envoyés depuis run_research_cycle.sh via supabase_realtime_streamer.py
-- ════════════════════════════════════════════════════════════════════════════
CREATE TABLE IF NOT EXISTS public.quantum_realtime_logs (
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
CREATE INDEX IF NOT EXISTS idx_rtlogs_run_id ON public.quantum_realtime_logs(run_id);
CREATE INDEX IF NOT EXISTS idx_rtlogs_module  ON public.quantum_realtime_logs(module);
CREATE INDEX IF NOT EXISTS idx_rtlogs_metric  ON public.quantum_realtime_logs(metric);
GRANT ALL ON public.quantum_realtime_logs TO postgres, anon, authenticated, service_role;
GRANT ALL ON SEQUENCE public.quantum_realtime_logs_id_seq TO postgres, anon, authenticated, service_role;

-- ════════════════════════════════════════════════════════════════════════════
-- TABLE : problems_config (STANDARD_NAMES.md Section J — NOM D'ORIGINE)
-- Colonnes MINUSCULES — cohérent avec upload_to_supabase.py
-- ════════════════════════════════════════════════════════════════════════════
CREATE TABLE IF NOT EXISTS public.problems_config (
    id         SERIAL PRIMARY KEY,
    name       TEXT NOT NULL UNIQUE,
    lx         INTEGER,
    ly         INTEGER,
    t_ev       DOUBLE PRECISION,
    u_ev       DOUBLE PRECISION,
    mu_ev      DOUBLE PRECISION DEFAULT 0.0,
    temp_k     DOUBLE PRECISION,
    dt         DOUBLE PRECISION,
    steps      BIGINT,
    enabled    BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMPTZ DEFAULT now()
);
GRANT ALL ON public.problems_config TO postgres, anon, authenticated, service_role;

-- ════════════════════════════════════════════════════════════════════════════
-- TABLE : research_modules_config (STANDARD_NAMES.md Section J — NOM D'ORIGINE)
-- ════════════════════════════════════════════════════════════════════════════
CREATE TABLE IF NOT EXISTS public.research_modules_config (
    id         SERIAL PRIMARY KEY,
    module     TEXT NOT NULL,
    lx         INTEGER,
    ly         INTEGER,
    t_ev       DOUBLE PRECISION,
    u_ev       DOUBLE PRECISION,
    mu_ev      DOUBLE PRECISION DEFAULT 0.0,
    temp_k     DOUBLE PRECISION,
    dt         DOUBLE PRECISION,
    steps      BIGINT,
    created_at TIMESTAMPTZ DEFAULT now()
);
GRANT ALL ON public.research_modules_config TO postgres, anon, authenticated, service_role;

-- ════════════════════════════════════════════════════════════════════════════
-- TABLE : benchmark_runtime (STANDARD_NAMES.md Section J — NOM D'ORIGINE)
-- Résultats de benchmark par run et par module (temps réel C68)
-- ════════════════════════════════════════════════════════════════════════════
CREATE TABLE IF NOT EXISTS public.benchmark_runtime (
    id                  SERIAL PRIMARY KEY,
    run_id              TEXT NOT NULL,
    dataset             TEXT NOT NULL,
    module              TEXT NOT NULL,
    observable          TEXT NOT NULL,
    t_k                 DOUBLE PRECISION,
    u_over_t            DOUBLE PRECISION,
    reference_value     DOUBLE PRECISION,
    error_bar           DOUBLE PRECISION,
    model_value         DOUBLE PRECISION,
    abs_error           DOUBLE PRECISION,
    rel_error           DOUBLE PRECISION,
    within_error_bar    BOOLEAN,
    created_at          TIMESTAMPTZ DEFAULT now(),
    UNIQUE (run_id, dataset, module, observable)
);
CREATE INDEX IF NOT EXISTS idx_benchmark_runtime_run_id ON public.benchmark_runtime(run_id);
CREATE INDEX IF NOT EXISTS idx_benchmark_runtime_module  ON public.benchmark_runtime(module);
GRANT ALL ON public.benchmark_runtime TO postgres, anon, authenticated, service_role;

-- ════════════════════════════════════════════════════════════════════════════
-- TABLE : quantum_benchmarks (STANDARD_NAMES.md Section J — NOM D'ORIGINE)
-- Références QMC/DMRG — source de vérité benchmark
-- ════════════════════════════════════════════════════════════════════════════
CREATE TABLE IF NOT EXISTS public.quantum_benchmarks (
    id               SERIAL PRIMARY KEY,
    dataset          TEXT NOT NULL,
    module           TEXT NOT NULL,
    observable       TEXT NOT NULL,
    t_k              DOUBLE PRECISION,
    u_over_t         DOUBLE PRECISION,
    reference_value  DOUBLE PRECISION,
    reference_method TEXT,
    source           TEXT,
    error_bar        DOUBLE PRECISION,
    notes            TEXT,
    created_at       TIMESTAMPTZ DEFAULT now()
);
GRANT ALL ON public.quantum_benchmarks TO postgres, anon, authenticated, service_role;

-- ════════════════════════════════════════════════════════════════════════════
-- TABLE : quantum_run_files (STANDARD_NAMES.md Section J — NOM D'ORIGINE)
-- ════════════════════════════════════════════════════════════════════════════
CREATE TABLE IF NOT EXISTS public.quantum_run_files (
    id          SERIAL PRIMARY KEY,
    run_id      TEXT NOT NULL,
    module      TEXT,
    lx          INTEGER,
    ly          INTEGER,
    t_ev        DOUBLE PRECISION,
    u_ev        DOUBLE PRECISION,
    mu_ev       DOUBLE PRECISION,
    temp_k      DOUBLE PRECISION,
    dt          DOUBLE PRECISION,
    steps       BIGINT,
    energy      DOUBLE PRECISION,
    pairing     DOUBLE PRECISION,
    sign_ratio  DOUBLE PRECISION,
    cpu_percent DOUBLE PRECISION,
    ram_percent DOUBLE PRECISION,
    created_at  TIMESTAMPTZ DEFAULT now()
);
GRANT ALL ON public.quantum_run_files TO postgres, anon, authenticated, service_role;

-- ════════════════════════════════════════════════════════════════════════════
-- TABLE : quantum_csv_rows (STANDARD_NAMES.md Section J — NOM D'ORIGINE)
-- ════════════════════════════════════════════════════════════════════════════
CREATE TABLE IF NOT EXISTS public.quantum_csv_rows (
    id         BIGSERIAL PRIMARY KEY,
    run_id     TEXT NOT NULL,
    file_name  TEXT,
    row_number INTEGER,
    data       JSONB,
    created_at TIMESTAMPTZ DEFAULT now()
);
GRANT ALL ON public.quantum_csv_rows TO postgres, anon, authenticated, service_role;
GRANT ALL ON SEQUENCE public.quantum_csv_rows_id_seq TO postgres, anon, authenticated, service_role;

-- ════════════════════════════════════════════════════════════════════════════
-- TABLE : run_scores (STANDARD_NAMES.md Section J — NOM D'ORIGINE)
-- ════════════════════════════════════════════════════════════════════════════
CREATE TABLE IF NOT EXISTS public.run_scores (
    id           SERIAL PRIMARY KEY,
    run_id       TEXT NOT NULL UNIQUE,
    runner       TEXT,
    score_iso    INTEGER,
    score_trace  INTEGER,
    score_repr   INTEGER,
    score_robust INTEGER,
    score_phys   INTEGER,
    score_expert INTEGER,
    score_total  INTEGER,
    notes        TEXT,
    created_at   TIMESTAMPTZ DEFAULT now()
);
GRANT ALL ON public.run_scores TO postgres, anon, authenticated, service_role;

-- ════════════════════════════════════════════════════════════════════════════
-- NOTE INCOHÉRENCE C65 RÉSOLUE (C37)
-- L'ancien DDL C65 utilisait "t_eV", "u_eV", "mu_eV", "temp_K" entre guillemets
-- pour problems_config. Ces colonnes camelCase étaient INCOHÉRENTES avec :
--   - upload_to_supabase.py qui envoie t_ev, u_ev, mu_ev, temp_k (minuscules)
--   - STANDARD_NAMES.md Section J qui documente t_ev, u_ev, temp_k
-- Ce DDL corrigé utilise uniformément des colonnes MINUSCULES.
-- Si l'ancienne table problems_config avait des colonnes camelCase,
-- exécuter d'abord : ALTER TABLE problems_config RENAME COLUMN "t_eV" TO t_ev; etc.
-- ════════════════════════════════════════════════════════════════════════════
