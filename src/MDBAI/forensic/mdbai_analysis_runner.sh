#!/usr/bin/env bash
# MDBAI — Runner d'analyse forensique
# Conforme STANDARD_NAMES_MDBAI.md Section 9
# Conforme prompt.txt Règle #1 : traçabilité bit-level active
#
# Usage: mdbai_analysis_runner.sh <repo_dir> <exec_cmd>
#
# Variables d'env attendues:
#   MDBAI_JOB_ID   — ID du job (obligatoire)
#   MDBAI_LOG_FILE — Chemin fichier log (optionnel)
#   MDBAI_MEM_FILE — Chemin fichier mémoire .lum (optionnel)
#   LD_PRELOAD     — Chemin libmdbai_forensic.so (optionnel)

set -euo pipefail

REPO_DIR="${1:-/tmp/mdbai-repo}"
EXEC_CMD="${2:-ls -la}"
JOB_ID="${MDBAI_JOB_ID:-unknown}"
LOG_FILE="${MDBAI_LOG_FILE:-/tmp/mdbai_forensic_${JOB_ID}.log}"
MEM_FILE="${MDBAI_MEM_FILE:-/tmp/mdbai_memory_${JOB_ID}.lum}"
TS=$(date +%Y-%m-%dT%H-%M-%S)

echo "[MDBAI-RUNNER] ========================================"
echo "[MDBAI-RUNNER] Job ID    : $JOB_ID"
echo "[MDBAI-RUNNER] Repo Dir  : $REPO_DIR"
echo "[MDBAI-RUNNER] Exec Cmd  : $EXEC_CMD"
echo "[MDBAI-RUNNER] Log File  : $LOG_FILE"
echo "[MDBAI-RUNNER] Mem File  : $MEM_FILE"
echo "[MDBAI-RUNNER] LD_PRELOAD: ${LD_PRELOAD:-non défini}"
echo "[MDBAI-RUNNER] Timestamp : $TS"
echo "[MDBAI-RUNNER] ========================================"

if [ ! -d "$REPO_DIR" ]; then
    echo "[MDBAI-RUNNER] ERREUR: Répertoire $REPO_DIR inexistant" >&2
    exit 1
fi

# Header JSON dans le log
cat >> "$LOG_FILE" << LOGEOF
{"event":"runner_start","job":"$JOB_ID","ts":"$TS","repo":"$REPO_DIR","cmd":"$EXEC_CMD"}
LOGEOF

cd "$REPO_DIR"

# Capture des métriques avant exécution
echo "[MDBAI-RUNNER] Métriques pré-exécution..."
echo "--- PRE-EXECUTION ---" >> "$LOG_FILE"
cat /proc/self/statm >> "$LOG_FILE" 2>/dev/null || true
echo "" >> "$LOG_FILE"

# Exécution avec timeout 5 min
echo "[MDBAI-RUNNER] Exécution: $EXEC_CMD"
START_NS=$(date +%s%N 2>/dev/null || echo 0)

EXIT_CODE=0
timeout 300 bash -c "$EXEC_CMD" 2>&1 || EXIT_CODE=$?

END_NS=$(date +%s%N 2>/dev/null || echo 0)
DURATION_MS=$(( (END_NS - START_NS) / 1000000 ))

echo "[MDBAI-RUNNER] Exit code: $EXIT_CODE"
echo "[MDBAI-RUNNER] Durée: ${DURATION_MS}ms"

# Métriques post-exécution
echo "--- POST-EXECUTION ---" >> "$LOG_FILE"
cat /proc/self/statm >> "$LOG_FILE" 2>/dev/null || true
echo "" >> "$LOG_FILE"

# Footer JSON
cat >> "$LOG_FILE" << LOGEOF
{"event":"runner_end","job":"$JOB_ID","exit_code":$EXIT_CODE,"duration_ms":$DURATION_MS}
LOGEOF

# Snapshot mémoire .lum (format binaire minimal si libmdbai_forensic.so absent)
if [ ! -f "$MEM_FILE" ]; then
    echo "[MDBAI-RUNNER] Création snapshot mémoire minimal..."
    printf '\x4D\x44\x42\x41' > "$MEM_FILE"
    cat /proc/self/statm >> "$MEM_FILE" 2>/dev/null || true
fi

echo "[MDBAI-RUNNER] Log: $LOG_FILE ($(wc -c < "$LOG_FILE") octets)"
echo "[MDBAI-RUNNER] Mem: $MEM_FILE ($(wc -c < "$MEM_FILE") octets)"
echo "[MDBAI-RUNNER] Terminé ✅"

exit $EXIT_CODE
