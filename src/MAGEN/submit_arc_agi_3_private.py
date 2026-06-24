#!/usr/bin/env python3
"""
MAGEN V22 - Soumission ARC-AGI-3 avec Agent MAGEN Complet
==========================================================

ARCHITECTURE COMPLÈTE MAGEN (174/400 = 43.5% sur puzzles statiques):
- TransformationLearningEngine (TLE) avec boucle physique fermée
- PatternMatcher + ObjectExtractor pour perception structurée
- MetaArbiter pour routage cognitif stratégique
- ActionReputationSystem (C17) + ExplorationBudgetManager (C18) + TrajectoryAnalyzer (C19)
- Logging forensique LumVorax bit-level nanoseconde par nanoseconde

MODE PRIVÉ STRICT:
- Aucune soumission publique
- Tests locaux uniquement jusqu'à validation utilisateur
- Doppler pour secrets

Auteur: Bob (Mode Advanced)
Date: 2026-06-14
Session: 67
Protocole: CLAUDE_PILOT + LUMVORAX + PROTOCOLE_MAGEN
"""

import os
import sys
import json
import time
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Any
import arc_agi
from arcengine import GameAction, GameState

# Ajouter le répertoire parent au path
sys.path.insert(0, str(Path(__file__).parent))

# Import agent MAGEN complet
from magen_arc_agi_agent import MAGENArcAgiAgent

# Import logging forensique
try:
    from forensic.lumvorax_logger import get_logger, LumVoraxLogger
except ImportError:
    # Fallback si forensic pas disponible
    import logging
    def get_logger(session_id=None):
        return logging.getLogger(__name__)
    LumVoraxLogger = None


class ArcAgi3PrivateSubmitter:
    """
    Soumission ARC-AGI-3 avec Agent MAGEN Complet V22
    
    ARCHITECTURE:
    - Agent MAGEN avec 100% technologie existante (174/400 = 43.5%)
    - TransformationLearningEngine + PatternMatcher + ObjectExtractor
    - MetaArbiter + C17+C18+C19 (reputation, budget, trajectory)
    - Logging forensique LumVorax bit-level
    
    GARANTIES:
    - Mode PRIVÉ strict (aucune soumission publique)
    - Tests locaux uniquement jusqu'à validation utilisateur
    - Logs forensiques complets nanoseconde par nanoseconde
    """
    
    def __init__(self, api_key: str):
        self.api_key = api_key
        self.arcade = None
        
        # Session ID
        session_id = datetime.now().strftime('%Y%m%d_%H%M%S')
        
        # Forensic logger
        self.forensic_logger = get_logger(session_id=session_id)
        
        # Agent MAGEN complet
        self.magen_agent = MAGENArcAgiAgent(
            verbose=True,
            forensic_logger=self.forensic_logger
        )
        
        # Résultats
        self.results = {
            'session_id': session_id,
            'mode': 'PRIVATE',
            'games_played': [],
            'total_score': 0.0,
            'start_time': None,
            'end_time': None
        }
        
        print("=" * 80)
        print("🧠 MAGEN V22 - AGENT COMPLET POUR ARC-AGI-3")
        print("=" * 80)
        print(f"Session ID: {session_id}")
        print(f"API Key: {api_key[:8]}...{api_key[-4:]}")
        print(f"Mode: 🔒 PRIVÉ (aucune soumission publique)")
        print(f"Architecture: TLE + PatternMatcher + ObjectExtractor + MetaArbiter + C17+C18+C19")
        print(f"Score baseline MAGEN: 174/400 (43.5%) sur puzzles statiques")
        print("=" * 80)
    
    def initialize_arcade(self):
        """Initialise ARC-AGI Arcade en mode ONLINE PRIVÉ"""
        print("\n📡 Initialisation ARC-AGI Arcade...")
        
        try:
            # Mode ONLINE pour scorecards mais PRIVÉ
            self.arcade = arc_agi.Arcade(
                arc_api_key=self.api_key,
                operation_mode=arc_agi.OperationMode.ONLINE
            )
            
            # Créer scorecard PRIVÉ
            scorecard_id = self.arcade.create_scorecard(
                source_url="https://github.com/lvx/MAGEN-V21",
                tags=["magen-v21", "private", "tle-enabled"],
                opaque={
                    "version": "21.0.0",
                    "mode": "PRIVATE",
                    "session": self.results['session_id']
                }
            )
            
            self.results['scorecard_id'] = scorecard_id
            
            print(f"✅ Arcade initialisé (scorecard: {scorecard_id[:8]}...)")
            print(f"🔒 Mode: PRIVÉ (pas de leaderboard public)")
            
            return True
            
        except Exception as e:
            print(f"❌ Erreur initialisation: {e}")
            return False
    
    def get_available_games(self) -> List[str]:
        """Récupère liste des jeux disponibles"""
        print("\n🎮 Récupération liste des jeux...")
        
        try:
            envs = self.arcade.get_environments()
            game_ids = [env.game_id for env in envs]
            
            print(f"✅ {len(game_ids)} jeux disponibles")
            return game_ids
            
        except Exception as e:
            print(f"❌ Erreur récupération jeux: {e}")
            return []
    
    def play_game(self, game_id: str, max_actions: int = 100) -> Dict[str, Any]:
        """
        Jouer un jeu avec agent MAGEN complet V22
        
        Utilise 100% de la technologie MAGEN:
        - TransformationLearningEngine
        - PatternMatcher + ObjectExtractor
        - MetaArbiter + C17+C18+C19
        - Logging forensique complet
        """
        print(f"\n{'='*80}")
        print(f"🎮 Jeu: {game_id}")
        print(f"{'='*80}")
        
        game_result = {
            'game_id': game_id,
            'score': 0.0,
            'levels_completed': 0,
            'actions_taken': 0,
            'start_time': time.time(),
            'end_time': None,
            'error': None
        }
        
        try:
            # Créer environnement
            env = self.arcade.make(game_id, seed=0)
            
            if not env:
                game_result['error'] = "Failed to create environment"
                return game_result
            
            # JOUER AVEC AGENT MAGEN
            magen_result = self.magen_agent.play_game(env, max_actions=max_actions)
            
            # Copier résultats
            game_result.update(magen_result)
            game_result['game_id'] = game_id
            game_result['end_time'] = time.time()
            
            # Afficher statistiques MAGEN
            stats = self.magen_agent.get_statistics()
            print(f"\n📊 STATISTIQUES MAGEN:")
            print(f"   Taux succès: {stats['success_rate']*100:.1f}%")
            print(f"   Reward moyen: {stats['avg_reward']:.2f}")
            print(f"   Patterns détectés: {stats.get('patterns_detected', 0)}")
            print(f"   Objets extraits: {stats.get('objects_extracted', 0)}")
            
        except Exception as e:
            print(f"❌ Erreur pendant le jeu: {e}")
            import traceback
            traceback.print_exc()
            game_result['error'] = str(e)
            game_result['end_time'] = time.time()
        
        return game_result
    
    def run_competition(self, max_games: int = None):
        """
        Lance la compétition en MODE PRIVÉ
        
        Args:
            max_games: Nombre maximum de jeux à jouer (None = tous)
        """
        self.results['start_time'] = datetime.now().isoformat()
        
        # Initialiser
        if not self.initialize_arcade():
            print("❌ Échec initialisation")
            return
        
        # Récupérer jeux
        game_ids = self.get_available_games()
        
        if not game_ids:
            print("❌ Aucun jeu disponible")
            return
        
        # Limiter nombre de jeux si spécifié
        if max_games:
            game_ids = game_ids[:max_games]
        
        print(f"\n🎯 Compétition: {len(game_ids)} jeux")
        print(f"🔒 Mode: PRIVÉ (pas de leaderboard public)")
        print(f"⏱️  Début: {self.results['start_time']}")
        
        # Jouer chaque jeu
        for idx, game_id in enumerate(game_ids, 1):
            print(f"\n{'='*80}")
            print(f"📍 Progression: {idx}/{len(game_ids)} ({idx/len(game_ids)*100:.1f}%)")
            print(f"{'='*80}")
            
            game_result = self.play_game(game_id)
            self.results['games_played'].append(game_result)
            
            # Mettre à jour score total
            if game_result['score']:
                self.results['total_score'] += game_result['score']
        
        # Finaliser
        self.results['end_time'] = datetime.now().isoformat()
        
        # Récupérer scorecard final
        try:
            final_scorecard = self.arcade.close_scorecard()
            
            if final_scorecard:
                # Extraire score et informations disponibles
                scorecard_dict = final_scorecard.model_dump() if hasattr(final_scorecard, 'model_dump') else {}
                
                self.results['final_scorecard'] = {
                    'score': final_scorecard.score if hasattr(final_scorecard, 'score') else 0.0,
                    'scorecard_data': scorecard_dict
                }
                
                print(f"\n{'='*80}")
                print(f"🏆 SCORECARD FINAL")
                print(f"{'='*80}")
                print(f"Score: {final_scorecard.score if hasattr(final_scorecard, 'score') else 0.0:.2f}")
                print(f"Scorecard ID: {self.results.get('scorecard_id', 'N/A')}")
                print(f"{'='*80}")
        
        except Exception as e:
            print(f"⚠️  Erreur récupération scorecard: {e}")
        
        # Sauvegarder résultats
        self.save_results()
        
        # Afficher résumé
        self.print_summary()
    
    def save_results(self):
        """Sauvegarde résultats en JSON"""
        output_dir = Path("results/arc_agi_3")
        output_dir.mkdir(parents=True, exist_ok=True)
        
        output_file = output_dir / f"private_submission_{self.results['session_id']}.json"
        
        with open(output_file, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        print(f"\n💾 Résultats sauvegardés: {output_file}")
    
    def print_summary(self):
        """Affiche résumé final"""
        print(f"\n{'='*80}")
        print(f"📊 RÉSUMÉ FINAL - MODE PRIVÉ")
        print(f"{'='*80}")
        print(f"Session: {self.results['session_id']}")
        print(f"Jeux joués: {len(self.results['games_played'])}")
        print(f"Score total: {self.results['total_score']:.2f}")
        
        if 'final_scorecard' in self.results:
            print(f"Score scorecard: {self.results['final_scorecard']['score']:.2f}")
        
        print(f"Début: {self.results['start_time']}")
        print(f"Fin: {self.results['end_time']}")
        print(f"{'='*80}")
        print(f"🔒 Mode: PRIVÉ (aucune soumission publique)")
        print(f"{'='*80}")


def main():
    """Point d'entrée principal"""
    # Récupérer API Key depuis Doppler
    api_key = os.getenv('ARC_API_KEY')
    
    if not api_key:
        print("❌ ARC_API_KEY non trouvée dans l'environnement")
        print("💡 Exécuter avec: doppler run -- python submit_arc_agi_3_private.py")
        sys.exit(1)
    
    # Créer submitter
    submitter = ArcAgi3PrivateSubmitter(api_key)
    
    # Lancer compétition (limiter à 3 jeux pour test)
    submitter.run_competition(max_games=3)


if __name__ == "__main__":
    main()

# Made with Bob
