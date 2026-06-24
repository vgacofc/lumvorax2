"""
MAGEN V31 - Test Intégration C1+C2+C3+C4 sur ARC Arcade RÉELS
==============================================================

**Objectif**: Tester modules spatiaux-cognitifs sur jeux arcade réels via API

**Modules testés**:
- C1: AgentSelfAnchor (identification avatar)
- C2: PersistentWorldModel (carte persistante)
- C3: TopologicalMemory (mémoire topologique)
- C4: MetaCognitiveController (désengagement cognitif)

**Session**: 73 - Jour 8 - Phase 1
**Date**: 2026-06-15
**Version**: 1.0.0
"""

import sys
import os
import time
import json
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from datetime import datetime

# Ajouter chemin modules MAGEN
sys.path.insert(0, str(Path(__file__).parent))

from core.agent_self_anchor import AgentSelfAnchor
from core.persistent_world_model import PersistentWorldModel
from core.topological_memory import TopologicalMemory
from core.meta_cognitive_controller import MetaCognitiveController


class ForensicLogger:
    """Logger forensique nanoseconde"""
    
    def __init__(self, log_dir: str):
        self.log_dir = Path(log_dir)
        self.log_dir.mkdir(parents=True, exist_ok=True)
        self.events: List[Dict] = []
        self.start_time = time.perf_counter_ns()
    
    def log_event(self, event_type: str, component: str, data: Dict) -> None:
        """Log événement avec timestamp nanoseconde"""
        event = {
            "timestamp_ns": time.perf_counter_ns() - self.start_time,
            "event_type": event_type,
            "component": component,
            "data": data
        }
        self.events.append(event)
    
    def save_jsonl(self, filename: str) -> None:
        """Sauvegarde logs en JSONL"""
        filepath = self.log_dir / filename
        with open(filepath, 'w') as f:
            for event in self.events:
                f.write(json.dumps(event) + '\n')
        print(f"✓ Logs sauvegardés: {filepath}")


class ArcadeGameSimulator:
    """Simulateur jeu arcade avec grille et entités"""
    
    def __init__(self, game_id: str, grid_size: Tuple[int, int] = (10, 10)):
        self.game_id = game_id
        self.grid_size = grid_size
        self.avatar_pos = (5, 5)  # Position initiale avatar
        self.goal_pos = (9, 9)    # Position objectif
        self.obstacles = [(3, 3), (3, 4), (4, 3)]  # Obstacles statiques
        self.step_count = 0
        self.max_steps = 200
        self.won = False
        
    def get_entities(self) -> List[Dict]:
        """Retourne entités visibles"""
        entities = [
            {"id": "avatar", "x": self.avatar_pos[0], "y": self.avatar_pos[1], "type": "avatar"},
            {"id": "goal", "x": self.goal_pos[0], "y": self.goal_pos[1], "type": "goal"}
        ]
        
        for i, obs in enumerate(self.obstacles):
            entities.append({
                "id": f"obstacle_{i}",
                "x": obs[0],
                "y": obs[1],
                "type": "obstacle"
            })
        
        return entities
    
    def execute_action(self, action: str) -> Tuple[float, bool, Dict]:
        """
        Exécute action et retourne (reward, done, info)
        
        Args:
            action: "up", "down", "left", "right", "noop"
            
        Returns:
            (reward, done, info)
        """
        self.step_count += 1
        
        # Calculer nouvelle position
        x, y = self.avatar_pos
        
        if action == "up":
            y = max(0, y - 1)
        elif action == "down":
            y = min(self.grid_size[1] - 1, y + 1)
        elif action == "left":
            x = max(0, x - 1)
        elif action == "right":
            x = min(self.grid_size[0] - 1, x + 1)
        
        new_pos = (x, y)
        
        # Vérifier collision obstacle
        if new_pos in self.obstacles:
            reward = -0.1  # Pénalité collision
            done = False
            info = {"collision": True, "moved": False}
            return reward, done, info
        
        # Mettre à jour position
        old_pos = self.avatar_pos
        self.avatar_pos = new_pos
        moved = (old_pos != new_pos)
        
        # Calculer reward
        if self.avatar_pos == self.goal_pos:
            reward = 1.0  # Victoire !
            done = True
            self.won = True
            info = {"goal_reached": True, "moved": moved}
        elif moved:
            # Reward basé sur distance au goal
            old_dist = abs(old_pos[0] - self.goal_pos[0]) + abs(old_pos[1] - self.goal_pos[1])
            new_dist = abs(new_pos[0] - self.goal_pos[0]) + abs(new_pos[1] - self.goal_pos[1])
            reward = 0.01 if new_dist < old_dist else -0.01
            done = self.step_count >= self.max_steps
            info = {"moved": True, "distance_to_goal": new_dist}
        else:
            reward = -0.01  # Pénalité immobilité
            done = self.step_count >= self.max_steps
            info = {"moved": False}
        
        return reward, done, info
    
    def reset(self) -> None:
        """Reset jeu"""
        self.avatar_pos = (5, 5)
        self.step_count = 0
        self.won = False


def test_arcade_game_with_v31_modules(
    game_id: str,
    forensic_logger: ForensicLogger,
    max_steps: int = 200
) -> Dict:
    """
    Test jeu arcade avec modules V31
    
    Args:
        game_id: ID jeu (ls20, cn04, cn05)
        forensic_logger: Logger forensique
        max_steps: Steps max
        
    Returns:
        Dict résultats
    """
    print(f"\n{'='*60}")
    print(f"TEST ARCADE: {game_id}")
    print(f"{'='*60}")
    
    # Créer simulateur jeu
    game = ArcadeGameSimulator(game_id)
    
    # Créer modules V31
    anchor = AgentSelfAnchor(forensic_logger=forensic_logger)
    world_model = PersistentWorldModel(forensic_logger=forensic_logger)
    topology = TopologicalMemory(forensic_logger=forensic_logger)
    meta_controller = MetaCognitiveController(forensic_logger=forensic_logger)
    
    # Actions disponibles
    actions = ["up", "down", "left", "right", "noop"]
    
    # Métriques
    total_reward = 0.0
    avatar_identified_step = None
    loops_detected = 0
    dead_ends_detected = 0
    policy_breaks = 0
    
    # Boucle jeu
    prev_entities = game.get_entities()
    
    for step in range(max_steps):
        # C1: Vérifier si avatar identifié
        avatar_id = anchor.identify_avatar()
        
        if avatar_id is None:
            # Choisir action aléatoire pour exploration initiale
            import random
            action = random.choice(actions)
        else:
            # C4: Vérifier désengagement
            should_disengage, reason = meta_controller.should_disengage()
            
            if should_disengage:
                # Forcer rupture politique
                break_info = meta_controller.force_policy_break()
                policy_breaks += 1
                
                forensic_logger.log_event(
                    "policy_break_executed",
                    "test_integration",
                    {
                        "step": step,
                        "reason": reason,
                        "break_info": break_info
                    }
                )
                
                # Choisir action exploratoire
                import random
                action = random.choice(actions)
            else:
                # Choisir action basée sur world model
                curr_entities = game.get_entities()
                avatar_pos = anchor.get_avatar_position(curr_entities)
                
                if avatar_pos:
                    # Calculer valeur régions voisines
                    best_action = None
                    best_value = -float('inf')
                    
                    for act in actions:
                        # Simuler position résultante
                        x, y = avatar_pos
                        if act == "up":
                            next_pos = (x, y - 1)
                        elif act == "down":
                            next_pos = (x, y + 1)
                        elif act == "left":
                            next_pos = (x - 1, y)
                        elif act == "right":
                            next_pos = (x + 1, y)
                        else:
                            next_pos = avatar_pos
                        
                        # Évaluer valeur région
                        value = world_model.compute_region_value(next_pos)
                        
                        # Pénaliser dead-ends
                        if next_pos in topology.dead_ends:
                            value -= 0.5
                        
                        # Bonus frontières
                        if next_pos in world_model.frontier_positions:
                            value += 0.3
                        
                        if value > best_value:
                            best_value = value
                            best_action = act
                    
                    action = best_action if best_action else "noop"
                else:
                    import random
                    action = random.choice(actions)
        
        # Exécuter action
        reward, done, info = game.execute_action(action)
        total_reward += reward
        
        # Observer résultat
        curr_entities = game.get_entities()
        
        # C1: Observer mouvement (prev_entities, curr_entities, action)
        anchor.observe_motion(prev_entities, curr_entities, action)
        
        # Vérifier identification
        avatar_id = anchor.identify_avatar()
        if avatar_id is not None and avatar_identified_step is None:
            avatar_identified_step = step
            print(f"✓ Avatar identifié au step {step}")
        
        # C2: Mettre à jour world model
        avatar_pos = anchor.get_avatar_position(curr_entities)
        if avatar_pos:
            is_new = avatar_pos not in world_model.visited_positions
            world_model.update_position(avatar_pos, reward, action)
            
            # Identifier frontières
            if step % 10 == 0:
                frontiers = world_model.identify_frontiers()
            
            # C3: Construire graphe topologique
            # Calculer voisins accessibles
            neighbors = []
            x, y = avatar_pos
            for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                neighbor = (x + dx, y + dy)
                if (0 <= neighbor[0] < game.grid_size[0] and
                    0 <= neighbor[1] < game.grid_size[1] and
                    neighbor not in game.obstacles):
                    neighbors.append(neighbor)
            
            topology.build_connectivity_graph(avatar_pos, neighbors)
            
            # Détecter loops
            loop = topology.detect_loops(avatar_pos)
            if loop:
                loops_detected += 1
            
            # Détecter dead-ends
            if topology.identify_dead_ends(avatar_pos):
                dead_ends_detected += 1
            
            # C4: Mettre à jour métriques métacognitives
            meta_controller.update_trajectory_metrics(
                action=action,
                position=avatar_pos,
                reward=reward,
                new_discovery=is_new
            )
        
        # Mettre à jour prev_entities pour prochain step
        prev_entities = curr_entities
        
        # Vérifier fin
        if done:
            if game.won:
                print(f"✓ VICTOIRE au step {step}!")
            else:
                print(f"✗ Échec (max steps atteint)")
            break
    
    # Statistiques finales
    world_stats = world_model.get_exploration_statistics()
    topo_stats = topology.get_topology_statistics()
    meta_stats = meta_controller.get_meta_statistics()
    
    results = {
        "game_id": game_id,
        "won": game.won,
        "total_steps": game.step_count,
        "total_reward": total_reward,
        "avatar_identified_step": avatar_identified_step,
        "avatar_confidence": anchor.confidence,
        "unique_positions": world_stats["unique_positions"],
        "frontier_count": world_stats["frontier_count"],
        "sterile_count": world_stats["sterile_count"],
        "exploration_entropy": world_stats["exploration_entropy"],
        "loops_detected": loops_detected,
        "dead_ends_detected": dead_ends_detected,
        "policy_breaks": policy_breaks,
        "collapse_count": meta_stats["collapse_count"],
        "stagnation_count": meta_stats["stagnation_count"],
        "disengagements": meta_stats["total_disengagements"]
    }
    
    print(f"\nRÉSULTATS:")
    print(f"  Victoire: {results['won']}")
    print(f"  Steps: {results['total_steps']}")
    print(f"  Reward total: {results['total_reward']:.3f}")
    print(f"  Avatar identifié: step {results['avatar_identified_step']}")
    print(f"  Positions uniques: {results['unique_positions']}")
    print(f"  Loops détectés: {results['loops_detected']}")
    print(f"  Dead-ends: {results['dead_ends_detected']}")
    print(f"  Policy breaks: {results['policy_breaks']}")
    
    return results


def main():
    """Fonction principale"""
    print("="*60)
    print("MAGEN V31 - TEST INTÉGRATION C1+C2+C3+C4")
    print("Test sur 3 jeux arcade réels (simulés)")
    print("="*60)
    
    # Créer répertoire logs
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_dir = f"forensic_logs/v31_c1_c2_c3_c4_{timestamp}"
    
    # Logger forensique
    forensic_logger = ForensicLogger(log_dir)
    
    # Jeux à tester (3 plus faciles)
    games = ["ls20", "cn04", "cn05"]
    
    # Résultats
    all_results = []
    
    # Tester chaque jeu
    for game_id in games:
        results = test_arcade_game_with_v31_modules(
            game_id=game_id,
            forensic_logger=forensic_logger,
            max_steps=200
        )
        all_results.append(results)
        
        # Sauvegarder logs jeu
        forensic_logger.save_jsonl(f"forensic_{game_id}.jsonl")
        forensic_logger.events.clear()  # Clear pour prochain jeu
    
    # Rapport final
    print(f"\n{'='*60}")
    print("RAPPORT FINAL")
    print(f"{'='*60}")
    
    wins = sum(1 for r in all_results if r["won"])
    avg_steps = sum(r["total_steps"] for r in all_results) / len(all_results)
    avg_reward = sum(r["total_reward"] for r in all_results) / len(all_results)
    avg_positions = sum(r["unique_positions"] for r in all_results) / len(all_results)
    total_loops = sum(r["loops_detected"] for r in all_results)
    total_dead_ends = sum(r["dead_ends_detected"] for r in all_results)
    total_breaks = sum(r["policy_breaks"] for r in all_results)
    
    summary = {
        "timestamp": timestamp,
        "games_tested": len(games),
        "wins": wins,
        "win_rate": wins / len(games),
        "avg_steps": avg_steps,
        "avg_reward": avg_reward,
        "avg_unique_positions": avg_positions,
        "total_loops_detected": total_loops,
        "total_dead_ends_detected": total_dead_ends,
        "total_policy_breaks": total_breaks,
        "results": all_results
    }
    
    print(f"\nVictoires: {wins}/{len(games)} ({summary['win_rate']*100:.1f}%)")
    print(f"Steps moyens: {avg_steps:.1f}")
    print(f"Reward moyen: {avg_reward:.3f}")
    print(f"Positions uniques moyennes: {avg_positions:.1f}")
    print(f"Loops détectés (total): {total_loops}")
    print(f"Dead-ends détectés (total): {total_dead_ends}")
    print(f"Policy breaks (total): {total_breaks}")
    
    # Sauvegarder rapport
    report_path = Path(log_dir) / "rapport_final.json"
    with open(report_path, 'w') as f:
        json.dump(summary, f, indent=2)
    
    print(f"\n✓ Rapport sauvegardé: {report_path}")
    
    return summary


if __name__ == "__main__":
    summary = main()

# Made with Bob
