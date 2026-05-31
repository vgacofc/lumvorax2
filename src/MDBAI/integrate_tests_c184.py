#!/usr/bin/env python3
"""
Script d'intégration tests C184 Phase 3.1 dans Firecracker device.rs
Insère 167 tests dans le module #[cfg(test)] existant
"""

import re
import sys
from pathlib import Path

# Chemins
TESTS_FILE = Path("/tmp/c184_phase3_1_virtio_net_tests_fixed.rs")
DEVICE_FILE = Path("/home/lvx/LVX/lumvorax2/src/MDBAI/firecracker/src/vmm/src/devices/virtio/net/device.rs")
BACKUP_FILE = Path("/home/lvx/LVX/lumvorax2/src/MDBAI/firecracker/src/vmm/src/devices/virtio/net/device.rs.backup_c184")

def read_file(path):
    """Lit un fichier et retourne son contenu"""
    with open(path, 'r', encoding='utf-8') as f:
        return f.read()

def write_file(path, content):
    """Écrit le contenu dans un fichier"""
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)

def extract_tests_from_generated(content):
    """
    Extrait les tests du fichier généré
    Retire les imports, use statements, et déclarations de modules
    Garde SEULEMENT les fonctions #[test]
    """
    lines = content.split('\n')
    test_lines = []
    in_test = False
    in_module = False
    brace_count = 0
    module_brace_count = 0
    
    for line in lines:
        # Détecter déclaration de module
        if line.strip().startswith('#[cfg(test)]'):
            in_module = True
            continue
        
        if in_module and line.strip().startswith('mod '):
            module_brace_count = 0
            continue
            
        # Ignorer les imports et use statements
        if line.strip().startswith('use ') or line.strip().startswith('//'):
            continue
        
        # Si on est dans le module, compter les accolades du module
        if in_module and not in_test:
            module_brace_count += line.count('{') - line.count('}')
            # Si accolade fermante seule du module, ignorer
            if line.strip() == '}' and module_brace_count == 0:
                continue
            
        # Détecter début d'un test (SEULEMENT #[test], pas #[cfg(test)])
        if '#[test]' in line:
            in_test = True
            brace_count = 0
            test_lines.append(line)
            continue
            
        if in_test:
            test_lines.append(line)
            
            # Compter les accolades pour savoir quand le test se termine
            brace_count += line.count('{') - line.count('}')
            
            # Si on revient à 0, le test est terminé
            if brace_count == 0 and '}' in line:
                in_test = False
                test_lines.append('')  # Ligne vide entre tests
    
    return '\n'.join(test_lines)

def find_test_module_end(content):
    """
    Trouve la position de fin du module #[cfg(test)] mod tests
    Retourne l'index de la dernière accolade fermante du module
    """
    # Chercher le module de tests (peut être pub mod tests)
    test_mod_pattern = r'#\[cfg\(test\)\].*?(?:pub\s+)?mod\s+tests\s*\{'
    match = re.search(test_mod_pattern, content, re.DOTALL)
    
    if not match:
        print("❌ Module #[cfg(test)] mod tests non trouvé!")
        return None
    
    start_pos = match.end()
    
    # Compter les accolades pour trouver la fin du module
    brace_count = 1  # On a déjà l'accolade ouvrante
    pos = start_pos
    
    while pos < len(content) and brace_count > 0:
        if content[pos] == '{':
            brace_count += 1
        elif content[pos] == '}':
            brace_count -= 1
        pos += 1
    
    if brace_count != 0:
        print("❌ Accolades non équilibrées dans le module tests!")
        return None
    
    # pos-1 est la position de la dernière accolade fermante
    return pos - 1

def insert_tests_in_device(device_content, tests_content):
    """
    Insère les tests dans le module #[cfg(test)] de device.rs
    """
    # Trouver la fin du module tests
    insert_pos = find_test_module_end(device_content)
    
    if insert_pos is None:
        return None
    
    # Préparer le contenu à insérer
    header = """
    // ============================================================================
    // TESTS C184 PHASE 3.1 — COUVERTURE EXHAUSTIVE VIRTIO-NET
    // Générés: 2026-05-31T18:21Z
    // Objectif: 34.72% → 87.50% couverture device.rs (+2.51% global)
    // Tests: 167 (constructor, activation, RX/TX, buffers, MMDS, rate limiting)
    // ============================================================================
"""
    
    # Insérer avant la dernière accolade du module tests
    new_content = (
        device_content[:insert_pos] +
        header +
        '\n' +
        tests_content +
        '\n' +
        device_content[insert_pos:]
    )
    
    return new_content

def count_tests(content):
    """Compte le nombre de #[test] dans le contenu"""
    return len(re.findall(r'#\[test\]', content))

def main():
    print("=" * 80)
    print("INTÉGRATION TESTS C184 PHASE 3.1 — VIRTIO-NET")
    print("=" * 80)
    
    # Vérifier que les fichiers existent
    if not TESTS_FILE.exists():
        print(f"❌ Fichier tests non trouvé: {TESTS_FILE}")
        print("   Exécutez d'abord generate_tests_c184_phase3_1_virtio_net.py")
        return 1
    
    if not DEVICE_FILE.exists():
        print(f"❌ Fichier device.rs non trouvé: {DEVICE_FILE}")
        return 1
    
    print(f"✅ Fichier tests: {TESTS_FILE}")
    print(f"✅ Fichier cible: {DEVICE_FILE}")
    print()
    
    # Lire les fichiers
    print("📖 Lecture fichiers...")
    tests_content = read_file(TESTS_FILE)
    device_content = read_file(DEVICE_FILE)
    
    tests_before = count_tests(device_content)
    print(f"   Tests existants dans device.rs: {tests_before}")
    print()
    
    # Extraire les tests du fichier généré
    print("🔍 Extraction tests du fichier généré...")
    extracted_tests = extract_tests_from_generated(tests_content)
    tests_extracted = count_tests(extracted_tests)
    print(f"   Tests extraits: {tests_extracted}")
    print()
    
    # Créer backup
    print("💾 Création backup...")
    write_file(BACKUP_FILE, device_content)
    print(f"   Backup: {BACKUP_FILE}")
    print()
    
    # Insérer les tests
    print("🔧 Insertion tests dans device.rs...")
    new_content = insert_tests_in_device(device_content, extracted_tests)
    
    if new_content is None:
        print("❌ Échec insertion tests")
        return 1
    
    tests_after = count_tests(new_content)
    tests_added = tests_after - tests_before
    
    print(f"   Tests avant: {tests_before}")
    print(f"   Tests après: {tests_after}")
    print(f"   Tests ajoutés: {tests_added}")
    print()
    
    # Écrire le nouveau contenu
    print("💾 Écriture device.rs modifié...")
    write_file(DEVICE_FILE, new_content)
    
    lines_before = len(device_content.split('\n'))
    lines_after = len(new_content.split('\n'))
    lines_added = lines_after - lines_before
    
    print(f"   Lignes avant: {lines_before}")
    print(f"   Lignes après: {lines_after}")
    print(f"   Lignes ajoutées: {lines_added}")
    print()
    
    print("=" * 80)
    print("✅ INTÉGRATION RÉUSSIE")
    print("=" * 80)
    print()
    print("📊 Résumé:")
    print(f"   • {tests_added} tests ajoutés")
    print(f"   • {lines_added} lignes ajoutées")
    print(f"   • Backup: {BACKUP_FILE}")
    print()
    print("🔄 Prochaines étapes:")
    print("   1. cd lumvorax2/src/MDBAI/firecracker")
    print("   2. cargo test --package vmm --lib devices::virtio::net --no-run")
    print("   3. cargo test --package vmm --lib devices::virtio::net")
    print("   4. cargo llvm-cov --package vmm --summary-only")
    print()
    
    return 0

if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
