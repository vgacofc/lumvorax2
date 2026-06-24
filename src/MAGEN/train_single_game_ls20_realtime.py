#!/usr/bin/env python3
"""
ENTRAÎNEMENT FOCALISÉ - JEU LS20 UNIQUEMENT
===========================================

OBJECTIF: Gagner le jeu ls20-9607627b en moins de 5 minutes
MONITORING: Temps réel pour CHAQUE action
STRATÉGIE: Entraînement en loop jusqu'à victoire

Auteur: Bob (Session 78)
Date: 2026-06-16
"""

import os
import sys
import json
import time
import numpy as np
from pathlib import Path
from datetime import datetime, timedelta
from typing import Dict, List, Any

# Configuration API - Lecture depuis Doppler (LEÇON #79.1)
ARC_API_KEY = os.getenv("ARC_API_KEY")
if not ARC_API_KEY:
    raise ValueError(
        "❌ ARC_API_KEY non configurée!\n"
        "Exécutez: doppler run -- python3 train_single_game_ls20_realtime.py\n"
        "Ou configurez: doppler secrets set ARC_API_KEY='votre-cle'"
    )
print(f"✅ ARC_API_KEY chargée depuis Doppler: {ARC_API_KEY[:8]}...")

# JEU CIBLE
TARGET_GAME = "ls20-9607627b"
MAX_TRAINING_TIME = 60  # 1 minute pour test (10 tentatives environ)

print(f"""
{'='*80}
🎯 ENTRAÎNEMENT FOCALISÉ - JEU LS20 UNIQUEMENT
{'='*80}
Jeu Cible: {TARGET_GAME}
Objectif: VICTOIRE en < 5 minutes
Monitoring: TEMPS RÉEL pour CHAQUE action
Timestamp: {datetime.now().isoformat()}
{'='*80}
""")

# Import arc-agi
try:
    from arc_agi import Arcade, OperationMode
    from arcengine import GameAction
    print("✅ arc-agi importé")
except ImportError as e:
    print(f"❌ ERREUR: {e}")
    sys.exit(1)

# Import MAGEN V40.3
MAGEN_ROOT = Path(__file__).parent
sys.path.insert(0, str(MAGEN_ROOT / "core"))

try:
    from world_state_graph_v39 import WorldStateGraph
    from agent_localization_v39 import AgentLocalizationSystem
    from causal_memory_v39 import CausalMemorySystem
    from minimal_learning_system_v39 import MinimalLearningSystemV39
    from policy_manager_v40_3 import PolicyManagerV40_3, ProblemType, ExplorationMode
    from reputation_system_v39 import ReputationSystem
    from budget_allocator_v39 import BudgetAllocator
    from trajectory_monitor_v39 import TrajectoryMonitor
    from cognitive_orchestrator_v39 import CognitiveOrchestrator
    # PHASE 1: Intégration Reward System (Session 79)
    from information_gain_reward import InformationGainReward
    print("✅ MAGEN V40.3 modules chargés")
    print("✅ InformationGainReward activé (Phase 1)\n")
except ImportError as e:
    print(f"❌ ERREUR import MAGEN: {e}")
    sys.exit(1)


class ArcadeActionMapper:
    """Mapping actions."""
    COMMON_ACTIONS = {
        'ACTION1': GameAction.ACTION1,
        'ACTION2': GameAction.ACTION2,
        'ACTION3': GameAction.ACTION3,
        'ACTION4': GameAction.ACTION4,
        'ACTION5': GameAction.ACTION5,
        'ACTION7': GameAction.ACTION7,  # ACTION6 blacklisté
        'RESET': GameAction.RESET
    }
    
    @classmethod
    def get_available_actions(cls):
        return [a for a in cls.COMMON_ACTIONS.keys() if a != 'RESET']


class MAGENArcadeAdapter:
    """Adaptateur MAGEN V40.3 pour Arcade."""
    
    def __init__(self, game_id: str, metadata: Dict):
        self.game_id = game_id
        self.metadata = metadata
        
        # Initialiser modules MAGEN V40.3
        # WorldStateGraph V39 et AgentLocalizationSystem V39 requièrent width et height
        # Utiliser dimensions standard pour jeux Arcade (100x100)
        self.world_graph = WorldStateGraph(width=100, height=100)
        self.agent_loc = AgentLocalizationSystem(grid_width=100, grid_height=100)
        self.causal_mem = CausalMemorySystem()
        self.learning_sys = MinimalLearningSystemV39()
        
        # PHASE 1: Initialiser Reward System Intrinsèque
        self.intrinsic_reward = InformationGainReward(
            knowledge_weight=0.4,   # Poids découverte nouveaux états
            causal_weight=0.3,      # Poids compréhension causale
            novelty_weight=0.3      # Poids nouveauté exploration
        )
        print(f"[PHASE 1] ✅ InformationGainReward initialisé")
        
        # PolicyManager V40.3 avec consultation forcée
        # PHASE 2 (P1): Activer DecisionKernelV34Causal
        self.policy = PolicyManagerV40_3(
            reputation_system=None,  # Sera créé par défaut
            budget_allocator=None,   # Sera créé par défaut
            trajectory_monitor=None, # Sera créé par défaut
            initial_mode=ExplorationMode.EXPLORE,
            enable_decision_kernel_v34=True  # ✅ PHASE 2 ACTIVÉE
        )
        # Configurer le type de problème séparément
        self.policy.set_problem_type(ProblemType.ARCADE)
        
        # Modules cognitifs
        self.reputation = ReputationSystem()
        self.budget = BudgetAllocator(
            max_total_steps=10000,
            max_total_time_ms=300000.0,  # 5 minutes
            max_total_memory_mb=1024.0
        )
        self.trajectory = TrajectoryMonitor()
        
        # Orchestrateur - CognitiveOrchestrator V39 requiert grid_width et grid_height
        self.orchestrator = CognitiveOrchestrator(
            grid_width=100,
            grid_height=100
        )
        
        # PHASE P0.3 (Session 79): Restreindre aux actions VALIDES dans ls20
        # Analyse forensique ls20.py (lignes 1943-1957): Seules ACTION1-4 sont reconnues
        # ACTION5 et ACTION7 sont IGNORÉES (40% actions inutiles)
        # QUICK WIN: Restriction immédiate pour améliorer diversité états
        self.available_actions = ['ACTION1', 'ACTION2', 'ACTION3', 'ACTION4']
        print(f"[P0.3 QUICK WIN] ✅ Actions restreintes à mouvements directionnels uniquement")
        print(f"[P0.3] ACTION1=HAUT, ACTION2=BAS, ACTION3=GAUCHE, ACTION4=DROITE")
        print(f"[P0.3] ACTION5 et ACTION7 DÉSACTIVÉES (ignorées par ls20)")
        
        self.step_count = 0
        self.total_reward = 0.0
        self.total_intrinsic_reward = 0.0
        self.last_action = None
        self.last_obs = None
        
        print(f"[MAGEN V40.3] Adaptateur initialisé pour {game_id}")
        print(f"[MAGEN V40.3] Actions disponibles: {len(self.available_actions)}")
    
    def select_action(self, obs):
        """Sélectionne action avec MAGEN V40.3."""
        if obs is None:
            action_name = np.random.choice(self.available_actions)
            return ArcadeActionMapper.COMMON_ACTIONS[action_name]
        
        # Extraire position agent
        x, y = 0, 0
        if hasattr(obs, 'metadata') and obs.metadata:
            x = obs.metadata.get('agent_x', 0)
            y = obs.metadata.get('agent_y', 0)
        
        # Décision PolicyManager V40.3 avec consultation forcée
        # BUG #5.1 FIX: decide_action() retourne (action, consultation)
        action_name, consultation = self.policy.decide_action(
            available_actions=self.available_actions,
            world_graph=self.world_graph,
            agent_loc=self.agent_loc,
            causal_mem=self.causal_mem,
            learning_sys=self.learning_sys
        )
        
        self.last_action = action_name
        return ArcadeActionMapper.COMMON_ACTIONS[action_name]
    
    def update_after_step(self, obs, reward: float, done: bool) -> float:
        """Met à jour MAGEN après step et retourne reward total (externe + intrinsèque)."""
        self.step_count += 1
        external_reward = reward
        
        # PHASE 1: Calculer reward intrinsèque depuis différence frame
        intrinsic_reward = 0.0
        if obs and self.last_obs is not None:
            # CORRECTION SESSION 79: Utiliser obs.frame au lieu de obs.metadata
            try:
                # Extraire frames
                frame_current = obs.frame if hasattr(obs, 'frame') else None
                frame_last = self.last_obs.frame if hasattr(self.last_obs, 'frame') else None
                
                if frame_current is not None and frame_last is not None:
                    # Convertir en numpy arrays
                    if isinstance(frame_current, list):
                        frame_current = np.array(frame_current)
                    if isinstance(frame_last, list):
                        frame_last = np.array(frame_last)
                    
                    # Calculer différence absolue
                    diff = np.abs(frame_current - frame_last).sum()
                    
                    # Normaliser (max = 64*64*12 = 49152)
                    intrinsic_reward = min(diff / 49152.0, 1.0)
                else:
                    # Fallback: utiliser InformationGainReward original
                    class SimpleTransition:
                        def __init__(self, state_before, action, state_after, reward_val):
                            self.state_before = np.zeros((10, 10))
                            self.action = action
                            self.state_after = np.zeros((10, 10))
                            self.reward = reward_val
                            self.context = {}
                            self.timestamp = time.time()
                    
                    transition = SimpleTransition(
                        self.last_obs, self.last_action or 'UNKNOWN', obs, external_reward
                    )
                    intrinsic_reward = self.intrinsic_reward.compute_reward(transition)
            except Exception as e:
                # En cas d'erreur, reward intrinsèque = 0
                intrinsic_reward = 0.0
        
        # Reward total = externe + intrinsèque
        total_reward = external_reward + intrinsic_reward
        self.total_reward += external_reward
        self.total_intrinsic_reward += intrinsic_reward
        
        # Sauvegarder observation pour prochaine transition
        self.last_obs = obs
        
        if obs and hasattr(obs, 'metadata') and obs.metadata:
            x = obs.metadata.get('agent_x', 0)
            y = obs.metadata.get('agent_y', 0)
            
            # Mettre à jour modules
            self.world_graph.update_cell(x, y, visited=True)
            self.agent_loc.update_position(x, y, confidence=0.9)
            
            if self.last_action:
                self.causal_mem.record_action_outcome(
                    self.last_action, x, y, total_reward, done  # Utiliser reward total
                )
                self.learning_sys.update_action_score(
                    self.last_action, total_reward  # Utiliser reward total
                )
            
            # Mettre à jour Policy avec reward total
            self.policy.update_after_action(
                self.last_action, total_reward, done,
                world_graph=self.world_graph,
                agent_loc=self.agent_loc
            )
            
            # Mettre à jour Orchestrator
            self.orchestrator.execute_step(
                self.last_action, x, y,
                self.last_action, int(total_reward * 10)
            )
        
        return total_reward


def reset_with_retry(env, max_retries=3, wait_time=2.0):
    """Reset avec gestion GAME_NOT_STARTED_ERROR.
    
    CORRECTION BUG #4: Ne PAS faire de double RESET !
    Le premier RESET suffit, pas besoin de tester puis re-RESET.
    """
    for attempt in range(max_retries):
        try:
            print(f"   🔄 RESET tentative {attempt+1}/{max_retries}...")
            obs = env.reset()
            print(f"   ⏳ Attente {wait_time}s...")
            time.sleep(wait_time)
            
            # ✅ CORRECTION: Retourner directement l'observation du RESET
            # Ne PAS faire de step() de test ni de deuxième reset()
            print(f"   ✅ Jeu démarré avec succès")
            return obs
                
        except Exception as e:
            if "GAME_NOT_STARTED" in str(e) and attempt < max_retries - 1:
                print(f"   ⚠️  GAME_NOT_STARTED, attente {wait_time * (attempt + 2)}s...")
                time.sleep(wait_time * (attempt + 2))
            else:
                if attempt == max_retries - 1:
                    raise RuntimeError(f"Impossible de démarrer après {max_retries} tentatives")
    
    raise RuntimeError("Impossible de démarrer le jeu")


# ============================================================================
# ENTRAÎNEMENT PRINCIPAL
# ============================================================================

print(f"{'='*80}")
print("CONNEXION API ARC-AGI-3")
print(f"{'='*80}\n")

try:
    arc = Arcade(
        arc_api_key=ARC_API_KEY,
        operation_mode=OperationMode.COMPETITION
    )
    print("✅ Connexion API réussie\n")
except Exception as e:
    print(f"❌ ERREUR: {e}")
    sys.exit(1)

# Statistiques entraînement (scorecard créé par tentative)
training_stats = {
    "game_id": TARGET_GAME,
    "start_time": datetime.now().isoformat(),
    "attempts": [],
    "best_reward": 0.0,
    "best_steps": 0,
    "max_level_reached": 0,
    "current_level": 0,
    "game_completed": False,
    "completion_time": None
}

start_time = time.time()
attempt = 0

print(f"{'='*80}")
print(f"ENTRAÎNEMENT EN LOOP - JEU {TARGET_GAME}")
print(f"{'='*80}\n")

while time.time() - start_time < MAX_TRAINING_TIME:
    attempt += 1
    elapsed = time.time() - start_time
    remaining = MAX_TRAINING_TIME - elapsed
    
    print(f"\n{'='*80}")
    print(f"TENTATIVE #{attempt} - Temps écoulé: {elapsed:.1f}s / {MAX_TRAINING_TIME}s")
    print(f"Temps restant: {remaining:.1f}s")
    print(f"{'='*80}\n")
    
    # ✅ CORRECTION BUG #1: Créer NOUVEAU scorecard pour CHAQUE tentative
    scorecard_id = arc.create_scorecard()
    print(f"✅ Scorecard #{attempt} créé: {scorecard_id}")
    
    env = None
    try:
        # Créer environnement
        env = arc.make(
            TARGET_GAME,
            scorecard_id=scorecard_id,
            seed=42 + attempt,
            save_recording=True
        )
        
        # Créer adaptateur
        adapter = MAGENArcadeAdapter(TARGET_GAME, {})
        
        # RESET avec retry
        try:
            last_obs = reset_with_retry(env)
        except RuntimeError as e:
            print(f"   ❌ Impossible de démarrer: {e}")
            continue
        
        # Jouer - CONTINUER JUSQU'À COMPLÉTION TOTALE DU JEU
        # BUG #6 FIX: Réduire max_steps pour éviter timeout de session
        max_steps = 100  # Réduit pour éviter timeout serveur (~5min)
        step = 0
        total_reward = 0.0
        current_level = 0
        max_level_reached = 0
        game_completed = False
        
        while step < max_steps:
            step += 1
            
            # Sélectionner action
            action = adapter.select_action(last_obs)
            action_name = [k for k, v in ArcadeActionMapper.COMMON_ACTIONS.items() if v == action][0]
            
            # MONITORING TEMPS RÉEL POUR CHAQUE ACTION
            print(f"   Step {step:3d} | Action: {action_name:8s} | ", end='', flush=True)
            
            try:
                # Exécuter action
                obs = env.step(action)
                last_obs = obs
                
                # Extraire reward et niveau depuis obs.state et obs.levels_completed
                reward = 0.0
                state_name = "UNKNOWN"
                level_info = ""
                if obs:
                    if hasattr(obs, 'state'):
                        state_name = obs.state.name
                        
                        # CORRECTION SESSION 79: Calculer reward depuis changement état
                        if obs.state.name == "GAME_OVER":
                            # Vérifier si victoire (levels_completed a augmenté)
                            if hasattr(obs, 'levels_completed'):
                                if obs.levels_completed > current_level:
                                    reward = +1.0  # Victoire niveau
                                    print(f"\n   🎉 NIVEAU COMPLÉTÉ: {current_level} → {obs.levels_completed}!")
                                    
                                    # LOGGING FORENSIQUE SESSION 79
                                    import json
                                    from pathlib import Path
                                    from datetime import datetime
                                    log_file = Path(__file__).parent / "results" / "game_over_victories.jsonl"
                                    log_file.parent.mkdir(parents=True, exist_ok=True)
                                    with open(log_file, 'a') as f:
                                        json.dump({
                                            "timestamp": datetime.now().isoformat(),
                                            "step": step,
                                            "type": "VICTORY",
                                            "level_before": current_level,
                                            "level_after": obs.levels_completed,
                                            "reward": reward
                                        }, f)
                                        f.write('\n')
                                else:
                                    reward = -1.0  # Défaite (GAME_OVER sans progression)
                                    print(f"\n   ❌ GAME OVER (défaite)")
                                    
                                    # LOGGING FORENSIQUE SESSION 79
                                    import json
                                    from pathlib import Path
                                    from datetime import datetime
                                    log_file = Path(__file__).parent / "results" / "game_over_defeats.jsonl"
                                    log_file.parent.mkdir(parents=True, exist_ok=True)
                                    with open(log_file, 'a') as f:
                                        json.dump({
                                            "timestamp": datetime.now().isoformat(),
                                            "step": step,
                                            "type": "DEFEAT",
                                            "level": current_level,
                                            "reward": reward
                                        }, f)
                                        f.write('\n')
                    
                    # Extraire niveau actuel depuis obs.levels_completed
                    if hasattr(obs, 'levels_completed'):
                        current_level = obs.levels_completed
                        level_info = f" | Niveau: {current_level}"
                        if current_level > max_level_reached:
                            max_level_reached = current_level
                
                # Mettre à jour MAGEN et obtenir reward total (externe + intrinsèque)
                done = (state_name == "GAME_OVER")
                total_reward_with_intrinsic = adapter.update_after_step(obs, reward, done)
                
                # Calculer reward intrinsèque pour affichage
                intrinsic_reward = total_reward_with_intrinsic - reward
                total_reward += reward  # Compteur reward externe uniquement
                
                # Afficher résultat IMMÉDIAT avec reward intrinsèque séparé
                print(f"État: {state_name:12s} | Ext: {reward:+5.2f} | Int: {intrinsic_reward:+5.2f} | Total: {total_reward:+7.2f}{level_info}")
                
                # Vérifier complétion du jeu (pas juste GAME_OVER)
                # Le jeu passe automatiquement au niveau suivant après victoire
                # On continue jusqu'à ce que le jeu soit vraiment terminé
                if state_name == "GAME_OVER":
                    if total_reward > 0:
                        # Victoire d'un niveau - le jeu continue automatiquement
                        print(f"\n   ✅ Niveau {current_level} complété! Le jeu continue...")
                    else:
                        # Défaite - NE PAS ARRÊTER, continuer à réessayer
                        print(f"\n   ❌ GAME_OVER (défaite) - On continue à réessayer!")
                        # Pas de break - on continue!
                
                # Vérifier si le jeu est VRAIMENT terminé (tous les niveaux)
                # Cela devrait être indiqué par un état spécial ou metadata
                if hasattr(obs, 'metadata') and obs.metadata:
                    if obs.metadata.get('game_complete', False) or obs.metadata.get('all_levels_complete', False):
                        game_completed = True
                        completion_time = time.time() - start_time
                        print(f"\n🏆 JEU COMPLÉTÉ! Tous les niveaux terminés!")
                        print(f"   Temps: {completion_time:.1f}s, Reward total: {total_reward:.2f}")
                        print(f"   Niveaux complétés: {max_level_reached}")
                        training_stats["game_completed"] = True
                        training_stats["completion_time"] = completion_time
                        training_stats["max_level_reached"] = max_level_reached
                        break
                    
            except Exception as e:
                error_msg = str(e)
                print(f"ERREUR: {error_msg[:100]}")
                
                # BUG #6 FIX: Détecter timeout de session et faire RESET
                if "GAME_NOT_STARTED" in error_msg or "400 Client Error" in error_msg:
                    print(f"   ⚠️  Session expirée - Re-RESET automatique...")
                    try:
                        last_obs = reset_with_retry(env)
                        print(f"   ✅ Session restaurée, on continue!")
                        continue  # Continuer la boucle au lieu de break
                    except Exception as reset_error:
                        print(f"   ❌ Impossible de restaurer: {reset_error}")
                        break
                elif "HTTPError" in error_msg:
                    print(f"   ⚠️  Erreur HTTP - Tentative de récupération...")
                    time.sleep(2.0)
                    try:
                        last_obs = reset_with_retry(env)
                        continue
                    except:
                        break
                else:
                    print(f"   ❌ Erreur non récupérable, arrêt de la tentative")
                    break
        
        # Enregistrer tentative
        attempt_data = {
            "attempt": attempt,
            "steps": step,
            "total_reward": float(total_reward),
            "max_level_reached": max_level_reached,
            "game_completed": game_completed,
            "elapsed_time": time.time() - start_time
        }
        training_stats["attempts"].append(attempt_data)
        
        if total_reward > training_stats["best_reward"]:
            training_stats["best_reward"] = float(total_reward)
            training_stats["best_steps"] = step
        
        if max_level_reached > training_stats["max_level_reached"]:
            training_stats["max_level_reached"] = max_level_reached
        
        print(f"\n   Résultat: Steps={step}, Reward={total_reward:.2f}, Niveau max={max_level_reached}, Complété={game_completed}")
        
        # Si jeu complété (TOUS les niveaux), arrêter
        if game_completed:
            print(f"\n🏆 JEU {TARGET_GAME} COMPLÈTEMENT TERMINÉ!")
            break
            
    except Exception as e:
        print(f"   ❌ Erreur tentative: {e}")
        continue
    finally:
        # ✅ CORRECTION BUG #2: TOUJOURS fermer l'environnement
        if env is not None:
            try:
                env.close()
                print(f"   🔒 Environnement fermé proprement")
            except Exception as e:
                print(f"   ⚠️  Erreur fermeture env: {e}")

# Résumé final
training_stats["end_time"] = datetime.now().isoformat()
training_stats["total_time"] = time.time() - start_time
training_stats["total_attempts"] = attempt

print(f"\n{'='*80}")
print("RÉSUMÉ ENTRAÎNEMENT")
print(f"{'='*80}")
print(f"Jeu: {TARGET_GAME}")
print(f"Tentatives: {attempt}")
print(f"Temps total: {training_stats['total_time']:.1f}s")
print(f"Meilleur reward: {training_stats['best_reward']:.2f}")
print(f"Niveau maximum atteint: {training_stats['max_level_reached']}")
print(f"Jeu complété: {'✅ OUI' if training_stats['game_completed'] else '❌ NON'}")
if training_stats['game_completed']:
    print(f"Temps complétion: {training_stats['completion_time']:.1f}s")
print(f"{'='*80}\n")

# Sauvegarder stats
stats_file = MAGEN_ROOT / "results" / f"training_ls20_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
stats_file.parent.mkdir(parents=True, exist_ok=True)
with open(stats_file, 'w') as f:
    json.dump(training_stats, f, indent=2)
print(f"✅ Stats sauvegardées: {stats_file}")

# Made with Bob
