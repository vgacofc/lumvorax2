#!/usr/bin/env python3
"""
MAGEN - Parallel World Frame Engine (PWFE)
==========================================

SESSION 50 - RÉVOLUTION COGNITIVE SPATIALE

Concept révolutionnaire inspiré par feedback utilisateur:
"Ce que j'imagine, c'est qu'il doit produire une IMAGE. Et sur cette image,
de la plus grande taille possible, générer à l'intérieur des images, les unes
à côté des autres, toutes les possibilités."

Architecture:
- Frame = Grille de mondes parallèles (N colonnes = N hypothèses)
- Chaque colonne = Une trajectoire complète (action → résultat)
- Évolution temporelle = Frames successives (t0, t1, t2, ...)
- Sélection = Perception globale de l'espace des futurs

Différence fondamentale vs test séquentiel:
❌ AVANT: Tester action A → voir résultat → tester B → etc.
✅ APRÈS: Simuler TOUTES actions en parallèle → comparer → choisir meilleure

Inspiré par:
- Monte Carlo Tree Search (MCTS) spatialisé
- World Models (simulation interne)
- Parallel Rollout Evaluation
- Latent Space Planning
- Perception arcade (voir tous les futurs simultanément)
"""

import numpy as np
from typing import Dict, List, Tuple, Set, Optional, Any, Callable
from dataclasses import dataclass, field
from enum import Enum
from collections import defaultdict
import time


class WorldStatus(Enum):
    """Statut d'un monde parallèle"""
    PROMISING = "promising"      # 🟢 Zone verte - trajectoire prometteuse
    UNCERTAIN = "uncertain"      # 🟡 Zone jaune - incertain
    DEAD_END = "dead_end"        # 🔴 Zone rouge - impasse détectée
    VALIDATED = "validated"      # 🔵 Zone bleue - validé sur train pairs
    INVALIDATED = "invalidated"  # ⚫ Zone noire - invalidé (ne pas répéter)


@dataclass
class WorldNode:
    """
    Nœud représentant un monde parallèle à un instant t
    
    Analogie: Une case dans la grille de mondes
    - Position (x, y) = (temps, hypothèse_id)
    - Contenu = État du monde après transformation
    """
    world_id: int
    hypothesis_id: int
    frame_time: int  # t0, t1, t2, ...
    
    # État du monde
    grid_state: np.ndarray
    program: Any
    
    # Métriques
    confidence: float  # 0.0 (faible) à 1.0 (haute)
    validation_score: float  # Score sur train pairs
    predicted_success: float  # Prédiction succès final
    
    # Statut
    status: WorldStatus
    
    # Historique
    parent_world_id: Optional[int]
    action_taken: Optional[str]
    
    # Raisons
    dead_end_reason: Optional[str]
    invalidation_reason: Optional[str]
    
    # Position spatiale dans visualisation
    position: Tuple[float, float]  # (x=temps, y=hypothèse)


@dataclass
class ParallelFrame:
    """
    Frame = Grille complète de tous les mondes à un instant t
    
    Structure:
    [ Monde 1 ] [ Monde 2 ] [ Monde 3 ] [ Monde 4 ] ...
    
    Chaque case = Un futur hypothétique différent
    """
    frame_id: int
    frame_time: int
    worlds: List[WorldNode]
    
    # Statistiques frame
    promising_count: int = 0
    dead_end_count: int = 0
    validated_count: int = 0
    
    def __post_init__(self):
        """Calculer statistiques après création"""
        self.promising_count = sum(1 for w in self.worlds if w.status == WorldStatus.PROMISING)
        self.dead_end_count = sum(1 for w in self.worlds if w.status == WorldStatus.DEAD_END)
        self.validated_count = sum(1 for w in self.worlds if w.status == WorldStatus.VALIDATED)


@dataclass
class TrajectoryPath:
    """
    Trajectoire complète = Colonne dans la grille de mondes
    
    Représente l'évolution d'une hypothèse à travers le temps:
    Frame 0 → Frame 1 → Frame 2 → ... → Frame N
    """
    trajectory_id: int
    hypothesis_id: int
    world_sequence: List[int]  # Liste de world_ids
    
    # Métriques globales
    total_confidence: float
    final_validation_score: float
    estimated_success_rate: float
    
    # Statut final
    final_status: WorldStatus
    
    # Raison échec (si applicable)
    failure_reason: Optional[str]


class ParallelWorldFrameEngine:
    """
    Moteur de Simulation Parallèle de Mondes
    
    Principe:
    1. Expansion: Générer N hypothèses initiales
    2. Projection: Simuler évolution de CHAQUE hypothèse
    3. Visualisation: Créer grille spatiale de tous les futurs
    4. Pruning: Éliminer impasses AVANT exécution complète
    5. Sélection: Choisir meilleure trajectoire par perception globale
    
    Différence clé vs séquentiel:
    - Séquentiel: O(N) tests successifs
    - Parallèle: O(1) perception globale (tous visibles simultanément)
    """
    
    def __init__(self, 
                 max_hypotheses: int = 50,
                 max_frames: int = 10,
                 pruning_threshold: float = 0.3,
                 verbose: bool = True):
        """
        Initialisation PWFE
        
        Args:
            max_hypotheses: Nombre max d'hypothèses parallèles (colonnes)
            max_frames: Nombre max de frames temporelles (lignes)
            pruning_threshold: Seuil confiance pour pruning précoce
            verbose: Afficher logs détaillés
        """
        self.max_hypotheses = max_hypotheses
        self.max_frames = max_frames
        self.pruning_threshold = pruning_threshold
        self.verbose = verbose
        
        # Grille de mondes (frames × hypothèses)
        self.frames: List[ParallelFrame] = []
        self.worlds: Dict[int, WorldNode] = {}
        self.next_world_id = 0
        
        # Trajectoires (colonnes)
        self.trajectories: Dict[int, TrajectoryPath] = {}
        
        # Zones spatiales
        self.promising_zone: Set[int] = set()  # 🟢
        self.dead_end_zone: Set[int] = set()   # 🔴
        self.uncertain_zone: Set[int] = set()  # 🟡
        self.validated_zone: Set[int] = set()  # 🔵
        self.invalidated_zone: Set[int] = set()  # ⚫
        
        # Cache trajectoires invalidées (mémoire persistante)
        self.dead_trajectory_cache: Set[str] = set()
        
        # Statistiques
        self.total_worlds_generated = 0
        self.early_eliminations = 0
        self.cache_hits = 0
        
        if self.verbose:
            print(f"🌍 Parallel World Frame Engine initialisé")
            print(f"   Max hypothèses: {max_hypotheses}")
            print(f"   Max frames: {max_frames}")
            print(f"   Pruning threshold: {pruning_threshold}")
    
    def generate_parallel_worlds(self,
                                 input_grid: np.ndarray,
                                 train_pairs: List[Tuple[np.ndarray, np.ndarray]],
                                 program_generators: List[Any],
                                 invariants: Any) -> Dict[str, Any]:
        """
        Génère la grille complète de mondes parallèles
        
        CORRECTION SESSION 51 #3: Accepte maintenant programmes déjà générés
        
        PHASE 1: EXPANSION
        Créer Frame 0 avec N hypothèses initiales
        
        Args:
            input_grid: Grille d'entrée test
            train_pairs: Paires d'entraînement
            program_generators: Liste de programmes déjà générés (pas de générateurs)
            invariants: Profil d'invariants
            
        Returns:
            Résultats simulation complète
        """
        if self.verbose:
            print(f"\n🚀 GÉNÉRATION GRILLE MONDES PARALLÈLES")
            print(f"{'='*70}")
        
        start_time = time.time()
        
        # PHASE 1: Expansion initiale (Frame 0)
        frame0 = self._expand_initial_hypotheses(
            input_grid, train_pairs, program_generators, invariants
        )
        self.frames.append(frame0)
        
        if self.verbose:
            print(f"\n📊 Frame 0 créée:")
            print(f"   Mondes générés: {len(frame0.worlds)}")
            print(f"   Prometteurs: {frame0.promising_count} 🟢")
            print(f"   Incertains: {len(frame0.worlds) - frame0.promising_count - frame0.dead_end_count} 🟡")
            print(f"   Impasses: {frame0.dead_end_count} 🔴")
        
        # PHASE 2: Projection parallèle (Frames 1..N)
        for frame_time in range(1, self.max_frames):
            frame = self._project_parallel_evolution(
                frame_time, train_pairs, invariants
            )
            
            if frame is None or len(frame.worlds) == 0:
                break  # Plus de mondes à explorer
            
            self.frames.append(frame)
            
            if self.verbose:
                print(f"\n📊 Frame {frame_time} créée:")
                print(f"   Mondes actifs: {len(frame.worlds)}")
                print(f"   Prometteurs: {frame.promising_count} 🟢")
                print(f"   Validés: {frame.validated_count} 🔵")
                print(f"   Impasses: {frame.dead_end_count} 🔴")
            
            # Arrêt précoce si solution validée trouvée
            if frame.validated_count > 0:
                if self.verbose:
                    print(f"   ✅ Solution validée trouvée!")
                break
        
        # PHASE 3: Construction trajectoires
        self._build_trajectories()
        
        # PHASE 4: Sélection meilleure trajectoire
        best_trajectory = self._select_best_trajectory()
        
        elapsed = time.time() - start_time
        
        if self.verbose:
            print(f"\n{'='*70}")
            print(f"✅ SIMULATION COMPLÈTE")
            print(f"   Temps: {elapsed:.2f}s")
            print(f"   Frames générées: {len(self.frames)}")
            print(f"   Mondes totaux: {self.total_worlds_generated}")
            print(f"   Éliminations précoces: {self.early_eliminations}")
            print(f"   Cache hits: {self.cache_hits}")
            print(f"{'='*70}")
        
        return {
            'success': best_trajectory is not None and best_trajectory.final_status == WorldStatus.VALIDATED,
            'best_trajectory': best_trajectory,
            'total_frames': len(self.frames),
            'total_worlds': self.total_worlds_generated,
            'early_eliminations': self.early_eliminations,
            'cache_hits': self.cache_hits,
            'elapsed_time': elapsed,
            'zones': {
                'promising': len(self.promising_zone),
                'validated': len(self.validated_zone),
                'dead_end': len(self.dead_end_zone),
                'invalidated': len(self.invalidated_zone)
            }
        }
    
    def _expand_initial_hypotheses(self,
                                   input_grid: np.ndarray,
                                   train_pairs: List[Tuple[np.ndarray, np.ndarray]],
                                   programs: List[Any],
                                   invariants: Any) -> ParallelFrame:
        """
        PHASE 1: Expansion initiale
        
        CORRECTION SESSION 51 #3: Accepte programmes déjà générés
        
        Génère Frame 0 avec N hypothèses parallèles à partir de programmes existants
        """
        worlds = []
        hypothesis_id = 0
        
        # CORRECTION: Itérer directement sur programmes (pas de .generate())
        for program in programs:
            if hypothesis_id >= self.max_hypotheses:
                break
            
            try:
                # Vérifier cache trajectoires invalidées
                program_hash = self._hash_program(program)
                if program_hash in self.dead_trajectory_cache:
                    self.cache_hits += 1
                    continue  # Skip, déjà invalidé
                
                # Créer monde initial
                world = self._create_world(
                    hypothesis_id=hypothesis_id,
                    frame_time=0,
                    grid_state=input_grid.copy(),
                    program=program,
                    parent_world_id=None,
                    action_taken=None
                )
                
                # Prédire trajectoire
                self._predict_world_trajectory(world, train_pairs, invariants)
                
                worlds.append(world)
                self.worlds[world.world_id] = world
                
                # Classer dans zones
                self._classify_world(world)
                
                hypothesis_id += 1
                self.total_worlds_generated += 1
            
            except Exception as e:
                if self.verbose:
                    print(f"    ⚠️  Erreur programme {hypothesis_id}: {e}")
                continue
        
        return ParallelFrame(
            frame_id=0,
            frame_time=0,
            worlds=worlds
        )
    
    def _project_parallel_evolution(self,
                                    frame_time: int,
                                    train_pairs: List[Tuple[np.ndarray, np.ndarray]],
                                    invariants: Any) -> Optional[ParallelFrame]:
        """
        PHASE 2: Projection parallèle
        
        Fait évoluer TOUS les mondes prometteurs simultanément
        """
        previous_frame = self.frames[-1]
        
        # Filtrer mondes à faire évoluer (seulement prometteurs et incertains)
        active_worlds = [w for w in previous_frame.worlds 
                        if w.status in [WorldStatus.PROMISING, WorldStatus.UNCERTAIN]]
        
        if not active_worlds:
            return None  # Plus de mondes actifs
        
        new_worlds = []
        
        for world in active_worlds:
            # Simuler évolution
            try:
                # Exécuter programme sur premier train pair
                first_input, first_output = train_pairs[0]
                result = world.program.execute(first_input)
                
                if result is None:
                    # Monde mort
                    world.status = WorldStatus.DEAD_END
                    world.dead_end_reason = "execution_failed"
                    self.dead_end_zone.add(world.world_id)
                    self.early_eliminations += 1
                    continue
                
                # Créer nouveau monde avec résultat
                new_world = self._create_world(
                    hypothesis_id=world.hypothesis_id,
                    frame_time=frame_time,
                    grid_state=result,
                    program=world.program,
                    parent_world_id=world.world_id,
                    action_taken="execute"
                )
                
                # Valider sur train pairs
                self._validate_world(new_world, train_pairs)
                
                new_worlds.append(new_world)
                self.worlds[new_world.world_id] = new_world
                self._classify_world(new_world)
                self.total_worlds_generated += 1
            
            except Exception as e:
                world.status = WorldStatus.DEAD_END
                world.dead_end_reason = f"evolution_error: {str(e)}"
                self.dead_end_zone.add(world.world_id)
                self.early_eliminations += 1
        
        if not new_worlds:
            return None
        
        return ParallelFrame(
            frame_id=frame_time,
            frame_time=frame_time,
            worlds=new_worlds
        )
    
    def _create_world(self,
                     hypothesis_id: int,
                     frame_time: int,
                     grid_state: np.ndarray,
                     program: Any,
                     parent_world_id: Optional[int],
                     action_taken: Optional[str]) -> WorldNode:
        """Crée un nouveau monde"""
        world_id = self.next_world_id
        self.next_world_id += 1
        
        return WorldNode(
            world_id=world_id,
            hypothesis_id=hypothesis_id,
            frame_time=frame_time,
            grid_state=grid_state,
            program=program,
            confidence=0.5,  # Initial
            validation_score=0.0,
            predicted_success=0.0,
            status=WorldStatus.UNCERTAIN,
            parent_world_id=parent_world_id,
            action_taken=action_taken,
            dead_end_reason=None,
            invalidation_reason=None,
            position=(frame_time, hypothesis_id)
        )
    
    def _predict_world_trajectory(self,
                                  world: WorldNode,
                                  train_pairs: List[Tuple[np.ndarray, np.ndarray]],
                                  invariants: Any):
        """
        ROLLBACK SESSION 52: Retour validation séquentielle (Test V5)
        
        Valide UNIQUEMENT le premier train pair (validation rapide)
        Évite les faux négatifs de la validation globale stricte
        """
        try:
            # VALIDATION SÉQUENTIELLE sur PREMIER train pair uniquement
            first_input, first_output = train_pairs[0]
            
            result = world.program.execute(first_input)
            
            if result is None:
                world.status = WorldStatus.DEAD_END
                world.dead_end_reason = "execution_returned_none"
                world.confidence = 0.0
                world.validation_score = 0.0
                return
            
            # Vérifier correspondance exacte
            if result.shape != first_output.shape:
                world.status = WorldStatus.DEAD_END
                world.dead_end_reason = f"shape_mismatch_{result.shape}_vs_{first_output.shape}"
                world.confidence = 0.0
                world.validation_score = 0.0
                return
            
            if np.array_equal(result, first_output):
                # ✅ Premier pair validé → VALIDATED
                world.status = WorldStatus.VALIDATED
                world.validation_score = 1.0
                world.confidence = 1.0
                world.predicted_success = 1.0
            else:
                # Premier pair échoué → DEAD_END
                world.status = WorldStatus.DEAD_END
                world.dead_end_reason = "first_pair_mismatch"
                world.confidence = 0.0
                world.validation_score = 0.0
        
        except Exception as e:
            world.status = WorldStatus.DEAD_END
            world.dead_end_reason = f"prediction_error: {str(e)}"
            world.confidence = 0.0
            world.validation_score = 0.0
    
    def _validate_world(self,
                       world: WorldNode,
                       train_pairs: List[Tuple[np.ndarray, np.ndarray]]):
        """Valide un monde sur TOUS les train pairs"""
        total_score = 0.0
        valid_count = 0
        
        for inp, out in train_pairs:
            try:
                result = world.program.execute(inp)
                
                if result is None:
                    continue
                
                if result.shape == out.shape:
                    matches = np.sum(result == out)
                    total = out.size
                    score = matches / total if total > 0 else 0.0
                    total_score += score
                    valid_count += 1
            
            except:
                continue
        
        if valid_count > 0:
            world.validation_score = total_score / valid_count
            world.confidence = world.validation_score
            
            # Statut basé sur validation globale
            if world.validation_score >= 0.95:
                world.status = WorldStatus.VALIDATED
            elif world.validation_score >= 0.7:
                world.status = WorldStatus.PROMISING
            elif world.validation_score >= 0.4:
                world.status = WorldStatus.UNCERTAIN
            else:
                world.status = WorldStatus.DEAD_END
                world.dead_end_reason = "low_validation_score"
        else:
            world.status = WorldStatus.DEAD_END
            world.dead_end_reason = "validation_failed"
            world.confidence = 0.0
    
    def _classify_world(self, world: WorldNode):
        """Classe un monde dans les zones spatiales"""
        if world.status == WorldStatus.PROMISING:
            self.promising_zone.add(world.world_id)
        elif world.status == WorldStatus.VALIDATED:
            self.validated_zone.add(world.world_id)
        elif world.status == WorldStatus.DEAD_END:
            self.dead_end_zone.add(world.world_id)
            self.early_eliminations += 1
            
            # Ajouter au cache
            program_hash = self._hash_program(world.program)
            self.dead_trajectory_cache.add(program_hash)
        elif world.status == WorldStatus.UNCERTAIN:
            self.uncertain_zone.add(world.world_id)
    
    def _build_trajectories(self):
        """Construit les trajectoires (colonnes) à partir des mondes"""
        # Grouper mondes par hypothesis_id
        hypothesis_worlds: Dict[int, List[WorldNode]] = defaultdict(list)
        
        for world in self.worlds.values():
            hypothesis_worlds[world.hypothesis_id].append(world)
        
        # Créer trajectoire pour chaque hypothèse
        for hypothesis_id, worlds in hypothesis_worlds.items():
            # Trier par frame_time
            worlds.sort(key=lambda w: w.frame_time)
            
            # Dernier monde = état final
            final_world = worlds[-1]
            
            trajectory = TrajectoryPath(
                trajectory_id=hypothesis_id,
                hypothesis_id=hypothesis_id,
                world_sequence=[w.world_id for w in worlds],
                total_confidence=final_world.confidence,
                final_validation_score=final_world.validation_score,
                estimated_success_rate=final_world.predicted_success,
                final_status=final_world.status,
                failure_reason=final_world.dead_end_reason
            )
            
            self.trajectories[hypothesis_id] = trajectory
    
    def _select_best_trajectory(self) -> Optional[TrajectoryPath]:
        """Sélectionne la meilleure trajectoire par perception globale"""
        if not self.trajectories:
            return None
        
        # Filtrer trajectoires validées
        validated = [t for t in self.trajectories.values() 
                    if t.final_status == WorldStatus.VALIDATED]
        
        if validated:
            # Choisir meilleure validée
            return max(validated, key=lambda t: t.final_validation_score)
        
        # Sinon, choisir meilleure prometteuse
        promising = [t for t in self.trajectories.values()
                    if t.final_status == WorldStatus.PROMISING]
        
        if promising:
            return max(promising, key=lambda t: t.total_confidence)
        
        # Dernière chance: meilleure incertaine
        uncertain = [t for t in self.trajectories.values()
                    if t.final_status == WorldStatus.UNCERTAIN]
        
        if uncertain:
            return max(uncertain, key=lambda t: t.total_confidence)
        
        return None
    
    def _hash_program(self, program: Any) -> str:
        """Calcule hash d'un programme pour cache"""
        try:
            return str(hash(str(program)))
        except:
            return str(id(program))
    
    def visualize_grid(self) -> str:
        """Génère visualisation ASCII de la grille de mondes"""
        lines = []
        lines.append("=" * 80)
        lines.append("PARALLEL WORLD FRAME ENGINE - GRILLE DE MONDES")
        lines.append("=" * 80)
        
        for frame in self.frames:
            lines.append(f"\nFrame {frame.frame_time} (t={frame.frame_time}):")
            lines.append(f"  Mondes: {len(frame.worlds)} | "
                        f"🟢 {frame.promising_count} | "
                        f"🔵 {frame.validated_count} | "
                        f"🔴 {frame.dead_end_count}")
            
            # Afficher top 3 mondes
            top_worlds = sorted(frame.worlds, key=lambda w: w.confidence, reverse=True)[:3]
            for i, world in enumerate(top_worlds, 1):
                status_icon = {
                    WorldStatus.PROMISING: "🟢",
                    WorldStatus.VALIDATED: "🔵",
                    WorldStatus.DEAD_END: "🔴",
                    WorldStatus.UNCERTAIN: "🟡",
                    WorldStatus.INVALIDATED: "⚫"
                }.get(world.status, "⚪")
                
                lines.append(f"    {i}. {status_icon} Monde {world.world_id} "
                           f"(H{world.hypothesis_id}): "
                           f"conf={world.confidence:.2f}, "
                           f"val={world.validation_score:.2f}")
        
        lines.append("\n" + "=" * 80)
        lines.append(f"STATISTIQUES GLOBALES:")
        lines.append(f"  Mondes générés: {self.total_worlds_generated}")
        lines.append(f"  Éliminations précoces: {self.early_eliminations}")
        lines.append(f"  Cache hits: {self.cache_hits}")
        lines.append(f"  Zones: 🟢 {len(self.promising_zone)} | "
                    f"🔵 {len(self.validated_zone)} | "
                    f"🔴 {len(self.dead_end_zone)}")
        lines.append("=" * 80)
        
        return "\n".join(lines)


# Made with Bob - Session 50 - Parallel World Frame Engine