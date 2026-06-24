#!/usr/bin/env python3
"""
CRÉATION NOUVEAU SCORECARD + RELANCE IMMÉDIATE
==============================================

EXPLICATION SCORECARD:
- Un scorecard = session de test sur l'API ARC-AGI-3
- Quota limité: ~200-300 tentatives par scorecard
- Une fois épuisé, TOUTES les requêtes RESET sont rejetées (erreur 400)
- Solution: Créer un NOUVEAU scorecard

Ce script:
1. Crée un nouveau scorecard
2. Affiche l'ID du scorecard
3. Relance automatiquement train_single_game_ls20_realtime.py

Auteur: Bob (Session 78)
Date: 2026-06-16
"""

import os
import sys
import subprocess
from datetime import datetime

# Configuration API
ARC_API_KEY = "b6ebc9c5-a296-4578-be69-0cc16c9455d4"
os.environ["ARC_API_KEY"] = ARC_API_KEY

print(f"""
{'='*80}
🆕 CRÉATION NOUVEAU SCORECARD ARC-AGI-3
{'='*80}

📋 QU'EST-CE QU'UN SCORECARD?
-----------------------------
Un scorecard est une SESSION DE TEST sur l'API ARC-AGI-3.

Chaque scorecard a:
- Un ID unique (ex: "abc123def456")
- Un quota limité de tentatives (~200-300 RESET)
- Une durée de vie (expire après X heures)

Quand le quota est épuisé:
- ❌ Toutes les requêtes RESET sont rejetées (erreur 400)
- ❌ Impossible de démarrer de nouveaux jeux
- ✅ Solution: Créer un NOUVEAU scorecard

{'='*80}
Timestamp: {datetime.now().isoformat()}
{'='*80}
""")

# Import arc-agi
try:
    from arc_agi import Arcade, OperationMode
    print("✅ arc-agi importé")
except ImportError as e:
    print(f"❌ ERREUR import: {e}")
    print("\n💡 Installation requise:")
    print("   pip install arc-agi")
    sys.exit(1)

# Créer nouveau scorecard
print("\n🔄 Création d'un nouveau scorecard...")
try:
    # Initialiser Arcade (sans paramètres - utilise env var ARC_API_KEY)
    arcade = Arcade()
    
    # Créer scorecard
    scorecard_id = arcade.create_scorecard()
    
    print(f"""
{'='*80}
✅ NOUVEAU SCORECARD CRÉÉ AVEC SUCCÈS!
{'='*80}

📋 Scorecard ID: {scorecard_id}

Ce scorecard vous permet de:
- Faire ~200-300 tentatives RESET
- Tester MAGEN sur les jeux Arcade
- Soumettre des solutions à la compétition

⚠️ IMPORTANT:
- Conservez cet ID pour référence
- Une fois épuisé, créez-en un nouveau
- Chaque scorecard est indépendant

{'='*80}
""")
    
    # Sauvegarder l'ID dans un fichier
    scorecard_file = "current_scorecard.txt"
    with open(scorecard_file, 'w') as f:
        f.write(f"{scorecard_id}\n")
        f.write(f"Created: {datetime.now().isoformat()}\n")
    
    print(f"💾 Scorecard ID sauvegardé dans: {scorecard_file}")
    
    # Relancer automatiquement le script d'entraînement
    print("\n" + "="*80)
    print("🎯 LANCEMENT DE L'ENTRAÎNEMENT")
    print("="*80)
    print(f"\nScorecard actif: {scorecard_id}")
    print("Script: train_single_game_ls20_realtime.py")
    print("\n" + "="*80 + "\n")
    
    # Exécuter le script
    script_path = "train_single_game_ls20_realtime.py"
    result = subprocess.run(
        [sys.executable, script_path],
        cwd=os.path.dirname(os.path.abspath(__file__))
    )
    
    sys.exit(result.returncode)
    
except Exception as e:
    print(f"\n❌ ERREUR lors de la création du scorecard:")
    print(f"   {type(e).__name__}: {e}")
    print("\n💡 Vérifications:")
    print("   1. API key valide?")
    print("   2. Connexion internet OK?")
    print("   3. API ARC-AGI-3 accessible?")
    sys.exit(1)

# Made with Bob
