# PLAN: Intégration V35+V36 avec Solver ARC et Tests Arcade Réels

**Status**: PENDING
**Created**: 2026-06-15T19:08:00Z
**Priority**: CRITICAL
**Estimated Effort**: 4-6 hours

---

## 🎯 PROBLEM (What & Why)

### What
Intégrer les modules V35 (métacognition) et V36 (world model) avec le solver ARC existant, puis tester sur vrais puzzles ARC ET jeux Arcade réels pour valider l'apprentissage sur les deux types.

### Why
- V35+V36 sont des avancées architecturales majeures mais non testées sur vrais problèmes
- Besoin de métriques réelles (pas seulement tests unitaires)
- Utilisateur demande EXPLICITEMENT tests sur jeux Arcade RÉELS
- Validation que le système apprend sur les deux types de problèmes
- Identifier limites réelles avant scale-up 400 puzzles

### Current State
- ✅ V35 (métacognition) : 3 modules avec forensic 100% conforme
- ✅ V36 (world model) : 4 composants avec forensic 100% conforme
- ✅ Tests unitaires : 100% succès
- ❌ Pas de tests sur vrais puzzles ARC
- ❌ Pas de tests sur jeux Arcade réels
- ❌ Warnings non corrigés dans test_world_model_persistent.py
- ❌ Forensic pas ajouté partout (LEÇON-73.1)

---

## 📋 REQUIREMENTS (Success Criteria)

### SC1: Corrections Warnings et Forensic
**MUST HAVE**
- [ ] Corriger warning test StrategicMemory (zones prometteuses)
- [ ] Ajouter forensic standard LEÇON-73.1 dans tous les nouveaux modules
- [ ] Validation : 0 warnings, forensic 100% conforme

### SC2: Adaptateur Solver ARC
**MUST HAVE**
- [ ] Créer `arc_solver_adapter.py` qui connecte V35+V36 au solver existant
- [ ] Intégrer MetaCognitiveController pour décisions stratégiques
- [ ] Intégrer WorldModelPersistent pour représentation spatiale
- [ ] Forensic complet sur toutes opérations
- [ ] Validation : Tests unitaires passent

### SC3: Tests Puzzles ARC Réels
**MUST HAVE**
- [ ] Créer `test_arc_real_puzzles.py` pour 10 puzzles ARC
- [ ] Collecter métriques : temps, steps, cohérence, régions, loops
- [ ] Générer logs forensic pour chaque puzzle
- [ ] Validation : 10/10 puzzles complétés, métriques collectées

### SC4: Tests Jeux Arcade RÉELS
**CRITICAL - DEMANDE EXPLICITE UTILISATEUR**
- [ ] Identifier jeux Arcade ARC-AGI-3 disponibles
- [ ] Créer `test_arcade_games.py` pour jeux réels
- [ ] Tester apprentissage sur les deux types (ARC + Arcade)
- [ ] Métriques comparatives : performance ARC vs Arcade
- [ ] Validation : Tests sur au moins 3 jeux Arcade réels

### SC5: Rapport Analyse Complète
**MUST HAVE**
- [ ] Analyser logs forensic réels générés
- [ ] Identifier patterns, limites, opportunités
- [ ] Comparer performance ARC vs Arcade
- [ ] Recommandations pour PHASE 0-2 MDBAI/PILOT
- [ ] Validation : Rapport 400+ lignes avec insights actionnables

---

## 🔧 PLAN (How - Step by Step)

### Phase 1: Corrections et Forensic (30 min)
**Objectif**: Code propre, 0 warnings, forensic partout

1. **Corriger test StrategicMemory**
   - Ajuster test pour auto-generation zones prometteuses
   - Valider frontière exploration fonctionne

2. **Audit forensic complet**
   - Vérifier tous modules V35+V36 ont forensic LEÇON-73.1
   - Ajouter forensic manquant si nécessaire
   - Valider chaîne hash intégrité

3. **Validation**
   - Exécuter tous tests : 100% succès
   - Vérifier logs forensic : 100% conforme

### Phase 2: Adaptateur Solver ARC (1h)
**Objectif**: Connecter V35+V36 au solver existant

1. **Créer arc_solver_adapter.py**
   ```python
   class ARCSolverAdapter:
       def __init__(self):
           self.metacog = MetaCognitiveController()
           self.world_model = WorldModelPersistent()
           self.forensic = ForensicLogger()
       
       def solve_puzzle(self, puzzle):
           # Intégration V35+V36
           pass
   ```

2. **Intégrer métacognition**
   - Utiliser MetaCognitiveController pour décisions
   - Tracker utilité trajectoires
   - Détecter stagnation et loops

3. **Intégrer world model**
   - Utiliser WorldModelPersistent pour carte
   - Tracker régions explorées/prometteuses/mortes
   - Maintenir cohérence monde

4. **Forensic complet**
   - Logger toutes décisions stratégiques
   - Logger tous updates world model
   - Chaîne hash intégrité

5. **Tests unitaires**
   - Créer test_arc_solver_adapter.py
   - Valider intégration fonctionne

### Phase 3: Tests Puzzles ARC Réels (1h)
**Objectif**: Métriques réelles sur 10 puzzles

1. **Créer test_arc_real_puzzles.py**
   - Charger 10 puzzles ARC variés
   - Exécuter avec adaptateur V35+V36
   - Collecter métriques détaillées

2. **Métriques à collecter**
   - Temps résolution
   - Steps totaux
   - Régions découvertes
   - Loops détectés
   - Cohérence finale
   - Zones explorées/prometteuses/mortes

3. **Logs forensic**
   - Générer logs pour chaque puzzle
   - Format JSONL standard

4. **Validation**
   - 10/10 puzzles complétés
   - Logs forensic générés
   - Métriques collectées

### Phase 4: Tests Jeux Arcade RÉELS (1.5h)
**Objectif**: Valider apprentissage sur Arcade

1. **Identifier jeux Arcade disponibles**
   - Chercher jeux ARC-AGI-3
   - Sélectionner 3-5 jeux représentatifs

2. **Créer test_arcade_games.py**
   - Adaptateur pour jeux Arcade
   - Même métriques que puzzles ARC
   - Forensic complet

3. **Exécuter tests**
   - Tester sur 3-5 jeux Arcade réels
   - Collecter métriques
   - Générer logs forensic

4. **Analyse comparative**
   - Comparer performance ARC vs Arcade
   - Identifier différences comportementales
   - Valider apprentissage sur les deux types

### Phase 5: Analyse et Rapport (1h)
**Objectif**: Insights actionnables

1. **Analyser logs forensic**
   - Lire tous logs générés
   - Identifier patterns récurrents
   - Détecter limites architecturales

2. **Métriques comparatives**
   - ARC vs Arcade : temps, steps, cohérence
   - Efficacité exploration
   - Qualité décisions métacognitives

3. **Recommandations**
   - Priorités pour PHASE 0-2 MDBAI/PILOT
   - Améliorations V36.2
   - Optimisations avant 400 puzzles

4. **Créer rapport**
   - RAPPORT_TESTS_ARC_ARCADE_REELS.md
   - 400+ lignes
   - Insights actionnables

---

## 🎯 VALIDATION CRITERIA

### Definition of Done
- [ ] 0 warnings dans tous les tests
- [ ] Forensic 100% conforme LEÇON-73.1 partout
- [ ] Adaptateur V35+V36 créé et testé
- [ ] 10 puzzles ARC réels testés avec métriques
- [ ] 3+ jeux Arcade réels testés avec métriques
- [ ] Logs forensic générés pour tous tests
- [ ] Rapport analyse 400+ lignes créé
- [ ] Recommandations claires pour suite

### Quality Gates
- Tests unitaires : 100% succès
- Forensic : 100% conforme
- Métriques : Collectées pour tous tests
- Logs : Format JSONL standard
- Rapport : Insights actionnables identifiés

---

## 📊 DEPENDENCIES

### Required Files
- `lumvorax2/src/MAGEN/core/metacognitive_controller.py` (V35)
- `lumvorax2/src/MAGEN/core/world_model_persistent.py` (V36)
- `lumvorax2/src/MAGEN/core/forensic_logger.py`
- Solver ARC existant (à identifier)
- Jeux Arcade ARC-AGI-3 (à identifier)

### External Dependencies
- Aucune nouvelle dépendance Python requise
- Utilise infrastructure existante

---

## 🚨 RISKS & MITIGATION

### Risk 1: Jeux Arcade introuvables
**Impact**: HIGH
**Mitigation**: Utiliser simulateur ou créer jeux simples si nécessaire

### Risk 2: Performance dégradée avec V35+V36
**Impact**: MEDIUM
**Mitigation**: Profiling et optimisation si nécessaire

### Risk 3: Forensic overhead trop élevé
**Impact**: LOW
**Mitigation**: Forensic déjà optimisé, overhead < 1ms

---

## 📝 NOTES

### Leçons Apprises Pertinentes
- **LEÇON-73.1**: Forensic standard obligatoire
- **LEÇON-82**: Forensic ≠ Intelligence
- **LEÇON-83**: Cohérence stable = stagnation
- **LEÇON-84**: Auto-generation zones prometteuses

### MDBAI/PILOT Workflow
Ce plan suit le pattern PRP :
- **Problem**: Clairement défini
- **Requirements**: Success Criteria mesurables
- **Plan**: Steps détaillés avec validation

Prochaine étape : `/01_confirm` pour approuver le plan