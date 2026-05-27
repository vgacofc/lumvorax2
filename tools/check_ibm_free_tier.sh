#!/usr/bin/env bash
# LumVorax C84 — Verification quota IBM free-tier (Quantum Runtime + Code Engine)
# Usage : IBM_API_KEY=... IBM_CLOUD_API_KEY=... ./tools/check_ibm_free_tier.sh
# Affiche : quota Runtime restant (10min/mois free), quota Code Engine (400 vCPU.s/mois),
#           quota Object Storage (25 Go free), quota Container Registry (500 Mo).
set -euo pipefail

IBM_API_KEY="${IBM_API_KEY:-}"             # token Quantum Runtime
IBM_CLOUD_API_KEY="${IBM_CLOUD_API_KEY:-}" # API key IAM IBM Cloud
IBM_INSTANCE_CRN="${IBM_INSTANCE_CRN:-}"

echo "[FREE-TIER] === LumVorax IBM Quota Check ==="
date -u +"[FREE-TIER] %Y-%m-%dT%H:%M:%SZ"

# --- 1. Quantum Runtime (open plan = 10 minutes/mois) ---
if [ -n "$IBM_API_KEY" ]; then
    echo "[FREE-TIER] --- Qiskit Runtime ---"
    python3 - <<PY
import os, sys
try:
    from qiskit_ibm_runtime import QiskitRuntimeService
    svc = QiskitRuntimeService(channel="ibm_quantum_platform", token=os.environ["IBM_API_KEY"])
    print(f"[FREE-TIER] Service connecte : {svc.active_account().get('channel')}")
    try:
        usage = svc.usage()
        print(f"[FREE-TIER] Usage runtime : {usage}")
    except Exception as e:
        print(f"[FREE-TIER] usage() indisponible ({e}), liste backends:")
    for b in svc.backends(operational=True, simulator=False):
        try:
            st = b.status()
            print(f"[FREE-TIER]   - {b.name:20s} {b.num_qubits}Q queue={st.pending_jobs} status={st.status_msg}")
        except Exception as e:
            print(f"[FREE-TIER]   - {b.name}: {e}")
except Exception as e:
    print(f"[FREE-TIER] Erreur Runtime : {e}", file=sys.stderr)
PY
else
    echo "[FREE-TIER] ⚠️  IBM_API_KEY absent -> skip Runtime"
fi

# --- 2. IBM Cloud (Code Engine + COS + CR) ---
if [ -n "$IBM_CLOUD_API_KEY" ] && command -v ibmcloud &>/dev/null; then
    echo "[FREE-TIER] --- IBM Cloud login ---"
    ibmcloud login --apikey "$IBM_CLOUD_API_KEY" --no-region >/dev/null 2>&1 \
        && echo "[FREE-TIER] ✅ ibmcloud login OK" \
        || echo "[FREE-TIER] ❌ ibmcloud login FAIL"

    echo "[FREE-TIER] --- Code Engine projects ---"
    ibmcloud ce project list 2>&1 | head -20 || true

    echo "[FREE-TIER] --- Code Engine consumption (mois courant) ---"
    ibmcloud billing account-usage 2>&1 | head -40 || true

    echo "[FREE-TIER] --- Object Storage instances ---"
    ibmcloud resource service-instances --service-name cloud-object-storage 2>&1 | head -10 || true

    echo "[FREE-TIER] --- Container Registry quota ---"
    ibmcloud cr quota 2>&1 | head -10 || true

    echo "[FREE-TIER] --- Resources resume ---"
    ibmcloud resource service-instances 2>&1 | head -30 || true
else
    echo "[FREE-TIER] ⚠️  IBM_CLOUD_API_KEY absent ou ibmcloud CLI non installe"
    echo "[FREE-TIER]    Install : curl -fsSL https://clis.cloud.ibm.com/install/linux | sh"
fi

# --- 3. Resume Doppler vs Replit ---
echo "[FREE-TIER] --- Synchronisation Doppler ---"
if command -v doppler &>/dev/null && [ -n "${DOPPLER_TOKEN:-}" ]; then
    doppler secrets --token "$DOPPLER_TOKEN" \
        --project "${DOPPLER_PROJECT:-lumvorax}" \
        --config "${DOPPLER_CONFIG:-dev_lumvorax}" \
        --only-names 2>&1 | head -30 || true
else
    echo "[FREE-TIER] doppler CLI ou DOPPLER_TOKEN absent"
fi

echo "[FREE-TIER] === Termine ==="
echo "[FREE-TIER] BUDGETS DE REFERENCE :"
echo "[FREE-TIER]   - Qiskit Runtime open plan : 10 min/mois reset 1er du mois"
echo "[FREE-TIER]   - Code Engine : 400 vCPU.s + 100 GB.s RAM/mois"
echo "[FREE-TIER]   - Object Storage Lite : 25 Go + 20k requests/mois"
echo "[FREE-TIER]   - Container Registry : 500 Mo + 5 Go pull/mois"
