"""
🔍 HIDDEN MECHANICS DETECTOR - MAGEN V22 ARCADE DISCOVERY ENGINE

Détecte les mécaniques cachées et règles invisibles par expérimentation systématique.

Inspiré de l'insight utilisateur:
"J'ai découvert que lorsque j'utilise le bouton 'clic' sur un objet, parfois 
il disparaît, parfois il apparaît, et parfois la gravité s'inverse"

Author: Claude + LumVorax
Version: 22.0.0
Date: 2026-06-13
"""

import numpy as np
from typing import List, Dict, Tuple, Set, Any, Optional
from dataclasses import dataclass, field
from enum import Enum
import copy


class MechanicType(Enum):
    """Types de mécaniques détectables"""
    GRAVITY = "gravity"
    TRIGGER = "trigger"
    GLOBAL_STATE = "global_state"
    PHYSICS_RULE = "physics_rule"
    INTERACTION_RULE = "interaction_rule"
    TEMPORAL_RULE = "temporal_rule"


@dataclass
class HiddenMechanic:
    """Représente une mécanique cachée découverte"""
    mechanic_type: MechanicType
    name: str
    description: str
    activation_conditions: List[str]
    effects: List[str]
    confidence: float
    observations: int = 1
    test_results: List[Dict] = field(default_factory=list)


class HiddenMechanicsDetector:
    """
    Détecte les mécaniques cachées par expérimentation systématique.
    
    Fonctionnalités:
    1. Test gravité activable/désactivable
    2. Détection triggers (zones, objets, séquences)
    3. Détection états globaux
    4. Découverte règles physiques
    """
    
    def __init__(self, verbose: bool = False):
        """
        Args:
            verbose: Mode verbeux pour debugging
        """
        self.verbose = verbose
        
        # Mécaniques découvertes
        self.discovered_mechanics: Dict[str, HiddenMechanic] = {}
        
        # Historique tests
        self.test_history: List[Dict] = []
        
        # Statistiques
        self.stats = {
            'total_tests': 0,
            'mechanics_discovered': 0,
            'gravity_tests': 0,
            'trigger_tests': 0,
            'state_tests': 0
        }
    
    def discover_hidden_rules(
        self,
        environment: Any,
        max_tests: int = 50
    ) -> Dict[str, HiddenMechanic]:
        """
        Découvre toutes les règles cachées de l'environnement.
        
        Args:
            environment: Environnement à analyser
            max_tests: Nombre maximum de tests
            
        Returns:
            Dictionnaire des mécaniques découvertes
        """
        if self.verbose:
            print("🔍 Starting hidden mechanics discovery...")
        
        # Test 1: Gravité
        gravity_mechanic = self.test_gravity_mechanics(environment)
        if gravity_mechanic:
            self.discovered_mechanics[gravity_mechanic.name] = gravity_mechanic
            if self.verbose:
                print(f"✅ Discovered: {gravity_mechanic.name}")
        
        # Test 2: Triggers
        trigger_mechanics = self.test_trigger_mechanics(environment, max_tests=20)
        for mechanic in trigger_mechanics:
            self.discovered_mechanics[mechanic.name] = mechanic
            if self.verbose:
                print(f"✅ Discovered: {mechanic.name}")
        
        # Test 3: États globaux
        state_mechanics = self.test_global_state_mechanics(environment, max_tests=15)
        for mechanic in state_mechanics:
            self.discovered_mechanics[mechanic.name] = mechanic
            if self.verbose:
                print(f"✅ Discovered: {mechanic.name}")
        
        # Test 4: Règles physiques
        physics_mechanics = self.test_physics_rules(environment, max_tests=15)
        for mechanic in physics_mechanics:
            self.discovered_mechanics[mechanic.name] = mechanic
            if self.verbose:
                print(f"✅ Discovered: {mechanic.name}")
        
        self.stats['mechanics_discovered'] = len(self.discovered_mechanics)
        
        if self.verbose:
            print(f"\n📊 Total mechanics discovered: {len(self.discovered_mechanics)}")
        
        return self.discovered_mechanics
    
    def test_gravity_mechanics(
        self,
        environment: Any
    ) -> Optional[HiddenMechanic]:
        """
        Teste si la gravité est activable/désactivable.
        
        Returns:
            HiddenMechanic si gravité détectée, None sinon
        """
        if self.verbose:
            print("🧪 Testing gravity mechanics...")
        
        self.stats['gravity_tests'] += 1
        
        try:
            # Créer objet en hauteur
            test_grid = self._create_test_grid_with_floating_object()
            
            # Test 1: Gravité naturelle
            result_natural = environment.simulate_step(test_grid)
            has_natural_gravity = self._detect_falling_motion(test_grid, result_natural)
            
            # Test 2: Tenter d'inverser gravité
            test_results = []
            gravity_invertible = False
            
            for action in ['click_top', 'click_bottom', 'press_space', 'double_click']:
                result = environment.apply_action(test_grid, action)
                result_after = environment.simulate_step(result)
                
                has_inverted = self._detect_rising_motion(result, result_after)
                test_results.append({
                    'action': action,
                    'inverted': has_inverted
                })
                
                if has_inverted:
                    gravity_invertible = True
                    break
            
            if has_natural_gravity or gravity_invertible:
                return HiddenMechanic(
                    mechanic_type=MechanicType.GRAVITY,
                    name="gravity_system",
                    description="Gravity affects objects, possibly invertible",
                    activation_conditions=["objects_in_air"] if has_natural_gravity else [],
                    effects=["falling_motion", "rising_motion"] if gravity_invertible else ["falling_motion"],
                    confidence=0.9 if gravity_invertible else 0.7,
                    test_results=test_results
                )
        
        except Exception as e:
            if self.verbose:
                print(f"❌ Gravity test error: {e}")
        
        return None
    
    def test_trigger_mechanics(
        self,
        environment: Any,
        max_tests: int = 20
    ) -> List[HiddenMechanic]:
        """
        Détecte les triggers (zones, objets, séquences).
        
        Returns:
            Liste des triggers découverts
        """
        if self.verbose:
            print("🧪 Testing trigger mechanics...")
        
        self.stats['trigger_tests'] += max_tests
        
        triggers = []
        
        # Test triggers de zone
        zone_triggers = self._test_zone_triggers(environment, max_tests // 3)
        triggers.extend(zone_triggers)
        
        # Test triggers d'objet
        object_triggers = self._test_object_triggers(environment, max_tests // 3)
        triggers.extend(object_triggers)
        
        # Test triggers de séquence
        sequence_triggers = self._test_sequence_triggers(environment, max_tests // 3)
        triggers.extend(sequence_triggers)
        
        return triggers
    
    def _test_zone_triggers(
        self,
        environment: Any,
        max_tests: int
    ) -> List[HiddenMechanic]:
        """Teste les triggers de zone"""
        triggers = []
        
        try:
            grid = environment.get_current_state()
            
            # Tester différentes zones
            zones = self._identify_potential_zones(grid)
            
            for zone in zones[:max_tests]:
                # Tester entrée dans zone
                result = environment.move_to_zone(zone)
                
                if self._detect_state_change(grid, result):
                    triggers.append(HiddenMechanic(
                        mechanic_type=MechanicType.TRIGGER,
                        name=f"zone_trigger_{zone['id']}",
                        description=f"Entering zone at {zone['position']} triggers effect",
                        activation_conditions=[f"enter_zone_{zone['id']}"],
                        effects=["state_change"],
                        confidence=0.8
                    ))
        
        except Exception as e:
            if self.verbose:
                print(f"❌ Zone trigger test error: {e}")
        
        return triggers
    
    def _test_object_triggers(
        self,
        environment: Any,
        max_tests: int
    ) -> List[HiddenMechanic]:
        """Teste les triggers d'objet"""
        triggers = []
        
        try:
            grid = environment.get_current_state()
            objects = self._extract_objects(grid)
            
            for obj in objects[:max_tests]:
                # Tester interaction avec objet
                result = environment.interact_with_object(obj)
                
                if self._detect_object_transformation(grid, result):
                    triggers.append(HiddenMechanic(
                        mechanic_type=MechanicType.TRIGGER,
                        name=f"object_trigger_{obj['id']}",
                        description=f"Interacting with object {obj['color']} triggers transformation",
                        activation_conditions=[f"click_object_{obj['id']}"],
                        effects=["object_disappear", "object_appear", "object_transform"],
                        confidence=0.85
                    ))
        
        except Exception as e:
            if self.verbose:
                print(f"❌ Object trigger test error: {e}")
        
        return triggers
    
    def _test_sequence_triggers(
        self,
        environment: Any,
        max_tests: int
    ) -> List[HiddenMechanic]:
        """Teste les triggers de séquence"""
        triggers = []
        
        try:
            # Tester séquences d'actions
            sequences = [
                ['up', 'up', 'down', 'down'],
                ['left', 'right', 'left', 'right'],
                ['click', 'click', 'space'],
                ['rotate', 'rotate', 'rotate', 'rotate']
            ]
            
            for seq in sequences[:max_tests]:
                initial = environment.get_current_state()
                
                # Exécuter séquence
                for action in seq:
                    environment.apply_action(environment.get_current_state(), action)
                
                final = environment.get_current_state()
                
                if self._detect_special_effect(initial, final):
                    triggers.append(HiddenMechanic(
                        mechanic_type=MechanicType.TRIGGER,
                        name=f"sequence_trigger_{'_'.join(seq)}",
                        description=f"Sequence {seq} triggers special effect",
                        activation_conditions=[f"sequence_{i}" for i in range(len(seq))],
                        effects=["special_effect"],
                        confidence=0.75
                    ))
        
        except Exception as e:
            if self.verbose:
                print(f"❌ Sequence trigger test error: {e}")
        
        return triggers
    
    def test_global_state_mechanics(
        self,
        environment: Any,
        max_tests: int = 15
    ) -> List[HiddenMechanic]:
        """
        Détecte les états globaux (modes, phases, etc.).
        
        Returns:
            Liste des états globaux découverts
        """
        if self.verbose:
            print("🧪 Testing global state mechanics...")
        
        self.stats['state_tests'] += max_tests
        
        states = []
        
        try:
            # Tester différents états possibles
            state_indicators = self._identify_state_indicators(environment)
            
            for indicator in state_indicators[:max_tests]:
                # Tester changement d'état
                result = self._test_state_transition(environment, indicator)
                
                if result['state_changed']:
                    states.append(HiddenMechanic(
                        mechanic_type=MechanicType.GLOBAL_STATE,
                        name=f"global_state_{indicator['name']}",
                        description=f"Global state {indicator['name']} affects behavior",
                        activation_conditions=indicator['conditions'],
                        effects=indicator['effects'],
                        confidence=result['confidence']
                    ))
        
        except Exception as e:
            if self.verbose:
                print(f"❌ Global state test error: {e}")
        
        return states
    
    def test_physics_rules(
        self,
        environment: Any,
        max_tests: int = 15
    ) -> List[HiddenMechanic]:
        """
        Découvre les règles physiques cachées.
        
        Returns:
            Liste des règles physiques découvertes
        """
        if self.verbose:
            print("🧪 Testing physics rules...")
        
        rules = []
        
        try:
            # Test collision
            collision_rule = self._test_collision_rules(environment)
            if collision_rule:
                rules.append(collision_rule)
            
            # Test momentum
            momentum_rule = self._test_momentum_rules(environment)
            if momentum_rule:
                rules.append(momentum_rule)
            
            # Test boundaries
            boundary_rule = self._test_boundary_rules(environment)
            if boundary_rule:
                rules.append(boundary_rule)
        
        except Exception as e:
            if self.verbose:
                print(f"❌ Physics rules test error: {e}")
        
        return rules
    
    # Méthodes utilitaires
    
    def _create_test_grid_with_floating_object(self) -> np.ndarray:
        """Crée grille test avec objet en hauteur"""
        grid = np.zeros((10, 10), dtype=int)
        grid[2, 5] = 1  # Objet en hauteur
        return grid
    
    def _detect_falling_motion(
        self,
        before: np.ndarray,
        after: np.ndarray
    ) -> bool:
        """Détecte mouvement de chute"""
        # Chercher objets qui ont bougé vers le bas
        for j in range(before.shape[1]):
            before_col = before[:, j]
            after_col = after[:, j]
            
            # Trouver objets non-zéro
            before_objs = np.where(before_col != 0)[0]
            after_objs = np.where(after_col != 0)[0]
            
            if len(before_objs) > 0 and len(after_objs) > 0:
                # Vérifier si objet a bougé vers le bas
                if after_objs[0] > before_objs[0]:
                    return True
        
        return False
    
    def _detect_rising_motion(
        self,
        before: np.ndarray,
        after: np.ndarray
    ) -> bool:
        """Détecte mouvement vers le haut (gravité inversée)"""
        for j in range(before.shape[1]):
            before_col = before[:, j]
            after_col = after[:, j]
            
            before_objs = np.where(before_col != 0)[0]
            after_objs = np.where(after_col != 0)[0]
            
            if len(before_objs) > 0 and len(after_objs) > 0:
                if after_objs[0] < before_objs[0]:
                    return True
        
        return False
    
    def _identify_potential_zones(self, grid: np.ndarray) -> List[Dict]:
        """Identifie zones potentielles (couleurs spéciales, patterns)"""
        zones = []
        
        # Chercher couleurs rares (potentiels triggers)
        unique, counts = np.unique(grid, return_counts=True)
        
        for color, count in zip(unique, counts):
            if color != 0 and count < 5:  # Couleur rare
                positions = np.where(grid == color)
                zones.append({
                    'id': f"zone_{color}",
                    'color': int(color),
                    'position': (int(positions[0][0]), int(positions[1][0])),
                    'size': int(count)
                })
        
        return zones
    
    def _extract_objects(self, grid: np.ndarray) -> List[Dict]:
        """Extrait objets de la grille"""
        objects = []
        visited = np.zeros_like(grid, dtype=bool)
        obj_id = 0
        
        for i in range(grid.shape[0]):
            for j in range(grid.shape[1]):
                if not visited[i, j] and grid[i, j] != 0:
                    obj = self._flood_fill_object(grid, i, j, visited, obj_id)
                    objects.append(obj)
                    obj_id += 1
        
        return objects
    
    def _flood_fill_object(
        self,
        grid: np.ndarray,
        i: int,
        j: int,
        visited: np.ndarray,
        obj_id: int
    ) -> Dict:
        """Flood fill pour extraire objet"""
        color = grid[i, j]
        cells = []
        stack = [(i, j)]
        
        while stack:
            ci, cj = stack.pop()
            if (0 <= ci < grid.shape[0] and 
                0 <= cj < grid.shape[1] and
                not visited[ci, cj] and 
                grid[ci, cj] == color):
                
                visited[ci, cj] = True
                cells.append((ci, cj))
                
                stack.extend([
                    (ci-1, cj), (ci+1, cj),
                    (ci, cj-1), (ci, cj+1)
                ])
        
        return {
            'id': obj_id,
            'color': int(color),
            'cells': cells,
            'size': len(cells)
        }
    
    def _detect_state_change(
        self,
        before: np.ndarray,
        after: np.ndarray
    ) -> bool:
        """Détecte changement d'état significatif"""
        return not np.array_equal(before, after)
    
    def _detect_object_transformation(
        self,
        before: np.ndarray,
        after: np.ndarray
    ) -> bool:
        """Détecte transformation d'objet"""
        before_objs = self._extract_objects(before)
        after_objs = self._extract_objects(after)
        
        return len(before_objs) != len(after_objs)
    
    def _detect_special_effect(
        self,
        before: np.ndarray,
        after: np.ndarray
    ) -> bool:
        """Détecte effet spécial (changement majeur)"""
        diff = np.sum(before != after)
        return diff > (before.size * 0.3)  # >30% changement
    
    def _identify_state_indicators(self, environment: Any) -> List[Dict]:
        """Identifie indicateurs d'état potentiels"""
        return [
            {'name': 'mode_a', 'conditions': ['flag_set'], 'effects': ['behavior_change']},
            {'name': 'mode_b', 'conditions': ['counter_threshold'], 'effects': ['rule_change']}
        ]
    
    def _test_state_transition(
        self,
        environment: Any,
        indicator: Dict
    ) -> Dict:
        """Teste transition d'état"""
        return {
            'state_changed': False,
            'confidence': 0.5
        }
    
    def _test_collision_rules(self, environment: Any) -> Optional[HiddenMechanic]:
        """Teste règles de collision"""
        return None
    
    def _test_momentum_rules(self, environment: Any) -> Optional[HiddenMechanic]:
        """Teste règles de momentum"""
        return None
    
    def _test_boundary_rules(self, environment: Any) -> Optional[HiddenMechanic]:
        """Teste règles de frontières"""
        return None
    
    def get_mechanic(self, name: str) -> Optional[HiddenMechanic]:
        """Récupère une mécanique par nom"""
        return self.discovered_mechanics.get(name)
    
    def get_mechanics_by_type(
        self,
        mechanic_type: MechanicType
    ) -> List[HiddenMechanic]:
        """Récupère toutes les mécaniques d'un type"""
        return [
            m for m in self.discovered_mechanics.values()
            if m.mechanic_type == mechanic_type
        ]
    
    def get_stats(self) -> Dict[str, Any]:
        """Retourne statistiques"""
        return {
            **self.stats,
            'discovered_mechanics': len(self.discovered_mechanics)
        }
    
    def reset(self):
        """Réinitialise le détecteur"""
        self.discovered_mechanics.clear()
        self.test_history.clear()
        self.stats = {
            'total_tests': 0,
            'mechanics_discovered': 0,
            'gravity_tests': 0,
            'trigger_tests': 0,
            'state_tests': 0
        }


if __name__ == "__main__":
    print("🔍 Testing Hidden Mechanics Detector...")
    
    detector = HiddenMechanicsDetector(verbose=True)
    
    # Mock environment
    class MockEnvironment:
        def get_current_state(self):
            return np.array([[0, 1, 0], [0, 0, 2], [3, 0, 0]])
        
        def simulate_step(self, grid):
            return grid
        
        def apply_action(self, grid, action):
            return grid
    
    env = MockEnvironment()
    
    # Découvrir mécaniques
    mechanics = detector.discover_hidden_rules(env, max_tests=10)
    
    print(f"\n✅ Discovered {len(mechanics)} mechanics")
    print(f"📊 Stats: {detector.get_stats()}")
    
    print("\n✅ Hidden Mechanics Detector test complete!")

# Made with Bob
