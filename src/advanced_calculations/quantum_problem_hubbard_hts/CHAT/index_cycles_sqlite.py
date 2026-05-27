"""
Indexation Sémantique Inter-Cycles — LumVorax (prompt.md percée 1)
===================================================================
Crée/met à jour une base SQLite des métriques cross-runs.
Requêtes disponibles :
  SELECT * FROM runs ORDER BY Tc_K DESC LIMIT 5;
  SELECT run_id, chi_sc_peak FROM runs WHERE N_sweeps >= 4000;
"""
import sqlite3, csv, json, os, sys, glob, re

RESULTS_ROOT = os.path.join(os.path.dirname(__file__), '..', 'results')
DB_PATH      = os.path.join(os.path.dirname(__file__), 'lumvorax_cycles.db')

conn = sqlite3.connect(DB_PATH)
cur  = conn.cursor()

cur.executescript("""
CREATE TABLE IF NOT EXISTS runs (
    run_id       TEXT PRIMARY KEY,
    utc_ts       TEXT, pid INTEGER,
    N_sweeps     INTEGER, N_thermalize INTEGER,
    Tc_K         REAL, Tc_method1_K REAL, Tc_method2_K REAL,
    chi_sc_peak  REAL, pairing_cold_avg REAL,
    E_inf_eV     REAL, thermo_rmse REAL,
    benchmark_pass_rate REAL,
    sweeps_per_sec REAL, site_updates_per_sec REAL,
    total_site_updates REAL, equiv_qubits_avg REAL,
    elapsed_total_s REAL,
    overall_progress_pct REAL,
    n_modules INTEGER,
    inserted_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE IF NOT EXISTS module_metrics (
    run_id TEXT, module TEXT, temp_K REAL,
    E_cold REAL, pairing REAL, chi_sc REAL,
    PRIMARY KEY (run_id, module)
);
CREATE TABLE IF NOT EXISTS benchmark_points (
    run_id TEXT, module TEXT, observable TEXT,
    T REAL, U REAL, reference REAL, model REAL,
    abs_error REAL, rel_error REAL, within_bar INTEGER
);
""")

def parse_run(run_dir):
    run_id = os.path.basename(run_dir)
    m = re.match(r'research_(\d{8}T\d{6}Z)_(\d+)', run_id)
    if not m: return
    utc_ts = m.group(1); pid = int(m.group(2))

    # ── tc_estimation_ptmc.csv
    Tc_K = None; Tc1 = None; Tc2 = None; chi_peak = None
    tc_csv = os.path.join(run_dir, 'tests', 'tc_estimation_ptmc.csv')
    if os.path.exists(tc_csv):
        with open(tc_csv) as f:
            for line in f:
                if line.startswith('# Tc_consensus_K'):
                    parts = line.strip().split(',')
                    try: Tc_K = float(parts[1])
                    except: pass
                elif line.startswith('# Tc_estimate_K'):
                    parts = line.strip().split(',')
                    try: Tc_K = float(parts[1])
                    except: pass
                elif line.startswith('# Tc_method1_dPdT_K'):
                    try: Tc1 = float(line.strip().split(',')[1])
                    except: pass
                elif line.startswith('# Tc_method2_chi_K'):
                    try: Tc2 = float(line.strip().split(',')[1])
                    except: pass
        # chi_sc peak from data rows
        chi_vals = []
        with open(tc_csv) as f:
            for line in f:
                if line.startswith('#') or line.startswith('temp'): continue
                parts = line.strip().split(',')
                if len(parts) >= 5:
                    try: chi_vals.append(float(parts[4]))
                    except: pass
        if chi_vals: chi_peak = max(chi_vals)

    # ── thermodynamic_limit_extrap.csv
    E_inf = None; thermo_rmse = None
    th_csv = os.path.join(run_dir, 'tests', 'thermodynamic_limit_extrap.csv')
    if os.path.exists(th_csv):
        with open(th_csv) as f:
            for line in f:
                if line.startswith('# E_inf_eV'):
                    parts = line.strip().split(',')
                    try:
                        E_inf = float(parts[1])
                        thermo_rmse = float(parts[7]) if len(parts)>7 else None
                    except: pass

    # ── benchmark_comparison_qmc_dmrg.csv
    bench_pass = 0; bench_total = 0
    bc_csv = os.path.join(run_dir, 'tests', 'benchmark_comparison_qmc_dmrg.csv')
    bench_rows = []
    if os.path.exists(bc_csv):
        with open(bc_csv) as f:
            r = csv.DictReader(f)
            for row in r:
                bench_total += 1
                if row.get('within_error_bar','0').strip() == '1': bench_pass += 1
                bench_rows.append(row)
    bench_rate = bench_pass / bench_total if bench_total else None

    # ── pairing_cold_avg from logs
    pair_avg = None
    log_dir = os.path.join(run_dir, 'logs')
    for lf in glob.glob(os.path.join(log_dir, 'lumvorax_*.csv')):
        with open(lf) as f:
            for line in f:
                if 'pairing_cold_avg' in line:
                    m2 = re.search(r'pairing_cold_avg,([0-9eE.+-]+)', line)
                    if m2:
                        try: pair_avg = float(m2.group(1))
                        except: pass
                        break

    # ── performance metrics from logs
    sweeps_ps = None; site_ups_ps = None; total_su = None; q_avg = None; elapsed = None
    for lf in glob.glob(os.path.join(log_dir, 'lumvorax_*.csv')):
        with open(lf) as f:
            for line in f:
                if 'sweeps_per_sec' in line and sweeps_ps is None:
                    m2 = re.search(r'sweeps_per_sec,([0-9eE.+-]+)', line)
                    if m2:
                        try: sweeps_ps = float(m2.group(1))
                        except: pass
                if 'site_updates_per_sec' in line and site_ups_ps is None:
                    m2 = re.search(r'site_updates_per_sec,([0-9eE.+-]+)', line)
                    if m2:
                        try: site_ups_ps = float(m2.group(1))
                        except: pass
                if 'total_site_updates' in line and total_su is None:
                    m2 = re.search(r'total_site_updates,([0-9eE.+-]+)', line)
                    if m2:
                        try: total_su = float(m2.group(1))
                        except: pass
                if 'equiv_qubits' in line and q_avg is None:
                    m2 = re.search(r'equiv_qubits,([0-9eE.+-]+)', line)
                    if m2:
                        try: q_avg = float(m2.group(1))
                        except: pass
                if 'elapsed_ns' in line and 'pt_mc' in line and elapsed is None:
                    m2 = re.search(r'elapsed_ns,([0-9eE.+-]+)', line)
                    if m2:
                        try: elapsed = float(m2.group(1)) * 1e-9
                        except: pass

    # ── analysis summary
    prog = None
    sj = os.path.join(run_dir, 'logs', 'analysis_scientifique_summary.json')
    if os.path.exists(sj):
        with open(sj) as f:
            try:
                d = json.load(f)
                prog = d.get('overall_progress_pct')
            except: pass

    # ── module count
    mm = os.path.join(run_dir, 'tests', 'module_physics_metadata.csv')
    n_mod = 0
    if os.path.exists(mm):
        with open(mm) as f:
            n_mod = sum(1 for l in f if not l.startswith('module') and l.strip())

    cur.execute("""
        INSERT OR REPLACE INTO runs
        (run_id, utc_ts, pid, Tc_K, Tc_method1_K, Tc_method2_K,
         chi_sc_peak, pairing_cold_avg,
         E_inf_eV, thermo_rmse, benchmark_pass_rate,
         sweeps_per_sec, site_updates_per_sec, total_site_updates,
         equiv_qubits_avg, elapsed_total_s, overall_progress_pct, n_modules)
        VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
    """, (run_id, utc_ts, pid, Tc_K, Tc1, Tc2, chi_peak, pair_avg,
          E_inf, thermo_rmse, bench_rate,
          sweeps_ps, site_ups_ps, total_su,
          q_avg, elapsed, prog, n_mod))

    for row in bench_rows:
        try:
            cur.execute("""
                INSERT OR REPLACE INTO benchmark_points
                (run_id, module, observable, T, U, reference, model, abs_error, rel_error, within_bar)
                VALUES (?,?,?,?,?,?,?,?,?,?)
            """, (run_id, row['module'], row['observable'],
                  float(row['T']), float(row['U']),
                  float(row['reference']), float(row['model']),
                  float(row['abs_error']), float(row['rel_error']),
                  int(row['within_error_bar'])))
        except: pass

    conn.commit()
    print(f"[INDEX] {run_id}: Tc={Tc_K}K chi_peak={chi_peak} bench={bench_rate} prog={prog}%")

# Indexer tous les runs
run_dirs = sorted(glob.glob(os.path.join(RESULTS_ROOT, 'research_*')))
for rd in run_dirs:
    parse_run(rd)

# Rapport de synthèse cross-cycles
print(f"\n{'='*65}")
print(f"SYNTHÈSE INTER-CYCLES — LumVorax SQLite")
print(f"{'='*65}")
rows = cur.execute("""
    SELECT run_id, utc_ts, Tc_K, chi_sc_peak, pairing_cold_avg,
           benchmark_pass_rate, sweeps_per_sec, site_updates_per_sec,
           total_site_updates, equiv_qubits_avg, overall_progress_pct
    FROM runs ORDER BY utc_ts DESC LIMIT 10
""").fetchall()
for r in rows:
    print(f"Run: {r[0]}")
    print(f"  Tc={r[2]}K  chi_sc_peak={r[3]}  pairing_avg={r[4]}")
    print(f"  bench_pass={r[5]}  progress={r[10]}%")
    print(f"  sweeps/s={r[6] or 'N/A'}  site_upd/s={r[7] or 'N/A'}")
    su = f"{r[8]:.2e}" if r[8] is not None else "N/A"
    eq = f"{r[9]:.0f}" if r[9] is not None else "N/A"
    print(f"  total_site_updates={su}  equiv_qubits={eq}")
    print()

conn.close()
print(f"DB: {DB_PATH}")
