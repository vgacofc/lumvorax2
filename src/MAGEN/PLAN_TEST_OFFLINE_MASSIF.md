# PLAN TEST OFFLINE MASSIF - MAGEN V28/V30
**Date** : 2026-06-15 02:24 UTC  
**Découverte** : Mode OFFLINE ARC-AGI-3 (2000 FPS, no limits)

---

## OPPORTUNITÉ DÉCOUVERTE

**Documentation ARC-AGI-3 révèle** :

```python
from arc_agi import Arcade, OperationMode

# Mode OFFLINE (recommandé développement)
arc = Arcade(operation_mode=OperationMode.OFFLINE)
env = arc.make("ls20", render_mode="terminal")
```

**Avantages vs COMPETITION/ONLINE** :

| Métrique | ONLINE/COMPETITION | OFFLINE |
|----------|-------------------|---------|
| **FPS** | ~10 FPS | **~2,000 FPS** |
| **Frames/min** | ~600 | **~120,000** |
| **Rate limit** | 600 RPM | **Aucune** |
| **Instances parallèles** | Limitées | **Illimitées** |
| **API key** | Requis | **Pas requis** |
| **Scorecards** | Oui | Non (local) |

**Implication** : On peut tester **TOUS les 400 puzzles** localement sans contraintes !

---

## PLAN IMMÉDIAT

### Phase 1 : Validation Mode OFFLINE (30 min)

**Objectif** : Confirmer que mode OFFLINE fonctionne avec MAGEN

**Actions** :
1. Modifier `test_v30_arc_arcade.py` pour utiliser `OperationMode.OFFLINE`
2. Tester sur 3 jeux (cd82, ft09, ls20)
3. Vérifier vitesse (devrait être ~200x plus rapide)
4. Confirmer pas de rate limits

**Script** :
```python
from arc_agi import Arcade, OperationMode

# OFFLINE mode
arc = Arcade(operation_mode=OperationMode.OFFLINE)

# Test rapide
env = arc.make("ls20", render_mode="terminal")
```

### Phase 2 : Test V28.2.2 sur 400 Puzzles (2-3h)

**Objectif** : Baseline complète V28.2.2 sur TOUS les puzzles

**Actions** :
1. Créer script `test_v28_offline_400.py`
2. Lister tous les jeux disponibles : `arc.get_environments()`
3. Tester V28.2.2 sur chaque jeu (max 50 steps)
4. Logger résultats forensic complets
5. Calculer score final

**Estimation temps** :
- 400 puzzles × 50 steps × 0.5ms/step = 10,000 steps = **5 secondes**
- Avec overhead (reset, logging) : **~10-15 minutes**

**Métriques attendues** :
- Succès total : ~170-180/400 (42-45%)
- Distribution par catégorie
- Temps moyen par puzzle
- Patterns d'échec

### Phase 3 : Décision V30 Repair vs Abandon (1h)

**Basé sur résultats V28 offline** :

**Si V28 offline = V28 online (~42.9%)** :
→ Confirme que mode test n'affecte pas résultats
→ Peut tester V30 repair en confiance

**Si V28 offline >> V28 online** :
→ Mode online était bottleneck
→ Re-tester V30 en offline avant abandon

**Actions** :
1. Analyser distribution succès V28 offline
2. Identifier patterns échec récurrents
3. Décider si V30 repair vaut le coût (2-3 jours)
4. Ou passer directement transformations symboliques

### Phase 4 : Test V30 Repair (si décision positive)

**Corrections prioritaires** :
1. Bug stagnation : utiliser `reward` au lieu de `distance(state, next_state)`
2. Condition rupture : `collapse OR stagnation` au lieu de `AND`
3. Discrétisation : grid 16x16 au lieu de 4x4
4. Héritage : V28.2.2 au lieu de V28.0

**Test rapide** :
- 10 puzzles représentatifs
- Validation métriques TVT actives
- Si succès > V28 : continuer
- Si échec : abandonner définitivement

---

## SCRIPTS À CRÉER

### 1. `test_v28_offline_400.py`

```python
"""Test V28.2.2 sur TOUS les puzzles en mode OFFLINE"""

from arc_agi import Arcade, OperationMode
from decision_kernel_v28_2_2 import DecisionKernelV28_2_2
import json
from datetime import datetime

def test_v28_offline_all():
    """Test V28 sur tous les puzzles"""
    
    # Mode OFFLINE (2000 FPS, no limits)
    arc = Arcade(operation_mode=OperationMode.OFFLINE)
    
    # Liste tous les jeux
    games = arc.get_environments()
    print(f"Found {len(games)} games")
    
    results = []
    
    for i, game_info in enumerate(games):
        game_id = game_info.game_id
        print(f"\n[{i+1}/{len(games)}] Testing {game_id}...")
        
        # Test V28
        result = run_v28_on_game(arc, game_id)
        results.append(result)
        
        # Log progress
        success_rate = sum(1 for r in results if r['success']) / len(results)
        print(f"  Success rate so far: {success_rate:.1%}")
    
    # Save results
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"test_v28_offline_400_{timestamp}.json"
    
    with open(filename, 'w') as f:
        json.dump({
            'timestamp': timestamp,
            'total_games': len(games),
            'results': results,
            'success_rate': sum(1 for r in results if r['success']) / len(results)
        }, f, indent=2)
    
    print(f"\nResults saved: {filename}")
    return results

def run_v28_on_game(arc, game_id, max_steps=50):
    """Run V28 on single game"""
    
    env = arc.make(game_id, render_mode=None)  # No render for speed
    kernel = DecisionKernelV28_2_2()
    
    state = env.reset()
    total_reward = 0
    
    for step in range(max_steps):
        # Select action
        action = kernel.select_action(state, env.action_space)
        
        # Take step
        next_state, reward, done, info = env.step(action)
        total_reward += reward
        
        # Update kernel
        kernel.update_after_step(state, action, reward, next_state)
        
        state = next_state
        
        if done:
            break
    
    return {
        'game_id': game_id,
        'success': info.get('game_state') == 'WIN',
        'steps': step + 1,
        'total_reward': total_reward,
        'final_state': info.get('game_state'),
        'levels_completed': info.get('levels_completed', 0)
    }

if __name__ == "__main__":
    results = test_v28_offline_all()
    
    # Print summary
    success_count = sum(1 for r in results if r['success'])
    print(f"\n{'='*80}")
    print(f"FINAL RESULTS")
    print(f"{'='*80}")
    print(f"Total games: {len(results)}")
    print(f"Success: {success_count}/{len(results)} ({success_count/len(results):.1%})")
```

### 2. `test_v30_repair_quick.py`

```python
"""Test V30 réparé sur 10 puzzles représentatifs"""

# Similar structure but with V30 repairs applied
# Test on 10 diverse puzzles before full 400 test
```

---

## DÉCISION IMMÉDIATE

**Je recommande** : Exécuter Phase 1 + Phase 2 **MAINTENANT**

**Raisons** :
1. Mode OFFLINE change complètement la donne (200x plus rapide)
2. Test 400 puzzles prend ~15 minutes (pas 3 jours)
3. Baseline V28 complète permettra décision éclairée sur V30
4. Pas de coût API, pas de rate limits

**Action** : Créer et exécuter `test_v28_offline_400.py` immédiatement

---

## TIMELINE RÉVISÉE

**Avant découverte OFFLINE** :
- Test 400 puzzles : 3-5 jours (rate limits)
- Coût : API calls massifs
- Risque : timeouts, errors

**Après découverte OFFLINE** :
- Test 400 puzzles : **15 minutes**
- Coût : **0€**
- Risque : **Aucun**

**Gain** : **~300x plus rapide** + **0 coût** + **0 risque**

---

## PROCHAINE ACTION

**IMMÉDIAT** : Créer `test_v28_offline_400.py` et lancer test complet

**Attente** : ~15 minutes pour résultats complets

**Puis** : Décision éclairée V30 repair vs abandon basée sur données réelles

---

**Statut** : 🚀 Prêt à exécuter test massif OFFLINE