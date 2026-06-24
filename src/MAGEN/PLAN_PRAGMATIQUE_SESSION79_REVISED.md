# PLAN PRAGMATIQUE RÉVISÉ - SESSION 79
**Approche Validation-First: Tester les hypothèses avant d'investir**

---

## 🎯 PHILOSOPHIE DU PLAN

**Principe**: Valider chaque hypothèse avec un test rapide (< 1h) avant d'investir dans une solution complexe (> 5h).

**Ordre de priorité**:
1. **Quick wins** (gains immédiats, < 2h)
2. **Validations critiques** (vérifier hypothèses, < 3h)
3. **Solutions incrémentales** (améliorer progressivement, 3-5h)
4. **Solutions avancées** (seulement si validations réussies, > 5h)

---

## 🚀 PHASE 1: QUICK WINS (2-3h) - PRIORITÉ MAXIMALE

### 1.1 Restreindre Actions à ACTION1-4 (30 min)

**Hypothèse à valider**: 40% des actions MAGEN sont ignorées par ls20.

**Test rapide**:
```python
# Dans train_single_game_ls20_realtime.py
class ArcadeAdapter:
    def __init__(self, ...):
        # AVANT
        # self.available_actions = list(range(1, 8))  # ACTION1-7
        
        # APRÈS
        self.available_actions = [1, 2, 3, 4]  # Mouvements uniquement
        print(f"✓ Actions restreintes: {self.available_actions}")
```

**Métriques à mesurer** (100 steps):
- Distribution actions utilisées
- États uniques observés
- Taux de changement environnement
- Reward externe

**Critère succès**: États uniques > 10/100 steps (vs 1/100 actuellement)

**Temps**: 30 min modification + 10 min test

---

### 1.2 Logger Actions Ignorées (15 min)

**Objectif**: Confirmer que ACTION5+ sont bien ignorées.

**Code**:
```python
# Dans train_single_game_ls20_realtime.py
class ActionLogger:
    def __init__(self):
        self.action_counts = {}
        self.ignored_actions = []
    
    def log_action(self, action, state_changed):
        self.action_counts[action] = self.action_counts.get(action, 0) + 1
        if not state_changed:
            self.ignored_actions.append(action)
    
    def report(self):
        print("\n=== ACTION ANALYSIS ===")
        for action, count in sorted(self.action_counts.items()):
            ignored = self.ignored_actions.count(action)
            print(f"ACTION{action}: {count} fois ({ignored} ignorées = {ignored/count*100:.1f}%)")

# Utilisation
logger = ActionLogger()
prev_obs = None
for step in range(100):
    action = policy.select_action(obs)
    next_obs, reward, done, info = env.step(action)
    
    state_changed = not np.array_equal(prev_obs, next_obs) if prev_obs is not None else True
    logger.log_action(action, state_changed)
    
    prev_obs = next_obs

logger.report()
```

**Critère succès**: Confirmation que ACTION5+ → 0% changement état

**Temps**: 15 min

---

### 1.3 Test Baseline avec Actions Restreintes (1h)

**Objectif**: Mesurer impact immédiat de la restriction.

**Protocole**:
```bash
# Test 1: Baseline actuel (ACTION1-7)
python train_single_game_ls20_realtime.py --steps 200 --mode baseline

# Test 2: Actions restreintes (ACTION1-4)
python train_single_game_ls20_realtime.py --steps 200 --mode restricted

# Comparaison
python compare_results.py --baseline results/baseline.json --restricted results/restricted.json
```

**Métriques comparées**:
```python
comparison = {
    "unique_states": {
        "baseline": 1,
        "restricted": ???  # Attendu: > 10
    },
    "change_rate": {
        "baseline": 0.00,
        "restricted": ???  # Attendu: > 0.10
    },
    "reward_external": {
        "baseline": 0.00,
        "restricted": ???  # Attendu: > 0.00
    },
    "action_efficiency": {
        "baseline": 0.60,  # 60% actions utiles
        "restricted": 1.00  # 100% actions utiles
    }
}
```

**Critère succès**: Amélioration mesurable sur au moins 2 métriques

**Temps**: 1h (30 min tests + 30 min analyse)

---

## 🔍 PHASE 2: VALIDATIONS CRITIQUES (2-3h)

### 2.1 Vérifier Contenu Replay API (30 min)

**Hypothèse à valider**: Le replay contient observations complètes.

**Test rapide**:
```python
import requests
import json

def check_replay_structure(session_id):
    """Vérifier structure replay sans télécharger tout"""
    url = f"https://arcprize.org/api/replay/{session_id}"
    
    # Requête HEAD pour métadonnées
    response = requests.head(url)
    print(f"Content-Length: {response.headers.get('Content-Length')} bytes")
    
    # Télécharger seulement premiers 1000 bytes
    response = requests.get(url, headers={"Range": "bytes=0-1000"})
    partial_data = response.text
    
    print("\n=== STRUCTURE REPLAY (premiers 1000 bytes) ===")
    print(partial_data)
    
    # Analyser structure
    try:
        data = json.loads(partial_data)
        print("\n=== CHAMPS DISPONIBLES ===")
        print(f"Keys: {list(data.keys())}")
        
        if "levels" in data and len(data["levels"]) > 0:
            level0 = data["levels"][0]
            print(f"\nLevel 0 keys: {list(level0.keys())}")
            
            # Vérifier présence observations
            has_observations = "observations" in level0
            has_states = "states" in level0
            has_frames = "frames" in level0
            
            print(f"\n✓ Observations: {has_observations}")
            print(f"✓ States: {has_states}")
            print(f"✓ Frames: {has_frames}")
            
            return {
                "has_observations": has_observations,
                "has_states": has_states,
                "has_frames": has_frames,
                "structure": level0.keys()
            }
    except:
        print("⚠️ Impossible de parser JSON (replay trop grand?)")
        return None

# Test
result = check_replay_structure("8aed7120-f7a9-45a1-837a-68bc7dc37a4f")
```

**Scénarios possibles**:

**Scénario A**: Observations complètes présentes
→ Continuer avec plan dataset CNN

**Scénario B**: Seulement actions + timestamps
→ Rejouer actions dans environnement pour reconstruire états

**Scénario C**: API inaccessible ou replay inexistant
→ Créer dataset synthétique depuis heuristiques

**Temps**: 30 min

---

### 2.2 Analyser Distribution Actions Humaines (30 min)

**Objectif**: Vérifier si humain utilise bien ACTION1-4 uniquement.

**Code**:
```python
def analyze_human_actions(replay_data):
    """Analyser actions utilisées par humain"""
    all_actions = []
    
    for level in replay_data.get("levels", []):
        actions = level.get("actions", [])
        all_actions.extend(actions)
    
    from collections import Counter
    action_dist = Counter(all_actions)
    
    print("\n=== DISTRIBUTION ACTIONS HUMAINES ===")
    for action in sorted(action_dist.keys()):
        count = action_dist[action]
        pct = count / len(all_actions) * 100
        print(f"ACTION{action}: {count} fois ({pct:.1f}%)")
    
    # Vérifier si actions > 4 utilisées
    invalid_actions = [a for a in action_dist.keys() if a > 4]
    
    if invalid_actions:
        print(f"\n⚠️ ATTENTION: Humain utilise actions > 4: {invalid_actions}")
        print("→ Hypothèse 'ACTION1-4 uniquement' INVALIDE")
    else:
        print(f"\n✓ CONFIRMÉ: Humain utilise uniquement ACTION1-4")
        print("→ Hypothèse validée")
    
    return {
        "distribution": dict(action_dist),
        "total_actions": len(all_actions),
        "unique_actions": len(action_dist),
        "uses_invalid_actions": len(invalid_actions) > 0
    }
```

**Critère validation**: Humain utilise ACTION1-4 uniquement

**Temps**: 30 min

---

### 2.3 Créer Dataset Symbolique Minimal (1h)

**Objectif**: Tester exploitabilité données AVANT CNN lourd.

**Structure simplifiée**:
```python
symbolic_dataset = {
    "samples": [
        {
            # État symbolique (pas d'image)
            "player_x": 34,
            "player_y": 10,
            "shape": 5,
            "color": 9,
            "rotation": 270,
            "steps_remaining": 38,
            "goal_x": 35,
            "goal_y": 11,
            "goal_shape": 5,
            "goal_color": 9,
            "goal_rotation": 0,
            
            # Action humaine
            "action": 4,  # DROITE
            
            # Contexte
            "level": 0,
            "step": 5,
            "phase": "exploration"
        },
        ...
    ]
}
```

**Avantages**:
- Pas besoin d'observations complètes
- Dataset 100x plus léger
- Entraînement 10x plus rapide
- Interprétable (on voit pourquoi action choisie)

**Test avec MLP simple**:
```python
import torch.nn as nn

class SymbolicPolicy(nn.Module):
    def __init__(self):
        super().__init__()
        self.fc1 = nn.Linear(10, 64)  # 10 features symboliques
        self.fc2 = nn.Linear(64, 32)
        self.fc3 = nn.Linear(32, 4)   # 4 actions
    
    def forward(self, x):
        x = F.relu(self.fc1(x))
        x = F.relu(self.fc2(x))
        return F.softmax(self.fc3(x), dim=1)
```

**Critère succès**: Accuracy > 70% sur dataset symbolique

**Temps**: 1h (30 min création + 30 min test MLP)

---

### 2.4 Identifier Phases par Événements (30 min)

**Objectif**: Remplacer découpage temporel arbitraire par détection événements.

**Code**:
```python
class PhaseDetector:
    def __init__(self):
        self.current_phase = "exploration"
        self.prev_shape = None
        self.prev_color = None
        self.prev_rotation = None
        self.transformations_done = 0
        self.goal_reached = False
    
    def detect_phase(self, state):
        """Détecter phase basée sur événements"""
        
        # Détecter transformation
        if self.prev_shape is not None:
            if (state["shape"] != self.prev_shape or 
                state["color"] != self.prev_color or 
                state["rotation"] != self.prev_rotation):
                self.transformations_done += 1
                self.current_phase = "transformation"
        
        # Détecter approche objectif
        dist_to_goal = abs(state["player_x"] - state["goal_x"]) + abs(state["player_y"] - state["goal_y"])
        if dist_to_goal < 5 and self.transformations_done > 0:
            self.current_phase = "goal"
        
        # Détecter victoire
        if (state["player_x"] == state["goal_x"] and 
            state["player_y"] == state["goal_y"] and
            state["shape"] == state["goal_shape"] and
            state["color"] == state["goal_color"] and
            state["rotation"] == state["goal_rotation"]):
            self.goal_reached = True
            self.current_phase = "success"
        
        self.prev_shape = state["shape"]
        self.prev_color = state["color"]
        self.prev_rotation = state["rotation"]
        
        return self.current_phase
```

**Critère succès**: Phases détectées correspondent aux transformations réelles

**Temps**: 30 min

---

## 📊 PHASE 3: SOLUTIONS INCRÉMENTALES (3-5h)

### 3.1 Politique Hybride: Heuristiques + Exploration (2h)

**Objectif**: Améliorer MAGEN sans ML complexe.

**Stratégie**:
```python
class HybridPolicy:
    def __init__(self):
        self.phase_detector = PhaseDetector()
        self.exploration_rate = 0.3
    
    def select_action(self, state):
        phase = self.phase_detector.detect_phase(state)
        
        # Phase exploration: exploration aléatoire
        if phase == "exploration":
            if random.random() < self.exploration_rate:
                return random.choice([1, 2, 3, 4])
            else:
                return self._move_towards_transformers(state)
        
        # Phase transformation: aller vers transformateurs
        elif phase == "transformation":
            return self._move_towards_transformers(state)
        
        # Phase goal: aller vers objectif
        elif phase == "goal":
            return self._move_towards_goal(state)
    
    def _move_towards_transformers(self, state):
        """Heuristique: se diriger vers transformateurs"""
        # Trouver transformateur le plus proche
        # Calculer direction
        # Retourner action appropriée
        pass
    
    def _move_towards_goal(self, state):
        """Heuristique: se diriger vers objectif"""
        dx = state["goal_x"] - state["player_x"]
        dy = state["goal_y"] - state["player_y"]
        
        if abs(dx) > abs(dy):
            return 4 if dx > 0 else 3  # DROITE ou GAUCHE
        else:
            return 2 if dy > 0 else 1  # BAS ou HAUT
```

**Avantages**:
- Pas besoin de dataset
- Pas besoin d'entraînement
- Interprétable
- Rapide à implémenter

**Critère succès**: Reward externe > 0.1 (progression mesurable)

**Temps**: 2h

---

### 3.2 Curriculum Learning: Niveau par Niveau (2h)

**Objectif**: Apprendre progressivement au lieu de tout en même temps.

**Stratégie**:
```python
class CurriculumTrainer:
    def __init__(self):
        self.current_level = 0
        self.success_threshold = 0.3  # 30% victoires
        self.episodes_per_level = 50
    
    def train(self):
        for level in range(7):
            print(f"\n=== NIVEAU {level} ===")
            
            success_rate = 0
            for episode in range(self.episodes_per_level):
                result = self.train_episode(level)
                success_rate = self.update_success_rate(result)
                
                if success_rate > self.success_threshold:
                    print(f"✓ Niveau {level} maîtrisé ({success_rate:.1%})")
                    break
            
            if success_rate < self.success_threshold:
                print(f"⚠️ Niveau {level} non maîtrisé, continuer entraînement")
```

**Avantages**:
- Apprentissage progressif
- Évite overfitting sur niveaux difficiles
- Permet d'identifier niveaux bloquants

**Temps**: 2h

---

## 🎓 PHASE 4: SOLUTIONS AVANCÉES (> 5h) - SEULEMENT SI VALIDATIONS OK

### 4.1 Imitation Learning avec Dataset Symbolique (3h)

**Condition préalable**: 
- ✅ Replay accessible avec actions
- ✅ Dataset symbolique créé
- ✅ MLP simple fonctionne (accuracy > 70%)

**Implémentation**: Voir plan original, mais avec dataset symbolique au lieu de CNN.

---

### 4.2 Imitation Learning avec CNN (5h+)

**Condition préalable**:
- ✅ Replay contient observations complètes
- ✅ Dataset symbolique insuffisant (accuracy < 80%)
- ✅ Ressources GPU disponibles

**Implémentation**: Voir plan original.

---

## 📋 PLAN D'EXÉCUTION RÉVISÉ

### Jour 1: Quick Wins + Validations (4-5h)

**Matin (2-3h)**:
1. ✅ Restreindre actions ACTION1-4 (30 min)
2. ✅ Logger actions ignorées (15 min)
3. ✅ Test baseline vs restricted (1h)
4. ✅ Analyser résultats (30 min)

**Après-midi (2h)**:
5. ✅ Vérifier contenu replay API (30 min)
6. ✅ Analyser distribution actions humaines (30 min)
7. ✅ Créer dataset symbolique minimal (1h)

**Décision Go/No-Go**: 
- Si amélioration mesurable avec actions restreintes → Continuer
- Si replay inaccessible → Passer à heuristiques
- Si dataset symbolique exploitable → Éviter CNN

---

### Jour 2: Solutions Incrémentales (3-5h)

**Matin (2h)**:
8. ✅ Implémenter politique hybride (2h)
9. ✅ Tester sur niveau 1 (30 min)

**Après-midi (2-3h)**:
10. ✅ Implémenter curriculum learning (2h)
11. ✅ Tester sur niveaux 1-3 (1h)

**Décision Go/No-Go**:
- Si reward externe > 0.1 → Succès, continuer optimisation
- Si reward externe = 0.0 → Analyser pourquoi, ajuster

---

### Jour 3+: Solutions Avancées (si nécessaire)

**Seulement si**:
- Validations réussies
- Quick wins insuffisants
- Ressources disponibles

12. ✅ Imitation learning (symbolique ou CNN)
13. ✅ Fine-tuning
14. ✅ Validation complète

---

## 🎯 CRITÈRES DE SUCCÈS RÉVISÉS

### Succès Minimal (Jour 1)
- ✅ Actions restreintes à ACTION1-4
- ✅ États uniques > 10/100 steps (vs 1/100)
- ✅ Reward externe > 0.00
- ✅ Hypothèses validées ou invalidées

### Succès Intermédiaire (Jour 2)
- ✅ Politique hybride implémentée
- ✅ Reward externe > 0.1
- ✅ Progression mesurable vers objectifs
- ✅ Niveau 1 complété (ou proche)

### Succès Optimal (Jour 3+)
- ✅ Imitation learning fonctionnel
- ✅ Reward externe > 0.5
- ✅ 2+ niveaux complétés
- ✅ Généralisation démontrée

---

## ⚠️ RISQUES ET MITIGATIONS RÉVISÉS

### Risque 1: Actions restreintes n'améliorent rien
**Probabilité**: 10%
**Impact**: Critique
**Mitigation**: Analyser pourquoi avec logger détaillé

### Risque 2: Replay inaccessible
**Probabilité**: 30%
**Impact**: Moyen
**Mitigation**: Passer directement à heuristiques (Phase 3)

### Risque 3: Dataset symbolique insuffisant
**Probabilité**: 40%
**Impact**: Faible
**Mitigation**: Ajouter features ou passer à CNN

### Risque 4: Heuristiques bloquées
**Probabilité**: 20%
**Impact**: Moyen
**Mitigation**: Analyser états bloquants, ajuster stratégie

---

## 🏁 CONCLUSION

**Différences clés avec plan original**:

1. **Validation-first**: Tester hypothèses avant d'investir
2. **Quick wins prioritaires**: Gains rapides (actions restreintes)
3. **Dataset symbolique**: Alternative légère au CNN
4. **Heuristiques**: Solution sans ML si replay inaccessible
5. **Incrémental**: Améliorer progressivement vs solution complète

**Temps total révisé**: 
- Jour 1 (validations): 4-5h
- Jour 2 (solutions simples): 3-5h
- Jour 3+ (solutions avancées): Optionnel

**Probabilité succès**:
- Succès minimal: 95% (actions restreintes)
- Succès intermédiaire: 70% (heuristiques)
- Succès optimal: 40% (imitation learning)

**Recommandation**: Commencer par Jour 1 (validations) et décider ensuite.