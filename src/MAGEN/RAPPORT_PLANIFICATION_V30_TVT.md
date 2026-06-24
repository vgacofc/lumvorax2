# RAPPORT PLANIFICATION MAGEN V30 - TRAJECTORY VALUE THEORY

**Date** : 2026-06-15  
**Session** : 72 - Jour 6  
**Version** : V30.0.0  
**Auteur** : Bob (LVX AI Assistant)

---

## CONTEXTE : LIMITE STRUCTURELLE IDENTIFIÉE

### Résultats Session 72 - Jour 5

**Test ARC Arcade réels (3 jeux via API officielle)** :
- **Taux succès : 0/3 (0.0%)**
- **Baseline synthétique : 174/400 (42.9%)**
- **Écart : -42.9 points**

### Diagnostic Clé

Le **vrai signal** n'est PAS le 0% de succès.

Le **vrai signal** est la **variance des scores = 0.015625**.

```
Scores uniformes observés :
{
  'ACTION1': 0.5,
  'ACTION2': 0.625,
  'ACTION3': 0.5,
  'ACTION4': 0.625
}

Variance = 0.015625 << 0.02 (seuil collapse)
```

### Interprétation

**MAGEN est entré dans un régime de collapse politique attractif.**

Le système ne différencie pratiquement plus les actions. Toutes les trajectoires deviennent équivalentes du point de vue décisionnel.

**Conséquence** : Biais directionnel massif (84% ACTION2) sans justification causale.

---

## ANALYSE PHILOSOPHIQUE : CHANGEMENT DE NATURE DU PROBLÈME

### Citation Utilisateur (Insight Majeur)

> "Le problème n'est plus perceptionnel, transformationnel ou infrastructurel.  
> Le problème devient : **gestion stratégique de trajectoires longues sous information partielle**.  
> MAGEN manque maintenant d'une **théorie interne de la valeur des trajectoires**."

### Limite Architecturale V28.2.2

MAGEN V28 fonctionne comme :
- ✓ Moteur de transformations
- ✓ Exploration locale
- ✓ Heuristiques court terme

MAGEN V28 ne fonctionne PAS comme :
- ✗ Système cognitif apprenant
- ✗ Allocation attentionnelle dynamique
- ✗ Détection trajectoires mortes
- ✗ Rupture politique intelligente

### Passage Critique

```text
MAGEN a atteint la limite de son architecture actuelle.
```

Le saut 45 → 174 (V27 → V28) a capturé les gains "faciles" :
- Transformations
- Détecteurs
- Exploration adaptative

**Maintenant** : Entrer dans une autre catégorie de problèmes.

---

## SAUT ARCHITECTURAL : V28 → V30

### Nouvelle Architecture Cognitive

```text
PHASE 0 : Affordance Discovery
PHASE 1 : Candidate Filtering
PHASE 2 : Reputation Prioritization      ← C17
PHASE 3 : Budget Allocation              ← C18
PHASE 4 : Trajectory Monitoring          ← C19
PHASE 5 : Symbolic Validation
```

### Trio Structurel Critique

| Module | Fonction Cognitive | Implémentation |
|--------|-------------------|----------------|
| **C17** | Mémoire de valeur des actions | Réputation contextualisée par région topologique |
| **C18** | Allocation attentionnelle | Budget dynamique par hypothèse de trajectoire |
| **C19** | Détection stagnation cognitive | Monitoring collapse + rupture politique |

### Question Centrale

**"Cette trajectoire mérite-t-elle encore des ressources cognitives ?"**

---

## IMPLÉMENTATION V30.0.0

### 1. Module Unifié : Trajectory Value Theory (TVT)

**Fichier** : [`trajectory_value_theory.py`](lumvorax2/src/MAGEN/core/trajectory_value_theory.py)  
**Lignes** : 371  
**Date** : 2026-06-15

#### Composants Clés

##### A. Mémoire Viabilité Topologique

```python
region_viability: Dict[Tuple[int, int], Dict]
# {
#   region: {
#     'total_visits': int,
#     'successful_visits': int,
#     'avg_reward': float,
#     'viability_score': float  # 0.0-1.0
#   }
# }

sterile_regions: Set[Tuple[int, int]]
# Régions identifiées comme stériles (viability < 0.2)
```

**Principe** : Le moteur apprend quelles régions topologiques sont fertiles vs stériles.

##### B. Réputation Actions Contextualisée

```python
action_reputation: Dict[Tuple[str, Tuple[int, int]], Dict]
# {
#   (action, region): {
#     'success_count': int,
#     'total_count': int,
#     'success_rate': float,
#     'avg_reward': float,
#     'avg_error': float
#   }
# }
```

**Principe** : La réputation d'une action dépend du contexte topologique.

##### C. Détection Collapse Politique

```python
def detect_policy_collapse(self, action_scores: Dict[str, float]) -> bool:
    """Détecte variance < 0.02 → collapse attractif."""
    variance = np.var(list(action_scores.values()))
    return variance < self.collapse_variance_threshold
```

**Principe** : Identifier quand toutes actions deviennent équivalentes.

##### D. Détection Stagnation

```python
def detect_stagnation(self, current_reward: float) -> bool:
    """Détecte N steps sans progrès."""
    self.reward_history.append(current_reward)
    
    if len(self.reward_history) < self.stagnation_threshold:
        return False
    
    recent_rewards = list(self.reward_history)[-self.stagnation_threshold:]
    return all(r <= 0 for r in recent_rewards)
```

**Principe** : Identifier trajectoires mortes (N steps sans reward positif).

##### E. Rupture Politique Forcée

```python
def break_policy(self, action_scores: Dict[str, float]) -> Dict[str, float]:
    """Casse attracteur via exploration forcée."""
    # Inverser scores
    inverted = {a: 1.0 - s for a, s in action_scores.items()}
    
    # Ajouter bruit
    noisy = {
        a: s + np.random.uniform(-0.3, 0.3)
        for a, s in inverted.items()
    }
    
    return noisy
```

**Principe** : Destruction contrôlée de politique pour échapper attracteurs stériles.

### 2. Decision Kernel V30 avec TVT

**Fichier** : [`decision_kernel_v30_tvt.py`](lumvorax2/src/MAGEN/core/decision_kernel_v30_tvt.py)  
**Lignes** : 199  
**Date** : 2026-06-15

#### Architecture

```python
class DecisionKernelV30TVT(DecisionKernelMinimal):
    """
    Hérite V28.2.2 + intègre TVT.
    
    Processus décision :
    1. Calcul scores base (V28.2.2)
    2. Détection collapse/stagnation (TVT)
    3. Rupture politique si nécessaire (TVT)
    4. Ajustement scores avec réputation topologique (TVT)
    5. Sélection finale
    """
```

#### Méthode Centrale : `select_action()`

```python
def select_action(
    self,
    state: Dict,
    action_candidates: List[str],
    reward_shaper=None,
    reputation_system=None,
    grid_analyzer=None,
    causal_graph=None,
    world_model=None
) -> Tuple[str, Dict]:
    # 1. Scores base (hérité V28.2.2)
    base_scores = self.compute_action_scores(...)
    
    # 2. Position actuelle
    agent_pos = state.get('agent_pos', (0, 0))
    
    # 3. Reward actuel
    current_reward = state.get('last_reward', 0.0)
    
    # 4. Sélection avec TVT
    selected_action, adjusted_scores, policy_broken = self.tvt.select_action_with_tvt(
        base_scores,
        agent_pos,
        current_reward
    )
    
    return selected_action, decision_info
```

#### Apprentissage Post-Step

```python
def update_after_step(
    self,
    state: Dict,
    action: str,
    next_state: Dict,
    reward: float,
    done: bool
):
    # 1. Mise à jour poids (hérité V28.2.2)
    self.update_weights(reward)
    
    # 2. Détection progrès
    progress = (next_agent_pos != agent_pos) or (reward > 0)
    
    # 3. Mise à jour TVT
    self.tvt.update_region_viability(agent_pos, reward, progress)
    self.tvt.update_action_reputation(action, agent_pos, reward, success)
```

### 3. Script Test Comparatif

**Fichier** : [`test_v30_synthetic.py`](lumvorax2/src/MAGEN/test_v30_synthetic.py)  
**Lignes** : 398  
**Date** : 2026-06-15

#### Environnements Test (7)

1. **simple_direct** : 10x10, pas d'obstacles
2. **with_obstacles** : 10x10, mur vertical
3. **maze_simple** : 12x12, 3 murs
4. **long_corridor** : 20x5, corridor long
5. **u_shape** : 10x10, forme U
6. **multi_goal** : 12x12, 4 goals
7. **complex_maze** : 15x15, 4 murs

#### Métriques Comparées

| Métrique | V28.2.2 | V30.0.0 |
|----------|---------|---------|
| Taux succès | ? | ? |
| Steps moyens (succès) | ? | ? |
| Reward total | ? | ? |
| Policy breaks | N/A | ? |
| Collapse détecté | N/A | ? |
| Stagnation détectée | N/A | ? |

---

## HYPOTHÈSES TESTABLES

### H1 : Amélioration Taux Succès

**Hypothèse** : V30 réussit là où V28 échoue (environnements complexes).

**Prédiction** : +5-10% taux succès sur environnements avec obstacles.

**Raison** : Rupture politique casse attracteurs stériles.

### H2 : Réduction Steps

**Hypothèse** : V30 plus efficace (moins de steps pour succès).

**Prédiction** : -10-20% steps moyens.

**Raison** : Mémoire viabilité évite régions stériles.

### H3 : Détection Collapse

**Hypothèse** : V30 détecte et corrige collapse politique.

**Prédiction** : 2-5 policy breaks par environnement complexe.

**Raison** : Variance < 0.02 déclenche rupture.

### H4 : Robustesse Trajectoires Longues

**Hypothèse** : V30 meilleur sur environnements nécessitant >50 steps.

**Prédiction** : Écart V30-V28 augmente avec complexité.

**Raison** : Horizon décision plus long via mémoire topologique.

---

## RISQUES IDENTIFIÉS

### R1 : Biais Exploitation Prématurée

**Risque** : Réputation favorise actions globalement bonnes, sous-explore branches rares.

**Exemple** :
- `rotate_90` marche souvent → réputation explose
- Certains puzzles nécessitent `identity` + transformation secondaire
- Ces branches deviennent sous-explorées

**Mitigation** : Rupture politique forcée + exploration résiduelle.

### R2 : Overhead Computationnel

**Risque** : TVT ajoute calculs (discrétisation, BFS, variance).

**Impact** : +10-30% temps exécution.

**Mitigation** : Cache BFS, discrétisation grossière (grid 4x4).

### R3 : Hyperparamètres Sensibles

**Risque** : Seuils collapse (0.02) et stagnation (10 steps) arbitraires.

**Impact** : Performances dépendent fortement des seuils.

**Mitigation** : Sweep hyperparamètres après validation initiale.

---

## PLAN EXÉCUTION

### Jour 7 (2026-06-15)

#### Matin
- [x] Implémenter TVT unifié (C17+C18+C19)
- [x] Créer Decision Kernel V30
- [x] Créer script test comparatif

#### Après-midi
- [ ] **Exécuter test V30 vs V28** (7 environnements)
- [ ] Analyser résultats
- [ ] Rapport comparatif détaillé

### Jour 8 (2026-06-16)

#### Matin
- [ ] Test V30 sur ARC Arcade réels (3 jeux)
- [ ] Comparer 0.0% (V28) vs ? (V30)

#### Après-midi
- [ ] Rapport final saut architectural V28 → V30
- [ ] Documentation leçons apprises
- [ ] Roadmap V31 (si nécessaire)

---

## CRITÈRES SUCCÈS V30

### Succès Minimal

- ✓ Taux succès V30 ≥ V28 (pas de régression)
- ✓ Au moins 1 policy break détecté
- ✓ Pas de crash/erreur

### Succès Attendu

- ✓ Taux succès V30 > V28 (+5-10%)
- ✓ Steps moyens V30 < V28 (-10-20%)
- ✓ 2-5 policy breaks par environnement complexe

### Succès Exceptionnel

- ✓ Taux succès V30 > V28 (+15%+)
- ✓ V30 réussit ARC Arcade réels (>0%)
- ✓ Collapse détecté et corrigé systématiquement

---

## CONCLUSION PLANIFICATION

### Saut Conceptuel

**V28.2.2** : Moteur transformationnel avec exploration locale.

**V30.0.0** : Agent cognitif décisionnel avec mémoire trajectoires.

### Innovation Clé

**Trajectory Value Theory** : Première implémentation d'une théorie interne de la valeur des trajectoires dans MAGEN.

### Question Centrale Résolue

> "Cette trajectoire mérite-t-elle encore des ressources cognitives ?"

**Réponse V30** :
1. Calculer viabilité région topologique
2. Évaluer réputation action contextualisée
3. Détecter collapse/stagnation
4. Casser politique si nécessaire
5. Allouer budget dynamiquement

### Prochaine Étape

**Exécuter test comparatif V30 vs V28 sur 7 environnements synthétiques.**

---

**Fichiers Créés** :
- [`trajectory_value_theory.py`](lumvorax2/src/MAGEN/core/trajectory_value_theory.py) (371 lignes)
- [`decision_kernel_v30_tvt.py`](lumvorax2/src/MAGEN/core/decision_kernel_v30_tvt.py) (199 lignes)
- [`test_v30_synthetic.py`](lumvorax2/src/MAGEN/test_v30_synthetic.py) (398 lignes)

**Total** : 968 lignes de code nouveau.

---

*Rapport généré automatiquement - Session 72 Jour 6*  
*MAGEN V30.0.0 - Trajectory Value Theory*