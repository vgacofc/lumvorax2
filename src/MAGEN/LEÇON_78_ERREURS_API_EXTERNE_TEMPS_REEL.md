# LEÇON #78 - ERREURS API EXTERNE DÉTECTÉES EN TEMPS RÉEL
**Date**: 2026-06-16  
**Session**: 78  
**Contexte**: Validation externe MAGEN V40.3 sur API ARC-AGI-3

---

## 🚨 RÉSUMÉ EXÉCUTIF

**DÉCOUVERTE MAJEURE**: Lors de l'exécution réelle sur l'API externe ARC-AGI-3, 3 erreurs critiques ont été détectées en monitoring temps réel:

1. **API 500 - ACTION6 SYSTÉMATIQUE**: L'API rejette ACTION6 avec erreur 500 serveur
2. **KeyError 'magen_v39'**: Erreur d'accès dictionnaire dans get_stats()
3. **GAME_NOT_STARTED**: Après retries, le jeu se ferme et devient injouable

---

## 📊 DONNÉES FORENSIQUES TEMPS RÉEL

### Pattern d'Erreur Observé (8 jeux testés)

```
Jeu 1 (ar25-0c556536):
  - 7x erreurs 500 sur ACTION6
  - État final: NOT_FINISHED
  - Reward: 0.00
  - Erreur: KeyError 'magen_v39'

Jeu 2 (cd82-fb555c5d):
  - 9x erreurs 500 sur ACTION6
  - État final: NOT_FINISHED
  - Reward: 0.00
  - Erreur: KeyError 'magen_v39'

Jeu 6 (su15-1944f8ab):
  - 9x erreurs 500 sur ACTION6
  - État final: NOT_FINISHED
  - Reward: 0.00
  - Erreur: KeyError 'magen_v39'

Jeu 7 (cn04-2fe56bfb):
  - 8x erreurs 500 sur ACTION6
  - État final: NOT_FINISHED
  - Reward: 0.00
  - Erreur: KeyError 'magen_v39'
```

**PATTERN 100% REPRODUCTIBLE**: Tous les jeux échouent de la même manière.

---

## 🔍 ANALYSE DÉTAILLÉE DES ERREURS

### ERREUR #1: API 500 - ACTION6 SYSTÉMATIQUE

**Symptôme**:
```
2026-06-16 16:05:37 | ERROR | Failed to perform action ACTION6 for game ar25-0c556536: 
500 Server Error: Internal Server Error for url: https://three.arcprize.org/api/cmd/ACTION6
```

**Cause Racine**:
- Le PolicyManager V40.3 sélectionne ACTION6 comme première action
- L'API ARC-AGI-3 rejette systématiquement ACTION6 avec erreur 500
- Ceci est un problème SERVEUR, pas un problème MAGEN

**Impact**:
- Chaque jeu commence par 6-9 tentatives échouées
- Perte de temps et de steps (50 steps max par jeu)
- Le retry logic actuel ne fonctionne pas car il continue à essayer ACTION6

**Solution Requise**:
```python
# BLACKLIST ACTION6 dans l'adaptateur
BLACKLISTED_ACTIONS = [GameAction.ACTION6]

def select_action(self, obs):
    # Filtrer les actions blacklistées
    available = [a for a in self.available_actions if a not in BLACKLISTED_ACTIONS]
    action = self.policy.decide_action(available, ...)
    return action
```

---

### ERREUR #2: KeyError 'magen_v39'

**Symptôme**:
```python
❌ ERREUR: 'magen_v39'
```

**Cause Racine**:
Dans `get_stats()` ligne ~429:
```python
print(f"MAGEN V39: {agent_stats['magen_v39']['problems_attempted']} problèmes tentés")
```

Mais `agent_stats` retourné par `adapter.get_stats()` n'a PAS de clé `'magen_v39'`.

**Structure Réelle de agent_stats**:
```python
{
    'game_id': str,
    'steps': int,
    'total_reward': float,
    'avg_reward': float,
    'policy': dict,
    'world_model': dict,
    'learning': dict
}
```

**Solution Requise**:
```python
# Ligne ~429 - AVANT
print(f"MAGEN V39: {agent_stats['magen_v39']['problems_attempted']} problèmes tentés")

# Ligne ~429 - APRÈS
print(f"Policy: {agent_stats['policy']['policy_state']['mode']}")
print(f"Learning: {len(agent_stats['learning']['action_scores'])} actions apprises")
```

---

### ERREUR #3: GAME_NOT_STARTED après retries

**Symptôme**:
```
2026-06-16 16:06:43 | ERROR | Failed to perform action ACTION1 for game bp35-0a0ad940: 
400 Client Error: Bad Request for url: https://three.arcprize.org/api/cmd/ACTION1
Response body: {"error":"GAME_NOT_STARTED_ERROR","message":"game bp35-0a0ad940 is available but has not been started, send RESET to begin playing"}
```

**Cause Racine**:
- Après 6 erreurs 500 consécutives sur ACTION6
- Le jeu se ferme automatiquement (timeout ou limite d'erreurs)
- Toutes les actions suivantes échouent avec GAME_NOT_STARTED

**Impact**:
- Le jeu devient complètement injouable
- Aucun reward ne peut être obtenu
- Le script continue mais ne fait rien

**Solution Requise**:
```python
# Réduire max_retries de 3 à 1
max_retries = 1  # Au lieu de 3

# Passer rapidement à une autre action
if "500" in error_msg:
    # Ne pas retry ACTION6, passer directement à ACTION1
    if action == GameAction.ACTION6:
        action = GameAction.ACTION1
        break  # Sortir du retry loop immédiatement
```

---

## 🎯 CORRECTIONS PRIORITAIRES

### PRIORITÉ 1: BLACKLIST ACTION6
**Urgence**: CRITIQUE  
**Impact**: Élimine 100% des erreurs 500  
**Effort**: 5 lignes de code

```python
# Dans MAGENArcadeAdapter.__init__()
self.blacklisted_actions = [GameAction.ACTION6]

# Dans select_action()
available = [a for a in self.available_actions if a not in self.blacklisted_actions]
```

### PRIORITÉ 2: CORRIGER KeyError 'magen_v39'
**Urgence**: HAUTE  
**Impact**: Élimine crash après chaque jeu  
**Effort**: 2 lignes de code

```python
# Ligne ~429
print(f"Policy: {agent_stats['policy']['policy_state']['mode']}")
print(f"Learning: {len(agent_stats['learning']['action_scores'])} actions")
```

### PRIORITÉ 3: AMÉLIORER RETRY LOGIC
**Urgence**: MOYENNE  
**Impact**: Réduit temps perdu sur retries  
**Effort**: 10 lignes de code

```python
max_retries = 1  # Réduire de 3 à 1
# Fallback immédiat si ACTION6 échoue
if action == GameAction.ACTION6 and "500" in error_msg:
    action = GameAction.ACTION1
    break
```

---

## 📈 IMPACT ATTENDU DES CORRECTIONS

### Avant Corrections
- **Erreurs 500**: 7-9 par jeu × 25 jeux = ~200 erreurs
- **Temps perdu**: ~15 secondes par jeu × 25 = 6.25 minutes
- **Reward obtenu**: 0.00 (tous les jeux échouent)
- **Taux de succès**: 0%

### Après Corrections
- **Erreurs 500**: 0 (ACTION6 blacklisté)
- **Temps perdu**: 0 (pas de retries inutiles)
- **Reward obtenu**: > 0 (jeux jouables)
- **Taux de succès**: > 0% (objectif réaliste)

---

## 🔬 MÉTHODOLOGIE DE DÉTECTION

**Monitoring Temps Réel Efficace**:
1. ✅ Exécution avec `tee` pour capturer logs
2. ✅ Observation pattern d'erreurs en temps réel
3. ✅ Arrêt immédiat dès pattern identifié
4. ✅ Analyse forensique des logs partiels
5. ✅ Documentation complète avant correction

**Leçon Clé**: Ne PAS attendre la fin de l'exécution (25 jeux × 50 steps = 20+ minutes). Détecter et corriger après 3-5 jeux suffit.

---

## 📝 PROTOCOLE OBLIGATOIRE

### AVANT toute exécution externe:
1. ✅ Lire guide API officiel
2. ✅ Vérifier structure des données retournées
3. ✅ Tester sur 1-2 jeux d'abord
4. ✅ Monitorer en temps réel
5. ✅ Arrêter dès pattern d'erreur détecté

### PENDANT l'exécution:
1. ✅ Observer logs en temps réel
2. ✅ Identifier patterns répétitifs
3. ✅ Arrêter si >3 jeux échouent identiquement
4. ✅ Analyser cause racine immédiatement

### APRÈS détection d'erreur:
1. ✅ Documenter dans LEÇON
2. ✅ Corriger toutes les erreurs détectées
3. ✅ Relancer avec corrections
4. ✅ Vérifier que corrections fonctionnent

---

## 🎓 ENSEIGNEMENTS

### Ce qui a BIEN fonctionné:
1. ✅ **Monitoring temps réel**: Détection rapide des erreurs
2. ✅ **Arrêt précoce**: Économie de temps (arrêt après 8/25 jeux)
3. ✅ **Analyse forensique**: Identification précise des causes
4. ✅ **Documentation immédiate**: Capture du contexte complet

### Ce qui a MAL fonctionné:
1. ❌ **Pas de test préalable**: Lancement direct sur 25 jeux
2. ❌ **Pas de blacklist**: ACTION6 devrait être testé d'abord
3. ❌ **Retry trop agressif**: 3 retries × 6 jeux = 18 tentatives inutiles
4. ❌ **Pas de validation structure**: KeyError évitable

### Améliorations pour prochaine fois:
1. 🔄 **Test sur 1 jeu d'abord**: Valider que tout fonctionne
2. 🔄 **Blacklist dynamique**: Détecter actions problématiques automatiquement
3. 🔄 **Retry intelligent**: Fallback immédiat si erreur serveur
4. 🔄 **Validation structure**: Assert sur structure des données

---

## ✅ CHECKLIST VALIDATION EXTERNE

Avant toute soumission externe:

- [ ] Lire documentation API officielle
- [ ] Tester sur 1 jeu en mode debug
- [ ] Vérifier structure données retournées
- [ ] Implémenter blacklist actions problématiques
- [ ] Ajouter retry logic intelligent
- [ ] Valider tous les accès dictionnaire
- [ ] Monitorer en temps réel
- [ ] Arrêter si pattern d'erreur détecté
- [ ] Documenter dans LEÇON
- [ ] Corriger avant relance complète

---

## 🔗 RÉFÉRENCES

- **Guide API**: `GUIDE_SOUMISSION_ARC_AGI_3_V21.md`
- **Rapport Versionnage**: `RAPPORT_CRITIQUE_VERSIONNAGE_SESSION78.md`
- **Logs Exécution**: `logs/execution_v40_3_FIXED_*.log`
- **Script Corrigé**: `submit_arc_agi_3_with_magen_v40_3_FINAL.py`

---

**STATUT**: ⏳ EN COURS - Corrections en cours d'application  
**PROCHAINE ÉTAPE**: Appliquer les 3 corrections prioritaires et relancer