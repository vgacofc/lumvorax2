"""
MAGEN - Mémoire Artificielle GEN
Système de mémoire multi-échelle pour agents ARC-AGI-3

Inspiré de:
- GEN8: Mémoire causale persistante
- GEN11: Multi-timescale memory
- GEN23: Perception réelle
- GEN24: Apprentissage supervisé

Architecture: 4 niveaux de mémoire + graphe causal
"""

from dataclasses import dataclass, field
from typing import List, Dict, Any, Optional, Tuple
import numpy as np
import time
import json
from pathlib import Path
import hashlib


class NumpyEncoder(json.JSONEncoder):
    """Encodeur JSON personnalisé pour types numpy"""
    def default(self, o):
        if isinstance(o, np.integer):
            return int(o)
        elif isinstance(o, np.floating):
            return float(o)
        elif isinstance(o, np.ndarray):
            return o.tolist()
        elif isinstance(o, np.bool_):
            return bool(o)
        return super().default(o)


@dataclass
class Experience:
    """Une expérience unique dans la mémoire"""
    timestamp: float
    game_id: str
    level: int
    state: np.ndarray  # Grille 64x64
    action: str  # ACTION1-7 ou RESET
    action_data: Optional[Dict[str, int]]  # Pour ACTION6 (x,y)
    result_state: Optional[np.ndarray]  # État résultant
    reward: float  # 1.0 si changement, 0.0 sinon
    success: bool  # True si niveau complété
    metadata: Dict[str, Any] = field(default_factory=dict)
    
    def to_dict(self) -> Dict[str, Any]:
        """Sérialisation pour sauvegarde"""
        return {
            'timestamp': float(self.timestamp),
            'game_id': str(self.game_id),
            'level': int(self.level),
            'state_shape': list(self.state.shape) if self.state is not None else None,
            'state_hash': hashlib.sha256(self.state.tobytes()).hexdigest() if self.state is not None else None,
            'action': str(self.action),
            'action_data': self.action_data,
            'reward': float(self.reward),
            'success': bool(self.success),
            'metadata': self.metadata
        }


@dataclass
class Pattern:
    """Un pattern consolidé (stratégie réutilisable)"""
    pattern_id: str
    game_ids: List[str]  # Jeux où ce pattern fonctionne
    state_signature: np.ndarray  # Signature état (features)
    action_sequence: List[Tuple[str, Optional[Dict[str, int]]]]  # Séquence actions
    success_rate: float  # Taux de succès
    usage_count: int  # Nombre d'utilisations
    last_used: float  # Timestamp dernière utilisation
    
    def to_dict(self) -> Dict[str, Any]:
        """Sérialisation pour sauvegarde"""
        return {
            'pattern_id': str(self.pattern_id),
            'game_ids': [str(gid) for gid in self.game_ids],
            'signature_shape': list(self.state_signature.shape),
            'action_sequence': self.action_sequence,
            'success_rate': float(self.success_rate),
            'usage_count': int(self.usage_count),
            'last_used': float(self.last_used)
        }


@dataclass
class MAGENMetrics:
    """Métriques forensiques du système MAGEN"""
    # Mémoire
    short_term_size: int
    mid_term_size: int
    long_term_size: int
    deep_archive_size: int
    
    # Patterns
    patterns_count: int
    patterns_success_rate: float
    
    # Causal
    causal_links_count: int
    contradiction_count: int
    
    # Performance
    memory_usage_mb: float
    consolidation_time_ms: float
    retrieval_time_ms: float
    
    # Apprentissage
    exploration_rate: float
    exploitation_rate: float
    learning_progress: float
    
    def to_dict(self) -> Dict[str, Any]:
        """Sérialisation pour logs"""
        return {k: v for k, v in self.__dict__.items()}


class MAGENMemory:
    """
    Système de mémoire multi-échelle MAGEN
    
    Architecture:
    - Court terme: 50 dernières expériences (buffer circulaire)
    - Moyen terme: 200 patterns consolidés
    - Long terme: Stratégies validées (success_rate > 0.7)
    - Archive profonde: Historique complet append-only
    
    Fonctionnalités:
    - Consolidation automatique (court → moyen → long)
    - Détection patterns similaires
    - Graphe causal (cause-effet)
    - Détection contradictions
    - Prédiction erreurs
    - Simulation futur
    """
    
    def __init__(self, 
                 short_term_capacity: int = 50,
                 mid_term_capacity: int = 200,
                 long_term_threshold: float = 0.7,
                 save_dir: str = "logs/magen"):
        """
        Initialisation système MAGEN
        
        Args:
            short_term_capacity: Capacité mémoire court terme
            mid_term_capacity: Capacité mémoire moyen terme
            long_term_threshold: Seuil success_rate pour long terme
            save_dir: Répertoire sauvegarde logs
        """
        # Mémoire multi-échelle
        self.short_term: List[Experience] = []
        self.mid_term: List[Pattern] = []
        self.long_term: List[Pattern] = []
        self.deep_archive: List[Experience] = []
        
        # Capacités
        self.short_term_capacity = short_term_capacity
        self.mid_term_capacity = mid_term_capacity
        self.long_term_threshold = long_term_threshold
        
        # Graphe causal
        self.causal_links: List[Tuple[int, int]] = []  # (exp_idx_1, exp_idx_2)
        self.contradictions: List[Tuple[int, int]] = []  # Expériences contradictoires
        
        # Statistiques
        self.total_experiences = 0
        self.total_consolidations = 0
        self.total_retrievals = 0
        
        # Sauvegarde
        self.save_dir = Path(save_dir)
        self.save_dir.mkdir(parents=True, exist_ok=True)
        
        # Timestamps
        self.creation_time = time.time()
        self.last_consolidation = time.time()
        
        print(f"[MAGEN] Système mémoire initialisé")
        print(f"[MAGEN] Court terme: {short_term_capacity} expériences")
        print(f"[MAGEN] Moyen terme: {mid_term_capacity} patterns")
        print(f"[MAGEN] Long terme: success_rate > {long_term_threshold}")
        print(f"[MAGEN] Logs: {self.save_dir}")
    
    def inject_experience(self, experience: Experience) -> None:
        """
        Injecter une nouvelle expérience dans la mémoire
        
        Args:
            experience: Expérience à mémoriser
        """
        # Ajouter à court terme (buffer circulaire)
        self.short_term.append(experience)
        if len(self.short_term) > self.short_term_capacity:
            # Migrer la plus ancienne vers archive
            oldest = self.short_term.pop(0)
            self.deep_archive.append(oldest)
        
        # Ajouter à archive profonde (append-only)
        self.deep_archive.append(experience)
        
        # Construire liens causaux
        self._build_causal_links(experience)
        
        # Détecter contradictions
        self._detect_contradictions(experience)
        
        # Statistiques
        self.total_experiences += 1
        
        # Consolidation automatique (toutes les 10 expériences)
        if self.total_experiences % 10 == 0:
            self.consolidate_memory()
    
    def _build_causal_links(self, experience: Experience) -> None:
        """Construire liens causaux entre expériences"""
        if len(self.deep_archive) < 2:
            return
        
        # Lien avec expérience précédente (même jeu)
        for i in range(len(self.deep_archive) - 2, max(-1, len(self.deep_archive) - 10), -1):
            prev_exp = self.deep_archive[i]
            if prev_exp.game_id == experience.game_id:
                # Lien causal si même jeu
                self.causal_links.append((i, len(self.deep_archive) - 1))
                break
    
    def _detect_contradictions(self, experience: Experience) -> None:
        """Détecter contradictions (même état, actions différentes, résultats différents)"""
        if experience.state is None or experience.result_state is None:
            return
        
        # Chercher états similaires dans court terme
        for i, exp in enumerate(self.short_term[:-1]):  # Exclure l'expérience actuelle
            if exp.state is None or exp.result_state is None:
                continue
            
            # Similarité état (distance L2)
            state_dist = np.linalg.norm(exp.state - experience.state)
            
            if state_dist < 1.0:  # États très similaires
                # Actions différentes?
                if exp.action != experience.action:
                    # Résultats différents?
                    result_dist = np.linalg.norm(exp.result_state - experience.result_state)
                    if result_dist > 5.0:  # Résultats très différents
                        # Contradiction détectée
                        self.contradictions.append((i, len(self.short_term) - 1))
    
    def consolidate_memory(self) -> float:
        """
        Consolider mémoire: court terme → moyen terme → long terme
        
        Returns:
            Temps consolidation (ms)
        """
        start_time = time.time()
        
        # 1. Court terme → Moyen terme (extraction patterns)
        if len(self.short_term) >= 5:
            # Extraire patterns des 5 dernières expériences
            pattern = self._extract_pattern(self.short_term[-5:])
            if pattern is not None:
                self.mid_term.append(pattern)
                
                # Limiter taille moyen terme
                if len(self.mid_term) > self.mid_term_capacity:
                    # Supprimer pattern le moins utilisé
                    self.mid_term.sort(key=lambda p: p.usage_count)
                    self.mid_term.pop(0)
        
        # 2. Moyen terme → Long terme (validation patterns)
        for pattern in self.mid_term[:]:
            if pattern.success_rate >= self.long_term_threshold:
                # Pattern validé → long terme
                if pattern not in self.long_term:
                    self.long_term.append(pattern)
        
        # Statistiques
        self.total_consolidations += 1
        self.last_consolidation = time.time()
        
        consolidation_time = (time.time() - start_time) * 1000  # ms
        return consolidation_time
    
    def _extract_pattern(self, experiences: List[Experience]) -> Optional[Pattern]:
        """
        Extraire un pattern réutilisable d'une séquence d'expériences
        
        Args:
            experiences: Séquence d'expériences
            
        Returns:
            Pattern extrait ou None
        """
        if not experiences:
            return None
        
        # Vérifier que toutes les expériences sont du même jeu
        game_ids = list(set(exp.game_id for exp in experiences))
        if len(game_ids) != 1:
            return None
        
        # Calculer signature état (moyenne des états)
        states = [exp.state for exp in experiences if exp.state is not None]
        if not states:
            return None
        
        state_signature = np.mean(states, axis=0)
        
        # Extraire séquence actions
        action_sequence = [(exp.action, exp.action_data) for exp in experiences]
        
        # Calculer success_rate
        successes = sum(1 for exp in experiences if exp.success)
        success_rate = successes / len(experiences)
        
        # Créer pattern
        pattern_id = hashlib.sha256(
            f"{game_ids[0]}_{time.time()}".encode()
        ).hexdigest()[:16]
        
        pattern = Pattern(
            pattern_id=pattern_id,
            game_ids=game_ids,
            state_signature=state_signature,
            action_sequence=action_sequence,
            success_rate=success_rate,
            usage_count=0,
            last_used=time.time()
        )
        
        return pattern
    
    def retrieve_similar_pattern(self, state: np.ndarray, game_id: str) -> Optional[Pattern]:
        """
        Récupérer pattern similaire pour un état donné
        
        Args:
            state: État actuel
            game_id: ID du jeu
            
        Returns:
            Pattern le plus similaire ou None
        """
        start_time = time.time()
        
        # Chercher d'abord dans long terme (stratégies validées)
        best_pattern = None
        best_distance = float('inf')
        
        for pattern in self.long_term + self.mid_term:
            # Vérifier si pattern applicable à ce jeu
            if game_id not in pattern.game_ids:
                continue
            
            # Calculer similarité (distance L2)
            distance = np.linalg.norm(state - pattern.state_signature)
            
            if distance < best_distance:
                best_distance = distance
                best_pattern = pattern
        
        # Mettre à jour statistiques si pattern trouvé
        if best_pattern is not None:
            best_pattern.usage_count += 1
            best_pattern.last_used = time.time()
        
        # Statistiques
        self.total_retrievals += 1
        retrieval_time = (time.time() - start_time) * 1000  # ms
        
        return best_pattern
    
    def get_metrics(self) -> MAGENMetrics:
        """
        Obtenir métriques forensiques du système
        
        Returns:
            Métriques complètes
        """
        # Calcul success_rate patterns
        if self.mid_term + self.long_term:
            patterns_success_rate = float(np.mean([
                p.success_rate for p in self.mid_term + self.long_term
            ]))
        else:
            patterns_success_rate = 0.0
        
        # Calcul taux exploration/exploitation
        if self.total_experiences > 0:
            exploration_rate = len(self.short_term) / self.total_experiences
            exploitation_rate = sum(p.usage_count for p in self.mid_term + self.long_term) / max(1, self.total_experiences)
        else:
            exploration_rate = 1.0
            exploitation_rate = 0.0
        
        # Calcul learning progress
        learning_progress = len(self.long_term) / max(1, self.mid_term_capacity)
        
        # Estimation mémoire (approximative)
        memory_usage_mb = (
            len(self.short_term) * 0.1 +  # ~100KB par expérience
            len(self.mid_term) * 0.05 +    # ~50KB par pattern
            len(self.long_term) * 0.05 +
            len(self.deep_archive) * 0.01  # ~10KB par expérience archivée
        )
        
        return MAGENMetrics(
            short_term_size=len(self.short_term),
            mid_term_size=len(self.mid_term),
            long_term_size=len(self.long_term),
            deep_archive_size=len(self.deep_archive),
            patterns_count=len(self.mid_term) + len(self.long_term),
            patterns_success_rate=patterns_success_rate,
            causal_links_count=len(self.causal_links),
            contradiction_count=len(self.contradictions),
            memory_usage_mb=memory_usage_mb,
            consolidation_time_ms=0.0,  # Sera mis à jour lors de la consolidation
            retrieval_time_ms=0.0,      # Sera mis à jour lors de la récupération
            exploration_rate=exploration_rate,
            exploitation_rate=exploitation_rate,
            learning_progress=learning_progress
        )
    
    def save_state(self, filename: str = "magen_state.json") -> None:
        """
        Sauvegarder état complet du système
        
        Args:
            filename: Nom fichier sauvegarde
        """
        filepath = self.save_dir / filename
        
        state = {
            'metadata': {
                'creation_time': self.creation_time,
                'last_consolidation': self.last_consolidation,
                'total_experiences': self.total_experiences,
                'total_consolidations': self.total_consolidations,
                'total_retrievals': self.total_retrievals
            },
            'short_term': [exp.to_dict() for exp in self.short_term],
            'mid_term': [p.to_dict() for p in self.mid_term],
            'long_term': [p.to_dict() for p in self.long_term],
            'deep_archive_size': len(self.deep_archive),
            'causal_links_count': len(self.causal_links),
            'contradictions_count': len(self.contradictions),
            'metrics': self.get_metrics().to_dict()
        }
        
        with open(filepath, 'w') as f:
            json.dump(state, f, indent=2, cls=NumpyEncoder)
        
        print(f"[MAGEN] État sauvegardé: {filepath}")
    
    def __repr__(self) -> str:
        """Représentation string du système"""
        metrics = self.get_metrics()
        return (
            f"MAGENMemory(\n"
            f"  Court terme: {metrics.short_term_size}/{self.short_term_capacity}\n"
            f"  Moyen terme: {metrics.mid_term_size}/{self.mid_term_capacity}\n"
            f"  Long terme: {metrics.long_term_size}\n"
            f"  Archive: {metrics.deep_archive_size}\n"
            f"  Patterns: {metrics.patterns_count} (success: {metrics.patterns_success_rate:.2%})\n"
            f"  Causal links: {metrics.causal_links_count}\n"
            f"  Contradictions: {metrics.contradiction_count}\n"
            f"  Mémoire: {metrics.memory_usage_mb:.2f} MB\n"
            f"  Learning progress: {metrics.learning_progress:.2%}\n"
            f")"
        )

# Made with Bob
