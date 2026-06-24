#!/usr/bin/env python3
"""
PHASE 4.4.2D - TEST CAUSAL ACTION2
==================================

OBJECTIF CRITIQUE:
Tester l'hypothèse "système de graphes spatiaux dynamiques" vs "avatar mobile"

HYPOTHÈSES À TESTER:
H1: ACTION2 = transformation interne du système (changement de rôle des composantes)
H2: ACTION2 = réécriture du graphe de dépendances
H3: ACTION2 = activation conditionnelle de sous-structures
H4: ACTION2 = mouvement classique (hypothèse nulle)

MÉTHODOLOGIE:
1. Exécuter ACTION2 sur état initial
2. Observer TOUTES les composantes (pas juste position)
3. Détecter changements de RÔLE (actif/passif)
4. Comparer avec ACTION1/3/4 (contrôle)
5. Identifier pattern unique ACTION2

CRITÈRE DE SUCCÈS:
- Si ACTION2 = mouvement → H4 validée (avatar classique)
- Si ACTION2 = transformation → H1/H2/H3 validées (système dynamique)

Date: 2026-06-19
Auteur: Bob (MAGEN Project)
"""

import sys
import os
import json
import numpy as np
from datetime import datetime
from scipy import ndimage
from pathlib import Path

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent))

# Import ls20 API
ls20_path = Path(__file__).parent.parent.parent.parent / "environment_files" / "ls20" / "9607627b"
sys.path.insert(0, str(ls20_path))

import ls20
from ls20 import GameAction


class ComponentState:
    """État d'une composante à un instant donné"""
    def __init__(self, value, comp_id, pixels, centroid, active=False):
        self.value = value
        self.comp_id = comp_id
        self.pixels = pixels
        self.centroid = centroid
        self.active = active  # Bouge ou change
        
    def to_dict(self):
        return {
            "value": int(self.value),
            "comp_id": int(self.comp_id),
            "pixel_count": int(self.pixels),
            "centroid": [float(self.centroid[0]), float(self.centroid[1])],
            "active": bool(self.active)
        }


class GraphTransformation:
    """Transformation du graphe de composantes"""
    def __init__(self):
        self.components_before = []
        self.components_after = []
        self.transformations = []
        
    def add_transformation(self, comp_before, comp_after, transform_type):
        """Enregistre une transformation"""
        self.transformations.append({
            "component": f"value_{comp_before.value}_comp_{comp_before.comp_id}",
            "type": transform_type,
            "before": comp_before.to_dict(),
            "after": comp_after.to_dict()
        })


def extract_all_components(pixels):
    """
    Extrait TOUTES les composantes connexes avec IDs stables
    
    Returns:
        dict: {value: [(comp_id, coords, centroid, pixel_count), ...]}
    """
    unique_values = np.unique(pixels)
    unique_values = unique_values[unique_values != 0]  # Ignore background
    
    components = {}
    
    for value in unique_values:
        mask = (pixels == value).astype(int)
        labeled, num_features = ndimage.label(mask)
        
        value_components = []
        for comp_id in range(1, num_features + 1):
            comp_mask = (labeled == comp_id)
            coords = np.argwhere(comp_mask)
            
            if len(coords) > 0:
                centroid = coords.mean(axis=0)
                pixel_count = len(coords)
                value_components.append((comp_id, coords, centroid, pixel_count))
        
        if value_components:
            components[value] = value_components
    
    return components


def detect_role_change(comp_before, comp_after):
    """
    Détecte si une composante a changé de RÔLE (pas juste position)
    
    Returns:
        str: Type de changement ("position", "size", "activation", "none")
    """
    # Changement de taille (pixels)
    if comp_before.pixels != comp_after.pixels:
        return "size_change"
    
    # Changement de position
    dist = np.linalg.norm(comp_before.centroid - comp_after.centroid)
    if dist > 0.1:
        return "position_change"
    
    # Changement d'activation
    if comp_before.active != comp_after.active:
        return "activation_change"
    
    return "none"


def execute_action_and_capture(game, action_enum, action_name):
    """
    Exécute une action et capture l'état complet AVANT/APRÈS
    
    Returns:
        tuple: (state_before, state_after, components_before, components_after)
    """
    # État AVANT
    state_before = game.camera.render(game.current_level._sprites).copy()
    components_before = extract_all_components(state_before)
    
    # Exécuter action
    class ActionInput:
        def __init__(self, action_id):
            self.id = action_id
    
    action_input = ActionInput(action_enum)
    game._set_action(action_input)
    
    # Loop jusqu'à action complète
    steps = 0
    max_steps = 100
    while not game.is_action_complete() and steps < max_steps:
        game.step()
        steps += 1
    
    # État APRÈS
    state_after = game.camera.render(game.current_level._sprites).copy()
    components_after = extract_all_components(state_after)
    
    return state_before, state_after, components_before, components_after


def analyze_graph_transformation(components_before, components_after):
    """
    Analyse la transformation du graphe de composantes
    
    Returns:
        dict: Analyse complète des transformations
    """
    transformations = []
    
    # Pour chaque valeur
    all_values = set(components_before.keys()) | set(components_after.keys())
    
    for value in all_values:
        comps_before = components_before.get(value, [])
        comps_after = components_after.get(value, [])
        
        # Apparition/disparition de composantes
        if len(comps_before) != len(comps_after):
            transformations.append({
                "type": "topology_change",
                "value": int(value),
                "components_before": len(comps_before),
                "components_after": len(comps_after)
            })
        
        # Analyse composante par composante
        for i, (comp_id_b, coords_b, centroid_b, pixels_b) in enumerate(comps_before):
            if i < len(comps_after):
                comp_id_a, coords_a, centroid_a, pixels_a = comps_after[i]
                
                # Changement de taille
                if pixels_b != pixels_a:
                    transformations.append({
                        "type": "size_change",
                        "value": int(value),
                        "comp_id": int(comp_id_b),
                        "pixels_before": int(pixels_b),
                        "pixels_after": int(pixels_a),
                        "delta": int(pixels_a - pixels_b)
                    })
                
                # Changement de position
                dist = np.linalg.norm(centroid_b - centroid_a)
                if dist > 0.1:
                    transformations.append({
                        "type": "position_change",
                        "value": int(value),
                        "comp_id": int(comp_id_b),
                        "centroid_before": [float(centroid_b[0]), float(centroid_b[1])],
                        "centroid_after": [float(centroid_a[0]), float(centroid_a[1])],
                        "distance": float(dist)
                    })
    
    return {
        "total_transformations": len(transformations),
        "transformations": transformations
    }


def test_action2_causality():
    """
    TEST PRINCIPAL: Causalité ACTION2
    """
    print("=" * 80)
    print("PHASE 4.4.2D - TEST CAUSAL ACTION2")
    print("=" * 80)
    print()
    
    # Initialiser jeu
    print("Initialisation du jeu ls20-9607627b...")
    game = ls20.Ls20()
    game.set_level(0)  # Niveau 1
    print("✓ Jeu initialisé")
    print()
    
    # Résultats
    results = {
        "metadata": {
            "phase": "4.4.2D",
            "objective": "Test causal ACTION2 - Hypothèse graphe dynamique",
            "timestamp": datetime.now().isoformat(),
            "game": "ls20-9607627b",
            "level": 1
        },
        "tests": []
    }
    
    # Test 1: ACTION2 (hypothèse à tester)
    print("=" * 80)
    print("TEST 1: ACTION2 (50 états uniques - transformation?)")
    print("=" * 80)
    
    state_b, state_a, comps_b, comps_a = execute_action_and_capture(
        game, GameAction.ACTION2, "ACTION2"
    )
    
    analysis_action2 = analyze_graph_transformation(comps_b, comps_a)
    
    print(f"Transformations détectées: {analysis_action2['total_transformations']}")
    for t in analysis_action2['transformations']:
        print(f"  - {t['type']}: value={t.get('value', 'N/A')}")
    print()
    
    results["tests"].append({
        "action": "ACTION2",
        "state_before_shape": state_b.shape,
        "state_after_shape": state_a.shape,
        "components_before": {int(k): len(v) for k, v in comps_b.items()},
        "components_after": {int(k): len(v) for k, v in comps_a.items()},
        "analysis": analysis_action2
    })
    
    # Reset pour tests de contrôle
    game = ls20.Ls20()
    game.set_level(0)
    
    # Test 2: ACTION1 (contrôle)
    print("=" * 80)
    print("TEST 2: ACTION1 (contrôle - mouvement classique)")
    print("=" * 80)
    
    state_b, state_a, comps_b, comps_a = execute_action_and_capture(
        game, GameAction.ACTION1, "ACTION1"
    )
    
    analysis_action1 = analyze_graph_transformation(comps_b, comps_a)
    
    print(f"Transformations détectées: {analysis_action1['total_transformations']}")
    for t in analysis_action1['transformations']:
        print(f"  - {t['type']}: value={t.get('value', 'N/A')}")
    print()
    
    results["tests"].append({
        "action": "ACTION1",
        "state_before_shape": state_b.shape,
        "state_after_shape": state_a.shape,
        "components_before": {int(k): len(v) for k, v in comps_b.items()},
        "components_after": {int(k): len(v) for k, v in comps_a.items()},
        "analysis": analysis_action1
    })
    
    # Reset
    game = ls20.Ls20()
    game.set_level(0)
    
    # Test 3: ACTION3 (contrôle)
    print("=" * 80)
    print("TEST 3: ACTION3 (contrôle - mouvement classique)")
    print("=" * 80)
    
    state_b, state_a, comps_b, comps_a = execute_action_and_capture(
        game, GameAction.ACTION3, "ACTION3"
    )
    
    analysis_action3 = analyze_graph_transformation(comps_b, comps_a)
    
    print(f"Transformations détectées: {analysis_action3['total_transformations']}")
    for t in analysis_action3['transformations']:
        print(f"  - {t['type']}: value={t.get('value', 'N/A')}")
    print()
    
    results["tests"].append({
        "action": "ACTION3",
        "state_before_shape": state_b.shape,
        "state_after_shape": state_a.shape,
        "components_before": {int(k): len(v) for k, v in comps_b.items()},
        "components_after": {int(k): len(v) for k, v in comps_a.items()},
        "analysis": analysis_action3
    })
    
    # Analyse comparative
    print("=" * 80)
    print("ANALYSE COMPARATIVE")
    print("=" * 80)
    
    action2_transforms = analysis_action2['total_transformations']
    action1_transforms = analysis_action1['total_transformations']
    action3_transforms = analysis_action3['total_transformations']
    
    print(f"ACTION2: {action2_transforms} transformations")
    print(f"ACTION1: {action1_transforms} transformations")
    print(f"ACTION3: {action3_transforms} transformations")
    print()
    
    # Détection pattern unique ACTION2
    action2_types = set(t['type'] for t in analysis_action2['transformations'])
    action1_types = set(t['type'] for t in analysis_action1['transformations'])
    action3_types = set(t['type'] for t in analysis_action3['transformations'])
    
    unique_to_action2 = action2_types - action1_types - action3_types
    
    print("Types de transformations:")
    print(f"  ACTION2: {action2_types}")
    print(f"  ACTION1: {action1_types}")
    print(f"  ACTION3: {action3_types}")
    print(f"  Unique à ACTION2: {unique_to_action2}")
    print()
    
    # Verdict
    print("=" * 80)
    print("VERDICT")
    print("=" * 80)
    
    if unique_to_action2:
        verdict = "TRANSFORMATION_GRAPHE"
        confidence = "HAUTE"
        hypothesis = "H1/H2/H3 validées - Système de graphes dynamiques"
    elif action2_transforms > action1_transforms + action3_transforms:
        verdict = "TRANSFORMATION_COMPLEXE"
        confidence = "MOYENNE"
        hypothesis = "H1/H2 plausibles - Transformations multiples"
    else:
        verdict = "MOUVEMENT_CLASSIQUE"
        confidence = "HAUTE"
        hypothesis = "H4 validée - Avatar mobile classique"
    
    print(f"Verdict: {verdict}")
    print(f"Confiance: {confidence}")
    print(f"Hypothèse: {hypothesis}")
    print()
    
    results["verdict"] = {
        "type": verdict,
        "confidence": confidence,
        "hypothesis": hypothesis,
        "unique_transformations": list(unique_to_action2),
        "action2_complexity": action2_transforms,
        "control_complexity": (action1_transforms + action3_transforms) / 2
    }
    
    # Sauvegarder résultats
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = f"phase_4_4_2D_causal_action2_{timestamp}.json"
    
    with open(output_file, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"✓ Résultats sauvegardés: {output_file}")
    print()
    
    return results


if __name__ == "__main__":
    results = test_action2_causality()
    
    print("=" * 80)
    print("PHASE 4.4.2D COMPLÉTÉE")
    print("=" * 80)
    print()
    print("Prochaine étape: Rapport MD basé sur logs forensiques")

# Made with Bob
