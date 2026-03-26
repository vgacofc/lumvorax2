
#!/bin/bash

# Système de logs automatique avec archivage
# Date: $(date +%Y%m%d_%H%M%S)

set -euo pipefail

echo "=== CONFIGURATION SYSTÈME LOGS AUTOMATIQUE ==="

# Timestamp pour cette session
SESSION=$(date +%Y%m%d_%H%M%S)
echo "Session: $SESSION"

# Création structure logs complète
echo "Création structure logs..."
mkdir -p logs/{current,archive,modules,compilation,execution,stress,optimization,crypto,forensic}
mkdir -p logs/modules/{lum_core,vorax_operations,parser,binary_converter,logger,optimization,parallel,metrics,crypto,persistence,debug,advanced_calculations,complex_modules}

# Archivage logs existants
echo "Archivage logs existants..."
if [ "$(ls -A logs/ 2>/dev/null | grep -v current | grep -v archive | grep -v modules)" ]; then
    mv logs/*.log logs/archive/ 2>/dev/null || true
    mv logs/*.json logs/archive/ 2>/dev/null || true
fi

# Création fichiers de logs par module
echo "Initialisation logs par module..."
for module in lum_core vorax_operations parser binary_converter logger optimization parallel metrics crypto persistence debug advanced_calculations complex_modules; do
    touch "logs/modules/$module/${module}_${SESSION}.log"
    echo "[$(date)] === LOG MODULE $module SESSION $SESSION ===" > "logs/modules/$module/${module}_${SESSION}.log"
done

# Fichiers logs système
touch "logs/current/system_${SESSION}.log"
touch "logs/current/compilation_${SESSION}.log"
touch "logs/current/execution_${SESSION}.log"
touch "logs/current/forensic_${SESSION}.log"

echo "[$(date)] === SYSTÈME LOGS INITIALISÉ SESSION $SESSION ===" > "logs/current/system_${SESSION}.log"

# Script de monitoring en temps réel
cat > logs/monitor_logs.sh << 'MONITOR_EOF'
#!/bin/bash
# Monitoring logs en temps réel

SESSION_FILE="logs/current_session.txt"
if [ -f "$SESSION_FILE" ]; then
    SESSION=$(cat "$SESSION_FILE")
else
    SESSION=$(date +%Y%m%d_%H%M%S)
    echo "$SESSION" > "$SESSION_FILE"
fi

echo "=== MONITORING LOGS SESSION $SESSION ==="
echo "Logs disponibles:"
find logs/modules -name "*.log" | head -10
echo ""
echo "Logs récents (5 dernières lignes par module):"
for log in logs/modules/*/*.log; do
    if [ -f "$log" ]; then
        echo "--- $(basename "$log") ---"
        tail -5 "$log" 2>/dev/null || echo "Log vide"
        echo ""
    fi
done
MONITOR_EOF

chmod +x logs/monitor_logs.sh

# Sauvegarde session courante
echo "$SESSION" > logs/current_session.txt

echo "✅ Système logs configuré avec succès"
echo "📁 Structure créée dans logs/"
echo "🔄 Session: $SESSION"
echo "📊 Monitoring disponible: ./logs/monitor_logs.sh"
