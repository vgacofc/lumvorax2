#!/usr/bin/env python3
"""
Layer 3: Attention Mechanism
============================

Mécanisme d'attention sélective pour MAGEN V2.
Focus sur régions pertinentes de l'espace d'observation.

Principe: Un agent ne peut pas traiter toute l'information simultanément.
L'attention sélectionne CE QUI EST IMPORTANT dans le contexte actuel.

Basé sur:
- Visual Attention Theory (Posner & Petersen, 1990)
- Saliency Maps (Itti & Koch, 2001)
- Feature Integration Theory (Treisman & Gelade, 1980)

Author: Bob (Assistant IA)
Date: 2026-06-18
Version: V2.0
"""

import numpy as np
from typing import Dict, List, Optional, Tuple, Any
from dataclasses import dataclass, field
from scipy.ndimage import gaussian_filter
import time

@dataclass
class AttentionMap:
    """
    Carte d'attention spatiale.
    
    Indique où l'agent doit focaliser son attention.
    Valeurs élevées = régions importantes.
    """
    saliency_map: np.ndarray  # Carte saillance (0-1)
    focus_regions: List[Tuple[int, int, int, int]]  # (row, col, height, width)
    attention_weights: np.ndarray  # Poids attention par région
    computation_time_ms: float
    method_used: str  # 'bottom_up', 'top_down', 'combined'
    
    @property
    def attention_map(self) -> np.ndarray:
        """Alias pour compatibilité avec tests: attention_map → attention_weights"""
        return self.attention_weights
    
    def to_dict(self) -> Dict:
        return {
            'saliency_map_shape': self.saliency_map.shape,
            'num_focus_regions': len(self.focus_regions),
            'attention_weights_shape': self.attention_weights.shape,
            'computation_time_ms': self.computation_time_ms,
            'method_used': self.method_used
        }


@dataclass
class AttentionContext:
    """
    Contexte pour guidage attention.
    
    Contient informations pour attention top-down (guidée par tâche).
    """
    goal_position: Optional[Tuple[int, int]] = None
    target_colors: List[int] = field(default_factory=list)
    target_patterns: List[str] = field(default_factory=list)
    avoid_regions: List[Tuple[int, int, int, int]] = field(default_factory=list)
    priority_level: float = 0.5  # 0-1
    
    def to_dict(self) -> Dict:
        return {
            'has_goal': self.goal_position is not None,
            'num_target_colors': len(self.target_colors),
            'num_target_patterns': len(self.target_patterns),
            'num_avoid_regions': len(self.avoid_regions),
            'priority_level': self.priority_level
        }


class AttentionMechanism:
    """
    Layer 3: Attention Mechanism
    
    Implémente attention sélective pour focaliser sur régions pertinentes.
    
    Deux modes:
    1. Bottom-up: Attention guidée par stimuli (saillance visuelle)
    2. Top-down: Attention guidée par tâche (objectifs)
    
    Combine les deux pour attention optimale.
    """
    
    def __init__(
        self,
        min_region_size: int = 3,
        max_focus_regions: int = 5,
        saliency_threshold: float = 0.5,
        gaussian_sigma: float = 1.0,
        forensic_logger=None
    ):
        """
        Initialise le mécanisme d'attention.
        
        Args:
            min_region_size: Taille minimale région focus
            max_focus_regions: Nombre max régions simultanées
            saliency_threshold: Seuil saillance (0-1)
            gaussian_sigma: Sigma pour lissage gaussien
            forensic_logger: Logger forensique
        """
        self.min_region_size = min_region_size
        self.max_focus_regions = max_focus_regions
        self.saliency_threshold = saliency_threshold
        self.gaussian_sigma = gaussian_sigma
        self.forensic_logger = forensic_logger
        
        # Cache attention maps
        self.attention_cache: Dict[str, AttentionMap] = {}
        
        # Statistiques
        self.total_computations = 0
        self.cache_hits = 0
        self.avg_computation_time = 0.0
        
        self._log_forensic("ATTENTION_MECHANISM_INIT", {
            "min_region_size": min_region_size,
            "max_focus_regions": max_focus_regions,
            "saliency_threshold": saliency_threshold
        })
    
    def compute_attention(
        self,
        state: np.ndarray,
        context: Optional[AttentionContext] = None,
        use_cache: bool = True
    ) -> AttentionMap:
        """
        Calcule la carte d'attention pour un état donné.
        
        Combine attention bottom-up et top-down.
        
        Args:
            state: État actuel (grille)
            context: Contexte pour attention top-down
            use_cache: Utiliser cache si disponible
            
        Returns:
            AttentionMap avec régions focus
        """
        start_time = time.time()
        
        # Vérifier cache
        state_hash = self._hash_state(state)
        if use_cache and state_hash in self.attention_cache:
            self.cache_hits += 1
            return self.attention_cache[state_hash]
        
        # Attention bottom-up (guidée par stimuli)
        bottom_up_map = self._compute_bottom_up_attention(state)
        
        # Attention top-down (guidée par tâche)
        if context:
            top_down_map = self._compute_top_down_attention(state, context)
            # Combiner les deux
            combined_map = self._combine_attention_maps(
                bottom_up_map, top_down_map, context.priority_level
            )
            method = 'combined'
        else:
            combined_map = bottom_up_map
            method = 'bottom_up'
        
        # Extraire régions focus
        focus_regions = self._extract_focus_regions(combined_map)
        
        # Calculer poids attention
        attention_weights = self._compute_attention_weights(combined_map, focus_regions)
        
        computation_time = (time.time() - start_time) * 1000
        
        attention_map = AttentionMap(
            saliency_map=combined_map,
            focus_regions=focus_regions,
            attention_weights=attention_weights,
            computation_time_ms=computation_time,
            method_used=method
        )
        
        # Mettre en cache
        if use_cache:
            self.attention_cache[state_hash] = attention_map
        
        # Statistiques
        self.total_computations += 1
        self.avg_computation_time = (
            (self.avg_computation_time * (self.total_computations - 1) + computation_time)
            / self.total_computations
        )
        
        self._log_forensic("ATTENTION_COMPUTED", {
            "method": method,
            "num_focus_regions": len(focus_regions),
            "computation_time_ms": computation_time
        })
        
        return attention_map
    
    def apply_attention(
        self,
        state: np.ndarray,
        attention_map: AttentionMap
    ) -> np.ndarray:
        """
        Applique l'attention à un état.
        
        Réduit dimensionnalité en focalisant sur régions importantes.
        
        Args:
            state: État original
            attention_map: Carte d'attention
            
        Returns:
            État avec attention appliquée
        """
        # Multiplier état par carte saillance
        attended_state = state * attention_map.saliency_map
        
        return attended_state
    
    def get_focus_windows(
        self,
        state: np.ndarray,
        attention_map: AttentionMap
    ) -> List[np.ndarray]:
        """
        Extrait fenêtres focus de l'état.
        
        Retourne sous-grilles correspondant aux régions focus.
        
        Args:
            state: État complet
            attention_map: Carte d'attention
            
        Returns:
            Liste de fenêtres focus
        """
        windows = []
        
        for row, col, height, width in attention_map.focus_regions:
            # Extraire fenêtre
            window = state[row:row+height, col:col+width]
            windows.append(window)
        
        return windows
    
    def _compute_bottom_up_attention(self, state: np.ndarray) -> np.ndarray:
        """
        Calcule attention bottom-up (guidée par stimuli).
        
        Basé sur saillance visuelle: contraste, couleur, mouvement.
        """
        # Normaliser état
        if state.max() > 0:
            normalized = state / state.max()
        else:
            normalized = state
        
        # Calculer gradients (contraste)
        grad_y = np.abs(np.gradient(normalized, axis=0))
        grad_x = np.abs(np.gradient(normalized, axis=1))
        gradient_magnitude = np.sqrt(grad_y**2 + grad_x**2)
        
        # Calculer variance locale (texture)
        from scipy.ndimage import uniform_filter
        local_mean = uniform_filter(normalized, size=3)
        local_variance = uniform_filter(normalized**2, size=3) - local_mean**2
        
        # Combiner features
        saliency = 0.5 * gradient_magnitude + 0.5 * local_variance
        
        # Normaliser
        if saliency.max() > 0:
            saliency = saliency / saliency.max()
        
        # Lisser avec gaussienne
        saliency = gaussian_filter(saliency, sigma=self.gaussian_sigma)
        
        return saliency
    
    def _compute_top_down_attention(
        self,
        state: np.ndarray,
        context: AttentionContext
    ) -> np.ndarray:
        """
        Calcule attention top-down (guidée par tâche).
        
        Basé sur objectifs et contraintes.
        """
        attention = np.zeros_like(state, dtype=float)
        
        # Attention vers position objectif
        if context.goal_position:
            row, col = context.goal_position
            if 0 <= row < state.shape[0] and 0 <= col < state.shape[1]:
                # Créer gaussienne centrée sur objectif
                y, x = np.ogrid[:state.shape[0], :state.shape[1]]
                distance = np.sqrt((y - row)**2 + (x - col)**2)
                goal_attention = np.exp(-distance**2 / (2 * 5**2))  # sigma=5
                attention += goal_attention
        
        # Attention vers couleurs cibles
        if context.target_colors:
            for color in context.target_colors:
                color_mask = (state == color).astype(float)
                attention += color_mask
        
        # Éviter régions spécifiées
        if context.avoid_regions:
            for row, col, height, width in context.avoid_regions:
                attention[row:row+height, col:col+width] *= 0.1
        
        # Normaliser
        if attention.max() > 0:
            attention = attention / attention.max()
        
        return attention
    
    def _combine_attention_maps(
        self,
        bottom_up: np.ndarray,
        top_down: np.ndarray,
        priority: float
    ) -> np.ndarray:
        """
        Combine attention bottom-up et top-down.
        
        priority=0: 100% bottom-up
        priority=1: 100% top-down
        """
        combined = (1 - priority) * bottom_up + priority * top_down
        
        # Normaliser
        if combined.max() > 0:
            combined = combined / combined.max()
        
        return combined
    
    def _extract_focus_regions(
        self,
        saliency_map: np.ndarray
    ) -> List[Tuple[int, int, int, int]]:
        """
        Extrait régions focus de la carte saillance.
        
        Retourne (row, col, height, width) pour chaque région.
        """
        # Seuiller carte saillance
        binary_map = (saliency_map > self.saliency_threshold).astype(int)
        
        # Trouver composantes connexes
        from scipy.ndimage import label
        labeled, num_features = label(binary_map)
        
        regions = []
        for i in range(1, num_features + 1):
            # Trouver bbox de la composante
            rows, cols = np.where(labeled == i)
            if len(rows) < self.min_region_size:
                continue
            
            min_row, max_row = rows.min(), rows.max()
            min_col, max_col = cols.min(), cols.max()
            
            height = max_row - min_row + 1
            width = max_col - min_col + 1
            
            if height >= self.min_region_size and width >= self.min_region_size:
                regions.append((min_row, min_col, height, width))
        
        # Trier par saillance moyenne (décroissant)
        regions_with_saliency = []
        for region in regions:
            row, col, h, w = region
            avg_saliency = np.mean(saliency_map[row:row+h, col:col+w])
            regions_with_saliency.append((region, avg_saliency))
        
        regions_with_saliency.sort(key=lambda x: x[1], reverse=True)
        
        # Garder top N régions
        top_regions = [r[0] for r in regions_with_saliency[:self.max_focus_regions]]
        
        return top_regions
    
    def _compute_attention_weights(
        self,
        saliency_map: np.ndarray,
        focus_regions: List[Tuple[int, int, int, int]]
    ) -> np.ndarray:
        """
        Calcule poids attention pour chaque région.
        """
        if not focus_regions:
            return np.array([])
    
    def focus(self, grid: np.ndarray, context: Optional[AttentionContext] = None) -> AttentionMap:
        """
        Méthode wrapper pour compatibilité avec tests.
        Alias de compute_attention() pour focaliser l'attention.
        
        Args:
            grid: Grille d'entrée
            context: Contexte attention optionnel
            
        Returns:
            AttentionMap avec carte d'attention
        """
        return self.compute_attention(grid, context)
        
        weights = []
        for row, col, height, width in focus_regions:
            avg_saliency = np.mean(saliency_map[row:row+height, col:col+width])
            weights.append(avg_saliency)
        
        # Normaliser (softmax)
        weights = np.array(weights)
        if weights.sum() > 0:
            weights = weights / weights.sum()
        
        return weights
    
    def _hash_state(self, state: np.ndarray) -> str:
        """Hash rapide d'un état."""
        import hashlib
        return hashlib.md5(state.tobytes()).hexdigest()[:16]
    
    def _log_forensic(self, event_type: str, data: Dict):
        """Log événement forensique."""
        if self.forensic_logger:
            try:
                self.forensic_logger.log(
                    event=event_type,
                    component="AttentionMechanism",
                    data=data
                )
            except Exception:
                pass
    
    def get_statistics(self) -> Dict[str, Any]:
        """Retourne statistiques du mécanisme."""
        cache_hit_rate = 0.0
        if self.total_computations > 0:
            cache_hit_rate = self.cache_hits / self.total_computations
        
        return {
            'total_computations': self.total_computations,
            'cache_hits': self.cache_hits,
            'cache_hit_rate': cache_hit_rate,
            'avg_computation_time_ms': self.avg_computation_time,
            'cache_size': len(self.attention_cache)
        }
    
    def reset(self):
        """Réinitialise le mécanisme."""
        self.attention_cache.clear()
        self.total_computations = 0
        self.cache_hits = 0
        self.avg_computation_time = 0.0
        
        self._log_forensic("ATTENTION_MECHANISM_RESET", {})


def test_attention_mechanism():
    """Test basique du mécanisme d'attention."""
    print("🧪 Test AttentionMechanism...")
    
    # Créer mécanisme
    attention = AttentionMechanism(
        min_region_size=3,
        max_focus_regions=5,
        saliency_threshold=0.5
    )
    
    # Test 1: Attention bottom-up
    print("\n1. Test attention bottom-up...")
    state = np.random.rand(20, 20)
    state[5:8, 5:8] = 1.0  # Région saillante
    state[15:18, 15:18] = 0.9  # Autre région
    
    attention_map = attention.compute_attention(state)
    print(f"   ✓ Régions focus: {len(attention_map.focus_regions)}")
    print(f"   ✓ Méthode: {attention_map.method_used}")
    print(f"   ✓ Temps: {attention_map.computation_time_ms:.2f}ms")
    
    # Test 2: Attention top-down
    print("\n2. Test attention top-down...")
    context = AttentionContext(
        goal_position=(10, 10),
        target_colors=[1],
        priority_level=0.7
    )
    
    attention_map_td = attention.compute_attention(state, context)
    print(f"   ✓ Régions focus: {len(attention_map_td.focus_regions)}")
    print(f"   ✓ Méthode: {attention_map_td.method_used}")
    
    # Test 3: Application attention
    print("\n3. Test application attention...")
    attended_state = attention.apply_attention(state, attention_map)
    print(f"   ✓ État avec attention: shape={attended_state.shape}")
    
    # Test 4: Extraction fenêtres
    print("\n4. Test extraction fenêtres...")
    windows = attention.get_focus_windows(state, attention_map)
    print(f"   ✓ Fenêtres extraites: {len(windows)}")
    
    # Statistiques
    print("\n📊 Statistiques:")
    stats = attention.get_statistics()
    print(f"   - Computations: {stats['total_computations']}")
    print(f"   - Cache hit rate: {stats['cache_hit_rate']:.2%}")
    print(f"   - Temps moyen: {stats['avg_computation_time_ms']:.2f}ms")
    
    print("\n✅ Test AttentionMechanism réussi!")


if __name__ == "__main__":
    test_attention_mechanism()

# Made with Bob
