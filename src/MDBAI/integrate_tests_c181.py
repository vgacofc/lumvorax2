#!/usr/bin/env python3
"""
Script d'intégration automatique des tests C180 dans sources Firecracker

Intègre les 45 tests réels générés dans les modules sources correspondants.
Crée des backups avant modification pour sécurité.

Date: 2026-05-31
Cycle: C181
"""

import os
import sys
import shutil
from pathlib import Path
from datetime import datetime

# Configuration
FIRECRACKER_PATH = Path("/home/lvx/LVX/lumvorax2/src/MDBAI/firecracker")
TESTS_DIR = Path("/home/lvx/LVX/lumvorax2/src/MDBAI/tests_c180_phase1_real")
BACKUP_DIR = Path("/home/lvx/LVX/lumvorax2/src/MDBAI/backups/sources_c181")

# Mapping tests → modules sources
TEST_TO_SOURCE_MAP = {
    "tests_signal_handler.rs": "src/vmm/src/signal_handler.rs",
    "tests_vmm_config_mmds.rs": "src/vmm/src/vmm_config/mmds.rs",
    "tests_pci_mod.rs": "src/vmm/src/devices/virtio/pci/mod.rs",
    "tests_rpc_interface.rs": "src/vmm/src/rpc_interface.rs",
    "tests_resources.rs": "src/vmm/src/resources.rs",
}

def create_backup_dir():
    """Crée le répertoire de backup"""
    BACKUP_DIR.mkdir(parents=True, exist_ok=True)
    print(f"✅ Répertoire backup créé: {BACKUP_DIR}")

def backup_source_file(source_path: Path) -> Path:
    """Crée un backup d'un fichier source"""
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_name = f"{source_path.stem}_backup_c181_{timestamp}{source_path.suffix}"
    backup_path = BACKUP_DIR / backup_name
    
    shutil.copy2(source_path, backup_path)
    print(f"   ✅ Backup: {backup_path.name}")
    return backup_path

def read_test_content(test_file: Path) -> str:
    """Lit le contenu d'un fichier de test"""
    with open(test_file, 'r') as f:
        content = f.read()
    return content.strip()

def integrate_tests_into_source(source_path: Path, test_content: str) -> bool:
    """Intègre les tests dans un fichier source"""
    
    # Lire le contenu source actuel
    with open(source_path, 'r') as f:
        source_lines = f.readlines()
    
    # Vérifier si des tests C180 existent déjà
    source_content = ''.join(source_lines)
    if 'tests_c180_' in source_content:
        print(f"   ⚠️  Tests C180 déjà présents, skip")
        return False
    
    # Trouver la position d'insertion (à la fin du fichier)
    # On ajoute simplement à la fin
    with open(source_path, 'a') as f:
        f.write("\n\n")
        f.write(test_content)
        f.write("\n")
    
    print(f"   ✅ Tests intégrés ({test_content.count('#[test]')} tests)")
    return True

def main():
    print("=" * 70)
    print("INTÉGRATION TESTS C180 DANS SOURCES FIRECRACKER")
    print("=" * 70)
    print()
    
    # Créer répertoire backup
    create_backup_dir()
    print()
    
    # Statistiques
    total_integrated = 0
    total_tests = 0
    files_modified = []
    
    # Intégrer chaque fichier de test
    for test_file_name, source_rel_path in TEST_TO_SOURCE_MAP.items():
        test_file = TESTS_DIR / test_file_name
        source_file = FIRECRACKER_PATH / source_rel_path
        
        print(f"📝 {test_file_name} → {source_rel_path}")
        
        # Vérifier que les fichiers existent
        if not test_file.exists():
            print(f"   ❌ Fichier test introuvable: {test_file}")
            continue
        
        if not source_file.exists():
            print(f"   ❌ Fichier source introuvable: {source_file}")
            continue
        
        # Créer backup
        backup_path = backup_source_file(source_file)
        
        # Lire contenu test
        test_content = read_test_content(test_file)
        test_count = test_content.count('#[test]')
        
        # Intégrer tests
        if integrate_tests_into_source(source_file, test_content):
            total_integrated += 1
            total_tests += test_count
            files_modified.append(source_rel_path)
        
        print()
    
    # Résumé
    print("=" * 70)
    print("RÉSUMÉ INTÉGRATION")
    print("=" * 70)
    print(f"Fichiers modifiés: {total_integrated}/{len(TEST_TO_SOURCE_MAP)}")
    print(f"Tests intégrés: {total_tests}")
    print()
    
    if files_modified:
        print("Fichiers sources modifiés:")
        for filepath in files_modified:
            print(f"  - {filepath}")
        print()
    
    print("Backups créés dans:")
    print(f"  {BACKUP_DIR}")
    print()
    
    print("✅ Intégration terminée avec succès!")
    print()
    print("Prochaines étapes:")
    print("1. Compiler: cd firecracker && cargo test --workspace --exclude jailer")
    print("2. Mesurer couverture: cargo llvm-cov --workspace --exclude jailer --summary-only")
    print("3. Analyser résultats")
    print()

if __name__ == "__main__":
    main()

# Made with Bob
