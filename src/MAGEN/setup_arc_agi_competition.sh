#!/bin/bash
# MAGEN - Setup ARC-AGI-3 Competition Environment
# SESSION 67 - Installation et configuration complète
# Date: 2026-06-14

set -e  # Exit on error

echo "🚀 MAGEN - Setup ARC-AGI-3 Competition Environment"
echo "=================================================="
echo ""

# Couleurs pour output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Répertoire de travail
WORK_DIR="/home/lvx/LVX/lumvorax2/src/MAGEN"
cd "$WORK_DIR"

echo -e "${YELLOW}📁 Répertoire de travail: $WORK_DIR${NC}"
echo ""

# Étape 1: Créer environnement virtuel
echo -e "${YELLOW}🔧 Étape 1/5: Création environnement virtuel Python${NC}"
if [ -d "venv_arc_agi" ]; then
    echo "  ⚠️  venv_arc_agi existe déjà, suppression..."
    rm -rf venv_arc_agi
fi

python3 -m venv venv_arc_agi
echo -e "${GREEN}  ✅ Environnement virtuel créé${NC}"
echo ""

# Étape 2: Activer venv et installer dépendances
echo -e "${YELLOW}🔧 Étape 2/5: Installation arc-agi et dépendances${NC}"
source venv_arc_agi/bin/activate

pip install --upgrade pip > /dev/null 2>&1
echo "  ✅ pip mis à jour"

pip install arc-agi 2>&1 | tee install_arc_agi.log
if [ $? -eq 0 ]; then
    echo -e "${GREEN}  ✅ arc-agi installé avec succès${NC}"
else
    echo -e "${RED}  ❌ Erreur installation arc-agi${NC}"
    exit 1
fi

# Installer dépendances MAGEN existantes
pip install numpy 2>&1 | tee -a install_arc_agi.log
echo "  ✅ numpy installé"
echo ""

# Étape 3: Configuration clé API
echo -e "${YELLOW}🔧 Étape 3/5: Configuration clé API ARC Prize${NC}"
export ARC_API_KEY="b6ebc9c5-a296-4578-be69-0cc16c9455d4"
echo "export ARC_API_KEY='b6ebc9c5-a296-4578-be69-0cc16c9455d4'" >> venv_arc_agi/bin/activate
echo -e "${GREEN}  ✅ Clé API configurée${NC}"
echo ""

# Étape 4: Créer script de test
echo -e "${YELLOW}🔧 Étape 4/5: Création script de test${NC}"
cat > test_arc_agi_basic.py << 'EOF'
#!/usr/bin/env python3
"""
MAGEN - Test ARC-AGI-3 Basic
SESSION 67 - Validation installation et connexion API
"""

import sys
import json
from datetime import datetime

print("🧪 MAGEN - Test ARC-AGI-3 Basic")
print("=" * 50)
print()

try:
    import arc_agi
    from arc_agi import Arcade, OperationMode
    from arcengine import GameAction
    print("✅ Imports arc_agi réussis")
except ImportError as e:
    print(f"❌ Erreur import: {e}")
    sys.exit(1)

# Test 1: Configuration API
print("\n📡 Test 1: Configuration API")
try:
    arc = Arcade(
        arc_api_key="b6ebc9c5-a296-4578-be69-0cc16c9455d4",
        operation_mode=OperationMode.ONLINE
    )
    print("✅ Arcade initialisé en mode ONLINE")
except Exception as e:
    print(f"❌ Erreur configuration: {e}")
    sys.exit(1)

# Test 2: Lister environnements disponibles
print("\n🎮 Test 2: Liste environnements disponibles")
try:
    games = arc.get_environments()
    print(f"✅ {len(games)} jeux disponibles:")
    for i, game in enumerate(games[:5], 1):  # Afficher 5 premiers
        print(f"   {i}. {game.game_id}: {game.title}")
    if len(games) > 5:
        print(f"   ... et {len(games) - 5} autres")
except Exception as e:
    print(f"❌ Erreur liste jeux: {e}")
    sys.exit(1)

# Test 3: Créer environnement et prendre quelques actions
print("\n🕹️  Test 3: Test gameplay basique (ls20)")
try:
    # Créer environnement
    env = arc.make("ls20", seed=0, save_recording=True)
    print("✅ Environnement ls20 créé")
    
    # Prendre 5 actions
    actions_taken = []
    for i in range(5):
        obs = env.step(GameAction.ACTION1)
        if obs:
            actions_taken.append({
                'step': i+1,
                'state': obs.state.name,
                'score': obs.score,
                'levels_completed': obs.levels_completed
            })
    
    print(f"✅ {len(actions_taken)} actions exécutées")
    print(f"   État final: {actions_taken[-1]['state']}")
    print(f"   Score final: {actions_taken[-1]['score']}")
    
except Exception as e:
    print(f"❌ Erreur gameplay: {e}")
    sys.exit(1)

# Test 4: Scorecard
print("\n📊 Test 4: Scorecard")
try:
    scorecard = arc.get_scorecard()
    if scorecard:
        print(f"✅ Scorecard récupéré")
        print(f"   Score: {scorecard.score}")
        print(f"   Jeux joués: {len(scorecard.games)}")
    else:
        print("⚠️  Pas de scorecard disponible")
except Exception as e:
    print(f"❌ Erreur scorecard: {e}")

# Résumé
print("\n" + "=" * 50)
print("✅ TOUS LES TESTS RÉUSSIS")
print("=" * 50)
print()
print("📝 Prochaines étapes:")
print("   1. Créer agent MAGEN pour ARC-AGI-3")
print("   2. Tester en mode COMPETITION")
print("   3. Analyser logs forensiques")
print()

sys.exit(0)
EOF

chmod +x test_arc_agi_basic.py
echo -e "${GREEN}  ✅ Script de test créé: test_arc_agi_basic.py${NC}"
echo ""

# Étape 5: Exécuter test
echo -e "${YELLOW}🔧 Étape 5/5: Exécution test de validation${NC}"
echo ""
python test_arc_agi_basic.py 2>&1 | tee test_arc_agi_basic.log

if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}✅ INSTALLATION ET TESTS RÉUSSIS${NC}"
    echo ""
    echo "📋 Résumé:"
    echo "   - Environnement virtuel: venv_arc_agi/"
    echo "   - Clé API configurée: b6ebc9c5-a296-4578-be69-0cc16c9455d4"
    echo "   - Logs installation: install_arc_agi.log"
    echo "   - Logs test: test_arc_agi_basic.log"
    echo ""
    echo "🚀 Pour activer l'environnement:"
    echo "   source venv_arc_agi/bin/activate"
    echo ""
else
    echo ""
    echo -e "${RED}❌ ERREUR LORS DES TESTS${NC}"
    echo "   Vérifier test_arc_agi_basic.log pour détails"
    exit 1
fi

# Made with Bob
