"""
ARC-AGI Dataset Loader
Charge les puzzles officiels du dataset ARC-AGI pour tests réels
"""

import json
import random
from pathlib import Path
from typing import List, Dict, Optional, Tuple
import numpy as np


class ARCDatasetLoader:
    """Loader pour dataset ARC-AGI officiel"""
    
    def __init__(self, data_dir: str = "."):
        """
        Initialise le loader
        
        Args:
            data_dir: Répertoire racine (défaut: répertoire courant MAGEN)
        """
        self.data_dir = Path(data_dir)
        
        # Chemins vers fichiers JSON officiels installés
        self.training_file = self.data_dir / "arc-agi_training_challenges.json"
        self.evaluation_file = self.data_dir / "data" / "arc-agi_evaluation_challenges.json"
        self.arcade_metadata_file = self.data_dir / "data" / "arc_agi_3" / "arc_agi_3_games" / "metadata.json"
        
        # Vérifier existence fichiers training (obligatoire)
        if not self.training_file.exists():
            raise FileNotFoundError(
                f"Training file not found: {self.training_file}\n"
                f"Expected: arc-agi_training_challenges.json in {self.data_dir}"
            )
        
        # Evaluation optionnel (peut être symlink vers training)
        if not self.evaluation_file.exists():
            print(f"Warning: Evaluation file not found: {self.evaluation_file}")
    
    def load_training_puzzles(self, limit: Optional[int] = None,
                            shuffle: bool = True) -> List[Dict]:
        """
        Charge puzzles training du dataset officiel
        
        Args:
            limit: Nombre maximum de puzzles à charger (None = tous)
            shuffle: Mélanger les puzzles avant sélection
            
        Returns:
            Liste de puzzles avec format:
            {
                'id': str,
                'train': List[{'input': List[List[int]], 'output': List[List[int]]}],
                'test': List[{'input': List[List[int]], 'output': List[List[int]]}]
            }
        """
        return self._load_puzzles_from_file(self.training_file, limit, shuffle)
    
    def load_evaluation_puzzles(self, limit: Optional[int] = None,
                               shuffle: bool = True) -> List[Dict]:
        """
        Charge puzzles evaluation du dataset officiel
        
        Args:
            limit: Nombre maximum de puzzles à charger (None = tous)
            shuffle: Mélanger les puzzles avant sélection
            
        Returns:
            Liste de puzzles (même format que load_training_puzzles)
        """
        if not self.evaluation_file.exists():
            print("Warning: Evaluation file not found, using training file")
            return self._load_puzzles_from_file(self.training_file, limit, shuffle)
        return self._load_puzzles_from_file(self.evaluation_file, limit, shuffle)
    
    def load_arcade_games(self) -> List[str]:
        """
        Charge liste des jeux Arcade officiels
        
        Returns:
            Liste des IDs de jeux Arcade (format: 's5i5-18d95033', etc.)
        """
        if not self.arcade_metadata_file.exists():
            raise FileNotFoundError(
                f"Arcade metadata not found: {self.arcade_metadata_file}"
            )
        
        with open(self.arcade_metadata_file, 'r') as f:
            metadata = json.load(f)
            return metadata.get('games', [])
    
    def _load_puzzles_from_file(self, json_file: Path, limit: Optional[int],
                               shuffle: bool) -> List[Dict]:
        """
        Charge puzzles depuis un fichier JSON
        
        Args:
            json_file: Fichier JSON contenant les puzzles
            limit: Nombre maximum de puzzles
            shuffle: Mélanger avant sélection
            
        Returns:
            Liste de puzzles
        """
        # Charger fichier JSON complet
        with open(json_file, 'r') as f:
            all_puzzles_dict = json.load(f)
        
        # Convertir dict en liste avec IDs
        puzzles = []
        for puzzle_id, puzzle_data in all_puzzles_dict.items():
            puzzle_data['id'] = puzzle_id
            puzzles.append(puzzle_data)
        
        # Mélanger si demandé
        if shuffle:
            random.shuffle(puzzles)
        
        # Limiter si nécessaire
        if limit is not None:
            puzzles = puzzles[:limit]
        
        return puzzles
    
    def get_puzzle_stats(self, puzzle: Dict) -> Dict:
        """
        Calcule statistiques d'un puzzle
        
        Args:
            puzzle: Puzzle à analyser
            
        Returns:
            Dictionnaire de statistiques
        """
        train_examples = puzzle['train']
        test_examples = puzzle['test']
        
        # Tailles moyennes
        train_input_sizes = [
            (len(ex['input']), len(ex['input'][0]) if ex['input'] else 0)
            for ex in train_examples
        ]
        train_output_sizes = [
            (len(ex['output']), len(ex['output'][0]) if ex['output'] else 0)
            for ex in train_examples
        ]
        
        # Nombre de couleurs
        all_train_colors = set()
        for ex in train_examples:
            all_train_colors.update(np.array(ex['input']).flatten())
            all_train_colors.update(np.array(ex['output']).flatten())
        
        return {
            'id': puzzle['id'],
            'num_train_examples': len(train_examples),
            'num_test_examples': len(test_examples),
            'avg_train_input_size': np.mean([h*w for h, w in train_input_sizes]),
            'avg_train_output_size': np.mean([h*w for h, w in train_output_sizes]),
            'num_colors': len(all_train_colors),
            'train_input_sizes': train_input_sizes,
            'train_output_sizes': train_output_sizes
        }
    
    def get_dataset_stats(self, puzzles: List[Dict]) -> Dict:
        """
        Calcule statistiques globales du dataset
        
        Args:
            puzzles: Liste de puzzles
            
        Returns:
            Statistiques agrégées
        """
        all_stats = [self.get_puzzle_stats(p) for p in puzzles]
        
        return {
            'total_puzzles': len(puzzles),
            'avg_train_examples': np.mean([s['num_train_examples'] for s in all_stats]),
            'avg_test_examples': np.mean([s['num_test_examples'] for s in all_stats]),
            'avg_input_size': np.mean([s['avg_train_input_size'] for s in all_stats]),
            'avg_output_size': np.mean([s['avg_train_output_size'] for s in all_stats]),
            'avg_colors': np.mean([s['num_colors'] for s in all_stats]),
            'min_colors': min([s['num_colors'] for s in all_stats]),
            'max_colors': max([s['num_colors'] for s in all_stats])
        }


def main():
    """Test du loader"""
    loader = ARCDatasetLoader()
    
    # Charger 10 puzzles training
    print("Loading 10 training puzzles...")
    train_puzzles = loader.load_training_puzzles(limit=10)
    print(f"✅ Loaded {len(train_puzzles)} training puzzles")
    print(f"   First 3 IDs: {[p['id'] for p in train_puzzles[:3]]}")
    
    # Charger jeux Arcade
    print("\nLoading Arcade games...")
    arcade_games = loader.load_arcade_games()
    print(f"✅ Loaded {len(arcade_games)} Arcade games")
    print(f"   First 3 IDs: {arcade_games[:3]}")
    
    # Statistiques
    stats = loader.get_dataset_stats(train_puzzles)
    print("\nDataset Statistics:")
    for key, value in stats.items():
        print(f"  {key}: {value:.2f}" if isinstance(value, float) else f"  {key}: {value}")
    
    # Exemple de puzzle
    print(f"\nExample puzzle: {train_puzzles[0]['id']}")
    puzzle_stats = loader.get_puzzle_stats(train_puzzles[0])
    print(f"  Train examples: {puzzle_stats['num_train_examples']}")
    print(f"  Test examples: {puzzle_stats['num_test_examples']}")
    print(f"  Colors: {puzzle_stats['num_colors']}")
    print(f"  Input sizes: {puzzle_stats['train_input_sizes']}")
    
    print("\n✅ Loader test successful - All official datasets accessible")


if __name__ == "__main__":
    main()

# Made with Bob
