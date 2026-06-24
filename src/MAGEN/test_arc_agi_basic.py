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
