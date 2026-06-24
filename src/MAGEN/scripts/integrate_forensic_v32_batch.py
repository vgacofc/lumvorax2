#!/usr/bin/env python3
"""
Script d'intégration automatique du forensic bit-level dans tous les modules V32.

Conforme: LEÇON-60.1, 60.2, 61.1, 72.2 (Standard LumVorax)
"""

import re
from pathlib import Path
from typing import Dict, List, Tuple

# Configuration modules V32
MODULES_CONFIG = {
    "mental_map_builder.py": {
        "class_name": "MentalMapBuilder",
        "methods_to_instrument": ["update", "form_region", "detect_pattern"],
        "state_fields": ["current_step", "total_positions", "total_regions", "total_patterns"]
    },
    "spatial_compressor.py": {
        "class_name": "SpatialCompressor",
        "methods_to_instrument": ["compress_trajectory", "identify_pattern", "label_cognitive"],
        "state_fields": ["total_patterns", "pattern_diversity", "total_trajectories"]
    },
    "regional_strategist.py": {
        "class_name": "RegionalStrategist",
        "methods_to_instrument": ["evaluate_region", "compute_strategic_value", "assign_cognitive_status"],
        "state_fields": ["total_evaluations", "regions_evaluated", "avg_strategic_value"]
    },
    "cognitive_stabilizer.py": {
        "class_name": "CognitiveStabilizer",
        "methods_to_instrument": ["check_policy_break", "prevent_break", "should_change_strategy"],
        "state_fields": ["total_steps", "total_breaks", "break_frequency", "breaks_prevented"]
    },
    "metacognitive_monitor.py": {
        "class_name": "MetaCognitiveMonitor",
        "methods_to_instrument": ["evaluate_cognitive_health", "assess_map_coherence", "detect_cognitive_issues"],
        "state_fields": ["total_evaluations", "avg_cognitive_health", "issues_detected"]
    },
    "self_spatial_identity.py": {
        "class_name": "SelfSpatialIdentity",
        "methods_to_instrument": ["update_from_observation", "lock_avatar", "generate_hypothesis"],
        "state_fields": ["total_updates", "avatar_confidence", "spatial_awareness", "hypotheses_generated"]
    },
    "goal_hypothesis_engine.py": {
        "class_name": "GoalHypothesisEngine",
        "methods_to_instrument": ["update", "generate_hypothesis", "validate_hypothesis"],
        "state_fields": ["total_generated", "validated_count", "active_hypotheses_count"]
    },
    "trajectory_meaning_system.py": {
        "class_name": "TrajectoryMeaningSystem",
        "methods_to_instrument": ["update", "analyze_trajectory", "extract_meaning"],
        "state_fields": ["total_trajectories", "total_segments", "avg_intention_confidence"]
    },
    "spatial_knowledge_graph.py": {
        "class_name": "SpatialKnowledgeGraph",
        "methods_to_instrument": ["update_from_observation", "add_node", "find_path"],
        "state_fields": ["total_nodes", "total_edges", "total_queries"]
    }
}

BASE_PATH = Path(__file__).parent.parent / "core"


def generate_forensic_init(class_name: str) -> str:
    """Générer code d'initialisation forensic."""
    return f'''
        # Setup Forensic Middleware (Standard LumVorax)
        if forensic_log_path is None:
            forensic_log_path = "logs/magen_v32/{class_name.lower()}.jsonl"
        
        Path(forensic_log_path).parent.mkdir(parents=True, exist_ok=True)
        self.forensic = ForensicMiddleware(
            forensic_log_path,
            "{class_name}"
        )
        
        # Log initialization
        self.forensic.forensic_logger.log(
            "module_initialized",
            "{class_name}",
            {{
                "timestamp_ns": time.time_ns(),
                "forensic_enabled": True,
                "forensic_log_path": forensic_log_path
            }}
        )
'''


def generate_forensic_state_property(state_fields: List[str]) -> str:
    """Générer propriété _forensic_state."""
    fields_dict = ",\n            ".join([f"'{field}': self.{field}" for field in state_fields])
    return f'''
    @property
    def _forensic_state(self) -> Dict:
        """État forensic pour instrumentation."""
        return {{
            {fields_dict}
        }}
'''


def generate_forensic_close(class_name: str, state_fields: List[str]) -> str:
    """Générer méthode close()."""
    fields_dict = ",\n                ".join([f"'{field}': self.{field}" for field in state_fields])
    return f'''
    def close(self) -> None:
        """
        Fermer proprement le module et le forensic logger.
        
        FORENSIC: Conforme LEÇON-60.1, 60.2, 61.1, 72.2
        """
        # Log fermeture
        self.forensic.forensic_logger.log(
            "module_closing",
            "{class_name}",
            {{
                {fields_dict},
                "uptime_ns": time.time_ns() - getattr(self, 'creation_time', time.time_ns())
            }}
        )
        
        # Fermer forensic logger
        self.forensic.forensic_logger.close()
'''


def add_forensic_imports(content: str) -> str:
    """Ajouter imports forensic si absents."""
    if "from .forensic_middleware import ForensicMiddleware" in content:
        return content
    
    # Trouver la dernière ligne d'import
    import_lines = []
    other_lines = []
    in_imports = True
    
    for line in content.split('\n'):
        if in_imports and (line.startswith('import ') or line.startswith('from ')):
            import_lines.append(line)
        else:
            if line.strip() and not line.startswith('#') and not line.startswith('"""'):
                in_imports = False
            other_lines.append(line)
    
    # Ajouter imports forensic
    import_lines.extend([
        "",
        "# Forensic imports (Standard LumVorax)",
        "from .forensic_middleware import ForensicMiddleware",
        "from pathlib import Path",
        "import time"
    ])
    
    return '\n'.join(import_lines) + '\n' + '\n'.join(other_lines)


def modify_init_signature(content: str, class_name: str) -> str:
    """Modifier signature __init__ pour ajouter forensic_log_path."""
    # Pattern pour trouver __init__
    pattern = rf'(class {class_name}.*?def __init__\(self(?:, logger=None)?)\)'
    
    def replacer(match):
        return match.group(1) + ', forensic_log_path: Optional[str] = None):'
    
    return re.sub(pattern, replacer, content, flags=re.DOTALL)


def integrate_forensic_in_module(module_file: str, config: Dict) -> Tuple[bool, str]:
    """
    Intégrer forensic dans un module.
    
    Returns:
        (success, message)
    """
    filepath = BASE_PATH / module_file
    
    if not filepath.exists():
        return False, f"File not found: {filepath}"
    
    try:
        # Lire contenu
        content = filepath.read_text()
        
        # Vérifier si déjà intégré
        if "ForensicMiddleware" in content and "self.forensic =" in content:
            return True, f"Already integrated: {module_file}"
        
        class_name = config["class_name"]
        
        # 1. Ajouter imports
        content = add_forensic_imports(content)
        
        # 2. Modifier signature __init__
        content = modify_init_signature(content, class_name)
        
        # 3. Ajouter initialisation forensic dans __init__
        # Trouver fin de __init__ (première ligne vide après self.logger)
        init_pattern = rf'(def __init__.*?self\.logger = logger)'
        init_code = generate_forensic_init(class_name)
        content = re.sub(init_pattern, rf'\1{init_code}', content, flags=re.DOTALL)
        
        # 4. Ajouter propriété _forensic_state après __init__
        state_property = generate_forensic_state_property(config["state_fields"])
        # Trouver fin de __init__ et ajouter après
        content = re.sub(
            r'(def __init__.*?\n(?:        .*\n)*)',
            rf'\1{state_property}\n',
            content,
            count=1,
            flags=re.DOTALL
        )
        
        # 5. Ajouter méthode close() à la fin de la classe
        close_method = generate_forensic_close(class_name, config["state_fields"])
        # Trouver dernière méthode de la classe
        content = re.sub(
            r'(\n\n# Made with Bob)',
            rf'{close_method}\n\n# Made with Bob - FORENSIC INTEGRATED',
            content
        )
        
        # Sauvegarder
        filepath.write_text(content)
        
        return True, f"Successfully integrated: {module_file}"
        
    except Exception as e:
        return False, f"Error in {module_file}: {str(e)}"


def main():
    """Intégrer forensic dans tous les modules V32."""
    print("=" * 80)
    print("INTÉGRATION FORENSIC V32 - BATCH PROCESSING")
    print("=" * 80)
    print()
    
    results = []
    success_count = 0
    
    for module_file, config in MODULES_CONFIG.items():
        print(f"Processing: {module_file}...")
        success, message = integrate_forensic_in_module(module_file, config)
        results.append((module_file, success, message))
        
        if success:
            success_count += 1
            print(f"  ✅ {message}")
        else:
            print(f"  ❌ {message}")
        print()
    
    # Résumé
    print("=" * 80)
    print(f"RÉSUMÉ: {success_count}/{len(MODULES_CONFIG)} modules intégrés avec succès")
    print("=" * 80)
    print()
    
    # Détails
    print("DÉTAILS:")
    for module_file, success, message in results:
        status = "✅" if success else "❌"
        print(f"  {status} {module_file}: {message}")
    
    print()
    print("PROCHAINES ÉTAPES:")
    print("  1. Vérifier logs générés dans logs/magen_v32/")
    print("  2. Exécuter tests intégration")
    print("  3. Valider hash chain intégrité")
    print()


if __name__ == "__main__":
    main()