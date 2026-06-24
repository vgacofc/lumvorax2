"""
World Model - MAGEN V23
Modèle prédictif du monde pour anticiper les conséquences des actions
"""

import numpy as np
from typing import List, Dict, Tuple, Optional
from dataclasses import dataclass, field
from collections import defaultdict
import hashlib
from sklearn.cluster import DBSCAN
from scipy.spatial.distance import cosine


@dataclass
class StateTransition:
    """Transition d'état observée (réutilisée depuis action_discovery_engine)"""
    state_before: np.ndarray
    action: str
    state_after: np.ndarray
    reward: float
    context: Dict = field(default_factory=dict)
    timestamp: float = 0.0
    
    def compute_diff(self) -> np.ndarray:
        """Calcule la différence entre états"""
        return self.state_after - self.state_before
    
    def get_state_hash(self, state: np.ndarray) -> str:
        """Hash d'un état pour comparaison"""
        return hashlib.md5(state.tobytes()).hexdigest()


@dataclass
class CausalRule:
    """Règle causale apprise: (state_pattern, action) → outcome_pattern"""
    state_pattern: np.ndarray  # Pattern d'état déclencheur
    action: str
    outcome_pattern: np.ndarray  # Pattern d'état résultant
    confidence: float
    support: int  # Nombre d'observations
    contexts: List[Dict] = field(default_factory=list)
    
    def __repr__(self):
        return f"CausalRule({self.action}, conf={self.confidence:.2f}, support={self.support})"


class WorldModel:
    """
    Modèle prédictif du monde
    
    Principe:
    1. Enregistrer toutes les transitions observées
    2. Construire un graphe causal (state, action) → state'
    3. Prédire les outcomes futurs par similarité
    4. Apprendre la "physique" du jeu
    """
    
    def __init__(self, max_transitions: int = 1000):
        self.transitions: List[StateTransition] = []
        self.max_transitions = max_transitions
        
        # Graphe causal: (state_hash, action) → [transitions]
        self.causal_graph: Dict[Tuple[str, str], List[StateTransition]] = defaultdict(list)
        
        # Règles causales apprises
        self.causal_rules: List[CausalRule] = []
        
        # Cache de prédictions
        self.prediction_cache: Dict[Tuple[str, str], np.ndarray] = {}
        
        # Statistiques
        self.stats = {
            'total_transitions': 0,
            'unique_states': 0,
            'causal_rules_learned': 0,
            'prediction_accuracy': 0.0,
            'cache_hits': 0,
            'cache_misses': 0
        }
    
    def record_transition(self, transition: StateTransition):
        """
        Enregistre une transition observée
        
        Args:
            transition: Transition à enregistrer
        """
        # Ajouter à la liste
        self.transitions.append(transition)
        self.stats['total_transitions'] += 1
        
        # Limiter taille mémoire
        if len(self.transitions) > self.max_transitions:
            self.transitions.pop(0)  # FIFO
        
        # Mettre à jour graphe causal
        state_hash = transition.get_state_hash(transition.state_before)
        key = (state_hash, transition.action)
        self.causal_graph[key].append(transition)
        
        # Invalider cache pour cette clé
        if key in self.prediction_cache:
            del self.prediction_cache[key]
        
        # Mettre à jour statistiques
        unique_states = len(set(t.get_state_hash(t.state_before) for t in self.transitions))
        self.stats['unique_states'] = unique_states
        
        # Apprendre règles causales périodiquement
        if len(self.transitions) % 50 == 0:
            self._learn_causal_rules()
    
    def predict_outcome(self, state: np.ndarray, action: str) -> np.ndarray:
        """
        Prédit l'état résultant d'une action
        
        Args:
            state: État actuel
            action: Action à exécuter
        
        Returns:
            État prédit après l'action
        """
        state_hash = hashlib.md5(state.tobytes()).hexdigest()
        cache_key = (state_hash, action)
        
        # Vérifier cache
        if cache_key in self.prediction_cache:
            self.stats['cache_hits'] += 1
            return self.prediction_cache[cache_key]
        
        self.stats['cache_misses'] += 1
        
        # Rechercher transitions similaires
        similar_transitions = self._find_similar_transitions(state, action)
        
        if similar_transitions:
            # Prédiction par moyenne pondérée
            weights = [self._compute_similarity(state, t.state_before) for t in similar_transitions]
            total_weight = sum(weights)
            
            if total_weight > 0:
                # Utiliser float64 pour calculs intermédiaires
                predicted_state = np.zeros_like(state, dtype=np.float64)
                for t, w in zip(similar_transitions, weights):
                    # FIX: Vérifier compatibilité shapes avant addition
                    if t.state_after.shape == state.shape:
                        predicted_state += (w / total_weight) * t.state_after.astype(np.float64)
                    else:
                        # Shapes incompatibles, ignorer cette transition
                        continue
                
                # Convertir en uint8 pour compatibilité avec grilles
                predicted_state = predicted_state.astype(np.uint8)
                
                # Mettre en cache
                self.prediction_cache[cache_key] = predicted_state
                return predicted_state
        
        # Pas de prédiction possible → retourner état inchangé
        return state.copy()
    
    def predict_reward(self, state: np.ndarray, action: str) -> float:
        """
        Prédit le reward d'une action
        
        Args:
            state: État actuel
            action: Action à exécuter
        
        Returns:
            Reward prédit
        """
        similar_transitions = self._find_similar_transitions(state, action)
        
        if similar_transitions:
            # Moyenne pondérée des rewards
            weights = [self._compute_similarity(state, t.state_before) for t in similar_transitions]
            total_weight = sum(weights)
            
            if total_weight > 0:
                predicted_reward = sum(t.reward * w for t, w in zip(similar_transitions, weights)) / total_weight
                return predicted_reward
        
        return 0.0  # Reward neutre par défaut
    
    def _find_similar_transitions(self, state: np.ndarray, action: str, k: int = 5) -> List[StateTransition]:
        """
        Trouve les k transitions les plus similaires
        
        Args:
            state: État de référence
            action: Action de référence
            k: Nombre de transitions à retourner
        
        Returns:
            Liste des transitions similaires
        """
        # Filtrer par action
        action_transitions = [t for t in self.transitions if t.action == action]
        
        if not action_transitions:
            return []
        
        # Calculer similarités
        similarities = []
        for t in action_transitions:
            sim = self._compute_similarity(state, t.state_before)
            similarities.append((sim, t))
        
        # Trier par similarité décroissante
        similarities.sort(key=lambda x: x[0], reverse=True)
        
        # Retourner top-k
        return [t for _, t in similarities[:k]]
    
    def _compute_similarity(self, state1: np.ndarray, state2: np.ndarray) -> float:
        """
        Calcule la similarité entre deux états
        
        Utilise distance cosinus inversée
        """
        # Aplatir les états
        flat1 = state1.flatten()
        flat2 = state2.flatten()
        
        # FIX: Vérifier que les shapes sont compatibles
        if flat1.shape != flat2.shape:
            return 0.0  # États incompatibles
        
        # Éviter division par zéro
        if np.linalg.norm(flat1) == 0 or np.linalg.norm(flat2) == 0:
            return 0.0
        
        # FIX: Normaliser vecteurs pour éviter overflow
        flat1_norm = flat1 / (np.linalg.norm(flat1) + 1e-10)
        flat2_norm = flat2 / (np.linalg.norm(flat2) + 1e-10)
        
        # Distance cosinus inversée (1 = identique, 0 = orthogonal)
        try:
            similarity = 1.0 - cosine(flat1_norm, flat2_norm)
        except Exception as e:
            print(f"[WORLD MODEL] Warning: cosine failed ({e}), returning 0.0")
            similarity = 0.0
        
        return max(0.0, similarity)  # Clamp à [0, 1]
    
    def _learn_causal_rules(self):
        """
        Apprend des règles causales par clustering des transitions
        
        Identifie des patterns récurrents: (state_pattern, action) → outcome_pattern
        """
        if len(self.transitions) < 10:
            return  # Pas assez de données
        
        print(f"[WORLD MODEL] Apprentissage règles causales ({len(self.transitions)} transitions)...")
        
        # Grouper par action
        action_groups = defaultdict(list)
        for t in self.transitions:
            action_groups[t.action].append(t)
        
        new_rules = []
        
        for action, transitions in action_groups.items():
            if len(transitions) < 3:
                continue  # Pas assez d'exemples
            
            # FIX: Grouper transitions par shape avant clustering
            shape_groups = defaultdict(list)
            for t in transitions:
                shape_key = str(t.state_before.shape)
                shape_groups[shape_key].append(t)
            
            # Clustering par groupe de shape
            for shape_key, shape_transitions in shape_groups.items():
                if len(shape_transitions) < 3:
                    continue
                
                # Extraire features (états avant) - maintenant homogènes
                features = np.array([t.state_before.flatten() for t in shape_transitions])
                
                # Clustering DBSCAN
                try:
                    clustering = DBSCAN(eps=0.3, min_samples=2).fit(features)
                    labels = clustering.labels_
                except Exception as e:
                    print(f"[WORLD MODEL] Warning: clustering failed for {action}/{shape_key}: {e}")
                    continue
                
                # Pour chaque cluster
                for cluster_id in set(labels):
                    if cluster_id == -1:  # Bruit
                        continue
                    
                    cluster_transitions = [t for t, l in zip(shape_transitions, labels) if l == cluster_id]
                    
                    if len(cluster_transitions) < 2:
                        continue
                    
                    # Calculer pattern moyen
                    state_pattern = np.mean([t.state_before for t in cluster_transitions], axis=0)
                    outcome_pattern = np.mean([t.state_after for t in cluster_transitions], axis=0)
                    
                    # Calculer confiance (cohérence du cluster)
                    variances = [np.var(t.state_after - outcome_pattern) for t in cluster_transitions]
                    confidence = float(1.0 / (1.0 + np.mean(variances)))
                    
                    # Créer règle
                    rule = CausalRule(
                        state_pattern=state_pattern,
                        action=action,
                        outcome_pattern=outcome_pattern,
                        confidence=confidence,
                        support=len(cluster_transitions),
                        contexts=[t.context for t in cluster_transitions]
                    )
                    
                    new_rules.append(rule)
        
        # Ajouter nouvelles règles
        self.causal_rules.extend(new_rules)
        self.stats['causal_rules_learned'] = len(self.causal_rules)
        
        print(f"  ✅ {len(new_rules)} nouvelles règles apprises (total: {len(self.causal_rules)})")
    
    def get_action_physics(self, action: str) -> Dict:
        """
        Retourne la 'physique' apprise d'une action
        
        Args:
            action: Nom de l'action
        
        Returns:
            Dictionnaire avec statistiques de l'action
        """
        action_transitions = [t for t in self.transitions if t.action == action]
        
        if not action_transitions:
            return {
                'observations': 0,
                'avg_reward': 0.0,
                'success_rate': 0.0,
                'effect_patterns': [],
                'causal_rules': []
            }
        
        # Calculer statistiques
        avg_reward = float(np.mean([t.reward for t in action_transitions]))
        success_rate = len([t for t in action_transitions if t.reward > 0]) / len(action_transitions)
        
        # Extraire patterns d'effets
        effect_patterns = self._cluster_effects(action_transitions)
        
        # Règles causales pour cette action
        action_rules = [r for r in self.causal_rules if r.action == action]
        
        return {
            'observations': len(action_transitions),
            'avg_reward': avg_reward,
            'success_rate': success_rate,
            'effect_patterns': effect_patterns,
            'causal_rules': action_rules
        }
    
    def _cluster_effects(self, transitions: List[StateTransition]) -> List[str]:
        """
        Identifie les patterns d'effets dominants
        
        Returns:
            Liste de descriptions d'effets
        """
        if not transitions:
            return []
        
        patterns = []
        
        # Analyser les diffs
        diffs = [t.compute_diff() for t in transitions]
        
        # Pattern 1: Identité
        identity_count = sum(1 for d in diffs if np.allclose(d, 0, atol=1e-6))
        if identity_count > len(diffs) * 0.5:
            patterns.append(f"identity ({identity_count}/{len(diffs)})")
        
        # Pattern 2: Changements localisés
        local_changes = sum(1 for d in diffs if 0.1 < np.count_nonzero(d) / d.size < 0.5)
        if local_changes > len(diffs) * 0.3:
            patterns.append(f"local_changes ({local_changes}/{len(diffs)})")
        
        # Pattern 3: Changements globaux
        global_changes = sum(1 for d in diffs if np.count_nonzero(d) / d.size > 0.5)
        if global_changes > len(diffs) * 0.3:
            patterns.append(f"global_changes ({global_changes}/{len(diffs)})")
        
        return patterns if patterns else ["unknown"]
    
    def evaluate_prediction_accuracy(self) -> float:
        """
        Évalue la précision des prédictions sur les dernières transitions
        
        Returns:
            Précision moyenne (0-1)
        """
        if len(self.transitions) < 20:
            return 0.0
        
        # Tester sur les 20 dernières transitions
        test_transitions = self.transitions[-20:]
        accuracies = []
        
        for t in test_transitions:
            # Prédire outcome
            predicted_state = self.predict_outcome(t.state_before, t.action)
            
            # Calculer erreur
            error = np.mean(np.abs(predicted_state - t.state_after))
            
            # Convertir en précision (1 = parfait, 0 = très mauvais)
            accuracy = 1.0 / (1.0 + error)
            accuracies.append(accuracy)
        
        avg_accuracy = float(np.mean(accuracies))
        self.stats['prediction_accuracy'] = avg_accuracy
        
        return avg_accuracy
    
    def get_statistics(self) -> Dict:
        """Retourne les statistiques du world model"""
        # Mettre à jour précision
        if len(self.transitions) >= 20:
            self.evaluate_prediction_accuracy()
        
        return {
            **self.stats,
            'transitions_stored': len(self.transitions),
            'causal_graph_size': len(self.causal_graph),
            'cache_size': len(self.prediction_cache),
            'cache_hit_rate': self.stats['cache_hits'] / (self.stats['cache_hits'] + self.stats['cache_misses']) if (self.stats['cache_hits'] + self.stats['cache_misses']) > 0 else 0.0
        }
    
    def __repr__(self):
        return f"WorldModel(transitions={len(self.transitions)}, rules={len(self.causal_rules)}, accuracy={self.stats['prediction_accuracy']:.2f})"

# Made with Bob
