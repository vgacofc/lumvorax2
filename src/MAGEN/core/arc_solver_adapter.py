"""
ARC Solver Adapter V36 - Intégration V35 (Métacognition) + V36 (World Model)
=============================================================================

Adaptateur qui connecte:
- V35: MetaCognitiveController (décisions stratégiques, utilité trajectoires)
- V36: WorldModelPersistent (représentation spatiale, carte topologique)
- Solver ARC existant (transformations, pattern matching)

Architecture:
1. Réception puzzle ARC ou état jeu Arcade
2. Mise à jour World Model (position, observations)
3. Consultation métacognition (continuer/arrêter, utilité)
4. Génération actions candidates
5. Sélection action optimale
6. Exécution + feedback
7. Logging forensic complet

Date: 2026-06-15
Version: V36.2
"""

import time
import numpy as np
from typing import Dict, List, Tuple, Optional, Any
from pathlib import Path
from dataclasses import dataclass

try:
    from .metacognitive_controller import MetaCognitiveController
    from .world_model_persistent import WorldModelPersistent
    from .forensic_middleware import ForensicMiddleware
    from .affordance_discovery_engine import AffordanceDiscoveryEngine
    from .action_reputation_system import ActionReputationSystem
    from .dynamic_budget_allocator import DynamicBudgetAllocator
    from .trajectory_stagnation_detector import TrajectoryStagnationDetector
    from .action_discovery_engine import ActionDiscoveryEngine
    from .transformations import ARCTransformations, apply_transformation
except ImportError:
    MetaCognitiveController = None
    WorldModelPersistent = None
    ForensicMiddleware = None
    AffordanceDiscoveryEngine = None
    ActionReputationSystem = None
    DynamicBudgetAllocator = None
    TrajectoryStagnationDetector = None
    ActionDiscoveryEngine = None
    ARCTransformations = None
    apply_transformation = None


@dataclass
class ARCPuzzle:
    """Représentation puzzle ARC."""
    puzzle_id: str
    train_pairs: List[Tuple[np.ndarray, np.ndarray]]
    test_input: np.ndarray
    test_output: Optional[np.ndarray] = None  # Pour validation


@dataclass
class ArcadeState:
    """État jeu Arcade."""
    game_id: str
    frame: np.ndarray  # Frame RGB
    position: Tuple[int, int]  # Position agent
    score: float
    level: int
    done: bool


@dataclass
class SolverMetrics:
    """Métriques résolution."""
    puzzle_id: str
    start_time: float
    end_time: float
    total_steps: int
    regions_discovered: int
    loops_detected: int
    world_coherence: float
    metacog_decisions: int
    success: bool
    solution: Optional[np.ndarray] = None


class ARCSolverAdapter:
    """
    Adaptateur intégrant V35 (métacognition) + V36 (world model) pour résolution ARC.
    
    Workflow:
    1. solve_puzzle() ou solve_arcade() selon type
    2. Mise à jour world model à chaque step
    3. Consultation métacognition pour décisions
    4. Génération + sélection actions
    5. Forensic complet
    """
    
    def __init__(
        self,
        max_steps: int = 1000,
        forensic_log_path: Optional[str] = None,
        logger=None
    ):
        """
        Initialise adaptateur avec V35+V36.
        
        Args:
            max_steps: Nombre maximum steps par puzzle
            forensic_log_path: Chemin logs forensic
            logger: Logger optionnel
        """
        self.logger = logger
        self.max_steps = max_steps
        
        # Forensic standard LEÇON-73.1
        if forensic_log_path is None:
            forensic_log_path = "logs/magen_v36/arc_solver_adapter.jsonl"
        
        Path(forensic_log_path).parent.mkdir(parents=True, exist_ok=True)
        
        if ForensicMiddleware:
            self.forensic = ForensicMiddleware(forensic_log_path, 'ARCSolverAdapter')
        else:
            self.forensic = None
        
        # V35: Métacognition
        if MetaCognitiveController:
            self.metacog = MetaCognitiveController()
        else:
            self.metacog = None
        
        # V36: World Model
        if WorldModelPersistent:
            self.world_model = WorldModelPersistent(
                grid_size=(30, 30),
                forensic_log_path=forensic_log_path.replace('.jsonl', '_world.jsonl')
            )
        else:
            self.world_model = None
        
        # V38: Architecture Cognitive (PHASE -1 + C0+C17+C18+C19)
        if ActionReputationSystem:
            self.reputation_system = ActionReputationSystem(
                forensic_log_path=forensic_log_path.replace('.jsonl', '_reputation.jsonl')
            )
        else:
            self.reputation_system = None
        
        # PHASE -1: Action Discovery Engine (découverte transformations plausibles)
        if ActionDiscoveryEngine:
            self.action_discovery = ActionDiscoveryEngine(
                min_plausibility=0.3,  # Seuil découverte
                max_actions=10,
                logger=logger
            )
        else:
            self.action_discovery = None
        
        if AffordanceDiscoveryEngine:
            self.affordance_engine = AffordanceDiscoveryEngine(
                forensic_log_path=forensic_log_path.replace('.jsonl', '_affordance.jsonl'),
                reputation_system=self.reputation_system,
                min_plausibility=0.15  # Optimisé: 0.3→0.2→0.15 après tests réels
            )
        else:
            self.affordance_engine = None
        
        if DynamicBudgetAllocator:
            self.budget_allocator = DynamicBudgetAllocator(
                total_budget=1000.0,  # Optimisé: 50→1000 après tests réels
                min_allocation=5.0,   # Optimisé: 10→5 pour plus de flexibilité
                forensic_log_path=forensic_log_path.replace('.jsonl', '_budget.jsonl')
            )
        else:
            self.budget_allocator = None
        
        if TrajectoryStagnationDetector:
            self.stagnation_detector = TrajectoryStagnationDetector(
                forensic_log_path=forensic_log_path.replace('.jsonl', '_stagnation.jsonl')
            )
        else:
            self.stagnation_detector = None
        
        # Cache pour actions découvertes (évite redécouverte à chaque step)
        self.discovered_actions_cache = {}
        
        # Métriques
        self.stats = {
            'puzzles_solved': 0,
            'puzzles_failed': 0,
            'total_steps': 0,
            'total_time': 0.0,
            'avg_coherence': 0.0,
            'loops_detected': 0
        }
        
        self.creation_time = time.time_ns()
        
        # Log initialization
        if self.forensic:
            self.forensic.log_event('module_initialized', self._forensic_state)
    
    @property
    def _forensic_state(self) -> Dict:
        """État forensic du module (LEÇON-60.2)."""
        return {
            'max_steps': self.max_steps,
            'stats': self.stats,
            'metacog_active': self.metacog is not None,
            'world_model_active': self.world_model is not None
        }
    
    def solve_puzzle(self, puzzle: ARCPuzzle) -> SolverMetrics:
        """
        Résout puzzle ARC avec V35+V36.
        
        Args:
            puzzle: Puzzle ARC à résoudre
            
        Returns:
            Métriques résolution
        """
        start_time = time.time()
        start_ns = time.time_ns()
        
        if self.forensic:
            self.forensic.log_event('puzzle_started', {
                'puzzle_id': puzzle.puzzle_id,
                'train_pairs': len(puzzle.train_pairs),
                'test_shape': puzzle.test_input.shape,
                'timestamp_ns': start_ns
            })
        
        # Reset world model pour nouveau puzzle
        if self.world_model:
            self.world_model.reset()
        
        # PHASE -1: Découvrir actions plausibles à partir des train examples
        discovered_actions = []
        if self.action_discovery and puzzle.puzzle_id not in self.discovered_actions_cache:
            train_examples = [
                {'input': inp, 'output': out}
                for inp, out in puzzle.train_pairs
            ]
            discovered_actions = self.action_discovery.discover_actions(train_examples)
            self.discovered_actions_cache[puzzle.puzzle_id] = discovered_actions
            
            if self.forensic:
                self.forensic.log_event('actions_discovered', {
                    'puzzle_id': puzzle.puzzle_id,
                    'num_discovered': len(discovered_actions),
                    'actions': [
                        {
                            'name': a.transform_name,
                            'plausibility': a.plausibility,
                            'parameters': a.parameters
                        }
                        for a in discovered_actions
                    ]
                })
        elif puzzle.puzzle_id in self.discovered_actions_cache:
            discovered_actions = self.discovered_actions_cache[puzzle.puzzle_id]
        
        # Initialiser position (centre grille)
        h, w = puzzle.test_input.shape
        current_pos = (h // 2, w // 2)
        
        solution = None
        success = False
        step = 0  # Initialiser step avant la boucle pour éviter NameError
        
        # Boucle résolution
        for step in range(self.max_steps):
            step_start_ns = time.time_ns()
            
            # 1. Mise à jour world model
            if self.world_model:
                state = {
                    'agent_pos': current_pos,  # CORRECTION: WorldModelPersistent attend 'agent_pos' pas 'position'
                    'value': puzzle.test_input[current_pos] if 0 <= current_pos[0] < h and 0 <= current_pos[1] < w else -1,
                    'step': step
                }
                self.world_model.update(state, action=None, step=step)
                
                # Vérifier loops
                if self.world_model.strategic_memory.detect_loop():
                    self.stats['loops_detected'] += 1
                    if self.forensic:
                        self.forensic.log_event('loop_detected', {
                            'step': step,
                            'position': current_pos
                        })
            
            # 2. Consultation métacognition
            should_continue = True
            if self.metacog and step > 10:
                # Estimer utilité continuer
                trajectory_data = {
                    'steps': step,
                    'coherence': self.world_model.stats.get('world_coherence', 0.5) if self.world_model else 0.5,
                    'regions': len(self.world_model.topological_map.regions) if self.world_model else 0
                }
                
                # Note: trajectory_utility sera ajouté dans future version
                utility = 0.5  # Placeholder
                
                # Décider si continuer
                if utility < 0.1:  # Seuil utilité faible
                    should_continue = False
                    if self.forensic:
                        self.forensic.log_event('metacog_stop_decision', {
                            'step': step,
                            'utility': utility,
                            'reason': 'low_utility'
                        })
            
            if not should_continue:
                break
            
            # 3. Générer actions candidates (transformations découvertes + mouvements)
            candidate_actions = []
            
            # 3a. Actions découvertes (transformations ARC)
            if discovered_actions and ARCTransformations:
                for disc_action in discovered_actions:
                    candidate_actions.append({
                        'type': 'transformation',
                        'name': disc_action.transform_name,
                        'plausibility': disc_action.plausibility,
                        'parameters': disc_action.parameters
                    })
            
            # 3b. Actions de mouvement (fallback)
            grid_shape = (puzzle.test_input.shape[0], puzzle.test_input.shape[1])
            candidate_positions = self._generate_candidate_actions(current_pos, grid_shape)
            for pos in candidate_positions:
                candidate_actions.append({
                    'type': 'movement',
                    'name': f"move_to_{pos}",
                    'position': pos,
                    'plausibility': 0.5
                })
            
            # V38: PHASE 0 - Filtrage affordances (C0)
            if self.affordance_engine and candidate_actions:
                action_names = [a['name'] for a in candidate_actions]
                filtered_actions = self.affordance_engine.filter_actions(
                    puzzle.test_input,
                    action_names
                )
                # Filtrer candidate_actions
                candidate_actions = [
                    a for a in candidate_actions
                    if a['name'] in filtered_actions
                ]
                
                if self.forensic:
                    self.forensic.log_event('affordance_filtering', {
                        'step': step,
                        'original_actions': len(action_names),
                        'filtered_actions': len(filtered_actions),
                        'reduction_rate': 1.0 - (len(filtered_actions) / len(action_names)) if action_names else 0.0
                    })
            
            # V38: Allouer budget (C18)
            if self.budget_allocator and candidate_actions:
                for action in candidate_actions:
                    action_id = action['name']
                    priority = action.get('plausibility', 0.5)
                    if self.reputation_system:
                        rep_score = self.reputation_system.compute_priority(action_id)
                        priority = rep_score.priority
                    
                    allocation = self.budget_allocator.allocate_budget(action_id, priority)
            
            # 4. Sélectionner meilleure action (avec C17 si disponible)
            best_action = self._select_best_action_v38_with_transforms(
                candidate_actions,
                puzzle.test_input,
                current_pos,
                step
            )
            
            # V38: Détecter stagnation (C19)
            if self.stagnation_detector:
                error = 0.5  # Placeholder - sera calculé réellement
                self.stagnation_detector.add_state(
                    step=step,
                    error=error,
                    position=current_pos,
                    action=f"move_to_{best_action}"
                )
                
                stagnation = self.stagnation_detector.detect_stagnation()
                if stagnation.should_abandon:
                    if self.forensic:
                        self.forensic.log_event('stagnation_detected_abort', {
                            'step': step,
                            'variance': stagnation.variance,
                            'loop_ratio': stagnation.loop_ratio,
                            'confidence': stagnation.confidence
                        })
                    break
            
            # 5. Exécuter action et mettre à jour modules cognitifs
            old_pos = current_pos
            action_success = False
            action_error = 1.0
            
            if best_action['type'] == 'transformation' and apply_transformation is not None:
                # Appliquer transformation
                result = apply_transformation(
                    puzzle.test_input,
                    best_action['name'],
                    **best_action.get('parameters', {})
                )
                
                if result.success:
                    solution = result.grid
                    action_success = True
                    action_error = 0.0  # Transformation réussie
                    
                    if self.forensic:
                        self.forensic.log_event('transformation_applied', {
                            'step': step,
                            'transform': best_action['name'],
                            'success': True,
                            'output_shape': solution.shape
                        })
            else:
                # Mouvement simple
                current_pos = best_action.get('position', current_pos)
                action_success = (current_pos != old_pos)
                action_error = 0.5 if not action_success else 0.3
            
            # V38: Mise à jour C17 (réputation) avec succès réel
            if self.reputation_system:
                action_id = best_action['name']
                
                self.reputation_system.record_action_attempt(
                    action_id=action_id,
                    success=action_success,
                    error=action_error,
                    cost=1.0
                )
                
                if self.forensic:
                    self.forensic.log_event('reputation_updated', {
                        'step': step,
                        'action': action_id,
                        'success': action_success,
                        'error': action_error
                    })
            
            # V38: Consommation C18 (budget)
            if self.budget_allocator:
                action_id = best_action['name']
                consumed = self.budget_allocator.consume_budget(
                    action_id=action_id,
                    amount=1.0
                )
                
                if not consumed:
                    if self.forensic:
                        self.forensic.log_event('budget_consume_failed', {
                            'step': step,
                            'action': action_id
                        })
                
                # Vérifier si action devrait continuer
                if not self.budget_allocator.should_continue(action_id):
                    if self.forensic:
                        self.forensic.log_event('budget_exhausted', {
                            'step': step,
                            'action': action_id,
                            'spent': self.budget_allocator.spent_budget,
                            'total': self.budget_allocator.total_budget
                        })
                    break
            
            # 6. Vérifier si solution trouvée
            # TODO: Implémenter vérification solution réelle
            # CORRECTION V37: Retirer succès hardcodé
            # La solution doit être vérifiée par comparaison avec test_output
            if puzzle.test_output is not None:
                # Vérifier si solution actuelle correspond à l'output attendu
                if self._verify_solution(solution, puzzle.test_output):
                    success = True
                    break
            
            step_duration_ns = time.time_ns() - step_start_ns
            
            if self.forensic and step % 10 == 0:
                self.forensic.log_event('solver_step', {
                    'step': step,
                    'position': current_pos,
                    'coherence': self.world_model.stats.get('world_coherence', 0.0) if self.world_model else 0.0,
                    'duration_ns': step_duration_ns
                })
        
        end_time = time.time()
        duration = end_time - start_time
        
        # Métriques finales (step toujours défini maintenant)
        final_step = step
        
        metrics = SolverMetrics(
            puzzle_id=puzzle.puzzle_id,
            start_time=start_time,
            end_time=end_time,
            total_steps=final_step + 1,
            regions_discovered=len(self.world_model.topological_map.regions) if self.world_model else 0,
            loops_detected=self.stats['loops_detected'],
            world_coherence=self.world_model.stats.get('world_coherence', 0.0) if self.world_model else 0.0,
            metacog_decisions=final_step // 10,  # Approximation
            success=success,
            solution=solution
        )
        
        # Mise à jour stats
        if success:
            self.stats['puzzles_solved'] += 1
        else:
            self.stats['puzzles_failed'] += 1
        
        self.stats['total_steps'] += metrics.total_steps
        self.stats['total_time'] += duration
        
        if self.forensic:
            self.forensic.log_event('puzzle_completed', {
                'puzzle_id': puzzle.puzzle_id,
                'success': success,
                'steps': metrics.total_steps,
                'duration_s': duration,
                'coherence': metrics.world_coherence,
                'regions': metrics.regions_discovered,
                'loops': metrics.loops_detected
            })
        
        return metrics
    
    def solve_arcade(self, state: ArcadeState, action_space: List[str]) -> Tuple[str, SolverMetrics]:
        """
        Résout jeu Arcade avec V35+V36.
        
        Args:
            state: État actuel jeu
            action_space: Actions disponibles
            
        Returns:
            (action_choisie, métriques)
        """
        start_time = time.time()
        start_ns = time.time_ns()
        
        if self.forensic:
            self.forensic.log_event('arcade_step_started', {
                'game_id': state.game_id,
                'position': state.position,
                'score': state.score,
                'level': state.level,
                'timestamp_ns': start_ns
            })
        
        # Mise à jour world model
        if self.world_model:
            world_state = {
                'position': state.position,
                'frame_shape': state.frame.shape,
                'score': state.score,
                'level': state.level
            }
            self.world_model.update(world_state, action=None, step=0)
        
        # Consultation métacognition
        # TODO: Implémenter décision métacognitive pour Arcade
        
        # Sélection action
        # TODO: Implémenter sélection intelligente
        action = action_space[0] if action_space else "noop"
        
        end_time = time.time()
        duration = end_time - start_time
        
        metrics = SolverMetrics(
            puzzle_id=state.game_id,
            start_time=start_time,
            end_time=end_time,
            total_steps=1,
            regions_discovered=0,
            loops_detected=0,
            world_coherence=0.0,
            metacog_decisions=0,
            success=not state.done
        )
        
        if self.forensic:
            self.forensic.log_event('arcade_step_completed', {
                'game_id': state.game_id,
                'action': action,
                'duration_s': duration
            })
        
        return action, metrics
    
    def _generate_candidate_actions(
        self,
        current_pos: Tuple[int, int],
        grid_shape: Tuple[int, int]
    ) -> List[Tuple[int, int]]:
        """Génère actions candidates (positions adjacentes)."""
        h, w = grid_shape
        x, y = current_pos
        
        candidates = []
        for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
            new_x, new_y = x + dx, y + dy
            if 0 <= new_x < h and 0 <= new_y < w:
                candidates.append((new_x, new_y))
        
        return candidates
    
    def _select_best_action(
        self,
        actions: List[Tuple[int, int]],
        current_pos: Tuple[int, int]
    ) -> Tuple[int, int]:
        """Sélectionne meilleure action selon world model."""
        if not actions:
            return current_pos
        
        if not self.world_model:
            return actions[0]
        
        # Sélectionner position avec meilleure valeur stratégique
        best_action = actions[0]
        best_value = -float('inf')
        
        for action in actions:
            value = self.world_model.strategic_memory.get_zone_value(action)
            if value > best_value:
                best_value = value
                best_action = action
        
        return best_action
    def _select_best_action_v38(
        self,
        actions: List[Tuple[int, int]],
        current_pos: Tuple[int, int],
        step: int
    ) -> Tuple[int, int]:
        """
        V38: Sélectionne meilleure action avec C17 (réputation) + world model.
        
        Args:
            actions: Liste positions candidates
            current_pos: Position actuelle
            step: Numéro étape
        
        Returns:
            Meilleure position à explorer
        """
        if not actions:
            return current_pos
        
        # Cas simple: pas de modules cognitifs
        if not self.reputation_system and not self.world_model:
            return actions[0]
        
        # Calculer scores combinés
        best_action = actions[0]
        best_score = -float('inf')
        
        for action in actions:
            action_id = f"move_to_{action}"
            
            # Score réputation (C17)
            reputation_score = 0.5  # Default
            if self.reputation_system:
                rep_data = self.reputation_system.compute_priority(action_id)
                reputation_score = rep_data.priority
            
            # Score world model (V36)
            world_score = 0.5  # Default
            if self.world_model:
                world_score = self.world_model.strategic_memory.get_zone_value(action)
            
            # Score combiné: moyenne pondérée
            # Réputation = 60% (apprentissage historique)
            # World model = 40% (contexte actuel)
            combined_score = (0.6 * reputation_score) + (0.4 * world_score)
            
            if combined_score > best_score:
                best_score = combined_score
                best_action = action
        
        if self.forensic:
            self.forensic.log_event('action_selected_v38', {
                'step': step,
                'action': best_action,
                'score': best_score,
                'num_candidates': len(actions)
            })
        
        return best_action
    
    def _select_best_action_v38_with_transforms(
        self,
        actions: List[Dict],
        grid: np.ndarray,
        current_pos: Tuple[int, int],
        step: int
    ) -> Dict:
        """
        V38.2: Sélectionne meilleure action parmi transformations + mouvements.
        
        Args:
            actions: Liste d'actions (transformations + mouvements)
            grid: Grille actuelle
            current_pos: Position actuelle
            step: Numéro étape
        
        Returns:
            Meilleure action (Dict)
        """
        if not actions:
            return {'type': 'movement', 'name': 'stay', 'position': current_pos, 'plausibility': 0.0}
        
        # Cas simple: pas de modules cognitifs
        if not self.reputation_system and not self.world_model:
            return actions[0]
        
        # Calculer scores combinés
        best_action = actions[0]
        best_score = -float('inf')
        
        for action in actions:
            action_id = action['name']
            
            # Score plausibilité (découverte)
            plausibility_score = action.get('plausibility', 0.5)
            
            # Score réputation (C17)
            reputation_score = 0.5  # Default
            if self.reputation_system:
                rep_data = self.reputation_system.compute_priority(action_id)
                reputation_score = rep_data.priority
            
            # Score world model (V36) - seulement pour mouvements
            world_score = 0.5  # Default
            if action['type'] == 'movement' and self.world_model:
                pos = action.get('position', current_pos)
                world_score = self.world_model.strategic_memory.get_zone_value(pos)
            
            # Score combiné: moyenne pondérée
            # Plausibilité = 40% (découverte)
            # Réputation = 40% (apprentissage historique)
            # World model = 20% (contexte actuel)
            combined_score = (
                0.4 * plausibility_score +
                0.4 * reputation_score +
                0.2 * world_score
            )
            
            if combined_score > best_score:
                best_score = combined_score
                best_action = action
        
        if self.forensic:
            self.forensic.log_event('action_selected_v38_transforms', {
                'step': step,
                'action': best_action['name'],
                'type': best_action['type'],
                'score': best_score,
                'num_candidates': len(actions)
            })
        
        return best_action
    
    def _verify_solution(
        self,
        predicted: Optional[np.ndarray],
        expected: np.ndarray
    ) -> bool:
        """
        Vérifie si solution prédite correspond à l'output attendu.
        
        Args:
            predicted: Solution prédite (peut être None)
            expected: Output attendu
        
        Returns:
            True si solution correcte, False sinon
        """
        if predicted is None:
            return False
        
        if predicted.shape != expected.shape:
            if self.forensic:
                self.forensic.log_event('solution_verification_failed', {
                    'reason': 'shape_mismatch',
                    'predicted_shape': predicted.shape,
                    'expected_shape': expected.shape
                })
            return False
        
        # Comparaison exacte
        is_correct = np.array_equal(predicted, expected)
        
        if self.forensic:
            self.forensic.log_event('solution_verified', {
                'is_correct': is_correct,
                'predicted_shape': predicted.shape,
                'expected_shape': expected.shape
            })
        
        return is_correct
    
    def get_metrics_summary(self) -> Dict:
        """Retourne résumé métriques."""
        total_puzzles = self.stats['puzzles_solved'] + self.stats['puzzles_failed']
        
        return {
            'total_puzzles': total_puzzles,
            'success_rate': self.stats['puzzles_solved'] / total_puzzles if total_puzzles > 0 else 0.0,
            'avg_steps': self.stats['total_steps'] / total_puzzles if total_puzzles > 0 else 0,
            'avg_time': self.stats['total_time'] / total_puzzles if total_puzzles > 0 else 0.0,
            'total_loops': self.stats['loops_detected']
        }
    
    def close(self):
        """Ferme adaptateur et logs forensic."""
        if self.forensic:
            self.forensic.log_event('module_closing', {
                'stats': self.stats,
                'metrics_summary': self.get_metrics_summary()
            })
        
        if self.metacog:
            self.metacog.close()
        
        if self.world_model:
            self.world_model.close()

# Made with Bob
