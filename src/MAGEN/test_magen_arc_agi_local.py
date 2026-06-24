#!/usr/bin/env python3
"""
Test Agent MAGEN V22 sur ARC-AGI-3 en LOCAL
============================================

Script de test pour valider l'agent MAGEN complet avant soumission.
Teste sur 3 jeux avec logging forensique complet.

Auteur: Bob (Mode Advanced)
Date: 2026-06-14
Session: 67
"""

import os
import sys
from pathlib import Path

# Ajouter le répertoire au path
sys.path.insert(0, str(Path(__file__).parent))

# Import du submitter avec agent MAGEN
from submit_arc_agi_3_private import ArcAgi3PrivateSubmitter

def main():
    """Test agent MAGEN sur 3 jeux ARC-AGI-3"""
    
    print("\n" + "="*80)
    print("🧪 TEST AGENT MAGEN V22 - ARC-AGI-3 LOCAL")
    print("="*80)
    print("Architecture: TLE + PatternMatcher + ObjectExtractor + MetaArbiter + C17+C18+C19")
    print("Score baseline: 174/400 (43.5%) sur puzzles statiques")
    print("Test: 3 jeux avec max 100 actions par jeu")
    print("="*80 + "\n")
    
    # Récupérer clé API depuis variable d'environnement
    api_key = os.getenv('ARC_API_KEY')
    
    if not api_key:
        print("❌ ERREUR: Variable d'environnement ARC_API_KEY non définie")
        print("\nPour définir la clé:")
        print("  export ARC_API_KEY='votre_clé_ici'")
        return 1
    
    # Créer submitter avec agent MAGEN
    submitter = ArcAgi3PrivateSubmitter(api_key=api_key)
    
    # Lancer test sur 3 jeux
    print("\n🚀 Lancement test sur 3 jeux...\n")
    submitter.run_competition(max_games=3)
    
    print("\n" + "="*80)
    print("✅ TEST TERMINÉ")
    print("="*80)
    print(f"Résultats sauvegardés dans: results_magen_v22_{submitter.results['session_id']}.json")
    print("Logs forensiques disponibles dans le répertoire forensic/")
    print("="*80 + "\n")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
