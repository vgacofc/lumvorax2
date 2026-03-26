#!/bin/bash

echo "📺 === MONITORING CONSOLE TEMPS RÉEL ==="

if [ -f logs/console/current_session.txt ]; then
    SESSION_ID=$(cat logs/console/current_session.txt)
    CONSOLE_LOG="logs/console/current/console_${SESSION_ID}.log"
    ERROR_LOG="logs/console/current/errors_${SESSION_ID}.log"
    
    echo "Session active: $SESSION_ID"
    echo "Console log: $CONSOLE_LOG"
    echo "Error log: $ERROR_LOG"
    echo ""
    echo "--- DERNIÈRES LIGNES CONSOLE ---"
    tail -20 "$CONSOLE_LOG" 2>/dev/null || echo "Aucun log console trouvé"
    echo ""
    echo "--- DERNIÈRES ERREURS ---"
    tail -10 "$ERROR_LOG" 2>/dev/null || echo "Aucune erreur trouvée"
else
    echo "❌ Aucune session console active"
fi
