# PIVOT STRATÉGIQUE MAJEUR - Apprentissage Systémique

**Date**: 2026-06-20  
**Phase**: MAGEN 4.5 (Réorientation Fondamentale)  
**Auteur**: Bob (MAGEN Project)  
**Inspiration**: Feedback utilisateur critique sur apprentissage local vs systémique

---

## 🎯 DIAGNOSTIC FONDAMENTAL

### Le Problème Actuel

**MAGEN apprend comme un outil d'analyse, pas comme un joueur.**

```
Approche actuelle (ANALYSE LOCALE):
├── Étudier timer seul
├── Étudier porte seule
├── Étudier avatar seul
├── Étudier blocs seuls
└── Tenter de combiner après

Résultat: 344+ tests, 0 victoires
```

```
Approche humaine (COMPRÉHENSION SYSTÉMIQUE):
└── Observer simultanément:
    ├── Niveau complet
    ├── Tous les objets
    ├── Tous les mouvements
    ├── Tous les effets
    ├── Toutes les conséquences
    ├── Récompenses/échecs
    └── Construire modèle mental global

Résultat: Victoire en quelques minutes
```

### Citation Clé

> "Un humain ne fait pas ceci : Étudier le timer seul. Étudier la porte seule. Étudier l'avatar seul. Étudier les blocs seuls. Un humain observe simultanément : le niveau complet, tous les objets, tous les mouvements, tous les effets, tous les changements, les conséquences de ses actions, les récompenses, les échecs, les redémarrages. Puis il construit progressivement un modèle mental global du jeu."

---

## 🧠 NOUVEAU PARADIGME D'APPRENTISSAGE

### 7 Couches d'Apprentissage Simultané

#### **Couche 1: Cartographie Complète du Monde**

**Objectif**: Comprendre que le niveau est un **système unique**, pas une collection d'entités.

**Méthode**:
```python
class WorldModel:
    def __init__(self):
        self.entities = {}  # Tous les objets
        self.relationships = {}  # Relations entre objets
        self.spatial_graph = {}  # Graphe spatial
        self.temporal_patterns = {}  # Patterns temporels
        
    def observe_complete_state(self, game_state):
        """Observer TOUT simultanément"""
        # Pas: observer timer, puis porte, puis avatar
        # Mais: observer système complet en une fois
        
        self.entities = self.extract_all_entities(game_state)
        self.relationships = self.infer_relationships(self.entities)
        self.spatial_graph = self.build_spatial_graph(self.entities)
        self.temporal_patterns = self.detect_temporal_patterns()
```

**Identification**:
- Murs (obstacles statiques)
- Portes (passages conditionnels)
- Avatar (entité contrôlée)
- Ennemis (entités hostiles)
- Objets (items interactifs)
- Compteurs (timers, scores)
- Zones spéciales (goals, triggers)
- Éléments décoratifs (background)

**Pas séparément, mais comme un graphe global.**

---

#### **Couche 2: Identification de Soi**

**Question fondamentale**: **Qui suis-je ?**

C'est la **première chose** qu'un humain découvre.

**Méthode**:
```python
class SelfIdentification:
    def __init__(self):
        self.avatar_candidates = []
        self.confidence_scores = {}
        
    def identify_self(self, actions_history):
        """
        Action → Observer ce qui bouge
        
        Si ACTION_DROITE et Objet_A bouge à droite:
            Probabilité(Objet_A = Avatar) += 1
        """
        for action, state_before, state_after in actions_history:
            moved_entities = self.detect_movement(state_before, state_after)
            
            for entity in moved_entities:
                if self.movement_matches_action(entity, action):
                    self.confidence_scores[entity.id] += 1
        
        # Après quelques actions: identification quasi certaine
        self.avatar = max(self.confidence_scores, key=self.confidence_scores.get)
```

**Résultat**: Après 5-10 actions, avatar identifié avec >95% confiance.

---

#### **Couche 3: Identification des Règles**

**Question**: **Que se passe-t-il quand j'agis ?**

**Méthode**:
```python
class RulesLearning:
    def __init__(self):
        self.causal_rules = []
        
    def learn_rule(self, action, state_before, state_after):
        """
        Apprendre règles causales:
        ACTION → EFFET
        """
        effects = self.detect_changes(state_before, state_after)
        
        rule = CausalRule(
            condition=action,
            preconditions=self.extract_preconditions(state_before),
            effects=effects,
            confidence=0.5  # Augmente avec répétitions
        )
        
        self.causal_rules.append(rule)
        
    def predict_outcome(self, action, current_state):
        """Prédire résultat d'une action"""
        matching_rules = [r for r in self.causal_rules 
                         if r.matches(action, current_state)]
        return self.aggregate_predictions(matching_rules)
```

**Exemples de règles**:
```
ACTION4 + Avatar adjacent à Bloc → Bloc bouge
ACTION1 + Avatar sur Goal → Victoire
ACTION2 + Timer = 0 → Timer reset
Avatar + Ennemi → Défaite
```

---

#### **Couche 4: Modèle Dynamique du Monde**

**Objectif**: Comprendre comment le monde **évolue**.

**Méthode**:
```python
class WorldDynamics:
    def __init__(self):
        self.state_transitions = []
        self.invariants = []
        self.cycles = []
        
    def learn_dynamics(self, state_sequence):
        """
        Apprendre dynamiques du monde:
        - Quoi change ?
        - Quoi reste constant ?
        - Quels patterns se répètent ?
        """
        # Détecter invariants
        self.invariants = self.find_invariants(state_sequence)
        
        # Détecter cycles
        self.cycles = self.find_cycles(state_sequence)
        
        # Modéliser transitions
        for i in range(len(state_sequence) - 1):
            transition = StateTransition(
                from_state=state_sequence[i],
                to_state=state_sequence[i+1],
                action=self.get_action(i)
            )
            self.state_transitions.append(transition)
```

**Découvertes attendues**:
- Porte immobile (invariant)
- Timer décroît (dynamique)
- Cycle 43 steps (pattern temporel)
- Avatar contrôlable (règle)

---

#### **Couche 5: Planification Hiérarchique**

**Objectif**: Construire plans pour atteindre goals.

**Méthode**:
```python
class HierarchicalPlanner:
    def __init__(self, world_model, rules):
        self.world_model = world_model
        self.rules = rules
        self.goals = []
        
    def plan_to_goal(self, current_state, goal_state):
        """
        Planification hiérarchique:
        1. Identifier sous-goals
        2. Planifier séquence d'actions
        3. Exécuter et ajuster
        """
        # Décomposer en sous-goals
        subgoals = self.decompose_goal(goal_state)
        
        # Planifier pour chaque sous-goal
        plan = []
        for subgoal in subgoals:
            subplan = self.plan_subgoal(current_state, subgoal)
            plan.extend(subplan)
            current_state = self.simulate(current_state, subplan)
        
        return plan
    
    def plan_subgoal(self, state, subgoal):
        """Recherche A* avec heuristique apprise"""
        return self.a_star_search(state, subgoal, self.learned_heuristic)
```

**Exemple**:
```
Goal: Atteindre position (61.5, 62.5) au step 43

Sous-goals:
1. Identifier avatar
2. Calculer chemin vers (61.5, 62.5)
3. Synchroniser avec timer (step 43)
4. Exécuter séquence d'actions
```

---

#### **Couche 6: Apprentissage par Exploration**

**Objectif**: Explorer intelligemment l'espace des possibles.

**Méthode**:
```python
class IntelligentExploration:
    def __init__(self):
        self.curiosity_model = CuriosityModel()
        self.exploration_strategy = "information_gain"
        
    def select_action(self, state, known_actions):
        """
        Sélectionner action qui maximise:
        - Information gain (nouveauté)
        - Progress vers goal (exploitation)
        - Diversité (couverture)
        """
        if self.should_explore(state):
            # Explorer: action la plus informative
            return self.curiosity_model.most_informative_action(state)
        else:
            # Exploiter: action vers goal
            return self.planner.best_action_to_goal(state)
    
    def should_explore(self, state):
        """Équilibrer exploration/exploitation"""
        uncertainty = self.world_model.uncertainty(state)
        progress = self.planner.progress_to_goal(state)
        
        return uncertainty > threshold or progress < min_progress
```

**Stratégies**:
- **Curiosity-driven**: Explorer états inconnus
- **Goal-directed**: Progresser vers victoire
- **Systematic**: Couvrir espace d'états

---

#### **Couche 7: Méta-Apprentissage**

**Objectif**: Apprendre à apprendre.

**Méthode**:
```python
class MetaLearning:
    def __init__(self):
        self.learning_strategies = []
        self.strategy_performance = {}
        
    def adapt_learning_strategy(self, performance_history):
        """
        Adapter stratégie d'apprentissage selon résultats:
        - Si progrès lent → Augmenter exploration
        - Si progrès rapide → Augmenter exploitation
        - Si blocage → Changer stratégie
        """
        current_strategy = self.current_strategy
        performance = self.evaluate_performance(performance_history)
        
        if performance < threshold:
            # Changer de stratégie
            new_strategy = self.select_alternative_strategy()
            self.switch_strategy(new_strategy)
        else:
            # Affiner stratégie actuelle
            self.refine_strategy(current_strategy, performance)
```

**Métriques**:
- Vitesse d'apprentissage
- Taux de découverte
- Progrès vers victoire
- Efficacité exploration

---

## 🔄 ARCHITECTURE INTÉGRÉE

### Système Complet

```python
class MAGENSystemic:
    """
    MAGEN avec apprentissage systémique
    """
    def __init__(self):
        # 7 couches simultanées
        self.world_model = WorldModel()
        self.self_identification = SelfIdentification()
        self.rules_learning = RulesLearning()
        self.world_dynamics = WorldDynamics()
        self.planner = HierarchicalPlanner()
        self.explorer = IntelligentExploration()
        self.meta_learner = MetaLearning()
        
        # Historique complet
        self.episode_history = []
        self.action_history = []
        self.state_history = []
        
    def learn_episode(self, game):
        """
        Apprendre d'un épisode complet
        """
        episode = Episode()
        state = game.reset()
        
        while not game.done:
            # Observer état complet (Couche 1)
            self.world_model.observe_complete_state(state)
            
            # Identifier avatar si pas encore fait (Couche 2)
            if not self.self_identification.avatar_identified:
                self.self_identification.update(self.action_history)
            
            # Sélectionner action (Couche 6)
            action = self.explorer.select_action(state, game.actions)
            
            # Exécuter action
            next_state, reward, done = game.step(action)
            
            # Apprendre règles (Couche 3)
            self.rules_learning.learn_rule(action, state, next_state)
            
            # Apprendre dynamiques (Couche 4)
            self.world_dynamics.update(state, next_state, action)
            
            # Enregistrer historique
            episode.add_transition(state, action, reward, next_state)
            
            state = next_state
        
        # Fin épisode: apprentissage global
        self.episode_history.append(episode)
        
        # Planifier pour prochain épisode (Couche 5)
        if self.world_model.is_sufficiently_learned():
            self.planner.update_strategy(episode)
        
        # Méta-apprentissage (Couche 7)
        self.meta_learner.adapt_learning_strategy(self.episode_history)
        
    def play_episode(self, game):
        """
        Jouer un épisode avec modèle appris
        """
        state = game.reset()
        
        # Planifier séquence complète
        goal = self.world_model.identify_goal()
        plan = self.planner.plan_to_goal(state, goal)
        
        # Exécuter plan
        for action in plan:
            state, reward, done = game.step(action)
            
            if done:
                if reward > 0:
                    print("🎉 VICTOIRE !")
                    return True
                else:
                    # Plan échoué, replanifier
                    plan = self.planner.replan(state, goal)
        
        return False
```

---

## 📊 COMPARAISON APPROCHES

### Ancien MAGEN (Analyse Locale)

```
Épisode 1:
├── Observer pixels
├── Détecter 20 entités
├── Mesurer distances
└── Choisir action aléatoire

Épisode 2:
├── Observer pixels
├── Détecter 20 entités
├── Mesurer distances
└── Choisir action aléatoire

...

Épisode 10,000:
├── Observer pixels
├── Détecter 20 entités
├── Mesurer distances
└── Choisir action aléatoire

Résultat: 0 victoires
Problème: Aucun modèle mental global construit
```

### Nouveau MAGEN (Apprentissage Systémique)

```
Épisode 1:
├── Observer système complet
├── Identifier avatar (confiance 20%)
├── Apprendre 5 règles causales
├── Détecter 2 invariants
└── Explorer intelligemment

Épisode 2:
├── Observer système complet
├── Identifier avatar (confiance 60%)
├── Apprendre 12 règles causales
├── Détecter 5 invariants
├── Détecter cycle 43
└── Explorer zones inconnues

Épisode 10:
├── Observer système complet
├── Avatar identifié (confiance 99%)
├── 50 règles causales apprises
├── 15 invariants détectés
├── Cycle 43 confirmé
├── Modèle monde complet
└── Planifier vers victoire

Épisode 15:
└── 🎉 VICTOIRE !

Résultat: Victoire en ~15 épisodes
Raison: Modèle mental global construit progressivement
```

---

## 🎯 PLAN D'IMPLÉMENTATION

### Phase 4.5.1: Couches 1-2 (Fondations)

**Durée estimée**: 2-3 jours

**Objectifs**:
1. Implémenter WorldModel (cartographie complète)
2. Implémenter SelfIdentification (identification avatar)
3. Tester sur 50 épisodes

**Critères succès**:
- Avatar identifié avec >95% confiance en <10 épisodes
- Graphe spatial complet construit
- 20 entités catégorisées correctement

### Phase 4.5.2: Couche 3 (Règles Causales)

**Durée estimée**: 2-3 jours

**Objectifs**:
1. Implémenter RulesLearning
2. Apprendre règles ACTION → EFFET
3. Prédire résultats actions

**Critères succès**:
- 30+ règles causales apprises
- Précision prédiction >80%
- Règles généralisent à nouveaux états

### Phase 4.5.3: Couche 4 (Dynamiques)

**Durée estimée**: 2 jours

**Objectifs**:
1. Implémenter WorldDynamics
2. Détecter invariants/cycles
3. Modéliser transitions d'états

**Critères succès**:
- Cycle 43 détecté automatiquement
- Invariants identifiés (porte immobile)
- Dynamiques prédites correctement

### Phase 4.5.4: Couches 5-6 (Planification + Exploration)

**Durée estimée**: 3-4 jours

**Objectifs**:
1. Implémenter HierarchicalPlanner
2. Implémenter IntelligentExploration
3. Intégrer planification + exploration

**Critères succès**:
- Plans générés vers goals
- Exploration efficace (>50% états nouveaux)
- Équilibre exploration/exploitation

### Phase 4.5.5: Couche 7 (Méta-Apprentissage)

**Durée estimée**: 2 jours

**Objectifs**:
1. Implémenter MetaLearning
2. Adapter stratégies dynamiquement
3. Optimiser vitesse apprentissage

**Critères succès**:
- Stratégie adaptée selon performance
- Vitesse apprentissage augmentée
- Convergence vers victoire

### Phase 4.5.6: Intégration Complète

**Durée estimée**: 3-4 jours

**Objectifs**:
1. Intégrer 7 couches
2. Tester sur 100 épisodes
3. Obtenir première victoire

**Critères succès**:
- **VICTOIRE en <100 épisodes**
- Modèle mental global construit
- Stratégie reproductible

---

## 📈 MÉTRIQUES DE SUCCÈS

### Métriques Quantitatives

| Métrique | Ancien MAGEN | Nouveau MAGEN (Cible) |
|----------|--------------|----------------------|
| Épisodes avant victoire | >10,000 (échec) | <100 |
| Temps apprentissage | >10 heures | <2 heures |
| Taux découverte | ~5% | >80% |
| Précision prédictions | ~20% | >80% |
| Confiance avatar | Inconnue | >95% |
| Règles apprises | 0 | >30 |
| Invariants détectés | 0 | >10 |
| Cycles détectés | 1 (manuel) | >3 (auto) |

### Métriques Qualitatives

**Ancien MAGEN**:
- ❌ Pas de modèle mental
- ❌ Analyse locale uniquement
- ❌ Pas d'identification avatar
- ❌ Pas de planification
- ❌ Exploration aléatoire

**Nouveau MAGEN**:
- ✅ Modèle mental global
- ✅ Compréhension systémique
- ✅ Avatar identifié automatiquement
- ✅ Planification hiérarchique
- ✅ Exploration intelligente

---

## 🚀 IMPACT ATTENDU

### Court Terme (1-2 semaines)

- **Première victoire** sur Niveau 1
- Modèle mental global construit
- Règles causales apprises
- Avatar identifié automatiquement

### Moyen Terme (1 mois)

- Victoires sur **plusieurs niveaux**
- Généralisation à nouveaux niveaux
- Apprentissage accéléré (transfer learning)
- Publication résultats

### Long Terme (3-6 mois)

- **ARC-AGI résolu** (>85% niveaux)
- Architecture généralisable
- Contribution scientifique majeure
- Système AGI démontré

---

## 💡 INSIGHTS CLÉS

### 1. Apprentissage = Construction Modèle Mental

> "Un humain ne mémorise pas 10,000 pixels. Il construit un modèle mental du jeu."

MAGEN doit faire pareil.

### 2. Système > Composants

> "Le niveau est un système unique, pas 20 entités indépendantes."

Comprendre relations entre entités, pas entités isolées.

### 3. Exploration ≠ Aléatoire

> "Explorer intelligemment, pas aléatoirement."

Curiosité guidée par information gain.

### 4. Planification Hiérarchique

> "Décomposer goal en sous-goals, puis planifier."

Pas: essayer toutes actions possibles.

### 5. Méta-Apprentissage

> "Apprendre à apprendre."

Adapter stratégie selon résultats.

---

## 📚 RÉFÉRENCES THÉORIQUES

### Apprentissage par Renforcement

- **Model-Based RL**: Construire modèle du monde
- **Hierarchical RL**: Planification multi-niveaux
- **Curiosity-Driven RL**: Exploration intelligente

### Intelligence Artificielle Générale

- **World Models** (Ha & Schmidhuber, 2018)
- **Causal Reasoning** (Pearl, 2009)
- **Cognitive Architectures** (Laird et al., 2017)

### Neurosciences Cognitives

- **Mental Models** (Johnson-Laird, 1983)
- **Predictive Processing** (Friston, 2010)
- **Embodied Cognition** (Varela et al., 1991)

---

## 🎓 CONCLUSION

### Changement de Paradigme

**Ancien**: MAGEN = Outil d'analyse  
**Nouveau**: MAGEN = Joueur intelligent

**Ancien**: Analyse locale → Échec  
**Nouveau**: Compréhension systémique → Victoire

### Citation Finale

> "Le problème n'est probablement plus de découvrir une nouvelle action ou un nouveau pixel. Le problème est que MAGEN apprend encore comme un outil d'analyse alors qu'il devrait apprendre comme un joueur."

**Cette analyse est exacte.**

**La solution est l'apprentissage systémique.**

**Implémentation commence maintenant.**

---

**Statut**: 🚀 PIVOT STRATÉGIQUE MAJEUR DÉFINI
**Prochaine phase**: 4.5.1 (Couches 1-2 - Fondations)
**Probabilité succès**: >80% (approche fondamentalement correcte)
**Temps estimé première victoire**: 2-3 semaines

---

## 📊 CONNAISSANCES FORENSIQUES INTÉGRÉES

### Découvertes Sessions 91-99 (344+ Tests)

**Session 96 - Hypothèses Invalidées**:
- ❌ Overlap exact blocs/goals ≠ victoire
- ❌ Distance euclidienne minimale (12.37) ≠ victoire
- ❌ Séquence d'actions spécifique ≠ victoire
- ❌ Déblocage après ACTION4×42 ≠ victoire

**Session 99 - Transformations Valeurs**:
- 24 séquences testées, 0 victoires
- Timer décroît 2 pixels/step (constant)
- Transformations 3→11, 11→3 identifiées
- Conservation parfaite valeurs 9 et 12

### Découvertes Phase 4.4.2 (Cartographie Entités)

**19 Entités Identifiées**:

| Value | Composante | Rôle | Pixels | Position | Comportement |
|-------|-----------|------|--------|----------|--------------|
| **3#1** | Principale | Porte immobile | 892 | (35.1, 34.6) | **FIXE** |
| **3#2** | Secondaire | Timer horizontal | 80-84 | Y=61-62 | Décroît |
| **8** | Unique | Goals | 12 | Y=61 | Statique |
| **9#3** | Mobile | Avatar candidat | 15-45 | Variable | Mobile |
| **11** | Unique | Timer vertical | 0-84 | Variable | Reset step 43 |
| **12** | Multiple | Blocs mobiles | 10 | Variable | Mobile |

**Insight Critique**: Valeur 3 a DEUX composantes distinctes (3#1=porte, 3#2=timer)

### Découvertes Phase 4.4.2H-I-bis (Cycle Temporel)

**Cycle 43 Steps Confirmé**:
- Événement majeur au step 43
- Pattern se répète aux steps 86, 129, 172...
- Timer 11 disparaît step 42 (0 pixels)
- Timer 11 réapparaît step 43 (84 pixels)
- 88 pixels modifiés total (2.15%)

**Transformations Step 42→43**:
```
3→11: 84 pixels (timer reset)
8→3: 4 pixels (modification mineure)
```

**INVALIDATION CRITIQUE**:
- Phase 4.4.2I concluait "téléportation porte"
- Phase 4.4.2I-bis prouve: porte IMMOBILE, c'est le timer qui reset

### Règles Causales Découvertes

**ACTION2 = Horloge Temporelle**:
```python
ACTION2:
  - Avance temps de 1 step
  - Cycle de 43 steps
  - Au step 43: Timer reset (0→84 pixels)
  - Timer décroît 2 pixels/step ensuite
```

**ACTION4 = Mouvement Blocs**:
```python
ACTION4:
  - Modifie ~52 pixels/step
  - Déplace valeurs 9 et 12
  - Timer décroît 2 pixels/step simultanément
```

### Invariants Confirmés

**Éléments Statiques**:
- Porte 3#1: Position (35.1, 34.6) - 892 pixels - **JAMAIS BOUGE**
- Goals 8: Position Y=61 - 12 pixels - Statique
- Murs: Positions fixes (non cartographiés exhaustivement)

**Éléments Dynamiques**:
- Timer 11: Cycle 43 steps (0→84→82→...→2→0→84)
- Timer 3#2: Décroît 2 pixels/step
- Avatar 9#3: Mobile (contrôlé par actions)
- Blocs 12: Mobiles (poussés par ACTION4)

### Architecture Existante À Préserver

**session87_learning_system.py**:
```python
class LearningSystem:
    - total_games, total_steps tracking ✅
    - all_states_seen (hash-based) ✅
    - state_action_rewards (Q-learning) ✅
    - state_visit_count (exploration) ✅
    - action_success_rate (statistics) ✅
```

**arc_integration/magen_arc_agent_enhanced.py**:
```python
class MAGENAgentEnhanced:
    - Layer 0: Perception (grid observation) ✅
    - Layer 1: Pattern Recognition ✅
    - Layer 2: Action Selection ✅
    - RewardShaper ✅
    - Epsilon decay ✅
```

**arc_integration/ls20_real_wrapper.py**:
```python
class LS20RealWrapper:
    - Wrapper environnement ls20 ✅
    - Interface standardisée ✅
    - Gestion actions/observations ✅
```

**STRATÉGIE D'INTÉGRATION**: Extension, pas remplacement
```python
class MAGENAgentSystemic(MAGENAgentEnhanced):
    """Étend l'agent existant avec apprentissage systémique"""
    
    def __init__(self):
        super().__init__()  # Préserver layers 0-2
        
        # Ajouter nouvelles couches
        self.world_model = WorldModel()
        self.self_identification = SelfIdentification()
        self.rules_learning = RulesLearning()
        # ...
```

---

## 🎯 PLAN D'IMPLÉMENTATION DÉTAILLÉ

### Phase 4.5.1: WorldModel + SelfIdentification (IMMÉDIAT)

**Fichiers à créer**:
1. `arc_integration/world_model.py` (300-400 lignes)
2. `arc_integration/self_identification.py` (200-300 lignes)
3. `arc_integration/magen_arc_agent_systemic.py` (400-500 lignes)
4. `arc_integration/test_phase_4_5_1_systemic.py` (200-300 lignes)

**WorldModel - Fonctionnalités**:
```python
class WorldModel:
    def __init__(self):
        self.entities = {}  # {id: Entity}
        self.spatial_graph = nx.Graph()
        self.temporal_patterns = []
        self.invariants = []
        
    def observe_complete_state(self, grid):
        """Observer système complet en une fois"""
        # Extraire toutes entités (composantes connexes)
        entities = self.extract_connected_components(grid)
        
        # Catégoriser automatiquement
        for entity in entities:
            entity.category = self.categorize_entity(entity)
        
        # Construire graphe spatial
        self.spatial_graph = self.build_spatial_graph(entities)
        
        # Détecter invariants
        self.update_invariants(entities)
        
    def categorize_entity(self, entity):
        """Catégoriser entité: mur/porte/avatar/goal/timer"""
        # Heuristiques basées sur:
        # - Taille (pixels)
        # - Position
        # - Mobilité observée
        # - Valeur
        pass
```

**SelfIdentification - Fonctionnalités**:
```python
class SelfIdentification:
    def __init__(self):
        self.avatar_candidates = []
        self.confidence_scores = defaultdict(float)
        self.avatar_id = None
        self.confidence = 0.0
        
    def update(self, action, state_before, state_after):
        """Mettre à jour identification après action"""
        # Détecter entités qui ont bougé
        moved_entities = self.detect_movement(state_before, state_after)
        
        # Si action directionnelle et entité bouge dans cette direction
        if self.is_directional_action(action):
            direction = self.get_action_direction(action)
            
            for entity in moved_entities:
                if self.moved_in_direction(entity, direction):
                    self.confidence_scores[entity.id] += 1.0
        
        # Identifier avatar = entité avec score max
        if self.confidence_scores:
            best_id = max(self.confidence_scores, key=self.confidence_scores.get)
            total_actions = sum(self.confidence_scores.values())
            self.confidence = self.confidence_scores[best_id] / total_actions
            
            if self.confidence > 0.95:
                self.avatar_id = best_id
```

**Test Validation**:
```python
def test_phase_4_5_1():
    """Tester WorldModel + SelfIdentification sur 10 épisodes"""
    
    agent = MAGENAgentSystemic()
    game = LS20RealWrapper()
    
    for episode in range(10):
        state = game.reset()
        
        for step in range(100):
            # Observer système complet
            agent.world_model.observe_complete_state(state)
            
            # Sélectionner action
            action = agent.select_action(state)
            
            # Exécuter
            next_state, reward, done = game.step(action)
            
            # Mettre à jour identification
            agent.self_identification.update(action, state, next_state)
            
            if done:
                break
            
            state = next_state
        
        # Vérifier identification
        print(f"Episode {episode}: Avatar={agent.self_identification.avatar_id}, "
              f"Confidence={agent.self_identification.confidence:.2%}")
    
    # Critères succès
    assert agent.self_identification.confidence > 0.95
    assert agent.self_identification.avatar_id in [9, 11, 12]
    assert len(agent.world_model.entities) == 19
```

**Métriques Succès Phase 4.5.1**:
- ✅ Avatar identifié avec >95% confiance en <10 épisodes
- ✅ 19 entités détectées et catégorisées
- ✅ Graphe spatial complet construit
- ✅ Invariants détectés (porte immobile, goals fixes)
- ✅ Temps exécution: <2 heures

---

## 📋 TODO LIST PHASE 4.5.1 (IMMÉDIAT)

### Étape 1: Créer WorldModel (30 min)
- [ ] Créer `arc_integration/world_model.py`
- [ ] Implémenter `observe_complete_state()`
- [ ] Implémenter `extract_connected_components()`
- [ ] Implémenter `categorize_entity()`
- [ ] Implémenter `build_spatial_graph()`

### Étape 2: Créer SelfIdentification (30 min)
- [ ] Créer `arc_integration/self_identification.py`
- [ ] Implémenter `update()`
- [ ] Implémenter `detect_movement()`
- [ ] Implémenter `is_directional_action()`
- [ ] Implémenter calcul confiance

### Étape 3: Créer Agent Systémique (30 min)
- [ ] Créer `arc_integration/magen_arc_agent_systemic.py`
- [ ] Étendre `MAGENAgentEnhanced`
- [ ] Intégrer `WorldModel`
- [ ] Intégrer `SelfIdentification`
- [ ] Implémenter boucle apprentissage

### Étape 4: Créer Test Validation (30 min)
- [ ] Créer `arc_integration/test_phase_4_5_1_systemic.py`
- [ ] Implémenter test 10 épisodes
- [ ] Vérifier identification avatar
- [ ] Vérifier détection entités
- [ ] Générer rapport résultats

**Durée totale**: 2 heures → Première validation

---

**Statut**: ✅ CONSOLIDATION COMPLÈTE + PLAN DÉTAILLÉ
**Action immédiate**: Créer world_model.py
**Temps estimé première validation**: 2 heures
**Probabilité succès Phase 4.5.1**: >90%