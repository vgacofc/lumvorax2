#!/usr/bin/env python3
"""
DIAGNOSTIC COMPRÉHENSION DU MONDE - SESSION 79
==============================================

Mesure ce qui MANQUE dans les logs forensiques actuels:
- Carte du monde (positions découvertes)
- Position agent (tracking continu)
- Mémoire salles visitées (graphe navigation)
- Identification avatar (pixel unique)
- Reconstruction niveau (état complet)
- Causalité action→conséquence (transitions)

Ce diagnostic répond à la critique fondamentale:
"Analyser le code ≠ Comprendre le jeu"

Auteur: Bob (Session 79 - World Understanding)
Date: 2026-06-17
"""

import os
import sys
import json
import numpy as np
from pathlib import Path
from datetime import datetime
from collections import defaultdict
from typing import Dict, List, Tuple, Set, Any

# Configuration API
ARC_API_KEY = os.getenv("ARC_API_KEY")
if not ARC_API_KEY:
    raise ValueError("❌ ARC_API_KEY non configurée!")

from arc_agi import Arcade, OperationMode
from arcengine import GameAction

# Configuration
TARGET_GAME = "ls20-9607627b"
MAGEN_ROOT = Path(__file__).parent
RESULTS_DIR = MAGEN_ROOT / "results" / "world_understanding"
RESULTS_DIR.mkdir(parents=True, exist_ok=True)

MAX_STEPS = 150  # Steps pour diagnostic

print(f"""
{'='*80}
🌍 DIAGNOSTIC COMPRÉHENSION DU MONDE
{'='*80}
Jeu: {TARGET_GAME}
Steps: {MAX_STEPS}
Objectif: Mesurer ce qui MANQUE dans les logs actuels
{'='*80}
""")


class WorldUnderstandingDiagnostic:
    """Diagnostic de la compréhension spatiale et causale du monde"""
    
    def __init__(self):
        # 1. CARTE DU MONDE
        self.discovered_positions: Set[Tuple[int, int]] = set()
        self.position_history: List[Tuple[int, int, int]] = []  # (x, y, step)
        
        # 2. POSITION AGENT
        self.agent_positions: List[Tuple[int, int]] = []
        self.agent_pixel_value: int = None
        self.agent_identified: bool = False
        
        # 3. MÉMOIRE SALLES VISITÉES
        self.visited_rooms: Set[int] = set()  # room_id
        self.room_transitions: List[Tuple[int, int, int]] = []  # (from_room, to_room, step)
        self.navigation_graph: Dict[int, Set[int]] = defaultdict(set)
        
        # 4. IDENTIFICATION AVATAR
        self.avatar_candidates: Dict[int, int] = defaultdict(int)  # pixel_value -> count
        self.avatar_confirmed: bool = False
        
        # 5. RECONSTRUCTION NIVEAU
        self.level_states: List[np.ndarray] = []
        self.level_changes: List[Dict] = []
        
        # 6. CAUSALITÉ ACTION→CONSÉQUENCE
        self.action_consequences: List[Dict] = []
        self.causal_patterns: Dict[str, List[Dict]] = defaultdict(list)
        
        # Métriques
        self.metrics = {
            'spatial_coverage': 0.0,  # % monde découvert
            'agent_tracking': 0.0,  # % steps avec position agent
            'room_memory': 0,  # nombre salles mémorisées
            'avatar_confidence': 0.0,  # confiance identification
            'level_reconstruction': 0.0,  # % niveau reconstruit
            'causal_understanding': 0.0  # % actions avec conséquence connue
        }
    
    def extract_agent_position(self, frame) -> Tuple[int, int]:
        """Extrait position agent depuis frame"""
        # Convertir en numpy si nécessaire
        if isinstance(frame, list):
            frame = np.array(frame)
        if len(frame.shape) == 2:
            frame = frame[np.newaxis, :, :]
        
        # Cherche pixel unique (agent)
        unique_pixels = {}
        for y in range(frame.shape[1]):
            for x in range(frame.shape[2]):
                pixel = int(frame[0, y, x])
                if pixel not in unique_pixels:
                    unique_pixels[pixel] = []
                unique_pixels[pixel].append((x, y))
        
        # Trouve pixel avec 1 seule occurrence (candidat agent)
        for pixel_value, positions in unique_pixels.items():
            if len(positions) == 1:
                self.avatar_candidates[pixel_value] += 1
                if self.avatar_candidates[pixel_value] > 5:
                    self.agent_pixel_value = pixel_value
                    self.agent_identified = True
                    self.avatar_confirmed = True
                return positions[0]
        
        # Fallback: centre de masse pixels non-noirs
        non_zero = np.argwhere(frame[0] > 0)
        if len(non_zero) > 0:
            center_y, center_x = non_zero.mean(axis=0).astype(int)
            return (int(center_x), int(center_y))
        
        return (32, 32)  # Centre par défaut
    
    def identify_room(self, frame) -> int:
        """Identifie salle actuelle par hash du frame"""
        # Convertir en numpy si nécessaire
        if isinstance(frame, list):
            frame = np.array(frame)
        if len(frame.shape) == 2:
            frame = frame[np.newaxis, :, :]
        
        # Hash simple: somme pixels par région
        h, w = frame.shape[1], frame.shape[2]
        regions = []
        for i in range(4):
            for j in range(4):
                region = frame[0, i*h//4:(i+1)*h//4, j*w//4:(j+1)*w//4]
                regions.append(int(region.sum()))
        return hash(tuple(regions)) % 10000
    
    def detect_frame_changes(self, frame1, frame2) -> Dict:
        """Détecte changements entre 2 frames"""
        # Convertir en numpy si nécessaire
        if isinstance(frame1, list):
            frame1 = np.array(frame1)
        if isinstance(frame2, list):
            frame2 = np.array(frame2)
        if len(frame1.shape) == 2:
            frame1 = frame1[np.newaxis, :, :]
        if len(frame2.shape) == 2:
            frame2 = frame2[np.newaxis, :, :]
        
        diff = np.abs(frame1.astype(int) - frame2.astype(int))
        changes = {
            'total_pixels_changed': int((diff > 0).sum()),
            'max_change': int(diff.max()),
            'mean_change': float(diff.mean()),
            'changed_positions': []
        }
        
        # Positions changées
        changed = np.argwhere(diff[0] > 0)
        if len(changed) > 0 and len(changed) < 100:  # Limite pour performance
            changes['changed_positions'] = [(int(y), int(x)) for y, x in changed]
        
        return changes
    
    def analyze_action_consequence(self, action: int, frame_before,
                                   frame_after, step: int):
        """Analyse conséquence d'une action"""
        changes = self.detect_frame_changes(frame_before, frame_after)
        
        consequence = {
            'step': step,
            'action': action,
            'changes': changes,
            'causal_link': changes['total_pixels_changed'] > 0
        }
        
        self.action_consequences.append(consequence)
        
        # Pattern causal
        action_name = ['UP', 'DOWN', 'LEFT', 'RIGHT'][action - 1]
        self.causal_patterns[action_name].append(consequence)
    
    def update_navigation_graph(self, from_room: int, to_room: int, step: int):
        """Met à jour graphe de navigation"""
        if from_room != to_room:
            self.navigation_graph[from_room].add(to_room)
            self.room_transitions.append((from_room, to_room, step))
    
    def calculate_metrics(self, total_steps: int):
        """Calcule métriques finales"""
        # 1. Couverture spatiale
        if len(self.discovered_positions) > 0:
            self.metrics['spatial_coverage'] = len(self.discovered_positions) / (64 * 64)
        
        # 2. Tracking agent
        if total_steps > 0:
            self.metrics['agent_tracking'] = len(self.agent_positions) / total_steps
        
        # 3. Mémoire salles
        self.metrics['room_memory'] = len(self.visited_rooms)
        
        # 4. Confiance avatar
        if self.avatar_confirmed:
            max_count = max(self.avatar_candidates.values()) if self.avatar_candidates else 0
            self.metrics['avatar_confidence'] = min(1.0, max_count / 10.0)
        
        # 5. Reconstruction niveau
        if len(self.level_states) > 0:
            self.metrics['level_reconstruction'] = min(1.0, len(self.level_states) / 50.0)
        
        # 6. Compréhension causale
        if len(self.action_consequences) > 0:
            causal_actions = sum(1 for c in self.action_consequences if c['causal_link'])
            self.metrics['causal_understanding'] = causal_actions / len(self.action_consequences)
    
    def generate_report(self) -> Dict:
        """Génère rapport complet"""
        return {
            'timestamp': datetime.now().isoformat(),
            'game': TARGET_GAME,
            
            # Métriques principales
            'metrics': self.metrics,
            
            # Détails carte monde
            'world_map': {
                'positions_discovered': len(self.discovered_positions),
                'total_positions': 64 * 64,
                'coverage_percent': self.metrics['spatial_coverage'] * 100
            },
            
            # Détails agent
            'agent': {
                'identified': self.agent_identified,
                'pixel_value': self.agent_pixel_value,
                'positions_tracked': len(self.agent_positions),
                'confidence': self.metrics['avatar_confidence']
            },
            
            # Détails navigation
            'navigation': {
                'rooms_visited': len(self.visited_rooms),
                'transitions': len(self.room_transitions),
                'graph_nodes': len(self.navigation_graph),
                'graph_edges': sum(len(edges) for edges in self.navigation_graph.values())
            },
            
            # Détails reconstruction
            'reconstruction': {
                'states_captured': len(self.level_states),
                'changes_detected': len(self.level_changes)
            },
            
            # Détails causalité
            'causality': {
                'actions_analyzed': len(self.action_consequences),
                'causal_links_found': sum(1 for c in self.action_consequences if c['causal_link']),
                'patterns_by_action': {k: len(v) for k, v in self.causal_patterns.items()}
            }
        }


def run_diagnostic():
    """Exécute diagnostic complet"""
    
    print("🔧 Initialisation environnement...")
    arcade = Arcade(arc_api_key=ARC_API_KEY, operation_mode=OperationMode.COMPETITION)
    env = arcade.make(TARGET_GAME)
    
    diagnostic = WorldUnderstandingDiagnostic()
    
    print("🎮 Démarrage jeu...")
    obs = env.reset()
    
    prev_frame = obs.frame.copy()
    prev_room = diagnostic.identify_room(obs.frame)
    diagnostic.visited_rooms.add(prev_room)
    
    print(f"\n{'='*80}")
    print("📊 COLLECTE DONNÉES WORLD UNDERSTANDING")
    print(f"{'='*80}\n")
    
    for step in range(MAX_STEPS):
        # Extraction position agent
        agent_pos = diagnostic.extract_agent_position(obs.frame)
        diagnostic.agent_positions.append(agent_pos)
        diagnostic.discovered_positions.add(agent_pos)
        diagnostic.position_history.append((agent_pos[0], agent_pos[1], step))
        
        # Identification salle
        current_room = diagnostic.identify_room(obs.frame)
        if current_room not in diagnostic.visited_rooms:
            diagnostic.visited_rooms.add(current_room)
            diagnostic.update_navigation_graph(prev_room, current_room, step)
            prev_room = current_room
        
        # Sauvegarde état niveau
        diagnostic.level_states.append(obs.frame.copy())
        
        # Action aléatoire (utiliser GameAction enum comme dans learn_loop_until_victory.py)
        actions_pool = [GameAction.ACTION1, GameAction.ACTION2, GameAction.ACTION3, GameAction.ACTION4]
        action = np.random.choice(actions_pool)
        
        # Observation avant action
        frame_before = obs.frame.copy() if isinstance(obs.frame, np.ndarray) else np.array(obs.frame).copy()
        
        # Exécution action (passer GameAction enum)
        obs = env.step(action)
        
        # Analyse conséquence (utiliser action.value pour l'int)
        diagnostic.analyze_action_consequence(action.value, frame_before, obs.frame, step)
        
        # Détection changements
        changes = diagnostic.detect_frame_changes(prev_frame, obs.frame)
        if changes['total_pixels_changed'] > 0:
            diagnostic.level_changes.append({
                'step': step,
                'action': action.name,
                'changes': changes
            })
        
        prev_frame = obs.frame.copy() if isinstance(obs.frame, np.ndarray) else np.array(obs.frame).copy()
        
        # Affichage progression
        if (step + 1) % 25 == 0:
            print(f"Step {step+1}/{MAX_STEPS} | "
                  f"Positions: {len(diagnostic.discovered_positions)} | "
                  f"Salles: {len(diagnostic.visited_rooms)} | "
                  f"Agent: {'✓' if diagnostic.agent_identified else '✗'}")
        
        # Arrêt si GAME_OVER
        if obs.state.name == "GAME_OVER":
            print(f"\n⚠️  GAME_OVER détecté au step {step+1}")
            break
    
    # Calcul métriques finales
    diagnostic.calculate_metrics(step + 1)
    
    # Génération rapport
    report = diagnostic.generate_report()
    
    # Sauvegarde
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    report_file = RESULTS_DIR / f"world_understanding_{timestamp}.json"
    
    with open(report_file, 'w') as f:
        json.dump(report, f, indent=2)
    
    print(f"\n{'='*80}")
    print("📊 RÉSULTATS DIAGNOSTIC WORLD UNDERSTANDING")
    print(f"{'='*80}\n")
    
    print("🌍 CARTE DU MONDE:")
    print(f"  Positions découvertes: {report['world_map']['positions_discovered']}/{report['world_map']['total_positions']}")
    print(f"  Couverture spatiale: {report['world_map']['coverage_percent']:.2f}%")
    
    print("\n🤖 AGENT:")
    print(f"  Identifié: {'✓' if report['agent']['identified'] else '✗'}")
    print(f"  Pixel value: {report['agent']['pixel_value']}")
    print(f"  Positions trackées: {report['agent']['positions_tracked']}")
    print(f"  Confiance: {report['agent']['confidence']:.2%}")
    
    print("\n🗺️  NAVIGATION:")
    print(f"  Salles visitées: {report['navigation']['rooms_visited']}")
    print(f"  Transitions: {report['navigation']['transitions']}")
    print(f"  Graphe: {report['navigation']['graph_nodes']} nœuds, {report['navigation']['graph_edges']} arêtes")
    
    print("\n🔨 RECONSTRUCTION:")
    print(f"  États capturés: {report['reconstruction']['states_captured']}")
    print(f"  Changements détectés: {report['reconstruction']['changes_detected']}")
    
    print("\n⚡ CAUSALITÉ:")
    print(f"  Actions analysées: {report['causality']['actions_analyzed']}")
    print(f"  Liens causaux trouvés: {report['causality']['causal_links_found']}")
    print(f"  Patterns par action: {report['causality']['patterns_by_action']}")
    
    print(f"\n{'='*80}")
    print("📈 MÉTRIQUES GLOBALES")
    print(f"{'='*80}\n")
    
    for metric_name, metric_value in report['metrics'].items():
        if isinstance(metric_value, float):
            print(f"  {metric_name}: {metric_value:.2%}")
        else:
            print(f"  {metric_name}: {metric_value}")
    
    print(f"\n✅ Rapport sauvegardé: {report_file}")
    
    return report


if __name__ == "__main__":
    try:
        report = run_diagnostic()
        sys.exit(0)
    except Exception as e:
        print(f"\n❌ ERREUR: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

# Made with Bob
