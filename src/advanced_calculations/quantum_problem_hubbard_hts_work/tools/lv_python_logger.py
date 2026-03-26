#!/usr/bin/env python3
"""
lv_python_logger.py — BC-LV04 — Logger Python LumVorax pour phases 8–39
=======================================================================
Implémentation Python native du protocole LumVorax forensique, permettant
aux scripts Python post-run (phases 8–39) d'émettre des événements
directement dans le CSV LumVorax, au même format que le binaire C.

Format CSV LumVorax (identique au C) :
  EVENT_TYPE,ISO_UTC,TIMESTAMP_NS,PID,MODULE:METRIC_NAME,VALUE

Événements supportés (conformes STANDARD_NAMES.md) :
  METRIC   — valeur numérique scalaire
  ANOMALY  — comportement non programmé détecté
  START    — début de module Python
  END      — fin de module Python
  HW       — snapshot hardware CPU/MEM
  SHADOW_MATCH   — C vs Python : résultats cohérents
  SHADOW_DRIFT   — C vs Python : divergence détectée

Usage (dans tout script Python de phase 8–39) :

    from tools.lv_python_logger import LumVoraxPythonLogger as LV

    lv = LV.from_env()          # lit LUMVORAX_CSV_PATH depuis l'env
    with lv.module("mon_script"):
        lv.metric("my_metric", 42.0)
        if bad: lv.anomaly("bad_value", 42.0)

Ou usage direct :

    lv = LV("/path/to/lumvorax.csv")
    lv.metric("phase_12_score", 0.95)
"""
from __future__ import annotations

import os
import sys
import time
import threading
from contextlib import contextmanager
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional


class LumVoraxPythonLogger:
    """Logger Python conforme au protocole LumVorax v3.0."""

    VERSION = "3.0"
    _lock = threading.Lock()

    def __init__(self, csv_path: Optional[str] = None):
        """
        Initialise le logger.

        Args:
            csv_path: Chemin du CSV LumVorax. Si None, utilise
                      LUMVORAX_CSV_PATH depuis l'environnement.
                      Si toujours None, les logs sont écrits sur stderr.
        """
        self.csv_path = csv_path or os.environ.get("LUMVORAX_CSV_PATH")
        self.pid = os.getpid()
        self._events_count = 0
        self._active_module: Optional[str] = None

        if self.csv_path:
            # Vérifie que le fichier est accessible (le C l'a peut-être créé)
            p = Path(self.csv_path)
            if not p.parent.exists():
                p.parent.mkdir(parents=True, exist_ok=True)

    @classmethod
    def from_env(cls) -> "LumVoraxPythonLogger":
        """Crée un logger depuis LUMVORAX_CSV_PATH (variable d'environnement)."""
        return cls(os.environ.get("LUMVORAX_CSV_PATH"))

    def _write(self, event_type: str, module: str, name: str, value: str) -> None:
        """Écrit une ligne dans le CSV LumVorax (thread-safe)."""
        ts_ns = time.time_ns()
        iso = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        line = f"{event_type},{iso},{ts_ns},{self.pid},{module}:{name},{value}\n"

        with self._lock:
            self._events_count += 1
            if self.csv_path:
                try:
                    with open(self.csv_path, "a") as f:
                        f.write(line)
                except OSError as e:
                    print(f"[LV_PY] WARN: impossible d'écrire dans {self.csv_path}: {e}",
                          file=sys.stderr)
            else:
                # Fallback stderr si pas de CSV configuré
                print(f"[LV_PY] {line.strip()}", file=sys.stderr)

    # ── API publique ─────────────────────────────────────────────────────────

    def metric(self, name: str, value: float, module: Optional[str] = None) -> None:
        """Émet un événement METRIC (valeur scalaire)."""
        m = module or self._active_module or "python_phase"
        self._write("METRIC", m, name, f"{value:.10g}")

    def anomaly(self, name: str, value: float, module: Optional[str] = None) -> None:
        """Émet un événement ANOMALY (comportement non programmé)."""
        m = module or self._active_module or "python_phase"
        self._write("ANOMALY", m, f"ANOMALY_{name}", f"{value:.10g}")

    def start(self, module: str, campaign: str = "run") -> None:
        """Émet un événement MODULE_START."""
        self._active_module = module
        self._write("START", module, "MODULE_START", campaign)

    def end(self, module: str, success: bool = True) -> None:
        """Émet un événement MODULE_END."""
        self._write("END", module, "MODULE_END", "true" if success else "false")
        if self._active_module == module:
            self._active_module = None

    def hw_sample(self, module: Optional[str] = None) -> None:
        """Émet un snapshot hardware CPU/MEM (via /proc/self/status)."""
        m = module or self._active_module or "python_phase"
        try:
            cpu = _read_proc_cpu()
            mem = _read_proc_mem_kb()
            self._write("HW", m, "cpu_pct", f"{cpu:.2f}")
            self._write("HW", m, "mem_rss_kb", f"{mem}")
        except Exception:
            pass  # HW sample optionnel

    def shadow_match(self, problem: str, c_value: float, py_value: float) -> None:
        """Émet SHADOW_MATCH : C et Python convergent."""
        self._write("SHADOW_MATCH", "shadow_monitor", f"{problem}:c", f"{c_value:.6f}")
        self._write("SHADOW_MATCH", "shadow_monitor", f"{problem}:py", f"{py_value:.6f}")

    def shadow_drift(self, problem: str, c_value: float, py_value: float,
                     delta: float) -> None:
        """Émet SHADOW_DRIFT : divergence C vs Python."""
        self._write("SHADOW_DRIFT", "shadow_monitor", f"{problem}:delta", f"{delta:.6f}")

    @contextmanager
    def module(self, name: str, campaign: str = "run"):
        """Context manager : START + END automatiques avec gestion d'exceptions."""
        self.start(name, campaign)
        success = True
        try:
            yield self
        except Exception as e:
            success = False
            self.anomaly("python_exception", 1.0, name)
            raise
        finally:
            self.end(name, success)

    @property
    def events_count(self) -> int:
        """Nombre total d'événements émis."""
        return self._events_count

    def __repr__(self) -> str:
        return f"LumVoraxPythonLogger(csv={self.csv_path!r}, events={self._events_count})"


# ── Helpers système ──────────────────────────────────────────────────────────

def _read_proc_cpu() -> float:
    """Lit CPU% via /proc/stat (Linux). Retourne 0.0 si non disponible."""
    try:
        stat1 = Path("/proc/stat").read_text().split("\n")[0].split()[1:]
        time.sleep(0.05)
        stat2 = Path("/proc/stat").read_text().split("\n")[0].split()[1:]
        idle1, total1 = int(stat1[3]), sum(int(x) for x in stat1)
        idle2, total2 = int(stat2[3]), sum(int(x) for x in stat2)
        dt = total2 - total1
        return (1.0 - (idle2 - idle1) / dt) * 100.0 if dt > 0 else 0.0
    except Exception:
        return 0.0


def _read_proc_mem_kb() -> int:
    """Lit RSS en KB depuis /proc/self/status."""
    try:
        for line in Path("/proc/self/status").read_text().split("\n"):
            if line.startswith("VmRSS:"):
                return int(line.split()[1])
    except Exception:
        pass
    return 0


# ── Décorateur utilitaire ────────────────────────────────────────────────────

def lv_phase(phase_num: int, lv: Optional[LumVoraxPythonLogger] = None):
    """
    Décorateur pour instrumenter automatiquement une fonction phase Python.

    Usage :
        @lv_phase(12)
        def phase_12_my_analysis(run_dir):
            ...
    """
    def decorator(fn):
        import functools
        @functools.wraps(fn)
        def wrapper(*args, **kwargs):
            logger = lv or LumVoraxPythonLogger.from_env()
            module_name = f"phase_{phase_num:02d}_{fn.__name__}"
            with logger.module(module_name):
                t0 = time.time_ns()
                result = fn(*args, **kwargs)
                elapsed = time.time_ns() - t0
                logger.metric("elapsed_ns", elapsed, module_name)
                return result
        return wrapper
    return decorator


# ── Singleton global accessible via import ──────────────────────────────────
_global_logger: Optional[LumVoraxPythonLogger] = None

def get_logger() -> LumVoraxPythonLogger:
    """Retourne le logger global (singleton, configuré depuis l'env)."""
    global _global_logger
    if _global_logger is None:
        _global_logger = LumVoraxPythonLogger.from_env()
    return _global_logger


def lv_metric(name: str, value: float, module: str = "python_global") -> None:
    """Raccourci : émet une métrique via le logger global."""
    get_logger().metric(name, value, module)


def lv_anomaly(name: str, value: float, module: str = "python_global") -> None:
    """Raccourci : émet une anomalie via le logger global."""
    get_logger().anomaly(name, value, module)


# ── Main (usage en ligne de commande) ────────────────────────────────────────

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(
        description="LumVorax Python Logger — émission manuelle d'événements")
    parser.add_argument("--csv", default=None, help="Chemin CSV LumVorax")
    parser.add_argument("--event", default="METRIC",
                        choices=["METRIC", "ANOMALY", "START", "END", "HW"],
                        help="Type d'événement")
    parser.add_argument("--module", default="cli", help="Nom du module")
    parser.add_argument("--name", default="test_event", help="Nom de la métrique")
    parser.add_argument("--value", default="1.0", help="Valeur (float)")
    args = parser.parse_args()

    lv = LumVoraxPythonLogger(args.csv)
    v = float(args.value)
    if args.event == "METRIC":
        lv.metric(args.name, v, args.module)
    elif args.event == "ANOMALY":
        lv.anomaly(args.name, v, args.module)
    elif args.event == "START":
        lv.start(args.module)
    elif args.event == "END":
        lv.end(args.module)
    elif args.event == "HW":
        lv.hw_sample(args.module)

    print(f"[LV_PY] {args.event} émis → {lv.csv_path or 'stderr'} "
          f"({lv.events_count} événement(s))", file=sys.stderr)
