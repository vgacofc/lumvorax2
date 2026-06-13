"""
Script pour identifier le log du test V22 à 174 succès
"""

import json
from pathlib import Path

forensic_dir = Path("forensic")

print("="*80)
print("RECHERCHE DU LOG V22 À 174 SUCCÈS")
print("="*80)

for log_file in sorted(forensic_dir.glob("lumvorax_phase2_*.json")):
    print(f"\n📂 Analyse: {log_file.name}")
    
    try:
        with open(log_file, 'r') as f:
            data = json.load(f)
        
        events = data.get('events', [])
        
        # Compter succès
        success_count = 0
        total_puzzles = set()
        
        for event in events:
            if event.get('event_type') == 'validation':
                puzzle_id = event.get('data', {}).get('puzzle_id')
                if puzzle_id:
                    total_puzzles.add(puzzle_id)
                    if event.get('data', {}).get('success'):
                        success_count += 1
        
        print(f"   Succès: {success_count}/{len(total_puzzles)}")
        print(f"   Durée: {data.get('total_duration_ns', 0) / 1e9:.2f}s")
        print(f"   Événements: {data.get('event_count', 0)}")
        
        if success_count == 174:
            print(f"\n✅ TROUVÉ ! C'est le bon log : {log_file.name}")
            print(f"   Session ID: {data.get('session_id')}")
            break
    
    except Exception as e:
        print(f"   ❌ Erreur: {e}")

print("\n" + "="*80)

# Made with Bob
