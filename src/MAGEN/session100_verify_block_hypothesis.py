#!/usr/bin/env python3
"""
SESSION 100 - VÉRIFICATION HYPOTHÈSE BLOC 5×5
==============================================

Objectif: Vérifier l'hypothèse du concurrent avec MES propres données
Question centrale: Parmi les composantes 9, laquelle est attachée au 12?

Phases:
1. Identifier le bloc 5×5 (12+9) dans frame initiale
2. Vérifier que le bloc se déplace comme une unité
3. Mapper le labyrinthe (corridors, salles, murs)
4. Localiser les glyphes fixes (4 composantes 9)
"""

import sys
import numpy as np
from scipy.ndimage import label, center_of_mass
import json
from datetime import datetime

# Ajouter le chemin pour arc_agi
sys.path.insert(0, '/home/lvx/LVX/environment_files/ls20/9607627b')

from arc_agi import Arcade, OperationMode
from arcengine import GameAction

PUZZLE_NAME = "ls20-9607627b"

def analyze_components(frame, value):
    """Identifier et analyser les composantes connexes d'une valeur."""
    mask = (frame == value).astype(int)
    labeled, num_components = label(mask)
    
    components = []
    for i in range(1, num_components + 1):
        positions = np.argwhere(labeled == i)
        components.append({
            'id': i,
            'size': len(positions),
            'positions': positions.tolist(),
            'centroid': positions.mean(axis=0).tolist(),
            'bbox': {
                'row_min': int(positions[:, 0].min()),
                'row_max': int(positions[:, 0].max()),
                'col_min': int(positions[:, 1].min()),
                'col_max': int(positions[:, 1].max())
            }
        })
    
    return components

def check_5x5_block(frame, row, col):
    """Vérifier si une position contient un bloc 5×5 avec pattern 12/9."""
    if row + 5 > frame.shape[0] or col + 5 > frame.shape[1]:
        return None
    
    block = frame[row:row+5, col:col+5]
    
    # Compter les pixels 12 et 9
    count_12 = np.sum(block == 12)
    count_9 = np.sum(block == 9)
    
    # Pattern attendu: 2 lignes de 12 (10 pixels), 3 lignes de 9 (15 pixels)
    # Tolérance: au moins 8/10 pour 12, au moins 12/15 pour 9
    if count_12 >= 8 and count_9 >= 12:
        return {
            'position': (row, col),
            'count_12': int(count_12),
            'count_9': int(count_9),
            'total': int(count_12 + count_9),
            'block': block.tolist(),
            'pattern_score': (count_12 / 10.0 + count_9 / 15.0) / 2.0
        }
    
    return None

def find_5x5_blocks(frame):
    """Chercher tous les blocs 5×5 potentiels dans la frame."""
    blocks = []
    
    for i in range(frame.shape[0] - 4):
        for j in range(frame.shape[1] - 4):
            block_info = check_5x5_block(frame, i, j)
            if block_info:
                blocks.append(block_info)
    
    return blocks

def track_component_movement(frame_before, frame_after, value):
    """Tracker le mouvement d'une composante entre deux frames."""
    components_before = analyze_components(frame_before, value)
    components_after = analyze_components(frame_after, value)
    
    movements = []
    for comp_before in components_before:
        # Trouver la composante correspondante dans frame_after
        best_match = None
        min_distance = float('inf')
        
        for comp_after in components_after:
            # Distance entre centroids
            dist = np.linalg.norm(
                np.array(comp_before['centroid']) - np.array(comp_after['centroid'])
            )
            if dist < min_distance:
                min_distance = dist
                best_match = comp_after
        
        if best_match:
            delta = np.array(best_match['centroid']) - np.array(comp_before['centroid'])
            movements.append({
                'component_id': comp_before['id'],
                'size_before': comp_before['size'],
                'size_after': best_match['size'],
                'centroid_before': comp_before['centroid'],
                'centroid_after': best_match['centroid'],
                'delta': delta.tolist(),
                'distance': float(min_distance)
            })
    
    return movements

def identify_mobile_component_9(frame_0, frame_1, components_9_initial):
    """Identifier quelle composante 9 est mobile (attachée au 12)."""
    # Tracker le mouvement du 12
    movements_12 = track_component_movement(frame_0, frame_1, 12)
    
    if not movements_12:
        return None, "Aucun mouvement détecté pour valeur 12"
    
    delta_12 = np.array(movements_12[0]['delta'])
    
    # Tracker les mouvements des composantes 9
    movements_9 = track_component_movement(frame_0, frame_1, 9)
    
    # Trouver la composante 9 qui bouge avec le 12
    mobile_component = None
    min_delta_diff = float('inf')
    
    for mov_9 in movements_9:
        delta_9 = np.array(mov_9['delta'])
        delta_diff = np.linalg.norm(delta_9 - delta_12)
        
        if delta_diff < min_delta_diff:
            min_delta_diff = delta_diff
            mobile_component = mov_9
    
    return mobile_component, movements_9

def map_labyrinth(frame):
    """Mapper la structure du labyrinthe."""
    labyrinth = {
        'walls': analyze_components(frame, 4),  # Murs
        'corridors': analyze_components(frame, 3),  # Corridors
        'rooms': analyze_components(frame, 5),  # Salles
        'glyphs': analyze_components(frame, 9),  # Glyphes (tous)
        'timer': analyze_components(frame, 11),  # Timer
        'statistics': {
            'walls_pixels': int(np.sum(frame == 4)),
            'corridors_pixels': int(np.sum(frame == 3)),
            'rooms_pixels': int(np.sum(frame == 5)),
            'glyphs_pixels': int(np.sum(frame == 9)),
            'timer_pixels': int(np.sum(frame == 11))
        }
    }
    
    return labyrinth

def main():
    print("=" * 80)
    print("SESSION 100 - VÉRIFICATION HYPOTHÈSE BLOC 5×5")
    print("=" * 80)
    print()
    
    # Initialiser l'environnement
    print("📦 Chargement de l'environnement...")
    arcade = Arcade(operation_mode=OperationMode.COMPETITION)
    env = arcade.make(PUZZLE_NAME)
    obs = env.reset()
    
    # Corriger la shape si nécessaire (enlever dimension batch)
    frame_0 = np.array(obs.frame)
    if frame_0.ndim == 3 and frame_0.shape[0] == 1:
        frame_0 = frame_0[0]  # Enlever la dimension batch
    
    print(f"✅ Frame initiale chargée: {frame_0.shape}")
    print()
    
    # ========================================================================
    # PHASE 1: IDENTIFIER LE BLOC 5×5
    # ========================================================================
    print("=" * 80)
    print("PHASE 1: IDENTIFICATION DU BLOC 5×5")
    print("=" * 80)
    print()
    
    # Analyser composante 12
    print("🔍 Analyse de la composante 12...")
    components_12 = analyze_components(frame_0, 12)
    print(f"   Composantes 12 détectées: {len(components_12)}")
    
    for comp in components_12:
        print(f"   - Composante {comp['id']}: {comp['size']} pixels")
        print(f"     Centroid: {comp['centroid']}")
        print(f"     BBox: rows {comp['bbox']['row_min']}-{comp['bbox']['row_max']}, "
              f"cols {comp['bbox']['col_min']}-{comp['bbox']['col_max']}")
    print()
    
    # Analyser composantes 9
    print("🔍 Analyse des composantes 9...")
    components_9 = analyze_components(frame_0, 9)
    print(f"   Composantes 9 détectées: {len(components_9)}")
    
    for comp in components_9:
        print(f"   - Composante {comp['id']}: {comp['size']} pixels")
        print(f"     Centroid: {comp['centroid']}")
        print(f"     BBox: rows {comp['bbox']['row_min']}-{comp['bbox']['row_max']}, "
              f"cols {comp['bbox']['col_min']}-{comp['bbox']['col_max']}")
    print()
    
    # Chercher blocs 5×5
    print("🔍 Recherche de blocs 5×5 (pattern 12/9)...")
    blocks_5x5 = find_5x5_blocks(frame_0)
    print(f"   Blocs 5×5 potentiels trouvés: {len(blocks_5x5)}")
    print()
    
    if blocks_5x5:
        # Trier par score de pattern
        blocks_5x5.sort(key=lambda x: x['pattern_score'], reverse=True)
        
        print("   📊 Meilleurs blocs 5×5:")
        for i, block in enumerate(blocks_5x5[:3], 1):
            print(f"   {i}. Position: row {block['position'][0]}, col {block['position'][1]}")
            print(f"      Pixels 12: {block['count_12']}/10, Pixels 9: {block['count_9']}/15")
            print(f"      Score: {block['pattern_score']:.2%}")
            print(f"      Bloc:")
            for row in block['block']:
                print(f"        {row}")
            print()
    else:
        print("   ❌ AUCUN bloc 5×5 trouvé!")
        print()
    
    # ========================================================================
    # PHASE 2: VÉRIFIER LE MOUVEMENT DU BLOC
    # ========================================================================
    print("=" * 80)
    print("PHASE 2: VÉRIFICATION DU MOUVEMENT")
    print("=" * 80)
    print()
    
    # Tester ACTION4 (devrait rapprocher selon Session 94)
    print("🎮 Exécution de ACTION4...")
    obs = env.step(GameAction.ACTION4)
    
    # Corriger la shape si nécessaire
    frame_1 = np.array(obs.frame)
    if frame_1.ndim == 3 and frame_1.shape[0] == 1:
        frame_1 = frame_1[0]
    
    print(f"✅ Frame après ACTION4 obtenue: {frame_1.shape}")
    print()
    
    # Tracker mouvement du 12
    print("🔍 Tracking du mouvement de la composante 12...")
    movements_12 = track_component_movement(frame_0, frame_1, 12)
    
    if movements_12:
        for mov in movements_12:
            print(f"   Composante {mov['component_id']}:")
            print(f"   - Taille: {mov['size_before']} → {mov['size_after']} pixels")
            print(f"   - Centroid: {mov['centroid_before']} → {mov['centroid_after']}")
            print(f"   - Delta: {mov['delta']} (distance: {mov['distance']:.2f})")
    else:
        print("   ⚠️ Aucun mouvement détecté pour valeur 12")
    print()
    
    # Identifier la composante 9 mobile
    print("🔍 Identification de la composante 9 mobile...")
    mobile_9, all_movements_9 = identify_mobile_component_9(frame_0, frame_1, components_9)
    
    if mobile_9:
        print(f"   ✅ Composante 9 mobile identifiée:")
        print(f"   - Composante ID: {mobile_9['component_id']}")
        print(f"   - Taille: {mobile_9['size_before']} pixels")
        print(f"   - Delta: {mobile_9['delta']}")
        print(f"   - Distance parcourue: {mobile_9['distance']:.2f}")
        print()
        
        # Vérifier si cette composante forme un bloc 5×5 avec le 12
        if blocks_5x5:
            best_block = blocks_5x5[0]
            print(f"   🔗 Vérification du lien avec le bloc 5×5:")
            print(f"   - Bloc position: {best_block['position']}")
            print(f"   - Composante 9 mobile: ID {mobile_9['component_id']}")
            print()
    else:
        print("   ❌ Impossible d'identifier la composante 9 mobile")
        print(f"   Raison: {all_movements_9}")
        print()
    
    # Afficher tous les mouvements 9 pour analyse
    if isinstance(all_movements_9, list):
        print("   📊 Tous les mouvements des composantes 9:")
        for mov in all_movements_9:
            print(f"   - Composante {mov['component_id']}: delta {mov['delta']}, "
                  f"distance {mov['distance']:.2f}")
        print()
    
    # ========================================================================
    # PHASE 3: MAPPER LE LABYRINTHE
    # ========================================================================
    print("=" * 80)
    print("PHASE 3: CARTOGRAPHIE DU LABYRINTHE")
    print("=" * 80)
    print()
    
    labyrinth = map_labyrinth(frame_0)
    
    print("📊 Statistiques du labyrinthe:")
    print(f"   - Murs (4): {labyrinth['statistics']['walls_pixels']} pixels, "
          f"{len(labyrinth['walls'])} composantes")
    print(f"   - Corridors (3): {labyrinth['statistics']['corridors_pixels']} pixels, "
          f"{len(labyrinth['corridors'])} composantes")
    print(f"   - Salles (5): {labyrinth['statistics']['rooms_pixels']} pixels, "
          f"{len(labyrinth['rooms'])} composantes")
    print(f"   - Glyphes (9): {labyrinth['statistics']['glyphs_pixels']} pixels, "
          f"{len(labyrinth['glyphs'])} composantes")
    print(f"   - Timer (11): {labyrinth['statistics']['timer_pixels']} pixels")
    print()
    
    # Identifier les salles avec glyphes
    print("🔍 Identification des salles avec glyphes...")
    rooms_with_glyphs = []
    
    for room in labyrinth['rooms']:
        room_bbox = room['bbox']
        # Chercher des glyphes dans cette salle
        glyphs_in_room = []
        
        for glyph in labyrinth['glyphs']:
            glyph_centroid = glyph['centroid']
            # Vérifier si le centroid du glyphe est dans la bbox de la salle
            if (room_bbox['row_min'] <= glyph_centroid[0] <= room_bbox['row_max'] and
                room_bbox['col_min'] <= glyph_centroid[1] <= room_bbox['col_max']):
                glyphs_in_room.append(glyph)
        
        if glyphs_in_room:
            rooms_with_glyphs.append({
                'room': room,
                'glyphs': glyphs_in_room
            })
    
    print(f"   Salles avec glyphes: {len(rooms_with_glyphs)}")
    for i, room_info in enumerate(rooms_with_glyphs, 1):
        room = room_info['room']
        print(f"   {i}. Salle rows {room['bbox']['row_min']}-{room['bbox']['row_max']}, "
              f"cols {room['bbox']['col_min']}-{room['bbox']['col_max']}")
        print(f"      Glyphes: {len(room_info['glyphs'])}")
        for glyph in room_info['glyphs']:
            print(f"      - Glyphe {glyph['id']}: {glyph['size']} pixels, "
                  f"centroid {glyph['centroid']}")
    print()
    
    # ========================================================================
    # RÉSULTATS ET CONCLUSIONS
    # ========================================================================
    print("=" * 80)
    print("RÉSULTATS ET CONCLUSIONS")
    print("=" * 80)
    print()
    
    results = {
        'timestamp': datetime.now().isoformat(),
        'puzzle': PUZZLE_NAME,
        'phase1_block_identification': {
            'blocks_5x5_found': len(blocks_5x5),
            'best_block': blocks_5x5[0] if blocks_5x5 else None,
            'components_12': len(components_12),
            'components_9': len(components_9)
        },
        'phase2_movement_verification': {
            'movements_12': movements_12,
            'mobile_component_9': mobile_9,
            'all_movements_9': all_movements_9 if isinstance(all_movements_9, list) else []
        },
        'phase3_labyrinth_mapping': {
            'statistics': labyrinth['statistics'],
            'rooms_with_glyphs': len(rooms_with_glyphs),
            'rooms_details': rooms_with_glyphs
        }
    }
    
    # Évaluation de l'hypothèse
    print("📊 ÉVALUATION DE L'HYPOTHÈSE BLOC 5×5:")
    print()
    
    confidence_scores = {}
    
    # Critère 1: Bloc 5×5 trouvé
    if blocks_5x5 and blocks_5x5[0]['pattern_score'] > 0.85:
        print("   ✅ Bloc 5×5 identifié (score > 85%)")
        confidence_scores['block_found'] = 95
    elif blocks_5x5:
        print(f"   ⚠️ Bloc 5×5 trouvé mais score faible ({blocks_5x5[0]['pattern_score']:.1%})")
        confidence_scores['block_found'] = 60
    else:
        print("   ❌ Aucun bloc 5×5 trouvé")
        confidence_scores['block_found'] = 10
    
    # Critère 2: Mouvement cohérent
    if mobile_9 and movements_12:
        delta_12 = np.array(movements_12[0]['delta'])
        delta_9 = np.array(mobile_9['delta'])
        delta_diff = np.linalg.norm(delta_9 - delta_12)
        
        if delta_diff < 1.0:
            print(f"   ✅ Mouvement cohérent (diff: {delta_diff:.2f})")
            confidence_scores['movement_coherent'] = 95
        elif delta_diff < 3.0:
            print(f"   ⚠️ Mouvement partiellement cohérent (diff: {delta_diff:.2f})")
            confidence_scores['movement_coherent'] = 70
        else:
            print(f"   ❌ Mouvement incohérent (diff: {delta_diff:.2f})")
            confidence_scores['movement_coherent'] = 30
    else:
        print("   ❌ Impossible de vérifier le mouvement")
        confidence_scores['movement_coherent'] = 20
    
    # Critère 3: Labyrinthe structuré
    if len(labyrinth['rooms']) > 0 and len(rooms_with_glyphs) > 0:
        print(f"   ✅ Labyrinthe structuré ({len(rooms_with_glyphs)} salles avec glyphes)")
        confidence_scores['labyrinth_structured'] = 90
    else:
        print("   ⚠️ Structure de labyrinthe incertaine")
        confidence_scores['labyrinth_structured'] = 50
    
    # Critère 4: Glyphes fixes identifiés
    if len(components_9) == 5 and mobile_9:
        print(f"   ✅ 5 composantes 9 détectées (1 mobile + 4 fixes)")
        confidence_scores['glyphs_identified'] = 90
    elif len(components_9) > 1:
        print(f"   ⚠️ {len(components_9)} composantes 9 (structure incertaine)")
        confidence_scores['glyphs_identified'] = 60
    else:
        print(f"   ❌ Structure des composantes 9 invalide")
        confidence_scores['glyphs_identified'] = 20
    
    print()
    
    # Confiance globale
    overall_confidence = np.mean(list(confidence_scores.values()))
    results['confidence_scores'] = confidence_scores
    results['overall_confidence'] = float(overall_confidence)
    
    print(f"🎯 CONFIANCE GLOBALE: {overall_confidence:.1f}%")
    print()
    
    if overall_confidence >= 90:
        print("✅ HYPOTHÈSE CONFIRMÉE (>90%)")
        print("   → Procéder au test de victoire (Phase 4-5)")
    elif overall_confidence >= 70:
        print("⚠️ HYPOTHÈSE PARTIELLEMENT CONFIRMÉE (70-90%)")
        print("   → Tests intermédiaires recommandés")
    else:
        print("❌ HYPOTHÈSE INFIRMÉE (<70%)")
        print("   → Analyse supplémentaire nécessaire")
    print()
    
    # Sauvegarder les résultats
    output_file = f"session100_verification_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
    with open(output_file, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"💾 Résultats sauvegardés: {output_file}")
    print()
    print("=" * 80)
    print("FIN DE LA VÉRIFICATION")
    print("=" * 80)

if __name__ == "__main__":
    main()

# Made with Bob
