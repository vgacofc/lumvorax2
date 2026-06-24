# PROTOCOLE MAGEN - RÈGLES ABSOLUES
## Mémoire Artificielle GEN — Protocole Exécution Obligatoire

**Version**: 3.1.0
**Date création**: 2026-06-14T16:37:00+02:00
**Date mise à jour**: 2026-06-17T16:55:00+02:00
**Statut**: ⚡ ACTIF - MODE COMPÉTITION + FORENSIC TOTAL + WORLD UNDERSTANDING
**Objectif**: Score >0/3 sur jeux ARC-AGI-3 arcade
**Principe Fondamental**: **FORENSIC TOTAL, TOUJOURS, SANS COMPROMIS**

### RÈGLE #0: RAPPORTS DANS FICHIERS MD DÉDIÉS
**Obligation**: TOUS rapports/résultats dans fichiers MD, JAMAIS dans attempt_completion
- ✅ Créer/mettre à jour fichier `AVANCEMENT_SESSION[N]_TEMPS_REEL.md`
- ✅ Ajouter nouveau contenu EN FIN de fichier (insert_content line 0)
- ✅ JAMAIS supprimer/modifier rapports précédents
- ✅ Utiliser attempt_completion UNIQUEMENT pour résumé ultra-court
- ❌ INTERDIT: Mettre rapports détaillés dans attempt_completion
- ❌ INTERDIT: Supprimer/modifier sections précédentes du fichier MD
- ❌ INTERDIT: Créer nouveau fichier MD pour chaque rapport

**Format Obligatoire**:
```markdown
## [TITRE SECTION] (YYYY-MM-DD HH:MM)

### Sous-section 1
Contenu...

### Sous-section 2
Contenu...

---
```

**Exemple**:
```bash
# Ajouter nouveau rapport en fin de fichier
insert_content(
  path="AVANCEMENT_SESSION80_TEMPS_REEL.md",
  line=0,  # 0 = fin de fichier
  content="## NOUVEAU RAPPORT...\n..."
)
```

---

## 🔴 RÈGLES ABSOLUES NON-NÉGOCIABLES

### RÈGLE #1: CORRECTION AUTOMATIQUE ERREURS
**Obligation**: Corriger TOUTES erreurs/warnings automatiquement après lecture logs forensiques
- ✅ Lire logs forensiques ligne par ligne
- ✅ Identifier cause racine erreur
- ✅ Appliquer correction immédiate
- ✅ Relancer test validation
- ❌ INTERDIT: Produire rapport avec erreurs non corrigées
- ❌ INTERDIT: Demander validation utilisateur pour corrections évidentes

### RÈGLE #2: AUCUNE DETTE TECHNIQUE TOLÉRÉE
**Obligation**: 0 warning, 0 erreur, 0 TODO non résolu
- ✅ Corriger warnings au fur et à mesure
- ✅ Résoudre TODOs immédiatement
- ✅ Refactorer code problématique
- ❌ INTERDIT: Laisser dette technique "pour plus tard"
- ❌ INTERDIT: Commenter code cassé au lieu de corriger

### RÈGLE #3: NE PAS S'ARRÊTER AVANT RÉSULTATS POSITIFS
**Obligation**: Continuer itérations jusqu'à score >0/3 sur jeux arcade
- ✅ Tester → Analyser → Corriger → Retester (boucle)
- ✅ Itérations rapides (30min max par cycle)
- ✅ Validation continue résultats
- ❌ INTERDIT: S'arrêter sur échec
- ❌ INTERDIT: Produire rapport final sans résultats positifs

### RÈGLE #4: LECTURE COMPLÈTE RAPPORTS EXISTANTS
**Obligation**: Lire TOUS rapports ligne par ligne AVANT toute modification
- ✅ Lister tous fichiers `/home/lvx/LVX/lumvorax2/src/MAGEN/*.md`
- ✅ Lire chaque rapport de la première à la dernière ligne
- ✅ Intégrer TOUTES leçons/insights dans version actuelle
- ✅ Vérifier aucune fonctionnalité oubliée
- ❌ INTERDIT: Sauter lignes ou sections
- ❌ INTERDIT: Réinventer solutions déjà documentées

### RÈGLE #5: NOTIFICATION EXPERTISES TEMPS RÉEL
**Obligation**: Notifier expertises mobilisées à chaque étape
- ✅ Déclarer domaines expertise au début
- ✅ Mettre à jour liste expertises si nouveaux domaines
- ✅ Expliquer pourquoi expertise nécessaire
- Format: `**Expertises**: Domain1, Domain2, Domain3`

### RÈGLE #6: PROGRESSION TEMPS RÉEL (%)
**Obligation**: Afficher progression % à chaque message
- ✅ Calculer % avancement tâche globale
- ✅ Afficher au début de chaque message
- ✅ Expliquer ce qui est fait/reste à faire
- Format: `**Progression**: X% - Description étape actuelle`

### RÈGLE #7: THINKING EN FRANÇAIS
**Obligation**: Expliquer raisonnement en français à chaque étape
- ✅ Décrire analyse en cours
- ✅ Expliquer décisions prises
- ✅ Justifier choix techniques
- ❌ INTERDIT: Sauter étapes raisonnement

### RÈGLE #8: FORENSIC TOTAL NON-NÉGOCIABLE
**Obligation**: JAMAIS diminuer logs forensic, TOUJOURS ajouter plus
- ✅ Traçabilité complète bit-level + nanoseconde
- ✅ Hash chain blockchain-style (intégrité)
- ✅ Causal IDs hiérarchiques (session→episode→transition→operation)
- ✅ Timestamps doubles (monotonic + wall clock)
- ✅ Sérialisation complète sans perte (pas de sample/truncation)
- ✅ Détection erreurs silencieuses (NaN, loops, collapse)
- ✅ Mode REPLAY obligatoire (exact/accéléré/différentiel/causal)
- ❌ INTERDIT: Réduire logs pour performance/stockage/latence
- ❌ INTERDIT: Utiliser `hash()` Python (non stable, non cryptographique)
- ❌ INTERDIT: Sample ou truncation des données

### RÈGLE #9: MODE COMPÉTITION AUTORISÉ
**Obligation**: Optimiser pour performance compétition ARC-AGI-3
- ✅ Prioriser score >0/3 sur jeux arcade
- ✅ Itérations rapides validation locale
- ✅ Soumission Kaggle après validation
- ✅ Analyse compétitive vs autres agents

### RÈGLE #10: ADAPTATION ARC-AGI-3 COMPLÈTE
**Obligation**: Fonctionner sur puzzles statiques ET jeux arcade
- ✅ Architecture unifiée (V22+V23+V24+V25)
- ✅ Tests sur puzzles ARC classiques
- ✅ Tests sur jeux arcade interactifs
- ✅ Validation double modalité

### RÈGLE #11: REPRÉSENTATION AVANT STRATÉGIE (CRITIQUE)
**Obligation**: Construire représentation persistante du monde AVANT optimiser stratégie
- ✅ Identifier avatar (confiance > 80%)
- ✅ Tracker position agent (95%+ steps)
- ✅ Construire graphe navigation (5+ salles)
- ✅ Mesurer causalité action→conséquence (100+ liens)
- ✅ Logs forensiques DOIVENT contenir métriques monde
- ❌ INTERDIT: Optimiser stratégie sans représentation monde
- ❌ INTERDIT: Logs avec seulement métriques infrastructure

**Principe Fondamental**:
> "Infrastructure sophistiquée ≠ Compréhension du monde"

**Ordre Critique**:
```
1. REPRÉSENTATION (connaissance du monde) ← PRIORITÉ P0
   ↓
2. STRATÉGIE (décision optimale)
```

**Métriques Monde Obligatoires dans Logs**:
```python
{
  "world_understanding": {
    "positions_tracked": int,        # Nombre positions agent trackées
    "spatial_coverage": float,       # % monde découvert (0.0-1.0)
    "rooms_memorized": int,          # Nombre salles visitées
    "navigation_graph": {            # Graphe navigation construit
      "nodes": int,                  # Nombre salles
      "edges": int                   # Nombre transitions
    },
    "avatar_identified": bool,       # Avatar identifié
    "avatar_confidence": float,      # Confiance identification (0.0-1.0)
    "causal_links": int             # Nombre liens causaux détectés
  }
}
```

**Validation Obligatoire**:
MAGEN DOIT pouvoir répondre à ces 5 questions:
1. "Où suis-je actuellement ?" → (x, y) avec confiance
2. "Quel pixel me représente ?" → pixel_value avec 80%+ confiance
3. "Quelles salles ai-je visitées ?" → Liste 5+ salles
4. "Comment aller de A à B ?" → Chemin dans graphe navigation
5. "Que fait l'action UP ?" → Conséquence mesurée

**Référence**: Leçon #79.7 dans LEÇONS_APPRISES_MAGEN.md


---

## 📋 WORKFLOW OBLIGATOIRE

### Phase 1: LECTURE CONTEXTE (10%)
```
1. Lister tous rapports /home/lvx/LVX/lumvorax2/src/MAGEN/*.md
2. Lire chaque rapport ligne par ligne
3. Extraire leçons/insights critiques
4. Identifier fonctionnalités existantes
5. Vérifier aucun oubli intégration
```

### Phase 2: ANALYSE LOGS FORENSIQUES (20%)
```
1. Lire logs dernière exécution
2. Identifier erreurs/warnings
3. Tracer chaîne causale
4. Diagnostiquer cause racine
5. Planifier corrections
```

### Phase 3: CORRECTIONS AUTOMATIQUES (30%)
```
1. Appliquer corrections identifiées
2. Vérifier aucune régression
3. Mettre à jour tests
4. Valider compilation/exécution
5. Documenter changements
```

### Phase 4: TEST VALIDATION (40%)
```
1. Exécuter tests locaux
2. Analyser résultats temps réel
3. Corriger erreurs détectées
4. Relancer tests jusqu'à succès
5. Valider métriques cibles
```

### Phase 5: ITÉRATION SI NÉCESSAIRE (50%+)
```
SI score <= 0/3:
  1. Analyser pourquoi échec
  2. Identifier amélioration nécessaire
  3. Implémenter amélioration
  4. Retour Phase 4
SINON:
  1. Documenter succès
  2. Préparer soumission Kaggle
```

### Phase 6: DOCUMENTATION (90%)
```
1. Mettre à jour LEÇONS_APPRISES_MAGEN.md
2. Créer rapport session
3. Archiver logs forensiques
4. Commit changements
```

### Phase 7: COMPLETION (100%)
```
1. Vérifier tous objectifs atteints
2. Confirmer 0 dette technique
3. Valider résultats positifs
4. Présenter résumé utilisateur
```

---

## 🎯 MÉTRIQUES SUCCÈS

### Critères Validation Locale
- [ ] Score >0/3 sur jeux arcade
- [ ] 0 crash pendant exécution
- [ ] 0 erreur compilation
- [ ] 0 warning non résolu
- [ ] Logs forensiques complets
- [ ] Documentation à jour

### Critères Soumission Kaggle
- [ ] Validation locale réussie
- [ ] Kernel créé et testé
- [ ] Soumission exécutée
- [ ] Logs récupérés et analysés
- [ ] Score public obtenu
- [ ] Rapport forensique complet

---

## 🔧 OUTILS OBLIGATOIRES

### Logs Forensiques
- **Niveau**: Bit-level + nanoseconde
- **Format**: JSONL structuré avec hash chain
- **Événements**: perception, decision, action, learning, goal_progress
- **Stockage**: `logs_forensic_v*/`
- **Causal IDs**: UUID v4 hiérarchiques (session→episode→transition→operation)
- **Timestamps**: Doubles (perf_counter_ns + time_ns + ISO)
- **Hashing**: SHA256 cryptographique (jamais `hash()` Python)
- **Intégrité**: Hash chain blockchain-style
- **Sérialisation**: Complète sans perte (data_full, pas sample)

### Tests Automatisés
- **Fréquence**: Après chaque modification
- **Scope**: Unitaire + intégration + end-to-end
- **Validation**: Résultats + logs + métriques

### Documentation Continue
- **LEÇONS_APPRISES_MAGEN.md**: Mise à jour chaque session
- **RAPPORT_SESSION*.md**: Créé chaque session
- **PROTOCOLE_MAGEN.md**: Ce fichier (référence)

---

## ⚠️ INTERDICTIONS ABSOLUES

### ❌ NE JAMAIS
1. Produire rapport avec erreurs non corrigées
2. S'arrêter avant résultats positifs (score >0/3)
3. Sauter lecture rapports existants
4. Laisser dette technique non résolue
5. Oublier notification expertises
6. Omettre progression %
7. Ignorer logs forensiques
8. Désactiver Claude Pilot
9. Tester sans validation locale d'abord
10. Documenter après coup (documenter en continu)

### ✅ TOUJOURS
1. Corriger erreurs automatiquement
2. Itérer jusqu'à succès
3. Lire rapports ligne par ligne
4. Résoudre dette technique immédiatement
5. Notifier expertises temps réel
6. Afficher progression %
7. Expliquer thinking en français
8. Activer Claude Pilot
9. Valider localement avant Kaggle
10. Documenter en continu

---

## 📊 TEMPLATE MESSAGE

```markdown
# 🚀 [TITRE ÉTAPE]

**Expertises mobilisées**: [Liste domaines]
**Progression**: X% - [Description étape]

## Thinking (Français)
[Explication raisonnement détaillé]

## Actions
[Liste actions concrètes]

## Résultats
[Métriques + validation]

## Prochaine Étape
[Si progression <100%]
```

---

## 🔄 CYCLE ITÉRATIF

```
┌─────────────────────────────────────┐
│ 1. Lire rapports existants          │
│ 2. Analyser logs forensiques        │
│ 3. Corriger erreurs automatiquement │
│ 4. Tester validation locale         │
│ 5. SI score >0/3: SUCCÈS            │
│    SINON: Retour étape 2            │
│ 6. Documenter session               │
│ 7. Commit + archivage               │
└─────────────────────────────────────┘
```

---

## 📝 HISTORIQUE VERSIONS

### v3.0.0 (2026-06-14)
- **FORENSIC TOTAL**: Intégration standards forensic industriels
- Ajout RÈGLE #8: Forensic total non-négociable
- Ajout causal IDs hiérarchiques (UUID v4)
- Ajout hash chain blockchain-style

---

## 🔬 STANDARDS FORENSIC TOTAL

### ARTICLE 1: HIÉRARCHIE CAUSALE OBLIGATOIRE

Chaque événement DOIT avoir une chaîne causale complète:

```
session_id (UUID v4)
  └─ episode_id (UUID v4)
      └─ transition_id (UUID v4)
          └─ operation_id (UUID v4)
```

**Implémentation obligatoire**:
```python
import uuid
import time

class CausalIDManager:
    def __init__(self):
        self.session_id = str(uuid.uuid4())
        self.episode_id = None
        self.transition_id = None
        self.operation_id = None
        
        self.episode_counter = 0
        self.transition_counter = 0
        self.operation_counter = 0
    
    def new_episode(self):
        self.episode_counter += 1
        self.episode_id = str(uuid.uuid4())
        self.transition_counter = 0
        return self.episode_id
    
    def new_transition(self):
        self.transition_counter += 1
        self.transition_id = str(uuid.uuid4())
        self.operation_counter = 0
        return self.transition_id
    
    def new_operation(self):
        self.operation_counter += 1
        self.operation_id = str(uuid.uuid4())
        return self.operation_id
    
    def get_full_context(self):
        return {
            'session_id': self.session_id,
            'episode_id': self.episode_id,
            'episode_number': self.episode_counter,
            'transition_id': self.transition_id,
            'transition_number': self.transition_counter,
            'operation_id': self.operation_id,
            'operation_number': self.operation_counter
        }
```

### ARTICLE 2: HASHING CRYPTOGRAPHIQUE SHA256

**INTERDIT**:
```python
hash(array.tobytes())  # ❌ Non stable, non cryptographique
```

**OBLIGATOIRE**:
```python
import hashlib

def compute_sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def compute_sha256_array(array: np.ndarray) -> str:
    return hashlib.sha256(array.tobytes()).hexdigest()
```

Pour chaque ndarray loggé:
```python
{
    'shape': list(array.shape),
    'dtype': str(array.dtype),
    'hash_sha256_full': compute_sha256_array(array),
    'min': float(array.min()),
    'max': float(array.max()),
    'mean': float(array.mean()),
    'std': float(array.std()),
    'size_bytes': array.nbytes
}
```

### ARTICLE 3: HASH CHAIN BLOCKCHAIN-STYLE

Chaque log N contient le hash du log N-1. Toute modification casse la chaîne.

```python
class ForensicLogger:
    def __init__(self, log_path: str):
        self.log_path = log_path
        self.log_file = open(log_path, 'a')
        self.previous_hash = "0" * 64  # Genesis hash
        self.log_counter = 0
    
    def log(self, event: str, component: str, data: Dict):
        self.log_counter += 1
        
        entry = {
            'log_number': self.log_counter,
            'previous_hash': self.previous_hash,
            'causal_context': self.id_manager.get_full_context(),
            'timestamp_ns': time.perf_counter_ns(),
            'timestamp_wall_ns': time.time_ns(),
            'event': event,
            'component': component,
            'data': self._serialize_complete(data)
        }
        
        # Calculer hash de cette entrée
        entry_json = json.dumps(entry, sort_keys=True)
        entry_hash = hashlib.sha256(entry_json.encode()).hexdigest()
        entry['entry_hash'] = entry_hash
        
        # Écrire
        self.log_file.write(json.dumps(entry) + '\n')
        self.log_file.flush()
        
        # Mettre à jour previous_hash
        self.previous_hash = entry_hash
    
    def verify_chain(self, log_path: str) -> bool:
        """Vérifier intégrité chaîne forensic"""
        with open(log_path) as f:
            previous_hash = "0" * 64
            for line in f:
                entry = json.loads(line)
                
                # Vérifier chaînage
                if entry['previous_hash'] != previous_hash:
                    return False
                
                # Vérifier hash entrée
                entry_copy = entry.copy()
                claimed_hash = entry_copy.pop('entry_hash')
                entry_json = json.dumps(entry_copy, sort_keys=True)
                computed_hash = hashlib.sha256(entry_json.encode()).hexdigest()
                
                if claimed_hash != computed_hash:
                    return False
                
                previous_hash = claimed_hash
        
        return True
```

### ARTICLE 4: TIMESTAMPS DOUBLES OBLIGATOIRES

**OBLIGATOIRE**:
```python
{
    'timestamp_ns': time.perf_counter_ns(),      # Monotonic, haute précision
    'timestamp_wall_ns': time.time_ns(),         # Wall clock, corrélation multi-processus
    'timestamp_iso': datetime.utcnow().isoformat()  # Humain-lisible
}
```

**Justification**:
- `perf_counter_ns()`: Durées précises, pas de saut
- `time_ns()`: Corrélation entre processus/machines
- ISO: Debugging humain

### ARTICLE 5: SÉRIALISATION COMPLÈTE BIT-LEVEL

**TOUT doit être loggé, RIEN ne doit être omis.**

```python
def _serialize_complete(self, data: Any) -> Any:
    """Sérialisation COMPLÈTE sans perte"""
    if isinstance(data, np.ndarray):
        return {
            'type': 'ndarray',
            'shape': list(data.shape),
            'dtype': str(data.dtype),
            'hash_sha256': hashlib.sha256(data.tobytes()).hexdigest(),
            'data_full': data.tolist(),  # ✅ COMPLET, pas sample!
            'min': float(data.min()),
            'max': float(data.max()),
            'mean': float(data.mean()),
            'std': float(data.std()),
            'size_bytes': data.nbytes
        }
    elif isinstance(data, dict):
        return {k: self._serialize_complete(v) for k, v in data.items()}
    elif isinstance(data, (list, tuple)):
        return [self._serialize_complete(v) for v in data]
    elif isinstance(data, (int, float, str, bool, type(None))):
        return data
    else:
        return str(data)
```

**INTERDIT**:
```python
'sample': array.flatten()[:10].tolist()  # ❌ Perte d'information
```

### ARTICLE 6: DÉTECTION ERREURS SILENCIEUSES

CLAUDE_PILOT DOIT surveiller:

```python
class SilentErrorDetector:
    def __init__(self):
        self.hash_history = []
        self.embedding_history = []
        self.reward_history = []
        self.action_history = []
    
    def check_anomalies(self, log_entry: Dict) -> List[str]:
        anomalies = []
        
        # 1. Hash constant (collision massive)
        if 'topological_hash' in log_entry:
            self.hash_history.append(log_entry['topological_hash'])
            if len(self.hash_history) > 10:
                unique_hashes = len(set(self.hash_history[-10:]))
                if unique_hashes < 3:
                    anomalies.append('HASH_CONSTANT')
        
        # 2. Embedding constant (stagnation)
        if 'embedding' in log_entry:
            emb = np.array(log_entry['embedding']['data_full'])
            self.embedding_history.append(emb)
            if len(self.embedding_history) > 5:
                variations = [
                    np.linalg.norm(self.embedding_history[i] - self.embedding_history[i-1])
                    for i in range(-4, 0)
                ]
                if max(variations) < 0.01:
                    anomalies.append('EMBEDDING_CONSTANT')
        
        # 3. Reward bloqué
        if 'reward' in log_entry:
            self.reward_history.append(log_entry['reward'])
            if len(self.reward_history) > 20:
                if len(set(self.reward_history[-20:])) == 1:
                    anomalies.append('REWARD_BLOCKED')
        
        # 4. Action loop
        if 'action' in log_entry:
            self.action_history.append(log_entry['action'])
            if len(self.action_history) > 10:
                last_10 = self.action_history[-10:]
                if len(set(last_10)) <= 2:
                    anomalies.append('ACTION_LOOP')
        
        # 5. NaN propagation
        if 'embedding' in log_entry:
            emb = np.array(log_entry['embedding']['data_full'])
            if np.isnan(emb).any():
                anomalies.append('NAN_PROPAGATION')
        
        # 6. Gradient explosion
        if 'embedding' in log_entry:
            emb = np.array(log_entry['embedding']['data_full'])
            if np.abs(emb).max() > 1e6:
                anomalies.append('GRADIENT_EXPLOSION')
        
        # 7. Entropy collapse
        if 'action_probs' in log_entry:
            probs = np.array(log_entry['action_probs'])
            entropy = -np.sum(probs * np.log(probs + 1e-10))
            if entropy < 0.1:
                anomalies.append('ENTROPY_COLLAPSE')
        
        return anomalies
```

### ARTICLE 7: MODE REPLAY OBLIGATOIRE

Le système DOIT permettre:

```python
class ForensicReplay:
    def __init__(self, log_path: str):
        self.logs = self._load_logs(log_path)
        self.verify_integrity()
    
    def replay_exact(self):
        """Replay exact avec même seed"""
        for log in self.logs:
            self._execute_operation(log)
    
    def replay_accelerated(self, speed: float = 10.0):
        """Replay accéléré pour debugging"""
        for log in self.logs:
            time.sleep(log['duration_ns'] / 1e9 / speed)
            self._execute_operation(log)
    
    def replay_differential(self, other_log_path: str):
        """Comparer deux exécutions"""
        other_logs = self._load_logs(other_log_path)
        differences = []
        
        for i, (log1, log2) in enumerate(zip(self.logs, other_logs)):
            if log1['data'] != log2['data']:
                differences.append({
                    'step': i,
                    'log1': log1,
                    'log2': log2
                })
        
        return differences
    
    def replay_causal(self, target_state: str):
        """Rejouer jusqu'à état cible"""
        for log in self.logs:
            self._execute_operation(log)
            if self._check_state(target_state):
                break
```

### ARTICLE 8: FORENSIC PAR MODULE

Chaque module MAGEN DOIT implémenter:

1. **State Encoder V25**: Log AVANT/APRÈS encoding
2. **Behavioral Entity Detector**: Log AVANT/APRÈS observation
3. **Causal Graph Manager**: Log AVANT/APRÈS transition
4. **Agent V27/V27.1**: Log AVANT/APRÈS perceive/decide/learn
5. **Goal Manager**: Log AVANT/APRÈS goal_update
6. **Memory Manager**: Log AVANT/APRÈS memory_operations

**Template obligatoire**:
```python
def operation(self, *args):
    op_id = self.id_manager.new_operation()
    start_ns = time.perf_counter_ns()
    
    # Log AVANT
    self._log_forensic('operation_start', 'ModuleName', {
        'args': args,  # COMPLET
        'state_before': self.state  # COMPLET
    })
    
    # Opération
    result = self._operation_internal(*args)
    
    # Log APRÈS
    end_ns = time.perf_counter_ns()
    self._log_forensic('operation_complete', 'ModuleName', {
        'args': args,  # COMPLET
        'result': result,  # COMPLET
        'state_after': self.state,  # COMPLET
        'duration_ns': end_ns - start_ns
    })
    
    return result
```

### ARTICLE 9: VALIDATION INTÉGRITÉ

Tests obligatoires pour chaque module:

```python
def test_forensic_integrity(module):
    # 1. Forensic activé
    assert module.forensic_enabled == True
    
    # 2. Logs générés
    module.operation()
    assert os.path.exists(module.forensic_log_path)
    
    # 3. Hash chain valide
    assert module.forensic_logger.verify_chain(module.forensic_log_path)
    
    # 4. Causal IDs présents
    logs = read_jsonl(module.forensic_log_path)
    for log in logs:
        assert 'causal_context' in log
        assert 'session_id' in log['causal_context']
        assert 'operation_id' in log['causal_context']
    
    # 5. Timestamps doubles
    for log in logs:
        assert 'timestamp_ns' in log
        assert 'timestamp_wall_ns' in log
    
    # 6. Hashes SHA256
    for log in logs:
        if 'data' in log and 'hash_sha256' in log['data']:
            # Vérifier format SHA256 (64 caractères hex)
            assert len(log['data']['hash_sha256']) == 64
            assert all(c in '0123456789abcdef' for c in log['data']['hash_sha256'])
```

### ARTICLE 10: MÉTRIQUES MATURITÉ FORENSIC

**Score cible**: 100% sur tous critères

```
Observabilité: 100%
Traçabilité: 100%
Rejouabilité: 100%
Intégrité: 100%
Causalité Explicable: 100%
Robustesse Production: 100%
```

**Aucun compromis**:
- Performance vs Forensic: **Forensic gagne TOUJOURS**
- Stockage vs Forensic: **Forensic gagne TOUJOURS**
- Latence vs Forensic: **Forensic gagne TOUJOURS**

---

- Ajout timestamps doubles (monotonic + wall clock)
- Ajout sérialisation complète bit-level
- Ajout détection erreurs silencieuses
- Ajout mode REPLAY obligatoire
- Interdiction `hash()` Python (SHA256 obligatoire)
- Interdiction sample/truncation données
- Principe: Performance/Stockage/Latence < Forensic TOUJOURS

### v2.0.0 (2026-06-14)
- Intégration règles utilisateur Session 69
- Ajout correction automatique erreurs
- Ajout interdiction dette technique
- Ajout obligation lecture rapports complète
- Ajout notification expertises temps réel
- Ajout progression % obligatoire
- Ajout thinking français obligatoire
- Ajout mode compétition
- Ajout adaptation ARC-AGI-3 complète

### v1.0.0 (2026-06-13)
- Version initiale protocole MAGEN
- Règles base forensiques
- Workflow itératif

---

*Protocole MAGEN - Aucune exception tolérée - Mode Compétition ACTIF*

---

## 🔴 RÈGLES CRITIQUES SESSION 81

### RÈGLE #12: WORLD UNDERSTANDING AVANT STRATÉGIE

**Obligation**: Construire représentation persistante du monde AVANT toute stratégie

**Principe fondamental**:
> "MAGEN mesure son activité interne mais ne construit pas de représentation persistante du monde"

**Étapes obligatoires**:
1. ✅ **Identifier avatar** (confiance > 80%)
   - Tester unicité pixel (présent à 1 seule position)
   - Valider contrôlabilité (corrélation action→mouvement > 95%)
   - Mesurer stabilité (100+ steps)

2. ✅ **Tracker position** (couverture > 90%)
   - Enregistrer position à chaque step
   - Détecter changements position
   - Construire historique déplacements

3. ✅ **Construire graphe navigation** (5+ salles)
   - Identifier salles/zones distinctes
   - Détecter transitions entre salles
   - Mapper topologie complète

4. ✅ **Mesurer causalité** (100+ liens)
   - Corréler actions→effets
   - Identifier patterns causaux
   - Valider prédictibilité

**Validation**:
- ❌ INTERDIT: Stratégie sans représentation monde
- ❌ INTERDIT: Assumer existence avatar sans validation
- ❌ INTERDIT: Tracker objet sans vérifier unicité

---

### RÈGLE #13: VALIDATION PROGRESSIVE OBLIGATOIRE

**Obligation**: Valider COMPLÈTEMENT un jeu avant de passer au suivant

**Critères validation complète**:
- ✅ Avatar identifié (confiance > 80%)
- ✅ Contrôlabilité validée (corrélation > 95%)
- ✅ Graphe navigation construit (5+ salles)
- ✅ Causalité mesurée (100+ liens)
- ✅ Score obtenu (>0/3 sur le jeu)

**Principe fondamental**:
> "Si ne passe pas le plus facile, ne passera pas le plus difficile"

**Interdictions absolues**:
- ❌ INTERDIT: Changer de jeu sans validation complète
- ❌ INTERDIT: Tester jeu plus difficile si échec sur facile
- ❌ INTERDIT: Passer au suivant avec score 0/3

**Justification**:
- Progression = validation niveau par niveau
- Échec niveau N → Impossible niveau N+1
- Validation partielle = Fausse progression

**Exemple Session 81**:
```
❌ ERREUR: Changé de ar25 sans valider ls20 d'abord
✅ CORRECTION: Retour obligatoire à ls20-9607627b
✅ RÈGLE: Ne JAMAIS sortir de ls20 sans score >0/3
```

---

### RÈGLE #14: VÉRIFICATION AUTOMATIQUE GAME ID

**Obligation**: Vérifier game_id au début de CHAQUE script

**Implémentation obligatoire**:
```python
# Constante au début du script
REQUIRED_GAME_ID = "ls20-9607627b"  # JEU OBLIGATOIRE

def verify_game_id(env, required_id: str):
    """Vérifier que le bon jeu est utilisé"""
    # Méthode 1: Vérifier dans logs
    print(f"🔍 Vérification game_id...")
    
    # Méthode 2: Vérifier dans environnement
    actual_id = get_game_id_from_env(env)
    
    if actual_id != required_id:
        raise ValueError(
            f"❌ ERREUR CRITIQUE: Jeu {actual_id} ≠ {required_id}\n"
            f"INTERDICTION de changer de jeu sans validation complète!"
        )
    
    print(f"✅ Jeu vérifié: {actual_id}")
    return True

# Appel obligatoire après création environnement
env = arcade.make(REQUIRED_GAME_ID)
verify_game_id(env, REQUIRED_GAME_ID)
```

**Validation dans logs**:
```python
# Afficher game_id dans TOUS les logs
logger.info(f"Game ID: {REQUIRED_GAME_ID}")
logger.info(f"Successfully reset game {REQUIRED_GAME_ID}, guid=...")
```

**Interdictions**:
- ❌ INTERDIT: Créer script sans vérification game_id
- ❌ INTERDIT: Changer REQUIRED_GAME_ID sans justification
- ❌ INTERDIT: Ignorer erreur vérification game_id

**Exemple Session 81**:
```python
# ❌ ERREUR: game_id = "ar25-0c556536" (MAUVAIS JEU)
# ✅ CORRECTION: game_id = "ls20-9607627b" (BON JEU)
```

---

### RÈGLE #15: ANALYSE DIFFÉRENTIELLE AVANT MODÉLISATION

**Obligation**: Observer effets actions AVANT de modéliser comportement

**Principe fondamental**:
> "Quelle partie de l'écran répond à l'action ?" pas "Où est l'avatar ?"

**Méthodologie obligatoire**:
```python
# 1. Tester TOUTES les actions
for action in [ACTION1, ACTION2, ACTION3, ACTION4]:
    # 2. Mesurer différence frame avant/après
    frame_before = env.reset().frame
    frame_after = env.step(action).frame
    diff = (frame_before != frame_after)
    
    # 3. Compter pixels modifiés
    pixels_modified = np.sum(diff)
    
    # 4. Analyser composantes connexes
    labeled, num_components = label(diff)
    
    # 5. Classifier type d'action
    if pixels_modified <= 5:
        action_type = "AVATAR" (objet unique)
    elif pixels_modified <= 100:
        action_type = "RÉGION" (transformation locale)
    else:
        action_type = "GLOBALE" (transformation complète)
```

**Interprétation résultats**:
- **2 pixels, 1 composante**: Avatar potentiel (ACTION2 ls20)
- **52 pixels, 2 composantes**: Transformation région (ACTION1/3/4 ls20)
- **109 pixels, 5 composantes**: Transformation globale (ar25)

**Interdictions**:
- ❌ INTERDIT: Assumer existence avatar sans test
- ❌ INTERDIT: Modéliser avant observer
- ❌ INTERDIT: Ignorer actions qui ne modifient pas avatar

**Validation**:
- ✅ Tester 20+ fois par action (stabilité)
- ✅ Mesurer variance (cohérence)
- ✅ Identifier patterns (prédictibilité)

---

## 📊 MISE À JOUR MÉTRIQUES SUCCÈS

### Critères Validation Locale (MISE À JOUR)

**Avant toute stratégie**:
- [ ] Avatar identifié (confiance > 80%, unicité validée)
- [ ] Position trackée (couverture > 90%, 100+ steps)
- [ ] Graphe navigation construit (5+ salles, transitions détectées)
- [ ] Causalité mesurée (100+ liens, corrélation > 95%)

**Validation jeu actuel (ls20-9607627b)**:
- [ ] Score >0/3 sur ls20-9607627b
- [ ] 0 crash pendant exécution
- [ ] 0 erreur compilation
- [ ] 0 warning non résolu
- [ ] Logs forensiques complets
- [ ] Documentation à jour
- [ ] Game ID vérifié automatiquement

**Interdiction absolue**:
- ❌ Passer à un autre jeu sans validation complète ls20

---

## ⚠️ INTERDICTIONS ABSOLUES (MISE À JOUR)

### ❌ NE JAMAIS

1. Produire rapport avec erreurs non corrigées
2. S'arrêter avant résultats positifs (score >0/3)
3. Sauter lecture rapports existants
4. Laisser dette technique non résolue
5. Oublier notification expertises
6. Omettre progression %
7. Ignorer logs forensiques
8. Désactiver Claude Pilot
9. Tester sans validation locale d'abord
10. Documenter après coup (documenter en continu)
11. **Changer de jeu sans validation complète** (RÈGLE #13)
12. **Créer script sans vérifier game_id** (RÈGLE #14)
13. **Modéliser avant observer** (RÈGLE #15)
14. **Assumer existence avatar sans test** (RÈGLE #12)

---

## 📝 HISTORIQUE VERSIONS (MISE À JOUR)

### v3.2.0 (2026-06-17) - SESSION 81

**Ajouts critiques**:
- RÈGLE #12: World Understanding avant stratégie
- RÈGLE #13: Validation progressive obligatoire
- RÈGLE #14: Vérification automatique game_id
- RÈGLE #15: Analyse différentielle avant modélisation

**Leçons intégrées**:
- Leçon #80.1-80.5: Validation unicité, confiance ≠ validité
- Leçon #81.1-81.6: API nouvelle, interdiction changement jeu

**Principe fondamental ajouté**:
> "Si ne passe pas le plus facile, ne passera pas le plus difficile"

**Erreur corrigée Session 81**:
- ❌ Changé de ar25 sans valider ls20
- ✅ Retour obligatoire à ls20-9607627b
- ✅ Interdiction absolue sortir de ls20 sans score >0/3

### v3.1.0 (2026-06-17) - SESSION 80

- RÈGLE #11: Représentation monde avant stratégie
- Découverte: Pixel 4 non unique (2609 occurrences)
- Découverte: Paradigme avatar non validé
