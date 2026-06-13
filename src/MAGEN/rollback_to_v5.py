#!/usr/bin/env python3
"""
ROLLBACK AUTOMATIQUE VERS V5 - SESSION 55
Restaure la version V5 stable (45/400) après catastrophe V9 (3/400)

Protocole: CLAUDE_PILOT + LUMVORAX
Mode: 100% LOCAL
"""

import os
import shutil
import json
from datetime import datetime
from pathlib import Path

class RollbackManager:
    def __init__(self):
        self.base_dir = Path(__file__).parent
        self.backup_v5 = self.base_dir / "backups" / "session54_pre_gsf"
        self.backup_pre_rollback = self.base_dir / "backups" / f"session55_pre_rollback_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        self.log_file = self.base_dir / f"rollback_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        
    def create_pre_rollback_backup(self):
        """Backup automatique avant rollback (V9 catastrophique)"""
        print("🔄 ÉTAPE 1: Backup pré-rollback (V9 catastrophique)")
        
        self.backup_pre_rollback.mkdir(parents=True, exist_ok=True)
        
        files_to_backup = [
            "test_phase2_400_puzzles.py",
            "core/dynamic_entity_tracker.py",
            "core/global_solution_field.py",
            "core/magen_memory.py",
            "core/magen_pipeline.py",
            "synthesis/adaptive_strategy.py"
        ]
        
        backed_up = []
        for file_path in files_to_backup:
            src = self.base_dir / file_path
            if src.exists():
                dst = self.backup_pre_rollback / file_path
                dst.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(src, dst)
                backed_up.append(file_path)
                print(f"   ✅ Sauvegardé: {file_path}")
        
        # Sauvegarder les résultats V9
        v9_results = self.base_dir / "results_phase2_20260613_142905.json"
        if v9_results.exists():
            shutil.copy2(v9_results, self.backup_pre_rollback / "results_v9.json")
            backed_up.append("results_v9.json")
            print(f"   ✅ Sauvegardé: results_v9.json")
        
        print(f"✅ Backup pré-rollback créé: {len(backed_up)} fichiers")
        return backed_up
    
    def archive_v9_modules(self):
        """Archive (ne supprime pas) les modules V9 défaillants"""
        print("\n🔄 ÉTAPE 2: Archivage modules V9 défaillants")
        
        v9_modules = [
            "core/dynamic_entity_tracker.py",
            "core/global_solution_field.py"
        ]
        
        archive_dir = self.base_dir / "backups" / "v9_failed_modules"
        archive_dir.mkdir(parents=True, exist_ok=True)
        
        archived = []
        for module in v9_modules:
            src = self.base_dir / module
            if src.exists():
                dst = archive_dir / Path(module).name
                shutil.move(str(src), str(dst))
                archived.append(module)
                print(f"   ✅ Archivé: {module} → v9_failed_modules/")
        
        print(f"✅ Modules V9 archivés: {len(archived)} fichiers")
        return archived
    
    def restore_v5_files(self):
        """Restaure les fichiers V5 depuis backup"""
        print("\n🔄 ÉTAPE 3: Restauration fichiers V5 (45/400)")
        
        if not self.backup_v5.exists():
            raise FileNotFoundError(f"Backup V5 introuvable: {self.backup_v5}")
        
        restored = []
        
        # Restaurer test_phase2_400_puzzles.py
        src = self.backup_v5 / "test_phase2_400_puzzles.py"
        dst = self.base_dir / "test_phase2_400_puzzles.py"
        if src.exists():
            shutil.copy2(src, dst)
            restored.append("test_phase2_400_puzzles.py")
            print(f"   ✅ Restauré: test_phase2_400_puzzles.py")
        
        # Restaurer core/
        core_files = [
            "__init__.py",
            "advanced_pipeline.py",
            "cognitive_strategy_map.py",
            "color_learning_hierarchy.py",
            "failure_memory.py",
            "invariant_extractor.py",
            "learning_memory.py",
            "magen_memory.py",
            "magen_pipeline.py",
            "parallel_world_frame_engine.py",
            "pattern_matcher.py",
            "predictive_simulator.py",
            "safe_operations.py",
            "shape_transformer.py",
            "trajectory_divergence_analyzer.py",
            "transform_detector.py",
            "transformation_space.py"
        ]
        
        for file_name in core_files:
            src = self.backup_v5 / "core" / file_name
            dst = self.base_dir / "core" / file_name
            if src.exists():
                shutil.copy2(src, dst)
                restored.append(f"core/{file_name}")
                print(f"   ✅ Restauré: core/{file_name}")
        
        # Restaurer synthesis/
        synthesis_files = [
            "__init__.py",
            "adaptive_strategy.py",
            "classified_synthesizer.py",
            "scorer.py",
            "synthesizer.py",
            "validator.py"
        ]
        
        for file_name in synthesis_files:
            src = self.backup_v5 / "synthesis" / file_name
            dst = self.base_dir / "synthesis" / file_name
            if src.exists():
                shutil.copy2(src, dst)
                restored.append(f"synthesis/{file_name}")
                print(f"   ✅ Restauré: synthesis/{file_name}")
        
        print(f"✅ Fichiers V5 restaurés: {len(restored)} fichiers")
        return restored
    
    def verify_restoration(self):
        """Vérifie que la restauration est complète"""
        print("\n🔄 ÉTAPE 4: Vérification restauration")
        
        critical_files = [
            "test_phase2_400_puzzles.py",
            "core/magen_memory.py",
            "core/magen_pipeline.py",
            "core/parallel_world_frame_engine.py",
            "synthesis/adaptive_strategy.py"
        ]
        
        missing = []
        for file_path in critical_files:
            if not (self.base_dir / file_path).exists():
                missing.append(file_path)
        
        # Vérifier que V9 modules sont archivés
        v9_still_present = []
        for module in ["core/dynamic_entity_tracker.py", "core/global_solution_field.py"]:
            if (self.base_dir / module).exists():
                v9_still_present.append(module)
        
        if missing:
            print(f"   ❌ Fichiers manquants: {missing}")
            return False
        
        if v9_still_present:
            print(f"   ⚠️ Modules V9 encore présents: {v9_still_present}")
            return False
        
        print("   ✅ Tous les fichiers critiques présents")
        print("   ✅ Modules V9 correctement archivés")
        return True
    
    def generate_log(self, backed_up, archived, restored, success):
        """Génère log forensique du rollback"""
        print("\n🔄 ÉTAPE 5: Génération log forensique")
        
        log_data = {
            "timestamp": datetime.now().isoformat(),
            "operation": "ROLLBACK_V9_TO_V5",
            "protocol": "CLAUDE_PILOT + LUMVORAX",
            "mode": "100% LOCAL",
            "reason": "V9 CATASTROPHE: 3/400 (0.8%) vs V5: 45/400 (11.25%)",
            "regression": "-93.3%",
            "pre_rollback_backup": str(self.backup_pre_rollback),
            "files_backed_up": backed_up,
            "v9_modules_archived": archived,
            "v5_files_restored": restored,
            "verification_success": success,
            "next_steps": [
                "Implémenter système backup automatique",
                "Implémenter architecture bidirectionnelle (méta-cognition)",
                "Appliquer corrections Session 52 (#3 et #4)",
                "Test V10 avec corrections ciblées"
            ]
        }
        
        with open(self.log_file, 'w') as f:
            json.dump(log_data, f, indent=2)
        
        print(f"✅ Log forensique généré: {self.log_file.name}")
        return log_data
    
    def execute_rollback(self):
        """Exécute le rollback complet"""
        print("=" * 70)
        print("🔥 ROLLBACK AUTOMATIQUE V9 → V5")
        print("=" * 70)
        print(f"Raison: V9 CATASTROPHE (3/400) vs V5 STABLE (45/400)")
        print(f"Régression: -93.3%")
        print(f"Timestamp: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print("=" * 70)
        
        try:
            # Étape 1: Backup pré-rollback
            backed_up = self.create_pre_rollback_backup()
            
            # Étape 2: Archiver modules V9
            archived = self.archive_v9_modules()
            
            # Étape 3: Restaurer V5
            restored = self.restore_v5_files()
            
            # Étape 4: Vérification
            success = self.verify_restoration()
            
            # Étape 5: Log forensique
            log_data = self.generate_log(backed_up, archived, restored, success)
            
            print("\n" + "=" * 70)
            if success:
                print("✅ ROLLBACK RÉUSSI - V5 RESTAURÉ (45/400)")
                print("=" * 70)
                print("\n📊 RÉSUMÉ:")
                print(f"   • Fichiers sauvegardés (V9): {len(backed_up)}")
                print(f"   • Modules V9 archivés: {len(archived)}")
                print(f"   • Fichiers V5 restaurés: {len(restored)}")
                print(f"   • Backup V9: {self.backup_pre_rollback.name}")
                print(f"   • Log forensique: {self.log_file.name}")
                print("\n🎯 PROCHAINES ÉTAPES:")
                print("   1. Implémenter système backup automatique")
                print("   2. Implémenter architecture bidirectionnelle")
                print("   3. Appliquer corrections Session 52 (#3 et #4)")
                print("   4. Test V10 avec corrections ciblées")
                return True
            else:
                print("❌ ROLLBACK INCOMPLET - VÉRIFICATION MANUELLE REQUISE")
                print("=" * 70)
                return False
                
        except Exception as e:
            print(f"\n❌ ERREUR ROLLBACK: {e}")
            print(f"Backup pré-rollback disponible: {self.backup_pre_rollback}")
            return False

if __name__ == "__main__":
    manager = RollbackManager()
    success = manager.execute_rollback()
    exit(0 if success else 1)

# Made with Bob
