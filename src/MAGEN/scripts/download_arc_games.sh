#!/bin/bash
# Script de téléchargement des jeux ARC-AGI-3 officiels

set -e

echo "🎮 Téléchargement des jeux ARC-AGI-3"
echo "===================================="

# Répertoires
MAGEN_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GAMES_DIR="$MAGEN_ROOT/data/arc_agi_3/arc_agi_3_games"
VENV_PYTHON="$MAGEN_ROOT/venv/bin/python"

echo "📁 Répertoire MAGEN: $MAGEN_ROOT"
echo "📁 Répertoire jeux: $GAMES_DIR"

# Créer le répertoire si nécessaire
mkdir -p "$GAMES_DIR"

# Utiliser arc_agi pour télécharger les jeux
echo ""
echo "📦 Téléchargement via arc_agi..."

cd "$MAGEN_ROOT"

# Script Python pour télécharger les jeux
$VENV_PYTHON << 'PYTHON_SCRIPT'
import os
import sys
from pathlib import Path

# Ajouter le chemin arc_agi
sys.path.insert(0, str(Path.cwd() / "data" / "arc_agi_3"))

try:
    from arc_agi import Arcade
    
    # Créer instance Arcade
    arcade = Arcade()
    
    # Les jeux sont automatiquement récupérés depuis l'API
    # Arcade a déjà fetché 25 environnements
    
    # Obtenir les environnements disponibles
    games = arcade.available_environments
    
    print(f"✅ {len(games)} jeux disponibles")
    
    # Créer un fichier metadata.json avec la liste
    games_dir = Path("data/arc_agi_3/arc_agi_3_games")
    games_dir.mkdir(parents=True, exist_ok=True)
    
    import json
    
    # Extraire les game_id depuis les EnvironmentInfo
    game_ids = []
    for env_info in games:
        if hasattr(env_info, 'game_id'):
            game_ids.append(env_info.game_id)
        elif hasattr(env_info, 'id'):
            game_ids.append(env_info.id)
        else:
            game_ids.append(str(env_info))
    
    metadata = {
        "games": game_ids,
        "count": len(game_ids),
        "source": "arc_agi Arcade API"
    }
    
    with open(games_dir / "metadata.json", "w") as f:
        json.dump(metadata, f, indent=2)
    
    print(f"✅ Metadata sauvegardé: {games_dir / 'metadata.json'}")
    print(f"✅ Jeux: {', '.join(sorted(game_ids)[:10])}...")
    
except ImportError as e:
    print(f"❌ Erreur import arc_agi: {e}")
    sys.exit(1)
except Exception as e:
    print(f"❌ Erreur: {e}")
    sys.exit(1)

PYTHON_SCRIPT

echo ""
echo "✅ Téléchargement terminé!"
echo ""
echo "📊 Prochaines étapes:"
echo "  1. Tester avec: python tests/test_magen_real_arc_games.py --games 1"
echo "  2. Exécuter 5 jeux: python tests/test_magen_real_arc_games.py --games 5"

# Made with Bob
