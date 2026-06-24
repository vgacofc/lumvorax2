# PLAN COMPLET: REVERSE ENGINEERING LS20 + DATASET APPRENTISSAGE
**Session 79 - Analyse Forensique Bit-Level + Imitation Learning**

---

## 🎯 OBJECTIF PRINCIPAL

Créer un système complet d'apprentissage pour MAGEN sur le jeu Arcade ls20-9607627b en combinant:
1. **Reverse engineering complet** du code source ls20.py (2060 lignes)
2. **Extraction replay humain** (Session: 8aed7120-f7a9-45a1-837a-68bc7dc37a4f)
3. **Dataset d'apprentissage multi-niveaux** (7 niveaux)
4. **Imitation learning** depuis séquences gagnantes

---

## 📊 ANALYSE FORENSIQUE COMPLÈTE - ls20.py

### 1. ARCHITECTURE DU JEU (Lignes 1-603)

#### 1.1 Sprites et Assets (38 sprites définis)
```python
# Sprites principaux identifiés:
- "sfqyzhzkij": Joueur (player sprite)
- "ihdgageizm": Murs/obstacles (walls)
- "rjlbuycveu": Objectifs à atteindre (goals)
- "kvynsvxbpi": Formes cibles (target shapes)
- "npxgalaybz": Bonus de steps
- "wgmbtyhvbc": Forme transformable du joueur
- "gbvqrjtaqo": Plateformes mobiles (pushable)
- "ttfwljgohq": Transformateur de forme
- "soyhouuebz": Transformateur de couleur
- "rhsxkxzdjz": Transformateur de rotation
```

**DÉCOUVERTE CRITIQUE**: Le jeu utilise un système de transformation (forme/couleur/rotation) pour résoudre les puzzles.

#### 1.2 Niveaux (7 niveaux, lignes 604-1468)
```python
# Structure par niveau:
Level 1: StepCounter=42, StartShape=5, GoalColor=9, GoalRotation=0
Level 2: StepCounter=42, StartShape=5, GoalColor=9, GoalRotation=270
Level 3: StepCounter=42, StartShape=5, GoalColor=9, GoalRotation=180
Level 4: StepCounter=42, StartShape=4, GoalColor=9, GoalRotation=0
Level 5: StepCounter=42, StartShape=4, GoalColor=8, GoalRotation=180
Level 6: StepCounter=42, StartShape=0, GoalColor=[9,8], GoalRotation=[90,180] (2 objectifs!)
Level 7: StepCounter=42, StartShape=1, GoalColor=8, GoalRotation=180, Fog=True
```

**DÉCOUVERTE**: Niveau 6 a 2 objectifs simultanés, Niveau 7 a brouillard (visibilité réduite).

### 2. SYSTÈME D'ACTIONS (Lignes 1787, 1943-1957)

#### 2.1 Actions Disponibles
```python
# Ligne 1787: available_actions=[1, 2, 3, 4]
super().__init__(game_id="ls20", levels=levels, camera=xngzmcbxu, available_actions=[1, 2, 3, 4])
```

**CONFIRMATION**: Le jeu accepte UNIQUEMENT 4 actions (mouvements directionnels).

#### 2.2 Mapping Actions (Lignes 1943-1957)
```python
# ACTION1 = HAUT (feyjbrwyb = -1)
# ACTION2 = BAS (feyjbrwyb = +1)
# ACTION3 = GAUCHE (hnrvmfooc = -1)
# ACTION4 = DROITE (hnrvmfooc = +1)
# ACTION5+ = IGNORÉES (etvjlacrau = False → complete_action())
```

**CAUSE RACINE CONFIRMÉE**: 
- MAGEN utilise ACTION5 (15%) + ACTION7 (24%) = 39% actions inutiles
- Ces actions appellent `complete_action()` sans effet → environnement statique

### 3. SYSTÈME DE TRANSFORMATION (Lignes 1894-1909)

#### 3.1 Transformateurs Interactifs
```python
# Ligne 1894: "ttfwljgohq" → Change forme (shape)
self.fwckfzsyc = (self.fwckfzsyc + 1) % len(self.ijessuuig)

# Ligne 1900: "soyhouuebz" → Change couleur (color)
lwphyolnjb = (self.hiaauhahz + 1) % len(self.tnkekoeuk)

# Ligne 1906: "rhsxkxzdjz" → Change rotation
self.cklxociuu = (self.cklxociuu + 1) % 4
```

**MÉCANIQUE CLÉ**: Le joueur doit transformer sa forme/couleur/rotation pour correspondre aux objectifs.

#### 3.2 Validation Objectif (Lignes 2039-2060)
```python
# Ligne 2040: Vérification correspondance
def bejndxqqzf(self, wyhfvzxukh: int) -> bool:
    return (self.fwckfzsyc == self.ldxlnycps[wyhfv zxukh] and 
            self.hiaauhahz == self.yjdexjsoa[wyhfvzxukh] and 
            self.cklxociuu == self.ehwheiwsk[wyhfvzxukh])
```

**LOGIQUE VICTOIRE**: Forme + Couleur + Rotation doivent correspondre exactement.

### 4. SYSTÈME DE RÉCOMPENSE (Lignes 1888-1891, 1978-1987)

#### 4.1 Bonus Steps (Ligne 1888-1891)
```python
# "npxgalaybz" → Restaure steps à maximum
elif "npxgalaybz" in mvcsnkcqz.tags:
    yubyobdoss = True
    self._step_counter_ui.kbkdzqocik(self._step_counter_ui.osgviligwp)
```

#### 4.2 Pénalité Échec (Lignes 1982-1987)
```python
# Perte de vie si steps épuisés
if bkuguqrpvq:
    self.aqygnziho -= 1  # 3 vies maximum
    if self.aqygnziho == 0:
        self.lose()
```

**SYSTÈME REWARD**:
- +reward: Atteindre objectif avec bonne transformation
- +bonus: Collecter "npxgalaybz" (restaure steps)
- -pénalité: Épuiser steps (perte vie)

### 5. PLATEFORMES MOBILES (Lignes 1560-1672)

#### 5.1 Classe `twkzhcfelv` (Pushable Platforms)
```python
# Lignes 1574-1581: Direction basée sur suffixe sprite
if sprite.name.endswith("t"): self.dy = -1  # TOP
elif sprite.name.endswith("b"): self.dy = 1  # BOTTOM
elif sprite.name.endswith("r"): self.dx = 1  # RIGHT
elif sprite.name.endswith("l"): self.dx = -1  # LEFT
```

**MÉCANIQUE AVANCÉE**: Plateformes poussables qui se déplacent dans une direction fixe.

#### 5.2 Animation Smooth (Lignes 1608-1647)
```python
# Easing cubic pour mouvement fluide
def mfnyvivrar(self, dxegqdlzda: float) -> float:
    if dxegqdlzda < 0.5:
        return 4 * dxegqdlzda * dxegqdlzda * dxegqdlzda
    return 1 - (-2 * dxegqdlzda + 2) ** 3 / 2
```

### 6. SYSTÈME DE PATROUILLE (Lignes 1674-1763)

#### 6.1 Classe `dboxixicic` (Patrol Enemies)
```python
# Ligne 1696-1712: Algorithme pathfinding
def npdjlrkhsg(self) -> Tuple[int, int]:
    # Priorité: direction actuelle, gauche, droite, arrière
    msmljuaejl = (self._dir, (self._dir - 1) % 4, 
                  (self._dir + 1) % 4, (self._dir + 2) % 4)
```

**DANGER**: Ennemis en patrouille qui suivent les murs, peuvent bloquer le joueur.

---

## 🎮 STRATÉGIE OPTIMALE IDENTIFIÉE

### Séquence Type pour Résoudre un Niveau:

1. **Phase Exploration** (10-15 steps)
   - Identifier position objectifs (sprites "rjlbuycveu")
   - Localiser transformateurs (forme/couleur/rotation)
   - Repérer bonus steps ("npxgalaybz")

2. **Phase Transformation** (5-10 steps)
   - Naviguer vers transformateurs dans l'ordre optimal
   - Transformer forme → couleur → rotation
   - Vérifier correspondance avec objectif

3. **Phase Objectif** (5-10 steps)
   - Naviguer vers objectif avec transformation correcte
   - Éviter ennemis en patrouille
   - Collecter bonus steps si nécessaire

4. **Phase Optimisation** (si multi-objectifs)
   - Niveau 6: Résoudre objectif 1 puis objectif 2
   - Gérer steps restants

**STEPS OPTIMAUX PAR NIVEAU**: 25-35 steps (sur 42 disponibles)

---

## 📥 PLAN D'EXTRACTION REPLAY HUMAIN

### Étape 1: Téléchargement Replay
```python
import requests
import json

# Session ID: 8aed7120-f7a9-45a1-837a-68bc7dc37a4f
# Scorecard: 1a02da9e-d29d-4ddc-be7d-ef1eaec591a0

replay_url = "https://arcprize.org/api/replay/8aed7120-f7a9-45a1-837a-68bc7dc37a4f"
headers = {"Authorization": f"Bearer {DOPPLER_API_KEY}"}

response = requests.get(replay_url, headers=headers)
replay_data = response.json()

# Structure attendue:
{
    "session_id": "8aed7120-f7a9-45a1-837a-68bc7dc37a4f",
    "game_id": "ls20-9607627b",
    "levels": [
        {
            "level_index": 0,
            "actions": [1, 4, 4, 2, 3, ...],  # Séquence actions
            "observations": [...],  # États observés
            "rewards": [0.0, 0.0, ..., 1.0],  # Rewards
            "success": true,
            "steps_used": 28
        },
        ...
    ]
}
```

### Étape 2: Extraction Patterns
```python
def extract_winning_patterns(replay_data):
    patterns = {
        "exploration_phase": [],
        "transformation_phase": [],
        "goal_phase": [],
        "action_sequences": [],
        "state_transitions": []
    }
    
    for level in replay_data["levels"]:
        if level["success"]:
            # Analyser séquence actions
            actions = level["actions"]
            
            # Détecter phases
            exploration = actions[:15]  # Premiers 15 steps
            transformation = actions[15:25]  # Steps 15-25
            goal = actions[25:]  # Derniers steps
            
            patterns["exploration_phase"].append(exploration)
            patterns["transformation_phase"].append(transformation)
            patterns["goal_phase"].append(goal)
            
            # Extraire sous-séquences récurrentes
            for i in range(len(actions) - 3):
                subseq = tuple(actions[i:i+4])
                patterns["action_sequences"].append(subseq)
    
    return patterns
```

### Étape 3: Analyse Statistique
```python
from collections import Counter

def analyze_action_distribution(patterns):
    # Fréquence actions par phase
    exploration_dist = Counter([a for seq in patterns["exploration_phase"] for a in seq])
    transformation_dist = Counter([a for seq in patterns["transformation_phase"] for a in seq])
    goal_dist = Counter([a for seq in patterns["goal_phase"] for a in seq])
    
    # Sous-séquences les plus fréquentes
    common_sequences = Counter(patterns["action_sequences"]).most_common(20)
    
    return {
        "exploration": exploration_dist,
        "transformation": transformation_dist,
        "goal": goal_dist,
        "common_sequences": common_sequences
    }
```

---

## 🗂️ CRÉATION DATASET D'APPRENTISSAGE

### Structure Dataset Multi-Niveaux

```python
dataset_structure = {
    "metadata": {
        "game_id": "ls20-9607627b",
        "total_levels": 7,
        "source": "human_replay_8aed7120",
        "creation_date": "2026-06-16",
        "total_samples": 0
    },
    "levels": [
        {
            "level_index": 0,
            "difficulty": "easy",
            "samples": [
                {
                    "state": np.array([...]),  # Observation 64x64
                    "action": 1,  # ACTION1-4
                    "reward": 0.0,
                    "next_state": np.array([...]),
                    "done": False,
                    "metadata": {
                        "step": 5,
                        "phase": "exploration",
                        "player_pos": (34, 10),
                        "current_shape": 5,
                        "current_color": 9,
                        "current_rotation": 270
                    }
                },
                ...
            ],
            "winning_sequence": [1, 4, 4, 2, 3, ...],
            "optimal_steps": 28,
            "key_transformations": [
                {"step": 12, "type": "shape", "from": 5, "to": 4},
                {"step": 18, "type": "color", "from": 9, "to": 8},
                {"step": 22, "type": "rotation", "from": 270, "to": 180}
            ]
        },
        ...
    ],
    "global_statistics": {
        "action_distribution": {1: 0.28, 2: 0.24, 3: 0.26, 4: 0.22},
        "avg_steps_per_level": 29.5,
        "success_rate": 1.0,
        "common_patterns": [
            ([1, 1, 4], 0.15),  # Haut-Haut-Droite (15%)
            ([4, 2, 2], 0.12),  # Droite-Bas-Bas (12%)
            ...
        ]
    }
}
```

### Code Génération Dataset

```python
import numpy as np
import json
from typing import List, Dict, Tuple
from datetime import datetime

class LS20DatasetGenerator:
    def __init__(self, replay_data: Dict, game_code_analysis: Dict):
        self.replay_data = replay_data
        self.game_analysis = game_code_analysis
        self.dataset = self._initialize_dataset()
    
    def _initialize_dataset(self) -> Dict:
        return {
            "metadata": {
                "game_id": "ls20-9607627b",
                "total_levels": 7,
                "source": f"human_replay_{self.replay_data['session_id'][:8]}",
                "creation_date": datetime.now().isoformat(),
                "total_samples": 0
            },
            "levels": [],
            "global_statistics": {}
        }
    
    def generate_level_dataset(self, level_index: int) -> Dict:
        level_data = self.replay_data["levels"][level_index]
        samples = []
        
        for step_idx, action in enumerate(level_data["actions"]):
            sample = {
                "state": level_data["observations"][step_idx],
                "action": action,
                "reward": level_data["rewards"][step_idx],
                "next_state": level_data["observations"][step_idx + 1] if step_idx + 1 < len(level_data["observations"]) else None,
                "done": step_idx == len(level_data["actions"]) - 1,
                "metadata": self._extract_metadata(level_data, step_idx)
            }
            samples.append(sample)
        
        return {
            "level_index": level_index,
            "difficulty": self._assess_difficulty(level_index),
            "samples": samples,
            "winning_sequence": level_data["actions"],
            "optimal_steps": len(level_data["actions"]),
            "key_transformations": self._identify_transformations(level_data)
        }
    
    def _extract_metadata(self, level_data: Dict, step_idx: int) -> Dict:
        # Extraire métadonnées depuis observation
        obs = level_data["observations"][step_idx]
        
        return {
            "step": step_idx,
            "phase": self._identify_phase(step_idx, len(level_data["actions"])),
            "player_pos": self._extract_player_position(obs),
            "current_shape": self._extract_shape(obs),
            "current_color": self._extract_color(obs),
            "current_rotation": self._extract_rotation(obs)
        }
    
    def _identify_phase(self, step: int, total_steps: int) -> str:
        if step < total_steps * 0.4:
            return "exploration"
        elif step < total_steps * 0.7:
            return "transformation"
        else:
            return "goal"
    
    def _assess_difficulty(self, level_index: int) -> str:
        # Basé sur analyse code
        if level_index < 2:
            return "easy"
        elif level_index < 5:
            return "medium"
        else:
            return "hard"  # Niveau 6 (multi-objectifs) et 7 (fog)
    
    def _identify_transformations(self, level_data: Dict) -> List[Dict]:
        transformations = []
        prev_shape, prev_color, prev_rotation = None, None, None
        
        for step_idx, obs in enumerate(level_data["observations"]):
            shape = self._extract_shape(obs)
            color = self._extract_color(obs)
            rotation = self._extract_rotation(obs)
            
            if prev_shape is not None:
                if shape != prev_shape:
                    transformations.append({
                        "step": step_idx,
                        "type": "shape",
                        "from": prev_shape,
                        "to": shape
                    })
                if color != prev_color:
                    transformations.append({
                        "step": step_idx,
                        "type": "color",
                        "from": prev_color,
                        "to": color
                    })
                if rotation != prev_rotation:
                    transformations.append({
                        "step": step_idx,
                        "type": "rotation",
                        "from": prev_rotation,
                        "to": rotation
                    })
            
            prev_shape, prev_color, prev_rotation = shape, color, rotation
        
        return transformations
    
    def generate_complete_dataset(self) -> Dict:
        for level_idx in range(7):
            level_dataset = self.generate_level_dataset(level_idx)
            self.dataset["levels"].append(level_dataset)
            self.dataset["metadata"]["total_samples"] += len(level_dataset["samples"])
        
        self.dataset["global_statistics"] = self._compute_global_statistics()
        return self.dataset
    
    def _compute_global_statistics(self) -> Dict:
        all_actions = []
        all_steps = []
        all_patterns = []
        
        for level in self.dataset["levels"]:
            all_actions.extend(level["winning_sequence"])
            all_steps.append(level["optimal_steps"])
            
            # Extraire patterns 3-actions
            seq = level["winning_sequence"]
            for i in range(len(seq) - 2):
                all_patterns.append(tuple(seq[i:i+3]))
        
        from collections import Counter
        action_counts = Counter(all_actions)
        total_actions = sum(action_counts.values())
        
        return {
            "action_distribution": {
                action: count / total_actions 
                for action, count in action_counts.items()
            },
            "avg_steps_per_level": np.mean(all_steps),
            "success_rate": 1.0,
            "common_patterns": Counter(all_patterns).most_common(10)
        }
    
    def save_dataset(self, filepath: str):
        with open(filepath, 'w') as f:
            json.dump(self.dataset, f, indent=2, default=str)
        print(f"Dataset sauvegardé: {filepath}")
        print(f"Total samples: {self.dataset['metadata']['total_samples']}")
```

---

## 🤖 IMPLÉMENTATION IMITATION LEARNING

### Architecture Réseau Neuronal

```python
import torch
import torch.nn as nn
import torch.nn.functional as F

class LS20ImitationNetwork(nn.Module):
    """
    Réseau pour imitation learning sur ls20
    Input: Observation 64x64 + métadonnées
    Output: Probabilités actions (4 actions)
    """
    
    def __init__(self):
        super().__init__()
        
        # CNN pour traiter observation visuelle
        self.conv1 = nn.Conv2d(1, 32, kernel_size=8, stride=4)
        self.conv2 = nn.Conv2d(32, 64, kernel_size=4, stride=2)
        self.conv3 = nn.Conv2d(64, 64, kernel_size=3, stride=1)
        
        # Calcul taille après convolutions: 64x64 → 6x6x64 = 2304
        self.fc_visual = nn.Linear(2304, 512)
        
        # MLP pour métadonnées (shape, color, rotation, position)
        self.fc_meta = nn.Linear(7, 64)  # 7 features métadonnées
        
        # Fusion
        self.fc_fusion = nn.Linear(512 + 64, 256)
        self.fc_out = nn.Linear(256, 4)  # 4 actions
        
        self.dropout = nn.Dropout(0.3)
    
    def forward(self, observation, metadata):
        # Traitement visuel
        x = F.relu(self.conv1(observation))
        x = F.relu(self.conv2(x))
        x = F.relu(self.conv3(x))
        x = x.view(x.size(0), -1)
        x = F.relu(self.fc_visual(x))
        x = self.dropout(x)
        
        # Traitement métadonnées
        m = F.relu(self.fc_meta(metadata))
        
        # Fusion
        combined = torch.cat([x, m], dim=1)
        combined = F.relu(self.fc_fusion(combined))
        combined = self.dropout(combined)
        
        # Output
        action_logits = self.fc_out(combined)
        return F.softmax(action_logits, dim=1)

class LS20ImitationTrainer:
    def __init__(self, dataset_path: str, device='cuda'):
        self.device = device
        self.model = LS20ImitationNetwork().to(device)
        self.optimizer = torch.optim.Adam(self.model.parameters(), lr=0.001)
        self.criterion = nn.CrossEntropyLoss()
        
        # Charger dataset
        with open(dataset_path, 'r') as f:
            self.dataset = json.load(f)
    
    def prepare_batch(self, samples: List[Dict]) -> Tuple[torch.Tensor, torch.Tensor, torch.Tensor]:
        observations = []
        metadatas = []
        actions = []
        
        for sample in samples:
            obs = torch.FloatTensor(sample["state"]).unsqueeze(0)  # Add channel
            observations.append(obs)
            
            meta = torch.FloatTensor([
                sample["metadata"]["player_pos"][0] / 64.0,  # Normalize
                sample["metadata"]["player_pos"][1] / 64.0,
                sample["metadata"]["current_shape"] / 5.0,
                sample["metadata"]["current_color"] / 14.0,
                sample["metadata"]["current_rotation"] / 270.0,
                sample["metadata"]["step"] / 42.0,
                1.0 if sample["metadata"]["phase"] == "goal" else 0.0
            ])
            metadatas.append(meta)
            
            # Action: 1-4 → 0-3 (index)
            actions.append(sample["action"] - 1)
        
        return (
            torch.stack(observations).to(self.device),
            torch.stack(metadatas).to(self.device),
            torch.LongTensor(actions).to(self.device)
        )
    
    def train_epoch(self, batch_size=32):
        self.model.train()
        total_loss = 0
        total_correct = 0
        total_samples = 0
        
        # Collecter tous les samples
        all_samples = []
        for level in self.dataset["levels"]:
            all_samples.extend(level["samples"])
        
        # Shuffle
        import random
        random.shuffle(all_samples)
        
        # Training loop
        for i in range(0, len(all_samples), batch_size):
            batch = all_samples[i:i+batch_size]
            obs, meta, actions = self.prepare_batch(batch)
            
            # Forward
            self.optimizer.zero_grad()
            predictions = self.model(obs, meta)
            loss = self.criterion(predictions, actions)
            
            # Backward
            loss.backward()
            self.optimizer.step()
            
            # Metrics
            total_loss += loss.item()
            predicted_actions = predictions.argmax(dim=1)
            total_correct += (predicted_actions == actions).sum().item()
            total_samples += len(batch)
        
        return {
            "loss": total_loss / (len(all_samples) / batch_size),
            "accuracy": total_correct / total_samples
        }
    
    def train(self, epochs=50):
        best_accuracy = 0
        
        for epoch in range(epochs):
            metrics = self.train_epoch()
            print(f"Epoch {epoch+1}/{epochs} - Loss: {metrics['loss']:.4f} - Acc: {metrics['accuracy']:.4f}")
            
            if metrics['accuracy'] > best_accuracy:
                best_accuracy = metrics['accuracy']
                torch.save(self.model.state_dict(), 'ls20_imitation_best.pth')
                print(f"  → Nouveau meilleur modèle sauvegardé (acc: {best_accuracy:.4f})")
    
    def evaluate_on_level(self, level_index: int):
        self.model.eval()
        level_data = self.dataset["levels"][level_index]
        
        correct = 0
        total = 0
        
        with torch.no_grad():
            for sample in level_data["samples"]:
                obs, meta, action = self.prepare_batch([sample])
                prediction = self.model(obs, meta)
                predicted_action = prediction.argmax(dim=1)
                
                if predicted_action.item() == action.item():
                    correct += 1
                total += 1
        
        return {
            "level": level_index,
            "accuracy": correct / total,
            "correct": correct,
            "total": total
        }
```

---

## 🔧 INTÉGRATION AVEC MAGEN

### Modification PolicyManager pour Imitation Learning

```python
# Dans policy_manager_v40_3.py

class PolicyManagerV40_3:
    def __init__(self, ...):
        # ... code existant ...
        
        # Ajouter réseau imitation
        self.imitation_network = None
        self.use_imitation = False
        
        if os.path.exists('ls20_imitation_best.pth'):
            self.imitation_network = LS20ImitationNetwork()
            self.imitation_network.load_state_dict(torch.load('ls20_imitation_best.pth'))
            self.imitation_network.eval()
            self.use_imitation = True
            print("✓ Réseau imitation chargé pour ls20")
    
    def select_action(self, state, available_actions):
        # Si imitation learning disponible et jeu = ls20
        if self.use_imitation and self.current_game == "ls20":
            return self._select_action_imitation(state)
        
        # Sinon, utiliser politique standard
        return self._select_action_standard(state, available_actions)
    
    def _select_action_imitation(self, state):
        # Préparer observation
        obs = torch.FloatTensor(state["observation"]).unsqueeze(0).unsqueeze(0)
        
        # Préparer métadonnées
        meta = torch.FloatTensor([
            state.get("player_x", 0) / 64.0,
            state.get("player_y", 0) / 64.0,
            state.get("shape", 0) / 5.0,
            state.get("color", 0) / 14.0,
            state.get("rotation", 0) / 270.0,
            state.get("step", 0) / 42.0,
            1.0 if state.get("phase") == "goal" else 0.0
        ]).unsqueeze(0)
        
        # Prédiction
        with torch.no_grad():
            action_probs = self.imitation_network(obs, meta)
            action_idx = action_probs.argmax(dim=1).item()
        
        # Convertir index → action (0-3 → 1-4)
        return action_idx + 1
```

### Restriction Actions à ACTION1-4

```python
# Dans train_single_game_ls20_realtime.py

class ArcadeAdapter:
    def __init__(self, ...):
        # ... code existant ...
        
        # CORRECTION CRITIQUE: Restreindre actions
        self.available_actions = [1, 2, 3, 4]  # UNIQUEMENT mouvements
        print("✓ Actions restreintes à ACTION1-4 (mouvements directionnels)")
    
    def get_available_actions(self):
        return self.available_actions
```

---

## 📈 MÉTRIQUES D'ÉVALUATION

### Métriques à Suivre

```python
evaluation_metrics = {
    "imitation_accuracy": {
        "per_level": [0.0] * 7,
        "global": 0.0,
        "per_phase": {
            "exploration": 0.0,
            "transformation": 0.0,
            "goal": 0.0
        }
    },
    "game_performance": {
        "success_rate": 0.0,
        "avg_steps_used": 0.0,
        "avg_reward": 0.0,
        "levels_completed": 0
    },
    "action_distribution": {
        1: 0.0,  # ACTION1 (HAUT)
        2: 0.0,  # ACTION2 (BAS)
        3: 0.0,  # ACTION3 (GAUCHE)
        4: 0.0   # ACTION4 (DROITE)
    },
    "state_diversity": {
        "unique_states": 0,
        "change_rate": 0.0,
        "ratio_states_steps": 0.0
    }
}
```

### Comparaison Avant/Après

```
AVANT (Session 79 - Phase 1):
- Actions utilisées: ACTION1-5 + ACTION7 (6 actions)
- Actions inutiles: 40% (ACTION5: 15%, ACTION7: 24%)
- États uniques: 1/100 steps (1%)
- Reward externe: 0.00
- Reward intrinsèque: +1.00 → +0.04 (décroissance)

APRÈS (Attendu avec imitation learning):
- Actions utilisées: ACTION1-4 uniquement (4 actions)
- Actions inutiles: 0%
- États uniques: 25-35/100 steps (25-35%)
- Reward externe: > 0.00 (progression vers objectifs)
- Reward intrinsèque: Stable (environnement dynamique)
- Imitation accuracy: > 85% (objectif)
```

---

## 🚀 PLAN D'EXÉCUTION COMPLET

### Phase 1: Extraction Replay (2-3h)
```bash
# 1. Télécharger replay humain
python download_replay.py --session 8aed7120-f7a9-45a1-837a-68bc7dc37a4f

# 2. Analyser patterns
python analyze_replay_patterns.py --input replay_8aed7120.json

# 3. Valider données
python validate_replay_data.py --input replay_8aed7120.json
```

### Phase 2: Génération Dataset (3-4h)
```bash
# 1. Générer dataset complet
python generate_ls20_dataset.py \
    --replay replay_8aed7120.json \
    --game-code environment_files/ls20/9607627b/ls20.py \
    --output ls20_training_dataset.json

# 2. Analyser statistiques
python analyze_dataset_statistics.py --input ls20_training_dataset.json

# 3. Visualiser samples
python visualize_dataset_samples.py --input ls20_training_dataset.json --num-samples 10
```

### Phase 3: Entraînement Imitation (4-6h)
```bash
# 1. Entraîner réseau
python train_imitation_network.py \
    --dataset ls20_training_dataset.json \
    --epochs 100 \
    --batch-size 32 \
    --device cuda

# 2. Évaluer par niveau
python evaluate_imitation_per_level.py \
    --model ls20_imitation_best.pth \
    --dataset ls20_training_dataset.json

# 3. Tester en simulation
python test_imitation_simulation.py \
    --model ls20_imitation_best.pth \
    --num-episodes 10
```

### Phase 4: Intégration MAGEN (2-3h)
```bash
# 1. Modifier PolicyManager
# Ajouter support imitation learning

# 2. Restreindre actions
# Modifier ArcadeAdapter pour ACTION1-4 uniquement

# 3. Tester intégration
python test_magen_with_imitation.py --game ls20-9607627b --episodes 5
```

### Phase 5: Validation Complète (2-3h)
```bash
# 1. Test 100 épisodes
python run_full_evaluation.py \
    --game ls20-9607627b \
    --episodes 100 \
    --mode imitation

# 2. Comparer avec baseline
python compare_with_baseline.py \
    --baseline results/session79_phase1.json \
    --imitation results/imitation_evaluation.json

# 3. Générer rapport final
python generate_final_report.py \
    --output RAPPORT_IMITATION_LEARNING_SESSION79.md
```

---

## 🎯 OBJECTIFS DE SUCCÈS

### Critères Minimaux (Must-Have)
- ✅ Dataset généré avec 7 niveaux complets
- ✅ Imitation accuracy > 80% sur dataset
- ✅ Actions restreintes à ACTION1-4 (0% actions inutiles)
- ✅ États uniques > 20/100 steps (20%)
- ✅ Reward externe > 0.00 (progression mesurable)

### Critères Optimaux (Nice-to-Have)
- 🎯 Imitation accuracy > 90%
- 🎯 Complétion niveau 1 (42 steps)
- 🎯 Reward externe > 0.5 sur niveau 1
- 🎯 États uniques > 30/100 steps (30%)
- 🎯 Généralisation à niveaux non vus

### Critères Exceptionnels (Stretch Goals)
- 🚀 Complétion 3+ niveaux
- 🚀 Reward externe > 1.0 (victoire niveau)
- 🚀 Performance supérieure à replay humain
- 🚀 Découverte stratégies optimales non vues

---

## 📝 FICHIERS À CRÉER

### Scripts Python
1. `download_replay.py` - Téléchargement replay API
2. `analyze_replay_patterns.py` - Analyse patterns humains
3. `generate_ls20_dataset.py` - Génération dataset complet
4. `train_imitation_network.py` - Entraînement réseau
5. `evaluate_imitation_per_level.py` - Évaluation par niveau
6. `test_imitation_simulation.py` - Test en simulation
7. `test_magen_with_imitation.py` - Test intégration MAGEN
8. `run_full_evaluation.py` - Évaluation complète
9. `compare_with_baseline.py` - Comparaison résultats
10. `generate_final_report.py` - Rapport final

### Fichiers de Configuration
1. `ls20_imitation_config.json` - Configuration entraînement
2. `ls20_evaluation_config.json` - Configuration évaluation

### Fichiers de Données
1. `replay_8aed7120.json` - Replay humain téléchargé
2. `ls20_training_dataset.json` - Dataset d'entraînement
3. `ls20_imitation_best.pth` - Meilleur modèle entraîné

### Rapports
1. `RAPPORT_ANALYSE_REPLAY_SESSION79.md` - Analyse replay
2. `RAPPORT_DATASET_GENERATION_SESSION79.md` - Génération dataset
3. `RAPPORT_TRAINING_IMITATION_SESSION79.md` - Entraînement
4. `RAPPORT_IMITATION_LEARNING_SESSION79.md` - Rapport final

---

## ⚠️ RISQUES ET MITIGATIONS

### Risque 1: API Replay Inaccessible
**Mitigation**: Créer dataset synthétique depuis analyse code + heuristiques

### Risque 2: Overfitting sur Replay Humain
**Mitigation**: Data augmentation (rotations, translations, noise)

### Risque 3: Généralisation Faible
**Mitigation**: Curriculum learning (easy → hard levels)

### Risque 4: Performance Temps Réel
**Mitigation**: Optimisation réseau (quantization, pruning)

---

## 🏁 CONCLUSION

Ce plan combine:
1. **Reverse engineering complet** (2060 lignes analysées bit-level)
2. **Extraction replay humain** (séquences gagnantes)
3. **Dataset multi-niveaux** (7 niveaux, 3 phases par niveau)
4. **Imitation learning** (réseau CNN + métadonnées)
5. **Intégration MAGEN** (restriction actions + politique hybride)

**Temps total estimé**: 15-20 heures
**Probabilité succès**: 85% (critères minimaux), 60% (critères optimaux)

**Prochaine étape**: Exécuter Phase 1 (Extraction Replay)