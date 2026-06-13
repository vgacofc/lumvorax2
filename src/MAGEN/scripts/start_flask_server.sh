#!/bin/bash

# Script de démarrage serveur Flask ARC-AGI-3
# Auteur: Claude Pilot MAGEN
# Date: 2026-06-12

set -e

MAGEN_ROOT="/home/lvx/LVX/lumvorax2/src/MAGEN"
FLASK_DIR="$MAGEN_ROOT/data/arc_agi_3/ARC-AGI-3-Agents"
VENV_PYTHON="$MAGEN_ROOT/venv/bin/python"
LOG_FILE="/tmp/flask_arc_server.log"
PID_FILE="/tmp/flask_arc_server.pid"

echo "🚀 CLAUDE PILOT - Démarrage serveur Flask ARC-AGI-3"
echo "=================================================="

# Vérifier que le venv existe
if [ ! -f "$VENV_PYTHON" ]; then
    echo "❌ ERREUR: Python venv non trouvé à $VENV_PYTHON"
    echo "Exécutez d'abord: python3 -m venv venv && source venv/bin/activate && pip install -r requirements.txt"
    exit 1
fi

# Vérifier que le répertoire Flask existe
if [ ! -d "$FLASK_DIR" ]; then
    echo "❌ ERREUR: Répertoire Flask non trouvé à $FLASK_DIR"
    exit 1
fi

# Tuer processus Flask existant si présent
if [ -f "$PID_FILE" ]; then
    OLD_PID=$(cat "$PID_FILE")
    if ps -p "$OLD_PID" > /dev/null 2>&1; then
        echo "⚠️  Arrêt serveur Flask existant (PID $OLD_PID)..."
        kill "$OLD_PID" 2>/dev/null || true
        sleep 2
    fi
    rm -f "$PID_FILE"
fi

# Nettoyer ancien log
rm -f "$LOG_FILE"

# Démarrer serveur Flask
echo "🔧 Démarrage Flask sur http://localhost:8001..."
cd "$FLASK_DIR"

export SCHEME=http
export HOST=localhost
export PORT=8001

nohup "$VENV_PYTHON" -m flask run --port 8001 > "$LOG_FILE" 2>&1 &
FLASK_PID=$!

echo "$FLASK_PID" > "$PID_FILE"

# Attendre démarrage
echo "⏳ Attente démarrage serveur (5s)..."
sleep 5

# Vérifier que le processus tourne
if ps -p "$FLASK_PID" > /dev/null 2>&1; then
    echo "✅ Serveur Flask démarré avec succès !"
    echo "   PID: $FLASK_PID"
    echo "   URL: http://localhost:8001"
    echo "   Logs: $LOG_FILE"
    echo ""
    echo "📋 Premières lignes du log:"
    head -20 "$LOG_FILE"
    echo ""
    echo "🎯 Pour arrêter: kill $FLASK_PID"
else
    echo "❌ ERREUR: Serveur Flask n'a pas démarré"
    echo "📋 Contenu du log:"
    cat "$LOG_FILE"
    exit 1
fi

# Made with Bob
