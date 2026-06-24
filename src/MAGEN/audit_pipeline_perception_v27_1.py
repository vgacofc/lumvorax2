"""
AUDIT PIPELINE PERCEPTIONNEL V27.1 - SESSION 73
================================================

Objectif: Tracer CHAQUE étape du pipeline perception pour identifier
où l'information est perdue/corrompue.

Pipeline:
  arcade.step(action)
    ↓
  obs_raw (dict/object)
    ↓
  agent.perceive(obs_raw)
    ↓
  perception (Dict)
    ↓
  state_encoder.encode(grid)
    ↓
  embedding (StateEmbedding)
    ↓
  topological_hash
    ↓
  causal_graph.add_transition()

Métriques Cibles:
  ✅ grid_shape: [H, W] (non vide)
  ✅ motion > 0.0 (si mouvement)
  ✅ unique_hashes > 10 (après 20 actions)
  ✅ agent_detected: true (>10% actions)

Auteur: Bob (Mode Advanced)
Date: 2026-06-14
Session: 73
"""

import numpy as np
import json
import time
import os
from typing import Dict, Any, List
from datetime import datetime

# Import agent V27.1
from magen_arc_agi_agent_v27_1_causal import MAGENArcAGIAgentV271

# Import ARC-AGI-3
try:
    from arc_agi import Arcade, OperationMode
except ImportError:
    print("⚠️ arc_agi non disponible, mode simulation")
    Arcade = None


class PerceptionPipelineAuditor:
    """Auditeur forensic du pipeline perceptionnel"""
    
    def __init__(self, log_dir: str = "audit_perception_v27_1"):
        self.log_dir = log_dir
        os.makedirs(log_dir, exist_ok=True)
        
        self.audit_log = []
        self.grid_shapes = []
        self.motion_values = []
        self.unique_hashes = set()
        self.agent_detections = []
        
        print(f"[AUDIT] Logs dans: {log_dir}")
    
    def log_step(self, step_name: str, data: Dict[str, Any]):
        """Log une étape avec copie profonde immédiate des ndarrays"""
        
        # CRITIQUE: Copier tous les ndarrays IMMÉDIATEMENT avant stockage
        # pour éviter corruption par buffer partagé ARC-AGI-3
        data_copy = self._deep_copy_data(data)
        
        entry = {
            'timestamp_ns': time.perf_counter_ns(),
            'step': step_name,
            'data': self._serialize(data_copy)  # Sérialiser copie, pas référence
        }
        self.audit_log.append(entry)
        
        # Print résumé (utiliser data original pour performance)
        print(f"\n[AUDIT STEP] {step_name}")
        for key, value in data.items():
            if isinstance(value, np.ndarray):
                print(f"  {key}: shape={value.shape}, dtype={value.dtype}, min={value.min()}, max={value.max()}")
            elif isinstance(value, dict):
                print(f"  {key}: dict with {len(value)} keys")
            elif isinstance(value, list):
                print(f"  {key}: list with {len(value)} items")
            else:
                print(f"  {key}: {value}")
    
    def _deep_copy_data(self, data: Dict[str, Any]) -> Dict[str, Any]:
        """Copie récursive profonde avec gestion ndarrays"""
        result = {}
        for k, v in data.items():
            if isinstance(v, np.ndarray):
                result[k] = v.copy()  # Copie profonde immédiate
            elif isinstance(v, dict):
                result[k] = self._deep_copy_data(v)  # Récursif
            elif isinstance(v, list):
                result[k] = [item.copy() if isinstance(item, np.ndarray) else item
                            for item in v]
            else:
                result[k] = v
        return result
    
    def _serialize(self, obj):
        """Sérialise objet pour JSON (gère numpy)"""
        if isinstance(obj, np.ndarray):
            # CRITIQUE: Copie profonde IMMÉDIATE pour éviter corruption buffer partagé
            obj_copy = obj.copy()
            return {
                'type': 'ndarray',
                'shape': list(obj_copy.shape),
                'dtype': str(obj_copy.dtype),
                'min': float(obj_copy.min()) if obj_copy.size > 0 else None,
                'max': float(obj_copy.max()) if obj_copy.size > 0 else None,
                'mean': float(obj_copy.mean()) if obj_copy.size > 0 else None,
                'sample': obj_copy.flatten()[:10].tolist() if obj_copy.size > 0 else []
            }
        elif isinstance(obj, (np.integer, np.floating)):
            return float(obj)
        elif isinstance(obj, np.bool_):
            return bool(obj)
        elif isinstance(obj, dict):
            return {k: self._serialize(v) for k, v in obj.items()}
        elif isinstance(obj, list):
            return [self._serialize(item) for item in obj]
        elif hasattr(obj, '__dict__'):
            return self._serialize(obj.__dict__)
        else:
            return str(obj)
    
    def audit_arcade_step(self, arcade, action: str, action_idx: int):
        """Audit étape 1: arcade.step()"""
        print(f"\n{'='*80}")
        print(f"ACTION {action_idx}: {action}")
        print(f"{'='*80}")
        
        # Avant step
        try:
            obs_before = arcade.get_observation()
            self.log_step('arcade_obs_before', {
                'type': type(obs_before).__name__,
                'has_grid': hasattr(obs_before, 'grid'),
                'dir': dir(obs_before)[:10]  # Premiers attributs
            })
        except Exception as e:
            self.log_step('arcade_obs_before_error', {'error': str(e)})
        
        # Step
        try:
            obs_after = arcade.step(action)
            
            # Extraire grid depuis structure ARC-AGI-3 réelle
            grid_after = None
            if hasattr(obs_after, 'frame') and obs_after.frame is not None:
                if isinstance(obs_after.frame, list) and len(obs_after.frame) > 0:
                    grid_after = obs_after.frame[0]
            
            self.log_step('arcade_step_result', {
                'type': type(obs_after).__name__,
                'has_frame': hasattr(obs_after, 'frame'),
                'frame_type': type(obs_after.frame).__name__ if hasattr(obs_after, 'frame') else None,
                'grid_extracted': grid_after is not None,
                'grid_shape': list(grid_after.shape) if grid_after is not None else [],
                'dir': dir(obs_after)[:10]
            })
            return obs_after
        except Exception as e:
            self.log_step('arcade_step_error', {'error': str(e)})
            return None
    
    def audit_perceive(self, agent, obs):
        """Audit étape 2: agent.perceive()"""
        try:
            perception = agent.perceive(obs)
            
            # Extraire grid
            grid = perception.get('grid') if isinstance(perception, dict) else getattr(perception, 'grid', None)
            
            # CRITIQUE: Copie profonde du grid pour éviter corruption par réutilisation buffer
            grid_copy = grid.copy() if grid is not None and hasattr(grid, 'copy') else grid
            
            self.log_step('agent_perceive_result', {
                'type': type(perception).__name__,
                'is_dict': isinstance(perception, dict),
                'keys': list(perception.keys()) if isinstance(perception, dict) else None,
                'grid': grid_copy,  # Utiliser copie, pas référence
                'grid_shape': list(grid.shape) if grid is not None and hasattr(grid, 'shape') else None,
                'embedding': perception.get('embedding') if isinstance(perception, dict) else None
            })
            
            # Track grid shape
            if grid is not None and hasattr(grid, 'shape'):
                self.grid_shapes.append(list(grid.shape))
            else:
                self.grid_shapes.append([])
            
            return perception
        except Exception as e:
            self.log_step('agent_perceive_error', {'error': str(e), 'traceback': str(e.__traceback__)})
            self.grid_shapes.append([])
            return None
    
    def audit_state_encoder(self, agent, grid):
        """Audit étape 3: state_encoder.encode()"""
        if grid is None:
            self.log_step('state_encoder_skip', {'reason': 'grid is None'})
            return None
        
        try:
            embedding = agent.state_encoder.encode(grid)
            
            self.log_step('state_encoder_result', {
                'vector_shape': list(embedding.vector.shape) if hasattr(embedding, 'vector') else None,
                'topological_hash': embedding.topological_hash if hasattr(embedding, 'topological_hash') else None,
                'motion_score': embedding.motion_score if hasattr(embedding, 'motion_score') else None,
                'spatial_relations': len(embedding.spatial_relations) if hasattr(embedding, 'spatial_relations') else None
            })
            
            # Track motion
            if hasattr(embedding, 'motion_score'):
                self.motion_values.append(float(embedding.motion_score))
            else:
                self.motion_values.append(0.0)
            
            # Track hash
            if hasattr(embedding, 'topological_hash'):
                self.unique_hashes.add(embedding.topological_hash)
            
            return embedding
        except Exception as e:
            self.log_step('state_encoder_error', {'error': str(e)})
            self.motion_values.append(0.0)
            return None
    
    def audit_entity_detector(self, agent, perception):
        """Audit étape 4: behavioral_entity_detector.observe()"""
        try:
            # Observer
            agent.entity_detector.observe(perception)
            
            # Récupérer world model
            world_model = agent.entity_detector.get_world_model()
            
            self.log_step('entity_detector_result', {
                'agent_detected': world_model.get('agent_detected_causal', False),
                'confidence': world_model.get('confidence', 0.0),
                'mobile_entities': world_model.get('mobile_entities', 0),
                'stable_entities': world_model.get('stable_entities', 0)
            })
            
            # Track detection
            self.agent_detections.append(world_model.get('agent_detected_causal', False))
            
        except Exception as e:
            self.log_step('entity_detector_error', {'error': str(e)})
            self.agent_detections.append(False)
    def audit_env_step(self, env, action: str, action_idx: int) -> Dict:
        """
        Audit complet env.step() avec extraction forensic.
        
        Args:
            env: Environment ARC-AGI
            action: Action à exécuter
            action_idx: Index action
        
        Returns:
            obs_after: Observation après step
        """
        print(f"\n[AUDIT env.step() - Action {action_idx}]")
        
        timestamp_before = int(time.time_ns())
        
        try:
            # Convertir action string → GameAction
            from arcengine import GameAction
            if hasattr(GameAction, action):
                action_to_execute = getattr(GameAction, action)
            else:
                action_to_execute = action
            
            # Exécuter step
            obs_after = env.step(action_to_execute)
            
            timestamp_after = int(time.time_ns())
            duration_ns = timestamp_after - timestamp_before
            
            # Extraire données observation depuis structure ARC-AGI-3 réelle
            # Structure: obs.frame = List[np.ndarray], frame[0] = grid actuel
            grid_after = None
            if hasattr(obs_after, 'frame') and obs_after.frame is not None:
                if isinstance(obs_after.frame, list) and len(obs_after.frame) > 0:
                    grid_after = obs_after.frame[0]
            
            state_after = obs_after.state if hasattr(obs_after, 'state') else 'UNKNOWN'
            
            # Log forensic
            self.log_step('env_step_result', {
                'action': action,
                'action_idx': action_idx,
                'duration_ns': duration_ns,
                'grid_shape': list(grid_after.shape) if grid_after is not None else [],
                'grid_dtype': str(grid_after.dtype) if grid_after is not None else 'None',
                'state': state_after,
                'timestamp_before': timestamp_before,
                'timestamp_after': timestamp_after
            })
            
            # Vérifications intégrité
            if grid_after is None:
                print(f"  ⚠️ Grid None après step")
            elif len(grid_after.shape) < 2:
                print(f"  ⚠️ Grid dimensions invalides: {grid_after.shape}")
            else:
                print(f"  ✓ Grid shape: {grid_after.shape}")
                print(f"  ✓ State: {state_after}")
                print(f"  ✓ Duration: {duration_ns/1e6:.2f}ms")
            
            return obs_after
            
        except Exception as e:
            self.log_step('env_step_error', {
                'action': action,
                'action_idx': action_idx,
                'error': str(e),
                'error_type': type(e).__name__
            })
            print(f"  ❌ Erreur env.step(): {e}")
            return None
    
    
    def generate_report(self, action_count: int):
        """Génère rapport d'audit complet"""
        report = {
            'timestamp': datetime.now().isoformat(),
            'action_count': action_count,
            'metrics': {
                'grid_shapes': {
                    'values': self.grid_shapes,
                    'empty_count': sum(1 for s in self.grid_shapes if not s),
                    'valid_count': sum(1 for s in self.grid_shapes if s),
                    'unique_shapes': list(set(tuple(s) for s in self.grid_shapes if s))
                },
                'motion': {
                    'values': self.motion_values,
                    'mean': np.mean(self.motion_values) if self.motion_values else 0.0,
                    'max': np.max(self.motion_values) if self.motion_values else 0.0,
                    'non_zero_count': sum(1 for m in self.motion_values if m > 0.0)
                },
                'hashes': {
                    'unique_count': len(self.unique_hashes),
                    'hashes': list(self.unique_hashes)
                },
                'detections': {
                    'values': self.agent_detections,
                    'detected_count': sum(self.agent_detections),
                    'detection_rate': sum(self.agent_detections) / len(self.agent_detections) if self.agent_detections else 0.0
                }
            },
            'audit_log': self.audit_log
        }
        
        # Sauvegarder
        report_path = os.path.join(self.log_dir, f'audit_report_{datetime.now().strftime("%Y%m%d_%H%M%S")}.json')
        with open(report_path, 'w') as f:
            json.dump(report, f, indent=2)
        
        print(f"\n{'='*80}")
        print("RAPPORT D'AUDIT FINAL")
        print(f"{'='*80}")
        print(f"\n📊 MÉTRIQUES GLOBALES ({action_count} actions)")
        print(f"\n1. GRID SHAPES:")
        print(f"   - Valides: {report['metrics']['grid_shapes']['valid_count']}/{action_count}")
        print(f"   - Vides: {report['metrics']['grid_shapes']['empty_count']}/{action_count}")
        print(f"   - Shapes uniques: {report['metrics']['grid_shapes']['unique_shapes']}")
        
        print(f"\n2. MOTION:")
        print(f"   - Moyenne: {report['metrics']['motion']['mean']:.4f}")
        print(f"   - Max: {report['metrics']['motion']['max']:.4f}")
        print(f"   - Non-zéro: {report['metrics']['motion']['non_zero_count']}/{action_count}")
        
        print(f"\n3. HASHES:")
        print(f"   - Uniques: {report['metrics']['hashes']['unique_count']}")
        print(f"   - Cible: >10 après 20 actions")
        print(f"   - Status: {'✅ OK' if report['metrics']['hashes']['unique_count'] > 10 else '❌ ÉCHEC'}")
        
        print(f"\n4. DÉTECTIONS AGENT:")
        print(f"   - Détectées: {report['metrics']['detections']['detected_count']}/{action_count}")
        print(f"   - Taux: {report['metrics']['detections']['detection_rate']*100:.1f}%")
        print(f"   - Cible: >10%")
        print(f"   - Status: {'✅ OK' if report['metrics']['detections']['detection_rate'] > 0.1 else '❌ ÉCHEC'}")
        
        print(f"\n📄 Rapport sauvegardé: {report_path}")
        
        return report


def main():
    """Audit complet pipeline perception sur 1 jeu"""
    print("="*80)
    print("AUDIT PIPELINE PERCEPTIONNEL V27.1 - SESSION 73")
    print("="*80)
    
    # Charger API key (variable environnement ou fichier)
    api_key = os.getenv("ARC_API_KEY")
    
    if not api_key:
        api_key_path = os.path.expanduser("~/.arc_agi_api_key")
        if os.path.exists(api_key_path):
            with open(api_key_path) as f:
                api_key = f.read().strip()
    
    if not api_key:
        print("❌ API key non trouvée (ni ARC_API_KEY env var, ni ~/.arc_agi_api_key)")
        print("💡 Utilisation mode anonyme (clé temporaire)")
        api_key = ""  # Arcade générera clé anonyme
    else:
        print("✅ API key chargée")
    
    # Initialiser agent
    print("\n[INITIALISATION AGENT V27.1]")
    agent = MAGENArcAGIAgentV271(forensic_log_path="logs_forensic_audit")
    print("✅ Agent initialisé")
    
    # Initialiser auditor
    auditor = PerceptionPipelineAuditor()
    
    # Connexion ARC-AGI-3
    print("\n[CONNEXION ARC-AGI-3]")
    arcade = Arcade(arc_api_key=api_key, operation_mode=OperationMode.NORMAL)
    print("✅ Arcade connecté")
    
    # Sélectionner jeu
    game_id = "ar25"
    print(f"\n[JEU: {game_id}]")
    arcade.create_scorecard(game_id)
    
    # Réinitialiser agent
    agent.reset()
    
    # Exécuter 20 actions avec audit complet
    max_actions = 20
    
    # Observation initiale via env.reset()
    env = arcade.make(game_id)
    obs = env.reset()
    
    for action_idx in range(1, max_actions + 1):
        # Available actions (LIMITÉ À ACTION1-7 VALIDES)
        from arcengine import GameAction
        available_actions = [f"ACTION{i}" for i in range(1, 8)]  # 7 actions valides
        
        # Perceive état AVANT action (pour decide)
        perception_before = auditor.audit_perceive(agent, obs)
        
        if perception_before is None:
            print(f"⚠️ Perception échouée, skip action {action_idx}")
            continue
        
        # Decide basé sur état AVANT
        decision = agent.decide(perception_before, available_actions)
        action_name = decision.get('action_name') if isinstance(decision, dict) else decision.action_name
        
        if not action_name:
            print(f"⚠️ Action invalide, skip action {action_idx}")
            continue
        
        # Audit env.step()
        obs_after = auditor.audit_env_step(env, action_name, action_idx)
        
        if obs_after is None:
            print(f"⚠️ env.step() échoué, skip action {action_idx}")
            continue
        
        # CRITIQUE: Perceive APRÈS action pour audit
        perception_after = auditor.audit_perceive(agent, obs_after)
        
        if perception_after is None:
            print(f"⚠️ Perception après action échouée, skip")
            continue
        
        # Audit state encoder avec grid APRÈS action
        grid_after = perception_after.get('grid') if isinstance(perception_after, dict) else getattr(perception_after, 'grid', None)
        embedding = auditor.audit_state_encoder(agent, grid_after)
        
        # Audit entity detector avec perception APRÈS
        auditor.audit_entity_detector(agent, perception_after)
        
        # Learn
        reward = 0.0
        done = False
        agent.learn(perception_before, action_name, perception_after, reward, done)
        
        # Update obs pour prochaine itération
        obs = obs_after
    
    # Générer rapport
    auditor.generate_report(max_actions)
    
    print("\n✅ AUDIT COMPLET TERMINÉ")


if __name__ == "__main__":
    main()

# Made with Bob
