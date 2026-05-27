#!/usr/bin/env bash
# LumVorax C84 — Synchronisation Replit -> Doppler
# Usage : DOPPLER_TOKEN=dp.st.xxx ./tools/sync_replit_to_doppler.sh
# Pousse les variables d'environnement actuelles (Replit Secrets) dans Doppler.
# A lancer depuis Replit shell ou depuis Ubuntu apres avoir charge l'env.
set -euo pipefail

DOPPLER_PROJECT="${DOPPLER_PROJECT:-lumvorax}"
DOPPLER_CONFIG="${DOPPLER_CONFIG:-dev_lumvorax}"
DOPPLER_TOKEN="${DOPPLER_TOKEN:-}"

if [ -z "$DOPPLER_TOKEN" ]; then
    echo "[SYNC] ❌ DOPPLER_TOKEN absent. export DOPPLER_TOKEN=dp.st.xxx avant de lancer."
    exit 1
fi
if ! command -v doppler &>/dev/null; then
    echo "[SYNC] ❌ doppler CLI absent. Installer : curl -Ls https://cli.doppler.com/install.sh | sh"
    exit 1
fi

# Liste blanche : SEULS ces secrets seront pousses (eviter de fuiter PATH/HOME/etc)
WHITELIST=(
    IBM_API_KEY
    IBM_QUANTUM_TOKEN
    IBM_CLOUD_API_KEY
    IBM_INSTANCE_CRN
    BTC_WALLET_PRIV_HEX
    BTC_WALLET_ADDR
    SUPABASE_URL
    SUPABASE_ANON_KEY
    SUPABASE_SERVICE_ROLE_KEY
    ARISTOCLE_IA_API_KEY
    SUPERMEMORY_API_KEY
    SESSION_SECRET
    DATABASE_URL
    OPENAI_API_KEY
    ANTHROPIC_API_KEY
)

echo "[SYNC] === Synchronisation Replit -> Doppler ==="
echo "[SYNC] Projet : $DOPPLER_PROJECT  Config : $DOPPLER_CONFIG"

PUSHED=0; SKIPPED=0
for key in "${WHITELIST[@]}"; do
    val="${!key:-}"
    if [ -z "$val" ]; then
        echo "[SYNC] - $key : absent dans env -> skip"
        SKIPPED=$((SKIPPED + 1))
        continue
    fi
    # Push silencieux (--silent), affiche seulement OK
    if doppler secrets set "$key=$val" \
        --token "$DOPPLER_TOKEN" \
        --project "$DOPPLER_PROJECT" \
        --config "$DOPPLER_CONFIG" \
        --silent --no-interactive >/dev/null 2>&1; then
        echo "[SYNC] ✅ $key (len=${#val}) pousse vers Doppler"
        PUSHED=$((PUSHED + 1))
    else
        echo "[SYNC] ⚠️  $key : echec push"
    fi
done

echo "[SYNC] === Termine : $PUSHED pousses, $SKIPPED ignores ==="
echo "[SYNC] Verification : doppler secrets --token \$DOPPLER_TOKEN --project $DOPPLER_PROJECT --config $DOPPLER_CONFIG"
