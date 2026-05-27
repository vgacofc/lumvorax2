#!/usr/bin/env python3
"""LumVorax — En-tete commun a tout runner (couleurs + version git).

Usage dans un runner Python:
    from run_header import print_header, ok, warn, err, record
    print_header(cycle="C90", backend="ibm_fez")
    record("Nouveau record CPU", "543 MH/s")
    err("Anomalie detectee", "S(pi) negatif")
"""
import os, sys, subprocess, datetime, socket

GREEN  = "\033[1;32m"
RED    = "\033[1;31m"
YELLOW = "\033[1;33m"
CYAN   = "\033[1;36m"
GRAY   = "\033[0;37m"
BOLD   = "\033[1m"
RESET  = "\033[0m"

def _git(cmd):
    try:
        return subprocess.check_output(["git"] + cmd, cwd=os.path.dirname(os.path.abspath(__file__))+"/..",
                                       stderr=subprocess.DEVNULL).decode().strip()
    except Exception:
        return "?"

def get_version_info():
    return {
        "commit":   _git(["rev-parse", "--short", "HEAD"]),
        "branch":   _git(["rev-parse", "--abbrev-ref", "HEAD"]),
        "date":     _git(["log", "-1", "--format=%ci"]),
        "subject":  _git(["log", "-1", "--format=%s"]),
        "remote":   _git(["config", "--get", "remote.origin.url"]),
        "host":     socket.gethostname(),
        "user":     os.environ.get("USER", "?"),
        "pwd":      os.getcwd(),
        "now_utc":  datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d %H:%M:%SZ"),
    }

def print_header(cycle="?", backend="?", extra=None):
    v = get_version_info()
    print(f"{BOLD}{CYAN}+{'-'*78}+{RESET}", flush=True)
    print(f"{BOLD}{CYAN}| LumVorax  cycle={cycle}  backend={backend}{' '*max(1,40-len(cycle)-len(backend))}|{RESET}", flush=True)
    print(f"{BOLD}{CYAN}+{'-'*78}+{RESET}", flush=True)
    print(f"  {GRAY}commit  :{RESET} {v['commit']}  ({v['branch']})", flush=True)
    print(f"  {GRAY}msg     :{RESET} {v['subject'][:60]}", flush=True)
    print(f"  {GRAY}date    :{RESET} {v['date']}", flush=True)
    print(f"  {GRAY}remote  :{RESET} {v['remote']}", flush=True)
    print(f"  {GRAY}host    :{RESET} {v['host']}  user={v['user']}", flush=True)
    print(f"  {GRAY}now UTC :{RESET} {v['now_utc']}", flush=True)
    if extra:
        for k, val in extra.items():
            print(f"  {GRAY}{k:<8}:{RESET} {val}", flush=True)
    print(f"{BOLD}{CYAN}+{'-'*78}+{RESET}\n", flush=True)
    return v

def ok(tag, msg=""):
    print(f"{GREEN}[OK   ]{RESET} {BOLD}{tag}{RESET}  {msg}", flush=True)

def record(tag, msg=""):
    print(f"{GREEN}{BOLD}[RECORD]{RESET} {GREEN}{BOLD}{tag}{RESET}  {GREEN}{msg}{RESET}", flush=True)

def warn(tag, msg=""):
    print(f"{YELLOW}[WARN ]{RESET} {tag}  {msg}", flush=True)

def err(tag, msg=""):
    print(f"{RED}{BOLD}[ERROR]{RESET} {RED}{tag}{RESET}  {RED}{msg}{RESET}", flush=True)

def info(tag, msg=""):
    print(f"{CYAN}[INFO ]{RESET} {tag}  {msg}", flush=True)

if __name__ == "__main__":
    print_header(cycle="DEMO", backend="local")
    ok("test_passed", "tout va bien")
    record("nouveau record CPU", "543.2 MH/s sur RTX 3090")
    warn("quota IBM faible", "245s restantes")
    err("anomalie detectee", "S(pi) negatif sur ibm_fez")
    info("prochaine etape", "lancer C90 EstimatorV2")
