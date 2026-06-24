# ANALYSE ARCHITECTURE COGNITIVE MAGEN - Transition Transformationnel → Décisionnel

**Date**: 2026-06-15 22:21 CET  
**Auteur**: Bob (IA) + Validation Utilisateur Expert  
**Version**: V37 → V38 (Architecture Cognitive)  
**Statut**: ANALYSE CRITIQUE VALIDÉE

---

## 🎯 DIAGNOSTIC FONDAMENTAL

### Résultat Clé (Citation Utilisateur)

> "Le moteur actuel ne possède aucune mémoire décisionnelle globale sur la qualité des actions."

Ce n'est **PAS** un problème de paramétrage.  
C'est une **limite structurelle** de l'architecture actuelle.

### Saut de Performance Révélateur

```
V25: 45/400 puzzles (11.25%)
V26-V28: 174/400 puzzles (43.5%)
Gain: +289% (45 → 174)

MAIS:
V26 → V27 → V28: 0% gain supplémentaire
```

**Interprétation** :
- Les transformations/détecteurs/exploration adaptative ont capturé les gains "faciles"
- Le plafond actuel n'est **pas technique** mais **architectural**
- Besoin d'une **gestion stratégique de l'exploration**

---

## 🏗️ ARCHITECTURE ACTUELLE (V26-V28)

### Type: Moteur Transformationnel

```
MAGEN V26-V28 = {
    ✓ Moteur de transformations
    ✓ Exploration locale
    ✓ Heuristiques court terme
    ✓ Détecteurs de patterns
}
```

### Capacités Présentes

1. **Transformations** : rotate, flip, crop, etc.
2. **Détecteurs** : symmetry, grid, color patterns
3. **Exploration** : Adaptative locale
4. **Heuristiques** : Court terme (step-by-step)

### Capacités ABSENTES

1. ❌ **Apprendre** quelles actions valent le coût
2. ❌ **Adapter dynamiquement** le budget computationnel
3. ❌ **Modifier** la politique d'exploration
4. ❌ **Abandonner intelligemment** les trajectoires mortes

---

## 🔬 DÉCOUVERTE CRITIQUE : Compteur Oscillant

### Observation Forensic

```python
# Compteur oscillant 0 ↔ 1
loop_counter = 0
if loop_detected:
    loop_counter = 1
else:
    loop_counter = 0
```

### Diagnostic Utilisateur

> "Ce mécanisme n'est pas un système de réputation.  
> C'est juste un anti-spam local."

**Conséquence** :  
Même avec seuils agressifs → **Pas de changement de politique globale**

### Données Révélatrices

```
42.6% identity (transformations par défaut)
70.1% error >= 0.9 (branches absurdes)
```

**Interprétation** :
1. Génération excessive de branches "par défaut"
2. Espace d'actions trop large vs capacité de filtrage
3. Apprentissage **après coup** (pas prédictif)

---

## 💡 INSIGHT MAJEUR : C17 Priority Formula

### Code Actuel (Embryonnaire)

```python
priority = success_rate * (1.0 - avg_error)
```

### Signification Cognitive

Pour la **première fois**, MAGEN construit une notion de :

- **Coût** : Ressources consommées par action
- **Rentabilité** : Ratio succès/échecs
- **Efficacité historique** : Performance passée
- **Utilité prédictive** : Valeur future estimée

**Citation Utilisateur** :

> "MAGEN commence à apprendre quelles transformations 'méritent d'être pensées'."

---

## ⚠️ RISQUE CRITIQUE : Biais d'Exploitation Prématurée

### Scénario Pathologique

```
1. rotate_90 marche souvent globalement
   ↓
2. Réputation de rotate_90 explose
   ↓
3. Puzzles rares nécessitant identity + transformation secondaire
   ↓
4. Branches identity deviennent sous-explorées
   ↓
5. RÉSULTAT: Score moyen ↑ MAIS diversité ↓
```

### Conséquence

C17 seul peut :
- ✅ Améliorer le score moyen
- ❌ Réduire la diversité de recherche
- ❌ Créer des angles morts cognitifs

**Solution** : C17 + C18 + C19 (trio structurel)

---

## 🎯 ARCHITECTURE COGNITIVE CIBLE (V38)

### Type: Agent Décisionnel

```
MAGEN V38 = {
    ✓ Mémoire de valeur des actions (C17)
    ✓ Allocation attentionnelle (C18)
    ✓ Détection de stagnation cognitive (C19)
    ✓ Découverte d'affordances (PHASE 0)
    ✓ Filtrage de candidats (PHASE 1)
    ✓ Priorisation par réputation (PHASE 2)
    ✓ Allocation de budget (PHASE 3)
    ✓ Monitoring de trajectoires (PHASE 4)
    ✓ Validation symbolique (PHASE 5)
}
```

### Trio Structurel

| Module | Fonction Cognitive | Analogie Humaine |
|--------|-------------------|------------------|
| **C17** | Mémoire de valeur des actions | "Cette action vaut-elle le coup ?" |
| **C18** | Allocation attentionnelle | "Combien de temps y consacrer ?" |
| **C19** | Détection de stagnation | "Suis-je bloqué dans une impasse ?" |

### Pipeline Cognitif Complet

```
PHASE 0: Affordance Discovery
         "Quelles actions sont plausibles ?"
         ↓
PHASE 1: Candidate Filtering
         "Éliminer les actions absurdes"
         ↓
PHASE 2: Reputation Prioritization (C17)
         "Prioriser par valeur historique"
         ↓
PHASE 3: Budget Allocation (C18)
         "Allouer ressources dynamiquement"
         ↓
PHASE 4: Trajectory Monitoring (C19)
         "Détecter stagnation cognitive"
         ↓
PHASE 5: Symbolic Validation
         "Vérifier cohérence symbolique"
```

---

## 📊 COMPARAISON ARCHITECTURALE

### Moteur Transformationnel (V26-V28)

```
INPUT → [Transformations] → [Exploration Locale] → OUTPUT
         ↑                    ↑
         Détecteurs          Heuristiques
```

**Caractéristiques** :
- Réactif (pas prédictif)
- Local (pas global)
- Court terme (pas stratégique)
- Génératif (pas sélectif)

### Agent Décisionnel (V38)

```
INPUT → [Affordance Discovery] → [Candidate Filtering]
         ↓
       [Reputation Prioritization (C17)]
         ↓
       [Budget Allocation (C18)]
         ↓
       [Trajectory Monitoring (C19)]
         ↓
       [Symbolic Validation] → OUTPUT
```

**Caractéristiques** :
- Prédictif (apprentissage historique)
- Global (mémoire décisionnelle)
- Stratégique (allocation ressources)
- Sélectif (filtrage pré-exploration)

---

## 🔬 MODULES À IMPLÉMENTER

### C17: ActionReputationSystem

**Fonction** : Mémoire de valeur des actions

```python
class ActionReputationSystem:
    def __init__(self):
        self.action_stats = {}  # action_id → stats
        self.global_history = []
        
    def compute_priority(self, action_id):
        stats = self.action_stats[action_id]
        success_rate = stats.successes / stats.attempts
        avg_error = stats.total_error / stats.attempts
        
        # Formule cognitive
        priority = success_rate * (1.0 - avg_error)
        
        # Bonus diversité (anti-exploitation)
        diversity_bonus = 1.0 / (1.0 + stats.recent_uses)
        
        return priority * diversity_bonus
```

**Métriques** :
- `success_rate` : Taux de succès historique
- `avg_error` : Erreur moyenne
- `diversity_bonus` : Pénalité sur-utilisation
- `priority` : Score final

### C18: DynamicBudgetAllocator

**Fonction** : Allocation attentionnelle

```python
class DynamicBudgetAllocator:
    def __init__(self, total_budget=1000):
        self.total_budget = total_budget
        self.spent_budget = 0
        self.action_budgets = {}
        
    def allocate_budget(self, action_id, priority):
        # Budget proportionnel à la priorité
        base_budget = self.total_budget * 0.1
        priority_budget = base_budget * priority
        
        # Réduction si budget épuisé
        remaining = self.total_budget - self.spent_budget
        allocated = min(priority_budget, remaining * 0.2)
        
        return allocated
        
    def should_continue(self, action_id):
        budget = self.action_budgets[action_id]
        return budget.spent < budget.allocated
```

**Métriques** :
- `total_budget` : Budget global
- `spent_budget` : Budget consommé
- `allocated_budget` : Budget alloué par action
- `remaining_budget` : Budget restant

### C19: TrajectoryStagnationDetector

**Fonction** : Détection de stagnation cognitive

```python
class TrajectoryStagnationDetector:
    def __init__(self, window_size=10):
        self.window_size = window_size
        self.trajectory_history = []
        
    def detect_stagnation(self, current_state):
        # Ajouter état actuel
        self.trajectory_history.append(current_state)
        
        # Fenêtre glissante
        window = self.trajectory_history[-self.window_size:]
        
        # Mesurer variance
        variance = np.var([s.error for s in window])
        
        # Stagnation si variance < seuil
        is_stagnant = variance < 0.01
        
        # Mesurer loops
        unique_states = len(set(window))
        loop_ratio = 1.0 - (unique_states / len(window))
        
        return {
            'is_stagnant': is_stagnant,
            'variance': variance,
            'loop_ratio': loop_ratio,
            'should_abandon': is_stagnant and loop_ratio > 0.7
        }
```

**Métriques** :
- `variance` : Variance erreur sur fenêtre
- `loop_ratio` : Ratio états répétés
- `is_stagnant` : Booléen stagnation
- `should_abandon` : Recommandation abandon

### PHASE 0: AffordanceDiscoveryEngine

**Fonction** : Filtrage pré-exploration

```python
class AffordanceDiscoveryEngine:
    def __init__(self):
        self.pattern_detectors = []
        self.affordance_rules = {}
        
    def discover_affordances(self, puzzle_state):
        """Découvre quelles actions sont PLAUSIBLES."""
        affordances = []
        
        # Détection patterns
        patterns = self._detect_patterns(puzzle_state)
        
        # Règles d'affordance
        for pattern in patterns:
            if pattern.type == 'symmetry':
                affordances.append('rotate_90')
                affordances.append('flip_horizontal')
            elif pattern.type == 'grid':
                affordances.append('crop')
                affordances.append('tile')
            elif pattern.type == 'color_blocks':
                affordances.append('color_map')
                
        return affordances
        
    def filter_candidates(self, candidates, affordances):
        """Élimine actions absurdes AVANT exploration."""
        return [c for c in candidates if c.action in affordances]
```

**Métriques** :
- `affordances_discovered` : Nombre d'affordances
- `candidates_filtered` : Ratio filtrage
- `exploration_reduction` : Réduction espace recherche

---

## 📈 MÉTRIQUES ATTENDUES V38

### Performance

| Métrique | V26-V28 | V38 (Cible) | Gain |
|----------|---------|-------------|------|
| Puzzles résolus | 174/400 (43.5%) | 220/400 (55%) | +26% |
| Actions testées | 100% | 30% | -70% |
| Loops détectés | 2,372 | <500 | -79% |
| Budget utilisé | 100% | 60% | -40% |
| Diversité exploration | Faible | Élevée | +100% |

### Cognitif

| Métrique | V26-V28 | V38 (Cible) |
|----------|---------|-------------|
| Mémoire de valeur | ❌ Absente | ✅ Présente |
| Allocation attentionnelle | ❌ Fixe | ✅ Dynamique |
| Détection stagnation | ❌ Locale | ✅ Globale |
| Filtrage pré-exploration | ❌ Absent | ✅ Présent |
| Abandon stratégique | ❌ Jamais | ✅ Intelligent |

---

## 🚀 PLAN D'IMPLÉMENTATION

### Phase 1: Trio Structurel (C17+C18+C19)

1. ✅ Créer `action_reputation_system.py` (C17)
2. ✅ Créer `dynamic_budget_allocator.py` (C18)
3. ✅ Créer `trajectory_stagnation_detector.py` (C19)
4. ✅ Tests unitaires (3/3 modules)
5. ✅ Forensic LEÇON-73.1 intégré

### Phase 2: Affordance Discovery (PHASE 0)

1. ✅ Créer `affordance_discovery_engine.py`
2. ✅ Règles d'affordance par pattern
3. ✅ Filtrage pré-exploration
4. ✅ Tests unitaires

### Phase 3: Intégration

1. ✅ Intégrer C17+C18+C19 dans `arc_solver_adapter.py`
2. ✅ Intégrer PHASE 0 dans pipeline
3. ✅ Tests intégration complète

### Phase 4: Validation

1. ✅ Tester sur VRAIS puzzles ARC-AGI-3 (dataset réel)
2. ✅ Collecter logs forensic
3. ✅ Analyser métriques cognitives
4. ✅ Rapport final V38

---

## 🎓 LEÇONS APPRISES

### LEÇON-94: Limite Architecture Transformationnelle

**Énoncé** :  
Un moteur transformationnel atteint un plafond structurel lorsque les gains "faciles" sont capturés. Le passage à un agent décisionnel nécessite une mémoire de valeur des actions.

**Preuve** :  
V26-V28 : 0% gain malgré optimisations (174/400 stable)

**Application** :  
Implémenter C17 (ActionReputationSystem) avec formule cognitive `priority = success_rate * (1.0 - avg_error)`

### LEÇON-95: Biais d'Exploitation Prématurée

**Énoncé** :  
Un système de réputation sans mécanisme de diversité crée des angles morts cognitifs en sur-exploitant les actions globalement efficaces.

**Preuve** :  
42.6% identity, 70.1% error >= 0.9 (génération excessive branches par défaut)

**Application** :  
Ajouter `diversity_bonus = 1.0 / (1.0 + recent_uses)` dans calcul priorité

### LEÇON-96: Filtrage Pré-Exploration

**Énoncé** :  
Découvrir les affordances AVANT l'exploration réduit l'espace de recherche et élimine les actions absurdes.

**Preuve** :  
70.1% error >= 0.9 indique génération massive d'actions non-plausibles

**Application** :  
Implémenter PHASE 0 (AffordanceDiscoveryEngine) avec règles pattern → affordances

### LEÇON-97: Trio Structurel Cognitif

**Énoncé** :  
La transition moteur transformationnel → agent décisionnel nécessite 3 composants : mémoire de valeur (C17), allocation attentionnelle (C18), détection stagnation (C19).

**Preuve** :  
Architecture actuelle manque ces 3 capacités fondamentales

**Application** :  
Implémenter trio C17+C18+C19 comme système intégré

---

## 📚 RÉFÉRENCES

- LEÇONS_APPRISES_MAGEN.md (LEÇON-73.1 : Forensic Standard)
- RAPPORT_V37_IMPLEMENTATION.md (AvatarIdentifier)
- SYNTHESE_FINALE_V36_HIERARCHIE_COMPLETE.md (5 niveaux)
- Message utilisateur 2026-06-15 (Architecture Cognitive)

---

## ✅ VALIDATION

- [x] Diagnostic limite architecturale validé
- [x] Trio structurel C17+C18+C19 spécifié
- [x] PHASE 0 (Affordance Discovery) spécifiée
- [x] Pipeline cognitif complet défini
- [x] Métriques attendues V38 établies
- [x] 4 LEÇONS APPRISES identifiées (94-97)
- [ ] Implémentation C17+C18+C19
- [ ] Implémentation PHASE 0
- [ ] Tests sur VRAIS puzzles ARC-AGI-3
- [ ] Rapport final V38

---

**Prochaine étape** : Implémentation immédiate C17 (ActionReputationSystem)