# PLAN D'IMPLÉMENTATION PHASE 1 V2 - SANS DUPLICATION
## Architecture d'Apprentissage MAGEN - Couches 0-2 Optimisées

**Date**: 2026-06-18  
**Version**: 2.0 (Corrigée après audit forensique)  
**Objectif**: Implémenter les 3 premières couches sans dupliquer le code existant

---

## 🎯 RÉSUMÉ EXÉCUTIF

### Audit Forensique Complété ✅

**Fichiers analysés**: Sessions 81-100  
**Duplications identifiées**: 5 fonctions majeures  
**Nouvelles couches requises**: 3 (Couches 0, 3, 9)  
**Code réutilisable**: `session100_verify_block_hypothesis.py`

### Différences V1 → V2

| Aspect | V1 (Problèmes) | V2 (Solutions) |
|--------|----------------|----------------|
| Couche 0 | ❌ Absente | ✅ Sensorimotrice ajoutée |
| Détection | ❌ Codée en dur (if 12 and 9) | ✅ Générique |
| Attention | ❌ Absente | ✅ Couche 3 ajoutée |
| Duplication | ❌ Code dupliqué | ✅ Réutilisation session100 |
| Architecture | ❌ 7 couches | ✅ 10 couches |

---

## 📊 ARCHITECTURE GLOBALE V2

```
┌─────────────────────────────────────────────────────────────┐
│                    MAGEN V2 - Phase 1                       │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌───────────────────────────────────────────────────────┐ │
│  │ Couche 0: SENSORIMOTRICE (NOUVEAU)                    │ │
│  │ - Découverte d'agence                                 │ │
│  │ - Test action → effet                                 │ │
│  └───────────────────────────────────────────────────────┘ │
│                          ↓                                  │
│  ┌───────────────────────────────────────────────────────┐ │
│  │ Couche 1: VISION (RÉUTILISE session100)              │ │
│  │ - Lecture pixels                                      │ │
│  │ - Détection valeurs uniques                           │ │
│  └───────────────────────────────────────────────────────┘ │
│                          ↓                                  │
│  ┌───────────────────────────────────────────────────────┐ │
│  │ Couche 2: OBJETS (AMÉLIORE session100)               │ │
│  │ - Composantes connexes (RÉUTILISE)                    │ │
│  │ - Détection générique structures (NOUVEAU)            │ │
│  └───────────────────────────────────────────────────────┘ │
│                          ↓                                  │
│  ┌───────────────────────────────────────────────────────┐ │
│  │ Couche 3: ATTENTION (NOUVEAU)                         │ │
│  │ - Carte d'attention                                   │ │
│  │ - Focus sur changements                               │ │
│  └───────────────────────────────────────────────────────┘ │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 📁 STRUCTURE DES FICHIERS

### Fichiers à Créer (6 modules + 4 tests)

```
lumvorax2/src/MAGEN/
├── primitives.py                    # Primitives extraites de session100
├── data_structures.py               # Structures de données
├── layer0_sensorimotor.py          # Couche 0 (NOUVEAU)
├── layer1_vision.py                # Couche 1 (réutilise)
├── layer2_objects.py               # Couche 2 (améliore)
├── layer3_attention.py             # Couche 3 (NOUVEAU)
└── tests/
    ├── test_primitives.py
    ├── test_layer0_sensorimotor.py
    ├── test_layer1_vision.py
    ├── test_layer2_objects.py
    └── test_layer3_attention.py
```

### Fichiers à Réutiliser (NE PAS DUPLIQUER)

```
session100_verify_block_hypothesis.py
├── analyze_components()           → primitives.py
├── check_5x5_block()              → layer2_objects.py (généraliser)
├── track_component_movement()     → layer2_objects.py
└── map_labyrinth()                → layer2_objects.py
```

---

## 🔍 MATRICE DE DUPLICATION

| Fonction | Existe? | Source | Action |
|----------|---------|--------|--------|
| `analyze_components` | ✅ | session100 | Réutiliser |
| `check_5x5_block` | ✅ | session100 | Généraliser |
| `track_movement` | ✅ | session100 | Réutiliser |
| `map_labyrinth` | ✅ | session100 | Réutiliser |
| Q-learning | ✅ | session87 | Remplacer |
| Sensorimotrice | ❌ | - | Créer |
| Attention | ❌ | - | Créer |
| Prédiction | ❌ | - | Créer (Phase 2) |
| Mémoire sémantique | ❌ | - | Créer (Phase 2) |
| Détection générique | ⚠️ | session100 | Améliorer |

---

## 🔧 MODULE 1: PRIMITIVES

### Fichier: `primitives.py`

**Objectif**: Extraire les fonctions de base de session100

**Fonctions principales**:
- `read_frame(obs)` - Lire frame et corriger shape
- `get_unique_values(frame)` - Valeurs uniques + comptes
- `get_positions(frame, value)` - Positions d'une valeur
- `compute_centroid(positions)` - Centroid
- `compute_bounding_box(positions)` - Bounding box
- `euclidean_distance(pos1, pos2)` - Distance
- `identify_connected_components(frame, value)` - Composantes connexes

**Source**: `session100_verify_block_hypothesis.py` (lignes 50-150)

**Tests**: Vérifier correction shape, détection valeurs, calculs géométriques

---

## 🔧 MODULE 2: STRUCTURES DE DONNÉES

### Fichier: `data_structures.py`

**Objectif**: Définir les structures pour l'architecture V2

**Classes principales**:

```python
@dataclass
class Value:
    """Valeur observée dans une frame."""
    value: int
    count: int
    positions: np.ndarray
    centroid: Tuple[float, float]

@dataclass
class Component:
    """Composante connexe."""
    value: int
    component_id: int
    positions: np.ndarray
    centroid: Tuple[float, float]
    bbox: Dict[str, int]
    size: int

@dataclass
class Transformation:
    """Transformation (Avant, Action, Après)."""
    frame_before: np.ndarray
    action: int
    frame_after: np.ndarray
    changed_positions: np.ndarray
    delta_values: Dict[int, int]

@dataclass
class CompositeStructure:
    """Structure composite (bloc 5×5, etc.)."""
    structure_type: str
    components: List[Component]
    bbox: Dict[str, int]
    centroid: Tuple[float, float]
    properties: Dict[str, Any]
    score: float

@dataclass
class AttentionMap:
    """Carte d'attention."""
    frame_shape: Tuple[int, int]
    attention_weights: np.ndarray
    focus_regions: List[Dict[str, int]]
    threshold: float

@dataclass
class SensorimotorExperience:
    """Expérience sensorimotrice."""
    action: int
    frame_before: np.ndarray
    frame_after: np.ndarray
    has_effect: bool
    effect_magnitude: float
```

**Tests**: Création, propriétés dérivées, méthodes utilitaires

---

## 🔧 MODULE 3: COUCHE 0 - SENSORIMOTRICE

### Fichier: `layer0_sensorimotor.py`

**Objectif**: Découverte d'agence (NOUVEAU - pas de duplication)

**Classe principale**:

```python
class SensorimotorLayer:
    """Couche 0: Apprentissage sensori-moteur."""
    
    def __init__(self):
        self.experiences: List[SensorimotorExperience] = []
        self.action_effects: Dict[int, List[float]] = {}
        self.has_agency: Dict[int, bool] = {}
    
    def test_action_effect(self, env, action: int) -> SensorimotorExperience:
        """Tester l'effet d'une action."""
        # Observer avant
        # Exécuter action
        # Observer après
        # Calculer magnitude de l'effet
    
    def discover_agency(self, env, num_tests_per_action: int = 5) -> Dict[int, bool]:
        """Découvrir quelles actions ont un effet."""
        # Tester chaque action plusieurs fois
        # Calculer magnitude moyenne
        # Déterminer has_agency
    
    def get_effective_actions(self) -> List[int]:
        """Obtenir les actions qui ont un effet."""
```

**Tests**: Découverte d'agence, magnitude d'effet, actions effectives

---

## 🔧 MODULE 4: COUCHE 1 - VISION

### Fichier: `layer1_vision.py`

**Objectif**: Réutiliser les primitives de session100

**Classe principale**:

```python
class VisionLayer:
    """Couche 1: Vision de base."""
    
    def __init__(self):
        self.current_frame: np.ndarray = None
        self.previous_frame: np.ndarray = None
        self.values: List[Value] = []
    
    def perceive(self, obs) -> List[Value]:
        """Percevoir une frame et extraire les valeurs."""
        # RÉUTILISE primitives.read_frame
        # RÉUTILISE primitives.get_unique_values
        # RÉUTILISE primitives.get_positions
        # RÉUTILISE primitives.compute_centroid
    
    def compare_frames(self, frame_before, frame_after) -> Transformation:
        """Comparer deux frames."""
        # Identifier positions changées
        # Calculer delta des valeurs
        # Créer Transformation
```

**Tests**: Perception, comparaison frames, détection changements

---

## 🔧 MODULE 5: COUCHE 2 - OBJETS

### Fichier: `layer2_objects.py`

**Objectif**: Améliorer session100 avec détection générique

**Classe principale**:

```python
class ObjectsLayer:
    """Couche 2: Identification d'objets."""
    
    def __init__(self):
        self.components: List[Component] = []
        self.composite_structures: List[CompositeStructure] = []
    
    def identify_components(self, frame, value) -> List[Component]:
        """Identifier composantes connexes."""
        # RÉUTILISE primitives.identify_connected_components
        # RÉUTILISE primitives.compute_centroid
        # RÉUTILISE primitives.compute_bounding_box
    
    def detect_composite_structures_generic(self, frame, window_sizes) -> List[CompositeStructure]:
        """Détecter structures composites de manière GÉNÉRIQUE."""
        # Scanner avec fenêtre glissante
        # Analyser chaque bloc de manière générique
        # Vérifier cohérence (PAS de code en dur)
        # Filtrer chevauchements
    
    def _is_coherent_structure(self, block, unique, counts) -> bool:
        """Vérifier cohérence (GÉNÉRIQUE)."""
        # Au moins 2 valeurs différentes
        # Distribution non uniforme
        # Ratio dominant > 0.4 mais < 0.95
    
    def track_component_movement(self, frame_before, frame_after, value) -> List[Dict]:
        """Tracker mouvement."""
        # RÉUTILISE identify_components
        # Matcher composantes (plus proche voisin)
        # Calculer deltas et distances
```

**Tests**: Identification composantes, détection générique, tracking

---

## 🔧 MODULE 6: COUCHE 3 - ATTENTION

### Fichier: `layer3_attention.py`

**Objectif**: Système d'attention sélective (NOUVEAU)

**Classe principale**:

```python
class AttentionLayer:
    """Couche 3: Attention sélective."""
    
    def __init__(self, decay_rate: float = 0.9):
        self.attention_map: AttentionMap = None
        self.decay_rate = decay_rate
        self.history: List[AttentionMap] = []
    
    def compute_attention_map(self, frame_before, frame_after, transformation) -> AttentionMap:
        """Calculer carte d'attention."""
        # Initialiser carte
        # Augmenter attention sur pixels changés
        # Diffuser aux voisins (convolution)
        # Normaliser [0, 1]
        # Identifier régions de focus
    
    def _diffuse_attention(self, attention, kernel_size=3) -> np.ndarray:
        """Diffuser l'attention aux voisins."""
        # Convolution avec kernel gaussien
    
    def _identify_focus_regions(self, attention_weights, threshold=0.5) -> List[Dict]:
        """Identifier régions de focus."""
        # Binariser
        # Composantes connexes
        # Extraire bounding boxes
```

**Tests**: Carte d'attention, diffusion, régions de focus

---

## 📋 PLAN D'EXÉCUTION

### Phase 1A: Extraction et Structures (Jour 1)

1. ✅ **Audit forensique complété**
2. ⏳ **Créer `primitives.py`**
   - Extraire de session100
   - Tests unitaires
3. ⏳ **Créer `data_structures.py`**
   - Définir toutes les classes
   - Tests unitaires

### Phase 1B: Couches 0-1 (Jour 2)

4. ⏳ **Créer `layer0_sensorimotor.py`**
   - Implémentation complète
   - Tests découverte d'agence
5. ⏳ **Créer `layer1_vision.py`**
   - Réutiliser primitives
   - Tests perception

### Phase 1C: Couches 2-3 (Jour 3)

6. ⏳ **Créer `layer2_objects.py`**
   - Détection générique
   - Tests structures
7. ⏳ **Créer `layer3_attention.py`**
   - Système d'attention
   - Tests carte d'attention

### Phase 1D: Intégration (Jour 4)

8. ⏳ **Tests d'intégration**
   - Tester pipeline complet
   - Valider sur ls20-9607627b
9. ⏳ **Documentation**
   - Diagrammes Mermaid
   - Guide d'utilisation

---

## 🏢 INFORMATIONS LUMVORAX

### Contacts

**LinkedIn**: [Gabriel Chaves](https://www.linkedin.com/in/mrvgac/?utm_source=chatgpt.com)  
**GitHub**: [VGACTEC](https://github.com/Vgactec?utm_source=chatgpt.com)  
**X/Twitter**: [@VGACTECH](https://x.com/VGACTECH?utm_source=chatgpt.com)

### Positionnement

**Mission**: IA explicable et auditable  
**Modèle**: SaaS B2B, audit IA, conformité  
**Hackathon**: Paris, 8-9 juillet 2026

### Synergie MAGEN ↔ LumVorax

| Aspect | MAGEN | LumVorax |
|--------|-------|----------|
| Traçabilité | Couches hiérarchiques | Audit temps réel |
| Explicabilité | Graphe causal | Visualisation |
| Auditabilité | Mémoire sémantique | Rapport instantané |
| Apprentissage | Prédiction/Erreur | Détection anomalies |

**Proposition**: MAGEN comme démonstrateur d'IA auditable

---

## 📊 MÉTRIQUES DE SUCCÈS

### Critères de Validation

1. **Pas de duplication**: ✅ Audit forensique complété
2. **Détection générique**: ⏳ À tester sur ls20-9607627b
3. **Découverte d'agence**: ⏳ À valider (5 actions)
4. **Attention fonctionnelle**: ⏳ À tester (carte d'attention)
5. **Tests passent**: ⏳ 100% de couverture

### KPIs

- **Lignes de code dupliquées**: 0 (objectif)
- **Couverture de tests**: >80%
- **Temps d'exécution**: <5s par couche
- **Mémoire utilisée**: <500MB

---

## 🎯 PROCHAINES ÉTAPES

### Immédiat (Phase 1)

1. Créer `primitives.py` avec extraction session100
2. Créer `data_structures.py` avec toutes les classes
3. Implémenter Couche 0 (Sensorimotrice)
4. Implémenter Couche 1 (Vision)
5. Implémenter Couche 2 (Objets) avec détection générique
6. Implémenter Couche 3 (Attention)
7. Tests d'intégration Phase 1

### Futur (Phase 2)

8. Couche 4: Monde (cartographie labyrinthe)
9. Couche 5: Causalité (graphe causal)
10. Couche 9: Prédiction (apprentissage par erreur)
11. Couche 6: Mémoire sémantique
12. Couche 7: Raisonnement
13. Couche 8: Méta-apprentissage

---

**Fin du Plan d'Implémentation Phase 1 V2**  
**Prêt pour l'implémentation sans duplication** ✅