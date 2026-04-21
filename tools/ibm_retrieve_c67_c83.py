#!/usr/bin/env python3
"""Récupération de TOUS les jobs IBM existants (C65-C83) + génération rapport final consolidé."""
import ctypes as _ctypes
import os as _os
for _p in [
    "/nix/store/bmi5znnqk4kg2grkrhk6py0irc8phf6l-gcc-14.2.1.20250322-lib/lib/libstdc++.so.6",
    "/lib/x86_64-linux-gnu/libstdc++.so.6",
    "/usr/lib/x86_64-linux-gnu/libstdc++.so.6",
]:
    if _os.path.exists(_p):
        try:
            _ctypes.CDLL(_p)
        except OSError:
            pass
        break
import os, sys, json, time, hashlib, gzip, math
from pathlib import Path
from datetime import datetime, timezone

STAMP = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
OUT_DIR = Path("src/advanced_calculations/bitcoin_quantum_mining")
RESULT_DIR = OUT_DIR / "results"
CHAT_DIR = OUT_DIR / "CHAT"
FORENSIC_DIR = OUT_DIR / "logs/forensic"
LUM_DIR = OUT_DIR / "logs/lum_native"
for p in (RESULT_DIR, FORENSIC_DIR, LUM_DIR):
    p.mkdir(parents=True, exist_ok=True)

# Job IDs connus (C65 + C69)
KNOWN_JOBS = {
    "d7j4otn16ugs73eud8qg": ("C65", "QDAYPRIZE_156Q"),
    "d7j4pff16ugs73eud9c0": ("C65", "ED_2x2_VALID"),
    "d7j4poq3fd4c73ddk1sg": ("C65", "BTC_GROVER_156Q"),
    "d7j4q1hs7cos73ejf760": ("C65", "RCS_XEB_156Q"),
    "d7j4qiv16ugs73eudae0": ("C65", "HTS_hubbard_core"),
    "d7j4ql716ugs73eudah0": ("C65", "HTS_spin_liquid"),
    "d7j4qmv16ugs73eudaj0": ("C65", "HTS_fermionic"),
    "d7j4qon16ugs73eudal0": ("C65", "HTS_qchem"),
    "d7j4r8q3fd4c73ddk3cg": ("C65", "QDAYPRIZE_8Q"),
    "d7jck723fd4c73ddsgv0": ("C69", "QDAYPRIZE_32anc_scale1"),
}

def shannon(counts):
    tot = sum(counts.values())
    if tot <= 0: return 0.0
    h = 0.0
    for v in counts.values():
        if v > 0:
            p = v/tot; h -= p*math.log2(p)
    return round(h, 4)

def extract_counts(result):
    """Retourne dict {bitstring: count} depuis un PrimitiveResult IBM."""
    try:
        pub = result[0]
        # SamplerV2 PubResult: data.<reg>.get_counts()
        for attr in dir(pub.data):
            if attr.startswith("_"): continue
            obj = getattr(pub.data, attr)
            if hasattr(obj, "get_counts"):
                return dict(obj.get_counts())
        # fallback meas
        if hasattr(pub.data, "meas"):
            return dict(pub.data.meas.get_counts())
    except Exception as e:
        return {"_error": str(e)}
    return {}

api_key = os.environ.get("IBM_API_KEY") or os.environ.get("QISKIT_IBM_TOKEN")
if not api_key:
    print("ERREUR: IBM_API_KEY absent"); sys.exit(2)

print(f"[INFO] Connexion IBM Quantum...")
from qiskit_ibm_runtime import QiskitRuntimeService
service = QiskitRuntimeService(channel="ibm_quantum_platform", token=api_key)
print(f"[INFO] Connecté.")

# 1. Lister tous les jobs récents (limit 50)
all_jobs = []
try:
    jobs = service.jobs(limit=50, descending=True)
    for j in jobs:
        try:
            jid = j.job_id()
            st = str(j.status())
            ct = j.creation_date.isoformat() if hasattr(j, "creation_date") and j.creation_date else "?"
            all_jobs.append({"job_id": jid, "status": st, "created": ct})
        except Exception as e:
            all_jobs.append({"job_id": "?", "error": str(e)})
    print(f"[INFO] {len(all_jobs)} jobs listés côté serveur.")
except Exception as e:
    print(f"[WARN] Liste jobs échouée: {e}")

# Ajouter les jobs inconnus à KNOWN_JOBS
for jinfo in all_jobs:
    jid = jinfo.get("job_id")
    if jid and jid != "?" and jid not in KNOWN_JOBS:
        KNOWN_JOBS[jid] = ("AUTO", "unknown_module")

# 2. Récupérer counts de chaque job
retrieved = []
for jid, (cycle, mod) in KNOWN_JOBS.items():
    entry = {"job_id": jid, "cycle": cycle, "module": mod}
    try:
        job = service.job(jid)
        st = str(job.status())
        entry["status"] = st
        if "DONE" not in st.upper():
            entry["note"] = f"Non terminé ({st})"
            retrieved.append(entry); continue
        try:
            res = job.result()
            counts = extract_counts(res)
            if "_error" in counts:
                entry["error"] = counts["_error"]
                retrieved.append(entry); continue
            tot = sum(counts.values())
            entry["n_shots_total"] = tot
            entry["n_distinct_states"] = len(counts)
            if counts:
                top = sorted(counts.items(), key=lambda kv: -kv[1])[:5]
                entry["top_states"] = {k:v for k,v in top}
                entry["dominant_state"] = top[0][0]
                entry["dominant_prob"] = round(top[0][1]/tot, 6) if tot>0 else 0
                entry["entropy_bits"] = shannon(counts)
            print(f"[OK] {cycle} {mod} ({jid}): shots={tot}, états={len(counts)}, dom={entry.get('dominant_prob')}, H={entry.get('entropy_bits')}b")
        except Exception as e:
            entry["result_error"] = str(e)
            print(f"[WARN] {jid}: result extract failed: {e}")
    except Exception as e:
        entry["error"] = str(e)
        print(f"[WARN] {jid} introuvable: {e}")
    retrieved.append(entry)

# 3. Sauvegarder JSON forensic
out_json = RESULT_DIR / f"ibm_c67_c83_retrieve_{STAMP}.json"
out_json.write_text(json.dumps({
    "stamp": STAMP, "cycle": "C67_C83_RETRIEVE",
    "all_jobs_listed": all_jobs,
    "retrieved": retrieved,
    "n_total": len(retrieved),
    "n_done": sum(1 for r in retrieved if r.get("status","").upper().endswith("DONE")),
}, indent=2, default=str), encoding="utf-8")
print(f"[INFO] JSON sauvegardé: {out_json}")

# 4. Forensic .lum natif (gzip+sha256, magic LUMQ v2)
lum_payload = json.dumps(retrieved, default=str, separators=(",",":")).encode("utf-8")
lum_compressed = gzip.compress(lum_payload, compresslevel=6)
lum_sha = hashlib.sha256(lum_compressed).hexdigest()
lum_file = LUM_DIR / f"ibm_c67_c83_retrieve_{STAMP}.lum"
with open(lum_file, "wb") as f:
    f.write(b"LUMQ" + b"\x02" + len(lum_sha).to_bytes(2,"big") + lum_sha.encode() + lum_compressed)
print(f"[INFO] .lum natif: {lum_file} ({lum_file.stat().st_size}B sha256={lum_sha[:16]}...)")

# 5. Rapport markdown
lines = [f"# Rapport LumVorax IBM Quantum C67→C83 — Récupération Finale {STAMP}", ""]
lines.append("## Avancement global")
lines.append(f"- 100% — {len(retrieved)} jobs IBM interrogés, {sum(1 for r in retrieved if r.get('n_shots_total',0)>0)} avec counts extraits")
lines.append(f"- 100% — JSON forensic: `{out_json.name}`")
lines.append(f"- 100% — `.lum` natif: `{lum_file.name}` (sha256_16={lum_sha[:16]})")
lines.append(f"- {len(all_jobs)} jobs listés côté serveur IBM (50 derniers)")
lines.append("")
lines.append("## Tableau AVANT / APRÈS — Tous jobs IBM récupérés")
lines.append("| Cycle | Module | Job ID | Statut | Shots | États | Dominant | Entropie H | Note |")
lines.append("|-------|--------|--------|--------|-------|-------|----------|------------|------|")
for r in retrieved:
    s = r.get("status","?")
    sh = r.get("n_shots_total","—")
    n = r.get("n_distinct_states","—")
    dp = r.get("dominant_prob","—")
    ds = r.get("dominant_state","—")
    h = r.get("entropy_bits","—")
    note = r.get("note","") or r.get("error","") or r.get("result_error","")
    if isinstance(ds, str) and len(ds) > 24: ds = ds[:8]+"…"+ds[-4:]
    lines.append(f"| {r.get('cycle','?')} | {r.get('module','?')} | `{r.get('job_id','?')[:24]}` | {s.replace('JobStatus.','')} | {sh} | {n} | {dp} (`{ds}`) | {h} | {note[:60]} |")
lines.append("")
lines.append("## Détail par job (DONE seulement)")
for r in retrieved:
    if r.get("n_shots_total",0) <= 0: continue
    lines.append(f"### {r['cycle']} — {r['module']} — `{r['job_id']}`")
    lines.append(f"- Shots: {r['n_shots_total']}, États distincts: {r['n_distinct_states']}, Entropie: {r['entropy_bits']} bits")
    lines.append(f"- État dominant: `{r['dominant_state']}` (prob={r['dominant_prob']})")
    top = r.get("top_states",{})
    tot = max(1, r['n_shots_total'])
    for k,v in list(top.items())[:5]:
        lines.append(f"  - `{k[:80]}{'…' if len(k)>80 else ''}`: {v} ({100*v/tot:.2f}%)")
    lines.append("")
lines.append("## Tous les jobs côté serveur IBM (50 derniers)")
for j in all_jobs[:30]:
    lines.append(f"- `{j.get('job_id')}` — {j.get('status','?').replace('JobStatus.','')} — {j.get('created','?')}")
lines.append("")
lines.append("## Conclusion")
lines.append("Tous les jobs IBM exécutés (C65 + C69 + autres) ont été interrogés. Les counts réels sont enregistrés au format `.lum` natif gzip+sha256 et JSON forensic. Le format `.lum` reste la source primaire de vérité (DB-like remplaçant PostgreSQL conformément à l'architecture LuM).")
report = CHAT_DIR / f"RAPPORT_IBM_QUANTUM_C67_C83_RETRIEVE_{STAMP}.md"
report.write_text("\n".join(lines)+"\n", encoding="utf-8")
print(f"[INFO] Rapport: {report}")
print(f"[DONE] Récupération terminée.")
