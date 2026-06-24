#!/usr/bin/env python3
"""
SESSION 93 - CARTOGRAPHIE SPATIALE DÉTAILLÉE
=============================================

OBJECTIF: Cartographier les 52 pixels modifiés pour ACTION1 et ACTION3

Test discriminant:
- Si ACTION3 = inverse spatial de ACTION1, nous le verrons immédiatement
- Coordonnées exactes (y, x)
- Valeurs avant/après
- Visualisation des décalages
- Analyse composantes connexes

Durée estimée: 2h
"""

import sys
import json
import numpy as np
import matplotlib.pyplot as plt
from datetime import datetime
from collections import defaultdict
from typing import Dict, List, Tuple
from scipy.ndimage import label

# Import ARC-AGI
import arc_agi
from arc_agi import Arcade, OperationMode
from arcengine import GameAction, GameState


class SpatialMapper:
    """Cartographie spatiale détaillée des changements"""
    
    def __init__(self, game_id: str = "ls20-9607627b"):
        self.game_id = game_id
        self.arcade = None
        self.env = None
        self.results = {
            "game_id": game_id,
            "timestamp": datetime.now().isoformat(),
            "mappings": {}
        }
        
    def initialize_game(self):
        """Initialise le jeu ARC-AGI"""
        print(f"\n{'='*80}")
        print(f"SESSION 93 - CARTOGRAPHIE SPATIALE")
        print(f"Jeu: {self.game_id}")
        print(f"{'='*80}\n")
        
        try:
            self.arcade = Arcade(operation_mode=OperationMode.COMPETITION)
            print(f"✓ Arcade créé")
            
            self.env = self.arcade.make(self.game_id)
            print(f"✓ Environnement créé")
            return True
        except Exception as e:
            print(f"✗ ERREUR: {e}")
            return False
    
    def extract_frame(self, obs) -> np.ndarray:
        """Extrait la grille 64x64 depuis l'observation"""
        if hasattr(obs, 'frame'):
            frame = obs.frame
            if isinstance(frame, list):
                frame = np.array(frame)
            else:
                frame = np.array(frame)
            
            # Si 3D, prendre première couche
            if frame.ndim == 3:
                frame = frame[0]
            
            if frame.ndim != 2:
                print(f"ATTENTION: Frame a {frame.ndim} dimensions")
                return np.zeros((64, 64), dtype=int)
            
            return frame
        return np.zeros((64, 64), dtype=int)
    
    def get_quadrant(self, y: int, x: int) -> str:
        """Détermine le quadrant d'une position"""
        if y < 32:
            return "top_left" if x < 32 else "top_right"
        else:
            return "bottom_left" if x < 32 else "bottom_right"
    
    def map_pixel_changes(self, action: GameAction, action_name: str) -> Dict:
        """Cartographie complète des changements pour une action"""
        print(f"\n{'='*80}")
        print(f"CARTOGRAPHIE: {action_name}")
        print(f"{'='*80}\n")
        
        # Reset et capture état initial
        obs = self.env.reset()
        frame_before = self.extract_frame(obs)
        
        print(f"État initial:")
        print(f"  Shape: {frame_before.shape}")
        print(f"  Valeurs uniques: {np.unique(frame_before)}")
        print(f"  Distribution: {dict(zip(*np.unique(frame_before, return_counts=True)))}")
        
        # Appliquer action
        obs = self.env.step(action)
        frame_after = self.extract_frame(obs)
        
        print(f"\nAprès {action_name}:")
        print(f"  Shape: {frame_after.shape}")
        print(f"  Valeurs uniques: {np.unique(frame_after)}")
        print(f"  Distribution: {dict(zip(*np.unique(frame_after, return_counts=True)))}")
        
        # Calculer différences
        diff = frame_after != frame_before
        changed_positions = np.argwhere(diff)
        
        print(f"\nChangements détectés: {len(changed_positions)}")
        
        # Extraire tous les changements
        changes = []
        quadrant_counts = defaultdict(int)
        value_transitions = defaultdict(int)
        
        for pos in changed_positions:
            y, x = int(pos[0]), int(pos[1])
            
            before_val = frame_before[y, x]
            after_val = frame_after[y, x]
            
            # Convertir en scalaire si nécessaire
            if isinstance(before_val, np.ndarray):
                before_val = before_val.flat[0]
            if isinstance(after_val, np.ndarray):
                after_val = after_val.flat[0]
            
            quadrant = self.get_quadrant(y, x)
            transition = f"{int(before_val)}→{int(after_val)}"
            
            change = {
                "y": y,
                "x": x,
                "before": int(before_val),
                "after": int(after_val),
                "delta": int(after_val) - int(before_val),
                "quadrant": quadrant
            }
            
            changes.append(change)
            quadrant_counts[quadrant] += 1
            value_transitions[transition] += 1
        
        # Statistiques
        print(f"\nRépartition par quadrant:")
        for quad, count in sorted(quadrant_counts.items()):
            print(f"  {quad}: {count} changements")
        
        print(f"\nTransitions de valeurs:")
        for trans, count in sorted(value_transitions.items(), key=lambda x: -x[1])[:10]:
            print(f"  {trans}: {count} fois")
        
        # Analyse des patterns
        if len(changes) > 0:
            y_coords = [c['y'] for c in changes]
            x_coords = [c['x'] for c in changes]
            
            print(f"\nCoordonnées:")
            print(f"  Y: min={min(y_coords)}, max={max(y_coords)}, range={max(y_coords)-min(y_coords)+1}")
            print(f"  X: min={min(x_coords)}, max={max(x_coords)}, range={max(x_coords)-min(x_coords)+1}")
        
        return {
            "action": action_name,
            "num_changes": len(changes),
            "changes": changes,
            "quadrant_counts": dict(quadrant_counts),
            "value_transitions": dict(value_transitions),
            "frames": {
                "before": frame_before.tolist(),
                "after": frame_after.tolist()
            }
        }
    
    def compare_actions(self, mapping1: Dict, mapping2: Dict) -> Dict:
        """Compare deux mappings pour détecter symétrie"""
        print(f"\n{'='*80}")
        print(f"COMPARAISON: {mapping1['action']} vs {mapping2['action']}")
        print(f"{'='*80}\n")
        
        changes1 = mapping1['changes']
        changes2 = mapping2['changes']
        
        print(f"Nombre de changements:")
        print(f"  {mapping1['action']}: {len(changes1)}")
        print(f"  {mapping2['action']}: {len(changes2)}")
        
        if len(changes1) != len(changes2):
            print(f"\n❌ Nombre de changements différent")
            return {
                "symmetric": False,
                "reason": "Nombre de changements différent"
            }
        
        # Trier par position
        changes1_sorted = sorted(changes1, key=lambda c: (c['y'], c['x']))
        changes2_sorted = sorted(changes2, key=lambda c: (c['y'], c['x']))
        
        # Analyser les décalages
        x_diffs = []
        y_diffs = []
        value_inversions = 0
        
        for c1, c2 in zip(changes1_sorted, changes2_sorted):
            y_diff = c2['y'] - c1['y']
            x_diff = c2['x'] - c1['x']
            
            y_diffs.append(y_diff)
            x_diffs.append(x_diff)
            
            # Vérifier inversion de valeurs
            if c1['before'] == c2['after'] and c1['after'] == c2['before']:
                value_inversions += 1
        
        # Statistiques des décalages
        print(f"\nDécalages Y:")
        print(f"  Min: {min(y_diffs)}, Max: {max(y_diffs)}, Unique: {set(y_diffs)}")
        
        print(f"\nDécalages X:")
        print(f"  Min: {min(x_diffs)}, Max: {max(x_diffs)}, Unique: {set(x_diffs)}")
        
        print(f"\nInversions de valeurs: {value_inversions}/{len(changes1)}")
        
        # Déterminer si symétrique
        is_symmetric = False
        reason = ""
        
        if len(set(y_diffs)) == 1 and len(set(x_diffs)) == 1:
            y_shift = y_diffs[0]
            x_shift = x_diffs[0]
            
            if y_shift == 0 and abs(x_shift) == 2:
                is_symmetric = True
                reason = f"Décalage horizontal opposé: {x_shift} pixels"
                print(f"\n✅ SYMÉTRIE DÉTECTÉE: {reason}")
            else:
                reason = f"Décalage uniforme mais pas opposé: (dy={y_shift}, dx={x_shift})"
                print(f"\n⚠️ {reason}")
        else:
            reason = "Décalages non uniformes"
            print(f"\n❌ {reason}")
        
        return {
            "symmetric": is_symmetric,
            "reason": reason,
            "y_diffs": {"min": min(y_diffs), "max": max(y_diffs), "unique": list(set(y_diffs))},
            "x_diffs": {"min": min(x_diffs), "max": max(x_diffs), "unique": list(set(x_diffs))},
            "value_inversions": value_inversions,
            "total_pairs": len(changes1)
        }
    
    def visualize_changes(self, mapping: Dict, output_file: str):
        """Visualise les changements sur la grille"""
        print(f"\nGénération visualisation: {output_file}")
        
        frame_before = np.array(mapping['frames']['before'])
        frame_after = np.array(mapping['frames']['after'])
        
        # Créer grille de différences
        diff = frame_after.astype(int) - frame_before.astype(int)
        
        # Créer figure avec 3 subplots
        fig, axes = plt.subplots(1, 3, figsize=(18, 6))
        
        # Avant
        im1 = axes[0].imshow(frame_before, cmap='tab20', vmin=0, vmax=12)
        axes[0].set_title(f"AVANT {mapping['action']}")
        axes[0].set_xlabel('Colonne')
        axes[0].set_ylabel('Ligne')
        axes[0].axhline(32, color='white', linestyle='--', alpha=0.3)
        axes[0].axvline(32, color='white', linestyle='--', alpha=0.3)
        plt.colorbar(im1, ax=axes[0])
        
        # Après
        im2 = axes[1].imshow(frame_after, cmap='tab20', vmin=0, vmax=12)
        axes[1].set_title(f"APRÈS {mapping['action']}")
        axes[1].set_xlabel('Colonne')
        axes[1].set_ylabel('Ligne')
        axes[1].axhline(32, color='white', linestyle='--', alpha=0.3)
        axes[1].axvline(32, color='white', linestyle='--', alpha=0.3)
        plt.colorbar(im2, ax=axes[1])
        
        # Différence
        im3 = axes[2].imshow(diff, cmap='RdYlGn', vmin=-5, vmax=5)
        axes[2].set_title(f"DIFFÉRENCE (après - avant)")
        axes[2].set_xlabel('Colonne')
        axes[2].set_ylabel('Ligne')
        axes[2].axhline(32, color='black', linestyle='--', alpha=0.5)
        axes[2].axvline(32, color='black', linestyle='--', alpha=0.5)
        plt.colorbar(im3, ax=axes[2], label='Delta')
        
        # Marquer les changements
        for change in mapping['changes']:
            axes[2].plot(change['x'], change['y'], 'ko', markersize=2, alpha=0.5)
        
        plt.tight_layout()
        plt.savefig(output_file, dpi=150, bbox_inches='tight')
        plt.close()
        
        print(f"✓ Visualisation sauvegardée")
    
    def find_connected_components(self, frame: np.ndarray, value: int) -> List[Dict]:
        """Trouve les composantes connexes de pixels de valeur donnée"""
        mask = (frame == value)
        labeled, num_features = label(mask)
        
        components = []
        for i in range(1, num_features + 1):
            positions = np.argwhere(labeled == i)
            
            if len(positions) > 0:
                components.append({
                    "id": i,
                    "value": value,
                    "size": len(positions),
                    "positions": positions.tolist(),
                    "bbox": {
                        "y_min": int(positions[:, 0].min()),
                        "y_max": int(positions[:, 0].max()),
                        "x_min": int(positions[:, 1].min()),
                        "x_max": int(positions[:, 1].max())
                    },
                    "centroid": {
                        "y": float(positions[:, 0].mean()),
                        "x": float(positions[:, 1].mean())
                    }
                })
        
        return components
    
    def analyze_components(self, mapping: Dict) -> Dict:
        """Analyse les composantes connexes avant/après"""
        print(f"\n{'='*80}")
        print(f"ANALYSE COMPOSANTES: {mapping['action']}")
        print(f"{'='*80}\n")
        
        frame_before = np.array(mapping['frames']['before'])
        frame_after = np.array(mapping['frames']['after'])
        
        # Analyser pour chaque valeur
        components_analysis = {}
        
        for value in range(13):
            comp_before = self.find_connected_components(frame_before, value)
            comp_after = self.find_connected_components(frame_after, value)
            
            if len(comp_before) > 0 or len(comp_after) > 0:
                components_analysis[value] = {
                    "before": {
                        "count": len(comp_before),
                        "total_pixels": sum(c['size'] for c in comp_before),
                        "components": comp_before
                    },
                    "after": {
                        "count": len(comp_after),
                        "total_pixels": sum(c['size'] for c in comp_after),
                        "components": comp_after
                    }
                }
                
                print(f"Valeur {value}:")
                print(f"  Avant: {len(comp_before)} composantes, {sum(c['size'] for c in comp_before)} pixels")
                print(f"  Après: {len(comp_after)} composantes, {sum(c['size'] for c in comp_after)} pixels")
                
                # Analyser déplacements des centroïdes
                if len(comp_before) == len(comp_after) and len(comp_before) > 0:
                    for cb, ca in zip(comp_before, comp_after):
                        dy = ca['centroid']['y'] - cb['centroid']['y']
                        dx = ca['centroid']['x'] - cb['centroid']['x']
                        print(f"    Composante {cb['id']}: déplacement (dy={dy:.1f}, dx={dx:.1f})")
        
        return components_analysis
    
    def run_full_mapping(self):
        """Exécute la cartographie complète"""
        if not self.initialize_game():
            return None
        
        # Cartographier ACTION1
        print(f"\n{'#'*80}")
        print(f"# PHASE 1: CARTOGRAPHIE ACTION1")
        print(f"{'#'*80}")
        mapping_action1 = self.map_pixel_changes(GameAction.ACTION1, "ACTION1")
        self.results["mappings"]["ACTION1"] = mapping_action1
        
        # Visualiser ACTION1
        self.visualize_changes(mapping_action1, "session93_ACTION1_visualization.png")
        
        # Analyser composantes ACTION1
        comp_action1 = self.analyze_components(mapping_action1)
        self.results["mappings"]["ACTION1"]["components"] = comp_action1
        
        # Cartographier ACTION3
        print(f"\n{'#'*80}")
        print(f"# PHASE 2: CARTOGRAPHIE ACTION3")
        print(f"{'#'*80}")
        mapping_action3 = self.map_pixel_changes(GameAction.ACTION3, "ACTION3")
        self.results["mappings"]["ACTION3"] = mapping_action3
        
        # Visualiser ACTION3
        self.visualize_changes(mapping_action3, "session93_ACTION3_visualization.png")
        
        # Analyser composantes ACTION3
        comp_action3 = self.analyze_components(mapping_action3)
        self.results["mappings"]["ACTION3"]["components"] = comp_action3
        
        # Comparer ACTION1 et ACTION3
        print(f"\n{'#'*80}")
        print(f"# PHASE 3: COMPARAISON ACTION1 vs ACTION3")
        print(f"{'#'*80}")
        comparison = self.compare_actions(mapping_action1, mapping_action3)
        self.results["comparison"] = comparison
        
        # Sauvegarder résultats
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"session93_spatial_mapping_{timestamp}.json"
        
        with open(filename, 'w') as f:
            json.dump(self.results, f, indent=2)
        
        print(f"\n{'='*80}")
        print(f"RÉSULTATS SAUVEGARDÉS: {filename}")
        print(f"{'='*80}\n")
        
        return filename


def main():
    """Point d'entrée principal"""
    mapper = SpatialMapper(game_id="ls20-9607627b")
    filename = mapper.run_full_mapping()
    
    if filename:
        print(f"✓ Cartographie terminée avec succès")
        print(f"✓ Résultats dans: {filename}")
        print(f"✓ Visualisations:")
        print(f"  - session93_ACTION1_visualization.png")
        print(f"  - session93_ACTION3_visualization.png")
        return 0
    else:
        print(f"✗ Échec de la cartographie")
        return 1


if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
