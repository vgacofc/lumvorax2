#!/usr/bin/env python3
"""
Analyse structure visuelle du jeu ls20-9607627b
Extrait patterns, objets, et mécaniques depuis recordings JSONL
"""

import json
import numpy as np
from pathlib import Path
from collections import Counter
import matplotlib.pyplot as plt
from datetime import datetime

class GameStructureAnalyzer:
    def __init__(self, recording_path):
        self.recording_path = Path(recording_path)
        self.frames = []
        self.actions = []
        self.states = []
        self.levels = []
        
    def load_recording(self):
        """Charge un recording JSONL"""
        print(f"📂 Chargement: {self.recording_path}")
        
        with open(self.recording_path, 'r') as f:
            for line in f:
                data = json.loads(line)
                if 'data' in data:
                    d = data['data']
                    self.frames.append(np.array(d['frame'][0]))
                    self.actions.append(d.get('action_input', {}).get('id', 'RESET'))
                    self.states.append(d['state'])
                    self.levels.append(d['levels_completed'])
        
        print(f"✅ {len(self.frames)} frames chargées")
        return len(self.frames)
    
    def analyze_color_palette(self):
        """Analyse palette couleurs utilisées"""
        print("\n🎨 ANALYSE PALETTE COULEURS")
        print("=" * 60)
        
        all_values = []
        for frame in self.frames:
            all_values.extend(frame.flatten().tolist())
        
        counter = Counter(all_values)
        
        print(f"\nValeurs uniques: {len(counter)}")
        print("\nDistribution:")
        for value, count in sorted(counter.items()):
            pct = (count / len(all_values)) * 100
            bar = "█" * int(pct / 2)
            print(f"  Valeur {value:2d}: {count:6d} ({pct:5.2f}%) {bar}")
        
        return counter
    
    def analyze_frame_structure(self, frame_idx=0):
        """Analyse structure d'une frame spécifique"""
        print(f"\n🔍 ANALYSE FRAME #{frame_idx}")
        print("=" * 60)
        
        frame = self.frames[frame_idx]
        h, w = frame.shape
        
        print(f"\nDimensions: {h}x{w}")
        print(f"Action: {self.actions[frame_idx]}")
        print(f"State: {self.states[frame_idx]}")
        print(f"Level: {self.levels[frame_idx]}")
        
        # Détecte régions
        unique_vals = np.unique(frame)
        print(f"\nValeurs présentes: {unique_vals.tolist()}")
        
        # Analyse par région
        print("\n📍 RÉGIONS DÉTECTÉES:")
        for val in unique_vals:
            if val == 4:  # Background
                continue
            positions = np.argwhere(frame == val)
            if len(positions) > 0:
                min_y, min_x = positions.min(axis=0)
                max_y, max_x = positions.max(axis=0)
                count = len(positions)
                print(f"  Valeur {val:2d}: {count:4d} pixels, région ({min_y:2d},{min_x:2d}) → ({max_y:2d},{max_x:2d})")
        
        return frame
    
    def detect_objects(self, frame):
        """Détecte objets distincts dans frame"""
        print("\n🎯 OBJETS DÉTECTÉS:")
        print("=" * 60)
        
        # Légende valeurs (basé sur observation)
        legend = {
            0: "Vide/Noir",
            1: "Agent/Joueur",
            3: "Mur/Bordure",
            4: "Background",
            5: "Zone claire",
            8: "Objectif type 1",
            9: "Objectif type 2",
            10: "Agent (autre représentation)",
            11: "Zone spéciale",
            12: "Transformateur"
        }
        
        unique_vals = np.unique(frame)
        for val in unique_vals:
            positions = np.argwhere(frame == val)
            desc = legend.get(val, "Inconnu")
            print(f"  [{val:2d}] {desc:25s}: {len(positions):4d} pixels")
        
        # Détecte agent
        agent_vals = [1, 10]
        for av in agent_vals:
            if av in unique_vals:
                pos = np.argwhere(frame == av)
                if len(pos) > 0:
                    print(f"\n🤖 AGENT DÉTECTÉ (valeur {av}):")
                    print(f"   Position: {pos[0].tolist()}")
                    print(f"   Pixels: {len(pos)}")
        
        # Détecte objectifs
        obj_vals = [8, 9, 12]
        objectives = []
        for ov in obj_vals:
            if ov in unique_vals:
                pos = np.argwhere(frame == ov)
                if len(pos) > 0:
                    center = pos.mean(axis=0).astype(int)
                    objectives.append((ov, center, len(pos)))
        
        if objectives:
            print(f"\n🎯 OBJECTIFS DÉTECTÉS: {len(objectives)}")
            for val, center, count in objectives:
                print(f"   Type {val}: centre {center.tolist()}, {count} pixels")
        
        return objectives
    
    def compare_frames(self, idx1=0, idx2=1):
        """Compare deux frames pour détecter changements"""
        print(f"\n🔄 COMPARAISON FRAMES #{idx1} vs #{idx2}")
        print("=" * 60)
        
        f1 = self.frames[idx1]
        f2 = self.frames[idx2]
        
        diff = np.abs(f1 - f2)
        changes = np.sum(diff > 0)
        total = f1.size
        pct = (changes / total) * 100
        
        print(f"\nAction: {self.actions[idx2]}")
        print(f"Pixels changés: {changes}/{total} ({pct:.2f}%)")
        
        if changes > 0:
            print("\nChangements détectés:")
            changed_pos = np.argwhere(diff > 0)
            for pos in changed_pos[:10]:  # Limite à 10
                y, x = pos
                print(f"  Position ({y},{x}): {f1[y,x]} → {f2[y,x]}")
            if len(changed_pos) > 10:
                print(f"  ... et {len(changed_pos)-10} autres")
        
        return changes, pct
    
    def visualize_frame(self, frame_idx=0, save_path=None):
        """Visualise une frame avec matplotlib"""
        frame = self.frames[frame_idx]
        
        plt.figure(figsize=(12, 12))
        plt.imshow(frame, cmap='tab20', interpolation='nearest')
        plt.colorbar(label='Valeur pixel')
        plt.title(f'Frame #{frame_idx} - Action: {self.actions[frame_idx]} - Level: {self.levels[frame_idx]}')
        plt.xlabel('X')
        plt.ylabel('Y')
        plt.grid(True, alpha=0.3)
        
        if save_path:
            plt.savefig(save_path, dpi=150, bbox_inches='tight')
            print(f"💾 Sauvegardé: {save_path}")
        else:
            plt.show()
        
        plt.close()
    
    def analyze_action_effects(self):
        """Analyse effet de chaque action"""
        print("\n⚡ ANALYSE EFFETS ACTIONS")
        print("=" * 60)
        
        action_effects = {}
        
        for i in range(1, len(self.frames)):
            action = self.actions[i]
            if action == 'RESET':
                continue
            
            changes, pct = self.compare_frames(i-1, i)
            
            if action not in action_effects:
                action_effects[action] = []
            action_effects[action].append((changes, pct))
        
        print("\nRésumé par action:")
        for action, effects in sorted(action_effects.items()):
            if effects:
                avg_changes = np.mean([e[0] for e in effects])
                avg_pct = np.mean([e[1] for e in effects])
                print(f"  {action:10s}: {avg_changes:6.1f} pixels changés ({avg_pct:5.2f}%)")
        
        return action_effects
    
    def generate_report(self, output_path):
        """Génère rapport complet"""
        print("\n📊 GÉNÉRATION RAPPORT")
        print("=" * 60)
        
        report = {
            "timestamp": datetime.now().isoformat(),
            "recording": str(self.recording_path),
            "total_frames": len(self.frames),
            "total_actions": len(self.actions),
            "final_state": self.states[-1] if self.states else None,
            "final_level": self.levels[-1] if self.levels else None,
            "color_palette": {},
            "frame_analysis": {},
            "action_effects": {}
        }
        
        # Palette
        counter = self.analyze_color_palette()
        report["color_palette"] = dict(counter)
        
        # Frame initiale
        self.analyze_frame_structure(0)
        
        # Objets
        objectives = self.detect_objects(self.frames[0])
        report["frame_analysis"]["objectives"] = [
            {"type": int(val), "center": center.tolist(), "pixels": count}
            for val, center, count in objectives
        ]
        
        # Effets actions
        action_effects = self.analyze_action_effects()
        report["action_effects"] = {}
        for action, effects in action_effects.items():
            if effects:
                report["action_effects"][action] = {
                    "avg_changes": float(np.mean([e[0] for e in effects])),
                    "avg_pct": float(np.mean([e[1] for e in effects]))
                }
        
        # Sauvegarde
        with open(output_path, 'w') as f:
            json.dump(report, f, indent=2)
        
        print(f"\n✅ Rapport sauvegardé: {output_path}")
        return report


def main():
    print("=" * 60)
    print("ANALYSE STRUCTURE JEU LS20-9607627B")
    print("=" * 60)
    
    # Trouve premier recording
    recordings_dir = Path("recordings")
    recording_files = list(recordings_dir.glob("*/ls20-*.jsonl"))
    
    if not recording_files:
        print("❌ Aucun recording trouvé")
        return
    
    recording_path = recording_files[0]
    print(f"\n📁 Recording: {recording_path}")
    
    # Analyse
    analyzer = GameStructureAnalyzer(recording_path)
    analyzer.load_recording()
    
    # Palette couleurs
    analyzer.analyze_color_palette()
    
    # Frame initiale
    frame = analyzer.analyze_frame_structure(0)
    analyzer.detect_objects(frame)
    
    # Comparaison frames
    if len(analyzer.frames) > 1:
        analyzer.compare_frames(0, 1)
        if len(analyzer.frames) > 10: analyzer.compare_frames(0, 10)
    
    # Effets actions
    analyzer.analyze_action_effects()
    
    # Visualisation
    viz_dir = Path("results/visualizations")
    viz_dir.mkdir(parents=True, exist_ok=True)
    
    # Visualise frames clés
    for idx in [0, 10, 50, -1]:
        if idx == -1:
            idx = len(analyzer.frames) - 1
        if idx < len(analyzer.frames):
            save_path = viz_dir / f"frame_{idx:03d}.png"
            analyzer.visualize_frame(idx, save_path)
    
    # Rapport JSON
    report_path = Path("results/game_structure_analysis.json")
    report_path.parent.mkdir(parents=True, exist_ok=True)
    analyzer.generate_report(report_path)
    
    print("\n" + "=" * 60)
    print("✅ ANALYSE TERMINÉE")
    print("=" * 60)
    print(f"\n📊 Rapport: {report_path}")
    print(f"🖼️  Visualisations: {viz_dir}")


if __name__ == "__main__":
    main()

# Made with Bob
