
#!/bin/bash

echo "=== CONFIGURATION REDIRECTION CONSOLE VERS LOGS ==="

# Création structure de logs console
mkdir -p logs/console/{current,archive,sessions}

# Génération timestamp unique pour cette session
TIMESTAMP=$(date '+%Y%m%d_%H%M%S')
SESSION_ID="console_${TIMESTAMP}_$$"

echo "📁 Session console: $SESSION_ID"

# Fichiers de log pour cette session
CONSOLE_LOG="logs/console/current/console_${SESSION_ID}.log"
ERROR_LOG="logs/console/current/errors_${SESSION_ID}.log"
COMBINED_LOG="logs/console/current/combined_${SESSION_ID}.log"

# Sauvegarde session actuelle
echo "$SESSION_ID" > logs/console/current_session.txt

# Script de redirection automatique
cat > logs/console/redirect_console.sh << 'EOF'
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
EOF

chmod +x logs/console/redirect_console.sh

# Script d'archivage des logs
cat > logs/console/archive_logs.sh << 'EOF'
#!/bin/bash

echo "🗄️ === ARCHIVAGE LOGS CONSOLE ==="

ARCHIVE_DIR="logs/console/archive/$(date '+%Y%m%d_%H%M%S')"
mkdir -p "$ARCHIVE_DIR"

# Déplacement des logs actuels vers archive
if [ -d logs/console/current ]; then
    mv logs/console/current/*.log "$ARCHIVE_DIR/" 2>/dev/null || true
    echo "✅ Logs archivés dans: $ARCHIVE_DIR"
    
    # Résumé de l'archivage
    echo "📊 RÉSUMÉ ARCHIVAGE:" >> "$ARCHIVE_DIR/archive_summary.txt"
    echo "Date: $(date)" >> "$ARCHIVE_DIR/archive_summary.txt"
    echo "Fichiers archivés: $(ls -1 "$ARCHIVE_DIR"/*.log 2>/dev/null | wc -l)" >> "$ARCHIVE_DIR/archive_summary.txt"
    echo "Taille totale: $(du -sh "$ARCHIVE_DIR" | cut -f1)" >> "$ARCHIVE_DIR/archive_summary.txt"
fi

echo "🔄 Nouvelle session console prête"
EOF

chmod +x logs/console/archive_logs.sh

# Script de monitoring des logs en temps réel
cat > logs/console/monitor_console.sh << 'EOF'
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
EOF

chmod +x logs/console/monitor_console.sh

# Informations de configuration
echo "✅ SYSTÈME DE REDIRECTION CONSOLE CONFIGURÉ"
echo ""
echo "📁 Structure créée:"
echo "   logs/console/current/     - Logs session courante"
echo "   logs/console/archive/     - Logs sessions précédentes"
echo "   logs/console/sessions/    - Métadonnées sessions"
echo ""
echo "🔧 Scripts disponibles:"
echo "   logs/console/redirect_console.sh  - Active la redirection"
echo "   logs/console/archive_logs.sh      - Archive les logs actuels"
echo "   logs/console/monitor_console.sh   - Surveille les logs en temps réel"
echo ""
echo "🚀 Session console actuelle: $SESSION_ID"
echo "📄 Logs sauvegardés dans:"
echo "   Console: $CONSOLE_LOG"
echo "   Erreurs: $ERROR_LOG"
echo "   Combiné: $COMBINED_LOG"
