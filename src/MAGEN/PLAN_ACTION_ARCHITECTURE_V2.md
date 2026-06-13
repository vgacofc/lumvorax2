# PLAN D'ACTION - ARCHITECTURE MAGEN V2
## Pivot vers Program Synthesis Engine

**Date**: 2026-06-13 00:20:00 UTC  
**Expertise**: Neuro-Symbolic AI, Program Synthesis, ARC-AGI Systems

---

## 🎯 OBJECTIF PRINCIPAL

Transformer MAGEN d'un **"heuristic solver"** vers un **"program synthesis engine"**

---

## 📊 ÉTAT ACTUEL (Session 26)

### Métriques Réelles
- **Succès ARC**: 11/400 (2.75%)
- **Pipeline traités**: 400/400 (100%)
- **Heuristiques**: 13 (H1-H13)
- **Temps moyen**: 0.058s/puzzle

### Problèmes Identifiés
1. ❌ **Plateau heuristique**: +6 heuristiques (H8-H13) = 0 gain
2. ❌ **Overfitting local**: Heuristiques puzzle-spécifiques
3. ❌ **Absence de généralisation**: Pas de composition automatique
4. ❌ **Explosion combinatoire imminente**: 13 heuristiques → 400+ si on continue
5. ❌ **Régressions**: Corrections cassent anciens succès

---

## 🚀 ARCHITECTURE CIBLE V2

### Transformation Fondamentale

```text
AVANT (V1):
Input → Heuristiques spécifiques → Output

APRÈS (V2):
Input → DSL Primitives → Program Synthesis → Search → Output
```

---

## 📋 PRIORITÉS ABSOLUES

### PRIORITÉ 1: Transformation DSL (CRITIQUE)

**Objectif**: Remplacer heuristiques par primitives composables

**Primitives Universelles** (20-30 opérateurs):
```python
# Géométriques
rotate90, rotate180, rotate270
mirror_h, mirror_v
transpose
scale(factor)

# Spatiales
crop(bbox)
translate(dx, dy)
extract_object(id)
move_object(id, target)

# Couleurs
recolor(from, to)
floodfill(pos, color)
propagate_color(rule)

# Objets
detect_objects()
filter_objects(predicate)
connect_objects(rule)
group_objects(similarity)

# Patterns
repeat(pattern, n)
tile(pattern, grid)
symmetrize(axis)

# Logique
if_then_else(condition, true_branch, false_branch)
for_each(collection, operation)
compose(op1, op2, ...)
```

**Implémentation**:
1. Créer [`dsl/primitives.py`](lumvorax2/src/MAGEN/dsl/primitives.py:1)
2. Créer [`dsl/composer.py`](lumvorax2/src/MAGEN/dsl/composer.py:1)
3. Créer [`dsl/program.py`](lumvorax2/src/MAGEN/dsl/program.py:1)

---

### PRIORITÉ 2: Program Synthesis Engine

**Objectif**: Générer automatiquement programmes à partir de train pairs

**Architecture**:
```python
class ProgramSynthesizer:
    def synthesize(self, train_pairs):
        # 1. Générer hypothèses de programmes
        candidates = self.generate_candidates(train_pairs)
        
        # 2. Scorer chaque programme
        scored = self.score_programs(candidates, train_pairs)
        
        # 3. Beam search
        best = self.beam_search(scored, beam_width=10)
        
        # 4. Validation
        return self.validate(best, train_pairs)
```

**Implémentation**:
1. Créer [`synthesis/synthesizer.py`](lumvorax2/src/MAGEN/synthesis/synthesizer.py:1)
2. Créer [`synthesis/scorer.py`](lumvorax2/src/MAGEN/synthesis/scorer.py:1)
3. Créer [`synthesis/validator.py`](lumvorax2/src/MAGEN/synthesis/validator.py:1)

---

### PRIORITÉ 3: Beam Search Symbolique

**Objectif**: Explorer espace de programmes efficacement

**Algorithme**:
```python
class BeamSearch:
    def search(self, initial_state, beam_width=10, max_depth=5):
        beam = [initial_state]
        
        for depth in range(max_depth):
            # Générer successeurs
            successors = []
            for state in beam:
                successors.extend(self.expand(state))
            
            # Scorer et filtrer
            scored = self.score_states(successors)
            beam = self.select_top_k(scored, beam_width)
            
            # Vérifier solutions
            solutions = [s for s in beam if self.is_solution(s)]
            if solutions:
                return self.best_solution(solutions)
        
        return None
```

**Implémentation**:
1. Créer [`search/beam_search.py`](lumvorax2/src/MAGEN/search/beam_search.py:1)
2. Créer [`search/state.py`](lumvorax2/src/MAGEN/search/state.py:1)
3. Créer [`search/scorer.py`](lumvorax2/src/MAGEN/search/scorer.py:1)

---

### PRIORITÉ 4: Anti-Regression Framework

**Objectif**: Empêcher régressions lors de modifications

**Système**:
```python
class RegressionTester:
    def __init__(self):
        self.baseline_results = self.load_baseline()
    
    def test_modification(self, new_code):
        # 1. Exécuter sur 400 puzzles
        new_results = self.run_full_suite(new_code)
        
        # 2. Comparer avec baseline
        regressions = self.detect_regressions(
            self.baseline_results,
            new_results
        )
        
        # 3. Rapport
        if regressions:
            self.report_regressions(regressions)
            return False
        
        # 4. Mettre à jour baseline si amélioration
        if new_results.score > self.baseline_results.score:
            self.update_baseline(new_results)
        
        return True
```

**Implémentation**:
1. Créer [`testing/regression_tester.py`](lumvorax2/src/MAGEN/testing/regression_tester.py:1)
2. Créer [`testing/baseline_manager.py`](lumvorax2/src/MAGEN/testing/baseline_manager.py:1)
3. Sauvegarder baseline Session 26 (11/400)

---

### PRIORITÉ 5: Failure Clustering

**Objectif**: Classifier échecs pour optimisation ciblée

**Analyse**:
```python
class FailureAnalyzer:
    def cluster_failures(self, failures):
        clusters = {
            'downscale': [],
            'upscale': [],
            'symmetry': [],
            'color_propagation': [],
            'object_manipulation': [],
            'pattern_repetition': [],
            'conditional_logic': [],
            'recursion': [],
            'topology': [],
            'other': []
        }
        
        for failure in failures:
            cluster = self.classify(failure)
            clusters[cluster].append(failure)
        
        return clusters
```

**Implémentation**:
1. Créer [`analysis/failure_analyzer.py`](lumvorax2/src/MAGEN/analysis/failure_analyzer.py:1)
2. Analyser 389 échecs Session 26
3. Générer rapport clusters

---

### PRIORITÉ 6: Constraint Engine

**Objectif**: Élimination précoce hypothèses invalides

**Contraintes ARC**:
```python
class ConstraintEngine:
    def check_constraints(self, program, train_pairs):
        constraints = [
            self.color_conservation,
            self.object_count_consistency,
            self.topology_preservation,
            self.symmetry_consistency,
            self.size_constraints,
            self.adjacency_rules
        ]
        
        for constraint in constraints:
            if not constraint(program, train_pairs):
                return False
        
        return True
```

**Implémentation**:
1. Créer [`constraints/engine.py`](lumvorax2/src/MAGEN/constraints/engine.py:1)
2. Créer [`constraints/rules.py`](lumvorax2/src/MAGEN/constraints/rules.py:1)

---

### PRIORITÉ 7: Meta-Scoring System

**Objectif**: Scorer programmes par qualité structurelle

**Métriques**:
```python
class ProgramScorer:
    def score(self, program, train_pairs):
        scores = {
            'correctness': self.correctness_score(program, train_pairs),
            'simplicity': self.mdl_score(program),  # Minimum Description Length
            'consistency': self.consistency_score(program, train_pairs),
            'generality': self.generality_score(program),
            'confidence': self.confidence_score(program, train_pairs)
        }
        
        # Weighted combination
        return (
            scores['correctness'] * 0.4 +
            scores['simplicity'] * 0.2 +
            scores['consistency'] * 0.2 +
            scores['generality'] * 0.1 +
            scores['confidence'] * 0.1
        )
```

**Implémentation**:
1. Créer [`scoring/program_scorer.py`](lumvorax2/src/MAGEN/scoring/program_scorer.py:1)
2. Créer [`scoring/mdl.py`](lumvorax2/src/MAGEN/scoring/mdl.py:1)

---

## 📅 ROADMAP IMPLÉMENTATION

### Phase 1: Fondations DSL (Semaine 1)
- [ ] Créer module `dsl/` avec 20 primitives
- [ ] Implémenter composer
- [ ] Tests unitaires primitives
- [ ] Validation sur 11 succès actuels

### Phase 2: Program Synthesis (Semaine 2)
- [ ] Créer module `synthesis/`
- [ ] Implémenter générateur candidats
- [ ] Implémenter scorer basique
- [ ] Tests sur 5 puzzles simples

### Phase 3: Search Engine (Semaine 3)
- [ ] Créer module `search/`
- [ ] Implémenter beam search
- [ ] Implémenter pruning
- [ ] Validation sur 20 puzzles

### Phase 4: Anti-Regression (Semaine 4)
- [ ] Créer module `testing/`
- [ ] Sauvegarder baseline
- [ ] Implémenter détection régressions
- [ ] CI/CD automatique

### Phase 5: Optimisation (Semaine 5-8)
- [ ] Failure clustering
- [ ] Constraint engine
- [ ] Meta-scoring
- [ ] Optimisation performance

---

## 🎯 OBJECTIFS MESURABLES

### Court Terme (1 mois)
- **Objectif**: 20-30/400 (5-7.5%)
- **Méthode**: DSL + Synthesis basique
- **Validation**: Aucune régression sur 11 succès actuels

### Moyen Terme (3 mois)
- **Objectif**: 60-100/400 (15-25%)
- **Méthode**: Beam search + Constraints
- **Validation**: Généralisation sur nouveaux puzzles

### Long Terme (6 mois)
- **Objectif**: 150-200/400 (37.5-50%)
- **Méthode**: Architecture complète V2
- **Validation**: Robustesse et stabilité

---

## ⚠️ RISQUES IDENTIFIÉS

### Risque 1: Explosion Combinatoire
**Mitigation**: Pruning agressif + Constraints

### Risque 2: Temps d'Exécution
**Mitigation**: Beam width adaptatif + Caching

### Risque 3: Overfitting DSL
**Mitigation**: Primitives génériques + Validation croisée

### Risque 4: Régressions
**Mitigation**: Framework anti-régression obligatoire

---

## 📊 MÉTRIQUES DE SUCCÈS

### Métriques Primaires
1. **Score ARC**: Succès exacts / 400
2. **Généralisation**: Score sur puzzles non vus
3. **Stabilité**: Absence de régressions

### Métriques Secondaires
1. **Temps d'exécution**: < 1s par puzzle
2. **Complexité programmes**: MDL moyen
3. **Taux de réussite synthesis**: Candidats valides / Total

---

## 🔄 MIGRATION V1 → V2

### Étape 1: Coexistence
- Garder V1 fonctionnel
- Développer V2 en parallèle
- Comparer résultats

### Étape 2: Validation
- V2 doit égaler V1 (11/400)
- Puis dépasser V1
- Sans régressions

### Étape 3: Transition
- Remplacer progressivement heuristiques par DSL
- Migrer pattern_matcher vers synthesizer
- Archiver code V1

---

## 📝 PROCHAINES ACTIONS IMMÉDIATES

### Action 1: Créer Structure DSL
```bash
mkdir -p lumvorax2/src/MAGEN/dsl
mkdir -p lumvorax2/src/MAGEN/synthesis
mkdir -p lumvorax2/src/MAGEN/search
mkdir -p lumvorax2/src/MAGEN/testing
mkdir -p lumvorax2/src/MAGEN/analysis
```

### Action 2: Implémenter 5 Primitives Basiques
- rotate90
- mirror_h
- crop
- recolor
- floodfill

### Action 3: Tester sur 3 Puzzles Simples
- Valider que DSL peut reproduire succès actuels
- Mesurer temps d'exécution
- Vérifier exactitude

### Action 4: Créer Baseline Regression
- Sauvegarder résultats Session 26
- Créer suite de tests automatique
- Documenter 11 succès actuels

---

**Rapport créé**: 2026-06-13 00:20:00 UTC  
**Prochaine session**: Implémentation DSL Primitives  
**Objectif Phase 1**: 20-30/400 (5-7.5%) avec architecture V2