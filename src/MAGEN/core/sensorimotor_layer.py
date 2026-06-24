#!/usr/bin/env python3
"""
Layer 0: Sensorimotor Layer
===========================

Couche de base de l'architecture MAGEN V2.
Découverte de l'agency et des affordances sensorimotrices.

Principe: Un agent doit d'abord découvrir CE QU'IL PEUT FAIRE
avant de pouvoir apprendre COMMENT le faire efficacement.

Basé sur:
- Sensorimotor contingencies (O'Regan & Noë, 2001)
- Embodied cognition (Varela, Thompson & Rosch, 1991)
- Affordance theory (Gibson, 1979)

Author: Bob (Assistant IA)
Date: 2026-06-18
Version: V2.0
"""

import numpy as np
from typing import Dict, List, Optional, Tuple, Set, Any
from dataclasses import dataclass, field
from collections import defaultdict
import json
import hashlib

# Imports modules existants
try:
    from .action_discovery_engine import ActionDiscoveryEngine, DiscoveredAction
    from .affordance_discovery_engine import AffordanceDiscoveryEngine, Affordance, AffordanceContext
    from .action_space_explorer import ActionSpaceExplorer, ActionEffect
except ImportError:
    # Fallback pour tests
    ActionDiscoveryEngine = None
    AffordanceDiscoveryEngine = None
    ActionSpaceExplorer = None
    Affordance = None
    AffordanceContext = None
    DiscoveredAction = None
    ActionEffect = None


@dataclass
class SensorimotorContingency:
    """
    Contingence sensorimotrice: relation action → effet observable.
    
    Représente la connaissance qu'une action produit un effet
    prévisible dans l'environnement.
    """
    action: str
    preconditions: Dict[str, Any]  # Conditions nécessaires
    expected_effect: Dict[str, Any]  # Effet attendu
    confidence: float  # Confiance (0-1)
    observations: int  # Nombre d'observations
    success_rate: float  # Taux de succès
    
    def to_dict(self) -> Dict:
        return {
            'action': self.action,
            'preconditions': self.preconditions,
            'expected_effect': self.expected_effect,
            'confidence': self.confidence,
            'observations': self.observations,
            'success_rate': self.success_rate
        }


@dataclass
class AgencyCapability:
    """
    Capacité d'agency: ce que l'agent peut faire.
    
    Représente une capacité découverte de l'agent
    (mouvement, manipulation, etc.)
    """
    capability_type: str  # 'movement', 'manipulation', 'observation'
    actions: List[str]  # Actions associées
    constraints: Dict[str, Any]  # Contraintes (ex: grid boundaries)
    reliability: float  # Fiabilité (0-1)
    discovered_at: int  # Step de découverte
    
    def to_dict(self) -> Dict:
        return {
            'capability_type': self.capability_type,
            'actions': self.actions,
            'constraints': self.constraints,
            'reliability': self.reliability,
            'discovered_at': self.discovered_at
        }


@dataclass
class SensorimotorOutput:
    """
    Output de la couche sensorimotrice.
    
    Contient les contingences détectées et affordances extraites.
    """
    contingencies: List[SensorimotorContingency]
    affordances: List  # List[Affordance] ou List vide si module non disponible
    grid_shape: Tuple[int, ...]  # Accepte n'importe quelle taille de tuple
    
    def to_dict(self) -> Dict:
        return {
            'num_contingencies': len(self.contingencies),
            'num_affordances': len(self.affordances),
            'grid_shape': self.grid_shape,
            'contingencies': [c.to_dict() for c in self.contingencies]
        }


class SensorimotorLayer:
    """
    Layer 0: Sensorimotor Layer
    
    Découvre et maintient:
    1. Agency capabilities (ce que l'agent PEUT faire)
    2. Affordances (ce qui est POSSIBLE dans contexte)
    3. Sensorimotor contingencies (action → effet)
    
    Cette couche est la FONDATION de tout apprentissage.
    Sans elle, l'agent ne sait pas ce qu'il peut faire.
    """
    
    def __init__(
        self,
        grid_width: int = 60,
        grid_height: int = 60,
        min_observations: int = 3,
        confidence_threshold: float = 0.7,
        forensic_logger=None
    ):
        """
        Initialise la couche sensorimotrice.
        
        Args:
            grid_width: Largeur grille
            grid_height: Hauteur grille
            min_observations: Observations min pour confiance
            confidence_threshold: Seuil confiance
            forensic_logger: Logger forensique
        """
        self.grid_width = grid_width
        self.grid_height = grid_height
        self.min_observations = min_observations
        self.confidence_threshold = confidence_threshold
        self.forensic_logger = forensic_logger
        
        # Modules de découverte
        if ActionDiscoveryEngine:
            self.action_discovery = ActionDiscoveryEngine()
        else:
            self.action_discovery = None
            
        if AffordanceDiscoveryEngine:
            self.affordance_discovery = AffordanceDiscoveryEngine(
                forensic_log_path=None
            )
        else:
            self.affordance_discovery = None
            
        if ActionSpaceExplorer:
            self.action_explorer = ActionSpaceExplorer(
                max_delay_steps=3,
                verbose=False
            )
        else:
            self.action_explorer = None
        
        # État interne
        self.capabilities: Dict[str, AgencyCapability] = {}
        self.contingencies: Dict[str, SensorimotorContingency] = {}
        self.affordances_cache: Dict[str, List[Affordance]] = {}
        
        # Statistiques
        self.total_observations = 0
        self.successful_predictions = 0
        self.failed_predictions = 0
        
        self._log_forensic("SENSORIMOTOR_INIT", {
            "grid_size": (grid_width, grid_height),
            "min_observations": min_observations,
            "confidence_threshold": confidence_threshold
        })
    
    def discover_agency(
        self,
        train_examples: List[Dict],
        available_actions: List[str]
    ) -> Dict[str, AgencyCapability]:
        """
        Découvre les capacités d'agency de l'agent.
        
        Analyse les exemples d'entraînement pour identifier:
        - Quelles actions l'agent peut effectuer
        - Quelles contraintes existent (boundaries, etc.)
        - Quelle fiabilité pour chaque capacité
        
        Args:
            train_examples: Exemples d'entraînement
            available_actions: Actions disponibles
            
        Returns:
            Dict des capacités découvertes
        """
        self._log_forensic("AGENCY_DISCOVERY_START", {
            "num_examples": len(train_examples),
            "num_actions": len(available_actions)
        })
        
        # Découvrir actions via ActionDiscoveryEngine
        if self.action_discovery and train_examples:
            discovered_actions = self.action_discovery.discover_actions(train_examples)
            
            # Grouper par type de capacité
            movement_actions = []
            manipulation_actions = []
            
            for action in discovered_actions:
                action_name = action.transform_name if hasattr(action, 'transform_name') else str(action)
                if 'move' in action_name.lower() or 'rotate' in action_name.lower():
                    movement_actions.append(action_name)
                else:
                    manipulation_actions.append(action_name)
            
            # Créer capacités
            if movement_actions:
                self.capabilities['movement'] = AgencyCapability(
                    capability_type='movement',
                    actions=movement_actions,
                    constraints={
                        'grid_width': self.grid_width,
                        'grid_height': self.grid_height
                    },
                    reliability=0.9,  # Haute fiabilité pour mouvement
                    discovered_at=0
                )
            
            if manipulation_actions:
                self.capabilities['manipulation'] = AgencyCapability(
                    capability_type='manipulation',
                    actions=manipulation_actions,
                    constraints={},
                    reliability=0.8,  # Fiabilité moyenne pour manipulation
                    discovered_at=0
                )
        
        # Ajouter actions disponibles comme capacité d'observation
        if available_actions:
            self.capabilities['observation'] = AgencyCapability(
                capability_type='observation',
                actions=available_actions,
                constraints={},
                reliability=1.0,  # Observation toujours fiable
                discovered_at=0
            )
        
        self._log_forensic("AGENCY_DISCOVERY_COMPLETE", {
            "num_capabilities": len(self.capabilities),
            "capabilities": {k: v.to_dict() for k, v in self.capabilities.items()}
        })
        
        return self.capabilities
    
    def extract_affordances(
        self,
        state: np.ndarray,
        available_actions: List[str],
        context: Optional[Dict] = None
    ) -> List[Affordance]:
        """
        Extrait les affordances du contexte actuel.
        
        Affordance = possibilité d'action dans contexte donné.
        Ex: "Je peux bouger à droite car case vide"
        
        Args:
            state: État actuel (grille)
            available_actions: Actions disponibles
            context: Contexte additionnel
            
        Returns:
            Liste des affordances
        """
        # Hash état pour cache
        state_hash = self._hash_state(state)
        
        # Vérifier cache
        if state_hash in self.affordances_cache:
            return self.affordances_cache[state_hash]
        
        affordances = []
        
        # Utiliser AffordanceDiscoveryEngine si disponible
        if self.affordance_discovery:
            affordances = self.affordance_discovery.discover_affordances(
                state=state,
                available_actions=available_actions
            )
        else:
            # Fallback: affordances basiques (dict simple)
            import time
            for action in available_actions:
                if Affordance:
                    affordances.append(Affordance(
                        action_id=action,
                        perceptual_score=0.5,
                        feasibility_score=0.5,
                        plausibility=0.5,
                        reasons=["Basic affordance"],
                        timestamp=time.time()
                    ))
                else:
                    # Fallback dict si Affordance non disponible
                    affordances.append({
                        'action_id': action,
                        'plausibility': 0.5
                    })
        
        # Mettre en cache
        self.affordances_cache[state_hash] = affordances
        
        # Log forensic
        top_actions = []
        for a in affordances[:3]:
            if hasattr(a, 'action_id'):
                top_actions.append(a.action_id)
            elif isinstance(a, dict):
                top_actions.append(a.get('action_id', 'unknown'))
        
        self._log_forensic("AFFORDANCES_EXTRACTED", {
            "state_hash": state_hash,
            "num_affordances": len(affordances),
            "top_affordances": top_actions
        })
        
        return affordances
    
    def learn_contingency(
        self,
        action: str,
        state_before: np.ndarray,
        state_after: np.ndarray,
        success: bool
    ) -> None:
        """
        Apprend une contingence sensorimotrice.
        
        Enregistre: action dans contexte → effet observé
        
        Args:
            action: Action effectuée
            state_before: État avant
            state_after: État après
            success: Succès de l'action
        """
        # Créer clé contingence
        preconditions = self._extract_preconditions(state_before)
        key = self._contingency_key(action, preconditions)
        
        # Calculer effet
        effect = self._compute_effect(state_before, state_after)
        
        # Mettre à jour ou créer contingence
        if key in self.contingencies:
            cont = self.contingencies[key]
            cont.observations += 1
            
            # Mettre à jour taux succès
            total_success = cont.success_rate * (cont.observations - 1)
            cont.success_rate = (total_success + (1.0 if success else 0.0)) / cont.observations
            
            # Mettre à jour confiance
            cont.confidence = min(1.0, cont.observations / self.min_observations)
            
            # Mettre à jour effet attendu (moyenne mobile)
            alpha = 0.3  # Taux apprentissage
            for k, v in effect.items():
                if k in cont.expected_effect:
                    cont.expected_effect[k] = (1 - alpha) * cont.expected_effect[k] + alpha * v
                else:
                    cont.expected_effect[k] = v
        else:
            # Nouvelle contingence
            self.contingencies[key] = SensorimotorContingency(
                action=action,
                preconditions=preconditions,
                expected_effect=effect,
                confidence=min(1.0, 1.0 / self.min_observations),
                observations=1,
                success_rate=1.0 if success else 0.0
            )
        
        self.total_observations += 1
        
        if success:
            self.successful_predictions += 1
        else:
            self.failed_predictions += 1
    
    def predict_effect(
        self,
        action: str,
        state: np.ndarray
    ) -> Tuple[Optional[Dict], float]:
        """
        Prédit l'effet d'une action dans un état donné.
        
        Args:
            action: Action à prédire
            state: État actuel
            
        Returns:
            (effet_prédit, confiance)
        """
        preconditions = self._extract_preconditions(state)
        key = self._contingency_key(action, preconditions)
        
        if key in self.contingencies:
            cont = self.contingencies[key]
            if cont.confidence >= self.confidence_threshold:
                return cont.expected_effect, cont.confidence
        
        return None, 0.0
    
    def process(self, grid: np.ndarray) -> 'SensorimotorOutput':
        """
        Méthode wrapper pour compatibilité avec tests.
        Traite une grille et retourne les contingences détectées.
        
        Args:
            grid: Grille d'entrée (numpy array)
            
        Returns:
            SensorimotorOutput avec contingences détectées
        """
        # Extraire affordances de la grille
        available_actions = ['move_up', 'move_down', 'move_left', 'move_right', 'wait']
        affordances = self.extract_affordances(grid, available_actions)
        
        # Récupérer contingences apprises
        contingencies = self.get_sensorimotor_contingencies(min_confidence=0.3)
        
        # Créer output
        return SensorimotorOutput(
            contingencies=contingencies,
            affordances=affordances,
            grid_shape=grid.shape
        )
    
    def get_sensorimotor_contingencies(
        self,
        min_confidence: float = 0.5
    ) -> List[SensorimotorContingency]:
        """
        Retourne les contingences sensorimotrices apprises.
        
        Args:
            min_confidence: Confiance minimale
            
        Returns:
            Liste des contingences
        """
        return [
            cont for cont in self.contingencies.values()
            if cont.confidence >= min_confidence
        ]
    
    def _extract_preconditions(self, state: np.ndarray) -> Dict[str, Any]:
        """Extrait préconditions d'un état."""
        return {
            'shape': state.shape,
            'unique_colors': len(np.unique(state)),
            'density': np.count_nonzero(state) / state.size
        }
    
    def _compute_effect(
        self,
        state_before: np.ndarray,
        state_after: np.ndarray
    ) -> Dict[str, Any]:
        """Calcule l'effet d'une transition."""
        diff = state_after - state_before
        return {
            'changed_cells': np.count_nonzero(diff),
            'added_cells': np.sum(diff > 0),
            'removed_cells': np.sum(diff < 0),
            'shape_change': state_after.shape != state_before.shape
        }
    
    def _contingency_key(self, action: str, preconditions: Dict) -> str:
        """Génère clé unique pour contingence."""
        key_str = f"{action}_{preconditions['shape']}_{preconditions['unique_colors']}"
        return hashlib.md5(key_str.encode()).hexdigest()[:16]
    
    def _hash_state(self, state: np.ndarray) -> str:
        """Hash rapide d'un état."""
        return hashlib.md5(state.tobytes()).hexdigest()[:16]
    
    def _log_forensic(self, event_type: str, data: Dict):
        """Log événement forensique."""
        if self.forensic_logger:
            try:
                self.forensic_logger.log(
                    event=event_type,
                    component="SensorimotorLayer",
                    data=data
                )
            except Exception:
                pass
    
    def get_statistics(self) -> Dict[str, Any]:
        """Retourne statistiques de la couche."""
        prediction_accuracy = 0.0
        if self.total_observations > 0:
            prediction_accuracy = self.successful_predictions / self.total_observations
        
        return {
            'num_capabilities': len(self.capabilities),
            'num_contingencies': len(self.contingencies),
            'total_observations': self.total_observations,
            'prediction_accuracy': prediction_accuracy,
            'cache_size': len(self.affordances_cache),
            'capabilities': {k: v.to_dict() for k, v in self.capabilities.items()}
        }
    
    def reset(self):
        """Réinitialise la couche."""
        self.capabilities.clear()
        self.contingencies.clear()
        self.affordances_cache.clear()
        self.total_observations = 0
        self.successful_predictions = 0
        self.failed_predictions = 0
        
        self._log_forensic("SENSORIMOTOR_RESET", {})


def test_sensorimotor_layer():
    """Test basique de la couche sensorimotrice."""
    print("🧪 Test SensorimotorLayer...")
    
    # Créer couche
    layer = SensorimotorLayer(
        grid_width=30,
        grid_height=30,
        min_observations=3
    )
    
    # Test 1: Découverte agency
    print("\n1. Test découverte agency...")
    train_examples = [
        {
            'input': np.random.randint(0, 10, (10, 10)),
            'output': np.random.randint(0, 10, (10, 10))
        }
        for _ in range(3)
    ]
    available_actions = ['move_up', 'move_down', 'move_left', 'move_right', 'wait']
    
    capabilities = layer.discover_agency(train_examples, available_actions)
    print(f"   ✓ Capacités découvertes: {len(capabilities)}")
    
    # Test 2: Extraction affordances
    print("\n2. Test extraction affordances...")
    state = np.random.randint(0, 10, (10, 10))
    affordances = layer.extract_affordances(state, available_actions)
    print(f"   ✓ Affordances extraites: {len(affordances)}")
    
    # Test 3: Apprentissage contingences
    print("\n3. Test apprentissage contingences...")
    state_before = np.zeros((10, 10))
    state_after = np.zeros((10, 10))
    state_after[5, 5] = 1
    
    for i in range(5):
        layer.learn_contingency('move_right', state_before, state_after, success=True)
    
    print(f"   ✓ Contingences apprises: {len(layer.contingencies)}")
    
    # Test 4: Prédiction effet
    print("\n4. Test prédiction effet...")
    effect, confidence = layer.predict_effect('move_right', state_before)
    print(f"   ✓ Effet prédit: {effect is not None}, confiance: {confidence:.2f}")
    
    # Statistiques
    print("\n📊 Statistiques:")
    stats = layer.get_statistics()
    print(f"   - Capacités: {stats['num_capabilities']}")
    print(f"   - Contingences: {stats['num_contingencies']}")
    print(f"   - Observations: {stats['total_observations']}")
    print(f"   - Précision: {stats['prediction_accuracy']:.2%}")
    
    print("\n✅ Test SensorimotorLayer réussi!")


if __name__ == "__main__":
    test_sensorimotor_layer()

# Made with Bob
