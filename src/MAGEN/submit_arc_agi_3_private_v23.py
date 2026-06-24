"""
ARC-AGI-3 Private Submitter V23 - MAGEN Agent avec Action Discovery
"""

import os
import sys
import json
import time
from pathlib import Path
from datetime import datetime
from typing import List, Dict, Optional
import numpy as np
import arc_agi
from arcengine import GameAction, GameState

# Ajouter le répertoire parent au path
sys.path.insert(0, str(Path(__file__).parent))

# Import agent MAGEN V23
from magen_arc_agi_agent_v23 import MAGENArcAgiAgentV23


class DateTimeEncoder(json.JSONEncoder):
    """Encodeur JSON personnalisé pour datetime, numpy et EnvironmentInfo"""
    def default(self, obj):
        if isinstance(obj, datetime):
            return obj.isoformat()
        if isinstance(obj, np.integer):
            return int(obj)
        if isinstance(obj, np.floating):
            return float(obj)
        if isinstance(obj, np.ndarray):
            return obj.tolist()
        # Sérialiser EnvironmentInfo comme string
        if hasattr(obj, 'game_id'):
            return str(obj)
        return super().default(obj)


class ArcAgi3PrivateSubmitterV23:
    """
    Submitter privé pour ARC-AGI-3 avec agent MAGEN V23
    
    Utilise Action Discovery + World Model + Information Gain Reward
    """
    
    def __init__(self, api_key: str):
        self.api_key = api_key
        self.arcade = None
        self.agent = None
        self.session_id = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        # Résultats
        self.results = {
            'session_id': self.session_id,
            'mode': 'PRIVATE',
            'agent_version': 'V23',
            'games_played': [],
            'total_score': 0.0,
            'start_time': None,
            'end_time': None,
            'scorecard_id': None,
            'final_scorecard': None
        }
    
    def initialize(self):
        """Initialise l'arcade et l'agent"""
        print("\n" + "="*80)
        print("🧠 MAGEN V23 - AGENT COMPLET POUR ARC-AGI-3")
        print("="*80)
        print(f"Session ID: {self.session_id}")
        print(f"API Key: {self.api_key[:12]}...{self.api_key[-4:]}")
        print(f"Mode: 🔒 PRIVÉ (aucune soumission publique)")
        print(f"Architecture: V22 (TLE+PatternMatcher+ObjectExtractor+MetaArbiter+C17+C18)")
        print(f"            + V23 (ActionDiscovery+WorldModel+InformationGainReward)")
        print("="*80 + "\n")
        
        # Initialiser arcade
        print("📡 Initialisation ARC-AGI Arcade...")
        # API key est configurée via variable d'environnement ARC_API_KEY
        self.arcade = arc_agi.Arcade()
        scorecard = self.arcade.get_scorecard()
        print(f"✅ Arcade initialisé (scorecard: {scorecard.card_id[:8]}...)")
        print(f"🔒 Mode: PRIVÉ (pas de leaderboard public)\n")
        
        # Initialiser agent MAGEN V23
        self.agent = MAGENArcAgiAgentV23()
        
        self.results['scorecard_id'] = scorecard.card_id
    
    def play_game(self, game_id: str, max_actions: int = 200) -> Dict:
        """
        Joue un jeu avec l'agent MAGEN V23
        
        Args:
            game_id: ID du jeu
            max_actions: Nombre maximum d'actions
        
        Returns:
            Résultats du jeu
        """
        print(f"\n🎮 Jeu: {game_id}\n")
        
        # Réinitialiser agent
        self.agent.reset_for_new_game()
        
        # Créer environnement
        # Extraire nom de base du jeu (ar25-0c556536 → ar25)
        if hasattr(game_id, 'game_id'):
            game_name = game_id.game_id.split('-')[0]
        else:
            game_name = str(game_id).split('-')[0]
        env = self.arcade.make(game_name)
        
        # Jouer
        start_time = time.time()
        obs = env.reset()
        actions_taken = 0
        
        print(f"[MAGEN V23] Début jeu - Max actions: {max_actions}\n")
        
        # Actions disponibles pour tous les jeux ARC-AGI-3
        available_actions = [
            GameAction.ACTION1,
            GameAction.ACTION2,
            GameAction.ACTION3,
            GameAction.ACTION4,
            GameAction.ACTION5,
            GameAction.ACTION6,
            GameAction.ACTION7,
            GameAction.RESET
        ]
        
        while obs.state not in ['WIN', 'GAME_OVER'] and actions_taken < max_actions:
            # Perception
            grid_obs = self.agent.perceive(obs)
            
            # Décision
            decision = self.agent.decide(grid_obs, available_actions)
            
            # Exécution
            obs_after = env.step(decision.action)
            actions_taken += 1
            
            # Apprentissage
            grid_obs_after = self.agent.perceive(obs_after)
            self.agent.learn(grid_obs, decision, grid_obs_after)
            
            # Mise à jour
            obs = obs_after
            
            # Logging périodique
            if actions_taken % 20 == 0:
                print(f"[MAGEN V23] Actions: {actions_taken}/{max_actions} | Niveaux: {obs.levels_completed} | État: {obs.state}")
        
        end_time = time.time()
        
        # Résultats
        result = {
            'game_id': str(game_id),
            'score': float(obs.levels_completed),  # FIX: obs.score n'existe pas, utiliser levels_completed
            'levels_completed': obs.levels_completed,
            'actions_taken': actions_taken,
            'start_time': start_time,
            'end_time': end_time,
            'error': None,
            'success': obs.levels_completed > 0,
            'final_state': str(obs.state)
        }
        
        print(f"\n[MAGEN V23] Résultats finaux:")
        print(f"  Niveaux complétés: {obs.levels_completed}")
        print(f"  Actions: {actions_taken}")
        print(f"  Succès: {result['success']}")
        print(f"  État final: {obs.state}\n")
        
        # Statistiques agent
        stats = self.agent.get_statistics()
        print(f"[MAGEN V23] Statistiques:")
        print(f"  Taux succès: {stats['successful_actions']/stats['total_actions']*100:.1f}%")
        print(f"  Reward moyen: {stats['avg_info_gain_reward']:.2f}")
        print(f"  Patterns détectés: {stats['patterns_detected']}")
        print(f"  Actions découvertes: {stats['actions_discovered']}")
        print(f"  World model accuracy: {stats['world_model_accuracy']:.2f}")
        
        return result
    
    def run_competition(self, max_games: Optional[int] = None):
        """
        Lance la compétition sur plusieurs jeux
        
        Args:
            max_games: Nombre maximum de jeux (None = tous)
        """
        self.initialize()
        
        # Récupérer liste des jeux
        print("🎮 Récupération liste des jeux...")
        games = self.arcade.available_environments
        print(f"✅ {len(games)} jeux disponibles\n")
        
        # Limiter nombre de jeux
        if max_games:
            games = games[:max_games]
        
        print(f"🎯 Compétition: {len(games)} jeux")
        print(f"🔒 Mode: PRIVÉ (pas de leaderboard public)")
        print(f"⏱️  Début: {datetime.now().isoformat()}\n")
        
        self.results['start_time'] = datetime.now().isoformat()
        
        # Jouer chaque jeu
        for i, game_id in enumerate(games):
            print("="*80)
            print(f"📍 Progression: {i+1}/{len(games)} ({(i+1)/len(games)*100:.1f}%)")
            print("="*80)
            
            try:
                result = self.play_game(game_id)
                self.results['games_played'].append(result)
                self.results['total_score'] += result['score']
            except Exception as e:
                print(f"❌ Erreur jeu {game_id}: {e}")
                self.results['games_played'].append({
                    'game_id': game_id,
                    'error': str(e),
                    'success': False
                })
        
        self.results['end_time'] = datetime.now().isoformat()
        
        # Scorecard final
        try:
            scorecard = self.arcade.get_scorecard()
            self.results['final_scorecard'] = {
                'score': scorecard.score,
                'scorecard_data': scorecard.to_dict()
            }
        except:
            pass
        
        # Afficher résumé
        self.print_summary()
        
        # Sauvegarder résultats
        self.save_results()
    
    def print_summary(self):
        """Affiche le résumé final"""
        print("\n" + "="*80)
        print("🏆 SCORECARD FINAL")
        print("="*80)
        print(f"Score: {self.results['total_score']:.2f}")
        print(f"Scorecard ID: {self.results['scorecard_id']}")
        print("="*80 + "\n")
        
        print("="*80)
        print("📊 RÉSUMÉ FINAL - MODE PRIVÉ")
        print("="*80)
        print(f"Session: {self.session_id}")
        print(f"Jeux joués: {len(self.results['games_played'])}")
        print(f"Score total: {self.results['total_score']:.2f}")
        
        if self.results['final_scorecard']:
            print(f"Score scorecard: {self.results['final_scorecard']['score']:.2f}")
        
        print(f"Début: {self.results['start_time']}")
        print(f"Fin: {self.results['end_time']}")
        print("="*80)
        print("🔒 Mode: PRIVÉ (aucune soumission publique)")
        print("="*80 + "\n")
    
    def save_results(self):
        """Sauvegarde les résultats"""
        os.makedirs('results/arc_agi_3', exist_ok=True)
        
        filename = f"results/arc_agi_3/private_submission_v23_{self.session_id}.json"
        with open(filename, 'w') as f:
            json.dump(self.results, f, indent=2, cls=DateTimeEncoder)
        
        print(f"💾 Résultats sauvegardés: {filename}\n")


if __name__ == "__main__":
    # Test avec 3 jeux
    api_key = os.getenv('ARC_API_KEY')
    if not api_key:
        print("❌ Erreur: ARC_API_KEY non définie")
        exit(1)
    
    submitter = ArcAgi3PrivateSubmitterV23(api_key=api_key)
    submitter.run_competition(max_games=3)

# Made with Bob
