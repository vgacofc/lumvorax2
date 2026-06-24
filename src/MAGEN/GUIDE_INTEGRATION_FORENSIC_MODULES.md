# GUIDE INTÉGRATION FORENSIC - MODULES MAGEN

**Date**: 2026-06-14T22:18:00+02:00  
**Auteur**: Bob (Mode Advanced)  
**Conforme**: PROTOCOLE_MAGEN V3.0 - FORENSIC TOTAL

---

## OBJECTIF

Intégrer forensic dans les 8 modules MAGEN avec **ZÉRO duplication structurelle** grâce au middleware standardisé.

---

## ARCHITECTURE MIDDLEWARE

### Composants Disponibles

1. **ForensicMiddleware**: Décorateur générique pour méthodes
2. **ForensicMixin**: Mixin pour classes (héritage)
3. **create_forensic_wrapper**: Factory pour wrapper classes existantes

### Tests Validation

✅ **26/26 tests PASSED** (16 forensic_logger + 10 forensic_middleware)
- Infrastructure forensic: 100% validée
- Middleware générique: 100% validé
- Prêt pour intégration modules

---

## MÉTHODE 1: DÉCORATEUR (Recommandé pour nouveaux modules)

### Avantages
- ✅ Minimal invasif
- ✅ Contrôle granulaire
- ✅ Facile à désactiver

### Template

```python
from core.forensic_middleware import ForensicMiddleware

class MyModule:
    def __init__(self, log_path: str):
        # Setup forensic
        self.forensic = ForensicMiddleware(log_path, "MyModule")
    
    @property
    def _state(self):
        """État à capturer"""
        return {
            'counter': self.counter,
            'status': self.status
        }
    
    @forensic.instrument(
        capture_state=True,
        state_getter=lambda self: self._state
    )
    def my_method(self, arg1, arg2):
        """Méthode instrumentée automatiquement"""
        # Implémentation
        return result
    
    def close(self):
        self.forensic.close()
```

### Exemple Complet

```python
from core.forensic_middleware import ForensicMiddleware
import numpy as np

class StateEncoderV25:
    def __init__(self, log_path: str = "logs/encoder.jsonl"):
        self.forensic = ForensicMiddleware(log_path, "StateEncoderV25")
        self.previous_state = None
        self.previous_embedding = None
    
    @property
    def _state(self):
        return {
            'has_previous_state': self.previous_state is not None,
            'has_previous_embedding': self.previous_embedding is not None
        }
    
    @forensic.instrument(
        operation_name='encode',
        capture_state=True,
        state_getter=lambda self: self._state
    )
    def encode(self, state, action=None):
        """Encoder état avec forensic automatique"""
        # Implémentation encoding
        embedding = self._encode_internal(state, action)
        
        # Mettre à jour état
        self.previous_state = state
        self.previous_embedding = embedding
        
        return embedding
    
    def _encode_internal(self, state, action):
        """Implémentation réelle (non instrumentée)"""
        # Logic encoding
        return np.random.rand(256)
    
    def close(self):
        self.forensic.close()
```

---

## MÉTHODE 2: MIXIN (Recommandé pour refactoring modules existants)

### Avantages
- ✅ Intégration propre par héritage
- ✅ API standardisée
- ✅ Gestion épisodes/transitions intégrée

### Template

```python
from core.forensic_middleware import ForensicMixin

class MyModule(ForensicMixin):
    def __init__(self, log_path: str):
        # Setup forensic
        self.setup_forensic(log_path, "MyModule")
        
        # Initialisation module
        self.counter = 0
    
    @property
    def _forensic_state(self):
        """Override pour fournir état"""
        return {
            'counter': self.counter
        }
    
    def my_method(self, arg):
        """Méthode avec forensic"""
        return self._forensic_call(
            'my_method',
            lambda: self._my_method_impl(arg)
        )
    
    def _my_method_impl(self, arg):
        """Implémentation réelle"""
        self.counter += 1
        return self.counter
```

### Exemple Complet

```python
from core.forensic_middleware import ForensicMixin

class AgentV27(ForensicMixin):
    def __init__(self, log_path: str = "logs/agent_v27.jsonl"):
        # Setup forensic
        self.setup_forensic(log_path, "AgentV27")
        
        # Initialisation agent
        self.state = None
        self.action_count = 0
    
    @property
    def _forensic_state(self):
        return {
            'action_count': self.action_count,
            'has_state': self.state is not None
        }
    
    def perceive(self, observation):
        """Percevoir avec forensic"""
        return self._forensic_call(
            'perceive',
            lambda: self._perceive_impl(observation)
        )
    
    def decide(self):
        """Décider avec forensic"""
        return self._forensic_call(
            'decide',
            lambda: self._decide_impl()
        )
    
    def learn(self, reward):
        """Apprendre avec forensic"""
        return self._forensic_call(
            'learn',
            lambda: self._learn_impl(reward)
        )
    
    def _perceive_impl(self, observation):
        """Implémentation perception"""
        self.state = observation
        return self.state
    
    def _decide_impl(self):
        """Implémentation décision"""
        self.action_count += 1
        return {'action': 'move', 'direction': 'up'}
    
    def _learn_impl(self, reward):
        """Implémentation apprentissage"""
        return {'loss': 0.1, 'reward': reward}
```

---

## MÉTHODE 3: FACTORY WRAPPER (Recommandé pour modules legacy sans modification)

### Avantages
- ✅ ZÉRO modification code existant
- ✅ Wrapper externe
- ✅ Idéal pour modules tiers

### Template

```python
from core.forensic_middleware import create_forensic_wrapper

# Classe existante (non modifiée)
class ExistingModule:
    def method1(self, arg):
        return result
    
    def method2(self, arg):
        return result

# Créer wrapper forensic
ExistingModuleForensic = create_forensic_wrapper(
    ExistingModule,
    log_path="logs/existing.jsonl",
    component_name="ExistingModule",
    methods_to_instrument=["method1", "method2"]
)

# Utiliser wrapper
module = ExistingModuleForensic()
result = module.method1(arg)  # Automatiquement instrumenté
```

### Exemple Complet

```python
from core.forensic_middleware import create_forensic_wrapper, CausalIDManager

# Module existant (legacy)
class BehavioralEntityDetector:
    def __init__(self):
        self.entities = []
    
    def observe(self, grid, action=None):
        # Implémentation existante
        return {'entities': self.entities}
    
    def update(self, grid):
        # Implémentation existante
        self.entities.append({'type': 'agent'})

# Créer wrapper forensic SANS modifier code original
BehavioralEntityDetectorForensic = create_forensic_wrapper(
    BehavioralEntityDetector,
    log_path="logs/behavioral_detector.jsonl",
    component_name="BehavioralEntityDetector",
    methods_to_instrument=["observe", "update"]
)

# Utiliser exactement comme l'original
detector = BehavioralEntityDetectorForensic()
result = detector.observe(grid)  # Forensic automatique!
detector.update(grid)  # Forensic automatique!
```

---

## GESTION ÉPISODES/TRANSITIONS

### Hiérarchie Causale

```
Session (UUID v4) - Créé automatiquement à l'init
  └─ Episode (UUID v4) - Créer manuellement
      └─ Transition (UUID v4) - Créer manuellement
          └─ Operation (UUID v4) - Créé automatiquement par instrument()
```

### Exemple Usage

```python
class AgentV27(ForensicMixin):
    def __init__(self, log_path: str):
        self.setup_forensic(log_path, "AgentV27")
    
    def run_episode(self, env):
        # Créer nouvel épisode
        episode_id = self._forensic_new_episode()
        
        for step in range(max_steps):
            # Créer nouvelle transition
            transition_id = self._forensic_new_transition()
            
            # Opérations (operations créées automatiquement)
            obs = self.perceive(env.observe())
            action = self.decide()
            reward = env.step(action)
            self.learn(reward)
```

---

## ID MANAGER PARTAGÉ (Multi-modules)

### Pourquoi Partager?

- ✅ Session ID unique pour tous modules
- ✅ Corrélation logs inter-modules
- ✅ Replay cohérent multi-modules

### Template

```python
from core.forensic_middleware import ForensicMiddleware, CausalIDManager

# Créer ID manager partagé
shared_id_manager = CausalIDManager()

# Module 1
encoder = StateEncoderV25(
    log_path="logs/encoder.jsonl",
    id_manager=shared_id_manager
)

# Module 2
agent = AgentV27(
    log_path="logs/agent.jsonl",
    id_manager=shared_id_manager
)

# Tous logs auront même session_id!
```

---

## ORDRE INTÉGRATION RECOMMANDÉ

### Phase 1: Modules P0 (Critiques)

1. **State Encoder V25** (Méthode 1: Décorateur)
   - Méthodes: `encode()`, `decode()`
   - État: `previous_state`, `previous_embedding`
   - Priorité: P0

2. **Agent V27** (Méthode 2: Mixin)
   - Méthodes: `perceive()`, `decide()`, `learn()`
   - État: `state`, `action_count`, `total_reward`
   - Priorité: P0

3. **Agent V27.1** (Méthode 2: Mixin)
   - Méthodes: `perceive()`, `decide()`, `learn()`
   - État: `state`, `action_count`, `total_reward`
   - Priorité: P0

### Phase 2: Modules P1 (Importantes)

4. **Behavioral Entity Detector** (Méthode 3: Factory)
   - Méthodes: `observe()`, `update()`
   - Priorité: P1

5. **Causal Graph Manager** (Méthode 3: Factory)
   - Méthodes: `add_transition()`, `query()`
   - Priorité: P1

### Phase 3: Modules P2 (Secondaires)

6. **Goal Manager** (Méthode 3: Factory)
7. **Memory Manager** (Méthode 3: Factory)
8. **Action Discovery Engine** (Méthode 3: Factory)

---

## CHECKLIST INTÉGRATION

### Pour Chaque Module

- [ ] Choisir méthode intégration (1, 2 ou 3)
- [ ] Identifier méthodes à instrumenter
- [ ] Définir état à capturer
- [ ] Implémenter forensic
- [ ] Créer tests unitaires
- [ ] Valider logs générés
- [ ] Vérifier hash chain intégrité
- [ ] Documenter intégration

### Validation Forensic

```python
# Test intégrité hash chain
assert module.forensic.verify_integrity() == True

# Test logs générés
import json
with open(log_path) as f:
    logs = [json.loads(line) for line in f]
    
    # Vérifier structure
    assert all('causal_context' in log for log in logs)
    assert all('timestamp_ns' in log for log in logs)
    assert all('entry_hash' in log for log in logs)
    
    # Vérifier hash chain
    for i in range(1, len(logs)):
        assert logs[i]['previous_hash'] == logs[i-1]['entry_hash']
```

---

## DÉSACTIVATION FORENSIC (Benchmarks)

### Temporaire

```python
# Désactiver
module.forensic.disable()

# Benchmark
result = module.method()

# Réactiver
module.forensic.enable()
```

### Permanente

```python
# Ne pas créer middleware
class MyModule:
    def __init__(self, enable_forensic: bool = True):
        if enable_forensic:
            self.forensic = ForensicMiddleware(...)
        else:
            self.forensic = None
```

---

## ERREURS COMMUNES

### ❌ Erreur 1: Duplication Code

```python
# MAUVAIS: Copier-coller pattern dans chaque module
class Module1:
    def method(self):
        start = time.time()
        log_before()
        result = impl()
        log_after()
        return result

class Module2:
    def method(self):
        start = time.time()  # Duplication!
        log_before()
        result = impl()
        log_after()
        return result
```

```python
# BON: Utiliser middleware
class Module1:
    @forensic.instrument()
    def method(self):
        return impl()

class Module2:
    @forensic.instrument()
    def method(self):
        return impl()
```

### ❌ Erreur 2: Oublier Fermer Logger

```python
# MAUVAIS
module = MyModule()
# ... utilisation
# Oubli close() → fichier non fermé

# BON
module = MyModule()
try:
    # ... utilisation
finally:
    module.forensic.close()
```

### ❌ Erreur 3: Capturer État Mutable

```python
# MAUVAIS: Référence mutable
@property
def _state(self):
    return {'data': self.data}  # Si self.data est mutable!

# BON: Copie profonde
@property
def _state(self):
    return {'data': copy.deepcopy(self.data)}
```

---

## MÉTRIQUES SUCCÈS

### Par Module

- ✅ Forensic activé: `module.forensic.enabled == True`
- ✅ Logs générés: `os.path.exists(log_path)`
- ✅ Hash chain valide: `module.forensic.verify_integrity() == True`
- ✅ Causal IDs présents: Tous logs ont `causal_context`
- ✅ Timestamps doubles: Tous logs ont `timestamp_ns` + `timestamp_wall_ns`

### Global

- ✅ 8/8 modules avec forensic
- ✅ 0 duplication code
- ✅ Tests intégration 100% PASSED
- ✅ Performance acceptable (<10% overhead)

---

## SUPPORT

### Documentation
- [`PROTOCOLE_MAGEN.md V3.0`](PROTOCOLE_MAGEN.md)
- [`core/forensic_logger.py`](core/forensic_logger.py)
- [`core/forensic_middleware.py`](core/forensic_middleware.py)

### Tests
- [`tests/test_forensic_logger.py`](tests/test_forensic_logger.py) (16 tests)
- [`tests/test_forensic_middleware.py`](tests/test_forensic_middleware.py) (10 tests)

### Exemples
- Voir sections "Exemple Complet" ci-dessus

---

**Signature**: Bob (Mode Advanced)  
**Date**: 2026-06-14T22:18:00+02:00  
**Version**: 1.0  
**Status**: ✅ GUIDE COMPLET - PRÊT POUR INTÉGRATION