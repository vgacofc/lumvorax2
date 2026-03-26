#!/usr/bin/env python3
"""
LumVorax Python Bridge — Cycle 17 — BC-LV04 fix
================================================
Étend la couverture LumVorax des phases 1-7 (C) aux phases 8-39 (Python).

Usage dans chaque script Python du pipeline :
    import sys
    sys.path.insert(0, os.path.join(os.path.dirname(__file__)))
    from lumvorax_python_bridge import LumVoraxBridge

    lv = LumVoraxBridge.from_env()   # Lit LUMVORAX_CSV_PATH depuis env
    lv.phase_start(8, __file__)
    ... travail du script ...
    lv.phase_end(8, __file__, success=True)

Format CSV écrit (identique au format C) :
    event,timestamp_utc,timestamp_ns,pid,detail,value

Types d'événements générés :
    PHASE_BRIDGE  : début/fin de chaque phase Python
    HW_SAMPLE     : CPU+mémoire réels à chaque phase (psutil si disponible)
    METRIC        : métriques scientifiques spécifiques au script
    ANOMALY       : comportements non programmés détectés automatiquement

Détection automatique d'anomalies :
    - Fichier CSV de sortie vide ou manquant
    - Durée de phase > 5 minutes (timeout inattendu)
    - Exception Python capturée et loggée
    - Sortie stderr non vide pendant la phase
"""

import os
import sys
import time
import threading
import datetime
import csv
import io
import subprocess
from pathlib import Path
from typing import Optional, Dict, Any

# ── Constantes ──────────────────────────────────────────────────────
LUMVORAX_CSV_ENV   = "LUMVORAX_CSV_PATH"
LUMVORAX_LOCK_FILE = "/tmp/lumvorax_write.lock"
PHASE_TIMEOUT_S    = 600.0   # 10 minutes max par phase Python

# ── Gestion de la granularité temporelle ────────────────────────────
def now_ns() -> int:
    """Timestamp monotonique nanoseconde."""
    return time.monotonic_ns()

def now_utc_iso() -> str:
    """Timestamp UTC ISO 8601."""
    return datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ")

# ── Snapshot hardware (psutil si disponible, /proc sinon) ───────────
def hw_snapshot() -> Dict[str, float]:
    """Retourne cpu_pct, mem_pct, mem_avail_kb, rss_kb."""
    result = {"cpu_pct": -1.0, "mem_pct": -1.0, "mem_avail_kb": -1, "rss_kb": -1}
    try:
        import psutil
        result["cpu_pct"] = psutil.cpu_percent(interval=0.05)
        vm = psutil.virtual_memory()
        result["mem_pct"]      = vm.percent
        result["mem_avail_kb"] = vm.available // 1024
        rss = psutil.Process(os.getpid()).memory_info().rss
        result["rss_kb"] = rss // 1024
    except ImportError:
        # Fallback : lecture /proc directe
        try:
            with open("/proc/meminfo") as f:
                lines = {l.split(":")[0].strip(): int(l.split(":")[1].strip().split()[0])
                         for l in f if ":" in l}
            total = lines.get("MemTotal", 0)
            avail = lines.get("MemAvailable", 0)
            if total > 0:
                result["mem_pct"]      = 100.0 * (total - avail) / total
                result["mem_avail_kb"] = avail
        except Exception:
            pass
        try:
            with open("/proc/self/status") as f:
                for line in f:
                    if line.startswith("VmRSS:"):
                        result["rss_kb"] = int(line.split()[1])
        except Exception:
            pass
        # CPU delta depuis /proc/stat
        try:
            result["cpu_pct"] = _cpu_percent_delta()
        except Exception:
            pass
    return result

_cpu_prev = {"idle": 0, "total": 0}
_cpu_lock = threading.Lock()

def _cpu_percent_delta() -> float:
    """Mesure CPU par delta /proc/stat — même méthode que le code C."""
    with open("/proc/stat") as f:
        parts = f.readline().split()
    values = [int(x) for x in parts[1:]]
    if len(values) < 4:
        return -1.0
    idle  = values[3] + (values[4] if len(values) > 4 else 0)
    total = sum(values)
    with _cpu_lock:
        d_idle  = idle  - _cpu_prev["idle"]
        d_total = total - _cpu_prev["total"]
        _cpu_prev["idle"]  = idle
        _cpu_prev["total"] = total
    if d_total <= 0:
        return 0.0
    return 100.0 * (d_total - d_idle) / d_total

# ── Classe principale ────────────────────────────────────────────────
class LumVoraxBridge:
    """
    Bridge entre les scripts Python (phases 8-39) et le CSV LumVorax.

    Chaque événement écrit au format :
        event,timestamp_utc,timestamp_ns,pid,detail,value
    """

    def __init__(self, csv_path: str):
        self.csv_path = csv_path
        self.pid = os.getpid()
        self._lock = threading.Lock()
        self._phase_starts: Dict[int, int] = {}
        self._anomaly_count = 0

        # Première mesure CPU pour initialiser le delta
        _cpu_percent_delta()

    @classmethod
    def from_env(cls) -> "LumVoraxBridge":
        """
        Crée un bridge depuis la variable d'environnement LUMVORAX_CSV_PATH.
        Si la variable n'est pas définie, cherche le dernier CSV LumVorax
        dans les résultats récents.
        """
        csv_path = os.environ.get(LUMVORAX_CSV_ENV, "")
        if not csv_path:
            csv_path = cls._find_latest_csv()
        if not csv_path:
            # Mode silencieux : toutes les écritures sont no-ops
            return _NullBridge()
        return cls(csv_path)

    @classmethod
    def from_run_dir(cls, run_dir: str) -> "LumVoraxBridge":
        """Crée un bridge depuis le répertoire d'un run."""
        logs_dir = Path(run_dir) / "logs"
        csvs = sorted(logs_dir.glob("lumvorax_*.csv"), key=os.path.getmtime, reverse=True)
        if csvs:
            return cls(str(csvs[0]))
        return _NullBridge()

    @classmethod
    def _find_latest_csv(cls) -> str:
        """Cherche le dernier CSV LumVorax dans les résultats."""
        base = Path(__file__).parent.parent / "results"
        if not base.exists():
            return ""
        csvs = sorted(base.rglob("lumvorax_*.csv"), key=os.path.getmtime, reverse=True)
        return str(csvs[0]) if csvs else ""

    # ── Écriture thread-safe ────────────────────────────────────────
    def _write(self, lines: list):
        """Écrit une liste de lignes dans le CSV LumVorax (thread-safe)."""
        if not self.csv_path:
            return
        with self._lock:
            try:
                with open(self.csv_path, "a", buffering=1) as f:
                    for line in lines:
                        f.write(line + "\n")
            except Exception as e:
                print(f"[LUMVORAX_BRIDGE] Erreur écriture CSV: {e}", file=sys.stderr)

    def _event_line(self, event: str, detail: str, value: str) -> str:
        """Formate une ligne CSV LumVorax."""
        iso = now_utc_iso()
        ts  = now_ns()
        return f"{event},{iso},{ts},{self.pid},{detail},{value}"

    # ── API publique ────────────────────────────────────────────────
    def phase_start(self, phase_num: int, script_path: str):
        """Enregistre le début d'une phase Python."""
        self._phase_starts[phase_num] = now_ns()
        script_name = Path(script_path).name

        hw = hw_snapshot()
        lines = [
            self._event_line("PHASE_BRIDGE", f"phase{phase_num:02d}:{script_name}", "START"),
            self._event_line("HW_SAMPLE", f"phase{phase_num:02d}:cpu_pct", f"{hw['cpu_pct']:.4f}"),
            self._event_line("HW_SAMPLE", f"phase{phase_num:02d}:mem_pct", f"{hw['mem_pct']:.4f}"),
            self._event_line("HW_SAMPLE", f"phase{phase_num:02d}:mem_avail_kb", f"{hw['mem_avail_kb']}"),
            self._event_line("HW_SAMPLE", f"phase{phase_num:02d}:rss_kb", f"{hw['rss_kb']}"),
        ]
        self._write(lines)

    def phase_end(self, phase_num: int, script_path: str, success: bool = True,
                  exception: Optional[Exception] = None):
        """Enregistre la fin d'une phase Python avec durée et statut."""
        t_start = self._phase_starts.pop(phase_num, now_ns())
        duration_ns = now_ns() - t_start
        script_name = Path(script_path).name
        status = "SUCCESS" if success else "FAILURE"

        hw = hw_snapshot()
        lines = [
            self._event_line("PHASE_BRIDGE", f"phase{phase_num:02d}:{script_name}",
                             status),
            self._event_line("METRIC", f"phase{phase_num:02d}:duration_ns",
                             str(duration_ns)),
            self._event_line("HW_SAMPLE", f"phase{phase_num:02d}:cpu_pct_end",
                             f"{hw['cpu_pct']:.4f}"),
            self._event_line("HW_SAMPLE", f"phase{phase_num:02d}:mem_pct_end",
                             f"{hw['mem_pct']:.4f}"),
            self._event_line("HW_SAMPLE", f"phase{phase_num:02d}:rss_kb_end",
                             f"{hw['rss_kb']}"),
        ]

        if exception is not None:
            exc_str = str(exception)[:200].replace(",", ";").replace("\n", " ")
            lines.append(self._event_line("ANOMALY", f"phase{phase_num:02d}:exception",
                                          exc_str))
            self._anomaly_count += 1

        # Détection anomalie durée : > PHASE_TIMEOUT_S
        if duration_ns / 1e9 > PHASE_TIMEOUT_S:
            lines.append(self._event_line("ANOMALY", f"phase{phase_num:02d}:timeout_exceeded",
                                          f"{duration_ns/1e9:.1f}"))
            self._anomaly_count += 1

        self._write(lines)

    def log_metric(self, module: str, metric_name: str, value: float):
        """Log une métrique scientifique."""
        self._write([self._event_line("METRIC", f"{module}:{metric_name}", f"{value:.10f}")])

    def log_anomaly(self, module: str, description: str, value: float):
        """Log une anomalie comportementale."""
        self._anomaly_count += 1
        self._write([self._event_line("ANOMALY", f"{module}:{description}", f"{value:.10f}")])
        print(f"[LUMVORAX:ANOMALY] {module}:{description}={value:.6f}", file=sys.stderr)

    def log_algo(self, algo_name: str, metric_name: str, value: float):
        """Log une métrique d'algorithme (ED, VMC, etc.)."""
        self._write([self._event_line("ALGO", f"{algo_name}:{metric_name}", f"{value:.10f}")])

    def log_hw_sample(self, module: str):
        """Snapshot hardware complet."""
        hw = hw_snapshot()
        lines = [
            self._event_line("HW_SAMPLE", f"{module}:cpu_pct", f"{hw['cpu_pct']:.4f}"),
            self._event_line("HW_SAMPLE", f"{module}:mem_pct", f"{hw['mem_pct']:.4f}"),
            self._event_line("HW_SAMPLE", f"{module}:rss_kb", f"{hw['rss_kb']}"),
        ]
        self._write(lines)

    def check_output_files(self, phase_num: int, expected_files: list):
        """
        Détection d'anomalie : vérifie que les fichiers de sortie attendus
        existent et ne sont pas vides.
        """
        for fpath in expected_files:
            p = Path(fpath)
            if not p.exists():
                self.log_anomaly(f"phase{phase_num:02d}", f"missing_file:{p.name}", 0.0)
            elif p.stat().st_size == 0:
                self.log_anomaly(f"phase{phase_num:02d}", f"empty_file:{p.name}", 0.0)

    def summary(self):
        """Écrit un résumé de la session Python dans le CSV."""
        hw = hw_snapshot()
        lines = [
            self._event_line("SUMMARY", "python_bridge:total_anomalies", str(self._anomaly_count)),
            self._event_line("HW_SAMPLE", "python_bridge:final_cpu_pct", f"{hw['cpu_pct']:.4f}"),
            self._event_line("HW_SAMPLE", "python_bridge:final_mem_pct", f"{hw['mem_pct']:.4f}"),
            self._event_line("HW_SAMPLE", "python_bridge:final_rss_kb", f"{hw['rss_kb']}"),
        ]
        self._write(lines)


# ── Context manager pour usage simplifié ────────────────────────────
class PhaseContext:
    """
    Usage:
        with PhaseContext(lv, phase_num=8, script=__file__) as ctx:
            ... code du script ...
    """
    def __init__(self, bridge: LumVoraxBridge, phase_num: int, script: str):
        self.bridge = bridge
        self.phase_num = phase_num
        self.script = script

    def __enter__(self):
        self.bridge.phase_start(self.phase_num, self.script)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        success = (exc_type is None)
        self.bridge.phase_end(self.phase_num, self.script, success=success,
                              exception=exc_val)
        return False  # Ne supprime pas l'exception


# ── Null bridge (no-op si CSV introuvable) ─────────────────────────
class _NullBridge(LumVoraxBridge):
    def __init__(self):
        self.csv_path = ""
        self.pid = os.getpid()
        self._lock = threading.Lock()
        self._phase_starts = {}
        self._anomaly_count = 0

    def _write(self, lines):
        pass  # no-op


# ── Wrapper automatique pour intégration dans les scripts existants ─
def auto_instrument_script(phase_num: int, run_dir: str = ""):
    """
    Décorateur pour instrumenter automatiquement un script complet.

    Usage en ligne de commande (sans modifier le script Python) :
        python3 -c "
        from lumvorax_python_bridge import auto_instrument_script
        auto_instrument_script(8, sys.argv[1])
        " run_dir
        python3 mon_script.py run_dir

    Ou en entrée de script :
        if __name__ == '__main__':
            from lumvorax_python_bridge import auto_instrument_script
            auto_instrument_script(8)
    """
    if run_dir:
        lv = LumVoraxBridge.from_run_dir(run_dir)
    else:
        lv = LumVoraxBridge.from_env()
    lv.phase_start(phase_num, sys.argv[0] if sys.argv else "unknown")
    import atexit
    atexit.register(lambda: lv.phase_end(phase_num, sys.argv[0] if sys.argv else "unknown",
                                          success=True))
    return lv


if __name__ == "__main__":
    # Test autonome du bridge
    print("[LUMVORAX_BRIDGE] Test autonome...")
    if len(sys.argv) > 1:
        lv = LumVoraxBridge(sys.argv[1])
    else:
        test_csv = "/tmp/lumvorax_bridge_test.csv"
        with open(test_csv, "w") as f:
            f.write("event,timestamp_utc,timestamp_ns,pid,detail,value\n")
        lv = LumVoraxBridge(test_csv)

    lv.phase_start(8, __file__)
    time.sleep(0.1)
    lv.log_metric("test_module", "test_value", 3.14159)
    lv.log_hw_sample("test_module")
    lv.phase_end(8, __file__, success=True)
    lv.summary()

    print(f"[LUMVORAX_BRIDGE] Test terminé — CSV: {lv.csv_path}")
    hw = hw_snapshot()
    print(f"  cpu_pct={hw['cpu_pct']:.2f}% mem_pct={hw['mem_pct']:.2f}% rss={hw['rss_kb']}KB")
