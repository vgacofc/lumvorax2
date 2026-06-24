#!/usr/bin/env python3
import json
from pathlib import Path

# Analyse recordings pour patterns victoire
recordings_dir = Path("recordings")
recording_files = list(recordings_dir.glob("*/ls20-*.jsonl"))

print(f"Analyse {len(recording_files)} recordings...")

for rec_file in recording_files[:10]:
    with open(rec_file, 'r') as f:
        for line in f:
            data = json.loads(line)
            if data.get('data', {}).get('levels_completed', 0) > 0:
                print(f"VICTOIRE DÉTECTÉE: {rec_file}")
                print(f"  Level: {data['data']['levels_completed']}")
                print(f"  Action: {data['data'].get('action_input', {})}")

print("Analyse terminée")
