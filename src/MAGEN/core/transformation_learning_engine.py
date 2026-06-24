"""
MAGEN V21 - Transformation Learning Engine
ARC3 COMPLIANT - Physical Loop Closure System

REFONTE COGNITIVE SESSION 57:
- Remplace Pattern Matcher comme source principale
- Implémente boucle physique fermée (try → transform → compare → update)
- Apprentissage des transformations par feedback réel
- Validation causale des actions

ROOT CAUSE V20 RÉSOLU:
"Tu as un bon cerveau (Meta-Arbiter) mais des mains faibles (transformations)"
→ V21 construit des mains qui apprennent

ARCHITECTURE:
    INPUT → ACTION DISCOVERY → TRANSFORMATION → VALIDATION → UPDATE → LOOP
"""

import numpy as np
from typing import List, Tuple, Dict, Optional, Callable, Any
from dataclasses import dataclass, field
from collections import defaultdict
import time

# C17+C18+C19: Trio cognitif décisionnel (Session 64)
from .action_reputation_system import ActionReputationSystem
from .exploration_budget_manager import ExplorationBudgetManager
from .trajectory_analyzer import TrajectoryAnalyzer


@dataclass
class TransformationAction:
    """Une action de transformation atomique"""
    name: str
    func: Callable[[np.ndarray], np.ndarray]
    preconditions: List[Callable[[np.ndarray], bool]] = field(default_factory=list)
    postconditions: List[Callable[[np.ndarray, np.ndarray], bool]] = field(default_factory=list)
    success_count: int = 0
    failure_count: int = 0
    avg_error: float = 1.0
    
    @property
    def success_rate(self) -> float:
        total = self.success_count + self.failure_count
        return self.success_count / total if total > 0 else 0.0
    
    @property
    def confidence(self) -> float:
        """Confiance basée sur succès et erreur moyenne"""
        return self.success_rate * (1.0 - self.avg_error)


@dataclass
class TransformationResult:
    """Résultat d'une transformation avec feedback"""
    output: np.ndarray
    action: TransformationAction
    error: float
    success: bool
    execution_time: float
    causal_consistency: bool = True


@dataclass
class LearningFeedback:
    """Feedback pour mise à jour du modèle"""
    action: TransformationAction
    input_grid: np.ndarray
    predicted_output: np.ndarray
    target_output: np.ndarray
    error: float
    success: bool
    timestamp: float


class TransformationLearningEngine:
    """
    Moteur d'apprentissage de transformations avec boucle physique fermée
    
    PRINCIPES ARC3:
    1. Action Discovery: Découvrir actions disponibles
    2. Physical Validation: Valider effet réel des actions
    3. Loop Closure: Boucle feedback complète
    4. Causal Learning: Apprendre structure causale
    5. Generalization: Généraliser à nouveaux cas
    """
    
    def __init__(self, verbose: bool = False, forensic_logger=None):
        self.verbose = verbose
        self.forensic_logger = forensic_logger
        
        # Action space (découvert dynamiquement)
        self.actions: Dict[str, TransformationAction] = {}
        
        # Learning history
        self.feedback_history: List[LearningFeedback] = []
        
        # Transformation memory (successful patterns)
        self.transformation_memory: Dict[str, List[Tuple[np.ndarray, np.ndarray, str]]] = defaultdict(list)
        
        # Performance metrics
        self.metrics = {
            'total_transformations': 0,
            'successful_transformations': 0,
            'failed_transformations': 0,
            'avg_error': 1.0,
            'learning_iterations': 0
        }
        
        # C6 FIX: Causal chain tracking
        self.current_causal_chain: List[str] = []
        self.causal_dependencies: Dict[str, List[str]] = defaultdict(list)
        
        # C6 FIX: Context-aware action filtering
        self.context_cache: Dict[str, List[str]] = {}
        self.action_success_history: Dict[str, List[bool]] = defaultdict(list)
        
        # C7 FIX: Identity failure blacklist
        self.identity_consecutive_failures: int = 0
        self.identity_blacklisted: bool = False
        self.identity_blacklist_threshold: int = 2  # C10: 5 → 2 (seuil agressif)
        
        # C9 FIX: High error tracking
        self.consecutive_high_errors: int = 0
        self.high_error_threshold: int = 2  # C10: 3 → 2 (seuil agressif)
        
        # C17+C18+C19: TRIO COGNITIF DÉCISIONNEL (Session 64)
        # PHASE 4.7.1: PARAMÈTRES CORRIGÉS - Budgets ×10, Early Stop ×4, Exploration Bonus
        try:
            self.reputation_system = ActionReputationSystem(
                forensic_log_path=None,  # Utiliser forensic_logger externe
                recent_window_size=10,
                min_confidence_attempts=5
            )
            self.budget_manager = ExplorationBudgetManager(
                base_budget_per_puzzle=1000,  # PHASE 4.7.1: ×10 (100→1000)
                min_budget_per_action=50,     # PHASE 4.7.1: ×10 (5→50)
                max_budget_per_action=500,    # PHASE 4.7.1: ×10 (50→500)
                reputation_weight=0.7,
                context_weight=0.3,
                verbose=False,
                forensic_logger=forensic_logger
            )
            self.trajectory_analyzer = TrajectoryAnalyzer(
                min_attempts_before_stop=20,      # PHASE 4.7.1: ×4 (5→20)
                stagnation_threshold=-0.001,      # PHASE 4.7.1: ×10 tolérant (-0.01→-0.001)
                oscillation_variance_threshold=0.05,
                oscillation_slope_threshold=0.005,
                verbose=False,
                forensic_logger=forensic_logger
            )
            self.use_cognitive_trio = True
            if self.verbose:
                print("[TLE] ✅ TRIO COGNITIF ACTIVÉ (C17+C18+C19)")
        except Exception as e:
            # Fallback mode simplifié si erreur
            self.reputation_system = None
            self.budget_manager = None
            self.trajectory_analyzer = None
            self.use_cognitive_trio = False
            if self.verbose:
                print(f"[TLE] ⚠️ Trio cognitif désactivé: {e}")
        
        # Initialize with basic actions
        self._initialize_action_space()
    
    def _initialize_action_space(self):
        """
        Phase 0: ACTION DISCOVERY
        Initialiser espace d'actions de base
        """
        # Actions géométriques
        self._register_action(
            "rotate_90",
            lambda g: np.rot90(g, k=-1),
            preconditions=[lambda g: g.size > 0],
            postconditions=[lambda inp, out: inp.shape == out.shape[::-1]]
        )
        
        self._register_action(
            "rotate_180",
            lambda g: np.rot90(g, k=2),
            preconditions=[lambda g: g.size > 0],
            postconditions=[lambda inp, out: inp.shape == out.shape]
        )
        
        self._register_action(
            "mirror_horizontal",
            lambda g: np.fliplr(g),
            preconditions=[lambda g: g.size > 0],
            postconditions=[lambda inp, out: inp.shape == out.shape]
        )
        
        self._register_action(
            "mirror_vertical",
            lambda g: np.flipud(g),
            preconditions=[lambda g: g.size > 0],
            postconditions=[lambda inp, out: inp.shape == out.shape]
        )
        
        # Actions de couleur
        self._register_action(
            "identity",
            lambda g: g.copy(),
            preconditions=[lambda g: True],
            postconditions=[lambda inp, out: np.array_equal(inp, out)]
        )
        
        if self.verbose:
            print(f"[TLE] Action space initialized: {len(self.actions)} actions")
    
    def _register_action(
        self,
        name: str,
        func: Callable,
        preconditions: Optional[List[Callable]] = None,
        postconditions: Optional[List[Callable]] = None
    ):
        """Enregistrer une nouvelle action"""
        self.actions[name] = TransformationAction(
            name=name,
            func=func,
            preconditions=preconditions or [],
            postconditions=postconditions or []
        )
    
    def discover_actions(self, train_pairs: List[Tuple[np.ndarray, np.ndarray]]) -> List[str]:
        """
        PHASE 0: ACTION DISCOVERY (Arcade Insight)
        Découvrir quelles actions sont pertinentes pour ce puzzle
        
        Returns:
            Liste des actions découvertes comme pertinentes
        """
        discovered = []
        
        for action_name, action in self.actions.items():
            # Tester action sur exemples d'entraînement
            relevance_score = 0.0
            
            for inp, out in train_pairs:
                # Vérifier préconditions
                if not all(pre(inp) for pre in action.preconditions):
                    continue
                
                try:
                    # Appliquer transformation
                    result = action.func(inp)
                    
                    # Mesurer similarité avec output attendu
                    if result.shape == out.shape:
                        similarity = 1.0 - np.mean(np.abs(result - out)) / 10.0
                        relevance_score += similarity
                    
                except Exception:
                    continue
            
            # Action pertinente si score > seuil
            avg_relevance = relevance_score / len(train_pairs) if train_pairs else 0.0
            if avg_relevance > 0.3:
                discovered.append(action_name)
                if self.verbose:
                    print(f"[TLE] Discovered relevant action: {action_name} (score={avg_relevance:.2f})")
        
        # P0.2 CORRECTION: Logger événement transformation
        if self.forensic_logger and discovered:
            self.forensic_logger.log_event(
                event_type="transformation",
                component="transformation_learning_engine",
                operation="discover_actions",
                data={
                    "num_actions_discovered": len(discovered),
                    "actions": discovered,
                    "num_train_pairs": len(train_pairs)
                }
            )
        
        return discovered
    
    def transform_with_feedback(
        self,
        input_grid: np.ndarray,
        action_name: str,
        target_output: Optional[np.ndarray] = None
    ) -> TransformationResult:
        """
        PHASE 1: TRANSFORMATION ENGINE
        Appliquer transformation avec feedback
        
        Args:
            input_grid: Grille d'entrée
            action_name: Nom de l'action à appliquer
            target_output: Output attendu (pour calcul erreur)
        
        Returns:
            TransformationResult avec feedback complet
        """
        if action_name not in self.actions:
            raise ValueError(f"Action inconnue: {action_name}")
        
        action = self.actions[action_name]
        start_time = time.time()
        
        try:
            # Vérifier préconditions
            if not all(pre(input_grid) for pre in action.preconditions):
                return TransformationResult(
                    output=input_grid.copy(),
                    action=action,
                    error=1.0,
                    success=False,
                    execution_time=time.time() - start_time,
                    causal_consistency=False
                )
            
            # Appliquer transformation
            output = action.func(input_grid)
            
            # Calculer erreur si target fourni
            error = 1.0
            success = False
            if target_output is not None:
                if output.shape == target_output.shape:
                    error = np.mean(np.abs(output - target_output)) / 10.0
                    success = error < 0.1
                else:
                    error = 1.0
            
            # Vérifier postconditions
            causal_consistency = all(
                post(input_grid, output) for post in action.postconditions
            )
            
            execution_time = time.time() - start_time
            
            # Métriques
            self.metrics['total_transformations'] += 1
            
            # LUMVORAX FORENSIC MAXIMAL - Logger TOUT sans exception
            if self.forensic_logger:
                self.forensic_logger.log_event(
                    event_type="transformation",
                    component="transformation_learning_engine",
                    operation="apply_transformation",
                    data={
                        "action": action_name,
                        "success": success,
                        "error": float(error),
                        "execution_time": execution_time,
                        # CONTEXTE FORENSIQUE ÉTENDU LUMVORAX
                        "timestamp_ns": time.time_ns(),
                        "input_shape": list(input_grid.shape) if hasattr(input_grid, 'shape') else None,
                        "output_shape": list(output.shape) if hasattr(output, 'shape') else None,
                        "target_shape": list(target_output.shape) if target_output is not None and hasattr(target_output, 'shape') else None,
                        "transformation_id": f"{action_name}_{time.time_ns()}",
                        "causal_consistency": causal_consistency,
                        "shape_match": output.shape == target_output.shape if target_output is not None else None,
                        "causal_chain": getattr(self, 'current_causal_chain', []),
                        "memory_state": {
                            "total_transformations": self.metrics.get('total_transformations', 0),
                            "successful_transformations": self.metrics.get('successful_transformations', 0)
                        }
                    }
                )
            if success:
                self.metrics['successful_transformations'] += 1
            else:
                self.metrics['failed_transformations'] += 1
            
            return TransformationResult(
                output=output,
                action=action,
                error=float(error),
                success=bool(success),
                execution_time=execution_time,
                causal_consistency=causal_consistency
            )
            
        except Exception as e:
            if self.verbose:
                print(f"[TLE] Transformation error: {e}")
            
            return TransformationResult(
                output=input_grid.copy(),
                action=action,
                error=1.0,
                success=False,
                execution_time=time.time() - start_time,
                causal_consistency=False
            )
    
    def validate_and_update(
        self,
        input_grid: np.ndarray,
        predicted_output: np.ndarray,
        target_output: np.ndarray,
        action_name: str
    ):
        """
        PHASE 2: PHYSICAL VALIDATION LOOP
        Valider résultat et mettre à jour modèle
        
        V29: Mise à jour C17 (réputation globale)
        
        Args:
            input_grid: Grille d'entrée
            predicted_output: Output prédit
            target_output: Output attendu
            action_name: Action utilisée
        """
        action = self.actions[action_name]
        
        # Calculer erreur
        if predicted_output.shape == target_output.shape:
            error = np.mean(np.abs(predicted_output - target_output)) / 10.0
        else:
            error = 1.0
        
        success = error < 0.1
        
        # C17: Mettre à jour réputation globale (DÉSACTIVÉ - incompatibilité API)
        # TODO Phase 4.7: Réactiver après harmonisation versions
        if self.reputation_system is not None:
            grid_size = input_grid.shape[0] * input_grid.shape[1]
            color_count = len(np.unique(input_grid))
            context = {
                'grid_size': grid_size,
                'color_count': color_count
            }
            # self.reputation_system.update(action_name, bool(success), float(error), context)
        
        # C6 FIX: Mettre à jour historique succès/échec
        self.action_success_history[action_name].append(bool(success))
        
        # C7 FIX + C11: Tracker identity failures avec réinitialisation progressive
        if action_name == 'identity':
            if error >= 0.99:
                self.identity_consecutive_failures += 1
                if self.identity_consecutive_failures >= self.identity_blacklist_threshold:
                    self.identity_blacklisted = True
            else:
                # C11: Réinitialisation progressive (-1) au lieu d'immédiate (0)
                self.identity_consecutive_failures = max(0, self.identity_consecutive_failures - 1)
                # Ne pas désactiver blacklist immédiatement
                if self.identity_consecutive_failures == 0:
                    self.identity_blacklisted = False
        
        # Créer feedback
        feedback = LearningFeedback(
            action=action,
            input_grid=input_grid,
            predicted_output=predicted_output,
            target_output=target_output,
            error=float(error),
            success=bool(success),
            timestamp=time.time()
        )
        
        self.feedback_history.append(feedback)
        
        # PHASE 3: UPDATE TRANSFORMATION SPACE
        # Mettre à jour statistiques action
        if success:
            action.success_count += 1
            
            # Mémoriser transformation réussie
            pattern_key = self._compute_pattern_key(input_grid, target_output)
            self.transformation_memory[pattern_key].append(
                (input_grid.copy(), target_output.copy(), action_name)
            )
            
            # C6 FIX: Mettre à jour dépendances causales
            if self.current_causal_chain:
                prev_action = self.current_causal_chain[-1]
                if prev_action != action_name:
                    self.causal_dependencies[prev_action].append(action_name)
        else:
            action.failure_count += 1
        
        # Mettre à jour erreur moyenne (moving average)
        alpha = 0.1
        action.avg_error = float(alpha * error + (1 - alpha) * action.avg_error)
        
        # Mettre à jour métriques globales
        self.metrics['learning_iterations'] += 1
        self.metrics['avg_error'] = np.mean([f.error for f in self.feedback_history[-100:]])
        
        if self.verbose:
            print(f"[TLE] Updated {action_name}: success_rate={action.success_rate:.2f}, "
                  f"avg_error={action.avg_error:.3f}, confidence={action.confidence:.2f}")
    
    def learn_from_examples(
        self,
        train_pairs: List[Tuple[np.ndarray, np.ndarray]],
        max_iterations: int = 10
    ) -> Dict[str, Any]:
        """
        BOUCLE D'APPRENTISSAGE COMPLÈTE
        Apprendre transformations depuis exemples d'entraînement
        
        Args:
            train_pairs: Paires (input, output) d'entraînement
            max_iterations: Nombre max d'itérations
        
        Returns:
            Statistiques d'apprentissage
        """
        if self.verbose:
            print(f"\n[TLE] === LEARNING FROM {len(train_pairs)} EXAMPLES ===")
        
        # Phase 0: Découvrir actions pertinentes
        relevant_actions = self.discover_actions(train_pairs)
        
        if not relevant_actions:
            if self.verbose:
                print("[TLE] No relevant actions discovered")
            return {'learned': False, 'reason': 'no_relevant_actions'}
        
        # Phase 1-3: Boucle d'apprentissage
        best_action = None
        best_error = float('inf')
        iteration = 0
        
        for iteration in range(max_iterations):
            for action_name in relevant_actions:
                total_error = 0.0
                
                for inp, out in train_pairs:
                    # Transform
                    result = self.transform_with_feedback(inp, action_name, out)
                    
                    # Validate and Update
                    self.validate_and_update(inp, result.output, out, action_name)
                    
                    total_error += result.error
                
                avg_error = total_error / len(train_pairs)
                
                if avg_error < best_error:
                    best_error = avg_error
                    best_action = action_name
            
            if self.verbose:
                print(f"[TLE] Iteration {iteration+1}/{max_iterations}: "
                      f"best_action={best_action}, best_error={best_error:.3f}")
            
            # Early stopping si erreur très faible
            if best_error < 0.05:
                break
        
        return {
            'learned': True,
            'best_action': best_action,
            'best_error': best_error,
            'iterations': iteration + 1,
            'relevant_actions': relevant_actions
        }
    
    def _get_context_key(self, grid: np.ndarray) -> str:
        """C6 FIX: Calculer clé de contexte pour cache"""
        shape_key = f"{grid.shape[0]}x{grid.shape[1]}"
        colors = len(np.unique(grid))
        density = np.count_nonzero(grid) / grid.size
        return f"{shape_key}_c{colors}_d{int(density*10)}"
    
    def _filter_actions_by_context(
        self,
        input_grid: np.ndarray,
        candidate_actions: List[str]
    ) -> List[str]:
        """
        C6 FIX: INTELLIGENT PRUNING
        C7 FIX: Blacklist identity si trop d'échecs
        Filtrer actions basé sur contexte et historique
        """
        context_key = self._get_context_key(input_grid)
        
        # C7 FIX: Blacklist identity si nécessaire
        if self.identity_blacklisted and 'identity' in candidate_actions:
            candidate_actions = [a for a in candidate_actions if a != 'identity']
            if self.verbose:
                print(f"[C7] Identity blacklisted ({self.identity_consecutive_failures} failures)")
        
        # Cache hit: utiliser actions connues pour ce contexte
        if context_key in self.context_cache:
            cached_actions = self.context_cache[context_key]
            # Filtrer uniquement actions disponibles
            return [a for a in cached_actions if a in candidate_actions]
        
        # Filtrer par préconditions
        valid_actions = []
        for action_name in candidate_actions:
            action = self.actions[action_name]
            
            # Vérifier préconditions
            if all(pre(input_grid) for pre in action.preconditions):
                # Vérifier historique récent (derniers 10 essais)
                recent_history = self.action_success_history[action_name][-10:]
                if not recent_history or sum(recent_history) / len(recent_history) > 0.1:
                    valid_actions.append(action_name)
        
        # Mettre en cache
        if valid_actions:
            self.context_cache[context_key] = valid_actions
        
        return valid_actions if valid_actions else candidate_actions[:3]  # Fallback: top 3
    
    def _select_best_actions(
        self,
        candidate_actions: List[str],
        top_k: int = 5
    ) -> List[str]:
        """
        C6 FIX: ADAPTIVE SELECTION
        Sélectionner top-k actions les plus prometteuses
        """
        # Trier par confiance décroissante
        sorted_actions = sorted(
            candidate_actions,
            key=lambda name: self.actions[name].confidence,
            reverse=True
        )
        
        # Retourner top-k
        return sorted_actions[:top_k]
    
    def predict(
        self,
        test_input: np.ndarray,
        use_best_action: bool = True
    ) -> Tuple[np.ndarray, str, float]:
        """
        Prédire output pour nouveau input
        
        ARCHITECTURE V29 (C17+C18+C19):
        1. C17: Sélection actions par réputation globale
        2. C18: Allocation budget proportionnelle
        3. C19: Early stopping sur trajectoire stagnante
        
        Args:
            test_input: Grille de test
            use_best_action: Utiliser meilleure action apprise
        
        Returns:
            (predicted_output, action_name, confidence)
        """
        # V33: LOGGING DEBUG - Entrée méthode
        if self.forensic_logger:
            self.forensic_logger.log_event(
                event_type="v33_predict_entry",
                component="transformation_learning_engine",
                operation="predict_method_called",
                data={
                    'use_best_action_param': use_best_action,
                    'use_best_action_type': type(use_best_action).__name__,
                    'use_best_action_str': str(use_best_action),
                    'use_best_action_repr': repr(use_best_action),
                    'input_shape': test_input.shape
                }
            )
        
        # V33: LOGGING DEBUG - Avant évaluation condition
        if self.forensic_logger:
            self.forensic_logger.log_event(
                event_type="v33_predict_condition_check",
                component="transformation_learning_engine",
                operation="predict_before_if_statement",
                data={
                    'condition_variable': 'use_best_action',
                    'condition_value': use_best_action,
                    'will_enter_if_branch': bool(use_best_action),
                    'python_truthiness': 'True' if use_best_action else 'False'
                }
            )
        
        if use_best_action:
            # PHASE 4.7.0: MODE COMPLET AVEC TRIO COGNITIF (C17+C18+C19)
            candidate_actions = list(self.actions.keys())
            
            if self.use_cognitive_trio and self.reputation_system:
                # C17: SÉLECTION PAR RÉPUTATION
                # Trier actions par réputation (success_rate * (1-avg_error))
                action_reputations = []
                for action_name in candidate_actions:
                    action = self.actions[action_name]
                    reputation = action.success_rate * (1.0 - action.avg_error)
                    action_reputations.append((action_name, reputation))
                
                # Trier par réputation décroissante
                action_reputations.sort(key=lambda x: x[1], reverse=True)
                selected_actions = [name for name, _ in action_reputations[:10]]  # Top 10
                
                if self.verbose:
                    print(f"\n[PREDICT C17] {len(selected_actions)} actions sélectionnées par réputation")
                
                # C18: ALLOCATION BUDGET
                grid_size = test_input.shape[0] * test_input.shape[1]
                color_count = len(np.unique(test_input))
                max_reputation = action_reputations[0][1] if action_reputations else 1.0
                
                # Tester chaque action avec budget et early stopping
                best_result = None
                best_confidence = -1.0
                error_history = []
                
                for action_name in selected_actions:
                    action = self.actions[action_name]
                    reputation = action.success_rate * (1.0 - action.avg_error)
                    
                    # C18: Vérifier budget disponible
                    should_explore, remaining = self.budget_manager.should_explore(
                        action_name=action_name,
                        reputation=reputation,
                        grid_size=grid_size,
                        color_count=color_count,
                        max_reputation=max_reputation
                    )
                    
                    if not should_explore:
                        if self.verbose:
                            print(f"[C18] Budget épuisé pour {action_name}")
                        continue
                    
                    # Appliquer transformation
                    result = self.transform_with_feedback(test_input, action_name)
                    error_history.append(result.error)
                    
                    # C18: Consommer budget (allocation automatique dans should_explore)
                    allocation = self.budget_manager.allocations.get(action_name)
                    if allocation:
                        allocation.consumed_budget += 1
                    
                    # C17: Enregistrer tentative dans réputation
                    self.reputation_system.record_action_attempt(
                        action_id=action_name,
                        success=result.success,
                        error=result.error,
                        cost=1.0
                    )
                    
                    # Mettre à jour meilleur résultat
                    if action.confidence > best_confidence:
                        best_confidence = action.confidence
                        best_result = (result.output, action_name, action.confidence)
                    
                    # C19: EARLY STOPPING sur trajectoire
                    if len(error_history) >= 3:
                        self.trajectory_analyzer.add_attempt(action_name, result.error)
                        metrics = self.trajectory_analyzer.analyze_trajectory(action_name)
                        
                        if metrics and metrics.should_stop:
                            if self.verbose:
                                print(f"[C19] Early stop - {metrics.stop_reason}: {action_name}")
                            break
                    
                    # Early stopping si confiance très élevée
                    if action.confidence > 0.95:
                        if self.verbose:
                            print(f"[PREDICT] Early stop - High confidence: {action.confidence:.3f}")
                        break
                
                # Mettre à jour causal chain
                if best_result:
                    self.current_causal_chain.append(best_result[1])
                
                return best_result if best_result else (test_input.copy(), "identity", 0.0)
            
            else:
                # FALLBACK: Mode simplifié si trio cognitif non disponible
                if self.verbose:
                    print(f"\n[PREDICT SIMPLIFIED] Testing {len(candidate_actions)} actions")
                
                # Sélectionner top-k actions par confiance
                sorted_actions = sorted(
                    candidate_actions,
                    key=lambda name: self.actions[name].confidence,
                    reverse=True
                )
                selected_actions = sorted_actions[:5]  # Top 5
                
                # Tester chaque action
                best_result = None
                best_confidence = -1.0
                
                for action_name in selected_actions:
                    # Appliquer transformation
                    result = self.transform_with_feedback(test_input, action_name)
                    action = self.actions[action_name]
                    
                    # Mettre à jour meilleur résultat
                    if action.confidence > best_confidence:
                        best_confidence = action.confidence
                        best_result = (result.output, action_name, action.confidence)
                    
                    # Early stopping si confiance très élevée
                    if action.confidence > 0.9:
                        if self.verbose:
                            print(f"[SIMPLIFIED] Early stop - High confidence: {action.confidence:.3f}")
                        break
                
                # Mettre à jour causal chain
                if best_result:
                    self.current_causal_chain.append(best_result[1])
                
                return best_result if best_result else (test_input.copy(), "identity", 0.0)
        
        else:
            # V33: LOGGING DEBUG - Entrée branche legacy
            if self.forensic_logger:
                self.forensic_logger.log_event(
                    event_type="v33_predict_branch_entered",
                    component="transformation_learning_engine",
                    operation="predict_legacy_branch_FALSE",
                    data={
                        'branch': 'use_best_action=False',
                        'reason': 'parameter_was_false'
                    }
                )
            
            # Essayer toutes les actions et retourner meilleure
            results = []
            
            for action_name in self.actions:
                result = self.transform_with_feedback(test_input, action_name)
                results.append((result.output, action_name, self.actions[action_name].confidence))
            
            # Retourner action avec meilleure confiance
            return max(results, key=lambda x: x[2])
    
    def _compute_pattern_key(self, input_grid: np.ndarray, output_grid: np.ndarray) -> str:
        """Calculer clé de pattern pour mémoire"""
        inp_shape = f"{input_grid.shape[0]}x{input_grid.shape[1]}"
        out_shape = f"{output_grid.shape[0]}x{output_grid.shape[1]}"
        inp_colors = len(np.unique(input_grid))
        out_colors = len(np.unique(output_grid))
        
        return f"{inp_shape}→{out_shape}_c{inp_colors}→{out_colors}"
    
    def get_statistics(self) -> Dict[str, Any]:
        """Obtenir statistiques d'apprentissage"""
        return {
            'metrics': self.metrics.copy(),
            'actions': {
                name: {
                    'success_rate': action.success_rate,
                    'avg_error': action.avg_error,
                    'confidence': action.confidence,
                    'total_uses': action.success_count + action.failure_count
                }
                for name, action in self.actions.items()
            },
            'feedback_history_size': len(self.feedback_history),
            'transformation_memory_size': sum(len(v) for v in self.transformation_memory.values())
        }


# Export
__all__ = [
    'TransformationLearningEngine',
    'TransformationAction',
    'TransformationResult',
    'LearningFeedback'
]

# Made with Bob
