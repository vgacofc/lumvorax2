#!/usr/bin/env fish
# ============================================================
# LumVorax — Mise à jour Doppler depuis Ubuntu (syntaxe Fish)
# À exécuter sur Ubuntu quand l'URL Replit change de session
#
# Usage :
#   bash tools/doppler_update.fish https://NOUVELLE-URL.replit.dev
#   ou :
#   source tools/doppler_update.fish  (depuis le répertoire du repo)
#
# Ce qui se passe :
#   1. Récupère le token via l'endpoint /agent/token du serveur Flask
#   2. Met à jour Doppler config=dev_lumvorax avec la nouvelle URL + token
#   3. Affiche confirmation
# ============================================================

# ─── 1. Vérifier l'argument URL ─────────────────────────────
if test (count $argv) -ge 1
    set NEW_URL $argv[1]
else
    echo ""
    echo "[QDPR-DOPPLER] Usage : bash tools/doppler_update.fish https://URL.replit.dev"
    echo ""
    echo "L'URL est visible dans l'onglet 'Preview' de Replit."
    echo "Elle change à chaque nouvelle session Replit."
    echo ""
    echo "[QDPR-DOPPLER] Entrez la nouvelle URL Replit (CTRL+C pour annuler) :"
    read NEW_URL
end

# ─── 2. Valider format URL ───────────────────────────────────
if not string match -q "https://*" $NEW_URL
    echo "[ERREUR] URL invalide : $NEW_URL (doit commencer par https://)"
    exit 1
end

# Supprimer le slash final si présent
set NEW_URL (string trim --right --chars=/ $NEW_URL)

echo ""
echo "[DOPPLER] ============================================"
echo "[DOPPLER] LumVorax — Mise à jour Doppler (Fish)"
echo "[DOPPLER] Nouvelle URL : $NEW_URL"
echo "[DOPPLER] ============================================"

# ─── 3. Récupérer le token depuis le serveur Flask ──────────
echo "[DOPPLER] Récupération du token depuis /agent/token ..."
set TOKEN_RESPONSE (curl -sf --max-time 10 "$NEW_URL/agent/token" 2>/dev/null)

if test $status -ne 0 -o -z "$TOKEN_RESPONSE"
    echo "[ERREUR] Impossible de contacter $NEW_URL/agent/token"
    echo "         → Vérifiez que Start application tourne sur Replit"
    echo ""
    echo "         Sinon, entrez le token manuellement :"
    read NEW_TOKEN
else
    set NEW_TOKEN (echo $TOKEN_RESPONSE | python3 -c "import sys,json; print(json.load(sys.stdin)['token'])" 2>/dev/null)
    if test -z "$NEW_TOKEN"
        echo "[ERREUR] Impossible de parser la réponse JSON : $TOKEN_RESPONSE"
        exit 1
    end
    echo "[DOPPLER] Token récupéré : "(string sub -l 8 $NEW_TOKEN)"..."
end

# ─── 4. Mise à jour Doppler ──────────────────────────────────
echo "[DOPPLER] Mise à jour Doppler config=dev_lumvorax ..."
set -x DOPPLER_NO_UPDATE_NOTIFIER true
set -x DOPPLER_UPDATE_CHECK false

doppler secrets set \
    REPLIT_URL="$NEW_URL" \
    AGENT_TOKEN="$NEW_TOKEN" \
    --config dev_lumvorax \
    --no-interactive 2>&1

if test $status -ne 0
    echo "[ERREUR] Doppler set échoué. Essai alternatif..."
    echo n | doppler secrets set REPLIT_URL="$NEW_URL" AGENT_TOKEN="$NEW_TOKEN" --config dev_lumvorax
end

# ─── 5. Vérification ─────────────────────────────────────────
echo ""
echo "[DOPPLER] ✅ Doppler mis à jour."
echo "          URL   : $NEW_URL"
echo "          Token : "(string sub -l 8 $NEW_TOKEN)"..."
echo ""
echo "[DOPPLER] → Lance maintenant l'agent WebSocket :"
echo "   doppler run --config dev_lumvorax -- bash tools/agent_ubuntu_ws.sh"
echo ""
