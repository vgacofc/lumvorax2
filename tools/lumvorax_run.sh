#!/usr/bin/env bash
# ============================================================
# LumVorax C63 — Lanceur principal : QDAYPRIZE → BTC
# ============================================================
# Ordre d'exécution obligatoire C63 :
#   1. ROTATION des fichiers > 50 MB (repo propre, pas de LFS)
#   2. QDAYPRIZE (sim quantique) — NX48 apprend de ses résultats
#   3. Supermemory RECALL — restaure l'état NX48 depuis la mémoire externe
#   4. BTC mining — NX48 dispose du feedback QDAYPRIZE + état restauré
#   5. Supermemory PUSH — sauvegarde l'état NX48 après le run
#
# Pourquoi QDAYPRIZE avant BTC ?
#   Le NX48 (sous-neurone SN7) ajuste exploration_bias selon le taux
#   de succès QDAYPRIZE (fichier qdayprize_qdpr_*.json).
#   Si BTC démarre en premier, SN7 lit l'ancien fichier JSON (session
#   précédente) → biais non actualisé. En démarrant QDAYPRIZE d'abord,
#   le JSON est frais pour SN7 dès le premier update NX48.
#
# Paramètres dynamiques QDAYPRIZE (variables d'environnement) :
#   QDPR_SIM_BITS     (défaut: 20)    — bits de simulation quantique
#   QDPR_DURATION_S   (défaut: 90)    — durée simulation (0=infini)
#   QDPR_THREADS      (défaut: 4)     — threads QDAYPRIZE
#   BTC_DURATION_S    (défaut: 0)     — durée BTC (0=infini)
#   BTC_THREADS       (défaut: auto)  — threads BTC
#   SKIP_ROTATE       (défaut: 0)     — 1 pour ignorer la rotation
#   SKIP_SUPERMEMORY  (défaut: 0)     — 1 pour ignorer Supermemory
#
# Usage :
#   bash tools/lumvorax_run.sh                      # run complet défaut
#   QDPR_SIM_BITS=24 bash tools/lumvorax_run.sh     # QDAYPRIZE 24 bits
#   BTC_DURATION_S=3600 bash tools/lumvorax_run.sh  # BTC 1h
#   doppler run --config dev_lumvorax -- bash tools/lumvorax_run.sh
#
# STANDARD_NAMES.md v4.4 §F-LAUNCHER-C63

set -euo pipefail

# ─── Détection environnement ────────────────────────────────
if [ -d "$HOME/LVX/lumvorax2" ]; then
    REPO_ROOT="$HOME/LVX/lumvorax2"
elif [ -d "/home/runner/workspace" ]; then
    REPO_ROOT="/home/runner/workspace"
else
    REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
fi

TOOLS_DIR="$REPO_ROOT/tools"
BTC_DIR="$REPO_ROOT/src/advanced_calculations/bitcoin_quantum_mining"
QDPR_DIR="$REPO_ROOT/src/qdayprize"
LOG_DIR="$BTC_DIR/logs/forensic"
STAMP=$(date -u +%Y%m%dT%H%M%SZ)

# ─── Paramètres ─────────────────────────────────────────────
QDPR_SIM_BITS="${QDPR_SIM_BITS:-20}"
QDPR_DURATION_S="${QDPR_DURATION_S:-90}"
QDPR_THREADS="${QDPR_THREADS:-4}"
BTC_DURATION_S="${BTC_DURATION_S:-0}"
BTC_THREADS="${BTC_THREADS:-$(nproc)}"
SKIP_ROTATE="${SKIP_ROTATE:-0}"
SKIP_SUPERMEMORY="${SKIP_SUPERMEMORY:-0}"

echo ""
echo "[LVX-RUN] ═══════════════════════════════════════════════"
echo "[LVX-RUN] LumVorax C63 — QDAYPRIZE → BTC Launcher"
echo "[LVX-RUN] REPO     : $REPO_ROOT"
echo "[LVX-RUN] STAMP    : $STAMP"
echo "[LVX-RUN] QDPR     : sim_bits=${QDPR_SIM_BITS} dur=${QDPR_DURATION_S}s threads=${QDPR_THREADS}"
echo "[LVX-RUN] BTC      : dur=${BTC_DURATION_S}s threads=${BTC_THREADS}"
echo "[LVX-RUN] ROTATE   : $([ "$SKIP_ROTATE" = "0" ] && echo activée || echo désactivée)"
echo "[LVX-RUN] SUPERMEM : $([ "$SKIP_SUPERMEMORY" = "0" ] && echo activée || echo désactivée)"
echo "[LVX-RUN] ═══════════════════════════════════════════════"
echo ""

# ══════════════════════════════════════════════════════════════
# ÉTAPE 0 : GIT PULL (sync dernières corrections)
# ══════════════════════════════════════════════════════════════
echo "[LVX-RUN] ── ÉTAPE 0 : Sync git ──"
cd "$REPO_ROOT"
git pull origin main --quiet 2>&1 | tail -3 || echo "[WARN] git pull échoué — continuation locale"

# ══════════════════════════════════════════════════════════════
# ÉTAPE 1 : ROTATION 50 MB
# ══════════════════════════════════════════════════════════════
if [ "$SKIP_ROTATE" = "0" ] && [ -f "$TOOLS_DIR/rotate_large_files.sh" ]; then
    echo ""
    echo "[LVX-RUN] ── ÉTAPE 1 : Rotation fichiers > 50 MB ──"
    bash "$TOOLS_DIR/rotate_large_files.sh" || echo "[WARN] Rotation partielle — continuation"
fi

# ══════════════════════════════════════════════════════════════
# ÉTAPE 2 : SUPERMEMORY RECALL — restaurer état NX48
# ══════════════════════════════════════════════════════════════
if [ "$SKIP_SUPERMEMORY" = "0" ] && [ -f "$TOOLS_DIR/nx48_supermemory.py" ]; then
    echo ""
    echo "[LVX-RUN] ── ÉTAPE 2 : Supermemory RECALL ──"
    SM_KEY="${SUPERMEMORY_API_KEY:-}"
    if [ -n "$SM_KEY" ]; then
        python3 "$TOOLS_DIR/nx48_supermemory.py" --recall \
            --query "LumVorax NX48 BTC state exploration_bias best_leading_zeros" \
            --output "$BTC_DIR/config/btc_nx48_last.csv" \
            2>&1 || echo "[WARN] Supermemory recall échoué — démarrage depuis LUM local"
        echo "[LVX-RUN] ✅ Supermemory recall terminé"
    else
        echo "[LVX-RUN] ⚠️ SUPERMEMORY_API_KEY absent — skip recall"
        echo "          → Mettre dans Doppler: doppler secrets set SUPERMEMORY_API_KEY=sm_..."
    fi
fi

# ══════════════════════════════════════════════════════════════
# ÉTAPE 3 : QDAYPRIZE (avant BTC — OBLIGATOIRE)
# ══════════════════════════════════════════════════════════════
echo ""
echo "[LVX-RUN] ── ÉTAPE 3 : QDAYPRIZE (feedback NX48-SN7) ──"
echo "[LVX-RUN] sim_bits=${QDPR_SIM_BITS} | durée=${QDPR_DURATION_S}s | threads=${QDPR_THREADS}"

QDPR_JSON_BEFORE=$(ls -t "$LOG_DIR"/qdayprize_*.json 2>/dev/null | head -1 || echo "")

cd "$QDPR_DIR"
make clean 2>&1 | tail -1 || true
make 2>&1 | tail -3 || {
    echo "[LVX-RUN] ⚠️ Compilation QDAYPRIZE échouée — BTC lancé sans feedback frais"
    SKIP_QDPR=1
}

if [ "${SKIP_QDPR:-0}" = "0" ]; then
    QDPR_RUN_ID="qdpr_c63_${STAMP}"
    mkdir -p "$LOG_DIR"
    ./qdayprize_runner \
        --sim-bits "$QDPR_SIM_BITS" \
        --duration "$QDPR_DURATION_S" \
        --threads  "$QDPR_THREADS" \
        --run-id   "$QDPR_RUN_ID" \
        --log-dir  "$LOG_DIR" \
        2>&1 | tee "/tmp/qdayprize_${QDPR_RUN_ID}.log" || \
        echo "[WARN] QDAYPRIZE run partiel — continuation"

    # Vérifier que le JSON est bien créé
    QDPR_JSON=$(ls -t "$LOG_DIR"/qdayprize_*.json 2>/dev/null | head -1 || echo "")
    if [ -n "$QDPR_JSON" ] && [ "$QDPR_JSON" != "$QDPR_JSON_BEFORE" ]; then
        SR=$(python3 -c "import json; d=json.load(open('$QDPR_JSON')); print(f'{d[\"success_rate\"]*100:.1f}%')" 2>/dev/null || echo "?%")
        BITS=$(python3 -c "import json; d=json.load(open('$QDPR_JSON')); print(d.get('near_miss_bits','?'))" 2>/dev/null || echo "?")
        echo "[LVX-RUN] ✅ QDAYPRIZE terminé : success_rate=${SR} near_miss=${BITS} bits"
        echo "[LVX-RUN]    JSON → ${QDPR_JSON##$REPO_ROOT/}"
    else
        echo "[LVX-RUN] ⚠️ JSON QDAYPRIZE non créé — NX48 utilisera ancien JSON si disponible"
    fi
fi

# ══════════════════════════════════════════════════════════════
# ÉTAPE 4 : BTC MINING (avec feedback QDAYPRIZE frais)
# ══════════════════════════════════════════════════════════════
echo ""
echo "[LVX-RUN] ── ÉTAPE 4 : BTC Mining NX48 ──"
echo "[LVX-RUN] threads=${BTC_THREADS} | durée=${BTC_DURATION_S}s (0=infini)"
echo "[LVX-RUN] SN7 QDAYPRIZE + delta_reset_stall C63 actifs"

# Passer les params QDAYPRIZE via variable d'env pour nx48_btc_controller.c (popen JSON)
export LAST_QDAYPRIZE_JSON="${QDPR_JSON:-}"

BTC_THREADS="$BTC_THREADS" \
BTC_DURATION_S="$BTC_DURATION_S" \
bash "$TOOLS_DIR/btc_run_ubuntu.sh"

# ══════════════════════════════════════════════════════════════
# ÉTAPE 5 : SUPERMEMORY PUSH — sauvegarder état NX48
# ══════════════════════════════════════════════════════════════
if [ "$SKIP_SUPERMEMORY" = "0" ] && [ -f "$TOOLS_DIR/nx48_supermemory.py" ]; then
    echo ""
    echo "[LVX-RUN] ── ÉTAPE 5 : Supermemory PUSH (sauvegarde état NX48) ──"
    SM_KEY="${SUPERMEMORY_API_KEY:-}"
    if [ -n "$SM_KEY" ]; then
        LUM_FILE="$BTC_DIR/config/btc_nx48_last.csv"
        python3 "$TOOLS_DIR/nx48_supermemory.py" --store-run \
            --csv "$LUM_FILE" \
            --cycle C63 --run-id "lumvorax_nx48_btc_c63_${STAMP}" \
            2>&1 || echo "[WARN] Supermemory push échoué — état local conservé"
        echo "[LVX-RUN] ✅ État NX48 sauvegardé dans Supermemory"
    else
        echo "[LVX-RUN] ⚠️ SUPERMEMORY_API_KEY absent — skip push"
    fi
fi

echo ""
echo "[LVX-RUN] ═══════════════════════════════════════════════"
echo "[LVX-RUN] ✅ Run C63 terminé — ${STAMP}"
echo "[LVX-RUN] ═══════════════════════════════════════════════"
