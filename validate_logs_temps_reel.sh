
#!/bin/bash

echo "=== VALIDATION LOGS TEMPS RÉEL ==="
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
echo "[$TIMESTAMP] Début validation logs temps réel"

# Vérification création logs
if [ -d "logs/temps_reel" ]; then
    echo "[$TIMESTAMP] Structure logs/temps_reel existe"
    LOG_COUNT=$(find logs/temps_reel -name "*.log" | wc -l)
    echo "[$TIMESTAMP] Nombre de logs temps réel: $LOG_COUNT"
    
    if [ $LOG_COUNT -gt 0 ]; then
        echo "[$TIMESTAMP] Logs temps réel générés avec succès"
        
        # Vérification contenu logs
        echo "[$TIMESTAMP] ÉCHANTILLON LOGS TEMPS RÉEL:"
        find logs/temps_reel -name "*.log" | head -3 | while read log_file; do
            echo "[$TIMESTAMP] $log_file:"
            head -5 "$log_file" | sed "s/^/[$TIMESTAMP]     /"
        done
    else
        echo "[$TIMESTAMP] ERREUR: Aucun log temps réel généré"
        exit 1
    fi
else
    echo "[$TIMESTAMP] ERREUR: Répertoire logs/temps_reel manquant"
    exit 1
fi

echo "[$TIMESTAMP] VALIDATION LOGS TEMPS RÉEL TERMINÉE"
