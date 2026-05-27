#!/usr/bin/env fish
# ====================================================================
# C128 — PIPELINE UBUNTU COMPLET — VALIDATION BIT-LEVEL + TUNE_FULL
# ====================================================================
# Auteur : Replit Agent — cycle C128 — 29 avril 2026
# Cible  : Ubuntu i5-8265U Kaby Lake 8 threads, GPU UHD 620 OpenCL
# Repo   : /home/lvx/LVX/lumvorax2 (HEAD attendu : >= 4bc2bb3 chatC127)
#
# But : exécuter de A à Z toutes les validations restantes des
# rapports C125, C126, C127 SANS rien laisser pour plus tard, et
# produire les artefacts forensic exploitables (signal filtré).
#
# Usage :
#   cd /home/lvx/LVX/lumvorax2
#   doppler run --config dev_lumvorax -- fish src/advanced_calculations/bitcoin_quantum_mining/scripts/c128_ubuntu_full.fish
#
# OU sans doppler :
#   fish src/advanced_calculations/bitcoin_quantum_mining/scripts/c128_ubuntu_full.fish
#
# Variables modifiables :
#   set DURATION_S 600   # mainnet long, def=600
#   set THREADS 8        # def=8
#   set TUNE_DEEP 1      # 1=sweep ASIC complet 5s/combo (16 min), 0=skip
#
# Tout est journalise dans logs/c128_ubuntu/ pour analyse forensic
# et commit ulterieur via "git add logs/c128_ubuntu && git push".
# ====================================================================

set -l ROOT (pwd)
set -l BTC_DIR "src/advanced_calculations/bitcoin_quantum_mining"
set -l LOG_DIR "$BTC_DIR/logs/c128_ubuntu"
set -l TS (date +%Y%m%dT%H%M%S)
set -l RUN_ID "c128_$TS"

# Defaults (override via environment)
test -z "$DURATION_S"; and set -g DURATION_S 600
test -z "$THREADS"; and set -g THREADS 8
test -z "$TUNE_DEEP"; and set -g TUNE_DEEP 1

mkdir -p $LOG_DIR/{baseline,tuned,bit_snapshot,signal,artifacts}

echo "============================================================"
echo "C128 UBUNTU PIPELINE — RUN $RUN_ID"
echo "============================================================"
echo "ROOT       : $ROOT"
echo "BTC_DIR    : $BTC_DIR"
echo "LOG_DIR    : $LOG_DIR"
echo "DURATION_S : $DURATION_S"
echo "THREADS    : $THREADS"
echo "TUNE_DEEP  : $TUNE_DEEP"
echo "============================================================"

# --------------------------------------------------------------------
# Etape 0 — sync git + verifications environnement
# --------------------------------------------------------------------
echo ""
echo "[ETAPE 0] git pull + verifications environnement"
git fetch origin 2>&1 | tail -3
git reset --hard origin/main 2>&1 | tail -2
echo "  HEAD : "(git log --oneline -1)

# Hardware
echo "  CPU  : "(grep -m1 "model name" /proc/cpuinfo | cut -d: -f2 | xargs)
echo "  RAM  : "(free -h | awk '/^Mem:/ {print $2" total, "$7" available"}')
echo "  Disk : "(df -h $ROOT | awk 'NR==2 {print $4" libre / "$2" total"}')
test -e /dev/dri/renderD128
and echo "  GPU  : DRI present"
or  echo "  GPU  : non detecte"

# --------------------------------------------------------------------
# Etape 1 — compilation propre
# --------------------------------------------------------------------
echo ""
echo "[ETAPE 1] make clean + make -j$THREADS dans $BTC_DIR"
pushd $BTC_DIR >/dev/null
make clean 2>&1 | tail -3
make -j$THREADS 2>&1 | tee $ROOT/$LOG_DIR/artifacts/build_$RUN_ID.log | tail -10
test -x ./btc_mining_runner
or begin
    echo "ECHEC : btc_mining_runner non produit. Voir $LOG_DIR/artifacts/build_$RUN_ID.log"
    popd >/dev/null
    exit 1
end
echo "  Binaire : "(ls -lh ./btc_mining_runner | awk '{print $5" "$9}')
popd >/dev/null

# --------------------------------------------------------------------
# Etape 2 — run BASELINE (sans TUNE_FULL, granularite PAGE)
# --------------------------------------------------------------------
echo ""
echo "[ETAPE 2] BASELINE $DURATION_S s thread=$THREADS gran=PAGE"
pushd $BTC_DIR >/dev/null
env BTC_MEM_TRACE=1 \
    BTC_MEM_TRACE_GRANULARITY=page \
    BTC_LUM_LOG=1 \
    LUM_LOGGING=1 \
    timeout (math "$DURATION_S + 60") \
    ./btc_mining_runner --mode BENCHMARK --threads $THREADS \
        --duration-s $DURATION_S \
        --log-dir $ROOT/$LOG_DIR/baseline \
        --run-id (string sub -l 8 baseline_$RUN_ID) \
    2>&1 | tee $ROOT/$LOG_DIR/baseline/stdout_$RUN_ID.log | tail -30
popd >/dev/null

# --------------------------------------------------------------------
# Etape 3 — run TUNED (TUNE_FULL=1, granularite BIT-1b)
# --------------------------------------------------------------------
echo ""
echo "[ETAPE 3] TUNED $DURATION_S s thread=$THREADS TUNE_FULL=1 gran=BIT"
if test "$TUNE_DEEP" = 1
    set -l SWEEP_OVERHEAD_S 960
    echo "  Sweep deep : ~$SWEEP_OVERHEAD_S s overhead avant mining (192 combos × 5 s)"
else
    set -l SWEEP_OVERHEAD_S 96
    echo "  Sweep quick : ~$SWEEP_OVERHEAD_S s overhead (192 combos × 0.5 s)"
end

pushd $BTC_DIR >/dev/null
env BTC_MEM_TRACE=1 \
    BTC_MEM_TRACE_GRANULARITY=bit \
    BTC_TUNE_FULL=1 \
    BTC_LUM_LOG=1 \
    LUM_LOGGING=1 \
    timeout (math "$DURATION_S + $SWEEP_OVERHEAD_S + 120") \
    ./btc_mining_runner --mode BENCHMARK --threads $THREADS \
        --duration-s $DURATION_S \
        --log-dir $ROOT/$LOG_DIR/tuned \
        --run-id (string sub -l 8 tuned_$RUN_ID) \
    2>&1 | tee $ROOT/$LOG_DIR/tuned/stdout_$RUN_ID.log | tail -50
popd >/dev/null

# --------------------------------------------------------------------
# Etape 4 — verifications artefacts BIT-LEVEL
# --------------------------------------------------------------------
echo ""
echo "[ETAPE 4] Verification artefacts BIT-LEVEL"
set -l BIT_FILES (find $LOG_DIR/tuned -name "*BIT-1b*" -type f 2>/dev/null)
echo "  Fichiers BIT-1b trouves : "(count $BIT_FILES)
for f in $BIT_FILES
    echo "    "(ls -lh $f | awk '{print $5"  "$9}')
end

# Rapatrier le snapshot BIT principal vers bit_snapshot/ pour archivage
test (count $BIT_FILES) -gt 0
and cp $BIT_FILES[1] $LOG_DIR/bit_snapshot/

# --------------------------------------------------------------------
# Etape 5 — application du filtre signal/bruit C128-A10
# --------------------------------------------------------------------
echo ""
echo "[ETAPE 5] Filtre signal/bruit C128-A10"
test -f tools/btc_log_signal_filter.py
and begin
    for log in $LOG_DIR/baseline/stdout_$RUN_ID.log $LOG_DIR/tuned/stdout_$RUN_ID.log
        test -f $log
        and python3 tools/btc_log_signal_filter.py $log 2>&1 | tail -15
    end
    # forensic engine metrics si present
    set -l METRICS (find $LOG_DIR -name "btc_qm_engine_metrics.log" 2>/dev/null)
    for m in $METRICS
        echo "  Filtrage forensic : $m"
        python3 tools/btc_log_signal_filter.py $m 2>&1 | tail -10
    end
    mv $LOG_DIR/**/*.signal.csv $LOG_DIR/signal/ 2>/dev/null
    mv $LOG_DIR/**/*.noise.summary.csv $LOG_DIR/signal/ 2>/dev/null
end
or echo "  ATTENTION : tools/btc_log_signal_filter.py absent (run 'git pull' d'abord)"

# --------------------------------------------------------------------
# Etape 6 — comparaison hashrate baseline vs tuned
# --------------------------------------------------------------------
echo ""
echo "[ETAPE 6] Comparaison hashrate"
set -l HR_BASE (grep "Hashrate.*MH/s" $LOG_DIR/baseline/stdout_$RUN_ID.log | tail -1 | grep -oE '[0-9]+\.[0-9]+' | head -1)
set -l HR_TUNED (grep "Hashrate.*MH/s" $LOG_DIR/tuned/stdout_$RUN_ID.log | tail -1 | grep -oE '[0-9]+\.[0-9]+' | head -1)
set -l BB_BASE (grep "Best leading" $LOG_DIR/baseline/stdout_$RUN_ID.log | tail -1 | grep -oE '[0-9]+' | head -1)
set -l BB_TUNED (grep "Best leading" $LOG_DIR/tuned/stdout_$RUN_ID.log | tail -1 | grep -oE '[0-9]+' | head -1)

echo "  Baseline : hashrate=$HR_BASE MH/s | best_leading=$BB_BASE bits"
echo "  Tuned    : hashrate=$HR_TUNED MH/s | best_leading=$BB_TUNED bits"
if test -n "$HR_BASE" -a -n "$HR_TUNED"
    set -l GAIN (math "round(($HR_TUNED - $HR_BASE) / $HR_BASE * 10000) / 100")
    echo "  GAIN     : $GAIN %"
end

# --------------------------------------------------------------------
# Etape 7 — resume artefacts produits (pour git add)
# --------------------------------------------------------------------
echo ""
echo "[ETAPE 7] Artefacts produits ($LOG_DIR)"
du -sh $LOG_DIR/* 2>/dev/null
echo ""
echo "Pour committer :"
echo "  cd $ROOT"
echo "  git add $LOG_DIR/"
echo "  git commit -m \"C128 ubuntu pipeline run $RUN_ID hashrate baseline=$HR_BASE tuned=$HR_TUNED gain=$GAIN%\""
echo "  git push origin main"
echo ""
echo "============================================================"
echo "C128 PIPELINE TERMINE — RUN $RUN_ID"
echo "============================================================"
