#!/usr/bin/env python3
"""
MAGEN ARC-AGI Agent - Intégration Architecture Cognitive Complète
================================================================

Agent RL utilisant l'architecture MAGEN V2 (10 layers) pour résoudre
les jeux ARC-AGI en temps réel.

Architecture:
- Layer 0-9: Pipeline cognitif complet
- Interface Gym-compatible
- Logs forensiques temps réel
- Métriques de performance

Author: MAGEN Team
Date: 2026-06-19
"""

import sys
import os
import numpy as np
from typing import Dict, Any, Tuple, Optional, List
from dataclasses import dataclass
import time

# Ajouter le chemin vers MAGEN
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

# Import architecture MAGEN
from layers.layer0_sensorimotor import SensorimotorLayer
from layers.layer1_vision import VisionLayer
from layers.layer2_objects import ObjectsLayer
from layers.layer3_attention import AttentionLayer
from layers.layer4_causality import CausalityLayer
from layers.layer5_world_model import WorldModelLayer
from layers.layer6_semantic_memory import SemanticMemoryLayer
from layers.layer7_reasoning import ReasoningLayer
from layers.layer8_meta_learning import MetaLearningLayer
from layers.layer9_prediction import PredictionLayer


@dataclass
class AgentMetrics:
    """Métriques de performance de l'agent."""
    episode: int
    step: int
    reward: float
    cumulative_reward: float
    action: int
    action_name: str
    processing_time_ms: float
    layer_activations: Dict[str, float]
    success: bool
    timestamp: float


class MAGENARCAgent:
    """
    Agent MAGEN pour ARC-AGI.
    
    Utilise l'architecture cognitive complète (10 layers) pour:
    - Percevoir l'environnement (Layers 0-2)
    - Raisonner sur les actions (Layers 3-7)
    - Apprendre et s'adapter (Layers 8-9)
    """
    
    def __init__(
        self,
        state_shape: Tuple[int, int] = (64, 64),
        n_actions: int = 4,
        learning_rate: float = 0.001,
        gamma: float = 0.99,
        epsilon: float = 1.0,
        epsilon_decay: float = 0.995,
        epsilon_min: float = 0.01,
        memory_size: int = 10000,
        batch_size: int = 32,
        verbose: bool = True
    ):
        """
        Initialise l'agent MAGEN.
        
        Args:
            state_shape: Dimensions de l'état (H, W)
            n_actions: Nombre d'actions possibles
            learning_rate: Taux d'apprentissage
            gamma: Facteur de discount
            epsilon: Exploration initiale
            epsilon_decay: Décroissance epsilon
            epsilon_min: Epsilon minimum
            memory_size: Taille mémoire replay
            batch_size: Taille batch entraînement
            verbose: Affichage détaillé
        """
        self.state_shape = state_shape
        self.n_actions = n_actions
        self.learning_rate = learning_rate
        self.gamma = gamma
        self.epsilon = epsilon
        self.epsilon_decay = epsilon_decay
        self.epsilon_min = epsilon_min
        self.memory_size = memory_size
        self.batch_size = batch_size
        self.verbose = verbose
        
        # Initialiser les 10 layers MAGEN
        self._init_layers()
        
        # Métriques
        self.episode_count = 0
        self.total_steps = 0
        self.metrics_history: List[AgentMetrics] = []
        
        if self.verbose:
            print("\n" + "="*80)
            print("🧠 MAGEN ARC-AGI AGENT INITIALISÉ")
            print("="*80)
            print(f"📊 Architecture: 10 layers cognitives")
            print(f"🎮 Actions: {n_actions}")
            print(f"📐 État: {state_shape}")
            print(f"🎯 Epsilon: {epsilon:.3f} → {epsilon_min:.3f}")
            print("="*80 + "\n")
    
    def _init_layers(self):
        """Initialise les 10 layers de l'architecture MAGEN."""
        # Layer 0: Sensorimotor (perception brute)
        self.layer0 = SensorimotorLayer(
            input_shape=self.state_shape,
            n_actions=self.n_actions
        )
        
        # Layer 1: Vision (extraction features visuelles)
        self.layer1 = VisionLayer(
            input_shape=self.state_shape,
            n_filters=32
        )
        
        # Layer 2: Objects (détection objets)
        self.layer2 = ObjectsLayer(
            feature_dim=32,
            max_objects=10
        )
        
        # Layer 3: Attention (focus sur éléments importants)
        self.layer3 = AttentionLayer(
            feature_dim=32,
            n_heads=4
        )
        
        # Layer 4: Causality (relations cause-effet)
        self.layer4 = CausalityLayer(
            feature_dim=32,
            n_relations=8
        )
        
        # Layer 5: World Model (modèle du monde)
        self.layer5 = WorldModelLayer(
            state_dim=32,
            action_dim=self.n_actions,
            hidden_dim=64
        )
        
        # Layer 6: Semantic Memory (mémoire sémantique)
        self.layer6 = SemanticMemoryLayer(
            memory_size=self.memory_size,
            feature_dim=32
        )
        
        # Layer 7: Reasoning (raisonnement, MetaArbiter)
        self.layer7 = ReasoningLayer(
            feature_dim=32,
            n_actions=self.n_actions,
            learning_rate=self.learning_rate
        )
        
        # Layer 8: Meta-Learning (apprentissage méta)
        self.layer8 = MetaLearningLayer(
            feature_dim=32,
            adaptation_rate=0.01
        )
        
        # Layer 9: Prediction (prédiction états futurs)
        self.layer9 = PredictionLayer(
            state_dim=32,
            action_dim=self.n_actions,
            horizon=5
        )
        
        if self.verbose:
            print("✅ 10 layers MAGEN initialisées")
    
    def select_action(self, state: np.ndarray, training: bool = True) -> int:
        """
        Sélectionne une action via le pipeline cognitif complet.
        
        Args:
            state: État actuel (H, W)
            training: Mode entraînement (epsilon-greedy)
            
        Returns:
            Action sélectionnée (0-3)
        """
        start_time = time.time()
        
        # Exploration vs Exploitation
        if training and np.random.random() < self.epsilon:
            action = np.random.randint(0, self.n_actions)
            if self.verbose:
                print(f"🎲 Exploration: action={action}")
            return action
        
        # Pipeline cognitif MAGEN
        # Layer 0: Perception sensorimotrice
        sensory_input = self.layer0.process(state)
        
        # Layer 1: Extraction features visuelles
        visual_features = self.layer1.extract_features(sensory_input)
        
        # Layer 2: Détection objets
        objects = self.layer2.detect_objects(visual_features)
        
        # Layer 3: Attention sur objets importants
        attended_features = self.layer3.attend(objects)
        
        # Layer 4: Analyse causalité
        causal_relations = self.layer4.analyze(attended_features)
        
        # Layer 5: Mise à jour world model
        world_state = self.layer5.update(causal_relations)
        
        # Layer 6: Récupération mémoire sémantique
        memory_context = self.layer6.retrieve(world_state)
        
        # Layer 7: Raisonnement et sélection action (MetaArbiter)
        action_logits = self.layer7.reason(world_state, memory_context)
        action = np.argmax(action_logits)
        
        # Layer 8: Adaptation méta-apprentissage
        self.layer8.adapt(world_state, action)
        
        # Layer 9: Prédiction états futurs
        predicted_states = self.layer9.predict(world_state, action)
        
        processing_time = (time.time() - start_time) * 1000
        
        if self.verbose:
            print(f"🧠 Pipeline MAGEN: action={action}, temps={processing_time:.2f}ms")
        
        return action
    
    def learn(
        self,
        state: np.ndarray,
        action: int,
        reward: float,
        next_state: np.ndarray,
        done: bool
    ):
        """
        Apprentissage à partir d'une transition.
        
        Args:
            state: État actuel
            action: Action exécutée
            reward: Récompense reçue
            next_state: État suivant
            done: Épisode terminé
        """
        # Stocker dans mémoire sémantique (Layer 6)
        transition = {
            'state': state,
            'action': action,
            'reward': reward,
            'next_state': next_state,
            'done': done
        }
        self.layer6.store(transition)
        
        # Entraîner Layer 7 (Reasoning/MetaArbiter)
        if len(self.layer6.memory) >= self.batch_size:
            batch = self.layer6.sample(self.batch_size)
            loss = self.layer7.train(batch, self.gamma)
            
            if self.verbose and self.total_steps % 100 == 0:
                print(f"📚 Training: loss={loss:.4f}, epsilon={self.epsilon:.3f}")
        
        # Décroissance epsilon
        if done:
            self.epsilon = max(self.epsilon_min, self.epsilon * self.epsilon_decay)
    
    def reset_episode(self):
        """Reset pour nouvel épisode."""
        self.episode_count += 1
        
        if self.verbose:
            print(f"\n🔄 Épisode {self.episode_count} - Reset")
    
    def log_metrics(
        self,
        step: int,
        action: int,
        action_name: str,
        reward: float,
        cumulative_reward: float,
        success: bool,
        processing_time_ms: float
    ):
        """Enregistre les métriques de performance."""
        metrics = AgentMetrics(
            episode=self.episode_count,
            step=step,
            reward=reward,
            cumulative_reward=cumulative_reward,
            action=action,
            action_name=action_name,
            processing_time_ms=processing_time_ms,
            layer_activations={
                'layer0': float(np.mean(self.layer0.get_activation())),
                'layer1': float(np.mean(self.layer1.get_activation())),
                'layer7': float(np.mean(self.layer7.get_activation()))
            },
            success=success,
            timestamp=time.time()
        )
        
        self.metrics_history.append(metrics)
        self.total_steps += 1
    
    def get_statistics(self) -> Dict[str, Any]:
        """Retourne statistiques de performance."""
        if not self.metrics_history:
            return {}
        
        recent_metrics = self.metrics_history[-100:]
        
        return {
            'total_episodes': self.episode_count,
            'total_steps': self.total_steps,
            'avg_reward': np.mean([m.reward for m in recent_metrics]),
            'avg_processing_time_ms': np.mean([m.processing_time_ms for m in recent_metrics]),
            'success_rate': np.mean([m.success for m in recent_metrics]),
            'epsilon': self.epsilon,
            'memory_size': len(self.layer6.memory)
        }
    
    def save(self, filepath: str):
        """Sauvegarde l'agent."""
        import pickle
        
        state = {
            'episode_count': self.episode_count,
            'total_steps': self.total_steps,
            'epsilon': self.epsilon,
            'metrics_history': self.metrics_history,
            'layer7_weights': self.layer7.get_weights()
        }
        
        with open(filepath, 'wb') as f:
            pickle.dump(state, f)
        
        print(f"💾 Agent sauvegardé: {filepath}")
    
    def load(self, filepath: str):
        """Charge l'agent."""
        import pickle
        
        with open(filepath, 'rb') as f:
            state = pickle.load(f)
        
        self.episode_count = state['episode_count']
        self.total_steps = state['total_steps']
        self.epsilon = state['epsilon']
        self.metrics_history = state['metrics_history']
        self.layer7.set_weights(state['layer7_weights'])
        
        print(f"📂 Agent chargé: {filepath}")


def test_agent():
    """Test de l'agent MAGEN."""
    print("\n" + "="*80)
    print("🧪 TEST AGENT MAGEN")
    print("="*80 + "\n")
    
    # Créer agent
    agent = MAGENARCAgent(
        state_shape=(64, 64),
        n_actions=4,
        verbose=True
    )
    
    # Test sélection action
    print("\n📊 Test sélection action...")
    state = np.random.randint(0, 10, (64, 64), dtype=np.int8)
    
    for i in range(5):
        action = agent.select_action(state, training=True)
        print(f"   Action {i+1}: {action}")
    
    # Test apprentissage
    print("\n📚 Test apprentissage...")
    next_state = np.random.randint(0, 10, (64, 64), dtype=np.int8)
    agent.learn(state, 2, -0.01, next_state, False)
    
    # Statistiques
    print("\n📈 Statistiques:")
    stats = agent.get_statistics()
    for key, value in stats.items():
        print(f"   {key}: {value}")
    
    print("\n" + "="*80)
    print("✅ TEST AGENT MAGEN RÉUSSI")
    print("="*80 + "\n")


if __name__ == "__main__":
    test_agent()

# Made with Bob
