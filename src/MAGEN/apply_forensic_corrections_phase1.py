"""
Script de correction automatique Phase 1 - Forensic V32
Ajoute méthodes get_state() manquantes aux modules V32
"""

import os
import re

# Définir les corrections pour chaque module
CORRECTIONS = {
    "spatial_compressor.py": {
        "insert_after_line": 449,  # Après get_statistics()
        "method": '''
    
    def get_state(self) -> Dict:
        """Obtenir état complet du module (forensic-grade, API standard)."""
        return self.get_statistics()
'''
    },
    
    "cognitive_stabilizer.py": {
        "insert_after_line": 399,  # Après export_to_json()
        "method": '''
    
    def get_state(self) -> Dict:
        """Obtenir état complet du module (forensic-grade, API standard)."""
        return self.get_stability_metrics()
'''
    },
    
    "goal_hypothesis_engine.py": {
        "insert_after_line": 553,  # Après export_to_json()
        "method": '''
    
    def get_state(self) -> Dict:
        """Obtenir état complet du module (forensic-grade, API standard)."""
        return self.get_statistics()
'''
    },
    
    "trajectory_meaning_system.py": {
        "insert_after_line": 497,  # Après export_to_json()
        "method": '''
    
    def get_state(self) -> Dict:
        """Obtenir état complet du module (forensic-grade, API standard)."""
        return self.get_statistics()
'''
    },
    
    "spatial_knowledge_graph.py": {
        "insert_after_line": 536,  # Après export_to_json()
        "method": '''
    
    def get_state(self) -> Dict:
        """Obtenir état complet du module (forensic-grade, API standard)."""
        return self.get_statistics()
'''
    },
    
    "decision_kernel_v32_spatial.py": {
        "insert_after_line": 487,  # Après export_state()
        "method": '''
    
    def get_full_state(self) -> Dict:
        """Obtenir état complet du système cognitif (forensic-grade, API standard)."""
        return self.get_global_state()
'''
    }
}


def apply_correction(filepath: str, insert_after_line: int, method_code: str) -> bool:
    """Appliquer une correction à un fichier."""
    try:
        # Lire fichier
        with open(filepath, 'r') as f:
            lines = f.readlines()
        
        # Insérer méthode
        lines.insert(insert_after_line, method_code)
        
        # Écrire fichier
        with open(filepath, 'w') as f:
            f.writelines(lines)
        
        print(f"✅ {os.path.basename(filepath)}: Correction appliquée")
        return True
        
    except Exception as e:
        print(f"❌ {os.path.basename(filepath)}: Erreur - {e}")
        return False


def main():
    """Appliquer toutes les corrections Phase 1."""
    base_dir = "lumvorax2/src/MAGEN/core"
    
    print("=" * 60)
    print("CORRECTIONS FORENSIC PHASE 1 - V32")
    print("=" * 60)
    print()
    
    success_count = 0
    total_count = len(CORRECTIONS)
    
    for filename, correction in CORRECTIONS.items():
        filepath = os.path.join(base_dir, filename)
        
        if not os.path.exists(filepath):
            print(f"⚠️  {filename}: Fichier non trouvé")
            continue
        
        if apply_correction(
            filepath,
            correction["insert_after_line"],
            correction["method"]
        ):
            success_count += 1
    
    print()
    print("=" * 60)
    print(f"RÉSULTAT: {success_count}/{total_count} corrections appliquées")
    print("=" * 60)
    
    if success_count == total_count:
        print("✅ Phase 1 COMPLÈTE - Tous les modules corrigés")
        return 0
    else:
        print(f"⚠️  Phase 1 PARTIELLE - {total_count - success_count} échecs")
        return 1


if __name__ == "__main__":
    exit(main())

# Made with Bob
