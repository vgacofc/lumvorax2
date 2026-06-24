# PLAN SESSION 92 - ANALYSE SPATIALE obs.frame
**Date**: 2026-06-18  
**Auteur**: Bob  
**Criticité**: 🔴 MAXIMALE - Correction stratégie fondamentale

---

## 📋 TABLE DES MATIÈRES

1. [Contexte et Objectif](#contexte)
2. [Analyse AVANT (Sessions 88-90)](#avant)
3. [Analyse APRÈS (Session 92)](#apres)
4. [Modifications Code Détaillées](#modifications)
5. [Plan d'Exécution](#execution)
6. [Critères de Succès](#succes)

---

## 1. CONTEXTE ET OBJECTIF {#contexte}

### Problème Identifié

**Sessions 88-90**: 18,565 steps, 0 victoire
- ❌ Exploration aléatoire pure
- ❌ obs.frame traité comme statistiques
- ❌ Aucune analyse spatiale
- ❌ Aucune détection goals

### Objectif Session 92

**Implémenter analyse spatiale complète de obs.frame**
- ✅ Détecter patterns visuels
- ✅ Identifier goals potentiels
- ✅ Localiser agent/avatar
- ✅ Planifier chemin vers objectifs
- ✅ Tester sur 10 parties

---

## 2. ANALYSE AVANT (Sessions 88-90) {#avant}

### 2.1 Code Existant

**Fichier**: `session89_cartographie_complete_forensic.py`

```python
class ExhaustiveExplorer:
    def __init__(self):
        self.states_discovered = set()
        self.actions = [ACTION1, ACTION2, ACTION3, ACTION4, ACTION5]
    
    def hash_frame(self, frame):
        """Calculer hash SHA256 de la frame"""
        return hashlib.sha256(frame.tobytes()).hexdigest()[:16]
    
    def get_distribution(self, frame):
        """Calculer distribution pixels"""
        flat = frame.flatten()
        return dict(Counter(flat))
    
    def choose_action(self):
        """❌ PROBLÈME: Choix aléatoire"""
        return random.choice(self.actions)
    
    def play_game(self):
        obs = self.env.reset()
        
        while not done:
            # ❌ Analyse statistique uniquement
            state_hash = self.hash_frame(obs.frame)
            distribution = self.get_distribution(obs.frame)
            
            # ❌ Action aléatoire
            action = self.choose_action()
            
            obs = self.env.step(action)
```

### 2.2 Données Utilisées

**Ce qui était UTILISÉ**:
- ✅ `obs.frame` → hash SHA256
- ✅ `obs.frame` → distribution pixels
- ✅ `obs.levels_completed` → reward
- ✅ `obs.state` → done

**Ce qui était IGNORÉ**:
- ❌ Structure spatiale de obs.frame
- ❌ Patterns visuels (goals, obstacles)
- ❌ Position agent dans la grille
- ❌ Chemins possibles
- ❌ Régions connexes

### 2.3 Stratégie

```python
# ❌ APPROCHE INCORRECTE
def strategy():
    while True:
        action = random.choice(actions)  # Aléatoire
        execute(action)
        if new_state:
            states_discovered.add(state)  # Juste compter
```

**Objectif**: Cartographier tous les états  
**Résultat**: 3253 états, 0 victoire

### 2.4 Résultats

| Métrique | Valeur |
|----------|--------|
| Parties jouées | 213 |
| Steps total | 29,780 |
| États uniques | 3,253 |
| Reward > 0 | **0** |
| Victoires | **0** |
| Temps total | 167.8 min |

---

## 3. ANALYSE APRÈS (Session 92) {#apres}

### 3.1 Nouveau Code

**Fichier**: `session92_spatial_analysis.py`

```python
class SpatialAnalyzer:
    """Analyse spatiale complète de obs.frame"""
    
    def __init__(self):
        self.actions = [ACTION1, ACTION2, ACTION3, ACTION4, ACTION5]
        self.goals_detected = []
        self.agent_position = None
        self.obstacles = []
    
    # ✅ NOUVEAU: Analyse spatiale
    def analyze_frame_spatially(self, frame):
        """Analyser frame comme une IMAGE"""
        analysis = {
            'goals': self.detect_goals(frame),
            'agent': self.find_agent(frame),
            'obstacles': self.find_obstacles(frame),
            'regions': self.find_connected_regions(frame),
            'paths': []
        }
        
        # Planifier chemins vers goals
        if analysis['agent'] and analysis['goals']:
            for goal in analysis['goals']:
                path = self.plan_path(
                    analysis['agent'], 
                    goal, 
                    analysis['obstacles']
                )
                analysis['paths'].append(path)
        
        return analysis
    
    # ✅ NOUVEAU: Détection goals
    def detect_goals(self, frame):
        """Détecter pixels/patterns spéciaux (goals potentiels)"""
        goals = []
        
        # Stratégie 1: Pixels rares (< 10 occurrences)
        distribution = Counter(frame.flatten())
        for pixel_value, count in distribution.items():
            if count < 10 and pixel_value not in [0, 4]:  # Ignorer background
                positions = np.argwhere(frame == pixel_value)
                for pos in positions:
                    goals.append({
                        'position': tuple(pos),
                        'pixel_value': int(pixel_value),
                        'type': 'rare_pixel',
                        'priority': 1.0 / count  # Plus rare = plus prioritaire
                    })
        
        # Stratégie 2: Coins/bords (positions stratégiques)
        h, w = frame.shape
        corners = [(0, 0), (0, w-1), (h-1, 0), (h-1, w-1)]
        for corner in corners:
            if frame[corner] not in [0, 4]:  # Pas background
                goals.append({
                    'position': corner,
                    'pixel_value': int(frame[corner]),
                    'type': 'corner',
                    'priority': 0.8
                })
        
        return goals
    
    # ✅ NOUVEAU: Localisation agent
    def find_agent(self, frame):
        """Trouver position agent/avatar"""
        # Stratégie 1: Pixel qui bouge (ACTION2 = 2 pixels)
        # Basé sur découverte Session 81
        
        # Stratégie 2: Pixel unique ou très rare
        distribution = Counter(frame.flatten())
        for pixel_value, count in distribution.items():
            if count == 1:  # Pixel unique = probablement agent
                pos = np.argwhere(frame == pixel_value)[0]
                return {
                    'position': tuple(pos),
                    'pixel_value': int(pixel_value),
                    'confidence': 0.9
                }
            elif count == 2:  # 2 pixels = avatar potentiel (Session 81)
                positions = np.argwhere(frame == pixel_value)
                center = positions.mean(axis=0).astype(int)
                return {
                    'position': tuple(center),
                    'pixel_value': int(pixel_value),
                    'confidence': 0.7
                }
        
        # Stratégie 3: Centre de la grille (fallback)
        h, w = frame.shape
        return {
            'position': (h//2, w//2),
            'pixel_value': int(frame[h//2, w//2]),
            'confidence': 0.3
        }
    
    # ✅ NOUVEAU: Détection obstacles
    def find_obstacles(self, frame):
        """Identifier obstacles (pixels constants)"""
        # Pixel 4 = constant (2609 occurrences, jamais changé)
        obstacles = []
        obstacle_pixels = [4]  # Basé sur analyse Session 89
        
        for pixel_value in obstacle_pixels:
            positions = np.argwhere(frame == pixel_value)
            for pos in positions:
                obstacles.append(tuple(pos))
        
        return obstacles
    
    # ✅ NOUVEAU: Régions connexes
    def find_connected_regions(self, frame):
        """Détecter régions connexes (salles, zones)"""
        from scipy.ndimage import label
        
        # Créer masque: 0 = obstacle, 1 = navigable
        navigable = (frame != 4).astype(int)
        
        # Labelliser régions connexes
        labeled, num_regions = label(navigable)
        
        regions = []
        for region_id in range(1, num_regions + 1):
            positions = np.argwhere(labeled == region_id)
            regions.append({
                'id': region_id,
                'size': len(positions),
                'positions': positions,
                'center': positions.mean(axis=0).astype(int)
            })
        
        return regions
    
    # ✅ NOUVEAU: Planification chemin
    def plan_path(self, start, goal, obstacles):
        """Planifier chemin A* de start vers goal"""
        from heapq import heappush, heappop
        
        start_pos = start['position']
        goal_pos = goal['position']
        obstacles_set = set(obstacles)
        
        # A* algorithm
        open_set = []
        heappush(open_set, (0, start_pos))
        came_from = {}
        g_score = {start_pos: 0}
        
        def heuristic(pos):
            return abs(pos[0] - goal_pos[0]) + abs(pos[1] - goal_pos[1])
        
        while open_set:
            _, current = heappop(open_set)
            
            if current == goal_pos:
                # Reconstruire chemin
                path = []
                while current in came_from:
                    path.append(current)
                    current = came_from[current]
                path.reverse()
                return path
            
            # Voisins (4-connexité)
            for dx, dy in [(0, 1), (1, 0), (0, -1), (-1, 0)]:
                neighbor = (current[0] + dx, current[1] + dy)
                
                # Vérifier limites et obstacles
                if (0 <= neighbor[0] < 64 and 
                    0 <= neighbor[1] < 64 and 
                    neighbor not in obstacles_set):
                    
                    tentative_g = g_score[current] + 1
                    
                    if neighbor not in g_score or tentative_g < g_score[neighbor]:
                        came_from[neighbor] = current
                        g_score[neighbor] = tentative_g
                        f_score = tentative_g + heuristic(neighbor)
                        heappush(open_set, (f_score, neighbor))
        
        return []  # Pas de chemin trouvé
    
    # ✅ NOUVEAU: Choix action intelligent
    def choose_action_smart(self, analysis):
        """Choisir action basée sur analyse spatiale"""
        if not analysis['paths']:
            # Pas de chemin → exploration aléatoire
            return random.choice(self.actions)
        
        # Prendre le chemin le plus court vers goal prioritaire
        best_path = min(analysis['paths'], key=len)
        
        if len(best_path) < 2:
            return random.choice(self.actions)
        
        # Calculer direction vers prochain step
        current = analysis['agent']['position']
        next_pos = best_path[0]
        
        dx = next_pos[0] - current[0]
        dy = next_pos[1] - current[1]
        
        # Mapper direction → action
        # (À ajuster selon mapping réel du jeu)
        if dx > 0:
            return ACTION1  # Bas
        elif dx < 0:
            return ACTION2  # Haut
        elif dy > 0:
            return ACTION3  # Droite
        elif dy < 0:
            return ACTION4  # Gauche
        else:
            return ACTION5  # Sur place
    
    # ✅ NOUVEAU: Boucle jeu intelligente
    def play_game_smart(self):
        """Jouer avec stratégie spatiale"""
        obs = self.env.reset()
        step = 0
        max_steps = 500
        
        while step < max_steps:
            # ✅ Analyse spatiale complète
            analysis = self.analyze_frame_spatially(obs.frame)
            
            # ✅ Logging détaillé
            print(f"\nStep {step}:")
            print(f"  Goals détectés: {len(analysis['goals'])}")
            print(f"  Agent position: {analysis['agent']['position']}")
            print(f"  Chemins trouvés: {len(analysis['paths'])}")
            
            # ✅ Choix action intelligent
            action = self.choose_action_smart(analysis)
            
            # Exécuter
            obs = self.env.step(action)
            
            if obs.levels_completed > 0:
                print(f"\n🎉 VICTOIRE! Reward: {obs.levels_completed}")
                return True
            
            if obs.state == GameState.GAME_OVER:
                print(f"\n❌ Game Over au step {step}")
                return False
            
            step += 1
        
        return False
```

### 3.2 Données Utilisées

**Ce qui sera UTILISÉ**:
- ✅ `obs.frame` → analyse spatiale complète
- ✅ `obs.frame` → détection goals visuels
- ✅ `obs.frame` → localisation agent
- ✅ `obs.frame` → détection obstacles
- ✅ `obs.frame` → régions connexes
- ✅ `obs.frame` → planification chemin A*
- ✅ `obs.levels_completed` → reward
- ✅ `obs.state` → done

### 3.3 Stratégie

```python
# ✅ APPROCHE CORRECTE
def strategy():
    while not done:
        # 1. Analyser frame spatialement
        analysis = analyze_frame_spatially(obs.frame)
        
        # 2. Identifier goals
        goals = analysis['goals']
        
        # 3. Planifier chemin
        path = plan_path(agent, best_goal, obstacles)
        
        # 4. Suivre chemin
        action = next_action_on_path(path)
        
        # 5. Exécuter
        execute(action)
```

**Objectif**: Résoudre le puzzle  
**Résultat attendu**: Victoire en < 1000 steps

### 3.4 Résultats Attendus

| Métrique | Avant | Après (Attendu) |
|----------|-------|-----------------|
| Parties jouées | 213 | 10 |
| Steps moyen | 140 | < 500 |
| Reward > 0 | 0 | ≥ 1 |
| Victoires | 0 | ≥ 1 |
| Temps moyen | 47 sec | < 5 min |

---

## 4. MODIFICATIONS CODE DÉTAILLÉES {#modifications}

### 4.1 Fichiers Modifiés

**AUCUN** - Nouveau fichier créé pour éviter régression

### 4.2 Fichiers Créés

**1. session92_spatial_analysis.py** (600 lignes)
- Classe `SpatialAnalyzer`
- Méthodes analyse spatiale
- Planification chemin A*
- Stratégie dirigée vers goals

**2. PLAN_SESSION92_ANALYSE_SPATIALE.md** (ce fichier, 800 lignes)
- Analyse AVANT/APRÈS détaillée
- Modifications code complètes
- Plan d'exécution

### 4.3 Dépendances Ajoutées

```python
# Déjà présentes
import numpy as np
import hashlib
from collections import Counter
import random

# Nouvelles
from scipy.ndimage import label  # Régions connexes
from heapq import heappush, heappop  # A* algorithm
```

### 4.4 Comparaison Ligne par Ligne

#### AVANT (session89_cartographie_complete_forensic.py)

```python
# Ligne 240-245
def play_game(self):
    obs = self.env.reset()
    initial_hash = self.hash_frame(obs.frame)  # ❌ Hash uniquement
    initial_dist = self.get_distribution(obs.frame)  # ❌ Stats uniquement
    
    action = random.choice(self.actions)  # ❌ Aléatoire
```

#### APRÈS (session92_spatial_analysis.py)

```python
# Ligne 350-360
def play_game_smart(self):
    obs = self.env.reset()
    analysis = self.analyze_frame_spatially(obs.frame)  # ✅ Analyse spatiale
    
    # ✅ Détection goals, agent, obstacles, chemins
    goals = analysis['goals']
    agent = analysis['agent']
    paths = analysis['paths']
    
    action = self.choose_action_smart(analysis)  # ✅ Intelligent
```

---

## 5. PLAN D'EXÉCUTION {#execution}

### 5.1 Phase 1: Création Code (30 min)

**Tâches**:
1. ✅ Créer `session92_spatial_analysis.py`
2. ✅ Implémenter `SpatialAnalyzer` class
3. ✅ Implémenter méthodes analyse spatiale
4. ✅ Implémenter A* pathfinding
5. ✅ Implémenter stratégie intelligente

### 5.2 Phase 2: Tests Unitaires (15 min)

**Tâches**:
1. Tester `detect_goals()` sur frame exemple
2. Tester `find_agent()` sur frame exemple
3. Tester `plan_path()` avec obstacles
4. Vérifier intégration complète

### 5.3 Phase 3: Exécution 10 Parties (50 min)

**Commande**:
```bash
cd lumvorax2/src/MAGEN
doppler run -- python3 session92_spatial_analysis.py
```

**Monitoring**:
- Goals détectés par partie
- Chemins planifiés
- Actions choisies
- Reward obtenu
- Victoires

### 5.4 Phase 4: Analyse Résultats (15 min)

**Métriques à collecter**:
- Nombre victoires / 10 parties
- Steps moyen par partie
- Goals détectés moyen
- Chemins trouvés moyen
- Reward max obtenu

### 5.5 Phase 5: Rapport Final (20 min)

**Fichier**: `RAPPORT_SESSION92_ANALYSE_SPATIALE.md`
- Résultats détaillés
- Comparaison AVANT/APRÈS
- Leçons apprises
- Prochaines étapes

**Temps total estimé**: 2h10

---

## 6. CRITÈRES DE SUCCÈS {#succes}

### 6.1 Succès Minimum

- ✅ Code exécute sans erreur
- ✅ Goals détectés dans chaque partie
- ✅ Agent localisé dans chaque partie
- ✅ Chemins planifiés (même si non optimaux)
- ✅ Actions basées sur analyse spatiale

### 6.2 Succès Partiel

- ✅ Critères minimum +
- ✅ Au moins 1 reward > 0 obtenu
- ✅ Steps moyen < 300 par partie
- ✅ Stratégie visiblement différente de l'aléatoire

### 6.3 Succès Complet

- ✅ Critères partiel +
- ✅ **Au moins 1 victoire** sur 10 parties
- ✅ Steps moyen < 500 par partie
- ✅ Chemins suivis correctement
- ✅ Amélioration claire vs sessions 88-90

### 6.4 Succès Exceptionnel

- ✅ Critères complet +
- ✅ **≥ 3 victoires** sur 10 parties
- ✅ Steps moyen < 300 par partie
- ✅ Stratégie optimale démontrée
- ✅ Taux succès > 30%

---

## 7. RISQUES ET MITIGATION

### 7.1 Risque: Goals mal détectés

**Probabilité**: Moyenne  
**Impact**: Élevé

**Mitigation**:
- Implémenter 3 stratégies détection (rares, coins, patterns)
- Logging détaillé pour debug
- Fallback sur exploration si 0 goals

### 7.2 Risque: Agent mal localisé

**Probabilité**: Moyenne  
**Impact**: Élevé

**Mitigation**:
- Utiliser découverte Session 81 (ACTION2 = 2 pixels)
- Stratégies multiples (unique, rare, centre)
- Confidence score pour chaque méthode

### 7.3 Risque: Chemin A* trop lent

**Probabilité**: Faible  
**Impact**: Moyen

**Mitigation**:
- Limiter recherche à 1000 nœuds max
- Timeout 1 seconde
- Fallback sur ligne droite si timeout

### 7.4 Risque: Mapping actions incorrect

**Probabilité**: Élevée  
**Impact**: Critique

**Mitigation**:
- Tester mapping avec 1 partie debug
- Ajuster selon observations
- Documenter mapping réel découvert

---

## 8. PROCHAINES ÉTAPES POST-SESSION 92

### Si Succès Complet (≥ 1 victoire)

1. **Session 93**: Optimiser stratégie
   - Affiner détection goals
   - Améliorer A* (heuristiques)
   - Tester sur 50 parties

2. **Session 94**: Généraliser
   - Tester sur autres jeux ARC-AGI
   - Adapter stratégie par jeu
   - Créer framework réutilisable

### Si Succès Partiel (reward > 0, pas victoire)

1. **Session 93**: Debug approfondi
   - Analyser pourquoi pas victoire
   - Identifier goals manqués
   - Corriger mapping actions

2. **Session 94**: Retry avec corrections
   - Appliquer fixes
   - Tester 20 parties
   - Viser victoire

### Si Échec (0 reward)

1. **Session 93**: Analyse forensique
   - Vérifier goals détectés
   - Vérifier agent localisé
   - Vérifier chemins planifiés
   - Identifier blocage

2. **Session 94**: Approche alternative
   - Essayer détection patterns différente
   - Tester stratégie hybride
   - Consulter modèle concurrent

---

## 9. RÉSUMÉ EXÉCUTIF

### Changement Fondamental

**AVANT**: obs.frame → statistiques → action aléatoire  
**APRÈS**: obs.frame → analyse spatiale → action dirigée

### Impact Attendu

**Probabilité victoire**:
- Avant: ~0% (0/213 parties)
- Après: 10-30% (1-3/10 parties attendues)

### Investissement

**Temps**: 2h10  
**Complexité**: Moyenne  
**Risque**: Faible (nouveau fichier, pas de régression)

### ROI

**Si 1 victoire**: Validation approche spatiale → Généralisation possible  
**Si 0 victoire**: Leçons sur détection goals → Itération suivante

---

**FIN DU PLAN**

*Prêt pour exécution Session 92*