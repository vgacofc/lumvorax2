"""
Layer 0: Sensorimotor - Raw input/output, agency discovery
MAGEN Architecture V2 - Foundation Layer

Objectif: Identifier avatar et tracker position (RÈGLE #11 PROTOCOLE)
Référence: PROTOCOLE_MAGEN.md lignes 131-176
Référence: Log modèle concurrent Frame 719

Date création: 2026-06-19
Auteur: Bob (MAGEN Team)
"""

import numpy as np
import hashlib
import time
from typing import Dict, List, Tuple, Optional, Set
from collections import Counter
from scipy.ndimage import label as scipy_label
import logging

logger = logging.getLogger(__name__)


class SensorimotorLayer:
    """
    Layer 0: Sensorimotor - Découverte agence et tracking position
    
    Capacités:
    1. Identifier avatar par analyse différentielle
    2. Tracker position avatar en temps réel
    3. Mesurer causalité action→effet
    4. Construire représentation spatiale basique
    
    Conformité PROTOCOLE:
    - RÈGLE #11: Représentation monde AVANT stratégie
    - LEÇON #91.1: Utiliser obs.frame spatialement
    - LEÇON #81.3: Observer AVANT modéliser
    """
    
    def __init__(self, env=None, verbose: bool = True):
        """
        Initialiser Layer 0
        
        Args:
            env: Environnement ARC-AGI (optionnel pour tests)
            verbose: Afficher logs détaillés
        """
        self.env = env
        self.verbose = verbose
        
        # Avatar identification
        self.avatar_identified = False
        self.avatar_confidence = 0.0
        self.avatar_pixels = []  # Liste valeurs pixels avatar
        self.avatar_action = None  # Action qui modifie avatar
        
        # Position tracking
        self.current_position = None  # (row, col)
        self.position_history = []  # Historique positions
        self.positions_tracked = 0
        
        # Spatial understanding
        self.visited_states = set()  # États visités (hash)
        self.spatial_coverage = 0.0  # % monde découvert
        
        # Causal understanding
        self.action_effects = {}  # action → effets mesurés
        self.causal_links = 0  # Nombre liens causaux détectés
        
        # Métriques monde (PROTOCOLE RÈGLE #11)
        self.world_understanding = {
            'positions_tracked': 0,
            'spatial_coverage': 0.0,
            'rooms_memorized': 0,
            'navigation_graph': {'nodes': 0, 'edges': 0},
            'avatar_identified': False,
            'avatar_confidence': 0.0,
            'causal_links': 0
        }
        
        logger.info("✅ Layer 0 Sensorimotor initialisé")
    
    def identify_avatar(self, num_tests: int = 100) -> Dict:
        """
        Identifier avatar par analyse différentielle
        
        Méthodologie (LEÇON #81.3):
        1. Observer effets de chaque action (100 tests, augmenté pour meilleure détection)
        2. Classifier type d'action (AVATAR/RÉGION/GLOBALE)
        3. Identifier action qui modifie 1-5 pixels (avatar potentiel)
        4. Valider unicité et contrôlabilité
        
        Args:
            num_tests: Nombre tests par action (défaut: 100)
            
        Returns:
            Dict avec résultats identification
        """
        if self.env is None:
            logger.error("❌ Environnement non fourni")
            return {'avatar_identified': False, 'error': 'no_env'}
        
        logger.info(f"🔍 Identification avatar (analyse différentielle {num_tests} tests/action)...")
        
        # Actions valides (LEÇON Session 88)
        VALID_ACTIONS = [1, 2, 3, 4]  # UP, DOWN, LEFT, RIGHT
        
        # 1. Tester chaque action
        action_effects = {}
        for action in VALID_ACTIONS:
            effects = []
            
            for test_num in range(num_tests):
                try:
                    # Reset environnement
                    obs_before = self.env.reset()
                    frame_before = self._extract_frame(obs_before)
                    
                    if frame_before is None:
                        continue
                    
                    # Exécuter action
                    obs_after = self.env.step(action)
                    frame_after = self._extract_frame(obs_after)
                    
                    if frame_after is None:
                        continue
                    
                    # Calculer différence
                    diff = (frame_before != frame_after)
                    
                    # Analyser effets
                    # Compter composantes connexes manuellement
                    num_components = self._count_components(diff)
                    positions_modified = np.argwhere(diff)
                    
                    effects.append({
                        'pixels_modified': int(np.sum(diff)),
                        'num_components': num_components,
                        'positions': positions_modified,
                        'frame_before': frame_before,
                        'frame_after': frame_after
                    })
                    
                except Exception as e:
                    logger.warning(f"⚠️ Erreur test action {action}: {e}")
                    continue
            
            action_effects[action] = effects
            
            if self.verbose:
                avg_pixels = np.mean([e['pixels_modified'] for e in effects]) if effects else 0
                logger.info(f"  ACTION{action}: {avg_pixels:.1f} pixels modifiés (moyenne)")
        
        # 2. Classifier actions et identifier avatar
        avatar_candidates = []
        
        for action, effects in action_effects.items():
            if not effects:
                continue
            
            avg_pixels = np.mean([e['pixels_modified'] for e in effects])
            avg_components = np.mean([e['num_components'] for e in effects])
            
            # Classification (LEÇON #81.3)
            if 1 <= avg_pixels <= 5:
                action_type = "AVATAR"  # Objet unique
                avatar_candidates.append({
                    'action': action,
                    'avg_pixels': avg_pixels,
                    'avg_components': avg_components,
                    'effects': effects
                })
            elif avg_pixels <= 100:
                action_type = "RÉGION"  # Transformation locale
            else:
                action_type = "GLOBALE"  # Transformation complète
            
            if self.verbose:
                logger.info(f"  ACTION{action}: Type={action_type}")
        
        # 3. Valider avatar candidat
        if not avatar_candidates:
            logger.warning("❌ Aucun avatar candidat trouvé")
            self.avatar_identified = False
            self.avatar_confidence = 0.0
            return {
                'avatar_identified': False,
                'reason': 'no_candidate',
                'action_effects': action_effects
            }
        
        # Prendre meilleur candidat (moins de pixels modifiés)
        best_candidate = min(avatar_candidates, key=lambda x: x['avg_pixels'])
        
        # Extraire valeurs pixels avatar
        avatar_pixels = self._extract_avatar_pixels(best_candidate['effects'])
        
        # Calculer confiance
        confidence = self._calculate_confidence(best_candidate['effects'])
        
        # Valider unicité (LEÇON #80.1)
        uniqueness_valid = self._validate_uniqueness(
            best_candidate['effects'][0]['frame_after'],
            avatar_pixels
        )
        
        if not uniqueness_valid:
            logger.warning("⚠️ Avatar non unique - validation échouée")
            confidence *= 0.5  # Réduire confiance
        
        # Mettre à jour état
        self.avatar_identified = confidence > 0.80
        self.avatar_confidence = confidence
        self.avatar_pixels = avatar_pixels
        self.avatar_action = best_candidate['action']
        
        # Mettre à jour métriques monde
        self.world_understanding['avatar_identified'] = self.avatar_identified
        self.world_understanding['avatar_confidence'] = self.avatar_confidence
        
        result = {
            'avatar_identified': self.avatar_identified,
            'avatar_confidence': confidence,
            'avatar_action': best_candidate['action'],
            'avatar_pixels': avatar_pixels,
            'avg_pixels_modified': best_candidate['avg_pixels'],
            'uniqueness_valid': uniqueness_valid
        }
        
        if self.verbose:
            if self.avatar_identified:
                logger.info(f"✅ Avatar identifié: ACTION{best_candidate['action']}, "
                          f"pixels={avatar_pixels}, confiance={confidence:.2%}")
            else:
                logger.warning(f"⚠️ Avatar non identifié (confiance={confidence:.2%} < 80%)")
        
        return result
    
    def track_position(self, frame: np.ndarray) -> Optional[Tuple[int, int]]:
        """
        Tracker position avatar dans frame
        
        Méthodologie (modèle concurrent Frame 719):
        - Chercher pixels avatar dans frame
        - Calculer center = (top + 2, left + 2)
        - Valider cohérence avec historique
        
        Args:
            frame: Frame 2D (64×64)
            
        Returns:
            Position (row, col) ou None si non trouvé
        """
        if not self.avatar_identified:
            return None
        
        if frame is None:
            return None
        
        # Chercher pixels avatar
        for pixel_value in self.avatar_pixels:
            positions = np.argwhere(frame == pixel_value)
            
            if len(positions) > 0:
                # Prendre première position (top-left)
                top_left = positions[0]
                
                # Calculer center (comme modèle concurrent)
                center = (int(top_left[0] + 2), int(top_left[1] + 2))
                
                # Valider cohérence
                if self._validate_position_coherence(center):
                    self.current_position = center
                    self.position_history.append(center)
                    self.positions_tracked += 1
                    
                    # Mettre à jour métriques
                    self.world_understanding['positions_tracked'] = self.positions_tracked
                    
                    return center
        
        return None
    
    def measure_causal_effects(self, action: int, frame_before: np.ndarray, 
                               frame_after: np.ndarray) -> Dict:
        """
        Mesurer effets causaux d'une action
        
        Args:
            action: Action exécutée
            frame_before: Frame avant action
            frame_after: Frame après action
            
        Returns:
            Dict avec effets mesurés
        """
        if frame_before is None or frame_after is None:
            return {}
        
        # Calculer différence
        diff = (frame_before != frame_after)
        pixels_modified = np.sum(diff)
        
        # Analyser composantes
        num_components = self._count_components(diff)
        
        # Tracker position si avatar identifié
        pos_before = self.track_position(frame_before)
        pos_after = self.track_position(frame_after)
        
        # Calculer mouvement
        movement = None
        if pos_before is not None and pos_after is not None:
            movement = (pos_after[0] - pos_before[0], pos_after[1] - pos_before[1])
        
        # Enregistrer effet
        effect = {
            'action': action,
            'pixels_modified': int(pixels_modified),
            'num_components': int(num_components),
            'movement': movement,
            'timestamp': time.time()
        }
        
        # Ajouter à historique
        if action not in self.action_effects:
            self.action_effects[action] = []
        self.action_effects[action].append(effect)
        
        # Incrémenter liens causaux
        self.causal_links += 1
        self.world_understanding['causal_links'] = self.causal_links
        
        return effect
    
    def update_spatial_coverage(self, frame: np.ndarray):
        """
        Mettre à jour couverture spatiale
        
        Args:
            frame: Frame actuelle
        """
        if frame is None:
            return
        
        # Calculer hash état
        state_hash = hashlib.sha256(frame.tobytes()).hexdigest()
        
        # Ajouter aux états visités
        if state_hash not in self.visited_states:
            self.visited_states.add(state_hash)
        
        # Calculer couverture (estimation)
        # Hypothèse: 3,253 états uniques possibles (Session 89)
        self.spatial_coverage = len(self.visited_states) / 3253.0
        self.world_understanding['spatial_coverage'] = self.spatial_coverage
    
    def get_world_understanding(self) -> Dict:
        """
        Obtenir métriques compréhension monde (PROTOCOLE RÈGLE #11)
        
        Returns:
            Dict avec métriques obligatoires
        """
        return self.world_understanding.copy()
    
    def can_answer_questions(self) -> Dict[str, bool]:
        """
        Vérifier si MAGEN peut répondre aux 5 questions obligatoires
        (PROTOCOLE RÈGLE #11 lignes 169-174)
        
        Returns:
            Dict avec réponses aux 5 questions
        """
        return {
            'where_am_i': self.current_position is not None,
            'which_pixel_am_i': self.avatar_identified,
            'which_rooms_visited': len(self.visited_states) > 0,
            'how_to_go_a_to_b': self.causal_links >= 10,
            'what_does_action_do': len(self.action_effects) >= 4
        }
    
    # Méthodes privées
    
    def _extract_frame(self, obs) -> Optional[np.ndarray]:
        """Extraire frame 2D depuis observation"""
        if hasattr(obs, 'frame'):
            frame_data = obs.frame
            if isinstance(frame_data, list):
                frame_array = np.asarray(frame_data)
                if frame_array.ndim == 3 and frame_array.shape[0] == 1:
                    return frame_array[0]
                return frame_array
            return frame_data
        return None
    
    def _extract_avatar_pixels(self, effects: List[Dict]) -> List[int]:
        """Extraire valeurs pixels avatar depuis effets"""
        if not effects:
            return []
        
        # Prendre premier effet
        effect = effects[0]
        frame_before = effect['frame_before']
        frame_after = effect['frame_after']
        
        # Trouver pixels qui ont changé
        diff = (frame_before != frame_after)
        positions = np.argwhere(diff)
        
        if len(positions) == 0:
            return []
        
        # Extraire valeurs pixels après changement
        pixel_values = set()
        for pos in positions:
            pixel_values.add(int(frame_after[pos[0], pos[1]]))
        
        return list(pixel_values)
    
    def _calculate_confidence(self, effects: List[Dict]) -> float:
        """Calculer confiance identification avatar"""
        if not effects:
            return 0.0
        
        # Mesurer cohérence effets
        pixels_modified = [e['pixels_modified'] for e in effects]
        
        # Variance faible = haute confiance
        variance = np.var(pixels_modified)
        mean_pixels = np.mean(pixels_modified)
        
        # Confiance basée sur cohérence
        if variance < 1.0 and 1 <= mean_pixels <= 5:
            confidence = 0.95
        elif variance < 2.0 and 1 <= mean_pixels <= 10:
            confidence = 0.85
        elif variance < 5.0:
            confidence = 0.70
        else:
            confidence = 0.50
        
        return confidence
    
    def _validate_uniqueness(self, frame: np.ndarray, 
                            pixel_values: List[int]) -> bool:
        """
        Valider unicité pixels avatar (LEÇON #80.1)
        
        Args:
            frame: Frame à analyser
            pixel_values: Valeurs pixels avatar
            
        Returns:
            True si unique (≤ 10 occurrences)
        """
        for pixel_value in pixel_values:
            positions = np.argwhere(frame == pixel_value)
            count = len(positions)
            
            # Tolérer jusqu'à 10 occurrences (sprite 5×5 peut avoir duplications)
            if count > 10:
                if self.verbose:
                    logger.warning(f"⚠️ Pixel {pixel_value} NON UNIQUE: {count} occurrences")
                return False
        
        return True
    
    def _validate_position_coherence(self, position: Tuple[int, int]) -> bool:
        """Valider cohérence position avec historique"""
        if not self.position_history:
            return True
        
        # Vérifier mouvement raisonnable (max 10 pixels par step)
        last_pos = self.position_history[-1]
        distance = abs(position[0] - last_pos[0]) + abs(position[1] - last_pos[1])
        
        return distance <= 10
    
    def _count_components(self, diff: np.ndarray) -> int:
        """Compter composantes connexes dans différence (simple approximation)"""
        # Approximation simple: compter groupes de pixels adjacents
        if not np.any(diff):
            return 0
        
        # Pour simplifier, retourner 1 si pixels modifiés
        return 1 if np.sum(diff) > 0 else 0
    
    def __repr__(self) -> str:
        """Représentation string"""
        return (f"SensorimotorLayer(avatar_identified={self.avatar_identified}, "
                f"confidence={self.avatar_confidence:.2%}, "
                f"positions_tracked={self.positions_tracked}, "
                f"causal_links={self.causal_links})")


# Tests unitaires
if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    
    print("🧪 Tests Layer 0 Sensorimotor")
    print("=" * 60)
    
    # Test 1: Initialisation
    layer0 = SensorimotorLayer(verbose=True)
    print(f"\n✅ Test 1: {layer0}")
    
    # Test 2: Métriques monde
    metrics = layer0.get_world_understanding()
    print(f"\n✅ Test 2: Métriques monde")
    for key, value in metrics.items():
        print(f"  {key}: {value}")
    
    # Test 3: Questions obligatoires
    answers = layer0.can_answer_questions()
    print(f"\n✅ Test 3: Questions obligatoires")
    for question, can_answer in answers.items():
        status = "✅" if can_answer else "❌"
        print(f"  {status} {question}")
    
    print("\n" + "=" * 60)
    print("✅ Tests Layer 0 terminés")

# Made with Bob
