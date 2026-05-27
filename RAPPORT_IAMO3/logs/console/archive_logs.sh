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
