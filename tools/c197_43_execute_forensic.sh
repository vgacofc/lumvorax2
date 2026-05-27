#!/bin/bash
# C197.43 - Exécution capture ultra-forensique avec privilèges root
# Capture TOUS les ioctls + contenus buffers complets

set -e

echo "=== C197.43 - Capture Ultra-Forensique OpenCL→GPU ==="
echo "Timestamp: $(date '+%Y-%m-%d %H:%M:%S.%N')"
echo ""

# Vérifier que la bibliothèque existe
if [ ! -f /tmp/c197_43_ultra_forensic.so ]; then
    echo "❌ ERREUR: /tmp/c197_43_ultra_forensic.so n'existe pas"
    echo "Compilation nécessaire d'abord"
    exit 1
fi

# Créer répertoire forensique
echo "📁 Création répertoire forensique..."
sudo rm -rf /tmp/c197_43_forensic
sudo mkdir -p /tmp/c197_43_forensic
sudo chmod 777 /tmp/c197_43_forensic

# Vérifier que le test OpenCL existe
if [ ! -f tools/c197_10_opencl_simple_test ]; then
    echo "❌ ERREUR: tools/c197_10_opencl_simple_test n'existe pas"
    exit 1
fi

echo "✅ Répertoire créé: /tmp/c197_43_forensic"
echo ""

# Exécuter capture avec LD_PRELOAD
echo "🔬 Lancement capture forensique (timeout 5s)..."
echo "   Programme: tools/c197_10_opencl_simple_test"
echo "   LD_PRELOAD: /tmp/c197_43_ultra_forensic.so"
echo ""

# Exécuter avec sudo pour accès DRM complet
sudo bash -c "cd /home/lvx/LVX/lumvorax2 && \
    LD_PRELOAD=/tmp/c197_43_ultra_forensic.so \
    timeout 5 tools/c197_10_opencl_simple_test 2>&1" | tee /tmp/c197_43_execution.log

EXIT_CODE=${PIPESTATUS[0]}
echo ""
echo "Exit code: $EXIT_CODE"

# Analyser résultats
echo ""
echo "=== Analyse Résultats Capture ==="
echo ""

if [ -d /tmp/c197_43_forensic ]; then
    echo "📊 Fichiers capturés:"
    ls -lh /tmp/c197_43_forensic/ | tail -n +2
    echo ""
    
    FILE_COUNT=$(ls -1 /tmp/c197_43_forensic/ 2>/dev/null | wc -l)
    TOTAL_SIZE=$(du -sh /tmp/c197_43_forensic/ 2>/dev/null | cut -f1)
    
    echo "📈 Statistiques:"
    echo "   - Fichiers capturés: $FILE_COUNT"
    echo "   - Taille totale: $TOTAL_SIZE"
    echo ""
    
    # Vérifier forensic.log
    if [ -f /tmp/c197_43_forensic/forensic.log ]; then
        LOG_LINES=$(wc -l < /tmp/c197_43_forensic/forensic.log)
        echo "   - Lignes forensic.log: $LOG_LINES"
        echo ""
        echo "📝 Premiers événements capturés:"
        head -20 /tmp/c197_43_forensic/forensic.log
    else
        echo "⚠️  forensic.log non trouvé"
    fi
    
    echo ""
    echo "✅ Capture terminée avec succès"
    echo "📂 Données disponibles dans: /tmp/c197_43_forensic/"
else
    echo "❌ ERREUR: Répertoire /tmp/c197_43_forensic/ non créé"
    echo "La bibliothèque LD_PRELOAD n'a peut-être pas été chargée"
fi

echo ""
echo "=== Fin Capture C197.43 ==="

# Made with Bob
