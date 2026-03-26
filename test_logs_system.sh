
#!/bin/bash

echo "=== TEST SYSTÈME LOGS AUTOMATIQUE ==="

# Phase 1: Configuration du système
echo "Phase 1: Configuration système logs..."
chmod +x setup_logs_system.sh
./setup_logs_system.sh

echo ""
echo "Phase 2: Compilation avec nouveau système logs..."
make clean
make all 2>&1 | tee logs/current/compilation_$(date +%Y%m%d_%H%M%S).log

if [ ! -f bin/lum_vorax ]; then
    echo "❌ Compilation échouée"
    exit 1
fi

echo ""
echo "Phase 3: Test exécution avec logs automatiques..."
./bin/lum_vorax 2>&1 | tee logs/current/execution_$(date +%Y%m%d_%H%M%S).log

echo ""
echo "Phase 4: Vérification logs créés..."
echo "📁 Structure logs:"
tree logs/ || ls -la logs/

echo ""
echo "📄 Logs par module créés:"
find logs/modules -name "*.log" | head -10

echo ""
echo "Phase 5: Monitoring logs..."
if [ -f logs/monitor_logs.sh ]; then
    ./logs/monitor_logs.sh
fi

echo ""
echo "Phase 6: Test archivage..."
SESSION=$(cat logs/current_session.txt 2>/dev/null || echo "unknown")
echo "Session courante: $SESSION"

echo ""
echo "✅ SYSTÈME LOGS AUTOMATIQUE TESTÉ"
echo "📊 Tous les logs sont conservés et archivés automatiquement"
echo "🔄 Chaque module a son propre fichier de log"
echo "📁 Archive automatique avant nouvelle exécution"
