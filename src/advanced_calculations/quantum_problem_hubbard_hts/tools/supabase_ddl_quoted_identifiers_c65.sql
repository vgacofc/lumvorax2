-- C65 — DDL Supabase avec identifières EXPLICITEMENT cités ("camel" / casse mixte).
-- PostgreSQL sans guillemets : tout est repliable en minuscules → désynchronisation
-- avec le code Python / la doc (t_eV vs t_ev).
--
-- À exécuter dans Supabase → SQL Editor. Adapter ou supprimer les DROP si données à garder.

-- ── problems_config (noms alignés documentation cycle06, colonnes sensibles à la casse)
DROP TABLE IF EXISTS public.problems_config CASCADE;
CREATE TABLE public.problems_config (
    id         SERIAL PRIMARY KEY,
    cycle      TEXT NOT NULL DEFAULT 'cycle06',
    name       TEXT NOT NULL,
    lx         INTEGER NOT NULL,
    ly         INTEGER NOT NULL,
    "t_eV"     DOUBLE PRECISION NOT NULL,
    "u_eV"     DOUBLE PRECISION NOT NULL,
    "mu_eV"    DOUBLE PRECISION NOT NULL DEFAULT 0.0,
    "temp_K"   DOUBLE PRECISION NOT NULL,
    dt         DOUBLE PRECISION NOT NULL,
    steps      BIGINT NOT NULL,
    notes      TEXT,
    created_at TIMESTAMPTZ DEFAULT now(),
    UNIQUE (cycle, name)
);

-- ── benchmark_runtime (REST JSON doit utiliser les mêmes clés que les colonnes)
DROP TABLE IF EXISTS public.benchmark_runtime CASCADE;
CREATE TABLE public.benchmark_runtime (
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
    rmse_global         DOUBLE PRECISION,
    mae_global          DOUBLE PRECISION,
    pct_within_global   DOUBLE PRECISION,
    created_at          TIMESTAMPTZ DEFAULT now(),
    UNIQUE (run_id, dataset, module, observable)
);
CREATE INDEX idx_benchmark_runtime_run_id ON public.benchmark_runtime(run_id);
CREATE INDEX idx_benchmark_runtime_module  ON public.benchmark_runtime(module);

-- Activer RLS + policies selon votre modèle (anon vs service role) — laissé au dashboard habituel.
