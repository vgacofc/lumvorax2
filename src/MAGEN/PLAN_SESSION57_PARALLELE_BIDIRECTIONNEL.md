# PLAN SESSION 57 - DÉVELOPPEMENT PARALLÈLE BIDIRECTIONNEL

**Date**: 2026-06-13 15:59 UTC+2  
**Mode**: Option C - Parallèle avec synergies bidirectionnelles  
**Autorisation**: Utilisateur validé  

---

## 🎯 OBJECTIF GLOBAL

Développer simultanément:
1. **MAGEN** (ARC classique 400 puzzles) - Finaliser transformations
2. **ARC-AGI-3** (Nouvelle compétition interactive) - Créer agent
3. **SYNERGIES**: Partager apprentissages entre les deux projets

---

## 📋 PHASE 57A: MAGEN - Transformations Critiques (1-2h)

### Priorité 1: Compléter 3 Transformations Essentielles

#### 1. ObjectCountingDetector (64.2% impact)
**Fichier**: `core/advanced_pattern_detectors.py:161-168`

**Implémentation**:
```python
def _create_transformation(self, rule: Dict) -> Callable:
    """Crée transformation basée sur règle comptage"""
    rule_type = rule['type']
    value = rule['value']
    
    def transform(grid: np.ndarray) -> np.ndarray:
        # Compter objets actuels
        binary = (grid > 0).astype(int)
        labeled, current_count = label(binary)
        
        # Calculer target
        if rule_type == 'add':
            target_count = current_count + value
        elif rule_type == 'remove':
            target_count = max(0, current_count - value)
        elif rule_type == 'multiply':
            target_count = int(current_count * value)
        elif rule_type == 'divide':
            target_count = int(current_count / value) if value > 0 else current_count
        else:
            return grid.copy()
        
        # Ajuster objets (simplification: duplication/suppression)
        result = grid.copy()
        if target_count > current_count:
            # Ajouter objets (duplication simple)
            for _ in range(target_count - current_count):
                # Dupliquer un objet existant aléatoirement
                pass  # TODO: Implémenter duplication intelligente
        elif target_count < current_count:
            # Supprimer objets
            for obj_id in range(target_count + 1, current_count + 1):
                result[labeled == obj_id] = 0
        
        return result
    
    return transform
```

**Gain attendu**: +15-20 puzzles

#### 2. ConditionalRulesDetector (36.1% impact)
**Fichier**: `core/advanced_pattern_detectors.py:254-264`

**DÉJÀ IMPLÉMENTÉ** ✅ (lignes 254-264)
```python
def _create_color_transformation(self, rules: Dict) -> Callable:
    mapping = rules['mapping']
    def transform(grid: np.ndarray) -> np.ndarray:
        result = grid.copy()
        for color_in, color_out in mapping.items():
            result[grid == color_in] = color_out
        return result
    return transform
```

**Statut**: FONCTIONNEL

#### 3. ColorGradientDetector (33.8% impact)
**Fichier**: `core/advanced_pattern_detectors.py:395-400`

**Implémentation**:
```python
def _create_gradient_transformation(self, gradient_type: str) -> Callable:
    """Crée transformation gradient"""
    def transform(grid: np.ndarray) -> np.ndarray:
        result = grid.copy()
        h, w = result.shape
        
        if gradient_type == 'horizontal':
            # Gradient horizontal (gauche→droite)
            for i in range(h):
                for j in range(w):
                    if result[i, j] > 0:
                        # Appliquer gradient basé sur position
                        result[i, j] = min(15, int(result[i, j] + j * 0.5))
        
        elif gradient_type == 'vertical':
            # Gradient vertical (haut→bas)
            for i in range(h):
                for j in range(w):
                    if result[i, j] > 0:
                        result[i, j] = min(15, int(result[i, j] + i * 0.5))
        
        return result
    
    return transform
```

**Gain attendu**: +10-15 puzzles

### Priorité 2: Transformations Secondaires (si temps)

#### 4. SpatialRelationshipsDetector (34.1% impact)
**DÉJÀ IMPLÉMENTÉ** ✅ (lignes 307-320)

#### 5. GridTilingDetector (32.7% impact)
**À IMPLÉMENTER** si temps disponible

#### 6. DiagonalSymmetryDetector (26.2% impact)
**À IMPLÉMENTER** si temps disponible

### Test MAGEN V16
```bash
cd lumvorax2/src/MAGEN
python3 test_phase2_400_puzzles.py
```

**Objectif**: 65-80/400 (16-20%, +44-78%)

---

## 📋 PHASE 57B: ARC-AGI-3 - Setup & Agent Initial (1-2h)

### Étape 1: Installation Toolkit (10 min)
```bash
cd lumvorax2/src
mkdir ARC_AGI_3
cd ARC_AGI_3

# Créer environnement Python 3.12
python3.12 -m venv venv
source venv/bin/activate

# Installer toolkit
pip install arc-agi

# Configurer clé API
export ARC_API_KEY="b6ebc9c5-a296-4578-be69-0cc16c9455d4"
echo 'ARC_API_KEY=b6ebc9c5-a296-4578-be69-0cc16c9455d4' > .env
```

### Étape 2: Agent MAGEN Adapté (30 min)

**Fichier**: `arc_agi_3_agent.py`

**Stratégie**: Adapter architecture MAGEN pour environnements interactifs

```python
#!/usr/bin/env python3
"""
ARC-AGI-3 Agent basé sur MAGEN
================================

Adapte l'architecture MAGEN (mémoire artificielle) pour 
environnements interactifs ARC-AGI-3.

Composants réutilisés:
- PatternMatcher (détection patterns)
- AdvancedPatternDetectors (6 détecteurs)
- MAGENMemory (apprentissage)
- PWFE (exploration parallèle)

Nouveautés:
- Action selection (7 actions standardisées)
- State tracking (frames multiples)
- Interactive reasoning (percept→plan→action)
"""

import arc_agi
from arcengine import GameAction, GameState
import numpy as np
from typing import List, Tuple, Optional
import sys
import os

# Importer modules MAGEN
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../MAGEN'))
from core.pattern_matcher import PatternMatcher
from core.advanced_pattern_detectors import AdvancedPatternDetectorManager
from core.magen_memory import MAGENMemory

class MAGENArcAgent:
    """Agent ARC-AGI-3 basé sur architecture MAGEN"""
    
    def __init__(self, verbose: bool = True):
        self.verbose = verbose
        
        # Composants MAGEN
        self.pattern_matcher = PatternMatcher(verbose=verbose)
        self.advanced_detectors = AdvancedPatternDetectorManager()
        self.memory = MAGENMemory()
        
        # État agent
        self.frames_history = []
        self.actions_taken = []
        self.current_strategy = None
        
    def reset(self):
        """Reset agent pour nouveau jeu"""
        self.frames_history = []
        self.actions_taken = []
        self.current_strategy = None
    
    def observe(self, frame_data) -> None:
        """Observer frame actuel"""
        self.frames_history.append(frame_data)
        
        if self.verbose:
            print(f"[MAGEN] Frame {len(self.frames_history)}")
            print(f"  State: {frame_data.state}")
            print(f"  Levels completed: {frame_data.levels_completed}")
            print(f"  Available actions: {frame_data.available_actions}")
    
    def analyze_pattern(self, frames: List) -> Optional[str]:
        """Analyser pattern depuis frames"""
        if len(frames) < 2:
            return None
        
        # Extraire grilles depuis frames
        grids = [self._frame_to_grid(f) for f in frames[-3:]]
        
        # Détecter pattern
        if len(grids) >= 2:
            train_pairs = [(grids[i], grids[i+1]) for i in range(len(grids)-1)]
            pattern = self.pattern_matcher.detect_exact_pattern(train_pairs)
            
            if pattern:
                return pattern.name
        
        return None
    
    def choose_action(self, frame_data) -> Tuple[GameAction, dict]:
        """Choisir action basée sur observation"""
        
        # Analyser pattern
        pattern = self.analyze_pattern(self.frames_history)
        
        if pattern:
            if self.verbose:
                print(f"[MAGEN] Pattern détecté: {pattern}")
        
        # Stratégie basée sur actions disponibles
        available = frame_data.available_actions
        
        # Priorité 1: ACTION6 si disponible (click/coordinate)
        if GameAction.ACTION6 in available:
            # Trouver position intéressante
            x, y = self._find_target_position(frame_data)
            return GameAction.ACTION6, {"x": x, "y": y}
        
        # Priorité 2: ACTION5 (interact/execute)
        if GameAction.ACTION5 in available:
            return GameAction.ACTION5, {}
        
        # Priorité 3: Exploration (ACTION1-4)
        for action in [GameAction.ACTION1, GameAction.ACTION2, 
                       GameAction.ACTION3, GameAction.ACTION4]:
            if action in available:
                return action, {}
        
        # Fallback: RESET
        return GameAction.RESET, {}
    
    def _frame_to_grid(self, frame_data) -> np.ndarray:
        """Convertir frame en grille numpy"""
        if hasattr(frame_data, 'frame') and frame_data.frame:
            return np.array(frame_data.frame, dtype=np.int32)
        return np.zeros((8, 8), dtype=np.int32)
    
    def _find_target_position(self, frame_data) -> Tuple[int, int]:
        """Trouver position cible pour ACTION6"""
        grid = self._frame_to_grid(frame_data)
        h, w = grid.shape
        
        # Chercher pixels non-nuls
        non_zero = np.argwhere(grid > 0)
        
        if len(non_zero) > 0:
            # Cliquer sur premier pixel non-nul
            y, x = non_zero[0]
            return int(x), int(y)
        
        # Fallback: centre
        return w // 2, h // 2

def play_game(game_id: str, max_actions: int = 100, seed: int = 0):
    """Jouer un jeu avec agent MAGEN"""
    
    # Créer arcade et agent
    arc = arc_agi.Arcade()
    agent = MAGENArcAgent(verbose=True)
    
    # Créer environnement
    env = arc.make(game_id, seed=seed, render_mode="terminal")
    
    if not env:
        print(f"❌ Impossible de créer environnement {game_id}")
        return
    
    print(f"\n{'='*80}")
    print(f"🎮 JEU: {game_id} (seed={seed})")
    print(f"{'='*80}\n")
    
    # Reset
    obs = env.reset()
    agent.reset()
    agent.observe(obs)
    
    # Boucle jeu
    for step in range(max_actions):
        # Choisir action
        action, data = agent.choose_action(obs)
        
        print(f"\n[Step {step+1}] Action: {action.name}")
        if data:
            print(f"  Data: {data}")
        
        # Exécuter action
        obs = env.step(action, data=data)
        agent.observe(obs)
        
        # Vérifier état
        if obs.state == GameState.WIN:
            print(f"\n🎉 VICTOIRE! (steps={step+1})")
            break
        elif obs.state == GameState.LOSE:
            print(f"\n💀 DÉFAITE (steps={step+1})")
            break
    
    # Scorecard
    scorecard = arc.get_scorecard()
    if scorecard:
        print(f"\n📊 SCORECARD:")
        print(f"  Score: {scorecard.score}")
        print(f"  Games: {len(scorecard.games)}")

if __name__ == "__main__":
    # Test sur jeu exemple
    play_game("ls20", max_actions=50, seed=0)
```

### Étape 3: Test Local (20 min)
```bash
cd lumvorax2/src/ARC_AGI_3
source venv/bin/activate
python3 arc_agi_3_agent.py
```

**Objectif**: Valider agent fonctionne localement

### Étape 4: Préparation Kaggle (30 min)

**Fichier**: `kaggle_submission.py`

Adapter pour format Kaggle selon guide fourni.

---

## 🔄 SYNERGIES BIDIRECTIONNELLES

### MAGEN → ARC-AGI-3
1. **PatternMatcher**: Réutilisé pour détecter patterns dans frames
2. **AdvancedDetectors**: Appliqués aux grilles de jeu
3. **MAGENMemory**: Apprentissage cross-game
4. **PWFE**: Exploration parallèle d'actions

### ARC-AGI-3 → MAGEN
1. **Action Selection**: Inspire nouvelles transformations MAGEN
2. **Interactive Reasoning**: Améliore PWFE
3. **State Tracking**: Enrichit mémoire MAGEN
4. **Multi-frame Analysis**: Patterns temporels pour MAGEN

---

## 📊 MÉTRIQUES SUCCÈS

### MAGEN
- **Baseline**: 45/400 (11.2%)
- **Target V16**: 65-80/400 (16-20%)
- **Gain**: +20-35 puzzles (+44-78%)

### ARC-AGI-3
- **Baseline**: 0 (nouveau)
- **Target**: 1-3 jeux résolus localement
- **Soumission**: Kaggle + arcprize.org

---

## ⏱️ TIMELINE

| Phase | Durée | Tâches |
|-------|-------|--------|
| 57A.1 | 30min | Implémenter ObjectCountingDetector |
| 57A.2 | 20min | Implémenter ColorGradientDetector |
| 57A.3 | 30min | Test MAGEN V16 |
| 57B.1 | 10min | Setup ARC-AGI-3 toolkit |
| 57B.2 | 30min | Créer agent MAGEN adapté |
| 57B.3 | 20min | Test local ARC-AGI-3 |
| 57B.4 | 30min | Préparation Kaggle |
| **TOTAL** | **2h50** | |

---

## 🚀 PROCHAINES ACTIONS IMMÉDIATES

1. ✅ Plan créé
2. ⏳ Implémenter ObjectCountingDetector
3. ⏳ Implémenter ColorGradientDetector
4. ⏳ Test MAGEN V16
5. ⏳ Setup ARC-AGI-3
6. ⏳ Créer agent
7. ⏳ Tests & soumissions

---

**Statut**: ✅ PLAN VALIDÉ - DÉMARRAGE IMMÉDIAT

*Généré par Bob (Mode Advanced)*  
*Session 57 - Développement Parallèle Bidirectionnel*