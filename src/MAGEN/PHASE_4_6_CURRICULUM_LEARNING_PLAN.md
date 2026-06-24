# PHASE 4.6 - CURRICULUM LEARNING MULTI-PUZZLES

## 🎯 Vision Stratégique

**Objectif**: Transformer MAGEN d'un "résolveur de puzzle unique" en un **"apprenant généraliste d'arcade"** capable d'apprendre progressivement sur TOUS les puzzles ARC en rotation continue.

### Changement de Paradigme

**❌ Approche Actuelle (Phase 4.5.9)**:
```
Puzzle A → 404 épisodes consécutifs → Échec
```

**✅ Nouvelle Approche (Phase 4.6)**:
```
CYCLE 1:
  Observe humain Puzzle A → Joue Puzzle A niveau 1 → Échec → Apprend
  Observe humain Puzzle B → Joue Puzzle B niveau 1 → Échec → Apprend
  Observe humain Puzzle C → Joue Puzzle C niveau 1 → Échec → Apprend
  ...
  Observe humain Puzzle Z → Joue Puzzle Z niveau 1 → Échec → Apprend

CYCLE 2:
  Retour Puzzle A (avec connaissances B,C,...Z)
  Retour Puzzle B (avec connaissances A,C,...Z)
  ...

CYCLE N:
  Puzzle A niveau 1 → VICTOIRE ✅
  → Débloquer Puzzle A niveau 2
  → Continuer rotation avec niveau 2
```

---

## 📊 Architecture Système

### 1. Composants Principaux

```
┌─────────────────────────────────────────────────────────────┐
│                    CURRICULUM MANAGER                        │
│  - Gestion progression multi-puzzles                        │
│  - Sélection puzzle suivant                                 │
│  - Déblocage niveaux                                        │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│              HUMAN DEMONSTRATION LEARNER                     │
│  - Extraction trajectoires replays humains                  │
│  - Analyse comportements gagnants                           │
│  - Identification objectifs implicites                      │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                PUZZLE ROTATION ENGINE                        │
│  - Boucle d'apprentissage cyclique                         │
│  - Rotation automatique puzzles                             │
│  - Gestion échecs/victoires                                 │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│              SHARED KNOWLEDGE BASE                           │
│  - Mémoire inter-puzzles persistante                        │
│  - Concepts réutilisables (portes, timers, blocs)          │
│  - Stratégies transférables                                 │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                  CONCEPT EXTRACTOR                           │
│  - Détection patterns communs                               │
│  - Abstraction concepts (pousser, ouvrir, éviter)          │
│  - Construction bibliothèque réutilisable                   │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                 MAGEN AGENT (7 Couches)                     │
│  - Utilise connaissances partagées                         │
│  - Applique concepts appris                                 │
│  - Génère nouvelles connaissances                           │
└─────────────────────────────────────────────────────────────┘
```

---

## 🔄 Boucle d'Apprentissage Complète

### Cycle Complet (1 Puzzle)

```python
def learning_cycle_one_puzzle(puzzle_id, level):
    """
    Cycle complet pour 1 puzzle
    """
    # ÉTAPE 1: Observer humain
    human_demo = load_human_demonstration(puzzle_id, level)
    concepts = extract_concepts_from_demo(human_demo)
    shared_knowledge.add_concepts(concepts)
    
    # ÉTAPE 2: Jouer avec connaissances partagées
    agent.load_shared_knowledge(shared_knowledge)
    result = agent.play_episode(puzzle_id, level)
    
    # ÉTAPE 3: Analyser résultat
    if result.victory:
        # Victoire → Renforcer connaissances
        shared_knowledge.reinforce_successful_strategy(result)
        curriculum.mark_level_completed(puzzle_id, level)
        curriculum.unlock_next_level(puzzle_id, level + 1)
    else:
        # Échec → Apprendre différences
        divergences = compare_with_human_demo(result, human_demo)
        shared_knowledge.add_failure_pattern(divergences)
    
    # ÉTAPE 4: Sauvegarder expérience
    shared_knowledge.save_episode_memory(result)
    
    return result
```

### Rotation Multi-Puzzles

```python
def rotation_engine(num_cycles=1000):
    """
    Boucle principale de rotation
    """
    for cycle in range(num_cycles):
        print(f"\n🔄 CYCLE {cycle + 1}")
        
        # Obtenir liste puzzles actifs
        active_puzzles = curriculum.get_active_puzzles()
        
        for puzzle_id, level in active_puzzles:
            # Cycle complet pour ce puzzle
            result = learning_cycle_one_puzzle(puzzle_id, level)
            
            # Statistiques
            curriculum.update_statistics(puzzle_id, level, result)
        
        # Révision puzzles maîtrisés
        if cycle % 10 == 0:
            revision_puzzles = curriculum.get_puzzles_needing_revision()
            for puzzle_id, level in revision_puzzles:
                learning_cycle_one_puzzle(puzzle_id, level)
        
        # Rapport progression
        if cycle % 50 == 0:
            generate_progress_report(cycle)
```

---

## 📚 Composants Détaillés

### 1. CurriculumManager

**Responsabilités**:
- Gérer progression sur tous les puzzles
- Débloquer niveaux progressivement
- Sélectionner puzzle suivant intelligemment
- Garantir révision régulière

**Critères de Progression**:
```python
LEVEL_COMPLETION_CRITERIA = {
    'success_rate': 0.95,  # 95% réussite requis
    'consecutive_wins': 10,  # 10 victoires consécutives
    'stability_cycles': 5    # Stable sur 5 cycles
}
```

**Algorithme Sélection Puzzle**:
```python
def select_next_puzzle(self):
    """
    Sélectionne puzzle suivant selon priorités:
    1. Puzzles jamais tentés
    2. Puzzles avec progression récente
    3. Puzzles nécessitant révision
    4. Puzzles aléatoires (exploration)
    """
    # Priorité 1: Nouveaux puzzles
    if self.untried_puzzles:
        return random.choice(self.untried_puzzles)
    
    # Priorité 2: Progression récente
    improving = self.get_improving_puzzles()
    if improving:
        return self.select_by_improvement_rate(improving)
    
    # Priorité 3: Révision
    needing_revision = self.get_puzzles_needing_revision()
    if needing_revision:
        return random.choice(needing_revision)
    
    # Priorité 4: Exploration
    return random.choice(self.all_puzzles)
```

---

### 2. HumanDemonstrationLearner

**Source de Données**:
- URL Replay: `https://arcprize.org/replay/8aed7120-f7a9-45a1-837a-68bc7dc37a4f`
- Extraction: États, actions, transitions, objectifs

**Format Démonstration**:
```python
class HumanDemonstration:
    puzzle_id: str
    level: int
    frames: List[GameState]  # Chaque frame du replay
    actions: List[Action]     # Actions humaines
    transitions: List[Transition]  # État → Action → Nouvel État
    implicit_goals: List[Goal]  # Objectifs déduits
    success_patterns: List[Pattern]  # Patterns gagnants
    duration: float
    score: int
```

**Extraction Concepts**:
```python
def extract_concepts_from_demo(demo):
    """
    Extrait concepts réutilisables d'une démo humaine
    """
    concepts = []
    
    # Analyser séquences d'actions
    for i in range(len(demo.actions) - 1):
        state_before = demo.frames[i]
        action = demo.actions[i]
        state_after = demo.frames[i + 1]
        
        # Détecter concepts
        if is_door_opening(state_before, action, state_after):
            concepts.append(Concept('open_door', ...))
        
        if is_block_pushing(state_before, action, state_after):
            concepts.append(Concept('push_block', ...))
        
        if is_enemy_avoiding(state_before, action, state_after):
            concepts.append(Concept('avoid_enemy', ...))
    
    return concepts
```

---

### 3. SharedKnowledgeBase

**Structure Mémoire**:
```python
class SharedKnowledgeBase:
    # Concepts réutilisables
    concepts: Dict[str, Concept] = {
        'open_door': Concept(...),
        'push_block': Concept(...),
        'avoid_enemy': Concept(...),
        'reach_goal': Concept(...),
        'use_teleporter': Concept(...),
    }
    
    # Stratégies transférables
    strategies: Dict[str, Strategy] = {
        'explore_then_exploit': Strategy(...),
        'follow_human_path': Strategy(...),
        'avoid_known_failures': Strategy(...),
    }
    
    # Patterns d'échec à éviter
    failure_patterns: List[FailurePattern] = []
    
    # Patterns de succès à reproduire
    success_patterns: List[SuccessPattern] = []
    
    # Mémoire épisodique inter-puzzles
    episode_memories: List[EpisodeMemory] = []
```

**Persistance**:
```python
def save_knowledge(self, filepath):
    """
    Sauvegarde connaissances sur disque
    Format: JSON ou pickle pour persistance entre sessions
    """
    knowledge_data = {
        'concepts': self.concepts,
        'strategies': self.strategies,
        'failure_patterns': self.failure_patterns,
        'success_patterns': self.success_patterns,
        'episode_memories': self.episode_memories[-1000:]  # Garder 1000 derniers
    }
    
    with open(filepath, 'wb') as f:
        pickle.dump(knowledge_data, f)
```

---

### 4. ConceptExtractor

**Concepts Cibles**:
```python
CORE_CONCEPTS = [
    # Manipulation objets
    'push_block',
    'pull_block',
    'carry_object',
    'drop_object',
    
    # Interactions environnement
    'open_door',
    'close_door',
    'activate_switch',
    'use_teleporter',
    
    # Navigation
    'reach_goal',
    'avoid_obstacle',
    'avoid_enemy',
    'follow_path',
    
    # Timing
    'wait_for_timer',
    'synchronize_actions',
    'react_to_event',
]
```

**Détection Automatique**:
```python
def detect_concept(self, state_before, action, state_after):
    """
    Détecte automatiquement quel concept est utilisé
    """
    # Analyser changements d'état
    changes = compute_state_diff(state_before, state_after)
    
    # Règles de détection
    if 'door_opened' in changes and action == 'interact':
        return Concept('open_door', confidence=0.95)
    
    if 'block_moved' in changes and action in ['up', 'down', 'left', 'right']:
        return Concept('push_block', confidence=0.90)
    
    # ... autres règles
    
    return None
```

---

## 🎓 Système de Révision

### Critères Révision

```python
class RevisionSystem:
    def needs_revision(self, puzzle_id, level):
        """
        Détermine si un puzzle nécessite révision
        """
        stats = self.get_puzzle_stats(puzzle_id, level)
        
        # Révision si:
        # 1. Pas joué depuis longtemps
        if stats.cycles_since_last_play > 100:
            return True
        
        # 2. Taux succès en baisse
        if stats.recent_success_rate < 0.80:
            return True
        
        # 3. Oubli détecté (échec après succès)
        if stats.consecutive_failures > 2 and stats.was_mastered:
            return True
        
        return False
```

### Planification Révisions

```python
def schedule_revisions(self):
    """
    Planifie révisions selon courbe d'oubli
    """
    revision_schedule = {
        'immediate': [],      # Révision immédiate (échec récent)
        'short_term': [],     # Révision dans 10 cycles
        'medium_term': [],    # Révision dans 50 cycles
        'long_term': [],      # Révision dans 100 cycles
    }
    
    for puzzle_id, level in self.mastered_puzzles:
        cycles_since = self.cycles_since_last_play(puzzle_id, level)
        
        if cycles_since > 100:
            revision_schedule['immediate'].append((puzzle_id, level))
        elif cycles_since > 50:
            revision_schedule['short_term'].append((puzzle_id, level))
        # ... etc
    
    return revision_schedule
```

---

## 📈 Métriques et Suivi

### Métriques Globales

```python
class GlobalMetrics:
    # Progression générale
    total_cycles: int
    total_episodes: int
    total_victories: int
    global_success_rate: float
    
    # Par puzzle
    puzzles_attempted: int
    puzzles_mastered: int  # 95% success rate
    puzzles_in_progress: int
    
    # Par niveau
    levels_unlocked: Dict[str, int]  # puzzle_id → max_level
    
    # Apprentissage
    concepts_learned: int
    strategies_discovered: int
    human_demos_analyzed: int
    
    # Transfert connaissances
    successful_transfers: int  # Concept appris sur A utilisé sur B
    failed_transfers: int
```

### Rapports Progression

```python
def generate_progress_report(cycle):
    """
    Génère rapport détaillé progression
    """
    report = f"""
    ╔══════════════════════════════════════════════════════════╗
    ║         RAPPORT PROGRESSION - CYCLE {cycle}              ║
    ╚══════════════════════════════════════════════════════════╝
    
    📊 STATISTIQUES GLOBALES
    ├─ Cycles complétés: {metrics.total_cycles}
    ├─ Épisodes joués: {metrics.total_episodes}
    ├─ Victoires totales: {metrics.total_victories}
    └─ Taux succès global: {metrics.global_success_rate:.1%}
    
    🎮 PROGRESSION PUZZLES
    ├─ Puzzles tentés: {metrics.puzzles_attempted}
    ├─ Puzzles maîtrisés: {metrics.puzzles_mastered}
    └─ Puzzles en cours: {metrics.puzzles_in_progress}
    
    🧠 APPRENTISSAGE
    ├─ Concepts appris: {metrics.concepts_learned}
    ├─ Stratégies découvertes: {metrics.strategies_discovered}
    ├─ Démos humaines analysées: {metrics.human_demos_analyzed}
    └─ Transferts réussis: {metrics.successful_transfers}
    
    🏆 TOP 5 PUZZLES MAÎTRISÉS
    {format_top_puzzles(metrics.mastered_puzzles)}
    
    ⚠️  TOP 5 PUZZLES DIFFICILES
    {format_difficult_puzzles(metrics.difficult_puzzles)}
    """
    
    return report
```

---

## 🚀 Plan d'Implémentation

### Phase 4.6.1 - Infrastructure (Semaine 1)

1. **Créer CurriculumManager** (300 lignes)
   - Gestion progression multi-puzzles
   - Sélection puzzle suivant
   - Déblocage niveaux

2. **Créer SharedKnowledgeBase** (250 lignes)
   - Structure mémoire persistante
   - Sauvegarde/chargement
   - API accès connaissances

3. **Créer PuzzleRotationEngine** (200 lignes)
   - Boucle principale rotation
   - Gestion cycles
   - Intégration composants

### Phase 4.6.2 - Apprentissage Humain (Semaine 2)

4. **Analyser Replay Humain** (URL fournie)
   - Extraction données replay
   - Conversion format utilisable
   - Validation données

5. **Créer HumanDemonstrationLearner** (350 lignes)
   - Extraction trajectoires
   - Analyse comportements
   - Identification objectifs implicites

6. **Créer ConceptExtractor** (280 lignes)
   - Détection patterns communs
   - Abstraction concepts
   - Construction bibliothèque

### Phase 4.6.3 - Intégration Multi-Puzzles (Semaine 3)

7. **Intégrer Tous Puzzles ARC Niveau 1**
   - Charger tous puzzles disponibles
   - Créer wrappers uniformes
   - Tester compatibilité

8. **Implémenter Système Révision**
   - Critères révision
   - Planification automatique
   - Intégration rotation

9. **Créer Système Métriques**
   - Collecte données
   - Génération rapports
   - Visualisation progression

### Phase 4.6.4 - Tests et Validation (Semaine 4)

10. **Test Boucle Complète 100 Cycles**
    - Validation rotation
    - Vérification apprentissage
    - Analyse transfert connaissances

11. **Optimisation Performance**
    - Profiling code
    - Optimisation mémoire
    - Parallélisation possible

12. **Documentation et Rapport Final**
    - Documentation code
    - Rapport première victoire
    - Guide utilisation

---

## 🎯 Critères de Succès

### Objectifs Minimaux (Phase 4.6)

- ✅ Rotation fonctionnelle sur 10+ puzzles
- ✅ Mémoire partagée persistante
- ✅ Au moins 1 puzzle maîtrisé (95% succès)
- ✅ Transfert connaissances démontré

### Objectifs Optimaux

- 🎯 Rotation sur 50+ puzzles
- 🎯 5+ puzzles maîtrisés
- 🎯 10+ concepts réutilisables identifiés
- 🎯 Transfert connaissances > 30% puzzles

### Objectif Final (Phase 4.7+)

- 🏆 **Première victoire ARC Prize officielle**
- 🏆 Système généraliste fonctionnel
- 🏆 Publication résultats

---

## 📝 Notes Importantes

### Différences Clés avec Phase 4.5

1. **Mémoire**: Persistante entre puzzles (vs isolée)
2. **Apprentissage**: Par observation humaine (vs exploration pure)
3. **Progression**: Curriculum adaptatif (vs entraînement fixe)
4. **Objectif**: Devenir joueur généraliste (vs résoudre 1 puzzle)

### Ajouts Critiques Identifiés

1. **Curriculum Automatique**: Progression contrôlée niveau par niveau
2. **Révision Permanente**: Éviter l'oubli des puzzles maîtrisés
3. **Détection Concepts**: Identifier briques réutilisables
4. **Base Démonstrations**: Apprendre des humains
5. **Mémoire Inter-Puzzles**: Partage connaissances global

---

## 🔗 Intégration avec Phase 4.5.9

La Phase 4.6 **conserve** toutes les améliorations de la Phase 4.5.9:
- ✅ Architecture 7 couches
- ✅ Corrections exploration (13.4% vs 84%)
- ✅ MetaLearning optimisé
- ✅ Persistance plans

Et **ajoute**:
- 🆕 Rotation multi-puzzles
- 🆕 Apprentissage par observation
- 🆕 Mémoire partagée
- 🆕 Curriculum adaptatif

---

**Auteur**: Bob (MAGEN Project)  
**Date**: 2026-06-20  
**Version**: 4.6.0-PLAN  
**Statut**: Prêt pour implémentation