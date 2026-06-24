"""
Test V32 - Diagnostic Inertie Cognitive avec Forensic Complet

Objectif: Utiliser forensic V32 pour identifier EXACTEMENT:
1. Quand le système cesse de "penser"
2. Quand le système commence à "tourner"
3. Cause exacte du collapse attractif ACTION1
4. Moment précis de transition cognition → inertie

Méthode:
- Exécuter V32 sur 1 puzzle simple
- Capturer TOUS événements forensic
- Analyser transition cognitive
- Identifier cause racine
"""

import sys
import json
from pathlib import Path
from datetime import datetime
import time

# Ajouter chemin pour imports
sys.path.insert(0, str(Path(__file__).parent))

from core.decision_kernel_v32_spatial import DecisionKernelV32Spatial

def load_simple_puzzle():
    """Charger un puzzle simple pour diagnostic."""
    # Puzzle simple: copier input → output
    return {
        'train': [
            {
                'input': [[1, 2], [3, 4]],
                'output': [[1, 2], [3, 4]]
            }
        ],
        'test': [
            {
                'input': [[5, 6], [7, 8]],
                'output': None  # À prédire
            }
        ]
    }

def simulate_arc_environment(puzzle):
    """Simuler environnement ARC-AGI pour test."""
    test_input = puzzle['test'][0]['input']
    
    # État initial
    observation = {
        'avatar_position': (0, 0),
        'grid': test_input,
        'grid_shape': (len(test_input), len(test_input[0])),
        'step': 0
    }
    
    return observation

def analyze_forensic_logs(log_path):
    """Analyser logs forensic pour détecter inertie cognitive."""
    print(f"\n{'='*80}")
    print("ANALYSE FORENSIC - DÉTECTION INERTIE COGNITIVE")
    print(f"{'='*80}\n")
    
    if not Path(log_path).exists():
        print(f"❌ Fichier log non trouvé: {log_path}")
        return
    
    # Lire tous les événements
    events = []
    with open(log_path, 'r') as f:
        for line in f:
            if line.strip():
                events.append(json.loads(line))
    
    print(f"Total événements: {len(events)}")
    
    # Analyser par type
    event_types = {}
    for event in events:
        event_type = event.get('event', 'unknown')
        event_types[event_type] = event_types.get(event_type, 0) + 1
    
    print(f"\nDistribution événements:")
    for event_type, count in sorted(event_types.items(), key=lambda x: -x[1]):
        print(f"  {event_type}: {count}")
    
    # Détecter patterns de répétition (inertie)
    print(f"\n{'='*80}")
    print("DÉTECTION PATTERNS RÉPÉTITION")
    print(f"{'='*80}\n")
    
    # Analyser séquences d'actions
    action_sequence = []
    for event in events:
        if event.get('event') == 'decision_complete':
            action = event.get('data', {}).get('action_chosen')
            if action:
                action_sequence.append(action)
    
    if action_sequence:
        print(f"Séquence actions ({len(action_sequence)} décisions):")
        print(f"  {action_sequence[:20]}...")  # Premiers 20
        
        # Détecter répétitions
        if len(action_sequence) > 5:
            # Chercher pattern répété
            for pattern_len in range(1, 6):
                pattern = action_sequence[:pattern_len]
                repetitions = 0
                for i in range(0, len(action_sequence) - pattern_len, pattern_len):
                    if action_sequence[i:i+pattern_len] == pattern:
                        repetitions += 1
                    else:
                        break
                
                if repetitions >= 3:
                    print(f"\n⚠️ INERTIE DÉTECTÉE:")
                    print(f"  Pattern: {pattern}")
                    print(f"  Répétitions: {repetitions}")
                    print(f"  Début: step {repetitions * pattern_len}")
                    break
    else:
        print("Aucune décision capturée")
    
    # Analyser santé cognitive
    print(f"\n{'='*80}")
    print("ANALYSE SANTÉ COGNITIVE")
    print(f"{'='*80}\n")
    
    cognitive_health_events = [
        e for e in events 
        if e.get('event') == 'observation_processing_complete'
    ]
    
    if cognitive_health_events:
        print(f"Observations traitées: {len(cognitive_health_events)}")
        
        # Analyser évolution santé cognitive
        health_values = []
        for event in cognitive_health_events:
            health = event.get('data', {}).get('cognitive_health')
            if health is not None:
                health_values.append(health)
        
        if health_values:
            print(f"\nSanté cognitive:")
            print(f"  Initiale: {health_values[0]:.3f}")
            print(f"  Finale: {health_values[-1]:.3f}")
            print(f"  Min: {min(health_values):.3f}")
            print(f"  Max: {max(health_values):.3f}")
            print(f"  Moyenne: {sum(health_values)/len(health_values):.3f}")
            
            # Détecter collapse
            if health_values[-1] < 0.3:
                print(f"\n⚠️ COLLAPSE COGNITIF DÉTECTÉ:")
                print(f"  Santé finale: {health_values[-1]:.3f} < 0.3")
                
                # Trouver moment collapse
                for i, health in enumerate(health_values):
                    if health < 0.3:
                        print(f"  Début collapse: step {i}")
                        break
    
    # Analyser stabilité
    print(f"\n{'='*80}")
    print("ANALYSE STABILITÉ DÉCISIONNELLE")
    print(f"{'='*80}\n")
    
    stability_events = [
        e for e in events
        if 'stability_ok' in e.get('data', {})
    ]
    
    if stability_events:
        unstable_count = sum(
            1 for e in stability_events
            if not e.get('data', {}).get('stability_ok', True)
        )
        
        print(f"Événements stabilité: {len(stability_events)}")
        print(f"Instabilité détectée: {unstable_count} fois")
        
        if unstable_count > 0:
            print(f"\n⚠️ INSTABILITÉ DÉCISIONNELLE:")
            print(f"  Taux: {unstable_count/len(stability_events)*100:.1f}%")
    
    return events

def main():
    """Test principal diagnostic inertie cognitive."""
    print("\n" + "="*80)
    print("TEST V32 - DIAGNOSTIC INERTIE COGNITIVE")
    print("="*80 + "\n")
    
    # Configuration
    log_path = "logs/magen_v32/cognitive_inertia_diagnosis.jsonl"
    max_steps = 50  # Limiter pour diagnostic rapide
    
    # Nettoyer ancien log
    if Path(log_path).exists():
        Path(log_path).unlink()
        print(f"✓ Ancien log nettoyé")
    
    # Charger puzzle
    print(f"✓ Chargement puzzle simple...")
    puzzle = load_simple_puzzle()
    
    # Créer environnement
    print(f"✓ Création environnement ARC...")
    observation = simulate_arc_environment(puzzle)
    
    # Créer DecisionKernel V32 avec forensic
    print(f"✓ Initialisation DecisionKernel V32...")
    print(f"  Log forensic: {log_path}")
    
    kernel = DecisionKernelV32Spatial(forensic_log_path=log_path)
    
    # Simuler exécution
    print(f"\n{'='*80}")
    print(f"EXÉCUTION SIMULATION ({max_steps} steps)")
    print(f"{'='*80}\n")
    
    actions_available = ['up', 'down', 'left', 'right', 'wait']
    action_history = []
    
    for step in range(max_steps):
        # Traiter observation
        cognitive_state = kernel.process_observation(
            observation=observation,
            action_taken=action_history[-1] if action_history else None,
            reward=0.0,
            step=step
        )
        
        # Décider action
        action, justification = kernel.decide_action(
            cognitive_state=cognitive_state,
            available_actions=actions_available
        )
        
        action_history.append(action)
        
        # Afficher progression
        if step % 10 == 0:
            health = cognitive_state['metacognition']['health']['cognitive_health']
            print(f"Step {step:3d}: action={action:5s} health={health:.3f}")
        
        # Simuler nouvelle observation (simplifiée)
        observation['step'] = step + 1
    
    # Fermer kernel
    print(f"\n✓ Fermeture DecisionKernel...")
    kernel.close()
    
    # Analyser logs forensic
    print(f"\n{'='*80}")
    print("ANALYSE LOGS FORENSIC")
    print(f"{'='*80}")
    
    events = analyze_forensic_logs(log_path)
    
    # Rapport final
    print(f"\n{'='*80}")
    print("RAPPORT FINAL")
    print(f"{'='*80}\n")
    
    print(f"Puzzle: Simple copy (2x2)")
    print(f"Steps exécutés: {max_steps}")
    print(f"Actions prises: {len(action_history)}")
    print(f"Événements forensic: {len(events) if events else 0}")
    print(f"Log forensic: {log_path}")
    
    print(f"\nActions distribution:")
    action_counts = {}
    for action in action_history:
        action_counts[action] = action_counts.get(action, 0) + 1
    
    for action, count in sorted(action_counts.items(), key=lambda x: -x[1]):
        pct = count / len(action_history) * 100
        print(f"  {action:5s}: {count:3d} ({pct:5.1f}%)")
    
    # Détecter inertie simple
    if len(action_history) > 10:
        last_10 = action_history[-10:]
        if len(set(last_10)) == 1:
            print(f"\n⚠️ INERTIE SIMPLE DÉTECTÉE:")
            print(f"  Dernières 10 actions identiques: {last_10[0]}")
    
    print(f"\n{'='*80}")
    print("FIN DIAGNOSTIC")
    print(f"{'='*80}\n")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
