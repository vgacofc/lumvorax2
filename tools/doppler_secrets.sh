#!/usr/bin/env bash
# LumVorax — Système de secrets Doppler (priorité) + Replit fallback
# Usage: source tools/doppler_secrets.sh
# STANDARD_NAMES.md v4.2 — Cycle C41 — 2026-04-13

DOPPLER_PROJECT="lumvorax"
DOPPLER_CONFIG="dev_lumvorax"
DOPPLER_TOKEN="${DOPPLER_TOKEN:-}"

echo "[DOPPLER] Chargement secrets — projet=${DOPPLER_PROJECT} config=${DOPPLER_CONFIG}"

if [ -n "$DOPPLER_TOKEN" ] && command -v doppler &>/dev/null; then
    # Télécharge les secrets Doppler dans l'environnement
    DOPPLER_SECRETS=$(doppler secrets download \
        --token "$DOPPLER_TOKEN" \
        --project "$DOPPLER_PROJECT" \
        --config "$DOPPLER_CONFIG" \
        --no-file --format json 2>/dev/null)

    if [ $? -eq 0 ] && [ -n "$DOPPLER_SECRETS" ]; then
        echo "[DOPPLER] ✅ Secrets Doppler chargés"
        # Exporte chaque secret comme variable d'env
        while IFS= read -r line; do
            export "$line"
        done < <(echo "$DOPPLER_SECRETS" | python3 -c "
import sys, json
data = json.load(sys.stdin)
for k, v in data.items():
    if k not in ('DOPPLER_PROJECT', 'DOPPLER_CONFIG', 'DOPPLER_ENVIRONMENT'):
        print(f'{k}={v}')
")
        echo "[DOPPLER] Variables exportées depuis Doppler"
    else
        echo "[DOPPLER] ⚠️  Doppler FAIL — bascule sur secrets Replit"
    fi
else
    echo "[DOPPLER] ℹ️  Doppler CLI absent ou token manquant — secrets Replit utilisés"
fi

# Les secrets Replit (BTC_WALLET_PRIV_HEX, SUPABASE_URL, etc.) sont déjà dans l'env
echo "[DOPPLER] BTC_WALLET_PRIV_HEX=$([ -n '$BTC_WALLET_PRIV_HEX' ] && echo '✅ OK' || echo '❌ MANQUANT')"
echo "[DOPPLER] SUPABASE_URL=$([ -n '$SUPABASE_URL' ] && echo '✅ OK' || echo '❌ MANQUANT')"
echo "[DOPPLER] ARISTOCLE_IA_API_KEY=$([ -n '$ARISTOCLE_IA_API_KEY' ] && echo '✅ OK' || echo '❌ MANQUANT')"
echo "[DOPPLER] SUPERMEMORY_API_KEY=$([ -n '$SUPERMEMORY_API_KEY' ] && echo '✅ OK' || echo '❌ MANQUANT')"
