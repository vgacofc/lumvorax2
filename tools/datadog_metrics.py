"""
LumVorax C46 — Datadog Metrics Integration
Envoie les métriques forensic BTC/NX48 vers Datadog (API v2).
Usage : python tools/datadog_metrics.py [--run-id <id>] [--cycle <C46>]

C46 : DD_SITE corrigé → datadoghq.eu (site EU)
      DD_API_KEY lu depuis env DD_API_KEY (secret GitHub Actions → Doppler → Replit)
"""

import os
import sys
import json
import time
import argparse
import urllib.request
import urllib.error
import logging

logging.basicConfig(level=logging.INFO, format="%(asctime)s [DATADOG] %(message)s")
log = logging.getLogger("datadog_metrics")

# C46 : DD_API_KEY depuis env (GitHub Actions → Doppler → Replit)
# DD_APP_KEY = Application Key (ddapp_...) — NON utilisée pour les métriques
# DD_API_KEY = 32 hex chars — requis pour POST /api/v2/series
DD_API_TOKEN = (
    os.environ.get("DD_API_KEY", "") or
    os.environ.get("DATADOG_API_KEY", "") or
    os.environ.get("DATADOG_API_TOKEN", "")
)
# C46 : Site EU — datadoghq.eu (corrigé depuis datadoghq.com C44/C45)
DD_SITE = os.environ.get("DD_SITE", os.environ.get("DATADOG_SITE", "datadoghq.eu"))
DD_METRICS_URL = f"https://api.{DD_SITE}/api/v2/series"

# C46 : Configuration RUM Datadog
DD_RUM_APPLICATION_ID = os.environ.get("DD_RUM_APPLICATION_ID", "bb33a5e8-33de-4ab1-b7ee-127c58afc2c2")
DD_RUM_CLIENT_TOKEN   = os.environ.get("DD_RUM_CLIENT_TOKEN",   "pubc75578b047a4e05c64024799f357e852")
DD_ENV                = os.environ.get("DD_ENV", "dev")

LUMVORAX_TAGS = [
    "project:lumvorax",
    "module:btc_quantum_mining",
    "module_id:17",
    f"env:{DD_ENV}",
    f"site:{DD_SITE}",
]


def _send_series(series: list) -> tuple:
    """Envoie une liste de métriques Datadog (format v2)."""
    if not DD_API_TOKEN:
        log.error("DATADOG_API_TOKEN absent — impossible d'envoyer les métriques")
        return 0, "DATADOG_API_TOKEN manquant"

    payload = json.dumps({"series": series}).encode()
    req = urllib.request.Request(
        DD_METRICS_URL,
        data=payload,
        headers={
            "DD-API-KEY": DD_API_TOKEN,
            "Content-Type": "application/json",
        },
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=20) as r:
            body = r.read().decode()
            return r.status, body
    except urllib.error.HTTPError as e:
        body = e.read().decode()
        return e.code, body
    except Exception as ex:
        return 0, str(ex)


def _metric(name: str, value: float, tags: list = None, ts: int = None) -> dict:
    """Construit un point de métrique Datadog v2."""
    return {
        "metric": name,
        "type": 0,
        "points": [{"timestamp": ts or int(time.time()), "value": float(value)}],
        "tags": (tags or []) + LUMVORAX_TAGS,
    }


def send_btc_run_metrics(
    run_id: str,
    cycle: str,
    best_leading_zeros: int,
    hashrate_mhs: float,
    nx48_enabled: bool,
    nx48_neuron_count: float = 0,
    nx48_update_count: float = 0,
    nx48_stall_count: float = 0,
    nx48_loss: float = 0,
    metrics_count: int = 0,
    anomalies_count: int = 0,
    duration_s: float = 0,
    threads: int = 4,
    ts: int = None,
) -> tuple:
    """Envoie les métriques d'un run BTC vers Datadog."""
    tags = [
        f"run_id:{run_id}",
        f"cycle:{cycle}",
        f"nx48_enabled:{'true' if nx48_enabled else 'false'}",
        f"threads:{threads}",
    ]

    series = [
        _metric("lumvorax.btc.best_leading_zeros", best_leading_zeros, tags, ts),
        _metric("lumvorax.btc.hashrate_mhs", hashrate_mhs, tags, ts),
        _metric("lumvorax.btc.metrics_count", metrics_count, tags, ts),
        _metric("lumvorax.btc.anomalies_count", anomalies_count, tags, ts),
        _metric("lumvorax.btc.duration_s", duration_s, tags, ts),
        _metric("lumvorax.btc.nx48.enabled", 1.0 if nx48_enabled else 0.0, tags, ts),
        _metric("lumvorax.btc.nx48.neuron_count", nx48_neuron_count, tags, ts),
        _metric("lumvorax.btc.nx48.update_count", nx48_update_count, tags, ts),
        _metric("lumvorax.btc.nx48.stall_count", nx48_stall_count, tags, ts),
        _metric("lumvorax.btc.nx48.loss", nx48_loss, tags, ts),
    ]

    status, body = _send_series(series)
    if status in (200, 202):
        log.info(f"[OK] run_id={run_id} — {len(series)} métriques envoyées (HTTP {status})")
    else:
        log.error(f"[ERREUR] HTTP {status} — {body[:200]}")
    return status, body


def send_c46_forensic():
    """Envoie les métriques forensic du cycle C46 vers Datadog (datadoghq.eu)."""
    log.info("=== Envoi métriques forensic C46 → Datadog EU ===")
    log.info(f"    URL : {DD_METRICS_URL}")
    log.info(f"    DD_SITE : {DD_SITE}")

    ts_a = int(time.mktime(time.strptime("2026-04-15T21:01:00", "%Y-%m-%dT%H:%M:%S")))
    ts_b = int(time.mktime(time.strptime("2026-04-15T21:01:05", "%Y-%m-%dT%H:%M:%S")))

    # Cas A — NX48 disabled
    s_a, body_a = send_btc_run_metrics(
        run_id="btc_20260415T210100Z_1169",
        cycle="C46",
        best_leading_zeros=0,
        hashrate_mhs=0.43,
        nx48_enabled=False,
        metrics_count=1734,
        anomalies_count=0,
        duration_s=5.21,
        threads=2,
        ts=ts_a,
    )
    log.info(f"Cas A (NX48 OFF) : HTTP {s_a} — {body_a[:80]}")

    # Cas B — NX48 enabled
    s_b, body_b = send_btc_run_metrics(
        run_id="btc_20260415T210105Z_1173",
        cycle="C46",
        best_leading_zeros=19,
        hashrate_mhs=0.43,
        nx48_enabled=True,
        nx48_neuron_count=2.0,
        nx48_update_count=1818.0,
        nx48_stall_count=0.0,
        nx48_loss=0.0,
        metrics_count=1734,
        anomalies_count=0,
        duration_s=5.04,
        threads=2,
        ts=ts_b,
    )
    log.info(f"Cas B (NX48 ON) : HTTP {s_b} — {body_b[:80]}")

    return s_a, s_b


def send_c44_forensic():
    """Envoie les métriques forensic du cycle C44 vers Datadog."""
    log.info("=== Envoi métriques forensic C44 → Datadog ===")

    ts_a = int(time.mktime(time.strptime("2026-04-15T17:58:28", "%Y-%m-%dT%H:%M:%S")))
    ts_b = int(time.mktime(time.strptime("2026-04-15T17:58:39", "%Y-%m-%dT%H:%M:%S")))

    # Run A — NX48 disabled
    s_a, _ = send_btc_run_metrics(
        run_id="btc_20260415T175828Z_3064",
        cycle="C44",
        best_leading_zeros=20,
        hashrate_mhs=0.4365138372,
        nx48_enabled=False,
        metrics_count=1734,
        anomalies_count=8,
        duration_s=10.0,
        threads=4,
        ts=ts_a,
    )

    # Run B — NX48 enabled
    s_b, _ = send_btc_run_metrics(
        run_id="btc_20260415T175839Z_3132",
        cycle="C44",
        best_leading_zeros=20,
        hashrate_mhs=0.4344127878,
        nx48_enabled=True,
        nx48_neuron_count=2.0,
        nx48_update_count=3.0,
        nx48_stall_count=2.0,
        nx48_loss=0.8338,
        metrics_count=1775,
        anomalies_count=8,
        duration_s=10.0,
        threads=4,
        ts=ts_b,
    )

    return s_a, s_b


def send_agent_heartbeat(host: str = "replit", cycle: str = "C45") -> tuple:
    """Envoie un heartbeat agent vers Datadog."""
    tags = [f"host:{host}", f"cycle:{cycle}"]
    series = [
        _metric("lumvorax.agent.heartbeat", 1.0, tags),
        _metric("lumvorax.agent.uptime_s", float(int(time.time())), tags),
    ]
    return _send_series(series)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="LumVorax C46 — Datadog Metrics EU (datadoghq.eu)")
    parser.add_argument("--c46-forensic", action="store_true", help="Envoyer métriques C46 vers Datadog EU")
    parser.add_argument("--c44-forensic", action="store_true", help="Envoyer métriques C44 vers Datadog EU")
    parser.add_argument("--heartbeat", action="store_true", help="Envoyer heartbeat agent")
    parser.add_argument("--host", default="replit", help="Nom de l'hôte pour le heartbeat")
    args = parser.parse_args()

    if not DD_API_TOKEN:
        log.error("DD_API_KEY non configuré — configurer dans Replit Secrets ou Doppler")
        log.error(f"  Site cible : {DD_SITE}")
        log.error(f"  URL cible  : {DD_METRICS_URL}")
        log.error("  Chercher dans : GitHub Actions Secrets → DD_API_KEY (32 hex chars)")
        sys.exit(1)

    log.info(f"DD_SITE = {DD_SITE}, URL = {DD_METRICS_URL}")

    if args.c46_forensic:
        s_a, s_b = send_c46_forensic()
        ok = all(s in (200, 202) for s in (s_a, s_b))
        sys.exit(0 if ok else 1)

    if args.c44_forensic:
        s_a, s_b = send_c44_forensic()
        ok = all(s in (200, 202) for s in (s_a, s_b))
        sys.exit(0 if ok else 1)

    if args.heartbeat:
        s, body = send_agent_heartbeat(args.host, cycle="C46")
        log.info(f"Heartbeat HTTP {s}")
        sys.exit(0 if s in (200, 202) else 1)

    parser.print_help()
