#!/usr/bin/env python3
"""
Script pour télécharger le dataset officiel ARC-AGI-3 depuis Kaggle
"""

import os
import sys
import json
import zipfile
from pathlib import Path
import subprocess

# Configuration Kaggle
KAGGLE_USERNAME = "lvx"
KAGGLE_KEY = "9002d163e9b18247583c6c6a699d1115"
COMPETITION_NAME = "arc-prize-2026-arc-agi-3"
DATASET_DIR = Path("../data/arc_agi_3")

def setup_kaggle_credentials():
    """Configurer credentials Kaggle"""
    kaggle_dir = Path.home() / ".kaggle"
    kaggle_dir.mkdir(exist_ok=True)
    
    kaggle_json = kaggle_dir / "kaggle.json"
    
    credentials = {
        "username": KAGGLE_USERNAME,
        "key": KAGGLE_KEY
    }
    
    with open(kaggle_json, 'w') as f:
        json.dump(credentials, f)
    
    # Permissions 600 (lecture/écriture propriétaire uniquement)
    os.chmod(kaggle_json, 0o600)
    
    print(f"✓ Credentials Kaggle configurées: {kaggle_json}")

def download_dataset():
    """Télécharger dataset ARC-AGI-3"""
    print(f"\n[1/3] Téléchargement dataset {COMPETITION_NAME}...")
    
    # Créer répertoire dataset
    DATASET_DIR.mkdir(parents=True, exist_ok=True)
    
    # Trouver kaggle CLI (venv ou system)
    kaggle_cmd = None
    venv_kaggle = Path("venv/bin/kaggle")
    if venv_kaggle.exists():
        kaggle_cmd = str(venv_kaggle.absolute())
    else:
        # Essayer system
        try:
            result = subprocess.run(["which", "kaggle"], capture_output=True, text=True)
            if result.returncode == 0:
                kaggle_cmd = result.stdout.strip()
        except:
            pass
    
    if not kaggle_cmd:
        print("✗ Kaggle CLI non trouvé. Installation...")
        subprocess.run([sys.executable, "-m", "pip", "install", "-q", "kaggle"], check=True)
        kaggle_cmd = "kaggle"
    
    print(f"  Utilisation: {kaggle_cmd}")
    
    # Télécharger avec kaggle CLI
    cmd = [
        kaggle_cmd, "competitions", "download",
        "-c", COMPETITION_NAME,
        "-p", str(DATASET_DIR)
    ]
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        print(result.stdout)
        print(f"✓ Dataset téléchargé dans: {DATASET_DIR}")
        return True
    except subprocess.CalledProcessError as e:
        print(f"✗ Erreur téléchargement: {e.stderr}")
        return False

def extract_dataset():
    """Extraire fichiers ZIP"""
    print(f"\n[2/3] Extraction fichiers...")
    
    zip_files = list(DATASET_DIR.glob("*.zip"))
    
    if not zip_files:
        print("✗ Aucun fichier ZIP trouvé")
        return False
    
    for zip_file in zip_files:
        print(f"  Extraction: {zip_file.name}")
        with zipfile.ZipFile(zip_file, 'r') as zip_ref:
            zip_ref.extractall(DATASET_DIR)
        
        # Supprimer ZIP après extraction
        zip_file.unlink()
    
    print(f"✓ Extraction complétée")
    return True

def analyze_dataset():
    """Analyser structure dataset"""
    print(f"\n[3/3] Analyse structure dataset...")
    
    # Lister fichiers
    files = list(DATASET_DIR.rglob("*"))
    
    print(f"\nFichiers trouvés: {len(files)}")
    
    # Grouper par extension
    by_ext = {}
    for f in files:
        if f.is_file():
            ext = f.suffix or "no_ext"
            by_ext[ext] = by_ext.get(ext, 0) + 1
    
    print("\nPar extension:")
    for ext, count in sorted(by_ext.items()):
        print(f"  {ext}: {count} fichiers")
    
    # Chercher fichiers JSON (jeux ARC)
    json_files = list(DATASET_DIR.rglob("*.json"))
    print(f"\nFichiers JSON (jeux ARC): {len(json_files)}")
    
    if json_files:
        print("\nExemples:")
        for jf in json_files[:5]:
            print(f"  - {jf.relative_to(DATASET_DIR)}")
    
    return True

def main():
    """Main"""
    print("=" * 80)
    print("TÉLÉCHARGEMENT DATASET ARC-AGI-3 OFFICIEL")
    print("=" * 80)
    
    # 1. Setup credentials
    setup_kaggle_credentials()
    
    # 2. Télécharger
    if not download_dataset():
        print("\n✗ Échec téléchargement")
        sys.exit(1)
    
    # 3. Extraire
    if not extract_dataset():
        print("\n✗ Échec extraction")
        sys.exit(1)
    
    # 4. Analyser
    analyze_dataset()
    
    print("\n" + "=" * 80)
    print("✓ DATASET ARC-AGI-3 PRÊT")
    print("=" * 80)
    print(f"\nRépertoire: {DATASET_DIR.absolute()}")
    print("\nProchaine étape: Exécuter tests avec jeux réels")
    print("  python3 tests/test_magen_real_arc_games.py")

if __name__ == "__main__":
    main()

# Made with Bob
