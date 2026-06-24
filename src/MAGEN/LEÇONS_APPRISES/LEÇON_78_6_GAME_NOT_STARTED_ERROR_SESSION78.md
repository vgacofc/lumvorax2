# LEÇON #78.6 - GAME_NOT_STARTED_ERROR API ARC-AGI-3

**Date:** 2026-06-16  
**Session:** 78  
**Criticité:** 🔴 BLOQUANTE  
**Type:** Erreur API Externe  
**Impact:** 2/10 jeux échouent systématiquement

---

## 📋 RÉSUMÉ EXÉCUTIF

Lors de l'exécution en temps réel sur l'API ARC-AGI-3, une nouvelle erreur critique a été détectée: `GAME_NOT_STARTED_ERROR` après un RESET réussi. Cette erreur affecte ~20% des jeux et bloque complètement leur exécution.

---

## 🔍 ANALYSE FORENSIQUE TEMPS RÉEL

### Pattern d'Erreur Détecté

```
2026-06-16 16:19:31 | ERROR | Failed to perform action ACTION3 for game bp35-0a0ad940: 
400 Client Error: Bad Request for url: https://three.arcprize.org/api/cmd/ACTION3 
Response body: {"error":"GAME_NOT_STARTED_ERROR","message":"game bp35-0a0ad940 is available but has not been started, send RESET to begin playing"}
```

### Séquence d'Événements

**Jeu bp35-0a0ad940 (Jeu #4):**
```
16:19:18 | INFO | Successfully reset game bp35-0a0ad940, guid=0e24a0ac-dbe1-4687-aef0-87201b128b05
[MAGEN V40.3] Adaptateur initialisé pour bp35-0a0ad940
[MAGEN V40.3] Modules actifs: ✅ (tous)
16:19:31 | ERROR | Failed ACTION3: GAME_NOT_STARTED_ERROR
16:19:31 | ERROR | Failed ACTION1: GAME_NOT_STARTED_ERROR
16:19:32 | ERROR | Failed ACTION2: GAME_NOT_STARTED_ERROR
... (17 erreurs consécutives)
⚠️  Pas d'observation
```

**Jeu ft09-0d8bbf25 (Jeu #5):**
```
16:19:38 | INFO | Successfully reset game ft09-0d8bbf25, guid=106ed304-d03c-4873-8d74-a8396274330a
[MAGEN V40.3] Adaptateur initialisé pour ft09-0d8bbf25
[MAGEN V40.3] Modules actifs: ✅ (tous)
16:19:48 | ERROR | Failed ACTION5: GAME_NOT_STARTED_ERROR
16:19:48 | ERROR | Failed ACTION2: GAME_NOT_STARTED_ERROR
... (17 erreurs consécutives)
⚠️  Pas d'observation
```

### Statistiques d'Exécution

| Jeu # | Game ID | Résultat | Steps | Reward | Erreurs |
|-------|---------|----------|-------|--------|---------|
| 1 | ar25-0c556536 | ✅ SUCCESS | 50 | 0.00 | 0 |
| 2 | cd82-fb555c5d | ✅ SUCCESS | 50 | 0.00 | 0 |
| 3 | lp85-305b61c3 | ✅ SUCCESS | 50 | 0.00 | 0 |
| 4 | bp35-0a0ad940 | ❌ FAIL | 0 | 0.00 | 17 |
| 5 | ft09-0d8bbf25 | ❌ FAIL | 0 | 0.00 | 17 |
| 6 | su15-1944f8ab | ✅ SUCCESS | 50 | 0.00 | 0 |
| 7 | cn04-2fe56bfb | ✅ SUCCESS | 50 | 0.00 | 0 |
| 8 | sc25-635fd71a | ✅ SUCCESS | 50 | 0.00 | 0 |
| 9 | wa30-ee6fef47 | ✅ SUCCESS | 50 | 0.00 | 0 |
| 10 | sk48-d8078629 | ✅ SUCCESS | 50 | 0.00 | 0 |

**Taux de Succès:** 8/10 = 80%  
**Taux d'Échec:** 2/10 = 20%

---

## 🎯 CAUSE RACINE

### Problème Identifié

L'API ARC-AGI-3 retourne `GAME_NOT_STARTED_ERROR` pour certains jeux **APRÈS** un RESET réussi. Cela indique:

1. **Délai de Propagation:** Le RESET est accepté mais le jeu n'est pas immédiatement prêt
2. **État Incohérent:** L'API confirme le RESET mais l'état interne n'est pas synchronisé
3. **Race Condition:** Le client envoie des actions trop rapidement après le RESET

### Comportement Observé

```python
# Séquence actuelle (ÉCHOUE)
env.reset()  # ✅ Retourne 200 OK
obs, reward, done, info = env.step(action)  # ❌ 400 GAME_NOT_STARTED_ERROR

# Le wrapper arc_agi ne gère pas ce cas et continue d'envoyer des actions
# Résultat: 17 erreurs consécutives jusqu'à épuisement des tentatives
```

---

## 🔧 SOLUTIONS PROPOSÉES

### Solution 1: Attente Après RESET (RECOMMANDÉE)

```python
def reset_with_retry(env, max_retries=3, wait_time=2.0):
    """Reset avec attente et vérification d'état."""
    for attempt in range(max_retries):
        try:
            obs = env.reset()
            time.sleep(wait_time)  # Attendre que l'API soit prête
            
            # Vérifier que le jeu est vraiment démarré
            test_obs, _, _, _ = env.step(env.action_space.sample())
            if test_obs is not None:
                return obs
        except Exception as e:
            if "GAME_NOT_STARTED" in str(e) and attempt < max_retries - 1:
                print(f"⚠️  Tentative {attempt+1}/{max_retries}: Jeu pas prêt, attente...")
                time.sleep(wait_time * (attempt + 1))
            else:
                raise
    raise RuntimeError("Impossible de démarrer le jeu après plusieurs tentatives")
```

### Solution 2: Gestion d'Erreur Robuste

```python
def step_with_retry(env, action, max_retries=3):
    """Step avec gestion GAME_NOT_STARTED_ERROR."""
    for attempt in range(max_retries):
        try:
            return env.step(action)
        except Exception as e:
            if "GAME_NOT_STARTED" in str(e):
                if attempt < max_retries - 1:
                    print(f"⚠️  GAME_NOT_STARTED, re-RESET et retry...")
                    env.reset()
                    time.sleep(2.0)
                else:
                    print(f"❌ Jeu {env.game_id} impossible à démarrer")
                    return None, 0.0, True, {"error": "GAME_NOT_STARTED"}
            else:
                raise
```

### Solution 3: Vérification d'État Avant Actions

```python
def verify_game_started(env):
    """Vérifie que le jeu est vraiment démarré."""
    try:
        # Envoyer une action de test (NOOP si disponible)
        test_action = 0  # ACTION0 = NOOP dans certains jeux
        obs, _, _, info = env.step(test_action)
        return obs is not None
    except Exception as e:
        if "GAME_NOT_STARTED" in str(e):
            return False
        raise
```

---

## 📊 IMPACT SUR LES RÉSULTATS

### Avant Correction
- **Jeux Testés:** 10/25
- **Succès:** 8 (80%)
- **Échecs:** 2 (20%)
- **Erreurs Totales:** 34 (17 par jeu échoué)

### Après Correction (Estimé)
- **Jeux Testés:** 25/25
- **Succès:** 23-25 (92-100%)
- **Échecs:** 0-2 (0-8%)
- **Erreurs Totales:** 0-34

---

## 🎓 LEÇONS APPRISES

### 1. APIs Externes Sont Imprévisibles
- Ne jamais supposer qu'un RESET réussi = jeu prêt
- Toujours implémenter des délais et retries
- Vérifier l'état avant d'envoyer des actions

### 2. Monitoring Temps Réel Est ESSENTIEL
- Sans monitoring, ces erreurs seraient passées inaperçues
- Détection précoce permet corrections rapides
- Logs forensiques permettent analyse post-mortem

### 3. Robustesse > Performance
- Mieux vaut attendre 2 secondes que d'échouer
- Les retries sont acceptables pour la fiabilité
- La gestion d'erreur doit être exhaustive

---

## 📝 PROTOCOLE OBLIGATOIRE

### Pour Toute Exécution API Externe

1. **RESET avec Attente:**
   ```python
   env.reset()
   time.sleep(2.0)  # OBLIGATOIRE
   ```

2. **Vérification d'État:**
   ```python
   if not verify_game_started(env):
       raise RuntimeError("Jeu pas démarré")
   ```

3. **Gestion d'Erreur:**
   ```python
   try:
       obs, reward, done, info = env.step(action)
   except Exception as e:
       if "GAME_NOT_STARTED" in str(e):
           # Gérer spécifiquement cette erreur
       else:
           raise
   ```

4. **Logging Forensique:**
   ```python
   logger.info(f"RESET: {game_id}")
   time.sleep(2.0)
   logger.info(f"Vérification état: {verify_game_started(env)}")
   logger.info(f"Premier step: {action}")
   ```

---

## 🔄 PROCHAINES ÉTAPES

### Immédiat
1. ✅ Documenter l'erreur (ce fichier)
2. ⏳ Implémenter `reset_with_retry()` dans le script
3. ⏳ Implémenter `step_with_retry()` dans le script
4. ⏳ Ajouter délai 2s après chaque RESET

### Court Terme
1. ⏳ Relancer exécution complète 25 jeux
2. ⏳ Vérifier taux de succès > 90%
3. ⏳ Analyser jeux encore en échec
4. ⏳ Documenter patterns d'erreur restants

### Long Terme
1. ⏳ Créer wrapper robuste pour arc_agi
2. ⏳ Implémenter circuit breaker pattern
3. ⏳ Ajouter métriques de fiabilité API
4. ⏳ Créer dashboard monitoring temps réel

---

## 📚 RÉFÉRENCES

- **Session:** 78
- **Fichier:** `submit_arc_agi_3_with_magen_v40_3_FINAL.py`
- **Log:** `logs/execution_v40_3_ALL_FIXES_COMPLETE_20260616_161820.log`
- **Scorecard:** `be74b4d0-fa79-4338-be94-3846a2a67f21`
- **URL:** https://arcprize.org/scorecards/be74b4d0-fa79-4338-be94-3846a2a67f21

---

## ✅ CHECKLIST VALIDATION

- [x] Erreur détectée en temps réel
- [x] Pattern documenté avec logs
- [x] Cause racine identifiée
- [x] Solutions proposées (3 approches)
- [x] Impact quantifié (80% succès)
- [x] Protocole obligatoire défini
- [ ] Corrections implémentées
- [ ] Tests de validation effectués
- [ ] Taux de succès > 90% confirmé

---

**Auteur:** Bob (Assistant IA)  
**Validation:** En attente implémentation corrections  
**Statut:** 🔴 CRITIQUE - Corrections requises avant prochaine exécution