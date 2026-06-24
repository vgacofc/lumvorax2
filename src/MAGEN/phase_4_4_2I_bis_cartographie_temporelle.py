#!/usr/bin/env python3
"""
MAGEN - Phase 4.4.2I-bis: Cartographie Temporelle Complète du Cycle

OBJECTIF:
Cartographier le comportement temporel complet autour de l'événement step 43
pour déterminer si la porte reste stable ou continue à évoluer.

PROTOCOLE:
1. Capturer états aux steps: 40, 41, 42, 43, 44, 45, 46, 50
2. Analyser évolution de la porte (entité 3#2)
3. Détecter si événement similaire au step 86
4. Mesurer stabilité temporelle post-événement

HYPOTHÈSES À TESTER:
H1: La porte reste à (61.5, 62.5) après step 43
H2: La porte disparaît après step 43
H3: La porte continue à évoluer après step 43
H4: La porte revient à sa position d'origine
H5: Événement similaire se reproduit au step 86 (cycle confirmé)

Date: 2026-06-20
Auteur: Bob (MAGEN Phase 4.4.2I-bis)
"""

import sys
import os
import json
import numpy as np
from datetime import datetime
from pathlib import Path
from scipy import ndimage

# Setup paths
MAGEN_ROOT = Path(__file__).parent
WORKSPACE_ROOT = MAGEN_ROOT.parent.parent.parent
sys.path.insert(0, str(WORKSPACE_ROOT))  # Pour accéder à environment_files

# Import du jeu RÉEL ls20
ls20_path = WORKSPACE_ROOT / "environment_files" / "ls20" / "9607627b"
sys.path.insert(0, str(ls20_path))

import ls20
from arcengine import GameAction


def get_grid_from_game(game):
    """
    Obtient la grille du jeu (64×64 values).
    
    Args:
        game: Instance du jeu ls20
        
    Returns:
        np.ndarray: Grille 2D (64, 64) avec valeurs de pixels
    """
    frame = game.camera.render(game.current_level._sprites)
    if not isinstance(frame, np.ndarray):
        frame = np.array(frame)
    return frame


def extract_connected_components(grid):
    """
    Extrait les composantes connexes de la grille.
    
    Args:
        grid: Grille 2D (64, 64)
        
    Returns:
        list: Liste de dictionnaires décrivant chaque composante
    """
    components = []
    
    # Pour chaque valeur unique dans la grille
    unique_values = np.unique(grid)
    
    for value in unique_values:
        if value == 0:  # Skip background
            continue
            
        # Créer masque binaire pour cette valeur
        binary_mask = (grid == value).astype(int)
        
        # Trouver composantes connexes
        labeled, num_features = ndimage.label(binary_mask)
        
        # Pour chaque composante
        for label_id in range(1, num_features + 1):
            component_mask = (labeled == label_id)
            pixels = np.argwhere(component_mask)
            
            if len(pixels) == 0:
                continue
            
            # Calculer propriétés
            centroid_y = float(np.mean(pixels[:, 0]))
            centroid_x = float(np.mean(pixels[:, 1]))
            pixel_count = len(pixels)
            
            # Bounding box
            min_y, min_x = pixels.min(axis=0)
            max_y, max_x = pixels.max(axis=0)
            
            components.append({
                'value': int(value),
                'label': label_id,
                'pixel_count': pixel_count,
                'centroid': (centroid_x, centroid_y),
                'bbox': {
                    'min_x': int(min_x),
                    'min_y': int(min_y),
                    'max_x': int(max_x),
                    'max_y': int(max_y)
                }
            })
    
    return components


def find_entity_by_id(entities, value, label=None):
    """
    Trouve une entité par sa valeur et optionnellement son label.
    
    Args:
        entities: Liste d'entités
        value: Valeur de pixel recherchée
        label: Label optionnel (si None, retourne première entité avec cette valeur)
        
    Returns:
        dict ou None: Entité trouvée ou None
    """
    for entity in entities:
        if entity['value'] == value:
            if label is None or entity['label'] == label:
                return entity
    return None


def capture_state_at_step(game, target_step):
    """
    Capture l'état complet du jeu à un step donné.
    
    Args:
        game: Instance du jeu
        target_step: Step cible à atteindre
        
    Returns:
        dict: État complet (grid, entities, step)
    """
    # Classe ActionInput locale
    class ActionInput:
        def __init__(self, action_id):
            self.id = action_id
    
    # Exécuter ACTION2 jusqu'au step cible
    current_step = 0
    while current_step < target_step:
        game_action = GameAction.ACTION2
        action_input = ActionInput(game_action)
        game._set_action(action_input)
        
        # Attendre que l'action soit complète
        max_steps = 100
        steps = 0
        while not game.is_action_complete() and steps < max_steps:
            game.step()
            steps += 1
        
        current_step += 1
    
    # Capturer état
    grid = get_grid_from_game(game)
    entities = extract_connected_components(grid)
    
    return {
        'step': target_step,
        'grid': grid,
        'entities': entities,
        'grid_hash': hash(grid.tobytes())
    }


def analyze_door_evolution(states):
    """
    Analyse l'évolution de la porte (entité 3#2) à travers les steps.
    
    Args:
        states: Liste d'états capturés
        
    Returns:
        dict: Analyse de l'évolution de la porte
    """
    door_evolution = []
    
    for state in states:
        step = state['step']
        entities = state['entities']
        
        # Chercher entité 3 (porte)
        door_entities = [e for e in entities if e['value'] == 3]
        
        if not door_entities:
            door_evolution.append({
                'step': step,
                'status': 'ABSENT',
                'door': None
            })
        else:
            # Prendre la plus grande composante (probablement la porte principale)
            door = max(door_entities, key=lambda e: e['pixel_count'])
            door_evolution.append({
                'step': step,
                'status': 'PRESENT',
                'door': {
                    'centroid': door['centroid'],
                    'pixel_count': door['pixel_count'],
                    'bbox': door['bbox']
                }
            })
    
    return door_evolution


def analyze_timer_evolution(states):
    """
    Analyse l'évolution du timer (entité 11) à travers les steps.
    
    Args:
        states: Liste d'états capturés
        
    Returns:
        dict: Analyse de l'évolution du timer
    """
    timer_evolution = []
    
    for state in states:
        step = state['step']
        entities = state['entities']
        
        # Chercher entité 11 (timer)
        timer_entities = [e for e in entities if e['value'] == 11]
        
        if not timer_entities:
            timer_evolution.append({
                'step': step,
                'status': 'ABSENT',
                'timer': None
            })
        else:
            # Prendre la plus grande composante
            timer = max(timer_entities, key=lambda e: e['pixel_count'])
            timer_evolution.append({
                'step': step,
                'status': 'PRESENT',
                'timer': {
                    'centroid': timer['centroid'],
                    'pixel_count': timer['pixel_count'],
                    'bbox': timer['bbox']
                }
            })
    
    return timer_evolution


def detect_events(states):
    """
    Détecte les événements majeurs (changements significatifs) entre steps.
    
    Args:
        states: Liste d'états capturés
        
    Returns:
        list: Liste d'événements détectés
    """
    events = []
    
    for i in range(1, len(states)):
        prev_state = states[i-1]
        curr_state = states[i]
        
        prev_grid = prev_state['grid']
        curr_grid = curr_state['grid']
        
        # Compter pixels modifiés
        diff = (prev_grid != curr_grid)
        modified_pixels = np.sum(diff)
        
        if modified_pixels > 0:
            events.append({
                'from_step': prev_state['step'],
                'to_step': curr_state['step'],
                'modified_pixels': int(modified_pixels),
                'percentage': float(modified_pixels / (64 * 64) * 100)
            })
    
    return events


def main():
    """Fonction principale du test."""
    print("=" * 80)
    print("MAGEN - Phase 4.4.2I-bis: Cartographie Temporelle Complète")
    print("=" * 80)
    print()
    
    # Steps à capturer
    target_steps = [40, 41, 42, 43, 44, 45, 46, 50]
    
    print(f"Steps à capturer: {target_steps}")
    print()
    
    # Capturer états
    print("Capture des états...")
    states = []
    
    for step in target_steps:
        print(f"  Step {step}...", end=" ", flush=True)
        
        # Créer nouvelle instance du jeu pour chaque step
        game = ls20.Ls20()
        game.set_level(0)  # Niveau 1
        state = capture_state_at_step(game, step)
        states.append(state)
        
        print(f"✓ ({len(state['entities'])} entités)")
    
    print()
    
    # Analyser évolution de la porte
    print("Analyse évolution porte (entité 3)...")
    door_evolution = analyze_door_evolution(states)
    
    for entry in door_evolution:
        step = entry['step']
        status = entry['status']
        
        if status == 'PRESENT':
            door = entry['door']
            centroid = door['centroid']
            pixels = door['pixel_count']
            print(f"  Step {step}: PRÉSENTE à ({centroid[0]:.1f}, {centroid[1]:.1f}) - {pixels} pixels")
        else:
            print(f"  Step {step}: ABSENTE")
    
    print()
    
    # Analyser évolution du timer
    print("Analyse évolution timer (entité 11)...")
    timer_evolution = analyze_timer_evolution(states)
    
    for entry in timer_evolution:
        step = entry['step']
        status = entry['status']
        
        if status == 'PRESENT':
            timer = entry['timer']
            centroid = timer['centroid']
            pixels = timer['pixel_count']
            print(f"  Step {step}: PRÉSENT à ({centroid[0]:.1f}, {centroid[1]:.1f}) - {pixels} pixels")
        else:
            print(f"  Step {step}: ABSENT")
    
    print()
    
    # Détecter événements
    print("Détection événements...")
    events = detect_events(states)
    
    for event in events:
        from_step = event['from_step']
        to_step = event['to_step']
        modified = event['modified_pixels']
        percentage = event['percentage']
        print(f"  Step {from_step}→{to_step}: {modified} pixels modifiés ({percentage:.2f}%)")
    
    print()
    
    # Sauvegarder résultats
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = f"phase_4_4_2I_bis_cartographie_temporelle_{timestamp}.json"
    
    # Préparer données pour JSON (convertir numpy arrays)
    json_data = {
        'metadata': {
            'phase': '4.4.2I-bis',
            'test': 'Cartographie Temporelle Complète',
            'timestamp': timestamp,
            'target_steps': target_steps
        },
        'states': [],
        'door_evolution': door_evolution,
        'timer_evolution': timer_evolution,
        'events': events
    }
    
    # Ajouter états (sans grilles complètes pour réduire taille)
    for state in states:
        json_data['states'].append({
            'step': state['step'],
            'entities': state['entities'],
            'grid_hash': state['grid_hash']
        })
    
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(json_data, f, indent=2, ensure_ascii=False)
    
    print(f"Résultats sauvegardés: {output_file}")
    print()
    
    # Analyse finale
    print("=" * 80)
    print("ANALYSE FINALE")
    print("=" * 80)
    print()
    
    # Vérifier stabilité de la porte après step 43
    door_43 = next((e for e in door_evolution if e['step'] == 43), None)
    door_44 = next((e for e in door_evolution if e['step'] == 44), None)
    door_45 = next((e for e in door_evolution if e['step'] == 45), None)
    door_50 = next((e for e in door_evolution if e['step'] == 50), None)
    
    if door_43 and door_43['status'] == 'PRESENT':
        pos_43 = door_43['door']['centroid']
        print(f"Porte au step 43: ({pos_43[0]:.1f}, {pos_43[1]:.1f})")
        
        if door_44 and door_44['status'] == 'PRESENT':
            pos_44 = door_44['door']['centroid']
            delta = (pos_44[0] - pos_43[0], pos_44[1] - pos_43[1])
            print(f"Porte au step 44: ({pos_44[0]:.1f}, {pos_44[1]:.1f}) - Δ=({delta[0]:.1f}, {delta[1]:.1f})")
        
        if door_45 and door_45['status'] == 'PRESENT':
            pos_45 = door_45['door']['centroid']
            delta = (pos_45[0] - pos_43[0], pos_45[1] - pos_43[1])
            print(f"Porte au step 45: ({pos_45[0]:.1f}, {pos_45[1]:.1f}) - Δ=({delta[0]:.1f}, {delta[1]:.1f})")
        
        if door_50 and door_50['status'] == 'PRESENT':
            pos_50 = door_50['door']['centroid']
            delta = (pos_50[0] - pos_43[0], pos_50[1] - pos_43[1])
            print(f"Porte au step 50: ({pos_50[0]:.1f}, {pos_50[1]:.1f}) - Δ=({delta[0]:.1f}, {delta[1]:.1f})")
    
    print()
    
    # Compter événements majeurs
    major_events = [e for e in events if e['modified_pixels'] > 50]
    print(f"Événements majeurs détectés: {len(major_events)}")
    
    for event in major_events:
        print(f"  Step {event['from_step']}→{event['to_step']}: {event['modified_pixels']} pixels")
    
    print()
    print("Test terminé avec succès!")


if __name__ == "__main__":
    main()

# Made with Bob
