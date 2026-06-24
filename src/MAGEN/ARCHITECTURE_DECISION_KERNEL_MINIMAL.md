# ARCHITECTURE DECISION KERNEL MINIMAL
## De la Boucle Structurelle à la Boucle Causale

**Date**: 2026-06-14  
**Objectif**: Transformer vecteur multi-signaux en distribution d'actions  

---

## 🎯 CORRECTION STRUCTURELLE FONDAMENTALE

### Diagnostic Corrigé

**Erreur initiale**: "Absence de boucle fermée"

**Réalité**: La boucle existe structurellement, mais est **ouverte causalement**.

```
Observation → Calculs → Décision → Action → Observation
     ✅           ✅         ❌          ✅          ✅
                        (pas d'influence)
```

**Problème réel**: Les signaux riches ne modifient pas la distribution des actions.

### Reformulation du Blocage (Version Minimale)

> **Tu n'as pas de fonction qui transforme un vecteur multi-signaux en distribution d'actions.**

Tout le reste est secondaire.

---

## 🔬 ANALYSE: Pourquoi Reward Dense Est Ignoré

### Ce N'est PAS un Bug d'Intégration

C'est une **absence de projection dans l'espace de décision**.

### État Actuel (3 Espaces Déconnectés)

```
┌─────────────────────────┐
│ Espace d'Analyse        │  reward_dense, causality, novelty
│ (comprend)              │  → Calcule des métriques
└─────────────────────────┘

┌─────────────────────────┐
│ Espace de Décision      │  reputation, sparse, heuristique
│ (choisit)               │  → Sélectionne action
└─────────────────────────┘

┌─────────────────────────┐
│ Espace de Perception    │  grid, entités, spatial
│ (observe)               │  → Décrit état
└─────────────────────────┘
```

**Manque**: Fonction de projection commune vers choix d'action.

**Conséquence**: Chaque module "comprend", aucun module "influence".

---

## 🎓 INSIGHT CLÉ: Détection Agent vs Ancrage Causal

### Reformulation Correcte de Phase 0

**Erreur**: "Détecter un agent"
- = Classification
- = Module descriptif de plus
- = Inutile pour la décision

**Correct**: "Identifier la variable d'état qui maximise la prédiction des transitions futures"
- = Optimisation prédictive
- = Ancrage causal
- = Utile pour la décision

### Heuristique Humaine (Utilisateur)

> "Quand je joue manuellement, la première chose que je fais (haut/bas/clic), le premier qui bouge, c'est moi. C'est là que je m'identifie dans le jeu."

**Traduction formelle**:
```python
# Agent = entité dont le mouvement est causalement lié à l'action
agent = argmax_entity(correlation(entity.motion, action_history))
```

**Critère minimal**:
1. Première action → Observer changement état
2. Identifier entité(s) qui ont bougé
3. Répéter 2-3 actions → Confirmer corrélation
4. Agent = entité avec corrélation maximale

---

## 🏗️ ARCHITECTURE DECISION KERNEL MINIMAL

### Objectif

**Construire une fonction de décision unique qui prend tous les signaux et les convertit en compétition directe entre actions.**

### Principe

```python
# Au lieu de:
action = select_best(reputation_scores)  # Ignore reward_dense

# Faire:
action = select_best(
    combine_all_signals(
        reputation_scores,
        reward_dense_scores,
        spatial_guidance_scores,
        causal_impact_scores
    )
)
```

### Architecture Minimale (Testable sur 5×5)

```python
class DecisionKernelMinimal:
    """
    Kernel de décision unifié: vecteur multi-signaux → distribution actions.
    """
    
    def __init__(self):
        # Poids appris (initialisés uniformes)
        self.weights = {
            'reward_dense': 0.25,
            'reputation': 0.25,
            'spatial': 0.25,
            'causal': 0.25
        }
        
        # Historique pour apprentissage
        self.history = []
    
    def compute_action_scores(self, state, action_candidates):
        """
        Calcule score unifié pour chaque action candidate.
        
        Returns:
            dict: {action: unified_score}
        """
        scores = {}
        
        for action in action_candidates:
            # Signal 1: Reward dense (novelty, causality, progress)
            reward_score = self._compute_reward_score(state, action)
            
            # Signal 2: Réputation (historique)
            reputation_score = self._compute_reputation_score(action)
            
            # Signal 3: Guidance spatiale (vers goal)
            spatial_score = self._compute_spatial_score(state, action)
            
            # Signal 4: Impact causal (prédiction transition)
            causal_score = self._compute_causal_score(state, action)
            
            # FUSION PONDÉRÉE
            unified_score = (
                self.weights['reward_dense'] * reward_score +
                self.weights['reputation'] * reputation_score +
                self.weights['spatial'] * spatial_score +
                self.weights['causal'] * causal_score
            )
            
            scores[action] = unified_score
        
        return scores
    
    def select_action(self, state, action_candidates):
        """
        Sélectionne action basée sur scores unifiés.
        """
        scores = self.compute_action_scores(state, action_candidates)
        
        # Softmax pour distribution probabiliste
        probs = self._softmax(scores)
        
        # Sélection (epsilon-greedy)
        if random.random() < 0.1:  # 10% exploration
            action = random.choice(action_candidates)
        else:
            action = max(probs, key=probs.get)
        
        # Log pour apprentissage
        self.history.append({
            'state': state,
            'action': action,
            'scores': scores,
            'probs': probs
        })
        
        return action
    
    def update_weights(self, reward_obtained):
        """
        Ajuste poids basés sur reward obtenu (gradient descent simple).
        """
        if len(self.history) < 2:
            return
        
        # Dernière décision
        last = self.history[-1]
        
        # Gradient simple: augmenter poids des signaux qui ont favorisé l'action
        learning_rate = 0.01
        
        for signal_name in self.weights:
            # Si signal a favorisé action choisie et reward > 0
            if reward_obtained > 0:
                # Augmenter poids
                self.weights[signal_name] += learning_rate * reward_obtained
            else:
                # Diminuer poids
                self.weights[signal_name] -= learning_rate * abs(reward_obtained)
        
        # Normaliser poids (somme = 1.0)
        total = sum(self.weights.values())
        for k in self.weights:
            self.weights[k] /= total
    
    def _compute_reward_score(self, state, action):
        """Prédit reward dense si action exécutée."""
        # Utilise ProgressiveRewardShaper
        return reward_shaper.predict(state, action)
    
    def _compute_reputation_score(self, action):
        """Score réputation historique."""
        # Utilise ActionReputationSystem
        return reputation_system.get_score(action)
    
    def _compute_spatial_score(self, state, action):
        """Score guidance spatiale (vers goal)."""
        # Utilise GridAnalyzer
        if state.agent_pos and state.goal_pos:
            # Distance avant action
            dist_before = distance(state.agent_pos, state.goal_pos)
            # Distance après action (prédite)
            agent_pos_after = predict_position(state.agent_pos, action)
            dist_after = distance(agent_pos_after, state.goal_pos)
            # Score = réduction distance
            return max(0, dist_before - dist_after)
        return 0.0
    
    def _compute_causal_score(self, state, action):
        """Score impact causal (prédiction transition)."""
        # Utilise CausalGraph
        return causal_graph.predict_impact(state, action)
    
    def _softmax(self, scores):
        """Convertit scores en distribution probabiliste."""
        import math
        exp_scores = {k: math.exp(v) for k, v in scores.items()}
        total = sum(exp_scores.values())
        return {k: v/total for k, v in exp_scores.items()}
```

---

## 🧪 TEST MINIMAL (Environnement 5×5)

### Setup Test

```python
# Environnement simple
grid = Grid(5, 5)
agent_pos = (2, 2)  # Centre
goal_pos = (4, 4)   # Coin
actions = ['up', 'down', 'left', 'right']

# Decision Kernel
kernel = DecisionKernelMinimal()

# Simulation 20 steps
for step in range(20):
    # État actuel
    state = {
        'grid': grid,
        'agent_pos': agent_pos,
        'goal_pos': goal_pos
    }
    
    # Décision
    action = kernel.select_action(state, actions)
    
    # Exécution
    agent_pos, reward = env.step(action)
    
    # Apprentissage
    kernel.update_weights(reward)
    
    # Log
    print(f"Step {step}: {action}, reward={reward}, weights={kernel.weights}")
```

### Métriques Succès

1. **Convergence poids**: Variance > 0.3 après 20 steps
2. **Progression spatiale**: Distance goal réduite > 50%
3. **Discrimination actions**: Actions "vers goal" sélectionnées > 70%

---

## 🔄 INTÉGRATION DANS MAGEN V27.1

### Phase 1: Remplacement Décision Actuelle

**Avant** (`magen_arc_agi_agent_v27_cognitive.py`):
```python
# Ligne ~800: Décision basée réputation OU exploration
if confidence > 0.5:
    action = reputation_system.select_action(...)  # Ignore reward_dense
else:
    action = exploration_policy.select_action(...)  # Ignore reward_dense
```

**Après**:
```python
# Décision unifiée
action = decision_kernel.select_action(
    state=current_state,
    action_candidates=available_actions
)

# Apprentissage après step
reward = env.step(action)
decision_kernel.update_weights(reward)
```

### Phase 2: Ancrage Agent (Heuristique Humaine)

```python
class AgentAnchor:
    """
    Identifie agent via corrélation motion ↔ action (heuristique humaine).
    """
    
    def __init__(self):
        self.candidate_entities = []
        self.motion_history = {}
        self.action_history = []
    
    def observe_first_action(self, state_before, action, state_after):
        """
        Première action: identifier entités qui ont bougé.
        """
        # Comparer états
        entities_moved = self._detect_motion(state_before, state_after)
        
        # Candidats agent = entités qui ont bougé
        self.candidate_entities = entities_moved
        
        # Log
        self.action_history.append(action)
        for entity in entities_moved:
            if entity.id not in self.motion_history:
                self.motion_history[entity.id] = []
            self.motion_history[entity.id].append(action)
    
    def confirm_agent(self, min_correlation=0.7):
        """
        Après 3-5 actions: confirmer agent via corrélation maximale.
        """
        if len(self.action_history) < 3:
            return None
        
        # Calculer corrélation pour chaque candidat
        correlations = {}
        for entity_id, motions in self.motion_history.items():
            corr = self._compute_correlation(motions, self.action_history)
            correlations[entity_id] = corr
        
        # Agent = corrélation maximale
        if correlations:
            agent_id = max(correlations, key=correlations.get)
            if correlations[agent_id] > min_correlation:
                return agent_id
        
        return None
    
    def _detect_motion(self, state_before, state_after):
        """Identifie entités dont position a changé."""
        moved = []
        for entity in state_after.entities:
            if entity.pos != state_before.get_entity(entity.id).pos:
                moved.append(entity)
        return moved
    
    def _compute_correlation(self, motions, actions):
        """Corrélation motion ↔ action."""
        # Simple: % actions où entité a bougé
        return len(motions) / len(actions)
```

### Phase 3: Intégration Complète

```python
# Dans MAGENArcAGIAgentV271.__init__()
self.decision_kernel = DecisionKernelMinimal()
self.agent_anchor = AgentAnchor()

# Dans decide()
# 1. Ancrage agent (premières actions)
if self.step_count < 5:
    self.agent_anchor.observe_first_action(
        state_before=self.last_state,
        action=action,
        state_after=current_state
    )
    
    if self.step_count == 4:
        agent_id = self.agent_anchor.confirm_agent()
        if agent_id:
            self.agent_detected = True
            self.agent_id = agent_id

# 2. Décision unifiée
action = self.decision_kernel.select_action(
    state=current_state,
    action_candidates=available_actions
)

# 3. Apprentissage
reward = env.step(action)
self.decision_kernel.update_weights(reward)
```

---

## 📊 MÉTRIQUES DE VALIDATION

### Niveau 1: Decision Kernel Fonctionne

- ✅ Variance poids > 0.3 après 20 steps
- ✅ Tous signaux contribuent (aucun poids = 0)
- ✅ Poids évoluent basés sur reward

### Niveau 2: Ancrage Agent Fonctionne

- ✅ Agent détecté après 3-5 actions (80% jeux)
- ✅ Corrélation motion ↔ action > 0.7
- ✅ Pas de faux positifs (entités non-contrôlables)

### Niveau 3: Performance Globale

- ✅ Score ARC-AGI-3: 0% → 15% (+15pp)
- ✅ Actions "vers goal": 30% → 70% (+40pp)
- ✅ Temps/jeu: 72s → 50s (-30%)

---

## 🎯 PLAN D'IMPLÉMENTATION (1 Semaine)

### Jour 1-2: Decision Kernel Minimal

**Fichier**: `lumvorax2/src/MAGEN/core/decision_kernel_minimal.py`

**Contenu**: Classe `DecisionKernelMinimal` complète (200 lignes)

**Test**: Environnement 5×5, 20 steps, validation métriques

### Jour 3-4: Agent Anchor (Heuristique Humaine)

**Fichier**: `lumvorax2/src/MAGEN/core/agent_anchor.py`

**Contenu**: Classe `AgentAnchor` complète (150 lignes)

**Test**: 10 jeux simples, validation détection > 80%

### Jour 5-6: Intégration MAGEN V27.1

**Fichier**: `lumvorax2/src/MAGEN/core/magen_arc_agi_agent_v27_cognitive.py`

**Modifications**:
- Ligne ~50: Import `DecisionKernelMinimal`, `AgentAnchor`
- Ligne ~100: Init kernel + anchor
- Ligne ~800: Remplacement décision actuelle

**Test**: 20 jeux ARC-AGI-3, validation score > 10%

### Jour 7: Validation & Forensic

**Exécution**: 50 jeux ARC-AGI-3

**Analyse**: Logs forensic temps réel

**Validation**:
- Poids kernel convergent?
- Agent détecté?
- Score améliore?

---

## 🏆 CONCLUSION

### Correction Fondamentale

**Erreur**: "Absence de boucle fermée"

**Réalité**: Boucle fermée structurellement, ouverte causalement

**Solution**: Fonction de projection commune (Decision Kernel)

### Priorité Absolue

> **Construire une fonction de décision unique qui prend tous les signaux et les convertit en compétition directe entre actions.**

C'est là que tout se débloque.

### Prochaine Étape

Implémenter Decision Kernel Minimal (200 lignes) et tester sur environnement 5×5 avant intégration MAGEN.

---

**Prochaine action**: Créer `decision_kernel_minimal.py` et `agent_anchor.py`.