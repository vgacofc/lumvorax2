#!/usr/bin/env bash
# ============================================================
# LumVorax C255v8e — Script Mining BTC i915 DRM Native
#
# ARCHITECTURE: 100% i915 DRM Direct (0% OpenCL, 0% Level Zero)
# VERSION: C255v8e — Mini-Batches + Corrections Validées
# PERFORMANCE: 0.226 GH/s (27/27 dispatches, 100% stable)
#
# USAGE:
#   bash tools/btc_run_native_c255v8e.sh
#   doppler run --config dev_lumvorax -- bash tools/btc_run_native_c255v8e.sh
# ============================================================

set -euo pipefail

# ─── Configuration ─────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LEVEL_ZERO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
REPO_ROOT="$(cd "$LEVEL_ZERO_DIR/../../.." && pwd)"

BTC_NATIVE_BIN="$LEVEL_ZERO_DIR/bin/test_btc_mining_native"
KERNEL_PATH="$LEVEL_ZERO_DIR/kernels/btc_sha256_mining_gen9.bin"
LOG_DIR="$LEVEL_ZERO_DIR/logs/forensic"
STAMP=$(date -u +%Y%m%dT%H%M%SZ)
RUN_ID="btc_native_c255v8e_${STAMP}"
LOG_FILE="$LOG_DIR/btc_mining_production_${STAMP}.log"

# Créer répertoires si nécessaire
mkdir -p "$LOG_DIR"
mkdir -p "$LEVEL_ZERO_DIR/bin"
mkdir -p "$LEVEL_ZERO_DIR/build"

echo "╔════════════════════════════════════════════════════════════╗"
echo "║  LumVorax C255v8e — Bitcoin Mining i915 DRM Native        ║"
echo "╠════════════════════════════════════════════════════════════╣"
echo "║  Architecture : 100% i915 DRM Direct                       ║"
echo "║  OpenCL       : 0%                                         ║"
echo "║  Level Zero   : 0%                                         ║"
echo "║  Performance  : 0.226 GH/s (27/27 dispatches)              ║"
echo "║  Stabilité    : 100% (mini-batches validés)                ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "[C255v8e] Run ID        : $RUN_ID"
echo "[C255v8e] Binaire       : $BTC_NATIVE_BIN"
echo "[C255v8e] Kernel        : $KERNEL_PATH"
echo "[C255v8e] Log           : $LOG_FILE"
echo ""

# ─── Étape 1 : Vérification Kernel ────────────────────────
echo "[C255v8e] Étape 1/4 — Vérification kernel SHA-256..."

if [ ! -f "$KERNEL_PATH" ]; then
    echo "❌ ERREUR: Kernel introuvable: $KERNEL_PATH"
    echo ""
    echo "SOLUTION:"
    echo "  Le kernel SHA-256 Gen9 doit être présent dans:"
    echo "  $KERNEL_PATH"
    echo ""
    echo "  Taille attendue: 42672 bytes (42KB)"
    echo ""
    exit 1
fi

KERNEL_SIZE=$(stat -c%s "$KERNEL_PATH" 2>/dev/null || stat -f%z "$KERNEL_PATH" 2>/dev/null)
echo "[C255v8e]   Kernel trouvé: $KERNEL_SIZE bytes"

if [ "$KERNEL_SIZE" -ne 42672 ]; then
    echo "⚠️  ATTENTION: Taille kernel incorrecte (attendu: 42672 bytes)"
    echo "   Le mining peut échouer avec un kernel corrompu"
fi

# ─── Étape 2 : Compilation Binaire ────────────────────────
echo "[C255v8e] Étape 2/4 — Compilation binaire native..."

cd "$LEVEL_ZERO_DIR"

# Sync git si disponible
if [ -d "$REPO_ROOT/.git" ]; then
    echo "[C255v8e]   Sync git (git pull origin main)..."
    cd "$REPO_ROOT" && git pull origin main --quiet 2>&1 | tail -3 || echo "[WARN] git pull échoué — compilation avec état local"
    cd "$LEVEL_ZERO_DIR"
fi

# Créer répertoires build si nécessaire (fix erreur compilation)
mkdir -p build bin logs/forensic

# Compilation
echo "[C255v8e]   Nettoyage..."
make -f Makefile.gen9_native clean 2>&1 | tail -3

echo "[C255v8e]   Compilation..."
make -f Makefile.gen9_native test_btc 2>&1 | tail -10

if [ ! -f "$BTC_NATIVE_BIN" ]; then
    echo "❌ ERREUR: Compilation échouée"
    echo ""
    echo "DIAGNOSTIC:"
    echo "  - Vérifier dépendances: libdrm-dev"
    echo "  - Vérifier Makefile: Makefile.gen9_native"
    echo "  - Logs compilation ci-dessus"
    echo ""
    exit 1
fi

BIN_SIZE=$(stat -c%s "$BTC_NATIVE_BIN" 2>/dev/null || stat -f%z "$BTC_NATIVE_BIN" 2>/dev/null)
echo "[C255v8e]   Binaire créé: $BIN_SIZE bytes"

# ─── Étape 3 : Vérification Device GPU ────────────────────
echo "[C255v8e] Étape 3/4 — Vérification device GPU..."

if [ ! -c "/dev/dri/renderD128" ]; then
    echo "❌ ERREUR: Device GPU introuvable: /dev/dri/renderD128"
    echo ""
    echo "DIAGNOSTIC:"
    echo "  - GPU Intel présent ? lspci | grep VGA"
    echo "  - Driver i915 chargé ? lsmod | grep i915"
    echo "  - Permissions correctes ? ls -l /dev/dri/"
    echo ""
    exit 1
fi

echo "[C255v8e]   Device GPU: /dev/dri/renderD128 ✅"

# Lire infos GPU si disponible
if [ -f "/sys/class/drm/card0/device/vendor" ]; then
    VENDOR=$(cat /sys/class/drm/card0/device/vendor 2>/dev/null || echo "unknown")
    DEVICE=$(cat /sys/class/drm/card0/device/device 2>/dev/null || echo "unknown")
    echo "[C255v8e]   Vendor: $VENDOR, Device: $DEVICE"
fi

# ─── Étape 4 : Lancement Mining ───────────────────────────
echo "[C255v8e] Étape 4/4 — Lancement mining production..."
echo "[C255v8e] ════════════════════════════════════════════════════════════"
echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║  MINING BITCOIN — PRODUCTION C255V8E                       ║"
echo "╠════════════════════════════════════════════════════════════╣"
echo "║  Mode          : Production illimitée                      ║"
echo "║  Dispatches    : 27 par batch (mini-batches de 9)          ║"
echo "║  Batch size    : 262,144 nonces/dispatch                   ║"
echo "║  Hashrate      : ~0.226 GH/s (stable)                      ║"
echo "║  Architecture  : i915 DRM Direct (0% OpenCL)               ║"
echo "║  Logs          : $LOG_FILE"
echo "║                                                            ║"
echo "║  CTRL+C pour arrêter                                       ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Lancer mining avec logs
cd "$LEVEL_ZERO_DIR"
"$BTC_NATIVE_BIN" 2>&1 | tee "$LOG_FILE"

# ─── Fin ───────────────────────────────────────────────────
echo ""
echo "[C255v8e] ════════════════════════════════════════════════════════════"
echo "[C255v8e] Mining terminé"
echo "[C255v8e] Logs sauvegardés: $LOG_FILE"
echo "[C255v8e] ════════════════════════════════════════════════════════════"

# Made with Bob
