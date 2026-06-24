#!/usr/bin/env python3
"""
Analyse Logs Forensic V38
=========================

Analyse détaillée des logs générés par les modules cognitifs.
"""

import json
import sys
from pathlib import Path
from collections import defaultdict
from typing import Dict, List


def analyze_log_file(log_path: Path) -> Dict:
    """Analyse un fichier log JSONL."""
    events = []
    
    with open(log_path, 'r') as f:
        for line in f:
            if line.strip():
                try:
                    events.append(json.loads(line))
                except json.JSONDecodeError as e:
                    print(f"⚠️  Erreur parsing ligne: {e}")
    
    # Compter types d'événements
    event_types = defaultdict(int)
    for event in events:
        event_type = event.get('event_type', 'unknown')
        event_types[event_type] += 1
    
    # Extraire données clés
    key_data = []
    for event in events:
        if 'data' in event:
            key_data.append(event['data'])
    
    return {
        'total_events': len(events),
        'event_types': dict(event_types),
        'key_data': key_data,
        'events': events
    }


def analyze_affordance_logs(analysis: Dict):
    """Analyse spécifique logs affordance."""
    print("\n" + "="*80)
    print("ANALYSE DÉTAILLÉE: C0 - AffordanceDiscoveryEngine")
    print("="*80)
    
    events = analysis['events']
    
    # Statistiques découvertes
    discoveries = [e for e in events if e.get('event_type') == 'affordances_discovered']
    if discoveries:
        print(f"\n📊 Découvertes d'affordances: {len(discoveries)}")
        
        # Analyser première découverte
        first = discoveries[0]['data']
        print(f"\nPremière découverte:")
        print(f"  - Shape état: {first.get('state_shape')}")
        print(f"  - Actions disponibles: {first.get('available_actions')}")
        print(f"  - Affordances trouvées: {first.get('affordances_found')}")
        print(f"  - Top affordance: {first.get('top_affordance')}")
        print(f"  - Top plausibility: {first.get('top_plausibility'):.3f}")
        
        context = first.get('context', {})
        print(f"\nContexte perceptuel:")
        print(f"  - Couleurs: {context.get('color_count')}")
        print(f"  - Symétrie: {context.get('has_symmetry')}")
        print(f"  - Patterns: {context.get('has_patterns')}")
        print(f"  - Sparse: {context.get('is_sparse')}")
        print(f"  - Complexité: {context.get('complexity'):.3f}")
    
    # Statistiques filtrage
    filtered = [e for e in events if e.get('event_type') == 'actions_filtered']
    if filtered:
        print(f"\n📊 Filtrages d'actions: {len(filtered)}")
        
        total_available = sum(e['data'].get('available_actions', 0) for e in filtered)
        total_filtered = sum(e['data'].get('filtered_actions', 0) for e in filtered)
        
        print(f"  - Actions disponibles totales: {total_available}")
        print(f"  - Actions filtrées totales: {total_filtered}")
        print(f"  - Taux filtrage moyen: {(1 - total_filtered/total_available)*100:.1f}%")


def analyze_stagnation_logs(analysis: Dict):
    """Analyse spécifique logs stagnation."""
    print("\n" + "="*80)
    print("ANALYSE DÉTAILLÉE: C19 - TrajectoryStagnationDetector")
    print("="*80)
    
    events = analysis['events']
    
    # Statistiques stagnation
    stagnations = [e for e in events if e.get('event_type') == 'stagnation_detected']
    if stagnations:
        print(f"\n📊 Détections de stagnation: {len(stagnations)}")
        
        # Analyser évolution
        for i, stag in enumerate(stagnations[:5], 1):
            data = stag['data']
            print(f"\nDétection #{i}:")
            print(f"  - Stagnant: {data.get('is_stagnant')}")
            print(f"  - Variance: {data.get('variance'):.6f}")
            print(f"  - Loop ratio: {data.get('loop_ratio'):.3f}")
            print(f"  - Unique states ratio: {data.get('unique_states_ratio'):.3f}")
            print(f"  - Should abandon: {data.get('should_abandon')}")
            print(f"  - Confidence: {data.get('confidence'):.3f}")
            print(f"  - Window size: {data.get('window_size')}")
    
    # États ajoutés
    states = [e for e in events if e.get('event_type') == 'state_added']
    if states:
        print(f"\n📊 États ajoutés: {len(states)}")
        print(f"  - Positions uniques: {len(set(str(e['data'].get('position')) for e in states))}")


def analyze_budget_logs(analysis: Dict):
    """Analyse spécifique logs budget."""
    print("\n" + "="*80)
    print("ANALYSE DÉTAILLÉE: C18 - DynamicBudgetAllocator")
    print("="*80)
    
    events = analysis['events']
    
    # Initialisation
    init = [e for e in events if e.get('event_type') == 'allocator_initialized']
    if init:
        data = init[0]['data']
        print(f"\n📊 Configuration:")
        print(f"  - Budget total: {data.get('total_budget', 0):.1f}")
        print(f"  - Allocation min: {data.get('min_allocation', 0):.1f}")
        if 'max_allocation' in data:
            print(f"  - Allocation max: {data.get('max_allocation'):.1f}")
    
    # Allocations
    allocations = [e for e in events if e.get('event_type') == 'budget_allocated']
    if allocations:
        print(f"\n📊 Allocations: {len(allocations)}")
        for alloc in allocations[:5]:
            data = alloc['data']
            print(f"  - Action: {data.get('action_id')}")
            print(f"    Priority: {data.get('priority'):.3f}")
            print(f"    Allocated: {data.get('allocated'):.1f}")
    else:
        print(f"\n⚠️  Aucune allocation effectuée")


def analyze_reputation_logs(analysis: Dict):
    """Analyse spécifique logs réputation."""
    print("\n" + "="*80)
    print("ANALYSE DÉTAILLÉE: C17 - ActionReputationSystem")
    print("="*80)
    
    events = analysis['events']
    
    # Initialisation
    init = [e for e in events if e.get('event_type') == 'system_initialized']
    if init:
        data = init[0]['data']
        print(f"\n📊 Configuration:")
        print(f"  - Decay rate: {data.get('decay_rate', 0):.3f}")
        print(f"  - Min attempts: {data.get('min_attempts', 0)}")
        print(f"  - Exploration threshold: {data.get('exploration_threshold', 0):.3f}")
    
    # Tentatives enregistrées
    attempts = [e for e in events if e.get('event_type') == 'action_attempt_recorded']
    if attempts:
        print(f"\n📊 Tentatives enregistrées: {len(attempts)}")
        for att in attempts[:5]:
            data = att['data']
            print(f"  - Action: {data.get('action_id')}")
            print(f"    Success: {data.get('success')}")
            print(f"    Error: {data.get('error'):.3f}")
    else:
        print(f"\n⚠️  Aucune tentative enregistrée")


def main():
    log_dir = Path("/tmp/magen_v38_simple")
    
    if not log_dir.exists():
        print(f"❌ Répertoire logs non trouvé: {log_dir}")
        return 1
    
    print("="*80)
    print("ANALYSE LOGS FORENSIC V38")
    print("="*80)
    print(f"\n📂 Répertoire: {log_dir}")
    
    log_files = list(log_dir.glob("*.jsonl"))
    print(f"📋 {len(log_files)} fichiers trouvés\n")
    
    # Analyser chaque fichier
    analyses = {}
    for log_file in sorted(log_files):
        print(f"\n{'='*80}")
        print(f"📄 {log_file.name}")
        print('='*80)
        
        analysis = analyze_log_file(log_file)
        analyses[log_file.stem] = analysis
        
        print(f"Événements: {analysis['total_events']}")
        print(f"Types:")
        for event_type, count in sorted(analysis['event_types'].items()):
            print(f"  - {event_type}: {count}")
    
    # Analyses détaillées
    if 'solver_affordance' in analyses:
        analyze_affordance_logs(analyses['solver_affordance'])
    
    if 'solver_stagnation' in analyses:
        analyze_stagnation_logs(analyses['solver_stagnation'])
    
    if 'solver_budget' in analyses:
        analyze_budget_logs(analyses['solver_budget'])
    
    if 'solver_reputation' in analyses:
        analyze_reputation_logs(analyses['solver_reputation'])
    
    print("\n" + "="*80)
    print("FIN ANALYSE")
    print("="*80)
    
    return 0


if __name__ == "__main__":
    sys.exit(main())

# Made with Bob
