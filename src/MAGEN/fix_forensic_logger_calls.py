#!/usr/bin/env python3
"""
Script de correction automatique des appels FORENSIC_LOGGER
Corrige l'API: log_event(event_type, data) → log_event(event_type, component, operation, data)
"""

import re
from pathlib import Path

def fix_file(filepath: Path):
    """Corriger les appels log_event dans un fichier"""
    print(f"Traitement: {filepath}")
    
    with open(filepath, 'r') as f:
        content = f.read()
    
    original = content
    
    # Pattern: FORENSIC_LOGGER.log_event("pattern_detection", {
    #          "detector": self.name,
    #          "num_pairs": len(train_pairs)
    #      })
    
    # Remplacer par: FORENSIC_LOGGER.log_event(
    #     event_type="pattern_detection",
    #     component="advanced_pattern_detectors", 
    #     operation="detect",
    #     data={
    #         "detector": self.name,
    #         "num_pairs": len(train_pairs)
    #     }
    # )
    
    pattern = r'FORENSIC_LOGGER\.log_event\("pattern_detection",\s*\{'
    replacement = '''FORENSIC_LOGGER.log_event(
                event_type="pattern_detection",
                component="advanced_pattern_detectors",
                operation="detect",
                data={'''
    
    content = re.sub(pattern, replacement, content)
    
    # Fermer le dict data correctement
    # Chercher les lignes avec }) après les appels log_event
    lines = content.split('\n')
    fixed_lines = []
    in_log_event = False
    
    for i, line in enumerate(lines):
        if 'FORENSIC_LOGGER.log_event(' in line and 'event_type="pattern_detection"' in line:
            in_log_event = True
            fixed_lines.append(line)
        elif in_log_event and '})' in line:
            # Remplacer }) par }
            # et ajouter ) sur ligne suivante
            fixed_lines.append(line.replace('})', '}'))
            fixed_lines.append(' ' * (len(line) - len(line.lstrip())) + ')')
            in_log_event = False
        else:
            fixed_lines.append(line)
    
    content = '\n'.join(fixed_lines)
    
    if content != original:
        with open(filepath, 'w') as f:
            f.write(content)
        print(f"  ✅ Corrigé: {filepath}")
        return True
    else:
        print(f"  ⏭️  Aucun changement: {filepath}")
        return False

def main():
    """Point d'entrée"""
    print("="*80)
    print("CORRECTION APPELS FORENSIC_LOGGER")
    print("="*80)
    
    # Fichier à corriger
    target = Path("core/advanced_pattern_detectors.py")
    
    if not target.exists():
        print(f"❌ Fichier non trouvé: {target}")
        return 1
    
    fixed = fix_file(target)
    
    print("\n" + "="*80)
    if fixed:
        print("✅ CORRECTION TERMINÉE")
    else:
        print("⚠️  AUCUNE CORRECTION NÉCESSAIRE")
    print("="*80)
    
    return 0

if __name__ == "__main__":
    exit(main())

# Made with Bob
