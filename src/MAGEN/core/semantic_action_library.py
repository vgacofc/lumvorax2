"""
SEMANTIC ACTION LIBRARY - MAGEN V24
Bibliothèque actions sémantiques avec primitives interaction

PROBLÈME V23: Actions géométriques (rotate, mirror) inadaptées jeux arcade
SOLUTION V24: Primitives interaction contextuelles (move, shoot, collect)

Architecture:
- Action Semantics: Mapping action → effet causal attendu
- Context Awareness: Actions adaptées à état environnement
- Learning from Feedback: Mise à jour sémantique basée reward
- Primitive Discovery: Identification automatique primitives utiles

Forensic: Logs bit-level sémantique + timestamps nanoseconde
"""

import numpy as np
from typing import Dict, List, Tuple, Optional, Set, Callable
from dataclasses import dataclass, field
from collections import defaultdict
import time

@dataclass
class ActionSemantics:
    """Sémantique action avec effet causal attendu"""
    name: str
    category: str  # MOVEMENT, INTERACTION, MANIPULATION, TIMING, META
    expected_effect: str  # Description effet causal
    context_requirements: List[str] = field(default_factory=list)
    success_indicators: List[str] = field(default_factory=list)
    learned_from_feedback: bool = False
    confidence: float = 0.5  # [0, 1]
    usage_count: int = 0
    success_count: int = 0
    
    def update_from_feedback(self, success: bool):
        """Mise à jour sémantique basée feedback"""
        self.usage_count += 1
        if success:
            self.success_count += 1
            self.learned_from_feedback = True
        
        # Mise à jour confidence
        if self.usage_count > 0:
            self.confidence = self.success_count / self.usage_count


class SemanticActionLibrary:
    """
    Bibliothèque actions sémantiques avec primitives interaction
    
    Remplace transformations géométriques (V23) par primitives gameplay (V24)
    """
    
    def __init__(self):
        # Bibliothèque actions sémantiques
        self.action_library: Dict[str, ActionSemantics] = {}
        
        # Mapping actions environnement → sémantique
        self.env_to_semantic: Dict[str, str] = {}
        
        # Actions découvertes dynamiquement
        self.discovered_actions: Set[str] = set()
        
        # Statistiques forensiques
        self.stats = {
            'total_actions_registered': 0,
            'actions_by_category': defaultdict(int),
            'total_usage': 0,
            'total_successes': 0,
            'avg_confidence': 0.0,
            'primitives_discovered': 0
        }
        
        # Initialiser bibliothèque par défaut
        self._initialize_default_library()
        
        print("[SEMANTIC ACTION LIBRARY] Initialisé")
        print(f"  Actions par défaut: {len(self.action_library)}")
    
    def _initialize_default_library(self):
        """Initialise bibliothèque actions par défaut"""
        
        # CATEGORY: MOVEMENT (déplacement agent)
        self.register_action(ActionSemantics(
            name="move_up",
            category="MOVEMENT",
            expected_effect="Déplace agent vers haut",
            context_requirements=["agent_exists", "space_above_free"],
            success_indicators=["agent_y_decreased", "no_collision"]
        ))
        
        self.register_action(ActionSemantics(
            name="move_down",
            category="MOVEMENT",
            expected_effect="Déplace agent vers bas",
            context_requirements=["agent_exists", "space_below_free"],
            success_indicators=["agent_y_increased", "no_collision"]
        ))
        
        self.register_action(ActionSemantics(
            name="move_left",
            category="MOVEMENT",
            expected_effect="Déplace agent vers gauche",
            context_requirements=["agent_exists", "space_left_free"],
            success_indicators=["agent_x_decreased", "no_collision"]
        ))
        
        self.register_action(ActionSemantics(
            name="move_right",
            category="MOVEMENT",
            expected_effect="Déplace agent vers droite",
            context_requirements=["agent_exists", "space_right_free"],
            success_indicators=["agent_x_increased", "no_collision"]
        ))
        
        # CATEGORY: INTERACTION (interaction objets)
        self.register_action(ActionSemantics(
            name="interact",
            category="INTERACTION",
            expected_effect="Interagit avec objet proche",
            context_requirements=["agent_exists", "object_nearby"],
            success_indicators=["object_state_changed", "reward_increased"]
        ))
        
        self.register_action(ActionSemantics(
            name="collect",
            category="INTERACTION",
            expected_effect="Collecte objet",
            context_requirements=["agent_exists", "collectible_nearby"],
            success_indicators=["object_removed", "inventory_increased", "reward_increased"]
        ))
        
        self.register_action(ActionSemantics(
            name="shoot",
            category="INTERACTION",
            expected_effect="Tire projectile",
            context_requirements=["agent_exists", "ammo_available"],
            success_indicators=["projectile_created", "target_hit"]
        ))
        
        # CATEGORY: MANIPULATION (manipulation environnement)
        self.register_action(ActionSemantics(
            name="push",
            category="MANIPULATION",
            expected_effect="Pousse objet",
            context_requirements=["agent_exists", "pushable_object_nearby"],
            success_indicators=["object_moved", "no_collision"]
        ))
        
        self.register_action(ActionSemantics(
            name="pull",
            category="MANIPULATION",
            expected_effect="Tire objet",
            context_requirements=["agent_exists", "pullable_object_nearby"],
            success_indicators=["object_moved_toward_agent"]
        ))
        
        # CATEGORY: TIMING (actions temporelles)
        self.register_action(ActionSemantics(
            name="wait",
            category="TIMING",
            expected_effect="Attend sans agir",
            context_requirements=[],
            success_indicators=["time_advanced", "state_changed"]
        ))
        
        self.register_action(ActionSemantics(
            name="hold",
            category="TIMING",
            expected_effect="Maintient action précédente",
            context_requirements=["previous_action_exists"],
            success_indicators=["action_sustained"]
        ))
        
        # CATEGORY: META (actions méta-jeu)
        self.register_action(ActionSemantics(
            name="reset",
            category="META",
            expected_effect="Réinitialise niveau",
            context_requirements=[],
            success_indicators=["level_reset", "state_initial"]
        ))
        
        self.register_action(ActionSemantics(
            name="undo",
            category="META",
            expected_effect="Annule action précédente",
            context_requirements=["previous_action_exists"],
            success_indicators=["state_reverted"]
        ))
    
    def register_action(self, action: ActionSemantics):
        """Enregistre action dans bibliothèque"""
        self.action_library[action.name] = action
        self.stats['total_actions_registered'] += 1
        self.stats['actions_by_category'][action.category] += 1
    
    def map_env_action_to_semantic(self, env_action: str, semantic_name: str):
        """
        Mappe action environnement → action sémantique
        
        Exemple: "ACTION1" → "move_right"
        """
        if semantic_name not in self.action_library:
            print(f"[SEMANTIC LIB] Warning: {semantic_name} not in library")
            return
        
        self.env_to_semantic[env_action] = semantic_name
        print(f"[SEMANTIC LIB] Mapped {env_action} → {semantic_name}")
    
    def get_semantic_name(self, env_action: str) -> str:
        """Récupère nom sémantique action environnement"""
        return self.env_to_semantic.get(env_action, env_action)
    
    def get_semantic_action(self, env_action: str) -> Optional[ActionSemantics]:
        """Récupère sémantique action environnement"""
        semantic_name = self.env_to_semantic.get(env_action)
        if semantic_name:
            return self.action_library.get(semantic_name)
        return None
    
    def discover_action_semantic(
        self,
        env_action: str,
        observed_effect: str,
        success: bool
    ) -> ActionSemantics:
        """
        Découvre sémantique action dynamiquement
        
        Args:
            env_action: Action environnement (ex: "ACTION1")
            observed_effect: Effet observé
            success: Si action a réussi
        
        Returns:
            ActionSemantics découverte
        """
        # Vérifier si déjà mappée
        if env_action in self.env_to_semantic:
            semantic = self.get_semantic_action(env_action)
            if semantic:
                semantic.update_from_feedback(success)
                return semantic
        
        # Créer nouvelle sémantique
        semantic_name = f"discovered_{env_action.lower()}"
        
        # Inférer catégorie basée effet observé
        category = self._infer_category(observed_effect)
        
        semantic = ActionSemantics(
            name=semantic_name,
            category=category,
            expected_effect=observed_effect,
            context_requirements=[],
            success_indicators=[],
            learned_from_feedback=True,
            confidence=1.0 if success else 0.0,
            usage_count=1,
            success_count=1 if success else 0
        )
        
        # Enregistrer
        self.register_action(semantic)
        self.map_env_action_to_semantic(env_action, semantic_name)
        self.discovered_actions.add(env_action)
        self.stats['primitives_discovered'] += 1
        
        print(f"[SEMANTIC LIB] Discovered: {env_action} → {semantic_name} "
              f"(category={category}, success={success})")
        
        return semantic
    
    def _infer_category(self, observed_effect: str) -> str:
        """Infère catégorie basée effet observé"""
        effect_lower = observed_effect.lower()
        
        if any(kw in effect_lower for kw in ["move", "déplace", "position"]):
            return "MOVEMENT"
        elif any(kw in effect_lower for kw in ["interact", "collect", "shoot", "hit"]):
            return "INTERACTION"
        elif any(kw in effect_lower for kw in ["push", "pull", "manipul"]):
            return "MANIPULATION"
        elif any(kw in effect_lower for kw in ["wait", "hold", "time"]):
            return "TIMING"
        else:
            return "META"
    
    def update_action_feedback(self, env_action: str, success: bool):
        """Mise à jour sémantique basée feedback"""
        semantic = self.get_semantic_action(env_action)
        if semantic:
            semantic.update_from_feedback(success)
            
            self.stats['total_usage'] += 1
            if success:
                self.stats['total_successes'] += 1
    
    def get_actions_by_category(self, category: str) -> List[ActionSemantics]:
        """Retourne actions d'une catégorie"""
        return [
            action for action in self.action_library.values()
            if action.category == category
        ]
    
    def get_most_confident_actions(self, top_k: int = 5) -> List[ActionSemantics]:
        """Retourne top-k actions par confidence"""
        sorted_actions = sorted(
            self.action_library.values(),
            key=lambda a: a.confidence,
            reverse=True
        )
        return sorted_actions[:top_k]
    
    def get_stats(self) -> Dict:
        """Statistiques forensiques"""
        stats = self.stats.copy()
        
        # Calcul avg confidence
        if self.action_library:
            confidences = [a.confidence for a in self.action_library.values()]
            stats['avg_confidence'] = np.mean(confidences)
        
        # Détails actions
        stats['action_details'] = {}
        for name, action in self.action_library.items():
            stats['action_details'][name] = {
                'category': action.category,
                'usage_count': action.usage_count,
                'success_count': action.success_count,
                'confidence': action.confidence,
                'learned': action.learned_from_feedback
            }
        
        # Actions découvertes
        stats['discovered_actions'] = list(self.discovered_actions)
        
        return stats
    
    def print_stats(self):
        """Affiche statistiques forensiques"""
        stats = self.get_stats()
        
        print("\n[SEMANTIC ACTION LIBRARY] Statistiques:")
        print(f"  Total actions: {stats['total_actions_registered']}")
        print(f"  Actions by category:")
        for category, count in stats['actions_by_category'].items():
            print(f"    {category}: {count}")
        print(f"  Total usage: {stats['total_usage']}")
        print(f"  Total successes: {stats['total_successes']}")
        print(f"  Avg confidence: {stats['avg_confidence']:.3f}")
        print(f"  Primitives discovered: {stats['primitives_discovered']}")
        
        if stats['discovered_actions']:
            print(f"  Discovered actions: {', '.join(stats['discovered_actions'])}")
        
        # Top actions
        top_actions = self.get_most_confident_actions(top_k=5)
        if top_actions:
            print(f"\n  Top 5 actions by confidence:")
            for i, action in enumerate(top_actions, 1):
                print(f"    {i}. {action.name}: confidence={action.confidence:.3f}, "
                      f"usage={action.usage_count}, category={action.category}")

# Made with Bob
