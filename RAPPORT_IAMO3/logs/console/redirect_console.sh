#!/bin/bash

# Lecture de la session courante
if [ -f logs/console/current_session.txt ]; then
    SESSION_ID=$(cat logs/console/current_session.txt)
else
    TIMESTAMP=$(date '+%Y%m%d_%H%M%S')
    SESSION_ID="console_${TIMESTAMP}_$$"
    echo "$SESSION_ID" > logs/console/current_session.txt
fi

# Définition des fichiers de log
CONSOLE_LOG="logs/console/current/console_${SESSION_ID}.log"
ERROR_LOG="logs/console/current/errors_${SESSION_ID}.log"
COMBINED_LOG="logs/console/current/combined_${SESSION_ID}.log"

# Redirection avec tee pour garder l'affichage console ET sauvegarder
exec > >(tee -a "$CONSOLE_LOG")
exec 2> >(tee -a "$ERROR_LOG" >&2)

# Header du log
echo "[$(date)] === DÉBUT SESSION CONSOLE $SESSION_ID ===" | tee -a "$COMBINED_LOG"
echo "Console stdout: $CONSOLE_LOG" | tee -a "$COMBINED_LOG"
echo "Console stderr: $ERROR_LOG" | tee -a "$COMBINED_LOG"
echo "Log combiné: $COMBINED_LOG" | tee -a "$COMBINED_LOG"
echo "================================================" | tee -a "$COMBINED_LOG"
