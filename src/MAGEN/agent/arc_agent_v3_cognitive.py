"""
MAGEN - Agent ARC-AGI-3 V3 COGNITIVE
Agent avec raisonnement par hypothèses et analyse training pairs

RÉVOLUTION V3 (CLAUDE PILOT):
- ✅ Analyse automatique training pairs (ARCRuleExtractor)
- ✅ Génération et test d'hypothèses (ARCHypothesisTester)
- ✅ Raisonnement abstrait sur transformations
- ✅ Prédiction basée sur règles extraites
- ✅ Planification d'actions intelligente
- ✅ Forensic logging nanoseconde par nanoseconde
"""

import numpy as np
from typing import List, Dict, Optional, Tuple, Any
import random
import time
from pathlib import Path
from collections import defaultdict, deque

# Imports locaux
import sys
sys.path.append(str(Path(__file__).parent.parent))
from core.magen_memory import MAGENMemory, Experience
from perception.arc_perception import ARCPerception
from agent.arc_rule_extractor import ARCRuleExtractor, TransformationHypothesis
from agent.arc_hypothesis_tester import ARCHypothesisTester, HypothesisTestResult
from agent.arc_action_planner import ARCActionPlanner, ActionPlan
from forensic.cognitive_logger import CognitiveLogger


class ARCAgentV3Cognitive:
    """
    Agent ARC-AGI-3 V3 avec raisonnement cognitif
    
    Architecture cognitive:
    1. Analyser training pairs → extraire règles
    2. Générer hypothèses de transformation
    3. Tester hypothèses sur training pairs
    4. Appliquer meilleure hypothèse au test
    5. Planifier actions pour atteindre l'objectif
    
    DIFFÉRENCE MAJEURE vs V2:
    - V2: Actions aléatoires basées sur features
    - V3: Raisonnement abstrait + prédiction + planification
    """
    
    def __init__(self,
                 memory: MAGENMemory,
                 perception: ARCPerception,
                 exploration_rate: float = 0.1,  # Très réduit (exploitation cognitive)
                 max_actions_per_level: int = 200,
                 verbose: bool = True):
        """
        Initialisation agent ARC V3 Cognitive
        
        Args:
            memory: Système mémoire MAGEN
            perception: Système perception ARC
            exploration_rate: Taux exploration [0-1] (très réduit)
            max_actions_per_level: Max actions par niveau
            verbose: Afficher logs détaillés
        """
        self.memory = memory
        self.perception = perception
        self.exploration_rate = exploration_rate
        self.max_actions_per_level = max_actions_per_level
        self.verbose = verbose
        
        # Modules cognitifs
        self.rule_extractor = ARCRuleExtractor(verbose=verbose)
        self.hypothesis_tester = ARCHypothesisTester(verbose=verbose)
        self.action_planner = ARCActionPlanner(verbose=verbose)
        
        # Logger forensique cognitif
        self.cognitive_logger: Optional[CognitiveLogger] = None
        
        # État cognitif
        self.current_game_id: Optional[str] = None
        self.training_pairs_analyzed: bool = False
        self.best_hypothesis: Optional[TransformationHypothesis] = None
        self.predicted_output: Optional[np.ndarray] = None
        self.action_plan: List[int] = []
        
        # Statistiques
        self.total_actions = 0
        self.successful_levels = 0
        self.failed_levels = 0
        self.total_levels = 0
        self.patterns_used = 0
        self.patterns_successful = 0
        
        # Historique par jeu
        self.game_history: Dict[str, Dict] = {}
        
        if self.verbose:
            print(f"[ARC Agent V3] 🧠 Initialisé (COGNITIVE)")
            print(f"[ARC Agent V3] Exploration rate: {self.exploration_rate}")
            print(f"[ARC Agent V3] Max actions/level: {self.max_actions_per_level}")
            print(f"[ARC Agent V3] Modules: RuleExtractor + HypothesisTester")
    
    def start_new_game(self, game_id: str, log_dir: Optional[Path] = None):
        """Démarrer un nouveau jeu"""
        self.current_game_id = game_id
        self.training_pairs_analyzed = False
        self.best_hypothesis = None
        self.predicted_output = None
        self.action_plan = []
        
        # Réinitialiser modules cognitifs
        self.rule_extractor = ARCRuleExtractor(verbose=self.verbose)
        self.hypothesis_tester = ARCHypothesisTester(verbose=self.verbose)
        self.action_planner = ARCActionPlanner(verbose=self.verbose)
        
        # Initialiser logger forensique cognitif
        if log_dir:
            self.cognitive_logger = CognitiveLogger(log_dir=log_dir, game_id=game_id)
            if self.verbose:
                print(f"[ARC Agent V3] 📝 CognitiveLogger activé: {self.cognitive_logger.log_file}")
        
        # Initialiser historique jeu
        if game_id not in self.game_history:
            self.game_history[game_id] = {
                "attempts": 0,
                "successes": 0,
                "best_hypothesis": None,
                "action_sequences": []
            }
        
        self.game_history[game_id]["attempts"] += 1
        
        if self.verbose:
            print(f"\n[ARC Agent V3] 🎮 Nouveau jeu: {game_id}")
            print(f"[ARC Agent V3] Tentative #{self.game_history[game_id]['attempts']}")
    
    def analyze_training_pairs(self, training_pairs: List[Tuple[np.ndarray, np.ndarray]]):
        """
        Analyser les training pairs pour extraire les règles
        
        Args:
            training_pairs: Liste de (input_grid, output_grid)
        """
        if self.verbose:
            print(f"\n[ARC Agent V3] 🔬 ANALYSE TRAINING PAIRS")
            print(f"[ARC Agent V3] Nombre de pairs: {len(training_pairs)}")
        
        # Ajouter chaque pair au rule extractor
        for i, (inp, out) in enumerate(training_pairs):
            self.rule_extractor.add_training_pair(inp, out, pair_id=i+1)
            self.hypothesis_tester.add_training_pair(inp, out, pair_id=i+1)
        
        # Extraire les règles
        hypotheses = self.rule_extractor.extract_rules()
        
        if hypotheses:
            self.best_hypothesis = hypotheses[0]  # Meilleure confiance
            
            # Log forensique: Hypothèse générée
            if self.cognitive_logger:
                alternatives = [f"{h.type.value} ({h.confidence:.1%})" for h in hypotheses[1:3]]
                self.cognitive_logger.log_hypothesis_generated(
                    hypothesis_type=self.best_hypothesis.type.value,
                    confidence=self.best_hypothesis.confidence,
                    parameters=self.best_hypothesis.parameters,
                    reasoning=self.best_hypothesis.description,
                    alternatives_considered=alternatives
                )
            
            if self.verbose:
                print(f"\n[ARC Agent V3] ✅ Règle identifiée:")
                print(f"   Type: {self.best_hypothesis.type.value}")
                print(f"   Confiance: {self.best_hypothesis.confidence:.1%}")
                print(f"   Description: {self.best_hypothesis.description}")
        else:
            if self.verbose:
                print(f"[ARC Agent V3] ⚠️ Aucune règle claire identifiée")
        
        self.training_pairs_analyzed = True
    
    def predict_test_output(self, test_input: np.ndarray) -> Optional[np.ndarray]:
        """
        Prédire l'output pour le test input
        
        Args:
            test_input: Grille de test
            
        Returns:
            Grille output prédite (ou None si échec)
        """
        if not self.training_pairs_analyzed:
            if self.verbose:
                print(f"[ARC Agent V3] ⚠️ Training pairs non analysés")
            return None
        
        if self.verbose:
            print(f"\n[ARC Agent V3] 🔮 PRÉDICTION TEST OUTPUT")
            print(f"[ARC Agent V3] Test input shape: {test_input.shape}")
        
        # Tester toutes les hypothèses
        results = self.hypothesis_tester.test_all_hypotheses(test_input)
        
        if not results:
            if self.verbose:
                print(f"[ARC Agent V3] ❌ Aucune prédiction possible")
            return None
        
        # Obtenir meilleure prédiction
        best_result = results[0]
        self.predicted_output = best_result.predicted_output
        
        # Log forensique: Prédiction générée
        if self.cognitive_logger and self.predicted_output is not None:
            self.cognitive_logger.log_prediction_generated(
                input_shape=test_input.shape,
                output_shape=self.predicted_output.shape,
                confidence=best_result.test_confidence,
                validation_score=best_result.validation_score,
                reasoning=f"Appliqué transformation {best_result.hypothesis.type.value}"
            )
        
        if self.verbose:
            print(f"\n[ARC Agent V3] ✅ Prédiction générée:")
            print(f"   Validation score: {best_result.validation_score:.1%}")
            print(f"   Test confidence: {best_result.test_confidence:.1%}")
            print(f"   Output shape: {self.predicted_output.shape if self.predicted_output is not None else 'None'}")
        
        return self.predicted_output
    
    def choose_action(self, 
                     grid: np.ndarray, 
                     available_actions: List[int],
                     training_pairs: Optional[List[Tuple[np.ndarray, np.ndarray]]] = None,
                     test_input: Optional[np.ndarray] = None) -> int:
        """
        Choisir une action de manière cognitive
        
        Args:
            grid: Grille actuelle
            available_actions: Actions disponibles
            training_pairs: Paires d'entraînement (si disponibles)
            test_input: Input de test (si disponible)
            
        Returns:
            ID de l'action choisie
        """
        self.total_actions += 1
        
        # Si training pairs fournis et pas encore analysés, les analyser
        if training_pairs and not self.training_pairs_analyzed:
            self.analyze_training_pairs(training_pairs)
            
            # Si test input fourni, faire prédiction
            if test_input is not None:
                self.predict_test_output(test_input)
        
        # Stratégie cognitive si prédiction disponible
        if self.predicted_output is not None and len(self.action_plan) == 0:
            # Planifier actions pour atteindre predicted_output
            try:
                plan = self.action_planner.plan_actions(
                    current_grid=grid,
                    predicted_output=self.predicted_output,
                    available_actions=available_actions,
                    training_pairs=training_pairs
                )
                self.action_plan = plan.actions
                
                # Log forensique: Plan d'actions généré
                if self.cognitive_logger:
                    self.cognitive_logger.log_action_plan_generated(
                        plan=plan.actions,
                        subgoals=plan.subgoals,
                        confidence=plan.confidence,
                        reasoning=plan.reasoning
                    )
                
                if self.verbose:
                    print(f"[ARC Agent V3] 📋 Plan généré: {len(self.action_plan)} actions")
                    print(f"   Confiance: {plan.confidence:.1%}")
                    print(f"   Sous-objectifs: {plan.subgoals[:3]}")
            except Exception as e:
                if self.verbose:
                    print(f"[ARC Agent V3] ⚠️ Échec planification: {e}")
                self.action_plan = []
        
        # Si plan disponible, suivre le plan
        if len(self.action_plan) > 0:
            action = self.action_plan.pop(0)
            if self.verbose:
                print(f"[ARC Agent V3] 🎯 Exécution plan: action {action} ({len(self.action_plan)} restantes)")
            return action
        
        # Extraire features de la grille actuelle
        features = self.perception.extract_features(grid)
        
        # FILTRER ACTION6 QUI CRASH bp35 (comparer .value pour objets GameAction)
        safe_actions = [a for a in available_actions if (isinstance(a, int) and a != 6) or (hasattr(a, 'value') and a.value != 6)]
        if not safe_actions:
            safe_actions = available_actions  # Fallback si tous filtrés
        
        # Décision: exploration vs exploitation
        if random.random() < self.exploration_rate:
            # Exploration: action aléatoire
            action = random.choice(safe_actions)
            if self.verbose:
                print(f"[ARC Agent V3] 🎲 Exploration: action {action}")
        else:
            # Exploitation: action basée sur features + hypothèse
            if self.best_hypothesis:
                # Utiliser l'hypothèse pour guider le choix
                # Pour l'instant, choisir action qui semble cohérente
                action = self._choose_hypothesis_guided_action(
                    grid, features, safe_actions
                )
                if self.verbose:
                    print(f"[ARC Agent V3] 🧠 Exploitation cognitive: action {action}")
            else:
                # Pas d'hypothèse, choisir basé sur features
                action = self._choose_feature_based_action(
                    features, safe_actions
                )
                if self.verbose:
                    print(f"[ARC Agent V3] 📊 Exploitation features: action {action}")
        
        return action
    
    def _choose_hypothesis_guided_action(self,
                                        grid: np.ndarray,
                                        features: Dict,
                                        available_actions: List[int]) -> int:
        """Choisir action guidée par l'hypothèse"""
        # Si l'hypothèse suggère une transformation géométrique,
        # privilégier les actions qui semblent faire des transformations
        
        # Pour l'instant, stratégie simple: varier les actions
        # TODO: Implémenter logique plus sophistiquée
        return random.choice(available_actions)
    
    def _choose_feature_based_action(self,
                                    features: Dict,
                                    available_actions: List[int]) -> int:
        """Choisir action basée sur features"""
        # Stratégie simple: varier les actions
        return random.choice(available_actions)
    
    def record_level_complete(self, success: bool, actions_taken: int):
        """Enregistrer la complétion d'un niveau"""
        self.total_levels += 1
        
        if success:
            self.successful_levels += 1
            if self.verbose:
                print(f"[ARC Agent V3] ✅ Niveau réussi en {actions_taken} actions")
            
            # Enregistrer succès dans historique
            if self.current_game_id:
                self.game_history[self.current_game_id]["successes"] += 1
                if self.best_hypothesis:
                    self.game_history[self.current_game_id]["best_hypothesis"] = self.best_hypothesis
        else:
            self.failed_levels += 1
            if self.verbose:
                print(f"[ARC Agent V3] ❌ Niveau échoué après {actions_taken} actions")
    
    def get_statistics(self) -> Dict:
        """Obtenir statistiques de l'agent"""
        success_rate = self.successful_levels / self.total_levels if self.total_levels > 0 else 0.0
        avg_actions = self.total_actions / self.total_levels if self.total_levels > 0 else 0.0
        pattern_success_rate = self.patterns_successful / self.patterns_used if self.patterns_used > 0 else 0.0
        
        return {
            "total_actions": self.total_actions,
            "successful_levels": self.successful_levels,
            "failed_levels": self.failed_levels,
            "total_levels": self.total_levels,
            "success_rate": success_rate,
            "avg_actions_per_level": avg_actions,
            "exploration_rate": self.exploration_rate,
            "patterns_used": self.patterns_used,
            "patterns_successful": self.patterns_successful,
            "pattern_success_rate": pattern_success_rate,
            "unique_games_learned": len(self.game_history),
            "total_patterns_learned": sum(
                len(h.get("action_sequences", [])) 
                for h in self.game_history.values()
            )
        }
    
    def save_patterns(self, filepath: Path):
        """Sauvegarder les patterns appris"""
        import json
        
        # Convertir game_history en format sérialisable
        serializable_history = {}
        for game_id, history in self.game_history.items():
            serializable_history[game_id] = {
                "attempts": history["attempts"],
                "successes": history["successes"],
                "action_sequences": history["action_sequences"],
                "best_hypothesis": {
                    "type": history["best_hypothesis"].type.value,
                    "confidence": history["best_hypothesis"].confidence,
                    "description": history["best_hypothesis"].description
                } if history["best_hypothesis"] else None
            }
        
        with open(filepath, 'w') as f:
            json.dump(serializable_history, f, indent=2)
        
        if self.verbose:
            print(f"[ARC Agent V3] 💾 Patterns sauvegardés: {filepath}")
    
    def load_patterns(self, filepath: Path):
        """Charger les patterns appris"""
        import json
        
        if not filepath.exists():
            if self.verbose:
                print(f"[ARC Agent V3] ⚠️ Fichier patterns introuvable: {filepath}")
            return
        
        with open(filepath, 'r') as f:
            loaded_history = json.load(f)
        
        # Reconstruire game_history (sans les hypothèses pour l'instant)
        for game_id, history in loaded_history.items():
            self.game_history[game_id] = {
                "attempts": history["attempts"],
                "successes": history["successes"],
                "action_sequences": history["action_sequences"],
                "best_hypothesis": None  # TODO: Reconstruire depuis JSON
            }
        
        if self.verbose:
            print(f"[ARC Agent V3] 📂 Patterns chargés: {len(self.game_history)} jeux")


if __name__ == "__main__":
    # Test simple
    print("🧪 Test ARCAgentV3Cognitive")
    
    # Créer composants
    from pathlib import Path
    save_dir = Path("/tmp/magen_test")
    save_dir.mkdir(exist_ok=True)
    
    memory = MAGENMemory(save_dir=save_dir)
    perception = ARCPerception(max_grid_size=64)
    
    # Créer agent V3
    agent = ARCAgentV3Cognitive(
        memory=memory,
        perception=perception,
        verbose=True
    )
    
    # Test avec training pairs simulés (rotation 90°)
    print("\n" + "="*70)
    print("TEST: Rotation 90°")
    print("="*70)
    
    agent.start_new_game("test_rotation_90")
    
    # Training pairs
    inp1 = np.array([[1, 2], [3, 4]])
    out1 = np.rot90(inp1, k=1)
    
    inp2 = np.array([[5, 6], [7, 8]])
    out2 = np.rot90(inp2, k=1)
    
    training_pairs = [(inp1, out1), (inp2, out2)]
    
    # Test input
    test_input = np.array([[9, 10], [11, 12]])
    
    # Analyser et prédire
    agent.analyze_training_pairs(training_pairs)
    predicted = agent.predict_test_output(test_input)
    
    if predicted is not None:
        print(f"\n✅ Prédiction réussie:")
        print(f"Input:\n{test_input}")
        print(f"\nPredicted:\n{predicted}")
        print(f"\nExpected:\n{np.rot90(test_input, k=1)}")
        
        # Vérifier si correct
        if np.array_equal(predicted, np.rot90(test_input, k=1)):
            print(f"\n🎉 PRÉDICTION CORRECTE!")
        else:
            print(f"\n❌ Prédiction incorrecte")
    
    # Statistiques
    print(f"\n📊 Statistiques:")
    stats = agent.get_statistics()
    for key, value in stats.items():
        print(f"   {key}: {value}")

# Made with Bob
