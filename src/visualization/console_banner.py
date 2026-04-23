"""
console_banner.py — Bannière de démarrage + helpers couleurs ANSI.

Affiche au démarrage la version courante du dépôt (commit court, date, sujet)
mise à jour automatiquement à chaque run via `git`.

Helpers couleurs :
  - print_record(msg)  -> VERT  (nouveau record CPU/GPU détecté)
  - print_anomaly(msg) -> ROUGE (anomalie / régression / erreur)
  - print_info(msg)    -> CYAN  (info neutre)
  - print_warn(msg)    -> JAUNE (avertissement)

Aucun emoji. Sortie compatible TTY et non-TTY (auto-désactive les codes ANSI
si stdout n'est pas un terminal, sauf override FORCE_COLOR=1).
"""

from __future__ import annotations

import os
import subprocess
import sys
from datetime import datetime
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent

_RESET = "\033[0m"
_BOLD = "\033[1m"
_DIM = "\033[2m"
_GREEN = "\033[32m"
_RED = "\033[31m"
_YELLOW = "\033[33m"
_CYAN = "\033[36m"
_MAGENTA = "\033[35m"


def _ansi_enabled() -> bool:
    if os.environ.get("NO_COLOR"):
        return False
    if os.environ.get("FORCE_COLOR") == "1":
        return True
    return sys.stdout.isatty()


def _c(code: str, text: str) -> str:
    if not _ansi_enabled():
        return text
    return f"{code}{text}{_RESET}"


def _git(args: list[str]) -> str:
    try:
        r = subprocess.run(
            ["git", *args],
            cwd=str(REPO_ROOT),
            capture_output=True,
            text=True,
            timeout=3,
        )
        if r.returncode == 0:
            return r.stdout.strip()
    except Exception:
        pass
    return ""


def get_version_info() -> dict:
    """Retourne les infos de version git du dernier commit (toujours le HEAD courant)."""
    info = {
        "commit_short": _git(["rev-parse", "--short", "HEAD"]) or "unknown",
        "commit_full": _git(["rev-parse", "HEAD"]) or "unknown",
        "branch": _git(["rev-parse", "--abbrev-ref", "HEAD"]) or "unknown",
        "date": _git(["log", "-1", "--format=%ci"]) or "unknown",
        "subject": _git(["log", "-1", "--format=%s"]) or "unknown",
        "author": _git(["log", "-1", "--format=%an"]) or "unknown",
        "dirty": bool(_git(["status", "--porcelain"])),
        "boot_time": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
    }
    return info


def render_banner() -> str:
    v = get_version_info()
    dirty_tag = _c(_YELLOW, " [DIRTY]") if v["dirty"] else ""
    bar = _c(_DIM, "=" * 78)
    title = _c(_BOLD + _CYAN, "LUM/VORAX  -  Visualization Server")
    line_ver = (
        _c(_BOLD, "Version  : ")
        + _c(_GREEN, v["commit_short"])
        + dirty_tag
        + _c(_DIM, f"  ({v['branch']})")
    )
    line_msg = _c(_BOLD, "Last push: ") + _c(_MAGENTA, v["subject"][:70])
    line_when = _c(_BOLD, "Pushed   : ") + _c(_DIM, v["date"]) + _c(_DIM, f"  by {v['author']}")
    line_boot = _c(_BOLD, "Boot     : ") + _c(_DIM, v["boot_time"])
    return "\n".join(["", bar, title, bar, line_ver, line_msg, line_when, line_boot, bar, ""])


def print_banner() -> None:
    print(render_banner(), flush=True)


def print_record(msg: str) -> None:
    """VERT — utiliser à chaque nouveau record CPU/GPU détecté."""
    print(_c(_BOLD + _GREEN, f"[RECORD] {msg}"), flush=True)


def print_anomaly(msg: str) -> None:
    """ROUGE — utiliser pour anomalie / régression / erreur."""
    print(_c(_BOLD + _RED, f"[ANOMALY] {msg}"), flush=True)


def print_info(msg: str) -> None:
    print(_c(_CYAN, f"[INFO] {msg}"), flush=True)


def print_warn(msg: str) -> None:
    print(_c(_YELLOW, f"[WARN] {msg}"), flush=True)


if __name__ == "__main__":
    print_banner()
    print_record("Demo: 1.234 GH/s SHA-256 (CPU AVX-512)")
    print_anomaly("Demo: fidelity 0.87 < target 0.95")
    print_info("Demo: info line")
    print_warn("Demo: warn line")
