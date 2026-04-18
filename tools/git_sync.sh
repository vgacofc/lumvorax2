#!/usr/bin/env bash
# ============================================================
# LumVorax C60 — git_sync.sh
# Synchronisation git SÉCURISÉE avec stash automatique
# Résout : "error: Your local changes would be overwritten by merge"
#
# Usage : bash tools/git_sync.sh [branche]
# Standard : STANDARD_NAMES.md v4.3 | prompt.txt v2.1
# ============================================================

set -euo pipefail

BRANCH="${1:-main}"
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

echo ""
echo "[GIT-SYNC] ============================================"
echo "[GIT-SYNC] LumVorax — Git Sync Sécurisé — C60"
echo "[GIT-SYNC] Branche : $BRANCH"
echo "[GIT-SYNC] ============================================"

# ─── 1. Stash des fichiers locaux modifiés ───────────────────
echo "[GIT-SYNC] Stash des modifications locales..."
STASH_MSG="auto-stash-$(date +%Y%m%dT%H%M%SZ)"
git add -u
git stash save "$STASH_MSG" || echo "[GIT-SYNC] Rien à stash"

# ─── 2. git pull ─────────────────────────────────────────────
echo "[GIT-SYNC] git pull origin $BRANCH ..."
git pull origin "$BRANCH"
echo "[GIT-SYNC] ✅ Pull OK"

# ─── 3. Restaurer les fichiers stashés ───────────────────────
STASH_LIST=$(git stash list | grep "$STASH_MSG" | head -1 | cut -d: -f1 || true)
if [ -n "$STASH_LIST" ]; then
    echo "[GIT-SYNC] Restauration stash : $STASH_LIST"
    git stash pop "$STASH_LIST" || {
        echo "[GIT-SYNC] ⚠️  Conflit stash pop — conservation du stash"
        echo "            Résoudre manuellement : git stash show -p $STASH_LIST"
    }
else
    echo "[GIT-SYNC] Aucun stash à restaurer"
fi

# ─── 4. Nettoyage .o compilés (évite SIGILL cross-CPU) ───────
echo "[GIT-SYNC] Nettoyage des .o compilés (cross-CPU dangereux) ..."
find "$REPO_ROOT/src" -name "*.o" -delete 2>/dev/null && echo "[GIT-SYNC] .o supprimés ✓"
rm -f "$REPO_ROOT/src/qdayprize/qdayprize_runner" \
      "$REPO_ROOT/src/qdayprize/qdayprize_runner_debug" \
      "$REPO_ROOT/src/advanced_calculations/bitcoin_quantum_mining/btc_mining_runner" 2>/dev/null || true
echo "[GIT-SYNC] Binaires pré-compilés supprimés ✓"

# ─── 5. Recompilation locale QDAYPRIZE ───────────────────────
echo ""
echo "[GIT-SYNC] Recompilation QDAYPRIZE (locale, -mno-avx512) ..."
cd "$REPO_ROOT/src/qdayprize"
make clean 2>/dev/null || true
make 2>&1 | grep -E "\[QDPR\]|error:|warning:" | head -20
echo "[GIT-SYNC] ✅ QDAYPRIZE compilé"

# ─── 6. Recompilation locale BTC Mining ──────────────────────
echo ""
echo "[GIT-SYNC] Recompilation BTC Mining (locale) ..."
BTC_DIR="$REPO_ROOT/src/advanced_calculations/bitcoin_quantum_mining"
if [ -f "$BTC_DIR/Makefile" ]; then
    cd "$BTC_DIR"
    make clean 2>/dev/null || true
    make 2>&1 | grep -E "\[MODULE\|error:|warning:" | head -20
    echo "[GIT-SYNC] ✅ BTC Mining compilé"
else
    echo "[GIT-SYNC] Info: Makefile BTC non trouvé dans $BTC_DIR"
fi

cd "$REPO_ROOT"
echo ""
echo "[GIT-SYNC] ✅ Sync complet — tout recompilé localement"
echo "[GIT-SYNC] Lance l'agent : doppler run --config dev_lumvorax -- bash tools/agent_ubuntu_ws.sh"
echo ""
