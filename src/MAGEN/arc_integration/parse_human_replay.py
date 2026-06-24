"""
MAGEN Phase 4.6.3 - Parser de Démonstrations HUMANS
Extrait et structure les données de replay depuis l'API ARC Prize
"""

import json
import sys
from pathlib import Path
from typing import List, Dict, Any, Optional
from dataclasses import dataclass, asdict
import numpy as np

@dataclass
class HumanAction:
    """Action humaine extraite du replay"""
    timestamp: str
    frame_number: int
    action_input: Optional[Dict[str, Any]]
    state: str
    levels_completed: int
    grid_state: Optional[np.ndarray]
    
@dataclass
class HumanDemonstration:
    """Démonstration humaine complète"""
    session_id: str
    game_id: str
    total_actions: int
    total_levels_completed: int
    duration_seconds: float
    actions: List[HumanAction]
    success_rate: float
    
    def to_dict(self):
        """Convert to dict for JSON serialization"""
        return {
            'session_id': self.session_id,
            'game_id': self.game_id,
            'total_actions': self.total_actions,
            'total_levels_completed': self.total_levels_completed,
            'duration_seconds': self.duration_seconds,
            'success_rate': self.success_rate,
            'actions': [
                {
                    'timestamp': a.timestamp,
                    'frame_number': a.frame_number,
                    'action_input': a.action_input,
                    'state': a.state,
                    'levels_completed': a.levels_completed,
                    'grid_state': a.grid_state.tolist() if a.grid_state is not None else None
                }
                for a in self.actions
            ]
        }

def parse_ndjson_replay(filepath: str) -> HumanDemonstration:
    """
    Parse un fichier NDJSON de replay ARC Prize
    
    Args:
        filepath: Chemin vers le fichier .bin (NDJSON)
        
    Returns:
        HumanDemonstration structurée
    """
    print(f"📖 Parsing replay: {filepath}")
    
    actions = []
    session_id = None
    game_id = None
    first_timestamp = None
    last_timestamp = None
    
    with open(filepath, 'r') as f:
        for line_num, line in enumerate(f, 1):
            try:
                entry = json.loads(line.strip())
                timestamp = entry['timestamp']
                data = entry['data']
                
                # Extract metadata from first line
                if line_num == 1:
                    session_id = data.get('guid')
                    game_id = data.get('game_id')
                    first_timestamp = timestamp
                
                last_timestamp = timestamp
                
                # Extract grid state from frame
                frame_data = data.get('frame', {})
                grid_state = None
                if 'grid' in frame_data:
                    grid_state = np.array(frame_data['grid'], dtype=np.int8)
                
                # Create action
                action = HumanAction(
                    timestamp=timestamp,
                    frame_number=line_num - 1,
                    action_input=data.get('action_input'),
                    state=data.get('state', 'UNKNOWN'),
                    levels_completed=data.get('levels_completed', 0),
                    grid_state=grid_state
                )
                actions.append(action)
                
                if line_num % 100 == 0:
                    print(f"  ⏳ Parsed {line_num} frames...")
                    
            except Exception as e:
                print(f"  ⚠️  Error parsing line {line_num}: {e}")
                continue
    
    # Calculate duration
    from datetime import datetime
    start = datetime.fromisoformat(first_timestamp.replace('Z', '+00:00'))
    end = datetime.fromisoformat(last_timestamp.replace('Z', '+00:00'))
    duration = (end - start).total_seconds()
    
    # Calculate success rate
    final_levels = actions[-1].levels_completed if actions else 0
    success_rate = final_levels / 7.0  # ls20 has 7 levels
    
    demo = HumanDemonstration(
        session_id=session_id,
        game_id=game_id,
        total_actions=len(actions),
        total_levels_completed=final_levels,
        duration_seconds=duration,
        actions=actions,
        success_rate=success_rate
    )
    
    print(f"\n✅ Parsing complete!")
    print(f"   Session: {session_id}")
    print(f"   Game: {game_id}")
    print(f"   Actions: {len(actions)}")
    print(f"   Levels completed: {final_levels}/7")
    print(f"   Duration: {duration:.1f}s")
    print(f"   Success rate: {success_rate*100:.1f}%")
    
    return demo

def extract_strategy_patterns(demo: HumanDemonstration) -> Dict[str, Any]:
    """
    Extrait les patterns stratégiques de la démonstration
    
    Returns:
        Dict avec patterns détectés
    """
    print("\n🔍 Analyzing strategy patterns...")
    
    patterns = {
        'avg_actions_per_level': demo.total_actions / max(demo.total_levels_completed, 1),
        'avg_time_per_action': demo.duration_seconds / demo.total_actions,
        'level_progression': [],
        'action_types': {}
    }
    
    # Analyze level progression
    current_level = 0
    level_actions = 0
    
    for action in demo.actions:
        if action.levels_completed > current_level:
            patterns['level_progression'].append({
                'level': current_level + 1,
                'actions': level_actions,
                'time': level_actions * patterns['avg_time_per_action']
            })
            current_level = action.levels_completed
            level_actions = 0
        level_actions += 1
    
    # Add final level
    if level_actions > 0:
        patterns['level_progression'].append({
            'level': current_level + 1,
            'actions': level_actions,
            'time': level_actions * patterns['avg_time_per_action']
        })
    
    print(f"   Avg actions/level: {patterns['avg_actions_per_level']:.1f}")
    print(f"   Avg time/action: {patterns['avg_time_per_action']:.2f}s")
    print(f"   Level progression: {len(patterns['level_progression'])} levels")
    
    return patterns

def save_demonstration(demo: HumanDemonstration, output_dir: str = "./human_replays"):
    """Sauvegarde la démonstration en format JSON"""
    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)
    
    # Save full demonstration
    demo_file = output_path / f"demo_{demo.session_id}.json"
    with open(demo_file, 'w') as f:
        json.dump(demo.to_dict(), f, indent=2)
    print(f"\n💾 Saved demonstration: {demo_file}")
    
    # Save strategy patterns
    patterns = extract_strategy_patterns(demo)
    patterns_file = output_path / f"patterns_{demo.session_id}.json"
    with open(patterns_file, 'w') as f:
        json.dump(patterns, f, indent=2)
    print(f"💾 Saved patterns: {patterns_file}")
    
    return demo_file, patterns_file

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python parse_human_replay.py <replay.bin>")
        sys.exit(1)
    
    replay_file = sys.argv[1]
    
    print("=" * 60)
    print("🎯 MAGEN PHASE 4.6.3 - HUMAN REPLAY PARSER")
    print("=" * 60)
    
    # Parse replay
    demo = parse_ndjson_replay(replay_file)
    
    # Save structured data
    save_demonstration(demo)
    
    print("\n" + "=" * 60)
    print("✅ PARSING COMPLETE - Ready for MAGEN integration")
    print("=" * 60)

# Made with Bob
