#!/usr/bin/env python3
"""
LumVorax — Module 17 — Supermemory Integration
tools/nx48_supermemory.py — Vraie mémoire persistante NX48 inter-sessions

RÔLE : Supermemory est la MÉMOIRE CENTRALE de NX48.
  - Au démarrage (--init) : récupère le meilleur état NX48 connu TOUTES sessions confondues
    et met à jour config/btc_nx48_last.csv si Supermemory a un meilleur record.
  - Après chaque run (--store-run) : envoie TOUTES les formules, calculs,
    paramètres NX48 et records vers Supermemory pour mémoire inter-sessions.
  - Accessible depuis TOUS les comptes qui partagent la même clé API Supermemory.

URL validée C43: POST https://api.supermemory.ai/v3/documents
Container: lumvorax_nx48

STANDARD_NAMES.md v4.3 §M-BTC17-C43 — 2026-04-15
"""

import os, sys, json, argparse, urllib.request, urllib.error, urllib.parse, time, csv

SUPERMEMORY_URL        = "https://api.supermemory.ai/v3/documents"
SUPERMEMORY_SEARCH_URL = "https://api.supermemory.ai/v3/search"
SUPERMEMORY_CONTAINER  = "lumvorax_nx48"
FALLBACK_CACHE         = "/tmp/lumvorax_supermemory_cache.jsonl"
SUPABASE_RECORDS_TABLE = "btc_records"
SUPABASE_METRICS_TABLE = "btc_metrics_realtime"

# ── Paramètres NX48 complets (C43) ───────────────────────────────
NX48_PARAMS_C43 = [
    "delta_nonce_scale",
    "n_replicas_scale",
    "swap_temp_scale",
    "batch_size_scale",
    "exploration_bias",
    "best_leading_zeros",
    "best_nonce",
    "update_count",
    "loss_curr",
    "grad_norm",
    "w0","w1","w2","w3","w4","w5","w6","w7",
    "bias",
    "exec_w0","exec_w1","exec_w2","exec_w3","exec_w4","exec_w5","exec_w6","exec_w7",
    "exec_bias",
    "dual_blend",
]
NX48_PARAMS_C42 = NX48_PARAMS_C43

NX48_FEATURES = [
    ("F0", "btc_best_leading_zeros",  "best_leading_zeros / 256.0"),
    ("F1", "btc_hashrate_norm",       "hashrate_mhs / hashrate_max"),
    ("F2", "btc_ptmc_swap_rate",      "swaps_accepted / swaps_attempted"),
    ("F3", "btc_time_stall",          "log10(1 + time_since_improvement_s)"),
    ("F4", "btc_nonce_coverage",      "nonce_coverage_pct / 100.0"),
    ("F5", "btc_delta_nonce_norm",    "delta_nonce / 4294967296.0"),
    ("F6", "btc_thread_eff",          "hashes_done / hashes_expected"),
    ("F7", "btc_temp_ratio",          "(T_hot / T_cold) / 100.0"),
]

NX48_FORMULAS = """
FORMULES NX48_BTC (Cycle C43 — STANDARD_NAMES.md v4.3) :

1. PRÉDICTION APPLICATEUR (sigmoid executor):
   z = exec_bias + sum(exec_weights[i] * features[i] for i in 0..7)
   prob = sigmoid(z) = 1 / (1 + exp(-z))

2. LABEL (linéaire sur 256 bits — C38-FIX-LABEL-256):
   label = best_leading_zeros / 256.0
   → label(28)=0.109 | label(32)=0.125 | label(256)=1.0 (objectif)

3. BCE LOSS:
   bce = -(label * log(prob) + (1-label) * log(1-prob))

4. GRADIENT ISTA PRODUCTEUR (déroulage 8 features — C41-SIMD-ISTA):
   err = prob - label
   w[i]_new = soft_threshold(w[i] - lr * err * features[i], lambda_L1)
   bias_new = bias - lr * err
   lr=0.01, lambda_L1=0.001

5. DISTILLATION DUAL-NEURON C43 :
   exec_w[i] = (1 - dual_blend) * exec_w[i] + dual_blend * w[i]
   exec_bias = (1 - dual_blend) * exec_bias + dual_blend * bias
   dual_blend ∈ [0.01, 0.50]

6. DELTA_NONCE ADAPTATIF (Xoshiro256++ — C65-FIX-ADAPT):
   loss_delta = (loss_curr - loss_prev) / loss_prev
   sigma = sigma_base * exp(stagnation * log(sigma_max / sigma_base))
   perturbation = N(0, sigma)  [Xoshiro256++]
   delta_nonce_new = delta_nonce_old * exp(alpha * (-loss_delta) + perturbation)
   alpha=0.8 | sigma_base=0.05 | sigma_max=0.25

7. BATCH SCHEDULING (tanh continu — C65-FIX-BATCH):
   adapt_rate = 1.0 + 0.10 * tanh(5.0 * grad_norm)
   batch_size_scale_new = batch_size_scale * adapt_rate  [clamp: 0.5, 4.0]

8. SCAN ORBITAL (30% des threads — C39-P3):
   Si best_leading >= 20 et U < 0.30:
     offset = gauss(0, ORBITAL_RADIUS / 0.577)  [ORBITAL_RADIUS = 50000]
     nonce = best_nonce + offset

9. LEBESGUE SCAN (25% des threads — C39-P5):
   level = randint(0, best_leading)
   weight = (level + 1) / (best_leading + 1)
   leb_radius = ORBITAL_RADIUS * (1 + (1 - weight) * 4)
   nonce = best_nonce ± randint(0, leb_radius)
"""


def get_key():
    key = os.environ.get("SUPERMEMORY_API_KEY", "")
    if not key:
        print("[NX48-MEM] WARN: SUPERMEMORY_API_KEY absent — Supermemory désactivé")
    return key


def _post_document(key, content, metadata=None):
    payload = {
        "content": content,
        "metadata": metadata or {},
        "containerTags": [SUPERMEMORY_CONTAINER]
    }
    req = urllib.request.Request(
        SUPERMEMORY_URL,
        data=json.dumps(payload).encode(),
        headers={"Authorization": f"Bearer {key}", "Content-Type": "application/json"},
        method="POST"
    )
    try:
        with urllib.request.urlopen(req, timeout=15) as r:
            return True, json.loads(r.read())
    except urllib.error.HTTPError as e:
        body = e.read().decode()[:300]
        return False, {"error": e.code, "body": body}
    except Exception as e:
        return False, {"error": str(e)}


def _search(key, query, limit=10):
    url = f"{SUPERMEMORY_SEARCH_URL}?q={urllib.parse.quote(query)}&limit={limit}"
    req = urllib.request.Request(
        url,
        headers={"Authorization": f"Bearer {key}", "Content-Type": "application/json"}
    )
    try:
        with urllib.request.urlopen(req, timeout=15) as r:
            return True, json.loads(r.read())
    except Exception as e:
        return False, {"error": str(e)}


def _fallback_save(content, metadata):
    entry = {"timestamp": time.time(), "content": content, "metadata": metadata}
    with open(FALLBACK_CACHE, "a") as f:
        f.write(json.dumps(entry) + "\n")
    print(f"[NX48-MEM] Fallback local: {FALLBACK_CACHE}")


def _supabase_auth():
    url = os.environ.get("SUPABASE_URL", "").strip() or os.environ.get("SUPABASE8_API_URL", "").strip()
    key = os.environ.get("SUPABASE_SERVICE_ROLE_KEY", "").strip()
    if not url or not key:
        return None, None
    return url.rstrip("/"), key


def _supabase_request(method, table, payload=None, query=""):
    url, key = _supabase_auth()
    if not url or not key:
        print("[NX48-MEM] Supabase service_role absent — skip")
        return False, {"error": "missing_supabase_service_role"}
    endpoint = f"{url}/rest/v1/{table}{query}"
    data = json.dumps(payload).encode() if payload is not None else None
    headers = {
        "apikey": key,
        "Authorization": f"Bearer {key}",
        "Content-Type": "application/json",
        "Prefer": "return=minimal",
    }
    req = urllib.request.Request(endpoint, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req, timeout=15) as r:
            body = r.read().decode()
            return True, json.loads(body) if body else {"status": r.status}
    except urllib.error.HTTPError as e:
        return False, {"error": e.code, "body": e.read().decode()[:500]}
    except Exception as e:
        return False, {"error": str(e)}


def _supabase_fetch_best():
    ok, data = _supabase_request(
        "GET",
        SUPABASE_RECORDS_TABLE,
        query="?select=*&order=leading_zeros.desc&limit=1",
    )
    if not ok:
        return {}
    rows = data if isinstance(data, list) else []
    return rows[0] if rows else {}


def _store_supabase_state(params, cycle, run_id):
    try:
        leading = int(float(params.get("best_leading_zeros", 0) or 0))
    except Exception:
        leading = 0
    payload = {
        "run_id": run_id,
        "cycle": cycle,
        "leading_zeros": leading,
        "nonce": str(params.get("best_nonce", "")),
        "update_count": int(float(params.get("update_count", 0) or 0)),
        "loss_curr": float(params.get("loss_curr", 0) or 0),
        "grad_norm": float(params.get("grad_norm", 0) or 0),
        "delta_nonce_scale": float(params.get("delta_nonce_scale", 0) or 0),
        "exploration_bias": float(params.get("exploration_bias", 0) or 0),
        "weights": {f"w{i}": params.get(f"w{i}") for i in range(8)},
        "bias": float(params.get("bias", 0) or 0),
        "created_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
    }
    ok, resp = _supabase_request("POST", SUPABASE_RECORDS_TABLE, payload)
    if ok:
        print(f"[NX48-MEM] Supabase btc_records OK — leading={leading}")
    else:
        print(f"[NX48-MEM] Supabase btc_records WARN: {resp}")
    metric_payload = dict(payload)
    ok2, resp2 = _supabase_request("POST", SUPABASE_METRICS_TABLE, metric_payload)
    if ok2:
        print("[NX48-MEM] Supabase btc_metrics_realtime OK")
    else:
        print(f"[NX48-MEM] Supabase btc_metrics_realtime WARN: {resp2}")
    return ok or ok2


def read_csv(csv_path):
    """Lit le CSV NX48 et retourne un dict de paramètres."""
    if not csv_path or not os.path.exists(csv_path):
        return {}
    try:
        with open(csv_path, "r") as f:
            reader = csv.DictReader(f)
            for row in reader:
                return dict(row)  # première ligne
    except Exception as e:
        print(f"[NX48-MEM] WARN: lecture CSV {csv_path}: {e}")
    return {}


def write_csv(csv_path, params):
    """Écrit le CSV NX48 depuis un dict de paramètres."""
    if not csv_path:
        return False
    os.makedirs(os.path.dirname(csv_path) if os.path.dirname(csv_path) else ".", exist_ok=True)
    try:
        fieldnames = ["run_id", "delta_nonce_scale", "n_replicas_scale", "swap_temp_scale",
                      "batch_size_scale", "exploration_bias", "best_leading_zeros",
                      "best_nonce", "update_count", "loss_curr", "grad_norm",
                      "w0","w1","w2","w3","w4","w5","w6","w7","bias",
                      "exec_w0","exec_w1","exec_w2","exec_w3","exec_w4","exec_w5","exec_w6","exec_w7",
                      "exec_bias","dual_blend"]
        with open(csv_path, "w", newline="") as f:
            writer = csv.DictWriter(f, fieldnames=fieldnames, extrasaction="ignore")
            writer.writeheader()
            writer.writerow(params)
        return True
    except Exception as e:
        print(f"[NX48-MEM] WARN: écriture CSV {csv_path}: {e}")
        return False


def init_session(stamp, csv_path=None):
    """
    C42-SUPRA-INIT : Récupère le meilleur état NX48 depuis Supermemory.
    Si Supermemory contient un record >= celui du CSV local → mise à jour CSV.
    Ceci garantit que même après un reset, NX48 repart de son meilleur état connu.
    """
    key = get_key()
    if not key:
        return

    print(f"[NX48-MEM] Init session C42 — stamp={stamp}")

    # Lire état CSV local actuel
    local_params = read_csv(csv_path) if csv_path else {}
    local_best = int(local_params.get("best_leading_zeros", 0))
    print(f"[NX48-MEM] État local CSV: best_leading={local_best} nonce={local_params.get('best_nonce','?')}")

    # Chercher le meilleur état dans Supermemory
    queries = [
        "LumVorax NX48 best_leading_zeros record C42",
        "LumVorax BTC quantum mining record leading zeros",
        "nx48_btc_state best nonce weights"
    ]

    sm_best = 0
    sm_best_params = {}

    for q in queries:
        ok, data = _search(key, q, limit=5)
        if not ok:
            print(f"[NX48-MEM] Recherche FAIL: {data}")
            continue
        # Analyser les résultats pour trouver le meilleur record
        results = data if isinstance(data, list) else data.get("results", [])
        for item in results:
            content = item.get("content", "") if isinstance(item, dict) else str(item)
            # Extraire best_leading_zeros du contenu
            for line in content.split("\n"):
                if "best_leading_zeros" in line and "=" in line:
                    try:
                        val = int(line.split("=")[1].strip().split()[0].replace(",",""))
                        if val > sm_best:
                            sm_best = val
                            # Extraire tous les paramètres disponibles
                            for param_line in content.split("\n"):
                                if "=" in param_line:
                                    k, _, v = param_line.partition("=")
                                    k = k.strip().lower().replace("-","_")
                                    v = v.strip().split()[0].replace(",","")
                                    sm_best_params[k] = v
                    except (ValueError, IndexError):
                        pass

    print(f"[NX48-MEM] Meilleur état Supermemory: best_leading={sm_best}")

    supa_best_params = _supabase_fetch_best()
    try:
        supa_best = int(float(supa_best_params.get("leading_zeros", 0) or supa_best_params.get("best_leading_zeros", 0) or 0))
    except Exception:
        supa_best = 0
    if supa_best > sm_best:
        sm_best = supa_best
        mapped = dict(supa_best_params)
        if "leading_zeros" in mapped:
            mapped["best_leading_zeros"] = mapped["leading_zeros"]
        if "nonce" in mapped:
            mapped["best_nonce"] = mapped["nonce"]
        weights = mapped.get("weights")
        if isinstance(weights, dict):
            mapped.update(weights)
        sm_best_params = mapped
        print(f"[NX48-MEM] Supabase > Supermemory: best_leading={sm_best}")

    # Mettre à jour le CSV si Supermemory a un meilleur record
    if sm_best > local_best and sm_best_params and csv_path:
        print(f"[NX48-MEM] ✅ Supermemory > CSV ({sm_best} > {local_best}) — mise à jour CSV")
        # Merger: garder les paramètres locaux, mettre à jour ceux de Supermemory
        merged = dict(local_params)
        for k, v in sm_best_params.items():
            if k in NX48_PARAMS_C42 or k == "run_id":
                merged[k] = v
        for i in range(8):
            merged.setdefault(f"exec_w{i}", merged.get(f"w{i}", "0"))
        merged.setdefault("exec_bias", merged.get("bias", "0"))
        merged.setdefault("dual_blend", "0.20")
        if "run_id" not in merged:
            merged["run_id"] = f"sm_restored_{stamp}"
        write_csv(csv_path, merged)
        print(f"[NX48-MEM] CSV mis à jour avec état Supermemory (best_leading={sm_best})")
    else:
        print(f"[NX48-MEM] CSV local à jour (best_leading={local_best} >= SM={sm_best})")

    # Envoyer confirmation de démarrage à Supermemory
    ram_mb = 0
    try:
        with open("/proc/meminfo") as f:
            for line in f:
                if "MemAvailable" in line:
                    ram_mb = int(line.split()[1]) // 1024
                    break
    except Exception:
        pass

    content = (
        f"LumVorax NX48 Session Init — stamp={stamp} cycle=C43\n"
        f"CSV local: best_leading_zeros={local_best} nonce={local_params.get('best_nonce','?')}\n"
        f"Supermemory best: best_leading_zeros={sm_best}\n"
        f"RAM disponible: {ram_mb}MB\n"
        f"STANDARD_NAMES.md v4.3 §M-BTC17-C43"
    )
    metadata = {
        "source": "LumVorax", "module": "btc_quantum_mining",
        "cycle": "C43", "event": "session_start", "stamp": stamp,
        "best_leading_zeros_local": local_best,
        "best_leading_zeros_sm": sm_best,
        "standard": "STANDARD_NAMES_v4.3"
    }
    ok, resp = _post_document(key, content, metadata)
    if ok:
        print(f"[NX48-MEM] ✅ Init envoyé Supermemory: {json.dumps(resp)[:150]}")
    else:
        print(f"[NX48-MEM] ⚠️ Init Supermemory FAIL: {resp}")
        _fallback_save(content, metadata)


def store_run(csv_path, cycle, run_id):
    """
    Envoie l'état complet NX48 + formules + calculs vers Supermemory après un run.
    C'est ici que TOUTE la connaissance accumulée est persistée inter-sessions.
    """
    key = get_key()
    if not key:
        return

    params = read_csv(csv_path) if csv_path else {}
    if not params:
        print("[NX48-MEM] CSV vide — rien à envoyer")
        return

    best_leading = params.get("best_leading_zeros", "?")
    best_nonce   = params.get("best_nonce", "?")
    update_count = params.get("update_count", "?")
    loss         = params.get("loss_curr", "?")
    delta        = params.get("delta_nonce_scale", "?")
    expl         = params.get("exploration_bias", "?")

    # Calcul de delta_nonce absolu (en nonces)
    try:
        delta_nonce_abs = int(float(delta) * 65536)
    except Exception:
        delta_nonce_abs = "?"

    # État complet des weights
    weights_str = ""
    for i in range(8):
        w = params.get(f"w{i}", "?")
        features_names = ["btc_best_leading_zeros/256", "hashrate_norm", "ptmc_swap_rate",
                          "time_stall", "nonce_coverage", "delta_nonce_norm", "thread_eff", "temp_ratio"]
        fname = features_names[i] if i < len(features_names) else f"f{i}"
        weights_str += f"  w{i}={w} ({fname})\n"
    bias_val = params.get("bias", "?")
    executor_str = ""
    for i in range(8):
        executor_str += f"  exec_w{i}={params.get(f'exec_w{i}', params.get(f'w{i}', '?'))}\n"
    exec_bias_val = params.get("exec_bias", bias_val)
    dual_blend_val = params.get("dual_blend", "0.20")

    content = f"""LumVorax NX48 État Complet — Cycle {cycle} — Run {run_id}
================================================

RECORD :
  best_leading_zeros = {best_leading}
  best_nonce         = {best_nonce}
  update_count       = {update_count}

HYPERPARAMÈTRES D'EXPLORATION :
  delta_nonce_scale  = {delta}  → rayon absolu = {delta_nonce_abs} nonces
  n_replicas_scale   = {params.get('n_replicas_scale','?')}
  swap_temp_scale    = {params.get('swap_temp_scale','?')}
  batch_size_scale   = {params.get('batch_size_scale','?')}
  exploration_bias   = {expl}

NEURONE PRODUCTEUR NX48 (C43-DUAL-PRODUCER) :
  bias = {bias_val}
{weights_str}
NEURONE APPLICATEUR NX48 (C43-DUAL-EXECUTOR) :
  exec_bias = {exec_bias_val}
  dual_blend = {dual_blend_val}
{executor_str}
ÉTAT APPRENTISSAGE :
  loss_curr  = {loss}
  grad_norm  = {params.get('grad_norm','?')}

{NX48_FORMULAS}

FEATURES (8 entrées du neurone) :
""" + "\n".join(f"  {fid}: {fname} = {formula}" for fid, fname, formula in NX48_FEATURES) + f"""

STANDARD_NAMES.md v4.3 §M-BTC17-C43
Enregistré: {time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())}
"""

    metadata = {
        "source": "LumVorax", "module": "btc_quantum_mining",
        "cycle": cycle, "run_id": run_id,
        "event": "run_complete",
        "best_leading_zeros": int(best_leading) if str(best_leading).isdigit() else 0,
        "best_nonce": str(best_nonce),
        "update_count": str(update_count),
        "standard": "STANDARD_NAMES_v4.3",
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
    }

    print(f"[NX48-MEM] Envoi état complet NX48 → Supermemory (best_leading={best_leading})")
    ok, resp = _post_document(key, content, metadata)
    if ok:
        doc_id = resp.get("id", "?") if isinstance(resp, dict) else "?"
        print(f"[NX48-MEM] ✅ État NX48 persisté — ID: {doc_id} — best_leading={best_leading}")
    else:
        print(f"[NX48-MEM] ⚠️ Supermemory FAIL: {resp}")
        _fallback_save(content, metadata)
    _store_supabase_state(params, cycle, run_id)


def store_discovery(cycle, run_id, content, extra_meta=None):
    key = get_key()
    if not key:
        return None
    metadata = {
        "source": "LumVorax", "module": "btc_quantum_mining",
        "cycle": cycle, "run_id": run_id,
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "standard": "STANDARD_NAMES_v4.3",
        **(extra_meta or {})
    }
    print(f"[NX48-MEM] Envoi découverte: cycle={cycle} run={run_id}")
    ok, resp = _post_document(key, content, metadata)
    if ok:
        print(f"[NX48-MEM] ✅ OK: {json.dumps(resp)[:200]}")
        return resp
    else:
        print(f"[NX48-MEM] ⚠️ FAIL: {resp}")
        _fallback_save(content, metadata)
        return None


def main():
    parser = argparse.ArgumentParser(description="LumVorax NX48 Supermemory C43")
    parser.add_argument("--init",       metavar="STAMP",   help="Init session (récupère état depuis SM)")
    parser.add_argument("--csv",        metavar="CSV_PATH", help="Chemin CSV NX48 (config/btc_nx48_last.csv)")
    parser.add_argument("--store-run",  action="store_true", help="Envoie état NX48 complet à Supermemory")
    parser.add_argument("--store",      metavar="CONTENT",  help="Envoie une découverte")
    parser.add_argument("--cycle",      default="C43")
    parser.add_argument("--run-id",     default="unknown")
    parser.add_argument("--recall",     action="store_true")
    parser.add_argument("--query",      default="LumVorax NX48 BTC")
    args = parser.parse_args()

    if args.init:
        init_session(args.init, csv_path=args.csv)
    elif args.store_run:
        store_run(args.csv, args.cycle, args.run_id)
    elif args.store:
        store_discovery(args.cycle, args.run_id, args.store)
    elif args.recall:
        key = get_key()
        ok, data = _search(key, args.query)
        print("RECALL:", json.dumps(data, indent=2)[:2000])


if __name__ == "__main__":
    main()
