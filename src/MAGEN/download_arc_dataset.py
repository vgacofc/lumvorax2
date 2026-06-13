#!/usr/bin/env python3
"""
Téléchargement dataset ARC-AGI officiel
"""

import json
import urllib.request
from pathlib import Path

def download_arc_dataset():
    """Télécharger dataset ARC-AGI training"""
    
    # URL dataset officiel (nouveau repo)
    url = "https://raw.githubusercontent.com/arc-prize/arc-prize-2024/main/data/training_challenges.json"
    
    output_path = Path(__file__).parent / "arc-agi_training_challenges.json"
    
    print(f"📥 Téléchargement dataset ARC-AGI...")
    print(f"   URL: {url}")
    print(f"   Destination: {output_path}")
    
    try:
        with urllib.request.urlopen(url) as response:
            data = json.loads(response.read().decode())
        
        with open(output_path, 'w') as f:
            json.dump(data, f, indent=2)
        
        print(f"✅ Dataset téléchargé: {len(data)} puzzles")
        print(f"   Fichier: {output_path}")
        
        return True
        
    except Exception as e:
        print(f"❌ Erreur téléchargement: {e}")
        return False

if __name__ == "__main__":
    download_arc_dataset()

# Made with Bob
