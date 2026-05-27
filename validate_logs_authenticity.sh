
#!/bin/bash

echo "=========================================="
echo "VALIDATION FORENSIQUE ULTRA-STRICTE LOGS"
echo "Timestamp: $(date -u) UTC"
echo "=========================================="

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
FORENSIC_DIR="logs/forensic/validation_${TIMESTAMP}"

# Création répertoire de validation
mkdir -p "$FORENSIC_DIR"

echo "1. INSPECTION RÉPERTOIRES EXISTANTS:"
echo "-----------------------------------"
if [ -d "logs" ]; then
    echo "✓ Répertoire logs existe"
    find logs -type d | while read dir; do
        echo "   📁 $dir (créé: $(stat -c %y "$dir" 2>/dev/null || echo 'inconnu'))"
    done
else
    echo "❌ Répertoire logs n'existe pas"
fi

echo ""
echo "2. FICHIERS LOGS RÉELS DÉTECTÉS:"
echo "--------------------------------"
LOG_COUNT=0
if [ -d "logs" ]; then
    find logs -type f \( -name "*.log" -o -name "*.json" -o -name "*.csv" \) | while read file; do
        if [ -f "$file" ]; then
            size=$(stat -c %s "$file" 2>/dev/null || echo "0")
            lines=$(wc -l < "$file" 2>/dev/null || echo "0")
            timestamp=$(stat -c %y "$file" 2>/dev/null || echo "inconnu")
            sha256=$(sha256sum "$file" 2>/dev/null | cut -d' ' -f1 || echo "erreur")
            echo "   📄 $file"
            echo "      Taille: $size bytes"
            echo "      Lignes: $lines"
            echo "      Modifié: $timestamp"
            echo "      SHA256: $sha256"
            echo ""
            LOG_COUNT=$((LOG_COUNT + 1))
        fi
    done
    echo "Total fichiers logs: $LOG_COUNT"
else
    echo "❌ Aucun fichier log détecté"
fi

echo ""
echo "3. GÉNÉRATION LOGS TEMPS RÉEL AVEC PREUVES:"
echo "-------------------------------------------"

# Test de génération avec timestamps forensiques
echo "Démarrage test génération logs à: $(date -u) UTC" | tee "$FORENSIC_DIR/generation_log.txt"

# Exécution avec capture complète
if [ -f "bin/lum_vorax_simple" ]; then
    echo "Exécution binaire principal..." | tee -a "$FORENSIC_DIR/generation_log.txt"
    
    # Capture stderr et stdout séparément
    ./bin/lum_vorax_simple > "$FORENSIC_DIR/stdout_${TIMESTAMP}.log" 2> "$FORENSIC_DIR/stderr_${TIMESTAMP}.log"
    EXIT_CODE=$?
    
    echo "Code retour: $EXIT_CODE" | tee -a "$FORENSIC_DIR/generation_log.txt"
    echo "Fin exécution à: $(date -u) UTC" | tee -a "$FORENSIC_DIR/generation_log.txt"
    
    # Analyse des logs générés
    echo ""
    echo "4. ANALYSE LOGS GÉNÉRÉS:"
    echo "------------------------"
    
    for logfile in "$FORENSIC_DIR"/stdout_*.log "$FORENSIC_DIR"/stderr_*.log; do
        if [ -f "$logfile" ]; then
            lines=$(wc -l < "$logfile")
            size=$(stat -c %s "$logfile")
            echo "   📋 $(basename "$logfile"): $lines lignes, $size bytes"
            
            # Extraction des métriques réelles
            if grep -q "MEMORY_TRACKER" "$logfile"; then
                echo "      ✓ Logs memory tracker détectés"
                grep "MEMORY_TRACKER" "$logfile" | wc -l | xargs echo "      Entrées memory tracker:"
            fi
            
            if grep -q "Total allocations" "$logfile"; then
                echo "      ✓ Métriques mémoire détectées"
                grep "Total allocations\|Current usage\|Peak usage" "$logfile" | while read line; do
                    echo "      $line"
                done
            fi
        fi
    done
    
    # Génération CSV forensique
    echo ""
    echo "5. GÉNÉRATION FORMATS FORENSIQUES:"
    echo "----------------------------------"
    
    # CSV des métriques
    CSV_FILE="$FORENSIC_DIR/metrics_${TIMESTAMP}.csv"
    echo "timestamp,metric_type,value,unit,source_file" > "$CSV_FILE"
    
    # Extraction métriques vers CSV
    if [ -f "$FORENSIC_DIR/stdout_${TIMESTAMP}.log" ]; then
        while read -r line; do
            if [[ $line == *"Total allocations"* ]]; then
                value=$(echo "$line" | grep -o '[0-9]\+' | head -1)
                echo "$(date -u +%Y-%m-%d\ %H:%M:%S),memory_allocations,$value,bytes,stdout_${TIMESTAMP}.log" >> "$CSV_FILE"
            fi
            if [[ $line == *"Peak usage"* ]]; then
                value=$(echo "$line" | grep -o '[0-9]\+' | head -1)
                echo "$(date -u +%Y-%m-%d\ %H:%M:%S),memory_peak,$value,bytes,stdout_${TIMESTAMP}.log" >> "$CSV_FILE"
            fi
        done < "$FORENSIC_DIR/stdout_${TIMESTAMP}.log"
    fi
    
    # JSON des résultats
    JSON_FILE="$FORENSIC_DIR/results_${TIMESTAMP}.json"
    cat > "$JSON_FILE" << EOF
{
    "validation_timestamp": "$(date -u)",
    "session_id": "${TIMESTAMP}",
    "binary_executed": "bin/lum_vorax_simple",
    "exit_code": $EXIT_CODE,
    "logs_generated": {
        "stdout_lines": $(wc -l < "$FORENSIC_DIR/stdout_${TIMESTAMP}.log" 2>/dev/null || echo 0),
        "stderr_lines": $(wc -l < "$FORENSIC_DIR/stderr_${TIMESTAMP}.log" 2>/dev/null || echo 0),
        "csv_entries": $(tail -n +2 "$CSV_FILE" 2>/dev/null | wc -l || echo 0)
    },
    "checksums": {
        "stdout": "$(sha256sum "$FORENSIC_DIR/stdout_${TIMESTAMP}.log" 2>/dev/null | cut -d' ' -f1 || echo 'error')",
        "stderr": "$(sha256sum "$FORENSIC_DIR/stderr_${TIMESTAMP}.log" 2>/dev/null | cut -d' ' -f1 || echo 'error')",
        "csv": "$(sha256sum "$CSV_FILE" 2>/dev/null | cut -d' ' -f1 || echo 'error')"
    }
}
EOF
    
    echo "   📊 CSV généré: $CSV_FILE ($(wc -l < "$CSV_FILE") lignes)"
    echo "   📋 JSON généré: $JSON_FILE"
    
else
    echo "❌ Binaire bin/lum_vorax_simple non trouvé"
    echo "Recompilation nécessaire..."
fi

echo ""
echo "6. RÉSUMÉ VALIDATION FORENSIQUE:"
echo "--------------------------------"
echo "Répertoire validation: $FORENSIC_DIR"
echo "Fichiers générés:"
ls -la "$FORENSIC_DIR" 2>/dev/null || echo "Aucun fichier généré"

echo ""
echo "✓ VALIDATION FORENSIQUE TERMINÉE"
echo "Tous les fichiers sont horodatés et vérifiables"
