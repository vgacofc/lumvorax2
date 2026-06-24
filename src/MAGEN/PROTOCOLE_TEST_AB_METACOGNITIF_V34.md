# 🧪 PROTOCOLE TEST A/B MÉTACOGNITIF V34

**Date**: 2026-06-15T18:43:00+02:00  
**Auteur**: Bob (LumVorax AI)  
**Version**: V34.0.0  
**Objectif**: Prouver que V34 a un **effet causal** sur la décision, pas juste du logging

---

## 🚨 PROBLÈME CRITIQUE IDENTIFIÉ

### Mon Audit A Validé (INCOMPLET):
- ✅ Structure modules correcte
- ✅ Conformité forensic 80%
- ✅ Tests unitaires passent
- ✅ Logs générés correctement

### Mon Audit N'A PAS Validé (CRITIQUE):
- ❌ **Impact réel sur décisions**
- ❌ **Sortie effective de stagnation**
- ❌ **Modification politique exploration**
- ❌ **Amélioration performance mesurable**

---

## 🔥 HYPOTHÈSE À TESTER

**H0 (Null)**: V34 est un système de **logging intelligent** sans effet causal
**H1 (Alternative)**: V34 est un système **métacognitif actif** qui modifie la politique

---

## 🧪 PROTOCOLE TEST A/B

### Configuration Expérimentale

```python
# TEST A: V34 ACTIVÉ (Métacognition ON)
config_A = {
    "enable_sterility_memory": True,
    "enable_progress_detection": True,
    "enable_disengagement": True,
    "metacognitive_intervention": True  # CRITIQUE
}

# TEST B: V34 DÉSACTIVÉ (Métacognition OFF)
config_B = {
    "enable_sterility_memory": False,
    "enable_progress_detection": False,
    "enable_disengagement": False,
    "metacognitive_intervention": False
}
```

### Puzzles Test (10 puzzles problématiques V32)

Sélectionner puzzles avec **stagnation confirmée V32**:
- WAIT loop > 50% steps
- Score gelé > 100 steps
- Diversité actions < 0.3
- Santé < 0.3 persistante

---

## 📊 MÉTRIQUES CAUSALES (CRITIQUES)

### 1. Métriques Décisionnelles (Prouvent Causalité)

| Métrique | Description | Seuil Significatif |
|----------|-------------|-------------------|
| **policy_entropy** | Entropie distribution actions | Δ > 0.2 |
| **action_diversity** | Ratio actions uniques/total | Δ > 0.15 |
| **exploration_rate** | % actions exploration vs exploitation | Δ > 10% |
| **weight_updates** | Nombre modifications poids actions | > 0 (V34 ON) |

### 2. Métriques Performance (Prouvent Efficacité)

| Métrique | Description | Seuil Significatif |
|----------|-------------|-------------------|
| **stagnation_time** | Steps en stagnation | Réduction > 30% |
| **recovery_speed** | Steps pour sortir stagnation | Réduction > 40% |
| **reward_gain** | Reward cumulé | Augmentation > 15% |
| **puzzle_solved** | Puzzles résolus | Augmentation > 2 |

### 3. Métriques Métacognitives (Prouvent Activité)

| Métrique | Description | Attendu V34 ON |
|----------|-------------|----------------|
| **interventions_count** | Nombre interventions métacognitives | > 5 |
| **disengagement_triggered** | Désengagements effectifs | > 2 |
| **sterile_regions_marked** | Régions marquées stériles | > 10 |
| **policy_mutations** | Modifications politique | > 3 |

---

## 🔬 PROTOCOLE EXÉCUTION

### Phase 1: Baseline V32 (Référence)
```bash
# Exécuter 10 puzzles avec V32 (sans V34)
python test_v32_baseline.py --puzzles problematic_10.json --output baseline_v32.json
```

**Métriques attendues**:
- Stagnation: 70-100% steps
- Diversité: 0.2-0.3
- Score: 0-2/10

---

### Phase 2: Test A (V34 ON)
```bash
# Exécuter 10 puzzles avec V34 ACTIVÉ
python test_v34_metacognitive_on.py \
    --puzzles problematic_10.json \
    --enable-sterility-memory \
    --enable-progress-detection \
    --enable-disengagement \
    --enable-policy-updates \  # CRITIQUE
    --output test_a_v34_on.json
```

**Métriques attendues SI V34 FONCTIONNE**:
- Stagnation: 30-50% steps (réduction 40-60%)
- Diversité: 0.4-0.6 (augmentation 100%)
- Interventions: 5-15
- Score: 3-5/10 (amélioration +50-150%)

---

### Phase 3: Test B (V34 OFF - Contrôle)
```bash
# Exécuter 10 puzzles avec V34 modules présents MAIS désactivés
python test_v34_metacognitive_off.py \
    --puzzles problematic_10.json \
    --disable-sterility-memory \
    --disable-progress-detection \
    --disable-disengagement \
    --disable-policy-updates \  # CRITIQUE
    --output test_b_v34_off.json
```

**Métriques attendues**:
- Identiques à V32 baseline
- Prouve que modules sans activation = pas d'effet

---

## 🔍 ANALYSE CAUSALE

### Test Statistique (Wilcoxon Signed-Rank)

```python
from scipy.stats import wilcoxon

# Comparer métriques A vs B
metrics_a = load_metrics("test_a_v34_on.json")
metrics_b = load_metrics("test_b_v34_off.json")

# Test stagnation_time
stat, p_value = wilcoxon(
    metrics_a['stagnation_time'],
    metrics_b['stagnation_time']
)

# Significatif si p < 0.05
if p_value < 0.05:
    print("✅ V34 a un EFFET CAUSAL significatif")
else:
    print("❌ V34 n'a PAS d'effet causal (logging décoratif)")
```

---

## 🚨 CRITÈRES VALIDATION V34

### ✅ V34 VALIDÉ SI:

1. **Causalité Décisionnelle**:
   - policy_entropy(A) > policy_entropy(B) + 0.2
   - action_diversity(A) > action_diversity(B) + 0.15
   - weight_updates(A) > 0 ET weight_updates(B) = 0

2. **Efficacité Performance**:
   - stagnation_time(A) < stagnation_time(B) × 0.7
   - reward_gain(A) > reward_gain(B) × 1.15
   - p_value < 0.05 (significatif statistiquement)

3. **Activité Métacognitive**:
   - interventions_count(A) > 5
   - disengagement_triggered(A) > 2
   - sterile_regions_marked(A) > 10

### ❌ V34 REJETÉ SI:

1. **Pas de Causalité**:
   - Métriques A ≈ Métriques B (différence < 10%)
   - p_value > 0.05 (pas significatif)

2. **Logging Décoratif**:
   - Interventions loggées MAIS pas de changement policy
   - Stérilité détectée MAIS pas de pénalité appliquée
   - Stagnation détectée MAIS pas de désengagement

---

## 🔧 INSTRUMENTATION REQUISE

### Modifications Code Nécessaires

#### 1. DecisionKernel - Traçage Modifications Policy

```python
class DecisionKernelV34:
    def __init__(self):
        self.policy_updates_count = 0  # NOUVEAU
        self.weight_history = []  # NOUVEAU
        
    def update_policy_from_metacognition(self, metacog_state):
        """CRITIQUE: Appliquer modifications métacognitives"""
        if metacog_state.should_disengage:
            # Modifier RÉELLEMENT les poids
            self._apply_sterility_penalty(metacog_state.sterility_penalty)
            self._reduce_stagnant_actions(metacog_state.stagnation_severity)
            self.policy_updates_count += 1  # TRACER
            
            # Log forensic
            self.forensic.log("policy_updated_by_metacognition", {
                "sterility_penalty": metacog_state.sterility_penalty,
                "stagnation_severity": metacog_state.stagnation_severity,
                "weights_before": self.weight_history[-1],
                "weights_after": self.current_weights
            })
```

#### 2. MetaCognitiveController - Vérification Effet

```python
class MetaCognitiveController:
    def verify_causal_effect(self, decision_kernel):
        """Vérifie que métacognition modifie réellement policy"""
        if self.total_interventions > 0:
            if decision_kernel.policy_updates_count == 0:
                raise RuntimeError(
                    "ERREUR CAUSALE: Interventions métacognitives "
                    "SANS modification policy → Logging décoratif détecté"
                )
```

---

## 📈 RAPPORT ATTENDU

### Structure Rapport Test A/B

```markdown
# RAPPORT TEST A/B MÉTACOGNITIF V34

## Résultats Expérimentaux

### Test A (V34 ON)
- Stagnation: 35% (vs 85% baseline)
- Diversité: 0.52 (vs 0.25 baseline)
- Interventions: 12
- Policy updates: 8
- Score: 4/10 (vs 1/10 baseline)

### Test B (V34 OFF)
- Stagnation: 83% (≈ baseline)
- Diversité: 0.27 (≈ baseline)
- Interventions: 0
- Policy updates: 0
- Score: 1/10 (≈ baseline)

## Analyse Statistique
- Wilcoxon p-value: 0.008 (< 0.05) ✅ SIGNIFICATIF
- Effect size: 0.72 (large)

## Verdict
✅ V34 a un EFFET CAUSAL prouvé
✅ Métacognition modifie réellement la politique
✅ Performance améliorée significativement
```

---

## 🎯 PROCHAINES ÉTAPES

### Si V34 VALIDÉ (effet causal prouvé):
1. ✅ Déployer V34 en production
2. ✅ Documenter LEÇON-75: "Test A/B Métacognitif Obligatoire"
3. ✅ Continuer Phase 2 (intégration DecisionKernel)

### Si V34 REJETÉ (pas d'effet causal):
1. ❌ Identifier boucle causale manquante
2. 🔧 Créer V35 avec couplage direct métacognition→policy
3. 🧪 Re-tester avec protocole A/B

---

## 🔥 LEÇON CRITIQUE

**Citation Utilisateur**:
> "Tu valides 'les modules respectent les règles' mais pas 'les modules ont un effet causal sur le système'"

**Réponse**:
Ce protocole test A/B est la **SEULE façon** de prouver causalité. Sans ça:
- V34 peut être du **logging intelligent** (illusion cognition)
- Pas de **boucle causale fermée** (observation→métacognition→policy→action)
- **Inertie cognitive** (même problème V28, niveau supérieur)

---

**Signature**: Bob (LumVorax AI)  
**Timestamp**: 2026-06-15T18:43:00+02:00  
**Priorité**: 🚨 **CRITIQUE** - Test obligatoire avant validation V34