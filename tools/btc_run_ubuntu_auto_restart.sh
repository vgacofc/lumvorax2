#!/usr/bin/env bash
# tools/btc_run_ubuntu_auto_restart.sh
# Wrapper qui relance automatiquement btc_run_ubuntu.sh après crash/SIGSEGV/exit
# Sauvegarde le best_leading entre les runs via le LUM persistant
#
# USAGE :
#   doppler run --config dev_lumvorax -- bash tools/btc_run_ubuntu_auto_restart.sh
#   (CTRL+C 2× rapide pour arrêter définitivement)

set -u

REPO_ROOT="${REPO_ROOT:-$HOME/LVX/lumvorax2}"
cd "$REPO_ROOT"

ITER=0
MAX_ITER="${MAX_ITER:-9999}"     # nombre max de relances (défaut illimité quasi)
MIN_RUN_SECONDS=30                # si run < 30s = bug, on attend 60s avant retry
LAST_BEST=0
GLOBAL_LOG="$HOME/btc_auto_restart_$(date -u +%Y%m%dT%H%M%SZ).log"

trap 'echo "[AUTO-RESTART] Arrêt manuel demandé"; exit 0' INT

echo "[AUTO-RESTART] ════════════════════════════════════════════════"  | tee -a "$GLOBAL_LOG"
echo "[AUTO-RESTART] BTC mining auto-restart wrapper démarré"             | tee -a "$GLOBAL_LOG"
echo "[AUTO-RESTART] PID parent : $$"                                     | tee -a "$GLOBAL_LOG"
echo "[AUTO-RESTART] Log global : $GLOBAL_LOG"                            | tee -a "$GLOBAL_LOG"
echo "[AUTO-RESTART] CTRL+C 2× rapide pour arrêt définitif"               | tee -a "$GLOBAL_LOG"
echo "[AUTO-RESTART] ════════════════════════════════════════════════"  | tee -a "$GLOBAL_LOG"

while [ $ITER -lt $MAX_ITER ]; do
    ITER=$((ITER + 1))
    T_START=$(date +%s)

    # Lecture best_leading actuel depuis CSV
    if [ -f "src/advanced_calculations/bitcoin_quantum_mining/config/btc_nx48_last.csv" ]; then
        CURRENT_BEST=$(awk -F',' 'NR==2 {print $7}' src/advanced_calculations/bitcoin_quantum_mining/config/btc_nx48_last.csv 2>/dev/null || echo "?")
    else
        CURRENT_BEST="?"
    fi

    echo ""                                                                 | tee -a "$GLOBAL_LOG"
    echo "[AUTO-RESTART] ─── Itération #$ITER — best_leading actuel: $CURRENT_BEST ───" | tee -a "$GLOBAL_LOG"
    echo "[AUTO-RESTART] $(date -u +%Y-%m-%dT%H:%M:%SZ)"                    | tee -a "$GLOBAL_LOG"

    # Hygiène RAM : tuer Chrome/Firefox si présents (évite throttle watchdog)
    KILLED=$(pkill -9 -c chrome 2>/dev/null || echo 0)
    KILLED2=$(pkill -9 -c firefox 2>/dev/null || echo 0)
    [ "$KILLED" != "0" ] || [ "$KILLED2" != "0" ] && echo "[AUTO-RESTART] Tué Chrome=$KILLED Firefox=$KILLED2 (hygiène RAM)" | tee -a "$GLOBAL_LOG"
    sync

    FREE_MB=$(awk '/MemAvailable/ {printf "%d", $2/1024}' /proc/meminfo)
    echo "[AUTO-RESTART] RAM disponible: ${FREE_MB} MB"                     | tee -a "$GLOBAL_LOG"

    # Lancement du runner BTC réel
    bash tools/btc_run_ubuntu.sh 2>&1 | tee -a "$GLOBAL_LOG"
    EXIT_CODE=$?

    T_END=$(date +%s)
    DURATION=$((T_END - T_START))

    echo ""                                                                 | tee -a "$GLOBAL_LOG"
    echo "[AUTO-RESTART] ─── Fin itération #$ITER ───"                       | tee -a "$GLOBAL_LOG"
    echo "[AUTO-RESTART] exit_code=$EXIT_CODE durée=${DURATION}s"           | tee -a "$GLOBAL_LOG"

    # Diagnostic exit_code
    case $EXIT_CODE in
        0)   REASON="exit_normal" ;;
        130) REASON="ctrl_c (arrêt manuel — sortie définitive)" ; break ;;
        139) REASON="SIGSEGV (segmentation fault)" ;;
        134) REASON="SIGABRT (assertion ou abort())" ;;
        137) REASON="SIGKILL (OOM ou kill -9)" ;;
        *)   REASON="exit_$EXIT_CODE" ;;
    esac
    echo "[AUTO-RESTART] Cause : $REASON"                                   | tee -a "$GLOBAL_LOG"

    # Backup du LUM si record amélioré
    if [ -f "src/advanced_calculations/bitcoin_quantum_mining/config/btc_nx48_last.lum" ]; then
        NEW_BEST=$(awk -F',' 'NR==2 {print $7}' src/advanced_calculations/bitcoin_quantum_mining/config/btc_nx48_last.csv 2>/dev/null || echo 0)
        if [ "$NEW_BEST" != "?" ] && [ "$NEW_BEST" -gt "$LAST_BEST" ] 2>/dev/null; then
            BACKUP_FILE="$HOME/btc_nx48_record_${NEW_BEST}bits_iter${ITER}_$(date -u +%Y%m%dT%H%M%SZ).lum"
            cp "src/advanced_calculations/bitcoin_quantum_mining/config/btc_nx48_last.lum" "$BACKUP_FILE"
            echo "[AUTO-RESTART] 🏆 NOUVEAU RECORD : $NEW_BEST bits — backup → $BACKUP_FILE" | tee -a "$GLOBAL_LOG"
            LAST_BEST=$NEW_BEST
        fi
    fi

    # Si crash très rapide (<30s), attendre avant de relancer (évite boucle infernale)
    if [ $DURATION -lt $MIN_RUN_SECONDS ]; then
        WAIT=60
        echo "[AUTO-RESTART] Run trop court (${DURATION}s < ${MIN_RUN_SECONDS}s) — attente ${WAIT}s avant retry" | tee -a "$GLOBAL_LOG"
        sleep $WAIT
    else
        echo "[AUTO-RESTART] Attente 5s avant relance..."                   | tee -a "$GLOBAL_LOG"
        sleep 5
    fi
done

echo "[AUTO-RESTART] Limite max d'itérations atteinte ($MAX_ITER)"          | tee -a "$GLOBAL_LOG"
