#!/usr/bin/env python3
"""
Téléchargement Dataset ARC-AGI-3 Complet
=========================================

Télécharge le dataset ARC-AGI officiel complet:
- Training challenges (400 puzzles)
- Evaluation challenges (400 puzzles)
- Test challenges (disponibles)

Protocole: Claude Pilot + LumVorax
Mode: LOCAL (100%)
Auteur: Bob (Mode Advanced)
Date: 2026-06-12
"""

import json
import urllib.request
import urllib.error
from pathlib import Path
from typing import Dict, List, Tuple, Optional
import time
import hashlib


class ARCDatasetDownloader:
    """Téléchargeur dataset ARC-AGI-3 complet"""
    
    # URLs officielles ARC-AGI
    URLS = {
        "training": "https://raw.githubusercontent.com/fchollet/ARC-AGI/master/data/training",
        "evaluation": "https://raw.githubusercontent.com/fchollet/ARC-AGI/master/data/evaluation",
        "test": "https://raw.githubusercontent.com/fchollet/ARC-AGI/master/data/test"
    }
    
    def __init__(self, output_dir: Optional[str] = None):
        """
        Initialiser téléchargeur
        
        Args:
            output_dir: Répertoire sortie (défaut: répertoire script)
        """
        if output_dir is None:
            self.output_dir = Path(__file__).parent
        else:
            self.output_dir = Path(output_dir)
        
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        self.stats = {
            "downloaded": 0,
            "failed": 0,
            "total": 0,
            "errors": []
        }
    
    def download_puzzle(self, puzzle_id: str, dataset_type: str) -> Tuple[bool, dict]:
        """
        Télécharger un puzzle individuel
        
        Args:
            puzzle_id: ID puzzle (ex: "007bbfb7")
            dataset_type: Type dataset ("training", "evaluation", "test")
        
        Returns:
            (success, puzzle_data)
        """
        url = f"{self.URLS[dataset_type]}/{puzzle_id}.json"
        
        try:
            with urllib.request.urlopen(url, timeout=10) as response:
                data = json.loads(response.read().decode())
                return True, data
        
        except urllib.error.HTTPError as e:
            if e.code == 404:
                return False, {"error": "Puzzle not found"}
            else:
                return False, {"error": f"HTTP {e.code}: {e.reason}"}
        
        except Exception as e:
            return False, {"error": str(e)}
    
    def discover_puzzles(self, dataset_type: str) -> List[str]:
        """
        Découvrir tous les puzzles d'un dataset
        
        Note: ARC-AGI utilise des IDs hexadécimaux 8 caractères
        On va essayer de télécharger les puzzles connus
        
        Args:
            dataset_type: Type dataset
        
        Returns:
            Liste IDs puzzles découverts
        """
        print(f"\n📡 Découverte puzzles {dataset_type}...")
        
        # Liste puzzles connus (à compléter)
        # Ces IDs viennent du dataset officiel ARC-AGI
        known_puzzles = []
        
        # Essayer de télécharger liste depuis GitHub
        try:
            # URL liste puzzles (si disponible)
            list_url = f"https://api.github.com/repos/fchollet/ARC-AGI/contents/data/{dataset_type}"
            
            with urllib.request.urlopen(list_url, timeout=10) as response:
                files = json.loads(response.read().decode())
                
                for file_info in files:
                    if file_info['name'].endswith('.json'):
                        puzzle_id = file_info['name'].replace('.json', '')
                        known_puzzles.append(puzzle_id)
            
            print(f"   ✓ {len(known_puzzles)} puzzles découverts via API GitHub")
        
        except Exception as e:
            print(f"   ⚠️  Impossible de découvrir via API: {e}")
            print(f"   → Utilisation liste puzzles connue")
            
            # Fallback: liste puzzles training connus
            if dataset_type == "training":
                known_puzzles = [
                    "007bbfb7", "00d62c1b", "025d127b", "045e512c", "0520fde7",
                    "05269061", "05f2a901", "06df4c85", "08ed6ac7", "09629e4f",
                    # ... (ajouter plus d'IDs connus)
                ]
        
        return known_puzzles
    
    def download_dataset(self, dataset_type: str, max_puzzles: Optional[int] = None) -> Dict:
        """
        Télécharger dataset complet
        
        Args:
            dataset_type: Type dataset ("training", "evaluation", "test")
            max_puzzles: Nombre max puzzles (None = tous)
        
        Returns:
            Dict avec tous les puzzles téléchargés
        """
        print(f"\n{'='*80}")
        print(f"TÉLÉCHARGEMENT DATASET: {dataset_type.upper()}")
        print(f"{'='*80}")
        
        # Découvrir puzzles
        puzzle_ids = self.discover_puzzles(dataset_type)
        
        if max_puzzles:
            puzzle_ids = puzzle_ids[:max_puzzles]
        
        self.stats["total"] = len(puzzle_ids)
        print(f"\nPuzzles à télécharger: {len(puzzle_ids)}")
        
        # Télécharger chaque puzzle
        dataset = {}
        
        for i, puzzle_id in enumerate(puzzle_ids, 1):
            print(f"\r[{i}/{len(puzzle_ids)}] {puzzle_id}...", end="", flush=True)
            
            success, data = self.download_puzzle(puzzle_id, dataset_type)
            
            if success:
                dataset[puzzle_id] = data
                self.stats["downloaded"] += 1
            else:
                self.stats["failed"] += 1
                self.stats["errors"].append({
                    "puzzle_id": puzzle_id,
                    "error": data.get("error", "Unknown")
                })
            
            # Pause pour éviter rate limiting
            time.sleep(0.1)
        
        print()  # Nouvelle ligne après progression
        
        return dataset
    
    def save_dataset(self, dataset: Dict, dataset_type: str) -> str:
        """
        Sauvegarder dataset sur disque
        
        Args:
            dataset: Dict puzzles
            dataset_type: Type dataset
        
        Returns:
            Chemin fichier sauvegardé
        """
        output_path = self.output_dir / f"arc-agi_{dataset_type}_challenges.json"
        
        with open(output_path, 'w') as f:
            json.dump(dataset, f, indent=2)
        
        # Calculer hash SHA256
        with open(output_path, 'rb') as f:
            file_hash = hashlib.sha256(f.read()).hexdigest()
        
        # Sauvegarder hash
        hash_path = output_path.with_suffix('.json.sha256')
        with open(hash_path, 'w') as f:
            f.write(f"{file_hash}  {output_path.name}\n")
        
        return str(output_path)
    
    def print_statistics(self):
        """Afficher statistiques téléchargement"""
        print(f"\n{'='*80}")
        print("STATISTIQUES TÉLÉCHARGEMENT")
        print(f"{'='*80}")
        
        print(f"\nTotal puzzles: {self.stats['total']}")
        print(f"Téléchargés: {self.stats['downloaded']}")
        print(f"Échecs: {self.stats['failed']}")
        
        if self.stats['downloaded'] > 0:
            success_rate = (self.stats['downloaded'] / self.stats['total']) * 100
            print(f"Taux succès: {success_rate:.1f}%")
        
        if self.stats['errors']:
            print(f"\nErreurs ({len(self.stats['errors'])}):")
            for error in self.stats['errors'][:10]:  # Afficher 10 premières
                print(f"  - {error['puzzle_id']}: {error['error']}")
            
            if len(self.stats['errors']) > 10:
                print(f"  ... et {len(self.stats['errors']) - 10} autres")


def download_all_datasets(max_puzzles_per_dataset: Optional[int] = None):
    """
    Télécharger tous les datasets ARC-AGI-3
    
    Args:
        max_puzzles_per_dataset: Limite puzzles par dataset (None = tous)
    """
    print("\n" + "="*80)
    print("TÉLÉCHARGEMENT DATASET ARC-AGI-3 COMPLET")
    print("="*80)
    print("\nProtocole: Claude Pilot + LumVorax")
    print("Mode: LOCAL (100%)")
    print()
    
    downloader = ARCDatasetDownloader()
    
    datasets_to_download = ["training", "evaluation"]
    
    for dataset_type in datasets_to_download:
        # Télécharger dataset
        dataset = downloader.download_dataset(dataset_type, max_puzzles_per_dataset)
        
        # Sauvegarder
        output_path = downloader.save_dataset(dataset, dataset_type)
        
        print(f"\n✓ Dataset sauvegardé: {output_path}")
        print(f"  Puzzles: {len(dataset)}")
        
        # Réinitialiser stats pour prochain dataset
        downloader.stats = {
            "downloaded": 0,
            "failed": 0,
            "total": 0,
            "errors": []
        }
    
    print(f"\n{'='*80}")
    print("✓ TÉLÉCHARGEMENT COMPLET")
    print(f"{'='*80}\n")


def main():
    """Point d'entrée principal"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description="Télécharger dataset ARC-AGI-3 complet"
    )
    parser.add_argument(
        "--max-puzzles",
        type=int,
        default=None,
        help="Nombre max puzzles par dataset (défaut: tous)"
    )
    parser.add_argument(
        "--dataset",
        choices=["training", "evaluation", "all"],
        default="all",
        help="Dataset à télécharger (défaut: all)"
    )
    
    args = parser.parse_args()
    
    if args.dataset == "all":
        download_all_datasets(args.max_puzzles)
    else:
        downloader = ARCDatasetDownloader()
        dataset = downloader.download_dataset(args.dataset, args.max_puzzles)
        output_path = downloader.save_dataset(dataset, args.dataset)
        downloader.print_statistics()
        
        print(f"\n✓ Dataset sauvegardé: {output_path}")
        print(f"  Puzzles: {len(dataset)}")


if __name__ == "__main__":
    main()

# Made with Bob