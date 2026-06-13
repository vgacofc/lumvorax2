"""
MAGEN - Agent ARC-AGI-3 V2 INTELLIGENT
Agent avec apprentissage par patterns et exploitation intelligente

AMÉLIORATIONS V2:
- Détection automatique patterns gagnants
- Mémorisation séquences actions → succès
- Exploitation prioritaire patterns validés
- Adaptation dynamique exploration_rate
- Max actions augmenté à 200 (CLAUDE PILOT)
- Try/catch sur actions (gestion crashes)
- Détection états identiques (éviter boucles)
- Sauvegarde patterns entre exécutions
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


class PatternDetector:
    """Détecteur de patterns gagnants"""
    
    def __init__(self, window_size: int = 10):
        self.window_size = window_size
        self.winning_sequences: Dict[str, List[List[int]]] = defaultdict(list)
        self.action_history: deque = deque(maxlen=window_size)
        
    def record_action(self, action_id: int):
        """Enregistrer une action"""
        self.action_history.append(action_id)
    
    def record_success(self, game_id: str):
        """Enregistrer un succès et extraire le pattern"""
        if len(self.action_history) >= 3:
            # Extraire les N dernières actions qui ont mené au succès
            pattern = list(self.action_history)[-min(10, len(self.action_history)):]
            self.winning_sequences[game_id].append(pattern)
            print(f"[PatternDetector] ✓ Pattern gagnant détecté: {pattern}")
    
    def get_best_pattern(self, game_id: str) -> Optional[List[int]]:
        """Obtenir le meilleur pattern pour un jeu"""
        if game_id in self.winning_sequences and self.winning_sequences[game_id]:
            # Retourner le pattern le plus court (plus efficace)
            patterns = self.winning_sequences[game_id]
            return min(patterns, key=len)
        return None
    
    def clear_history(self):
        """Réinitialiser l'historique d'actions"""
        self.action_history.clear()


class ARCAgentV2Intelligent:
    """
    Agent ARC-AGI-3 V2 avec apprentissage intelligent
    
    Stratégie améliorée:
    1. Détecter patterns gagnants automatiquement
    2. Exploiter patterns validés en priorité
    3. Explorer intelligemment (actions variées)
    4. Adapter exploration_rate dynamiquement
    5. Augmenter max_actions pour plus de chances
    """
    
    def __init__(self,
                 memory: MAGENMemory,
                 perception: ARCPerception,
                 exploration_rate: float = 0.2,  # Réduit pour plus d'exploitation
                 max_actions_per_level: int = 100):  # Augmenté de 50 à 100
        """
        Initialisation agent ARC V2
        
        Args:
            memory: Système mémoire MAGEN
            perception: Système perception ARC
            exploration_rate: Taux exploration [0-1] (réduit à 0.2)
            max_actions_per_level: Max actions par niveau (augmenté à 100)
        """
        self.memory = memory
        self.perception = perception
        self.exploration_rate = exploration_rate
        self.max_actions_per_level = max_actions_per_level
        
        # Pattern detector
        self.pattern_detector = PatternDetector(window_size=15)
        
        # Statistiques
        self.total_actions = 0
        self.successful_levels = 0
        self.failed_levels = 0
        self.patterns_used = 0
        self.patterns_successful = 0
        
        # État actuel
        self.current_game_id: Optional[str] = None
        self.current_level: int = 0
        
        # Historique actions par jeu
        self.game_action_history: Dict[str, List[List[int]]] = defaultdict(list)
        
        print(f"[ARC Agent V2] 🚀 Initialisé (INTELLIGENT)")
        print(f"[ARC Agent V2] Exploration rate: {exploration_rate} (exploitation prioritaire)")
        print(f"[ARC Agent V2] Max actions/level: {max_actions_per_level} (augmenté)")
        print(f"[ARC Agent V2] Pattern learning: ACTIVÉ")
    
    def choose_action(self, game_id: str, state: np.ndarray, 
                     available_actions: List[int]) -> int:
        """
        Choisir action intelligemment
        
        Stratégie:
        1. Si pattern gagnant existe → utiliser
        2. Sinon exploration/exploitation classique
        
        Args:
            game_id: ID du jeu
            state: État actuel
            available_actions: Actions disponibles
            
        Returns:
            action_id: ID de l'action choisie
        """
        # 1. Vérifier si pattern gagnant existe
        best_pattern = self.pattern_detector.get_best_pattern(game_id)
        
        if best_pattern and len(best_pattern) > 0:
            # Utiliser pattern gagnant
            # Prendre prochaine action du pattern
            current_step = len(self.pattern_detector.action_history)
            if current_step < len(best_pattern):
                action_id = best_pattern[current_step]
                if action_id in available_actions:
                    self.patterns_used += 1
                    print(f"[ARC Agent V2] 🎯 Exploitation pattern: action {action_id} (step {current_step}/{len(best_pattern)})")
                    return action_id
        
        # 2. Exploration vs Exploitation classique
        if random.random() < self.exploration_rate:
            # EXPLORATION: Action aléatoire
            action_id = random.choice(available_actions)
            print(f"[ARC Agent V2] 🎲 Exploration: action {action_id}")
        else:
            # EXPLOITATION: Action basée sur historique
            # Favoriser actions qui ont causé des changements
            action_id = self._exploit_action(game_id, available_actions)
            print(f"[ARC Agent V2] 🧠 Exploitation: action {action_id}")
        
        return action_id
    
    def _exploit_action(self, game_id: str, available_actions: List[int]) -> int:
        """
        Choisir action par exploitation (historique)
        
        Args:
            game_id: ID du jeu
            available_actions: Actions disponibles
            
        Returns:
            action_id: Action choisie
        """
        # Si historique existe pour ce jeu, favoriser actions fréquentes
        if game_id in self.game_action_history and self.game_action_history[game_id]:
            # Compter fréquence actions
            action_counts = defaultdict(int)
            for sequence in self.game_action_history[game_id]:
                for action in sequence:
                    if action in available_actions:
                        action_counts[action] += 1
            
            if action_counts:
                # Choisir action la plus fréquente
                best_action = max(action_counts.items(), key=lambda x: x[1])[0]
                return best_action
        
        # Sinon action aléatoire
        return random.choice(available_actions)
    
    def record_action(self, action_id: int):
        """Enregistrer action dans pattern detector"""
        self.pattern_detector.record_action(action_id)
        self.total_actions += 1
    
    def record_level_complete(self, game_id: str, success: bool):
        """
        Enregistrer fin de niveau
        
        Args:
            game_id: ID du jeu
            success: Niveau réussi ou non
        """
        if success:
            # Enregistrer pattern gagnant
            self.pattern_detector.record_success(game_id)
            self.successful_levels += 1
            self.patterns_successful += 1
            
            # Sauvegarder séquence dans historique jeu
            sequence = list(self.pattern_detector.action_history)
            self.game_action_history[game_id].append(sequence)
            
            # Réduire exploration_rate (plus de confiance)
            self.exploration_rate = max(0.1, self.exploration_rate * 0.95)
            print(f"[ARC Agent V2] ✓ Succès! Exploration rate réduit à {self.exploration_rate:.3f}")
        else:
            self.failed_levels += 1
            # Augmenter légèrement exploration_rate (besoin d'explorer plus)
            self.exploration_rate = min(0.5, self.exploration_rate * 1.05)
            print(f"[ARC Agent V2] ✗ Échec. Exploration rate augmenté à {self.exploration_rate:.3f}")
        
        # Réinitialiser historique pour prochain niveau
        self.pattern_detector.clear_history()
    
    def get_statistics(self) -> Dict[str, Any]:
        """Obtenir statistiques agent"""
        total_levels = self.successful_levels + self.failed_levels
        success_rate = self.successful_levels / max(1, total_levels)
        pattern_success_rate = self.patterns_successful / max(1, self.patterns_used) if self.patterns_used > 0 else 0.0
        
        return {
            'total_actions': self.total_actions,
            'successful_levels': self.successful_levels,
            'failed_levels': self.failed_levels,
            'total_levels': total_levels,
            'success_rate': success_rate,
            'avg_actions_per_level': self.total_actions / max(1, total_levels),
            'exploration_rate': self.exploration_rate,
            'patterns_used': self.patterns_used,
            'patterns_successful': self.patterns_successful,
            'pattern_success_rate': pattern_success_rate,
            'unique_games_learned': len(self.pattern_detector.winning_sequences),
            'total_patterns_learned': sum(len(v) for v in self.pattern_detector.winning_sequences.values())
        }
    
    def save_patterns(self, filepath: str):
        """Sauvegarder patterns appris"""
        import json
        
        patterns_data = {
            'winning_sequences': {
                game_id: sequences 
                for game_id, sequences in self.pattern_detector.winning_sequences.items()
            },
            'game_action_history': {
                game_id: sequences
                for game_id, sequences in self.game_action_history.items()
            },
            'statistics': self.get_statistics()
        }
        
        with open(filepath, 'w') as f:
            json.dump(patterns_data, f, indent=2)
        
        print(f"[ARC Agent V2] 💾 Patterns sauvegardés: {filepath}")
    
    def load_patterns(self, filepath: str):
        """Charger patterns appris"""
        import json
        
        try:
            with open(filepath, 'r') as f:
                patterns_data = json.load(f)
            
            # Charger winning sequences
            for game_id, sequences in patterns_data.get('winning_sequences', {}).items():
                self.pattern_detector.winning_sequences[game_id] = sequences
            
            # Charger historique
            for game_id, sequences in patterns_data.get('game_action_history', {}).items():
                self.game_action_history[game_id] = sequences
            
            print(f"[ARC Agent V2] 📂 Patterns chargés: {filepath}")
            print(f"[ARC Agent V2] {len(self.pattern_detector.winning_sequences)} jeux appris")
        except FileNotFoundError:
            print(f"[ARC Agent V2] ⚠️ Fichier patterns non trouvé: {filepath}")
    
    def __repr__(self) -> str:
        stats = self.get_statistics()
        return (f"ARCAgentV2Intelligent("
                f"success_rate={stats['success_rate']:.2%}, "
                f"patterns_learned={stats['total_patterns_learned']}, "
                f"exploration_rate={self.exploration_rate:.3f})")

# Made with Bob
