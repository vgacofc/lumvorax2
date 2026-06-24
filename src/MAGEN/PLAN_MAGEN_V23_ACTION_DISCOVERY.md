# 🧠 PLAN MAGEN V23 - ACTION DISCOVERY & WORLD MODEL

## 🎯 VISION STRATÉGIQUE

### Changement de Paradigme

**V22 (Actuel)**:
```
Pattern détecté → Action prédéfinie → Échec
```

**V23 (Cible)**:
```
Observation → Hypothèse causale → Expérimentation → Modèle du monde → Action informée
```

### Insight Clé du Feedback Utilisateur

> "MAGEN V22 n'échoue plus par instabilité. Il échoue par inadéquation stratégique entre perception ↔ action."

**Preuve empirique**:
- ✅ 228 décisions cohérentes sans crash
- ✅ Scores décroissants 0.60→0.02 (apprentissage actif)
- ✅ Patterns détectés (object_counting, diagonal_symmetry)
- ❌ Actions géométriques fixes inadaptées

**Diagnostic**:
```
Intelligence × Espace d'actions insuffisant = 0 score
```

**Solution**:
```
Intelligence × Action Discovery × World Model = Score > 0
```

---

## 🏗️ ARCHITECTURE V23

### Nouveaux Modules (3 Priorités)

#### 1. ActionDiscoveryEngine (Priorité Absolue)
```python
class ActionModel:
    """Modèle appris d'une action inconnue"""
    action_id: GameAction
    observed_effects: List[StateTransition]
    success_contexts: List[Context]
    failure_contexts: List[Context]
    entropy_gain: float
    causal_signature: np.ndarray
    
class ActionDiscoveryEngine:
    """Découvre les actions par expérimentation"""
    
    def __init__(self):
        self.action_models = {}  # GameAction -> ActionModel
        self.exploration_budget = 50  # Actions dédiées à la découverte
        
    def discover_action(self, action: GameAction, env) -> ActionModel:
        """Expérimente une action pour comprendre ses effets"""
        transitions = []
        for _ in range(10):  # 10 tests par action
            state_before = env.observe()
            env.step(action)
            state_after = env.observe()
            transitions.append((state_before, state_after))
        
        # Analyse causale
        effects = self._analyze_causal_effects(transitions)
        contexts = self._extract_success_contexts(transitions)
        
        return ActionModel(
            action_id=action,
            observed_effects=effects,
            success_contexts=contexts,
            entropy_gain=self._compute_information_gain(transitions)
        )
    
    def _analyze_causal_effects(self, transitions):
        """Identifie les effets causaux d'une action"""
        # Clustering des changements observés
        # Exemple: "ACTION3 crée symétrie horizontale"
        pass
    
    def should_explore(self, action: GameAction) -> bool:
        """Décide si une action mérite exploration"""
        if action not in self.action_models:
            return True  # Action inconnue
        
        model = self.action_models[action]
        if model.entropy_gain > 0.5:
            return True  # Action informative
        
        return False
```

#### 2. WorldModel (Priorité 2)
```python
class StateTransition:
    """Transition d'état observée"""
    state_before: np.ndarray
    action: GameAction
    state_after: np.ndarray
    reward: float
    context: Dict
    
class WorldModel:
    """Modèle prédictif du monde"""
    
    def __init__(self):
        self.transitions = []  # Mémoire des transitions
        self.causal_graph = {}  # (state, action) -> predicted_state
        
    def record_transition(self, transition: StateTransition):
        """Enregistre une transition observée"""
        self.transitions.append(transition)
        self._update_causal_graph(transition)
    
    def predict_outcome(self, state: np.ndarray, action: GameAction) -> np.ndarray:
        """Prédit l'état résultant d'une action"""
        # Recherche transitions similaires
        similar = self._find_similar_states(state)
        
        # Agrégation prédictions
        predictions = [t.state_after for t in similar if t.action == action]
        
        if predictions:
            return np.mean(predictions, axis=0)
        else:
            return state  # Pas de prédiction
    
    def _update_causal_graph(self, transition):
        """Met à jour le graphe causal"""
        # Clustering automatique:
        # - Quelles actions produisent quels effets
        # - Dans quels contextes
        # - Sur quels patterns
        pass
    
    def get_action_physics(self, action: GameAction) -> Dict:
        """Retourne la 'physique' apprise d'une action"""
        action_transitions = [t for t in self.transitions if t.action == action]
        
        return {
            'avg_reward': np.mean([t.reward for t in action_transitions]),
            'effect_patterns': self._cluster_effects(action_transitions),
            'success_rate': len([t for t in action_transitions if t.reward > 0]) / len(action_transitions)
        }
```

#### 3. InformationGainReward (Priorité 3)
```python
class InformationGainReward:
    """Récompense l'exploration informative"""
    
    def __init__(self):
        self.knowledge_base = set()  # États connus
        
    def compute_reward(self, transition: StateTransition) -> float:
        """Calcule reward = score_reward + knowledge_reward"""
        
        # Reward classique (niveaux complétés)
        score_reward = transition.reward
        
        # Reward épistémique (information gagnée)
        state_hash = hash(transition.state_after.tobytes())
        if state_hash not in self.knowledge_base:
            knowledge_reward = 0.1  # Nouvel état découvert
            self.knowledge_base.add(state_hash)
        else:
            knowledge_reward = 0.0
        
        # Reward causal (compréhension action)
        causal_reward = self._compute_causal_understanding(transition)
        
        return score_reward + knowledge_reward + causal_reward
    
    def _compute_causal_understanding(self, transition) -> float:
        """Mesure si la transition améliore la compréhension causale"""
        # Exemple: transition confirme/infirme une hypothèse
        # → reward positif si réduit incertitude
        pass
```

---

## 🔄 WORKFLOW V23

### Phase 1: Exploration (50 premières actions)
```python
def exploration_phase(env, action_discovery, world_model):
    """Phase dédiée à la découverte des actions"""
    
    for action in [ACTION1, ACTION2, ACTION3, ACTION4, ACTION5, ACTION6, ACTION7]:
        print(f"[EXPLORATION] Découverte {action}...")
        
        # Expérimentation
        action_model = action_discovery.discover_action(action, env)
        
        # Logging forensique
        print(f"  Effets observés: {action_model.observed_effects}")
        print(f"  Gain d'information: {action_model.entropy_gain:.3f}")
        
        # Mise à jour world model
        for transition in action_model.observed_effects:
            world_model.record_transition(transition)
    
    # Synthèse
    print("[EXPLORATION] Physique du jeu apprise:")
    for action in action_discovery.action_models:
        physics = world_model.get_action_physics(action)
        print(f"  {action}: success_rate={physics['success_rate']:.2f}")
```

### Phase 2: Exploitation (150 actions restantes)
```python
def exploitation_phase(env, action_discovery, world_model, pattern_matcher):
    """Phase d'exploitation avec actions informées"""
    
    while not env.done and env.actions_taken < 200:
        # Perception
        obs = env.observe()
        pattern = pattern_matcher.detect_exact_pattern(obs.grid)
        
        # Prédiction
        best_action = None
        best_predicted_reward = -float('inf')
        
        for action in action_discovery.action_models:
            # Prédire outcome
            predicted_state = world_model.predict_outcome(obs.grid, action)
            
            # Évaluer outcome
            predicted_reward = evaluate_state(predicted_state, pattern)
            
            if predicted_reward > best_predicted_reward:
                best_action = action
                best_predicted_reward = predicted_reward
        
        # Exécution
        obs_after = env.step(best_action)
        
        # Apprentissage
        transition = StateTransition(obs.grid, best_action, obs_after.grid, obs_after.reward)
        world_model.record_transition(transition)
```

---

## 🎯 OBJECTIFS V23

### Métriques de Succès

| Métrique | V22 | V23 Cible | Justification |
|----------|-----|-----------|---------------|
| **Score** | 0/3 (0%) | > 0/3 | Action discovery permet adaptation |
| **Actions découvertes** | 0 | 7 | Comprendre ACTION1-7 |
| **Transitions enregistrées** | 0 | > 200 | World model actif |
| **Information gain** | N/A | > 0 | Exploration informative |
| **Prédictions correctes** | N/A | > 50% | World model fonctionnel |

### Validation Scientifique

**Hypothèse V23**:
> Un agent qui découvre ses actions et modélise le monde peut s'adapter à des jeux interactifs inconnus.

**Preuves attendues**:
1. ✅ Actions découvertes avec effets causaux identifiés
2. ✅ World model prédit outcomes avec >50% précision
3. ✅ Score > 0% (au moins 1 niveau complété)
4. ✅ Exploration informative (knowledge_reward > 0)

---

## 🔧 IMPLÉMENTATION

### Fichiers à Créer

1. **`core/action_discovery_engine.py`** (300 lignes)
   - ActionModel
   - ActionDiscoveryEngine
   - Expérimentation causale

2. **`core/world_model.py`** (250 lignes)
   - StateTransition
   - WorldModel
   - Graphe causal

3. **`core/information_gain_reward.py`** (150 lignes)
   - InformationGainReward
   - Reward épistémique
   - Reward causal

4. **`magen_arc_agi_agent_v23.py`** (700 lignes)
   - Intégration 3 nouveaux modules
   - Phase exploration + exploitation
   - Logging forensique enrichi

### Modifications Existantes

**`magen_arc_agi_agent.py`** → **`magen_arc_agi_agent_v23.py`**:
```python
class MAGENArcAgiAgentV23:
    def __init__(self):
        # V22 modules
        self.tle = TransformationLearningEngine()
        self.pattern_matcher = PatternMatcher()
        self.object_extractor = ObjectExtractor()
        self.meta_arbiter = MetaArbiter()
        self.reputation_system = ActionReputationSystem()
        self.budget_manager = ExplorationBudgetManager()
        
        # V23 modules (NOUVEAUX)
        self.action_discovery = ActionDiscoveryEngine()
        self.world_model = WorldModel()
        self.info_gain_reward = InformationGainReward()
        
        self.exploration_phase_active = True
        self.exploration_budget = 50
    
    def play_game(self, env):
        """Joue un jeu avec exploration puis exploitation"""
        
        # Phase 1: Exploration (50 actions)
        if self.exploration_phase_active:
            self._exploration_phase(env)
            self.exploration_phase_active = False
        
        # Phase 2: Exploitation (150 actions)
        self._exploitation_phase(env)
```

---

## 📊 TESTS V23

### Test 1: Action Discovery
```bash
python test_action_discovery.py
# Attendu: 7 actions découvertes avec effets causaux
```

### Test 2: World Model
```bash
python test_world_model.py
# Attendu: Prédictions >50% précision après 100 transitions
```

### Test 3: Information Gain
```bash
python test_information_gain.py
# Attendu: knowledge_reward > 0 pour nouveaux états
```

### Test 4: Agent Complet V23
```bash
python test_magen_v23_local.py
# Attendu: Score > 0/3 (au moins 1 niveau complété)
```

---

## 🚀 ROADMAP

### Semaine 1: Modules Core
- [ ] Créer ActionDiscoveryEngine
- [ ] Créer WorldModel
- [ ] Créer InformationGainReward
- [ ] Tests unitaires

### Semaine 2: Intégration
- [ ] Créer MAGENArcAgiAgentV23
- [ ] Intégrer 3 nouveaux modules
- [ ] Phase exploration + exploitation
- [ ] Tests locaux 3 jeux

### Semaine 3: Validation
- [ ] Exécution 31 jeux complets
- [ ] Analyse logs forensiques
- [ ] Rapport final V23
- [ ] Comparaison V22 vs V23

---

## 💡 INSIGHTS CLÉS

### 1. "L'intelligence sans les bons outils reste impuissante"
**V22**: Cognition sophistiquée mais actions inadaptées
**V23**: Cognition + découverte d'outils = adaptation

### 2. "Scores décroissants 0.60→0.02 = apprentissage actif"
**Preuve**: Le système comprend l'échec
**Problème**: Pas d'alternatives à explorer
**Solution**: Action discovery élargit l'espace

### 3. "Actions = tests, sondes, interactions, validations d'hypothèses"
**V22**: Actions = transformations géométriques
**V23**: Actions = primitives à modéliser

### 4. "Séparer score_reward et knowledge_reward"
**Problème V22**: Exploration pénalisée (reward=0)
**Solution V23**: Exploration récompensée (info_gain>0)

---

## 🔬 VALIDATION SCIENTIFIQUE

### Hypothèse Centrale
> "Un agent qui découvre ses actions et modélise le monde peut résoudre des jeux interactifs inconnus."

### Prédictions Testables
1. **Action Discovery**: 7 actions avec signatures causales distinctes
2. **World Model**: Prédictions >50% précision après 100 transitions
3. **Score**: >0% (au moins 1 niveau complété sur 3 jeux)
4. **Exploration**: knowledge_reward >0 pour >30% des actions

### Métriques de Validation
```json
{
  "v22_baseline": {
    "score": 0.0,
    "actions_discovered": 0,
    "world_model_accuracy": 0.0,
    "exploration_reward": 0.0
  },
  "v23_target": {
    "score": ">0.0",
    "actions_discovered": 7,
    "world_model_accuracy": ">0.5",
    "exploration_reward": ">0.0"
  }
}
```

---

## 📝 CONCLUSION

### Changement de Paradigme

**V22 → V23**:
```
Reconnaissance de patterns
    ↓
Découverte causale + Modélisation du monde
```

### Impact Attendu

**Technique**:
- ✅ Actions adaptées au contexte
- ✅ Prédictions informées
- ✅ Exploration récompensée

**Scientifique**:
- ✅ Validation architecture cognitive
- ✅ Preuve de concept action discovery
- ✅ Embryon de physique interne

**Compétition**:
- ✅ Score > 0% (objectif minimal)
- ✅ Baseline crédible pour itérations futures

---

**Prochaine étape**: Créer les 3 modules core (ActionDiscoveryEngine, WorldModel, InformationGainReward) et tester localement avant intégration complète.