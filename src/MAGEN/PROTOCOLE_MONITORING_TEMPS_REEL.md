# PROTOCOLE MONITORING TEMPS RÉEL - MAGEN
## Obligation Absolue pour Toute Exécution

**Version**: 1.0  
**Date**: 2026-06-14  
**Statut**: OBLIGATOIRE  

---

## 🚨 RÈGLE ABSOLUE

Lors de **TOUTE exécution MAGEN**, vous **DEVEZ**:

1. ✅ Identifier les logs forensic correspondants (timestamp)
2. ✅ Lire les logs **PENDANT** ou **IMMÉDIATEMENT APRÈS** l'exécution
3. ✅ Baser **TOUTES** les corrections sur données forensic réelles
4. ❌ **NE JAMAIS** inférer sans preuves empiriques

**Violation de ce protocole = Diagnostic invalide**

---

## 📂 LOGS À ANALYSER (Par Ordre de Priorité)

### 1. Agent Decisions (`agent_v27_1_YYYYMMDD_HHMMSS.jsonl`)

**Contenu**: Décisions prises par l'agent à chaque step

**Métriques critiques**:
```json
{
  "component": "exploration" | "reputation",  // Mode décision
  "env_action": "ACTION1-7",  // Action choisie
  "score": 0.0-1.0,  // Score exploration
  "reputation_score": 0.0-1.0,  // Score réputation
  "player_detected": true/false,  // Agent détecté?
  "goal_detected": true/false,  // Goal détecté?
  "path_found": true/false  // Chemin trouvé?
}
```

**Questions à répondre**:
- Quel % de décisions en mode exploration vs réputation?
- Les scores de réputation convergent-ils vers équilibre uniforme?
- L'agent détecte-t-il player/goal?

### 2. Entity Detector (`entity_detector_YYYYMMDD_HHMMSS.jsonl`)

**Contenu**: Détection d'entités comportementales (agent, mobiles, stables)

**Métriques critiques**:
```json
{
  "agent_detected": true/false,  // Agent identifié?
  "agent_confidence": 0.0-1.0,  // Confiance détection
  "causal_events_count": 0-N,  // Événements causaux
  "entities_detected": 0-N,  // Total entités
  "mobile_entities": 0-N,  // Entités mobiles
  "stable_entities": 0-N  // Entités stables
}
```

**Questions à répondre**:
- L'agent est-il détecté? (agent_detected: true/false)
- Quelle est la confiance? (agent_confidence > 0.7 = bon)
- Y a-t-il des événements causaux? (causal_events_count > 0)

### 3. Reward Shaper (`reward_shaper_YYYYMMDD_HHMMSS.jsonl`)

**Contenu**: Calcul des rewards denses (exploration, novelty, progress, etc.)

**Métriques critiques**:
```json
{
  "total_reward": 0.0-1.0,  // Reward total
  "exploration_reward": 0.0-1.0,  // Bonus exploration
  "novelty_reward": 0.0-1.0,  // Bonus nouveauté
  "topology_reward": 0.0-1.0,  // Bonus topologie
  "causality_reward": 0.0-1.0,  // Bonus causalité
  "progress_reward": 0.0-1.0,  // Bonus progression
  "completion_reward": 0.0-1.0,  // Bonus completion
  "agent_pos_before": [x, y] | null,  // Position avant
  "agent_pos_after": [x, y] | null,  // Position après
  "goal_pos": [x, y] | null  // Position goal
}
```

**Questions à répondre**:
- Quels rewards sont actifs? (> 0.0)
- Le reward est-il dirigé? (progress_reward > 0.0)
- L'agent a-t-il une position? (agent_pos != null)

### 4. Action Validator (`action_validator_YYYYMMDD_HHMMSS.jsonl`)

**Contenu**: Validation contextuelle des actions

**Métriques critiques**:
```json
{
  "is_valid": true/false,  // Action valide?
  "confidence": 0.0-1.0,  // Confiance validation
  "reason": "insufficient_data" | "blacklisted" | "valid",
  "validity_score": 0.0-1.0,  // Score validité
  "success_rate": 0.0-1.0,  // Taux succès
  "blacklisted": true/false,  // Action blacklistée?
  "total_attempts": 0-N  // Tentatives totales
}
```

**Questions à répondre**:
- Le success_rate varie-t-il? (variance > 0.3 = bon)
- Des actions sont-elles blacklistées? (blacklisted: true)
- La confiance augmente-t-elle avec l'expérience?

---

## 🔍 PROCÉDURE D'ANALYSE

### Étape 1: Identifier Logs Correspondants

```bash
# Lister logs par date (plus récents en premier)
ls -lht lumvorax2/src/MAGEN/logs_forensic_v27_1/ | head -20

# Identifier timestamp exécution (format: YYYYMMDD_HHMMSS)
# Exemple: 20260614_215448 = 2026-06-14 21:54:48
```

### Étape 2: Lire Logs (50 Premières Lignes)

```bash
# Lire début de chaque log pour comprendre pattern
head -50 lumvorax2/src/MAGEN/logs_forensic_v27_1/agent_v27_1_TIMESTAMP.jsonl
head -50 lumvorax2/src/MAGEN/logs_forensic_v27_1/entity_detector_TIMESTAMP.jsonl
head -50 lumvorax2/src/MAGEN/logs_forensic_v27_1/reward_shaper_TIMESTAMP.jsonl
head -50 lumvorax2/src/MAGEN/logs_forensic_v27_1/action_validator_TIMESTAMP.jsonl
```

### Étape 3: Extraire Métriques Critiques

Pour chaque log, extraire:

**Agent Decisions**:
- % mode exploration vs réputation
- Variance scores réputation
- % player_detected: true

**Entity Detector**:
- % agent_detected: true
- Moyenne agent_confidence
- % causal_events_count > 0

**Reward Shaper**:
- % progress_reward > 0.0
- % agent_pos != null
- Composition reward (novelty, causality, progress)

**Action Validator**:
- Variance success_rate
- % blacklisted: true
- Évolution confidence

### Étape 4: Diagnostiquer Problèmes

Comparer métriques aux seuils critiques:

| Métrique | Seuil Critique | Problème Si |
|----------|----------------|-------------|
| agent_detected | > 80% | < 50% → Agent non détecté |
| agent_confidence | > 0.7 | < 0.5 → Détection incertaine |
| progress_reward > 0 | > 50% | < 20% → Pas de progression |
| success_rate variance | > 0.3 | < 0.1 → Pas de discrimination |
| causal_events_count > 0 | > 30% | < 10% → Pas de causalité |

### Étape 5: Proposer Corrections

Basées sur **données empiriques**, pas inférences:

**Exemple CORRECT**:
> "Les logs montrent agent_detected: false (50/50 observations). Le BehavioralEntityDetector ne détecte jamais l'agent. Correction: implémenter heuristique 'entité avec motion le plus élevé'."

**Exemple INCORRECT**:
> "Le score est 0/10, donc l'agent ne doit pas être détecté. Correction: améliorer détection."

---

## 📊 TEMPLATE RAPPORT ANALYSE

```markdown
# ANALYSE FORENSIC - Session XX

## Logs Analysés
- agent_v27_1_YYYYMMDD_HHMMSS.jsonl (XXX lignes)
- entity_detector_YYYYMMDD_HHMMSS.jsonl (XXX lignes)
- reward_shaper_YYYYMMDD_HHMMSS.jsonl (XXX lignes)
- action_validator_YYYYMMDD_HHMMSS.jsonl (XXX lignes)

## Métriques Critiques

### Agent Detection
- agent_detected: XX% (XX/XX observations)
- agent_confidence: X.XX moyenne
- causal_events_count: XX% > 0

### Reward Composition
- progress_reward > 0: XX% (XX/XX calculs)
- novelty_reward: X.XX moyenne
- causality_reward: X.XX moyenne

### Action Validation
- success_rate variance: X.XX
- blacklisted actions: XX% (XX/XX)
- confidence évolution: X.XX → X.XX

## Problèmes Identifiés

### Problème #1: [Titre]
**Données empiriques**: [Citation logs]
**Impact**: [Description impact]
**Correction**: [Action basée sur données]

### Problème #2: [Titre]
...

## Corrections Proposées

1. [Action 1 basée sur données]
2. [Action 2 basée sur données]
3. [Action 3 basée sur données]
```

---

## ⚠️ ERREURS FRÉQUENTES À ÉVITER

### ❌ Erreur #1: Analyser Uniquement JSON Final

**Problème**: Le JSON final montre symptômes, pas causes racines.

**Exemple**:
- JSON: `"successful_games": 0`
- Logs: `"agent_detected": false` (50/50)

**Solution**: TOUJOURS lire logs forensic en plus du JSON.

### ❌ Erreur #2: Inférer Sans Preuves

**Problème**: Proposer corrections basées sur intuition, pas données.

**Exemple INCORRECT**:
> "Le score est bas, donc il faut augmenter le learning rate."

**Exemple CORRECT**:
> "Les logs montrent success_rate: 1.0 constant (200/200). Le validator ne discrimine pas les actions. Correction: lier success_rate au reward obtenu."

### ❌ Erreur #3: Ignorer Patterns Répétitifs

**Problème**: Ne pas identifier valeurs constantes (signaux de problème).

**Exemple**:
- `agent_detected: false` (50/50) → PROBLÈME
- `success_rate: 1.0` (200/200) → PROBLÈME
- `progress_reward: 0.0` (40/40) → PROBLÈME

**Solution**: Chercher variance = 0 → Indicateur de dysfonctionnement.

### ❌ Erreur #4: Analyser Trop Tard

**Problème**: Lire logs après plusieurs itérations de corrections.

**Solution**: Lire logs IMMÉDIATEMENT après exécution, avant toute correction.

---

## 🎯 CHECKLIST VALIDATION

Avant de proposer toute correction, vérifier:

- [ ] J'ai identifié les logs forensic correspondants (timestamp)
- [ ] J'ai lu les 50 premières lignes de chaque log
- [ ] J'ai extrait les métriques critiques (agent_detected, progress_reward, success_rate)
- [ ] J'ai identifié les valeurs constantes (variance = 0)
- [ ] J'ai comparé aux seuils critiques
- [ ] Mes corrections sont basées sur données empiriques, pas inférences
- [ ] J'ai cité les logs dans mon diagnostic

**Si un seul item n'est pas coché → Diagnostic invalide**

---

## 📚 RÉFÉRENCES

- **Session 72**: Exemple parfait de convergence philosophie ↔ empirisme
  - Analyse philosophique: "Aucune mémoire décisionnelle globale"
  - Logs forensic: `success_rate: 1.0` constant (100% validation)
  - Rapport: `RAPPORT_SYNTHESE_SESSION_72_PHILOSOPHIE_ET_EMPIRISME.md`

- **Logs Forensic**: `lumvorax2/src/MAGEN/logs_forensic_v27_1/`
- **Analyse Forensic**: `ANALYSE_FORENSIC_TEMPS_REEL_SESSION_72.md`

---

**Dernière mise à jour**: 2026-06-14  
**Auteur**: Bob (Assistant IA)  
**Validé par**: Utilisateur (Session 72)