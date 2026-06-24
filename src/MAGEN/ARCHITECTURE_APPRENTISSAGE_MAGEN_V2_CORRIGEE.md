# ARCHITECTURE D'APPRENTISSAGE MAGEN V2 - CORRIGÉE
## Système d'Apprentissage Cognitif avec Prédiction et Attention

**Date**: 2026-06-18  
**Version**: 2.0 (Corrections majeures)  
**Objectif**: Créer un système qui apprend les TRANSFORMATIONS, pas seulement les états

---

## 🎯 CORRECTIONS CRITIQUES PAR RAPPORT À V1

### ❌ Problèmes Identifiés dans V1

1. **Manque de Couche 0 Sensorimotrice**
   - V1 démarre directement avec la vision
   - Manque: comprendre qu'une action produit un changement

2. **Apprentissage des États vs Transformations**
   - V1 stocke des états
   - Manque: stocker (État_Avant, Action, État_Après)

3. **Ordre Causalité/Monde Inversé**
   - V1: Vision → Objets → Monde → Causalité
   - Meilleur: Vision → Objets → Causalité → Monde

4. **Pas de Système de Prédiction**
   - V1: accumule des observations
   - Manque: prédit → compare → apprend de l'erreur

5. **Pas de Système d'Attention**
   - V1: analyse tous les pixels uniformément
   - Manque: focus sur ce qui bouge/change/surprend

6. **Détection Bloc 5×5 Codée en Dur**
   - V1: `if 12 in values and 9 in values`
   - Problème: fuite de connaissance spécifique au puzzle

7. **Mémoire Épisodique Trop Lourde**
   - V1: stocke 344+ épisodes bruts
   - Meilleur: mémoire sémantique (concepts extraits)

### ✅ Solutions Implémentées dans V2

1. ✅ **Couche 0 Sensorimotrice** ajoutée
2. ✅ **Triplets (Avant, Action, Après)** comme unité de base
3. ✅ **Causalité avant Monde** (ordre corrigé)
4. ✅ **Couche 9 Prédiction** ajoutée (cœur du système)
5. ✅ **Couche 3 Attention** ajoutée
6. ✅ **Détection générique de structures** (sans valeurs codées)
7. ✅ **Mémoire sémantique** (concepts, pas épisodes bruts)

---

## 🏗️ ARCHITECTURE V2 EN 10 COUCHES

### Vue d'Ensemble

```
┌─────────────────────────────────────────────────────────┐
│  COUCHE 9: PRÉDICTION (Cœur du système)               │
│  (Prédit → Compare → Apprend de l'erreur)             │
└─────────────────────────────────────────────────────────┘
                          ↑
┌─────────────────────────────────────────────────────────┐
│  COUCHE 8: MÉTA-APPRENTISSAGE                          │
│  (Optimisation de l'apprentissage)                     │
└─────────────────────────────────────────────────────────┘
                          ↑
┌─────────────────────────────────────────────────────────┐
│  COUCHE 7: RAISONNEMENT STRATÉGIQUE                    │
│  (Planification, hypothèses)                           │
└─────────────────────────────────────────────────────────┘
                          ↑
┌─────────────────────────────────────────────────────────┐
│  COUCHE 6: MÉMOIRE SÉMANTIQUE                          │
│  (Concepts extraits, pas épisodes bruts)               │
└─────────────────────────────────────────────────────────┘
                          ↑
┌─────────────────────────────────────────────────────────┐
│  COUCHE 5: MONDE                                       │
│  (Carte spatiale construite par causalité)             │
└─────────────────────────────────────────────────────────┘
                          ↑
┌─────────────────────────────────────────────────────────┐
│  COUCHE 4: CAUSALITÉ                                   │
│  (Actions → Transformations)                           │
└─────────────────────────────────────────────────────────┘
                          ↑
┌─────────────────────────────────────────────────────────┐
│  COUCHE 3: ATTENTION                                   │
│  (Focus sur ce qui est important)                      │
└─────────────────────────────────────────────────────────┘
                          ↑
┌─────────────────────────────────────────────────────────┐
│  COUCHE 2: OBJETS                                      │
│  (Regroupement en entités)                             │
└─────────────────────────────────────────────────────────┘
                          ↑
┌─────────────────────────────────────────────────────────┐
│  COUCHE 1: VISION                                      │
│  (Perception brute)                                    │
└─────────────────────────────────────────────────────────┘
                          ↑
┌─────────────────────────────────────────────────────────┐
│  COUCHE 0: SENSORIMOTRICE                              │
│  (Action → Changement)                                 │
└─────────────────────────────────────────────────────────┘
```

---

## 🔧 COUCHE 0: SENSORIMOTRICE (NOUVELLE)

### Objectif
Comprendre qu'**une action produit un changement** avant même de comprendre les objets

### Capacités Primitives

```python
class SensorimotorLayer:
    """Couche 0: Découverte sensorimotrice fondamentale."""
    
    def __init__(self):
        self.action_effects = {}  # {action: [has_effect, no_effect]}
        self.change_history = []
    
    def test_action_effect(self, frame_before, action, frame_after):
        """Tester si une action produit un changement."""
        has_change = not np.array_equal(frame_before, frame_after)
        
        if action not in self.action_effects:
            self.action_effects[action] = {'has_effect': 0, 'no_effect': 0}
        
        if has_change:
            self.action_effects[action]['has_effect'] += 1
        else:
            self.action_effects[action]['no_effect'] += 1
        
        self.change_history.append({
            'action': action,
            'has_change': has_change,
            'num_pixels_changed': np.sum(frame_before != frame_after)
        })
        
        return has_change
    
    def discover_agency(self):
        """Découvrir quelles actions ont un effet."""
        effective_actions = []
        
        for action, stats in self.action_effects.items():
            if stats['has_effect'] > 0:
                effectiveness = stats['has_effect'] / (stats['has_effect'] + stats['no_effect'])
                effective_actions.append({
                    'action': action,
                    'effectiveness': effectiveness
                })
        
        return effective_actions
```

### Découvertes Attendues

```python
# Après 10-20 expériences:
{
    'ACTION1': {'effectiveness': 0.80},  # Produit souvent un changement
    'ACTION2': {'effectiveness': 0.10},  # Rarement un changement
    'ACTION3': {'effectiveness': 0.75},
    'ACTION4': {'effectiveness': 0.90}
}

# Concept formé:
"Certaines actions modifient le monde"
```

---

## 📊 COUCHE 1: VISION (AMÉLIORÉE)

### Changements par rapport à V1

**Ajout**: Notion de **stabilité temporelle**

```python
class Value:
    """Représente une valeur observée."""
    value: int
    count: int
    behavior: str  # 'static', 'mobile', 'decreasing', 'increasing'
    stability: str  # NOUVEAU: 'permanent', 'semi_permanent', 'temporaire'
    change_rate: float
    affected_by_actions: List[str]  # NOUVEAU: quelles actions l'affectent
```

**Ajout**: Détection de patterns **de taille variable**

```python
def detect_patterns_multi_scale(self, frame: np.ndarray, value: int) -> Dict[str, List]:
    """Détecter des patterns de tailles 3×3, 5×5, 7×7, etc."""
    patterns = {
        '3x3': [],
        '5x5': [],
        '7x7': [],
        'custom': []
    }
    
    mask = (frame == value)
    
    # Chercher patterns 3×3
    patterns['3x3'] = self._find_patterns(mask, size=3)
    
    # Chercher patterns 5×5
    patterns['5x5'] = self._find_patterns(mask, size=5)
    
    # Chercher patterns 7×7
    patterns['7x7'] = self._find_patterns(mask, size=7)
    
    # Chercher patterns de forme libre (bounding box)
    patterns['custom'] = self._find_custom_patterns(mask)
    
    return patterns
```

---

## 🎨 COUCHE 2: OBJETS (AMÉLIORÉE)

### Changements par rapport à V1

**Ajout**: Stockage des **triplets (Avant, Action, Après)**

```python
@dataclass
class Transformation:
    """Représente une transformation d'objet."""
    object_before: Component
    action: str
    object_after: Component
    delta_position: np.ndarray
    delta_size: int
    transformation_type: str  # 'move', 'grow', 'shrink', 'disappear', 'appear'

class ObjectLayer:
    def __init__(self):
        self.library = ObjectLibrary()
        self.transformations = []  # NOUVEAU: liste de transformations
        self.object_action_effects = {}  # NOUVEAU: {object_id: {action: effect}}
    
    def record_transformation(self, frame_before, action, frame_after, value):
        """Enregistrer une transformation complète."""
        components_before = self.identify_components(frame_before, value)
        components_after = self.identify_components(frame_after, value)
        
        # Matcher les composantes
        for comp_before in components_before:
            comp_after = self._find_matching_component(comp_before, components_after)
            
            if comp_after:
                transformation = Transformation(
                    object_before=comp_before,
                    action=action,
                    object_after=comp_after,
                    delta_position=comp_after.centroid - comp_before.centroid,
                    delta_size=comp_after.size - comp_before.size,
                    transformation_type=self._classify_transformation(comp_before, comp_after)
                )
                
                self.transformations.append(transformation)
                
                # Enregistrer l'effet de l'action sur cet objet
                if comp_before.id not in self.object_action_effects:
                    self.object_action_effects[comp_before.id] = {}
                
                self.object_action_effects[comp_before.id][action] = transformation
```

**Ajout**: Détection **générique** de structures composites

```python
def detect_composite_structures_generic(self, frame: np.ndarray) -> List[CompositeStructure]:
    """Détecter des structures composites SANS valeurs codées en dur."""
    structures = []
    
    # Chercher des blocs de différentes tailles
    for size in [3, 5, 7, 9]:
        for i in range(frame.shape[0] - size + 1):
            for j in range(frame.shape[1] - size + 1):
                block = frame[i:i+size, j:j+size]
                
                # Analyser la composition du bloc
                unique, counts = np.unique(block, return_counts=True)
                
                # Vérifier si c'est une structure cohérente
                if self._is_coherent_structure(block, unique, counts):
                    structure = CompositeStructure(
                        type=f'block_{size}x{size}',
                        position=(i, j),
                        values=dict(zip(unique.astype(int), counts.astype(int))),
                        pattern_score=self._compute_coherence_score(block),
                        components=[]
                    )
                    structures.append(structure)
    
    return structures

def _is_coherent_structure(self, block, unique, counts):
    """Vérifier si un bloc est une structure cohérente."""
    # Critères génériques (pas de valeurs codées):
    # 1. Au moins 2 valeurs différentes
    # 2. Chaque valeur occupe une zone continue
    # 3. Les zones sont organisées (pas aléatoires)
    
    if len(unique) < 2:
        return False
    
    # Vérifier la continuité spatiale de chaque valeur
    for value in unique:
        mask = (block == value)
        labeled, num = label(mask)
        
        # Si une valeur est fragmentée en >2 composantes, pas cohérent
        if num > 2:
            return False
    
    # Vérifier l'organisation (ex: valeurs en couches horizontales/verticales)
    organization_score = self._compute_organization_score(block)
    
    return organization_score > 0.7
```

---

## 🎯 COUCHE 3: ATTENTION (NOUVELLE)

### Objectif
Concentrer les ressources sur **ce qui est important**

### Système d'Attention

```python
class AttentionLayer:
    """Couche 3: Système d'attention."""
    
    def __init__(self):
        self.attention_map = None
        self.focus_history = []
    
    def compute_attention_map(self, frame_before, frame_after, transformations):
        """Calculer une carte d'attention."""
        attention_map = np.zeros_like(frame_before, dtype=float)
        
        # Score 1: Mouvement
        movement_mask = (frame_before != frame_after)
        attention_map += movement_mask.astype(float) * 1.0
        
        # Score 2: Nouveauté (nouvelles valeurs)
        for value in np.unique(frame_after):
            if value not in np.unique(frame_before):
                attention_map[frame_after == value] += 0.8
        
        # Score 3: Surprise (changements inattendus)
        surprise_score = self._compute_surprise(frame_before, frame_after)
        attention_map += surprise_score
        
        # Score 4: Importance historique
        for transformation in transformations:
            if transformation.transformation_type in ['move', 'grow']:
                positions = transformation.object_after.positions
                for pos in positions:
                    attention_map[pos[0], pos[1]] += 0.5
        
        # Normaliser
        if attention_map.max() > 0:
            attention_map = attention_map / attention_map.max()
        
        self.attention_map = attention_map
        return attention_map
    
    def get_focus_regions(self, attention_map, threshold=0.5):
        """Obtenir les régions à analyser en priorité."""
        high_attention = attention_map > threshold
        labeled, num_regions = label(high_attention)
        
        focus_regions = []
        for i in range(1, num_regions + 1):
            positions = np.argwhere(labeled == i)
            focus_regions.append({
                'positions': positions,
                'attention_score': attention_map[positions[:, 0], positions[:, 1]].mean(),
                'size': len(positions)
            })
        
        # Trier par score d'attention
        focus_regions.sort(key=lambda x: x['attention_score'], reverse=True)
        
        return focus_regions
    
    def _compute_surprise(self, frame_before, frame_after):
        """Calculer le score de surprise."""
        # Surprise = changements non prédits
        # Pour l'instant, simple différence
        # Plus tard: différence entre prédit et observé
        return (frame_before != frame_after).astype(float) * 0.3
```

---

## ⚙️ COUCHE 4: CAUSALITÉ (RÉORGANISÉE)

### Changements: Maintenant AVANT la couche Monde

**Principe**: Comprendre `ACTION → TRANSFORMATION` avant de cartographier tout l'espace

```python
class CausalityLayer:
    """Couche 4: Causalité (Actions → Transformations)."""
    
    def __init__(self):
        self.causal_graph = nx.DiGraph()
        self.action_transformations = {}  # {action: [transformations]}
        self.object_behaviors = {}  # {object_id: behavior_profile}
    
    def learn_action_effect(self, action, transformations):
        """Apprendre l'effet d'une action."""
        if action not in self.action_transformations:
            self.action_transformations[action] = []
        
        self.action_transformations[action].extend(transformations)
        
        # Mettre à jour le graphe causal
        for trans in transformations:
            self.causal_graph.add_edge(
                action,
                f"transform_{trans.transformation_type}",
                delta=trans.delta_position.tolist(),
                confidence=1.0
            )
    
    def infer_object_role(self, object_id, action_effects):
        """Inférer le rôle d'un objet basé sur ses réactions aux actions."""
        behavior_profile = {
            'mobile': False,
            'affected_by_actions': [],
            'typical_transformation': None,
            'role': 'unknown'
        }
        
        # Analyser les effets
        for action, effect in action_effects.items():
            if effect.transformation_type == 'move':
                behavior_profile['mobile'] = True
                behavior_profile['affected_by_actions'].append(action)
        
        # Inférer le rôle
        if behavior_profile['mobile'] and len(behavior_profile['affected_by_actions']) >= 2:
            behavior_profile['role'] = 'controllable'  # Probablement l'avatar
        elif behavior_profile['mobile']:
            behavior_profile['role'] = 'dynamic'  # Bouge mais pas contrôlé
        else:
            behavior_profile['role'] = 'static'  # Décor ou obstacle
        
        self.object_behaviors[object_id] = behavior_profile
        return behavior_profile
    
    def discover_self(self):
        """Découvrir quel objet est l'avatar (contrôlé par les actions)."""
        candidates = []
        
        for object_id, behavior in self.object_behaviors.items():
            if behavior['role'] == 'controllable':
                candidates.append({
                    'object_id': object_id,
                    'num_actions_responsive': len(behavior['affected_by_actions']),
                    'confidence': len(behavior['affected_by_actions']) / 4.0  # 4 actions max
                })
        
        if candidates:
            # L'objet qui répond au plus d'actions = MOI
            candidates.sort(key=lambda x: x['confidence'], reverse=True)
            return candidates[0]
        
        return None
```

---

## 🗺️ COUCHE 5: MONDE (CONSTRUITE PAR CAUSALITÉ)

### Changements: Maintenant APRÈS la causalité

**Principe**: La carte du monde est construite en explorant avec les actions découvertes

```python
class WorldLayer:
    """Couche 5: Monde (construit par exploration causale)."""
    
    def __init__(self):
        self.spatial_map = None
        self.regions = []
        self.navigation_graph = None
        self.explored_positions = set()
    
    def build_map_from_exploration(self, transformations, self_object):
        """Construire la carte en explorant avec l'avatar."""
        if self.spatial_map is None:
            # Initialiser la carte (taille inférée des observations)
            self.spatial_map = {}
        
        # Pour chaque transformation de l'avatar
        for trans in transformations:
            if trans.object_before.id == self_object['object_id']:
                # Enregistrer la position avant
                pos_before = tuple(trans.object_before.centroid)
                self.explored_positions.add(pos_before)
                
                # Enregistrer la position après
                pos_after = tuple(trans.object_after.centroid)
                self.explored_positions.add(pos_after)
                
                # Si mouvement réussi: passage libre
                if trans.transformation_type == 'move':
                    self.spatial_map[pos_before] = 'passable'
                    self.spatial_map[pos_after] = 'passable'
                
                # Si pas de mouvement: obstacle
                elif trans.transformation_type == 'blocked':
                    # La position cible était bloquée
                    intended_pos = pos_before + trans.delta_position
                    self.spatial_map[tuple(intended_pos)] = 'blocked'
    
    def identify_regions(self, frame):
        """Identifier les régions après avoir exploré."""
        # Maintenant on peut analyser les valeurs pour comprendre la structure
        for value in np.unique(frame):
            mask = (frame == value)
            labeled, num = label(mask)
            
            for i in range(1, num + 1):
                positions = np.argwhere(labeled == i)
                
                # Vérifier si cette région est passable ou bloquée
                is_passable = any(tuple(pos) in self.spatial_map and 
                                self.spatial_map[tuple(pos)] == 'passable' 
                                for pos in positions)
                
                region = {
                    'value': int(value),
                    'positions': positions,
                    'type': 'passable' if is_passable else 'obstacle',
                    'size': len(positions)
                }
                
                self.regions.append(region)
```

---

## 🧠 COUCHE 6: MÉMOIRE SÉMANTIQUE (SIMPLIFIÉE)

### Changements: Concepts extraits, pas épisodes bruts

```python
class SemanticMemory:
    """Couche 6: Mémoire sémantique (concepts, pas épisodes)."""
    
    def __init__(self):
        self.concepts = {}
        self.rules = []
        self.episode_buffer = []  # Buffer temporaire
        self.max_buffer_size = 50
    
    def add_episode(self, episode):
        """Ajouter un épisode au buffer."""
        self.episode_buffer.append(episode)
        
        # Si buffer plein: extraire concepts et vider
        if len(self.episode_buffer) >= self.max_buffer_size:
            self.extract_concepts()
            self.episode_buffer = []
    
    def extract_concepts(self):
        """Extraire des concepts des épisodes."""
        # Exemple: extraire le concept "mur = impassable"
        blocked_values = []
        
        for episode in self.episode_buffer:
            for transformation in episode.transformations:
                if transformation.transformation_type == 'blocked':
                    # Quelle valeur bloquait?
                    blocked_values.append(transformation.blocking_value)
        
        # Si une valeur bloque souvent: concept "obstacle"
        if blocked_values:
            most_common = max(set(blocked_values), key=blocked_values.count)
            frequency = blocked_values.count(most_common) / len(blocked_values)
            
            if frequency > 0.7:
                self.concepts['obstacle'] = {
                    'value': most_common,
                    'property': 'impassable',
                    'confidence': frequency,
                    'examples': len(blocked_values)
                }
    
    def retrieve_concept(self, query):
        """Récupérer un concept."""
        # Recherche sémantique
        for concept_name, concept_data in self.concepts.items():
            if query in concept_name or query in concept_data.get('property', ''):
                return concept_data
        return None
```

---

## 🎯 COUCHE 7: RAISONNEMENT (INCHANGÉE)

Identique à V1, mais utilise maintenant:
- Les transformations (pas les états)
- La mémoire sémantique (pas les épisodes bruts)
- La carte construite par exploration

---

## 🔄 COUCHE 8: MÉTA-APPRENTISSAGE (INCHANGÉE)

Identique à V1.

---

## 🔮 COUCHE 9: PRÉDICTION (NOUVELLE - CŒUR DU SYSTÈME)

### Objectif
**Prédit → Compare → Apprend de l'erreur**

```python
class PredictionLayer:
    """Couche 9: Prédiction (cœur du système d'apprentissage)."""
    
    def __init__(self):
        self.prediction_model = {}  # {(state, action): predicted_state}
        self.prediction_errors = []
        self.surprise_threshold = 0.3
    
    def predict_next_state(self, current_state, action, causal_knowledge):
        """Prédire l'état suivant."""
        # Utiliser les connaissances causales
        if action in causal_knowledge.action_transformations:
            # Prédire basé sur les transformations connues
            transformations = causal_knowledge.action_transformations[action]
            
            # Calculer la transformation moyenne
            avg_delta = np.mean([t.delta_position for t in transformations], axis=0)
            
            # Prédire le nouvel état
            predicted_state = self._apply_transformation(current_state, avg_delta)
            
            return predicted_state
        else:
            # Pas de connaissance: prédire "pas de changement"
            return current_state.copy()
    
    def compute_prediction_error(self, predicted_state, observed_state):
        """Calculer l'erreur de prédiction."""
        error = np.sum(predicted_state != observed_state)
        error_rate = error / predicted_state.size
        
        self.prediction_errors.append({
            'error': error,
            'error_rate': error_rate,
            'is_surprise': error_rate > self.surprise_threshold
        })
        
        return error_rate
    
    def learn_from_error(self, predicted_state, observed_state, action, causal_layer):
        """Apprendre de l'erreur de prédiction."""
        error_rate = self.compute_prediction_error(predicted_state, observed_state)
        
        if error_rate > self.surprise_threshold:
            # Surprise! Analyser ce qui n'a pas été prédit
            unexpected_changes = self._analyze_unexpected_changes(
                predicted_state, 
                observed_state
            )
            
            # Mettre à jour les connaissances causales
            for change in unexpected_changes:
                causal_layer.update_action_effect(action, change)
            
            return {
                'surprise_level': error_rate,
                'unexpected_changes': unexpected_changes,
                'learning_occurred': True
            }
        else:
            # Prédiction correcte: renforcer la confiance
            return {
                'surprise_level': error_rate,
                'unexpected_changes': [],
                'learning_occurred': False
            }
    
    def _analyze_unexpected_changes(self, predicted, observed):
        """Analyser les changements non prédits."""
        diff = (predicted != observed)
        changed_positions = np.argwhere(diff)
        
        changes = []
        for pos in changed_positions:
            changes.append({
                'position': pos,
                'predicted_value': predicted[pos[0], pos[1]],
                'observed_value': observed[pos[0], pos[1]]
            })
        
        return changes
```

---

## 🔄 CYCLE D'APPRENTISSAGE V2 (CORRIGÉ)

### Boucle Principale avec Prédiction

```python
def learning_cycle_v2():
    """Cycle d'apprentissage complet avec prédiction."""
    
    # Initialiser toutes les couches
    sensorimotor = SensorimotorLayer()
    vision = VisionLayer()
    objects = ObjectLayer()
    attention = AttentionLayer()
    causality = CausalityLayer()
    world = WorldLayer()
    memory = SemanticMemory()
    reasoning = ReasoningLayer()
    meta = MetaLearningLayer()
    prediction = PredictionLayer()
    
    while not mastered:
        # 1. OBSERVER l'état actuel
        current_state = perceive_environment()
        
        # 2. CHOISIR une action (exploration ou exploitation)
        action = select_action(current_state, causality, meta)
        
        # 3. PRÉDIRE l'état suivant
        predicted_state = prediction.predict_next_state(
            current_state, 
            action, 
            causality
        )
        
        # 4. EXÉCUTER l'action
        observed_state = execute_action(action)
        
        # 5. COMPARER prédiction vs observation
        learning_result = prediction.learn_from_error(
            predicted_state,
            observed_state,
            action,
            causality
        )
        
        # 6. SI SURPRISE: analyser en profondeur
        if learning_result['surprise_level'] > 0.3:
            # Augmenter l'attention sur les zones surprenantes
            attention_map = attention.compute_attention_map(
                current_state,
                observed_state,
                learning_result['unexpected_changes']
            )
            
            # Analyser les régions à haute attention
            focus_regions = attention.get_focus_regions(attention_map)
            
            for region in focus_regions[:3]:  # Top 3
                analyze_region_deeply(region, vision, objects)
        
        # 7. ENREGISTRER la transformation
        transformation = objects.record_transformation(
            current_state,
            action,
            observed_state
        )
        
        # 8. METTRE À JOUR les connaissances causales
        causality.learn_action_effect(action, transformation)
        
        # 9. CONSTRUIRE/METTRE À JOUR la carte du monde
        self_object = causality.discover_self()
        if self_object:
            world.build_map_from_exploration(transformation, self_object)
        
        # 10. EXTRAIRE des concepts (tous les 50 épisodes)
        episode = create_episode(current_state, action, observed_state, transformation)
        memory.add_episode(episode)
        
        # 11. AUTO-OPTIMISER la stratégie
        efficiency = meta.analyze_learning_efficiency()
        meta.adjust_strategy(efficiency)
```

---

## 📊 EXEMPLE CONCRET: ls20-9607627b

### Phase 0: Sensorimotrice (Épisodes 1-10)

```
Épisode 1:
  ACTION1 → Changement détecté (52 pixels)
  Conclusion: ACTION1 a un effet

Épisode 2:
  ACTION2 → Changement détecté (2 pixels)
  Conclusion: ACTION2 a un effet (faible)

Épisode 3:
  ACTION3 → Changement détecté (52 pixels)
  Conclusion: ACTION3 a un effet

Épisode 4:
  ACTION4 → Changement détecté (52 pixels)
  Conclusion: ACTION4 a un effet

Concept formé:
  "Toutes les actions produisent des changements"
```

### Phase 1-2: Vision + Objets (Épisodes 11-30)

```
Découvertes:
  - 8 valeurs uniques
  - Valeur 4: 2609 pixels, statique, permanent
  - Valeur 11: 84 pixels, décroissant, temporaire
  - Valeur 12: 10 pixels, mobile
  - Valeur 9: 45 pixels (5 composantes)

Patterns détectés:
  - Valeur 9: patterns 3×3 (glyphes)
  - Valeur 12+9: structure 5×5 cohérente
```

### Phase 3: Attention (Épisodes 31-50)

```
Carte d'attention après ACTION4:
  - Zone haute attention: rows 45-49, cols 34-43
  - Raison: mouvement détecté
  - Focus: analyser cette zone en priorité

Découverte:
  - Structure 5×5 se déplace ensemble
  - Composante 9 (ID 3) bouge avec 12
```

### Phase 4: Causalité (Épisodes 51-80)

```
Transformations apprises:
  ACTION1 + Objet_12 → Déplacement [-5, 0]
  ACTION4 + Objet_12 → Déplacement [0, +5]
  ACTION1 + Objet_9_3 → Déplacement [-5, 0]
  ACTION4 + Objet_9_3 → Déplacement [0, +5]

Inférence:
  Objet_12 + Objet_9_3 = Structure contrôlable (MOI)
  Objet_9_1,2,4,5 = Statiques (décor)
```

### Phase 5: Monde (Épisodes 81-120)

```
Exploration:
  - ACTION4 × 8 → Bloqué (valeur 4 détectée)
  - ACTION1 × 6 → Passage libre (valeur 3)
  - ACTION3 × 4 → Passage libre (valeur 3)

Carte construite:
  - Valeur 4 = Murs (impassable)
  - Valeur 3 = Corridors (passable)
  - Valeur 5 = Salles (passable)
  - 2 salles identifiées avec glyphes 9
```

### Phase 6-7: Mémoire + Raisonnement (Épisodes 121-150)

```
Concepts extraits:
  - "Mur" (valeur 4) = impassable
  - "Corridor" (valeur 3) = passage
  - "Salle" (valeur 5) = zone spéciale
  - "Glyphe" (valeur 9 fixe) = objectif?

Hypothèse formée:
  "Atteindre une salle avec glyphe"
  Confiance: 75%

Plan:
  1. Naviguer vers corridor vertical
  2. Traverser band horizontale
  3. Entrer dans salle bottom-left
  4. Aligner avec glyphe
```

### Phase 9: Prédiction (Tout au long)

```
Épisode 100:
  État actuel: Bloc à (45, 36)
  Action: ACTION4
  Prédiction: Bloc à (45, 41)
  Observation: Bloc à (45, 41)
  Erreur: 0.00
  → Prédiction correcte! Confiance renforcée

Épisode 105:
  État actuel: Bloc à (45, 51)
  Action: ACTION4
  Prédiction: Bloc à (45, 56)
  Observation: Bloc à (45, 51) (pas de mouvement!)
  Erreur: 0.15
  → SURPRISE! Analyser pourquoi

Analyse:
  - Valeur 4 détectée à (45, 56)
  - Concept mis à jour: "Valeur 4 bloque ACTION4"
```

---

## 📊 COMPARAISON V1 vs V2

| Aspect | V1 | V2 |
|--------|----|----|
| Couches | 7 | 10 |
| Couche 0 | ❌ Absente | ✅ Sensorimotrice |
| Unité de base | États | Triplets (Avant, Action, Après) |
| Ordre | Vision→Objets→Monde→Causalité | Vision→Objets→Causalité→Monde |
| Prédiction | ❌ Absente | ✅ Cœur du système |
| Attention | ❌ Absente | ✅ Focus intelligent |
| Détection structures | Codée (12+9) | Générique |
| Mémoire | Épisodes bruts | Concepts extraits |
| Apprentissage | Accumulation | Erreur de prédiction |

---

## 🎯 PLAN D'IMPLÉMENTATION V2

### Phase 1: Fondations (Semaine 1)
- [ ] Couche 0 (Sensorimotrice)
- [ ] Couche 1 (Vision améliorée)
- [ ] Couche 2 (Objets avec transformations)
- [ ] Tests sur ls20-9607627b

### Phase 2: Attention et Causalité (Semaine 2)
- [ ] Couche 3 (Attention)
- [ ] Couche 4 (Causalité)
- [ ] Découverte automatique de l'avatar
- [ ] Tests de prédiction simple

### Phase 3: Monde et Mémoire (Semaine 3)
- [ ] Couche 5 (Monde par exploration)
- [ ] Couche 6 (Mémoire sémantique)
- [ ] Extraction de concepts

### Phase 4: Raisonnement (Semaine 4)
- [ ] Couche 7 (Raisonnement)
- [ ] Planification basée sur causalité
- [ ] Tests de navigation

### Phase 5: Méta et Prédiction (Semaine 5)
- [ ] Couche 8 (Méta-apprentissage)
- [ ] Couche 9 (Prédiction complète)
- [ ] Boucle d'apprentissage intégrée

### Phase 6: Validation (Semaine 6)
- [ ] Tests sur ls20-9607627b
- [ ] Première victoire attendue
- [ ] Tests sur autres puzzles

---

## 🏆 MÉTRIQUES DE SUCCÈS V2

| Métrique | V1 Objectif | V2 Objectif |
|----------|-------------|-------------|
| Épisodes pour découvrir avatar | 15 | 10 |
| Épisodes pour carte complète | 50 | 30 |
| Épisodes pour première victoire | 120 | 80 |
| Taux de prédiction correcte | N/A | >70% |
| Concepts extraits | N/A | >10 |
| Transfert à nouveau puzzle | 50% | 70% |

---

## 💡 CONCLUSION

### Améliorations Majeures de V2

1. **Couche 0 Sensorimotrice**: Comprend l'agence avant les objets
2. **Triplets (Avant, Action, Après)**: Apprend les transformations
3. **Ordre Causalité→Monde**: Construit la carte par exploration
4. **Prédiction**: Cœur du système d'apprentissage
5. **Attention**: Focus intelligent sur l'important
6. **Détection générique**: Pas de fuite de connaissance
7. **Mémoire sémantique**: Scalable et efficace

### Prochaine Étape

Implémenter la Phase 1 de V2 et valider sur ls20-9607627b.

**La victoire viendra de la COMPRÉHENSION, pas de la force brute.** 🚀

---

**Fin de l'Architecture V2 Corrigée**  
**Prochaine action**: Créer le plan d'implémentation détaillé Phase 1 V2