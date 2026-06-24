# PLAN D'IMPLÉMENTATION PHASE 1
## Couches 1-2: Vision et Objets

**Date**: 2026-06-18  
**Durée estimée**: 1 semaine  
**Objectif**: Implémenter les fondations de l'apprentissage visuel de MAGEN

---

## 🎯 OBJECTIFS DE LA PHASE 1

### Couche 1: Vision Primitive
- ✅ Détection automatique des valeurs uniques
- ✅ Analyse des changements entre frames
- ✅ Détection de patterns visuels récurrents
- ✅ Classification des comportements (statique, mobile, décroissant)

### Couche 2: Objets et Identités
- ✅ Identification des composantes connexes
- ✅ Classification automatique par taille/forme
- ✅ Détection de structures composites (blocs 5×5)
- ✅ Distinction mobile vs fixe

---

## 📋 ARCHITECTURE DES MODULES

### Structure des Fichiers

```
lumvorax2/src/MAGEN/
├── learning_system/
│   ├── __init__.py
│   ├── layer1_vision.py          # Couche 1: Vision
│   ├── layer2_objects.py         # Couche 2: Objets
│   ├── primitives.py             # Fonctions primitives
│   └── data_structures.py        # Structures de données
├── tests/
│   ├── test_layer1_vision.py
│   └── test_layer2_objects.py
└── examples/
    └── example_ls20_discovery.py  # Exemple sur ls20-9607627b
```

---

## 🔧 MODULE 1: Primitives

### Fichier: `primitives.py`

```python
"""
Fonctions primitives pour l'apprentissage de MAGEN.
Ces fonctions représentent les capacités de base dont MAGEN dispose.
"""

import numpy as np
from typing import List, Tuple, Dict, Any

def read_pixels(frame: np.ndarray) -> np.ndarray:
    """Lire les pixels d'une frame."""
    return np.array(frame)

def compare_frames(frame1: np.ndarray, frame2: np.ndarray) -> np.ndarray:
    """Comparer deux frames et retourner les différences."""
    return frame1 != frame2

def measure_distance(pos1: np.ndarray, pos2: np.ndarray) -> float:
    """Mesurer la distance euclidienne entre deux positions."""
    return np.linalg.norm(pos1 - pos2)

def detect_change(frame1: np.ndarray, frame2: np.ndarray) -> Dict[str, Any]:
    """Détecter les changements entre deux frames."""
    diff_mask = compare_frames(frame1, frame2)
    changed_positions = np.argwhere(diff_mask)
    
    changes = {
        'has_change': len(changed_positions) > 0,
        'num_changes': len(changed_positions),
        'positions': changed_positions,
        'change_rate': len(changed_positions) / frame1.size
    }
    
    return changes
```

---

## 🔧 MODULE 2: Structures de Données

### Fichier: `data_structures.py`

```python
"""
Structures de données pour l'apprentissage de MAGEN.
"""

from dataclasses import dataclass, field
from typing import List, Tuple, Dict, Any, Optional
import numpy as np
from datetime import datetime

@dataclass
class Value:
    """Représente une valeur observée dans le jeu."""
    value: int
    first_seen: datetime = field(default_factory=datetime.now)
    count: int = 0
    positions: List[Tuple[int, int]] = field(default_factory=list)
    behavior: str = 'unknown'  # 'static', 'mobile', 'decreasing', 'increasing'
    change_rate: float = 0.0

@dataclass
class Component:
    """Représente une composante connexe."""
    id: str
    value: int
    size: int
    positions: np.ndarray
    centroid: np.ndarray
    bbox: Dict[str, int]
    first_seen: datetime = field(default_factory=datetime.now)
    is_mobile: Optional[bool] = None
    movement_history: List[np.ndarray] = field(default_factory=list)

@dataclass
class CompositeStructure:
    """Représente une structure composite (ex: bloc 5×5)."""
    type: str  # 'block_5x5', 'pattern_3x3', etc.
    position: Tuple[int, int]
    values: Dict[int, int]  # {value: count}
    pattern_score: float
    components: List[Component]
    is_mobile: Optional[bool] = None

@dataclass
class VisualLibrary:
    """Bibliothèque des observations visuelles."""
    values: Dict[int, Value] = field(default_factory=dict)
    patterns: Dict[int, List[str]] = field(default_factory=dict)
    change_rates: Dict[int, float] = field(default_factory=dict)
    
    def register_value(self, value: int, count: int, positions: List):
        """Enregistrer une nouvelle valeur."""
        if value not in self.values:
            self.values[value] = Value(value=value, count=count, positions=positions)
        else:
            self.values[value].count = count
            self.values[value].positions = positions

@dataclass
class ObjectLibrary:
    """Bibliothèque des objets identifiés."""
    components: Dict[int, List[Component]] = field(default_factory=dict)
    structures: List[CompositeStructure] = field(default_factory=list)
    mobile_objects: List[Component] = field(default_factory=list)
    fixed_objects: List[Component] = field(default_factory=list)
    
    def add_component(self, component: Component):
        """Ajouter une composante."""
        if component.value not in self.components:
            self.components[component.value] = []
        self.components[component.value].append(component)
```

---

## 🔧 MODULE 3: Couche 1 - Vision

### Fichier: `layer1_vision.py`

```python
"""
Couche 1: Vision Primitive
Apprendre ce qui existe avant d'apprendre à quoi cela sert.
"""

import numpy as np
from typing import Dict, List, Tuple, Any
from scipy.ndimage import label
from .data_structures import VisualLibrary, Value
from .primitives import read_pixels, detect_change

class VisionLayer:
    """Couche 1: Vision primitive."""
    
    def __init__(self):
        self.library = VisualLibrary()
        self.frame_history = []
    
    def observe(self, frame: np.ndarray) -> Dict[str, Any]:
        """Observer une frame et extraire les informations visuelles."""
        frame = read_pixels(frame)
        
        # Détecter les valeurs uniques
        unique_values = self.detect_unique_values(frame)
        
        # Analyser les patterns
        patterns = self.detect_patterns(frame)
        
        # Détecter les changements (si historique disponible)
        changes = None
        if len(self.frame_history) > 0:
            changes = detect_change(self.frame_history[-1], frame)
        
        # Stocker dans l'historique
        self.frame_history.append(frame)
        
        return {
            'unique_values': unique_values,
            'patterns': patterns,
            'changes': changes,
            'frame': frame
        }
    
    def detect_unique_values(self, frame: np.ndarray) -> Dict[int, Dict[str, Any]]:
        """Détecter toutes les valeurs uniques dans la frame."""
        unique, counts = np.unique(frame, return_counts=True)
        
        values_info = {}
        for value, count in zip(unique, counts):
            positions = np.argwhere(frame == value)
            
            # Enregistrer dans la bibliothèque
            self.library.register_value(
                value=int(value),
                count=int(count),
                positions=positions.tolist()
            )
            
            values_info[int(value)] = {
                'count': int(count),
                'percentage': float(count / frame.size),
                'positions': positions
            }
        
        return values_info
    
    def detect_patterns(self, frame: np.ndarray) -> Dict[int, List[str]]:
        """Détecter les patterns visuels récurrents."""
        patterns = {}
        
        for value in np.unique(frame):
            if value == 0:  # Ignorer le background
                continue
            
            mask = (frame == value).astype(int)
            value_patterns = self._find_patterns_in_mask(mask)
            
            if value_patterns:
                patterns[int(value)] = value_patterns
                self.library.patterns[int(value)] = value_patterns
        
        return patterns
    
    def _find_patterns_in_mask(self, mask: np.ndarray) -> List[str]:
        """Trouver des patterns dans un masque binaire."""
        patterns = []
        
        # Chercher patterns 3×3
        for i in range(mask.shape[0] - 2):
            for j in range(mask.shape[1] - 2):
                block = mask[i:i+3, j:j+3]
                if np.sum(block) >= 3:  # Au moins 3 pixels
                    pattern_str = self._block_to_string(block)
                    if pattern_str not in patterns:
                        patterns.append(pattern_str)
        
        return patterns[:5]  # Limiter à 5 patterns
    
    def _block_to_string(self, block: np.ndarray) -> str:
        """Convertir un bloc en string pattern."""
        rows = []
        for row in block:
            row_str = ''.join(['X' if x else '.' for x in row])
            rows.append(row_str)
        return '/'.join(rows)
    
    def analyze_behavior(self, value: int, window: int = 10) -> str:
        """Analyser le comportement d'une valeur sur les dernières frames."""
        if len(self.frame_history) < 2:
            return 'unknown'
        
        # Prendre les dernières frames
        recent_frames = self.frame_history[-window:]
        counts = [np.sum(frame == value) for frame in recent_frames]
        
        if len(set(counts)) == 1:
            return 'static'
        elif all(counts[i] >= counts[i+1] for i in range(len(counts)-1)):
            return 'decreasing'
        elif all(counts[i] <= counts[i+1] for i in range(len(counts)-1)):
            return 'increasing'
        else:
            return 'variable'
    
    def compute_change_rate(self, value: int) -> float:
        """Calculer le taux de changement d'une valeur."""
        if len(self.frame_history) < 2:
            return 0.0
        
        frame_prev = self.frame_history[-2]
        frame_curr = self.frame_history[-1]
        
        mask_prev = (frame_prev == value)
        mask_curr = (frame_curr == value)
        
        changed = np.sum(mask_prev != mask_curr)
        total = np.sum(mask_prev) + np.sum(mask_curr)
        
        if total == 0:
            return 0.0
        
        return changed / total
    
    def get_summary(self) -> Dict[str, Any]:
        """Obtenir un résumé des observations visuelles."""
        summary = {
            'num_values': len(self.library.values),
            'values': {},
            'patterns': self.library.patterns,
            'frames_observed': len(self.frame_history)
        }
        
        for value, value_obj in self.library.values.items():
            behavior = self.analyze_behavior(value)
            change_rate = self.compute_change_rate(value)
            
            summary['values'][value] = {
                'count': value_obj.count,
                'behavior': behavior,
                'change_rate': change_rate
            }
        
        return summary
```

---

## 🔧 MODULE 4: Couche 2 - Objets

### Fichier: `layer2_objects.py`

```python
"""
Couche 2: Objets et Identités
Regrouper les pixels en entités cohérentes.
"""

import numpy as np
from typing import Dict, List, Tuple, Any, Optional
from scipy.ndimage import label, center_of_mass
from .data_structures import ObjectLibrary, Component, CompositeStructure
from .primitives import measure_distance

class ObjectLayer:
    """Couche 2: Objets et identités."""
    
    def __init__(self):
        self.library = ObjectLibrary()
        self.component_history = {}  # {component_id: [positions]}
    
    def identify_components(self, frame: np.ndarray, value: int) -> List[Component]:
        """Identifier les composantes connexes d'une valeur."""
        mask = (frame == value).astype(int)
        labeled, num_components = label(mask)
        
        components = []
        for i in range(1, num_components + 1):
            positions = np.argwhere(labeled == i)
            
            component = Component(
                id=f'Component_{value}_{i}',
                value=int(value),
                size=len(positions),
                positions=positions,
                centroid=positions.mean(axis=0),
                bbox={
                    'row_min': int(positions[:, 0].min()),
                    'row_max': int(positions[:, 0].max()),
                    'col_min': int(positions[:, 1].min()),
                    'col_max': int(positions[:, 1].max())
                }
            )
            
            components.append(component)
            self.library.add_component(component)
        
        return components
    
    def classify_components(self, components: List[Component]) -> Dict[str, List[Component]]:
        """Classifier les composantes par taille et forme."""
        categories = {
            'tiny': [],      # < 5 pixels
            'small': [],     # 5-20 pixels
            'medium': [],    # 20-100 pixels
            'large': [],     # > 100 pixels
            'structured': [] # Patterns réguliers
        }
        
        for comp in components:
            if self._is_structured(comp):
                categories['structured'].append(comp)
            elif comp.size < 5:
                categories['tiny'].append(comp)
            elif comp.size < 20:
                categories['small'].append(comp)
            elif comp.size < 100:
                categories['medium'].append(comp)
            else:
                categories['large'].append(comp)
        
        return categories
    
    def _is_structured(self, component: Component) -> bool:
        """Vérifier si une composante a une structure régulière."""
        bbox = component.bbox
        height = bbox['row_max'] - bbox['row_min'] + 1
        width = bbox['col_max'] - bbox['col_min'] + 1
        
        # Vérifier si c'est un rectangle parfait
        expected_size = height * width
        actual_size = component.size
        
        return actual_size / expected_size > 0.8
    
    def detect_composite_structures(self, frame: np.ndarray) -> List[CompositeStructure]:
        """Détecter les structures composites (ex: bloc 5×5)."""
        structures = []
        
        # Chercher blocs 5×5
        for i in range(frame.shape[0] - 4):
            for j in range(frame.shape[1] - 4):
                block = frame[i:i+5, j:j+5]
                
                structure = self._check_5x5_block(block, (i, j))
                if structure:
                    structures.append(structure)
                    self.library.structures.append(structure)
        
        return structures
    
    def _check_5x5_block(self, block: np.ndarray, position: Tuple[int, int]) -> Optional[CompositeStructure]:
        """Vérifier si un bloc 5×5 a un pattern valide."""
        unique, counts = np.unique(block, return_counts=True)
        values_dict = dict(zip(unique.astype(int), counts.astype(int)))
        
        # Pattern attendu: 12 (10px) + 9 (15px)
        if 12 in values_dict and 9 in values_dict:
            count_12 = values_dict[12]
            count_9 = values_dict[9]
            
            if count_12 >= 8 and count_9 >= 12:
                pattern_score = (count_12 / 10.0 + count_9 / 15.0) / 2.0
                
                return CompositeStructure(
                    type='block_5x5',
                    position=position,
                    values=values_dict,
                    pattern_score=pattern_score,
                    components=[]  # À remplir plus tard
                )
        
        return None
    
    def track_movement(self, frame_before: np.ndarray, frame_after: np.ndarray, value: int) -> List[Dict[str, Any]]:
        """Tracker le mouvement des composantes d'une valeur."""
        components_before = self.identify_components(frame_before, value)
        components_after = self.identify_components(frame_after, value)
        
        movements = []
        
        for comp_before in components_before:
            # Trouver la composante correspondante dans frame_after
            best_match = None
            min_distance = float('inf')
            
            for comp_after in components_after:
                dist = measure_distance(comp_before.centroid, comp_after.centroid)
                if dist < min_distance:
                    min_distance = dist
                    best_match = comp_after
            
            if best_match:
                delta = best_match.centroid - comp_before.centroid
                
                movement = {
                    'component_id': comp_before.id,
                    'size_before': comp_before.size,
                    'size_after': best_match.size,
                    'centroid_before': comp_before.centroid,
                    'centroid_after': best_match.centroid,
                    'delta': delta,
                    'distance': min_distance,
                    'is_mobile': min_distance > 0.5
                }
                
                movements.append(movement)
                
                # Mettre à jour le statut mobile
                comp_before.is_mobile = movement['is_mobile']
                
                # Stocker dans l'historique
                if comp_before.id not in self.component_history:
                    self.component_history[comp_before.id] = []
                self.component_history[comp_before.id].append(comp_before.centroid)
        
        return movements
    
    def classify_mobile_vs_fixed(self, movements: List[Dict[str, Any]]) -> Tuple[List, List]:
        """Classifier les composantes en mobiles vs fixes."""
        mobile = []
        fixed = []
        
        for mov in movements:
            if mov['is_mobile']:
                mobile.append(mov)
            else:
                fixed.append(mov)
        
        return mobile, fixed
    
    def get_summary(self) -> Dict[str, Any]:
        """Obtenir un résumé des objets identifiés."""
        summary = {
            'num_components': sum(len(comps) for comps in self.library.components.values()),
            'components_by_value': {
                value: len(comps) for value, comps in self.library.components.items()
            },
            'num_structures': len(self.library.structures),
            'structures': [
                {
                    'type': s.type,
                    'position': s.position,
                    'score': s.pattern_score
                }
                for s in self.library.structures
            ],
            'mobile_objects': len(self.library.mobile_objects),
            'fixed_objects': len(self.library.fixed_objects)
        }
        
        return summary
```

---

## 🧪 TESTS UNITAIRES

### Fichier: `tests/test_layer1_vision.py`

```python
"""
Tests unitaires pour la Couche 1: Vision.
"""

import pytest
import numpy as np
from learning_system.layer1_vision import VisionLayer

def test_detect_unique_values():
    """Test de détection des valeurs uniques."""
    vision = VisionLayer()
    
    frame = np.array([
        [0, 0, 3, 3],
        [0, 4, 4, 3],
        [12, 12, 9, 9]
    ])
    
    result = vision.observe(frame)
    values = result['unique_values']
    
    assert 0 in values
    assert 3 in values
    assert 4 in values
    assert 9 in values
    assert 12 in values
    assert len(values) == 5

def test_detect_changes():
    """Test de détection des changements."""
    vision = VisionLayer()
    
    frame1 = np.zeros((5, 5))
    frame2 = np.zeros((5, 5))
    frame2[2, 2] = 1
    
    vision.observe(frame1)
    result = vision.observe(frame2)
    
    assert result['changes'] is not None
    assert result['changes']['has_change'] == True
    assert result['changes']['num_changes'] == 1

def test_analyze_behavior():
    """Test d'analyse du comportement."""
    vision = VisionLayer()
    
    # Créer une séquence avec valeur décroissante
    for i in range(10, 0, -1):
        frame = np.full((5, 5), i)
        vision.observe(frame)
    
    behavior = vision.analyze_behavior(5)
    assert behavior == 'decreasing'
```

### Fichier: `tests/test_layer2_objects.py`

```python
"""
Tests unitaires pour la Couche 2: Objets.
"""

import pytest
import numpy as np
from learning_system.layer2_objects import ObjectLayer

def test_identify_components():
    """Test d'identification des composantes."""
    objects = ObjectLayer()
    
    frame = np.array([
        [0, 0, 9, 9, 0],
        [0, 0, 9, 9, 0],
        [0, 0, 0, 0, 0],
        [9, 9, 0, 0, 0],
        [9, 9, 0, 0, 0]
    ])
    
    components = objects.identify_components(frame, 9)
    
    assert len(components) == 2
    assert components[0].size == 4
    assert components[1].size == 4

def test_detect_5x5_block():
    """Test de détection de bloc 5×5."""
    objects = ObjectLayer()
    
    frame = np.zeros((10, 10))
    # Créer un bloc 5×5 avec pattern 12/9
    frame[2:4, 3:8] = 12  # 2 lignes de 12
    frame[4:7, 3:8] = 9   # 3 lignes de 9
    
    structures = objects.detect_composite_structures(frame)
    
    assert len(structures) > 0
    assert structures[0].type == 'block_5x5'
    assert structures[0].pattern_score > 0.9

def test_track_movement():
    """Test de tracking de mouvement."""
    objects = ObjectLayer()
    
    frame1 = np.zeros((10, 10))
    frame1[5, 5] = 12
    
    frame2 = np.zeros((10, 10))
    frame2[5, 7] = 12  # Déplacé de 2 colonnes
    
    movements = objects.track_movement(frame1, frame2, 12)
    
    assert len(movements) > 0
    assert movements[0]['is_mobile'] == True
    assert movements[0]['delta'][1] == 2.0  # Déplacement en colonne
```

---

## 📝 EXEMPLE D'UTILISATION

### Fichier: `examples/example_ls20_discovery.py`

```python
"""
Exemple d'utilisation des Couches 1-2 sur le puzzle ls20-9607627b.
"""

import sys
sys.path.insert(0, '/home/lvx/LVX/environment_files/ls20/9607627b')

from arc_agi import Arcade, OperationMode
from arcengine import GameAction
import numpy as np

from learning_system.layer1_vision import VisionLayer
from learning_system.layer2_objects import ObjectLayer

def main():
    print("=" * 80)
    print("EXEMPLE: Découverte Automatique du Puzzle ls20-9607627b")
    print("=" * 80)
    print()
    
    # Initialiser les couches
    vision = VisionLayer()
    objects = ObjectLayer()
    
    # Charger l'environnement
    arcade = Arcade(operation_mode=OperationMode.COMPETITION)
    env = arcade.make("ls20-9607627b")
    obs = env.reset()
    
    # Corriger la shape
    frame_0 = np.array(obs.frame)
    if frame_0.ndim == 3 and frame_0.shape[0] == 1:
        frame_0 = frame_0[0]
    
    print("📊 PHASE 1: OBSERVATION INITIALE")
    print("-" * 80)
    
    # Observer la frame initiale
    vision_result = vision.observe(frame_0)
    
    print(f"Valeurs détectées: {list(vision_result['unique_values'].keys())}")
    print()
    
    for value, info in vision_result['unique_values'].items():
        print(f"Valeur {value}: {info['count']} pixels ({info['percentage']:.1%})")
    
    print()
    print("📊 PHASE 2: IDENTIFICATION DES OBJETS")
    print("-" * 80)
    
    # Identifier les composantes pour chaque valeur
    for value in vision_result['unique_values'].keys():
        if value == 0:  # Ignorer le background
            continue
        
        components = objects.identify_components(frame_0, value)
        print(f"Valeur {value}: {len(components)} composante(s)")
        
        for comp in components:
            print(f"  - {comp.id}: {comp.size} pixels, centroid {comp.centroid}")
    
    print()
    print("📊 PHASE 3: DÉTECTION DE STRUCTURES")
    print("-" * 80)
    
    # Détecter les structures composites
    structures = objects.detect_composite_structures(frame_0)
    
    print(f"Structures détectées: {len(structures)}")
    for struct in structures:
        print(f"  - {struct.type} à position {struct.position}")
        print(f"    Score: {struct.pattern_score:.1%}")
        print(f"    Composition: {struct.values}")
    
    print()
    print("📊 PHASE 4: TEST DE MOUVEMENT")
    print("-" * 80)
    
    # Exécuter ACTION4
    obs = env.step(GameAction.ACTION4)
    frame_1 = np.array(obs.frame)
    if frame_1.ndim == 3 and frame_1.shape[0] == 1:
        frame_1 = frame_1[0]
    
    # Observer la nouvelle frame
    vision.observe(frame_1)
    
    # Tracker les mouvements
    for value in [12, 9]:
        movements = objects.track_movement(frame_0, frame_1, value)
        
        print(f"Mouvements de la valeur {value}:")
        for mov in movements:
            status = "MOBILE" if mov['is_mobile'] else "FIXE"
            print(f"  - {mov['component_id']}: {status}")
            print(f"    Delta: {mov['delta']}, Distance: {mov['distance']:.2f}")
    
    print()
    print("📊 RÉSUMÉ")
    print("-" * 80)
    
    vision_summary = vision.get_summary()
    objects_summary = objects.get_summary()
    
    print("Vision:")
    print(f"  - {vision_summary['num_values']} valeurs observées")
    print(f"  - {vision_summary['frames_observed']} frames analysées")
    
    print()
    print("Objets:")
    print(f"  - {objects_summary['num_components']} composantes identifiées")
    print(f"  - {objects_summary['num_structures']} structures détectées")
    
    print()
    print("=" * 80)
    print("FIN DE L'EXEMPLE")
    print("=" * 80)

if __name__ == "__main__":
    main()
```

---

## ✅ CRITÈRES DE VALIDATION

### Tests de Validation

1. **Vision Layer**
   - [ ] Détecte correctement les 8 valeurs du puzzle ls20
   - [ ] Identifie les patterns 3×3 des glyphes
   - [ ] Détecte le comportement décroissant de la valeur 11
   - [ ] Calcule les taux de changement correctement

2. **Object Layer**
   - [ ] Identifie 1 composante de valeur 12
   - [ ] Identifie 5 composantes de valeur 9
   - [ ] Détecte le bloc 5×5 avec score > 90%
   - [ ] Distingue la composante 9 mobile des 4 fixes

3. **Intégration**
   - [ ] Les deux couches fonctionnent ensemble
   - [ ] Les données sont correctement partagées
   - [ ] Les performances sont acceptables (< 1s par frame)

---

## 📅 PLANNING DÉTAILLÉ

### Jour 1: Setup et Primitives
- [ ] Créer la structure des dossiers
- [ ] Implémenter `primitives.py`
- [ ] Implémenter `data_structures.py`
- [ ] Tests unitaires des primitives

### Jour 2-3: Couche 1 (Vision)
- [ ] Implémenter `VisionLayer`
- [ ] Tests unitaires
- [ ] Validation sur ls20-9607627b

### Jour 4-5: Couche 2 (Objets)
- [ ] Implémenter `ObjectLayer`
- [ ] Tests unitaires
- [ ] Validation sur ls20-9607627b

### Jour 6: Intégration
- [ ] Créer l'exemple complet
- [ ] Tests d'intégration
- [ ] Optimisation des performances

### Jour 7: Documentation et Validation
- [ ] Documentation complète
- [ ] Rapport de validation
- [ ] Préparation Phase 2

---

## 🎯 MÉTRIQUES DE SUCCÈS

### Objectifs Quantitatifs

| Métrique | Objectif | Actuel |
|----------|----------|--------|
| Valeurs détectées | 8/8 | - |
| Composantes 12 | 1 | - |
| Composantes 9 | 5 | - |
| Bloc 5×5 détecté | Oui | - |
| Score bloc 5×5 | > 90% | - |
| Composante 9 mobile | 1/5 | - |
| Temps par frame | < 1s | - |

---

## 📝 PROCHAINES ÉTAPES

Après la Phase 1, nous passerons à:
- **Phase 2**: Couches 3-4 (Monde + Causalité)
- **Phase 3**: Couches 5-6 (Mémoire + Raisonnement)
- **Phase 4**: Couche 7 (Méta-apprentissage)
- **Phase 5**: Test de victoire sur ls20-9607627b

---

**Fin du Plan d'Implémentation Phase 1**  
**Prochaine action**: Créer les modules et commencer l'implémentation