#!/usr/bin/env python3
"""
MAGEN ARC-AGI Agent Systemic - Apprentissage Systémique (Phase 4.5.1)
======================================================================

Agent MAGEN avec apprentissage systémique intégrant:
- WorldModel: Cartographie complète du monde
- SelfIdentification: Identification automatique avatar
- Observation système complet (pas analyse locale)
- Extension de MAGENEnhancedAgent (préserve layers 0-2)

Paradigme:
- Ancien: Analyser timer seul, porte seule, avatar seul → ÉCHEC
- Nouveau: Observer système complet simultanément → VICTOIRE

Author: Bob (MAGEN Project)
Date: 2026-06-20
Version: 4.5.1 (Systemic Learning)
"""

import sys
import os
import numpy as np
import json
from typing import Dict, Any, Tuple, Optional, List
from pathlib import Path
from datetime import datetime
import logging

# Ajouter chemin MAGEN
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

# Imports MAGEN existants
try:
    from arc_integration.magen_arc_agent_enhanced import MAGENEnhancedAgent
except ImportError:
    from .magen_arc_agent_enhanced import MAGENEnhancedAgent

try:
    from arc_integration.ls20_real_wrapper import LS20RealWrapper
except ImportError:
    from .ls20_real_wrapper import LS20RealWrapper

# Imports nouveaux modules systémiques
try:
    from arc_integration.world_model import WorldModel, Entity
except ImportError:
    from .world_model import WorldModel, Entity

try:
    from arc_integration.self_identification import SelfIdentification
except ImportError:
    from .self_identification import SelfIdentification

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


class MAGENSystemicAgent(MAGENEnhancedAgent):
    """
    Agent MAGEN avec apprentissage systémique
    
    Étend MAGENEnhancedAgent en ajoutant:
    - WorldModel: Cartographie complète monde
    - SelfIdentification: Identification avatar automatique
    - Observation systémique: Tout simultanément
    
    Préserve:
    - Layer 0 Sensorimotor (layers 0-2)
    - Reward Shaper
    - Q-learning de base
    """
    
    def __init__(
        self,
        env: Optional[LS20RealWrapper] = None,
        n_actions: int = 4,
        learning_rate: float = 0.1,
        gamma: float = 0.99,
        epsilon: float = 0.9,
        epsilon_decay: float = 0.995,
        epsilon_min: float = 0.01,
        use_layer0: bool = True,
        use_reward_shaper: bool = True,
        use_systemic: bool = True,
        verbose: bool = True,
        log_dir: Optional[str] = None
    ):
        """
        Initialise agent systémique
        
        Args:
            use_systemic: Activer apprentissage systémique
            log_dir: Répertoire logs forensiques
            (autres args: voir MAGENEnhancedAgent)
        """
        # Initialiser agent de base
        super().__init__(
            env=env,
            n_actions=n_actions,
            learning_rate=learning_rate,
            gamma=gamma,
            epsilon=epsilon,
            epsilon_decay=epsilon_decay,
            epsilon_min=epsilon_min,
            use_layer0=use_layer0,
            use_reward_shaper=use_reward_shaper,
            verbose=verbose
        )
        
        self.use_systemic = use_systemic
        
        # Modules systémiques
        if self.use_systemic:
            self.world_model = WorldModel()
            self.self_identification = SelfIdentification()
            logger.info("✅ Apprentissage systémique activé")
            logger.info("   - WorldModel: Cartographie complète")
            logger.info("   - SelfIdentification: Détection avatar")
        else:
            self.world_model = None
            self.self_identification = None
        
        # Logs forensiques
        if log_dir is None:
            self.log_dir = Path(__file__).parent / "logs" / "phase_4_5_1"
        else:
            self.log_dir = Path(log_dir)
        self.log_dir.mkdir(parents=True, exist_ok=True)
        
        # Historique épisodes
        self.episode_data = []
        self.current_episode = None
        
        if self.verbose:
            print("\n" + "="*80)
            print("🧠 MAGEN SYSTEMIC AGENT INITIALISÉ (Phase 4.5.1)")
            print("="*80)
            print(f"🌍 WorldModel: {'✅ ACTIVÉ' if use_systemic else '❌ DÉSACTIVÉ'}")
            print(f"👤 SelfIdentification: {'✅ ACTIVÉ' if use_systemic else '❌ DÉSACTIVÉ'}")
            print(f"📁 Logs: {self.log_dir}")
            print("="*80 + "\n")
    
    def start_episode(self, episode_num: int):
        """Démarrer nouvel épisode"""
        self.current_episode = {
            'episode': episode_num,
            'start_time': datetime.now().isoformat(),
            'steps': [],
            'world_observations': [],
            'identification_progress': [],
            'total_reward': 0.0,
            'victory': False
        }
    
    def observe_systemic(self, state: np.ndarray, action: int, reward: float) -> Dict:
        """
        Observer système complet (approche systémique)
        
        Pas: observer timer seul, puis porte seule, puis avatar seul
        Mais: observer TOUT simultanément
        
        Args:
            state: État actuel (grille)
            action: Action effectuée
            reward: Récompense reçue
            
        Returns:
            Dict avec observations complètes
        """
        observation = {
            'step': self.total_steps,
            'action': action,
            'reward': reward
        }
        
        if not self.use_systemic:
            return observation
        
        # 1. Observer monde complet
        entities = self.world_model.observe_complete_state(state)
        
        # 2. Mettre à jour identification avatar
        if self.total_steps > 0 and hasattr(self, '_prev_entities'):
            self.self_identification.update(
                action=action,
                entities_before=self._prev_entities,
                entities_after=entities
            )
        
        # Sauvegarder pour prochaine itération
        self._prev_entities = entities.copy()
        
        # 3. Collecter observations
        if self.world_model and self.self_identification:
            observation.update({
                'world_model': {
                    'total_entities': len(entities),
                    'mobile_entities': len([e for e in entities.values() if e.is_mobile]),
                    'invariants': len(self.world_model.invariants),
                    'cycles': len(self.world_model.cycles),
                    'spatial_graph_nodes': self.world_model.spatial_graph.number_of_nodes(),
                    'spatial_graph_edges': self.world_model.spatial_graph.number_of_edges()
                },
                'self_identification': {
                    'avatar_id': self.self_identification.get_avatar_id(),
                    'confidence': self.self_identification.get_confidence(),
                    'candidates': self.self_identification.get_candidates()[:3],
                    'identified': self.self_identification.get_avatar_id() is not None
                }
            })
        
        return observation
    
    def train_episode(self, max_steps: int = 100) -> Dict:
        """
        Entraîner un épisode avec apprentissage systémique
        
        Args:
            max_steps: Nombre maximum de steps
            
        Returns:
            Dict avec résultats épisode
        """
        if self.env is None:
            raise ValueError("Environnement non initialisé")
        
        # Démarrer épisode
        self.episode_count += 1
        self.start_episode(self.episode_count)
        
        # Reset environnement
        state = self.env.reset()
        done = False
        episode_reward = 0.0
        step = 0
        
        # Reset identification pour nouvel épisode
        if self.use_systemic:
            self.self_identification.reset()
            self._prev_entities = {}
        
        while not done and step < max_steps:
            # Sélectionner action
            action = self.select_action(state, training=True)
            
            # Exécuter action
            next_state, reward, done, info = self.env.step(action)
            
            # Observer système complet (APPROCHE SYSTÉMIQUE)
            observation = self.observe_systemic(state, action, reward)
            
            # Apprendre (Q-learning de base)
            agent_pos = info.get('agent_pos', None)
            self.learn(state, action, reward, next_state, done, agent_pos)
            
            # Enregistrer step
            if self.current_episode:
                self.current_episode['steps'].append({
                    'step': step,
                    'action': action,
                    'reward': reward,
                    'done': done
                })
                self.current_episode['world_observations'].append(observation)
                
                # Enregistrer progression identification
                if self.use_systemic:
                    self.current_episode['identification_progress'].append({
                        'step': step,
                        'avatar_id': self.self_identification.get_avatar_id(),
                        'confidence': self.self_identification.get_confidence(),
                        'candidates': self.self_identification.get_candidates()[:3]
                    })
            
            # Mise à jour
            state = next_state
            episode_reward += reward
            step += 1
            self.total_steps += 1
        
        # Fin épisode
        if self.current_episode:
            self.current_episode['total_reward'] = episode_reward
            self.current_episode['victory'] = reward > 0 and done
            self.current_episode['end_time'] = datetime.now().isoformat()
            self.current_episode['total_steps'] = step
            
            # Ajouter résumé monde
            if self.use_systemic:
                self.current_episode['world_summary'] = self.world_model.get_summary()
                self.current_episode['identification_summary'] = self.self_identification.get_statistics()
            
            self.episode_data.append(self.current_episode)
        
        # Decay epsilon
        if self.epsilon > self.epsilon_min:
            self.epsilon *= self.epsilon_decay
        
        # Résultats
        result = {
            'episode': self.episode_count,
            'steps': step,
            'reward': episode_reward,
            'victory': reward > 0 and done,
            'epsilon': self.epsilon
        }
        
        if self.use_systemic:
            result.update({
                'avatar_identified': self.self_identification.get_avatar_id() is not None,
                'avatar_confidence': self.self_identification.get_confidence(),
                'entities_detected': len(self.world_model.entities),
                'invariants_detected': len(self.world_model.invariants),
                'cycles_detected': len(self.world_model.cycles)
            })
        
        return result
    
    def save_logs(self, filename: Optional[str] = None):
        """
        Sauvegarder logs forensiques
        
        Args:
            filename: Nom fichier (auto si None)
        """
        if filename is None:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            filename = f"phase_4_5_1_systemic_{timestamp}.json"
        
        filepath = self.log_dir / filename
        
        # Préparer données
        log_data = {
            'metadata': {
                'phase': '4.5.1',
                'agent_type': 'MAGENSystemicAgent',
                'timestamp': datetime.now().isoformat(),
                'total_episodes': self.episode_count,
                'total_steps': self.total_steps,
                'use_systemic': self.use_systemic
            },
            'episodes': self.episode_data,
            'final_state': {
                'epsilon': self.epsilon,
                'q_table_size': len(self.q_table),
                'states_visited': len(self.states_visited)
            }
        }
        
        if self.use_systemic:
            log_data['final_state'].update({
                'world_model_summary': self.world_model.get_summary(),
                'identification_summary': self.self_identification.get_statistics()
            })
        
        # Sauvegarder
        with open(filepath, 'w') as f:
            json.dump(log_data, f, indent=2)
        
        logger.info(f"✅ Logs sauvegardés: {filepath}")
        return filepath
    
    def get_summary(self) -> Dict:
        """
        Obtenir résumé complet agent
        
        Returns:
            Dict avec statistiques complètes
        """
        summary = {
            'episodes': self.episode_count,
            'total_steps': self.total_steps,
            'epsilon': self.epsilon,
            'q_table_size': len(self.q_table),
            'states_visited': len(self.states_visited)
        }
        
        if self.use_systemic and self.world_model:
            summary['world_model'] = self.world_model.get_summary()
            summary['identification'] = self.self_identification.get_statistics()
        
        return summary
    
    def __repr__(self):
        if self.use_systemic:
            return (f"MAGENSystemicAgent(episodes={self.episode_count}, "
                   f"steps={self.total_steps}, "
                   f"avatar_id={self.self_identification.get_avatar_id()}, "
                   f"confidence={self.self_identification.get_confidence():.2%})")
        else:
            return f"MAGENSystemicAgent(episodes={self.episode_count}, steps={self.total_steps})"

# Made with Bob
