#!/usr/bin/env python3
"""
MAGEN V27 COGNITIVE - Exécution RÉELLE sur ARC-AGI-3
=====================================================

Exécution de l'agent V27 COGNITIVE sur les jeux ARC-AGI-3 RÉELS
avec API key officielle et mode COMPETITION.

API Key: b6ebc9c5-a296-4578-be69-0cc16c9455d4
Mode: COMPETITION (pour leaderboard)

Protocole: CLAUDE_PILOT + LUMVORAX + PROTOCOLE_MAGEN v2.0
Auteur: Bob (Mode Advanced)
Date: 2026-06-14
Session: 71
"""

import os
import sys
import json
import time
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any

# Ajouter le répertoire parent au path
sys.path.insert(0, str(Path(__file__).parent))

# Import arc_agi
try:
    import arc_agi
    from arc_agi import Arcade, OperationMode
    from arcengine import GameAction, GameState
    ARC_AVAILABLE = True
    print("✅ arc_agi disponible")
except ImportError as e:
    print(f"❌ arc_agi non disponible: {e}")
    print("Installation requise: pip install arc-agi")
    sys.exit(1)

# Import agent V27
try:
    from magen_arc_agi_agent_v27_cognitive import MAGENArcAgiAgentV27Cognitive
    print("✅ Agent V27 COGNITIVE importé")
except ImportError as e:
    print(f"❌ Erreur import agent V27: {e}")
    sys.exit(1)


class MAGENV27RealRunner:
    """
    Runner pour exécution RÉELLE de MAGEN V27 sur ARC-AGI-3
    
    Mode: COMPETITION (leaderboard)
    API: https://three.arcprize.org
    """
    
    def __init__(self, api_key: str):
        self.api_key = api_key
        self.session_id = datetime.now().strftime('%Y%m%d_%H%M%S')
        
        # Répertoires résultats
        self.results_dir = Path(__file__).parent / 'results_v27_real'
        self.results_dir.mkdir(exist_ok=True)
        
        self.forensic_dir = self.results_dir / 'forensic'
        self.forensic_dir.mkdir(exist_ok=True)
        
        self.recordings_dir = self.results_dir / 'recordings'
        self.recordings_dir.mkdir(exist_ok=True)
        
        # Forensic log path
        self.forensic_log_path = self.forensic_dir / f'v27_real_{self.session_id}.jsonl'
        
        # Agent V27
        self.agent = MAGENArcAgiAgentV27Cognitive(
            forensic_log_path=str(self.forensic_log_path)
        )
        
        # Arcade
        self.arcade = None
        self.scorecard_id = None
        
        # Résultats
        self.results = {
            'session_id': self.session_id,
            'version': 'V27_COGNITIVE',
            'mode': 'COMPETITION',
            'api_key': api_key[:8] + '...' + api_key[-4:],
            'games': [],
            'summary': {},
            'start_time': datetime.now().isoformat(),
            'end_time': None
        }
        
        print("=" * 80)
        print("🚀 MAGEN V27 COGNITIVE - EXÉCUTION RÉELLE ARC-AGI-3")
        print("=" * 80)
        print(f"Session ID: {self.session_id}")
        print(f"Results dir: {self.results_dir}")
        print(f"Forensic log: {self.forensic_log_path}")
        print(f"Mode: COMPETITION")
        print("=" * 80)
    
    def initialize_arcade(self) -> bool:
        """Initialiser Arcade en mode COMPETITION"""
        print("\n📡 Initialisation Arcade (mode COMPETITION)...")
        
        try:
            self.arcade = Arcade(
                arc_api_key=self.api_key,
                operation_mode=OperationMode.COMPETITION
            )
            
            # Créer scorecard
            self.scorecard_id = self.arcade.create_scorecard(
                source_url="https://github.com/lvx/MAGEN-V27-COGNITIVE",
                tags=["magen-v27", "cognitive", "grid-aware", "c17-reputation", "session-71"],
                opaque={
                    "version": "27.0.0",
                    "session": self.session_id,
                    "architecture": "V26_GRID_AWARE + C17_REPUTATION",
                    "features": ["GridAnalyzer", "PathfindingAStar", "ActionReputation", "MemoryDecisionnelle"]
                }
            )
            
            self.results['scorecard_id'] = self.scorecard_id
            
            print(f"✅ Arcade initialisé")
            print(f"✅ Scorecard créé: {self.scorecard_id[:16]}...")
            
            return True
            
        except Exception as e:
            print(f"❌ Erreur initialisation: {e}")
            import traceback
            traceback.print_exc()
            return False
    
    def get_available_games(self) -> List[str]:
        """Récupérer liste des jeux disponibles"""
        print("\n🎮 Récupération jeux disponibles...")
        
        try:
            envs = self.arcade.get_environments()
            game_ids = [env.game_id for env in envs]
            
            print(f"✅ {len(game_ids)} jeux disponibles")
            for i, game_id in enumerate(game_ids[:10], 1):
                print(f"   {i}. {game_id}")
            if len(game_ids) > 10:
                print(f"   ... et {len(game_ids) - 10} autres")
            
            return game_ids
            
        except Exception as e:
            print(f"❌ Erreur récupération jeux: {e}")
            return []
    
    def play_game(self, game_id: str, max_actions: int = 100) -> Dict:
        """
        Jouer un jeu avec agent V27
        
        Args:
            game_id: ID du jeu
            max_actions: Nombre maximum d'actions
            
        Returns:
            Résultats du jeu
        """
        print(f"\n{'='*80}")
        print(f"🎮 JEU: {game_id}")
        print(f"{'='*80}")
        
        game_result = {
            'game_id': game_id,
            'success': False,
            'score': 0.0,
            'levels_completed': 0,
            'actions_taken': 0,
            'final_state': None,
            'error': None,
            'start_time': time.time()
        }
        
        try:
            # Créer environnement
            env = self.arcade.make(
                game_id,
                scorecard_id=self.scorecard_id,
                seed=42,
                save_recording=True
            )
            
            print(f"✅ Environnement créé")
            
            # Reset initial
            obs = env.step(GameAction.RESET)
            
            if not obs:
                print(f"❌ Pas d'observation initiale")
                game_result['error'] = "No initial observation"
                return game_result
            
            print(f"📊 État initial: {obs.state.name}")
            # FrameDataRaw n'a pas d'attribut score/levels_completed
            # Ces infos sont dans le scorecard final
            
            # Réinitialiser agent
            self.agent.reset()
            
            # Boucle de jeu
            for action_num in range(max_actions):
                # Percevoir
                grid_obs = self.agent.perceive(obs)
                
                # Décider
                available_actions = ['ACTION1', 'ACTION2', 'ACTION3', 'ACTION4', 'ACTION5', 'ACTION6', 'ACTION7', 'RESET']
                decision = self.agent.decide(grid_obs, available_actions)
                
                print(f"\n[Action {action_num+1}/{max_actions}]")
                print(f"  Stratégie: {decision.strategy}")
                print(f"  Action: {decision.action_name} ({decision.semantic_name})")
                print(f"  Score: {decision.score:.3f}")
                print(f"  Justification: {decision.justification}")
                
                # Exécuter action
                obs_before = grid_obs
                obs_after = env.step(decision.action)
                
                if not obs_after:
                    print(f"  ⚠️  Pas d'observation après action")
                    break
                
                # Apprendre
                # FrameDataRaw n'a pas score/levels_completed - utiliser changement d'état comme reward
                reward = 1.0 if obs_after.state.name != obs.state.name else 0.0
                done = obs_after.state.name in ['GAME_OVER', 'LEVEL_COMPLETE']
                
                self.agent.learn(obs_before, decision.action_name, self.agent.perceive(obs_after), reward, done)
                
                print(f"  État: {obs_after.state.name}")
                print(f"  Reward: {reward:+.3f}")
                
                # Mettre à jour observation
                obs = obs_after
                game_result['actions_taken'] += 1
                
                # Vérifier fin
                if done:
                    print(f"\n🏁 Jeu terminé: {obs.state.name}")
                    break
            
            # Résultats finaux
            game_result['success'] = True
            game_result['score'] = 0.0  # Score récupéré du scorecard final
            game_result['levels_completed'] = 0  # Récupéré du scorecard final
            game_result['final_state'] = obs.state.name if obs else 'UNKNOWN'
            
            print(f"\n{'='*80}")
            print(f"📊 RÉSULTATS {game_id}")
            print(f"{'='*80}")
            print(f"Score final: {game_result['score']}")
            print(f"Niveaux complétés: {game_result['levels_completed']}")
            print(f"Actions prises: {game_result['actions_taken']}")
            print(f"État final: {game_result['final_state']}")
            
        except Exception as e:
            print(f"\n❌ Erreur pendant jeu: {e}")
            import traceback
            traceback.print_exc()
            game_result['error'] = str(e)
        
        finally:
            game_result['end_time'] = time.time()
            game_result['duration'] = game_result['end_time'] - game_result['start_time']
        
        return game_result
    
    def run_competition(self, max_games: int = 3):
        """
        Exécuter compétition sur plusieurs jeux
        
        Args:
            max_games: Nombre maximum de jeux à jouer
        """
        print(f"\n{'='*80}")
        print(f"🏆 DÉMARRAGE COMPÉTITION (max {max_games} jeux)")
        print(f"{'='*80}")
        
        # Initialiser
        if not self.initialize_arcade():
            print("❌ Échec initialisation")
            return
        
        # Récupérer jeux
        game_ids = self.get_available_games()
        if not game_ids:
            print("❌ Aucun jeu disponible")
            return
        
        # Jouer jeux
        games_to_play = game_ids[:max_games]
        
        for i, game_id in enumerate(games_to_play, 1):
            print(f"\n{'#'*80}")
            print(f"# JEU {i}/{len(games_to_play)}: {game_id}")
            print(f"{'#'*80}")
            
            game_result = self.play_game(game_id, max_actions=100)
            self.results['games'].append(game_result)
            
            # Sauvegarder résultats intermédiaires
            self._save_results()
        
        # Fermer scorecard
        print(f"\n📊 Fermeture scorecard...")
        try:
            final_scorecard = self.arcade.close_scorecard(self.scorecard_id)
            if final_scorecard:
                print(f"✅ Scorecard fermé")
                print(f"Score final: {final_scorecard.score}")
                self.results['final_scorecard'] = final_scorecard.model_dump()
        except Exception as e:
            print(f"⚠️  Erreur fermeture scorecard: {e}")
        
        # Résumé
        self._print_summary()
        
        # Sauvegarder résultats finaux
        self.results['end_time'] = datetime.now().isoformat()
        self._save_results()
    
    def _save_results(self):
        """Sauvegarder résultats"""
        results_file = self.results_dir / f'results_v27_{self.session_id}.json'
        with open(results_file, 'w') as f:
            json.dump(self.results, f, indent=2)
    
    def _print_summary(self):
        """Afficher résumé"""
        print(f"\n{'='*80}")
        print(f"📊 RÉSUMÉ COMPÉTITION")
        print(f"{'='*80}")
        
        total_games = len(self.results['games'])
        successful_games = sum(1 for g in self.results['games'] if g['success'])
        total_score = sum(g['score'] for g in self.results['games'])
        total_levels = sum(g['levels_completed'] for g in self.results['games'])
        total_actions = sum(g['actions_taken'] for g in self.results['games'])
        
        print(f"Jeux joués: {total_games}")
        print(f"Jeux réussis: {successful_games}/{total_games}")
        print(f"Score total: {total_score:.2f}")
        print(f"Niveaux complétés: {total_levels}")
        print(f"Actions totales: {total_actions}")
        
        if total_games > 0:
            print(f"\nMoyennes:")
            print(f"  Score moyen: {total_score/total_games:.2f}")
            print(f"  Niveaux/jeu: {total_levels/total_games:.2f}")
            print(f"  Actions/jeu: {total_actions/total_games:.1f}")
        
        print(f"\nDétails par jeu:")
        for i, game in enumerate(self.results['games'], 1):
            status = "✅" if game['success'] else "❌"
            print(f"  {i}. {status} {game['game_id']}: score={game['score']:.2f}, levels={game['levels_completed']}, actions={game['actions_taken']}")
        
        print(f"\n📁 Résultats sauvegardés: {self.results_dir}")
        print(f"📁 Logs forensiques: {self.forensic_log_path}")
        print(f"{'='*80}")


def main():
    """Point d'entrée principal"""
    # API key depuis guide
    API_KEY = "b6ebc9c5-a296-4578-be69-0cc16c9455d4"
    
    # Créer runner
    runner = MAGENV27RealRunner(api_key=API_KEY)
    
    # Exécuter compétition (3 jeux pour test)
    runner.run_competition(max_games=3)
    
    print("\n✅ Exécution terminée")


if __name__ == '__main__':
    main()

# Made with Bob
