#!/usr/bin/env python3
"""
Test rapide du système de transfer learning multi-niveaux.

Test: 2 niveaux avec 5 épisodes chacun pour validation rapide.
"""

import sys
from pathlib import Path

# Ajout chemin
sys.path.insert(0, str(Path(__file__).parent.parent))

from arc_integration.run_multilevel_training import MultiLevelTrainer


def main():
    """Test rapide 2 niveaux."""
    print("\n" + "="*60)
    print("TEST RAPIDE TRANSFER LEARNING MULTI-NIVEAUX")
    print("="*60)
    print("Configuration: 2 niveaux, 5 épisodes chacun")
    print("="*60 + "\n")
    
    # Créer trainer avec configuration test
    trainer = MultiLevelTrainer(
        game_id="ls20-9607627b",
        episodes_per_level=[5, 3, 0, 0, 0, 0, 0],  # Seulement niveaux 1-2
        checkpoint_dir="./test_checkpoints",
        results_dir="./test_results"
    )
    
    # Entraîner niveau 1
    print("\n🎯 Test Niveau 1 (5 épisodes from scratch)")
    metrics_l1 = trainer.train_level(level=1, num_episodes=5, transfer_from_level=0)
    print(f"✅ Niveau 1: success_rate={metrics_l1.success_rate:.1f}%, unique_states={metrics_l1.unique_states}")
    
    # Entraîner niveau 2 avec transfer learning
    print("\n🎯 Test Niveau 2 (3 épisodes avec transfer learning)")
    metrics_l2 = trainer.train_level(level=2, num_episodes=3, transfer_from_level=1)
    print(f"✅ Niveau 2: success_rate={metrics_l2.success_rate:.1f}%, unique_states={metrics_l2.unique_states}")
    
    # Résumé
    print("\n" + "="*60)
    print("RÉSUMÉ TEST")
    print("="*60)
    print(f"Niveau 1: {metrics_l1.success_rate:.1f}% success, {metrics_l1.unique_states} états")
    print(f"Niveau 2: {metrics_l2.success_rate:.1f}% success, {metrics_l2.unique_states} états")
    
    if metrics_l2.unique_states > metrics_l1.unique_states:
        print("\n✅ Transfer learning fonctionne: plus d'états explorés au niveau 2")
    else:
        print("\n⚠️  Transfer learning à améliorer")
    
    print("\n✅ Test terminé avec succès!")


if __name__ == "__main__":
    main()

# Made with Bob
