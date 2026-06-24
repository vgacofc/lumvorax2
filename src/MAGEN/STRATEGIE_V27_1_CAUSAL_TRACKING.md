# STRATÉGIE V27.1 - CAUSAL TRACKING & PERCEPTION SÉMANTIQUE
**Date**: 2026-06-14  
**Contexte**: Post-exécution réelle V27 (Session 71)  
**Objectif**: Transformer agent "aveugle" en agent "qui comprend"

---

## 🎯 DIAGNOSTIC RACINE

### Ce que V27 prouve ✅
- **Architecture cognitive stable**: 0 crash, 10 actions, 137 act/s
- **Boucle agentique vivante**: Perception → Décision → Apprentissage
- **C17 fonctionnel**: Réputation évolue (0.000→0.358)
- **Colonne vertébrale existe**: Modules V22→V27 opérationnels

### Ce que V27 révèle ❌
- **Perception symbolique incomplète**: Voit le décor, pas l'acteur
- **Compréhension causale absente**: Pas de tracking temporel
- **Reward shaping binaire**: Signal trop pauvre pour apprentissage
- **Navigation aveugle**: GridAnalyzer détecte 0% joueurs

### Verdict
```
Le système n'est pas cassé.
Il apprend simplement sur une représentation incorrecte du monde.
```

---

## 🔬 ANALYSE CAUSALE

### Problème Fondamental

**V27 actuel**:
```python
# Détection statique
if cell_value == 5:
    player = cell
```

**Résultat**: 0% détection sur jeux réels (représentation variable)

**V27.1 requis**:
```python
# Détection causale
player = object_with_highest_temporal_causality
```

**Principe**:
> Le joueur n'est pas une couleur.  
> Le joueur est l'entité qui modifie le futur.

---

## 📊 TROIS PROBLÈMES LIÉS

### 1. GridAnalyzer ne comprend pas la scène
```json
{
  "goal_detected": true,      // ✅ Structures statiques OK
  "player_detected": false,   // ❌ Entités agentiques KO
  "path_found": false,        // ❌ Navigation impossible
  "distance_to_goal": Infinity // ❌ Pas de métrique
}
```

### 2. Reward shaping ne comprend pas la progression
```python
reward = 1.0 if state == 'LEVEL_COMPLETE' else 0.0
# État reste 'NOT_FINISHED' → reward = 0.0 toujours
```

### 3. C17 apprend sur signaux pauvres
```python
# Toutes actions → reward intrinsèque 0.2-0.3
# Résultat: "toutes actions sont équivalentes"
# Réputation converge vers 0.25-0.30 (indifférencié)
```

**Conclusion**: Les 3 problèmes ont la même cause racine → **absence de compréhension causale**

---

## 🚀 SOLUTION ARCHITECTURALE V27.1

### Pipeline Cognitif Complet

```
Observation (grille_t, grille_t+1)
    ↓
Object Extraction (StateEncoder V25)
    ↓
Temporal Tracking (NEW - détection mouvement)
    ↓
Causal Attribution (NEW - qui cause quoi)
    ↓
Agent Identification (NEW - détection joueur causal)
    ↓
Goal Relation Mapping (GridAnalyzer amélioré)
    ↓
Dense Reward Shaping (NEW - reward multi-couches)
    ↓
C17 Reputation Update (existant)
    ↓
Planning (BFS existant)
```

---

## 🔧 MODULES À CRÉER

### Module 1: Temporal Object Tracker

**Fonction**: Détecter objets qui changent entre t et t+1

```python
class TemporalObjectTracker:
    def track(self, grid_t, grid_t+1):
        """
        Compare deux grilles successives.
        
        Returns:
            objects_moved: List[(obj_id, pos_t, pos_t+1)]
            objects_appeared: List[obj_id]
            objects_disappeared: List[obj_id]
            objects_modified: List[(obj_id, change_type)]
        """
```

**Métriques**:
- `movement_score`: Distance parcourue
- `persistence_score`: Présence continue
- `interaction_score`: Modifications causées

---

### Module 2: Causal Attribution Engine

**Fonction**: Identifier qui cause quoi

```python
class CausalAttributionEngine:
    def attribute_causality(self, temporal_data):
        """
        Calcule score causal pour chaque objet.
        
        causal_score = 
            movement_score * 0.4 +
            interaction_score * 0.4 +
            environment_change_score * 0.2
        
        Returns:
            agent_entity: Objet avec score maximal
            causal_graph: Relations cause→effet
        """
```

**Principe**:
```
L'objet qui bouge ET modifie l'environnement = agent
```

---

### Module 3: Dense Reward Shaper

**Fonction**: Reward multi-couches au lieu de binaire

```python
class DenseRewardShaper:
    def compute_reward(self, obs_t, obs_t+1, action):
        """
        Reward = sum of:
        - exploration_reward (0.05): Nouvelle zone visitée
        - novelty_reward (0.10): État jamais vu
        - topology_reward (0.20): Chemin vers goal plus court
        - causality_reward (0.30): Action modifie environnement
        - progress_reward (0.50): Distance à goal réduite
        - completion_reward (10.0): Niveau complété
        
        Returns:
            total_reward: float
            reward_breakdown: dict
        """
```

**Impact**:
- Signal dense au lieu de sparse
- C17 peut différencier actions utiles/inutiles
- Apprentissage 10-100x plus rapide

---

### Module 4: Action Validity Memory

**Fonction**: Mémoire contextuelle des actions valides

```python
class ActionValidityMemory:
    def __init__(self):
        self.validity = {}  # {action: {env_hash: valid}}
    
    def is_valid(self, action, environment):
        """
        Vérifie si action valide dans cet environnement.
        Apprend des erreurs API (500, 400).
        
        Returns:
            valid: bool
            confidence: float
        """
```

**Exemple**:
```python
# ACTION6 cause 500 dans ar25
validity['ACTION6']['ar25_hash'] = False

# Mais peut être valide dans cd82
validity['ACTION6']['cd82_hash'] = True
```

---

## 📈 MÉTRIQUES ATTENDUES

### Avant V27.1 (Actuel)
| Métrique | Valeur |
|----------|--------|
| Détection joueur | 0% |
| Reward environnemental | 0% |
| Actions différenciées | 0% |
| Navigation intelligente | Désactivée |
| Score | 0/3 |

### Après V27.1 (Cible)
| Métrique | Valeur |
|----------|--------|
| Détection joueur | 80%+ |
| Reward environnemental | 50%+ |
| Actions différenciées | 70%+ |
| Navigation intelligente | Activée |
| Score | 1-2/3 |

---

## 🎯 PRIORITÉS RÉORDONNÉES

### ❌ Ordre Initial (Rapport)
1. Corriger GridAnalyzer (détection statique)
2. Améliorer Reward (binaire→dense)
3. Gérer erreurs API

### ✅ Ordre Optimal (Causal)
1. **Temporal/Causal Tracking** (résout tout)
2. **Dense Reward Shaping** (signal apprentissage)
3. **Action Validity Memory** (robustesse API)

**Raison**: Causal tracking résout simultanément:
- Détection joueur
- Reward shaping
- Navigation
- Planning
- Réputation
- Exploration

---

## 🔬 PREUVE DE CONCEPT

### Signal Existant dans V27

Le rapport montre:
```json
{
  "motion_magnitude": 0.0174
}
```

**Signification**:
- ✅ Système détecte déjà variations dynamiques
- ✅ Base perception temporelle existe
- ✅ Peut construire causalité sans réécrire architecture

**Opportunité**:
```python
# StateEncoder V25 calcule déjà motion_magnitude
# Il suffit d'exploiter cette métrique pour:
# 1. Identifier objets mobiles
# 2. Tracker trajectoires
# 3. Attribuer causalité
```

---

## 🚀 ROADMAP V27.1

### Phase 1: Temporal Tracking (3-4h)
**Objectif**: Détecter qui bouge

1. Créer `TemporalObjectTracker`
2. Intégrer dans boucle perception
3. Tester sur 10 actions réelles
4. Valider détection mouvement

**Livrable**: `core/temporal_tracker.py`

---

### Phase 2: Causal Attribution (2-3h)
**Objectif**: Identifier agent

1. Créer `CausalAttributionEngine`
2. Calculer scores causaux
3. Identifier agent_entity
4. Valider sur jeux réels

**Livrable**: `core/causal_engine.py`

---

### Phase 3: Dense Reward (2h)
**Objectif**: Signal apprentissage riche

1. Créer `DenseRewardShaper`
2. Implémenter 6 couches reward
3. Intégrer dans boucle apprentissage
4. Valider différenciation actions

**Livrable**: `core/dense_reward.py`

---

### Phase 4: Action Validity (1h)
**Objectif**: Robustesse API

1. Créer `ActionValidityMemory`
2. Gérer erreurs 500/400
3. Apprendre contextes valides
4. Valider sur 10 jeux

**Livrable**: `core/action_validity.py`

---

### Phase 5: Intégration V27.1 (2h)
**Objectif**: Agent V27.1 complet

1. Intégrer 4 modules dans agent
2. Créer `magen_arc_agi_agent_v27_1_causal.py`
3. Tests unitaires complets
4. Validation 10 jeux réels

**Livrable**: Agent V27.1 CAUSAL

---

### Phase 6: Validation Terrain (1h)
**Objectif**: Mesurer impact réel

1. Exécuter 10 jeux ARC-AGI-3
2. Analyser forensic logs
3. Comparer V27 vs V27.1
4. Documenter résultats

**Livrable**: `RAPPORT_V27_1_VALIDATION.md`

---

## 📊 ESTIMATION IMPACT

### Transition Cognitive

**V27 (Actuel)**:
```
Agent qui agit sans comprendre
```

**V27.1 (Cible)**:
```
Agent qui comprend pourquoi le monde change
```

### Capacités Émergentes

Avec causal tracking, l'agent peut:
1. **Identifier son avatar** dans la grille
2. **Comprendre ses actions** (cause→effet)
3. **Prédire conséquences** (modèle causal)
4. **Planifier intelligemment** (navigation causale)
5. **Apprendre efficacement** (reward dense)

### Score Attendu

```
V27:   0/3 (exploration aveugle)
V27.1: 1-2/3 (navigation causale)
V28:   2-3/3 (compréhension complète)
```

**Mais surtout**:
```
Les décisions deviendront cohérentes temporellement
```

C'est plus important que le score brut.

---

## 🎓 INSIGHTS ARCHITECTURAUX

### 1. Perception ≠ Vision
```
Voir la grille ≠ Comprendre la scène
```

V27 voit pixels, V27.1 comprend causalité.

### 2. Reward ≠ Score
```
Score binaire ≠ Signal apprentissage
```

Reward dense permet exploration dirigée.

### 3. Action ≠ Mouvement
```
Exécuter action ≠ Comprendre effet
```

Causal tracking lie action→conséquence.

### 4. Mémoire ≠ Historique
```
Stocker états ≠ Comprendre relations
```

C17 + causalité = apprentissage sémantique.

---

## 🔮 VISION V28

Après V27.1, la prochaine étape devient:

**V28 - PREDICTIVE CAUSAL MODEL**

Capacités:
- Prédire effet de chaque action
- Simuler trajectoires mentalement
- Planifier multi-étapes
- Généraliser patterns causaux

Architecture:
```
V27.1 (comprend présent)
    ↓
V28 (prédit futur)
    ↓
V29 (généralise patterns)
```

---

## 📁 STRUCTURE FICHIERS V27.1

```
lumvorax2/src/MAGEN/
├── core/
│   ├── temporal_tracker.py          # NEW - Phase 1
│   ├── causal_engine.py              # NEW - Phase 2
│   ├── dense_reward.py               # NEW - Phase 3
│   ├── action_validity.py            # NEW - Phase 4
│   ├── grid_analyzer.py              # EXISTANT - À améliorer
│   ├── action_reputation_system.py   # EXISTANT - C17
│   └── state_encoder_v25.py          # EXISTANT - Base temporelle
├── magen_arc_agi_agent_v27_1_causal.py  # NEW - Phase 5
├── test_temporal_tracker.py          # NEW - Tests
├── test_causal_engine.py             # NEW - Tests
├── test_dense_reward.py              # NEW - Tests
├── run_v27_1_real_arc_agi_3.py       # NEW - Runner
└── RAPPORT_V27_1_VALIDATION.md       # NEW - Phase 6
```

---

## 🎯 CRITÈRES DE SUCCÈS V27.1

### Critères Techniques
- [ ] Détection joueur: >80% sur 10 jeux
- [ ] Reward environnemental: >50% actions
- [ ] C17 différencie actions: écart réputation >0.3
- [ ] Navigation activée: path_found = true
- [ ] 0 crash sur 100 actions

### Critères Cognitifs
- [ ] Agent identifie son avatar
- [ ] Décisions cohérentes temporellement
- [ ] Apprentissage visible (réputation évolue)
- [ ] Exploration dirigée (pas aléatoire)
- [ ] Adaptation contextuelle (validity memory)

### Critères Gameplay
- [ ] Score: 1-2/3 jeux complétés
- [ ] Actions: 50-100 par jeu
- [ ] Niveaux: 1-3 complétés
- [ ] Temps: <5min par jeu

---

## 🚀 PROCHAINE SESSION 72

### Objectif
Implémenter V27.1 CAUSAL TRACKING complet

### Livrables
1. 4 nouveaux modules (temporal, causal, reward, validity)
2. Agent V27.1 intégré
3. Tests unitaires complets
4. Validation 10 jeux réels
5. Rapport forensic comparatif

### Durée Estimée
10-12 heures (2 sessions)

### Impact Attendu
**Transformation qualitative**: Agent aveugle → Agent qui comprend

---

## 📊 CONCLUSION

### Ce que Session 71 a prouvé
✅ Architecture cognitive V27 est **stable et fonctionnelle**  
✅ Boucle agentique est **vivante et performante**  
✅ C17 Reputation **apprend correctement**  
✅ Forensic logging **capture tout**

### Ce que Session 71 a révélé
❌ Perception symbolique **incomplète**  
❌ Compréhension causale **absente**  
❌ Reward shaping **trop binaire**  
❌ Navigation **aveugle**

### Ce que Session 72 va résoudre
🎯 **Temporal/Causal Tracking** → Perception sémantique  
🎯 **Dense Reward Shaping** → Signal apprentissage  
🎯 **Action Validity Memory** → Robustesse contextuelle  
🎯 **Agent V27.1 CAUSAL** → Compréhension du monde

### Transition Fondamentale
```
V27:   Agent qui agit
V27.1: Agent qui comprend pourquoi le monde change
V28:   Agent qui prédit le futur
```

---

**Document créé**: 2026-06-14 19:28  
**Auteur**: MAGEN V27 COGNITIVE  
**Méthodologie**: CLAUDE_PILOT (forensic-first)  
**Statut**: PRÊT POUR IMPLÉMENTATION