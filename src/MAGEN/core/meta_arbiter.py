"""
MAGEN - Meta Arbiter (Cognitive Routing Layer)
===============================================

ROOT CAUSE SOLUTION: Système de routage intelligent qui choisit
la stratégie de résolution optimale pour chaque puzzle.

Architecture:
- Analyse complexité puzzle
- Scoring multi-dimensionnel
- Routage dynamique vers moteur approprié
- Feedback loop pour apprentissage

Auteur: Bob (Mode Advanced)
Date: 2026-06-13
Protocole: Claude Pilot + LumVorax
"""

import numpy as np
from typing import List, Tuple, Dict, Any, Optional
from dataclasses import dataclass
from enum import Enum


class ReasoningStrategy(Enum):
    """Stratégies de raisonnement disponibles"""
    PATTERN_MATCHER = "pattern_matcher"  # Exact match cases
    OBJECT_TRANSFORMATION = "object_transformation"  # Object-based cases
    ADVANCED_DETECTORS = "advanced_detectors"  # Rule-based cases
    TRANSFORMATION_LEARNING = "transformation_learning"  # TLE learning-based (V21)
    ARCADE_DISCOVERY = "arcade_discovery"  # Arcade-style interactive discovery (V22)
    PWFE_EXPLORATION = "pwfe_exploration"  # High ambiguity cases
    HYBRID = "hybrid"  # Multiple strategies combined


@dataclass
class ComplexitySignals:
    """Signaux de complexité d'un puzzle"""
    # Pattern matching signals
    exact_match_probability: float  # 0.0-1.0
    pattern_consistency: float  # 0.0-1.0
    
    # Object structure signals
    object_count_change: bool
    object_structure_change: bool
    spatial_relationship_change: bool
    
    # Rule-based signals
    rule_based_behavior: bool
    color_transformation: bool
    geometric_transformation: bool
    
    # Ambiguity signals
    high_ambiguity: bool
    multi_step_reasoning: bool
    context_dependent: bool
    
    # Metadata
    train_size: int
    grid_size_avg: Tuple[int, int]
    color_count: int


@dataclass
class RoutingDecision:
    """Décision de routage avec justification"""
    strategy: ReasoningStrategy
    confidence: float  # 0.0-1.0
    reasoning: str
    fallback_strategies: List[ReasoningStrategy]
    signals: ComplexitySignals


class MetaArbiter:
    """
    Meta-Arbiter: Cerveau de routage cognitif
    
    Responsabilités:
    1. Analyser complexité puzzle
    2. Scorer chaque dimension
    3. Router vers stratégie optimale
    4. Apprendre des succès/échecs
    """
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.routing_history: List[Dict[str, Any]] = []
        
        # Seuils de décision (appris empiriquement)
        self.thresholds = {
            'exact_match_high': 0.95,
            'exact_match_medium': 0.75,
            'object_change_threshold': 0.6,
            'rule_based_threshold': 0.7,
            'ambiguity_threshold': 0.5
        }
        
        # Statistiques par stratégie
        self.strategy_stats = {
            strategy: {'attempts': 0, 'successes': 0}
            for strategy in ReasoningStrategy
        }
    
    def route(
        self,
        train_pairs: List[Tuple[np.ndarray, np.ndarray]],
        puzzle_id: Optional[str] = None
    ) -> RoutingDecision:
        """
        Router puzzle vers stratégie optimale
        
        Args:
            train_pairs: Paires (input, output) d'entraînement
            puzzle_id: ID puzzle (pour logging)
        
        Returns:
            RoutingDecision avec stratégie choisie
        """
        # 1. Analyser complexité
        signals = self.analyze_complexity(train_pairs)
        
        # 2. Scorer chaque stratégie
        scores = self._score_strategies(signals)
        
        # 3. Sélectionner meilleure stratégie
        best_strategy = max(scores.items(), key=lambda x: x[1])
        strategy, confidence = best_strategy
        
        # 4. Déterminer fallbacks
        fallbacks = self._get_fallback_strategies(scores, strategy)
        
        # 5. Générer justification
        reasoning = self._generate_reasoning(strategy, signals, confidence)
        
        # 6. Créer décision
        decision = RoutingDecision(
            strategy=strategy,
            confidence=confidence,
            reasoning=reasoning,
            fallback_strategies=fallbacks,
            signals=signals
        )
        
        # 7. Logger décision
        if self.verbose and puzzle_id:
            self._log_decision(decision, puzzle_id)
        
        # 8. Enregistrer historique
        self.routing_history.append({
            'puzzle_id': puzzle_id,
            'strategy': strategy.value,
            'confidence': confidence,
            'signals': signals
        })
        
        return decision
    
    def analyze_complexity(
        self,
        train_pairs: List[Tuple[np.ndarray, np.ndarray]]
    ) -> ComplexitySignals:
        """
        Analyser complexité puzzle via signaux multi-dimensionnels
        
        Returns:
            ComplexitySignals avec tous les indicateurs
        """
        if len(train_pairs) == 0:
            return self._default_signals()
        
        # Extraire inputs/outputs
        inputs = [pair[0] for pair in train_pairs]
        outputs = [pair[1] for pair in train_pairs]
        
        # 1. PATTERN MATCHING SIGNALS
        exact_match_prob = self._compute_exact_match_probability(train_pairs)
        pattern_consistency = self._compute_pattern_consistency(train_pairs)
        
        # 2. OBJECT STRUCTURE SIGNALS
        object_count_change = self._detect_object_count_change(inputs, outputs)
        object_structure_change = self._detect_object_structure_change(inputs, outputs)
        spatial_relationship_change = self._detect_spatial_relationship_change(inputs, outputs)
        
        # 3. RULE-BASED SIGNALS
        rule_based_behavior = self._detect_rule_based_behavior(train_pairs)
        color_transformation = self._detect_color_transformation(inputs, outputs)
        geometric_transformation = self._detect_geometric_transformation(inputs, outputs)
        
        # 4. AMBIGUITY SIGNALS
        high_ambiguity = self._detect_high_ambiguity(train_pairs)
        multi_step_reasoning = self._detect_multi_step_reasoning(train_pairs)
        context_dependent = self._detect_context_dependent(train_pairs)
        
        # 5. METADATA
        train_size = len(train_pairs)
        grid_size_avg = self._compute_avg_grid_size(inputs)
        color_count = self._compute_color_count(inputs + outputs)
        
        return ComplexitySignals(
            exact_match_probability=exact_match_prob,
            pattern_consistency=pattern_consistency,
            object_count_change=object_count_change,
            object_structure_change=object_structure_change,
            spatial_relationship_change=spatial_relationship_change,
            rule_based_behavior=rule_based_behavior,
            color_transformation=color_transformation,
            geometric_transformation=geometric_transformation,
            high_ambiguity=high_ambiguity,
            multi_step_reasoning=multi_step_reasoning,
            context_dependent=context_dependent,
            train_size=train_size,
            grid_size_avg=grid_size_avg,
            color_count=color_count
        )
    
    def _score_strategies(
        self,
        signals: ComplexitySignals
    ) -> Dict[ReasoningStrategy, float]:
        """
        Scorer chaque stratégie basé sur signaux
        
        CORRECTION V20: Optimisation scoring pour réduire Object Transformation
        - V19: 51% Object Transformation (9.3% succès)
        - V19: 33.8% Pattern Matcher (14.8% succès) ⭐
        - Objectif V20: Privilégier Pattern Matcher, réduire Object Transformation
        
        Returns:
            Dict[Strategy, Score] avec scores 0.0-1.0
        """
        scores = {}
        
        # PATTERN MATCHER: BOOST (meilleur taux succès V19: 14.8%)
        scores[ReasoningStrategy.PATTERN_MATCHER] = (
            signals.exact_match_probability * 0.7 +  # 0.6 → 0.7
            signals.pattern_consistency * 0.4 +      # 0.3 → 0.4
            (0.2 if not signals.high_ambiguity else 0.0)  # 0.1 → 0.2
        )
        
        # OBJECT TRANSFORMATION: RÉDUCTION (sur-utilisé en V19: 51% mais 9.3% succès)
        # Exiger TOUS les signaux pour activer (au lieu de n'importe lequel)
        object_score = 0.0
        object_signals_count = sum([
            signals.object_count_change,
            signals.object_structure_change,
            signals.spatial_relationship_change
        ])
        
        # Exiger au moins 2/3 signaux pour score significatif
        if object_signals_count >= 2:
            object_score = 0.5 + (object_signals_count - 2) * 0.2  # Max 0.7
        elif object_signals_count == 1:
            object_score = 0.2  # Score faible si 1 seul signal
        
        scores[ReasoningStrategy.OBJECT_TRANSFORMATION] = object_score
        
        # ADVANCED DETECTORS: BOOST (sous-utilisé en V19: 7.5% mais 13.3% succès)
        rule_score = 0.0
        if signals.rule_based_behavior:
            rule_score += 0.5  # 0.4 → 0.5
        if signals.color_transformation:
            rule_score += 0.3
        if signals.geometric_transformation:
            rule_score += 0.3
        scores[ReasoningStrategy.ADVANCED_DETECTORS] = rule_score
        
        # PWFE EXPLORATION: Maintenir (9.1% succès en V19, mais 0% récupérations)
        exploration_score = 0.0
        if signals.high_ambiguity:
            exploration_score += 0.4
        if signals.multi_step_reasoning:
            exploration_score += 0.3
        if signals.context_dependent:
            exploration_score += 0.3
        scores[ReasoningStrategy.PWFE_EXPLORATION] = exploration_score
        
        # TRANSFORMATION LEARNING: NOUVEAU V21 (apprentissage génératif)
        # Privilégier pour puzzles avec patterns consistants mais non-exacts
        tle_score = 0.0
        if 0.3 < signals.pattern_consistency <= 1.0:  # Patterns consistants
            tle_score += 0.7  # 0.6 → 0.7 (plus agressif)
        if signals.exact_match_probability < 0.8:  # Pas de match exact très évident
            tle_score += 0.4  # 0.3 → 0.4
        if not signals.high_ambiguity:  # Ambiguïté faible/moyenne
            tle_score += 0.3  # 0.2 → 0.3
        # Bonus si transformations géométriques détectées
        if signals.geometric_transformation:
            tle_score += 0.2
        scores[ReasoningStrategy.TRANSFORMATION_LEARNING] = tle_score
        
        # ARCADE DISCOVERY: NOUVEAU V22 (exploration interactive + causalité)
        # Privilégier pour puzzles avec échecs répétés ou comportements cachés
        arcade_score = 0.0
        # Signal 1: Échecs multiples sur patterns simples (index errors, validation failures)
        if signals.high_ambiguity:
            arcade_score += 0.5
        # Signal 2: Comportements contextuels détectés
        if signals.context_dependent:
            arcade_score += 0.4
        # Signal 3: Multi-step reasoning (séquences d'actions)
        if signals.multi_step_reasoning:
            arcade_score += 0.3
        # Signal 4: Transformations spatiales complexes
        if signals.spatial_relationship_change:
            arcade_score += 0.2
        # Bonus: Petits grids (arcade-style games sont souvent compacts)
        if signals.grid_size_avg[0] <= 10 and signals.grid_size_avg[1] <= 10:
            arcade_score += 0.2
        scores[ReasoningStrategy.ARCADE_DISCOVERY] = arcade_score
        
        # HYBRID: BOOST (meilleur taux V19: 33.3% sur 9 puzzles!)
        max_score = max(scores.values())
        if max_score < 0.5:  # 0.6 → 0.5 (seuil plus bas)
            scores[ReasoningStrategy.HYBRID] = 0.8  # 0.7 → 0.8
        else:
            scores[ReasoningStrategy.HYBRID] = max_score * 0.6  # 0.5 → 0.6
        
        return scores
    
    def _get_fallback_strategies(
        self,
        scores: Dict[ReasoningStrategy, float],
        primary: ReasoningStrategy
    ) -> List[ReasoningStrategy]:
        """Déterminer stratégies de fallback ordonnées"""
        # Trier par score décroissant, exclure primary
        sorted_strategies = sorted(
            [(s, score) for s, score in scores.items() if s != primary],
            key=lambda x: x[1],
            reverse=True
        )
        
        # Retourner top 2 fallbacks
        return [s for s, _ in sorted_strategies[:2]]
    
    def _generate_reasoning(
        self,
        strategy: ReasoningStrategy,
        signals: ComplexitySignals,
        confidence: float
    ) -> str:
        """Générer justification textuelle"""
        reasons = []
        
        if strategy == ReasoningStrategy.PATTERN_MATCHER:
            reasons.append(f"Exact match probability: {signals.exact_match_probability:.2f}")
            reasons.append(f"Pattern consistency: {signals.pattern_consistency:.2f}")
        
        elif strategy == ReasoningStrategy.OBJECT_TRANSFORMATION:
            if signals.object_count_change:
                reasons.append("Object count changes detected")
            if signals.object_structure_change:
                reasons.append("Object structure modifications detected")
            if signals.spatial_relationship_change:
                reasons.append("Spatial relationships change")
        
        elif strategy == ReasoningStrategy.ADVANCED_DETECTORS:
            if signals.rule_based_behavior:
                reasons.append("Rule-based behavior detected")
            if signals.color_transformation:
                reasons.append("Color transformations present")
            if signals.geometric_transformation:
                reasons.append("Geometric transformations present")
        
        elif strategy == ReasoningStrategy.PWFE_EXPLORATION:
            if signals.high_ambiguity:
                reasons.append("High ambiguity requires exploration")
            if signals.multi_step_reasoning:
                reasons.append("Multi-step reasoning needed")
            if signals.context_dependent:
                reasons.append("Context-dependent solution")
        
        elif strategy == ReasoningStrategy.TRANSFORMATION_LEARNING:
            reasons.append(f"Pattern consistency: {signals.pattern_consistency:.2f}")
            reasons.append("Learning-based approach suitable")
            if not signals.high_ambiguity:
                reasons.append("Low ambiguity enables learning")
        
        elif strategy == ReasoningStrategy.ARCADE_DISCOVERY:
            if signals.high_ambiguity:
                reasons.append("High ambiguity requires interactive exploration")
            if signals.context_dependent:
                reasons.append("Context-dependent behavior detected")
            if signals.multi_step_reasoning:
                reasons.append("Multi-step action sequences needed")
            if signals.spatial_relationship_change:
                reasons.append("Complex spatial transformations")
            reasons.append("Arcade-style discovery engine activated")
        
        elif strategy == ReasoningStrategy.HYBRID:
            reasons.append("No single dominant strategy")
            reasons.append("Combining multiple approaches")
        
        return f"{strategy.value} (conf={confidence:.2f}): " + "; ".join(reasons)
    
    def _log_decision(self, decision: RoutingDecision, puzzle_id: str):
        """Logger décision de routage"""
        print(f"\n🧠 META-ARBITER DECISION [{puzzle_id}]")
        print(f"   Strategy: {decision.strategy.value}")
        print(f"   Confidence: {decision.confidence:.2f}")
        print(f"   Reasoning: {decision.reasoning}")
        if decision.fallback_strategies:
            fallbacks = ", ".join([s.value for s in decision.fallback_strategies])
            print(f"   Fallbacks: {fallbacks}")
    
    def record_outcome(
        self,
        puzzle_id: str,
        strategy: ReasoningStrategy,
        success: bool
    ):
        """Enregistrer résultat pour apprentissage"""
        self.strategy_stats[strategy]['attempts'] += 1
        if success:
            self.strategy_stats[strategy]['successes'] += 1
    
    def get_statistics(self) -> Dict[str, Any]:
        """Obtenir statistiques routage"""
        stats = {}
        for strategy, data in self.strategy_stats.items():
            attempts = data['attempts']
            successes = data['successes']
            success_rate = successes / attempts if attempts > 0 else 0.0
            stats[strategy.value] = {
                'attempts': attempts,
                'successes': successes,
                'success_rate': success_rate
            }
        return stats
    
    # ========== DETECTION METHODS ==========
    
    def _compute_exact_match_probability(self, train_pairs) -> float:
        """Probabilité qu'un pattern exact existe"""
        if len(train_pairs) < 2:
            return 0.5
        
        # Vérifier si transformations identiques entre paires
        transforms_similar = 0
        for i in range(len(train_pairs) - 1):
            inp1, out1 = train_pairs[i]
            inp2, out2 = train_pairs[i + 1]
            
            # Comparer types de transformations
            if inp1.shape == inp2.shape and out1.shape == out2.shape:
                transforms_similar += 1
        
        return transforms_similar / (len(train_pairs) - 1) if len(train_pairs) > 1 else 0.5
    
    def _compute_pattern_consistency(self, train_pairs) -> float:
        """Consistance des patterns entre exemples"""
        if len(train_pairs) < 2:
            return 0.5
        
        # Vérifier consistance shapes
        shapes_consistent = all(
            pair[0].shape == train_pairs[0][0].shape and
            pair[1].shape == train_pairs[0][1].shape
            for pair in train_pairs
        )
        
        return 1.0 if shapes_consistent else 0.3
    
    def _detect_object_count_change(self, inputs, outputs) -> bool:
        """Détecte si nombre d'objets change"""
        for inp, out in zip(inputs, outputs):
            inp_objects = np.sum(inp > 0)
            out_objects = np.sum(out > 0)
            if abs(inp_objects - out_objects) > inp_objects * 0.2:
                return True
        return False
    
    def _detect_object_structure_change(self, inputs, outputs) -> bool:
        """Détecte si structure objets change"""
        # Simplification: vérifier si patterns de pixels changent
        for inp, out in zip(inputs, outputs):
            if inp.shape != out.shape:
                return True
            if not np.array_equal(inp > 0, out > 0):
                return True
        return False
    
    def _detect_spatial_relationship_change(self, inputs, outputs) -> bool:
        """Détecte si relations spatiales changent"""
        # Simplification: vérifier si centres de masse changent
        for inp, out in zip(inputs, outputs):
            if inp.shape != out.shape:
                return True
        return False
        return False
    
    def _detect_rule_based_behavior(self, train_pairs) -> bool:
        """Détecte comportement basé sur règles"""
        # Vérifier si transformations suivent pattern régulier
        return len(train_pairs) >= 2
    
    def _detect_color_transformation(self, inputs, outputs) -> bool:
        """Détecte transformations de couleurs"""
        for inp, out in zip(inputs, outputs):
            inp_colors = set(inp.flatten())
            out_colors = set(out.flatten())
            if inp_colors != out_colors:
                return True
        return False
    
    def _detect_geometric_transformation(self, inputs, outputs) -> bool:
        """Détecte transformations géométriques"""
        for inp, out in zip(inputs, outputs):
            if inp.shape != out.shape:
                return True
        return False
    
    def _detect_high_ambiguity(self, train_pairs) -> bool:
        """Détecte haute ambiguïté"""
        # Simplification: peu d'exemples = haute ambiguïté
        return len(train_pairs) < 3
    
    def _detect_multi_step_reasoning(self, train_pairs) -> bool:
        """Détecte raisonnement multi-étapes"""
        # Simplification: grilles complexes = multi-step
        for inp, _ in train_pairs:
            if inp.shape[0] * inp.shape[1] > 100:
                return True
        return False
    
    def _detect_context_dependent(self, train_pairs) -> bool:
        """Détecte dépendance au contexte"""
        # Simplification: variations entre exemples
        if len(train_pairs) < 2:
            return False
        
        shapes = [pair[0].shape for pair in train_pairs]
        return len(set(shapes)) > 1
    
    def _compute_avg_grid_size(self, grids) -> Tuple[int, int]:
        """Taille moyenne grilles"""
        if not grids:
            return (0, 0)
        avg_h = int(np.mean([g.shape[0] for g in grids]))
        avg_w = int(np.mean([g.shape[1] for g in grids]))
        return (avg_h, avg_w)
    
    def _compute_color_count(self, grids) -> int:
        """Nombre de couleurs uniques"""
        all_colors = set()
        for grid in grids:
            all_colors.update(grid.flatten())
        return len(all_colors)
    
    def _default_signals(self) -> ComplexitySignals:
        """Signaux par défaut si pas de données"""
        return ComplexitySignals(
            exact_match_probability=0.5,
            pattern_consistency=0.5,
            object_count_change=False,
            object_structure_change=False,
            spatial_relationship_change=False,
            rule_based_behavior=False,
            color_transformation=False,
            geometric_transformation=False,
            high_ambiguity=True,
            multi_step_reasoning=False,
            context_dependent=False,
            train_size=0,
            grid_size_avg=(0, 0),
            color_count=0
        )


# Made with Bob - Session 57 - Meta-Arbiter