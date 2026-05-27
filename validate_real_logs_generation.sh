
#!/bin/bash

echo "=== VALIDATION LOGS RÉELS GÉNÉRÉS ==="
echo "Timestamp: $(date -u)"

# Compilation avec logs forensiques
make clean
make all

echo ""
echo "=== LANCEMENT TEST AVEC LOGS OBLIGATOIRES ==="

# Créer répertoire logs horodaté
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
mkdir -p "logs/validation_reelle_${TIMESTAMP}"

# Lancement avec redirection obligatoire
echo "Démarrage exécution avec logs forensiques..."
./bin/lum_vorax_complete --progressive-stress-all 2>&1 | tee "logs/validation_reelle_${TIMESTAMP}/execution_complete.log"

echo ""
echo "=== VALIDATION PRÉSENCE LOGS ==="

# Compter les logs réels
LOG_FILES=$(find logs -name "*.log" -newer logs/forensic/test_execution.log 2>/dev/null | wc -l)
LOG_LINES=$(find logs -name "*.log" -exec wc -l {} \; 2>/dev/null | awk '{sum+=$1} END {print sum}')

echo "Nouveaux fichiers logs: $LOG_FILES"
echo "Total lignes logs: $LOG_LINES"

if [ "$LOG_LINES" -gt 100 ]; then
    echo "✅ LOGS RÉELS GÉNÉRÉS - $LOG_LINES lignes détectées"
else
    echo "❌ LOGS INSUFFISANTS - Seulement $LOG_LINES lignes"
fi

echo ""
echo "=== ÉCHANTILLON LOGS TEMPS RÉEL ==="
find "logs/validation_reelle_${TIMESTAMP}" -name "*.log" | head -3 | while read file; do
    echo "--- $file ---"
    tail -10 "$file"
    echo ""
done
#!/bin/bash

echo "=== VALIDATION LOGS RÉELS GÉNÉRÉS ==="
echo "Timestamp: $(date -u)"

# Compilation avec logs forensiques
make clean
make all

echo ""
echo "=== LANCEMENT TEST AVEC LOGS OBLIGATOIRES ==="

# Créer répertoire logs horodaté
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
mkdir -p "logs/validation_reelle_${TIMESTAMP}"

# Lancement avec redirection obligatoire
echo "Démarrage exécution avec logs forensiques..."
./bin/lum_vorax_complete --progressive-stress-all 2>&1 | tee "logs/validation_reelle_${TIMESTAMP}/execution_complete.log"

echo ""
echo "=== VALIDATION PRÉSENCE LOGS ==="

# Compter les logs réels
LOG_FILES=$(find logs -name "*.log" -newer logs/forensic/test_execution.log 2>/dev/null | wc -l)
LOG_LINES=$(find logs -name "*.log" -exec wc -l {} \; 2>/dev/null | awk '{sum+=$1} END {print sum}')

echo "Nouveaux fichiers logs: $LOG_FILES"
echo "Total lignes logs: $LOG_LINES"

if [ "$LOG_LINES" -gt 100 ]; then
    echo "✅ LOGS RÉELS GÉNÉRÉS - $LOG_LINES lignes détectées"
else
    echo "❌ LOGS INSUFFISANTS - Seulement $LOG_LINES lignes"
fi

echo ""
echo "=== ÉCHANTILLON LOGS TEMPS RÉEL ==="
find "logs/validation_reelle_${TIMESTAMP}" -name "*.log" | head -3 | while read file; do
    echo "--- $file ---"
    tail -10 "$file"
    echo ""
done
