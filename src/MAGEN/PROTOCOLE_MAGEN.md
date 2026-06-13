# PROTOCOLE MAGEN - MÉMOIRE ARTIFICIELLE GEN

**Date création**: 2026-06-13
**Dernière mise à jour**: 2026-06-13T23:33:00+02:00
**Version**: 1.0.0
**Avancement global**: 🟡 **43.5%** — Score 174/400 ✅ | V5→V29: +287% performance | Plateau V26-V29
**Objectif**: Système cognitif auto-optimisant pour résolution puzzles ARC-AGI
**Principe**: Closed-Loop Self-Optimization + Forensic Maximal LumVorax + Mémoire Causale Persistante
**Architecture**: Agent réflexif avec boucle d'apprentissage fermée

> **État 2026-06-13** : V29 implémentée (C17+C18+C19) mais non activée. Diagnostic complet effectué.
> **Tests**: V26-V29 score stable 174/400 (43.5%)
> **Cycle actuel**: SESSION 65 — Diagnostic intégration + Préparation V30 auto-optimisante

---

## 🎯 VISION DU PROJET

### Objectif Principal

Créer un système cognitif auto-optimisant qui:
1. **Exécute** des stratégies de résolution de puzzles ARC-AGI
2. **Trace** exhaustivement chaque action, décision, résultat (forensic bit-level)
3. **Analyse** ses propres logs pour identifier patterns d'échec/succès
4. **Apprend** en modifiant sa politique décisionnelle de manière persistante
5. **S'améliore** automatiquement à chaque cycle d'exécution
6. **Converge** vers stratégies optimales par feedback-driven adaptation

### Contrainte Absolue

**MODE: 100% LOCAL** - Aucune soumission Kaggle avant validation locale complète:
- Tests locaux exhaustifs (400 puzzles)
- Validation forensique des améliorations
- Confirmation gains performance ≥+1 puzzle
- Analyse causale des changements

---

## 📚 LECTURE OBLIGATOIRE AVANT CHAQUE SESSION

### Fichiers à Lire Systématiquement

**RÈGLE ABSOLUE**: Avant de commencer toute tâche MAGEN, lire dans l'ordre :

1. **[LEÇONS_APPRISES_MAGEN.md](LEÇONS_APPRISES_MAGEN.md)** ⭐ CRITIQUE
   - 62+ sessions forensiques documentées
   - Leçons identifiées V1→V29
   - Éviter répétition erreurs passées
   - Comprendre décisions architecture

2. **[PROTOCOLE_MAGEN.md](PROTOCOLE_MAGEN.md)** (ce fichier)
   - Protocole exécution obligatoire
   - Règles forensiques LumVorax
   - Standards rapports horodatés

3. **Dernier rapport de session** (ex: RAPPORT_ANALYSE_INTEGRATION_V29.md)
   - Contexte session précédente
   - Bugs/problèmes en cours
   - Prochaines actions planifiées

### Pourquoi Cette Lecture Est Critique

- **Éviter régression**: Ne pas recréer bugs déjà résolus
- **Cohérence**: Respecter conventions établies (CLAUDE_PILOT + LUMVORAX)
- **Efficacité**: Comprendre architecture sans redécouvrir
- **Qualité**: Appliquer leçons apprises immédiatement
- **Continuité**: Maintenir mémoire artificielle entre sessions

---

## 🔄 ARCHITECTURE CLOSED-LOOP SELF-OPTIMIZATION

### Principe Fondamental

MAGEN n'est PAS un simple système d'exécution + logging.
MAGEN EST un **agent réflexif auto-optimisant** avec boucle fermée:

```
┌─────────────────────────────────────────────────────────────┐
│                    BOUCLE FERMÉE MAGEN                       │
│                                                              │
│  1. EXÉCUTION                                               │
│     ├─ Stratégies de résolution (TLE, Pattern Matcher...)  │
│     ├─ Actions atomiques (rotate, mirror, identity...)     │
│     └─ Décisions guidées par politique actuelle            │
│                          ↓                                   │
│  2. TRACE EXHAUSTIVE (Forensic LumVorax)                   │
│     ├─ Chaque action + contexte + résultat                 │
│     ├─ Timestamps nanoseconde                              │
│     ├─ État avant/après                                     │
│     └─ Causal chains                                        │
│                          ↓                                   │
│  3. ANALYSE POST-RUN                                        │
│     ├─ Identification patterns échec/succès                │
│     ├─ Attribution scores actions                          │
│     ├─ Détection stagnation/divergence                     │
│     └─ Causal inference                                     │
│                          ↓                                   │
│  4. MISE À JOUR POLITIQUE                                   │
│     ├─ Modification réputation actions (C17)               │
│     ├─ Réallocation budget exploration (C18)               │
│     ├─ Ajustement critères early stopping (C19)           │
│     └─ Persistance mémoire cross-puzzle                    │
│                          ↓                                   │
│  5. NOUVELLE EXÉCUTION (avec politique modifiée)           │
│     └─ Retour à étape 1 avec apprentissages intégrés      │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### Ce Qui Différencie MAGEN d'un Système Classique

| Aspect | Système Classique | MAGEN (Closed-Loop) |
|--------|-------------------|---------------------|
| Exécution | Script linéaire | Agent décisionnel |
| Logging | Audit humain | Dataset apprentissage |
| Échec | Retry identique | Modification stratégie |
| Mémoire | Volatile (par run) | Persistante (cross-run) |
| Amélioration | Manuelle (dev) | Automatique (agent) |
| Convergence | Aucune | Vers optimum local |

---

## 📊 EXPERTISES MOBILISÉES

### Expertises Techniques Identifiées

En tant qu'expert MAGEN, je mobilise simultanément:

1. **🧠 Cognitive Architecture**
   - Systèmes multi-agents
   - Reinforcement Learning
   - Meta-learning
   - Causal inference

2. **🔬 Forensic Analysis**
   - Bit-level tracing (LumVorax)
   - Event sourcing
   - Causal chain reconstruction
   - Performance profiling nanoseconde

3. **📈 Machine Learning**
   - Action reputation systems
   - Exploration/exploitation trade-off
   - Trajectory analysis
   - Policy gradient methods

4. **🏗️ Software Architecture**
   - Modular design (12,200+ lignes)
   - Dependency injection
   - Observer pattern
   - Strategy pattern

5. **🔍 Debugging Expert**
   - Root cause analysis
   - Integration debugging
   - Flow tracing
   - Regression detection

6. **📝 Scientific Documentation**
   - Rapports forensiques horodatés
   - Leçons apprises structurées
   - Métriques quantitatives
   - Reproductibilité

---

## 🛠️ RÈGLES D'EXÉCUTION OBLIGATOIRES

### RÈGLE 1: Mode 100% Local

**INTERDICTION ABSOLUE** de soumettre sur Kaggle avant:
- ✅ Tests locaux 400 puzzles validés
- ✅ Analyse forensique complète
- ✅ Gain performance confirmé (≥+1 puzzle)
- ✅ Validation utilisateur explicite

### RÈGLE 2: Forensic Maximal

**OBLIGATION** de logger exhaustivement:
- Chaque action avec timestamp nanoseconde
- Contexte complet (état avant/après)
- Résultat (succès/échec + erreur)
- Causal chain
- Métriques performance

### RÈGLE 3: Analyse Avant Action

**INTERDICTION** de modifier code sans:
1. Lire logs forensiques existants
2. Identifier cause racine précise
3. Documenter hypothèse correction
4. Valider avec tests locaux

### RÈGLE 4: Rapports Horodatés

**OBLIGATION** de créer rapport après chaque:
- Session de développement
- Test complet (400 puzzles)
- Découverte de bug/problème
- Implémentation de correction

Format: `RAPPORT_<TYPE>_<VERSION>_<TIMESTAMP>.md`

### RÈGLE 5: Mise à Jour Leçons Apprises

**OBLIGATION** de mettre à jour LEÇONS_APPRISES_MAGEN.md:
- Après chaque découverte significative
- Après chaque échec de correction
- Après chaque gain de performance
- Avec principe généralisable

---

## 🔧 MODULES COGNITIFS ACTUELS

### C17: Action Reputation System

**Fonction**: Mémoire globale réputation actions cross-puzzle

**Formule**:
```python
priority = reputation + exploration_bonus * (1/√attempts)
reputation = (successes - failures) / total_attempts
```

**État**: ✅ Implémenté (283 lignes) | ❌ Non activé (V29)

### C18: Exploration Budget Manager

**Fonction**: Allocation budget proportionnelle à réputation

**Formule**:
```python
budget = base_budget * (reputation/max_reputation) * context_multiplier
base_budget = 100 per puzzle
allocation_range = [5, 50] per action
```

**État**: ✅ Implémenté (318 lignes) | ❌ Non activé (V29)

### C19: Trajectory Analyzer

**Fonction**: Détection stagnation/divergence/oscillation

**Formule**:
```python
slope = Σ((x-x̄)(y-ȳ)) / Σ((x-x̄)²)
stagnation: slope ≥ -0.01
divergence: slope > 0
oscillation: variance > 0.05
```

**État**: ✅ Implémenté (372 lignes) | ❌ Non activé (V29)

---

## 📈 HISTORIQUE PERFORMANCE

### Évolution Score V1→V29

| Version | Score | Gain | Changement Principal |
|---------|-------|------|---------------------|
| V1-V5 | 45/400 (11.2%) | Baseline | Pattern Matcher statique |
| V21 | 174/400 (43.5%) | +287% | Transformation Learning Engine |
| V22-V25 | 174/400 (43.5%) | 0% | Arcade Discovery, optimisations |
| V26 | 174/400 (43.5%) | 0% | Corrections C7+C8+C9 |
| V27 | 174/400 (43.5%) | 0% | Seuils agressifs C10+C11 |
| V28 | 174/400 (43.5%) | 0% | Réinit progressive C16 |
| V29 | 174/400 (43.5%) | 0% | Trio cognitif C17+C18+C19 (NON ACTIVÉ) |

**Plateau actuel**: V26-V29 bloqué à 174/400 (43.5%)

---

## 🚨 PROBLÈME CRITIQUE ACTUEL (SESSION 65)

### Diagnostic V29: Modules Implémentés Mais Non Utilisés

**Symptôme**: Score V29 identique à V26 (174/400)

**Cause racine identifiée**:
```python
# test_phase2_400_puzzles.py ligne 216
predicted_output, action_name, confidence = tle.predict(
    test_input, 
    use_best_action=True  # ← COURT-CIRCUITE C17+C18+C19
)
```

**Preuve forensique**:
- ❌ 0 événements "action_reputation" dans logs V29
- ❌ 0 événements "exploration_budget" dans logs V29
- ❌ 0 événements "trajectory_analyzer" dans logs V29

**Solution V30**:
```python
# Changer use_best_action=True → False
predicted_output, action_name, confidence = tle.predict(
    test_input, 
    use_best_action=False  # ← ACTIVE C17+C18+C19
)
```

---

## 🎯 PROCHAINES ÉTAPES (V30)

### Phase 1: Activation Modules Cognitifs
1. Modifier `test_phase2_400_puzzles.py:216`
2. Changer `use_best_action=True` → `False`
3. Commit avec message explicite

### Phase 2: Test Local V30
1. Exécuter `python test_v23_corrected_pipeline.py`
2. Durée: ~70 secondes (400 puzzles)
3. Vérifier logs forensiques

### Phase 3: Validation Forensique
1. Chercher événements C17/C18/C19 dans logs
2. Confirmer modules actifs
3. Analyser métriques (budget, réputation, trajectoires)

### Phase 4: Analyse Performance
1. Comparer score V30 vs V26
2. Objectif minimum: ≥175/400 (+0.25%)
3. Objectif optimiste: 189-220/400 (+8.6% à +26.4%)

### Phase 5: Documentation
1. Créer RAPPORT_RESULTATS_V30.md
2. Mettre à jour LEÇONS_APPRISES_MAGEN.md
3. Documenter gains/échecs

---

## 📝 STANDARDS DOCUMENTATION

### Format Rapports

**Nom fichier**:
```
RAPPORT_<TYPE>_<VERSION>_<TIMESTAMP>.md
```

**Exemples**:
- `RAPPORT_ANALYSE_INTEGRATION_V29.md`
- `RAPPORT_FORENSIQUE_SESSION_63_20260613.md`
- `RAPPORT_RESULTATS_V30_20260613_233000.md`

### Contenu Obligatoire

Chaque rapport DOIT contenir:
1. **En-tête**: Date, session, statut, objectif
2. **Contexte**: Situation avant intervention
3. **Analyse**: Diagnostic détaillé avec preuves
4. **Solution**: Corrections proposées/appliquées
5. **Résultats**: Métriques avant/après
6. **Leçons**: Principes généralisables

---

## 🔍 PHILOSOPHIE LUMVORAX

### Principes Fondamentaux

1. **Vérité Absolue**: Seuls les logs forensiques disent la vérité
2. **Preuve Empirique**: Toute affirmation doit être prouvée par métriques
3. **Traçabilité Totale**: Chaque décision doit être traçable
4. **Reproductibilité**: Chaque test doit être reproductible
5. **Forensic First**: Analyser avant corriger

### Application MAGEN

- **Pas d'intuition**: Seuils calibrés empiriquement
- **Pas d'hypothèse**: Causes racines identifiées forensiquement
- **Pas de confiance aveugle**: Tests unitaires ≠ efficacité réelle
- **Pas de modification sans preuve**: Logs avant code

---

## ✅ CHECKLIST SESSION

Avant de terminer chaque session, vérifier:

- [ ] Logs forensiques générés et analysés
- [ ] Rapport session créé et horodaté
- [ ] LEÇONS_APPRISES_MAGEN.md mis à jour si découverte
- [ ] Code committé avec message explicite
- [ ] Tests locaux validés (si modifications)
- [ ] Prochaines actions documentées
- [ ] État système clair pour session suivante

---

**Dernière mise à jour**: 2026-06-13T23:33:00+02:00  
**Prochaine action**: Attendre ordre utilisateur pour correction V30