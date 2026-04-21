#!/usr/bin/env bash
# tools/ibmcloud_ubuntu_login.sh
# Script Doppler-aware pour login IBM Cloud + OpenShift depuis Ubuntu
# REQUIS : doppler login fait, projet=lumvorax, config=dev_lumvorax
# Le PASSCODE a été poussé sur Doppler depuis Replit (2026-04-21).
#
# USAGE :
#   bash tools/ibmcloud_ubuntu_login.sh
#   ou : doppler run --config dev_lumvorax -- bash tools/ibmcloud_ubuntu_login.sh

set -u

# ────────────────────────────────────────────────────────────
# 1. Vérification CLI installées (à installer une fois)
# ────────────────────────────────────────────────────────────
need_install=0
for c in ibmcloud oc kubectl; do
    if ! command -v "$c" >/dev/null 2>&1; then
        echo "❌ $c absent"
        need_install=1
    fi
done

if [ "$need_install" = "1" ]; then
    echo ""
    echo "=== Installation ibmcloud + oc + kubectl ==="
    # IBM Cloud CLI
    curl -fsSL https://clis.cloud.ibm.com/install/linux | sh
    ibmcloud plugin install code-engine container-service container-registry -f
    # OpenShift CLI
    curl -fsSL https://mirror.openshift.com/pub/openshift-v4/clients/ocp/stable/openshift-client-linux.tar.gz | sudo tar -xz -C /usr/local/bin oc kubectl
    echo "✅ Installation terminée — relancez ce script."
    exit 0
fi

# ────────────────────────────────────────────────────────────
# 2. Récupérer PASSCODE depuis env (Doppler ou export manuel)
# ────────────────────────────────────────────────────────────
if [ -z "${PASSCODE:-}" ]; then
    if command -v doppler >/dev/null 2>&1; then
        PASSCODE=$(doppler secrets get PASSCODE --project lumvorax --config dev_lumvorax --plain 2>/dev/null)
    fi
fi

if [ -z "${PASSCODE:-}" ]; then
    echo "❌ PASSCODE introuvable (ni env, ni Doppler)"
    echo "   Récupérer un nouveau passcode (valide 30s) depuis :"
    echo "   https://cloud.ibm.com/iam#/passcode"
    echo "   Puis : doppler secrets set PASSCODE --project lumvorax --config dev_lumvorax"
    exit 1
fi

echo "✅ PASSCODE récupéré (${#PASSCODE} chars)"

# ────────────────────────────────────────────────────────────
# 3. ibmcloud login (passcode = SSO one-time)
# ────────────────────────────────────────────────────────────
echo ""
echo "=== ibmcloud login ==="
ibmcloud login -a https://cloud.ibm.com -u passcode -p "$PASSCODE" -r us-south 2>&1 | tail -20
if [ $? -ne 0 ]; then
    echo "❌ Login échoué — passcode probablement expiré (>30s)"
    echo "   Régénérer : https://cloud.ibm.com/iam#/passcode"
    exit 2
fi

# ────────────────────────────────────────────────────────────
# 4. Vérifier ressources disponibles (free tier)
# ────────────────────────────────────────────────────────────
echo ""
echo "=== Quotas Code Engine free-tier ==="
ibmcloud ce project list 2>&1 | head -10 || echo "  (pas de project Code Engine — créer avec : ibmcloud ce project create --name lumvorax-bare-metal)"

echo ""
echo "=== Quotas Container Registry free-tier ==="
ibmcloud cr quota 2>&1 | head -10

echo ""
echo "=== Quotas Cloud Object Storage ==="
ibmcloud resource service-instances --service-name cloud-object-storage 2>&1 | head -10

# ────────────────────────────────────────────────────────────
# 5. (Optionnel) oc login si cluster OpenShift dispo
# ────────────────────────────────────────────────────────────
echo ""
echo "=== Clusters OpenShift dispo (free-tier IBM Cloud n'inclut PAS OpenShift gratuit) ==="
ibmcloud oc cluster ls 2>&1 | head -10 || echo "  Pas de cluster Openshift (plan gratuit)."
echo ""
echo "  ℹ️  IBM Cloud free-tier inclut :"
echo "    ✅ Code Engine : 100k vCPU-secondes/mois (suffisant pour run BTC mining ponctuel)"
echo "    ✅ Object Storage : 25 GB Lite"
echo "    ✅ Container Registry : 0.5 GB stockage + 5 GB pull traffic"
echo "    ❌ OpenShift : payant (≥ \$1071/mois)"
echo "    ❌ Bare Metal : payant (≥ \$700/mois)"
echo ""
echo "=== Stratégie recommandée pour LumVorax ==="
echo "  → Pousser le code BTC mining sur Container Registry (gratuit)"
echo "  → Lancer un job Code Engine ponctuel pour valider quelques minutes (gratuit)"
echo "  → Stocker les .lum sur Object Storage Lite (gratuit, 25GB)"
echo "  → Pas besoin d'OpenShift / Bare Metal pour cette phase"
echo ""
echo "=== Prochaine commande pour push image BTC ==="
echo "  ibmcloud cr namespace-add lumvorax-btc"
echo "  docker build -t us.icr.io/lumvorax-btc/btc_mining_runner:latest ."
echo "  docker push us.icr.io/lumvorax-btc/btc_mining_runner:latest"
echo "  ibmcloud ce job create --name btc-mining-c63 --image us.icr.io/lumvorax-btc/btc_mining_runner:latest --cpu 8 --memory 16G"
echo "  ibmcloud ce jobrun submit --job btc-mining-c63"
echo ""
echo "=== Script terminé ==="
