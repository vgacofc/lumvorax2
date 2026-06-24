"""
Analyse Forensic Logs - Extraction Preuves Réelles
===================================================

Analyse les logs forensic pour extraire les métriques RÉELLES d'exécution
et répondre aux questions critiques sur le comportement de MAGEN.

Date: 2026-06-15
Version: V38.2
"""

import json
import sys
from pathlib import Path
from collections import defaultdict, Counter
from typing import Dict, List, Any

def analyze_logs(log_dir: str = "/tmp/magen_unified"):
    """Analyse tous les logs forensic."""
    
    print("=" * 80)
    print("ANALYSE FORENSIC LOGS - PREUVES RÉELLES D'EXÉCUTION")
    print("=" * 80)
    print(f"\nRépertoire: {log_dir}\n")
    
    results = {
        'main': analyze_main_log(f"{log_dir}/solver.jsonl"),
        'affordance': analyze_affordance_log(f"{log_dir}/solver_affordance.jsonl"),
        'reputation': analyze_reputation_log(f"{log_dir}/solver_reputation.jsonl"),
        'budget': analyze_budget_log(f"{log_dir}/solver_budget.jsonl"),
        'stagnation': analyze_stagnation_log(f"{log_dir}/solver_stagnation.jsonl"),
        'world': analyze_world_log(f"{log_dir}/solver_world.jsonl")
    }
    
    # Rapport unifié
    print_unified_report(results)
    
    return results


def analyze_main_log(filepath: str) -> Dict:
    """Analyse solver.jsonl principal."""
    print(f"\n📊 Analyse: {Path(filepath).name}")
    
    events = load_jsonl(filepath)
    
    stats = {
        'total_events': len(events),
        'puzzles_started': 0,
        'puzzles_completed': 0,
        'actions_discovered': 0,
        'transformations_applied': 0,
        'loops_detected': 0,
        'event_types': Counter()
    }
    
    for event in events:
        event_type = event.get('event_type', 'unknown')
        stats['event_types'][event_type] += 1
        
        if event_type == 'puzzle_started':
            stats['puzzles_started'] += 1
        elif event_type == 'puzzle_completed':
            stats['puzzles_completed'] += 1
        elif event_type == 'actions_discovered':
            stats['actions_discovered'] += 1
        elif event_type == 'transformation_applied':
            stats['transformations_applied'] += 1
        elif event_type == 'loop_detected':
            stats['loops_detected'] += 1
    
    print(f"  Total événements: {stats['total_events']}")
    print(f"  Puzzles démarrés: {stats['puzzles_started']}")
    print(f"  Puzzles complétés: {stats['puzzles_completed']}")
    print(f"  Actions découvertes: {stats['actions_discovered']}")
    print(f"  Transformations appliquées: {stats['transformations_applied']}")
    print(f"  Loops détectés: {stats['loops_detected']}")
    
    return stats


def analyze_affordance_log(filepath: str) -> Dict:
    """Analyse solver_affordance.jsonl (PHASE 0)."""
    print(f"\n🔍 Analyse: {Path(filepath).name}")
    
    events = load_jsonl(filepath)
    
    stats = {
        'total_events': len(events),
        'discoveries': 0,
        'filterings': 0,
        'total_actions_generated': 0,
        'total_actions_filtered': 0,
        'avg_reduction_rate': 0.0
    }
    
    reduction_rates = []
    
    for event in events:
        event_type = event.get('event_type', '')
        
        if 'discover' in event_type.lower():
            stats['discoveries'] += 1
            data = event.get('data', {})
            stats['total_actions_generated'] += data.get('num_discovered', 0)
        
        elif 'filter' in event_type.lower():
            stats['filterings'] += 1
            data = event.get('data', {})
            reduction = data.get('reduction_rate', 0.0)
            if reduction > 0:
                reduction_rates.append(reduction)
    
    if reduction_rates:
        stats['avg_reduction_rate'] = sum(reduction_rates) / len(reduction_rates)
    
    print(f"  Total événements: {stats['total_events']}")
    print(f"  Découvertes: {stats['discoveries']}")
    print(f"  Filtrages: {stats['filterings']}")
    print(f"  Taux réduction moyen: {stats['avg_reduction_rate']*100:.1f}%")
    
    return stats


def analyze_reputation_log(filepath: str) -> Dict:
    """Analyse solver_reputation.jsonl (PHASE 2 - C17)."""
    print(f"\n⭐ Analyse: {Path(filepath).name}")
    
    events = load_jsonl(filepath)
    
    stats = {
        'total_events': len(events),
        'actions_tracked': set(),
        'total_attempts': 0,
        'total_successes': 0,
        'avg_priority': 0.0,
        'learning_events': 0
    }
    
    priorities = []
    
    for event in events:
        data = event.get('data', {})
        
        if 'action' in data or 'action_id' in data:
            action_id = data.get('action') or data.get('action_id')
            stats['actions_tracked'].add(action_id)
        
        if 'success' in data:
            stats['total_attempts'] += 1
            if data['success']:
                stats['total_successes'] += 1
            stats['learning_events'] += 1
        
        if 'priority' in data:
            priorities.append(data['priority'])
    
    if priorities:
        stats['avg_priority'] = sum(priorities) / len(priorities)
    
    stats['unique_actions'] = len(stats['actions_tracked'])
    stats['actions_tracked'] = list(stats['actions_tracked'])[:10]  # Top 10
    
    print(f"  Total événements: {stats['total_events']}")
    print(f"  Actions uniques: {stats['unique_actions']}")
    print(f"  Tentatives totales: {stats['total_attempts']}")
    print(f"  Succès totaux: {stats['total_successes']}")
    if stats['total_attempts'] > 0:
        print(f"  Taux succès: {stats['total_successes']/stats['total_attempts']*100:.1f}%")
    print(f"  Priorité moyenne: {stats['avg_priority']:.3f}")
    
    return stats


def analyze_budget_log(filepath: str) -> Dict:
    """Analyse solver_budget.jsonl (PHASE 3 - C18)."""
    print(f"\n💰 Analyse: {Path(filepath).name}")
    
    events = load_jsonl(filepath)
    
    stats = {
        'total_events': len(events),
        'allocations': 0,
        'consumptions': 0,
        'exhaustions': 0,
        'total_allocated': 0.0,
        'total_consumed': 0.0,
        'budget_critical_events': 0
    }
    
    for event in events:
        event_type = event.get('event_type', '')
        data = event.get('data', {})
        
        if 'allocat' in event_type.lower():
            stats['allocations'] += 1
            stats['total_allocated'] += data.get('allocated', 0.0)
        
        elif 'consum' in event_type.lower():
            stats['consumptions'] += 1
            stats['total_consumed'] += data.get('amount', 0.0)
        
        elif 'exhaust' in event_type.lower():
            stats['exhaustions'] += 1
        
        elif 'critical' in event_type.lower():
            stats['budget_critical_events'] += 1
    
    print(f"  Total événements: {stats['total_events']}")
    print(f"  Allocations: {stats['allocations']}")
    print(f"  Consommations: {stats['consumptions']}")
    print(f"  Budget alloué: {stats['total_allocated']:.1f}")
    print(f"  Budget consommé: {stats['total_consumed']:.1f}")
    print(f"  Épuisements: {stats['exhaustions']}")
    
    return stats


def analyze_stagnation_log(filepath: str) -> Dict:
    """Analyse solver_stagnation.jsonl (PHASE 4 - C19)."""
    print(f"\n🔄 Analyse: {Path(filepath).name}")
    
    events = load_jsonl(filepath)
    
    stats = {
        'total_events': len(events),
        'stagnations_detected': 0,
        'trajectories_abandoned': 0,
        'strategy_changes': 0,
        'avg_loop_ratio': 0.0,
        'avg_variance': 0.0
    }
    
    loop_ratios = []
    variances = []
    
    for event in events:
        event_type = event.get('event_type', '')
        data = event.get('data', {})
        
        if 'stagnation' in event_type.lower():
            stats['stagnations_detected'] += 1
            
            if 'abandon' in event_type.lower():
                stats['trajectories_abandoned'] += 1
        
        if 'strategy' in event_type.lower():
            stats['strategy_changes'] += 1
        
        if 'loop_ratio' in data:
            loop_ratios.append(data['loop_ratio'])
        
        if 'variance' in data:
            variances.append(data['variance'])
    
    if loop_ratios:
        stats['avg_loop_ratio'] = sum(loop_ratios) / len(loop_ratios)
    
    if variances:
        stats['avg_variance'] = sum(variances) / len(variances)
    
    print(f"  Total événements: {stats['total_events']}")
    print(f"  Stagnations détectées: {stats['stagnations_detected']}")
    print(f"  Trajectoires abandonnées: {stats['trajectories_abandoned']}")
    print(f"  Changements stratégie: {stats['strategy_changes']}")
    print(f"  Loop ratio moyen: {stats['avg_loop_ratio']:.3f}")
    
    return stats


def analyze_world_log(filepath: str) -> Dict:
    """Analyse solver_world.jsonl (World Model)."""
    print(f"\n🗺️  Analyse: {Path(filepath).name}")
    
    events = load_jsonl(filepath)
    
    stats = {
        'total_events': len(events),
        'regions_discovered': set(),
        'positions_visited': set(),
        'coherence_updates': 0,
        'avg_coherence': 0.0
    }
    
    coherences = []
    
    for event in events:
        data = event.get('data', {})
        
        if 'region' in data:
            stats['regions_discovered'].add(str(data['region']))
        
        if 'position' in data or 'agent_pos' in data:
            pos = data.get('position') or data.get('agent_pos')
            if pos:
                stats['positions_visited'].add(str(pos))
        
        if 'coherence' in data:
            coherences.append(data['coherence'])
            stats['coherence_updates'] += 1
    
    if coherences:
        stats['avg_coherence'] = sum(coherences) / len(coherences)
    
    stats['unique_regions'] = len(stats['regions_discovered'])
    stats['unique_positions'] = len(stats['positions_visited'])
    
    print(f"  Total événements: {stats['total_events']}")
    print(f"  Régions uniques: {stats['unique_regions']}")
    print(f"  Positions uniques: {stats['unique_positions']}")
    print(f"  Cohérence moyenne: {stats['avg_coherence']:.3f}")
    
    return stats


def load_jsonl(filepath: str) -> List[Dict]:
    """Charge un fichier JSONL."""
    events = []
    try:
        with open(filepath, 'r') as f:
            for line in f:
                if line.strip():
                    events.append(json.loads(line))
    except FileNotFoundError:
        print(f"  ⚠️  Fichier non trouvé: {filepath}")
        return []
    except Exception as e:
        print(f"  ❌ Erreur lecture: {e}")
        return []
    
    return events


def print_unified_report(results: Dict):
    """Affiche rapport unifié."""
    print("\n" + "=" * 80)
    print("RAPPORT UNIFIÉ - PREUVES RÉELLES")
    print("=" * 80)
    
    # Métriques globales
    total_events = sum(r.get('total_events', 0) for r in results.values())
    print(f"\n📈 MÉTRIQUES GLOBALES:")
    print(f"   Total événements forensic: {total_events}")
    
    # Puzzles
    main = results.get('main', {})
    print(f"\n🧩 PUZZLES ARC-AGI:")
    print(f"   Démarrés: {main.get('puzzles_started', 0)}")
    print(f"   Complétés: {main.get('puzzles_completed', 0)}")
    print(f"   Transformations appliquées: {main.get('transformations_applied', 0)}")
    
    # Architecture cognitive
    print(f"\n🧠 ARCHITECTURE COGNITIVE:")
    
    affordance = results.get('affordance', {})
    print(f"   PHASE 0 (Affordances):")
    print(f"     - Découvertes: {affordance.get('discoveries', 0)}")
    print(f"     - Réduction actions: {affordance.get('avg_reduction_rate', 0)*100:.1f}%")
    
    reputation = results.get('reputation', {})
    print(f"   PHASE 2 (Réputation):")
    print(f"     - Actions uniques: {reputation.get('unique_actions', 0)}")
    print(f"     - Apprentissages: {reputation.get('learning_events', 0)}")
    print(f"     - Taux succès: {reputation.get('total_successes', 0)}/{reputation.get('total_attempts', 0)}")
    
    budget = results.get('budget', {})
    print(f"   PHASE 3 (Budget):")
    print(f"     - Allocations: {budget.get('allocations', 0)}")
    print(f"     - Consommé: {budget.get('total_consumed', 0):.1f}")
    print(f"     - Épuisements: {budget.get('exhaustions', 0)}")
    
    stagnation = results.get('stagnation', {})
    print(f"   PHASE 4 (Stagnation):")
    print(f"     - Détections: {stagnation.get('stagnations_detected', 0)}")
    print(f"     - Abandons: {stagnation.get('trajectories_abandoned', 0)}")
    
    world = results.get('world', {})
    print(f"   World Model:")
    print(f"     - Régions: {world.get('unique_regions', 0)}")
    print(f"     - Positions: {world.get('unique_positions', 0)}")
    print(f"     - Cohérence: {world.get('avg_coherence', 0):.3f}")
    
    print("\n" + "=" * 80)


if __name__ == '__main__':
    log_dir = sys.argv[1] if len(sys.argv) > 1 else "/tmp/magen_unified"
    analyze_logs(log_dir)

# Made with Bob
