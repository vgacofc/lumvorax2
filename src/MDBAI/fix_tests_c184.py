#!/usr/bin/env python3
"""
Script de correction des tests C184 Phase 3.1
Corrige les imports et références manquantes dans les tests générés
"""

import re
from pathlib import Path

# Chemins
TESTS_FILE = Path("/tmp/c184_phase3_1_virtio_net_tests.rs")
FIXED_FILE = Path("/tmp/c184_phase3_1_virtio_net_tests_fixed.rs")

def fix_tests(content):
    """
    Corrige les tests générés:
    1. Remplace test_utils::default_tap() par default_net().tap
    2. Tous les types/constantes sont déjà accessibles via use super::*
    """
    
    # Remplacement simple : test_utils::default_tap() -> default_net().tap
    fixed_content = content.replace('test_utils::default_tap()', 'default_net().tap')
    
    return fixed_content

def main():
    print("=" * 80)
    print("CORRECTION TESTS C184 PHASE 3.1")
    print("=" * 80)
    
    # Lire le fichier de tests
    print(f"\n📖 Lecture: {TESTS_FILE}")
    with open(TESTS_FILE, 'r', encoding='utf-8') as f:
        content = f.read()
    
    print(f"   Taille originale: {len(content)} caractères")
    
    # Corriger les tests
    print("\n🔧 Application des corrections...")
    fixed_content = fix_tests(content)
    
    print(f"   Taille corrigée: {len(fixed_content)} caractères")
    
    # Écrire le fichier corrigé
    print(f"\n💾 Écriture: {FIXED_FILE}")
    with open(FIXED_FILE, 'w', encoding='utf-8') as f:
        f.write(fixed_content)
    
    print("\n" + "=" * 80)
    print("✅ CORRECTION TERMINÉE")
    print("=" * 80)
    print(f"\nFichier corrigé: {FIXED_FILE}")
    print("\n🔄 Prochaine étape:")
    print("   python3 integrate_tests_c184.py (avec TESTS_FILE mis à jour)")

if __name__ == "__main__":
    main()

# Made with Bob
